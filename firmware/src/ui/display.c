#include "ui_internal.h"

void pico_display_center_message(const char *text)
{
    uint32_t text_length;

    text_length = strlen(text);
    pico_graphics_cursor(
        &pico_ui_active->display,
        0x40 - (short)((int)((uint32_t)pico_ui_active->display.text_scale * 6 * text_length) / 2),
        0x1c);
    pico_graphics_text(&pico_ui_active->display, text);
    return;
}

void pico_display_refresh_current_page(void)
{
    pico_display_clear();
    /* The recovered entry always redraws page zero; its historical name is kept.
     */
    pico_ui_active->app->carousel.pages[0]();
    pico_display_flush();
    return;
}

void pico_screen_set_recipe_name(PicoScreenState *screen, const char *recipe_name)
{
    ui_assert(!ui_empty(recipe_name), "!CString::isNullOrEmpty(_recipeName)");
    ui_cstring_copy(screen->recipe_name, sizeof screen->recipe_name, recipe_name);
}

void pico_screen_set_step(PicoScreenState *screen, uint32_t component, const char *step_name)
{
    ui_assert(!ui_empty(step_name), "!CString::isNullOrEmpty(_stepName)");
    ui_cstring_copy(screen->step_name, sizeof screen->step_name, step_name);
    screen->progress_component = component;
    char log_buffer[128];
    snprintf(log_buffer, sizeof log_buffer, "setStep comp = %d stepName = \"%s\"", (int)component,
             step_name);
    ui_log("I", "SCR", log_buffer);
}

void pico_screen_add_lines(PicoTextScreen *screen, int count, ...)
{
    va_list arguments;
    va_start(arguments, count);
    for (int index = 0; index < count; ++index) {
        const char *line = va_arg(arguments, const char *);
        ui_assert(!ui_empty(line), "!CString::isNullOrEmpty(line)");
        if (screen->line_count < PICO_TEXT_LINE_CAPACITY)
            screen->lines[screen->line_count++] = line;
        else
            ui_log("E", "SCREEN", "Too many lines");
    }
    va_end(arguments);
}

void pico_display_advance_carousel(PicoPageCarousel *carousel)
{
    uint32_t now_ms;
    uint32_t next_page;

    now_ms = ui_millis();
    if (carousel->interval_ms < now_ms - carousel->last_update_ms) {
        now_ms = ui_millis();
        carousel->last_update_ms = now_ms;
        next_page = carousel->page_index + 1;
        carousel->page_index = next_page;
        if ((int)carousel->page_count <= (int)next_page) {
            carousel->page_index = 0;
        }
    }
    if (carousel->pages[carousel->page_index] == (PicoPageCallback)0x0) {
        return;
    }
    pico_display_clear();
    (*carousel->pages[carousel->page_index])();
    pico_display_flush();
    return;
}

void pico_display_initialize(void)
{
    pico_oled_initialize(&pico_ui_active->display);
    pico_graphics_text_colors(&pico_ui_active->display, 1, 0);
    pico_graphics_rotation(&pico_ui_active->display, '\x02');
    pico_oled_command(&pico_ui_active->display, 0x81);
    pico_oled_command(&pico_ui_active->display, '~');
    pico_display_clear();
    return;
}

void pico_display_text_at_cell(short row, short column, const char *text)
{
    pico_graphics_cursor(&pico_ui_active->display, column * 6, row << 3);
    pico_graphics_text(&pico_ui_active->display, text);
    return;
}

void pico_display_center_text(short row, const char *text)
{
    uint32_t text_length;
    int text_pixel_width;
    short x;

    text_length = strlen(text);
    text_pixel_width = (uint32_t)pico_ui_active->display.text_scale * 6 * text_length;
    if (text_pixel_width < 0x80) {
        x = (short)((uint32_t)((0x80 - text_pixel_width) * 0x8000) >> 0x10);
    } else {
        x = 0;
    }
    pico_graphics_cursor(&pico_ui_active->display, x, row << 3);
    pico_graphics_text(&pico_ui_active->display, text);
    return;
}

void pico_display_text_at_pixels(short y, short x, const char *text)
{
    pico_graphics_cursor(&pico_ui_active->display, x, y);
    pico_graphics_text(&pico_ui_active->display, text);
    return;
}

void pico_display_device_bitmap(void)
{
    pico_graphics_bitmap(&pico_ui_active->display, 6, 9, pico_ui_bitmap_device, 0x28, 0x30, 1);
    return;
}

void pico_display_logo(void)
{
    pico_graphics_bitmap(&pico_ui_active->display, 0x18, 2, pico_ui_bitmap_logo, 0x50, 0x15, 1);
    pico_display_flush();
    return;
}

void pico_display_right_aligned_text(short row, short column, const char *text,
                                     uint32_t field_width, char padding_character)
{
    uint32_t text_length;
    const char *text_cursor;
    uint8_t padding_index;
    uint32_t padding_count;

    pico_graphics_cursor(&pico_ui_active->display, column * 6, row << 3);
    text_length = strlen(text);
    if (text_length < field_width) {
        padding_index = 0;
        while (true) {
            text_length = strlen(text);
            padding_count = (uint32_t)padding_index;
            padding_index = padding_index + 1;
            if (field_width - text_length <= padding_count)
                break;
            pico_graphics_write(&pico_ui_active->display, padding_character);
        }
    }
    text_cursor = text + -1;
    while (true) {
        text_cursor = text_cursor + 1;
        if (*text_cursor == '\0')
            break;
        pico_graphics_write(&pico_ui_active->display, *text_cursor);
    }
    return;
}

