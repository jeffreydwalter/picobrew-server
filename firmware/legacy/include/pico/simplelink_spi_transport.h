#ifndef PICO_SIMPLELINK_SPI_TRANSPORT_H
#define PICO_SIMPLELINK_SPI_TRANSPORT_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void nwp_spi_transfer_interrupt(void);
void nwp_spi_start_dma_read(void *destination, int byteCount);
uint32_t nwp_spi_open(void);
uint32_t nwp_spi_close(void);
int nwp_spi_read(int descriptor, uint32_t *destination, int byteCount);
int nwp_spi_write(int descriptor, void *source, int byteCount);
uint32_t nwp_register_interrupt_handler(PicoInterruptHandler *handler);
void NwpMaskInterrupt(void);
void NwpUnMaskInterrupt(void);
void NwpPowerOn(void);
void NwpPowerOff(void);
void NwpPowerOnPreamble(void);

#endif /* PICO_SIMPLELINK_SPI_TRANSPORT_H */
