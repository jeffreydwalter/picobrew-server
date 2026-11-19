#include "network_internal.h"

static void still_error(PicoNetwork *n, const char *message)
{
    net_serial(n, "ERROR ");
    net_line(n, message);
}

void pico_still_connection_initialize(PicoStillConnection *c)
{
    c->home_ip = (PicoIpv4){{0, 0, 0, 0}};
    c->active_ip = c->home_ip;
    c->lan_connected = 0;
    c->session_started = 0;
    memset(c->ssid, 0, sizeof(c->ssid));
    memset(c->identifier, 0, sizeof(c->identifier));
}

bool pico_still_set_identifier(PicoNetwork *n, const char *identifier)
{
    memset(n->still.ssid, 0, sizeof(n->still.ssid));
    memset(n->still.identifier, 0, sizeof(n->still.identifier));
    if (!identifier)
        return true;
    if (strlen(identifier) >= sizeof(n->still.identifier)) {
        still_error(n, "Still identifier exceeds buffer size");
        return false;
    }
    net_copy(n->still.identifier, sizeof(n->still.identifier), identifier);
    snprintf(n->still.ssid, sizeof(n->still.ssid), "PicoStill_%s", identifier);
    net_serial(n, "Selected Still [UID=");
    net_serial(n, identifier);
    net_serial(n, "] [SSID=");
    net_serial(n, n->still.ssid);
    net_line(n, "]");
    return true;
}

void pico_still_select_network(PicoNetwork *n, bool home)
{
    n->still.active_ip = home ? n->still.home_ip : (PicoIpv4){{192, 168, 4, 1}};
    pico_network_assert(n, !home || !pico_network_ipv4_is_zero(n->still.home_ip),
                        "accessPoint != AccessPoint::Home || hasHomeIP()");
}

bool pico_still_parse_response(PicoNetwork *n, PicoStillTelemetry *telemetry, const char *response)
{
    float parsed[8];
    unsigned fields = 0, length = 0;
    char field[16] = {0};
    for (;; ++response) {
        char value = *response;
        if (!value || value == ',') {
            parsed[fields] = length ? (float)strtod(field, NULL) : -1.0f;
            char label[16];
            snprintf(label, sizeof(label), "data[%u]=", fields);
            net_serial(n, label);
            n->platform->serial_write_float(n->platform_context, (double)parsed[fields], 2);
            net_line(n, "");
            ++fields;
            if (fields == 8)
                break;
            if (!value) {
                still_error(n, "Still response has fewer than eight fields");
                return false;
            }
            length = 0;
            memset(field, 0, sizeof(field));
        } else {
            field[length++] = value;
            /* The original exits as soon as its fifteenth character is read. */
            if (length > 14) {
                still_error(n, "not a float");
                return false;
            }
        }
    }
    telemetry->received = 1;
    telemetry->received_ms = net_now(n);
    telemetry->temperature1 = (double)parsed[0];
    telemetry->temperature2 = (double)parsed[1];
    telemetry->temperature3 = (double)parsed[2];
    telemetry->reading4 = (double)parsed[3];
    telemetry->pressure = (double)parsed[4];
    telemetry->pressure_status = net_truncate_signed((double)parsed[5]) != 0;
    telemetry->fan_on = net_truncate_signed((double)parsed[6]) != 0;
    telemetry->status3 = net_truncate_signed((double)parsed[7]) != 0;
    return true;
}

bool pico_still_pressure_ok(const PicoStillTelemetry *telemetry)
{
    return !telemetry->pressure_status && telemetry->pressure < telemetry->pressure_limit;
}

