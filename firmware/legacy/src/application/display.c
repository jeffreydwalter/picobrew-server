/* Pico Pro 0.1.34 — display. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Zeroes the 1024-byte monochrome framebuffer.
 * Firmware address: 0x20004488. */
void pico_display_clear(void)
{
    libc_memset(&g_display_framebuffer, 0, 0x400);
    return;
}

/* Sets OLED row/column window and expands framebuffer bits into grayscale SPI bytes.
 * Firmware address: 0x20004494. */
void pico_display_flush(void)
{
    uint8_t right_pixels;
    uint8_t left_pixels;
    uint8_t grayscale_pair;
    uint8_t *column_pair;
    int page_offset;
    uint32_t bit_index;

    oled_write_command(&g_display, '\x15');
    oled_write_command(&g_display, '\0');
    oled_write_command(&g_display, '?');
    oled_write_command(&g_display, 'u');
    oled_write_command(&g_display, '\0');
    oled_write_command(&g_display, '?');
    column_pair = &g_display_framebuffer;
    digitalWrite((uint32_t)g_spi_chip_select_display, 1);
    digitalWrite((uint32_t)g_spi_chip_select_auxiliary, 1);
    digitalWrite((uint32_t)g_display_pin_command_data, 1);
    digitalWrite((uint32_t)g_spi_chip_select_display, 0);
    digitalWrite((uint32_t)g_spi_chip_select_auxiliary, 0);
    delay(1);
    do {
        page_offset = 0;
        do {
            left_pixels = column_pair[page_offset];
            right_pixels = column_pair[page_offset + 1];
            bit_index = 0;
            do {
                if (((int)(uint32_t)left_pixels >> (bit_index & 0xff) & 1U) == 0) {
                    grayscale_pair = 0;
                } else {
                    grayscale_pair = 0xf0;
                }
                if (((int)(uint32_t)right_pixels >> (bit_index & 0xff)) << 0x1f < 0) {
                    grayscale_pair = grayscale_pair | 0xf;
                }
                bit_index = bit_index + 1;
                SPI_transfer_byte(&global_spi, grayscale_pair);
            } while (bit_index != 8);
            page_offset = page_offset + 0x80;
        } while (page_offset != 0x400);
        column_pair = column_pair + 2;
    } while (column_pair != &g_display_unknown_07);
    digitalWrite((uint32_t)g_spi_chip_select_display, 1);
    digitalWrite((uint32_t)g_spi_chip_select_auxiliary, 1);
    return;
}

/* Centers bitmap descriptor width on the 128-pixel display and draws at supplied y.
 * Firmware address: 0x200044d6. */
void pico_display_center_bitmap(short y, uint32_t *bitmap)
{
    GFX_draw_bitmap(&g_display, 0x40 - (short)((uint32_t)bitmap[1] >> 1), y, (uint8_t *)*bitmap,
                    *(short *)(bitmap + 1), *(short *)(bitmap + 2), 1);
    return;
}

/* Centers text horizontally at y=28 using current font scale.
 * Firmware address: 0x20004538. */
void pico_display_center_message(char *text)
{
    uint32_t text_length;

    text_length = libc_strlen(text);
    GFX_set_cursor(&g_display,
                   0x40 - (short)((int)((uint32_t)g_display_text_scale * 6 * text_length) / 2),
                   0x1c);
    Print_print_string(&g_display, text);
    return;
}

/* Constructs 128x64 graphics object and assigns display GPIO pins/vtable.
 * Firmware address: 0x20004540. */
void pico_display_construct(void)
{
    GFX_construct(&g_display, 0x80, 0x40);
    g_display = &g_display_unknown_06;
    g_display_pin_20 = 0xf;
    g_display_pin_21 = 7;
    g_display_pin_command_data_as_uint8_t = 0xd;
    g_spi_chip_select_display_as_uint8_t = 4;
    g_spi_chip_select_auxiliary_as_uint8_t = 0x12;
    return;
}

/* Clears, calls current page callback, flushes.
 * Firmware address: 0x20004870. */
void pico_display_refresh_current_page(void)
{
    pico_display_clear();
    ((PicoOpaqueFunction *)(uintptr_t)g_carousel_page_callbacks)();
    pico_display_flush();
    return;
}

/* Asserts nonempty recipeName then copies into32-byte screen field.
 * Firmware address: 0x20004d28. */
