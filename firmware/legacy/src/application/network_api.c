/* Pico Pro 0.1.34 — network api. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Checks initialized/associated/local-IP/DNS conditions and returns firmware status codes.
 * Firmware address: 0x2000bc50. */
int pico_wifi_check_status(int background, int resolve_server)
{
    char wi_fi_status_result;
    bool wifi_resolve_server_address_result;
    uint8_t wi_fi_client_status_result;
    int wi_fi_initialize_result;
    int unresolved_input_r3;
    uint64_t ipaddress_construct_octets_result;
    uint32_t value_18;
    int value_14;

    value_14 = unresolved_input_r3;
    if ((wifi_initialized == '\0') &&
        (wi_fi_initialize_result = WiFi_initialize(), wi_fi_initialize_result == 0)) {
        return 9;
    }
    wi_fi_status_result = WiFi_status();
    if (wi_fi_status_result != '\x03') {
        wi_fi_initialize_result = pico_wifi_reconnect();
        if (wi_fi_initialize_result == 0) {
            return 10;
        }
        if (background != 0) {
            return 0;
        }
    }
    WiFi_get_local_ip(&value_18);
    if (value_14 == any_ip_address_value) {
        wi_fi_initialize_result = 0xb;
    } else {
        wifi_resolve_server_address_result = false;
        if (resolve_server == 0) {
            wifi_resolve_server_address_result = false;
        } else {
            ipaddress_construct_octets_result =
                PICO_UNRESOLVED_RETURN(IPAddress_construct_octets(&value_18, 0, 0, 0, 0));
            if (g_server_ipv4_value == value_14) {
                wifi_resolve_server_address_result = pico_wifi_resolve_server_address(
                    (int)ipaddress_construct_octets_result,
                    (int)((uint64_t)ipaddress_construct_octets_result >> 0x20), g_server_ipv4_value,
                    value_14);
                if (!wifi_resolve_server_address_result) {
                    return 3;
                }
                wifi_resolve_server_address_result = true;
            }
        }
        wi_fi_client_status_result = WiFiClient_status(&g_http_client);
        if (wi_fi_client_status_result != '\0') {
            Print_println_string(&global_serial0, "WARNING: wClient disconnected!");
            WiFiClient_stop(&g_http_client);
        }
        if (wifi_resolve_server_address_result) {
            if (background == 0) {
                wi_fi_initialize_result = 1;
            } else {
                wi_fi_initialize_result = 2;
            }
        } else {
            wi_fi_initialize_result = 1;
        }
    }
    return wi_fi_initialize_result;
}

/* If WiFiClient connected, flushes and stops it.
 * Firmware address: 0x2000bd10. */
void pico_api_close_connection(void)
{
    uint8_t wi_fi_client_status_result;

    wi_fi_client_status_result = WiFiClient_status(&g_http_client);
    if (wi_fi_client_status_result == '\0') {
        return;
    }
    WiFiClient_flush(&g_http_client);
    WiFiClient_stop(&g_http_client);
    return;
}

/* Framed response reader: skips to hash, copies through caller-selected terminator with timeout.
 * Firmware address: 0x2000bd30. */
/* Firmware entry: 0x2000bd30. Reviewed source reconstruction.
 * Preserves the firmware's supplied length, deadline arithmetic, post-write
 * capacity check, and terminator handling. No bounds repair is applied here.
 */
int pico_read_api_response(char *buffer, uint32_t capacity, uint32_t *length, int close_after,
                           uint8_t terminator)
{
    bool frame_started = false;
    const char *error_message;

    Print_print_string(&global_serial0, "readApiResponse() ");
    Print_print_string(&global_serial0, " [closeConnection = ");
    Print_print_string(&global_serial0, close_after ? "true" : "false");
    Print_println_string(&global_serial0, "]");
    uint32_t read_started_ms = millis();

    for (;;) {
        while (WiFiClient_available(&g_http_client) == 0) {
            if (WiFiClient_status(&g_http_client) == 0) {
                error_message = "ERROR wClient disconnected!";
                goto failed;
            }
            if ((uint32_t)(read_started_ms + 30000) < millis()) {
                error_message = "ERROR timeout";
                goto failed;
            }
        }

        uint32_t input_byte = WiFiClient_read_byte(&g_http_client);
        if (input_byte > 255) {
            error_message = " wClient.read() ERROR";
            goto failed;
        }
        Print_print_char(&global_serial0, (char)input_byte);

        if (!frame_started) {
            frame_started = (input_byte == '#');
            continue;
        }

        uint32_t write_index = *length;
        if (input_byte == terminator) {
            buffer[write_index] = '\0';
            Print_print_string(&global_serial0, "RECV [");
            Print_print_string(&global_serial0, buffer);
            Print_print_string(&global_serial0, "] ");
            Print_print_signed_alias(&global_serial0, *length, 10);
            Print_println_string(&global_serial0, " Bytes");
            if (close_after) {
                pico_api_close_connection();
            }
            return 1;
        }

        *length = write_index + 1;
        buffer[write_index] = (char)input_byte;
        if (capacity < *length) {
            error_message = "ERROR data exceeds buffer size";
            goto failed;
        }
    }

failed:
    Print_println_string(&global_serial0, (char *)error_message);
    pico_api_close_connection();
    return 0;
}

