#include "picobrew/recipe.h"
#include "picobrew/local_control.h"
#if PICO_STANDALONE
#include "picobrew/local_history.h"
#endif
#include "picobrew/text.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "recipe_data.inc"

static uint32_t clock_ms(PicoRecipeRuntime *r)
{
    pico_local_poll();
    return r->phase->control->ops->millis(r->phase->control->context);
}
static void delay_ms(PicoRecipeRuntime *r, uint32_t ms)
{
#if PICO_STANDALONE
    (void)r;
    pico_local_control_delay(ms);
#else
    r->phase->actuators->ops->delay_milliseconds(r->phase->actuators->context, ms);
#endif
}
static void show(PicoRecipeRuntime *r, PicoRecipeScreen screen, uint32_t value, const char *text)
{
    r->ops->screen(r->context, screen, value, text);
}
static void log_message(PicoRecipeRuntime *r, const char *level, const char *module,
                        const char *message)
{
    r->ops->log(r->context, level, module, message);
}
static void move_to(PicoRecipeRuntime *r, uint32_t compartment)
{
    r->phase->ops->move_to_compartment(r->phase->context, compartment);
}
static void advance(PicoRecipeRuntime *r)
{
    r->ops->advance_carousel(r->context);
}
static int32_t signed_bits(uint32_t value)
{
    int32_t result;
    memcpy(&result, &value, sizeof result);
    return result;
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
static void save_recovery(PicoRecipeRuntime *r, uint32_t active, uint32_t drain, int32_t index,
                          bool force)
{
#if PICO_STANDALONE
    uint32_t phase = r->state->controller.phase;
#else
    uint32_t phase = 0;
#endif
    r->ops->save_recovery(r->context, active, drain, index, phase, force,
                          r->state->controller.shuttle_calibration_complete);
}
static void normal_pages(PicoRecipeRuntime *r)
{
    for (unsigned i = 0; i != 5; ++i)
        r->state->carousel.pages[i] = r->pages->normal[i];
    r->state->carousel.page_count = 5;
}

/* The firmware's delimiter routine has no destination-capacity argument.
 * Network and restart records now enter through this checked equivalent so a
 * missing separator or an oversized field cannot overwrite a local buffer or
 * the next recipe field. Successfully delimited, in-range fields are unchanged.
 * limit remains an inclusive delimiter position, matching the original ABI. */
static int bounded_field(const char *input, int32_t length, char *output, size_t capacity,
                         int32_t *cursor, char delimiter, int32_t limit)
{
    int32_t start = *cursor;
    if (start < 0 || start > length || capacity == 0)
        return 0;
    for (int32_t index = start; index < length && index <= limit; ++index) {
        size_t count = (size_t)(index - start);
        if (count >= capacity)
            return 0;
        if (input[index] == delimiter) {
            memcpy(output, input + start, count);
            output[count] = '\0';
            *cursor = index + 1;
            return 1;
        }
        if (input[index] == '\0' || count + 1 == capacity)
            return 0;
    }
    return 0;
}

/* Original 0x20008c90. The limit bounds the input cursor, not output size.
 * Valid callers provide enough output storage for the delimited field. */
int pico_recipe_delimited_field(const char *input, char *output, int32_t *cursor, char delimiter,
                                int32_t input_limit)
{
    int32_t out = 0;
    for (;;) {
        int32_t in = *cursor;
        if (input[in] == delimiter) {
            output[out] = '\0';
            *cursor = signed_bits((uint32_t)in + 1u);
            return 1;
        }
        if (input[in] == '\0')
            return 0;
        *cursor = signed_bits((uint32_t)in + 1u);
        output[out++] = input[in];
        if (input_limit < *cursor)
            return 0;
    }
}

/* Original 0x20008ccc. The original incomplete-field side effects are retained.
 * Wire input must honor original field/capacity contracts; this is not a new
 * untrusted-input parser or a silent bounds-behavior change. */
bool pico_recipe_parse_response(PicoRecipeRuntime *r, const char *response, int32_t length)
{
    if (response == NULL || length < 0 ||
        memchr(response, '\0', (size_t)length + 1) != response + length)
        return false;
    int32_t cursor = 0;
    char field[100];
    PicoRecipe *recipe = &r->state->recipe;
    uint32_t count = 0;
    r->ops->serial(r->context, response, true);
    snprintf(field, sizeof field, "%ld", (long)length);
    r->ops->serial(r->context, field, true);
    if (!bounded_field(response, length, field, sizeof field, &cursor, '/', length))
        return false;
    r->ops->set_recipe_name(r->context, field);
    if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length))
        return false;
    recipe->abv_adjustment_reference = atoi(field);
    if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length))
        return false;
    recipe->ibu_adjustment_reference = atoi(field);
    if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length))
        return false;
    recipe->decimal_metric = atof(field);
    if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length))
        return false;
    recipe->integer_metric = atoi(field);
    r->ops->serial(r->context, "steps", true);
    while (cursor < length && count != PICO_RECIPE_STEP_CAPACITY) {
        PicoRecipeStep *step = &recipe->steps[count];
        if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length)) {
            if (*r->offline == 0)
                return false;
            break;
        }
        step->target_fahrenheit = (uint8_t)atoi(field);
        if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length))
            return false;
        step->active_minutes = (uint32_t)atoi(field);
        if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length))
            return false;
        step->drain_minutes = (uint32_t)atoi(field);
        if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length))
            return false;
        step->location = (uint8_t)atoi(field);
        if (!bounded_field(response, length, field, sizeof field, &cursor, ',', length))
            return false;
        strncpy(step->name, field, 21);
        step->name[21] = '\0';
        ++count;
    }
    recipe->step_count = (uint8_t)count;
    return recipe->step_count != 0;
}

