/* Pico Pro 0.1.34 — parsing. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Copies until one-character delimiter, updates cursor, NUL terminates and observes input limit.
 * Firmware address: 0x20008c90. */
int pico_parse_delimited_field(char *input, char *output, int *cursor, char *delimiter,
                               int input_limit)
{
    int output_index;
    int input_index;

    output_index = 0;
    while (true) {
        input_index = *cursor;
        if (*delimiter == input[input_index]) {
            output[output_index] = '\0';
            *cursor = *cursor + 1;
            return 1;
        }
        if (input[input_index] == '\0')
            break;
        *cursor = input_index + 1;
        output[output_index] = input[input_index];
        output_index = output_index + 1;
        if (input_limit < *cursor) {
            return 0;
        }
    }
    return 0;
}

/* Parses recipe header and up to15 comma-separated32-byte step records.
 * Firmware address: 0x20008ccc. */
uint32_t pico_recipe_parse_server_response(char *response, long response_length)
{
    int parse_delimited_field_result;
    char *destination;
    uint32_t step_count;
    double parsed_decimal_bits;
    int cursor;
    char field_text[100];

    step_count = 0;
    cursor = 0;
    Print_println_string(&global_serial0, response);
    Print_println_signed(&global_serial0, response_length, 10);
    parse_delimited_field_result =
        pico_parse_delimited_field(response, field_text, &cursor, "/", response_length);
    if (parse_delimited_field_result != 0) {
        pico_screen_set_recipe_name((PicoScreenState *)&g_screen_recipe_name, field_text);
        parse_delimited_field_result =
            pico_parse_delimited_field(response, field_text, &cursor, ",", response_length);
        if (parse_delimited_field_result != 0) {
            g_recipe_abv_adjustment_reference = libc_atoi(field_text);
            parse_delimited_field_result =
                pico_parse_delimited_field(response, field_text, &cursor, ",", response_length);
            if (parse_delimited_field_result != 0) {
                g_recipe_ibu_adjustment_reference = libc_atoi(field_text);
                parse_delimited_field_result =
                    pico_parse_delimited_field(response, field_text, &cursor, ",", response_length);
                if (parse_delimited_field_result != 0) {
                    parsed_decimal_bits = libc_atof(field_text);
                    g_recipe_decimal_metric_high =
                        (uint32_t)((uint64_t)parsed_decimal_bits >> 0x20);
                    g_recipe_decimal_metric_low = PICO_SUB_U32(parsed_decimal_bits, 0);
                    parse_delimited_field_result = pico_parse_delimited_field(
                        response, field_text, &cursor, ",", response_length);
                    if (parse_delimited_field_result != 0) {
                        g_recipe_integer_metric = libc_atoi(field_text);
                        Print_println_string(&global_serial0, "steps");
                        destination = &g_recipe_steps;
                        while (true) {
                            if ((response_length <= cursor) || (step_count == 0xf))
                                goto branch_1;
                            parse_delimited_field_result = pico_parse_delimited_field(
                                response, field_text, &cursor, ",", response_length);
                            if (parse_delimited_field_result == 0)
                                break;
                            parse_delimited_field_result = libc_atoi(field_text);
                            destination[0x17] = (char)parse_delimited_field_result;
                            parse_delimited_field_result = pico_parse_delimited_field(
                                response, field_text, &cursor, ",", response_length);
                            if (parse_delimited_field_result == 0) {
                                return 0;
                            }
                            parse_delimited_field_result = libc_atoi(field_text);
                            *(int *)(destination + 0x1c) = parse_delimited_field_result;
                            parse_delimited_field_result = pico_parse_delimited_field(
                                response, field_text, &cursor, ",", response_length);
                            if (parse_delimited_field_result == 0) {
                                return 0;
                            }
                            parse_delimited_field_result = libc_atoi(field_text);
                            *(int *)(destination + 0x18) = parse_delimited_field_result;
                            parse_delimited_field_result = pico_parse_delimited_field(
                                response, field_text, &cursor, ",", response_length);
                            if (parse_delimited_field_result == 0) {
                                return 0;
                            }
                            parse_delimited_field_result = libc_atoi(field_text);
                            destination[0x16] = (char)parse_delimited_field_result;
                            parse_delimited_field_result = pico_parse_delimited_field(
                                response, field_text, &cursor, ",", response_length);
                            if (parse_delimited_field_result == 0) {
                                return 0;
                            }
                            libc_strncpy(destination, field_text, 0x15);
                            destination[0x15] = '\0';
                            step_count = step_count + 1;
                            destination = destination + 0x20;
                        }
                        if (g_offline_mode == '\0') {
                            return 0;
                        }
                    branch_1:
                        if ((step_count & 0xff) == 0) {
                            g_recipe_step_count_as_uint8_t = (char)step_count;
                            return 0;
                        }
                        g_recipe_step_count_as_uint8_t = (char)step_count;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* Copies3-character percent escape to destination at given offset.
 * Firmware address: 0x20008f54. */
void pico_url_write_escape(char *output, char *escape, int offset)
{
    output[offset] = *escape;
    output[offset + 1] = escape[1];
    output[offset + 2] = escape[2];
    return;
}

/* Maps reserved characters to fixed percent escapes and returns output buffer.
 * Firmware address: 0x20008f64. */
char *pico_url_encode(char *input, char *output)
{
    uint8_t character;
    uint32_t input_length;
    char *escape;
    int output_index;
    uint32_t input_index;

    input_index = 0;
    *output = '\0';
    output_index = 0;
    do {
        input_length = libc_strlen(input);
        if (input_length <= input_index) {
            output[output_index] = '\0';
            return output;
        }
        character = input[input_index];
        if (character == 0x3c) {
            escape = "%3C";
        branch_1:
            pico_url_write_escape(output, escape, output_index);
            output_index = output_index + 3;
        } else {
            if (0x3c < character) {
                if (character == 0x5c) {
                    escape = "%7C";
                } else if (character < 0x5d) {
                    if (character == 0x3f) {
                        escape = "%3F";
                    } else if (character < 0x40) {
                        if (character == 0x3d) {
                            escape = "%3D";
                        } else {
                            if (character != 0x3e)
                                goto branch_2;
                            escape = "%3E";
                        }
                    } else if (character == 0x40) {
                        escape = "%40";
                    } else {
                        if (character != 0x5b)
                            goto branch_2;
                        escape = "%5B";
                    }
                } else if (character == 0x60) {
                    escape = "%60";
                } else if (character < 0x61) {
                    if (character == 0x5d) {
                        escape = "%5D";
                    } else {
                        if (character != 0x5e)
                            goto branch_2;
                        escape = "%5E";
                    }
                } else if (character == 0x7d) {
                    escape = "%7D";
                } else if (character == 0x7e) {
                    escape = "%7E";
                } else {
                    if (character != 0x7b)
                        goto branch_2;
                    escape = "%7B";
                }
                goto branch_1;
            }
            if (character == 0x26) {
                escape = "%26";
                goto branch_1;
            }
            if (0x26 < character) {
                if (character == 0x2f) {
                    escape = "%2F";
                } else if (character < 0x30) {
                    if (character == 0x2b) {
                        escape = "%2B";
                    } else {
                        if (character != 0x2c)
                            goto branch_2;
                        escape = "%2C";
                    }
                } else if (character == 0x3a) {
                    escape = "%3A";
                } else {
                    if (character != 0x3b)
                        goto branch_2;
                    escape = "%3B";
                }
                goto branch_1;
            }
            if (character == 0x23) {
                escape = "%23";
                goto branch_1;
            }
            if (0x23 < character) {
                if (character == 0x24) {
                    escape = "%24";
                } else {
                    if (character != 0x25)
                        goto branch_2;
                    escape = "%25";
                }
                goto branch_1;
            }
            if (character == 0x20) {
                escape = "%20";
                goto branch_1;
            }
            if (character == 0x22) {
                escape = "%22";
                goto branch_1;
            }
        branch_2:
            output[output_index] = character;
            output_index = output_index + 1;
        }
        input_index = input_index + 1;
    } while (true);
}