/* Polls WiFi local IP until assigned or timeout, optionally showing IP error.
 * Firmware address: 0x2000dc64. */
int pico_wifi_wait_for_local_ip(uint32_t timeout_ms, int show_error)
{
    uint32_t now_ms;
    uint32_t now_ms_2;
    int value_3;
    uint32_t value_20;
    int value;

    value_20 = timeout_ms;
    value = show_error;
    now_ms = millis();
    while (
        (WiFi_get_local_ip(&value_20),
         value == any_ip_address_value && (now_ms_2 = millis(), now_ms_2 - now_ms < timeout_ms))) {
        delay(300);
    }
    WiFi_get_local_ip(&value_20);
    if ((value == any_ip_address_value) && (show_error != 0)) {
        pico_report_error(0xb, 1, 0);
        value_3 = 0;
    } else {
        value_3 = 1;
    }
    return value_3;
}

/* Waits for local IP and resolves server, retrying after setting fallback DNS8.8.8.8.
 * Firmware address: 0x2000dd58. */
int pico_wifi_resolve_server(void)
{
    bool wifi_resolve_server_address_result;
    int wifi_wait_for_local_ip_result;
    uint32_t value_3;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r2_00;
    int unresolved_return_r3;
    int unresolved_return_r3_00;
    uint64_t wi_fi_set_dns_result;
    uint32_t buffer_18[2];

    IPAddress_construct_octets(buffer_18, 8, 8, 8, 8);
    wifi_wait_for_local_ip_result = pico_wifi_wait_for_local_ip(45000, 1);
    wifi_resolve_server_address_result =
        pico_wifi_resolve_server_address(wifi_wait_for_local_ip_result, unresolved_return_r1,
                                         unresolved_return_r2, unresolved_return_r3);
    value_3 = (uint32_t)wifi_resolve_server_address_result;
    if (value_3 == 0) {
        wi_fi_set_dns_result =
            PICO_UNRESOLVED_RETURN(WiFi_set_dns(&global_wifi, (int)(buffer_18 + 2)));
        wifi_resolve_server_address_result = pico_wifi_resolve_server_address(
            (int)wi_fi_set_dns_result, (int)((uint64_t)wi_fi_set_dns_result >> 0x20),
            unresolved_return_r2_00, unresolved_return_r3_00);
        value_3 = (uint32_t)wifi_resolve_server_address_result;
    }
    return value_3;
}

/* Logs API failure context and displays server communication error7.
 * Firmware address: 0x2000dd9c. */
void pico_api_show_error(char *context)
{
    Print_print_string(&global_serial0, "SERVER_COMMS_ERROR ");
    Print_println_string(&global_serial0, context);
    pico_report_error(7, 1, 0);
    return;
}

/* Checks WiFi, connects to API IP port80 and writes HTTP GET request and headers.
 * Firmware address: 0x2000ddc8. */
int pico_api_open_request(char *request, int keep_open, int background)
{
    int wifi_check_status_result;
    uint32_t print_print_string_result;
    int unresolved_return_r1;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;
    uint8_t **pointer_24;
    uint32_t value_20;

    Print_println_string(&global_serial0, "in connectToApi");
    wifi_check_status_result = pico_wifi_check_status(background, 1);
    if (background == 0) {
        print_print_string_result = wifi_check_status_result - 3U & 0xff;
        if ((print_print_string_result < 9) && ((1 << print_print_string_result & 0x1c1U) != 0)) {
            pico_report_error(wifi_check_status_result, 1, 0);
            return 0;
        }
    } else if (wifi_check_status_result != 1) {
        return 0;
    }
    Print_println_string(&global_serial0, "sending: ");
    Print_println_string(&global_serial0, request);
    if (background == 0) {
        wifi_check_status_result = 0x18;
    } else {
        wifi_check_status_result = 0x38;
    }
    pointer_24 = &g_network_addresses_unknown_01;
    value_20 = g_server_ipv4_value_as_uint32_t;
    wifi_check_status_result =
        (**(PicoOpaqueFunction **)(g_http_client + wifi_check_status_result))(&g_http_client,
                                                                              &pointer_24, 0x50);
    if (wifi_check_status_result == 0) {
        pico_api_close_connection();
        return 0;
    }
    String_construct_c_string((int *)&pointer_24, "GET ");
    Print_print_String(&g_http_client, &pointer_24);
    String_destroy(&pointer_24);
    delay(5);
    print_print_string_result = Print_print_string(&g_http_client, request);
    pico_http_write_request_headers(print_print_string_result, unresolved_return_r1,
                                    unresolved_return_r2, unresolved_return_r3);
    if (keep_open == 0) {
        pico_api_close_connection();
    }
    return 1;
}

/* Builds picoChangeState with picoUID and state parameters.
 * Firmware address: 0x2000de98. */
