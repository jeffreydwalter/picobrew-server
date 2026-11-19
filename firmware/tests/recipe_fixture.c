/* Explicit emulator boundary traps, never part of a production build. */
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "picobrew/recipe.h"
#include <stddef.h>
#include <string.h>
PicoAppState recipe_fixture_state;
PicoRecipeRuntime recipe_fixture_runtime;
PicoStepRequest recipe_fixture_request;
PicoControl recipe_fixture_control;
PicoActuators recipe_fixture_actuators;
PicoPhaseControl recipe_fixture_phase;
PicoShuttleCalibration recipe_fixture_calibration;
uint8_t recipe_fixture_language, recipe_fixture_detailed, recipe_fixture_offline,
    recipe_fixture_legacy;
static char recipe_fixture_asset1, recipe_fixture_asset2, recipe_fixture_asset3;
void recipe_fixture_page0(void) {}
void recipe_fixture_page1(void) {}
void recipe_fixture_page2(void) {}
void recipe_fixture_page3(void) {}
void recipe_fixture_page4(void) {}
void recipe_fixture_page5(void) {}
void recipe_fixture_page6(void) {}
void recipe_fixture_page7(void) {}
void recipe_fixture_page8(void) {}
void recipe_fixture_page9(void) {}
void recipe_fixture_page10(void) {}
void recipe_fixture_page11(void) {}
static const PicoProgramAssets recipe_fixture_assets = {
    &recipe_fixture_asset1, &recipe_fixture_asset2, &recipe_fixture_asset3};
static const PicoRecipePages recipe_fixture_pages = {
    {recipe_fixture_page0, recipe_fixture_page1, recipe_fixture_page2, recipe_fixture_page3,
     recipe_fixture_page4},
    {recipe_fixture_page5, recipe_fixture_page6},
    {recipe_fixture_page7, recipe_fixture_page8, recipe_fixture_page9},
    recipe_fixture_page10,
    recipe_fixture_page11};
__attribute__((noinline, used)) uint32_t recipe_fixture_millis(void *ctx)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}
__attribute__((noinline, used)) void recipe_fixture_delay(void *ctx, uint32_t ms)
{
    __asm__ volatile("bkpt #0");
}
__attribute__((noinline, used)) float recipe_fixture_wort(void *ctx)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}
__attribute__((noinline, used)) void recipe_fixture_move(void *ctx, uint32_t compartment)
{
    __asm__ volatile("bkpt #0");
}
static const PicoControlOps recipe_fixture_control_ops = {
    .millis = recipe_fixture_millis, .read_wort_fahrenheit = recipe_fixture_wort};
__attribute__((noinline, used)) void recipe_fixture_filter_prompt(void *ctx)
{
    __asm__ volatile("bkpt #0");
}
static const PicoActuatorOps recipe_fixture_actuator_ops = {
    .delay_milliseconds = recipe_fixture_delay,
    .prompt_insert_filter = recipe_fixture_filter_prompt};
static const PicoPhaseOps recipe_fixture_phase_ops = {.move_to_compartment = recipe_fixture_move};
__attribute__((noinline, used)) void recipe_fixture_log(void *context, const char *level,
                                                        const char *module, const char *message)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_serial(void *context, const char *text,
                                                           bool newline)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_assertion(void *context, bool condition,
                                                              const char *expression)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) const char *
