#include "picobrew/local_store.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

/* The catalog is the commit record. A mutation writes and verifies an unused
 * recipe bank, then writes the other catalog bank. The previous catalog and
 * every recipe it references remain untouched until the new commit verifies.
 * On-disk records have explicit little-endian fields; no C pointers/padding. */
enum { HEADER = 16, INDEX_ENTRY = 26, INDEX_BYTES = HEADER + 16 * INDEX_ENTRY,
       STEP_BYTES = 28, RECIPE_BYTES = HEADER + 21 + 32 + 15 * STEP_BYTES,
       SETTINGS_BYTES = HEADER + 8, COUNTER_BYTES = HEADER + 4,
       RECOVERY_BYTES = HEADER + 36 };
typedef struct Entry { char id[21]; uint8_t bank; uint32_t generation; } Entry;
enum { INDEX_STARTERS_HANDLED = 1 };
typedef struct Index { uint32_t generation; uint8_t bank, flags; Entry entries[16]; } Index;
typedef struct StarterRecipe {
    const char *id, *name;
    uint8_t step_count;
    const uint8_t (*steps)[5];
} StarterRecipe;
#include "local_starter_recipes.inc"
static PicoNetwork *bound_network;
static PicoLocalFileStat file_stat;
static PicoLocalReadFile file_read_block;
static void *stat_context;
static uint32_t selected_recipe_signature;

