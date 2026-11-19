#ifndef PICO_RECONSTRUCTED_PID_CONTROLLER_H
#define PICO_RECONSTRUCTED_PID_CONTROLLER_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Layout reconstructed from the CC3200 image. Pointers are 32 bits on target.
 * Host builds can inspect the algorithm but have a different object layout. */
typedef struct PidController {
    _Alignas(8) double displayedProportionalGain; /* 0x00 */
    _Alignas(8) double displayedIntegralGain;     /* 0x08 */
    _Alignas(8) double displayedDerivativeGain;   /* 0x10 */
    _Alignas(8) double proportionalGain;          /* 0x18 */
    _Alignas(8) double integralGain;              /* 0x20: scaled by sample interval */
    _Alignas(8) double derivativeGain;            /* 0x28: divided by sample interval */
    int32_t direction;                            /* 0x30: 1 reverses gain signs */
    double *input;                                /* 0x34 */
    double *output;                               /* 0x38 */
    double *setpoint;                             /* 0x3c */
    uint32_t lastUpdateMs;                        /* 0x40 */
    _Alignas(8) double integralTerm;              /* 0x48 */
    _Alignas(8) double previousInput;             /* 0x50 */
    int32_t sampleIntervalMs;                     /* 0x58 */
    _Alignas(8) double minimumOutput;             /* 0x60 */
    _Alignas(8) double maximumOutput;             /* 0x68 */
    bool automatic;                               /* 0x70 */
} PidController;

#if UINTPTR_MAX == UINT32_MAX
_Static_assert(offsetof(PidController, input) == 0x34, "PID input layout");
_Static_assert(offsetof(PidController, integralTerm) == 0x48, "PID integral layout");
_Static_assert(offsetof(PidController, automatic) == 0x70, "PID mode layout");
#endif

void PID_disable(void *controller);
void PID_compute(void *controller);
void PID_set_tunings(void *controller, double proportionalGain, double integralGain,
                     double derivativeGain);
void PID_set_output_limits(void *controller, double minimum, double maximum);
void PID_initialize_integral(void *controller);
void PID_set_mode(void *controller, int mode);
void PID_set_direction(void *controller, int direction);
#endif
