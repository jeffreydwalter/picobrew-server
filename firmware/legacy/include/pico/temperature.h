#ifndef PICO_TEMPERATURE_H
#define PICO_TEMPERATURE_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_heat_record_temperature_sample(int controller);
void pico_heat_controller_start(int controller);
int pico_heat_classify_temperature_progress(uint32_t argument_1, uint32_t argument_2,
                                            uint32_t temperature_low, uint32_t temperature_high);
char pico_shuttle_update_calibration(int controller);
bool pico_heat_controller_stop(int controller);

#endif /* PICO_TEMPERATURE_H */
