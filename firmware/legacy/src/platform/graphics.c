/* Pico Pro 0.1.34 — graphics. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013ae8.
 */
void GFX_draw_line(void *graphics, short x0, short y0, short x1_2, short y1, uint16_t color)
{
    uint16_t unresolved_input_register_00000026;
    int value_1;
    int value_2;
    uint16_t unresolved_input_register_0000002a;
    int value_3;
    int value_4;
    uint32_t value_5;
    int value_6;
    uint16_t unresolved_input_register_0000002e;
    int x1;
    int value_8;
    int value_9;
    uint32_t value_10;
    int value_11;
    uint32_t value_12;

    x1 = pico_join_u16_u16(unresolved_input_register_0000002e, x1_2);
    value_3 = pico_join_u16_u16(unresolved_input_register_0000002a, y0);
    value_1 = pico_join_u16_u16(unresolved_input_register_00000026, x0);
    value_8 = (int)y1;
    value_11 = value_8 - value_3;
    value_4 = x1 - value_1;
    if (value_11 < 0) {
        value_11 = -value_11;
    }
    if (value_4 < 0) {
        value_4 = -value_4;
    }
    value_2 = value_8;
    value_6 = value_1;
    value_9 = value_3;
    if (value_4 < value_11) {
        value_2 = x1;
        value_6 = value_3;
        value_9 = value_1;
        x1 = value_8;
    }
    value_1 = value_2;
    value_8 = value_6;
    if (x1 < value_6) {
        value_1 = value_9;
        value_8 = x1;
        value_9 = value_2;
        x1 = value_6;
    }
    value_12 = x1 - value_8;
    value_10 = value_1 - value_9 >> 0x1f;
    value_2 = (value_1 - value_9 ^ value_10) - value_10;
    value_10 = (int)(short)value_12 / 2;
    value_5 = 0xffff;
    if (value_9 < value_1) {
        value_5 = 1;
    }
    for (; value_8 <= x1; value_8 = (int)(short)((short)value_8 + 1)) {
        if (value_4 < value_11) {
            value_1 = value_9;
            value_6 = value_8;
        } else {
            value_1 = value_8;
            value_6 = value_9;
        }
        (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(graphics, value_1, value_6, color,
                                                          value_2, value_5, value_3);
        value_10 = (value_10 & 0xffff) - value_2;
        if ((int)(value_10 * 0x10000) < 0) {
            value_10 = (value_10 & 0xffff) + (value_12 & 0xffff);
            value_9 = (int)(short)((short)value_9 + (short)value_5);
        }
    }
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013bb0.
 */
void GFX_draw_rectangle(void *graphics, short x, short y, short width, short height, uint16_t color)
{
    uint16_t unresolved_input_register_00000026;
    uint32_t value_1;
    uint16_t unresolved_input_register_0000002a;
    uint16_t unresolved_input_register_0000002e;
    uint32_t value_2;

    value_1 = pico_join_u16_u16(unresolved_input_register_00000026, x);
    value_2 = value_1;
    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x14))();
    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x14))(
        graphics, value_1, (int)(short)(height + -1 + y),
        pico_join_u16_u16(unresolved_input_register_0000002e, width), color, value_2);
    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x10))(
        graphics, value_1, pico_join_u16_u16(unresolved_input_register_0000002a, y), (int)height,
        color);
    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x10))(
        graphics, (int)(short)(width + -1 + x),
        pico_join_u16_u16(unresolved_input_register_0000002a, y), (int)height, color);
    return;
}

/* Thumb instructions calculate the second endpoint and dispatch drawLine through vtable+0x0c. The
 * vertical wrapper adds height to y; horizontal adds width to x. Firmware address: 0x20013c0e. */
void GFX_draw_fast_vertical_line(int *graphics)
{
    (**(PicoOpaqueFunction **)(*graphics + 0xc))();
    return;
}

/* Thumb instructions calculate the second endpoint and dispatch drawLine through vtable+0x0c. The
 * vertical wrapper adds height to y; horizontal adds width to x. Firmware address: 0x20013c2a. */