/* Firmware entry: 0x20004d28. Reviewed source reconstruction. */
void pico_screen_set_recipe_name(PicoScreenState *screen, char *recipe_name)
{
    pico_assert_nonfatal(!pico_cstring_is_null_or_empty(recipe_name),
                         "!CString::isNullOrEmpty(_recipeName)");
    pico_cstring_copy(screen->recipe_name, sizeof screen->recipe_name, recipe_name);
}

/* Asserts nonempty stepName, copies32bytes and stores component; explicit setStep log.
 * Firmware address: 0x20004d50. */
/* Firmware entry: 0x20004d50. Reviewed source reconstruction. */
void pico_screen_set_step(PicoScreenState *screen, pico_u32 component, char *step_name)
{
    pico_assert_nonfatal(!pico_cstring_is_null_or_empty(step_name),
                         "!CString::isNullOrEmpty(_stepName)");
    pico_cstring_copy(screen->step_name, sizeof screen->step_name, step_name);
    screen->progress_component = component;
    char log_buffer[128];
    libc_snprintf(log_buffer, sizeof log_buffer, "setStep comp = %d stepName = \"%s\"", component,
                  step_name);
    pico_log_serial("I", "SCR", log_buffer);
}

/* Variadic count of nonempty text lines appended to six-line screen object.
 * Firmware address: 0x20004ed8. */
/* Firmware entry: 0x20004ed8. Reviewed source reconstruction. */
void pico_screen_add_lines(PicoTextScreen *screen, int count, ...)
{
    va_list arguments;
    va_start(arguments, count);
    for (int index = 0; index < count; ++index) {
        char *line = va_arg(arguments, char *);
        pico_assert_nonfatal(!pico_cstring_is_null_or_empty(line), "!CString::isNullOrEmpty(line)");
        if (screen->line_count < PICO_TEXT_LINE_CAPACITY)
            screen->lines[screen->line_count++] = line;
        else
            pico_log_serial("E", "SCREEN", "Too many lines");
    }
    va_end(arguments);
}

/* Tail wrapper sharing framebuffer-clear implementation.
 * Firmware address: 0x20004f40. */
void pico_display_clear_thunk(void)
{
    libc_memset(&g_display_framebuffer, 0, 0x400);
    return;
}

/* Rotates page callback index on interval and redraws selected callback.
 * Firmware address: 0x20004fa6. */
void pico_display_advance_carousel(PicoPageCarousel *carousel)
{
    uint32_t now_ms;
    pico_u32 value_2;

    now_ms = millis();
    if (carousel->interval_ms < now_ms - carousel->last_update_ms) {
        now_ms = millis();
        carousel->last_update_ms = now_ms;
        value_2 = carousel->page_index + 1;
        carousel->page_index = value_2;
        if ((int)carousel->page_count <= (int)value_2) {
            carousel->page_index = 0;
        }
    }
    if (carousel->pages[carousel->page_index] == (PicoInterruptHandler *)0x0) {
        return;
    }
    pico_display_clear();
    (*carousel->pages[carousel->page_index])();
    pico_display_flush();
    return;
}

/* Selects digit bitmap0..9 with zero fallback.
 * Firmware address: 0x20004ff0. */
uint8_t *pico_display_digit_bitmap(int digit)
{
    uint8_t *bitmap;

    if (digit - 1U < 9) {
        bitmap = (&g_large_digit_bitmaps)[digit];
    } else {
        bitmap = g_large_digit_bitmaps;
    }
    return bitmap;
}

/* Initializes display, text/color settings, OLED contrast and framebuffer.
 * Firmware address: 0x20005248. */
void pico_display_initialize(void)
{
    OLED_initialize_controller(&g_display);
    GFX_set_text_colors(&g_display, 1, 0);
    GFX_set_rotation(&g_display, '\x02');
    oled_write_command(&g_display, 0x81);
    oled_write_command(&g_display, '~');
    libc_memset(&g_display_framebuffer, 0, 0x400);
    return;
}

/* Sets pixel cursor(column*6,row*8) and prints C string.
 * Firmware address: 0x20005284. */
void pico_display_text_at_cell(short row, short column, char *text)
{
    GFX_set_cursor(&g_display, column * 6, row << 3);
    Print_print_string(&g_display, text);
    return;
}

/* Same grid cursor with temporary Arduino String construction/printing/destruction.
 * Firmware address: 0x200052a8. */
void pico_display_string_at_cell(short row, short column, char *text)
{
    int temporary_string[4];

    GFX_set_cursor(&g_display, column * 6, row << 3);
    String_construct_c_string(temporary_string, text);
    Print_print_String(&g_display, temporary_string);
    String_destroy(temporary_string);
    return;
}

