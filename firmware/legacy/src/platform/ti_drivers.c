/* Pico Pro 0.1.34 — ti drivers. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Purpose incomplete.
 * Firmware address: 0x20019acc. */
void unresolved_ti_drivers_01(int argument_1, uint32_t argument_2)
{
    *(uint32_t *)(argument_1 + 0x808) = argument_2;
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001a0ec. */
uint32_t unresolved_ti_drivers_02(uint32_t argument_1, uint8_t *argument_2, uint32_t argument_3,
                                  uint8_t *argument_4)
{
    do {
    } while (-1 < *(int *)(argument_1 + 0x118) << 0x1c);
    *(uint32_t *)(argument_1 + 0x48) = argument_3;
    SHAMD5DataWriteMultiple(argument_1, argument_2, argument_3);
    do {
    } while (-1 < *(int *)(argument_1 + 0x118) << 0x1f);
    SHAMD5ResultRead(argument_1, argument_4);
    return 1;
}

/* Purpose incomplete.
 * Firmware address: 0x2001a170. */
void unresolved_ti_drivers_03(int argument_1, uint32_t argument_2)
{
    *(uint32_t *)(argument_1 + 300) = argument_2 | *(uint32_t *)(argument_1 + 300);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001a1f8. */
void unresolved_ti_drivers_04(int argument_1, uint32_t argument_2)
{
    *(uint32_t *)(argument_1 + 300) = argument_2 | *(uint32_t *)(argument_1 + 300);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001a7cc. */
void unresolved_ti_drivers_05(uint32_t argument_1, PicoInterruptHandler *argument_2)
{
    IntRegister(argument_1, argument_2);
    IntEnable(argument_1);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001a824. */
void unresolved_ti_drivers_06(int argument_1)
{
    do {
        argument_1 = argument_1 + -1;
    } while (argument_1 != 0);
    return;
}
