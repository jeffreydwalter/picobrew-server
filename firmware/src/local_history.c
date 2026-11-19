#include "picobrew/local_history.h"
#include "picobrew/local_control.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 24 data files +2 checkpoint tails +2 catalogs, each one4 KiB allocation.
 * A checkpoint writes/verifies the unused tail, then the other catalog.
 * Reclaiming old histories commits their removal BEFORE any chunk is reused.
 * Every referenced chunk carries its history id, ordinal and CRC; loading an
 * older catalog cannot resurrect a history whose physical slot was reused.
 * All filesystem calls are foreground and leave interrupts enabled. */
enum { RECORD_BYTES = 20, CHUNK_HEADER = 32,
       CHUNK_BYTES = PICO_HISTORY_BUFFER_BYTES,
       META_BYTES = 1024, ENTRY_BYTES = 96, SLOTS_OFFSET = 32 + 8 * ENTRY_BYTES,
       FLAG_TRUNCATED = 1, FLAG_RECOVERED = 2 };
typedef struct Entry {
    uint32_t id, duration_ms, start_ms, records, gaps;
    uint8_t state, flags, chunks;
    char command[16], recipe_name[32];
} Entry;
typedef struct Slot { uint32_t id; uint16_t count; uint8_t ordinal; } Slot;
typedef struct Catalog {
    uint32_t generation, next_id, active_id, tail_generation, tail_crc;
    uint16_t tail_count;
    uint8_t tail_bank;
    Entry entries[PICO_HISTORY_CAPACITY];
    Slot slots[PICO_HISTORY_CHUNKS];
} Catalog;
static struct {
    PicoApplication *application;
    const PicoHistoryStorage *storage;
    void *context;
    Catalog catalog;
    uint8_t *tail;
    PicoHistorySample last;
    uint32_t last_checkpoint_ms, persisted_active_count;
    unsigned meta_bank;
    bool loaded, storage_error, have_last;
} history;

