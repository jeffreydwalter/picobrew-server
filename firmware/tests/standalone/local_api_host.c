/* Host-only integration boundary. The real JSON dispatcher and two-bank store
 * run unchanged; flash, clock and already-tested appliance controls are fakes. */
#include "picobrew/local_api.h"
#include "picobrew/local_control.h"
#include "picobrew/local_history.h"
#include "picobrew/local_telemetry.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { FILES = 128, FILE_BYTES = 3584 };
typedef struct File { char name[40]; unsigned length; uint8_t data[FILE_BYTES]; } File;
static File files[FILES];
static PicoApplication appliance;
static PicoLocalApi api;
static int opened = -1;
static unsigned offset;
static bool writing, busy, stopped, ready, retain_tail;
static int fail_write_at, fail_write_after, fail_stat_at, fail_read_at;
static unsigned writes, stats, reads, control_calls, encoder_calls;
static int last_delta;
static bool last_click;
static char last_command[32], last_recipe[21];
static uint32_t now_ms = 123456;

static int find(const char *name)
{
    for (unsigned i = 0; i < FILES; ++i)
        if (files[i].name[0] && !strcmp(files[i].name, name)) return (int)i;
    return -1;
}
static uint32_t clock_ms(void *context) { (void)context; return now_ms; }
static int stat_file(void *context, const char *name, size_t *size)
{
    (void)context;
    if (fail_stat_at >= 0 && (int)stats++ == fail_stat_at) return -2;
    int at = find(name); if (at < 0) return 1;
    *size = files[at].length; return 0;
}
static bool open_file(void *context, const char *name, uint32_t mode)
{
    (void)context;
    if (opened >= 0) return false;
    int at = find(name);
    if (at < 0 && (mode & 0x3000) == 0x3000) {
        for (unsigned i = 0; i < FILES; ++i) if (!files[i].name[0]) {
            at = (int)i; snprintf(files[i].name, sizeof files[i].name, "%s", name); break;
        }
    }
    if (at < 0) return false;
    opened = at; offset = 0; writing = mode != 0; return true;
}
static int read_byte(void *context)
{
    (void)context;
    if (opened < 0 || writing || offset == files[opened].length) return -1;
    return files[opened].data[offset++];
}
static int32_t read_block(void *context, const char *name, uint8_t *data, size_t length)
{
    (void)context;
    if (fail_read_at >= 0 && (int)reads++ == fail_read_at) return -2;
    int at = find(name); if (at < 0) return -11;
    if (length > files[at].length) length = files[at].length;
    memcpy(data, files[at].data, length); return (int32_t)length;
}
static int32_t write_file(void *context, const uint8_t *data, size_t length)
{
    (void)context;
    if (opened < 0 || !writing || length > FILE_BYTES) return -1;
    bool fail = (int)writes++ == fail_write_at;
    size_t count = length;
    if (fail && (size_t)fail_write_after < count) count = (size_t)fail_write_after;
    memcpy(files[opened].data, data, count);
    if (!retain_tail || count > files[opened].length) files[opened].length = (unsigned)count;
    return fail ? (int32_t)count : (int32_t)length;
}
static bool close_file(void *context) { (void)context; bool result = opened >= 0; opened = -1; return result; }
static int pin_read(void *context, uint8_t pin) { (void)context; (void)pin; return 0; }
static void mask(void *context) { (void)context; }
static bool unmask(void *context) { (void)context; return false; }
static const PicoNetworkPlatform network_platform = {
    .millis = clock_ms, .file_open = open_file, .file_read = read_byte,
    .file_write = write_file, .file_close = close_file
};
static const PicoApplicationHardware hardware = {
    .millis = clock_ms, .digital_read = pin_read,
    .interrupts_disable = mask, .interrupts_enable = unmask
};
uint32_t pico_storage_create_mode(uint32_t size, uint32_t flags)
{
    (void)flags; return size && size <= FILE_BYTES ? 0x3000 : 0;
}
PicoLocalControlResult pico_local_control_command(const char *command, const char *recipe)
{
    static const char *names[] = {"brew", "sous-vide", "cold-brew", "manual", "rinse", "first-rinse",
        "descale", "circulate", "drain-reservoir", "drain-filter", "rack-beer", "utilities", "settings"};
    if (!ready) return PICO_LOCAL_CONTROL_NOT_READY;
    if (!strcmp(command, "stop")) stopped = true;
    else if (!strcmp(command, "clear-stop")) { if (busy) return PICO_LOCAL_CONTROL_BUSY; stopped = false; }
    else {
        bool valid = false;
        for (unsigned i = 0; i < sizeof names / sizeof names[0]; ++i)
            if (!strcmp(command, names[i])) valid = true;
        if (!valid) return PICO_LOCAL_CONTROL_INVALID;
        if (stopped) return PICO_LOCAL_CONTROL_STOPPED;
        if (busy) return PICO_LOCAL_CONTROL_BUSY;
        busy = true;
    }
    ++control_calls; snprintf(last_command, sizeof last_command, "%s", command);
    snprintf(last_recipe, sizeof last_recipe, "%s", recipe ? recipe : "");
    return PICO_LOCAL_CONTROL_OK;
}
PicoLocalControlResult pico_local_control_encoder(int delta, bool click)
{
    if (!ready) return PICO_LOCAL_CONTROL_NOT_READY;
    ++encoder_calls; last_delta = delta; last_click = click; return PICO_LOCAL_CONTROL_OK;
}
bool pico_local_control_busy(void) { return busy; }
bool pico_local_control_stopped(void) { return stopped; }
const char *pico_local_control_stop_reason(void) { return stopped ? "requested" : "none"; }
bool pico_local_control_is_ready(void) { return ready; }
bool pico_local_control_foreground(void) { return true; }
const char *pico_local_control_command_name(void) { return last_command; }
static bool history_ready(void *context) { (void)context; return true; }
static int history_replace(void *context, const char *name, const uint8_t *data, size_t length)
{
    (void)context;
    if (!length || length > FILE_BYTES) return -1;
    int at = find(name);
    if (at < 0) {
        for (unsigned i = 0; i < FILES; ++i) if (!files[i].name[0]) {
            at = (int)i; snprintf(files[i].name, sizeof files[i].name, "%s", name); break;
        }
    }
    if (at < 0) return -1;
    memcpy(files[at].data, data, length); files[at].length = (unsigned)length;
    return 0;
}
static const PicoHistoryStorage history_storage = {history_ready, stat_file, read_block, history_replace};
const char *pico_http_method_name(PicoHttpMethod method)
{
    static const char *names[] = {"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"};
    return method <= PICO_HTTP_OPTIONS ? names[method] : "";
}

