/* Pico Pro 0.1.34 — cc3200 clock. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019d24. */
void PRCMMCUReset(bool b_include_subsystem)
{
    uint32_t unresolved_input_register_00000021;
    uint32_t value_1;

    if (pico_join_u24_u8(unresolved_input_register_00000021, b_include_subsystem) == 0) {
        value_1 = 1;
    } else {
        value_1 = 2;
    }
    mmio_cc3200_clock_unknown_05 = value_1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019d38. */
uint32_t PRCMSysResetCauseGet(void)
{
    uint32_t value_1;
    uint32_t value_2;

    value_1 = mmio_cc3200_clock_unknown_06;
    value_2 = value_1 & 0xff;
    if ((value_2 == 0) && (value_1 = mmio_cc3200_clock_unknown_19, unresolved_ti_drivers_06(0x14d5),
                           (value_1 & 1) != 0)) {
        value_2 = 7;
    }
    return value_2;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019d64. */
void PRCMPeripheralClkEnable(uint32_t ul_peripheral, uint32_t ul_clk_flags)
{
    if ((ul_peripheral != 0xff) &&
        (*(uint32_t *)((int)&mmio_cc3200_clock_unknown_03 +
                       *(int *)(&g_cc3200_clock_unknown_01 + ul_peripheral * 8)) =
             ul_clk_flags | *(uint32_t *)((int)&mmio_cc3200_clock_unknown_03 +
                                          *(int *)(&g_cc3200_clock_unknown_01 + ul_peripheral * 8)),
         ul_peripheral == 0)) {
        mmio_cc3200_clock_unknown_03 = 0x404;
    }
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019d90. */
void PRCMPeripheralClkDisable(uint32_t ul_peripheral, uint32_t ul_clk_flags)
{
    *(uint32_t *)((int)&mmio_cc3200_clock_unknown_03 +
                  *(int *)(&g_cc3200_clock_unknown_01 + ul_peripheral * 8)) =
        *(uint32_t *)((int)&mmio_cc3200_clock_unknown_03 +
                      *(int *)(&g_cc3200_clock_unknown_01 + ul_peripheral * 8)) &
        ~ul_clk_flags;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019dac. */
uint32_t PRCMPeripheralClockGet(uint32_t ul_peripheral)
{
    uint32_t *pointer_1;

    if ((ul_peripheral - 3 < 2) || (ul_peripheral == 0x13)) {
        return 40000000;
    }
    if (ul_peripheral == 0) {
        pointer_1 = &mmio_cc3200_clock_unknown_03;
    } else {
        if (ul_peripheral != 2) {
            return 80000000;
        }
        pointer_1 = &mmio_cc3200_clock_unknown_04;
    }
    return (uint32_t)(240000000 /
                      (uint64_t)(((*pointer_1 & 0x7ff) >> 8) + (*pointer_1 & 0xff) + 2));
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019df4. */
void PRCMPeripheralReset(uint32_t ul_peripheral)
{
    uint32_t index;
    uint32_t *pointer_2;

    if (ul_peripheral != 0x12) {
        pointer_2 = (uint32_t *)((int)&mmio_cc3200_clock_unknown_03 +
                                 *(int *)(&g_cc3200_clock_unknown_02 + ul_peripheral * 8));
        *pointer_2 = *pointer_2 | 1;
        for (index = 0; index < 0x10; index = index + 1) {
        }
        *pointer_2 = *pointer_2 & 0xfffffffe;
    }
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019e34. */
bool PRCMPeripheralStatusGet(uint32_t ul_peripheral)
{
    return PICO_SUB_U8((*(uint32_t *)((int)&mmio_cc3200_clock_unknown_03 +
                                      *(int *)(&g_cc3200_clock_unknown_02 + ul_peripheral * 8)) &
                        3) >>
                           1,
                       0);
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019e50. */
void PRCMCC3200MCUInit(void)
{
    uint32_t value_1;
    uint32_t prcmsys_reset_cause_get_result;
    uint32_t value_3;

    value_3 = mmio_cc3200_clock_unknown_13;
    mmio_cc3200_clock_unknown_13 = value_3 | 0x800000;
    value_3 = mmio_cc3200_clock_unknown_18;
    unresolved_ti_drivers_06(0x14d5);
    mmio_cc3200_clock_unknown_18 = value_3 | 0x10;
    unresolved_ti_drivers_06(0x14d5);
    value_3 = mmio_simplelink_spi_transport_unknown_15;
    mmio_simplelink_spi_transport_unknown_15 = value_3 | 0x3c;
    PRCMPeripheralClkEnable(5, 1);
    PRCMPeripheralReset(5);
    PRCMPeripheralClkDisable(5, 1);
    prcmsys_reset_cause_get_result = PRCMSysResetCauseGet();
    if (prcmsys_reset_cause_get_result == 0) {
        mmio_cc3200_clock_unknown_17 = 1;
        unresolved_ti_drivers_06(0x14d5);
    }
    value_3 = mmio_cc3200_clock_unknown_16;
    if ((value_3 & 0xff) == 2) {
        value_3 = mmio_cc3200_clock_unknown_10;
        mmio_cc3200_clock_unknown_10 = value_3 & 0xfffff3f0 | 2;
        value_3 = mmio_cc3200_clock_unknown_10;
        mmio_cc3200_clock_unknown_11 = value_3 & 0xfffff3f0 | 2;
    }
    value_3 = mmio_cc3200_clock_unknown_12;
    mmio_cc3200_clock_unknown_12 = value_3 | 2;
    value_3 = mmio_cc3200_clock_unknown_08;
    if ((value_3 & 0xf) == 1) {
        value_3 = mmio_cc3200_clock_unknown_08;
        mmio_cc3200_clock_unknown_08 = value_3 & 0xfffffff0;
    }
    value_3 = mmio_cc3200_clock_unknown_09;
    if ((value_3 & 0xf) == 1) {
        value_3 = mmio_cc3200_clock_unknown_09;
        mmio_cc3200_clock_unknown_09 = value_3 & 0xfffffff0;
    }
    value_3 = mmio_cc3200_clock_unknown_07;
    value_1 = mmio_cc3200_clock_unknown_15;
    if ((value_3 & 0x3ffffff) >> 0x16 == 0xe) {
        value_3 = value_1 & 0xff03ffff | 0xc80000;
    } else {
        value_3 = value_1 & 0xff03ffff | 0xa40000;
    }
    mmio_cc3200_clock_unknown_15 = value_3;
    value_3 = mmio_cc3200_clock_unknown_20;
    mmio_cc3200_clock_unknown_20 = value_3 & 0xefffffff;
    value_3 = mmio_cc3200_clock_unknown_14;
    mmio_cc3200_clock_unknown_14 = value_3 | 4;
    return;
}