static bool ensure_tail(void)
{
    if (!history.tail) history.tail = malloc(CHUNK_BYTES);
    if (history.tail) return true;
    history.storage_error = true;
    return false;
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static unsigned get16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static void put32(uint8_t *p, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (i * 8));
}
static void put16(uint8_t *p, unsigned v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static uint32_t crc_update(uint32_t crc, const uint8_t *data, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return crc;
}
static uint32_t chunk_crc(const uint8_t *raw, unsigned count)
{
    return ~crc_update(crc_update(UINT32_MAX, raw, 28), raw + CHUNK_HEADER,
                       (size_t)count * RECORD_BYTES);
}
static Entry *entry(Catalog *catalog, uint32_t id)
{
    if (id) for (unsigned i = 0; i < PICO_HISTORY_CAPACITY; ++i)
        if (catalog->entries[i].id == id) return &catalog->entries[i];
    return NULL;
}
static unsigned slot_for(const Catalog *catalog, uint32_t id, unsigned ordinal)
{
    for (unsigned i = 0; i < PICO_HISTORY_CHUNKS; ++i)
        if (catalog->slots[i].id == id && catalog->slots[i].ordinal == ordinal) return i;
    return PICO_HISTORY_CHUNKS;
}
static void path(char *name, const char *kind, unsigned bank)
{
    if (kind) snprintf(name, 32, "/local/history.%s.%c", kind, bank ? 'b' : 'a');
    else snprintf(name, 32, "/local/history%02u", bank);
}
static bool text_valid(const uint8_t *data, size_t capacity)
{
    for (size_t i = 0; i < capacity; ++i) {
        if (!data[i]) return true;
        if (data[i] < 32 || data[i] > 126) return false;
    }
    return false;
}
static void copy_label(char *destination, size_t capacity, const char *source)
{
    size_t i = 0;
    if (source) for (; i + 1 < capacity && source[i]; ++i) {
        unsigned char ch = (unsigned char)source[i];
        destination[i] = ch >= 32 && ch <= 126 ? (char)ch : '?';
    }
    memset(destination + i, 0, capacity - i);
}
static void encode_meta(uint8_t *raw, const Catalog *c)
{
    memset(raw, 0, META_BYTES); memcpy(raw, "PHM2", 4); raw[4] = 2;
    raw[5] = c->tail_bank; put16(raw + 6, c->tail_count);
    put32(raw + 8, c->generation); put32(raw + 12, c->next_id);
    put32(raw + 16, c->active_id); put32(raw + 20, c->tail_generation);
    put32(raw + 24, c->tail_crc);
    for (unsigned i = 0; i < PICO_HISTORY_CAPACITY; ++i) {
        const Entry *e = &c->entries[i]; uint8_t *p = raw + 32 + i * ENTRY_BYTES;
        put32(p, e->id); put32(p + 4, e->duration_ms); put32(p + 8, e->start_ms);
        put32(p + 12, e->records); put32(p + 16, e->gaps);
        p[20] = e->state; p[21] = e->flags; p[22] = e->chunks;
        memcpy(p + 24, e->command, sizeof e->command);
        memcpy(p + 40, e->recipe_name, sizeof e->recipe_name);
    }
    for (unsigned i = 0; i < PICO_HISTORY_CHUNKS; ++i) {
        const Slot *s = &c->slots[i]; uint8_t *p = raw + SLOTS_OFFSET + i * 8;
        put32(p, s->id); p[4] = s->ordinal; put16(p + 6, s->count);
    }
    put32(raw + META_BYTES - 4, ~crc_update(UINT32_MAX, raw, META_BYTES - 4));
}
static bool decode_meta(const uint8_t *raw, Catalog *c)
{
    if (memcmp(raw, "PHM2", 4) || raw[4] != 2 || raw[5] > 1 ||
        get16(raw + 6) > PICO_HISTORY_CHUNK_RECORDS || !get32(raw + 8) ||
        get32(raw + META_BYTES - 4) != ~crc_update(UINT32_MAX, raw, META_BYTES - 4)) return false;
    memset(c, 0, sizeof *c); c->tail_bank = raw[5]; c->tail_count = (uint16_t)get16(raw + 6);
    c->generation = get32(raw + 8); c->next_id = get32(raw + 12);
    c->active_id = get32(raw + 16); c->tail_generation = get32(raw + 20); c->tail_crc = get32(raw + 24);
    for (unsigned i = 0; i < PICO_HISTORY_CAPACITY; ++i) {
        Entry *e = &c->entries[i]; const uint8_t *p = raw + 32 + i * ENTRY_BYTES;
        e->id = get32(p); e->duration_ms = get32(p + 4); e->start_ms = get32(p + 8);
        e->records = get32(p + 12); e->gaps = get32(p + 16);
        e->state = p[20]; e->flags = p[21]; e->chunks = p[22];
        memcpy(e->command, p + 24, sizeof e->command); memcpy(e->recipe_name, p + 40, sizeof e->recipe_name);
        if (!e->id) continue;
        if (e->id > c->next_id || e->state < PICO_HISTORY_RECORDING ||
            e->state > PICO_HISTORY_INTERRUPTED || e->flags & ~3u ||
            e->chunks > PICO_HISTORY_CHUNKS || e->records > PICO_HISTORY_RECORD_CAPACITY ||
            !text_valid(p + 24, 16) || !text_valid(p + 40, 32) ||
            (e->state == PICO_HISTORY_RECORDING) != (e->id == c->active_id)) return false;
        for (unsigned j = 0; j < i; ++j) if (c->entries[j].id == e->id) return false;
    }
    if ((c->active_id && !entry(c, c->active_id)) || (!c->active_id && c->tail_count)) return false;
    for (unsigned i = 0; i < PICO_HISTORY_CHUNKS; ++i) {
        Slot *s = &c->slots[i]; const uint8_t *p = raw + SLOTS_OFFSET + i * 8;
        s->id = get32(p); s->ordinal = p[4]; s->count = (uint16_t)get16(p + 6);
        if (!s->id) continue;
        Entry *e = entry(c, s->id);
        if (!e || s->ordinal >= e->chunks || !s->count || s->count > PICO_HISTORY_CHUNK_RECORDS ||
            (s->ordinal + 1u < e->chunks && s->count != PICO_HISTORY_CHUNK_RECORDS)) return false;
        for (unsigned j = 0; j < i; ++j)
            if (c->slots[j].id == s->id && c->slots[j].ordinal == s->ordinal) return false;
    }
    for (unsigned i = 0; i < PICO_HISTORY_CAPACITY; ++i) {
        Entry *e = &c->entries[i]; if (!e->id) continue;
        uint32_t records = e->id == c->active_id ? c->tail_count : 0;
        for (unsigned ordinal = 0; ordinal < e->chunks; ++ordinal) {
            unsigned slot = slot_for(c, e->id, ordinal);
            if (slot == PICO_HISTORY_CHUNKS) return false;
            records += c->slots[slot].count;
        }
        if (records != e->records) return false;
    }
    return true;
}
static void encode_sample(uint8_t *raw, const PicoHistorySample *s)
{
    put32(raw, s->elapsed_ms); put16(raw + 4, (uint16_t)s->wort_tenths_f);
    put16(raw + 6, (uint16_t)s->thermoblock_tenths_f); put16(raw + 8, (uint16_t)s->target_tenths_f);
    put32(raw + 10, (uint32_t)s->step_seconds); raw[14] = s->phase; raw[15] = s->controller_status;
    raw[16] = s->compartment; raw[17] = s->current_step; raw[18] = s->flags; raw[19] = s->device_state;
}
static void decode_sample(const uint8_t *raw, PicoHistorySample *s)
{
    s->elapsed_ms = get32(raw); s->wort_tenths_f = (int16_t)get16(raw + 4);
    s->thermoblock_tenths_f = (int16_t)get16(raw + 6); s->target_tenths_f = (int16_t)get16(raw + 8);
    s->step_seconds = (int32_t)get32(raw + 10); s->phase = raw[14]; s->controller_status = raw[15];
    s->compartment = raw[16]; s->current_step = raw[17]; s->flags = raw[18]; s->device_state = raw[19];
}
static void encode_chunk(uint8_t *raw, uint32_t id, unsigned ordinal, unsigned count, uint32_t generation)
{
    memset(raw, 0, CHUNK_HEADER); memcpy(raw, "PHC2", 4); raw[4] = 2; put16(raw + 6, count);
    put32(raw + 8, id); put32(raw + 12, ordinal); put32(raw + 16, generation);
    put32(raw + 28, chunk_crc(raw, count));
}
static bool valid_chunk(const uint8_t *raw, uint32_t id, unsigned ordinal, unsigned count,
                         uint32_t generation, uint32_t expected_crc)
{
    return !memcmp(raw, "PHC2", 4) && raw[4] == 2 && !raw[5] && get16(raw + 6) == count &&
        get32(raw + 8) == id && get32(raw + 12) == ordinal && get32(raw + 16) != 0 &&
        (!generation || get32(raw + 16) == generation) &&
        (!expected_crc || get32(raw + 28) == expected_crc) && get32(raw + 28) == chunk_crc(raw, count);
}
static int read_exact(const char *name, uint8_t *raw, size_t length)
{
    size_t actual = 0; int found = history.storage->stat(history.context, name, &actual);
    if (found != 0) return found == 1 ? 1 : -1;
    if (actual != length) return -1;
    int32_t got = history.storage->read(history.context, name, raw, length);
    return got >= 0 && (size_t)got == length ? 0 : -1;
}
static bool write_verified(const char *name, const uint8_t *data, size_t length)
{
    uint8_t verification[CHUNK_BYTES];
    if (length > sizeof verification || history.storage->replace(history.context, name, data, length) ||
        read_exact(name, verification, length) || memcmp(data, verification, length)) {
        history.storage_error = true; return false;
    }
    return true;
}
static bool can_io(void)
{
    return history.application && history.application->hardware && history.application->hardware->millis &&
        history.storage && history.storage->ready && history.storage->stat && history.storage->read &&
        history.storage->replace && pico_local_control_foreground() && history.storage->ready(history.context);
}
static bool commit(Catalog *next)
{
    if (history.storage_error || !can_io() || history.catalog.generation == UINT32_MAX) {
        history.storage_error = true; return false;
    }
    next->generation = history.catalog.generation + 1;
    uint8_t raw[META_BYTES]; char name[32]; encode_meta(raw, next); path(name, "meta", !history.meta_bank);
    if (!write_verified(name, raw, sizeof raw)) return false;
    history.catalog = *next; history.meta_bank = !history.meta_bank;
    Entry *active = entry(&history.catalog, history.catalog.active_id);
    history.persisted_active_count = active ? active->records : 0;
    return true;
}
static bool checkpoint(uint32_t now)
{
    Catalog next = history.catalog; Entry *e = entry(&next, next.active_id);
    if (!e || history.storage_error || !can_io() || next.generation == UINT32_MAX) return false;
    if (next.tail_count) {
        next.tail_bank = !next.tail_bank; next.tail_generation = next.generation + 1;
        encode_chunk(history.tail, e->id, e->chunks, next.tail_count, next.tail_generation);
        next.tail_crc = get32(history.tail + 28); char name[32]; path(name, "tail", next.tail_bank);
        if (!write_verified(name, history.tail, CHUNK_HEADER + (size_t)next.tail_count * RECORD_BYTES)) return false;
    }
    if (!commit(&next)) return false;
    history.last_checkpoint_ms = now; return true;
}
static bool evict_oldest(void)
{
    Catalog next = history.catalog; Entry *oldest = NULL;
    for (unsigned i = 0; i < PICO_HISTORY_CAPACITY; ++i) {
        Entry *e = &next.entries[i];
        if (e->id && e->id != next.active_id && e->state != PICO_HISTORY_RECORDING &&
            (!oldest || e->id < oldest->id)) oldest = e;
    }
    if (!oldest) return false;
    uint32_t id = oldest->id; memset(oldest, 0, sizeof *oldest);
    for (unsigned i = 0; i < PICO_HISTORY_CHUNKS; ++i)
        if (next.slots[i].id == id) memset(&next.slots[i], 0, sizeof next.slots[i]);
    return commit(&next);
}
static unsigned free_slot(void)
{
    for (unsigned i = 0; i < PICO_HISTORY_CHUNKS; ++i)
        if (!history.catalog.slots[i].id) return i;
    return PICO_HISTORY_CHUNKS;
}
static bool ensure_slot(uint32_t now)
{
    if (free_slot() < PICO_HISTORY_CHUNKS) return true;
    /* Commit the RAM tail before the catalog eviction transaction. */
    Entry *active = entry(&history.catalog, history.catalog.active_id);
    if (active && active->records != history.persisted_active_count && !checkpoint(now)) return false;
    return evict_oldest() && free_slot() < PICO_HISTORY_CHUNKS;
}
static bool seal_tail(uint32_t now, PicoHistoryState state)
{
    if (history.storage_error || !can_io()) return false;
    if (history.catalog.tail_count && !ensure_slot(now)) return false;
    Catalog next = history.catalog; Entry *e = entry(&next, next.active_id);
    if (!e || next.generation == UINT32_MAX) return false;
    if (next.tail_count) {
        unsigned slot = free_slot(); char name[32]; path(name, NULL, slot);
        encode_chunk(history.tail, e->id, e->chunks, next.tail_count, next.generation + 1);
        if (!write_verified(name, history.tail, CHUNK_HEADER + (size_t)next.tail_count * RECORD_BYTES)) return false;
        next.slots[slot] = (Slot){e->id, next.tail_count, e->chunks}; ++e->chunks;
        next.tail_count = 0; next.tail_generation = 0; next.tail_crc = 0;
    }
    e->state = (uint8_t)state;
    if (state != PICO_HISTORY_RECORDING) next.active_id = 0;
    if (!commit(&next)) return false;
    history.last_checkpoint_ms = now; return true;
}
static bool catalog_files(const Catalog *c)
{
    uint8_t raw[CHUNK_BYTES]; char name[32];
    for (unsigned i = 0; i < PICO_HISTORY_CHUNKS; ++i) {
        const Slot *s = &c->slots[i]; if (!s->id) continue;
        path(name, NULL, i); size_t length = CHUNK_HEADER + (size_t)s->count * RECORD_BYTES;
        if (read_exact(name, raw, length) || !valid_chunk(raw, s->id, s->ordinal, s->count, 0, 0)) return false;
    }
    if (c->tail_count) {
        if (!ensure_tail()) return false;
        const Entry *e = NULL;
        for (unsigned i = 0; i < PICO_HISTORY_CAPACITY; ++i)
            if (c->entries[i].id == c->active_id) e = &c->entries[i];
        if (!e) return false;
        path(name, "tail", c->tail_bank); size_t length = CHUNK_HEADER + (size_t)c->tail_count * RECORD_BYTES;
        if (read_exact(name, raw, length) || !valid_chunk(raw, e->id, e->chunks,
                c->tail_count, c->tail_generation, c->tail_crc)) return false;
        memcpy(history.tail, raw, length);
    }
    return true;
}
static bool load(void)
{
    if (history.loaded) return true;
    if (!can_io()) return false;
    Catalog candidate; uint32_t generations[2] = {0, 0}; bool absent = true;
    uint8_t raw[META_BYTES]; char name[32];
    for (unsigned bank = 0; bank < 2; ++bank) {
        path(name, "meta", bank); int result = read_exact(name, raw, sizeof raw);
        if (result != 1) absent = false;
        if (!result && decode_meta(raw, &candidate)) generations[bank] = candidate.generation;
    }
    history.loaded = true;
    if (absent) return true;
    unsigned first = generations[1] > generations[0];
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        unsigned bank = first ^ attempt; if (!generations[bank]) continue;
        path(name, "meta", bank);
        if (read_exact(name, raw, sizeof raw) || !decode_meta(raw, &candidate) || !catalog_files(&candidate)) continue;
        history.catalog = candidate; history.meta_bank = bank;
        Entry *active = entry(&history.catalog, history.catalog.active_id);
        if (active) {
            history.persisted_active_count = active->records;
            active->flags |= FLAG_RECOVERED;
            /* Reboot never resumes a program; archive exactly the last durable
             * checkpoint. The uncheckpointed RAM tail is explicitly lost. */
            if (!seal_tail(history.application->hardware->millis(history.application->hardware_context),
                           PICO_HISTORY_INTERRUPTED)) {
                active = entry(&history.catalog, history.catalog.active_id);
                if (active) active->state = PICO_HISTORY_INTERRUPTED;
                history.storage_error = true;
            }
        }
        return true;
    }
    history.storage_error = true; return true;
}
static int16_t temperature(double value, bool ready)
{
    if (!ready || !(value >= -3276.7 && value <= 3276.7)) return INT16_MIN;
    return (int16_t)(value * 10.0 + (value < 0 ? -0.5 : 0.5));
}
static void snapshot(PicoHistorySample *s, uint32_t now, const Entry *e)
{
    PicoApplication *a = history.application; PicoBrewController *c = &a->state.controller;
    double wort, thermoblock, target; int32_t step;
    if (a->hardware->interrupts_disable) a->hardware->interrupts_disable(a->hardware_context);
    wort = c->wort_fahrenheit; thermoblock = c->thermoblock_fahrenheit; target = c->wort_target_fahrenheit;
    s->phase = c->phase; s->controller_status = c->status; s->compartment = c->compartment;
    s->step_seconds = c->step_seconds; step = a->state.recipe.current_step; s->device_state = a->state.device_state;
    if (a->hardware->interrupts_enable) a->hardware->interrupts_enable(a->hardware_context);
    bool ready = pico_local_control_is_ready(); s->elapsed_ms = now - e->start_ms;
    s->wort_tenths_f = temperature(wort, ready); s->thermoblock_tenths_f = temperature(thermoblock, ready);
    s->target_tenths_f = temperature(target, ready); s->current_step = step >= 0 && step <= 254 ? (uint8_t)step : 255;
    s->flags = (ready ? PICO_HISTORY_READY : 0) | (pico_local_control_busy() ? PICO_HISTORY_BUSY : 0) |
        (pico_local_control_stopped() ? PICO_HISTORY_STOPPED : 0);
    if (ready && a->hardware->digital_read && !a->hardware->digital_read(a->hardware_context, PICO_PIN_STEP_FILTER))
        s->flags |= PICO_HISTORY_FILTER_INSERTED;
}
static void collect(uint32_t now, bool force)
{
    Entry *e = entry(&history.catalog, history.catalog.active_id); if (!e) return;
    PicoHistorySample current; snapshot(&current, now, e); e->duration_ms = current.elapsed_ms;
    if (e->flags & FLAG_TRUNCATED) return;
    bool changed = !history.have_last || current.phase != history.last.phase ||
        current.current_step != history.last.current_step || current.device_state != history.last.device_state ||
        current.controller_status != history.last.controller_status || current.compartment != history.last.compartment ||
        current.target_tenths_f != history.last.target_tenths_f ||
        ((current.flags ^ history.last.flags) & (PICO_HISTORY_READY | PICO_HISTORY_BUSY |
                                                PICO_HISTORY_STOPPED | PICO_HISTORY_FILTER_INSERTED));
    if (!force && !changed && current.elapsed_ms - history.last.elapsed_ms < PICO_HISTORY_SAMPLE_MS) return;
    if (e->records >= PICO_HISTORY_RECORD_CAPACITY || history.catalog.tail_count >= PICO_HISTORY_CHUNK_RECORDS) {
        e->flags |= FLAG_TRUNCATED; return;
    }
    if (!history.catalog.tail_count && !ensure_slot(now)) {
        e = entry(&history.catalog, history.catalog.active_id);
        if (e) e->flags |= FLAG_TRUNCATED;
        return;
    }
    e = entry(&history.catalog, history.catalog.active_id);
    if (history.have_last && current.elapsed_ms - history.last.elapsed_ms > 2u * PICO_HISTORY_SAMPLE_MS) {
        current.flags |= PICO_HISTORY_SAMPLING_GAP; ++e->gaps;
    }
    encode_sample(history.tail + CHUNK_HEADER + (size_t)history.catalog.tail_count * RECORD_BYTES, &current);
    ++history.catalog.tail_count; ++e->records; history.last = current; history.have_last = true;
    if (history.catalog.tail_count == PICO_HISTORY_CHUNK_RECORDS) seal_tail(now, PICO_HISTORY_RECORDING);
}