/* Centers a text line using glyph width and font scale, clamping x to0.
 * Firmware address: 0x200052dc. */
void pico_display_center_text(short row, char *text)
{
    uint32_t text_length;
    int value_2;
    short x;

    text_length = libc_strlen(text);
    value_2 = (uint32_t)g_display_text_scale * 6 * text_length;
    if (value_2 < 0x80) {
        x = (short)((uint32_t)((0x80 - value_2) * 0x8000) >> 0x10);
    } else {
        x = 0;
    }
    GFX_set_cursor(&g_display, x, row << 3);
    Print_print_string(&g_display, text);
    return;
}

/* Sets pixel x/y cursor then prints C string.
 * Firmware address: 0x20005524. */
void pico_display_text_at_pixels(short y, short x, char *text)
{
    GFX_set_cursor(&g_display, x, y);
    Print_print_string(&g_display, text);
    return;
}

/* Draws fixed40x48 device illustration at6,9.
 * Firmware address: 0x20005550. */
void pico_display_device_bitmap(void)
{
    GFX_draw_bitmap(&g_display, 6, 9, &g_display_unknown_05, 0x28, 0x30, 1);
    return;
}

/* Draws80x21 Pico logo at24,2 and flushes.
 * Firmware address: 0x20005578. */
void pico_display_logo(void)
{
    GFX_draw_bitmap(&g_display, 0x18, 2, "", 0x50, 0x15, 1);
    pico_display_flush();
    return;
}

/* Prints padding before text to reach requested character width.
 * Firmware address: 0x200055a4. */
void pico_display_right_aligned_text(short row, short column, char *text, uint32_t field_width,
                                     char argument_5)
{
    uint32_t text_length;
    char *text_2;
    uint8_t padding_index;
    uint32_t value_4;

    GFX_set_cursor(&g_display, column * 6, row << 3);
    text_length = libc_strlen(text);
    if (text_length < field_width) {
        padding_index = 0;
        while (true) {
            text_length = libc_strlen(text);
            value_4 = (uint32_t)padding_index;
            padding_index = padding_index + 1;
            if (field_width - text_length <= value_4)
                break;
            Print_print_char(&g_display, argument_5);
        }
    }
    text_2 = text + -1;
    while (true) {
        text_2 = text_2 + 1;
        if (*text_2 == '\0')
            break;
        Print_print_char(&g_display, *text_2);
    }
    return;
}

/* Positions x and row*8 then emits character through Print byte writer.
 * Firmware address: 0x20005604. */
void pico_display_character_on_row(short x, short row, char argument_3)
{
    GFX_set_cursor(&g_display, x, row << 3);
    Print_print_char(&g_display, argument_3);
    return;
}

/* Positions x/y then emits character through Print byte writer.
 * Firmware address: 0x20005628. */
void pico_display_character_at_pixels(short x, short y, char argument_3)
{
    GFX_set_cursor(&g_display, x, y);
    Print_print_char(&g_display, argument_3);
    return;
}

/* Draws fixed rounded content frame below title row.
 * Firmware address: 0x20007268. */
void pico_display_draw_content_frame(void)
{
    GFX_draw_round_rectangle(&g_display, 0, 0xb, 0x80, 0x2b, 0x14, 1);
    return;
}

/* Centers supplied title on row0 then draws content frame.
 * Firmware address: 0x2000728c. */
void pico_display_draw_titled_frame(char *title)
{
    pico_display_center_text(0, title);
    pico_display_draw_content_frame();
    return;
}

/* Reads active program's title and draws framed screen.
 * Firmware address: 0x2000729c. */
void pico_display_draw_program_frame(void)
{
    pico_display_draw_titled_frame(*(char **)(g_active_program_context_as_int + 0x34));
    return;
}

/* Renders recipe double field with one decimal using large bitmap digits; likely ABV, exact
 * semantic left provisional. Firmware address: 0x20007960. */
