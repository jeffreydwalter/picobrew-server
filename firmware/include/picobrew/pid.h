#ifndef PICOBREW_PID_H
#define PICOBREW_PID_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#define PICO_ALIGNAS(n) alignas(n)
extern "C" {
#else
#define PICO_ALIGNAS(n) _Alignas(n)
#endif

/* Pico's PID variant. Pointer-bearing state is owned by the application;
 * no field refers to an address in the original firmware image. */
typedef struct PicoPid {
    PICO_ALIGNAS(8) double displayedProportionalGain;
    PICO_ALIGNAS(8) double displayedIntegralGain;
    PICO_ALIGNAS(8) double displayedDerivativeGain;
    PICO_ALIGNAS(8) double proportionalGain;
    PICO_ALIGNAS(8) double integralGain;
    PICO_ALIGNAS(8) double derivativeGain;
    int32_t direction;
    volatile double *input;
    double *output;
    double *setpoint;
    uint32_t lastUpdateMs;
    PICO_ALIGNAS(8) double integralTerm;
    PICO_ALIGNAS(8) double previousInput;
    int32_t sampleIntervalMs;
    PICO_ALIGNAS(8) double minimumOutput;
    PICO_ALIGNAS(8) double maximumOutput;
    bool automatic;
} PicoPid;

void pico_pid_disable(PicoPid *pid);
void pico_pid_compute(PicoPid *pid, uint32_t now_ms);
void pico_pid_set_tunings(PicoPid *pid, double kp, double ki, double kd);
void pico_pid_set_output_limits(PicoPid *pid, double minimum, double maximum);
void pico_pid_initialize_integral(PicoPid *pid);
void pico_pid_set_mode(PicoPid *pid, int mode);
void pico_pid_set_direction(PicoPid *pid, int direction);
/* Like the original constructor, existing fields not written below are
 * preserved. Call on zero-initialized storage or an explicitly disabled,
 * otherwise valid object. The original global initializer disables it first.
 * The timestamp is the constructor's observed clock value, not an interval. */
void pico_pid_construct(PicoPid *pid, volatile double *input, double *output, double *setpoint,
                        double kp, double ki, double kd, int direction, double maximum_output,
                        uint32_t now_ms);
typedef uint32_t (*PicoMillisSource)(void *context);
/* Preserves the original constructor's ordering around the clock call. */
void pico_pid_construct_clocked(PicoPid *pid, volatile double *input, double *output,
                                double *setpoint, double kp, double ki, double kd, int direction,
                                double maximum_output, PicoMillisSource millis_source,
                                void *context);

#ifdef __cplusplus
}
#endif
#endif
