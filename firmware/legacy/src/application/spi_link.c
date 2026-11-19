/* Pico Pro 0.1.34 — spi link. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Configures chip-select GPIO4/18 high and clears link counters.
 * Firmware address: 0x2000621c. */
void pico_spi_chip_selects_initialize(void)
{
    pinMode(4, 1);
    pinMode(0x12, 1);
    digitalWrite(4, 1);
    digitalWrite(0x12, 1);
    g_buzzer_enabled = 0;
    g_stepper_capabilities = 0;
    return;
}

/* Selects GPIO18, waits100ms, sends/receives byte, releases and restores SPI mode.
 * Firmware address: 0x2000624c. */
uint8_t pico_spi_transfer_selected_byte(uint8_t outgoing_byte)
{
    uint8_t incoming_byte;

    SPI_set_data_mode(&global_spi, '\x01');
    digitalWrite(4, 1);
    digitalWrite(0x12, 0);
    delay(100);
    incoming_byte = SPI_transfer_byte(&global_spi, outgoing_byte);
    digitalWrite(4, 1);
    digitalWrite(0x12, 1);
    SPI_set_data_mode(&global_spi, '\0');
    return incoming_byte;
}

/* Critical section sends command then dummy0xff to read response with1ms gaps.
 * Firmware address: 0x2000629c. */
uint8_t pico_spi_command_read_byte(uint8_t command)
{
    uint8_t response;

    IntMasterDisable();
    SPI_set_data_mode(&global_spi, '\x01');
    digitalWrite(4, 1);
    digitalWrite(0x12, 0);
    delay(1);
    SPI_transfer_byte(&global_spi, command);
    delay(1);
    response = SPI_transfer_byte(&global_spi, 0xff);
    delay(1);
    digitalWrite(4, 1);
    digitalWrite(0x12, 1);
    SPI_set_data_mode(&global_spi, '\0');
    IntMasterEnable();
    return response;
}