void pico_display_recipe_decimal_metric(void)
{
    uint32_t value_1;
    uint32_t value_2;
    int character_index;
    float __aeabi_d2f_result;
    uint32_t value_5;
    uint8_t *digit_bitmap;
    short x;
    uint8_t metric_text[12];

    value_2 = g_recipe_decimal_metric_high;
    value_1 = g_recipe_decimal_metric_low;
    character_index = __aeabi_dcmpgt(
        (double)pico_join_u32_u32(g_recipe_decimal_metric_high, g_recipe_decimal_metric_low), 9.9);
    metric_text[0] = 0;
    if (character_index == 0) {
        x = 2;
    } else {
        x = 3;
    }
    __aeabi_d2f_result = __aeabi_d2f((double)pico_join_u32_u32(value_2, value_1));
    pico_format_float_decimal(__aeabi_d2f_result, (char *)metric_text, 1);
    pico_display_text_at_pixels(0x2a, 0x11, "% ABV");
    x = x * -8 + 0x1f;
    for (character_index = 0;
         (value_5 = (uint32_t)metric_text[character_index], value_5 != 0 && (character_index != 5));
         character_index = character_index + 1) {
        if (value_5 == 0x2e) {
            GFX_draw_bitmap(&g_display, x, 0x19, "", 8, 8, 1);
            x = x + 8;
        } else {
            digit_bitmap = pico_display_digit_bitmap(value_5 - 0x30);
            GFX_draw_bitmap(&g_display, x, 0x10, digit_bitmap, 0x10, 0x10, 1);
            x = x + 0x10;
        }
    }
    return;
}

/* Renders recipe integer field with large bitmap digits; likely IBU, exact semantic left
 * provisional. Firmware address: 0x20007a18. */
void pico_display_recipe_integer_metric(void)
{
    int metric_value;
    uint8_t *digit_bitmap;
    short digit_count_or_x;
    uint8_t *cursor;
    uint8_t metric_text_storage[17];

    metric_value = g_recipe_integer_metric;
    metric_text_storage[1] = 0;
    itoa(g_recipe_integer_metric, (char *)(metric_text_storage + 1), 10);
    if (metric_value < 100) {
        if (metric_value < 10) {
            digit_count_or_x = 1;
        } else {
            digit_count_or_x = 2;
        }
    } else {
        digit_count_or_x = 3;
    }
    pico_display_text_at_pixels(0x2a, 0x57, "IBU");
    digit_count_or_x = (0xc - digit_count_or_x) * 8 + 3;
    cursor = metric_text_storage;
    while (true) {
        cursor = cursor + 1;
        if (*cursor == 0)
            break;
        digit_bitmap = pico_display_digit_bitmap(*cursor - 0x30);
        GFX_draw_bitmap(&g_display, digit_count_or_x, 0x10, digit_bitmap, 0x10, 0x10, 1);
        digit_count_or_x = digit_count_or_x + 0x10;
    }
    return;
}

/* Formats Target with current target and F/C suffix, suppressing zero target.
 * Firmware address: 0x20007a90. */
void pico_display_target_temperature(short row)
{
    int __aeabi_dcmpeq_result;
    char *itoa_result;
    double converted_target_bits;
    char number_text[12];
    char target_line[24];

    libc_strcpy(target_line, "Target: ");
    __aeabi_dcmpeq_result = __aeabi_dcmpeq(
        (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low),
        0.0);
    if (__aeabi_dcmpeq_result == 0) {
        if (g_temperature_unit_celsius == '\0') {
            __aeabi_dcmpeq_result = __aeabi_d2iz((double)pico_join_u32_u32(
                g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low));
            itoa_result = itoa(__aeabi_dcmpeq_result, number_text, 10);
            libc_strcat(target_line, itoa_result);
            itoa_result = "F";
        } else {
            converted_target_bits =
                __aeabi_dsub((double)pico_join_u32_u32(g_wort_target_fahrenheit_high,
                                                       g_wort_target_fahrenheit_low),
                             32.0);
            converted_target_bits = __aeabi_ddiv(converted_target_bits, 1.8);
            __aeabi_dcmpeq_result = __aeabi_d2iz(converted_target_bits);
            itoa_result = itoa(__aeabi_dcmpeq_result, number_text, 10);
            libc_strcat(target_line, itoa_result);
            itoa_result = "C";
        }
        libc_strcat(target_line, itoa_result);
        pico_display_center_text(row, target_line);
    }
    return;
}

/* Renders elapsed/remaining minutes and seconds as large bitmap digits.
 * Firmware address: 0x20007b30. */
