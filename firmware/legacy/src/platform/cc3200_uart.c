/* Pico Pro 0.1.34 — cc3200 uart. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a59e. */
void UARTEnable(uint32_t ul_base)
{
    *(uint32_t *)(ul_base + 0x2c) = *(uint32_t *)(ul_base + 0x2c) | 0x10;
    *(uint32_t *)(ul_base + 0x30) = *(uint32_t *)(ul_base + 0x30) | 0x301;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a5b4. */
void UARTDisable(uint32_t ul_base)
{
    do {
    } while (*(int *)(ul_base + 0x18) << 0x1c < 0);
    *(uint32_t *)(ul_base + 0x2c) = *(uint32_t *)(ul_base + 0x2c) & 0xffffffef;
    *(uint32_t *)(ul_base + 0x30) = *(uint32_t *)(ul_base + 0x30) & 0xfffffcfe;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a5d0. */
void UARTConfigSetExpClk(uint32_t ul_base, uint32_t ul_uartclk, uint32_t ul_baud,
                         uint32_t ul_config)
{
    uint32_t value_1;

    UARTDisable(ul_base);
    if (ul_uartclk < ul_baud << 4) {
        *(uint32_t *)(ul_base + 0x30) = *(uint32_t *)(ul_base + 0x30) | 0x20;
        ul_baud = ul_baud >> 1;
    } else {
        *(uint32_t *)(ul_base + 0x30) = *(uint32_t *)(ul_base + 0x30) & 0xffffffdf;
    }
    value_1 = (ul_uartclk << 3) / ul_baud + 1;
    *(uint32_t *)(ul_base + 0x24) = value_1 >> 7;
    *(uint32_t *)(ul_base + 0x28) = (value_1 & 0x7f) >> 1;
    *(uint32_t *)(ul_base + 0x2c) = ul_config;
    *(uint32_t *)(ul_base + 0x18) = 0;
    UARTEnable(ul_base);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a618. */
bool UARTCharsAvail(uint32_t ul_base)
{
    return PICO_SUB_U8(((*(uint32_t *)(ul_base + 0x18) ^ 0x10) & 0x1f) >> 4, 0);
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a624. */
bool UARTSpaceAvail(uint32_t ul_base)
{
    return PICO_SUB_U8(((*(uint32_t *)(ul_base + 0x18) ^ 0x20) & 0x3f) >> 5, 0);
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a630. */
long UARTCharGetNonBlocking(uint32_t ul_base)
{
    long value_1;

    if (*(int *)(ul_base + 0x18) << 0x1b < 0) {
        value_1 = -1;
    } else {
        value_1 = *(long *)ul_base;
    }
    return value_1;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a63e. */
bool UARTCharPutNonBlocking(uint32_t ul_base, uint8_t uc_data)
{
    uint32_t unresolved_input_register_00000025;
    bool condition_1;

    condition_1 = -1 < *(int *)(ul_base + 0x18) << 0x1a;
    if (condition_1) {
        *(uint32_t *)ul_base = pico_join_u24_u8(unresolved_input_register_00000025, uc_data);
    }
    return condition_1;
}