void pico_api_report_device_state(void)
{
    char *source;
    char buffer_210[20];
    char buffer[500];

    pico_api_write_base_path(buffer);
    libc_strcat(buffer, "picoChangeState?picoUID=");
    libc_strcat(buffer, &g_device_uid);
    libc_strcat(buffer, "&state=");
    source = itoa((uint32_t)g_device_state_as_uint8_t, buffer_210, 10);
    libc_strcat(buffer, source);
    pico_api_open_request(buffer, 0, 1);
    return;
}

/* Builds error query with error code and RFID.
 * Firmware address: 0x2000dfec. */
int pico_api_report_error(int error_code)
{
    char *source;
    int api_open_request_result;
    char buffer_208[12];
    char buffer[500];

    pico_api_write_base_path(buffer);
    libc_strcat(buffer, "error?");
    pico_api_append_device_uid(buffer);
    libc_strcat(buffer, "&code=");
    source = itoa(error_code, buffer_208, 10);
    libc_strcat(buffer, source);
    libc_strcat(buffer, "&rfid=");
    libc_strcat(buffer, &g_recipe_identifier);
    api_open_request_result = pico_api_open_request(buffer, 0, 0);
    return api_open_request_result;
}

/* Tries stored credentials then repeatedly prompts scanned/manual connection until successful and
 * saves credentials. Firmware address: 0x2000fc4c. */
uint32_t pico_wifi_ensure_connection(void)
{
    int storage_load_wifi_credentials_result;
    int wifi_resolve_server_result;
    uint32_t unresolved_return_r0;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r1_00;
    uint32_t unresolved_return_r1_01;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r2_00;
    uint32_t unresolved_return_r2_01;
    uint32_t unresolved_return_r2_02;
    uint32_t value_3;
    uint32_t unresolved_return_r3;
    uint32_t unresolved_return_r3_00;
    uint32_t value_4;
    uint64_t value_5;

    pico_display_clear();
    pico_display_device_bitmap();
    pico_display_flush();
    storage_load_wifi_credentials_result = pico_storage_load_wifi_credentials();
    if (storage_load_wifi_credentials_result == 0) {
        storage_load_wifi_credentials_result =
            pico_ui_scan_and_connect_wifi(0, unresolved_return_r1, unresolved_return_r2);
    } else {
        storage_load_wifi_credentials_result = pico_wifi_connect_with_screen();
        if ((storage_load_wifi_credentials_result != 0) &&
            (wifi_resolve_server_result = pico_wifi_resolve_server(),
             wifi_resolve_server_result == 0)) {
            pico_display_stop_progress_indicator(&g_progress_indicator);
            pico_display_device_bitmap_variant_b();
            pico_display_flush();
            delay(1000);
            pico_display_clear();
            pico_diagnostic_wifi();
        }
        pico_display_stop_progress_indicator(&g_progress_indicator);
    }
    if (storage_load_wifi_credentials_result == 0) {
        do {
            pico_display_device_bitmap_variant_b();
            pico_display_flush();
            delay(1000);
            storage_load_wifi_credentials_result = pico_ui_scan_and_connect_wifi(
                unresolved_return_r0, unresolved_return_r1_00, unresolved_return_r2_01);
            value_5 =
                pico_join_u32_u32(unresolved_return_r1_01, storage_load_wifi_credentials_result);
            value_3 = unresolved_return_r2_02;
            value_4 = unresolved_return_r3_00;
        } while (storage_load_wifi_credentials_result == 0);
    } else {
        pico_display_device_bitmap_variant_a();
        value_5 = PICO_UNRESOLVED_RETURN(pico_display_flush());
        value_3 = unresolved_return_r2_00;
        value_4 = unresolved_return_r3;
    }
    pico_settings_save_wifi((int)value_5, (int)((uint64_t)value_5 >> 0x20), value_3, value_4);
    return 1;
}

/* Response wrapper closing connection with hash terminator.
 * Firmware address: 0x200105e4. */
int pico_api_read_hash_response(char *buffer, uint32_t capacity, uint32_t *length)
{
    int read_api_response_result;

    read_api_response_result = pico_read_api_response(buffer, capacity, length, 1, '#');
    return read_api_response_result;
}

/* register query, registration-code display and registered/not-found response handling.
 * Firmware address: 0x200105f8. */
uint32_t pico_api_register_device(void)
{
    int api_open_request_result;
    int report_to_server;
    uint32_t value_200;
    char buffer[2];
    char buffer_2[498];

    value_200 = 0;
    pico_api_write_base_path(buffer);
    libc_strcat(buffer, "register?");
    pico_api_append_device_uid(buffer);
    api_open_request_result = pico_api_open_request(buffer, 1, 0);
    if ((api_open_request_result != 0) &&
        (api_open_request_result = pico_api_read_hash_response(buffer, 500, &value_200),
         api_open_request_result != 0)) {
        if ((value_200 != 1) || (buffer[0] != 'F')) {
            if (buffer[0] != 'F') {
                return 1;
            }
            if (4 < (int)value_200) {
                pico_ui_draw_registration_code(buffer_2);
                pico_display_flush();
                delay(15000);
                return 0;
            }
            api_open_request_result = 0xd;
            report_to_server = 1;
            goto branch_1;
        }
        pico_report_error(6, 0, 1);
    }
    api_open_request_result = 7;
    report_to_server = 0;
branch_1:
    pico_report_error(api_open_request_result, 1, report_to_server);
    return 0;
}

