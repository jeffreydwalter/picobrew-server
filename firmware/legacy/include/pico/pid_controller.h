#ifndef PICO_PID_CONTROLLER_H
#define PICO_PID_CONTROLLER_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void PID_disable(void *controller);
void PID_compute(void *controller);
void PID_set_tunings(void *controller, double proportionalGain, double integralGain,
                     double derivativeGain);
void PID_set_output_limits(void *controller, double minimum, double maximum);
void PID_initialize_integral(void *controller);
void PID_set_mode(void *controller, int mode);
void PID_set_direction(void *controller, int direction);
void PID_construct(void *controller, uint32_t input, uint32_t output, uint32_t setpoint,
                   double argument_5, uint32_t argument_6, uint32_t argument_7, uint32_t argument_8,
                   uint32_t argument_9, int argument_10, uint32_t argument_11, uint32_t argument_12,
                   uint32_t argument_13);

#endif /* PICO_PID_CONTROLLER_H */