void pico_display_large_timer(int total_minutes, uint32_t seconds, int show_seconds)
{
    uint8_t byte_value_1;
    uint8_t *digit_bitmap;
    char *itoa_result;
    short seconds_x;
    uint8_t *cursor;
    short minutes_x;
    short leading_offset;
    uint32_t seconds_enabled;
    uint8_t number_text[12];
    char seconds_text[16];

    number_text[0] = 0;
    seconds_enabled = show_seconds;
    itoa(total_minutes / 0x3c, (char *)number_text, 10);
    if (total_minutes / 0x3c < 10) {
        leading_offset = 0xc;
    } else {
        leading_offset = 4;
    }
    minutes_x = leading_offset + 0x1e;
    cursor = (uint8_t *)((int)&seconds_enabled + 3);
    while (true) {
        cursor = cursor + 1;
        byte_value_1 = *cursor;
        if (byte_value_1 == 0)
            break;
        digit_bitmap = pico_display_digit_bitmap(byte_value_1 - 0x30);
        GFX_draw_bitmap(&g_display, minutes_x, 0x18, digit_bitmap, 0x10, 0x10, 1);
        minutes_x = minutes_x + 0x10;
    }
    GFX_draw_bitmap(&g_display, minutes_x, 0x18, "", 8, 0x10, 1);
    number_text[0] = byte_value_1;
    itoa(total_minutes % 0x3c, (char *)number_text, 10);
    seconds_x = minutes_x + 8;
    if (total_minutes % 0x3c < 10) {
        GFX_draw_bitmap(&g_display, seconds_x, 0x18, g_large_digit_bitmaps, 0x10, 0x10, 1);
        seconds_x = minutes_x + 0x18;
    }
    cursor = (uint8_t *)((int)&seconds_enabled + 3);
    while (true) {
        cursor = cursor + 1;
        if (*cursor == 0)
            break;
        digit_bitmap = pico_display_digit_bitmap(*cursor - 0x30);
        GFX_draw_bitmap(&g_display, seconds_x, 0x18, digit_bitmap, 0x10, 0x10, 1);
        seconds_x = seconds_x + 0x10;
    }
    if (seconds_enabled != 0) {
        if ((int)seconds < 10) {
            seconds = seconds & ~((int)seconds >> 0x1f);
            itoa_result = ":0";
        } else {
            itoa_result = ":";
        }
        libc_strcpy(seconds_text, itoa_result);
        itoa_result = itoa(seconds, (char *)number_text, 10);
        libc_strcat(seconds_text, itoa_result);
        pico_display_text_at_pixels(0x18, seconds_x, seconds_text);
        pico_display_text_at_pixels(0x18, leading_offset, "Time");
        pico_display_text_at_pixels(0x20, leading_offset, "Left");
    }
    return;
}

/* Converts Fahrenheit to display units and renders centered large temperature digits.
 * Firmware address: 0x20007c90. */
void pico_display_large_temperature(uint32_t temperature_low, uint32_t temperature_high,
                                    int show_decimal)
{
    short x;
    char *unit_label;
    int width_or_x;
    float __aeabi_d2f_result;
    uint32_t value_4;
    uint8_t *digit_bitmap;
    int character_index;
    double __aeabi_dsub_result;
    uint8_t temperature_text[20];

    __aeabi_dsub_result = (double)pico_join_u32_u32(temperature_high, temperature_low);
    if (g_temperature_unit_celsius != '\0') {
        __aeabi_dsub_result =
            __aeabi_dsub((double)pico_join_u32_u32(temperature_high, temperature_low), 32.0);
        __aeabi_dsub_result = __aeabi_ddiv(__aeabi_dsub_result, 1.8);
    }
    width_or_x = __aeabi_dcmpgt(__aeabi_dsub_result, 99.9);
    if (width_or_x == 0) {
        width_or_x = __aeabi_dcmpgt(__aeabi_dsub_result, 9.9);
        if (width_or_x == 0) {
            width_or_x = 2;
        } else {
            width_or_x = 3;
        }
    } else {
        width_or_x = 4;
    }
    if (show_decimal == 0) {
        width_or_x = width_or_x + -1;
    }
    if (show_decimal == 0) {
        width_or_x = width_or_x * 0x10 + 6;
    } else {
        width_or_x = width_or_x * 0x10 + 0xe;
    }
    width_or_x = (int)(0x80U - width_or_x & ~((int)(0x80U - width_or_x) >> 0x1f)) >> 1;
    if (show_decimal == 0) {
        width_or_x = width_or_x + 4;
    }
    __aeabi_d2f_result = __aeabi_d2f(__aeabi_dsub_result);
    pico_format_float_decimal(__aeabi_d2f_result, (char *)temperature_text, 1);
    character_index = 0;
    while (true) {
        value_4 = (uint32_t)temperature_text[character_index];
        x = (short)width_or_x;
        if ((value_4 == 0) || (character_index == 5))
            break;
        if (value_4 == 0x2e) {
            if (show_decimal == 0)
                break;
            GFX_draw_bitmap(&g_display, x, 0x21, "", 8, 8, 1);
            width_or_x = width_or_x + 8;
        } else {
            digit_bitmap = pico_display_digit_bitmap(value_4 - 0x30);
            GFX_draw_bitmap(&g_display, x, 0x18, digit_bitmap, 0x10, 0x10, 1);
            width_or_x = width_or_x + 0x10;
        }
        character_index = character_index + 1;
    }
    if (g_temperature_unit_celsius == '\0') {
        unit_label = "   F        F       F";
    } else {
        unit_label = "   C        C       C";
    }
    libc_strcpy((char *)temperature_text, unit_label + 0x14);
    pico_display_text_at_pixels(0x18, x, (char *)temperature_text);
    return;
}

