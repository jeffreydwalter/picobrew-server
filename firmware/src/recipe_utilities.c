#include "picobrew/recipe.h"
#include "picobrew/local_control.h"
#include <limits.h>
#include <stdio.h>

static uint32_t clock_ms(PicoRecipeRuntime *r)
{
    pico_local_poll();
    return r->phase->control->ops->millis(r->phase->control->context);
}
static void delay_ms(PicoRecipeRuntime *r, uint32_t milliseconds)
{
#if PICO_STANDALONE
    (void)r;
    pico_local_control_delay(milliseconds);
#else
    r->phase->actuators->ops->delay_milliseconds(r->phase->actuators->context, milliseconds);
#endif
}
static void show(PicoRecipeRuntime *r, PicoRecipeScreen screen, uint32_t argument, const char *text)
{
    r->ops->screen(r->context, screen, argument, text);
}
static int prompt(PicoRecipeRuntime *r, PicoRecipePrompt kind, uint32_t argument)
{
    return r->ops->prompt(r->context, kind, argument, NULL);
}
static void move_to(PicoRecipeRuntime *r, uint32_t compartment)
{
    r->phase->ops->move_to_compartment(r->phase->context, compartment);
}
static void center(PicoRecipeRuntime *r, uint16_t row, const char *text)
{
    r->ops->center_text(r->context, row, text);
}
static void timer(PicoRecipeRuntime *r, uint16_t row, uint32_t start, uint32_t seconds)
{
    r->ops->timer(r->context, row, start, seconds);
}
static void temperature_columns(PicoRecipeRuntime *r)
{
    r->ops->temperature_columns(r->context, 5, r->state->controller.wort_fahrenheit,
                                r->state->controller.thermoblock_fahrenheit, 0.0);
}
static int32_t trunc_i32(double value)
{
    if (value != value)
        return 0;
    if (value >= 2147483647.0)
        return INT32_MAX;
    if (value <= -2147483648.0)
        return INT32_MIN;
    return (int32_t)value;
}

/* Original 0x2000d2e8. The return value is always zero, including a resumed
 * filter-removal pause; callers retain that behavior rather than inferring it. */
bool pico_recipe_handle_filter_removal(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return false;
    if (!pico_filter_is_inserted(r->phase->actuators)) {
        pico_control_stop(r->phase->control);
        pico_actuators_steam_and_drain_off(r->phase->actuators);
        r->phase->actuators->ops->prompt_insert_filter(r->phase->actuators->context);
        pico_phase_set(r->phase, r->state->controller.phase);
    }
    return false;
}

/* Original 0x2000d380. */
bool pico_recipe_prime_steam(PicoRecipeRuntime *r, const char *title)
{
    if (pico_local_cancelled())
        return false;
    PicoActuators *a = r->phase->actuators;
    if (!pico_filter_is_inserted(a)) {
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        r->phase->actuators->ops->prompt_insert_filter(r->phase->actuators->context);
    }
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_TITLED_FRAME, 0, title);
    center(r, 3, "Priming steam");
    center(r, 4, "generator");
    show(r, PICO_RECIPE_FLUSH, 0, NULL);
    move_to(r, 2);
    r->state->controller.compartment = 2;
    pico_phase_start_circulation(r->phase);
    bool heated = r->ops->check_thermoblock_heating(r->context);
    if (heated) {
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        show(r, PICO_RECIPE_TITLED_FRAME, 0, title);
        center(r, 3, "Priming steam");
        center(r, 4, "pump");
        show(r, PICO_RECIPE_FLUSH, 0, NULL);
        delay_ms(r, 10000);
        pico_actuator_steam_off(a);
        pico_actuator_drain_off(a);
    }
    return heated;
}

/* Original 0x20011338. The existing nonzero calibration byte is returned
 * literally, not normalized, after a successful thermal check. */
