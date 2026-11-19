/* Pico Pro 0.1.34 — storage settings. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Opens named file, reads up to capacity bytes, NUL terminates, closes.
 * Firmware address: 0x2000a538. */
int pico_storage_read_text(char *filename, char *buffer, uint32_t capacity)
{
    int read_count_or_open_result;
    uint32_t input_byte;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r1_00;

    read_count_or_open_result = SLFS_open(0x2002e58c, (uint8_t *)filename, 0);
    if (read_count_or_open_result == 0) {
    branch_1:
        SLFS_close(0x2002e58c);
        return 0;
    }
    read_count_or_open_result = 0;
    input_byte = unresolved_return_r1;
    do {
        input_byte = SLFS_read_byte(0x2002e58c, input_byte);
        if (input_byte == 0xffffffff) {
            if (read_count_or_open_result == 0)
                goto branch_1;
            break;
        }
        buffer[read_count_or_open_result] = (char)input_byte;
        read_count_or_open_result = read_count_or_open_result + 1;
        input_byte = unresolved_return_r1_00;
    } while (read_count_or_open_result < (int)capacity);
    buffer[read_count_or_open_result] = '\0';
    SLFS_close(0x2002e58c);
    return 1;
}

/* Reads restart.txt with 100-byte bound.
 * Firmware address: 0x2000a584. */
int pico_storage_read_restart_record(char *buffer)
{
    int storage_read_text_result;

    storage_read_text_result = pico_storage_read_text("restart.txt", buffer, 100);
    return storage_read_text_result;
}

/* Loads sys/uid.txt or initializes and saves UID if absent.
 * Firmware address: 0x2000a594. */
uint32_t pico_storage_load_device_uid(void)
{
    uint32_t storage_read_text_result;
    uint32_t unresolved_return_r3;

    storage_read_text_result = pico_storage_read_text("sys/uid.txt", &g_device_uid, 0x21);
    if ((storage_read_text_result == 0) &&
        (storage_read_text_result = pico_device_compute_uid(), storage_read_text_result != 0)) {
        g_device_uid_terminator = 0;
        pico_file_write_text((uint8_t *)"sys/uid.txt", &g_device_uid, 0x21, unresolved_return_r3);
    }
    return storage_read_text_result;
}

/* Reads SS.txt and parses floating-point shuttle scaler.
 * Firmware address: 0x2000a5cc. */
int pico_storage_load_shuttle_scale(void)
{
    int storage_read_text_result;
    double libc_atof_result;
    char buffer_10[16];

    storage_read_text_result = pico_storage_read_text("SS.txt", buffer_10, 6);
    libc_atof_result = (double)pico_join_u32_u32(g_shuttle_scale_high, g_shuttle_scale_low);
    if (storage_read_text_result != 0) {
        libc_atof_result = libc_atof(buffer_10);
    }
    g_shuttle_scale_high = (uint32_t)((uint64_t)libc_atof_result >> 0x20);
    g_shuttle_scale_low = PICO_SUB_U32(libc_atof_result, 0);
    return (uint32_t)(storage_read_text_result != 0);
}

/* Reads ssid.txt, wifiSecurity.txt and wifiPass.txt.
 * Firmware address: 0x2000a5f8. */
int pico_storage_load_wifi_credentials(void)
{
    int storage_read_text_result;
    char unresolved_input_r1;
    char buffer[4];

    buffer[0] = unresolved_input_r1;
    storage_read_text_result = pico_storage_read_text("ssid.txt", &g_wifi_ssid, 0x21);
    if (storage_read_text_result != 0) {
        delay(0x32);
        storage_read_text_result = pico_storage_read_text("wifiSecurity.txt", buffer, 2);
        if (storage_read_text_result != 0) {
            g_wifi_security_type = buffer[0] + -0x30;
            delay(0x32);
            if (g_wifi_security_type != '\a') {
                storage_read_text_result =
                    pico_storage_read_text("wifiPass.txt", &g_wifi_password, 0x40);
                if (storage_read_text_result == 0) {
                    return 0;
                }
                delay(0x32);
            }
            return 1;
        }
    }
    return 0;
}

/* RESET SHUTTLE PUMP screen then sets scale to0.2 and stores it.
 * Firmware address: 0x2000a7c0. */
bool pico_reset_shuttle_scale(void)
{
    bool settings_save_shuttle_scale_result;

    pico_display_clear();
    pico_ui_draw_header("RESET SHUTTLE PUMP");
    pico_display_center_text(3, "Removing Shuttle");
    pico_display_center_text(4, "Scaler");
    pico_display_flush();
    delay(1000);
    g_shuttle_scale_low = 0x9999999a;
    g_shuttle_scale_high = 0x3fc99999;
    settings_save_shuttle_scale_result = pico_settings_save_shuttle_scale();
    return settings_save_shuttle_scale_result;
}

/* Reads userPref.txt or prompts for language/units, then loads shuttle scale and UID.
 * Firmware address: 0x2000c390. */
void pico_storage_load_user_preferences(void)
{
    int storage_read_text_result;
    uint32_t ui_choose_units_result;
    uint32_t unresolved_input_r1;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    uint32_t value_10;
    uint32_t value;

    value = unresolved_input_r1;
    SLFS_begin();
    value_10 = 0;
    value = value & 0xffffff00;
    storage_read_text_result = pico_storage_read_text("userPref.txt", (char *)&value_10, 5);
    if (storage_read_text_result == 0) {
        pico_ui_choose_language();
        ui_choose_units_result = pico_ui_choose_units();
        pico_settings_save_display(ui_choose_units_result, unresolved_return_r1,
                                   unresolved_return_r2);
        pico_display_clear();
    } else {
        storage_read_text_result = libc_atoi((char *)((int)&value_10 + 1));
        g_language_as_uint8_t_2 = (uint8_t)storage_read_text_result;
        g_temperature_unit_celsius_as_uint8_t_2 = (value_10 & 0xff) != 0x30;
    }
    storage_read_text_result = pico_storage_load_shuttle_scale();
    if (storage_read_text_result == 0) {
        pico_settings_save_shuttle_scale();
    }
    pico_storage_load_device_uid();
    return;
}
