#ifndef PICO_CC3200_DMA_H
#define PICO_CC3200_DMA_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void uDMAEnable(void);
void uDMAChannelEnable(uint32_t ul_channel_num);
void uDMAControlBaseSet(void *p_control_table);
void uDMAChannelAttributeEnable(uint32_t ul_channel_num, uint32_t ul_attr);
void uDMAChannelAttributeDisable(uint32_t ul_channel_num, uint32_t ul_attr);
void uDMAChannelControlSet(uint32_t ul_channel_struct_index, uint32_t ul_control);
void uDMAChannelTransferSet(uint32_t ul_channel_struct_index, uint32_t ul_mode, void *pv_src_addr,
                            void *pv_dst_addr, uint32_t ul_transfer_size);
uint32_t uDMAIntStatus(void);
void uDMAIntClear(uint32_t ul_chan_mask);
void uDMAChannelAssign(uint32_t ul_mapping);

#endif /* PICO_CC3200_DMA_H */
