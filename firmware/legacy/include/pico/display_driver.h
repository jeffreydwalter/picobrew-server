#ifndef PICO_DISPLAY_DRIVER_H
#define PICO_DISPLAY_DRIVER_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void oled_draw_pixel(void *display, int x, int y, int color);
void oled_write_command(void *display, uint8_t command);

#endif /* PICO_DISPLAY_DRIVER_H */