uint32_t pico_recipe_reprime_steam(PicoRecipeRuntime *r, const char *title, const char *step,
                                   bool logging, bool restart_circulation)
{
    if (pico_local_cancelled())
        return 0;
    PicoActuators *a = r->phase->actuators;
    PicoBrewController *c = &r->state->controller;
    if (!pico_filter_is_inserted(a)) {
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        r->phase->actuators->ops->prompt_insert_filter(r->phase->actuators->context);
    }
    if (restart_circulation)
        pico_phase_start_circulation(r->phase);
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_TITLED_FRAME, 0, title);
    center(r, 3, "Priming steam");
    center(r, 4, "pump");
    show(r, PICO_RECIPE_FLUSH, 0, NULL);
    uint32_t heated = r->ops->check_thermoblock_heating(r->context);
    if (!heated)
        return 0;
    uint32_t last_log = clock_ms(r);
    pico_actuator_shuttle_on(a);
    unsigned intervals = 16;
    while (c->thermoblock_fahrenheit > 230.0) {
        pico_local_poll();
        if (pico_local_cancelled())
            return 0;
        if (logging && clock_ms(r) - last_log >= 25001u) {
            r->ops->log_progress(r->context, step);
            last_log = clock_ms(r);
            if (--intervals == 0)
                break;
        }
    }
    pico_actuator_shuttle_off(a);
    if (restart_circulation) {
        pico_phase_start_circulation(r->phase);
        delay_ms(r, 5000);
    }
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    if (c->shuttle_calibration_complete)
        return c->shuttle_calibration_complete;
    c->active_shuttle_scale = 0.2;
    c->candidate_shuttle_scale = 0.2;
    return heated;
}

/* Original 0x2000d418. */
void pico_recipe_restore_idle(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    show(r, PICO_RECIPE_ENDING_PROCESS, 0, NULL);
    r->ops->move_home(r->context);
}

/* Original 0x2000d484. Its first machine-register argument was unused. */
void pico_recipe_vacuum(PicoRecipeRuntime *r, uint32_t compartment, uint32_t first)
{
    if (pico_local_cancelled())
        return;
    PicoActuators *a = r->phase->actuators;
    pico_actuators_all_off(a);
    if (prompt(r, PICO_RECIPE_VACUUM, 0)) {
        pico_actuator_steam_on(a);
        prompt(r, PICO_RECIPE_VACUUM, 1);
        uint32_t started = clock_ms(r);
        clock_ms(r);
        uint32_t dwell = 3000;
        while (!r->ops->take_click(r->context)) {
            pico_local_poll();
            if (pico_local_cancelled())
                return;
            uint32_t now = clock_ms(r);
            if (pico_actuators_pause_for_missing_filter(a, true, false, true)) {
                prompt(r, PICO_RECIPE_VACUUM, 1);
                started = started - now + clock_ms(r);
            }
            if (clock_ms(r) - started > dwell) {
                pico_actuator_steam_off(a);
                move_to(r, compartment);
                pico_actuator_steam_on(a);
                started = clock_ms(r);
                compartment = (compartment + 1u) & 255u;
                if (compartment > 6) {
                    dwell *= 3;
                    compartment = first;
                }
            }
        }
        pico_actuator_steam_off(a);
        pico_actuator_drain_off(a);
    }
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    pico_recipe_restore_idle(r);
}

/* Original 0x2000d524. */
void pico_recipe_circulate(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    PicoActuators *a = r->phase->actuators;
    show(r, PICO_RECIPE_PROGRAM_NOTICE, 0, NULL);
    move_to(r, 2);
    pico_actuators_start_drain_and_steam(a);
    uint32_t next_update = 0, elapsed = 0;
    while (!r->ops->take_click(r->context)) {
        pico_local_poll();
        if (pico_local_cancelled())
            return;
        if (!pico_filter_is_inserted(a)) {
            pico_actuators_steam_and_drain_off(a);
            r->phase->actuators->ops->prompt_insert_filter(r->phase->actuators->context);
            pico_actuators_start_drain_and_steam(a);
        }
        uint32_t now = clock_ms(r);
        if (next_update < now) {
            r->ops->circulation_status(r->context, r->state->controller.wort_fahrenheit, elapsed);
            next_update = now + 1000;
            ++elapsed;
        }
    }
    pico_actuators_steam_and_drain_off(a);
    pico_recipe_restore_idle(r);
}

