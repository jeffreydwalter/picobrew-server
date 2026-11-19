/* Pico Pro 0.1.34 — cc3200 interrupts. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x200199d0. */
uint32_t CPUcpsid(void)
{
    bool condition_1;
    uint32_t value_2;

    value_2 = 0;
    condition_1 = (bool)pico_cpu_is_privileged();
    if (condition_1) {
        value_2 = pico_cpu_irq_is_enabled();
    }
    pico_cpu_disable_irq();
    pico_cpu_data_sync_barrier(0xf);
    pico_cpu_instruction_sync_barrier(0xf);
    return value_2;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x200199e0. */
uint32_t CPUcpsie(void)
{
    bool condition_1;
    uint32_t value_2;

    value_2 = 0;
    condition_1 = (bool)pico_cpu_is_privileged();
    if (condition_1) {
        value_2 = pico_cpu_irq_is_enabled();
    }
    pico_cpu_enable_irq();
    pico_cpu_data_sync_barrier(0xf);
    pico_cpu_instruction_sync_barrier(0xf);
    return value_2;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019ae8. */
void IntVTableBaseSet(uint32_t ul_vtable_base)
{
    mmio_cc3200_interrupts_unknown_08 = ul_vtable_base;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019af4. */
void IntRegister(uint32_t ul_interrupt, PicoInterruptHandler *pfn_handler)
{
    int value_1;

    value_1 = mmio_cc3200_interrupts_unknown_08;
    *(PicoInterruptHandler **)(value_1 + ul_interrupt * 4) = pfn_handler;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019b04. */
void IntUnregister(uint32_t ul_interrupt)
{
    int value_1;

    value_1 = mmio_cc3200_interrupts_unknown_08;
    *(uint8_t **)(value_1 + ul_interrupt * 4) = &g_cc3200_interrupts_unknown_01;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019b18. */
void IntPrioritySet(uint32_t ul_interrupt, uint8_t uc_priority)
{
    int value_1;
    uint32_t unresolved_input_register_00000025;

    value_1 = (ul_interrupt & 3) << 3;
    **(uint32_t **)(&g_cc3200_interrupts_unknown_05 + (ul_interrupt & 0xfffffffc)) =
        **(uint32_t **)(&g_cc3200_interrupts_unknown_05 + (ul_interrupt & 0xfffffffc)) &
            ~(0xff << value_1) |
        pico_join_u24_u8(unresolved_input_register_00000025, uc_priority) << value_1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019b40. */
void IntEnable(uint32_t ul_interrupt)
{
    uint32_t value_1;
    uint32_t *pointer_2;

    if (ul_interrupt == 4) {
        pointer_2 = &mmio_cc3200_interrupts_unknown_09;
        value_1 = mmio_cc3200_interrupts_unknown_09;
        value_1 = value_1 | 0x10000;
    } else if (ul_interrupt == 5) {
        pointer_2 = &mmio_cc3200_interrupts_unknown_09;
        value_1 = mmio_cc3200_interrupts_unknown_09;
        value_1 = value_1 | 0x20000;
    } else if (ul_interrupt == 6) {
        pointer_2 = &mmio_cc3200_interrupts_unknown_09;
        value_1 = mmio_cc3200_interrupts_unknown_09;
        value_1 = value_1 | 0x40000;
    } else {
        if (ul_interrupt != 0xf) {
            if (ul_interrupt < 0x10) {
                return;
            }
            *(int *)(&g_cc3200_interrupts_unknown_02)[ul_interrupt - 0x10 >> 5] =
                1 << (ul_interrupt - 0x10 & 0x1f);
            goto branch_1;
        }
        pointer_2 = &mmio_cc3200_interrupts_unknown_06;
        value_1 = mmio_cc3200_interrupts_unknown_06;
        value_1 = value_1 | 2;
    }
    *pointer_2 = value_1;
branch_1:
    pico_cpu_data_sync_barrier(0xf);
    pico_cpu_instruction_sync_barrier(0xf);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019ba8. */
void IntDisable(uint32_t ul_interrupt)
{
    uint32_t value_1;
    uint32_t *pointer_2;

    if (ul_interrupt == 4) {
        pointer_2 = &mmio_cc3200_interrupts_unknown_09;
        value_1 = mmio_cc3200_interrupts_unknown_09;
        value_1 = value_1 & 0xfffeffff;
    } else if (ul_interrupt == 5) {
        pointer_2 = &mmio_cc3200_interrupts_unknown_09;
        value_1 = mmio_cc3200_interrupts_unknown_09;
        value_1 = value_1 & 0xfffdffff;
    } else if (ul_interrupt == 6) {
        pointer_2 = &mmio_cc3200_interrupts_unknown_09;
        value_1 = mmio_cc3200_interrupts_unknown_09;
        value_1 = value_1 & 0xfffbffff;
    } else {
        if (ul_interrupt != 0xf) {
            if (ul_interrupt < 0x10) {
                return;
            }
            *(int *)(&g_cc3200_interrupts_unknown_03)[ul_interrupt - 0x10 >> 5] =
                1 << (ul_interrupt - 0x10 & 0x1f);
            goto branch_1;
        }
        pointer_2 = &mmio_cc3200_interrupts_unknown_06;
        value_1 = mmio_cc3200_interrupts_unknown_06;
        value_1 = value_1 & 0xfffffffd;
    }
    *pointer_2 = value_1;
branch_1:
    pico_cpu_data_sync_barrier(0xf);
    pico_cpu_instruction_sync_barrier(0xf);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019c10. */
void IntPendClear(uint32_t ul_interrupt)
{
    uint32_t value_1;

    if (ul_interrupt == 0xe) {
        value_1 = mmio_cc3200_interrupts_unknown_07;
        value_1 = value_1 | 0x8000000;
    } else {
        if (ul_interrupt != 0xf) {
            if (0xf < ul_interrupt) {
                *(int *)(&g_cc3200_interrupts_unknown_04)[ul_interrupt - 0x10 >> 5] =
                    1 << (ul_interrupt - 0x10 & 0x1f);
            }
            return;
        }
        value_1 = mmio_cc3200_interrupts_unknown_07;
        value_1 = value_1 | 0x2000000;
    }
    mmio_cc3200_interrupts_unknown_07 = value_1;
    return;
}