static void serial_number(PicoRecipeRuntime *r, int32_t number, bool newline)
{
    char text[24];
    snprintf(text, sizeof text, "%ld", (long)number);
    r->ops->serial(r->context, text, newline);
}
void pico_recipe_program_log(PicoRecipeRuntime *r, const PicoProgramContext *p, uint32_t language)
{
    char line[128];
    const char *fields[] = {"  title = ", "  name = ", "  state = ", "  logoText = "};
    const char *values[] = {p->title, p->name, p->state_label, p->logo_text};
    r->ops->serial(r->context, "Session: ", true);
    r->ops->serial(r->context, "  id = ", false);
    r->ops->serial(r->context, p->identifier, true);
    r->ops->serial(r->context, "  programId = ", false);
    r->ops->serial(r->context, program_names_debug[p->program_id], false);
    r->ops->serial(r->context, " (", false);
    serial_number(r, (int32_t)p->program_id, false);
    r->ops->serial(r->context, ")", true);
    r->ops->serial(r->context, "  temperature = ", false);
    serial_number(r, (int32_t)p->temperature, true);
    r->ops->serial(r->context, "  duration    = ", false);
    serial_number(r, (int32_t)p->duration, true);
    r->ops->serial(r->context, "  location    = ", false);
    serial_number(r, (int32_t)p->location, true);
    r->ops->serial(r->context, "  type = ", false);
    r->ops->serial(r->context, program_session_names[p->session_type], false);
    r->ops->serial(r->context, " (", false);
    serial_number(r, (int32_t)p->session_type, false);
    r->ops->serial(r->context, ")", true);
    for (unsigned i = 0; i != 4; ++i) {
        r->ops->serial(r->context, fields[i], false);
        r->ops->serial(r->context, values[i], true);
    }
    snprintf(line, sizeof line, "\"%s\" has %d actions", program_session_names[p->session_type],
             (int)p->action_count);
    r->ops->serial(r->context, line, true);
    for (uint32_t i = 0; i < p->action_count; ++i) {
        const char *label = r->ops->action_label(r->context, p->actions[i], language, 0);
        snprintf(line, sizeof line, "  - %d \"%s\"", (int)p->actions[i], label);
        r->ops->serial(r->context, line, true);
    }
}

void pico_recipe_program_initialize(PicoRecipeRuntime *r, PicoProgramContext *p, uint32_t id,
                                    uint32_t language)
{
    r->ops->assertion(r->context, language < 3, "language >= 0 && language <= 2");
    r->ops->assertion(r->context, id < 7, "_programId >= 0 && _programId < ProgramId::_EnumCount");
    if (language > 2)
        language = 0;
    if (id > 6)
        id = 1;
    p->identifier[0] = '\0';
    p->temperature = p->duration = p->location = p->session_type = 0;
    p->actions = NULL;
    p->action_count = 0;
    p->program_id = id;
    p->title = program_titles[id][language];
    p->name = program_names[id][language];
    p->state_label = program_states[id][language];
    p->logo_text = program_logos[id][language];
    p->interrupted_text = program_interrupted[id][language];
    p->bitmap_primary = r->assets->primary;
    p->bitmap_secondary = r->assets->secondary;
    switch (id) {
    case 1:
        break;
    case 2:
        p->session_type = 1;
        break;
    case 3:
        p->session_type = 2;
        p->temperature = 135;
        p->duration = 105;
        p->bitmap_secondary = r->assets->sousvide_secondary;
        break;
    case 4:
        p->session_type = 4;
        p->temperature = 120;
        p->duration = 90;
        break;
    case 5:
        p->session_type = 3;
        break;
    case 6:
        p->session_type = 5;
        p->temperature = 60;
        break;
    default:
        r->ops->serial(r->context, "ERROR unsuported programId", true);
        break;
    }
    switch (p->session_type) {
    case 0:
        p->actions = actions_brewing;
        p->action_count = 6;
        break;
    case 1:
        p->actions = actions_cleaning;
        p->action_count = 5;
        break;
    case 2:
        p->actions = actions_sousvide;
        p->action_count = 5;
        break;
    case 3:
        p->actions = actions_still;
        p->action_count = 5;
        break;
    case 4:
        p->actions = actions_coffee;
        p->action_count = 5;
        break;
    case 5:
        p->actions = actions_manual;
        p->action_count = 3;
        break;
    }
    pico_recipe_program_log(r, p, language);
}

void pico_recipe_initialize_cleaning(PicoRecipeRuntime *r, PicoRecipe *recipe, int32_t temperature)
{
#if PICO_STANDALONE
    PicoProgramContext *program = recipe->program;
    memset(recipe, 0, sizeof *recipe);
    recipe->program = program;
    snprintf(recipe->identifier, sizeof recipe->identifier, "clean-%d", (int)temperature);
#endif
    pico_recipe_program_initialize(r, recipe->program, 2, *r->language);
    r->ops->set_recipe_name(r->context, recipe->program->title);
    recipe->step_count = 6;
    for (uint32_t i = 0; i < recipe->step_count; ++i) {
        PicoRecipeStep *step = &recipe->steps[i];
        step->target_fahrenheit = (uint8_t)temperature;
        step->drain_minutes = 0;
        step->active_minutes = temperature == 0 ? 1 : 5;
        step->location = (uint8_t)(i + 1);
        r->ops->compartment_label(r->context, step->name, step->location);
    }
    recipe->steps[recipe->step_count - 1].drain_minutes = temperature == 0 ? 2 : 5;
#if PICO_STANDALONE
    (void)pico_local_recovery_select(recipe);
#endif
}