/* Original 0x2000d580. */
void pico_recipe_drain_reservoir(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    PicoActuators *a = r->phase->actuators;
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    clock_ms(r);
    show(r, PICO_RECIPE_DRAIN_RESERVOIR, 0, NULL);
    show(r, PICO_RECIPE_FLUSH, 0, NULL);
    move_to(r, 2);
    pico_actuator_shuttle_on(a);
    delay_ms(r, 500);
    pico_actuator_steam_on(a);
    for (;;) {
        pico_local_poll();
        if (pico_local_cancelled())
            return;
        show(r, PICO_RECIPE_STOP_FOOTER, 0, NULL);
        show(r, PICO_RECIPE_FLUSH, 0, NULL);
        for (;;) {
            pico_local_poll();
            if (pico_local_cancelled())
                return;
            if (r->ops->take_click(r->context)) {
                pico_actuator_shuttle_off(a);
                delay_ms(r, 500);
                pico_actuator_steam_off(a);
                pico_recipe_restore_idle(r);
                return;
            }
            if (!pico_filter_is_inserted(a))
                break;
        }
        pico_actuator_shuttle_off(a);
        pico_actuator_steam_off(a);
        r->phase->actuators->ops->prompt_insert_filter(r->phase->actuators->context);
        pico_actuator_steam_on(a);
        delay_ms(r, 500);
        pico_actuator_shuttle_on(a);
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        show(r, PICO_RECIPE_DRAIN_RESERVOIR, 0, NULL);
    }
}

/* Original 0x2000d738. completed_cycles deliberately retains byte wraparound. */
void pico_recipe_flush_compartments(PicoRecipeRuntime *r, uint32_t minimum_cycles, bool automatic)
{
    if (pico_local_cancelled())
        return;
    PicoActuators *a = r->phase->actuators;
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    clock_ms(r);
    show(r, PICO_RECIPE_HEADER, 0, "TEST ROTARY VALVE");
    show(r, PICO_RECIPE_FLUSH, 0, NULL);
    pico_actuator_drain_on(a);
    move_to(r, 0);
    uint8_t completed = 0;
    for (;;) {
        pico_local_poll();
        if (pico_local_cancelled())
            return;
        if (minimum_cycles <= completed) {
            if (automatic)
                break;
            show(r, PICO_RECIPE_STOP_FOOTER, 0, NULL);
            show(r, PICO_RECIPE_FLUSH, 0, NULL);
        }
        for (uint32_t compartment = 1; compartment != 7; ++compartment) {
            move_to(r, compartment);
            uint32_t started = clock_ms(r);
            r->ops->valve_position(r->context, 3, compartment);
            center(r, 5, "(Keg Pump Active)");
            show(r, PICO_RECIPE_FLUSH, 0, NULL);
            pico_actuator_steam_on(a);
            while (clock_ms(r) - started < 3000) {
                pico_local_poll();
                if (pico_local_cancelled())
                    return;
                if (r->ops->take_click(r->context) && minimum_cycles <= completed)
                    goto finished;
            }
            pico_actuator_steam_off(a);
            r->ops->clear_row(r->context, 5, 0);
            show(r, PICO_RECIPE_FLUSH, 0, NULL);
        }
        ++completed;
    }
finished:
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    pico_actuators_steam_and_drain_off(a);
    pico_recipe_restore_idle(r);
}

/* Original 0x2000d7ec. */
void pico_recipe_flush_until_press(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    pico_recipe_flush_compartments(r, 0, false);
}

/* Original 0x2000da08. r1 was padding before the AAPCS double in r2/r3. */
void pico_recipe_heat_flush_compartments(PicoRecipeRuntime *r, bool steam, double target)
{
    if (pico_local_cancelled())
        return;
    PicoActuators *a = r->phase->actuators;
    PicoBrewController *c = &r->state->controller;
    pico_actuator_drain_on(a);
    for (uint32_t compartment = 1; compartment != 7; ++compartment) {
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        show(r, PICO_RECIPE_HEADER, 0, "RV Pressure Test");
        center(r, 4, "Moving Squid");
        show(r, PICO_RECIPE_FLUSH, 0, NULL);
        move_to(r, compartment);
        r->ops->valve_position(r->context, 3, compartment);
        r->ops->clear_row(r->context, 4, 0);
        char label[40];
        int32_t integer_target = trunc_i32(target);
        snprintf(label, sizeof label, "Heating to%ld",
                 (long)trunc_i32((double)integer_target - 20.0));
        center(r, 4, label);
        uint32_t started = clock_ms(r);
        double preheat = target - 20.0;
        if (c->thermoblock_fahrenheit < preheat) {
            pico_actuator_heater_on(a);
            while (c->thermoblock_fahrenheit < preheat) {
                pico_local_poll();
                if (pico_local_cancelled())
                    return;
                if (clock_ms(r) - started > 15000 && c->thermoblock_fahrenheit < 90.0) {
                    pico_actuators_off_except_drain(a);
                    r->ops->error(r->context, 19, false, false);
                    return;
                }
                temperature_columns(r);
                show(r, PICO_RECIPE_FLUSH, 0, NULL);
            }
            pico_actuator_heater_off(a);
        }
        r->ops->clear_row(r->context, 4, 0);
        snprintf(label, sizeof label, "Heating to%ld", (long)integer_target);
        center(r, 4, label);
        started = clock_ms(r);
        while (c->thermoblock_fahrenheit < target && clock_ms(r) - started < 5000) {
            pico_local_poll();
            if (pico_local_cancelled())
                return;
            temperature_columns(r);
            show(r, PICO_RECIPE_FLUSH, 0, NULL);
        }
        r->ops->clear_row(r->context, 4, 0);
        center(r, 4, "SHUTTLE ON");
        if (steam)
            pico_actuator_steam_on(a);
        pico_actuator_shuttle_on(a);
        clock_ms(r);
        double threshold = target;
        for (unsigned stage = 0; stage != 4; ++stage) {
            clock_ms(r);
            while (c->thermoblock_fahrenheit > threshold) {
                pico_local_poll();
                if (pico_local_cancelled())
                    return;
                temperature_columns(r);
                show(r, PICO_RECIPE_FLUSH, 0, NULL);
            }
            clock_ms(r);
            threshold = target - (double)((stage + 1) * 10);
        }
        pico_actuator_shuttle_off(a);
        pico_actuator_steam_off(a);
    }
    pico_actuator_drain_off(a);
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    pico_recipe_restore_idle(r);
}

