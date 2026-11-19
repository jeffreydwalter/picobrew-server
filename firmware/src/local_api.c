#include "picobrew/local_api.h"
#include "picobrew/local_control.h"
#include "picobrew/local_history.h"
#include "picobrew/local_telemetry.h"
#include "picobrew/version.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Deliberately small JSON grammar: explicit schemas, no allocations, no
 * recursive descent over arbitrary input, duplicate/unknown keys rejected.
 * Labels are printable ASCII because the appliance's built-in font is ASCII.
 */
typedef struct Json { const char *p, *end; bool ok; } Json;
typedef struct Writer { char *data; size_t length, capacity; bool ok; } Writer;
static PicoLocalApi *active_api;

static void whitespace(Json *j)
{
    while (j->p < j->end && (*j->p == ' ' || *j->p == '\t' || *j->p == '\r' || *j->p == '\n'))
        ++j->p;
}
static bool token(Json *j, char value)
{
    whitespace(j);
    if (!j->ok || j->p == j->end || *j->p != value) { j->ok = false; return false; }
    ++j->p; return true;
}
static bool peek(Json *j, char value)
{
    whitespace(j); return j->ok && j->p < j->end && *j->p == value;
}
static int hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static bool string(Json *j, char *out, size_t capacity)
{
    size_t count = 0;
    if (!out || !capacity || !token(j, '"')) return false;
    while (j->p < j->end) {
        unsigned c = (unsigned char)*j->p++;
        if (c == '"') { out[count] = 0; return true; }
        if (c == '\\') {
            if (j->p == j->end) break;
            c = (unsigned char)*j->p++;
            if (c == 'u') {
                if (j->end - j->p < 4) break;
                unsigned value = 0;
                for (unsigned k = 0; k < 4; ++k) {
                    int digit = hex(*j->p++);
                    if (digit < 0) { j->ok = false; return false; }
                    value = value * 16 + (unsigned)digit;
                }
                c = value;
            } else if (c != '"' && c != '\\' && c != '/') break;
        }
        if (c < 32 || c > 126 || count + 1 >= capacity) break;
        out[count++] = (char)c;
    }
    j->ok = false; return false;
}
static bool integer(Json *j, int32_t minimum, int32_t maximum, int32_t *value)
{
    whitespace(j);
    if (!j->ok || j->p == j->end) return j->ok = false;
    bool negative = *j->p == '-';
    if (negative) ++j->p;
    if (j->p == j->end || *j->p < '0' || *j->p > '9') return j->ok = false;
    bool zero = *j->p == '0';
    uint32_t v = 0; unsigned digits = 0;
    while (j->p < j->end && *j->p >= '0' && *j->p <= '9') {
        unsigned digit = (unsigned)(*j->p++ - '0');
        if (++digits > 10 || (zero && digits > 1) || v > (UINT32_MAX - digit) / 10)
            return j->ok = false;
        v = v * 10 + digit;
    }
    if (v > (negative ? 2147483648u : 2147483647u)) return j->ok = false;
    int32_t signed_value = negative ? (v == 2147483648u ? INT32_MIN : -(int32_t)v) : (int32_t)v;
    if (signed_value < minimum || signed_value > maximum) return j->ok = false;
    *value = signed_value; return true;
}
static bool boolean(Json *j, bool *value)
{
    whitespace(j);
    if (j->end - j->p >= 4 && !memcmp(j->p, "true", 4)) {
        j->p += 4; *value = true; return true;
    }
    if (j->end - j->p >= 5 && !memcmp(j->p, "false", 5)) {
        j->p += 5; *value = false; return true;
    }
    return j->ok = false;
}
static bool done(Json *j) { whitespace(j); return j->ok && j->p == j->end; }
static bool key(Json *j, char name[32]) { return string(j, name, 32) && token(j, ':'); }
static bool comma_or_end(Json *j, char end)
{
    if (peek(j, end)) return false;
    return token(j, ',');
}
static void append(Writer *w, const char *s, size_t length)
{
    if (!w->ok || length >= w->capacity - w->length) { w->ok = false; return; }
    memcpy(w->data + w->length, s, length); w->length += length; w->data[w->length] = 0;
}
static void text(Writer *w, const char *s) { append(w, s, strlen(s)); }
static void format(Writer *w, const char *pattern, ...)
{
    if (!w->ok) return;
    va_list args; va_start(args, pattern);
    int n = vsnprintf(w->data + w->length, w->capacity - w->length, pattern, args);
    va_end(args);
    if (n < 0 || (size_t)n >= w->capacity - w->length) w->ok = false;
    else w->length += (size_t)n;
}
static void quote(Writer *w, const char *s)
{
    static const char digits[] = "0123456789abcdef";
    text(w, "\"");
    for (; s && *s; ++s) {
        unsigned c = (unsigned char)*s;
        if (c == '"' || c == '\\') { char escaped[] = {'\\', (char)c}; append(w, escaped, 2); }
        else if (c < 32 || c > 126) {
            char escaped[] = {'\\', 'u', '0', '0', digits[c >> 4], digits[c & 15]};
            append(w, escaped, sizeof escaped);
        } else { char byte = (char)c; append(w, &byte, 1); }
    }
    text(w, "\"");
}
static void number(Writer *w, double v)
{
    if (!(v > -1000000.0 && v < 1000000.0)) { text(w, "null"); return; }
    bool negative = v < 0; if (negative) v = -v;
    unsigned long tenths = (unsigned long)(v * 10.0 + 0.5);
    format(w, "%s%lu.%lu", negative ? "-" : "", tenths / 10, tenths % 10);
}
static int error(Writer *w, int status, const char *code, const char *message)
{
    w->length = 0; w->ok = w->capacity != 0;
    if (w->capacity) w->data[0] = 0;
    text(w, "{\"error\":"); quote(w, code); text(w, ",\"message\":"); quote(w, message); text(w, "}");
    return status;
}
static int storage_error(Writer *w, PicoLocalStoreResult result)
{
    switch (result) {
    case PICO_LOCAL_STORE_NOT_FOUND: return error(w, 404, "not_found", "Recipe not found");
    case PICO_LOCAL_STORE_INVALID: return error(w, 400, "invalid_recipe", "Invalid recipe");
    case PICO_LOCAL_STORE_FULL: return error(w, 507, "recipe_capacity", "Local recipe storage is full");
    case PICO_LOCAL_STORE_CORRUPT: return error(w, 503, "storage_corrupt", "Local records failed integrity checks; preserved for recovery");
    default: return error(w, 503, "storage_io", "Local storage failed; read the resource before retrying a write");
    }
}

