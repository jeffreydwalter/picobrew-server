#include "picobrew/recipe.h"
#include "picobrew/local_control.h"
#if PICO_STANDALONE
#include "picobrew/local_history.h"
#endif
#include <stdio.h>
#include <string.h>

static void show(PicoRecipeRuntime *r, PicoRecipeScreen screen, uint32_t argument, const char *text)
{
    r->ops->screen(r->context, screen, argument, text);
}
static void log_message(PicoRecipeRuntime *r, const char *level, const char *message)
{
    r->ops->log(r->context, level, "RCTL", message);
}
static void assert_condition(PicoRecipeRuntime *r, bool condition, const char *expression)
{
    r->ops->assertion(r->context, condition, expression);
}

/* Original 0x20005fdc. Only the six LocationId values are a valid caller
 * contract. The original assertion is nonfatal; it does not clamp the ID. */
uint32_t pico_recipe_location_value(PicoRecipeRuntime *r, uint32_t location)
{
    static const uint32_t compartments[] = {2, 1, 3, 4, 5, 6};
    assert_condition(r, location < 6, "LocationId::isValid(locationId)");
    return compartments[location];
}

/* Original 0x20006128. */
uint32_t pico_recipe_instruction(PicoRecipeRuntime *r, uint32_t instruction, int option)
{
    if (pico_local_cancelled())
        return 34;
    return r->ops->instruction_screen(r->context, instruction, option, r->state->session.program_id,
                                      r->state->session.title, true);
}

/* Original 0x20012020. */
void pico_recipe_manual_program(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    static const uint32_t setup_screens[] = {16, 13, 7, 1};
    static const uint32_t cleanup_screens[] = {2, 3, 1};
    static const uint32_t actions[] = {19, 21, 27, 35};
    PicoProgramContext *p = &r->state->session;
    log_message(r, "I", "manualBrewProgram");
    pico_recipe_program_initialize(r, p, 6, *r->language);
    assert_condition(r, p->session_type == 5, "session.type == SessionTypes::ManualBrew");
    p->location = 1;
#if PICO_STANDALONE
    r->state->recipe.current_step = 0;
#endif
    r->ops->set_recipe_name(r->context, p->title);
    const char *title = p->title;
    if (r->ops->instruction_sequence(r->context, setup_screens, 4, 0, p->program_id, p->title,
                                     true) != 23) {
        log_message(r, "W", "User aborted");
        return;
    }
    if (!pico_recipe_session_start(r, true)) {
        log_message(r, "W", "startSession() failed");
        return;
    }
    if (!pico_recipe_setup(r, true, title)) {
        log_message(r, "W", "setupInstructions() failed");
#if PICO_STANDALONE
        pico_history_finish(true);
#endif
        return;
    }
    for (unsigned i = 0; i != 3; ++i)
        r->state->carousel.pages[i] = r->pages->manual[i];
    r->state->carousel.page_count = 3;
    int32_t selected = 0, index = 0;
#if PICO_STANDALONE
    bool manual_interrupted = false;
#endif
    for (;;) {
        pico_local_poll();
        if (pico_local_cancelled()) {
#if PICO_STANDALONE
            pico_history_finish(true);
#endif
            return;
        }
        r->ops->log_step(r->context, "Waiting Instructions");
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        for (unsigned i = 0; i != 4; ++i)
            assert_condition(r, true, "0 <= actionId && actionId < ActionId::_EnumCount");
        show(r, PICO_RECIPE_INSTRUCTION, 19, title);
        r->ops->populate_actions(r->context, actions, 4);
        uint32_t action = r->ops->select_action(r->context, 7, selected);
        if (action == 19) {
            p->temperature = r->ops->temperature_input(r->context, 19, title, p->temperature);
            selected = 1;
        } else if (action == 21) {
            p->duration = r->ops->time_input(r->context, 21, title, p->duration);
            selected = 2;
        } else if (action == 27) {
            char name[64];
            snprintf(name, sizeof name, "Step %ld", (long)(index + 1));
            uint32_t compartment = pico_recipe_location_value(r, p->location);
            r->ops->set_step_name(r->context, compartment, name);
            show(r, PICO_RECIPE_SETTING_UP_STEP, 0, p->title);
            show(r, PICO_RECIPE_FLUSH, 0, NULL);
            r->ops->log_step(r->context, name);
            PicoStepRequest request = {p->temperature & 255u,
                                       p->duration,
                                       0,
                                       (uint8_t)compartment,
                                       r->state->screen.recipe_name,
                                       name,
                                       1,
                                       (int32_t)p->duration,
                                       index,
                                       p->session_type};
            if (!pico_recipe_run_step(r, &request))
                log_message(r, "W", "runStep() returned false");
            else {
                show(r, PICO_RECIPE_INSTRUCTION, 20, title);
                r->ops->alarm_until_press_or_timeout(r->context, 60000);
            }
            selected = 0;
            ++index;
        } else if (action == 35)
            break;
        else {
            log_message(r, "W", "Unexpected action in manualBrewProgram");
            if (action == 34) {
#if PICO_STANDALONE
                manual_interrupted = true;
#endif
                break;
            }
        }
    }
    pico_recipe_session_end(r, true);
#if PICO_STANDALONE
    pico_history_finish(manual_interrupted || pico_local_cancelled());
#endif
    if (r->ops->run_drain_instructions(r->context, 12) != 23) {
        log_message(r, "W", "User aborted drain");
        return;
    }
    if (r->ops->instruction_sequence(r->context, cleanup_screens, 3, 0, p->program_id, p->title,
                                     true) != 23) {
        log_message(r, "W", "User aborted");
        return;
    }
    pico_recipe_circulate(r);
}