bool pico_still_call(PicoNetwork *n, unsigned action, char *response, size_t capacity)
{
    static const char *const methods[] = {"GET",  "POST", "GET",  "POST", "POST",
                                          "POST", "GET",  "POST", "POST"};
    static const char *const paths[] = {"ping", "configure", "stillip", "connecttolan", "start",
                                        "end",  "data",      "fanon",   "fanoff"};
    if (action >= 9 || !response || !capacity)
        return false;
    char text[128], body[256] = {0};
    size_t length = 0;
    snprintf(text, sizeof(text), "communicateWithStill() [action = %u] [ip = %u.%u.%u.%u]", action,
             n->still.active_ip.octet[0], n->still.active_ip.octet[1], n->still.active_ip.octet[2],
             n->still.active_ip.octet[3]);
    net_line(n, text);
    if (!n->platform->client_connect(n->platform_context, n->still.active_ip, 80, true)) {
        still_error(n, "connectTimeout(still.CurrentIp)");
        return false;
    }
    snprintf(text, sizeof(text), "%s /%s HTTP/1.0\r\n", methods[action], paths[action]);
    net_serial(n, text);
    if (!net_put(n, text))
        goto failed;
    if (action == 1) {
        pico_storage_load_wifi(n);
        snprintf(body, sizeof(body), "ssid=%s&pass=%s", n->settings->wifi_ssid,
                 n->settings->wifi_password);
    }
    snprintf(text, sizeof(text), "Content-Length: %u\r\n\r\n", (unsigned)strlen(body));
    if (!net_put(n, text) || (*body && !net_put(n, body)))
        goto failed;
    if (*body)
        net_line(n, body);
    n->platform->client_flush(n->platform_context);
    return pico_network_read_response(n, response, capacity, &length, true, '#');
failed:
    pico_network_close(n);
    return false;
}

bool pico_still_send_wifi(PicoNetwork *n)
{
    char response[51];
    return pico_still_call(n, 1, response, sizeof(response)) && response[0] == 'T';
}

bool pico_still_get_ip(PicoNetwork *n)
{
    char response[51];
    if (!pico_still_call(n, 2, response, sizeof(response))) {
        still_error(n, "StillAction::GetStillIp");
        return false;
    }
    if (pico_network_parse_ipv4(response, &n->still.home_ip))
        return true;
    still_error(n, "IPAddressfromString");
    return false;
}

bool pico_still_connect_lan(PicoNetwork *n)
{
    char response[51];
    bool success = pico_still_call(n, 3, response, sizeof(response));
    if (success)
        n->still.lan_connected = response[0] == 'T';
    else
        still_error(n, "StillAction::ConnectToLAN");
    return success;
}

bool pico_still_read_status(PicoNetwork *n)
{
    PicoStillTelemetry *t = &n->app->controller.still;
    pico_still_clear_telemetry(t);
    char response[101] = {0};
    bool success = pico_still_call(n, 6, response, sizeof(response));
    net_serial(n, "Communication succeeded? ");
    net_line(n, success ? "1" : "0");
    net_serial(n, "response: ");
    net_line(n, response);
    if (success && *response && pico_still_parse_response(n, t, response)) {
        n->still.consecutive_failures = 0;
        return true;
    }
    /* The original connection counter is a byte; the telemetry copy widens
     * that wrapped byte to uint32_t. Preserve rollover at 255. */
    n->still.consecutive_failures = (uint8_t)(n->still.consecutive_failures + 1u);
    t->communication_failures = n->still.consecutive_failures;
    still_error(n, "cannot get still data");
    still_error(n, success ? "communication failed" : "response empty");
    return false;
}

bool pico_still_blocked(PicoNetwork *n)
{
    PicoBrewController *c = &n->app->controller;
    PicoStillTelemetry *t = &c->still;
    if (pico_still_read_status(n) ||
        (int32_t)t->communication_failures < (int32_t)t->communication_failure_limit) {
        if (c->status != 9)
            return c->status == 10 ? !pico_still_pressure_ok(t) : false;
        if (!(t->temperature2 < t->temperature2_limit - t->temperature2_margin))
            return pico_still_pressure_ok(t);
    }
    return true;
}

bool pico_still_choose_access_point(PicoNetwork *n)
{
    char identifiers[22][22];
    size_t count = 0;
    int networks = n->platform->scan_networks(n->platform_context);
    if (networks < 1) {
        still_error(n, "No Wifi Network");
        return false;
    }
    for (int i = 0; i < networks; ++i) {
        const char *ssid = n->platform->scan_ssid(n->platform_context, (unsigned)i);
        net_line(n, ssid);
        if (strncmp(ssid, "PicoStill_", 10))
            continue;
        size_t length = strlen(ssid);
        pico_network_assert(n, length < 32, "len < sizeof(still.Ssid)");
        if (length >= 32 || count >= 22)
            return false;
        net_copy(identifiers[count++], 22, ssid + 10);
        net_copy(n->still.ssid, sizeof(n->still.ssid), ssid);
    }
    if (!count) {
        still_error(n, "No PicoStill Access Point found");
        return false;
    }
    int selected = n->ui->choose_still(n->ui_context, (const char(*)[22])identifiers, count);
    return selected >= 0 && (size_t)selected < count &&
           pico_still_set_identifier(n, identifiers[selected]);
}

