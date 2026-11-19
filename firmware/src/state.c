#include "picobrew/state.h"

/* Original 0x2000489a. In particular, preserve limit settings and padding. */
void pico_still_clear_telemetry(PicoStillTelemetry *telemetry)
{
    telemetry->received = 0;
    telemetry->communication_failures = 0;
    telemetry->temperature_units = 0;
    telemetry->received_ms = 0;
    telemetry->temperature1 = 0.0;
    telemetry->temperature2 = 0.0;
    telemetry->temperature3 = 0.0;
    telemetry->reading4 = 0.0;
    telemetry->pressure = 0.0;
    telemetry->pressure_status = 0;
    telemetry->fan_on = 0;
    telemetry->status3 = 0;
}

/* Reconstructed from the global constructor at 0x200090f8.
 * This writes exactly the established logical fields; it is not memset/reset.
 * SDK objects and callbacks are supplied by the actual platform integration.
 * No original code, object, callback, or RAM address is embedded here. */
void pico_application_initialize(PicoAppState *state, const PicoPlatformObjects *objects,
                                 const PicoInitializationOps *ops, void *context,
                                 const PicoPageCallback pages[5])
{
    ops->construct_zero_address(context, objects->still_home_address);
    ops->construct_zero_address(context, objects->still_active_address);

    state->screen.recipe_name[0] = '\0';
    state->screen.step_name[0] = '\0';
    state->screen.progress_component = 0;
    state->screen.brew_state = 0;
    state->screen.heated_hold = 0;

    ops->construct_http_client(context, objects->http_client);
    ops->register_http_client_destructor(context, objects->http_client);
    ops->construct_rfid(context, objects->rfid_reader, 4, 18, 0);
    state->device_uid[0] = '\0';
    state->device_state = 2;
    ops->construct_address_octets(context, objects->server_address, 0, 0, 0, 0);

    state->carousel.last_update_ms = 0;
    for (unsigned i = 0; i < PICO_CAROUSEL_CAPACITY; ++i)
        state->carousel.pages[i] = NULL;
    state->carousel.interval_ms = 10000;
    /* Original page order: brew name, current step, step progress,
     * controller phase/temperature, recipe metrics. */
    for (unsigned i = 0; i < 5; ++i)
        state->carousel.pages[i] = pages[i];
    state->carousel.page_count = 5;

    state->control_signals.progress_row = 0;
    state->control_signals.progress_start_column = 0;
    state->control_signals.progress_end_column = 0;
    state->control_signals.progress_glyph_index = 0;
    state->progress_unknown_04 = 0;
    state->progress_unknown_08 = 0;
    state->progress_unknown_0c = 0;
    state->control_signals.progress_active = 0;

    PicoBrewController *controller = &state->controller;
    controller->still.temperature2_limit = 95.0;
    controller->still.temperature2_margin = 5.0;
    controller->still.pressure_limit = 10.0;
    controller->still.communication_failure_limit = 5;
    pico_still_clear_telemetry(&controller->still);
    pico_pid_disable(&controller->wort_pid);
    pico_pid_disable(&controller->thermoblock_pid);
    controller->compartment = 2;
    controller->status = 0;
    controller->wort_fahrenheit = 70.0;
    controller->thermoblock_fahrenheit = 83.0;
    controller->thermoblock_target_fahrenheit = 300.0;
    controller->active_shuttle_scale = 0.2;
    controller->candidate_shuttle_scale = 0.2;
    controller->drain_phase_started_ms = 0;
    controller->trend_seconds = 0;
    controller->phase = PICO_BREW_PHASE_IDLE;
    controller->history_count = 0;
    controller->shuttle_calibration_complete = 0;
    controller->calibration_flag_1a1 = 0;
    controller->drain_subphase = 0;
    controller->step_seconds = 0;
    pico_still_clear_telemetry(&controller->still);

    /* Preserve the constructor's state changes before and after each clock
     * call; the supplied clock is the only runtime service used by the PID. */
    pico_pid_construct_clocked(&controller->wort_pid, &controller->wort_fahrenheit,
                               &controller->wort_pid_output, &controller->wort_target_fahrenheit,
                               130.0, 50.0, 30.0, 0, 3000.0, ops->millis, context);
    pico_pid_construct_clocked(&controller->thermoblock_pid, &controller->thermoblock_fahrenheit,
                               &controller->thermoblock_pid_output,
                               &controller->thermoblock_target_fahrenheit, 150.0, 50.0, 80.0, 0,
                               3000.0, ops->millis, context);

    for (unsigned i = 0; i < PICO_MENU_CAPACITY; ++i)
        state->menu.actions[i] = 0;
    state->menu.count = 0;

    state->recipe.program = &state->session;
    for (unsigned i = 0; i < PICO_RECIPE_STEP_CAPACITY; ++i) {
        PicoRecipeStep *step = &state->recipe.steps[i];
        step->name[0] = '\0';
        step->kind = 0;
        step->duration = 0;
        step->temperature = 0;
        step->location = 2;
    }
    for (unsigned i = 0; i < 21; ++i) {
        state->recipe.identifier[i] = '\0';
        state->recipe.secondary_identifier[i] = '\0';
    }
    state->recipe.abv_adjustment = -1;
    state->recipe.ibu_adjustment = -1;
    state->recipe.current_step = 0;
    state->recipe.integer_metric = 0;
    state->recipe.decimal_metric = 0.0;
    state->buzzer_enabled = 0;
    state->stepper_capabilities = 0;
}