/* Wrapper enabling optional seconds suffix for large timer.
 * Firmware address: 0x20008800. */
void pico_display_large_timer_with_seconds(int total_minutes, uint32_t seconds)
{
    pico_display_large_timer(total_minutes, seconds, 1);
    return;
}

/* Wrapper enabling decimal component in large temperature display.
 * Firmware address: 0x20008960. */
void pico_display_large_temperature_with_decimal(uint32_t temperature_low,
                                                 uint32_t temperature_high)
{
    pico_display_large_temperature(temperature_low, temperature_high, 1);
    return;
}

/* Clears animation-active byte and erases its character-cell rectangle.
 * Firmware address: 0x200093d0. */
void pico_display_stop_progress_indicator(uint8_t *indicator)
{
    uint8_t byte_value_1;

    byte_value_1 = indicator[1];
    indicator[0x10] = 0;
    GFX_fill_rectangle(&g_display, ((uint16_t)byte_value_1 + (uint16_t)byte_value_1 * 2) * 2,
                       (uint16_t)*indicator << 3,
                       ((uint16_t)indicator[2] - (uint16_t)byte_value_1) * 6, 8, 0);
    pico_display_flush();
    return;
}

/* Draws device bitmap then substitutes specific24x5 status graphic; exact icon meaning unverified.
 * Firmware address: 0x20009408. */
void pico_display_device_bitmap_variant_a(void)
{
    GFX_draw_bitmap(&g_display, 6, 9, &g_display_unknown_05, 0x28, 0x30, 1);
    GFX_fill_rectangle(&g_display, 0xe, 0x1f, 0x18, 5, 0);
    GFX_draw_bitmap(&g_display, 0xe, 0x1f, &g_display_unknown_03, 0x18, 5, 1);
    return;
}

/* Draws device bitmap with alternate24x5 status graphic; exact icon meaning unverified.
 * Firmware address: 0x2000945c. */
void pico_display_device_bitmap_variant_b(void)
{
    GFX_draw_bitmap(&g_display, 6, 9, &g_display_unknown_05, 0x28, 0x30, 1);
    GFX_fill_rectangle(&g_display, 0xe, 0x1f, 0x18, 5, 0);
    GFX_draw_bitmap(&g_display, 0xe, 0x1f, &g_display_unknown_01, 0x18, 5, 1);
    return;
}

/* Erases full-width text row with height from current text scale.
 * Firmware address: 0x200094b0. */
void pico_display_clear_row(short argument_1)
{
    GFX_fill_rectangle(&g_display, 0, argument_1 << 3, 0x80, (uint16_t)g_display_text_scale << 3,
                       0);
    return;
}

/* Formats elapsed/countdown milliseconds as minutes:seconds, draws double-sized row and flushes.
 * Firmware address: 0x200094d4. */
void pico_display_update_timer(short argument_1, int started_ms, int duration_seconds)
{
    uint32_t now_ms;
    char *itoa_result;
    uint32_t display_ms;
    uint32_t seconds;
    char number_buffer[24];
    char time_text[24];

    if (duration_seconds == 0) {
        now_ms = millis();
        display_ms = now_ms - started_ms;
    } else {
        now_ms = millis();
        display_ms = (duration_seconds * 1000 + started_ms) - now_ms;
        if (duration_seconds * 1000 + 1U < display_ms) {
            display_ms = 0;
        }
    }
    seconds = (display_ms % 60000) / 1000;
    time_text[0] = '\0';
    itoa_result = itoa(display_ms / 60000, number_buffer, 10);
    libc_strcat(time_text, itoa_result);
    libc_strcat(time_text, ":");
    if (seconds < 10) {
        libc_strcat(time_text, "0");
    }
    itoa_result = itoa(seconds, number_buffer, 10);
    libc_strcat(time_text, itoa_result);
    GFX_set_text_size(&g_display, '\x02');
    pico_display_clear_row(argument_1);
    pico_display_center_text(argument_1, time_text);
    GFX_set_text_size(&g_display, '\x01');
    pico_display_flush();
    return;
}

