#include "picobrew/control.h"
#include "picobrew/pid.h"
#include <limits.h>
#include "picobrew/local_control.h"

enum {
    PICO_TEMPERATURE_TIMER_BASE = 0x40030000u,
    PICO_OUTPUT_TIMER_BASE = 0x40031000u,
    PICO_TIMER_A = 0xffu,
    PICO_TIMER_TIMEOUT = 1u,
    PICO_TIMER_PERIODIC = 0x22u,
    PICO_HEATER_WINDOW_MS = 3000u,
    PICO_SHUTTLE_WINDOW_MS = 500u
};

static PicoControl *interrupt_control;

static int32_t signed_bits(uint32_t bits)
{
    int32_t value;
    __builtin_memcpy(&value, &bits, sizeof(value));
    return value;
}

/* The original conversion helpers saturate and map NaN to zero. Ordinary C
 * floating-to-integer casts would be undefined outside their valid ranges. */
static int32_t truncate_signed(double value)
{
    if (value != value)
        return 0;
    if (value <= -2147483648.0)
        return INT32_MIN;
    if (value >= 2147483647.0)
        return INT32_MAX;
    return (int32_t)value;
}

static uint32_t truncate_unsigned(double value)
{
    if (!(value > 0.0))
        return 0;
    if (value >= 4294967295.0)
        return UINT32_MAX;
    return (uint32_t)value;
}

void pico_control_record_temperature_sample(PicoControl *control)
{
    PicoBrewController *state = control->controller;
    /* Valid operating state has history_count in 0..6. The image has no lower
     * bound check; arbitrary corrupted negative indices remain out of scope. */
    if (state->history_count < 6) {
        int32_t index = state->history_count++;
        state->wort_history[index] = state->wort_fahrenheit;
    }
}

void pico_control_start(PicoControl *control)
{
    if (pico_local_cancelled())
        return;
    PicoBrewController *state = control->controller;
    if (state->status != 0)
        return;
    state->trend_seconds = 0;
    state->history_count = 0;
    state->heater_cycle_started_ms = control->ops->millis(control->context);
    state->shuttle_cycle_started_ms = state->heater_cycle_started_ms;
    pico_pid_set_mode(&state->wort_pid, true);
    pico_pid_set_mode(&state->thermoblock_pid, true);
    state->highest_wort_fahrenheit = 0.0;
    control->signals->enabled = 1;
}

bool pico_control_stop(PicoControl *control)
{
    PicoBrewController *state = control->controller;
    const PicoControlOps *io = control->ops;
    void *context = control->context;
    io->interrupts_disable(context);
    control->signals->enabled = 0;
    io->heater_off(context);
    io->shuttle_pump_off(context);
    pico_pid_set_mode(&state->wort_pid, false);
    pico_pid_set_mode(&state->thermoblock_pid, false);
    /* This is the image's unconditional enable, not an entry-mask restore. */
    return io->interrupts_enable(context);
}

int pico_control_classify_temperature_progress(PicoControl *control, double temperature)
{
    PicoBrewController *state = control->controller;
    if (state->phase == 0) {
        if (state->history_count != 6)
            return 0;
        uint32_t sum = 0;
        uint32_t weighted_sum = 0;
        for (int32_t index = 1; index <= 6; ++index) {
            int32_t sample = truncate_signed(state->wort_history[index - 1]);
            sum += (uint32_t)sample;
            weighted_sum += (uint32_t)sample * (uint32_t)index;
        }
        state->history_count = 0;
        int32_t numerator = signed_bits(weighted_sum * 6u - sum * 21u);
        /* The original integer denominator is 10, not floating-point 10.5. */
        int32_t trend = numerator / 10;
        if (trend < 1) {
            if (control->signals->slow_window_pending != 0) {
                control->signals->slow_window_pending = 0;
                return 1;
            }
            control->signals->slow_window_pending = 1;
        } else {
            control->signals->slow_window_pending = 0;
        }
    } else if (state->phase == 1) {
        double target = state->wort_target_fahrenheit;
        int result = temperature > target + 30.0 ? 2 : 0;
        if (temperature < target - 15.0 && state->highest_wort_fahrenheit > temperature) {
            /* Retain the two original volatile reads. */
            if (state->highest_wort_fahrenheit > temperature)
                return result + 1;
            state->highest_wort_fahrenheit = temperature;
        }
        return result;
    }
    return 0;
}

