/* Pico Pro 0.1.34 — network. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Parses four decimal octets with dot/range checks into IPAddress bytes.
 * Firmware address: 0x2000661c. */
uint16_t pico_parse_ipv4_address(int text, int ip_address)
{
    uint16_t character;
    uint32_t octet_index;
    uint16_t octet_value;
    uint8_t *cursor;

    octet_index = 0;
    octet_value = 0;
    cursor = (uint8_t *)(text + -1);
    while (true) {
        while (true) {
            cursor = cursor + 1;
            character = (uint16_t)*cursor;
            if (character == 0) {
                if (octet_index == 3) {
                    *(char *)(ip_address + 7) = (char)octet_value;
                    return 1;
                }
                return 0;
            }
            if (9 < (character - 0x30 & 0xff))
                break;
            octet_value = (character - 0x30) + octet_value * 10;
            if (0xff < octet_value) {
                return 0;
            }
        }
        if (character != 0x2e)
            break;
        if (octet_index == 3) {
            return 0;
        }
        *(char *)(octet_index + ip_address + 4) = (char)octet_value;
        octet_index = octet_index + 1 & 0xff;
        octet_value = 0;
    }
    return 0;
}

/* Tail wrapper around WiFi driver initialization routine20014a70.
 * Firmware address: 0x20008994. */
uint32_t pico_wifi_ensure_started_thunk(void)
{
    int sl_start_result;
    uint32_t value_2;

    if (wifi_initialized != '\0') {
        return 1;
    }
    unresolved_simplelink_protocol_48();
    sl_start_result = sl_Start(0, 0, 0);
    value_2 = 0;
    if ((1 < sl_start_result + 2U) && (sl_start_result != -3)) {
        if (sl_start_result != 0) {
            sl_WlanSetMode('\0');
            sl_Stop(0);
            sl_Start(0, 0, 0);
        }
        sl_WlanDisconnect();
        value_2 = 1;
        unresolved_simplelink_protocol_31((char *)0x0, '\0');
        wifi_initialized = '\x01';
        sl_NetAppStop(1);
        sl_WlanRxStatStart();
    }
    return value_2;
}

/* Explicit connectToApInternal log; selects WEP/WPA/open paths from stored security type.
 * Firmware address: 0x20008998. */
