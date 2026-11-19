#include "picobrew/phase.h"
#include <stdio.h>
#include <string.h>
#include "picobrew/local_control.h"

static uint32_t now(PicoPhaseControl *phase)
{
    pico_local_poll();
    return phase->control->ops->millis(phase->control->context);
}

static void delay_ms(PicoPhaseControl *phase, uint32_t milliseconds)
{
#if PICO_STANDALONE
    (void)phase;
    pico_local_control_delay(milliseconds);
#else
    phase->actuators->ops->delay_milliseconds(phase->actuators->context, milliseconds);
#endif
}

static void copy_phase_label(PicoPhaseControl *phase, unsigned label)
{
    static const char *const labels[4][3] = {{"HEATING", "CALENTAMIENTO", "CHAUFFAGE"},
                                             {"HOLDING", "MANTENER", "MAINTIEN"},
                                             {"COOLING", "ENFRIAMIENTO", "REFROIDISSEMENT"},
                                             {"DRAINING", "DRENAR", "VIDANGE"}};
    uint8_t language = *phase->language;
    strcpy(phase->control->controller->phase_label, labels[label][language == 1   ? 1
                                                                  : language == 2 ? 2
                                                                                  : 0]);
}

static void complete_calibration(PicoPhaseControl *phase)
{
    PicoBrewController *state = phase->control->controller;
    state->thermoblock_target_fahrenheit = 330.0;
    state->shuttle_calibration_complete = 1;
    state->active_shuttle_scale = state->candidate_shuttle_scale + 0.15;
    phase->ops->save_shuttle_scale(phase->context, state->candidate_shuttle_scale);
    phase->calibration->settling_complete = 0;
    phase->calibration->history_count = 0;
}

uint8_t pico_phase_update_shuttle_calibration(PicoPhaseControl *phase)
{
    PicoBrewController *state = phase->control->controller;
    PicoShuttleCalibration *calibration = phase->calibration;
    uint8_t complete = state->shuttle_calibration_complete;
    if (complete != 0) {
        state->active_shuttle_scale = state->candidate_shuttle_scale + 0.15;
        return complete;
    }
    if (calibration->window_active == 0) {
        calibration->window_active = state->thermoblock_fahrenheit >= 300.0;
        state->heater_on_time_ms = 0;
        calibration->window_started_ms = now(phase);
    } else if (calibration->settling_complete == 0) {
        calibration->settling_complete = now(phase) - calibration->window_started_ms > 60000u;
        calibration->window_active = !calibration->settling_complete;
    } else if (now(phase) - calibration->window_started_ms > 60000u) {
        calibration->window_active = complete;
        if (state->heater_on_time_ms > 26700u) {
            state->candidate_shuttle_scale -= 0.04;
        } else if (state->heater_on_time_ms >= 22700u) {
            complete_calibration(phase);
            return state->shuttle_calibration_complete;
        } else {
            state->candidate_shuttle_scale += 0.04;
        }
        int32_t index = calibration->history_count++;
        calibration->candidate_history[index] = (float)state->candidate_shuttle_scale;
        if (calibration->history_count == 3) {
            if (calibration->candidate_history[0] == calibration->candidate_history[2]) {
                complete_calibration(phase);
                return state->shuttle_calibration_complete;
            }
            calibration->candidate_history[0] = calibration->candidate_history[1];
            calibration->candidate_history[1] = calibration->candidate_history[2];
            calibration->history_count = 2;
        }
    }
    state->active_shuttle_scale = state->candidate_shuttle_scale;
    return state->shuttle_calibration_complete;
}

void pico_phase_start_circulation(PicoPhaseControl *phase)
{
    if (pico_local_cancelled())
        return;
    if (phase->session->session_type - 3u < 3u) {
        pico_actuator_drain_off(phase->actuators);
        pico_actuator_steam_on(phase->actuators);
    } else {
        pico_actuators_start_drain_and_steam(phase->actuators);
    }
}