void pico_recipe_prepare_temperature(PicoRecipeRuntime *r, int32_t target, const char *title)
{
    if (pico_local_cancelled())
        return;
    PicoBrewController *s = &r->state->controller;
    PicoActuators *a = r->phase->actuators;
    clock_ms(r);
    uint32_t started = clock_ms(r);
    if (!pico_filter_is_inserted(a)) {
        pico_actuators_steam_and_drain_off(a);
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        a->ops->prompt_insert_filter(a->context);
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        show(r, PICO_RECIPE_TITLED_FRAME, 0, title);
        show(r, PICO_RECIPE_CHANGING_STEP, 0, NULL);
        show(r, PICO_RECIPE_FLUSH, 0, NULL);
    }
    pico_phase_start_circulation(r->phase);
    if (target == 0) {
        pico_phase_set(r->phase, PICO_BREW_PHASE_UNHEATED_CIRCULATION);
        return;
    }
    s->wort_fahrenheit =
        (double)r->phase->control->ops->read_wort_fahrenheit(r->phase->control->context);
    double target_double = (double)target;
    if (s->wort_fahrenheit <= target_double && s->thermoblock_fahrenheit < 210.0) {
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        show(r, PICO_RECIPE_TITLED_FRAME, 0, title);
        show(r, PICO_RECIPE_PREHEATING_STEAM, 3, NULL);
        show(r, PICO_RECIPE_FLUSH, 0, NULL);
        s->active_shuttle_scale = 0.1;
        for (;;) {
            pico_local_poll();
            if (pico_local_cancelled())
                return;
            pico_control_start(r->phase->control);
            for (;;) {
                pico_local_poll();
                if (pico_local_cancelled())
                    return;
                if (!(s->thermoblock_fahrenheit < 210.0) || !(s->wort_fahrenheit <= target_double))
                    goto select_phase;
                if (clock_ms(r) - started > 15000u && s->thermoblock_fahrenheit < 90.0) {
                    pico_control_stop(r->phase->control);
                    pico_actuators_off_except_drain(a);
                    r->ops->error(r->context, 19, true, true);
                    s->status = 3;
                    s->phase = PICO_BREW_PHASE_COOL_THERMOBLOCK;
                    return;
                }
                if (clock_ms(r) - started > 15000u && r->ops->take_click(r->context))
                    goto select_phase;
                if (!pico_filter_is_inserted(a))
                    break;
            }
            pico_control_stop(r->phase->control);
            pico_actuators_steam_and_drain_off(a);
            show(r, PICO_RECIPE_CLEAR, 0, NULL);
            a->ops->prompt_insert_filter(a->context);
            show(r, PICO_RECIPE_CLEAR, 0, NULL);
            pico_phase_start_circulation(r->phase);
            show(r, PICO_RECIPE_CLEAR, 0, NULL);
            show(r, PICO_RECIPE_TITLED_FRAME, 0, title);
            show(r, PICO_RECIPE_PREHEATING_STEAM, 3, NULL);
            show(r, PICO_RECIPE_FLUSH, 0, NULL);
        }
    }
    for (;;) {
        pico_local_poll();
        if (pico_local_cancelled())
            return;
        started = clock_ms(r);
        for (;;) {
            pico_local_poll();
            if (pico_local_cancelled())
                return;
            if (clock_ms(r) - started > 4999u)
                goto select_phase;
            if (!pico_filter_is_inserted(a))
                break;
        }
        pico_actuators_steam_and_drain_off(a);
        a->ops->prompt_insert_filter(a->context);
        pico_phase_start_circulation(r->phase);
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        show(r, PICO_RECIPE_CHANGING_STEP, 0, NULL);
        show(r, PICO_RECIPE_FLUSH, 0, NULL);
    }
select_phase:
    s->wort_fahrenheit =
        (double)r->phase->control->ops->read_wort_fahrenheit(r->phase->control->context);
    int phase;
    if (s->wort_fahrenheit > (double)signed_bits((uint32_t)target + 5u))
        phase = PICO_BREW_PHASE_COOLING;
    else if (s->wort_fahrenheit < target_double)
        phase = PICO_BREW_PHASE_HEATING;
    else
        phase = PICO_BREW_PHASE_HEATED_HOLD;
    pico_phase_set(r->phase, phase);
}

/* Original 0x20011590. The loop preserves counter wrapping, phase-entry order,
 * recovery writes and pause-duration compensation. Hardware interrupts supply
 * temperature/control progress while the foreground workflow is waiting. */
