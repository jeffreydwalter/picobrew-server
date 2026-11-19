/* Pico Pro 0.1.34 — energia runtime. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Matches radix bounds 2..36, negative decimal handling, repeated quotient/remainder and reverse
 * output in Energia ltoa source Firmware address: 0x20018c56. */
char *ltoa(int value, char *string, int radix)
{
    bool condition_1;
    char *text_2;
    char *index;
    char byte_value_4;
    int value_5;
    char buffer[40];

    if (string != (char *)0x0) {
        if (0x22 < radix - 2U) {
            return (char *)0x0;
        }
        if ((radix == 10) && (value < 0)) {
            value = -value;
            condition_1 = true;
        } else {
            condition_1 = false;
        }
        for (index = buffer; (value != 0 || (index == buffer)); index = index + 1) {
            value_5 = value - radix * ((uint32_t)value / (uint32_t)radix);
            byte_value_4 = (char)value_5;
            if (value_5 < 10) {
                byte_value_4 = byte_value_4 + '0';
            } else {
                byte_value_4 = byte_value_4 + 'W';
            }
            *index = byte_value_4;
            value = (uint32_t)value / (uint32_t)radix;
        }
        if (condition_1) {
            *string = '-';
            text_2 = string + 1;
        } else {
            text_2 = string;
        }
        while (buffer < index) {
            index = index + -1;
            *text_2 = *index;
            text_2 = text_2 + 1;
        }
        *text_2 = '\0';
    }
    return string;
}

/* Single tail branch to matched ltoa, same Energia wrapper
 * Firmware address: 0x20018cc4. */
char *itoa(int value, char *string, int radix)
{
    bool condition_1;
    char *text_2;
    char *index;
    char byte_value_4;
    int value_5;
    char buffer[40];

    if (string != (char *)0x0) {
        if (0x22 < radix - 2U) {
            return (char *)0x0;
        }
        if ((radix == 10) && (value < 0)) {
            value = -value;
            condition_1 = true;
        } else {
            condition_1 = false;
        }
        for (index = buffer; (value != 0 || (index == buffer)); index = index + 1) {
            value_5 = value - radix * ((uint32_t)value / (uint32_t)radix);
            byte_value_4 = (char)value_5;
            if (value_5 < 10) {
                byte_value_4 = byte_value_4 + '0';
            } else {
                byte_value_4 = byte_value_4 + 'W';
            }
            *index = byte_value_4;
            value = (uint32_t)value / (uint32_t)radix;
        }
        if (condition_1) {
            *string = '-';
            text_2 = string + 1;
        } else {
            text_2 = string;
        }
        while (buffer < index) {
            index = index + -1;
            *text_2 = *index;
            text_2 = text_2 + 1;
        }
        *text_2 = '\0';
    }
    return string;
}

/* Returns milliseconds counter used by matching SysTick handler and delay runtime
 * Firmware address: 0x20018efc. */
uint32_t millis(void)
{
    return elapsed_milliseconds;
}

/* Loops 2*argument calls to delayMicroseconds(500), exact Energia algorithm
 * Firmware address: 0x20018f08. */
void delay(uint32_t millis_2)
{
    int index;

    for (index = 0; index != millis_2 * 2; index = index + 1) {
        delayMicroseconds(500);
    }
    return;
}

/* Matches Energia 1ms split, 24-bit SysTick rollover logic, 80 ticks per microsecond
 * Firmware address: 0x20018f20. */
void delayMicroseconds(uint32_t us)
{
    uint32_t value_1;
    int value_2;
    uint32_t value_3;

    if (999 < us) {
        delay(us / 1000);
        us = us % 1000;
    }
    value_1 = mmio_energia_runtime_unknown_01;
    value_3 = us * 0x50;
    if ((value_1 & 0xffffff) < value_3) {
        value_3 = value_3 + 0xfec77f;
    }
    do {
        value_2 = mmio_energia_runtime_unknown_01;
    } while (((value_1 & 0xffffff) - value_2 & 0xffffff) <= value_3);
    return;
}

/* Observed platform initialization, main setup/loop, and empty weak hook; existing named
 * entrypoints retained. Firmware address: 0x200195b4. */
void energia_platform_init(void)
{
    IntVTableBaseSet(0x20004000);
    PRCMPeripheralClkEnable(6, 1);
    PRCMPeripheralClkEnable(7, 1);
    PRCMPeripheralClkEnable(8, 1);
    PRCMPeripheralClkEnable(9, 1);
    IntMasterEnable();
    PRCMCC3200MCUInit();
    SysTickIntEnable();
    SysTickPeriodSet(80000);
    SysTickEnable();
    return;
}

/* Observed platform initialization, main setup/loop, and empty weak hook; existing named
 * entrypoints retained. Firmware address: 0x20019600. */
void energia_main(void)
{
    pico_setup();
    do {
        pico_loop();
        serialEventRun();
    } while (true);
}

/* Single bx lr instruction near main and Print code; identity among empty weak hooks cannot be
 * distinguished. Firmware address: 0x2001961c. */
void energia_empty_hook(void)
{
    return;
}