/* Original 0x2000deec. */
void pico_recipe_rinse(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    static const uint32_t instructions[] = {21, 22};
    PicoActuators *a = r->phase->actuators;
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    r->state->device_state = 6;
    uint32_t first = *r->legacy_compartment;
    const char *title = r->ops->title_label(r->context, 1, *r->language);
    if (r->ops->instruction_sequence(r->context, instructions, 2, 0, 0, title, false) != 23) {
        r->ops->log(r->context, "W", "RCTL", "Rinse setup aborted by user");
        r->state->device_state = 2;
        return;
    }
    show(r, PICO_RECIPE_WAIT_FILTER_INSERTION, 0, NULL);
    show(r, PICO_RECIPE_INSTRUCTION_HEADER, 23, title);
    r->ops->report_state(r->context);
    uint32_t started = clock_ms(r);
    pico_actuator_drain_on(a);
    move_to(r, first);
    while (clock_ms(r) - started < 60000) {
        pico_local_poll();
        if (pico_local_cancelled())
            return;
        uint32_t now = clock_ms(r);
        if (pico_actuators_pause_for_missing_filter(a, false, true, false)) {
            show(r, PICO_RECIPE_INSTRUCTION_HEADER, 23, title);
            started = started - now + clock_ms(r);
        }
        timer(r, 5, started, 60);
    }
    pico_actuator_drain_off(a);
    pico_actuator_steam_off(a);
    pico_recipe_vacuum(r, (first + 1u) & 255u, first);
    r->state->device_state = 2;
    r->ops->report_state(r->context);
    r->ops->instruction_screen(r->context, 24, 0, 0, title, false);
}

/* Original 0x2000ed9c. Disassembly confirms r3 points at the caller's int32
 * next-compartment counter; the first three original register values are unused. */
bool pico_recipe_first_rinse_cool(PicoRecipeRuntime *r, int32_t *next_compartment)
{
    if (pico_local_cancelled())
        return false;
    PicoActuators *a = r->phase->actuators;
    double initial = r->state->controller.thermoblock_fahrenheit;
    show(r, PICO_RECIPE_FIRST_RINSE_STEP, 5, NULL);
    pico_actuator_steam_off(a);
    pico_actuator_shuttle_on(a);
    pico_actuator_drain_on(a);
    uint32_t started = clock_ms(r), last_move = started;
    while (clock_ms(r) - started < 120000) {
        pico_local_poll();
        if (pico_local_cancelled())
            return false;
        uint32_t now = clock_ms(r);
        if (pico_actuators_pause_for_missing_filter(a, false, true, true)) {
            show(r, PICO_RECIPE_FIRST_RINSE_STEP, 5, NULL);
            started = started - now + clock_ms(r);
            last_move = last_move - now + clock_ms(r);
        }
        if (clock_ms(r) - last_move > 10000) {
            move_to(r, (uint8_t)*next_compartment);
            last_move = clock_ms(r);
            int32_t next = *next_compartment + 1;
            *next_compartment = next > 6 ? 1 : next;
            timer(r, 6, started, 120);
        }
        timer(r, 6, started, 120);
    }
    return r->state->controller.thermoblock_fahrenheit < initial - 85.0;
}