static void advance_heater_cycle(PicoBrewController *state, uint32_t now_ms)
{
    if ((uint32_t)(now_ms - state->heater_cycle_started_ms) > PICO_HEATER_WINDOW_MS) {
        /* Advance one window, even when more than one period was missed. */
        state->heater_cycle_started_ms += PICO_HEATER_WINDOW_MS;
        state->shuttle_cycle_started_ms = state->heater_cycle_started_ms;
    }
}

static void report_control_fault(PicoControl *control, uint8_t status)
{
    pico_control_stop(control);
    control->ops->steam_pump_off(control->context);
    control->ops->drain_pump_off(control->context);
    control->controller->status = status;
    control->ops->interrupts_disable(control->context);
}

static _Noreturn void halt_heater(PicoControl *control)
{
    /* Original 0x2000e054 never returns. Its platform implementation must
     * disable outputs, display/report the fault, and remain halted. */
    control->ops->halt_heater_malfunction(control->context);
    for (;;) {
    }
}

void pico_control_temperature_tick(PicoControl *control)
{
    PicoBrewController *state = control->controller;
    PicoControlSignals *signals = control->signals;
    const PicoControlOps *io = control->ops;
    void *context = control->context;
    uint32_t now_ms = io->millis(context);
    io->interrupts_disable(context);
    uint32_t flags = io->timer_interrupt_status(context, PICO_TEMPERATURE_TIMER_BASE, true);
    io->timer_interrupt_clear(context, PICO_TEMPERATURE_TIMER_BASE, flags);

    if (signals->progress_active != 0) {
        uint8_t column = (uint8_t)(signals->progress_glyph_index + signals->progress_start_column);
        signals->progress_glyph_index = (uint8_t)(signals->progress_glyph_index + 1u);
        io->progress_dot(context, signals->progress_row, column);
        if ((uint32_t)signals->progress_glyph_index + signals->progress_start_column ==
            signals->progress_end_column) {
            io->progress_clear(context, signals->progress_row, signals->progress_start_column);
            signals->progress_glyph_index = 0;
        }
        io->display_flush(context);
    }

    state->wort_fahrenheit = (double)io->read_wort_fahrenheit(context);
    state->thermoblock_fahrenheit = (double)io->read_thermoblock_fahrenheit(context);
    if (state->phase == 8)
        state->step_seconds = signed_bits((uint32_t)state->step_seconds + 1u);
    else if (state->step_seconds != 0)
        state->step_seconds = signed_bits((uint32_t)state->step_seconds - 1u);

    if (signals->enabled != 0 && state->status == 0) {
        if (state->wort_fahrenheit < 32.0 || state->thermoblock_fahrenheit < 32.0) {
            report_control_fault(control, 3);
        } else {
            if (state->thermoblock_fahrenheit > 385.0)
                halt_heater(control);
            if (state->wort_fahrenheit > 215.0 && state->wort_target_fahrenheit > 195.0) {
                pico_control_stop(control);
                state->status = 1;
                io->interrupts_disable(context);
            } else {
                pico_pid_compute(&state->wort_pid,
                                 state->wort_pid.automatic ? io->millis(context) : 0);
                pico_pid_compute(&state->thermoblock_pid,
                                 state->thermoblock_pid.automatic ? io->millis(context) : 0);
                advance_heater_cycle(state, now_ms);
                if (state->active_shuttle_scale < 0.0)
                    state->active_shuttle_scale = 0.0;
                double duty = state->wort_pid_output / 3000.0;
                duty = duty * state->active_shuttle_scale;
                duty = duty * 500.0;
                state->shuttle_on_time_ms = truncate_unsigned(duty);
                if (state->shuttle_on_time_ms < 22u)
                    state->shuttle_on_time_ms = 21u;
            }
        }

        if (state->status == 0) {
            state->trend_seconds += 1u;
            if (signed_bits(state->trend_seconds) >= 60) {
                pico_control_record_temperature_sample(control);
                state->trend_seconds = 0;
                double temperature = state->wort_fahrenheit;
                if (temperature > state->highest_wort_fahrenheit)
                    state->highest_wort_fahrenheit = temperature;
                if (temperature > state->previous_window_wort_fahrenheit)
                    state->previous_window_wort_fahrenheit = temperature;
                int progress = pico_control_classify_temperature_progress(control, temperature);
                if (progress == 1)
                    report_control_fault(control, 5);
                else if (progress == 2)
                    report_control_fault(control, 6);
                else if (progress == 3)
                    report_control_fault(control, 7);
            } else if (signed_bits(state->trend_seconds) % 30 == 0) {
                state->previous_window_wort_fahrenheit = state->wort_fahrenheit;
                pico_control_record_temperature_sample(control);
            }
        }
    }

    if (signals->enabled == 0 && state->thermoblock_fahrenheit > 385.0)
        halt_heater(control);
    io->interrupts_enable(context);
}

