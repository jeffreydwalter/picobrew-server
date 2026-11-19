#ifndef PICO_HARDWARE_SERIAL_H
#define PICO_HARDWARE_SERIAL_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int HardwareSerial_available(int serial);
uint32_t HardwareSerial_peek(int serial);
void HardwareSerial_read(int *serial);
void HardwareSerial_flush(int serial);
void HardwareSerial_construct(uint32_t *serial);
void HardwareSerial_clear(int serial);
void HardwareSerial_prime_transmit(int serial, uint32_t uartBase);
uint32_t HardwareSerial_write_byte(int serial, uint8_t value);
void HardwareSerial_begin(int serial, uint32_t baudRate);
void UARTIntHandler(void);
void UARTIntHandler1(void);
void serialEvent(void);
void serialEvent1(void);
void serialEventRun(void);
void initialize_global_serial_ports(void);

#endif /* PICO_HARDWARE_SERIAL_H */
