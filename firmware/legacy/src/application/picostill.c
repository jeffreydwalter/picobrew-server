/* Pico Pro 0.1.34 — picostill. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Uses PicoStill action path/method tables and optionally serializes ssid/pass data.
 * Firmware address: 0x2000a664. */
void pico_still_write_request(uint32_t action_id)
{
    uint32_t value;
    char *text;
    char *text_00;
    char buffer_110[256];

    text_00 = (&g_picostill_unknown_02)[action_id];
    text = (&g_picostill_unknown_01)[action_id];
    Print_print_string(&global_serial0, text_00);
    Print_print_string(&global_serial0, " /");
    Print_print_string(&global_serial0, text);
    Print_println_string(&global_serial0, " HTTP/1.0");
    Print_print_string(&g_http_client, text_00);
    Print_print_string(&g_http_client, " /");
    Print_print_string(&g_http_client, text);
    Print_println_string(&g_http_client, " HTTP/1.0");
    libc_memset(buffer_110, 0, 0x100);
    if (action_id == 1) {
        pico_storage_load_wifi_credentials();
        libc_strcpy(buffer_110, "ssid=");
        libc_strcat(buffer_110, &g_wifi_ssid);
        libc_strcat(buffer_110, "&pass=");
        libc_strcat(buffer_110, &g_wifi_password);
    }
    value = libc_strlen(buffer_110);
    Print_print_string(&g_http_client, "Content-Length: ");
    Print_println_unsigned(&g_http_client, value, 10);
    Print_println_string(&g_http_client, "");
    if (value != 0) {
        Print_print_string(&g_http_client, buffer_110);
        Print_println_string(&global_serial0, buffer_110);
    }
    WiFiClient_flush(&g_http_client);
    return;
}

/* Connects to selected Still IP on port80, writes action request, reads framed response.
 * Firmware address: 0x2000be68. */
int pico_still_call(uint32_t action_id, char *response, uint32_t capacity)
{
    int wi_fi_client_connect_ipv4_result;
    uint8_t **pointer_20;
    uint32_t value;

    Print_print_string(&global_serial0, "communicateWithStill() ");
    Print_print_string(&global_serial0, "[action = ");
    Print_print_signed_alias(&global_serial0, action_id, 10);
    Print_print_string(&global_serial0, "] [ip = ");
    pico_log_ipv4_address(&g_initialization_unknown_22);
    Print_println_string(&global_serial0, "]");
    pointer_20 = &g_network_addresses_unknown_01;
    value = g_still_active_ipv4;
    wi_fi_client_connect_ipv4_result = WiFiClient_connect_ipv4(&g_http_client, &pointer_20, 0x50);
    if (wi_fi_client_connect_ipv4_result == 0) {
        pico_still_log_error("connectTimeout(still.CurrentIp)");
        wi_fi_client_connect_ipv4_result = 0;
    } else {
        pico_still_write_request(action_id);
        pointer_20 = (uint8_t **)0x0;
        wi_fi_client_connect_ipv4_result =
            pico_read_api_response(response, capacity, (uint32_t *)&pointer_20, 1, '#');
    }
    return wi_fi_client_connect_ipv4_result;
}

/* Calls Still action1 and tests T response.
 * Firmware address: 0x2000bf08. */
bool pico_still_send_wifi_credentials(void)
{
    char buffer[60];

    pico_still_call(1, buffer, 0x32);
    return buffer[0] == 'T';
}

/* Calls GetStillIp and parses returned IP address.
 * Firmware address: 0x2000bf28. */
short pico_still_get_ip_address(void)
{
    short parse_ipv4_address_result;
    int still_call_result;
    char *message;
    char buffer[52];

    still_call_result = pico_still_call(2, buffer, 0x32);
    if (still_call_result == 0) {
        message = "StillAction::GetStillIp";
    } else {
        parse_ipv4_address_result = pico_parse_ipv4_address((int)buffer, 0x2002d580);
        if (parse_ipv4_address_result != 0) {
            return parse_ipv4_address_result;
        }
        message = "IPAddressfromString";
    }
    pico_still_log_error(message);
    return 0;
}

/* Calls action3; explicit ConnectToLAN error text and T-status flag.
 * Firmware address: 0x2000bf64. */
