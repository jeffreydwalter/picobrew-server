#ifndef PICO_WIRING_GPIO_H
#define PICO_WIRING_GPIO_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void gpio_port_a0_interrupt(void);
void gpio_port_a1_interrupt(void);
void gpio_port_a2_interrupt(void);
void gpio_port_a3_interrupt(void);
uint32_t attachInterrupt(uint32_t pin, uint32_t callback, uint32_t mode);
void pinMode(int pin, int mode);
bool digitalRead(int pin);
void digitalWrite(int pin, int value);

#endif /* PICO_WIRING_GPIO_H */
