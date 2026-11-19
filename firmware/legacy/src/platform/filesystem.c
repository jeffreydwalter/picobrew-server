/* Pico Pro 0.1.34 — filesystem. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Purpose incomplete.
 * Firmware address: 0x20018004. */
uint32_t unresolved_filesystem_01(void)
{
    uint32_t value_1;

    value_1 = (uint32_t)g_filesystem_unknown_09;
    if (g_filesystem_unknown_11 == value_1) {
        return 0xffffffff;
    }
    g_filesystem_unknown_09 = g_filesystem_unknown_09 + 1 & 0x3f;
    return (uint32_t)(uint8_t)(&g_filesystem_unknown_04)[value_1];
}

/* Purpose incomplete.
 * Firmware address: 0x20018030. */
uint32_t unresolved_filesystem_02(void)
{
    uint32_t value_1;

    if ((uint32_t)g_filesystem_unknown_11 == (uint32_t)g_filesystem_unknown_09) {
        value_1 = 0xffffffff;
    } else {
        value_1 = (uint32_t)(uint8_t)(&g_filesystem_unknown_04)[g_filesystem_unknown_09];
    }
    return value_1;
}

/* Purpose incomplete.
 * Firmware address: 0x20018054. */
void unresolved_filesystem_03(void)
{
    g_filesystem_unknown_08 = 0;
    g_filesystem_unknown_09_as_uint8_t = g_filesystem_unknown_11_as_uint8_t;
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20018070. */
int unresolved_filesystem_04(void)
{
    int value_1;

    value_1 = (uint32_t)g_filesystem_unknown_11 - (uint32_t)g_filesystem_unknown_09;
    if ((uint32_t)g_filesystem_unknown_11 < (uint32_t)g_filesystem_unknown_09) {
        value_1 = value_1 + 0x40;
    }
    return value_1;
}

/* Purpose incomplete.
 * Firmware address: 0x2001808c. */
uint32_t unresolved_filesystem_05(uint32_t argument_1, uint8_t argument_2)
{
    uint32_t value_1;

    value_1 = g_filesystem_unknown_08_as_uint8_t + 1 & 0x3f;
    if (value_1 == g_filesystem_unknown_02) {
        if (g_filesystem_unknown_10 != '\0') {
            return 0;
        }
        unresolved_ti_drivers_01(0x40020000,
                                 (uint32_t)(uint8_t)(&g_filesystem_unknown_03)[value_1]);
        g_filesystem_unknown_02 = g_filesystem_unknown_02 + 1 & 0x3f;
    }
    (&g_filesystem_unknown_03)[g_filesystem_unknown_08_as_uint8_t] = argument_2;
    g_filesystem_unknown_08_as_uint8_t = g_filesystem_unknown_08_as_uint8_t + 1 & 0x3f;
    return 1;
}

/* Purpose incomplete.
 * Firmware address: 0x200180fc. */
void unresolved_filesystem_06(void)
{
    g_filesystem_unknown_06 = 0;
    g_filesystem_unknown_07 = 1000;
    g_filesystem_unknown_05 = &g_filesystem_unknown_01;
    return;
}