int pico_still_connect_to_lan(void)
{
    int still_call_result;
    char buffer[52];

    still_call_result = pico_still_call(3, buffer, 0x32);
    if (still_call_result == 0) {
        pico_still_log_error("StillAction::ConnectToLAN");
    } else {
        g_still_lan_connected = buffer[0] == 'T';
    }
    return still_call_result;
}

/* Calls Still action6, tracks consecutive failures, parses status response into controller.
 * Firmware address: 0x2000bf9c. */
uint32_t pico_still_read_status(uint8_t *controller)
{
    int request_succeeded;
    int response_invalid;
    uint32_t value_2;
    char *error_message;
    char response[104];

    pico_still_clear_telemetry(controller);
    response[0] = '\0';
    request_succeeded = pico_still_call(6, response, 100);
    Print_print_string(&global_serial0, "Communication succeeded? ");
    Print_println_signed(&global_serial0, request_succeeded, 10);
    Print_print_string(&global_serial0, "response: ");
    Print_println_string(&global_serial0, response);
    if ((request_succeeded == 0) ||
        (response_invalid = pico_cstring_is_null_or_empty_duplicate(response),
         response_invalid != 0)) {
        g_still_consecutive_failures = g_still_consecutive_failures + 1;
        *(uint32_t *)(controller + 4) = (uint32_t)g_still_consecutive_failures;
        pico_still_log_error("cannot get still data");
        error_message = "response empty";
        if (request_succeeded != 0) {
            error_message = "communication failed";
        }
        pico_still_log_error(error_message);
        value_2 = 0;
    } else {
        g_still_consecutive_failures = 0;
        pico_still_parse_telemetry(controller, response);
        value_2 = 1;
    }
    return value_2;
}

/* Checks repeated communications failure and Still status/temperature conditions.
 * Firmware address: 0x2000c038. */
bool pico_still_has_blocking_status(uint8_t *controller)
{
    bool still_check_pressure_limit_result;
    int still_read_status_result;
    double right;

    still_read_status_result = pico_still_read_status(controller);
    if ((still_read_status_result != 0) ||
        (*(int *)(controller + 4) < *(int *)(controller + 0x58))) {
        if (controller[0x61] != '\t') {
            if (controller[0x61] == '\n') {
                still_check_pressure_limit_result =
                    pico_still_check_pressure_limit((int)controller);
                return !still_check_pressure_limit_result;
            }
            return false;
        }
        right = __aeabi_dsub(*(double *)(controller + 0x40), *(double *)(controller + 0x48));
        still_read_status_result = __aeabi_dcmplt(*(double *)(controller + 0x18), right);
        if (still_read_status_result == 0) {
            still_check_pressure_limit_result = pico_still_check_pressure_limit((int)controller);
            return still_check_pressure_limit_result;
        }
    }
    return true;
}

/* Scans WiFi SSIDs for PicoStill_ prefix and offers matching units.
 * Firmware address: 0x2000c468. */
