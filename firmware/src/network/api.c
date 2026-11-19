#include "network_internal.h"
#include "picobrew/text.h"
#include "picobrew/config.h"

#if PICO_STANDALONE
#include "picobrew/local_store.h"

/* The appliance owns recipes and session identity in the standalone build.
 * These adapters intentionally contain no HTTP/DNS fallback. */
bool pico_api_register(PicoNetwork *n)
{
    return pico_storage_load_uid(n);
}

bool pico_api_get_session(PicoNetwork *n, char *identifier, size_t capacity)
{
    return pico_local_session_new(n, identifier, capacity);
}

bool pico_api_create_picopak(PicoNetwork *n, int recipe_id)
{
    (void)n; (void)recipe_id;
    /* Local catalog selection uses the stored recipe ID directly. */
    return false;
}

int pico_api_required_actions(PicoNetwork *n, uint32_t *actions, size_t capacity)
{
    (void)n; (void)actions; (void)capacity;
    return 0; /* Maintenance is selected locally, never mandated by a server. */
}

int pico_api_fetch_recipe(PicoNetwork *n, const char *identifier, int ibu, int abv)
{
    (void)ibu; (void)abv;
    n->app->recipe.program = &n->app->session;
    return pico_local_recipe_load(n, identifier, &n->app->recipe,
                                  n->app->screen.recipe_name) ? 0 : 14;
}

void pico_api_report_state(PicoNetwork *n)
{
    char detail[16];
    snprintf(detail, sizeof detail, "%u", n->app->device_state);
    pico_local_event(n, "state", detail);
}

bool pico_api_error(PicoNetwork *n, int code)
{
    char detail[16];
    snprintf(detail, sizeof detail, "%d", code);
    pico_local_event(n, "error", detail);
    return true;
}

bool pico_api_log_event(PicoNetwork *n, const char *step, const char *event)
{
    if (!step || !*step)
        return false;
    char detail[96];
    snprintf(detail, sizeof detail, "%s%s%s", step, event ? ": " : "", event ? event : "");
    pico_local_event(n, event ? "step" : "progress", detail);
    return true;
}

bool pico_api_log_step(PicoNetwork *n, const char *step)
{
    return pico_api_log_event(n, step, step);
}

bool pico_api_log_progress(PicoNetwork *n, const char *step)
{
    return pico_api_log_event(n, step, NULL);
}

bool pico_api_recipe_list(PicoNetwork *n, PicoRecipeListing *list, bool associated)
{
    (void)associated;
    return pico_local_recipe_list(n, list);
}

bool pico_api_still_permission(PicoNetwork *n, const char *method, const char *identifier)
{
    (void)n; (void)method; (void)identifier;
    /* PicoStill commissioning/cleaning authorization has not been ported.
     * Fail closed without attempting to reach the discontinued service. */
    return false;
}
#else

static void text_warning(void *context, const char *level, const char *component,
                         const char *message)
{
    pico_network_log(context, level, component, message);
}

static bool api_hash(PicoNetwork *n, char *request, size_t capacity, size_t *length)
{
    return pico_network_open_request(n, request, true, false) &&
           pico_network_read_response(n, request, capacity, length, true, '#');
}

bool pico_api_register(PicoNetwork *n)
{
    char text[501];
    size_t length = 0;
    net_prefix(n, text, sizeof(text), "register");
    if (api_hash(n, text, sizeof(text), &length)) {
        if (length != 1 || text[0] != 'F') {
            if (text[0] != 'F')
                return true;
            if (length > 4) {
                net_ui(n, PICO_NET_UI_REGISTRATION_CODE, text + 2);
                net_ui(n, PICO_NET_UI_FLUSH, NULL);
                net_delay(n, 15000);
                return false;
            }
            net_error(n, 13, true, true);
            return false;
        }
        net_error(n, 6, false, true);
    }
    net_error(n, 7, true, false);
    return false;
}

bool pico_api_get_session(PicoNetwork *n, char *identifier, size_t capacity)
{
    pico_network_assert(n, identifier && capacity, "buffer != NULL && bufferSize > 0");
    if (!identifier || !capacity)
        return false;
    char text[501];
    size_t length = 0;
    snprintf(text, sizeof(text), "/API/pico/getSession?uid=%s&sesType=%u", n->app->device_uid,
             (unsigned)n->app->session.session_type);
    if (!api_hash(n, text, sizeof(text), &length) || !length || length > 21) {
        net_error(n, 7, true, false);
        return false;
    }
    pico_text_copy(identifier, (uint32_t)capacity, text, text_warning, n);
    return true;
}