int pico_recipe_run_step(PicoRecipeRuntime *r, const PicoStepRequest *request)
{
    if (pico_local_cancelled())
        return 0;
#if PICO_STANDALONE
    /* Manual brewing carries its index in the request rather than the saved
     * recipe loop. Keep status/history aligned with the step actually run. */
    r->state->recipe.current_step = request->step_index;
#endif
    PicoBrewController *s = &r->state->controller;
    PicoActuators *a = r->phase->actuators;
    uint32_t target = request->target_fahrenheit;
    uint32_t active = request->active_minutes;
    uint32_t drain = request->drain_minutes;
    uint32_t session_type = request->session_type;
    uint32_t original_compartment = request->compartment;
    const char *title = request->program_title;
    const char *name = request->step_name;
    uint32_t now = 0, resumed, selected, started, last_log = 0;
#if !PICO_STANDALONE
    uint32_t last_still = 0;
#endif
    bool complete;
    char message[132];
    r->ops->populate_brew_menu(r->context);
    s->compartment = request->compartment;
    s->wort_target_fahrenheit = (double)target;
    s->step_seconds = signed_bits((uint32_t)request->remaining_minutes * 60u);
    snprintf(message, 128, "%s %dF %d min %d \"%s\"", "runstep", (int)target, (int)active,
             (int)original_compartment, name);
    log_message(r, "I", "BREW", message);
#if PICO_STANDALONE
    uint8_t recovered_phase = r->recovery_phase;
    r->recovery_phase = 0;
    if (recovered_phase == PICO_BREW_PHASE_DRAINING + 1) {
        move_to(r, original_compartment);
        pico_phase_set(r->phase, PICO_BREW_PHASE_DRAINING);
    } else
#else
    save_recovery(r, active, drain, request->step_index, true);
#endif
    if (s->compartment == 0 || s->compartment == 7) {
        s->compartment = s->compartment == 0 ? 2 : 1;
        move_to(r, s->compartment);
        pico_phase_set(r->phase, PICO_BREW_PHASE_COOL_THERMOBLOCK);
    } else if (active == UINT32_MAX && (session_type == 2 || session_type == 4)) {
        move_to(r, 1);
        s->compartment = 1;
        pico_phase_set(r->phase, PICO_BREW_PHASE_INDEFINITE_HEATING);
    } else {
        move_to(r, original_compartment);
        pico_recipe_prepare_temperature(r, signed_bits(target), title);
    }
    if (s->compartment == 1 && drain != 0 && session_type == 0
#if PICO_STANDALONE
        && !recovered_phase
#endif
        ) {
        float seconds = (float)signed_bits(drain * 60u);
        double extra = ceil(ceil((double)seconds / 45.0) * 0.5);
        drain = (uint32_t)trunc_i32((double)signed_bits(drain) + extra);
    }
#if PICO_STANDALONE
    save_recovery(r, active, drain, request->step_index, true);
#endif
    advance(r);
    started = clock_ms(r);
    int32_t total = signed_bits(active + drain);
    for (;;) {
        pico_local_poll();
        if (pico_local_cancelled())
            return 0;
        advance(r);
        now = clock_ms(r);
        if (now - last_log > 25000u) {
            if (request->logging_enabled != 0)
                r->ops->log_progress(r->context, name);
            last_log = clock_ms(r);
        }
#if !PICO_STANDALONE
        if (session_type == 3 && clock_ms(r) - last_still > 2000u) {
            bool received = r->ops->still_read_status(r->context, &s->still);
            uint8_t status = s->status;
            if (!received) {
                if (status != 8 && (int32_t)s->still.communication_failure_limit <=
                                       (int32_t)s->still.communication_failures) {
                    pico_control_stop(r->phase->control);
                    s->status = 8;
                }
            } else {
                if (status != 10) {
                    if (r->ops->still_pressure_limit(r->context, &s->still)) {
                        pico_control_stop(r->phase->control);
                        s->status = 10;
                        goto still_polled;
                    }
                    if (status == 9)
                        goto still_polled;
                }
                if (s->still.temperature2 > s->still.temperature2_limit) {
                    pico_control_stop(r->phase->control);
                    s->status = 9;
                }
            }
        still_polled:
            last_still = clock_ms(r);
        }
#endif
        if (s->thermoblock_target_fahrenheit != 300.0 && target > 179u)
            s->thermoblock_target_fahrenheit = 340.0;
        complete = false;
        switch (s->phase) {
        case PICO_BREW_PHASE_HEATING:
            pico_phase_update_shuttle_calibration(r->phase);
            /* fall through */
        case PICO_BREW_PHASE_COOLING: {
            double first_read = s->wort_fahrenheit;
            double second_read = s->wort_fahrenheit;
            bool reached = s->phase == PICO_BREW_PHASE_HEATING
                               ? second_read >= (double)target
                               : first_read <= (double)signed_bits(target + 5u);
            if (reached) {
                advance(r);
                if (signed_bits(active) < 1) {
                    if (signed_bits(drain) > 0)
                        pico_phase_set(r->phase, PICO_BREW_PHASE_DRAINING);
                    else
                        complete = true;
                } else
                    pico_phase_set(r->phase, PICO_BREW_PHASE_HEATED_HOLD);
                r->ops->clear_row(r->context, 4, 9);
                r->ops->clear_row(r->context, 5, 9);
                started = clock_ms(r);
#if PICO_STANDALONE
                save_recovery(r, active, drain, request->step_index, true);
#endif
            }
            if (s->step_seconds / 60 + 1 < total)
                s->step_seconds = signed_bits((uint32_t)total * 60u);
            break;
        }
        case PICO_BREW_PHASE_HEATED_HOLD:
        case PICO_BREW_PHASE_UNHEATED_CIRCULATION:
        case PICO_BREW_PHASE_UNHEATED_PUMP_HOLD: {
            int32_t remaining = s->step_seconds;
            now = clock_ms(r);
            if ((uint32_t)(remaining / 60) + 1u < (uint32_t)total - (now - started) / 60000u) {
                now = clock_ms(r);
                s->step_seconds = signed_bits(((uint32_t)total - (now - started) / 60000u) * 60u);
            }
            advance(r);
            now = clock_ms(r);
            save_recovery(r,
#if PICO_STANDALONE
                          (now - started) / 60000u >= active ? 0 :
#endif
                          active - (now - started) / 60000u, drain, request->step_index, false);
            now = clock_ms(r);
            if ((now - started) / 60000u >= active) {
                if (signed_bits(drain) > 0) {
                    pico_phase_set(r->phase, PICO_BREW_PHASE_DRAINING);
                    save_recovery(r, 0, drain, request->step_index, true);
                    started = clock_ms(r);
                } else
                    complete = true;
            }
            break;
        }
        case PICO_BREW_PHASE_DRAINING:
            if (original_compartment == 1 && session_type == 0) {
                uint32_t interval;
                switch (s->drain_subphase) {
                case 0:
                    interval = clock_ms(r) - s->drain_phase_started_ms;
                    if (interval > 2000u)
                        goto stop_drain_and_advance;
                    break;
                case 1:
                    interval = clock_ms(r) - s->drain_phase_started_ms;
                    if (interval > 30000u)
                        goto start_drain_and_advance;
                    break;
                case 2:
                case 4:
                    interval = clock_ms(r) - s->drain_phase_started_ms;
                    if (interval > 15000u)
                        goto stop_drain_and_advance;
                    break;
                case 3:
                    interval = clock_ms(r) - s->drain_phase_started_ms;
                    if (interval > 45000u)
                        goto start_drain_and_advance;
                    break;
                case 5:
                    if (clock_ms(r) - s->drain_phase_started_ms > 45000u) {
                        pico_actuator_drain_on(a);
                        s->drain_phase_started_ms = clock_ms(r);
                        s->drain_subphase++;
                    }
                    /* The original falls through to phase 6 even if still 5. */
                    /* fall through */
                case 6:
                    if (clock_ms(r) - s->drain_phase_started_ms > 15000u) {
                        pico_actuator_drain_off(a);
                        s->drain_phase_started_ms = clock_ms(r);
                        s->drain_subphase = 0;
                    }
                    break;
                default:
                    break;
                }
                goto drain_progress;
            stop_drain_and_advance:
                pico_actuator_drain_off(a);
                goto advance_drain;
            start_drain_and_advance:
                pico_actuator_drain_on(a);
            advance_drain:
                s->drain_phase_started_ms = clock_ms(r);
                s->drain_subphase++;
            }
        drain_progress:
            now = clock_ms(r);
            save_recovery(r, 0,
#if PICO_STANDALONE
                          (now - started) / 60000u >= drain ? 0 :
#endif
                          drain - (now - started) / 60000u, request->step_index, false);
            {
                int32_t remaining = s->step_seconds;
                now = clock_ms(r);
                if ((uint32_t)(remaining / 60) + 1u < drain - (now - started) / 60000u) {
                    now = clock_ms(r);
                    s->step_seconds = signed_bits((drain - (now - started) / 60000u) * 60u);
                }
            }
            advance(r);
            complete = drain <= (clock_ms(r) - started) / 60000u;
            break;
        case PICO_BREW_PHASE_COOL_THERMOBLOCK:
            if (s->thermoblock_fahrenheit < 210.9 || clock_ms(r) - started > 30000u)
                pico_phase_set(r->phase, PICO_BREW_PHASE_UNHEATED_PUMP_HOLD);
            advance(r);
            break;
        case PICO_BREW_PHASE_INDEFINITE_HEATING:
            selected = r->ops->populate_post_brew_menu(r->context);
            selected = r->ops->choose_menu(r->context, 7, selected, 0, 1, 1, 1, 20);
            if (selected != 1)
                goto cancelled;
            break;
        default:
            pico_recipe_prepare_temperature(r, signed_bits(target), title);
            break;
        }
        switch (s->status) {
        case 1:
            if (s->wort_fahrenheit < 215.0 &&
                (s->wort_fahrenheit < (double)signed_bits(target) || target == 0))
                goto clear_status;
            break;
        case 2:
            if (s->thermoblock_fahrenheit < 350.0)
                goto clear_status;
            break;
        case 3:
            advance(r);
            pico_phase_finish_step(r->phase, title);
            r->ops->error(r->context, 15, true, true);
            return 0;
        case 4:
            pico_phase_clear_status(r->phase);
            advance(r);
            break;
        case 5:
        case 6:
        case 7:
            now = clock_ms(r);
            if (s->status == 5 && s->reprime_attempted == 0) {
                s->reprime_attempted = 1;
                r->ops->log_step(r->context, "Re-Prime Steam Pump");
            retry_priming:
                if (!pico_recipe_reprime_steam(r, title, name, true, true)) {
                    r->ops->log_step(r->context, "Thermoblock Issue");
                    goto cancelled;
                }
                r->ops->log_step(r->context, name);
            resume_after_recovery:
                resumed = clock_ms(r);
                started = started - now + resumed;
                show(r, PICO_RECIPE_CLEAR, 0, NULL);
                advance(r);
                goto clear_status;
            }
            if (r->ops->prompt(r->context, PICO_RECIPE_CONFIRM_THERMAL_RECOVERY, s->status, NULL)) {
                if (s->status == 5) {
                    r->ops->log_step(r->context, "Re-Prime Steam Pump");
                    goto retry_priming;
                }
                goto resume_after_recovery;
            }
            goto abort_after_display;
        case 8:
            if ((int32_t)s->still.communication_failures <
                (int32_t)s->still.communication_failure_limit)
                goto clear_status;
            show(r, PICO_RECIPE_STILL_COMMUNICATION_FAILURE, 0, NULL);
            break;
        case 9:
            show(r, PICO_RECIPE_STILL_TEMPERATURE_WARNING, 0, NULL);
            goto clear_status;
        case 10:
            show(r, PICO_RECIPE_STILL_COMPLETION, 0, NULL);
        clear_status:
            pico_phase_clear_status(r->phase);
            break;
        default:
            break;
        }
        now = clock_ms(r);
        if (pico_recipe_handle_filter_removal(r)) {
        abort_after_display:
            advance(r);
            goto cancelled;
        }
        resumed = clock_ms(r);
        started = started - now + resumed;
        if (r->ops->take_click(r->context)) {
            r->ops->serial(r->context, "Encoder Press in Brew\n", false);
            selected = r->ops->populate_brew_menu(r->context);
            selected = r->ops->choose_menu(r->context, 7, selected, 0, 1, 0, 1, 60);
#if PICO_STANDALONE
            /* A remote stop cancels the blocking menu with UINT32_MAX. Do
             * not reinterpret that sentinel as a negative action index. */
            if (pico_local_cancelled() || selected >= r->state->session.action_count)
                return 0;
#endif
            const uint32_t *actions = r->state->session.actions;
            int32_t count = (int32_t)r->state->session.action_count;
            r->ops->assertion(r->context, actions != NULL, "actions != NULL");
            r->ops->assertion(r->context, signed_bits(selected) < count,
                              "0 <= index && index < actionsCount");
            if (actions != NULL && signed_bits(selected) < count) {
                switch (actions[(int16_t)selected]) {
                case 1:
                    r->ops->serial(r->context, "Selected 0: Continue", false);
                    break;
                case 2:
                    r->ops->assertion(r->context, session_type == 3,
                                      "sessionType == SessionTypes::Still");
                    if (session_type != 3)
                        goto drain_requested;
                    r->ops->serial(r->context, "Selected 1: Still Fan State", false);
                    show(r, PICO_RECIPE_CLEAR, 0, NULL);
                    show(r, PICO_RECIPE_STILL_FAN_MENU, 0, NULL);
                    break;
                case 3:
                    r->ops->serial(r->context, "Selected 1: Skip Step", false);
                    show(r, PICO_RECIPE_SKIP_CONFIRMATION, 0, NULL);
                    if (r->ops->prompt(r->context, PICO_RECIPE_CONFIRM_AT_ROW, 5, NULL))
                        goto completed;
                    break;
                case 4:
                    goto cancelled;
                case 5:
                    r->ops->serial(r->context, "Selected 3: Pause", false);
                    now = clock_ms(r);
                    pico_phase_pause_until_resume(r->phase);
                    resumed = clock_ms(r);
                    started = resumed + started - now;
                    last_log = 0;
                    break;
                case 6:
                    if (session_type != 3) {
                    drain_requested:
                        pico_control_stop(r->phase->control);
                        now = clock_ms(r);
                        pico_actuator_steam_off(a);
                        pico_recipe_drain_filter(r);
                        pico_phase_set(r->phase, s->phase);
                        if ((s->phase & 0xfdu) == 0) {
                            show(r, PICO_RECIPE_CLEAR, 0, NULL);
                            r->ops->refresh_page(r->context);
                            delay_ms(r, 5000);
                        }
                        resumed = clock_ms(r);
                        started = started - now + resumed;
                        last_log = 0;
                    } else
                        r->ops->serial(r->context, "Selected 2: BUG Drain -> Continue", false);
                    break;
                case 7:
                case 8:
                case 9:
                    r->ops->serial(r->context, "Selected 4: Change ProView Display", false);
                    r->ops->assertion(r->context, session_type != 1,
                                      "sessionType == SessionTypes::Brewing || sessionType != "
                                      "SessionTypes::Cleaning");
                    *r->detailed_status ^= 1u;
                    break;
                case 10:
                    selected = (uint32_t)trunc_i32(s->wort_target_fahrenheit);
                    selected = r->ops->temperature_input(r->context, 18, r->state->session.title,
                                                         selected);
                    target = selected & 255u;
                    s->wort_target_fahrenheit = (double)target;
                    break;
                case 11:
                    r->ops->serial(r->context, "Selected 5: Exit Session", false);
                    show(r, PICO_RECIPE_STOP_CONFIRMATION, 0, NULL);
                    if (r->ops->prompt(r->context, PICO_RECIPE_CONFIRM_AT_ROW, 5, NULL))
                        goto cancelled;
                    break;
                default:
                    break;
                }
            }
            advance(r);
        }
        if (complete)
            break;
    }
    advance(r);
completed:
    pico_phase_finish_step(r->phase, title);
    return 1;
cancelled:
    pico_phase_finish_step(r->phase, title);
    return 0;
}