void pico_history_bind(PicoApplication *application, const PicoHistoryStorage *storage, void *context)
{
    free(history.tail);
    memset(&history, 0, sizeof history); history.application = application;
    history.storage = storage; history.context = context;
}
bool pico_history_start(const char *command)
{
    if (!command || (strcmp(command, "brew") && strcmp(command, "manual") &&
        strcmp(command, "sous-vide") && strcmp(command, "cold-brew")) ||
        !load() || history.storage_error || !can_io()) return false;
    Entry *active = entry(&history.catalog, history.catalog.active_id);
    if (active && active->state == PICO_HISTORY_RECORDING) return true;
    /* Reserve the single reusable RAM tail before eviction or any new-brew
     * catalog write. Allocation failure must leave older histories intact. */
    if (!ensure_tail()) return false;
    if (history.catalog.next_id == UINT32_MAX || history.catalog.generation == UINT32_MAX) {
        history.storage_error = true; return false;
    }
    unsigned index;
    for (index = 0; index < PICO_HISTORY_CAPACITY; ++index) if (!history.catalog.entries[index].id) break;
    if (index == PICO_HISTORY_CAPACITY) {
        if (!evict_oldest()) return false;
        for (index = 0; index < PICO_HISTORY_CAPACITY; ++index) if (!history.catalog.entries[index].id) break;
    }
    uint32_t now = history.application->hardware->millis(history.application->hardware_context);
    if (!ensure_slot(now)) return false;
    Catalog next = history.catalog; Entry *e = &next.entries[index]; memset(e, 0, sizeof *e);
    e->id = ++next.next_id; e->start_ms = now; e->state = PICO_HISTORY_RECORDING;
    copy_label(e->command, sizeof e->command, command);
    copy_label(e->recipe_name, sizeof e->recipe_name, history.application->state.screen.recipe_name);
    next.active_id = e->id; next.tail_count = 0; next.tail_generation = 0; next.tail_crc = 0;
    if (!commit(&next)) return false;
    history.have_last = false; history.last_checkpoint_ms = now;
    collect(now, true); checkpoint(now); return !history.storage_error;
}
void pico_history_finish(bool interrupted)
{
    if (!load() || !can_io()) return;
    Entry *e = entry(&history.catalog, history.catalog.active_id);
    if (!e || e->state != PICO_HISTORY_RECORDING) return;
    uint32_t now = history.application->hardware->millis(history.application->hardware_context);
    collect(now, true);
    if (!seal_tail(now, interrupted ? PICO_HISTORY_INTERRUPTED : PICO_HISTORY_COMPLETED)) {
        e = entry(&history.catalog, history.catalog.active_id);
        if (e) e->state = interrupted ? PICO_HISTORY_INTERRUPTED : PICO_HISTORY_COMPLETED;
    }
}
void pico_history_poll(void)
{
    if (!load() || !can_io()) return;
    Entry *e = entry(&history.catalog, history.catalog.active_id);
    if (!e || e->state != PICO_HISTORY_RECORDING) return;
    uint32_t now = history.application->hardware->millis(history.application->hardware_context);
    const char *name = history.application->state.screen.recipe_name;
    if (*name) copy_label(e->recipe_name, sizeof e->recipe_name, name);
    collect(now, false);
    if (!history.storage_error && now - history.last_checkpoint_ms >= PICO_HISTORY_CHECKPOINT_MS) checkpoint(now);
}
static void metadata(const Entry *e, PicoHistoryMetadata *m)
{
    memset(m, 0, sizeof *m); m->storage_error = history.storage_error;
    if (!e) return;
    m->id = e->id; m->duration_ms = e->duration_ms; m->record_count = e->records;
    m->persisted_record_count = e->id == history.catalog.active_id ? history.persisted_active_count : e->records;
    m->gap_count = e->gaps; m->state = (PicoHistoryState)e->state; m->available = true;
    m->truncated = (e->flags & FLAG_TRUNCATED) != 0; m->recovered_after_restart = (e->flags & FLAG_RECOVERED) != 0;
    memcpy(m->command, e->command, sizeof m->command); memcpy(m->recipe_name, e->recipe_name, sizeof m->recipe_name);
}
size_t pico_history_count(void)
{
    load(); size_t count = 0;
    for (unsigned i = 0; i < PICO_HISTORY_CAPACITY; ++i) if (history.catalog.entries[i].id) ++count;
    return count;
}
bool pico_history_at(size_t ordinal, PicoHistoryMetadata *m)
{
    load(); if (!m) return false;
    uint32_t ceiling = UINT32_MAX; Entry *found = NULL;
    for (size_t n = 0; n <= ordinal && n < PICO_HISTORY_CAPACITY; ++n) {
        found = NULL;
        for (unsigned i = 0; i < PICO_HISTORY_CAPACITY; ++i) {
            Entry *e = &history.catalog.entries[i];
            if (e->id && e->id <= ceiling && (!found || e->id > found->id)) found = e;
        }
        if (!found) break;
        ceiling = found->id - 1;
    }
    if (ordinal >= PICO_HISTORY_CAPACITY) found = NULL;
    metadata(found, m); return found != NULL;
}
void pico_history_metadata(PicoHistoryMetadata *m) { if (m) pico_history_at(0, m); }
bool pico_history_find(uint32_t id, PicoHistoryMetadata *m)
{
    load(); if (!m) return false;
    Entry *e = entry(&history.catalog, id); metadata(e, m); return e != NULL;
}
int pico_history_read(uint32_t id, uint32_t offset, PicoHistorySample *samples, size_t capacity, size_t *count)
{
    if (count) *count = 0;
    if (!load()) return 1;
    Entry *e = entry(&history.catalog, id); if (!e) return 1;
    if (!samples || !count || !capacity || capacity > PICO_HISTORY_PAGE_RECORDS || offset > e->records) return 2;
    size_t wanted = e->records - offset; if (wanted > capacity) wanted = capacity;
    uint8_t raw[CHUNK_BYTES]; unsigned loaded_slot = UINT_MAX; char name[32];
    for (size_t i = 0; i < wanted; ++i) {
        unsigned position = offset + (unsigned)i;
        unsigned ordinal = position / PICO_HISTORY_CHUNK_RECORDS, index = position % PICO_HISTORY_CHUNK_RECORDS;
        const uint8_t *data;
        if (id == history.catalog.active_id && ordinal == e->chunks) data = history.tail;
        else {
            unsigned slot = slot_for(&history.catalog, id, ordinal);
            if (slot == PICO_HISTORY_CHUNKS || !can_io()) return -1;
            const Slot *s = &history.catalog.slots[slot];
            if (index >= s->count) return -1;
            if (slot != loaded_slot) {
                path(name, NULL, slot); size_t length = CHUNK_HEADER + (size_t)s->count * RECORD_BYTES;
                if (read_exact(name, raw, length) || !valid_chunk(raw, id, ordinal, s->count, 0, 0)) {
                    history.storage_error = true; return -1;
                }
                loaded_slot = slot;
            }
            data = raw;
        }
        decode_sample(data + CHUNK_HEADER + index * RECORD_BYTES, &samples[i]);
    }
    *count = wanted; return 0;
}
