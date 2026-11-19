#ifndef PICO_CC3200_UART_H
#define PICO_CC3200_UART_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void UARTEnable(uint32_t ul_base);
void UARTDisable(uint32_t ul_base);
void UARTConfigSetExpClk(uint32_t ul_base, uint32_t ul_uartclk, uint32_t ul_baud,
                         uint32_t ul_config);
bool UARTCharsAvail(uint32_t ul_base);
bool UARTSpaceAvail(uint32_t ul_base);
long UARTCharGetNonBlocking(uint32_t ul_base);
bool UARTCharPutNonBlocking(uint32_t ul_base, uint8_t uc_data);

#endif /* PICO_CC3200_UART_H */