void test_reset(int clear_files)
{
    if (clear_files) memset(files, 0, sizeof files);
    memset(&appliance, 0, sizeof appliance);
    appliance.hardware = &hardware;
    appliance.network.app = &appliance.state;
    appliance.network.settings = &appliance.settings;
    appliance.network.platform = &network_platform;
    appliance.settings.home_compartment = 6;
    appliance.state.recipe.program = &appliance.state.session;
    strcpy(appliance.state.device_uid, "0123456789abcdef0123456789abcdef");
    appliance.state.controller.wort_fahrenheit = 72.4;
    appliance.state.controller.thermoblock_fahrenheit = 90.5;
    appliance.state.controller.wort_target_fahrenheit = 152;
    appliance.state.controller.step_seconds = 61;
    appliance.ui.encoder.maximum = 5;
    for (unsigned i = 0; i < 1024; ++i) appliance.ui.display.framebuffer[i] = (uint8_t)i;
    opened = -1; busy = stopped = retain_tail = false; ready = true;
    fail_write_at = fail_stat_at = fail_read_at = -1; fail_write_after = 0;
    writes = stats = reads = control_calls = encoder_calls = 0;
    last_command[0] = last_recipe[0] = 0;
    now_ms = 123456;
    pico_local_api_initialize(&api, &appliance);
    pico_local_store_bind(&appliance.network, stat_file, NULL);
    pico_local_store_set_reader(read_block);
    pico_history_bind(&appliance, &history_storage, NULL);
    pico_telemetry_bind(&appliance, &history_storage, NULL, NULL);
}
int test_history_start(const char *name)
{
    snprintf(appliance.state.screen.recipe_name, sizeof appliance.state.screen.recipe_name, "%s", name);
    appliance.state.controller.phase = PICO_BREW_PHASE_HEATING;
    return pico_history_start("brew");
}
void test_history_advance(unsigned milliseconds, unsigned phase)
{
    now_ms += milliseconds;
    appliance.state.controller.phase = (uint8_t)phase;
    pico_history_poll();
}
void test_history_finish(int interrupted) { pico_history_finish(interrupted != 0); }
int test_request(const char *method, const char *path, const char *body, size_t length,
                 char *out, size_t capacity)
{
    char arena[4096];
    if (length >= sizeof arena) return 413;
    if (length) memcpy(arena, body, length);
    arena[length] = 0;
    size_t count;
    int status = pico_local_api_handle(&api, method, path, arena, length,
                                       arena, sizeof arena, &count);
    if (count + 1 > capacity) return -1;
    memcpy(out, arena, count); out[count] = 0;
    return status;
}
void test_busy(int value) { busy = value != 0; }
void test_ready(int value) { ready = value != 0; }
void test_write_failure(int nth, int count) { fail_write_at = nth; fail_write_after = count; writes = 0; }
void test_retain_tail(int value) { retain_tail = value != 0; }
void test_stat_failure(int nth) { fail_stat_at = nth; stats = 0; }
void test_read_failure(int nth) { fail_read_at = nth; reads = 0; }
int test_corrupt(const char *name, unsigned at)
{
    int file = find(name); if (file < 0 || at >= files[file].length) return 0;
    files[file].data[at] ^= 0x80; return 1;
}
int test_file_count(void)
{
    int count = 0; for (unsigned i = 0; i < FILES; ++i) if (files[i].name[0]) ++count; return count;
}
unsigned test_control_calls(void) { return control_calls; }
unsigned test_write_count(void) { return writes; }
int test_initialize_recipes(void) { return pico_local_recipes_initialize(&appliance.network); }
int test_save_recovery(unsigned active, unsigned drain, unsigned phase)
{
    PicoLocalRecovery record = {.step = 0, .active_minutes = (uint16_t)active,
        .drain_minutes = (uint16_t)drain, .phase = (uint8_t)phase};
    strcpy(record.recipe_id, appliance.state.recipe.identifier);
    return pico_local_recovery_save(&appliance.network, &record);
}
int test_read_recovery(unsigned *active, unsigned *drain, unsigned *phase)
{
    PicoLocalRecovery record;
    int result = pico_local_recovery_get(&appliance.network, &record);
    if (!result) { *active = record.active_minutes; *drain = record.drain_minutes; *phase = record.phase; }
    return result;
}
int test_clear_recovery(void) { return pico_local_recovery_clear(&appliance.network); }
unsigned test_encoder_calls(void) { return encoder_calls; }
int test_last_delta(void) { return last_delta; }
int test_last_click(void) { return last_click; }
int test_session(char *out, size_t capacity) { return pico_local_session_new(&appliance.network, out, capacity); }
int test_load_settings(void) { return pico_local_settings_load(&appliance.network); }
int test_load_recipe(const char *id)
{
    return pico_local_recipe_load(&appliance.network, id, &appliance.state.recipe,
                                   appliance.state.screen.recipe_name);
}
int test_recipe_list(void)
{
    PicoRecipeListing list = {0};
    return pico_local_recipe_list(&appliance.network, &list) ? list.loaded_items : -1;
}
void test_event(const char *kind, const char *detail) { pico_local_event(&appliance.network, kind, detail); }

