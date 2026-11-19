/* Pico Pro 0.1.34 — cc3200 spi. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a118. */
long SPIDmaMaskGet(uint32_t ul_base)
{
    int value_1;

    if ((uint8_t *)ul_base == &mmio_cc3200_spi_unknown_03) {
        value_1 = 0;
    } else if ((uint8_t *)ul_base == &mmio_cc3200_spi_unknown_05) {
        value_1 = 1;
    } else {
        if ((uint8_t *)ul_base != &mmio_cc3200_spi_unknown_04) {
            return -1;
        }
        value_1 = 2;
    }
    return (&g_cc3200_spi_unknown_02)[value_1 * 2];
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a154. */
void SPIEnable(uint32_t ul_base)
{
    *(uint32_t *)(ul_base + 0x134) = *(uint32_t *)(ul_base + 0x134) | 1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a162. */
void SPIDisable(uint32_t ul_base)
{
    *(uint32_t *)(ul_base + 0x134) = *(uint32_t *)(ul_base + 0x134) & 0xfffffffe;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a17c. */
void SPIDmaDisable(uint32_t ul_base, uint32_t ul_flags)
{
    *(uint32_t *)(ul_base + 300) = ul_flags & *(uint32_t *)(ul_base + 300);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a188. */
void SPIReset(uint32_t ul_base)
{
    *(uint32_t *)(ul_base + 0x110) = *(uint32_t *)(ul_base + 0x110) | 2;
    do {
    } while (-1 < *(int *)(ul_base + 0x114) << 0x1f);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a19e. */
void SPIConfigSetExpClk(uint32_t ul_base, uint32_t ul_spiclk, uint32_t ul_bit_rate,
                        uint32_t ul_mode, uint32_t ul_sub_mode, uint32_t ul_config)
{
    uint32_t value_1;
    uint32_t value_2;

    value_1 = ul_spiclk / ul_bit_rate - 1;
    *(uint32_t *)(ul_base + 0x128) =
        *(uint32_t *)(ul_base + 0x128) & 0xfffffffa | ul_mode & 0xff | ul_config >> 0x18;
    *(uint32_t *)(ul_base + 0x134) = (value_1 & 0xff0) << 4;
    if (ul_mode == 0) {
        value_2 = 0x10000;
    } else {
        value_2 = 0x60000;
    }
    *(uint32_t *)(ul_base + 300) =
        value_2 | ul_sub_mode | 0x20000000 | ul_config & 0x8ffff | (value_1 & 0xf) << 2;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a204. */
void SPIFIFODisable(uint32_t ul_base, uint32_t ul_flags)
{
    *(uint32_t *)(ul_base + 300) = *(uint32_t *)(ul_base + 300) & ~ul_flags;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a212. */
void SPIFIFOLevelSet(uint32_t ul_base, uint32_t ul_tx_level, uint32_t ul_rx_level)
{
    *(uint32_t *)(ul_base + 0x17c) =
        *(uint32_t *)(ul_base + 0x17c) & 0xffff0000 | ul_tx_level - 1 | (ul_rx_level - 1) * 0x100;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a22a. */
void SPIWordCountSet(uint32_t ul_base, uint32_t ul_word_count)
{
    *(uint32_t *)(ul_base + 0x17c) = *(uint32_t *)(ul_base + 0x17c) & 0xffff | ul_word_count
                                                                                   << 0x10;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a23c. */
void SPIIntUnregister(uint32_t ul_base)
{
    int value_1;
    uint32_t ul_interrupt;

    if ((uint8_t *)ul_base == &mmio_cc3200_spi_unknown_03) {
        value_1 = 0;
    } else if ((uint8_t *)ul_base == &mmio_cc3200_spi_unknown_04) {
        value_1 = 1;
    } else {
        if ((uint8_t *)ul_base != &mmio_cc3200_spi_unknown_05) {
            ul_interrupt = 0xffffffff;
            goto branch_1;
        }
        value_1 = 2;
    }
    ul_interrupt = (&g_cc3200_spi_unknown_01)[value_1 * 3];
branch_1:
    IntDisable(ul_interrupt);
    IntUnregister(ul_interrupt);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a288. */
void SPIIntEnable(uint32_t ul_base, uint32_t ul_int_flags)
{
    long spidma_mask_get_result;

    if ((int)(ul_int_flags << 2) < 0) {
        spidma_mask_get_result = SPIDmaMaskGet(ul_base);
        mmio_cc3200_spi_unknown_06 = spidma_mask_get_result;
    }
    if ((int)(ul_int_flags << 3) < 0) {
        spidma_mask_get_result = SPIDmaMaskGet(ul_base);
        mmio_cc3200_spi_unknown_06 = spidma_mask_get_result >> 1;
    }
    *(uint32_t *)(ul_base + 0x11c) = ul_int_flags & 0x3000f | *(uint32_t *)(ul_base + 0x11c);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a2c4. */
void SPIIntClear(uint32_t ul_base, uint32_t ul_int_flags)
{
    long spidma_mask_get_result;

    if ((int)(ul_int_flags << 2) < 0) {
        spidma_mask_get_result = SPIDmaMaskGet(ul_base);
        mmio_cc3200_spi_unknown_07 = spidma_mask_get_result;
    }
    if ((int)(ul_int_flags << 3) < 0) {
        spidma_mask_get_result = SPIDmaMaskGet(ul_base);
        mmio_cc3200_spi_unknown_07 = spidma_mask_get_result >> 1;
    }
    *(uint32_t *)(ul_base + 0x118) = ul_int_flags & 0x3000f;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a2f8. */
void SPICSEnable(uint32_t ul_base)
{
    *(uint32_t *)(ul_base + 300) = *(uint32_t *)(ul_base + 300) | 0x100000;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a306. */
void SPICSDisable(uint32_t ul_base)
{
    *(uint32_t *)(ul_base + 300) = *(uint32_t *)(ul_base + 300) & 0xffefffff;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a314. */
long SPITransfer(uint32_t ul_base, uint8_t *uc_dout, uint8_t *uc_din, uint32_t ul_size,
                 uint32_t ul_flags)
{
    bool condition_1;
    bool condition_2;
    long value_3;
    uint32_t value_4;
    uint32_t value_28;
    uint8_t *pointer_24;

    pointer_24 = uc_dout;
    value_28 = ul_base;
    value_4 = *(uint32_t *)(ul_base + 300) & 0xf80;
    if (value_4 == 0x380) {
        condition_1 = uc_dout != (uint8_t *)0x0;
        if (!condition_1) {
            value_28 = 0xffffffff;
            uc_dout = (uint8_t *)&value_28;
        }
        condition_2 = uc_din == (uint8_t *)0x0;
        if (condition_2) {
            uc_din = (uint8_t *)&pointer_24;
        }
        if ((ul_flags & 1) != 0) {
            *(uint32_t *)(ul_base + 300) = *(uint32_t *)(ul_base + 300) | 0x100000;
        }
        for (; ul_size != 0; ul_size = ul_size - 1) {
            do {
            } while ((*(uint32_t *)(ul_base + 0x130) & 2) == 0);
            *(uint32_t *)(ul_base + 0x138) = (uint32_t)*uc_dout;
            do {
            } while ((*(uint32_t *)(ul_base + 0x130) & 1) == 0);
            *uc_din = (uint8_t) * (uint32_t *)(ul_base + 0x13c);
            uc_dout = uc_dout + condition_1;
            uc_din = (uint8_t *)((int)uc_din + (uint32_t)!condition_2);
        }
        if ((int)(ul_flags << 0x1e) < 0) {
            *(uint32_t *)(ul_base + 300) = *(uint32_t *)(ul_base + 300) & 0xffefffff;
        }
    branch_1:
        value_3 = 0;
    } else {
        if (value_4 == 0x780) {
            if (-1 < (int)(ul_size << 0x1f)) {
                value_4 = ul_size >> 1;
                condition_1 = uc_dout != (uint8_t *)0x0;
                if (!condition_1) {
                    value_28 = 0xffffffff;
                    uc_dout = (uint8_t *)&value_28;
                }
                condition_2 = uc_din == (uint8_t *)0x0;
                if (condition_2) {
                    uc_din = (uint8_t *)&pointer_24;
                }
                if ((ul_flags & 1) != 0) {
                    *(uint32_t *)(ul_base + 300) = *(uint32_t *)(ul_base + 300) | 0x100000;
                }
                for (; value_4 != 0; value_4 = value_4 - 1) {
                    do {
                    } while ((*(uint32_t *)(ul_base + 0x130) & 2) == 0);
                    *(uint32_t *)(ul_base + 0x138) = (uint32_t) * (uint16_t *)uc_dout;
                    do {
                    } while ((*(uint32_t *)(ul_base + 0x130) & 1) == 0);
                    *(short *)uc_din = (short)*(uint32_t *)(ul_base + 0x13c);
                    uc_dout = (uint8_t *)((int)uc_dout + (uint32_t)condition_1 * 2);
                    uc_din = (uint8_t *)((int)uc_din + (uint32_t)!condition_2 * 2);
                }
                goto branch_2;
            }
        } else if ((value_4 == 0xf80) && ((ul_size & 3) == 0)) {
            value_4 = ul_size >> 2;
            condition_1 = uc_dout != (uint8_t *)0x0;
            if (!condition_1) {
                value_28 = 0xffffffff;
                uc_dout = (uint8_t *)&value_28;
            }
            condition_2 = uc_din == (uint8_t *)0x0;
            if (condition_2) {
                uc_din = (uint8_t *)&pointer_24;
            }
            if ((ul_flags & 1) != 0) {
                *(uint32_t *)(ul_base + 300) = *(uint32_t *)(ul_base + 300) | 0x100000;
            }
            for (; value_4 != 0; value_4 = value_4 - 1) {
                do {
                } while ((*(uint32_t *)(ul_base + 0x130) & 2) == 0);
                *(uint32_t *)(ul_base + 0x138) = *(uint32_t *)uc_dout;
                do {
                } while ((*(uint32_t *)(ul_base + 0x130) & 1) == 0);
                *(uint8_t **)uc_din = *(uint8_t **)(ul_base + 0x13c);
                uc_dout = uc_dout + (uint32_t)condition_1 * 4;
                uc_din = (uint8_t *)((int)uc_din + (uint32_t)!condition_2 * 4);
            }
        branch_2:
            if ((int)(ul_flags << 0x1e) < 0) {
                *(uint32_t *)(ul_base + 300) = *(uint32_t *)(ul_base + 300) & 0xffefffff;
            }
            goto branch_1;
        }
        value_3 = -1;
    }
    return value_3;
}