/* getSession query with session type; validates and copies returned session ID.
 * Firmware address: 0x200107c0. */
int pico_api_get_session_id(char *session_id, uint32_t capacity)
{
    uint32_t condition;
    int request_succeeded;
    uint32_t response_length;
    char session_type_query[16];
    char request_and_response[500];

    condition = 0;
    if (session_id != (char *)0x0) {
        condition = (uint32_t)(capacity != 0);
    }
    pico_assert_nonfatal(condition, "buffer != NULL && bufferSize > 0");
    libc_snprintf(session_type_query, 0x10, "&sesType=%d",
                  *(int *)(g_active_program_context_as_int + 0x30));
    response_length = 0;
    pico_api_write_base_path(request_and_response);
    libc_strcat(request_and_response, "getSession?");
    pico_api_append_device_uid(request_and_response);
    libc_strcat(request_and_response, session_type_query);
    request_succeeded = pico_api_open_request(request_and_response, 1, 0);
    if ((((request_succeeded == 0) || (request_succeeded = pico_api_read_hash_response(
                                           request_and_response, 500, &response_length),
                                       request_succeeded == 0)) ||
         (response_length == 0)) ||
        (0x15 < (int)response_length)) {
        pico_report_error(7, 1, 0);
        request_succeeded = 0;
    } else {
        pico_cstring_copy(session_id, capacity, request_and_response);
        request_succeeded = 1;
    }
    return request_succeeded;
}

/* createPicoPak query for recipe ID, stores returned RFID.
 * Firmware address: 0x20010894. */
uint32_t pico_api_create_picopak(int recipe_id)
{
    char *source;
    int api_open_request_result;
    uint32_t value_2;
    uint32_t response_length;
    char recipe_id_text[20];
    char request_and_response[504];

    response_length = 0;
    pico_api_write_base_path(request_and_response);
    libc_strcat(request_and_response, "createPicoPak?");
    pico_api_append_device_uid(request_and_response);
    libc_strcat(request_and_response, "&recipeid=");
    source = itoa(recipe_id, recipe_id_text, 10);
    libc_strcat(request_and_response, source);
    api_open_request_result = pico_api_open_request(request_and_response, 1, 0);
    if ((((api_open_request_result == 0) || (api_open_request_result = pico_api_read_hash_response(
                                                 request_and_response, 500, &response_length),
                                             api_open_request_result == 0)) ||
         (response_length == 0)) ||
        (0x14 < (int)response_length)) {
        value_2 = 0;
    } else {
        libc_strcpy(&g_recipe_identifier, request_and_response);
        value_2 = 1;
    }
    return value_2;
}

/* getActionsNeeded query; parses comma-separated action IDs into output array.
 * Firmware address: 0x20010914. */
int pico_api_get_required_actions(uint32_t *action_ids, uint32_t capacity)
{
    int request_succeeded;
    uint32_t action_id;
    int action_count;
    uint32_t response_length;
    int parse_position;
    uint8_t action_text[28];
    char request_and_response[500];

    action_count = 0;
    response_length = 0;
    parse_position = 0;
    pico_api_write_base_path(request_and_response);
    libc_strcat(request_and_response, "getActionsNeeded?");
    pico_api_append_device_uid(request_and_response);
    request_succeeded = pico_api_open_request(request_and_response, 1, 0);
    if (((request_succeeded == 0) || (request_succeeded = pico_api_read_hash_response(
                                          request_and_response, 500, &response_length),
                                      request_succeeded == 0)) ||
        ((int)response_length < 1)) {
        action_count = 0;
    } else {
        libc_strcat(request_and_response, ",");
        while ((request_and_response[parse_position] != '\0' && (action_count < (int)capacity))) {
            pico_parse_delimited_field(request_and_response, (char *)action_text, &parse_position,
                                       ",", capacity);
            if (action_text[0] - 0x30 < 10) {
                action_id = libc_atoi((char *)action_text);
                action_ids[action_count] = action_id;
                action_count = action_count + 1;
            }
        }
    }
    return action_count;
}

/* Decodes pairs of hex digits into a1024-byte image through supplied terminator.
 * Firmware address: 0x200109b4. */
