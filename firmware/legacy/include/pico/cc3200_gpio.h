#ifndef PICO_CC3200_GPIO_H
#define PICO_CC3200_GPIO_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

long GPIOGetIntNumber(uint32_t ul_port);
void GPIODirModeSet(uint32_t ul_port, uint8_t uc_pins, uint32_t ul_pin_io);
void GPIOIntTypeSet(uint32_t ul_port, uint8_t uc_pins, uint32_t ul_int_type);
void GPIOIntEnable(uint32_t ul_port, uint32_t ul_int_flags);
long GPIOIntStatus(uint32_t ul_port, bool b_masked);
void GPIOIntRegister(uint32_t ul_port, PicoInterruptHandler *pfn_int_handler);
void PinModeSet(uint32_t ul_pin, uint32_t ul_pin_mode);
void PinConfigSet(uint32_t ul_pin, uint32_t ul_pin_strength, uint32_t ul_pin_type);
void PinTypeGPIO(uint32_t ul_pin, uint32_t ul_pin_mode, bool b_open_drain);

#endif /* PICO_CC3200_GPIO_H */
