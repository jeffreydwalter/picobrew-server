/* Pico Pro 0.1.34 — persistence. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Validates session enum then allows only types0/1; caller uses result to save recovery state.
 * Firmware address: 0x20004b88. */
bool pico_session_supports_recovery(uint32_t session_type)
{
    pico_assert_nonfatal((uint32_t)(session_type < 6), "isSessionTypeValid(sessionType)");
    if (session_type < 6) {
        return session_type < 2;
    }
    Print_println_string(&global_serial0, "Invalid session type argument");
    return false;
}

/* Opens /sys/mcuimg1.bin then closes and reports success; checks recovery firmware slot, not
 * restart.txt. Firmware address: 0x2000564c. */
uint32_t pico_mcu_image1_exists(void)
{
    int opened;

    opened = SLFS_open(0x2002e58c, (uint8_t *)"/sys/mcuimg1.bin", 0);
    if (opened == 0) {
        SLFS_close(0x2002e58c);
        return 0;
    }
    SLFS_close(0x2002e58c);
    return 1;
}

/* Opens/creates bounded file, writes string through File stream and closes.
 * Firmware address: 0x20005678. */
bool pico_file_write_text(uint8_t *filename, char *text, uint32_t capacity, uint32_t argument_4)
{
    int operation_result;
    uint32_t value_2;

    operation_result = SLFS_open(0x2002e58c, filename, 0x1000);
    if (operation_result == 0) {
        SLFS_close(0x2002e58c);
        value_2 = unresolved_simplelink_protocol_30(capacity, 0);
        operation_result = SLFS_open(0x2002e58c, filename, value_2);
        if (operation_result == 0) {
            SLFS_close(0x2002e58c);
            return false;
        }
    }
    value_2 = libc_strlen(text);
    operation_result = (**(PicoOpaqueFunction **)(global_serial_flash_as_uint32_t + 4))(
        &global_serial_flash_as_uint32_t, text, value_2,
        *(PicoOpaqueFunction **)(global_serial_flash_as_uint32_t + 4), argument_4);
    SLFS_close(0x2002e58c);
    return operation_result <= (int)capacity;
}

/* Serializes supported session fields to CSV at most once per60s unless forced.
 * Firmware address: 0x200056e0. */
bool pico_recovery_save_state(char *recipe_identifier, int remaining_active_minutes,
                              int remaining_hold_minutes, int step_index, int resume_state,
                              uint32_t session_type, int adjustment_a, int adjustment_b,
                              char force_write, uint8_t calibration_complete)
{
    bool saved_or_supported;
    uint32_t now_ms;
    char *itoa_result;
    uint32_t unresolved_return_r3;
    char number_buffer[12];
    char recovery_text[100];

    saved_or_supported = pico_session_supports_recovery(session_type);
    if ((saved_or_supported) &&
        ((now_ms = millis(),
          &g_persistence_unknown_01 <= (uint8_t *)(now_ms - g_last_recovery_save_ms) ||
              (force_write != '\0')))) {
        g_last_recovery_save_ms = millis();
        itoa_result = itoa(session_type, number_buffer, 10);
        libc_strcpy(recovery_text, itoa_result);
        libc_strcat(recovery_text, ":");
        libc_strcat(recovery_text, recipe_identifier);
        libc_strcat(recovery_text, ":");
        itoa_result = itoa(adjustment_a, number_buffer, 10);
        libc_strcat(recovery_text, itoa_result);
        libc_strcat(recovery_text, ":");
        itoa_result = itoa(adjustment_b, number_buffer, 10);
        libc_strcat(recovery_text, itoa_result);
        libc_strcat(recovery_text, ":");
        itoa_result = itoa(step_index, number_buffer, 10);
        libc_strcat(recovery_text, itoa_result);
        libc_strcat(recovery_text, ":");
        itoa_result = itoa(remaining_active_minutes, number_buffer, 10);
        libc_strcat(recovery_text, itoa_result);
        libc_strcat(recovery_text, ":");
        itoa_result = itoa(remaining_hold_minutes, number_buffer, 10);
        libc_strcat(recovery_text, itoa_result);
        libc_strcat(recovery_text, ":");
        itoa_result = itoa(resume_state, number_buffer, 10);
        libc_strcat(recovery_text, itoa_result);
        libc_strcat(recovery_text, ":");
        itoa_result = itoa((uint32_t)calibration_complete, number_buffer, 10);
        libc_strcat(recovery_text, itoa_result);
        libc_strcat(recovery_text, ":");
        saved_or_supported = pico_file_write_text((uint8_t *)"restart.txt", recovery_text, 100,
                                                  unresolved_return_r3);
    } else {
        saved_or_supported = false;
    }
    return saved_or_supported;
}

/* Saves SSID, encryption mode and password files with short delays.
 * Firmware address: 0x2000581c. */