int pico_api_read_hex_image(uint8_t *image, uint8_t terminator)
{
    uint8_t socket_connected;
    uint32_t last_byte_ms;
    int available_bytes;
    uint32_t input_character;
    uint32_t decoded_byte_or_now_ms;
    uint32_t unresolved_input_register_00000025;
    int hex_pair_index;
    int image_bytes;
    int hex_characters;
    char hex_pair[52];

    last_byte_ms = millis();
    hex_characters = 0;
    hex_pair_index = 0;
    image_bytes = 0;
    do {
        while (true) {
            socket_connected = WiFiClient_status(&g_http_client);
            if (socket_connected == '\0')
                goto branch_1;
            available_bytes = WiFiClient_available(&g_http_client);
            if (available_bytes == 0)
                break;
            last_byte_ms = millis();
            input_character = WiFiClient_read_byte(&g_http_client);
            if ((input_character & 0xff) == 0x23)
                goto branch_1;
            if ((input_character & 0xff) ==
                pico_join_u24_u8(unresolved_input_register_00000025, terminator)) {
                if ((hex_characters >> 1 == 0x400) || (image_bytes == 0x400)) {
                    return 1;
                }
                goto branch_1;
            }
            hex_pair[hex_pair_index] = (char)input_character;
            hex_pair[hex_pair_index + 1] = '\0';
            hex_characters = hex_characters + 1;
            if (hex_pair_index == 1) {
                if (0x400 < image_bytes)
                    goto branch_1;
                decoded_byte_or_now_ms = libc_strtoul(hex_pair, (char **)0x0, 0x10);
                image[image_bytes] = (uint8_t)decoded_byte_or_now_ms;
                hex_pair_index = 0;
                image_bytes = image_bytes + 1;
            } else {
                hex_pair_index = 1;
            }
        }
        decoded_byte_or_now_ms = millis();
    } while ((uint8_t *)(decoded_byte_or_now_ms - last_byte_ms) < &g_network_api_unknown_02);
branch_1:
    pico_api_close_connection();
    return 0;
}

/* Hex-image reader wrapper with pipe delimiter.
 * Firmware address: 0x20010a64. */
int pico_api_read_recipe_bitmap(uint8_t *image)
{
    int api_read_hex_image_result;

    api_read_hex_image_result = pico_api_read_hex_image(image, '|');
    return api_read_hex_image_result;
}

/* getRecipe query with RFID/IBU/ABV; parses recipe text and following bitmaps.
 * Firmware address: 0x20010a6c. */
uint32_t pico_api_fetch_recipe(char *recipe_identifier, int ibu_adjustment, int abv_adjustment)
{
    uint32_t value_1;
    char *itoa_result;
    float __aeabi_i2f_result;
    int api_open_request_result;
    uint32_t value_5;
    int api_read_recipe_bitmap_result;
    uint8_t *image;
    uint32_t value_220;
    char buffer_2[16];
    char buffer[504];

    value_220 = 0;
    pico_api_write_base_path(buffer);
    libc_strcat(buffer, "getRecipe?");
    pico_api_append_device_uid(buffer);
    libc_strcat(buffer, "&rfid=");
    libc_strcat(buffer, recipe_identifier);
    libc_strcat(buffer, "&ibu=");
    itoa_result = itoa(ibu_adjustment, buffer_2, 10);
    libc_strcat(buffer, itoa_result);
    libc_strcat(buffer, "&abv=");
    __aeabi_i2f_result = __aeabi_i2f(abv_adjustment);
    itoa_result = pico_format_float_decimal(__aeabi_i2f_result, buffer_2, 1);
    libc_strcat(buffer, itoa_result);
    api_open_request_result = pico_api_open_request(buffer, 1, 0);
    if (((api_open_request_result == 0) ||
         (api_open_request_result = pico_read_api_response(buffer, 500, &value_220, 0, '|'),
          value_1 = value_220, api_open_request_result == 0)) ||
        ((int)value_220 < 1)) {
        pico_report_error(7, 1, 0);
        value_5 = 7;
    } else {
        api_open_request_result = libc_strcmp(buffer, "Invalid");
        if (api_open_request_result == 0) {
            pico_report_error(0xe, 1, 1);
            value_5 = 0xe;
        } else {
            api_open_request_result = pico_recipe_parse_server_response(buffer, value_1);
            if (api_open_request_result == 0) {
                pico_report_error(0xd, 1, 1);
                value_5 = 0xd;
            } else {
                api_open_request_result = pico_api_read_hex_image(&g_network_api_unknown_04, '|');
                value_5 = 0;
                if (api_open_request_result != 0) {
                    g_carousel_page_callbacks = &LAB_20009a60_1;
                    api_open_request_result = g_carousel_page_count_as_int + -4;
                    image = &g_network_api_unknown_03 + api_open_request_result * 0x400;
                    while (((api_open_request_result < 4 && (g_carousel_page_count_as_int < 0xf)) &&
                            (api_read_recipe_bitmap_result = pico_api_read_recipe_bitmap(image),
                             api_read_recipe_bitmap_result != 0))) {
                        switch (api_open_request_result) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                            api_read_recipe_bitmap_result = g_carousel_page_count_as_int + 4;
                            g_carousel_page_count_as_int = g_carousel_page_count_as_int + 1;
                            (&g_carousel_page_index)[api_read_recipe_bitmap_result] =
                                &LAB_20004788_1;
                        }
                        g_carousel_page_count_as_int = g_carousel_page_count_as_int + 1;
                        api_open_request_result = api_open_request_result + 1;
                        image = image + 0x400;
                    }
                    value_5 = 0;
                }
            }
        }
    }
    return value_5;
}

/* Calls recipe fetch with -1 defaults and returns nonzero status comparison.
 * Firmware address: 0x20010f0c. */
bool pico_api_fetch_default_recipe_failed(char *recipe_identifier)
{
    int api_fetch_recipe_result;

    api_fetch_recipe_result = pico_api_fetch_recipe(recipe_identifier, -1, -1);
    return api_fetch_recipe_result != 0;
}

