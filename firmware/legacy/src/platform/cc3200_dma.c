/* Pico Pro 0.1.34 — cc3200 dma. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a66c. */
void uDMAEnable(void)
{
    mmio_cc3200_dma_unknown_01 = 1;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a678. */
void uDMAChannelEnable(uint32_t ul_channel_num)
{
    mmio_cc3200_dma_unknown_07 = 1 << (ul_channel_num & 0x1f);
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a68c. */
void uDMAControlBaseSet(void *p_control_table)
{
    mmio_cc3200_dma_unknown_02 = p_control_table;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a698. */
void uDMAChannelAttributeEnable(uint32_t ul_channel_num, uint32_t ul_attr)
{
    uint32_t value_1;

    value_1 = ul_channel_num & 0x1f;
    if ((int)(ul_attr << 0x1f) < 0) {
        mmio_cc3200_dma_unknown_03 = 1 << value_1;
    }
    if ((int)(ul_attr << 0x1e) < 0) {
        mmio_cc3200_dma_unknown_08 = 1 << value_1;
    }
    if ((int)(ul_attr << 0x1d) < 0) {
        mmio_cc3200_dma_unknown_10 = 1 << value_1;
    }
    if ((int)(ul_attr << 0x1c) < 0) {
        mmio_cc3200_dma_unknown_05 = 1 << value_1;
    }
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a6e0. */
void uDMAChannelAttributeDisable(uint32_t ul_channel_num, uint32_t ul_attr)
{
    uint32_t value_1;

    value_1 = ul_channel_num & 0x1f;
    if ((int)(ul_attr << 0x1f) < 0) {
        mmio_cc3200_dma_unknown_04 = 1 << value_1;
    }
    if ((int)(ul_attr << 0x1e) < 0) {
        mmio_cc3200_dma_unknown_09 = 1 << value_1;
    }
    if ((int)(ul_attr << 0x1d) < 0) {
        mmio_cc3200_dma_unknown_11 = 1 << value_1;
    }
    if ((int)(ul_attr << 0x1c) < 0) {
        mmio_cc3200_dma_unknown_06 = 1 << value_1;
    }
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a728. */
void uDMAChannelControlSet(uint32_t ul_channel_struct_index, uint32_t ul_control)
{
    int value_1;

    value_1 = mmio_cc3200_dma_unknown_02;
    value_1 = value_1 + (ul_channel_struct_index & 0x3f) * 0x10;
    *(uint32_t *)(value_1 + 8) = ul_control | *(uint32_t *)(value_1 + 8) & 0xfc3ff7;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a748. */
void uDMAChannelTransferSet(uint32_t ul_channel_struct_index, uint32_t ul_mode, void *pv_src_addr,
                            void *pv_dst_addr, uint32_t ul_transfer_size)
{
    int value_1;
    uint32_t value_2;
    int value_3;
    int value_4;

    value_1 = mmio_cc3200_dma_unknown_02;
    value_4 = (ul_channel_struct_index & 0x3f) * 0x10;
    value_3 = value_1 + value_4;
    if (((int)(ul_channel_struct_index << 0x1a) < 0) && ((ul_mode & 0xfffffffd) == 4)) {
        ul_mode = ul_mode | 1;
    }
    value_2 = *(uint32_t *)(value_3 + 8) & 0xffffc008 | (ul_transfer_size - 1) * 0x10 | ul_mode;
    if ((value_2 & 0xc000000) != 0xc000000) {
        pv_src_addr =
            (void *)((int)pv_src_addr + (ul_transfer_size << ((value_2 & 0xc000000) >> 0x1a)) + -1);
    }
    *(void **)(value_1 + value_4) = pv_src_addr;
    if ((value_2 & 0xc0000000) != 0xc0000000) {
        if ((ul_mode & 0xfffffffd) == 4) {
            pv_dst_addr = (void *)(value_1 + (ul_channel_struct_index & 0x3f | 0x20) * 0x10 + 0xc);
        } else {
            pv_dst_addr = (void *)((int)pv_dst_addr + (ul_transfer_size << (value_2 >> 0x1e)) + -1);
        }
    }
    *(void **)(value_3 + 4) = pv_dst_addr;
    *(uint32_t *)(value_3 + 8) = value_2;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a7e0. */
uint32_t uDMAIntStatus(void)
{
    uint32_t value_1;

    value_1 = mmio_cc3200_dma_unknown_12;
    return value_1;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a7ec. */
void uDMAIntClear(uint32_t ul_chan_mask)
{
    mmio_cc3200_dma_unknown_12 = ul_chan_mask;
    return;
}

/* Unique exact SDK object-function byte match with only ELF linker relocation sites masked
 * Firmware address: 0x2001a7f8. */
void uDMAChannelAssign(uint32_t ul_mapping)
{
    int value_1;
    uint32_t *pointer_2;

    pointer_2 = (uint32_t *)((((ul_mapping & 0x1f) >> 3) + 0x1003fd44) * 4);
    value_1 = (ul_mapping & 7) << 2;
    *pointer_2 = (ul_mapping >> 0x10) << value_1 | *pointer_2 & ~(0xf << value_1);
    return;
}
