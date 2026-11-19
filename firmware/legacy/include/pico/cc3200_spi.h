#ifndef PICO_CC3200_SPI_H
#define PICO_CC3200_SPI_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

long SPIDmaMaskGet(uint32_t ul_base);
void SPIEnable(uint32_t ul_base);
void SPIDisable(uint32_t ul_base);
void SPIDmaDisable(uint32_t ul_base, uint32_t ul_flags);
void SPIReset(uint32_t ul_base);
void SPIConfigSetExpClk(uint32_t ul_base, uint32_t ul_spiclk, uint32_t ul_bit_rate,
                        uint32_t ul_mode, uint32_t ul_sub_mode, uint32_t ul_config);
void SPIFIFODisable(uint32_t ul_base, uint32_t ul_flags);
void SPIFIFOLevelSet(uint32_t ul_base, uint32_t ul_tx_level, uint32_t ul_rx_level);
void SPIWordCountSet(uint32_t ul_base, uint32_t ul_word_count);
void SPIIntUnregister(uint32_t ul_base);
void SPIIntEnable(uint32_t ul_base, uint32_t ul_int_flags);
void SPIIntClear(uint32_t ul_base, uint32_t ul_int_flags);
void SPICSEnable(uint32_t ul_base);
void SPICSDisable(uint32_t ul_base);
long SPITransfer(uint32_t ul_base, uint8_t *uc_dout, uint8_t *uc_din, uint32_t ul_size,
                 uint32_t ul_flags);

#endif /* PICO_CC3200_SPI_H */