/* Clears inclusive row range clamped to last row7.
 * Firmware address: 0x200095d8. */
void pico_display_clear_rows(uint32_t first_row, uint32_t last_row)
{
    uint32_t end_row_exclusive;

    if (last_row < 8) {
        end_row_exclusive = last_row + 1 & 0xff;
    } else {
        end_row_exclusive = 8;
    }
    for (; first_row < end_row_exclusive; first_row = first_row + 1 & 0xff) {
        pico_display_clear_row((short)first_row);
    }
    return;
}

/* Draws Mash/Pass Thru/Adj1..4/StopPos label for valve position.
 * Firmware address: 0x200095fc. */
void pico_display_valve_position(short argument_1, uint32_t position)
{
    char *position_label;

    pico_display_clear_row(argument_1);
    switch (position) {
    case 1:
        position_label = "Mash";
        break;
    case 2:
        position_label = "Pass Thru";
        break;
    case 3:
        position_label = "Adj 1";
        break;
    case 4:
        position_label = "Adj 2";
        break;
    case 5:
        position_label = "Adj 4";
        break;
    case 6:
        position_label = "Adj 3";
        break;
    default:
        position_label = "StopPos";
    }
    pico_display_center_text(argument_1, position_label);
    return;
}

/* Two-row Wort/Steam/Goal readout using requested units.
 * Firmware address: 0x20009768. */
void pico_display_temperature_columns(short argument_1, uint32_t argument_2,
                                      uint32_t wort_temperature_low, uint32_t wort_temperature_high,
                                      double argument_5, double __aeabi_ddiv_result)
{
    int __aeabi_d2iz_result;
    char *text;
    uint16_t row;
    double converted_temperature_bits;
    char number_buffer[12];

    row = argument_1 + 1U & 0xff;
    pico_display_clear_row(argument_1);
    pico_display_clear_row(row);
    if (g_language == '\x01') {
        text = "Mosto   Vapor    Meta";
    } else if (g_language == '\x02') {
        text = "Mout    Vapeur  Cible";
    } else {
        text = "Wort    Steam    Goal";
    }
    pico_display_string_at_cell(argument_1, 0, text);
    if (g_temperature_unit_celsius == '\0') {
        pico_display_text_at_cell(row, 0, "   F        F       F");
        __aeabi_d2iz_result =
            __aeabi_d2iz((double)pico_join_u32_u32(wort_temperature_high, wort_temperature_low));
        text = itoa(__aeabi_d2iz_result, number_buffer, 10);
        pico_display_right_aligned_text(row, 0, text, 3, ' ');
        __aeabi_d2iz_result = __aeabi_d2iz(argument_5);
        text = itoa(__aeabi_d2iz_result, number_buffer, 10);
    } else {
        pico_display_text_at_cell(row, 0, "   C        C       C");
        converted_temperature_bits = __aeabi_dsub(
            (double)pico_join_u32_u32(wort_temperature_high, wort_temperature_low), 32.0);
        converted_temperature_bits = __aeabi_ddiv(converted_temperature_bits, 1.8);
        __aeabi_d2iz_result = __aeabi_d2iz(converted_temperature_bits);
        text = itoa(__aeabi_d2iz_result, number_buffer, 10);
        pico_display_right_aligned_text(row, 0, text, 3, ' ');
        converted_temperature_bits = __aeabi_dsub(argument_5, 32.0);
        converted_temperature_bits = __aeabi_ddiv(converted_temperature_bits, 1.8);
        __aeabi_d2iz_result = __aeabi_d2iz(converted_temperature_bits);
        text = itoa(__aeabi_d2iz_result, number_buffer, 10);
    }
    pico_display_right_aligned_text(row, 9, text, 3, ' ');
    __aeabi_d2iz_result = __aeabi_dcmpeq(__aeabi_ddiv_result, 0.0);
    if (__aeabi_d2iz_result == 0) {
        if (g_temperature_unit_celsius != '\0') {
            converted_temperature_bits = __aeabi_dsub(__aeabi_ddiv_result, 32.0);
            __aeabi_ddiv_result = __aeabi_ddiv(converted_temperature_bits, 1.8);
        }
        __aeabi_d2iz_result = __aeabi_d2iz(__aeabi_ddiv_result);
        text = itoa(__aeabi_d2iz_result, number_buffer, 10);
        pico_display_right_aligned_text(row, 0x11, text, 3, ' ');
    } else {
        pico_display_text_at_cell(row, 0x11, " - ");
    }
    return;
}

