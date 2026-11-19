#ifndef PICO_SPI_LINK_H
#define PICO_SPI_LINK_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_spi_chip_selects_initialize(void);
uint8_t pico_spi_transfer_selected_byte(uint8_t outgoing_byte);
uint8_t pico_spi_command_read_byte(uint8_t command);

#endif /* PICO_SPI_LINK_H */
