/* Native monochrome renderer reconstructed from the original OLED/GFX paths. */
#include "ui_internal.h"

void pico_graphics_pixel(PicoDisplay *d, int x, int y, int color)
{
    if (x < 0 || y < 0 || x >= d->width || y >= d->height)
        return;
    int px = x, py = y;
    switch (d->rotation) {
    case 1:
        px = 127 - y;
        py = x & 255;
        break;
    case 2:
        px = 127 - x;
        py = 63 - y;
        break;
    case 3:
        px = y;
        py = 63 - (x & 255);
        break;
    default:
        break;
    }
    uint8_t *cell = &d->framebuffer[(py / 8) * 128 + px];
    uint8_t mask = (uint8_t)(1u << (py & 7));
    if (color == 1)
        *cell |= mask;
    else
        *cell &= (uint8_t)~mask;
}
void pico_graphics_bitmap(PicoDisplay *d, int16_t x, int16_t y, const uint8_t *pixels,
                          int16_t width, int16_t height, uint16_t color)
{
    int stride = (width + 7) / 8;
    for (int row = 0; row < height; row++)
        for (int column = 0; column < width; column++)
            if (pixels[row * stride + column / 8] & (0x80u >> (column & 7)))
                pico_graphics_pixel(d, (int16_t)(x + column), (int16_t)(y + row), color);
}
void pico_graphics_fill_rectangle(PicoDisplay *d, int16_t x, int16_t y, int16_t width,
                                  int16_t height, uint16_t color)
{
    for (int column = x; column < (int)x + width; column++)
        for (int row = y; row < (int)y + height; row++)
            pico_graphics_pixel(d, column, row, color);
}
static void swap16(int16_t *a, int16_t *b)
{
    int16_t t = *a;
    *a = *b;
    *b = t;
}
void pico_graphics_line(PicoDisplay *d, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        uint16_t color)
{
    bool steep = abs((int)y1 - y0) > abs((int)x1 - x0);
    if (steep) {
        swap16(&x0, &y0);
        swap16(&x1, &y1);
    }
    if (x0 > x1) {
        swap16(&x0, &x1);
        swap16(&y0, &y1);
    }
    int16_t dx = x1 - x0, dy = abs((int)y1 - y0), error = dx / 2, ystep = y0 < y1 ? 1 : -1;
    for (int x = x0; x <= x1; x++) {
        pico_graphics_pixel(d, steep ? y0 : x, steep ? x : y0, color);
        error -= dy;
        if (error < 0) {
            y0 += ystep;
            error += dx;
        }
    }
}
static void circle_corners(PicoDisplay *d, int16_t cx, int16_t cy, int16_t radius, uint8_t corners,
                           uint16_t color)
{
    int16_t f = 1 - radius, dx = 1, dy = -2 * radius, x = 0, y = radius;
    while (x < y) {
        if (f >= 0) {
            y--;
            dy += 2;
            f += dy;
        }
        x++;
        dx += 2;
        f += dx;
        if (corners & 4) {
            pico_graphics_pixel(d, cx + x, cy + y, color);
            pico_graphics_pixel(d, cx + y, cy + x, color);
        }
        if (corners & 2) {
            pico_graphics_pixel(d, cx + x, cy - y, color);
            pico_graphics_pixel(d, cx + y, cy - x, color);
        }
        if (corners & 8) {
            pico_graphics_pixel(d, cx - y, cy + x, color);
            pico_graphics_pixel(d, cx - x, cy + y, color);
        }
        if (corners & 1) {
            pico_graphics_pixel(d, cx - y, cy - x, color);
            pico_graphics_pixel(d, cx - x, cy - y, color);
        }
    }
}
void pico_graphics_round_rectangle(PicoDisplay *d, int16_t x, int16_t y, int16_t w, int16_t h,
                                   int16_t r, uint16_t color)
{
    pico_graphics_line(d, x + r, y, x + w - r - 1, y, color);
    pico_graphics_line(d, x + r, y + h - 1, x + w - r - 1, y + h - 1, color);
    pico_graphics_line(d, x, y + r, x, y + h - r - 1, color);
    pico_graphics_line(d, x + w - 1, y + r, x + w - 1, y + h - r - 1, color);
    circle_corners(d, x + r, y + r, r, 1, color);
    circle_corners(d, x + w - r - 1, y + r, r, 2, color);
    circle_corners(d, x + w - r - 1, y + h - r - 1, r, 4, color);
    circle_corners(d, x + r, y + h - r - 1, r, 8, color);
}
void pico_graphics_fill_triangle(PicoDisplay *d, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                 int16_t x2, int16_t y2, uint16_t color)
{
    if (y0 > y1) {
        swap16(&y0, &y1);
        swap16(&x0, &x1);
    }
    if (y1 > y2) {
        swap16(&y2, &y1);
        swap16(&x2, &x1);
    }
    if (y0 > y1) {
        swap16(&y0, &y1);
        swap16(&x0, &x1);
    }
    if (y0 == y2) {
        int a = x0, b = x0;
        if (x1 < a)
            a = x1;
        else if (x1 > b)
            b = x1;
        if (x2 < a)
            a = x2;
        else if (x2 > b)
            b = x2;
        pico_graphics_line(d, a, y0, b, y0, color);
        return;
    }
    int32_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1,
            dy12 = y2 - y1, sa = 0, sb = 0;
    int y, last = y1 == y2 ? y1 : y1 - 1;
    for (y = y0; y <= last; y++) {
        int a = x0 + sa / dy01, b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) {
            int t = a;
            a = b;
            b = t;
        }
        pico_graphics_line(d, a, y, b, y, color);
    }
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++) {
        int a = x1 + sa / dy12, b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) {
            int t = a;
            a = b;
            b = t;
        }
        pico_graphics_line(d, a, y, b, y, color);
    }
}
void pico_graphics_cursor(PicoDisplay *d, int16_t x, int16_t y)
{
    d->cursor_x = x;
    d->cursor_y = y;
}
void pico_graphics_text_color(PicoDisplay *d, uint16_t color)
{
    d->foreground = d->background = color;
}
void pico_graphics_text_colors(PicoDisplay *d, uint16_t foreground, uint16_t background)
{
    d->foreground = foreground;
    d->background = background;
}
void pico_graphics_text_scale(PicoDisplay *d, uint8_t scale)
{
    d->text_scale = scale ? scale : 1;
}
void pico_graphics_rotation(PicoDisplay *d, uint8_t rotation)
{
    d->rotation = rotation & 3;
    d->width = (d->rotation & 1) ? 64 : 128;
    d->height = (d->rotation & 1) ? 128 : 64;
}
void pico_graphics_write(PicoDisplay *d, uint8_t c)
{
    int scale = d->text_scale;
    if (c == '\n') {
        d->cursor_y = (int16_t)(d->cursor_y + scale * 8);
        d->cursor_x = 0;
        return;
    }
    if (c == '\r')
        return;
    if (d->wrap && d->width <= d->cursor_x + scale * 6) {
        d->cursor_x = 0;
        d->cursor_y = (int16_t)(d->cursor_y + scale * 8);
    }
    uint8_t glyph = c;
    if (!d->codepage_437 && glyph > 175)
        glyph++;
    int x = d->cursor_x, y = d->cursor_y;
    if (x < d->width && y < d->height && x + scale * 6 - 1 >= 0 && y + scale * 8 - 1 >= 0)
        for (int column = 0; column < 6; column++) {
            uint8_t bits = column == 5 ? 0 : pico_ui_builtin_font[glyph * 5 + column];
            for (int row = 0; row < 8; row++, bits >>= 1) {
                if ((bits & 1) || d->foreground != d->background)
                    pico_graphics_fill_rectangle(d, x + column * scale, y + row * scale, scale,
                                                 scale, (bits & 1) ? d->foreground : d->background);
            }
        }
    d->cursor_x = (int16_t)(d->cursor_x + 6 * scale);
}
void pico_graphics_text(PicoDisplay *d, const char *text)
{
    while (*text)
        pico_graphics_write(d, (uint8_t)*text++);
}
void pico_oled_command(PicoDisplay *d, uint8_t command)
{
    ui_write(d->chip_select_pin, 1);
    ui_write(d->shared_chip_select_pin, 1);
    ui_write(d->command_data_pin, 0);
    ui_write(d->chip_select_pin, 0);
    ui_write(d->shared_chip_select_pin, 0);
    ui_spi(command);
    ui_write(d->chip_select_pin, 1);
    ui_write(d->shared_chip_select_pin, 1);
}
void pico_oled_initialize(PicoDisplay *d)
{
    static const uint8_t commands[] = {
        0xae, 0xb3, 0xf1, 0xa8, 0x3f, 0xa2, 0x4c, 0xa1, 0x00, 0xad, 0x02, 0xa0, 0x56, 0x86,
        0xb8, 0x01, 0x11, 0x22, 0x32, 0x43, 0x54, 0x65, 0x76, 0x81, 0x7f, 0xb2, 0x51, 0xb1,
        0x55, 0xb4, 0x02, 0xb0, 0x28, 0xbe, 0x1c, 0xbf, 0x0f, 0xa4, 0x23, 0x23, 0xaf};
    ui_pin_mode(d->command_data_pin, 1);
    ui_pin_mode(d->chip_select_pin, 1);
    ui_pin_mode(d->shared_chip_select_pin, 1);
    pico_ui_active->hardware->spi_begin(pico_ui_active->hardware_context);
    for (size_t i = 0; i < sizeof commands; i++)
        pico_oled_command(d, commands[i]);
}
void pico_oled_invert(PicoDisplay *d, bool invert)
{
    pico_oled_command(d, invert ? 0xa7 : 0xa4);
}