void pico_local_api_initialize(PicoLocalApi *api, PicoApplication *application)
{
    memset(api, 0, sizeof *api); api->application = application; api->next_sequence = 1;
    active_api = api;
}
void pico_local_event(PicoNetwork *n, const char *kind, const char *detail)
{
    PicoLocalApi *api = active_api;
    if (!api || !api->application || n != &api->application->network) return;
    PicoLocalEvent *event = &api->events[api->event_head];
    event->sequence = api->next_sequence++;
    if (!api->next_sequence) api->next_sequence = 1;
    event->uptime_ms = n->platform->millis(n->platform_context);
    snprintf(event->kind, sizeof event->kind, "%s", kind ? kind : "event");
    snprintf(event->detail, sizeof event->detail, "%s", detail ? detail : "");
    api->event_head = (uint8_t)((api->event_head + 1) % PICO_LOCAL_EVENT_CAPACITY);
    if (api->event_count < PICO_LOCAL_EVENT_CAPACITY) ++api->event_count;
}
static int health(PicoLocalApi *api, Writer *w)
{
    PicoNetwork *n = &api->application->network;
    format(w, "{\"api_version\":1,\"firmware_version\":\"" PICO_STANDALONE_VERSION
              "\",\"mode\":\"standalone\",\"ready\":%s,\"auth_configured\":%s,\"uptime_ms\":%lu}",
           pico_local_control_is_ready() ? "true" : "false", api->auth_configured ? "true" : "false",
           (unsigned long)n->platform->millis(n->platform_context));
    return 200;
}
static int discovery(Writer *w)
{
    text(w, "{\"api_version\":1,\"firmware_version\":\"" PICO_STANDALONE_VERSION
            "\",\"mode\":\"standalone\",\"recipe_capacity\":16,\"step_capacity\":15,"
            "\"zero_minute_steps\":true,\"restore_default_recipes\":true,"
            "\"commands\":[\"brew\",\"sous-vide\",\"cold-brew\",\"manual\",\"rinse\",\"first-rinse\","
            "\"descale\",\"circulate\",\"drain-reservoir\",\"drain-filter\",\"rack-beer\",\"utilities\","
            "\"settings\",\"stop\",\"clear-stop\",\"encoder\"],\"dedicated_pause_resume\":false,"
            "\"front_panel\":true,\"events_persistent\":false,\"history_persistent\":true,"
            "\"history_capacity_records\":4224,\"history_capacity_sessions\":8,"
            "\"telemetry_forwarding\":true,"
            "\"recipe_labels\":\"printable ASCII\",");
    format(w, "\"starter_recipe_count\":%u}", pico_local_starter_recipe_count());
    return 200;
}
static int status(PicoLocalApi *api, Writer *w)
{
    PicoApplication *a = api->application;
    PicoNetwork *n = &a->network;
    double wort, thermoblock, target;
    int32_t seconds; uint8_t phase, controller_status, compartment;
    /* All callers are foreground, outside masked-interrupt sections. Only the
     * snapshot copy is protected; formatting and network I/O run unmasked. */
    if (a->hardware && a->hardware->interrupts_disable)
        a->hardware->interrupts_disable(a->hardware_context);
    wort = a->state.controller.wort_fahrenheit;
    thermoblock = a->state.controller.thermoblock_fahrenheit;
    target = a->state.controller.wort_target_fahrenheit;
    seconds = a->state.controller.step_seconds;
    phase = a->state.controller.phase; controller_status = a->state.controller.status;
    compartment = a->state.controller.compartment;
    if (a->hardware && a->hardware->interrupts_enable)
        a->hardware->interrupts_enable(a->hardware_context);
    bool ready = pico_local_control_is_ready();
    format(w, "{\"uptime_ms\":%lu,\"ready\":%s,\"device_uid\":",
           (unsigned long)n->platform->millis(n->platform_context), ready ? "true" : "false");
    quote(w, a->state.device_uid);
    format(w, ",\"busy\":%s,\"stopped\":%s,\"stop_reason\":", pico_local_control_busy() ? "true" : "false",
           pico_local_control_stopped() ? "true" : "false");
    quote(w, pico_local_control_stop_reason()); text(w, ",\"command\":");
    quote(w, pico_local_control_command_name());
    format(w, ",\"device_state\":%u,\"phase\":%u,\"controller_status\":%u,\"compartment\":%u,\"wort_f\":",
           a->state.device_state, phase, controller_status, compartment);
    if (ready) number(w, wort); else text(w, "null");
    text(w, ",\"thermoblock_f\":"); if (ready) number(w, thermoblock); else text(w, "null");
    text(w, ",\"target_f\":"); if (ready) number(w, target); else text(w, "null");
    format(w, ",\"step_seconds\":%ld,\"current_step\":%ld,\"step_count\":%u,\"recipe_name\":",
           (long)seconds, (long)a->state.recipe.current_step, a->state.recipe.step_count);
    quote(w, a->state.screen.recipe_name); text(w, ",\"step_name\":"); quote(w, a->state.screen.step_name);
    text(w, ",\"session_id\":"); quote(w, a->state.session.identifier);
    text(w, ",\"filter_inserted\":");
    if (ready && a->hardware && a->hardware->digital_read)
        text(w, a->hardware->digital_read(a->hardware_context, PICO_PIN_STEP_FILTER) ? "false" : "true");
    else text(w, "null");
    format(w, ",\"encoder\":{\"position\":%d,\"min\":%d,\"max\":%d}}",
           a->ui.encoder.position, a->ui.encoder.minimum, a->ui.encoder.maximum);
    return 200;
}
static int display(PicoLocalApi *api, Writer *w)
{
    static const char hexes[] = "0123456789abcdef";
    text(w, "{\"width\":128,\"height\":64,\"format\":\"page-lsb-hex\",\"data\":\"");
    for (unsigned i = 0; i < PICO_FRAMEBUFFER_SIZE; ++i) {
        unsigned v = api->application->ui.display.framebuffer[i];
        char chars[2] = {hexes[v >> 4], hexes[v & 15]}; append(w, chars, 2);
    }
    text(w, "\"}"); return 200;
}
static bool parse_step(Json *j, PicoRecipeStep *step)
{
    if (!token(j, '{')) return false;
    unsigned seen = 0;
    if (peek(j, '}')) return false;
    do {
        char field[32]; int32_t value;
        if (!key(j, field)) return false;
        unsigned bit;
        if (!strcmp(field, "name")) {
            bit = 1; if (!string(j, step->name, sizeof step->name)) return false;
        } else if (!strcmp(field, "location")) {
            bit = 2; if (!integer(j, 1, 6, &value)) return false; step->location = (uint8_t)value;
        } else if (!strcmp(field, "target_f")) {
            bit = 4; if (!integer(j, 0, 212, &value) || (value && value < 60)) return false;
            step->target_fahrenheit = (uint8_t)value;
        } else if (!strcmp(field, "minutes")) {
            bit = 8; if (!integer(j, 0, 599, &value)) return false; step->active_minutes = (uint32_t)value;
        } else if (!strcmp(field, "drain_minutes")) {
            bit = 16; if (!integer(j, 0, 120, &value)) return false; step->drain_minutes = (uint32_t)value;
        } else return false;
        if (seen & bit) return false;
        seen |= bit;
    } while (comma_or_end(j, '}'));
    return token(j, '}') && (seen & 15) == 15;
}
static bool parse_recipe(const char *body, size_t length, const char *id, PicoLocalRecipe *r)
{
    Json j = {body, body + length, true}; unsigned seen = 0;
    memset(r, 0, sizeof *r); snprintf(r->id, sizeof r->id, "%s", id);
    if (!token(&j, '{') || peek(&j, '}')) return false;
    do {
        char field[32]; unsigned bit;
        if (!key(&j, field)) return false;
        if (!strcmp(field, "name")) {
            bit = 1; if (!string(&j, r->name, sizeof r->name)) return false;
        } else if (!strcmp(field, "steps")) {
            bit = 2; if (!token(&j, '[') || peek(&j, ']')) return false;
            do {
                if (r->step_count == 15 || !parse_step(&j, &r->steps[r->step_count++])) return false;
            } while (comma_or_end(&j, ']'));
            if (!token(&j, ']')) return false;
        } else if (!strcmp(field, "id")) {
            char supplied[21]; bit = 4;
            if (!string(&j, supplied, sizeof supplied) || strcmp(supplied, id)) return false;
        } else return false;
        if (seen & bit) return false;
        seen |= bit;
    } while (comma_or_end(&j, '}'));
    return token(&j, '}') && done(&j) && (seen & 3) == 3 && pico_local_recipe_valid(r);
}
static void recipe_json(Writer *w, const PicoLocalRecipe *r)
{
    text(w, "{\"id\":"); quote(w, r->id); text(w, ",\"name\":"); quote(w, r->name);
    text(w, ",\"steps\":[");
    for (unsigned i = 0; i < r->step_count; ++i) {
        const PicoRecipeStep *s = &r->steps[i];
        if (i) text(w, ",");
        text(w, "{\"name\":"); quote(w, s->name);
        format(w, ",\"location\":%u,\"target_f\":%u,\"minutes\":%lu,\"drain_minutes\":%lu}",
               s->location, s->target_fahrenheit, (unsigned long)s->active_minutes,
               (unsigned long)s->drain_minutes);
    }
    text(w, "]}");
}
typedef struct Listing { Writer *writer; unsigned count; } Listing;
static bool list_recipe(void *context, const PicoLocalRecipe *r)
{
    Listing *list = context; Writer *w = list->writer;
    if (list->count++) text(w, ",");
    text(w, "{\"id\":"); quote(w, r->id); text(w, ",\"name\":"); quote(w, r->name);
    format(w, ",\"steps\":%u}", r->step_count);
    return w->ok;
}
static int recipes(PicoLocalApi *api, Writer *w, const char *method, const char *id,
                   const char *body, size_t length)
{
    PicoNetwork *n = &api->application->network;
    if (!id) {
        if (strcmp(method, "GET")) return error(w, 405, "method_not_allowed", "Use GET for the recipe collection");
        text(w, "{\"recipes\":["); Listing list = {w, 0};
        PicoLocalStoreResult result = pico_local_recipe_visit(n, list_recipe, &list);
        if (result != PICO_LOCAL_STORE_OK) return storage_error(w, result);
        text(w, "],\"capacity\":16}"); return 200;
    }
    if (!pico_local_recipe_id_valid(id)) return error(w, 400, "invalid_id", "Recipe IDs use 1-20 letters, digits, hyphens or underscores");
    PicoLocalRecipe recipe;
    if (!strcmp(method, "GET")) {
        PicoLocalStoreResult result = pico_local_recipe_get(n, id, &recipe);
        if (result != PICO_LOCAL_STORE_OK) return storage_error(w, result);
        recipe_json(w, &recipe); return 200;
    }
    if (strcmp(method, "PUT") && strcmp(method, "DELETE"))
        return error(w, 405, "method_not_allowed", "Use GET, PUT or DELETE for a recipe");
    if (pico_local_control_busy()) return error(w, 409, "busy", "Edit recipes only while the appliance is idle");
    if (!strcmp(method, "DELETE")) {
        if (length) return error(w, 400, "unexpected_body", "DELETE takes no body");
        PicoLocalStoreResult result = pico_local_recipe_delete(n, id);
        if (result != PICO_LOCAL_STORE_OK) return storage_error(w, result);
        pico_local_event(n, "recipe_deleted", id); return 204;
    }
    if (!parse_recipe(body, length, id, &recipe))
        return error(w, 400, "invalid_recipe", "Use a printable ASCII name and 1-15 valid recipe steps");
    bool created = false;
    PicoLocalStoreResult result = pico_local_recipe_put(n, &recipe, &created);
    if (result != PICO_LOCAL_STORE_OK) return storage_error(w, result);
    pico_local_event(n, "recipe_saved", id); recipe_json(w, &recipe); return created ? 201 : 200;
}
static int restore_recipes(PicoLocalApi *api, Writer *w, size_t length)
{
    if (length) return error(w, 400, "unexpected_body", "Restoring starter recipes takes no body");
    if (pico_local_control_busy()) return error(w, 409, "busy", "Edit recipes only while the appliance is idle");
    unsigned added = 0, preserved = 0;
    PicoLocalStoreResult result = pico_local_recipes_restore_defaults(&api->application->network,
                                                                    &added, &preserved);
    if (result != PICO_LOCAL_STORE_OK) return storage_error(w, result);
    if (added) pico_local_event(&api->application->network, "recipes_restored", "Added missing starter recipes");
    format(w, "{\"added\":%u,\"preserved\":%u,\"capacity\":16}", added, preserved);
    return 200;
}
static int command(PicoLocalApi *api, Writer *w, const char *body, size_t length)
{
    Json j = {body, body + length, true}; unsigned seen = 0;
    char name[32] = {0}, recipe_id[21] = {0}; int32_t delta = 0; bool click = false;
    if (!token(&j, '{') || peek(&j, '}')) goto bad;
    do {
        char field[32]; unsigned bit;
        if (!key(&j, field)) goto bad;
        if (!strcmp(field, "command")) { bit = 1; if (!string(&j, name, sizeof name)) goto bad; }
        else if (!strcmp(field, "recipe_id")) { bit = 2; if (!string(&j, recipe_id, sizeof recipe_id)) goto bad; }
        else if (!strcmp(field, "delta")) { bit = 4; if (!integer(&j, -100, 100, &delta)) goto bad; }
        else if (!strcmp(field, "click")) { bit = 8; if (!boolean(&j, &click)) goto bad; }
        else goto bad;
        if (seen & bit) goto bad;
        seen |= bit;
    } while (comma_or_end(&j, '}'));
    if (!token(&j, '}') || !done(&j) || !(seen & 1)) goto bad;
    PicoLocalControlResult result;
    if (!strcmp(name, "encoder")) {
        if ((seen & 2) || !(seen & 12) || (!delta && !click)) goto bad;
        result = pico_local_control_encoder((int)delta, click);
    } else {
        if ((seen & 12) || ((seen & 2) && strcmp(name, "brew"))) goto bad;
        if (!strcmp(name, "brew") && !pico_local_recipe_id_valid(recipe_id)) goto bad;
        if (!strcmp(name, "brew") && pico_local_control_is_ready() &&
            !pico_local_control_busy() && !pico_local_control_stopped()) {
            PicoLocalRecipe selected;
            PicoLocalStoreResult stored = pico_local_recipe_get(&api->application->network,
                                                                 recipe_id, &selected);
            if (stored != PICO_LOCAL_STORE_OK) return storage_error(w, stored);
        }
        result = pico_local_control_command(name, recipe_id);
    }
    if (result == PICO_LOCAL_CONTROL_INVALID) goto bad;
    if (result == PICO_LOCAL_CONTROL_BUSY) return error(w, 409, "busy", "The appliance is executing another operation");
    if (result == PICO_LOCAL_CONTROL_STOPPED) return error(w, 409, "stopped", "Stop is latched; clear it at the main menu before starting");
    if (result == PICO_LOCAL_CONTROL_NOT_READY) return error(w, 503, "not_ready", "The appliance has not completed initialization");
    pico_local_event(&api->application->network, "command", name);
    text(w, "{\"accepted\":true,\"command\":"); quote(w, name); text(w, "}"); return 202;
bad:
    return error(w, 400, "invalid_command", "Invalid, duplicate or unsupported command fields");
}
static void settings_json(Writer *w, const PicoSettings *s)
{
    static const char *languages[] = {"en", "es", "fr"};
    format(w, "{\"temperature_unit\":\"%s\",\"language\":\"%s\",\"detailed_status\":%s}",
           s->temperature_unit_celsius ? "C" : "F", s->language < 3 ? languages[s->language] : "en",
           s->detailed_status ? "true" : "false");
}
static int settings(PicoLocalApi *api, Writer *w, const char *method, const char *body, size_t length)
{
    PicoNetwork *n = &api->application->network;
    if (!strcmp(method, "GET")) { settings_json(w, n->settings); return 200; }
    if (strcmp(method, "PATCH")) return error(w, 405, "method_not_allowed", "Use GET or PATCH for settings");
    if (pico_local_control_busy()) return error(w, 409, "busy", "Change settings only while the appliance is idle");
    PicoSettings changed = *n->settings, previous = *n->settings;
    Json j = {body, body + length, true}; unsigned seen = 0;
    if (!token(&j, '{') || peek(&j, '}')) goto bad;
    do {
        char field[32], value[4]; unsigned bit; bool flag;
        if (!key(&j, field)) goto bad;
        if (!strcmp(field, "temperature_unit")) {
            bit = 1; if (!string(&j, value, sizeof value)) goto bad;
            if (!strcmp(value, "C")) changed.temperature_unit_celsius = 1;
            else if (!strcmp(value, "F")) changed.temperature_unit_celsius = 0;
            else goto bad;
        } else if (!strcmp(field, "language")) {
            bit = 2; if (!string(&j, value, sizeof value)) goto bad;
            if (!strcmp(value, "en")) changed.language = 0;
            else if (!strcmp(value, "es")) changed.language = 1;
            else if (!strcmp(value, "fr")) changed.language = 2;
            else goto bad;
        } else if (!strcmp(field, "detailed_status")) {
            bit = 4; if (!boolean(&j, &flag)) goto bad; changed.detailed_status = flag;
        } else goto bad;
        if (seen & bit) goto bad;
        seen |= bit;
    } while (comma_or_end(&j, '}'));
    if (!token(&j, '}') || !done(&j)) goto bad;
    *n->settings = changed;
    if (!pico_local_settings_save(n)) {
        *n->settings = previous;
        return error(w, 503, "storage_io", "Could not verify saved settings");
    }
    pico_local_event(n, "settings", "changed"); settings_json(w, n->settings); return 200;
bad:
    return error(w, 400, "invalid_settings", "Supported settings are temperature_unit, language and detailed_status");
}
static int events(PicoLocalApi *api, Writer *w)
{
    text(w, "{\"events\":[");
    unsigned first = (api->event_head + PICO_LOCAL_EVENT_CAPACITY - api->event_count) % PICO_LOCAL_EVENT_CAPACITY;
    for (unsigned i = 0; i < api->event_count; ++i) {
        const PicoLocalEvent *e = &api->events[(first + i) % PICO_LOCAL_EVENT_CAPACITY];
        if (i) text(w, ",");
        format(w, "{\"sequence\":%lu,\"uptime_ms\":%lu,\"kind\":",
               (unsigned long)e->sequence, (unsigned long)e->uptime_ms);
        quote(w, e->kind); text(w, ",\"detail\":"); quote(w, e->detail); text(w, "}");
    }
    format(w, "],\"next_sequence\":%lu,\"capacity\":8,\"volatile\":true}", (unsigned long)api->next_sequence);
    return 200;
}
static void history_metadata_fields(Writer *w, const PicoHistoryMetadata *m)
{
    const char *state = m->state == PICO_HISTORY_RECORDING ? "recording" :
                        m->state == PICO_HISTORY_COMPLETED ? "completed" :
                        m->state == PICO_HISTORY_INTERRUPTED ? "interrupted" : "empty";
    format(w, "\"available\":%s,\"id\":%lu,\"command\":", m->available ? "true" : "false",
           (unsigned long)m->id);
    quote(w, m->command); text(w, ",\"recipe_name\":"); quote(w, m->recipe_name);
    text(w, ",\"state\":"); quote(w, state);
    format(w, ",\"duration_ms\":%lu,\"record_count\":%lu,\"persisted_record_count\":%lu,"
              "\"first_elapsed_ms\":%lu,\"gap_count\":%lu,\"truncated\":%s,\"storage_error\":%s,"
              "\"recovered_after_restart\":%s",
           (unsigned long)m->duration_ms, (unsigned long)m->record_count,
           (unsigned long)m->persisted_record_count, (unsigned long)m->first_elapsed_ms,
           (unsigned long)m->gap_count, m->truncated ? "true" : "false",
           m->storage_error ? "true" : "false", m->recovered_after_restart ? "true" : "false");
}
static int history(Writer *w)
{
    PicoHistoryMetadata metadata;
    pico_history_metadata(&metadata);
    text(w, "{"); history_metadata_fields(w, &metadata);
    format(w, ",\"sample_interval_ms\":%u,\"checkpoint_interval_ms\":%u,"
              "\"capacity_records\":%u,\"capacity_sessions\":8,\"page_records\":%u,"
              "\"fields\":[\"elapsed_ms\",\"wort_f\",\"thermoblock_f\",\"target_f\","
              "\"step_seconds\",\"phase\",\"controller_status\",\"compartment\","
              "\"current_step\",\"flags\",\"device_state\"],\"sessions\":[",
           PICO_HISTORY_SAMPLE_MS, PICO_HISTORY_CHECKPOINT_MS,
           PICO_HISTORY_RECORD_CAPACITY, PICO_HISTORY_PAGE_RECORDS);
    size_t count = pico_history_count();
    for (size_t i = 0; i < count; ++i) {
        if (!pico_history_at(i, &metadata))
            return error(w, 503, "history_storage", "Recorded history could not be read");
        if (i) text(w, ",");
        text(w, "{"); history_metadata_fields(w, &metadata); text(w, "}");
    }
    text(w, "]}");
    return 200;
}
static bool decimal_path(const char **cursor, uint32_t *value)
{
    const char *p = *cursor;
    if (*p < '0' || *p > '9' || (*p == '0' && p[1] >= '0' && p[1] <= '9')) return false;
    uint32_t number = 0;
    do {
        unsigned digit = (unsigned)(*p++ - '0');
        if (number > (UINT32_MAX - digit) / 10u) return false;
        number = number * 10u + digit;
    } while (*p >= '0' && *p <= '9');
    *cursor = p; *value = number; return true;
}
static void history_temperature(Writer *w, int16_t value)
{
    if (value == INT16_MIN) { text(w, "null"); return; }
    int magnitude = value < 0 ? -value : value;
    format(w, "%s%d.%d", value < 0 ? "-" : "", magnitude / 10, magnitude % 10);
}
static int history_samples(Writer *w, const char *path)
{
    uint32_t offset, id;
    if (!decimal_path(&path, &offset) || strncmp(path, "?id=", 4))
        return error(w, 400, "invalid_history_page", "Use a numeric offset and the retained brew id");
    path += 4;
    if (!decimal_path(&path, &id) || !id || *path)
        return error(w, 400, "invalid_history_page", "Use one positive numeric id query parameter");
    PicoHistoryMetadata metadata;
    if (!pico_history_find(id, &metadata))
        return error(w, 409, "history_changed", "This brew is no longer retained; refresh the history list");
    PicoHistorySample samples[PICO_HISTORY_PAGE_RECORDS];
    size_t count = 0;
    int result = pico_history_read(id, offset, samples, PICO_HISTORY_PAGE_RECORDS, &count);
    if (result == 1)
        return error(w, 409, "history_changed", "This brew is no longer retained; refresh the history list");
    if (result == 2)
        return error(w, 400, "invalid_history_offset", "Offset exceeds this brew's recorded samples");
    if (result != 0)
        return error(w, 503, "history_storage", "Recorded samples could not be read or verified");
    format(w, "{\"id\":%lu,\"offset\":%lu,\"samples\":[", (unsigned long)id, (unsigned long)offset);
    for (size_t i = 0; i < count; ++i) {
        const PicoHistorySample *sample = &samples[i];
        if (i) text(w, ",");
        format(w, "[%lu,", (unsigned long)sample->elapsed_ms);
        history_temperature(w, sample->wort_tenths_f); text(w, ",");
        history_temperature(w, sample->thermoblock_tenths_f); text(w, ",");
        history_temperature(w, sample->target_tenths_f);
        format(w, ",%ld,%u,%u,%u,%u,%u,%u]", (long)sample->step_seconds,
               sample->phase, sample->controller_status, sample->compartment,
               sample->current_step, sample->flags, sample->device_state);
    }
    text(w, "],\"next_offset\":");
    if (offset + count < metadata.record_count)
        format(w, "%lu", (unsigned long)(offset + count));
    else text(w, "null");
    text(w, "}"); return 200;
}
static int telemetry(Writer *w, const char *method, const char *body, size_t length)
{
    if (strcmp(method, "GET") && strcmp(method, "PUT"))
        return error(w, 405, "method_not_allowed", "Use GET or PUT for telemetry settings");
    PicoTelemetryConfig config;
    if (!strcmp(method, "PUT")) {
        memset(&config, 0, sizeof config);
        Json j = {body, body + length, true}; unsigned seen = 0;
        if (!token(&j, '{') || peek(&j, '}')) goto bad;
        do {
            char field[32]; unsigned bit; int32_t port;
            if (!key(&j, field)) goto bad;
            if (!strcmp(field, "enabled")) {
                bit = 1; if (!boolean(&j, &config.enabled)) goto bad;
            } else if (!strcmp(field, "host")) {
                bit = 2; if (!string(&j, config.host, sizeof config.host)) goto bad;
            } else if (!strcmp(field, "port")) {
                bit = 4; if (!integer(&j, 1, 65535, &port)) goto bad; config.port = (uint16_t)port;
            } else goto bad;
            if (seen & bit) goto bad;
            seen |= bit;
        } while (comma_or_end(&j, '}'));
        if (!token(&j, '}') || !done(&j) || seen != 7) goto bad;
        PicoTelemetryConfigureResult result = pico_telemetry_configure(&config);
        if (result == PICO_TELEMETRY_CONFIG_INVALID) goto bad;
        if (result == PICO_TELEMETRY_CONFIG_BUSY)
            return error(w, 409, "busy", "Edit the telemetry destination while the brewer is idle");
        if (result != PICO_TELEMETRY_CONFIG_OK)
            return error(w, 503, "storage_io", "Could not verify saved telemetry settings");
    }
    pico_telemetry_get_config(&config);
    PicoTelemetryStatus status; pico_telemetry_get_status(&status);
    format(w, "{\"enabled\":%s,\"host\":", config.enabled ? "true" : "false");
    quote(w, config.host);
    format(w, ",\"port\":%u,\"interval_ms\":%u,\"path\":\"/api/v1/telemetry\",\"status\":{\"state\":",
           config.port, PICO_TELEMETRY_INTERVAL_MS);
    quote(w, pico_telemetry_state_name(status.state));
    format(w, ",\"resolved\":%s,\"sent\":%lu,\"failed\":%lu,\"dropped\":%lu,\"last_success_ms\":%lu,"
              "\"last_http_status\":%u,\"storage_error\":%s,\"last_error\":",
           status.resolved ? "true" : "false", (unsigned long)status.sent,
           (unsigned long)status.failed, (unsigned long)status.dropped,
           (unsigned long)status.last_success_ms, status.last_http_status,
           status.storage_error ? "true" : "false");
    quote(w, pico_telemetry_error_name(status.last_error)); text(w, "}}"); return 200;
bad:
    return error(w, 400, "invalid_telemetry", "Use enabled, an IPv4 address or hostname, and port 1-65535");
}
int pico_local_api_handle(PicoLocalApi *api, const char *method, const char *path,
                          const char *body, size_t length, char *response, size_t capacity,
                          size_t *response_length)
{
    if (response_length) *response_length = 0;
    if (!api || !api->application || !method || !path || !response || !capacity ||
        (!body && length) || !response_length) return 500;
    if (!body) body = "";
    /* Do not clear response here: body may point into the same arena. */
    Writer w = {response, 0, capacity, true}; int result;
    if (length >= PICO_HTTP_ARENA_CAPACITY) {
        result = error(&w, 413, "body_too_large", "Request exceeds the bounded input buffer");
        *response_length = w.ok ? w.length : 0;
        return result;
    }
    bool get = !strcmp(method, "GET");
    if (!strcmp(path, "/api/v1/health")) result = get ? health(api, &w) :
        error(&w, 405, "method_not_allowed", "Use GET for health");
    else if (!strcmp(path, "/api/v1")) result = get ? discovery(&w) :
        error(&w, 405, "method_not_allowed", "Use GET for discovery");
    else if (!strcmp(path, "/api/v1/status")) result = get ? status(api, &w) :
        error(&w, 405, "method_not_allowed", "Use GET for status");
    else if (!strcmp(path, "/api/v1/display")) result = get ? display(api, &w) :
        error(&w, 405, "method_not_allowed", "Use GET for display");
    else if (!strcmp(path, "/api/v1/events")) result = get ? events(api, &w) :
        error(&w, 405, "method_not_allowed", "Use GET for events");
    else if (!strcmp(path, "/api/v1/telemetry")) result = telemetry(&w, method, body, length);
    else if (!strcmp(path, "/api/v1/history")) result = get ? history(&w) :
        error(&w, 405, "method_not_allowed", "Use GET for brew history");
    else if (!strncmp(path, "/api/v1/history/samples/", 24)) result = get ? history_samples(&w, path + 24) :
        error(&w, 405, "method_not_allowed", "Use GET for recorded samples");
    else if (!strcmp(path, "/api/v1/recipes")) result = recipes(api, &w, method, NULL, body, length);
    else if (!strcmp(path, "/api/v1/recipes/restore-defaults") && !strcmp(method, "POST"))
        result = restore_recipes(api, &w, length);
    else if (!strncmp(path, "/api/v1/recipes/", 16)) result = recipes(api, &w, method, path + 16, body, length);
    else if (!strcmp(path, "/api/v1/commands"))
        result = !strcmp(method, "POST") ? command(api, &w, body, length) :
            error(&w, 405, "method_not_allowed", "Use POST for commands");
    else if (!strcmp(path, "/api/v1/settings")) result = settings(api, &w, method, body, length);
    else result = error(&w, 404, "not_found", "Unknown API resource");
    if (!w.ok) result = error(&w, 500, "response_capacity", "Response exceeds the bounded buffer");
    *response_length = w.ok ? w.length : 0;
    if (w.ok && !w.length) response[0] = 0;
    return result;
}
bool pico_local_api_http_handler(const PicoHttpRequest *request, PicoHttpResponse *response,
                                 char *arena, size_t capacity, void *context)
{
    PicoLocalApi *api = context;
    if (!request || !response || !api) return false;
    api->auth_configured = request->auth_configured;
    size_t length = 0;
    int result = pico_local_api_handle(api, pico_http_method_name(request->method), request->path,
                                       request->body, request->body_length, arena, capacity, &length);
    response->status = (unsigned)result;
    response->content_type = "application/json; charset=utf-8";
    response->body = (const uint8_t *)arena;
    response->body_length = length;
    response->content_encoding = NULL;
    return true;
}
