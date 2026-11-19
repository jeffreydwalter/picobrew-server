/* Pico Pro 0.1.34 — firmware update. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Chooses and opens/creates MCU image slot file with matching size bound.
 * Firmware address: 0x20008ecc. */
uint32_t pico_open_firmware_destination(int recovery_image, uint32_t argument_2,
                                        uint32_t argument_3)
{
    int opened;
    char *filename;
    uint32_t capacity_or_create_flags;

    if (recovery_image == 0) {
        boot_info_select_factory_slot(0, argument_2, argument_3);
        filename = "/sys/mcuimg2.bin";
        SLFS_remove(0x2002e58c, (uint8_t *)"/sys/mcuimg3.bin");
        capacity_or_create_flags = 0x2f800;
    } else {
        filename = "/sys/mcuimg1.bin";
        capacity_or_create_flags = 0xf000;
    }
    SLFS_remove(0x2002e58c, (uint8_t *)filename);
    opened = SLFS_open(0x2002e58c, (uint8_t *)filename, 0x1000);
    if (opened == 0) {
        SLFS_close(0x2002e58c);
        capacity_or_create_flags = unresolved_simplelink_protocol_30(capacity_or_create_flags, 0);
        opened = SLFS_open(0x2002e58c, (uint8_t *)filename, capacity_or_create_flags);
        if (opened == 0) {
            SLFS_close(0x2002e58c);
            return 0;
        }
    }
    return 1;
}

/* checkFirmware query includes running version0.1.34 and interprets response.
 * Firmware address: 0x20012ea0. */
int pico_check_firmware_update(void)
{
    uint8_t socket_connected;
    int request_or_available;
    uint32_t read_started_ms;
    uint32_t input_byte_or_update_required;
    uint8_t *now_ms;
    char request[504];

    Print_println_string(&global_serial0, "wifi_checkFirmware");
    pico_api_write_base_path(request);
    libc_strcat(request, "checkFirmware?");
    pico_api_append_device_uid(request);
    libc_strcat(request, "&version=");
    libc_strcat(request, "0.1.34");
    request_or_available = pico_api_make_wifi_call(request, 1, 0);
    input_byte_or_update_required = 0;
    if (request_or_available != 0) {
        read_started_ms = millis();
        do {
            socket_connected = WiFiClient_status(&g_http_client);
            if (socket_connected == '\0') {
                input_byte_or_update_required = 0;
                goto branch_1;
            }
            request_or_available = WiFiClient_available(&g_http_client);
            if (request_or_available != 0) {
                input_byte_or_update_required = WiFiClient_read_byte(&g_http_client);
                if (0xff < input_byte_or_update_required) {
                    Print_println_string(&global_serial0, " wClient.read() ERROR");
                    pico_api_close_connection();
                    return 0;
                }
                input_byte_or_update_required =
                    (uint32_t)((input_byte_or_update_required & 0xff) == 0x54);
                goto branch_1;
            }
            now_ms = (uint8_t *)millis();
        } while (now_ms <= &g_brewing_unknown_07 + read_started_ms);
        Print_println_string(&global_serial0, "TIMEOUT");
        input_byte_or_update_required = 0;
    branch_1:
        pico_api_close_connection();
        Print_print_string(&global_serial0, "IsUpdatedRequired = ");
        Print_println_signed(&global_serial0, input_byte_or_update_required, 10);
    }
    return input_byte_or_update_required;
}

/* Writes buffered decoded firmware bytes to selected destination file.
 * Firmware address: 0x20012f74. */
uint32_t pico_write_firmware_chunk(void *data, uint32_t length)
{
    uint32_t slfs_write_buffer_result;

    slfs_write_buffer_result = SLFS_write_buffer(&global_serial_flash_as_uint32_t, data, length);
    return slfs_write_buffer_result;
}

/* Parses ASCII firmware envelope, validates length/checksum and writes248-byte chunks.
 * Firmware address: 0x20012f84. */
/* Firmware entry: 0x20012f84. Reviewed source reconstruction.
 * Envelope: decimal byte count, checksum text | hexadecimal payload #.
 * Preserves delimiter precedence, ignored open result, original buffer bounds,
 * low-byte write-result comparison and checksum/write order. This reconstruction
 * deliberately does not repair potential firmware defects.
 */