void GFX_draw_fast_horizontal_line(int *graphics)
{
    (**(PicoOpaqueFunction **)(*graphics + 0xc))();
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013c44.
 */
void GFX_fill_rectangle(void *graphics, short x, short y, short width, short height, uint16_t color)
{
    uint16_t unresolved_input_register_00000026;
    int value_1;
    uint16_t unresolved_input_register_0000002a;
    uint16_t unresolved_input_register_0000002e;
    int index;
    int value_3;

    value_1 = pico_join_u16_u16(unresolved_input_register_00000026, x);
    value_3 = value_1;
    for (index = value_1;
         index < value_1 + pico_join_u16_u16(unresolved_input_register_0000002e, width);
         index = (int)(short)((short)index + 1)) {
        (**(PicoOpaqueFunction **)(*(int *)graphics + 0x10))(
            graphics, index, pico_join_u16_u16(unresolved_input_register_0000002a, y), (int)height,
            color, value_3);
    }
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013c80.
 */
void GFX_fill_screen(void *graphics, uint16_t color)
{
    uint16_t unresolved_input_register_00000026;

    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x1c))(
        graphics, 0, 0, (int)*(short *)((int)graphics + 0xc), (int)*(short *)((int)graphics + 0xe),
        pico_join_u16_u16(unresolved_input_register_00000026, color));
    return;
}

/* Empty virtual method adjacent to GFX constructor; compatible Adafruit GFX invertDisplay default
 * no-op. Firmware address: 0x20013c9c. */
void GFX_invert_display_noop(void)
{
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013ca0.
 */
void GFX_construct(void *graphics, short width, short height)
{
    *(short *)((int)graphics + 8) = width;
    *(short *)((int)graphics + 10) = height;
    *(short *)((int)graphics + 0xc) = width;
    *(short *)((int)graphics + 0xe) = height;
    *(uint32_t *)((int)graphics + 4) = 0;
    *(uint8_t ***)graphics = &g_graphics_unknown_02;
    *(uint8_t *)((int)graphics + 0x19) = 0;
    *(uint16_t *)((int)graphics + 0x10) = 0;
    *(uint16_t *)((int)graphics + 0x12) = 0;
    *(uint8_t *)((int)graphics + 0x18) = 1;
    *(uint16_t *)((int)graphics + 0x16) = 0xffff;
    *(uint16_t *)((int)graphics + 0x14) = 0xffff;
    *(uint8_t *)((int)graphics + 0x1a) = 1;
    *(uint8_t *)((int)graphics + 0x1b) = 0;
    *(uint32_t *)((int)graphics + 0x1c) = 0;
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013cd0.
 */
void GFX_draw_circle_quadrants(void *graphics, short centerX, short centerY, short radius_2,
                               uint8_t quadrants, uint16_t color)
{
    short short_value_1;
    short index;
    uint32_t value_3;
    short short_value_4;
    uint16_t unresolved_input_register_0000002e;
    int radius;
    uint16_t short_value_6;

    radius = pico_join_u16_u16(unresolved_input_register_0000002e, radius_2);
    short_value_6 = 1 - radius_2;
    short_value_4 = radius_2 * -2;
    value_3 = (uint32_t)quadrants;
    for (index = 0; index < radius; index = index + 1) {
        if ((short_value_6 & 0x8000) == 0) {
            short_value_4 = short_value_4 + 2;
            short_value_6 = short_value_6 + short_value_4;
            radius = (int)(short)((short)radius + -1);
        }
        short_value_6 = short_value_6 + index * 2 + 3;
        short_value_1 = (short)radius;
        if ((int)(value_3 << 0x1d) < 0) {
            (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                graphics, (int)(short)(centerX + 1 + index), (int)(short)(short_value_1 + centerY),
                color);
            (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                graphics, (int)(short)(short_value_1 + centerX), (int)(short)(index + centerY + 1),
                color);
        }
        if ((int)(value_3 << 0x1e) < 0) {
            (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                graphics, (int)(short)(centerX + 1 + index), (int)(short)(centerY - short_value_1),
                color);
            (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                graphics, (int)(short)(short_value_1 + centerX),
                (int)(short)((centerY + -1) - index), color);
        }
        if ((int)(value_3 << 0x1c) < 0) {
            (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                graphics, (int)(short)(centerX - short_value_1), (int)(short)(centerY + 1 + index),
                color);
            (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                graphics, (int)(short)((centerX + -1) - index),
                (int)(short)(short_value_1 + centerY), color);
        }
        if ((int)(value_3 << 0x1f) < 0) {
            (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                graphics, (int)(short)(centerX - short_value_1),
                (int)(short)((centerY + -1) - index), color);
            (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                graphics, (int)(short)((centerX + -1) - index),
                (int)(short)(centerY - short_value_1), color);
        }
    }
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013e70.
 */
void GFX_draw_round_rectangle(void *graphics, short x, short y, short width, short height,
                              short radius, uint16_t color)
{
    short center_x;
    short center_y;
    short center_x_00;
    uint16_t unresolved_input_register_00000026;
    short short_value_1;
    int value_2;

    center_x = x + radius;
    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x14))();
    short_value_1 = height + -1 + y;
    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x14))(
        graphics, (int)center_x, (int)short_value_1, (int)(short)(radius * -2 + width), color);
    center_y = y + radius;
    value_2 = (int)(short)(height + radius * -2);
    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x10))(
        graphics, pico_join_u16_u16(unresolved_input_register_00000026, x), (int)center_y, value_2,
        color);
    (**(PicoOpaqueFunction **)(*(int *)graphics + 0x10))(graphics, (int)(short)(x + width + -1),
                                                         (int)center_y, value_2, color);
    GFX_draw_circle_quadrants(graphics, center_x, center_y, radius, '\x01', color);
    center_x_00 = x + width + ~radius;
    short_value_1 = short_value_1 - radius;
    GFX_draw_circle_quadrants(graphics, center_x_00, center_y, radius, '\x02', color);
    GFX_draw_circle_quadrants(graphics, center_x_00, short_value_1, radius, '\x04', color);
    GFX_draw_circle_quadrants(graphics, center_x, short_value_1, radius, '\b', color);
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013f7c.
 */