/* Reads socket until hash marker or timeout/disconnect.
 * Firmware address: 0x20010f20. */
uint32_t pico_api_wait_for_hash(void)
{
    uint8_t wi_fi_client_status_result;
    uint32_t now_ms;
    int available_bytes;
    uint32_t wi_fi_client_read_byte_result;
    uint32_t now_ms_2;

    now_ms = millis();
    do {
        while (true) {
            wi_fi_client_status_result = WiFiClient_status(&g_http_client);
            if (wi_fi_client_status_result == '\0')
                goto branch_1;
            available_bytes = WiFiClient_available(&g_http_client);
            if (available_bytes == 0)
                break;
            wi_fi_client_read_byte_result = WiFiClient_read_byte(&g_http_client);
            if ((wi_fi_client_read_byte_result & 0xff) == 0x23) {
                return 1;
            }
        }
        now_ms_2 = millis();
    } while ((uint8_t *)(now_ms_2 - now_ms) < &g_network_api_unknown_02);
branch_1:
    pico_api_close_connection();
    return 0;
}

/* HTTP GET transport with optional response-marker wait and failure log.
 * Firmware address: 0x20010f64. */
int pico_api_make_wifi_call(char *request, int wait_for_response, int background)
{
    int wifi_check_status_result;
    uint32_t print_print_string_result;
    int unresolved_return_r1;
    uint32_t unresolved_return_r2;
    char *message;
    uint32_t unresolved_return_r3;
    uint8_t **pointer_98;
    uint32_t value_94;

    wifi_check_status_result = pico_wifi_check_status(background, 1);
    if (background == 0) {
        print_print_string_result = wifi_check_status_result - 3U & 0xff;
        if ((8 < print_print_string_result) || ((1 << print_print_string_result & 0x1c1U) == 0)) {
            wifi_check_status_result = 0x18;
            goto branch_1;
        }
    } else if (wifi_check_status_result == 1) {
        wifi_check_status_result = 0x38;
    branch_1:
        pointer_98 = &g_network_addresses_unknown_01;
        value_94 = g_server_ipv4_value_as_uint32_t;
        wifi_check_status_result =
            (**(PicoOpaqueFunction **)(g_http_client + wifi_check_status_result))(
                &g_http_client, &pointer_98, 0x50);
        if (wifi_check_status_result != 0) {
            String_construct_c_string((int *)&pointer_98, "GET ");
            Print_print_String(&g_http_client, &pointer_98);
            String_destroy(&pointer_98);
            delay(5);
            print_print_string_result = Print_print_string(&g_http_client, request);
            pico_http_write_request_headers(print_print_string_result, unresolved_return_r1,
                                            unresolved_return_r2, unresolved_return_r3);
            if (wait_for_response == 0) {
                pico_api_close_connection();
                return 1;
            }
            wifi_check_status_result = pico_api_wait_for_hash();
            return wifi_check_status_result;
        }
        message = "connect failed";
        goto branch_2;
    }
    libc_snprintf((char *)&pointer_98, 0x80, "%s %d", "checkWifiStatus", wifi_check_status_result);
    message = (char *)&pointer_98;
branch_2:
    pico_log_serial("E", "WIFI", message);
    return 0;
}

/* Builds log query carrying session, temperatures, step/event, error and time-left fields.
 * Firmware address: 0x20011034. */