uint32_t pico_still_select_access_point_2(void)
{
    int wi_fi_scan_networks_result;
    char *wi_fi_scan_result_ssid_result;
    int libc_strncmp_result;
    uint32_t text_length;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r2_00;
    uint32_t unresolved_return_r2_01;
    uint32_t value_5;
    uint32_t unresolved_return_r3;
    uint32_t unresolved_return_r3_00;
    uint32_t unresolved_return_r3_01;
    uint32_t value_6;
    uint32_t menu_choose_result;
    uint32_t value_8;

    wi_fi_scan_networks_result = WiFi_scan_networks();
    if (wi_fi_scan_networks_result < 1) {
        wi_fi_scan_result_ssid_result = "No Wifi Network";
    } else {
        value_8 = 0;
        menu_choose_result = 0;
        value_5 = unresolved_return_r2;
        value_6 = unresolved_return_r3;
        do {
            wi_fi_scan_result_ssid_result =
                WiFi_scan_result_ssid(&global_wifi, value_8 & 0xff, value_5, value_6);
            Print_println_string(&global_serial0, wi_fi_scan_result_ssid_result);
            libc_strncmp_result = libc_strncmp(wi_fi_scan_result_ssid_result, "PicoStill_", 10);
            value_5 = unresolved_return_r2_00;
            value_6 = unresolved_return_r3_00;
            if (libc_strncmp_result == 0) {
                text_length = libc_strlen(wi_fi_scan_result_ssid_result);
                pico_assert_nonfatal((uint32_t)(text_length < 0x20), "len < sizeof(still.Ssid)");
                if (0x1f < text_length) {
                    return 0;
                }
                libc_memset(&g_menu_label_0 + menu_choose_result * 0x16, 0, 0x16);
                libc_strncpy(&g_menu_label_0 + menu_choose_result * 0x16,
                             wi_fi_scan_result_ssid_result + 10, 0x15);
                menu_choose_result = menu_choose_result + 1;
                libc_strcpy(&g_still_access_point_ssid, wi_fi_scan_result_ssid_result);
                value_5 = unresolved_return_r2_01;
                value_6 = unresolved_return_r3_01;
            }
            value_8 = value_8 + 1;
        } while ((int)value_8 < wi_fi_scan_networks_result);
        if (menu_choose_result != 0) {
            pico_display_clear();
            pico_ui_draw_subheader("CHOOSE A STILL");
            menu_choose_result = pico_menu_choose(3, menu_choose_result & 0xff, 0);
            pico_still_set_access_point_name(0x2002d580,
                                             &g_menu_label_0 + menu_choose_result * 0x16);
            return 1;
        }
        wi_fi_scan_result_ssid_result = "No PicoStill Access Point found";
    }
    pico_still_log_error(wi_fi_scan_result_ssid_result);
    return 0;
}

/* Asks user to confirm PicoStill has been cleaned before use.
 * Firmware address: 0x2000c7b8. */
void pico_ui_confirm_still_cleaned(void)
{
    pico_display_clear();
    pico_display_center_text(1, "Please confirm that");
    pico_display_center_text(2, "you have cleaned");
    pico_display_center_text(3, "your PicoStill prior");
    pico_display_center_text(4, "to use.");
    pico_label_confirm(&g_menu_label_0);
    pico_menu_choose(7, 1, 0);
    return;
}

/* Chooses PicoStill AP or stored home WiFi, connects and waits for local IP.
 * Firmware address: 0x2000dcc8. */
int pico_still_connect_wifi_network(int network_mode)
{
    int wifi_connect_stored_network_result;
    char *message;
    uint32_t unresolved_return_r2;

    WiFi_disconnect();
    if (network_mode == 0) {
        libc_strcpy(&g_wifi_ssid, &g_still_access_point_ssid);
        libc_strcpy(&g_wifi_password, "12345678");
        g_wifi_security_type_as_uint8_t_2 = 4;
    } else {
        if (network_mode != 1) {
            pico_still_log_error("Unexpected Access Point");
            return 0;
        }
        delay(100);
        pico_storage_load_wifi_credentials();
    }
    wifi_connect_stored_network_result = pico_wifi_connect_stored_network();
    if (wifi_connect_stored_network_result == 0) {
        message = "connectToStoredWiFi";
    } else {
        wifi_connect_stored_network_result = pico_wifi_wait_for_local_ip(15000, 0);
        if (wifi_connect_stored_network_result != 0) {
            pico_still_select_access_point(0x2002d580, network_mode, unresolved_return_r2);
            return wifi_connect_stored_network_result;
        }
        message = "waitForLocalIP";
    }
    pico_still_log_error(message);
    return 0;
}

/* Still communication failure message and Continue acknowledgement.
 * Firmware address: 0x2000e3d0. */
void pico_ui_show_still_communication_failure(void)
{
    pico_display_clear();
    pico_display_center_text(1, "Connection lost.");
    pico_display_center_text(2, "Please check");
    pico_display_center_text(3, "PicoStill.");
    pico_ui_wait_for_continue();
    return;
}

/* Alarm and Still completion message with acknowledgement.
 * Firmware address: 0x2000e404. */
void pico_ui_show_still_completion(void)
{
    pico_alarm_until_encoder_press();
    pico_display_clear();
    pico_display_center_text(1, "Remove pressure");
    pico_display_center_text(2, "plug");
    pico_ui_wait_for_continue();
    return;
}

/* Alarm and Still temperature condition message.
 * Firmware address: 0x2000e430. */
