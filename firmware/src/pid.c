#include "picobrew/pid.h"

/* Reviewed image behavior: pico_pid_compute has no elapsed-period guard, and
 * pico_pid_set_mode initializes on either mode transition. Retain these details
 * when comparing with other Arduino PID implementations. */

/* Original entry 0x200146cc. */
void pico_pid_disable(PicoPid *pid)
{
    pid->automatic = false;
}

/* Original entry 0x200146d4. */
void pico_pid_compute(PicoPid *pid, uint32_t now_ms)
{
    if (!pid->automatic)
        return;

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
    pid->lastUpdateMs = now_ms;
}

/* Original entry 0x200147c0. */
void pico_pid_set_tunings(PicoPid *pid, double proportionalGain, double integralGain,
                          double derivativeGain)
{
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

/* Original entry 0x20014890. */
void pico_pid_set_output_limits(PicoPid *pid, double minimum, double maximum)
{
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

/* Original entry 0x2001491c. */
void pico_pid_initialize_integral(PicoPid *pid)
{
    pid->integralTerm = *pid->output;
    pid->previousInput = *pid->input;
    if (pid->integralTerm > pid->maximumOutput)
        pid->integralTerm = pid->maximumOutput;
    else if (pid->integralTerm < pid->minimumOutput)
        pid->integralTerm = pid->minimumOutput;
}

/* Original entry 0x20014962. */
void pico_pid_set_mode(PicoPid *pid, int mode)
{
    bool automatic = mode == 1;
    if (automatic != pid->automatic)
        pico_pid_initialize_integral(pid);
    pid->automatic = automatic;
}

/* Original entry 0x20014982. */
void pico_pid_set_direction(PicoPid *pid, int direction)
{
    if (pid->automatic && direction != pid->direction) {
        pid->proportionalGain = 0.0 - pid->proportionalGain;
        pid->integralGain = 0.0 - pid->integralGain;
        pid->derivativeGain = 0.0 - pid->derivativeGain;
    }
    pid->direction = direction;
}

/* Original constructor 0x200149c8. The word between direction and maximum
 * in the old decompilation is double-argument alignment padding, not an input. */
static void prepare_constructor(PicoPid *pid, double kp, double ki, double kd, int direction,
                                double maximum_output)
{
    pico_pid_set_output_limits(pid, 0.0, maximum_output);
    pid->sampleIntervalMs = 100;
    pico_pid_set_direction(pid, direction);
    pico_pid_set_tunings(pid, kp, ki, kd);
}

static void bind_constructor(PicoPid *pid, volatile double *input, double *output, double *setpoint,
                             uint32_t now_ms)
{
    pid->output = output;
    pid->lastUpdateMs = now_ms - (uint32_t)pid->sampleIntervalMs;
    pid->automatic = false;
    pid->input = input;
    pid->setpoint = setpoint;
}

void pico_pid_construct(PicoPid *pid, volatile double *input, double *output, double *setpoint,
                        double kp, double ki, double kd, int direction, double maximum_output,
                        uint32_t now_ms)
{
    prepare_constructor(pid, kp, ki, kd, direction, maximum_output);
    bind_constructor(pid, input, output, setpoint, now_ms);
}

void pico_pid_construct_clocked(PicoPid *pid, volatile double *input, double *output,
                                double *setpoint, double kp, double ki, double kd, int direction,
                                double maximum_output, PicoMillisSource millis_source,
                                void *context)
{
    prepare_constructor(pid, kp, ki, kd, direction, maximum_output);
    uint32_t now_ms = millis_source(context);
    bind_constructor(pid, input, output, setpoint, now_ms);
}