bool pico_recipe_run(PicoRecipeRuntime *r, PicoRecipe *recipe, bool logging)
{
    if (pico_local_cancelled()) {
#if PICO_STANDALONE
        pico_history_finish(true);
#endif
        return false;
    }
    if (!pico_recipe_setup(r, logging, r->state->screen.recipe_name)) {
#if PICO_STANDALONE
        pico_history_finish(true);
#endif
        return false;
    }
    bool cancelled = false;
    for (int32_t index = recipe->current_step; index < recipe->step_count; ++index) {
        recipe->current_step = index;
        PicoRecipeStep *step = &recipe->steps[index];
        r->ops->set_step_name(r->context, step->location, step->name);
        if (step->location >= 9)
            continue;
        if (step->location == 8) {
            pico_actuators_all_off(r->phase->actuators);
            /* Original early return bypasses the common finalizer. */
            if (!pico_recipe_pause_instructions(r, recipe->program->program_id)) {
#if PICO_STANDALONE
                pico_history_finish(true);
#endif
                return false;
            }
            continue;
        }
        step->name[21] = '\0';
        char short_name[21];
        strncpy(short_name, step->name, 20);
        short_name[20] = '\0';
        show(r, PICO_RECIPE_CHANGING_STEP, 0, NULL);
        show(r, PICO_RECIPE_FLUSH, 0, NULL);
        if (logging)
            r->ops->log_step(r->context, step->name);
        show(r, PICO_RECIPE_SETTING_UP_STEP, 0, r->state->screen.recipe_name);
        show(r, PICO_RECIPE_FLUSH, 0, NULL);
        double initial_temperature = r->state->controller.wort_fahrenheit;
        int32_t remaining = 0;
        uint32_t session_type = recipe->program->session_type;
        for (int32_t next = index; next < recipe->step_count; ++next) {
            const PicoRecipeStep *future = &recipe->steps[next];
            remaining =
                signed_bits((uint32_t)remaining + future->drain_minutes + future->active_minutes);
            if (future->location == 2) {
                if (session_type == 0)
                    remaining = signed_bits((uint32_t)remaining + 8u);
                else if (session_type - 2u < 3u) {
                    double estimate = (double)future->target_fahrenheit - initial_temperature;
                    estimate = (estimate / 10.0) * 8.0;
                    remaining = trunc_i32((double)remaining + estimate);
                }
            } else if ((future->location == 3 || future->location == 1) && session_type == 0)
                remaining = signed_bits((uint32_t)remaining + 7u);
        }
        PicoStepRequest request = {step->target_fahrenheit,
                                   step->active_minutes,
                                   step->drain_minutes,
                                   step->location,
                                   r->state->screen.recipe_name,
                                   short_name,
                                   logging,
                                   remaining,
                                   index,
                                   session_type};
        if (!pico_recipe_run_step(r, &request)) {
            log_message(r, "W", "RCTL", "runStep() returned false");
            cancelled = true;
            break;
        }
    }
    if (logging)
        r->ops->log_step(r->context, cancelled ? "Brew Canceled" : "Brew Complete");
    recipe->current_step = 0;
    pico_recipe_session_end(r, logging);
#if PICO_STANDALONE
    pico_history_finish(cancelled || pico_local_cancelled());
#endif
    return !cancelled;
}

