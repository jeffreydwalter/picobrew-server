/* Pico Pro 0.1.34 — cc3200 hash. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019f74. */
uint32_t SHAMD5IntStatus(uint32_t ui32_base, bool b_masked)
{
    uint32_t unresolved_input_register_00000025;
    uint32_t value_1;
    uint32_t value_2;

    if (pico_join_u24_u8(unresolved_input_register_00000025, b_masked) == 0) {
        value_1 = mmio_cc3200_hash_unknown_02;
        value_2 = *(uint32_t *)(ui32_base + 0x118);
    } else {
        value_1 = mmio_cc3200_hash_unknown_03;
        value_2 = *(uint32_t *)(ui32_base + 0x11c) & *(uint32_t *)(ui32_base + 0x118);
    }
    return value_2 | (value_1 & 7) << 0x10;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019fa0. */
void SHAMD5IntEnable(uint32_t ui32_base, uint32_t ui32_int_flags)
{
    uint32_t value_1;

    value_1 = mmio_cc3200_hash_unknown_01;
    mmio_cc3200_hash_unknown_01 = value_1 & ~((ui32_int_flags & 0x7ffff) >> 0x10);
    *(uint32_t *)(ui32_base + 0x11c) = ui32_int_flags & 0xffff | *(uint32_t *)(ui32_base + 0x11c);
    *(uint32_t *)(ui32_base + 0x110) = *(uint32_t *)(ui32_base + 0x110) | 4;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x20019fd0. */
void SHAMD5IntDisable(uint32_t ui32_base, uint32_t ui32_int_flags)
{
    uint32_t value_1;

    value_1 = mmio_cc3200_hash_unknown_01;
    mmio_cc3200_hash_unknown_01 = value_1 | (ui32_int_flags & 0x7ffff) >> 0x10;
    *(uint32_t *)(ui32_base + 0x11c) =
        *(uint32_t *)(ui32_base + 0x11c) & ~(ui32_int_flags & 0xffff);
    if (*(int *)(ui32_base + 0x11c) == 0) {
        *(uint32_t *)(ui32_base + 0x110) = *(uint32_t *)(ui32_base + 0x110) & 0xfffffffb;
    }
    return;
}

/* Calls matched IntRegister and IntEnable for interrupt 164, exactly as TI SHAMD5IntRegister source
 * Firmware address: 0x2001a004. */
void SHAMD5IntRegister(uint32_t ui32_base, PicoInterruptHandler *pfn_handler)
{
    IntRegister(0xa4, pfn_handler);
    IntEnable(0xa4);
    return;
}

/* Stores mode to base+0x44; source/call context uses SHAMD5 base and MD5 mode
 * Firmware address: 0x2001a016. */
void SHAMD5ConfigSet(uint32_t ui32_base, uint32_t ui32_mode)
{
    *(uint32_t *)(ui32_base + 0x44) = ui32_mode;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a01a. */
void SHAMD5DataWrite(uint32_t ui32_base, uint8_t *pui8_src)
{
    uint32_t *pointer_1;
    uint32_t *pointer_2;

    do {
    } while (-1 < *(int *)(ui32_base + 0x118) << 0x1e);
    pointer_1 = (uint32_t *)(ui32_base + 0x80);
    do {
        pointer_2 = pointer_1 + 1;
        *pointer_1 = *(uint32_t *)((int)pointer_1 + (int)(pui8_src + (-0x80 - ui32_base)));
        pointer_1 = pointer_2;
    } while (pointer_2 != (uint32_t *)(ui32_base + 0xc0));
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a03e. */
void SHAMD5DataWriteMultiple(uint32_t ui32_base, uint8_t *pui8_data_src, uint32_t ui32_data_length)
{
    uint8_t *pui8_src;
    uint32_t *pointer_1;
    uint32_t value_2;
    int value_3;
    uint32_t index;
    uint32_t value;

    value = 0;
    for (index = 0; pui8_src = pui8_data_src + index * 0x40, index != ui32_data_length >> 6;
         index = index + 1) {
        SHAMD5DataWrite(ui32_base, pui8_src);
    }
    if ((ui32_data_length & 0x3f) != 0) {
        do {
        } while (-1 < *(int *)(ui32_base + 0x118) << 0x1e);
        pointer_1 = (uint32_t *)(ui32_base + 0x80);
        for (index = 0; index != (ui32_data_length & 0x3f) >> 2; index = index + 1) {
            *pointer_1 = *(uint32_t *)((int)pointer_1 + (int)(pui8_src + (-0x80 - ui32_base)));
            pointer_1 = pointer_1 + 1;
        }
        if ((ui32_data_length & 3) != 0) {
            value_3 = 0;
            if (index != 0) {
                value_3 = index * 4 + -4;
            }
            value_2 = 0;
            do {
                *(uint8_t *)((int)&value + value_2) = pui8_src[value_2 + index * 4];
                value_2 = value_2 + 1;
            } while (value_2 != (ui32_data_length & 3));
            *(uint32_t *)((int)(ui32_base + 0x80) + value_3) = value;
        }
    }
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a0b4. */
void SHAMD5ResultRead(uint32_t ui32_base, uint8_t *pui8_dest)
{
    uint32_t value_1;
    uint32_t value_2;

    switch (*(uint32_t *)(ui32_base + 0x44) & 6) {
    case 0:
        value_1 = 0x10;
        break;
    case 2:
        value_1 = 0x14;
        break;
    case 4:
        value_1 = 0x1c;
        break;
    case 6:
        value_1 = 0x20;
        break;
    default:
        goto switchD_2001a0c0_default;
    }
    value_2 = 0;
    do {
        *(uint32_t *)(pui8_dest + value_2) = *(uint32_t *)(ui32_base + 0x20 + value_2);
        value_2 = value_2 + 4;
    } while (value_2 < value_1);
switchD_2001a0c0_default:
    return;
}
