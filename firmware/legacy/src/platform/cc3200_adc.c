/* Pico Pro 0.1.34 — cc3200 adc. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019956. */
void ADCChannelEnable(uint32_t ul_base, uint32_t ul_channel)
{
    uint32_t value_1;

    if (ul_channel == 0) {
        value_1 = 2;
    } else if (ul_channel == 8) {
        value_1 = 4;
    } else if (ul_channel == 0x10) {
        value_1 = 8;
    } else {
        value_1 = 0x10;
    }
    *(uint32_t *)(ul_base + 0xb8) = value_1 | *(uint32_t *)(ul_base + 0xb8);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019978. */
void ADCChannelDisable(uint32_t ul_base, uint32_t ul_channel)
{
    uint32_t value_1;

    if (ul_channel == 0) {
        value_1 = 2;
    } else if (ul_channel == 8) {
        value_1 = 4;
    } else if (ul_channel == 0x10) {
        value_1 = 8;
    } else {
        value_1 = 0x10;
    }
    *(uint32_t *)(ul_base + 0xb8) = *(uint32_t *)(ul_base + 0xb8) & ~value_1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001999c. */
void ADCTimerConfig(uint32_t ul_base, uint32_t ul_value)
{
    *(uint32_t *)(ul_base + 0x68) = *(uint32_t *)(ul_base + 0x68) & 0xfffe0000 | ul_value & 0x1ffff;
    return;
}
