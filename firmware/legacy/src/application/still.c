/* Pico Pro 0.1.34 — still. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Zeroes received status, timestamp, five double fields and three flag bytes.
 * Firmware address: 0x2000489a. */
void pico_still_clear_telemetry(uint8_t *telemetry)
{
    *telemetry = 0;
    *(uint32_t *)(telemetry + 4) = 0;
    telemetry[8] = 0;
    *(uint32_t *)(telemetry + 0xc) = 0;
    *(uint32_t *)(telemetry + 0x10) = 0;
    *(uint32_t *)(telemetry + 0x14) = 0;
    *(uint32_t *)(telemetry + 0x18) = 0;
    *(uint32_t *)(telemetry + 0x1c) = 0;
    *(uint32_t *)(telemetry + 0x20) = 0;
    *(uint32_t *)(telemetry + 0x24) = 0;
    *(uint32_t *)(telemetry + 0x28) = 0;
    *(uint32_t *)(telemetry + 0x2c) = 0;
    *(uint32_t *)(telemetry + 0x30) = 0;
    *(uint32_t *)(telemetry + 0x34) = 0;
    telemetry[0x38] = 0;
    telemetry[0x39] = 0;
    telemetry[0x3a] = 0;
    return;
}

/* Tests flag0x38 and compares pressure double at0x30 with threshold at0x50.
 * Firmware address: 0x200048ca. */
bool pico_still_check_pressure_limit(int telemetry)
{
    int comparison_result;

    if (*(char *)(telemetry + 0x38) == '\0') {
        comparison_result =
            __aeabi_dcmplt(*(double *)(telemetry + 0x30), *(double *)(telemetry + 0x50));
        return comparison_result != 0;
    }
    return false;
}

/* Writes ERROR prefix and supplied still error message through Serial.
 * Firmware address: 0x20006308. */
void pico_still_log_error(char *message)
{
    Print_print_string(&global_serial0, "ERROR ");
    Print_println_string(&global_serial0, message);
    return;
}

/* Clears connection flags/IP state and two text buffers.
 * Firmware address: 0x20006380. */
void pico_still_initialize_connection(int connection)
{
    uint32_t empty_ip_address;

    empty_ip_address = any_ip_address_value_as_uint32_t;
    *(uint8_t *)(connection + 8) = 0;
    *(uint32_t *)(connection + 4) = empty_ip_address;
    *(uint32_t *)(connection + 0x10) = empty_ip_address;
    *(uint8_t *)(connection + 0x14) = 0;
    libc_memset((void *)(connection + 0x16), 0, 0x20);
    libc_memset((void *)(connection + 0x36), 0, 0x10);
    return;
}

/* Copies identifier and forms SSID by prefixing PicoStill_.
 * Firmware address: 0x200063b0. */
void pico_still_set_access_point_name(int connection, char *still_identifier)
{
    libc_memset((void *)(connection + 0x16), 0, 0x20);
    libc_memset((void *)(connection + 0x36), 0, 0x10);
    if (still_identifier == (char *)0x0) {
        return;
    }
    libc_strcpy(&g_still_identifier, still_identifier);
    libc_strcpy(&g_still_access_point_ssid, "PicoStill_");
    libc_strcat(&g_still_access_point_ssid, &g_still_identifier);
    Print_print_string(&global_serial0, "Selected Still [UID=");
    Print_print_string(&global_serial0, &g_still_identifier);
    Print_print_string(&global_serial0, "] [SSID=");
    Print_print_string(&global_serial0, &g_still_access_point_ssid);
    Print_println_string(&global_serial0, "]");
    return;
}

/* Compares stored home IP against constructed0.0.0.0; explicit hasHomeIP assertion caller.
 * Firmware address: 0x20006434. */
bool pico_still_has_home_ip(int connection, uint32_t argument_2, uint32_t argument_3,
                            int argument_4)
{
    int home_ip;
    uint32_t value_10;
    int zero_ip;

    home_ip = *(int *)(connection + 4);
    value_10 = argument_3;
    zero_ip = argument_4;
    IPAddress_construct_octets(&value_10, 0, 0, 0, 0);
    return home_ip != zero_ip;
}

