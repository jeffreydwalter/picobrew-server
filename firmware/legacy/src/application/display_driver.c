/* Pico Pro 0.1.34 — display driver. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Bounds/rotation transform and bit set/clear in128-column framebuffer.
 * Firmware address: 0x20013768. */
void oled_draw_pixel(void *display, int x, int y, int color)
{
    uint8_t gfx_get_rotation_result;
    short gfx_get_width_result;
    uint8_t byte_value_3;
    uint32_t value_4;
    uint32_t value_5;
    int value_6;

    gfx_get_width_result = GFX_get_width(display);
    if (((gfx_get_width_result <= x) ||
         (gfx_get_width_result = GFX_get_height(display), gfx_get_width_result <= y)) ||
        (x < 0)) {
        return;
    }
    if (y < 0) {
        return;
    }
    gfx_get_rotation_result = GFX_get_rotation(display);
    gfx_get_width_result = (short)y;
    if (gfx_get_rotation_result == '\x02') {
        y = (int)(short)((*(short *)((int)display + 8) + -1) - (uint16_t)x);
        value_4 = (int)(short)((*(short *)((int)display + 10) + -1) - gfx_get_width_result);
    } else if (gfx_get_rotation_result == '\x03') {
        value_4 = (int)(short)((*(short *)((int)display + 10) + -1) - ((uint16_t)x & 0xff));
    } else {
        value_4 = y;
        y = x;
        if (gfx_get_rotation_result == '\x01') {
            value_4 = x & 0xff;
            y = (int)(short)((*(short *)((int)display + 8) + -1) - gfx_get_width_result);
        }
    }
    value_5 = value_4 & 0x80000007;
    value_6 = ((int)value_4 / 8) * 0x80 + y;
    if (color == 1) {
        if ((int)value_5 < 0) {
            value_5 = (value_5 - 1 | 0xfffffff8) + 1;
        }
        byte_value_3 =
            (uint8_t)(1 << ((int)(short)value_5 & 0xffU)) | (&g_display_framebuffer)[value_6];
    } else {
        if ((int)value_5 < 0) {
            value_5 = (value_5 - 1 | 0xfffffff8) + 1;
        }
        byte_value_3 =
            (&g_display_framebuffer)[value_6] & ~(uint8_t)(1 << ((int)(short)value_5 & 0xffU));
    }
    (&g_display_framebuffer)[value_6] = byte_value_3;
    return;
}

/* Toggles display control pins and writes command byte through SPI.
 * Firmware address: 0x20013824. */
void oled_write_command(void *display, uint8_t command)
{
    digitalWrite((uint32_t) * (uint8_t *)((int)display + 0x24), 1);
    digitalWrite((uint32_t) * (uint8_t *)((int)display + 0x25), 1);
    digitalWrite((uint32_t) * (uint8_t *)((int)display + 0x22), 0);
    digitalWrite((uint32_t) * (uint8_t *)((int)display + 0x24), 0);
    digitalWrite((uint32_t) * (uint8_t *)((int)display + 0x25), 0);
    SPI_transfer_byte(&global_spi, command);
    digitalWrite((uint32_t) * (uint8_t *)((int)display + 0x24), 1);
    digitalWrite((uint32_t) * (uint8_t *)((int)display + 0x25), 1);
    return;
}