int pico_receive_firmware_image(int destination)
{
    char header_or_hex_pair[52];
    char expected_checksum[52];
    uint8_t chunk[256];
    uint32_t buffer_length = 0;
    uint32_t hex_pair_length = 0;
    int declared_length = 0;
    int hex_character_count = 0;
    int nibble_sum = 0;
    bool payload_started = false;

    /* Residual ABI uncertainty from the destination-open helper. These are the
     * original unresolved r1/r2 values, retained rather than invented as zero.
     * The helper's reviewed declaration must be resolved before executable use.
     */
    uint32_t unresolved_open_register_r1;
    uint32_t unresolved_open_register_r2;
    libc_memset(expected_checksum, 0, 50);
    uint32_t last_byte_ms = millis();
    pico_open_firmware_destination(destination, unresolved_open_register_r1,
                                   unresolved_open_register_r2);

    for (;;) {
        if (WiFiClient_status(&g_http_client) == 0) {
            goto failed;
        }
        if (WiFiClient_available(&g_http_client) == 0) {
            if ((uint32_t)(millis() - last_byte_ms) > 30000) {
                /* The original timeout path closes the connection twice. */
                pico_api_close_connection();
                goto failed;
            }
            continue;
        }

        last_byte_ms = millis();
        uint32_t read_value = WiFiClient_read_byte(&g_http_client);
        uint8_t input_character = (uint8_t)read_value;
        if (input_character == ',') {
            header_or_hex_pair[buffer_length] = '\0';
            buffer_length = 0;
            declared_length = libc_atoi(header_or_hex_pair);
            continue;
        }
        if (input_character == '|') {
            header_or_hex_pair[buffer_length] = '\0';
            libc_strcpy(expected_checksum, header_or_hex_pair);
            payload_started = true;
            buffer_length = 0;
            continue;
        }
        if (input_character == '#') {
            pico_api_close_connection();
            if (!payload_started) {
                goto failed;
            }
            if (buffer_length != 0 &&
                (pico_write_firmware_chunk(chunk, buffer_length) & 255) != buffer_length) {
                goto failed;
            }
            if (declared_length != (hex_character_count >> 1) || declared_length == 0) {
                goto failed;
            }
            int checksum_result = pico_validate_ota_checksum(expected_checksum, nibble_sum);
            if (checksum_result == 0) {
                goto failed;
            }
            pico_file_close();
            return checksum_result;
        }
        if (!payload_started) {
            header_or_hex_pair[buffer_length++] = (char)read_value;
            continue;
        }

        header_or_hex_pair[hex_pair_length] = (char)read_value;
        hex_pair_length = (hex_pair_length + 1) & 255;
        header_or_hex_pair[hex_pair_length] = '\0';

        int nibble_value = (int)(read_value & 15) + (input_character >> 6) * 9;
        if (hex_character_count % 15 == 0) {
            nibble_sum -= nibble_value;
        } else {
            nibble_sum += nibble_value;
        }
        ++hex_character_count;

        if (hex_pair_length > 1) {
            chunk[buffer_length++] = (uint8_t)libc_strtoul(header_or_hex_pair, NULL, 16);
            hex_pair_length = 0;
            if ((int32_t)buffer_length >= 248) {
                if ((pico_write_firmware_chunk(chunk, buffer_length) & 255) != buffer_length) {
                    goto failed;
                }
                buffer_length = 0;
            }
        }
    }

failed:
    pico_api_close_connection();
    pico_file_close();
    return 0;
}

/* getFirmware request then streaming decoder.
 * Firmware address: 0x20013100. */
int pico_download_firmware(void)
{
    int api_make_wifi_call_result;
    uint32_t storage_load_device_uid_result;
    char buffer[508];

    if ((g_device_uid_as_char != '\0') ||
        (storage_load_device_uid_result = pico_storage_load_device_uid(),
         storage_load_device_uid_result != 0)) {
        pico_api_write_base_path(buffer);
        libc_strcat(buffer, "getFirmware?");
        pico_api_append_device_uid(buffer);
        pico_display_clear();
        api_make_wifi_call_result = pico_api_make_wifi_call(buffer, 1, 0);
        if (api_make_wifi_call_result != 0) {
            api_make_wifi_call_result = pico_receive_firmware_image(0);
            return api_make_wifi_call_result;
        }
    }
    return 0;
}

/* Checks availability, downloads, displays outcome and invokes update activation.
 * Firmware address: 0x20013150. */
void pico_update_firmware(void)
{
    int check_firmware_update_result;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;

    check_firmware_update_result = pico_check_firmware_update();
    if (check_firmware_update_result == 0) {
        return;
    }
    pico_display_clear();
    pico_display_logo();
    pico_screen_updating_firmware();
    pico_display_flush();
    check_firmware_update_result = pico_download_firmware();
    if (check_firmware_update_result != 0) {
        pico_display_clear_rows(4, 7);
        pico_screen_downloading_firmware();
        pico_display_flush();
        WiFi_disconnect();
        delay(1000);
        boot_info_toggle_application_slot(1, unresolved_return_r1, unresolved_return_r2);
        PRCMMCUReset(true);
    }
    pico_display_clear();
    pico_display_logo();
    pico_screen_firmware_update_failed();
    pico_display_flush();
    pico_api_report_error(0x17);
    pico_ui_wait_for_continue();
    return;
}