bool pico_api_create_picopak(PicoNetwork *n, int recipe_id)
{
    char text[501];
    size_t length = 0;
    snprintf(text, sizeof(text), "/API/pico/createPicoPak?uid=%s&recipeid=%d", n->app->device_uid,
             recipe_id);
    if (!api_hash(n, text, sizeof(text), &length) || !length || length > 20)
        return false;
    net_copy(n->app->recipe.identifier, sizeof(n->app->recipe.identifier), text);
    return true;
}

int pico_api_required_actions(PicoNetwork *n, uint32_t *actions, size_t capacity)
{
    char text[501];
    size_t length = 0, count = 0;
    net_prefix(n, text, sizeof(text), "getActionsNeeded");
    if (!api_hash(n, text, sizeof(text), &length) || !length)
        return 0;
    for (char *field = text; *field && count < capacity;) {
        char *end = strchr(field, ',');
        if (end)
            *end = 0;
        if ((uint8_t)(field[0] - '0') < 10)
            actions[count++] = (uint32_t)strtol(field, NULL, 10);
        if (!end)
            break;
        field = end + 1;
    }
    return (int)count;
}

int pico_api_fetch_recipe(PicoNetwork *n, const char *identifier, int ibu, int abv)
{
    char text[501], number[32];
    size_t length = 0;
    pico_network_format_float((float)abv, number, 1);
    int count = snprintf(text, sizeof(text), "/API/pico/getRecipe?uid=%s&rfid=%s&ibu=%d&abv=%s",
                         n->app->device_uid, identifier, ibu, number);
    if (!net_request_fits(count, sizeof(text)) ||
        !pico_network_open_request(n, text, true, false) ||
        !pico_network_read_response(n, text, sizeof(text), &length, false, '|') || !length) {
        net_error(n, 7, true, false);
        return 7;
    }
    if (!strcmp(text, "Invalid")) {
        net_error(n, 14, true, true);
        return 14;
    }
    if (!n->ui->decode_recipe(n->ui_context, text, length)) {
        net_error(n, 13, true, true);
        return 13;
    }
    if (pico_network_read_hex_image(n, n->recipe_logo, '|')) {
        n->ui->recipe_logo_ready(n->ui_context, n->recipe_logo);
        int index = (int)n->app->carousel.page_count - 4;
        /* Both increments are present at 0x20010b80..0x20010b94. */
        while (index >= 0 && index < 4 && n->app->carousel.page_count < 15 &&
               pico_network_read_hex_image(n, n->recipe_images[index], '|')) {
            n->ui->recipe_image_ready(n->ui_context, (unsigned)index, n->recipe_images[index]);
            n->app->carousel.page_count += 2;
            ++index;
        }
    }
    return 0;
}

void pico_api_report_state(PicoNetwork *n)
{
    char request[500];
    snprintf(request, sizeof(request), "/API/pico/picoChangeState?picoUID=%s&state=%u",
             n->app->device_uid, n->app->device_state);
    pico_network_open_request(n, request, false, true);
}

bool pico_api_error(PicoNetwork *n, int code)
{
    char request[500];
    snprintf(request, sizeof(request), "/API/pico/error?uid=%s&code=%d&rfid=%s", n->app->device_uid,
             code, n->app->recipe.identifier);
    return pico_network_open_request(n, request, false, false);
}

bool pico_api_log_event(PicoNetwork *n, const char *step, const char *event)
{
    char request[500], encoded_step[128], encoded_event[128], scale[32], t1[32], t2[32],
        pressure[32];
    if (!step || pico_network_url_encode(step, encoded_step, sizeof(encoded_step)) == SIZE_MAX)
        return false;
    if (n->log_consecutive_failures >= 5) {
        n->platform->wifi_disconnect(n->platform_context);
        net_ui(n, PICO_NET_UI_REFRESH_PAGE, NULL);
        n->log_consecutive_failures = 0;
        return false;
    }
    if (event && pico_network_url_encode(event, encoded_event, sizeof(encoded_event)) == SIZE_MAX)
        return false;
    pico_network_format_float((float)n->app->controller.candidate_shuttle_scale, scale, 2);
    int count = snprintf(request, sizeof(request),
                         "/API/pico/"
                         "log?uid=%s&sesId=%s&wort=%d&therm=%d&step=%s%s%s&error=%u&sesType=%u&"
                         "timeLeft=%d&shutScale=%s",
                         n->app->device_uid, n->app->session.identifier,
                         (int)net_truncate_signed(n->app->controller.wort_fahrenheit),
                         (int)net_truncate_signed(n->app->controller.thermoblock_fahrenheit),
                         encoded_step, event ? "&event=" : "", event ? encoded_event : "",
                         n->app->controller.status, (unsigned)n->app->session.session_type,
                         (int)n->app->controller.step_seconds, scale);
    if (!net_request_fits(count, sizeof(request)))
        return false;
    if (n->app->session.session_type == 3) {
        pico_network_format_float((float)n->app->controller.still.temperature1, t1, 1);
        pico_network_format_float((float)n->app->controller.still.temperature2, t2, 1);
        pico_network_format_float((float)n->app->controller.still.pressure, pressure, 2);
        int added = snprintf(request + count, sizeof(request) - (size_t)count, "&t1=%s&t2=%s&p=%s",
                             t1, t2, pressure);
        if (!net_request_fits(added, sizeof(request) - (size_t)count))
            return false;
    }
    bool success = pico_network_call(n, request, false, true);
    if (success)
        n->log_consecutive_failures = 0;
    else {
        pico_network_log(n, "E", "WIFI", "makeWiFiCall() failed");
        ++n->log_consecutive_failures;
    }
    return success;
}