void pico_recipe_end_outputs(PicoRecipeRuntime *r, uint32_t duration, const char *title, bool drain,
                             bool shuttle)
{
    if (pico_local_cancelled())
        return;
    char message[128];
    snprintf(message, sizeof message, "endBrew_runFor %d drain = %s shuttle = %s", (int)duration,
             drain ? " ON" : "OFF", shuttle ? " ON" : "OFF");
    log_message(r, "I", "RCTL", message);
    uint32_t deadline = clock_ms(r) + duration;
    while (clock_ms(r) < deadline) {
        pico_local_poll();
        if (pico_local_cancelled())
            return;
        if (pico_actuators_pause_for_missing_filter(r->phase->actuators, false, drain, shuttle))
            show(r, PICO_RECIPE_ENDING_PROGRAM, 0, title);
    }
}

void pico_recipe_session_end(PicoRecipeRuntime *r, bool logging)
{
#if PICO_STANDALONE
    r->recovery_phase = 0;
#endif
    PicoBrewController *s = &r->state->controller;
    PicoActuators *a = r->phase->actuators;
    if (pico_local_cancelled())
        goto stopped;
    log_message(r, "I", "RCTL", "endBrewSession() BEGIN");
    pico_actuator_heater_off(a);
    pico_actuator_shuttle_off(a);
    pico_actuator_steam_off(a);
    r->ops->delete_recovery(r->context);
    if (r->state->session.session_type == 3)
        r->ops->still_stop(r->context);
    show(r, PICO_RECIPE_ENDING_PROGRAM, 0, r->active_title);
    move_to(r, 6);
    uint32_t start = clock_ms(r);
    pico_actuator_shuttle_on(a);
    show(r, PICO_RECIPE_ENDING_PROGRAM, 0, r->active_title);
    if (s->thermoblock_fahrenheit < 250.0) {
        pico_actuator_heater_on(a);
        while (s->thermoblock_fahrenheit < 250.0 && clock_ms(r) < start + 45000u) {
            if (pico_local_cancelled())
                goto stopped;
        }
        pico_actuator_heater_off(a);
    }
    pico_recipe_end_outputs(r, 3000, r->active_title, true, true);
    start = clock_ms(r);
    log_message(r, "I", "RCTL", "!fullRotation || steamTemp <= 220");
    bool rotation = false;
    uint32_t compartment = 1;
    for (;;) {
        if (clock_ms(r) >= start + 45000u)
            break;
        if (pico_local_cancelled())
            goto stopped;
        move_to(r, compartment);
        pico_actuator_shuttle_on(a);
        compartment = (compartment + 1u) & 255u;
        pico_recipe_end_outputs(r, 1000, r->active_title, true, true);
        if (compartment >= 6) {
            compartment = 1;
            rotation = true;
        } else if (!rotation)
            continue;
        if (!(s->thermoblock_fahrenheit > 220.0))
            break;
    }
    pico_actuator_shuttle_off(a);
    log_message(r, "I", "RCTL", "drain only for 5s");
    pico_recipe_end_outputs(r, 5000, r->active_title, true, false);
    pico_actuator_drain_off(a);
    if (s->shuttle_calibration_complete == 0) {
        s->candidate_shuttle_scale = 0.2;
        r->ops->save_shuttle_scale(r->context, s->candidate_shuttle_scale);
    }
    s->thermoblock_target_fahrenheit = 300.0;
    s->phase = PICO_BREW_PHASE_IDLE;
    s->shuttle_calibration_complete = 0;
    s->status = 0;
    r->ops->load_shuttle_scale(r->context);
    normal_pages(r);
    s->history_count = 0;
    r->state->device_state = 2;
    if (logging)
        r->ops->report_state(r->context);
    r->ops->move_home(r->context);
    log_message(r, "I", "RCTL", "endBrewSession() END");
    return;
stopped:
    pico_control_stop(r->phase->control);
    pico_actuators_all_off(a);
    r->ops->delete_recovery(r->context);
    s->phase = PICO_BREW_PHASE_IDLE;
    s->status = 0;
    r->state->device_state = 2;
}

bool pico_recipe_session_start(PicoRecipeRuntime *r, bool server_session)
{
    if (pico_local_cancelled())
        return false;
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_LOADING, 0, NULL);
    bool succeeded =
        !server_session || r->ops->get_session_id(r->context, r->state->session.identifier, 32);
    show(r, PICO_RECIPE_STOP_PROGRESS, 0, NULL);