int pico_wifi_log_event(char *step_name, char *event_name)
{
    char *formatted_number;
    float telemetry_value;
    char *session_or_formatted_number;
    int temperature_or_request_result;
    char session_type_query[16];
    char number_text[20];
    char encoded_step_name[32];
    char encoded_event_name[32];
    char request[500];

    session_or_formatted_number = g_active_program_context_as_char_pointer;
    libc_snprintf(session_type_query, 0x10, "&sesType=%d",
                  *(int *)(g_active_program_context_as_char_pointer + 0x30));
    pico_url_encode(step_name, encoded_step_name);
    if (g_api_log_consecutive_failures < 5) {
        pico_api_write_base_path(request);
        libc_strcat(request, "log?");
        pico_api_append_device_uid(request);
        libc_strcat(request, "&sesId=");
        libc_strcat(request, session_or_formatted_number);
        libc_strcat(request, "&wort=");
        temperature_or_request_result = __aeabi_d2iz((double)pico_join_u32_u32(
            g_wort_temperature_fahrenheit_high, g_wort_temperature_fahrenheit_low));
        formatted_number = itoa(temperature_or_request_result, number_text, 10);
        libc_strcat(request, formatted_number);
        libc_strcat(request, "&therm=");
        temperature_or_request_result = __aeabi_d2iz((double)pico_join_u32_u32(
            g_steam_temperature_fahrenheit_high, g_steam_temperature_fahrenheit_low));
        formatted_number = itoa(temperature_or_request_result, number_text, 10);
        libc_strcat(request, formatted_number);
        libc_strcat(request, "&step=");
        libc_strcat(request, encoded_step_name);
        if (event_name != (char *)0x0) {
            pico_url_encode(event_name, encoded_event_name);
            libc_strcat(request, "&event=");
            libc_strcat(request, encoded_event_name);
        }
        libc_strcat(request, "&error=");
        formatted_number = itoa((uint32_t)g_controller_status, number_text, 10);
        libc_strcat(request, formatted_number);
        libc_strcat(request, session_type_query);
        libc_strcat(request, "&timeLeft=");
        formatted_number = itoa(g_step_time_seconds, number_text, 10);
        libc_strcat(request, formatted_number);
        libc_strcat(request, "&shutScale=");
        telemetry_value =
            __aeabi_d2f((double)pico_join_u32_u32(g_shuttle_scale_high, g_shuttle_scale_low));
        formatted_number = pico_format_float_decimal(telemetry_value, number_text, 2);
        libc_strcat(request, formatted_number);
        if (*(int *)(session_or_formatted_number + 0x30) == 3) {
            libc_strcat(request, "&t1=");
            telemetry_value = __aeabi_d2f(
                (double)pico_join_u32_u32(g_still_temperature1_high, g_still_temperature1_low));
            session_or_formatted_number =
                pico_format_float_decimal(telemetry_value, number_text, 1);
            libc_strcat(request, session_or_formatted_number);
            libc_strcat(request, "&t2=");
            telemetry_value = __aeabi_d2f(
                (double)pico_join_u32_u32(g_still_temperature2_high, g_still_temperature2_low));
            session_or_formatted_number =
                pico_format_float_decimal(telemetry_value, number_text, 1);
            libc_strcat(request, session_or_formatted_number);
            libc_strcat(request, "&p=");
            telemetry_value =
                __aeabi_d2f((double)pico_join_u32_u32(g_still_pressure_high, g_still_pressure_low));
            session_or_formatted_number =
                pico_format_float_decimal(telemetry_value, number_text, 2);
            libc_strcat(request, session_or_formatted_number);
        }
        temperature_or_request_result = pico_api_make_wifi_call(request, 0, 1);
        if (temperature_or_request_result == 0) {
            pico_log_serial("E", "WIFI", "makeWiFiCall() failed");
            g_api_log_consecutive_failures = g_api_log_consecutive_failures + 1;
        } else {
            g_api_log_consecutive_failures = 0;
        }
    } else {
        temperature_or_request_result = 0;
        WiFi_disconnect();
        pico_display_refresh_current_page();
        g_api_log_consecutive_failures = 0;
    }
    return temperature_or_request_result;
}

/* Logs same label as step and event while respecting offline mode.
 * Firmware address: 0x200112a8. */
int pico_wifi_log_step(char *step_name)
{
    uint32_t cstring_is_null_or_empty_result;
    int wifi_log_event_result;
    int buffer[4];

    cstring_is_null_or_empty_result = pico_cstring_is_null_or_empty(step_name);
    pico_assert_nonfatal((cstring_is_null_or_empty_result ^ 1) & 0xff,
                         "!PicoBrew::CString::isNullOrEmpty(stepName)");
    if (g_offline_mode == '\0') {
        wifi_log_event_result = pico_wifi_log_event(step_name, step_name);
    } else {
        String_construct_c_string(buffer, "wifi_log() disabled as g_offlineMode is True");
        Print_println_String(&global_serial0, buffer);
        String_destroy(buffer);
        wifi_log_event_result = 1;
    }
    return wifi_log_event_result;
}

/* Step-only logger wrapper with null event argument, same offline handling.
 * Firmware address: 0x2001132c. */
int pico_wifi_log_progress(char *step_name)
{
    uint32_t cstring_is_null_or_empty_result;
    int wifi_log_event_result;
    int buffer[4];

    cstring_is_null_or_empty_result = pico_cstring_is_null_or_empty(step_name);
    pico_assert_nonfatal((cstring_is_null_or_empty_result ^ 1) & 0xff,
                         "!PicoBrew::CString::isNullOrEmpty(stepName)");
    if (g_offline_mode == '\0') {
        wifi_log_event_result = pico_wifi_log_event(step_name, (char *)0x0);
    } else {
        String_construct_c_string(buffer, "wifi_log() disabled as g_offlineMode is True");
        Print_println_String(&global_serial0, buffer);
        String_destroy(buffer);
        wifi_log_event_result = 1;
    }
    return wifi_log_event_result;
}

/* Parses comma/pipe-delimited list records with pagination and selection metadata.
 * Firmware address: 0x20012968. */
