#include "ui_internal.h"

void pico_display_construct(void)
{
    PicoDisplay *d = &pico_ui_active->display;
    d->width = 128;
    d->height = 64;
    d->cursor_x = d->cursor_y = 0;
    d->text_scale = 1;
    d->foreground = d->background = 0xffff;
    d->rotation = 0;
    d->wrap = 1;
    d->codepage_437 = 0;
    d->command_data_pin = 13;
    d->chip_select_pin = 4;
    d->shared_chip_select_pin = 18;
}
void pico_display_clear(void)
{
    memset(pico_ui_active->display.framebuffer, 0, PICO_FRAMEBUFFER_SIZE);
}
void pico_display_clear_thunk(void)
{
    pico_display_clear();
}
void pico_display_flush(void)
{
    PicoDisplay *d = &pico_ui_active->display;
    static const uint8_t window[] = {0x15, 0, 63, 0x75, 0, 63};
    for (size_t i = 0; i < sizeof window; i++)
        pico_oled_command(d, window[i]);
    ui_write(d->chip_select_pin, 1);
    ui_write(d->shared_chip_select_pin, 1);
    ui_write(d->command_data_pin, 1);
    ui_write(d->chip_select_pin, 0);
    ui_write(d->shared_chip_select_pin, 0);
    ui_delay(1);
    for (unsigned x = 0; x < 128; x += 2)
        for (unsigned page = 0; page < 1024; page += 128) {
            unsigned left = d->framebuffer[page + x], right = d->framebuffer[page + x + 1];
            for (unsigned bit = 0; bit < 8; bit++)
                ui_spi(((left >> bit) & 1 ? 0xf0 : 0) | ((right >> bit) & 1 ? 15 : 0));
        }
    ui_write(d->chip_select_pin, 1);
    ui_write(d->shared_chip_select_pin, 1);
}
void pico_display_center_bitmap(short y, const PicoBitmap *bitmap)
{
    pico_graphics_bitmap(&pico_ui_active->display, 64 - (short)((uint32_t)bitmap->width >> 1), y,
                         bitmap->pixels, (short)bitmap->width, (short)bitmap->height, 1);
}
const uint8_t *pico_display_digit_bitmap(int digit)
{
    return pico_ui_digits[digit - 1u < 9 ? (unsigned)digit : 0];
}
void pico_display_string_at_cell(short row, short column, const char *text)
{
    pico_display_text_at_cell(row, column, text);
}
void pico_display_stop_progress_indicator(PicoControlSignals *signals)
{
    uint8_t first = signals->progress_start_column;
    signals->progress_active = 0;
    pico_graphics_fill_rectangle(&pico_ui_active->display, first * 6, signals->progress_row * 8,
                                 (signals->progress_end_column - first) * 6, 8, 0);
    pico_display_flush();
}
char *pico_format_float_decimal(float value, char *destination, uint32_t decimals)
{
    ui_itoa(ui_int(value), destination, 10);
    if (signbit(value))
        value = -value;
    if (decimals) {
        strcat(destination, ".");
        float fraction = value - (float)ui_int(value);
        uint32_t scale = 1;
        for (uint32_t i = 0; i < decimals; i++)
            scale *= 10;
        fraction *= (float)scale;
        char digits[12];
        ui_itoa((int)ui_uint(fraction), digits, 10);
        size_t length = strlen(digits);
        for (uint32_t i = (uint32_t)length; i < decimals; i++)
            strcat(destination, "0");
        strcat(destination, digits);
    }
    return destination;
}
static void large_digits(const char *text, int16_t x, int16_t y, int decimal_y, unsigned limit)
{
    for (unsigned i = 0; text[i] && i < limit; i++) {
        if (text[i] == '.') {
            pico_graphics_bitmap(&pico_ui_active->display, x, decimal_y, pico_ui_bitmap_colon_dot,
                                 8, 8, 1);
            x += 8;
        } else {
            pico_graphics_bitmap(&pico_ui_active->display, x, y,
                                 pico_display_digit_bitmap(text[i] - '0'), 16, 16, 1);
            x += 16;
        }
    }
}
void pico_display_recipe_decimal_metric(void)
{
    double value = pico_ui_active->app->recipe.decimal_metric;
    char text[24];
    pico_format_float_decimal((float)value, text, 1);
    pico_display_text_at_pixels(42, 17, "% ABV");
    large_digits(text, 31 - (value > 9.9 ? 3 : 2) * 8, 16, 25, 5);
}
void pico_display_recipe_integer_metric(void)
{
    int value = pico_ui_active->app->recipe.integer_metric;
    char text[16];
    ui_itoa(value, text, 10);
    int digits = value < 100 ? (value < 10 ? 1 : 2) : 3;
    pico_display_text_at_pixels(42, 87, "IBU");
    large_digits(text, (12 - digits) * 8 + 3, 16, 25, 16);
}
void pico_display_target_temperature(short row)
{
    double target = pico_ui_active->app->controller.wort_target_fahrenheit;
    if (target == 0.0)
        return;
    char line[32];
    snprintf(line, sizeof line, "Target: %d%c", ui_int(ui_display_temperature(target)),
             pico_ui_active->network->settings->temperature_unit_celsius ? 'C' : 'F');
    pico_display_center_text(row, line);
}
void pico_display_large_timer(int minutes, uint32_t seconds, int show_seconds)
{
    char text[16];
    ui_itoa(minutes / 60, text, 10);
    int lead = minutes / 60 < 10 ? 12 : 4;
    int x = lead + 30;
    for (const char *c = text; *c; c++) {
        pico_graphics_bitmap(&pico_ui_active->display, x, 24, pico_display_digit_bitmap(*c - '0'),
                             16, 16, 1);
        x += 16;
    }
    pico_graphics_bitmap(&pico_ui_active->display, x, 24, pico_ui_bitmap_colon_dot, 8, 16, 1);
    x += 8;
    if (minutes % 60 < 10) {
        pico_graphics_bitmap(&pico_ui_active->display, x, 24, pico_ui_digits[0], 16, 16, 1);
        x += 16;
    }
    ui_itoa(minutes % 60, text, 10);
    for (const char *c = text; *c; c++) {
        pico_graphics_bitmap(&pico_ui_active->display, x, 24, pico_display_digit_bitmap(*c - '0'),
                             16, 16, 1);
        x += 16;
    }
    if (show_seconds) {
        char suffix[20];
        if ((int32_t)seconds < 0)
            seconds = 0;
        snprintf(suffix, sizeof suffix, seconds < 10 ? ":0%u" : ":%u", (unsigned)seconds);
        pico_display_text_at_pixels(24, x, suffix);
        pico_display_text_at_pixels(24, lead, "Time");
        pico_display_text_at_pixels(32, lead, "Left");
    }
}
void pico_display_large_temperature(double fahrenheit, int show_decimal)
{
    double value = ui_display_temperature(fahrenheit);
    int digits = value > 99.9 ? 4 : value > 9.9 ? 3 : 2;
    if (!show_decimal)
        digits--;
    int width = digits * 16 + (show_decimal ? 14 : 6);
    int x = (128 - width) > 0 ? (128 - width) / 2 : 0;
    if (!show_decimal)
        x += 4;
    char text[24];
    pico_format_float_decimal((float)value, text, 1);
    for (unsigned i = 0; text[i] && i < 5; i++) {
        if (text[i] == '.') {
            if (!show_decimal)
                break;
            pico_graphics_bitmap(&pico_ui_active->display, x, 33, pico_ui_bitmap_colon_dot, 8, 8,
                                 1);
            x += 8;
        } else {
            pico_graphics_bitmap(&pico_ui_active->display, x, 24,
                                 pico_display_digit_bitmap(text[i] - '0'), 16, 16, 1);
            x += 16;
        }
    }
    pico_display_text_at_pixels(
        24, x, pico_ui_active->network->settings->temperature_unit_celsius ? "C" : "F");
}
void pico_display_large_temperature_with_decimal(double fahrenheit)
{
    pico_display_large_temperature(fahrenheit, 1);
}
void pico_display_temperature_columns(short row, double wort, double steam, double target)
{
    uint16_t next = (row + 1u) & 255;
    pico_display_clear_row(row);
    pico_display_clear_row(next);
    static const char *labels[3] = {"Wort    Steam    Goal", "Mosto   Vapor    Meta",
                                    "Mout    Vapeur  Cible"};
    unsigned language = pico_ui_active->network->settings->language;
    pico_display_string_at_cell(row, 0, labels[language < 3 ? language : 0]);
    pico_display_text_at_cell(next, 0,
                              pico_ui_active->network->settings->temperature_unit_celsius
                                  ? "   C        C       C"
                                  : "   F        F       F");
    char text[16];
    ui_itoa(ui_int(ui_display_temperature(wort)), text, 10);
    pico_display_right_aligned_text(next, 0, text, 3, ' ');
    ui_itoa(ui_int(ui_display_temperature(steam)), text, 10);
    pico_display_right_aligned_text(next, 9, text, 3, ' ');
    if (target != 0.0) {
        ui_itoa(ui_int(ui_display_temperature(target)), text, 10);
        pico_display_right_aligned_text(next, 17, text, 3, ' ');
    } else
        pico_display_text_at_cell(next, 17, " - ");
}
void pico_display_temperature_summary(double wort, double steam, double target, uint16_t row)
{
    char line[64], goal[20];
    char unit = pico_ui_active->network->settings->temperature_unit_celsius ? 'C' : 'F';
    if (target == 0.0)
        strcpy(goal, " - ");
    else
        ui_itoa(ui_int(ui_display_temperature(target)), goal, 10);
    snprintf(line, sizeof line, "W:%d%c S:%d%c G:%s%c", ui_int(ui_display_temperature(wort)), unit,
             ui_int(ui_display_temperature(steam)), unit, goal, unit);
    pico_display_clear_row(row & 255);
    pico_display_center_text(row, line);
}