char pico_wifi_connect_access_point(void)
{
    int low_nibble_class;
    int high_nibble_class;
    int value_3;
    char connection_status;
    int wi_fi_initialize_result;
    uint32_t text_length;
    uint32_t text_length_2;
    int hex_index;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r1_00;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r2_00;
    uint8_t *decoded_key_cursor;
    char security_parameters[4];
    uint8_t *password;
    char security_storage[11];
    uint8_t byte_value_19;

    pico_log_serial("I", "WIFI", "connectToApInternal");
    if (g_wifi_security_type_as_uint8_t < 7) {
        if (g_wifi_security_type_as_uint8_t == 5) {
            pico_log_serial("I", "WIFI", &g_wifi_ssid);
            pico_log_serial("I", "WIFI", &g_wifi_password);
            connection_status =
                WiFi_begin_with_passphrase(&global_wifi, &g_wifi_ssid, 0, &g_wifi_password);
            if (connection_status != '\x03') {
                wi_fi_initialize_result = 0;
                do {
                    if ((&g_wifi_password)[wi_fi_initialize_result] == '\0')
                        goto branch_1;
                    wi_fi_initialize_result = wi_fi_initialize_result + 1;
                } while (wi_fi_initialize_result != 0x20);
                wi_fi_initialize_result = 0;
            branch_1:
                value_3 = -((wi_fi_initialize_result >> 1) + 8U & 0xfffffff8);
                decoded_key_cursor = (uint8_t *)(security_storage + value_3 + 0xb);
                for (hex_index = 0; hex_index < wi_fi_initialize_result;
                     hex_index = hex_index + 2) {
                    low_nibble_class =
                        (int)(uint32_t)(uint8_t)(&g_wifi_password_second_byte)[hex_index] >> 6;
                    high_nibble_class = (int)(uint32_t)(uint8_t)(&g_wifi_password)[hex_index] >> 6;
                    decoded_key_cursor = decoded_key_cursor + 1;
                    *decoded_key_cursor =
                        (char)low_nibble_class + (char)(low_nibble_class << 3) +
                            ((&g_wifi_password_second_byte)[hex_index] & 0xf) |
                        ((char)high_nibble_class + (char)(high_nibble_class << 3) +
                         ((&g_wifi_password)[hex_index] & 0xf)) *
                            '\x10';
                }
                security_storage[(wi_fi_initialize_result >> 1) + value_3 + 0xc] = '\0';
                connection_status = WiFi_begin_with_passphrase(&global_wifi, &g_wifi_ssid, 0,
                                                               security_storage + value_3 + 0xc);
            }
            return connection_status;
        }
        if (wifi_begin_called == '\0') {
            wi_fi_initialize_result = WiFi_initialize();
            if (wi_fi_initialize_result != 0) {
                sl_WlanProfileDel(0xff);
                WiFi_enable_dhcp_if_unconfigured(&global_wifi, unresolved_return_r1_00,
                                                 unresolved_return_r2_00);
                sl_WlanPolicySet('\x10', '\x02', (uint8_t *)0x0, '\0');
                text_length = libc_strlen(&g_wifi_ssid);
                libc_memset(security_parameters, 0, 0xc);
                security_parameters[0] = '\x02';
                password = &g_wifi_password;
                text_length_2 = libc_strlen(&g_wifi_password);
                security_storage[0] = (char)text_length_2;
                wi_fi_initialize_result = unresolved_simplelink_protocol_49(
                    &g_wifi_ssid, (int)(short)text_length, (uint32_t *)0x0, security_parameters,
                    (uint32_t *)0x0);
                if (wi_fi_initialize_result == 0) {
                    unresolved_simplelink_protocol_50(&g_wifi_ssid, (int)(short)text_length,
                                                      (uint32_t *)0x0, security_parameters,
                                                      (uint32_t *)0x0, 6);
                    wifi_begin_called = '\x01';
                    goto branch_2;
                }
            }
            connection_status = '\x04';
        } else {
            delay(500);
        branch_2:
            connection_status = WiFi_status();
        }
        return connection_status;
    }
    if (wifi_begin_called == '\0') {
        wi_fi_initialize_result = WiFi_initialize();
        if (wi_fi_initialize_result != 0) {
            sl_WlanProfileDel(0xff);
            WiFi_enable_dhcp_if_unconfigured(&global_wifi, unresolved_return_r1,
                                             unresolved_return_r2);
            sl_WlanPolicySet('\x10', '\x02', (uint8_t *)0x0, '\0');
            text_length = libc_strlen(&g_wifi_ssid);
            libc_memset(security_storage, 0, 0xc);
            wi_fi_initialize_result = unresolved_simplelink_protocol_49(
                &g_wifi_ssid, (int)(short)text_length, (uint32_t *)0x0, security_storage,
                (uint32_t *)0x0);
            if (wi_fi_initialize_result == 0) {
                unresolved_simplelink_protocol_50(&g_wifi_ssid, (int)(short)text_length,
                                                  (uint32_t *)0x0, security_storage,
                                                  (uint32_t *)0x0, 6);
                wifi_begin_called = '\x01';
                goto branch_3;
            }
        }
        connection_status = '\x04';
    } else {
        delay(500);
    branch_3:
        connection_status = WiFi_status();
    }
    return connection_status;
}

/* Explicit connectToStoredWiFi log; retries stored network connection and polls status.
 * Firmware address: 0x20008a8c. */
uint32_t pico_wifi_connect_stored_network(void)
{
    char connection_status;
    uint32_t started_ms;
    uint32_t now_ms;
    uint8_t attempts;

    pico_log_serial("I", "WIFI", "connectToStoredWiFi");
    connection_status = pico_wifi_connect_access_point();
    if (connection_status != '\x03') {
        attempts = 0;
        do {
            if (wifi_begin_called == '\0') {
                pico_wifi_connect_access_point();
                attempts = attempts + 1;
            } else {
                started_ms = millis();
                do {
                    connection_status = WiFi_status();
                    if (connection_status == '\x03')
                        break;
                    now_ms = millis();
                } while ((uint8_t *)(now_ms - started_ms) < &g_brewing_unknown_03);
                connection_status = WiFi_status();
                if (connection_status == '\x03') {
                    delay(500);
                    return 1;
                }
                WiFi_disconnect();
            }
        } while (attempts < 2);
    }
    connection_status = WiFi_status();
    if (connection_status == '\x03') {
        return 1;
    }
    WiFi_disconnect();
    return 0;
}

/* Draws device image/progress animation then connects stored WiFi.
 * Firmware address: 0x20008b1c. */
