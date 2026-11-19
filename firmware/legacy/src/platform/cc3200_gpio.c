/* Pico Pro 0.1.34 — cc3200 gpio. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x200199f0. */
long GPIOGetIntNumber(uint32_t ul_port)
{
    if ((uint8_t *)ul_port == &mmio_wiring_gpio_unknown_02) {
        return 0x11;
    }
    if (&mmio_wiring_gpio_unknown_02 < ul_port) {
        if ((uint8_t *)ul_port == &mmio_wiring_gpio_unknown_03) {
            return 0x12;
        }
        if ((uint8_t *)ul_port == &mmio_wiring_gpio_unknown_04) {
            return 0x13;
        }
    } else if ((uint8_t *)ul_port == &mmio_wiring_gpio_unknown_01) {
        return 0x10;
    }
    return -1;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019a2c. */
void GPIODirModeSet(uint32_t ul_port, uint8_t uc_pins, uint32_t ul_pin_io)
{
    uint32_t unresolved_input_register_00000025;
    uint32_t value_1;

    if ((ul_pin_io & 1) == 0) {
        value_1 = *(uint32_t *)(ul_port + 0x400) &
                  ~pico_join_u24_u8(unresolved_input_register_00000025, uc_pins);
    } else {
        value_1 = pico_join_u24_u8(unresolved_input_register_00000025, uc_pins) |
                  *(uint32_t *)(ul_port + 0x400);
    }
    *(uint32_t *)(ul_port + 0x400) = value_1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019a44. */
void GPIOIntTypeSet(uint32_t ul_port, uint8_t uc_pins, uint32_t ul_int_type)
{
    uint32_t unresolved_input_register_00000025;
    uint32_t value_1;
    uint32_t value_2;

    value_1 = pico_join_u24_u8(unresolved_input_register_00000025, uc_pins);
    if ((ul_int_type & 1) == 0) {
        value_2 = *(uint32_t *)(ul_port + 0x408) & ~value_1;
    } else {
        value_2 = *(uint32_t *)(ul_port + 0x408) | value_1;
    }
    *(uint32_t *)(ul_port + 0x408) = value_2;
    if ((ul_int_type & 2) == 0) {
        value_2 = *(uint32_t *)(ul_port + 0x404) & ~value_1;
    } else {
        value_2 = *(uint32_t *)(ul_port + 0x404) | value_1;
    }
    *(uint32_t *)(ul_port + 0x404) = value_2;
    if ((ul_int_type & 4) == 0) {
        value_1 = *(uint32_t *)(ul_port + 0x40c) & ~value_1;
    } else {
        value_1 = value_1 | *(uint32_t *)(ul_port + 0x40c);
    }
    *(uint32_t *)(ul_port + 0x40c) = value_1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019a86. */
void GPIOIntEnable(uint32_t ul_port, uint32_t ul_int_flags)
{
    *(uint32_t *)(ul_port + 0x410) = ul_int_flags | *(uint32_t *)(ul_port + 0x410);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019a92. */
long GPIOIntStatus(uint32_t ul_port, bool b_masked)
{
    uint32_t unresolved_input_register_00000025;

    if (pico_join_u24_u8(unresolved_input_register_00000025, b_masked) == 0) {
        return *(long *)(ul_port + 0x414);
    }
    return *(long *)(ul_port + 0x418);
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019aa6. */
void GPIOIntRegister(uint32_t ul_port, PicoInterruptHandler *pfn_int_handler)
{
    uint32_t ul_interrupt;

    ul_interrupt = GPIOGetIntNumber(ul_port);
    IntRegister(ul_interrupt, (PicoInterruptHandler *)pfn_int_handler);
    IntEnable(ul_interrupt);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019c50. */
void PinModeSet(uint32_t ul_pin, uint32_t ul_pin_mode)
{
    *(uint32_t *)(&mmio_cc3200_gpio_unknown_02 +
                  *(int *)(&g_cc3200_gpio_unknown_01 + (ul_pin & 0x3f) * 4) * 4) =
        (ul_pin_mode |
         *(uint32_t *)(&mmio_cc3200_gpio_unknown_02 +
                       *(int *)(&g_cc3200_gpio_unknown_01 + (ul_pin & 0x3f) * 4) * 4) &
             0xfffffff0) &
        0xfffff3ff;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019c78. */
void PinConfigSet(uint32_t ul_pin, uint32_t ul_pin_strength, uint32_t ul_pin_type)
{
    uint32_t value_1;
    uint32_t value_2;

    value_2 = *(uint32_t *)(&g_cc3200_gpio_unknown_01 + (ul_pin & 0x3f) * 4);
    if (ul_pin_type == 0x10000000) {
        value_1 = mmio_cc3200_gpio_unknown_03;
        mmio_cc3200_gpio_unknown_03 = 0x80 << (value_2 & 0xff) & 0x1e00U | value_1;
        value_1 = 0xc00;
    } else {
        value_1 = mmio_cc3200_gpio_unknown_03;
        mmio_cc3200_gpio_unknown_03 = value_1 & ~(0x80 << (value_2 & 0xff) & 0x1e00U);
        value_1 = ul_pin_type | ul_pin_strength |
                  *(uint32_t *)(&mmio_cc3200_gpio_unknown_02 + value_2 * 4) & 0xfffffc0f;
    }
    *(uint32_t *)(&mmio_cc3200_gpio_unknown_02 + value_2 * 4) = value_1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019cfc. */
void PinTypeGPIO(uint32_t ul_pin, uint32_t ul_pin_mode, bool b_open_drain)
{
    uint32_t unresolved_input_register_00000029;
    uint32_t ul_pin_type;

    ul_pin_type = 0;
    if (pico_join_u24_u8(unresolved_input_register_00000029, b_open_drain) != 0) {
        ul_pin_type = 0x10;
    }
    PinConfigSet(ul_pin, 0x20, ul_pin_type);
    PinModeSet(ul_pin, ul_pin_mode);
    return;
}