/* Original 0x2000ee78. */
bool pico_recipe_first_rinse_heat(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return false;
    PicoActuators *a = r->phase->actuators;
    PicoBrewController *c = &r->state->controller;
    clock_ms(r);
    pico_actuator_heater_on(a);
    show(r, PICO_RECIPE_FLUSH, 0, NULL);
    uint32_t started = clock_ms(r);
    bool success;
    for (;;) {
        pico_local_poll();
        if (pico_local_cancelled())
            return false;
        if (!(c->thermoblock_fahrenheit < 230.0)) {
            success = true;
            break;
        }
        if (clock_ms(r) - started > 30000 && c->thermoblock_fahrenheit < 130.0) {
            success = false;
            break;
        }
        timer(r, 6, started, 0);
    }
    pico_actuator_heater_off(a);
    started = clock_ms(r);
    show(r, PICO_RECIPE_FIRST_RINSE_STEP, 3, NULL);
    while (!pico_local_cancelled() && clock_ms(r) - started < 30000)
        timer(r, 6, started, 30);
    return success;
}

/* Original 0x2000ef2c. */
void pico_recipe_drain_filter(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_DRAIN_FILTER, 0, NULL);
    pico_actuator_drain_on(r->phase->actuators);
    show(r, PICO_RECIPE_WAIT_STOP, 0, NULL);
    pico_actuator_drain_off(r->phase->actuators);
}

/* Original 0x2000ef48. */
void pico_recipe_first_rinse(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    PicoActuators *a = r->phase->actuators;
    PicoBrewController *c = &r->state->controller;
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_FIRST_RINSE_STEP, 0, NULL);
    if (!pico_filter_is_inserted(a)) {
        show(r, PICO_RECIPE_FIRST_RINSE_STEP, 1, NULL);
        r->ops->wait_for_filter(r->context);
    }
    show(r, PICO_RECIPE_FIRST_RINSE_STEP, 2, NULL);
    r->state->device_state = 6;
    r->ops->report_state(r->context);
    int32_t next = 2;
    move_to(r, 1);
    bool heat_ok = pico_recipe_first_rinse_heat(r);
    show(r, PICO_RECIPE_FIRST_RINSE_STEP, 4, NULL);
    pico_actuator_steam_on(a);
    pico_actuator_shuttle_on(a);
    pico_actuator_drain_on(a);
    show(r, PICO_RECIPE_FLUSH, 0, NULL);
    uint32_t started = clock_ms(r), last_move = started;
    while (clock_ms(r) - started < 130000) {
        pico_local_poll();
        if (pico_local_cancelled())
            return;
        uint32_t now = clock_ms(r);
        if (pico_actuators_pause_for_missing_filter(a, true, false, true)) {
            show(r, PICO_RECIPE_FIRST_RINSE_STEP, 4, NULL);
            started = started - now + clock_ms(r);
            last_move = last_move - now + clock_ms(r);
        }
        if (c->thermoblock_fahrenheit < 225.0)
            pico_actuator_heater_on(a);
        else if (c->thermoblock_fahrenheit > 230.0)
            pico_actuator_heater_off(a);
        if (clock_ms(r) - last_move > 10000) {
            pico_actuator_heater_off(a);
            pico_actuator_steam_off(a);
            pico_actuator_shuttle_off(a);
            move_to(r, (uint8_t)next);
            pico_actuator_steam_on(a);
            pico_actuator_shuttle_on(a);
            last_move = clock_ms(r);
            if (++next > 6)
                next = 1;
            timer(r, 6, started, 130);
        }
        r->ops->temperature_summary(r->context, c->wort_fahrenheit, c->thermoblock_fahrenheit, 0.0,
                                    5);
        timer(r, 6, started, 130);
    }
    pico_actuator_heater_off(a);
    bool cool_ok = pico_recipe_first_rinse_cool(r, &next);
    if (!cool_ok) {
        pico_actuator_shuttle_off(a);
        pico_actuator_steam_off(a);
        pico_actuator_drain_off(a);
        pico_recipe_first_rinse_heat(r);
        cool_ok = pico_recipe_first_rinse_cool(r, &next);
    }
    show(r, PICO_RECIPE_ENDING_FIRST_RINSE, 0, NULL);
    pico_actuator_shuttle_off(a);
    pico_actuator_drain_off(a);
    pico_recipe_vacuum(r, (uint8_t)next, 1);
    if (!cool_ok) {
        show(r, PICO_RECIPE_FIRST_RINSE_STEP, 6, NULL);
        r->ops->report_error(r->context, 22);
        if (!heat_ok)
            r->ops->report_error(r->context, 19);
        show(r, PICO_RECIPE_WAIT_STOP, 0, NULL);
    } else if (heat_ok) {
        show(r, PICO_RECIPE_FIRST_RINSE_STEP, 8, NULL);
    }
    if (!heat_ok) {
        show(r, PICO_RECIPE_FIRST_RINSE_STEP, 7, NULL);
        r->ops->report_error(r->context, 19);
        show(r, PICO_RECIPE_WAIT_STOP, 0, NULL);
    }
    show(r, PICO_RECIPE_FLUSH, 0, NULL);
    r->ops->move_home(r->context);
    r->state->device_state = 2;
    r->ops->report_state(r->context);
    show(r, PICO_RECIPE_WAIT_STOP, 0, NULL);
}

