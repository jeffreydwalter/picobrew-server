#ifndef PICO_ACTUATORS_H
#define PICO_ACTUATORS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_actuators_initialize(void);
void pico_steam_pump_off(void);
void pico_drain_pump_off(void);
void pico_shuttle_pump_off(void);
void pico_steam_and_drain_pumps_off(void);
void pico_heater_on(void);
void pico_heater_off(void);
void pico_actuators_all_off(void);
void pico_actuators_off_except_drain(void);
bool pico_drain_pump_is_on(void);

#endif /* PICO_ACTUATORS_H */