/* Original 0x20012658. Despite its historical name, the machine code runs
 * the six-compartment heated cleaning program, not the circulation utility. */
void pico_recipe_circulation_program(PicoRecipeRuntime *r)
{
    if (pico_local_cancelled())
        return;
    pico_recipe_initialize_cleaning(r, &r->state->recipe, 135);
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    show(r, PICO_RECIPE_DEEP_CLEAN_SETUP, 0, NULL);
    r->state->carousel.pages[0] = r->pages->normal[0];
    r->state->carousel.pages[1] = r->pages->normal[1];
    r->state->carousel.page_count = 2;
    pico_recipe_run_control_program(r, 7);
}

/* Original 0x20012730. The pause record retains its untouched fields, as in
 * the original; location 8 dispatches instructions without using those fields. */
void pico_recipe_run_cooking_program(PicoRecipeRuntime *r, uint32_t program)
{
    if (pico_local_cancelled())
        return;
    static const uint32_t sousvide_instructions[] = {0, 8};
    static const uint32_t coffee_instructions[] = {5, 1, 13};
    PicoProgramContext *p = &r->state->session;
    PicoRecipe *recipe = &r->state->recipe;
    pico_recipe_program_initialize(r, p, program, *r->language);
    assert_condition(
        r, p->session_type == 2 || p->session_type == 4,
        "session.type == SessionTypes::SousVide || session.type == SessionTypes::Coffee");
    const uint32_t *instructions;
    unsigned count;
    if (program == 3) {
        instructions = sousvide_instructions;
        count = 2;
    } else if (program == 4) {
        instructions = coffee_instructions;
        count = 3;
    } else {
        r->ops->log(r->context, "E", "MENU", "Unsupported program");
        return;
    }
    pico_recipe_prompt_parameters(r);
    unsigned index = 0;
    for (;;) {
        pico_local_poll();
        if (pico_local_cancelled())
            return;
        uint32_t action = pico_recipe_instruction(r, instructions[index], 0);
        if (action == 23) {
            if (++index == count) {
                r->ops->log(r->context, "W", "MENU", "Falling through ActionId::StartCooking");
                break;
            }
        } else if (action == 25)
            pico_recipe_prompt_parameters(r);
        else if (action == 26)
            break;
        else if (action == 34)
            return;
    }
    PicoProgramContext *active = recipe->program;
    assert_condition(r, active->session_type == (program == 3 ? 2u : 4u),
                     program == 3 ? "session.type == SessionTypes::SousVide"
                                  : "session.type == SessionTypes::Coffee");
    assert_condition(r, active->program_id == program,
                     program == 3 ? "session.programId == ProgramId::SousVideDefault"
                                  : "session.programId == ProgramId::CoffeeColdBrew");
    r->ops->serial(r->context, "Selected session parameters", true);
    pico_recipe_program_log(r, active, *r->language);
    uint8_t target = (uint8_t)active->temperature;
    uint32_t duration = active->duration;
    r->ops->set_recipe_name(r->context, active->title);
    recipe->steps[0].drain_minutes = 0;
    recipe->steps[0].active_minutes = program == 3 ? 1 : 0;
    recipe->step_count = 0;
    recipe->steps[0].target_fahrenheit = target;
    strcpy(recipe->steps[0].name, "Heating");
    recipe->steps[0].location = program == 3 ? 2 : 1;
    uint8_t maintain = program == 3 ? 2 : 1;
    if (program == 3)
        recipe->steps[1].location = 8;
    recipe->step_count = maintain;
    PicoRecipeStep *step = &recipe->steps[maintain];
    step->target_fahrenheit = target;
    step->drain_minutes = 0;
    step->active_minutes = duration;
    strcpy(step->name, "Maintain");
    ++recipe->step_count;
    step->location = 1;
    show(r, PICO_RECIPE_WAIT_FILTER_INSERTION, 0, NULL);
    r->state->carousel.pages[0] = r->pages->cooking[0];
    r->state->carousel.pages[1] = r->pages->cooking[1];
    r->state->carousel.page_count = 2;
    pico_recipe_run_control_program(r, 4);
}

/* Original 0x20012568 (shared noncontiguous machine-code tails). All still
 * network decisions are real fallible services; this owns the recipe/mode flow. */