bool pico_api_log_step(PicoNetwork *n, const char *step)
{
    pico_network_assert(n, step && *step, "!PicoBrew::CString::isNullOrEmpty(stepName)");
    if (!n->settings->offline_mode)
        return pico_api_log_event(n, step, step);
    net_line(n, "wifi_log() disabled as g_offlineMode is True");
    return true;
}

bool pico_api_log_progress(PicoNetwork *n, const char *step)
{
    pico_network_assert(n, step && *step, "!PicoBrew::CString::isNullOrEmpty(stepName)");
    if (!n->settings->offline_mode)
        return pico_api_log_event(n, step, NULL);
    net_line(n, "wifi_log() disabled as g_offlineMode is True");
    return true;
}

bool pico_api_recipe_list(PicoNetwork *n, PicoRecipeListing *list, bool associated)
{
    char record[501];
    net_prefix(n, record, sizeof(record), associated ? "getAssociatedPaks" : "recipelist");
    if (!pico_network_call(n, record, true, false)) {
        pico_network_close(n);
        return false;
    }
    uint32_t started = net_now(n);
    int total = 0, loaded = 0, menus = 0;
    size_t length = 0;
    for (;;) {
        if (!n->platform->client_connected(n->platform_context) ||
            (uint32_t)(net_now(n) - started) > 29999u)
            break;
        if (!n->platform->client_available(n->platform_context))
            continue;
        uint8_t value = (uint8_t)n->platform->client_read(n->platform_context);
        if (value == '#') {
            if (loaded) {
                list->total_items = total;
                list->loaded_items = loaded;
                list->menu_item_count = associated ? menus : loaded;
                pico_network_close(n);
                return true;
            }
            break;
        }
        if (value == '|') {
            record[length] = 0;
            ++total;
            if (total > list->page_offset && total <= list->page_offset + 19 && loaded < 19) {
                char *comma = strchr(record, ',');
                if (!comma)
                    break;
                *comma = 0;
                if ((size_t)(comma - record) > 199 || strlen(comma + 1) > 50)
                    break;
                net_copy(list->identifiers[loaded], 22, record);
                net_copy(list->titles[loaded], 22, comma + 1);
                if (!associated)
                    net_copy(list->menu_labels[loaded], 22, comma + 1);
                else if (!menus || strcmp(list->menu_labels[menus - 1], list->titles[loaded]))
                    net_copy(list->menu_labels[menus++], 22, comma + 1);
                ++loaded;
            }
            length = 0;
        } else {
            record[length++] = (char)value;
            if (length > 399)
                break;
        }
    }
    pico_network_close(n);
    return false;
}

bool pico_api_still_permission(PicoNetwork *n, const char *method, const char *identifier)
{
    if (!identifier || !*identifier)
        return false;
    if (strcmp(method, "canUsePicoStill") && strcmp(method, "hasCleanedAck") &&
        strcmp(method, "setCleanedAck"))
        return false;
    char text[501];
    size_t length = 0;
    int count = snprintf(text, sizeof(text), "/API/pico/%s?picoUid=%s&picoStillUid=%s", method,
                         n->app->device_uid, identifier);
    if (!net_request_fits(count, sizeof(text)) || !api_hash(n, text, sizeof(text), &length) ||
        length != 1) {
        net_error(n, 7, true, false);
        return false;
    }
    return text[0] == 'T';
}
#endif
