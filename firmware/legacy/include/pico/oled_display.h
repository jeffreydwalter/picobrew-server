#ifndef PICO_OLED_DISPLAY_H
#define PICO_OLED_DISPLAY_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void OLED_initialize_controller(void *display);
void OLED_invert_display(void *display, int inverted);

#endif /* PICO_OLED_DISPLAY_H */
