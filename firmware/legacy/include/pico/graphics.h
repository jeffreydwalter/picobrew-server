#ifndef PICO_GRAPHICS_H
#define PICO_GRAPHICS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void GFX_draw_line(void *graphics, short x0, short y0, short x1_2, short y1, uint16_t color);
void GFX_draw_rectangle(void *graphics, short x, short y, short width, short height,
                        uint16_t color);
void GFX_draw_fast_vertical_line(int *graphics);
void GFX_draw_fast_horizontal_line(int *graphics);
void GFX_fill_rectangle(void *graphics, short x, short y, short width, short height,
                        uint16_t color);
void GFX_fill_screen(void *graphics, uint16_t color);
void GFX_invert_display_noop(void);
void GFX_construct(void *graphics, short width, short height);
void GFX_draw_circle_quadrants(void *graphics, short centerX, short centerY, short radius_2,
                               uint8_t quadrants, uint16_t color);
void GFX_draw_round_rectangle(void *graphics, short x, short y, short width, short height,
                              short radius, uint16_t color);
void GFX_fill_triangle(void *graphics, short x0, short y0_2, short x1, short y1, short x2, short y2,
                       uint16_t color);
void GFX_draw_bitmap(void *graphics, short x, short y, uint8_t *bitmap, short width, short height,
                     uint16_t color);
void GFX_draw_bitmap_alias(void *graphics, short x, short y, uint8_t *bitmap, short width,
                           short height, uint16_t color);
void GFX_draw_character(void *graphics, short x, short y, uint8_t character, uint16_t foreground,
                        uint16_t background, uint8_t scale);
uint32_t GFX_write_character(void *graphics, uint8_t character);
void GFX_set_cursor(void *graphics, short x, short y);
void GFX_set_text_size(void *graphics, uint8_t scale);
void GFX_set_text_color(void *graphics, uint16_t color);
void GFX_set_text_colors(void *graphics, uint16_t foreground, uint16_t background);
void GFX_set_text_wrap(void *graphics, bool enabled);
uint8_t GFX_get_rotation(void *graphics);
void GFX_set_rotation(void *graphics, uint8_t rotation);
short GFX_get_width(void *graphics);
short GFX_get_height(void *graphics);

#endif /* PICO_GRAPHICS_H */