uint32_t pico_api_parse_recipe_list(int *total_items, int *page_offset, int *menu_item_count,
                                    int item_identifiers, char *request_and_record_buffer,
                                    int item_titles, int *loaded_items)
{
    uint8_t socket_connected;
    int api_make_wifi_call_result;
    uint32_t read_started_ms;
    uint32_t now_ms;
    int available_bytes;
    uint32_t input_character;
    int parse_delimited_field_result;
    char *destination_row;
    int loaded_count;
    int total_count;
    int parse_position;
    char field_text[56];

    total_count = 0;
    parse_position = 0;
    api_make_wifi_call_result = pico_api_make_wifi_call(request_and_record_buffer, 1, 0);
    if (api_make_wifi_call_result != 0) {
        read_started_ms = millis();
        api_make_wifi_call_result = 0;
        loaded_count = 0;
        parse_delimited_field_result = 0;
        while (true) {
            do {
                socket_connected = WiFiClient_status(&g_http_client);
                if ((socket_connected == '\0') ||
                    (now_ms = millis(),
                     &g_network_api_unknown_01 < (uint8_t *)(now_ms - read_started_ms)))
                    goto branch_1;
                available_bytes = WiFiClient_available(&g_http_client);
            } while (available_bytes == 0);
            input_character = WiFiClient_read_byte(&g_http_client);
            if ((input_character & 0xff) == 0x23)
                break;
            if ((input_character & 0xff) == 0x7c) {
                request_and_record_buffer[parse_delimited_field_result] = (char)input_character;
                total_count = total_count + 1;
                if (((total_count <= *page_offset + 0x13) && (*page_offset < total_count)) &&
                    (loaded_count < 0x13)) {
                    parse_delimited_field_result = pico_parse_delimited_field(
                        request_and_record_buffer, field_text, &parse_position, ",", 200);
                    if (parse_delimited_field_result == 0)
                        goto branch_1;
                    destination_row = (char *)(item_identifiers + loaded_count * 0x16);
                    libc_strncpy(destination_row, field_text, 0x15);
                    destination_row[0x15] = '\0';
                    parse_delimited_field_result =
                        pico_parse_delimited_field(request_and_record_buffer, field_text,
                                                   &parse_position, "|", parse_position + 0x32);
                    if (parse_delimited_field_result == 0)
                        goto branch_1;
                    if (item_titles == 0) {
                        parse_delimited_field_result = loaded_count * 0x16;
                        libc_strncpy(&g_menu_label_0 + parse_delimited_field_result, field_text,
                                     0x15);
                        loaded_count = loaded_count + 1;
                        (&g_ui_menus_unknown_04)[parse_delimited_field_result] = 0;
                    } else {
                        destination_row = (char *)(item_titles + loaded_count * 0x16);
                        libc_strncpy(destination_row, field_text, 0x15);
                        destination_row[0x15] = '\0';
                        if (api_make_wifi_call_result == 0) {
                            libc_strncpy(&g_menu_label_0, field_text, 0x15);
                            loaded_count = loaded_count + 1;
                            g_ui_menus_unknown_04 = 0;
                            api_make_wifi_call_result = 1;
                        } else {
                            parse_delimited_field_result = libc_strcmp(
                                &g_menu_label_0 + (api_make_wifi_call_result + -1) * 0x16,
                                destination_row);
                            loaded_count = loaded_count + 1;
                            if (parse_delimited_field_result != 0) {
                                parse_delimited_field_result = api_make_wifi_call_result * 0x16;
                                libc_strncpy(&g_menu_label_0 + parse_delimited_field_result,
                                             field_text, 0x15);
                                api_make_wifi_call_result = api_make_wifi_call_result + 1;
                                (&g_ui_menus_unknown_04)[parse_delimited_field_result] = 0;
                            }
                        }
                    }
                }
                available_bytes = 0;
                parse_position = 0;
            } else {
                available_bytes = parse_delimited_field_result + 1;
                request_and_record_buffer[parse_delimited_field_result] = (char)input_character;
                if (399 < available_bytes)
                    goto branch_1;
            }
            parse_delimited_field_result = available_bytes;
        }
        if (loaded_count != 0) {
            if (item_titles == 0) {
                api_make_wifi_call_result = loaded_count;
            }
            *total_items = total_count;
            *menu_item_count = api_make_wifi_call_result;
            *loaded_items = loaded_count;
            pico_api_close_connection();
            return 1;
        }
    }
branch_1:
    pico_api_close_connection();
    return 0;
}

/* getAssociatedPaks request wrapper around recipe-list parser.
 * Firmware address: 0x20012b04. */
int pico_api_get_associated_picopaks(int *total_items, int *page_offset, int *menu_item_count,
                                     int item_identifiers, int item_titles, int *loaded_items)
{
    int api_parse_recipe_list_result;
    char request_and_record_buffer[500];

    pico_api_write_base_path(request_and_record_buffer);
    libc_strcat(request_and_record_buffer, "getAssociatedPaks?");
    pico_api_append_device_uid(request_and_record_buffer);
    api_parse_recipe_list_result =
        pico_api_parse_recipe_list(total_items, page_offset, menu_item_count, item_identifiers,
                                   request_and_record_buffer, item_titles, loaded_items);
    return api_parse_recipe_list_result;
}

/* recipelist request wrapper around common list parser.
 * Firmware address: 0x20012b4c. */
int pico_api_get_recipe_list(int *total_items, int *page_offset, int *menu_item_count,
                             int item_identifiers)
{
    int api_parse_recipe_list_result;
    int loaded_items;
    char request_and_record_buffer[500];

    pico_api_write_base_path(request_and_record_buffer);
    libc_strcat(request_and_record_buffer, "recipelist?");
    pico_api_append_device_uid(request_and_record_buffer);
    api_parse_recipe_list_result =
        pico_api_parse_recipe_list(total_items, page_offset, menu_item_count, item_identifiers,
                                   request_and_record_buffer, 0, &loaded_items);
    return api_parse_recipe_list_result;
}