bool pico_still_connect_wifi(PicoNetwork *n, bool home)
{
    n->platform->wifi_disconnect(n->platform_context);
    if (home) {
        net_delay(n, 100);
        pico_storage_load_wifi(n);
    } else {
        net_copy(n->settings->wifi_ssid, sizeof(n->settings->wifi_ssid), n->still.ssid);
        net_copy(n->settings->wifi_password, sizeof(n->settings->wifi_password), "12345678");
        n->settings->wifi_security = 4;
    }
    if (!pico_network_connect_stored(n)) {
        still_error(n, "connectToStoredWiFi");
        return false;
    }
    if (!pico_network_wait_local_ip(n, 15000, false)) {
        still_error(n, "waitForLocalIP");
        return false;
    }
    pico_still_select_network(n, home);
    return true;
}

bool pico_still_wait_response(PicoNetwork *n, bool home, uint32_t delay_seconds,
                              uint32_t timeout_seconds)
{
    char response[51];
    char log[96];
    snprintf(log, sizeof(log), "waitForStill() [ap=%s] [delay=%u] [to=%u]",
             home ? "Home" : "Still", (unsigned)delay_seconds, (unsigned)timeout_seconds);
    net_line(n, log);
    net_delay(n, delay_seconds * 1000u);
    uint32_t started = net_now(n);
    for (;;) {
        if ((uint32_t)(started + timeout_seconds * 1000u) < net_now(n)) {
            still_error(n, "timeout while trying to hear back from still");
            return false;
        }
        if (!pico_still_connect_wifi(n, home)) {
            still_error(n, "failed to find PicoStill AP");
            return false;
        }
        if (pico_still_call(n, 0, response, sizeof(response)))
            return response[0] == 'T';
        net_delay(n, 1000);
    }
}

bool pico_still_configure(PicoNetwork *n)
{
    net_line(n, "setupStillSession()");
    unsigned attempts = 0, delay = 0;
    const char *error;
    net_ui(n, PICO_NET_UI_STILL_CONNECTING, "Still setup 1/3");
    for (;;) {
        if (!pico_still_wait_response(n, false, delay, 30)) {
            error = "failed to get answer from PicoStill";
            break;
        }
        net_ui(n, PICO_NET_UI_STILL_CONNECTING, "Still setup 2/3");
        if (!pico_still_get_ip(n)) {
            error = "getStillIPAddress() failed";
            break;
        }
        if (!pico_network_ipv4_is_zero(n->still.home_ip)) {
            net_line(n, "Got Still IP address");
            net_ui(n, PICO_NET_UI_STILL_CONNECTING, "Still setup 3/3");
            if (!pico_still_connect_lan(n)) {
                error = "stillConnectToLan()";
                break;
            }
            if (pico_still_wait_response(n, true, 2, 30))
                return true;
            error = "failed to find PicoStill on LAN";
            break;
        }
        ++attempts;
        net_line(n, "Still has no local IP address");
        if (attempts == 3) {
            error = "Failed to configure PicoStill";
            break;
        }
        if (!pico_still_send_wifi(n)) {
            error = "sendStillWifiData() failed";
            break;
        }
        delay = 2;
    }
    still_error(n, error);
    return false;
}

bool pico_still_start(PicoNetwork *n)
{
    char response[101];
    net_line(n, "startStillSession");
    bool success = pico_still_call(n, 4, response, sizeof(response));
    if (!success)
        still_error(n, "StillAction::Start");
    return success && response[0] == 'T';
}

bool pico_still_start_once(PicoNetwork *n)
{
    net_line(n, "StillController::sessionStart");
    if (n->still.session_started)
        return false;
    n->still.session_started = pico_still_start(n);
    return n->still.session_started != 0;
}

bool pico_still_stop(PicoNetwork *n)
{
    net_line(n, "StillController::sessionEnd");
    if (!n->still.session_started)
        return false;
    n->still.session_started = 0;
    net_line(n, "endStillSession");
    net_ui(n, PICO_NET_UI_STILL_EXITING, NULL);
    char response[101];
    bool success = pico_still_call(n, 5, response, sizeof(response));
    net_ui(n, PICO_NET_UI_STOP_PROGRESS, NULL);
    return success;
}

void pico_still_set_fan(PicoNetwork *n, bool enabled)
{
    char response[101];
    if (!pico_still_call(n, enabled ? 7 : 8, response, sizeof(response)))
        still_error(n, "cannot set fan state");
}