void GFX_fill_triangle(void *graphics, short x0, short y0_2, short x1, short y1, short x2, short y2,
                       uint16_t color)
{
    short short_value_1;
    short short_value_2;
    short short_value_3;
    short short_value_4;
    short short_value_5;
    uint16_t unresolved_input_register_00000026;
    uint16_t unresolved_input_register_0000002a;
    uint32_t value_6;
    int value_7;
    short short_value_8;
    short short_value_9;
    uint16_t unresolved_input_register_0000002e;
    uint32_t index;
    uint32_t y0;
    uint32_t value_12;
    int value_13;
    int value_14;
    int value_15;
    bool condition_16;
    uint32_t value_44;

    value_6 = pico_join_u16_u16(unresolved_input_register_0000002a, y0_2);
    index = (uint32_t)y1;
    value_12 = (uint32_t)y2;
    value_13 = pico_join_u16_u16(unresolved_input_register_0000002e, x1);
    y0 = index;
    value_15 = pico_join_u16_u16(unresolved_input_register_00000026, x0);
    if ((int)index < (int)value_6) {
        value_13 = pico_join_u16_u16(unresolved_input_register_00000026, x0);
        y0 = value_6;
        value_6 = index;
        value_15 = pico_join_u16_u16(unresolved_input_register_0000002e, x1);
    }
    value_14 = (int)x2;
    index = y0;
    if ((int)value_12 < (int)y0) {
        value_14 = value_13;
        value_13 = (int)x2;
        index = value_12;
        value_12 = y0;
    }
    value_7 = value_13;
    y0 = index;
    if ((int)index < (int)value_6) {
        value_7 = value_15;
        y0 = value_6;
        value_6 = index;
        value_15 = value_13;
    }
    if (value_6 == value_12) {
        value_13 = value_15;
        if (value_15 <= value_7) {
            if (value_7 < value_15) {
                value_7 = value_15;
            } else {
            }
            value_13 = (int)(short)value_7;
            value_7 = value_15;
        }
        if (value_7 <= value_14) {
            if (value_13 < value_14) {
                value_13 = value_14;
            }
            value_13 = (int)(short)value_13;
            value_14 = value_7;
        }
        (**(PicoOpaqueFunction **)(*(int *)graphics + 0x14))(
            graphics, value_14, value_6, (int)(short)(((short)value_13 + 1) - (short)value_14),
            color);
    } else {
        short_value_4 = (short)value_7;
        short_value_3 = (short)value_15;
        value_44 = y0 & 0xffff;
        short_value_2 = (short)value_12 - (short)value_6;
        condition_16 = y0 != value_12;
        short_value_1 = (short)value_14 - short_value_3;
        if (condition_16) {
            value_44 = value_44 - 1;
        }
        if (condition_16) {
            value_44 = (uint32_t)(short)value_44;
        }
        value_13 = 0;
        if (condition_16) {
        } else {
            value_44 = y0;
        }
        value_15 = 0;
        for (index = value_6; (int)index <= (int)value_44;
             index = (uint32_t)(short)((short)index + 1)) {
            short_value_8 =
                (short)(value_15 / (int)(short)((short)y0 - (short)value_6)) + short_value_3;
            short_value_5 = (short)(value_13 / (int)short_value_2) + short_value_3;
            value_15 = (short)(short_value_4 - short_value_3) + value_15;
            value_13 = short_value_1 + value_13;
            short_value_9 = short_value_8;
            if (short_value_8 <= short_value_5) {
                short_value_9 = short_value_5;
                short_value_5 = short_value_8;
            }
            (**(PicoOpaqueFunction **)(*(int *)graphics + 0x14))(
                graphics, (int)short_value_5, index,
                (int)(short)((short_value_9 + 1) - short_value_5), color);
        }
        value_14 = (int)(short)((short)value_14 - short_value_4);
        value_13 = (index - y0) * value_14;
        value_15 = (index - value_6) * (int)short_value_1;
        for (; (int)index <= (int)value_12; index = (uint32_t)(short)((short)index + 1)) {
            short_value_8 =
                (short)(value_13 / (int)(short)((short)value_12 - (short)y0)) + short_value_4;
            short_value_5 = (short)(value_15 / (int)short_value_2) + short_value_3;
            value_13 = value_13 + value_14;
            value_15 = value_15 + short_value_1;
            short_value_9 = short_value_8;
            if (short_value_8 <= short_value_5) {
                short_value_9 = short_value_5;
                short_value_5 = short_value_8;
            }
            (**(PicoOpaqueFunction **)(*(int *)graphics + 0x14))(
                graphics, (int)short_value_5, index,
                (int)(short)((short_value_9 + 1) - short_value_5), color);
        }
    }
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x2001412c.
 */
void GFX_draw_bitmap(void *graphics, short x, short y, uint8_t *bitmap, short width, short height,
                     uint16_t color)
{
    int value_1;
    short index;
    uint16_t index_2;
    uint32_t unresolved_saved_r7;

    value_1 = 0;
    for (index = 0; index < height; index = index + 1) {
        for (index_2 = 0; (int)(short)index_2 < (int)width; index_2 = index_2 + 1) {
            if ((index_2 & 7) == 0) {
                unresolved_saved_r7 = (uint32_t)bitmap[value_1 + ((int)(short)index_2 >> 3)];
            } else {
                unresolved_saved_r7 = (unresolved_saved_r7 & 0x7f) << 1;
            }
            if ((int)(unresolved_saved_r7 << 0x18) < 0) {
                (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                    graphics, (int)(short)(index_2 + x), (int)(short)(index + y), color);
            }
        }
        value_1 = value_1 + (width + 7) / 8;
    }
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x200141aa.
 */
void GFX_draw_bitmap_alias(void *graphics, short x, short y, uint8_t *bitmap, short width,
                           short height, uint16_t color)
{
    int value_1;
    short index;
    uint16_t index_2;
    uint32_t unresolved_saved_r7;

    value_1 = 0;
    for (index = 0; index < height; index = index + 1) {
        for (index_2 = 0; (int)(short)index_2 < (int)width; index_2 = index_2 + 1) {
            if ((index_2 & 7) == 0) {
                unresolved_saved_r7 = (uint32_t)bitmap[value_1 + ((int)(short)index_2 >> 3)];
            } else {
                unresolved_saved_r7 = (unresolved_saved_r7 & 0x7f) << 1;
            }
            if ((int)(unresolved_saved_r7 << 0x18) < 0) {
                (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                    graphics, (int)(short)(index_2 + x), (int)(short)(index + y), color);
            }
        }
        value_1 = value_1 + (width + 7) / 8;
    }
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20014228.
 */
void GFX_draw_character(void *graphics, short x, short y, uint8_t character, uint16_t foreground,
                        uint16_t background, uint8_t scale)
{
    uint8_t byte_value_1;
    uint16_t short_value_2;
    short short_value_3;
    uint16_t short_value_4;
    uint16_t unresolved_input_register_00000026;
    uint16_t unresolved_input_register_0000002a;
    int *pointer_5;
    int value_6;
    char byte_value_7;
    char byte_value_8;
    uint16_t short_value_9;
    uint32_t unresolved_input_register_0000002d;
    uint32_t value_10;
    uint32_t value_11;
    uint16_t *pointer_12;
    uint32_t value_13;
    uint16_t index;
    uint32_t unresolved_saved_r6;
    uint32_t value_15;
    uint16_t short_value_16;
    uint32_t value_17;
    uint32_t unresolved_saved_r9;
    uint32_t value_18;
    uint16_t short_value_19;
    uint32_t value_20;
    PicoOpaqueFunction *pointer_21;
    short short_value;

    value_10 = pico_join_u24_u8(unresolved_input_register_0000002d, character);
    pointer_5 = *(int **)((int)graphics + 0x1c);
    value_13 = (uint32_t)scale;
    short_value_2 = (uint16_t)scale;
    if (pointer_5 == (int *)0x0) {
        if ((((pico_join_u16_u16(unresolved_input_register_00000026, x) <
               (int)*(short *)((int)graphics + 0xc)) &&
              (pico_join_u16_u16(unresolved_input_register_0000002a, y) <
               (int)*(short *)((int)graphics + 0xe))) &&
             (0 < (short)short_value_2 * 6 +
                      pico_join_u16_u16(unresolved_input_register_00000026, x))) &&
            (0 < (int)(pico_join_u16_u16(unresolved_input_register_0000002a, y) + value_13 * 8))) {
            if ((*(char *)((int)graphics + 0x1b) == '\0') && (0xaf < value_10)) {
                value_10 = value_10 + 1 & 0xff;
            }
            byte_value_8 = '\x06';
            byte_value_7 = '\0';
            do {
                if (byte_value_7 == 5) {
                    value_18 = 0;
                } else {
                    value_18 = (uint32_t)(uint8_t)(&g_graphics_unknown_01)[(int)byte_value_7 +
                                                                           value_10 * 5];
                }
                value_15 = (uint32_t)(uint16_t)y;
                value_11 = value_15 + 8;
                value_17 = value_15;
                value_20 = value_15;
                do {
                    if ((value_18 & 1) == 0) {
                        if (background != foreground) {
                            if (value_13 != 1) {
                                pointer_21 = *(PicoOpaqueFunction **)(*(int *)graphics + 0x1c);
                                short_value_3 = (short)value_20;
                                short_value_9 = background;
                                goto branch_1;
                            }
                            pointer_21 = *(PicoOpaqueFunction **)(*(int *)graphics + 8);
                            short_value_9 = background;
                            goto branch_2;
                        }
                    } else if (value_13 == 1) {
                        pointer_21 = *(PicoOpaqueFunction **)(*(int *)graphics + 8);
                        short_value_9 = foreground;
                    branch_2:
                        (*pointer_21)(graphics, (int)(short)(x + byte_value_7),
                                      (int)(short)value_17, short_value_9);
                    } else {
                        pointer_21 = *(PicoOpaqueFunction **)(*(int *)graphics + 0x1c);
                        short_value_3 = (short)value_15;
                        short_value_9 = foreground;
                    branch_1:
                        (*pointer_21)(
                            graphics, (int)(short)((uint16_t)scale * (short)byte_value_7 + x),
                            (int)short_value_3, short_value_2, short_value_2, short_value_9);
                    }
                    value_17 = value_17 + 1 & 0xffff;
                    value_18 = value_18 >> 1;
                    value_15 = value_15 + value_13 & 0xffff;
                    value_20 = value_20 + value_13 & 0xffff;
                } while (value_17 != (value_11 & 0xffff));
                byte_value_7 = byte_value_7 + '\x01';
                byte_value_8 = byte_value_8 + -1;
            } while (byte_value_8 != '\0');
        }
    } else {
        value_6 = *pointer_5;
        pointer_12 =
            (uint16_t *)(pointer_5[1] + (value_10 - *(uint8_t *)(pointer_5 + 2) & 0xff) * 8);
        short_value_9 = pointer_12[1];
        byte_value_1 = *(uint8_t *)((int)pointer_12 + 3);
        value_10 = (uint32_t)*pointer_12;
        byte_value_7 = *(char *)((int)pointer_12 + 5);
        short_value_4 = pointer_12[3];
        if (1 < value_13) {
            short_value = (short)(char)short_value_4;
            unresolved_saved_r6 = (uint32_t)(uint16_t)(short)byte_value_7;
        }
        short_value_19 = 0;
        for (index = 0; (index & 0xff) < (uint16_t)byte_value_1; index = index + 1) {
            short_value_16 = 0;
            value_18 = unresolved_saved_r6 * value_13 + (uint32_t)(uint16_t)x;
            while (true) {
                if ((uint16_t)(uint8_t)short_value_9 <= (short_value_16 & 0xff))
                    break;
                if (((short_value_16 & 0xff) + short_value_19 & 7) == 0) {
                    unresolved_saved_r9 = (uint32_t) * (uint8_t *)(value_6 + value_10);
                    value_10 = value_10 + 1 & 0xffff;
                }
                if ((unresolved_saved_r9 & 0x80) != 0) {
                    if (value_13 == 1) {
                        (**(PicoOpaqueFunction **)(*(int *)graphics + 8))(
                            graphics, (int)(short)(byte_value_7 + x + short_value_16),
                            (int)(short)((char)short_value_4 + y + index), foreground);
                    } else {
                        (**(PicoOpaqueFunction **)(*(int *)graphics + 0x1c))(
                            graphics, (int)(short)value_18,
                            (int)(short)((index + short_value) * (uint16_t)scale + y),
                            short_value_2, short_value_2, foreground);
                    }
                }
                value_18 = (value_18 & 0xffff) + value_13;
                unresolved_saved_r9 = (unresolved_saved_r9 & 0x7f) << 1;
                short_value_16 = short_value_16 + 1;
            }
            short_value_19 = short_value_19 + (uint8_t)short_value_9 & 0xff;
        }
    }
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20014470.
 */
uint32_t GFX_write_character(void *graphics, uint8_t character)
{
    uint8_t byte_value_1;
    uint32_t unresolved_input_register_00000025;
    uint32_t value_2;
    int value_3;
    short short_value_4;
    int value_5;

    value_2 = pico_join_u24_u8(unresolved_input_register_00000025, character);
    value_3 = *(int *)((int)graphics + 0x1c);
    if (value_3 == 0) {
        if (value_2 == 10) {
            *(uint16_t *)((int)graphics + 0x10) = 0;
            *(uint16_t *)((int)graphics + 0x12) =
                *(short *)((int)graphics + 0x12) +
                (uint16_t) * (uint8_t *)((int)graphics + 0x18) * 8;
            return 1;
        }
        if (value_2 == 0xd) {
            return 1;
        }
        byte_value_1 = *(uint8_t *)((int)graphics + 0x18);
        if ((*(char *)((int)graphics + 0x1a) != '\0') &&
            ((int)*(short *)((int)graphics + 0xc) <=
             (short)(uint16_t)byte_value_1 * 6 + (int)*(short *)((int)graphics + 0x10))) {
            *(uint16_t *)((int)graphics + 0x10) = 0;
            *(uint16_t *)((int)graphics + 0x12) =
                *(short *)((int)graphics + 0x12) + (uint16_t)byte_value_1 * 8;
        }
        GFX_draw_character(
            graphics, *(short *)((int)graphics + 0x10), *(short *)((int)graphics + 0x12), character,
            *(uint16_t *)((int)graphics + 0x14), *(uint16_t *)((int)graphics + 0x16), byte_value_1);
        short_value_4 =
            *(short *)((int)graphics + 0x10) + (uint16_t) * (uint8_t *)((int)graphics + 0x18) * 6;
    } else {
        if (value_2 == 10) {
            *(uint16_t *)((int)graphics + 0x10) = 0;
            *(uint16_t *)((int)graphics + 0x12) = (uint16_t) * (uint8_t *)(value_3 + 10) *
                                                      (uint16_t) *
                                                      (uint8_t *)((int)graphics + 0x18) +
                                                  *(short *)((int)graphics + 0x12);
            return 1;
        }
        if (value_2 == 0xd) {
            return 1;
        }
        if (value_2 < *(uint8_t *)(value_3 + 8)) {
            return 1;
        }
        if (*(uint8_t *)(value_3 + 9) < value_2) {
            return 1;
        }
        value_5 = *(int *)(value_3 + 4) + (value_2 - *(uint8_t *)(value_3 + 8) & 0xff) * 8;
        if ((*(uint8_t *)(value_5 + 2) != 0) && (*(char *)(value_5 + 3) != '\0')) {
            byte_value_1 = *(uint8_t *)((int)graphics + 0x18);
            if ((*(char *)((int)graphics + 0x1a) != '\0') &&
                ((int)*(short *)((int)graphics + 0xc) <=
                 (int)(((int)*(char *)(value_5 + 5) + (uint32_t) * (uint8_t *)(value_5 + 2)) *
                           (uint32_t)byte_value_1 +
                       (int)*(short *)((int)graphics + 0x10)))) {
                *(uint16_t *)((int)graphics + 0x10) = 0;
                *(uint16_t *)((int)graphics + 0x12) =
                    (uint16_t) * (uint8_t *)(value_3 + 10) * (uint16_t)byte_value_1 +
                    *(short *)((int)graphics + 0x12);
            }
            GFX_draw_character(graphics, *(short *)((int)graphics + 0x10),
                               *(short *)((int)graphics + 0x12), character,
                               *(uint16_t *)((int)graphics + 0x14),
                               *(uint16_t *)((int)graphics + 0x16), byte_value_1);
        }
        short_value_4 =
            (uint16_t) * (uint8_t *)(value_5 + 4) * (uint16_t) * (uint8_t *)((int)graphics + 0x18) +
            *(short *)((int)graphics + 0x10);
    }
    *(short *)((int)graphics + 0x10) = short_value_4;
    return 1;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20014568.
 */
void GFX_set_cursor(void *graphics, short x, short y)
{
    *(short *)((int)graphics + 0x10) = x;
    *(short *)((int)graphics + 0x12) = y;
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x2001456e.
 */
void GFX_set_text_size(void *graphics, uint8_t scale)
{
    uint32_t unresolved_input_register_00000025;
    int value_1;

    value_1 = pico_join_u24_u8(unresolved_input_register_00000025, scale);
    if (value_1 == 0) {
        value_1 = 1;
    }
    *(char *)((int)graphics + 0x18) = (char)value_1;
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20014578.
 */
void GFX_set_text_color(void *graphics, uint16_t color)
{
    *(uint16_t *)((int)graphics + 0x16) = color;
    *(uint16_t *)((int)graphics + 0x14) = color;
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x2001457e.
 */
void GFX_set_text_colors(void *graphics, uint16_t foreground, uint16_t background)
{
    *(uint16_t *)((int)graphics + 0x14) = foreground;
    *(uint16_t *)((int)graphics + 0x16) = background;
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20014584.
 */
void GFX_set_text_wrap(void *graphics, bool enabled)
{
    *(bool *)((int)graphics + 0x1a) = enabled;
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20014588.
 */
uint8_t GFX_get_rotation(void *graphics)
{
    return *(uint8_t *)((int)graphics + 0x19);
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x2001458c.
 */
void GFX_set_rotation(void *graphics, uint8_t rotation)
{
    uint8_t byte_value_1;

    byte_value_1 = rotation & 3;
    *(uint8_t *)((int)graphics + 0x19) = byte_value_1;
    if ((byte_value_1 == 1) || (byte_value_1 == 3)) {
        *(uint16_t *)((int)graphics + 0xc) = *(uint16_t *)((int)graphics + 10);
        *(uint16_t *)((int)graphics + 0xe) = *(uint16_t *)((int)graphics + 8);
        return;
    }
    *(uint16_t *)((int)graphics + 0xc) = *(uint16_t *)((int)graphics + 8);
    *(uint16_t *)((int)graphics + 0xe) = *(uint16_t *)((int)graphics + 10);
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x200145aa.
 */
short GFX_get_width(void *graphics)
{
    return *(short *)((int)graphics + 0xc);
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x200145b0.
 */
short GFX_get_height(void *graphics)
{
    return *(short *)((int)graphics + 0xe);
}