void pico_control_outputs_tick(PicoControl *control)
{
    PicoBrewController *state = control->controller;
    const PicoControlOps *io = control->ops;
    void *context = control->context;
    uint32_t now_ms = io->millis(context);
    io->interrupts_disable(context);
    uint32_t flags = io->timer_interrupt_status(context, PICO_OUTPUT_TIMER_BASE, true);
    io->timer_interrupt_clear(context, PICO_OUTPUT_TIMER_BASE, flags);
    if (control->signals->enabled != 0 && state->status == 0) {
        advance_heater_cycle(state, now_ms);
        if (state->thermoblock_pid_output >
            (double)(uint32_t)(now_ms - state->heater_cycle_started_ms)) {
            io->heater_on(context);
            state->heater_on_time_ms += 20u;
        } else {
            io->heater_off(context);
        }
        if ((uint32_t)(now_ms - state->shuttle_cycle_started_ms) < state->shuttle_on_time_ms)
            io->shuttle_pump_on(context);
        else
            io->shuttle_pump_off(context);
        if ((uint32_t)(now_ms - state->shuttle_cycle_started_ms) > PICO_SHUTTLE_WINDOW_MS)
            state->shuttle_cycle_started_ms += PICO_SHUTTLE_WINDOW_MS;
    }
    io->interrupts_enable(context);
}

void pico_control_bind_interrupts(PicoControl *control)
{
    interrupt_control = control;
}

void pico_temperature_control_timer_isr(void)
{
    pico_control_temperature_tick(interrupt_control);
}

void pico_heater_pump_timer_isr(void)
{
    pico_control_outputs_tick(interrupt_control);
}

bool pico_control_timers_initialize(PicoControl *control)
{
    const PicoControlOps *io = control->ops;
    void *context = control->context;
    pico_control_bind_interrupts(control);
    io->interrupts_disable(context);
    io->peripheral_clock_enable(context, 14, 1);
    io->peripheral_reset(context, 14);
    io->timer_configure(context, PICO_TEMPERATURE_TIMER_BASE, PICO_TIMER_PERIODIC);
    io->timer_load_set(context, PICO_TEMPERATURE_TIMER_BASE, PICO_TIMER_A, 80000000u);
    io->timer_interrupt_register(context, PICO_TEMPERATURE_TIMER_BASE, PICO_TIMER_A,
                                 pico_temperature_control_timer_isr);
    io->timer_interrupt_enable(context, PICO_TEMPERATURE_TIMER_BASE, PICO_TIMER_TIMEOUT);
    io->peripheral_clock_enable(context, 15, 1);
    io->peripheral_reset(context, 15);
    io->timer_configure(context, PICO_OUTPUT_TIMER_BASE, PICO_TIMER_PERIODIC);
    io->timer_load_set(context, PICO_OUTPUT_TIMER_BASE, PICO_TIMER_A, 1600000u);
    io->timer_interrupt_register(context, PICO_OUTPUT_TIMER_BASE, PICO_TIMER_A,
                                 pico_heater_pump_timer_isr);
    io->timer_interrupt_enable(context, PICO_OUTPUT_TIMER_BASE, PICO_TIMER_TIMEOUT);
    io->timer_enable(context, PICO_OUTPUT_TIMER_BASE, PICO_TIMER_A);
    io->timer_enable(context, PICO_TEMPERATURE_TIMER_BASE, PICO_TIMER_A);
    return io->interrupts_enable(context);
}