/* Original 0x2000f800. */
void pico_recipe_rack_beer(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    PicoActuators *a = r->phase->actuators;
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_RACK_BEER_STEP, 0, NULL);
    pico_actuator_drain_on(a);
    show(r, PICO_RECIPE_RACK_BEER_STEP, 1, NULL);
    pico_actuator_drain_off(a);
    for (uint32_t stage = 2; stage != 6; ++stage)
        show(r, PICO_RECIPE_RACK_BEER_STEP, stage, NULL);
    pico_actuator_drain_on(a);
    show(r, PICO_RECIPE_RACK_BEER_STEP, 6, NULL);
    pico_actuator_drain_off(a);
    show(r, PICO_RECIPE_RACK_BEER_STEP, 7, NULL);
    pico_actuator_drain_on(a);
    show(r, PICO_RECIPE_RACK_BEER_STEP, 8, NULL);
    pico_actuator_drain_off(a);
}

/* Original 0x20011420. */
void pico_recipe_descale(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    PicoActuators *a = r->phase->actuators;
    PicoBrewController *c = &r->state->controller;
    prompt(r, PICO_RECIPE_DESCALE_STEP, 0);
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_DESCALE_HEADING, 0, NULL);
    while (!pico_local_cancelled() && !pico_filter_is_inserted(a))
        prompt(r, PICO_RECIPE_DESCALE_STEP, 1);
    prompt(r, PICO_RECIPE_DESCALE_STEP, 6);
    move_to(r, 1);
    pico_recipe_reprime_steam(r, "PRIME", "Descaling", false, false);
    pico_actuators_all_off(a);
    prompt(r, PICO_RECIPE_DESCALE_STEP, 2);
    for (unsigned cycle = 0; cycle != 2; ++cycle) {
        clock_ms(r);
        prompt(r, PICO_RECIPE_DESCALE_STEP, 3);
        for (;;) {
            pico_local_poll();
            if (pico_local_cancelled())
                return;
            pico_actuator_shuttle_on(a);
            if (!pico_filter_is_inserted(a)) {
                clock_ms(r);
                pico_actuator_shuttle_off(a);
                pico_actuator_heater_off(a);
                while (!pico_local_cancelled() && !pico_filter_is_inserted(a))
                    prompt(r, PICO_RECIPE_DESCALE_STEP, 1);
                prompt(r, PICO_RECIPE_DESCALE_STEP, 3);
                clock_ms(r);
            }
            temperature_columns(r);
            show(r, PICO_RECIPE_FLUSH, 0, NULL);
            if (c->thermoblock_fahrenheit > 130.0)
                pico_actuator_heater_off(a);
            else if (c->thermoblock_fahrenheit < 120.0)
                pico_actuator_heater_on(a);
            if (!r->ops->take_click(r->context))
                continue;
            clock_ms(r);
            pico_actuator_shuttle_off(a);
            pico_actuator_heater_off(a);
            if (prompt(r, PICO_RECIPE_DESCALE_STEP, 4))
                break;
            prompt(r, PICO_RECIPE_DESCALE_STEP, 3);
            clock_ms(r);
        }
        clock_ms(r);
        pico_actuators_all_off(a);
        clock_ms(r);
        if (cycle == 0)
            prompt(r, PICO_RECIPE_DESCALE_STEP, 5);
    }
    pico_recipe_restore_idle(r);
    pico_actuators_all_off(a);
    r->state->device_state = 2;
    r->ops->report_state(r->context);
}