void pico_ui_show_still_temperature_warning(void)
{
    pico_alarm_until_encoder_press();
    pico_display_clear();
    pico_display_center_text(2, "Still T2 overtemp");
    pico_ui_wait_for_continue();
    return;
}

/* Explicit waitForStill log; initial delay, AP/home reconnect and action0 ping until timeout.
 * Firmware address: 0x20010104. */
bool pico_still_wait_for_response(int network_mode, uint32_t initial_delay_seconds,
                                  uint32_t timeout_seconds)
{
    uint32_t wait_started_ms;
    uint32_t now_ms;
    int request_succeeded;
    char *network_name;
    char response[56];

    Print_print_string(&global_serial0, "waitForStill() [ap=");
    network_name = "Home";
    if (network_mode == 0) {
        network_name = "Still";
    }
    Print_print_string(&global_serial0, network_name);
    Print_print_string(&global_serial0, "] [delay=");
    Print_print_unsigned(&global_serial0, initial_delay_seconds, 10);
    Print_print_string(&global_serial0, "] [to=");
    Print_print_unsigned(&global_serial0, timeout_seconds, 10);
    Print_println_string(&global_serial0, "]");
    delay(initial_delay_seconds * 1000);
    wait_started_ms = millis();
    while (true) {
        now_ms = millis();
        if (timeout_seconds * 1000 + wait_started_ms < now_ms) {
            pico_still_log_error("timeout while trying to hear back from still");
            return false;
        }
        request_succeeded = pico_still_connect_wifi_network(network_mode);
        if (request_succeeded == 0)
            break;
        request_succeeded = pico_still_call(0, response, 0x32);
        if (request_succeeded != 0) {
            return response[0] == 'T';
        }
        delay(1000);
    }
    pico_still_log_error("failed to find PicoStill AP");
    return false;
}

/* Sends WiFi data, gets Still IP, connects to LAN and discovers configured Still.
 * Firmware address: 0x200101d8. */
uint8_t pico_still_configure(void)
{
    bool still_wait_for_response_result;
    short still_get_ip_address_result;
    char *message;
    uint32_t value_3;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    int unresolved_return_r3;
    int still_connect_to_lan_result;

    Print_println_string(&global_serial0, "setupStillSession()");
    still_connect_to_lan_result = 0;
    pico_screen_still_connecting("Still setup 1/3");
    value_3 = 0;
    do {
        still_wait_for_response_result = pico_still_wait_for_response(0, value_3, 0x1e);
        if (!still_wait_for_response_result) {
            message = "failed to get answer from PicoStill";
        branch_1:
            pico_still_log_error(message);
            return 0;
        }
        pico_screen_still_connecting("Still setup 2/3");
        still_get_ip_address_result = pico_still_get_ip_address();
        if (still_get_ip_address_result == 0) {
            message = "getStillIPAddress() failed";
            goto branch_1;
        }
        still_wait_for_response_result = pico_still_has_home_ip(
            0x2002d580, unresolved_return_r1, unresolved_return_r2, unresolved_return_r3);
        if (still_wait_for_response_result) {
            Print_println_string(&global_serial0, "Got Still IP address");
            pico_screen_still_connecting("Still setup 3/3");
            still_connect_to_lan_result = pico_still_connect_to_lan();
            if (still_connect_to_lan_result == 0) {
                message = "stillConnectToLan()";
            } else {
                still_wait_for_response_result = pico_still_wait_for_response(1, 2, 0x1e);
                if (still_wait_for_response_result) {
                    return 1;
                }
                message = "failed to find PicoStill on LAN";
            }
            goto branch_1;
        }
        still_connect_to_lan_result = still_connect_to_lan_result + 1;
        Print_println_string(&global_serial0, "Still has no local IP address");
        if (still_connect_to_lan_result == 3) {
            message = "Failed to configure PicoStill";
            goto branch_1;
        }
        still_wait_for_response_result = pico_still_send_wifi_credentials();
        if (!still_wait_for_response_result) {
            message = "sendStillWifiData() failed";
            goto branch_1;
        }
        value_3 = 2;
    } while (true);
}

/* Explicit startStillSession and StillAction::Start; action4 response must be T.
 * Firmware address: 0x200102a4. */
