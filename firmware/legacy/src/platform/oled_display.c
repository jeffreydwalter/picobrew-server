/* Pico Pro 0.1.34 — oled display. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20013880.
 */
void OLED_initialize_controller(void *display)
{
    pinMode((uint32_t) * (uint8_t *)((int)display + 0x22), 1);
    pinMode((uint32_t) * (uint8_t *)((int)display + 0x24), 1);
    pinMode((uint32_t) * (uint8_t *)((int)display + 0x25), 1);
    SPI_begin(&global_spi);
    oled_write_command(display, 0xae);
    oled_write_command(display, 0xb3);
    oled_write_command(display, 0xf1);
    oled_write_command(display, 0xa8);
    oled_write_command(display, '?');
    oled_write_command(display, 0xa2);
    oled_write_command(display, 'L');
    oled_write_command(display, 0xa1);
    oled_write_command(display, '\0');
    oled_write_command(display, 0xad);
    oled_write_command(display, '\x02');
    oled_write_command(display, 0xa0);
    oled_write_command(display, 'V');
    oled_write_command(display, 0x86);
    oled_write_command(display, 0xb8);
    oled_write_command(display, '\x01');
    oled_write_command(display, '\x11');
    oled_write_command(display, '\"');
    oled_write_command(display, '2');
    oled_write_command(display, 'C');
    oled_write_command(display, 'T');
    oled_write_command(display, 'e');
    oled_write_command(display, 'v');
    oled_write_command(display, 0x81);
    oled_write_command(display, '\x7f');
    oled_write_command(display, 0xb2);
    oled_write_command(display, 'Q');
    oled_write_command(display, 0xb1);
    oled_write_command(display, 'U');
    oled_write_command(display, 0xb4);
    oled_write_command(display, '\x02');
    oled_write_command(display, 0xb0);
    oled_write_command(display, '(');
    oled_write_command(display, 0xbe);
    oled_write_command(display, '\x1c');
    oled_write_command(display, 0xbf);
    oled_write_command(display, '\x0f');
    oled_write_command(display, 0xa4);
    oled_write_command(display, '#');
    oled_write_command(display, '#');
    oled_write_command(display, 0xaf);
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x200139f8.
 */
void OLED_invert_display(void *display, int inverted)
{
    uint8_t command;

    if (inverted == 0) {
        command = 0xa4;
    } else {
        command = 0xa7;
    }
    oled_write_command(display, command);
    return;
}