recipe_fixture_action_label(void *context, uint32_t action, uint32_t language, uint32_t status)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void
recipe_fixture_compartment_label(void *context, char *destination, uint32_t compartment)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_screen(void *context, PicoRecipeScreen screen,
                                                           uint32_t argument, const char *text)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) int recipe_fixture_prompt(void *context, PicoRecipePrompt prompt,
                                                          uint32_t argument, const char *text)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_clear_row(void *context, uint16_t row,
                                                              uint16_t column)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_advance_carousel(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_refresh_page(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_set_recipe_name(void *context, const char *name)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_set_step_name(void *context, uint32_t location,
                                                                  const char *name)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_take_click(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_clear_click(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) uint32_t recipe_fixture_populate_brew_menu(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) uint32_t recipe_fixture_populate_post_brew_menu(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) uint32_t recipe_fixture_choose_menu(
    void *context, uint32_t row, uint32_t count, uint32_t initial, int use_timeout,
    uint8_t default_index, uint8_t redraw, uint32_t timeout_seconds)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) uint32_t recipe_fixture_temperature_input(void *context,
                                                                          uint32_t label,
                                                                          const char *title,
                                                                          uint32_t initial)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) uint32_t recipe_fixture_time_input(void *context, uint32_t label,
                                                                   const char *title,
                                                                   uint32_t initial)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_error(void *context, uint32_t code,
                                                          bool logging, bool display)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_wait_for_filter(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_check_thermoblock_heating(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_move_home(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void
recipe_fixture_save_recovery(void *context, uint32_t active_minutes, uint32_t drain_minutes,
                             int32_t step_index, uint32_t reserved, bool force, uint8_t calibrated)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_read_recovery(void *context, char *record)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_delete_recovery(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_get_session_id(void *context, char *identifier,
                                                                   uint32_t capacity)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_report_state(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_log_step(void *context, const char *name)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_log_progress(void *context, const char *name)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) int
recipe_fixture_fetch_recipe(void *context, const char *identifier, int32_t ibu, int32_t abv)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool
recipe_fixture_fetch_default_recipe_failed(void *context, const char *identifier)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_read_picopak(void *context, char *identifier,
                                                                 uint32_t capacity)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) int
recipe_fixture_required_actions(void *context, uint32_t *actions, uint32_t capacity)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_save_shuttle_scale(void *context, double scale)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_load_shuttle_scale(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_still_read_status(void *context,
                                                                      PicoStillTelemetry *telemetry)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool
recipe_fixture_still_pressure_limit(void *context, PicoStillTelemetry *telemetry)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_still_stop(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) uint32_t recipe_fixture_run_drain_instructions(void *context,
                                                                               uint32_t instruction)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) const char *recipe_fixture_title_label(void *context, uint32_t id,
                                                                       uint32_t language)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_center_text(void *context, uint16_t row,
                                                                const char *text)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void
recipe_fixture_timer(void *context, uint16_t row, uint32_t started_ms, uint32_t duration_seconds)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_temperature_columns(void *context, uint16_t row,
                                                                        double wort, double steam,
                                                                        double target)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_temperature_summary(void *context, double wort,
                                                                        double steam, double target,
                                                                        uint16_t row)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_valve_position(void *context, uint16_t row,
                                                                   uint32_t position)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_circulation_status(void *context, double wort,
                                                                       uint32_t elapsed_seconds)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) uint32_t
recipe_fixture_instruction_screen(void *context, uint32_t screen, int alarm, uint32_t program,
                                  const char *title, bool use_context_title)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) int
recipe_fixture_instruction_sequence(void *context, const uint32_t *screens, int count, int alarm,
                                    uint32_t program, const char *title, bool use_context_title)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void
recipe_fixture_populate_actions(void *context, const uint32_t *actions, uint32_t count)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) uint32_t recipe_fixture_select_action(void *context, uint32_t row,
                                                                      int32_t initial)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void
recipe_fixture_alarm_until_press_or_timeout(void *context, uint32_t timeout_ms)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_report_error(void *context, uint32_t code)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) void recipe_fixture_still_initialize(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_still_has_home_ip(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_still_choose_access_point(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_still_permission(void *context,
                                                                     const char *method)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_still_configure(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_still_connect_wifi(void *context, bool home)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

__attribute__((noinline, used)) bool recipe_fixture_still_start_once(void *context)
{
    __asm__ volatile("bkpt #0");
    __builtin_unreachable();
}

static const PicoRecipeOps recipe_fixture_recipe_ops = {
    .log = recipe_fixture_log,
    .serial = recipe_fixture_serial,
    .assertion = recipe_fixture_assertion,
    .action_label = recipe_fixture_action_label,
    .compartment_label = recipe_fixture_compartment_label,
    .screen = recipe_fixture_screen,
    .prompt = recipe_fixture_prompt,
    .clear_row = recipe_fixture_clear_row,
    .advance_carousel = recipe_fixture_advance_carousel,
    .refresh_page = recipe_fixture_refresh_page,
    .set_recipe_name = recipe_fixture_set_recipe_name,
    .set_step_name = recipe_fixture_set_step_name,
    .take_click = recipe_fixture_take_click,
    .clear_click = recipe_fixture_clear_click,
    .populate_brew_menu = recipe_fixture_populate_brew_menu,
    .populate_post_brew_menu = recipe_fixture_populate_post_brew_menu,
    .choose_menu = recipe_fixture_choose_menu,
    .temperature_input = recipe_fixture_temperature_input,
    .time_input = recipe_fixture_time_input,
    .error = recipe_fixture_error,
    .wait_for_filter = recipe_fixture_wait_for_filter,
    .check_thermoblock_heating = recipe_fixture_check_thermoblock_heating,
    .move_home = recipe_fixture_move_home,
    .save_recovery = recipe_fixture_save_recovery,
    .read_recovery = recipe_fixture_read_recovery,
    .delete_recovery = recipe_fixture_delete_recovery,
    .get_session_id = recipe_fixture_get_session_id,
    .report_state = recipe_fixture_report_state,
    .log_step = recipe_fixture_log_step,
    .log_progress = recipe_fixture_log_progress,
    .fetch_recipe = recipe_fixture_fetch_recipe,
    .fetch_default_recipe_failed = recipe_fixture_fetch_default_recipe_failed,
    .read_picopak = recipe_fixture_read_picopak,
    .required_actions = recipe_fixture_required_actions,
    .save_shuttle_scale = recipe_fixture_save_shuttle_scale,
    .load_shuttle_scale = recipe_fixture_load_shuttle_scale,
    .still_read_status = recipe_fixture_still_read_status,
    .still_pressure_limit = recipe_fixture_still_pressure_limit,
    .still_stop = recipe_fixture_still_stop,
    .run_drain_instructions = recipe_fixture_run_drain_instructions,
    .title_label = recipe_fixture_title_label,
    .center_text = recipe_fixture_center_text,
    .timer = recipe_fixture_timer,
    .temperature_columns = recipe_fixture_temperature_columns,
    .temperature_summary = recipe_fixture_temperature_summary,
    .valve_position = recipe_fixture_valve_position,
    .circulation_status = recipe_fixture_circulation_status,
    .instruction_screen = recipe_fixture_instruction_screen,
    .instruction_sequence = recipe_fixture_instruction_sequence,
    .populate_actions = recipe_fixture_populate_actions,
    .select_action = recipe_fixture_select_action,
    .alarm_until_press_or_timeout = recipe_fixture_alarm_until_press_or_timeout,
    .report_error = recipe_fixture_report_error,
    .still_initialize = recipe_fixture_still_initialize,
    .still_has_home_ip = recipe_fixture_still_has_home_ip,
    .still_choose_access_point = recipe_fixture_still_choose_access_point,
    .still_permission = recipe_fixture_still_permission,
    .still_configure = recipe_fixture_still_configure,
    .still_connect_wifi = recipe_fixture_still_connect_wifi,
    .still_start_once = recipe_fixture_still_start_once,
};

void recipe_fixture_reset(void)
{
    memset(&recipe_fixture_state, 0, sizeof recipe_fixture_state);
    memset(&recipe_fixture_calibration, 0, sizeof recipe_fixture_calibration);
    recipe_fixture_state.recipe.program = &recipe_fixture_state.session;
    recipe_fixture_language = recipe_fixture_detailed = recipe_fixture_offline = 0;
    recipe_fixture_legacy = 1;
    recipe_fixture_control =
        (PicoControl){&recipe_fixture_state.controller, &recipe_fixture_state.control_signals,
                      &recipe_fixture_control_ops, 0};
    recipe_fixture_actuators = (PicoActuators){&recipe_fixture_actuator_ops, 0};
    recipe_fixture_phase =
        (PicoPhaseControl){&recipe_fixture_control,      &recipe_fixture_actuators,
                           &recipe_fixture_state.screen, &recipe_fixture_state.session,
                           &recipe_fixture_calibration,  &recipe_fixture_language,
                           &recipe_fixture_phase_ops,    0};
    recipe_fixture_runtime = (PicoRecipeRuntime){&recipe_fixture_state,
                                                 &recipe_fixture_phase,
                                                 &recipe_fixture_language,
                                                 &recipe_fixture_detailed,
                                                 &recipe_fixture_offline,
                                                 &recipe_fixture_legacy,
                                                 &recipe_fixture_assets,
                                                 &recipe_fixture_pages,
                                                 &recipe_fixture_recipe_ops,
                                                 0,
                                                 0};
}
const uint32_t recipe_fixture_offsets[] = {sizeof(PicoAppState),
                                           offsetof(PicoAppState, controller),
                                           offsetof(PicoAppState, control_signals),
                                           offsetof(PicoAppState, screen),
                                           offsetof(PicoAppState, carousel),
                                           offsetof(PicoAppState, session),
                                           offsetof(PicoAppState, recipe),
                                           offsetof(PicoAppState, menu),
                                           offsetof(PicoAppState, device_state),
                                           sizeof(PicoProgramContext),
                                           sizeof(PicoRecipe),
                                           sizeof(PicoBrewController),
                                           sizeof(PicoScreenState),
                                           offsetof(PicoRecipeRuntime, active_title),
                                           offsetof(PicoBrewController, wort_fahrenheit),
                                           offsetof(PicoBrewController, thermoblock_fahrenheit),
                                           sizeof(PicoStepRequest)};