bool pico_still_start_session_request(void)
{
    int still_call_result;
    char buffer[100];

    Print_println_string(&global_serial0, "startStillSession");
    still_call_result = pico_still_call(4, buffer, 100);
    if (still_call_result == 0) {
        pico_still_log_error("StillAction::Start");
    }
    return still_call_result != 0 && buffer[0] == 'T';
}

/* StillController::sessionStart; calls start request only while context session-started byte is
 * false. Firmware address: 0x200102e8. */
bool pico_still_start_session_once(int still_controller)
{
    bool still_start_session_request_result;

    Print_println_string(&global_serial0, "StillController::sessionStart");
    if (*(char *)(still_controller + 0x14) == '\0') {
        still_start_session_request_result = pico_still_start_session_request();
        *(bool *)(still_controller + 0x14) = still_start_session_request_result;
        return still_start_session_request_result;
    }
    return false;
}

/* Still shutdown command used by endBrewSession when session type is Still.
 * Firmware address: 0x20010340. */
uint32_t pico_still_stop(int still_controller)
{
    uint32_t print_println_string_result;
    char response[100];

    print_println_string_result =
        Print_println_string(&global_serial0, "StillController::sessionEnd");
    if (*(char *)(still_controller + 0x14) == '\0') {
        return print_println_string_result;
    }
    *(uint8_t *)(still_controller + 0x14) = 0;
    Print_println_string(&global_serial0, "endStillSession");
    pico_screen_still_exiting();
    print_println_string_result = pico_still_call(5, response, 100);
    pico_display_stop_progress_indicator(&g_progress_indicator);
    return print_println_string_result;
}

/* Chooses action7/8 from requested fan state and reports command failure.
 * Firmware address: 0x20010560. */
void pico_still_set_fan(int fan_enabled)
{
    uint32_t action_id;
    int still_call_result;
    char buffer[108];

    if (fan_enabled == 0) {
        action_id = 8;
    } else {
        action_id = 7;
    }
    still_call_result = pico_still_call(action_id, buffer, 100);
    if (still_call_result == 0) {
        pico_still_log_error("cannot set fan state");
    }
    return;
}

/* StillFan action menu invoked by step runner.
 * Firmware address: 0x20010588. */
void pico_ui_still_fan_menu(void)
{
    uint32_t menu_populate_still_fan_result;

    menu_populate_still_fan_result = pico_menu_populate_still_fan();
    menu_populate_still_fan_result =
        pico_menu_choose_default_timeout(7, menu_populate_still_fan_result, 0, 1,
                                         (char)menu_populate_still_fan_result + 0xff, '\x01');
    if (menu_populate_still_fan_result < 2) {
        pico_display_clear();
        pico_display_center_text(2, "Sending To Still");
        g_progress_indicator = 3;
        g_progress_start_column = 5;
        g_progress_end_column = 0x10;
        g_progress_glyph_index = 0;
        g_progress_active = 1;
        pico_still_set_fan(menu_populate_still_fan_result);
        pico_display_stop_progress_indicator(&g_progress_indicator);
    }
    return;
}

/* canUsePicoStill query with Pico UID and Still UID; tests T response.
 * Firmware address: 0x2001079c. */
int pico_api_can_use_still(char *still_uid)
{
    int api_open_request_result;
    char *context;
    uint32_t value_208;
    char buffer_204[500];

    if ((still_uid == (char *)0x0) || (*still_uid == '\0')) {
        Print_println_string(&global_serial0, "ERROR wifi_picoStillApi ARG picoStillUid");
        return 0;
    }
    Print_print_string(&global_serial0, "wifi_picoStillApi(");
    Print_print_string(&global_serial0, "canUsePicoStill");
    Print_print_string(&global_serial0, ",");
    Print_print_string(&global_serial0, still_uid);
    Print_println_string(&global_serial0, ")");
    value_208 = 0;
    pico_api_write_base_path(buffer_204);
    libc_strcat(buffer_204, "canUsePicoStill");
    libc_strcat(buffer_204, "?picoUid=");
    libc_strcat(buffer_204, &g_device_uid);
    libc_strcat(buffer_204, "&picoStillUid=");
    libc_strcat(buffer_204, still_uid);
    api_open_request_result = pico_api_open_request(buffer_204, 1, 0);
    if (api_open_request_result == 0) {
        context = "connectToApi";
    } else {
        api_open_request_result = pico_api_read_hash_response(buffer_204, 500, &value_208);
        if (api_open_request_result != 0) {
            if (value_208 == 1) {
                if (buffer_204[0] == 'T') {
                    return 1;
                }
                Print_println_string(&global_serial0,
                                     "WARNING Did not get TRUE response from backend");
            } else {
                pico_api_show_error("length != 1");
            }
            return 0;
        }
        context = "readApiResponse";
    }
    pico_api_show_error(context);
    return 0;
}