void pico_local_store_bind(PicoNetwork *n, PicoLocalFileStat stat, void *context)
{
    bound_network = n;
    file_stat = stat;
    stat_context = context;
    file_read_block = NULL;
    selected_recipe_signature = 0;
}
void pico_local_store_set_reader(PicoLocalReadFile reader) { file_read_block = reader; }
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
           (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (i * 8));
}
static unsigned get16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static void put16(uint8_t *p, unsigned v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static uint32_t crc(const uint8_t *p, size_t length, bool header)
{
    uint32_t v = UINT32_MAX;
    for (size_t i = 0; i < length; ++i) {
        if (header && i >= 12 && i < 16) continue;
        v ^= p[i];
        for (unsigned b = 0; b < 8; ++b)
            v = (v >> 1) ^ (0xedb88320u & (0u - (v & 1u)));
    }
    return ~v;
}
static void seal(uint8_t *data, size_t length, const char magic[4], uint32_t generation)
{
    memcpy(data, magic, 4);
    data[4] = 1;
    put32(data + 8, generation);
    put32(data + 12, crc(data, length, true));
}
static bool sealed(const uint8_t *data, size_t length, const char magic[4])
{
    return !memcmp(data, magic, 4) && data[4] == 1 && data[6] == 0 && data[7] == 0 &&
           get32(data + 8) != 0 && get32(data + 12) == crc(data, length, true);
}
static PicoLocalStoreResult read_record(PicoNetwork *n, const char *name,
                                       uint8_t *data, size_t length)
{
    if (n != bound_network || !file_stat || !n || !n->platform)
        return PICO_LOCAL_STORE_IO;
    size_t actual = 0;
    int status = file_stat(stat_context, name, &actual);
    if (status == 1) return PICO_LOCAL_STORE_NOT_FOUND;
    if (status != 0) return PICO_LOCAL_STORE_IO;
    if (actual != length) return PICO_LOCAL_STORE_CORRUPT;
    if (file_read_block) {
        int32_t count = file_read_block(stat_context, name, data, length);
        return count >= 0 && (size_t)count == length ? PICO_LOCAL_STORE_OK : PICO_LOCAL_STORE_IO;
    }
    if (!n->platform->file_open(n->platform_context, name, 0))
        return PICO_LOCAL_STORE_IO;
    bool valid = true;
    for (size_t i = 0; i < length; ++i) {
        int value = n->platform->file_read(n->platform_context);
        if (value < 0) { valid = false; break; }
        data[i] = (uint8_t)value;
    }
    if (valid && n->platform->file_read(n->platform_context) != -1) valid = false;
    bool closed = n->platform->file_close(n->platform_context);
    return valid && closed ? PICO_LOCAL_STORE_OK : PICO_LOCAL_STORE_IO;
}
static bool write_record(PicoNetwork *n, const char *name, const uint8_t *data, size_t length)
{
    if (n != bound_network || !file_stat) return false;
    size_t previous = 0;
    int status = file_stat(stat_context, name, &previous);
    if (status < 0 || status > 1) return false;
    uint32_t mode = status == 1 ? pico_storage_create_mode((uint32_t)length, 0) : 0x1000;
    if (!mode || !n->platform->file_open(n->platform_context, name, mode)) return false;
    int32_t count = n->platform->file_write(n->platform_context, data, length);
    bool closed = n->platform->file_close(n->platform_context);
    if (!closed || count < 0 || (size_t)count != length) return false;
    if (file_read_block) {
        uint8_t verification[RECIPE_BYTES];
        if (length > sizeof verification) return false;
        return read_record(n, name, verification, length) == PICO_LOCAL_STORE_OK &&
               !memcmp(data, verification, length);
    }
    size_t actual = 0;
    if (file_stat(stat_context, name, &actual) != 0 || actual != length ||
        !n->platform->file_open(n->platform_context, name, 0)) return false;
    bool same = true;
    for (size_t i = 0; i < length; ++i) {
        int value = n->platform->file_read(n->platform_context);
        if (value < 0 || (uint8_t)value != data[i]) { same = false; break; }
    }
    if (same && n->platform->file_read(n->platform_context) != -1) same = false;
    closed = n->platform->file_close(n->platform_context);
    return same && closed;
}
bool pico_local_recipe_id_valid(const char *id)
{
    if (!id || !*id) return false;
    for (unsigned i = 0; ; ++i) {
        unsigned c = (unsigned char)id[i];
        if (!c) return i <= 20;
        if (i == 20 || !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
    }
}
static bool label(const char *s, size_t capacity)
{
    if (!s || !*s) return false;
    for (size_t i = 0; i < capacity; ++i) {
        unsigned c = (unsigned char)s[i];
        if (!c) return true;
        if (c < 32 || c > 126) return false;
    }
    return false;
}
bool pico_local_recipe_valid(const PicoLocalRecipe *r)
{
    if (!r || !pico_local_recipe_id_valid(r->id) || !label(r->name, sizeof r->name) ||
        !r->step_count || r->step_count > PICO_RECIPE_STEP_CAPACITY) return false;
    for (unsigned i = 0; i < r->step_count; ++i) {
        const PicoRecipeStep *s = &r->steps[i];
        if (!label(s->name, sizeof s->name) || s->location < 1 || s->location > 6 ||
            (s->target_fahrenheit && (s->target_fahrenheit < 60 || s->target_fahrenheit > 212)) ||
            s->active_minutes > 599 || s->drain_minutes > 120)
            return false;
    }
    return true;
}
static void index_name(char *name, unsigned bank)
{
    memcpy(name, "/local/index.a", 15);
    name[13] = (char)('a' + bank);
}
static void recipe_name(char *name, unsigned slot, unsigned bank)
{
    memcpy(name, "/local/recipe00.a", 18);
    name[13] = (char)('0' + slot / 10);
    name[14] = (char)('0' + slot % 10);
    name[16] = (char)('a' + bank);
}
static bool index_valid(const uint8_t *raw)
{
    if (!sealed(raw, INDEX_BYTES, "PLIX") || (raw[5] & ~INDEX_STARTERS_HANDLED)) return false;
    for (unsigned i = 0; i < 16; ++i) {
        const uint8_t *e = raw + HEADER + i * INDEX_ENTRY;
        if (!e[0]) continue;
        if (!memchr(e, 0, 21) || !pico_local_recipe_id_valid((const char *)e) || e[21] > 1 ||
            !get32(e + 22)) return false;
        for (unsigned j = 0; j < i; ++j)
            if (!strcmp((const char *)e, (const char *)(raw + HEADER + j * INDEX_ENTRY)))
                return false;
    }
    return true;
}
static PicoLocalStoreResult load_index(PicoNetwork *n, Index *index)
{
    uint8_t raw[INDEX_BYTES];
    memset(index, 0, sizeof *index);
    unsigned absent = 0;
    for (unsigned bank = 0; bank < 2; ++bank) {
        char name[24]; index_name(name, bank);
        PicoLocalStoreResult result = read_record(n, name, raw, sizeof raw);
        if (result == PICO_LOCAL_STORE_IO) return result;
        if (result == PICO_LOCAL_STORE_NOT_FOUND) { ++absent; continue; }
        if (result != PICO_LOCAL_STORE_OK || !index_valid(raw)) continue;
        uint32_t generation = get32(raw + 8);
        if (generation <= index->generation) continue;
        index->generation = generation;
        index->bank = (uint8_t)bank;
        index->flags = raw[5];
        for (unsigned i = 0; i < 16; ++i) {
            const uint8_t *e = raw + HEADER + i * INDEX_ENTRY;
            memcpy(index->entries[i].id, e, 21);
            index->entries[i].bank = e[21];
            index->entries[i].generation = get32(e + 22);
        }
    }
    if (!index->generation && absent != 2) return PICO_LOCAL_STORE_CORRUPT;
    return PICO_LOCAL_STORE_OK;
}
static bool save_index(PicoNetwork *n, Index *index)
{
    if (index->generation == UINT32_MAX) return false;
    uint8_t raw[INDEX_BYTES] = {0};
    raw[5] = index->flags;
    for (unsigned i = 0; i < 16; ++i) {
        uint8_t *e = raw + HEADER + i * INDEX_ENTRY;
        memcpy(e, index->entries[i].id, 21);
        e[21] = index->entries[i].bank;
        put32(e + 22, index->entries[i].generation);
    }
    unsigned bank = index->generation ? 1u - index->bank : 0u;
    seal(raw, sizeof raw, "PLIX", index->generation + 1);
    char name[24]; index_name(name, bank);
    return write_record(n, name, raw, sizeof raw);
}
static void encode_recipe(uint8_t raw[RECIPE_BYTES], const PicoLocalRecipe *r, uint32_t generation)
{
    memset(raw, 0, RECIPE_BYTES);
    raw[5] = r->step_count;
    memcpy(raw + HEADER, r->id, 21);
    memcpy(raw + HEADER + 21, r->name, 32);
    for (unsigned i = 0; i < r->step_count; ++i) {
        const PicoRecipeStep *s = &r->steps[i];
        uint8_t *out = raw + HEADER + 53 + i * STEP_BYTES;
        memcpy(out, s->name, 22); out[22] = s->location; out[23] = s->target_fahrenheit;
        put16(out + 24, s->active_minutes); put16(out + 26, s->drain_minutes);
    }
    seal(raw, RECIPE_BYTES, "PLRC", generation);
}
static bool decode_recipe(const uint8_t raw[RECIPE_BYTES], const Entry *entry, PicoLocalRecipe *r)
{
    if (!sealed(raw, RECIPE_BYTES, "PLRC") || get32(raw + 8) != entry->generation ||
        memcmp(raw + HEADER, entry->id, 21)) return false;
    memset(r, 0, sizeof *r);
    memcpy(r->id, raw + HEADER, 21); memcpy(r->name, raw + HEADER + 21, 32);
    r->step_count = raw[5];
    if (r->step_count > 15) return false;
    for (unsigned i = 0; i < r->step_count; ++i) {
        const uint8_t *in = raw + HEADER + 53 + i * STEP_BYTES;
        PicoRecipeStep *s = &r->steps[i];
        memcpy(s->name, in, 22); s->location = in[22]; s->target_fahrenheit = in[23];
        s->active_minutes = get16(in + 24); s->drain_minutes = get16(in + 26);
    }
    return pico_local_recipe_valid(r);
}
static PicoLocalStoreResult read_recipe(PicoNetwork *n, const Index *index, unsigned slot,
                                       PicoLocalRecipe *r)
{
    uint8_t raw[RECIPE_BYTES]; char name[24];
    recipe_name(name, slot, index->entries[slot].bank);
    PicoLocalStoreResult result = read_record(n, name, raw, sizeof raw);
    if (result != PICO_LOCAL_STORE_OK) return result == PICO_LOCAL_STORE_NOT_FOUND ?
        PICO_LOCAL_STORE_CORRUPT : result;
    return decode_recipe(raw, &index->entries[slot], r) ? PICO_LOCAL_STORE_OK :
           PICO_LOCAL_STORE_CORRUPT;
}
unsigned pico_local_starter_recipe_count(void)
{
    return sizeof starter_recipes / sizeof starter_recipes[0];
}
static PicoLocalStoreResult install_starters(PicoNetwork *n, Index *index,
                                             unsigned *added, unsigned *preserved)
{
    unsigned free_slots = 0, matching = 0;
    bool missing[sizeof starter_recipes / sizeof starter_recipes[0]];
    for (unsigned slot = 0; slot < PICO_LOCAL_RECIPE_CAPACITY; ++slot) {
        if (!index->entries[slot].id[0]) { ++free_slots; continue; }
        PicoLocalRecipe stored;
        PicoLocalStoreResult result = read_recipe(n, index, slot, &stored);
        if (result != PICO_LOCAL_STORE_OK) return result;
    }
    for (unsigned i = 0; i < pico_local_starter_recipe_count(); ++i) {
        missing[i] = true;
        for (unsigned slot = 0; slot < PICO_LOCAL_RECIPE_CAPACITY; ++slot)
            if (!strcmp(index->entries[slot].id, starter_recipes[i].id)) {
                missing[i] = false; ++matching; break;
            }
    }
    unsigned needed = pico_local_starter_recipe_count() - matching;
    if (needed > free_slots || (index->generation == UINT32_MAX &&
        (needed || !(index->flags & INDEX_STARTERS_HANDLED)))) return PICO_LOCAL_STORE_FULL;
    /* Establish a verified empty rollback before a first batch. Once this
     * commit exists, an interrupted seed can always retry from empty. */
    if (!index->generation) {
        if (!save_index(n, index)) return PICO_LOCAL_STORE_IO;
        index->generation = 1; index->bank = 0;
    }
    unsigned slot = 0;
    for (unsigned i = 0; i < pico_local_starter_recipe_count(); ++i) {
        if (!missing[i]) continue;
        while (index->entries[slot].id[0]) ++slot;
        PicoLocalRecipe recipe = {0};
        const StarterRecipe *starter = &starter_recipes[i];
        strcpy(recipe.id, starter->id); strcpy(recipe.name, starter->name);
        recipe.step_count = starter->step_count;
        for (unsigned step = 0; step < starter->step_count; ++step) {
            const uint8_t *packed = starter->steps[step];
            PicoRecipeStep *decoded = &recipe.steps[step];
            strcpy(decoded->name, starter_step_names[packed[0]]);
            decoded->location = packed[1]; decoded->target_fahrenheit = packed[2];
            decoded->active_minutes = packed[3]; decoded->drain_minutes = packed[4];
        }
        if (!pico_local_recipe_valid(&recipe)) return PICO_LOCAL_STORE_INVALID;
        Entry *entry = &index->entries[slot];
        memset(entry, 0, sizeof *entry);
        strcpy(entry->id, recipe.id); entry->generation = 1;
        uint8_t raw[RECIPE_BYTES]; char name[24];
        encode_recipe(raw, &recipe, 1); recipe_name(name, slot, 0);
        if (!write_record(n, name, raw, sizeof raw)) return PICO_LOCAL_STORE_IO;
    }
    if (needed || !(index->flags & INDEX_STARTERS_HANDLED)) {
        index->flags |= INDEX_STARTERS_HANDLED;
        if (!save_index(n, index)) return PICO_LOCAL_STORE_IO;
    }
    if (added) *added = needed;
    if (preserved) *preserved = matching;
    return PICO_LOCAL_STORE_OK;
}
PicoLocalStoreResult pico_local_recipes_initialize(PicoNetwork *n)
{
    Index index; PicoLocalStoreResult result = load_index(n, &index);
    if (result != PICO_LOCAL_STORE_OK || (index.flags & INDEX_STARTERS_HANDLED)) return result;
    /* An older firmware's occupied catalogue belongs to the user. Mark the
     * migration handled so deleting it later will not recreate defaults. */
    bool occupied = false;
    for (unsigned i = 0; i < PICO_LOCAL_RECIPE_CAPACITY; ++i) {
        if (!index.entries[i].id[0]) continue;
        occupied = true;
        PicoLocalRecipe recipe;
        result = read_recipe(n, &index, i, &recipe);
        if (result != PICO_LOCAL_STORE_OK) return result;
    }
    if (occupied) {
        index.flags |= INDEX_STARTERS_HANDLED;
        return save_index(n, &index) ? PICO_LOCAL_STORE_OK : PICO_LOCAL_STORE_IO;
    }
    return install_starters(n, &index, NULL, NULL);
}
PicoLocalStoreResult pico_local_recipes_restore_defaults(PicoNetwork *n,
                                                        unsigned *added, unsigned *preserved)
{
    Index index; PicoLocalStoreResult result = load_index(n, &index);
    if (result != PICO_LOCAL_STORE_OK) return result;
    return install_starters(n, &index, added, preserved);
}
PicoLocalStoreResult pico_local_recipe_get(PicoNetwork *n, const char *id, PicoLocalRecipe *r)
{
    if (!pico_local_recipe_id_valid(id) || !r) return PICO_LOCAL_STORE_INVALID;
    Index index; PicoLocalStoreResult result = load_index(n, &index);
    if (result != PICO_LOCAL_STORE_OK) return result;
    for (unsigned i = 0; i < 16; ++i)
        if (!strcmp(index.entries[i].id, id)) return read_recipe(n, &index, i, r);
    return PICO_LOCAL_STORE_NOT_FOUND;
}
PicoLocalStoreResult pico_local_recipe_at(PicoNetwork *n, unsigned ordinal, PicoLocalRecipe *r)
{
    if (!r || ordinal >= 16) return PICO_LOCAL_STORE_NOT_FOUND;
    Index index; PicoLocalStoreResult result = load_index(n, &index);
    if (result != PICO_LOCAL_STORE_OK) return result;
    for (unsigned i = 0; i < 16; ++i)
        if (index.entries[i].id[0] && ordinal-- == 0) return read_recipe(n, &index, i, r);
    return PICO_LOCAL_STORE_NOT_FOUND;
}
PicoLocalStoreResult pico_local_recipe_visit(PicoNetwork *n,
    bool (*visitor)(void *context, const PicoLocalRecipe *recipe), void *context)
{
    if (!visitor) return PICO_LOCAL_STORE_INVALID;
    Index index; PicoLocalStoreResult result = load_index(n, &index);
    if (result != PICO_LOCAL_STORE_OK) return result;
    for (unsigned i = 0; i < 16; ++i) {
        if (!index.entries[i].id[0]) continue;
        PicoLocalRecipe recipe;
        result = read_recipe(n, &index, i, &recipe);
        if (result != PICO_LOCAL_STORE_OK) return result;
        if (!visitor(context, &recipe)) return PICO_LOCAL_STORE_FULL;
    }
    return PICO_LOCAL_STORE_OK;
}
PicoLocalStoreResult pico_local_recipe_put(PicoNetwork *n, const PicoLocalRecipe *r, bool *created)
{
    if (!pico_local_recipe_valid(r)) return PICO_LOCAL_STORE_INVALID;
    Index index; PicoLocalStoreResult result = load_index(n, &index);
    if (result != PICO_LOCAL_STORE_OK) return result;
    int slot = -1, empty = -1;
    for (unsigned i = 0; i < 16; ++i) {
        if (!strcmp(index.entries[i].id, r->id)) slot = (int)i;
        if (!index.entries[i].id[0] && empty < 0) empty = (int)i;
    }
    bool is_new = slot < 0;
    if (is_new) slot = empty;
    if (slot < 0) return PICO_LOCAL_STORE_FULL;
    Entry *e = &index.entries[slot];
    if (e->generation == UINT32_MAX || index.generation == UINT32_MAX)
        return PICO_LOCAL_STORE_FULL;
    e->bank = is_new ? 0 : (uint8_t)(1u - e->bank);
    ++e->generation;
    memcpy(e->id, r->id, sizeof e->id);
    uint8_t raw[RECIPE_BYTES]; char name[24];
    encode_recipe(raw, r, e->generation); recipe_name(name, (unsigned)slot, e->bank);
    if (!write_record(n, name, raw, sizeof raw) || !save_index(n, &index))
        return PICO_LOCAL_STORE_IO;
    if (created) *created = is_new;
    return PICO_LOCAL_STORE_OK;
}
PicoLocalStoreResult pico_local_recipe_delete(PicoNetwork *n, const char *id)
{
    if (!pico_local_recipe_id_valid(id)) return PICO_LOCAL_STORE_INVALID;
    Index index; PicoLocalStoreResult result = load_index(n, &index);
    if (result != PICO_LOCAL_STORE_OK) return result;
    for (unsigned i = 0; i < 16; ++i) {
        if (strcmp(index.entries[i].id, id)) continue;
        memset(&index.entries[i], 0, sizeof index.entries[i]);
        return save_index(n, &index) ? PICO_LOCAL_STORE_OK : PICO_LOCAL_STORE_IO;
    }
    return PICO_LOCAL_STORE_NOT_FOUND;
}
bool pico_local_recipe_load(PicoNetwork *n, const char *id, PicoRecipe *r, char name[32])
{
    PicoLocalRecipe stored;
    if (!r || !name || pico_local_recipe_get(n, id, &stored) != PICO_LOCAL_STORE_OK) return false;
    PicoProgramContext *program = r->program;
    memset(r, 0, sizeof *r); r->program = program;
    memcpy(r->identifier, stored.id, sizeof r->identifier);
    r->step_count = stored.step_count;
    memcpy(r->steps, stored.steps, sizeof r->steps);
    (void)pico_local_recovery_select(r);
    memcpy(name, stored.name, 32);
    return true;
}
bool pico_local_recipe_list(PicoNetwork *n, PicoRecipeListing *list)
{
    if (!list) return false;
    int offset = list->page_offset;
    memset(list, 0, sizeof *list); list->page_offset = offset < 0 ? 0 : offset;
    Index index;
    if (load_index(n, &index) != PICO_LOCAL_STORE_OK) return false;
    for (unsigned i = 0; i < 16; ++i) {
        if (!index.entries[i].id[0]) continue;
        ++list->total_items;
        if (list->total_items <= list->page_offset) continue;
        PicoLocalRecipe r;
        if (read_recipe(n, &index, i, &r) != PICO_LOCAL_STORE_OK) return false;
        int at = list->loaded_items++;
        memcpy(list->identifiers[at], r.id, 21);
        snprintf(list->titles[at], 22, "%.21s", r.name);
        memcpy(list->menu_labels[at], list->titles[at], 22);
    }
    list->menu_item_count = list->loaded_items;
    return true;
}

/* Small two-bank records use their own generation; these are independent of
 * recipe catalog commits. A malformed pair fails closed rather than resetting. */
static PicoLocalStoreResult load_pair(PicoNetwork *n, const char *stem, const char magic[4],
                                     uint8_t *selected, size_t size, unsigned *selected_bank)
{
    uint8_t raw[RECOVERY_BYTES]; uint32_t best = 0; unsigned absent = 0;
    if (size > sizeof raw) return PICO_LOCAL_STORE_INVALID;
    memset(selected, 0, size);
    for (unsigned bank = 0; bank < 2; ++bank) {
        char name[32]; snprintf(name, sizeof name, "/local/%s.%c", stem, 'a' + bank);
        PicoLocalStoreResult result = read_record(n, name, raw, size);
        if (result == PICO_LOCAL_STORE_IO) return result;
        if (result == PICO_LOCAL_STORE_NOT_FOUND) { ++absent; continue; }
        if (result == PICO_LOCAL_STORE_OK && sealed(raw, size, magic) && !raw[5] &&
            get32(raw + 8) > best) {
            best = get32(raw + 8); *selected_bank = bank; memcpy(selected, raw, size);
        }
    }
    if (!best) return absent == 2 ? PICO_LOCAL_STORE_NOT_FOUND : PICO_LOCAL_STORE_CORRUPT;
    return PICO_LOCAL_STORE_OK;
}
static bool save_pair(PicoNetwork *n, const char *stem, const char magic[4], uint8_t *raw,
                      size_t size, uint32_t generation, unsigned previous_bank)
{
    if (generation == UINT32_MAX) return false;
    unsigned bank = generation ? 1u - previous_bank : 0u;
    seal(raw, size, magic, generation + 1);
    char name[32]; snprintf(name, sizeof name, "/local/%s.%c", stem, 'a' + bank);
    return write_record(n, name, raw, size);
}

uint32_t pico_local_recovery_select(const PicoRecipe *r)
{
    /* Every field of a step is initialized by the local decoder/cleaning
     * constructor. This fixed 32-byte target layout has no padding. Include
     * names as well as every actuator parameter, but not mutable progress. */
    _Static_assert(sizeof(PicoRecipeStep) == 32, "recovery step encoding");
    selected_recipe_signature = crc((const uint8_t *)r->steps,
                                    r->step_count * sizeof r->steps[0], false);
    return selected_recipe_signature;
}

PicoLocalStoreResult pico_local_recovery_get(PicoNetwork *n, PicoLocalRecovery *record)
{
    uint8_t raw[RECOVERY_BYTES]; unsigned bank = 0;
    PicoLocalStoreResult result = load_pair(n, "resume", "PLRS", raw, sizeof raw, &bank);
    if (result != PICO_LOCAL_STORE_OK) return result;
    const uint8_t *p = raw + HEADER;
    if (p[0] == 0xff) return PICO_LOCAL_STORE_NOT_FOUND; /* committed tombstone */
    if (p[0] > 1 || p[1] >= PICO_RECIPE_STEP_CAPACITY || p[2] > 5 || p[2] == 4 ||
        p[3] > 1 || get16(p + 4) > 599 || get16(p + 6) > 200 ||
        !memchr(p + 12, 0, 21) || !pico_local_recipe_id_valid((const char *)p + 12))
        return PICO_LOCAL_STORE_CORRUPT;
    memset(record, 0, sizeof *record);
    record->session_type = p[0]; record->step = p[1];
    record->phase = p[2]; record->calibrated = p[3];
    record->active_minutes = (uint16_t)get16(p + 4);
    record->drain_minutes = (uint16_t)get16(p + 6);
    record->recipe_signature = get32(p + 8);
    memcpy(record->recipe_id, p + 12, 21);
    return PICO_LOCAL_STORE_OK;
}

bool pico_local_recovery_save(PicoNetwork *n, const PicoLocalRecovery *record)
{
    if (record && (record->session_type > 1 || record->step >= PICO_RECIPE_STEP_CAPACITY ||
        record->phase > 5 || record->phase == 4 || record->calibrated > 1 ||
        record->active_minutes > 599 || record->drain_minutes > 200 ||
        !memchr(record->recipe_id, 0, sizeof record->recipe_id) ||
        !pico_local_recipe_id_valid(record->recipe_id))) return false;
    uint8_t raw[RECOVERY_BYTES]; unsigned bank = 0;
    PicoLocalStoreResult result = load_pair(n, "resume", "PLRS", raw, sizeof raw, &bank);
    if (result != PICO_LOCAL_STORE_OK && result != PICO_LOCAL_STORE_NOT_FOUND) return false;
    if (!record && (result == PICO_LOCAL_STORE_NOT_FOUND || raw[HEADER] == 0xff)) return true;
    uint32_t generation = get32(raw + 8);
    memset(raw + HEADER, 0, sizeof raw - HEADER);
    uint8_t *p = raw + HEADER;
    p[0] = 0xff;
    if (record) {
        p[0] = record->session_type; p[1] = record->step;
        p[2] = record->phase; p[3] = record->calibrated;
        put16(p + 4, record->active_minutes); put16(p + 6, record->drain_minutes);
        put32(p + 8, selected_recipe_signature);
        memcpy(p + 12, record->recipe_id, 21);
    }
    return save_pair(n, "resume", "PLRS", raw, sizeof raw, generation, bank);
}

bool pico_local_recovery_clear(PicoNetwork *n) { return pico_local_recovery_save(n, NULL); }

bool pico_local_session_new(PicoNetwork *n, char *identifier, size_t capacity)
{
    if (!identifier || capacity < 24) return false;
    uint8_t raw[COUNTER_BYTES]; unsigned bank = 0;
    PicoLocalStoreResult result = load_pair(n, "sessions", "PLSN", raw, sizeof raw, &bank);
    if (result != PICO_LOCAL_STORE_OK && result != PICO_LOCAL_STORE_NOT_FOUND) return false;
    uint32_t generation = get32(raw + 8), sequence = get32(raw + HEADER);
    if (sequence == UINT32_MAX) return false;
    put32(raw + HEADER, ++sequence);
    if (!save_pair(n, "sessions", "PLSN", raw, sizeof raw, generation, bank)) return false;
    snprintf(identifier, capacity, "local-%.8s-%08lx", n->app->device_uid, (unsigned long)sequence);
    pico_local_event(n, "session", identifier);
    return true;
}
bool pico_local_settings_load(PicoNetwork *n)
{
    uint8_t raw[SETTINGS_BYTES]; unsigned bank = 0;
    if (load_pair(n, "settings", "PLST", raw, sizeof raw, &bank) != PICO_LOCAL_STORE_OK)
        return false;
    const uint8_t *s = raw + HEADER;
    if (s[0] > 2 || s[1] > 1 || s[2] > 1 || s[3] > 1 || s[4] > 6) return false;
    n->settings->language = s[0]; n->settings->temperature_unit_celsius = s[1];
    n->settings->detailed_status = s[2]; n->settings->legacy_compartment_mapping = s[3];
    n->settings->home_compartment = s[4];
    return true;
}
bool pico_local_settings_save(PicoNetwork *n)
{
    const PicoSettings *s = n->settings;
    if (s->language > 2 || s->temperature_unit_celsius > 1 || s->detailed_status > 1 ||
        s->legacy_compartment_mapping > 1 || s->home_compartment > 6) return false;
    uint8_t raw[SETTINGS_BYTES]; unsigned bank = 0;
    PicoLocalStoreResult result = load_pair(n, "settings", "PLST", raw, sizeof raw, &bank);
    if (result != PICO_LOCAL_STORE_OK && result != PICO_LOCAL_STORE_NOT_FOUND) return false;
    uint32_t generation = get32(raw + 8);
    raw[HEADER] = s->language; raw[HEADER + 1] = s->temperature_unit_celsius;
    raw[HEADER + 2] = s->detailed_status; raw[HEADER + 3] = s->legacy_compartment_mapping;
    raw[HEADER + 4] = s->home_compartment;
    return save_pair(n, "settings", "PLST", raw, sizeof raw, generation, bank);
}
