#ifndef PICO_SENSORS_H
#define PICO_SENSORS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int pico_step_filter_is_inserted(void);
void pico_wait_for_step_filter(void);
uint32_t pico_step_filter_is_missing(void);
void pico_step_filter_gpio_initialize(void);
uint32_t pico_temperature_inputs_initialize(void);
float pico_temperature_read_pin26(void);
float pico_temperature_read_pin25(void);
uint32_t pico_wait_gpio19_low(void);

#endif /* PICO_SENSORS_H */