/* hasCleanedAck query with Pico and Still UIDs.
 * Firmware address: 0x200107a8. */
int pico_api_has_cleaned_ack(char *still_uid)
{
    int api_open_request_result;
    char *context;
    uint32_t value_208;
    char buffer_204[500];

    if ((still_uid == (char *)0x0) || (*still_uid == '\0')) {
        Print_println_string(&global_serial0, "ERROR wifi_picoStillApi ARG picoStillUid");
        return 0;
    }
    Print_print_string(&global_serial0, "wifi_picoStillApi(");
    Print_print_string(&global_serial0, "hasCleanedAck");
    Print_print_string(&global_serial0, ",");
    Print_print_string(&global_serial0, still_uid);
    Print_println_string(&global_serial0, ")");
    value_208 = 0;
    pico_api_write_base_path(buffer_204);
    libc_strcat(buffer_204, "hasCleanedAck");
    libc_strcat(buffer_204, "?picoUid=");
    libc_strcat(buffer_204, &g_device_uid);
    libc_strcat(buffer_204, "&picoStillUid=");
    libc_strcat(buffer_204, still_uid);
    api_open_request_result = pico_api_open_request(buffer_204, 1, 0);
    if (api_open_request_result == 0) {
        context = "connectToApi";
    } else {
        api_open_request_result = pico_api_read_hash_response(buffer_204, 500, &value_208);
        if (api_open_request_result != 0) {
            if (value_208 == 1) {
                if (buffer_204[0] == 'T') {
                    return 1;
                }
                Print_println_string(&global_serial0,
                                     "WARNING Did not get TRUE response from backend");
            } else {
                pico_api_show_error("length != 1");
            }
            return 0;
        }
        context = "readApiResponse";
    }
    pico_api_show_error(context);
    return 0;
}

/* setCleanedAck query with Pico and Still UIDs.
 * Firmware address: 0x200107b4. */
int pico_api_set_cleaned_ack(char *still_uid)
{
    int api_open_request_result;
    char *context;
    uint32_t value_208;
    char buffer_204[500];

    if ((still_uid == (char *)0x0) || (*still_uid == '\0')) {
        Print_println_string(&global_serial0, "ERROR wifi_picoStillApi ARG picoStillUid");
        return 0;
    }
    Print_print_string(&global_serial0, "wifi_picoStillApi(");
    Print_print_string(&global_serial0, "setCleanedAck");
    Print_print_string(&global_serial0, ",");
    Print_print_string(&global_serial0, still_uid);
    Print_println_string(&global_serial0, ")");
    value_208 = 0;
    pico_api_write_base_path(buffer_204);
    libc_strcat(buffer_204, "setCleanedAck");
    libc_strcat(buffer_204, "?picoUid=");
    libc_strcat(buffer_204, &g_device_uid);
    libc_strcat(buffer_204, "&picoStillUid=");
    libc_strcat(buffer_204, still_uid);
    api_open_request_result = pico_api_open_request(buffer_204, 1, 0);
    if (api_open_request_result == 0) {
        context = "connectToApi";
    } else {
        api_open_request_result = pico_api_read_hash_response(buffer_204, 500, &value_208);
        if (api_open_request_result != 0) {
            if (value_208 == 1) {
                if (buffer_204[0] == 'T') {
                    return 1;
                }
                Print_println_string(&global_serial0,
                                     "WARNING Did not get TRUE response from backend");
            } else {
                pico_api_show_error("length != 1");
            }
            return 0;
        }
        context = "readApiResponse";
    }
    pico_api_show_error(context);
    return 0;
}