#if PICO_STANDALONE
    /* Both physical and REST starts pass this boundary. Maintenance/cooking
     * sessions do not consume the retained brew-history budget. A storage
     * failure is reported by history without changing appliance control. */
    if (succeeded && (r->state->session.session_type == 0 ||
                      r->state->session.session_type == 5))
        (void)pico_history_start(r->state->session.session_type == 5 ? "manual" : "brew");
#endif
    return succeeded;
}

bool pico_recipe_setup(PicoRecipeRuntime *r, bool logging, const char *title)
{
    if (pico_local_cancelled())
        return false;
    if (*r->offline != 0) {
        r->ops->serial(r->context, "setupInstructions without logging (g_offlineMode)", true);
        logging = false;
    }
    r->active_title = title;
    show(r, PICO_RECIPE_STARTING_PROGRAM, 0, title);
    show(r, PICO_RECIPE_FLUSH, 0, NULL);
    if (logging)
        r->ops->report_state(r->context);
    if (r->ops->prompt(r->context, PICO_RECIPE_SETUP_CHOICE, 0, title)) {
        if (!pico_filter_is_inserted(r->phase->actuators)) {
            show(r, PICO_RECIPE_PAUSED, 0, NULL);
            r->ops->wait_for_filter(r->context);
        }
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
#if PICO_STANDALONE
        /* Drain recovery still requires setup/filter confirmation, but steam
         * priming would unnecessarily heat a batch that has finished its hold. */
        if (r->recovery_phase == PICO_BREW_PHASE_DRAINING + 1)
            return true;
#endif
        if (pico_recipe_prime_steam(r, title))
            return true;
        if (logging)
            r->ops->log_step(r->context, "Thermoblock Issue");
    }
    pico_recipe_session_end(r, logging);
    return false;
}

void pico_recipe_prompt_parameters(PicoRecipeRuntime *r)
{
    PicoProgramContext *p = &r->state->session;
    p->temperature = r->ops->temperature_input(r->context, 18, p->title, p->temperature);
    p->duration = r->ops->time_input(r->context, 20, p->title, p->duration);
}

bool pico_recipe_pause_instructions(PicoRecipeRuntime *r, uint32_t program)
{
    if (pico_local_cancelled())
        return false;
    PicoProgramContext *p = &r->state->session;
    char message[132];
    r->ops->assertion(
        r->context, p->session_type == 2 || p->session_type == 4,
        "session.type == SessionTypes::SousVide || session.type == SessionTypes::Coffee");
    r->ops->assertion(r->context, p->program_id == program, "session.programId == programId");
    snprintf(message, 128, "%s %s", "executePauseCustomInstructions", program_names_debug[program]);
    log_message(r, "I", "SVIDE", message);
    uint32_t result = 0;
    if (p->program_id == 4) {
        result = pico_recipe_instruction(r, 14, 1);
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
    } else if (p->program_id == 3) {
        result = pico_recipe_instruction(r, 9, 1);
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        if (result == 23) {
            result = pico_recipe_instruction(r, 10, 0);
            show(r, PICO_RECIPE_CLEAR, 0, NULL);
        }
    }
    r->ops->clear_click(r->context);
    return result != 34;
}

bool pico_recipe_session_restore(PicoRecipeRuntime *r, PicoRecipe *recipe)
{
#if PICO_STANDALONE
    PicoLocalRecovery record;
    r->recovery_phase = 0;
    if (!r->ops->read_local_recovery || !r->ops->read_local_recovery(r->context, &record))
        return false;
    if (record.session_type == 0) {
        if (r->ops->fetch_recipe(r->context, record.recipe_id, -1, -1)) return false;
    } else if (record.session_type == 1 && !strcmp(record.recipe_id, "clean-135")) {
        pico_recipe_initialize_cleaning(r, recipe, 135);
    } else if (record.session_type == 1 && !strcmp(record.recipe_id, "clean-0")) {
        pico_recipe_initialize_cleaning(r, recipe, 0);
    } else return false;
    /* A saved checkpoint must match the complete original program before any
     * remaining durations are applied. Merely offering recovery starts no
     * session, changes no checkpoint, and energizes no output. */
    if (record.step >= recipe->step_count ||
        pico_local_recovery_select(recipe) != record.recipe_signature) return false;
    PicoRecipeStep *step = &recipe->steps[record.step];
    uint32_t max_drain = step->drain_minutes;
    if (step->location == 1 && record.session_type == 0)
        max_drain += (uint32_t)ceil(ceil((double)(max_drain * 60u) / 45.0) * 0.5);
    if (record.active_minutes > step->active_minutes || record.drain_minutes > max_drain ||
        (record.phase == PICO_BREW_PHASE_DRAINING && record.active_minutes)) return false;
    recipe->current_step = record.step;
    step->active_minutes = record.active_minutes;
    step->drain_minutes = record.drain_minutes;
    r->state->controller.shuttle_calibration_complete = record.calibrated;
    pico_recipe_program_initialize(r, recipe->program, record.session_type == 0 ? 1 : 2, *r->language);
    r->recovery_phase = record.phase + 1;
    return true;
#else
    char record[101] = {0}, field[12];
    int32_t cursor = 0;
    log_message(r, "I", "RCTL", "parseWasBrewing");
    if (!r->ops->read_recovery(r->context, record))
        return false;
    const char *end = memchr(record, '\0', sizeof record);
    if (end == NULL)
        return false;
    int32_t length = (int32_t)(end - record);
    if (!bounded_field(record, length, field, sizeof field, &cursor, ':', 10))
        return false;
    uint32_t type = (uint32_t)atoi(field);
    r->ops->assertion(r->context, type < 6, "isSessionTypeValid(sessionType)");
    if (type >= 6)
        r->ops->serial(r->context, "Invalid session type argument", true);
    if (type > 1) {
        log_message(r, "W", "RCTL", "Cannot restore this session type");
        return false;
    }
    if (!bounded_field(record, length, recipe->identifier, sizeof recipe->identifier, &cursor, ':',
                       cursor + 21))
        return false;
    if (!bounded_field(record, length, field, sizeof field, &cursor, ':', cursor + 10))
        return false;
    recipe->abv_adjustment = atoi(field);
    if (!bounded_field(record, length, field, sizeof field, &cursor, ':', cursor + 10))
        return false;
    recipe->ibu_adjustment = atoi(field);
    if (type == 0) {
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        show(r, PICO_RECIPE_LOADING_PACK, 0, NULL);
        int error = r->ops->fetch_recipe(r->context, recipe->identifier, recipe->ibu_adjustment,
                                         recipe->abv_adjustment);
        show(r, PICO_RECIPE_STOP_PROGRESS, 0, NULL);
        if (error != 0)
            return false;
    } else
        pico_recipe_initialize_cleaning(r, &r->state->recipe, 135);
    if (!bounded_field(record, length, field, sizeof field, &cursor, ':', cursor + 10))
        return false;
    recipe->current_step = atoi(field);
    if (recipe->current_step < 0 || recipe->current_step >= recipe->step_count)
        return false;
    PicoRecipeStep *step = &recipe->steps[recipe->current_step];
    if (!bounded_field(record, length, field, sizeof field, &cursor, ':', cursor + 10))
        return false;
    step->active_minutes = (uint32_t)atoi(field);
    if (!bounded_field(record, length, field, sizeof field, &cursor, ':', cursor + 10))
        return false;
    step->drain_minutes = (uint32_t)atoi(field);
    if (!bounded_field(record, length, field, sizeof field, &cursor, ':', cursor + 10))
        return false;
    if (atoi(field) != 0)
        step->target_fahrenheit = 0;
    if (!bounded_field(record, length, field, sizeof field, &cursor, ':', cursor + 10))
        return false;
    if (atoi(field) != 0)
        r->state->controller.shuttle_calibration_complete = 1;
    pico_recipe_program_initialize(r, recipe->program, type == 0 ? 1 : 2, *r->language);
    pico_text_copy(recipe->program->identifier, sizeof recipe->program->identifier,
                   recipe->identifier, r->ops->log, r->context);
    return true;
#endif
}