void pico_display_character_on_row(short x, short row, char character)
{
    pico_graphics_cursor(&pico_ui_active->display, x, row << 3);
    pico_graphics_write(&pico_ui_active->display, character);
    return;
}

void pico_display_character_at_pixels(short x, short y, char character)
{
    pico_graphics_cursor(&pico_ui_active->display, x, y);
    pico_graphics_write(&pico_ui_active->display, character);
    return;
}

void pico_display_draw_content_frame(void)
{
    pico_graphics_round_rectangle(&pico_ui_active->display, 0, 0xb, 0x80, 0x2b, 0x14, 1);
    return;
}

void pico_display_draw_titled_frame(const char *title)
{
    pico_display_center_text(0, title);
    pico_display_draw_content_frame();
    return;
}

void pico_display_draw_program_frame(void)
{
    pico_display_draw_titled_frame(pico_ui_active->app->recipe.program->title);
    return;
}

void pico_display_large_timer_with_seconds(int total_minutes, uint32_t seconds)
{
    pico_display_large_timer(total_minutes, seconds, 1);
    return;
}

void pico_display_device_bitmap_variant_a(void)
{
    pico_graphics_bitmap(&pico_ui_active->display, 6, 9, pico_ui_bitmap_device, 0x28, 0x30, 1);
    pico_graphics_fill_rectangle(&pico_ui_active->display, 0xe, 0x1f, 0x18, 5, 0);
    pico_graphics_bitmap(&pico_ui_active->display, 0xe, 0x1f, pico_ui_bitmap_connection_failed,
                         0x18, 5, 1);
    return;
}

void pico_display_device_bitmap_variant_b(void)
{
    pico_graphics_bitmap(&pico_ui_active->display, 6, 9, pico_ui_bitmap_device, 0x28, 0x30, 1);
    pico_graphics_fill_rectangle(&pico_ui_active->display, 0xe, 0x1f, 0x18, 5, 0);
    pico_graphics_bitmap(&pico_ui_active->display, 0xe, 0x1f, pico_ui_bitmap_connection_ready, 0x18,
                         5, 1);
    return;
}

void pico_display_clear_row(short row)
{
    pico_graphics_fill_rectangle(&pico_ui_active->display, 0, row << 3, 0x80,
                                 (uint16_t)pico_ui_active->display.text_scale << 3, 0);
    return;
}

void pico_display_update_timer(short row, int started_ms, int duration_seconds)
{
    uint32_t now_ms;
    char *itoa_result;
    uint32_t display_ms;
    uint32_t seconds;
    char number_buffer[24];
    char time_text[24];

    if (duration_seconds == 0) {
        now_ms = ui_millis();
        display_ms = now_ms - started_ms;
    } else {
        now_ms = ui_millis();
        display_ms = (duration_seconds * 1000 + started_ms) - now_ms;
        if (duration_seconds * 1000 + 1U < display_ms) {
            display_ms = 0;
        }
    }
    seconds = (display_ms % 60000) / 1000;
    time_text[0] = '\0';
    itoa_result = ui_itoa(display_ms / 60000, number_buffer, 10);
    strcat(time_text, itoa_result);
    strcat(time_text, ":");
    if (seconds < 10) {
        strcat(time_text, "0");
    }
    itoa_result = ui_itoa(seconds, number_buffer, 10);
    strcat(time_text, itoa_result);
    pico_graphics_text_scale(&pico_ui_active->display, '\x02');
    pico_display_clear_row(row);
    pico_display_center_text(row, time_text);
    pico_graphics_text_scale(&pico_ui_active->display, '\x01');
    pico_display_flush();
    return;
}

void pico_display_clear_rows(uint32_t first_row, uint32_t last_row)
{
    uint32_t end_row_exclusive;

    if (last_row < 8) {
        end_row_exclusive = (last_row + 1) & 0xff;
    } else {
        end_row_exclusive = 8;
    }
    for (; first_row < end_row_exclusive; first_row = (first_row + 1) & 0xff) {
        pico_display_clear_row((short)first_row);
    }
    return;
}

void pico_display_valve_position(short row, uint32_t position)
{
    char *position_label;

    pico_display_clear_row(row);
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
    pico_display_center_text(row, position_label);
    return;
}

void pico_display_clear_row_from_column(short row, short column)
{
    pico_graphics_fill_rectangle(&pico_ui_active->display, column * 6, row << 3, 0x80,
                                 (uint16_t)pico_ui_active->display.text_scale << 3, 0);
    return;
}

void pico_display_adjustment_box(int box_style, uint32_t box_index)
{
    const uint8_t *box_bitmap;
    short x;

    if (box_index < 10) {
        x = (short)box_index * 8 + 0x14;
        if (box_style == 0) {
            pico_graphics_fill_rectangle(&pico_ui_active->display, x, 0x28, 8, 8, 0);
            box_bitmap = pico_ui_bitmap_adjustment_empty;
        } else {
            if (box_style != 2) {
                return;
            }
            pico_graphics_fill_rectangle(&pico_ui_active->display, x, 0x28, 8, 8, 0);
            box_bitmap = pico_ui_bitmap_adjustment_filled;
        }
        pico_graphics_bitmap(&pico_ui_active->display, x, 0x28, box_bitmap, 8, 8, 1);
    }
    return;
}