/* Selects192.168.4.1 for device AP or saved home IP; checks hasHomeIP for home mode.
 * Firmware address: 0x20006454. */
void pico_still_select_access_point(int connection, int access_point, uint32_t argument_3)
{
    bool has_home_ip;
    uint32_t condition;
    int unresolved_return_r1;
    int value_2;
    uint32_t unresolved_return_r2;
    uint32_t value_18;
    int selected_ip;

    if (access_point == 0) {
        IPAddress_construct_octets(&value_18, 0xc0, 0xa8, 4, 1);
        value_2 = unresolved_return_r1;
        argument_3 = unresolved_return_r2;
    } else {
        selected_ip = *(int *)(connection + 4);
        value_2 = access_point;
    }
    *(int *)(connection + 0x10) = selected_ip;
    if (access_point == 1) {
        has_home_ip = pico_still_has_home_ip(connection, value_2, argument_3, selected_ip);
        condition = (uint32_t)has_home_ip;
    } else {
        condition = 1;
    }
    pico_assert_nonfatal(condition, "accessPoint != AccessPoint::Home || hasHomeIP()");
    return;
}

/* Parses eight comma-separated numeric fields, updates timestamp/doubles/three status flags.
 * Firmware address: 0x200064d8. */
void pico_still_parse_telemetry(uint8_t *telemetry, char *response)
{
    char byte_value_1;
    float value;
    uint32_t now_ms;
    int __aeabi_f2iz_result;
    uint32_t value_4;
    uint32_t next_field_length;
    uint32_t field_index;
    double converted_double_bits;
    char field_text[16];
    float parsed_float_fields[9];

    field_index = 0;
    libc_memset(field_text, 0, 0x10);
    value_4 = 0;
    do {
        byte_value_1 = *response;
        if ((byte_value_1 == '\0') || (byte_value_1 == ',')) {
            if (value_4 == 0) {
                value = -1.0;
            } else {
                converted_double_bits = libc_atof(field_text);
                value = __aeabi_d2f(converted_double_bits);
            }
            parsed_float_fields[field_index] = value;
            Print_print_string(&global_serial0, "data[");
            Print_print_unsigned_alias_b(&global_serial0, field_index, 10);
            Print_print_string(&global_serial0, "]=");
            Print_println_float(&global_serial0, value, '\x02');
            libc_memset(field_text, 0, 0x10);
            field_index = field_index + 1;
            next_field_length = 0;
        } else {
            next_field_length = value_4 + 1;
            field_text[value_4] = byte_value_1;
            if (0xe < next_field_length) {
                pico_still_log_error("not a float");
            branch_1:
                *telemetry = 1;
                now_ms = millis();
                *(uint32_t *)(telemetry + 0xc) = now_ms;
                converted_double_bits = __aeabi_f2d(parsed_float_fields[0]);
                *(double *)(telemetry + 0x10) = converted_double_bits;
                converted_double_bits = __aeabi_f2d(parsed_float_fields[1]);
                *(double *)(telemetry + 0x18) = converted_double_bits;
                converted_double_bits = __aeabi_f2d(parsed_float_fields[2]);
                *(double *)(telemetry + 0x20) = converted_double_bits;
                converted_double_bits = __aeabi_f2d(parsed_float_fields[3]);
                *(double *)(telemetry + 0x28) = converted_double_bits;
                converted_double_bits = __aeabi_f2d(parsed_float_fields[4]);
                *(double *)(telemetry + 0x30) = converted_double_bits;
                __aeabi_f2iz_result = __aeabi_f2iz(parsed_float_fields[5]);
                telemetry[0x38] = __aeabi_f2iz_result != 0;
                __aeabi_f2iz_result = __aeabi_f2iz(parsed_float_fields[6]);
                telemetry[0x39] = __aeabi_f2iz_result != 0;
                __aeabi_f2iz_result = __aeabi_f2iz(parsed_float_fields[7]);
                telemetry[0x3a] = __aeabi_f2iz_result != 0;
                return;
            }
        }
        if ((*response == '\0') ||
            (value_4 = next_field_length, response = response + 1, 7 < field_index))
            goto branch_1;
    } while (true);
}