void pico_phase_set(PicoPhaseControl *phase, int32_t brew_phase)
{
    if (pico_local_cancelled())
        return;
    PicoBrewController *state = phase->control->controller;
    char message[132];
    snprintf(message, 128, "%s %d", "setBrewState", (int)brew_phase);
    phase->ops->log_message(phase->context, "I", "BREW", message);
    phase->screen->brew_state = 0;
    phase->screen->heated_hold = 0;
    switch (brew_phase) {
    case PICO_BREW_PHASE_HEATING:
    case PICO_BREW_PHASE_INDEFINITE_HEATING:
        phase->screen->brew_state = 1;
        state->thermoblock_target_fahrenheit =
            pico_phase_update_shuttle_calibration(phase) ? 330.0 : 300.0;
        copy_phase_label(phase, 0);
        pico_phase_start_circulation(phase);
        pico_control_start(phase->control);
        break;
    case PICO_BREW_PHASE_HEATED_HOLD:
        phase->screen->brew_state = 2;
        phase->screen->heated_hold = 1;
        state->active_shuttle_scale = state->candidate_shuttle_scale + 0.1;
        copy_phase_label(phase, 1);
        pico_phase_start_circulation(phase);
        pico_control_start(phase->control);
        break;
    case PICO_BREW_PHASE_COOLING:
        phase->screen->brew_state = 3;
        pico_control_stop(phase->control);
        copy_phase_label(phase, 2);
        pico_phase_start_circulation(phase);
        break;
    case PICO_BREW_PHASE_DRAINING:
        pico_control_stop(phase->control);
        copy_phase_label(phase, 3);
        pico_actuator_drain_on(phase->actuators);
        pico_actuator_steam_on(phase->actuators);
        pico_actuator_shuttle_on(phase->actuators);
        delay_ms(phase, 5000);
        state->drain_phase_started_ms = now(phase);
        pico_actuator_shuttle_off(phase->actuators);
        pico_actuator_steam_off(phase->actuators);
        phase->ops->move_to_compartment(phase->context, 0);
        state->compartment = 0;
        break;
    case PICO_BREW_PHASE_UNHEATED_CIRCULATION:
        pico_control_stop(phase->control);
        copy_phase_label(phase, 1);
        pico_phase_start_circulation(phase);
        break;
    case PICO_BREW_PHASE_COOL_THERMOBLOCK:
        copy_phase_label(phase, 1);
        pico_control_stop(phase->control);
        pico_actuator_drain_on(phase->actuators);
        pico_actuator_shuttle_on(phase->actuators);
        if (state->compartment != 1)
            pico_actuator_steam_on(phase->actuators);
        break;
    case PICO_BREW_PHASE_UNHEATED_PUMP_HOLD:
        pico_control_stop(phase->control);
        pico_actuator_shuttle_on(phase->actuators);
        copy_phase_label(phase, 1);
        pico_actuator_drain_on(phase->actuators);
        if (state->compartment != 1)
            pico_actuator_steam_on(phase->actuators);
        break;
    default:
        pico_control_stop(phase->control);
        break;
    }
    /* Pump interlocks deliberately observe the previous phase until here. */
    state->phase = (uint8_t)brew_phase;
}

void pico_phase_clear_status(PicoPhaseControl *phase)
{
    phase->ops->display_clear_row(phase->context, 0, 9);
    phase->control->controller->status = 0;
    pico_phase_set(phase, phase->control->controller->phase);
}

void pico_phase_pause_until_resume(PicoPhaseControl *phase)
{
    PicoBrewController *state = phase->control->controller;
    pico_control_stop(phase->control);
    pico_actuators_steam_and_drain_off(phase->actuators);
    phase->ops->wait_for_resume(phase->context);
    if (pico_local_cancelled())
        return;
    pico_phase_set(phase, state->phase);
    if (state->phase == PICO_BREW_PHASE_HEATING || state->phase == PICO_BREW_PHASE_COOLING) {
        phase->ops->refresh_current_page(phase->context);
        delay_ms(phase, 5000);
    }
}

void pico_phase_finish_step(PicoPhaseControl *phase, const char *program_title)
{
    PicoBrewController *state = phase->control->controller;
    now(phase);
    pico_control_stop(phase->control);
    if (pico_local_cancelled()) {
        pico_actuators_all_off(phase->actuators);
        state->phase = PICO_BREW_PHASE_IDLE;
        return;
    }
    phase->ops->show_ending_step(phase->context, program_title);
    phase->ops->display_flush(phase->context);
    pico_actuator_steam_off(phase->actuators);
    if (phase->session->session_type - 3u >= 3u)
        pico_actuator_drain_on(phase->actuators);
    delay_ms(phase, 1000);
    if (state->compartment == 3) {
        phase->ops->move_to_compartment(phase->context, 6);
        state->compartment = 6;
        delay_ms(phase, 30000);
    }
    state->status = 0;
    state->phase = PICO_BREW_PHASE_IDLE;
    state->drain_subphase = 0;
    phase->ops->display_clear(phase->context);
}
