/* Pico Pro 0.1.34 — cc3200 timer. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Increments milliseconds and calls each nonzero member of 8-entry callback table with value 1,
 * exact Energia implementation Firmware address: 0x20018f70. */
void SysTickIntHandler(void)
{
    int value_1;

    elapsed_milliseconds_as_int = elapsed_milliseconds_as_int + 1;
    value_1 = 0;
    do {
        if (*(PicoOpaqueFunction **)((int)&systick_callbacks + value_1) !=
            (PicoOpaqueFunction *)0x0) {
            (**(PicoOpaqueFunction **)((int)&systick_callbacks + value_1))(1);
        }
        value_1 = value_1 + 4;
    } while (value_1 != 0x20);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a4b0. */
void SysTickEnable(void)
{
    uint32_t value_1;

    value_1 = mmio_cc3200_interrupts_unknown_06;
    mmio_cc3200_interrupts_unknown_06 = value_1 | 5;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a4c0. */
void SysTickIntEnable(void)
{
    uint32_t value_1;

    value_1 = mmio_cc3200_interrupts_unknown_06;
    mmio_cc3200_interrupts_unknown_06 = value_1 | 2;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a4d0. */
void SysTickPeriodSet(uint32_t ul_period)
{
    mmio_cc3200_timer_unknown_05 = ul_period - 1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a4dc. */
void TimerEnable(uint32_t ul_base, uint32_t ul_timer)
{
    *(uint32_t *)(ul_base + 0xc) = *(uint32_t *)(ul_base + 0xc) | ul_timer & 0x101;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a4ec. */
void TimerConfigure(uint32_t ul_base, uint32_t ul_config)
{
    mmio_cc3200_timer_unknown_04 = 0xff;
    *(uint32_t *)(ul_base + 0xc) = *(uint32_t *)(ul_base + 0xc) & 0xfffffefe;
    *(uint32_t *)ul_base = ul_config >> 0x18;
    *(uint32_t *)(ul_base + 4) = ul_config & 0xff;
    *(uint32_t *)(ul_base + 8) = (ul_config & 0xffff) >> 8;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a514. */
void TimerLoadSet(uint32_t ul_base, uint32_t ul_timer, uint32_t ul_value)
{
    if ((ul_timer & 0xff) != 0) {
        *(uint32_t *)(ul_base + 0x28) = ul_value;
    }
    if ((ul_timer & 0xff00) != 0) {
        *(uint32_t *)(ul_base + 0x2c) = ul_value;
    }
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a528. */
void TimerIntRegister(uint32_t ul_base, uint32_t ul_timer, PicoInterruptHandler *pfn_handler)
{
    uint32_t ul_interrupt;

    if ((uint8_t *)ul_base == &mmio_cc3200_timer_unknown_01) {
        ul_interrupt = 0x23;
    } else if ((uint8_t *)ul_base == &mmio_cc3200_timer_unknown_02) {
        ul_interrupt = 0x25;
    } else if ((uint8_t *)ul_base == &mmio_cc3200_timer_unknown_03) {
        ul_interrupt = 0x27;
    } else {
        ul_interrupt = 0x33;
    }
    if ((ul_timer & 0xff) != 0) {
        IntRegister(ul_interrupt, pfn_handler);
        IntEnable(ul_interrupt);
    }
    if ((ul_timer & 0xff00) == 0) {
        return;
    }
    IntRegister(ul_interrupt + 1, pfn_handler);
    IntEnable(ul_interrupt + 1);
    return;
}
