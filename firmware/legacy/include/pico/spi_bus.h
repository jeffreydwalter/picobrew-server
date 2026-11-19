#ifndef PICO_SPI_BUS_H
#define PICO_SPI_BUS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void SPI_begin(void *spi);
void SPI_set_bit_order(void *spi, uint8_t bitOrder);
void SPI_set_data_mode(void *spi, uint8_t mode);
uint8_t SPI_transfer_byte(void *spi, uint8_t transmittedByte);
void initialize_global_spi(void);

#endif /* PICO_SPI_BUS_H */