uint32_t pico_recipe_still_program(PicoRecipeRuntime *r)
{
#if PICO_STANDALONE
    log_message(r, "W", "PicoStill is unavailable in the standalone profile");
    return 34;
#else
    if (pico_local_cancelled())
        return 34;
    r->ops->still_initialize(r->context);
    assert_condition(r, !r->ops->still_has_home_ip(r->context), "!still.hasHomeIP()");
    uint32_t choice =
        (uint32_t)r->ops->prompt(r->context, PICO_RECIPE_CHOOSE_STILL_PROGRAM, 0, NULL);
    uint8_t target;
    if (choice == 0)
        target = (uint8_t)r->ops->temperature_input(r->context, 18, r->state->session.title, 135);
    else if (choice == 1)
        target = 180;
    else
        return choice;
    choice = (uint32_t)r->ops->prompt(r->context, PICO_RECIPE_WALK_STILL_STEPS, 0, NULL);
    if (choice)
        return choice;
    const char *failure;
    if (!r->ops->still_choose_access_point(r->context))
        failure = "No Still found";
    else if (!r->ops->still_permission(r->context, "canUsePicoStill"))
        failure = "Cannot Use Still";
    else {
        if (!r->ops->still_permission(r->context, "hasCleanedAck")) {
            show(r, PICO_RECIPE_CONFIRM_STILL_CLEANED, 0, NULL);
            r->ops->still_permission(r->context, "setCleanedAck");
        }
        if (r->ops->still_configure(r->context)) {
            show(r, PICO_RECIPE_STOP_PROGRESS, 0, NULL);
            show(r, PICO_RECIPE_CLEAR, 0, NULL);
            r->ops->center_text(r->context, 2, "Starting Still");
            PicoControlSignals *signals = &r->state->control_signals;
            signals->progress_row = 3;
            signals->progress_start_column = 5;
            signals->progress_end_column = 16;
            signals->progress_glyph_index = 0;
            signals->progress_active = 1;
            if (r->ops->still_start_once(r->context)) {
                PicoRecipe *recipe = &r->state->recipe;
                pico_recipe_program_initialize(r, recipe->program, 5, *r->language);
                r->ops->set_recipe_name(r->context, recipe->program->title);
                recipe->steps[0].drain_minutes = 0;
                recipe->steps[0].active_minutes = 0x7fffffffu;
                recipe->step_count = 0;
                recipe->steps[0].target_fahrenheit = target;
                strcpy(recipe->steps[0].name, "Heating");
                recipe->steps[0].location = 2;
                recipe->step_count = 1;
                /* This name is written beyond step_count in the original too. */
                strcpy(recipe->steps[1].name, "Maintain");
                show(r, PICO_RECIPE_STOP_PROGRESS, 0, NULL);
                show(r, PICO_RECIPE_CLEAR, 0, NULL);
                for (unsigned i = 0; i != 3; ++i)
                    r->state->carousel.pages[i] = r->pages->still;
                r->state->carousel.page_count = 3;
                log_message(r, "I", "runProgram()");
                bool result = false;
                if (pico_recipe_session_start(r, false)) {
                    r->state->device_state = 8;
                    log_message(r, "I", "runControlProgram() BEGIN");
                    result = pico_recipe_run(r, recipe, false);
                    log_message(r, "I", "runControlProgram() END");
                    show(r, PICO_RECIPE_CLEAR, 0, NULL);
                    r->state->device_state = 2;
                    pico_recipe_post_session(r);
                    if (r->state->session.session_type - 2u < 2u) {
                        assert_condition(r, true,
                                         "locState == PICO_SOUS_VIDE || locState == PICO_DISTILL");
                        pico_recipe_vacuum(r, *r->legacy_compartment, *r->legacy_compartment);
                    }
                    if (r->state->session.session_type == 1) {
                        assert_condition(r, false,
                                         "locState == PICO_CLEANING || locState == PICO_SOUS_VIDE");
                        show(r, PICO_RECIPE_RINSE_FINISHED, 0, NULL);
                        pico_recipe_rinse(r);
                    }
                    r->ops->serial(r->context, "runProgram(): DONE", true);
                }
                return result;
            }
            r->ops->still_connect_wifi(r->context, true);
            show(r, PICO_RECIPE_STOP_PROGRESS, 0, NULL);
            show(r, PICO_RECIPE_CLEAR, 0, NULL);
        } else {
            r->ops->still_connect_wifi(r->context, true);
            show(r, PICO_RECIPE_STOP_PROGRESS, 0, NULL);
        }
        show(r, PICO_RECIPE_CLEAR, 0, NULL);
        r->ops->center_text(r->context, 1, "Connection delay.");
        r->ops->center_text(r->context, 2, "Wait one minute");
        r->ops->center_text(r->context, 3, "and try again. If");
        r->ops->center_text(r->context, 4, "problem persists,");
        r->ops->center_text(r->context, 5, "restart Still.");
        return (uint32_t)r->ops->prompt(r->context, PICO_RECIPE_WAIT_CONTINUE, 0, NULL);
    }
    show(r, PICO_RECIPE_CLEAR, 0, NULL);
    r->ops->center_text(r->context, 1, failure);
    return (uint32_t)r->ops->prompt(r->context, PICO_RECIPE_WAIT_CONTINUE, 0, NULL);
#endif
}