/* One-line W:/S:/G: readout with units and dash for zero target.
 * Firmware address: 0x200098f0. */
void pico_display_temperature_summary(uint32_t wort_temperature_low, uint32_t wort_temperature_high,
                                      uint32_t steam_temperature_low,
                                      uint32_t steam_temperature_high, double __aeabi_ddiv_result,
                                      uint16_t argument_6)
{
    int __aeabi_dcmpeq_result;
    char *itoa_result;
    double __aeabi_dsub_result_2;
    double __aeabi_dsub_result;
    double left;
    char unit_text[4];
    char number_buffer[12];
    char summary_line[24];

    __aeabi_dsub_result = (double)pico_join_u32_u32(steam_temperature_high, steam_temperature_low);
    __aeabi_dsub_result_2 = (double)pico_join_u32_u32(wort_temperature_high, wort_temperature_low);
    if (g_temperature_unit_celsius == '\0') {
        libc_strcpy(unit_text, "F");
    } else {
        libc_strcpy(unit_text, "C");
        __aeabi_dsub_result_2 = __aeabi_dsub(
            (double)pico_join_u32_u32(wort_temperature_high, wort_temperature_low), 32.0);
        __aeabi_dsub_result_2 = __aeabi_ddiv(__aeabi_dsub_result_2, 1.8);
        __aeabi_dsub_result = __aeabi_dsub(
            (double)pico_join_u32_u32(steam_temperature_high, steam_temperature_low), 32.0);
        __aeabi_dsub_result = __aeabi_ddiv(__aeabi_dsub_result, 1.8);
        __aeabi_dcmpeq_result = __aeabi_dcmpeq(__aeabi_ddiv_result, 0.0);
        if (__aeabi_dcmpeq_result == 0) {
            left = __aeabi_dsub(__aeabi_ddiv_result, 32.0);
            __aeabi_ddiv_result = __aeabi_ddiv(left, 1.8);
        }
    }
    libc_strcpy(summary_line, "W:");
    __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_dsub_result_2);
    itoa_result = itoa(__aeabi_dcmpeq_result, number_buffer, 10);
    libc_strcat(summary_line, itoa_result);
    libc_strcat(summary_line, unit_text);
    libc_strcat(summary_line, " ");
    libc_strcat(summary_line, "S:");
    __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_dsub_result);
    itoa_result = itoa(__aeabi_dcmpeq_result, number_buffer, 10);
    libc_strcat(summary_line, itoa_result);
    libc_strcat(summary_line, unit_text);
    libc_strcat(summary_line, " ");
    libc_strcat(summary_line, "G:");
    __aeabi_dcmpeq_result = __aeabi_dcmpeq(__aeabi_ddiv_result, 0.0);
    if (__aeabi_dcmpeq_result == 0) {
        __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_ddiv_result);
        itoa_result = itoa(__aeabi_dcmpeq_result, number_buffer, 10);
    } else {
        itoa_result = " - ";
    }
    libc_strcat(summary_line, itoa_result);
    libc_strcat(summary_line, unit_text);
    pico_display_clear_row(argument_6 & 0xff);
    pico_display_center_text(argument_6, summary_line);
    return;
}

/* Erases row beginning at specified character column.
 * Firmware address: 0x20009ef4. */
void pico_display_clear_row_from_column(short argument_1, short argument_2)
{
    GFX_fill_rectangle(&g_display, argument_2 * 6, argument_1 << 3, 0x80,
                       (uint16_t)g_display_text_scale << 3, 0);
    return;
}

/* Draws filled/empty bitmap at one of ten recipe adjustment positions.
 * Firmware address: 0x20009f1c. */
void pico_display_adjustment_box(int box_style, uint32_t box_index)
{
    uint8_t *box_bitmap;
    short x;

    if (box_index < 10) {
        x = (short)box_index * 8 + 0x14;
        if (box_style == 0) {
            GFX_fill_rectangle(&g_display, x, 0x28, 8, 8, 0);
            box_bitmap = &g_display_unknown_04;
        } else {
            if (box_style != 2) {
                return;
            }
            GFX_fill_rectangle(&g_display, x, 0x28, 8, 8, 0);
            box_bitmap = &g_display_unknown_02;
        }
        GFX_draw_bitmap(&g_display, x, 0x28, box_bitmap, 8, 8, 1);
    }
    return;
}
