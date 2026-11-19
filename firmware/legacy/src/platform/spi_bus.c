/* Pico Pro 0.1.34 — spi bus. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x200145b8.
 */
void SPI_begin(void *spi)
{
    uint32_t ul_spiclk;

    if (*(char *)spi == '\n') {
        *(uint8_t *)spi = 0;
    }
    PRCMPeripheralClkEnable(3, 1);
    PinConfigurePeripheral6mA(
        *(uint32_t *)(&g_spi_bus_unknown_01 + (uint32_t) * (uint8_t *)spi * 0x10),
        *(uint32_t *)(&g_spi_bus_unknown_03 + (uint32_t) * (uint8_t *)spi * 0x10));
    PinConfigurePeripheral6mA(
        *(uint32_t *)(&g_spi_bus_unknown_02 + (uint32_t) * (uint8_t *)spi * 0x10),
        *(uint32_t *)(&g_spi_bus_unknown_04 + (uint32_t) * (uint8_t *)spi * 0x10));
    PinConfigurePeripheral6mA(6, 7);
    PinConfigSet(4, 0x40, 0);
    PRCMPeripheralReset(3);
    SPIReset(0x44021000);
    ul_spiclk = PRCMPeripheralClockGet(3);
    SPIConfigSetExpClk(0x44021000, ul_spiclk, 4000000, 0, 0, 0x20003c0);
    SPIEnable(0x44021000);
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20014648.
 */
void SPI_set_bit_order(void *spi, uint8_t bitOrder)
{
    *(uint8_t *)((int)spi + 1) = bitOrder;
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x2001464c.
 */
void SPI_set_data_mode(void *spi, uint8_t mode)
{
    uint32_t value_1;
    uint32_t unresolved_input_register_00000025;

    PRCMPeripheralClkEnable(3, 1);
    value_1 = mmio_spi_bus_unknown_05;
    mmio_spi_bus_unknown_05 = value_1 & 0xfffffffc;
    value_1 = mmio_spi_bus_unknown_05;
    mmio_spi_bus_unknown_05 = pico_join_u24_u8(unresolved_input_register_00000025, mode) | value_1;
    return;
}

/* Drawing geometry/text-state or SPI register behavior is explicit in the executable; GFX interface
 * matches Adafruit-style graphics layout and SPI matches Energia API. Firmware address: 0x20014670.
 */
uint8_t SPI_transfer_byte(void *spi, uint8_t transmittedByte)
{
    uint8_t buffer_11[8];
    uint8_t byte_value_9;

    buffer_11[0] = transmittedByte;
    if (*(char *)((int)spi + 1) == '\0') {
        buffer_11[0] = (((((((transmittedByte & 1) << 1 | transmittedByte >> 1 & 1) << 1 |
                            transmittedByte >> 2 & 1)
                               << 1 |
                           transmittedByte >> 3 & 1)
                              << 1 |
                          transmittedByte >> 4 & 1)
                             << 1 |
                         transmittedByte >> 5 & 1)
                            << 1 |
                        transmittedByte >> 6 & 1)
                           << 1 |
                       transmittedByte >> 7;
    }
    SPITransfer(0x44021000, buffer_11, &byte_value_9, 1, 0);
    if (*(char *)((int)spi + 1) == '\0') {
        byte_value_9 =
            (((((((byte_value_9 & 1) << 1 | byte_value_9 >> 1 & 1) << 1 | byte_value_9 >> 2 & 1)
                    << 1 |
                byte_value_9 >> 3 & 1)
                   << 1 |
               byte_value_9 >> 4 & 1)
                  << 1 |
              byte_value_9 >> 5 & 1)
                 << 1 |
             byte_value_9 >> 6 & 1)
                << 1 |
            byte_value_9 >> 7;
    }
    return byte_value_9;
}

/* Static initializer stores SPI module0 and MSB-first1 into global SPI object at2002dad0.
 * Firmware address: 0x200146bc. */
void initialize_global_spi(void)
{
    global_spi = 0;
    spi_bit_order = 1;
    return;
}
