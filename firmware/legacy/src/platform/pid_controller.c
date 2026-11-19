/* Pico Pro 0.1.34 — pid controller. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Reviewed image behavior: PID_compute has no elapsed-period guard, and
 * PID_set_mode initializes on either mode transition. Retain these details
 * when comparing with other Arduino PID implementations. */

/* Consistent PID state layout: input/output/setpoint pointers, gains, integral term, last input,
 * sample interval and output limits; double arithmetic helpers identified. Firmware address:
 * 0x200146cc. */
void PID_disable(void *controller)
{
    PidController *pid = controller;
    pid->automatic = false;
}

/* Consistent PID state layout: input/output/setpoint pointers, gains, integral term, last input,
 * sample interval and output limits; double arithmetic helpers identified. Firmware address:
 * 0x200146d4. */
void PID_compute(void *controller)
{
    PidController *pid = controller;
    if (!pid->automatic)
        return;

    uint32_t now = (uint32_t)millis();
    double input = *pid->input;
    double error = *pid->setpoint - input;

    pid->integralTerm = error * pid->integralGain + pid->integralTerm;
    double maximumOutput = pid->maximumOutput;
    if (pid->integralTerm > maximumOutput)
        pid->integralTerm = maximumOutput;
    else if (pid->integralTerm < pid->minimumOutput)
        pid->integralTerm = pid->minimumOutput;

    double proportionalAndIntegral = error * pid->proportionalGain + pid->integralTerm;
    double inputChange = input - pid->previousInput;
    double output = proportionalAndIntegral - inputChange * pid->derivativeGain;
    if (output > maximumOutput)
        output = maximumOutput;
    else if (output < pid->minimumOutput)
        output = pid->minimumOutput;

    *pid->output = output;
    pid->previousInput = input;
    pid->lastUpdateMs = now;
}

/* Consistent PID state layout: input/output/setpoint pointers, gains, integral term, last input,
 * sample interval and output limits; double arithmetic helpers identified. Firmware address:
 * 0x200147c0. */
void PID_set_tunings(void *controller, double proportionalGain, double integralGain,
                     double derivativeGain)
{
    PidController *pid = controller;
    if (proportionalGain < 0.0 || integralGain < 0.0 || derivativeGain < 0.0)
        return;

    pid->displayedProportionalGain = proportionalGain;
    pid->displayedIntegralGain = integralGain;
    pid->displayedDerivativeGain = derivativeGain;

    double intervalSeconds = (double)pid->sampleIntervalMs / 1000.0;
    pid->proportionalGain = proportionalGain;
    pid->integralGain = integralGain * intervalSeconds;
    pid->derivativeGain = derivativeGain / intervalSeconds;
    if (pid->direction == 1) {
        pid->proportionalGain = 0.0 - pid->proportionalGain;
        pid->integralGain = 0.0 - pid->integralGain;
        pid->derivativeGain = 0.0 - pid->derivativeGain;
    }
}

/* Consistent PID state layout: input/output/setpoint pointers, gains, integral term, last input,
 * sample interval and output limits; double arithmetic helpers identified. Firmware address:
 * 0x20014890. */
void PID_set_output_limits(void *controller, double minimum, double maximum)
{
    PidController *pid = controller;
    if (minimum >= maximum)
        return;

    pid->minimumOutput = minimum;
    pid->maximumOutput = maximum;
    if (pid->automatic) {
        if (*pid->output > maximum)
            *pid->output = maximum;
        else if (*pid->output < minimum)
            *pid->output = minimum;

        if (pid->integralTerm > maximum)
            pid->integralTerm = maximum;
        else if (pid->integralTerm < minimum)
            pid->integralTerm = minimum;
    }
}

/* Consistent PID state layout: input/output/setpoint pointers, gains, integral term, last input,
 * sample interval and output limits; double arithmetic helpers identified. Firmware address:
 * 0x2001491c. */
void PID_initialize_integral(void *controller)
{
    PidController *pid = controller;
    pid->integralTerm = *pid->output;
    pid->previousInput = *pid->input;
    if (pid->integralTerm > pid->maximumOutput)
        pid->integralTerm = pid->maximumOutput;
    else if (pid->integralTerm < pid->minimumOutput)
        pid->integralTerm = pid->minimumOutput;
}

/* Consistent PID state layout: input/output/setpoint pointers, gains, integral term, last input,
 * sample interval and output limits; double arithmetic helpers identified. Firmware address:
 * 0x20014962. */
void PID_set_mode(void *controller, int mode)
{
    PidController *pid = controller;
    bool automatic = mode == 1;
    if (automatic != pid->automatic)
        PID_initialize_integral(pid);
    pid->automatic = automatic;
}

/* Consistent PID state layout: input/output/setpoint pointers, gains, integral term, last input,
 * sample interval and output limits; double arithmetic helpers identified. Firmware address:
 * 0x20014982. */
void PID_set_direction(void *controller, int direction)
{
    PidController *pid = controller;
    if (pid->automatic && direction != pid->direction) {
        pid->proportionalGain = 0.0 - pid->proportionalGain;
        pid->integralGain = 0.0 - pid->integralGain;
        pid->derivativeGain = 0.0 - pid->derivativeGain;
    }
    pid->direction = direction;
}

/* Constructor stores input/output/setpoint pointers at0x34/0x38/0x3c, default interval100ms,
 * direction and tunings, output limits, last-update and manual mode. Exact remaining optimized
 * argument layout not reconstructed. Firmware address: 0x200149c8. */
void PID_construct(void *controller, uint32_t input, uint32_t output, uint32_t setpoint,
                   double argument_5, uint32_t argument_6, uint32_t argument_7, uint32_t argument_8,
                   uint32_t argument_9, int argument_10, uint32_t argument_11, uint32_t argument_12,
                   uint32_t argument_13)
{
    uint32_t now_ms;

    PID_set_output_limits(controller, 0.0, (double)pico_join_u32_u32(argument_13, argument_12));
    *(uint32_t *)((int)controller + 0x58) = 100;
    PID_set_direction(controller, argument_10);
    PID_set_tunings(controller, argument_5, (double)pico_join_u32_u32(argument_7, argument_6),
                    (double)pico_join_u32_u32(argument_9, argument_8));
    now_ms = millis();
    *(uint32_t *)((int)controller + 0x38) = output;
    *(uint32_t *)((int)controller + 0x40) = now_ms - *(int *)((int)controller + 0x58);
    *(uint8_t *)((int)controller + 0x70) = 0;
    *(uint32_t *)((int)controller + 0x34) = input;
    *(uint32_t *)((int)controller + 0x3c) = setpoint;
    return;
}