bool pico_settings_save_wifi(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                             uint32_t argument_4)
{
    bool saved;
    uint32_t security_text;

    security_text = argument_2;
    saved = pico_file_write_text((uint8_t *)"ssid.txt", &g_wifi_ssid, 0x21, argument_4);
    if (saved) {
        delay(0x32);
        security_text = pico_join_u24_u8(PICO_PART_U24(security_text, 1),
                                         (char)(g_wifi_security_type_as_uint8_t + 0x30));
        saved = pico_file_write_text((uint8_t *)"wifiSecurity.txt", (char *)&security_text, 2,
                                     g_wifi_security_type_as_uint8_t + 0x30);
        if (saved) {
            delay(0x32);
            if (g_wifi_security_type_as_uint8_t != 7) {
                saved = pico_file_write_text((uint8_t *)"wifiPass.txt", &g_wifi_password, 0x40,
                                             (uint32_t)g_wifi_security_type_as_uint8_t);
                if (!saved) {
                    return saved;
                }
                delay(0x32);
            }
            return true;
        }
    }
    return false;
}

/* Serializes temperature-unit byte and language into preferences file.
 * Firmware address: 0x20005888. */
bool pico_settings_save_display(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3)
{
    bool saved;
    char buffer_10[4];
    uint32_t text_storage_tail;
    uint32_t value_8;

    buffer_10[0] = '\0';
    buffer_10[1] = '\0';
    buffer_10[2] = '\0';
    buffer_10[3] = '\0';
    text_storage_tail = argument_2 & 0xffffff00;
    value_8 = argument_3;
    itoa((uint32_t)g_language_as_uint8_t, buffer_10 + 1, 10);
    buffer_10[0] = (char)(g_temperature_unit_celsius_as_uint8_t + 0x30);
    saved = pico_file_write_text((uint8_t *)"userPref.txt", buffer_10, 5,
                                 g_temperature_unit_celsius_as_uint8_t + 0x30);
    return saved;
}

/* Delegates filename deletion to file object and returns1.
 * Firmware address: 0x200058c8. */
uint32_t pico_file_delete(uint8_t *filename)
{
    SLFS_remove(0x2002e58c, filename);
    return 1;
}

/* Deletes stored brew-state file.
 * Firmware address: 0x200058dc. */
void pico_recovery_delete_state(void)
{
    pico_file_delete((uint8_t *)"restart.txt");
    return;
}

/* Deletes SSID, password and encryption-mode files.
 * Firmware address: 0x200058e8. */
void pico_settings_delete_wifi(void)
{
    pico_file_delete((uint8_t *)"ssid.txt");
    pico_file_delete((uint8_t *)"wifiPass.txt");
    pico_file_delete((uint8_t *)"wifiSecurity.txt");
    return;
}

/* Deletes preference/calibration and cached firmware-related user files.
 * Firmware address: 0x2000590c. */
void pico_settings_delete_user_files(void)
{
    pico_file_delete((uint8_t *)"userPref.txt");
    pico_file_delete((uint8_t *)"SS.txt");
    pico_file_delete((uint8_t *)"/sys/mcuimg2.bin");
    pico_file_delete((uint8_t *)"/sys/mcuimg3.bin");
    pico_file_delete((uint8_t *)"sys/uid.txt");
    pico_file_delete((uint8_t *)"/sys/mcubootinfo.bin");
    return;
}

/* Formats shuttle calibration scale to3decimals and writes SS.txt; network logger calls the field
 * shutScale. Firmware address: 0x200059f4. */
bool pico_settings_save_shuttle_scale(void)
{
    bool saved;
    float coefficient_float_bits;
    uint32_t unresolved_return_r3;
    char buffer_10[16];

    coefficient_float_bits =
        __aeabi_d2f((double)pico_join_u32_u32(g_shuttle_scale_high, g_shuttle_scale_low));
    pico_format_float_decimal(coefficient_float_bits, buffer_10, 3);
    saved = pico_file_write_text((uint8_t *)"SS.txt", buffer_10, 6, unresolved_return_r3);
    return saved;
}

/* Passes current recipe identifier/session fields to recovery serializer.
 * Firmware address: 0x20005ffc. */
bool pico_recovery_save_current_session(int remaining_active_minutes, int remaining_hold_minutes,
                                        int step_index, int resume_state, char force_write,
                                        uint8_t calibration_complete)
{
    bool saved;

    saved = pico_recovery_save_state(&g_recipe_identifier, remaining_active_minutes,
                                     remaining_hold_minutes, step_index, resume_state,
                                     g_session_type_as_uint32_t, g_recipe_abv_adjustment,
                                     g_recipe_ibu_adjustment, force_write, calibration_complete);
    return saved;
}

/* Closes shared File object and returns nonzero result status.
 * Firmware address: 0x20008f40. */
bool pico_file_close(void)
{
    int close_result;

    close_result = SLFS_close(0x2002e58c);
    return close_result != 0;
}