int pico_wifi_connect_with_screen(void)
{
    int wifi_connect_stored_network_result;

    pico_display_clear();
    GFX_set_text_wrap(&g_display, false);
    pico_display_device_bitmap();
    pico_display_text_at_cell(2, 8, &g_wifi_ssid);
    GFX_set_text_wrap(&g_display, true);
    pico_display_flush();
    g_progress_indicator = 3;
    g_progress_start_column = 8;
    g_progress_end_column = 0x15;
    g_progress_glyph_index = 0;
    g_progress_active = 1;
    wifi_connect_stored_network_result = pico_wifi_connect_stored_network();
    return wifi_connect_stored_network_result;
}

/* Disconnects network, ensures driver startup, reconnects stored AP.
 * Firmware address: 0x20008b70. */
int pico_wifi_reconnect(void)
{
    int wi_fi_initialize_result;

    WiFi_disconnect();
    if ((wifi_initialized == '\0') &&
        (wi_fi_initialize_result = WiFi_initialize(), wi_fi_initialize_result == 0)) {
        return 0;
    }
    wi_fi_initialize_result = pico_wifi_connect_stored_network();
    return wi_fi_initialize_result;
}

/* While WiFi connected, resolves picobrew.com to nonzero address with30s timeout.
 * Firmware address: 0x20008b98. */
bool pico_wifi_resolve_server_address(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                      int argument_4)
{
    char connection_status;
    uint32_t started_ms;
    uint32_t now_ms;
    int dns_result;
    uint32_t value_18;
    int zero_ip;

    if ((wifi_initialized != '\0') &&
        (value_18 = argument_3, zero_ip = argument_4, connection_status = WiFi_status(),
         connection_status == '\x03')) {
        started_ms = millis();
        IPAddress_construct_octets(&value_18, 0, 0, 0, 0);
        while ((g_server_ipv4_value == zero_ip &&
                (now_ms = millis(), (uint8_t *)(now_ms - started_ms) < &g_brewing_unknown_07))) {
            dns_result = WiFi_host_by_name(&global_wifi, "picobrew.com", 0x2002dac8);
            if (dns_result < 0) {
                g_server_ipv4_value = zero_ip;
                return false;
            }
            delay(500);
        }
        now_ms = millis();
        if ((uint8_t *)(now_ms - started_ms) < &g_brewing_unknown_07) {
            return g_server_ipv4_value != zero_ip;
        }
    }
    return false;
}

/* Constructs ping request for8.8.4.4 with5attempts/1s timeout; returns report byte.
 * Firmware address: 0x20008c28. */
uint8_t pico_wifi_ping_public_dns(void)
{
    uint32_t value;
    uint8_t ping_status;
    uint32_t ping_parameters;
    uint16_t payload_size;
    uint16_t timeout_ms;
    uint32_t attempt_count;
    uint32_t flags;
    uint32_t target_ip;

    target_ip = 0x8080404;
    payload_size = 0x38;
    ping_parameters = 0x19;
    timeout_ms = 1000;
    attempt_count = 5;
    flags = 0;
    sl_NetAppPingStart(&ping_parameters, 2, &value, 0);
    return ping_status;
}

/* Copies literal /API/pico/ into URL buffer.
 * Firmware address: 0x20008c64. */
void pico_api_write_base_path(char *url)
{
    libc_strcpy(url, "/API/pico/");
    return;
}

/* Appends uid= followed by generated device UID.
 * Firmware address: 0x20008c70. */
void pico_api_append_device_uid(char *url)
{
    libc_strcat(url, "uid=");
    libc_strcat(url, &g_device_uid);
    return;
}

/* Writes HTTP/1.0 Host and Connection: close framing through WiFiClient.
 * Firmware address: 0x20008e68. */
void pico_http_write_request_headers(uint32_t argument_1, int argument_2, uint32_t argument_3,
                                     uint32_t argument_4)
{
    int value_14;
    uint32_t value_10;
    uint32_t value;

    value_14 = argument_2;
    value_10 = argument_3;
    value = argument_4;
    delay(5);
    String_construct_c_string(&value_14, " HTTP/1.0\r\nHost: ");
    Print_print_String(&g_http_client, &value_14);
    String_destroy(&value_14);
    delay(5);
    Print_print_string(&g_http_client, "picobrew.com");
    delay(5);
    String_construct_c_string(&value_14, "\nConnection: close\n\n");
    Print_print_String(&g_http_client, &value_14);
    String_destroy(&value_14);
    delay(5);
    return;
}