#ifdef LOCAL_API_SELFTEST_MAIN
int main(void)
{
    static const char recipe[] = "{\"name\":\"Test\",\"steps\":[{\"name\":\"Mash\",\"location\":1,\"target_f\":152,\"minutes\":60,\"drain_minutes\":2}]}";
    char result[4096], body[4096];
    test_reset(1);
    assert(test_request("PUT", "/api/v1/recipes/test", recipe, strlen(recipe), result, sizeof result) == 201);
    for (size_t length = 0; length < strlen(recipe); ++length)
        assert(test_request("PUT", "/api/v1/recipes/broken", recipe, length, result, sizeof result) == 400);
    for (unsigned iteration = 0; iteration < 5000; ++iteration) {
        size_t length = (size_t)(rand() % 4095);
        for (size_t i = 0; i < length; ++i) body[i] = (char)(rand() & 255);
        test_request("PUT", "/api/v1/recipes/fuzz", body, length, result, sizeof result);
        test_request("POST", "/api/v1/commands", body, length, result, sizeof result);
        test_request("PATCH", "/api/v1/settings", body, length, result, sizeof result);
    }
    /* Structured mutations reach fields deep in the grammar, in addition to
     * the arbitrary byte sequences above. All calls alias input and output. */
    static const char *seeds[] = {recipe,
        "{\"command\":\"encoder\",\"delta\":1,\"click\":true}",
        "{\"temperature_unit\":\"F\",\"language\":\"en\",\"detailed_status\":false}"};
    static const char *methods[] = {"PUT", "POST", "PATCH"};
    static const char *paths[] = {"/api/v1/recipes/fuzz", "/api/v1/commands", "/api/v1/settings"};
    for (unsigned iteration = 0; iteration < 2000; ++iteration) {
        for (unsigned endpoint = 0; endpoint < 3; ++endpoint) {
            size_t length = strlen(seeds[endpoint]); memcpy(body, seeds[endpoint], length);
            unsigned changes = 1 + (unsigned)rand() % 8;
            for (unsigned i = 0; i < changes; ++i)
                body[(unsigned)rand() % length] = (char)(rand() & 255);
            test_busy(0);
            test_request(methods[endpoint], paths[endpoint], body, length, result, sizeof result);
        }
    }
    puts("local API ASan/UBSan: truncations and 21000 random/structured input trials passed");
    return 0;
}
#endif