void pico_recipe_post_session(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    log_message(r, "I", "RCTL", "runPostSessionInstructions() BEGIN");
    PicoProgramContext *p = &r->state->session;
    if (p->session_type == 2 || p->session_type == 4) {
        uint32_t action = pico_recipe_instruction(r, 11, 1);
        if (action == 28) {
            if (pico_filter_is_inserted(r->phase->actuators)) {
                r->state->carousel.pages[0] = r->pages->holding;
                r->state->carousel.page_count = 1;
                advance(r);
                PicoStepRequest request = {(uint8_t)p->temperature,
                                           UINT32_MAX,
                                           0,
                                           1,
                                           r->state->screen.recipe_name,
                                           "Holding",
                                           1,
                                           (int32_t)p->duration,
                                           2,
                                           p->session_type};
                pico_recipe_run_step(r, &request);
                pico_recipe_session_end(r, true);
            }
        } else if (action == 29)
            r->ops->run_drain_instructions(r->context, 15);
    }
    log_message(r, "I", "RCTL", "runPostSessionInstructions() END");
}

bool pico_recipe_run_control_program(PicoRecipeRuntime *r, uint32_t device_state)
{
    if (pico_local_cancelled())
        return false;
    log_message(r, "I", "RCTL", "runProgram()");
    if (!pico_recipe_session_start(r, true))
        return false;
    r->state->device_state = (uint8_t)device_state;
    log_message(r, "I", "RCTL", "runControlProgram() BEGIN");
    bool succeeded = pico_recipe_run(r, &r->state->recipe, true);
    log_message(r, "I", "RCTL", "runControlProgram() END");
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    r->state->device_state = 2;
    r->ops->report_state(r->context);
    pico_recipe_post_session(r);
    if (r->state->session.session_type - 2u < 2u) {
        r->ops->assertion(r->context, device_state == 4 || device_state == 8,
                          "locState == PICO_SOUS_VIDE || locState == PICO_DISTILL");
        pico_recipe_vacuum(r, *r->legacy_compartment, *r->legacy_compartment);
    }
    if (r->state->session.session_type == 1) {
        r->ops->assertion(r->context, device_state == 7 || device_state == 4,
                          "locState == PICO_CLEANING || locState == PICO_SOUS_VIDE");
        show(r, PICO_RECIPE_RINSE_FINISHED, 0, NULL);
        pico_recipe_rinse(r);
    }
    r->ops->serial(r->context, "runProgram(): DONE", true);
    return succeeded;
}

void pico_recipe_run_deep_clean(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    if (!r->ops->prompt(r->context, PICO_RECIPE_CONFIRM_CLEANING, 0, NULL))
        return;
    pico_recipe_initialize_cleaning(r, &r->state->recipe, 135);
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_DEEP_CLEAN_SETUP, 0, NULL);
    r->state->carousel.pages[0] = r->pages->normal[0];
    r->state->carousel.pages[1] = r->pages->normal[1];
    r->state->carousel.page_count = 2;
    pico_recipe_run_control_program(r, 7);
}
void pico_recipe_run_required_maintenance(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    uint32_t actions[11];
    int count = r->ops->required_actions(r->context, actions, 10);
    for (int i = 0; i < count; ++i) {
        if (actions[i] == 7)
            pico_recipe_run_deep_clean(r);
        else if (actions[i] == 9)
            show(r, PICO_RECIPE_START_DESCALE, 0, NULL);
    }
}

void pico_recipe_run_brew_program(PicoRecipeRuntime *r, PicoRecipe *recipe)
{
    if (pico_local_cancelled())
        return;
    recipe->current_step = 0;
    pico_recipe_program_initialize(r, recipe->program, 1, *r->language);
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_LOGO, 0, NULL);
    normal_pages(r);
    recipe->abv_adjustment = recipe->ibu_adjustment = -1;
#if PICO_STANDALONE
    /* Local recipes have explicit IDs; no cloud PicoPak ownership lookup. */
    bool read = false;
#else
    bool read = r->ops->read_picopak(r->context, recipe->identifier, 21);
#endif
    if (!read || r->ops->fetch_default_recipe_failed(r->context, recipe->identifier)) {
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        if (!r->ops->prompt(r->context, PICO_RECIPE_SELECT_RECIPE, 0, NULL))
            return;
    }
#if PICO_STANDALONE
    if (!pico_recipe_session_start(r, true))
        return;
#else
    pico_text_copy(recipe->program->identifier, sizeof recipe->program->identifier,
                   recipe->identifier, r->ops->log, r->context);
#endif
    if (!r->ops->prompt(r->context, PICO_RECIPE_ADJUST_RECIPE, 0, NULL)) {
#if PICO_STANDALONE
        pico_history_finish(true);
#endif
        return;
    }
    r->state->device_state = 3;
    if (!pico_recipe_run(r, recipe, true))
        show(r, PICO_RECIPE_REQUIRE_FIRST_RINSE, 0, NULL);
    else {
        show(r, PICO_RECIPE_BREW_COMPLETE, 0, r->state->screen.recipe_name);
        show(r, PICO_RECIPE_WAIT_CLICK, 0, NULL);
    }
    pico_recipe_rinse(r);
}
