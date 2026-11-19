#include "picobrew/application.h"
#include <stdio.h>
#include <string.h>
#include "picobrew/config.h"
#if PICO_STANDALONE
#include "picobrew/local_control.h"
#endif

static PicoApplication *app(void *context)
{
    return context;
}
static uint32_t clock_ms(void *c)
{
#if PICO_STANDALONE
    pico_local_service();
#endif
    return app(c)->hardware->millis(app(c)->hardware_context);
}
static void delay_ms(void *c, uint32_t ms)
{
#if PICO_STANDALONE
    /* Slice foreground waits so the LAN API remains responsive to Stop and
     * remote front-panel input. The service hook rejects ISR/IRQ-masked calls. */
    while (ms) {
        uint32_t slice = ms > 10 ? 10 : ms;
        app(c)->hardware->delay_ms(app(c)->hardware_context, slice);
        ms -= slice;
        pico_local_service();
    }
#else
    app(c)->hardware->delay_ms(app(c)->hardware_context, ms);
#endif
}
static void pin_mode(void *c, uint8_t pin, uint8_t mode)
{
    app(c)->hardware->pin_mode(app(c)->hardware_context, pin, mode);
}
static int digital_read(void *c, uint8_t pin)
{
    return app(c)->hardware->digital_read(app(c)->hardware_context, pin);
}
static void digital_write(void *c, uint8_t pin, uint8_t value)
{
#if PICO_STANDALONE
    if (value && pico_local_control_stopped() &&
        (pin == PICO_PIN_HEATER || pin == PICO_PIN_DRAIN_PUMP ||
         pin == PICO_PIN_SHUTTLE_PUMP || pin == PICO_PIN_STEAM_PUMP))
        return;
    pico_local_control_bus_select(pin, value == 0);
#endif
    app(c)->hardware->digital_write(app(c)->hardware_context, pin, value);
}
static void spi_begin(void *c)
{
    app(c)->hardware->spi_begin(app(c)->hardware_context);
}
static uint8_t spi_transfer(void *c, uint8_t value)
{
    return app(c)->hardware->spi_transfer(app(c)->hardware_context, value);
}
static void spi_mode(void *c, uint8_t mode)
{
    app(c)->hardware->spi_mode(app(c)->hardware_context, mode);
}
static void spi_order(void *c, uint8_t order)
{
    app(c)->hardware->spi_bit_order(app(c)->hardware_context, order);
}
static void disable_interrupts(void *c)
{
    app(c)->hardware->interrupts_disable(app(c)->hardware_context);
}
static bool enable_interrupts(void *c)
{
    return app(c)->hardware->interrupts_enable(app(c)->hardware_context);
}
static void enable_interrupts_void(void *c)
{
    (void)enable_interrupts(c);
}
static void attach_interrupt(void *c, unsigned interrupt, void (*handler)(void), unsigned mode)
{
    app(c)->hardware->attach_interrupt(app(c)->hardware_context, interrupt, handler, mode);
}
static void ui_pin_mode(void *c, unsigned pin, unsigned mode)
{
    pin_mode(c, (uint8_t)pin, (uint8_t)mode);
}
static int ui_digital_read(void *c, unsigned pin)
{
    return digital_read(c, (uint8_t)pin);
}
static void ui_digital_write(void *c, unsigned pin, unsigned value)
{
    digital_write(c, (uint8_t)pin, (uint8_t)value);
}
static void log_message(void *c, const char *level, const char *module, const char *message)
{
    pico_network_log(&app(c)->network, level, module, message);
}
static void assertion(void *c, bool condition, const char *expression)
{
    pico_network_assert(&app(c)->network, condition, expression);
}
static void serial_text(void *c, const char *text, bool newline)
{
    PicoNetwork *n = &app(c)->network;
    n->platform->serial_write(n->platform_context, text);
    if (newline)
        n->platform->serial_write(n->platform_context, "\r\n");
}
static void error(void *c, unsigned code, bool acknowledge, bool report)
{
    (void)c;
    pico_report_error((int)code, acknowledge, report);
}
static void network_error(void *c, int code, bool acknowledge, bool report)
{
    error(c, (unsigned)code, acknowledge, report);
}
static void report_error_to_server(void *c, uint32_t code)
{
    pico_api_error(&app(c)->network, (int)code);
}
static void clear_display(void *c)
{
    (void)c;
    pico_display_clear();
}
static void flush_display(void *c)
{
    (void)c;
    pico_display_flush();
}
static void service_screen(void *c)
{
    (void)c;
    pico_screen_service_status();
}
static void error_heading(void *c)
{
    (void)c;
    pico_screen_error_heading();
}
static void clear_click(void *c)
{
    pico_encoder_clear_click(&app(c)->ui.encoder);
}
static bool take_click(void *c)
{
    return pico_encoder_take_click(&app(c)->ui.encoder) != 0;
}
static bool filter_inserted(void *c)
{
    return pico_filter_is_inserted(app(c)->actuators);
}
static void wait_filter(void *c)
{
    pico_device_wait_for_filter(&app(c)->device);
}
static void insert_picopak_screen(void *c)
{
    (void)c;
    pico_ui_draw_insert_picopak();
}
static bool buzzer(void *c, bool on)
{
    return pico_device_buzzer(&app(c)->device, on);
}
static void alarm_filter(void *c)
{
    pico_device_alarm_while_filter_missing(&app(c)->device);
}
static void alarm_click(void *c)
{
    pico_device_alarm_until_encoder_press(&app(c)->device);
}
static void alarm_timeout(void *c, uint32_t timeout)
{
    pico_alarm_until_press_or_timeout(app(c)->settings.language, app(c)->settings.detailed_status,
                                      (int)timeout);
}
static void move_compartment(void *c, uint32_t compartment)
{
    pico_device_move_compartment(&app(c)->device, compartment);
}
static void ui_move_compartment(void *c, uint8_t compartment)
{
    move_compartment(c, compartment);
}
static void move_home(void *c)
{
    pico_device_move_home(&app(c)->device);
}
static bool set_torque(void *c, unsigned mode)
{
    return pico_device_set_torque(&app(c)->device, mode);
}
static bool check_heat(void *c)
{
    return pico_device_check_thermoblock_heating(&app(c)->device, app(c)->control);
}
static void pause_until_resume(void *c)
{
    pico_phase_pause_until_resume(&app(c)->phase);
}
static void wait_resume(void *c)
{
    (void)c;
    pico_ui_wait_for_resume();
}
static void clear_row(void *c, uint16_t row, uint16_t column)
{
    (void)c;
    pico_display_clear_row_from_column((short)row, (short)column);
}
static void refresh_page(void *c)
{
    (void)c;
    pico_display_refresh_current_page();
}
static void advance_page(void *c)
{
    pico_display_advance_carousel(&app(c)->state.carousel);
}
static void show_ending_step(void *c, const char *title)
{
    (void)c;
    pico_screen_ending_step(title);
}
static void save_shuttle(void *c, double scale)
{
    app(c)->state.controller.candidate_shuttle_scale = scale;
    pico_storage_save_shuttle_scale(&app(c)->network);
}
static void load_shuttle(void *c)
{
    pico_storage_load_shuttle_scale(&app(c)->network);
}
static void first_rinse(void *c)
{
    pico_recipe_first_rinse(&app(c)->recipe);
}
static void rinse(void *c)
{
    pico_recipe_rinse(&app(c)->recipe);
}
static void circulation_program(void *c)
{
    pico_recipe_circulation_program(&app(c)->recipe);
}
static void rack_beer(void *c)
{
    pico_recipe_rack_beer(&app(c)->recipe);
}
static void descale(void *c)
{
    pico_recipe_descale(&app(c)->recipe);
}
static void still_program(void *c)
{
#if PICO_STANDALONE
    (void)c;
    log_message(c, "W", "LOCAL", "PicoStill is unavailable in the standalone profile");
#else
    pico_recipe_still_program(&app(c)->recipe);
#endif
}
static void drain_reservoir(void *c)
{
    pico_recipe_drain_reservoir(&app(c)->recipe);
}
static void drain_filter(void *c)
{
    pico_recipe_drain_filter(&app(c)->recipe);
}
static void circulate(void *c)
{
    pico_recipe_circulate(&app(c)->recipe);
}
static void restore_idle(void *c)
{
    pico_recipe_restore_idle(&app(c)->recipe);
}
static void flush_compartments(void *c, int cycles, int stop)
{
    pico_recipe_flush_compartments(&app(c)->recipe, (uint32_t)cycles, stop != 0);
}
static void flush_until_press(void *c)
{
    pico_recipe_flush_until_press(&app(c)->recipe);
}
static void heat_flush(void *c, unsigned steam, double target)
{
    pico_recipe_heat_flush_compartments(&app(c)->recipe, steam != 0, target);
}
static uint32_t instruction(void *c, uint32_t id, int32_t option)
{
    return pico_recipe_instruction(&app(c)->recipe, id, option);
}
static uint32_t ui_instruction(void *c, uint32_t id, bool alarm)
{
    return instruction(c, id, alarm);
}
static bool fetch_default_failed(void *c, const char *identifier)
{
    return pico_api_fetch_recipe(&app(c)->network, identifier, -1, -1) != 0;
}
static bool read_uid(void *c, char uid[22])
{
    return pico_rfid_read_uid(&app(c)->rfid, uid, 22);
}
static bool read_picopak(void *c, char *uid, uint32_t capacity)
{
    return pico_rfid_read_picopak(&app(c)->rfid, uid, capacity);
}
static const char *action_label(void *c, uint32_t action, uint32_t language, uint32_t status)
{
    (void)c;
    return pico_action_label(action, language, status);
}
static void compartment_label(void *c, char *destination, uint32_t compartment)
{
    (void)c;
    pico_label_compartment(destination, compartment);
}
static const char *title_label(void *c, uint32_t id, uint32_t language)
{
    (void)c;
    return pico_title_label(id, language);
}

static void recipe_screen(void *c, PicoRecipeScreen screen, uint32_t argument, const char *text)
{
    PicoApplication *a = app(c);
    switch (screen) {
    case PICO_RECIPE_CLEAR:
        pico_display_clear();
        break;
    case PICO_RECIPE_FLUSH:
        pico_display_flush();
        break;
    case PICO_RECIPE_LOGO:
        pico_display_logo();
        break;
    case PICO_RECIPE_CHANGING_STEP:
        pico_screen_changing_step();
        break;
    case PICO_RECIPE_SETTING_UP_STEP:
        pico_screen_setting_up_step(text);
        break;
    case PICO_RECIPE_TITLED_FRAME:
        pico_display_draw_titled_frame(text);
        break;
    case PICO_RECIPE_PREHEATING_STEAM:
        pico_screen_preheating_steam((short)argument);
        break;
    case PICO_RECIPE_STARTING_PROGRAM:
        pico_screen_starting_program(text);
        break;
    case PICO_RECIPE_ENDING_PROGRAM:
        pico_screen_show_ending_program(text);
        break;
    case PICO_RECIPE_LOADING:
        pico_screen_loading((int)argument);
        break;
    case PICO_RECIPE_LOADING_PACK:
        pico_screen_loading_picopak();
        break;
    case PICO_RECIPE_STOP_PROGRESS:
        pico_display_stop_progress_indicator(&a->state.control_signals);
        break;
    case PICO_RECIPE_PAUSED:
        pico_ui_draw_paused();
        break;
    case PICO_RECIPE_SKIP_CONFIRMATION:
        pico_ui_draw_skip_step_confirmation();
        break;
    case PICO_RECIPE_STOP_CONFIRMATION:
        pico_ui_draw_stop_confirmation();
        break;
    case PICO_RECIPE_STILL_COMMUNICATION_FAILURE:
#if !PICO_STANDALONE
        pico_ui_show_still_communication_failure();
#endif
        break;
    case PICO_RECIPE_STILL_TEMPERATURE_WARNING:
#if !PICO_STANDALONE
        pico_ui_show_still_temperature_warning();
#endif
        break;
    case PICO_RECIPE_STILL_COMPLETION:
#if !PICO_STANDALONE
        pico_ui_show_still_completion();
#endif
        break;
    case PICO_RECIPE_STILL_FAN_MENU:
#if !PICO_STANDALONE
        pico_ui_still_fan_menu();
#endif
        break;
    case PICO_RECIPE_REQUIRE_FIRST_RINSE:
        pico_ui_require_first_rinse();
        break;
    case PICO_RECIPE_BREW_COMPLETE:
        pico_ui_draw_brew_complete(text);
        break;
    case PICO_RECIPE_WAIT_CLICK:
        pico_display_wait_for_click();
        break;
    case PICO_RECIPE_RINSE_FINISHED:
        pico_ui_show_rinse_finished();
        break;
    case PICO_RECIPE_DEEP_CLEAN_SETUP:
        pico_ui_prompt_deep_clean_setup();
        break;
    case PICO_RECIPE_ENDING_PROCESS:
        pico_screen_ending_process();
        break;
    case PICO_RECIPE_PROGRAM_NOTICE:
        pico_screen_show_program_notice();
        break;
    case PICO_RECIPE_DRAIN_RESERVOIR:
        pico_ui_draw_drain_reservoir();
        break;
    case PICO_RECIPE_DRAIN_FILTER:
        pico_ui_draw_drain_filter();
        break;
    case PICO_RECIPE_STOP_FOOTER:
        pico_ui_draw_stop_footer();
        break;
    case PICO_RECIPE_HEADER:
        pico_ui_draw_header(text);
        break;
    case PICO_RECIPE_FIRST_RINSE_STEP:
        pico_ui_first_rinse_step(argument);
        break;
    case PICO_RECIPE_ENDING_FIRST_RINSE:
        pico_screen_ending_first_rinse();
        break;
    case PICO_RECIPE_RACK_BEER_STEP:
        pico_ui_rack_beer_step(argument);
        break;
    case PICO_RECIPE_DESCALE_HEADING:
        pico_ui_draw_descale_heading();
        break;
    case PICO_RECIPE_WAIT_STOP:
        pico_ui_wait_for_stop();
        break;
    case PICO_RECIPE_INSTRUCTION:
        pico_ui_draw_instruction_screen(argument, a->settings.language, text, 1);
        break;
    case PICO_RECIPE_INSTRUCTION_HEADER:
        pico_ui_draw_instruction_with_header(argument, a->settings.language, text);
        break;
    case PICO_RECIPE_START_DESCALE:
        pico_ui_start_descale();
        break;
    case PICO_RECIPE_CONFIRM_STILL_CLEANED:
#if !PICO_STANDALONE
        pico_ui_confirm_still_cleaned();
#endif
        break;
    case PICO_RECIPE_WAIT_FILTER_INSERTION:
        pico_ui_wait_for_filter_insertion();
        break;
    }
}
static int recipe_prompt(void *c, PicoRecipePrompt prompt, uint32_t argument, const char *text)
{
#if PICO_STANDALONE
    if (pico_local_control_stopped())
        return 0;
#endif
    switch (prompt) {
    case PICO_RECIPE_CONFIRM_AT_ROW:
        return pico_ui_confirm_at_row(argument);
    case PICO_RECIPE_CONFIRM_THERMAL_RECOVERY:
        return pico_ui_confirm_thermal_recovery((int)argument);
    case PICO_RECIPE_SETUP_CHOICE:
        return pico_ui_choose_binary_option(text);
    case PICO_RECIPE_SELECT_RECIPE:
        return (int)pico_ui_select_recipe(&app(c)->state.recipe);
    case PICO_RECIPE_ADJUST_RECIPE:
        return pico_ui_adjust_recipe(&app(c)->state.recipe);
    case PICO_RECIPE_CONFIRM_CLEANING:
        return pico_ui_confirm_screen(pico_ui_draw_deep_clean_confirmation);
    case PICO_RECIPE_VACUUM:
        return pico_ui_prompt_vacuum((int)argument);
    case PICO_RECIPE_DESCALE_STEP:
        return pico_ui_descale_step(argument);
    case PICO_RECIPE_CHOOSE_STILL_PROGRAM:
#if PICO_STANDALONE
        return -1;
#else
        return (int)pico_ui_choose_program();
#endif
    case PICO_RECIPE_WALK_STILL_STEPS:
#if PICO_STANDALONE
        return -1;
#else
        return (int)pico_ui_walk_program_steps((int)argument);
#endif
    case PICO_RECIPE_WAIT_CONTINUE:
        return (int)pico_ui_wait_for_continue();
    }
    return 0; /* Exhaustive enum; invalid callers are outside the API contract. */
}

static void set_recipe_name(void *c, const char *name)
{
    pico_screen_set_recipe_name(&app(c)->state.screen, name);
}
static void set_step_name(void *c, uint32_t location, const char *name)
{
    pico_screen_set_step(&app(c)->state.screen, location, name);
}
static uint32_t brew_menu(void *c)
{
    (void)c;
    return pico_menu_populate_in_brew_options();
}
static uint32_t post_brew_menu(void *c)
{
    (void)c;
    return pico_menu_populate_post_brew();
}
static uint32_t choose_menu(void *c, uint32_t row, uint32_t count, uint32_t initial, int timeout,
                            uint8_t default_index, uint8_t redraw, uint32_t seconds)
{
    (void)c;
    return pico_menu_choose_with_timeout(row, count, initial, timeout, default_index, redraw,
                                         (int)seconds);
}
static uint32_t temperature_input(void *c, uint32_t label, const char *title, uint32_t initial)
{
    (void)c;
    return pico_get_temperature_input(label, title, initial);
}
static uint32_t time_input(void *c, uint32_t label, const char *title, uint32_t initial)
{
    (void)c;
    return (uint32_t)pico_get_time_input(label, title, (int)initial);
}
static void recipe_error(void *c, uint32_t code, bool acknowledge, bool report)
{
    error(c, code, acknowledge, report);
}
static void save_recovery(void *c, uint32_t active, uint32_t drain, int32_t index,
                          uint32_t reserved, bool force, uint8_t calibrated)
{
    pico_storage_save_recovery(&app(c)->network, (int)active, (int)drain, (int)index, (int)reserved,
                               force, calibrated);
}
#if PICO_STANDALONE
static bool read_local_recovery(void *c, PicoLocalRecovery *record)
{
    return pico_local_recovery_get(&app(c)->network, record) == PICO_LOCAL_STORE_OK;
}
#else
static bool read_recovery(void *c, char *record)
{
    return pico_storage_read_restart(&app(c)->network, record);
}
#endif
static void delete_recovery(void *c)
{
    pico_storage_delete_restart(&app(c)->network);
}
static bool session_id(void *c, char *identifier, uint32_t capacity)
{
    return pico_api_get_session(&app(c)->network, identifier, capacity);
}
static void report_state(void *c)
{
    pico_api_report_state(&app(c)->network);
}
static void log_step(void *c, const char *name)
{
    pico_api_log_step(&app(c)->network, name);
}
static void log_progress(void *c, const char *name)
{
    pico_api_log_progress(&app(c)->network, name);
}
static int fetch_recipe(void *c, const char *identifier, int32_t ibu, int32_t abv)
{
    return pico_api_fetch_recipe(&app(c)->network, identifier, (int)ibu, (int)abv);
}
static int required_actions(void *c, uint32_t *actions, uint32_t capacity)
{
    return pico_api_required_actions(&app(c)->network, actions, capacity);
}
static bool still_status(void *c, PicoStillTelemetry *telemetry)
{
#if PICO_STANDALONE
    (void)c; (void)telemetry;
    return false;
#else
    bool result = pico_still_read_status(&app(c)->network);
    if (telemetry != &app(c)->state.controller.still)
        *telemetry = app(c)->state.controller.still;
    return result;
#endif
}
static bool still_pressure(void *c, PicoStillTelemetry *telemetry)
{
#if PICO_STANDALONE
    (void)c; (void)telemetry;
    return false;
#else
    (void)c;
    return pico_still_pressure_ok(telemetry);
#endif
}
static void still_stop(void *c)
{
#if PICO_STANDALONE
    (void)c;
#else
    pico_still_stop(&app(c)->network);
#endif
}
static uint32_t drain_instructions(void *c, uint32_t instruction_id)
{
    (void)c;
    return pico_run_drain_instructions(instruction_id);
}
static void center_text(void *c, uint16_t row, const char *text)
{
    (void)c;
    pico_display_center_text((short)row, text);
}
static void timer(void *c, uint16_t row, uint32_t started, uint32_t seconds)
{
    (void)c;
    pico_display_update_timer((short)row, (int)started, (int)seconds);
}
static void temperature_columns(void *c, uint16_t row, double wort, double steam, double target)
{
    (void)c;
    pico_display_temperature_columns((short)row, wort, steam, target);
}
static void temperature_summary(void *c, double wort, double steam, double target, uint16_t row)
{
    (void)c;
    pico_display_temperature_summary(wort, steam, target, row);
}
static void valve_position(void *c, uint16_t row, uint32_t position)
{
    (void)c;
    pico_display_valve_position((short)row, position);
}
static void circulation_status(void *c, double wort, uint32_t seconds)
{
    (void)c;
    pico_ui_draw_circulation_status(wort, seconds);
}
static uint32_t instruction_screen(void *c, uint32_t screen, int alarm, uint32_t program,
                                   const char *title, bool context_title)
{
    (void)c;
    return pico_ui_run_instruction_screen(screen, alarm, (int)program, title, context_title);
}
static int instruction_sequence(void *c, const uint32_t *screens, int count, int alarm,
                                uint32_t program, const char *title, bool context_title)
{
    (void)c;
    return pico_ui_run_instruction_sequence(screens, count, alarm, (int)program, title,
                                            context_title);
}
static void populate_actions(void *c, const uint32_t *actions, uint32_t count)
{
    pico_menu_populate_action_labels(actions, count, app(c)->settings.language,
                                     app(c)->settings.detailed_status);
}
static uint32_t select_action(void *c, uint32_t row, int32_t initial)
{
    (void)c;
    return pico_menu_select_action(row, (int)initial);
}
static void still_initialize(void *c)
{
#if PICO_STANDALONE
    memset(&app(c)->network.still, 0, sizeof app(c)->network.still);
#else
    pico_still_connection_initialize(&app(c)->network.still);
#endif
}
static bool still_home(void *c)
{
#if PICO_STANDALONE
    (void)c;
    return false;
#else
    return !pico_network_ipv4_is_zero(app(c)->network.still.home_ip);
#endif
}
static bool still_choose_ap(void *c)
{
#if PICO_STANDALONE
    (void)c;
    return false;
#else
    return pico_still_choose_access_point(&app(c)->network);
#endif
}
static bool still_permission(void *c, const char *method)
{
#if PICO_STANDALONE
    (void)c; (void)method;
    return false;
#else
    return pico_api_still_permission(&app(c)->network, method, app(c)->network.still.identifier);
#endif
}
static bool still_configure(void *c)
{
#if PICO_STANDALONE
    (void)c;
    return false;
#else
    return pico_still_configure(&app(c)->network);
#endif
}
static bool still_wifi(void *c, bool home)
{
#if PICO_STANDALONE
    (void)c; (void)home;
    return false;
#else
    return pico_still_connect_wifi(&app(c)->network, home);
#endif
}
static bool still_start_once(void *c)
{
#if PICO_STANDALONE
    (void)c;
    return false;
#else
    return pico_still_start_once(&app(c)->network);
#endif
}
static bool choose_wifi(void *c)
{
    (void)c;
    return pico_ui_scan_and_connect_wifi() != 0;
}
static void diagnose_wifi(void *c)
{
    (void)c;
    pico_diagnostic_wifi();
}
static void choose_language(void *c)
{
    (void)c;
    (void)pico_ui_choose_language();
}
static void choose_units(void *c)
{
    (void)c;
    (void)pico_ui_choose_units();
}
static void wait_continue(void *c)
{
    (void)c;
    pico_ui_wait_for_continue();
}
static bool decode_recipe(void *c, const char *text, size_t length)
{
#if PICO_STANDALONE
    /* Local recipes use the validated binary store. Keep this legacy callback
     * as an explicit failure without retaining the unused cloud text parser. */
    (void)c; (void)text; (void)length;
    return false;
#else
    return pico_recipe_parse_response(&app(c)->recipe, text, (int32_t)length);
#endif
}
static void network_event(void *c, PicoNetworkUiEvent event, const char *text)
{
    pico_ui_network_event(&app(c)->ui, event, text);
}
static void recipe_logo_ready(void *c, const uint8_t image[1024])
{
    pico_ui_recipe_logo_ready(&app(c)->ui, image);
}
static void recipe_image_ready(void *c, unsigned index, const uint8_t image[1024])
{
    pico_ui_recipe_image_ready(&app(c)->ui, index, image);
}
static int choose_still(void *c, const char identifiers[][22], size_t count)
{
#if PICO_STANDALONE
    (void)c; (void)identifiers; (void)count;
    return -1;
#else
    return pico_ui_choose_still(&app(c)->ui, identifiers, count);
#endif
}

static const PicoUiHardware ui_hardware = {
    clock_ms,  delay_ms,     ui_pin_mode,        ui_digital_read,   ui_digital_write,
    spi_begin, spi_transfer, disable_interrupts, enable_interrupts, attach_interrupt};
static const PicoDeviceOps device_ops = {
    clock_ms,    delay_ms,      pin_mode,           digital_read,           digital_write,
    spi_mode,    spi_transfer,  disable_interrupts, enable_interrupts_void, log_message,
    error,       clear_display, flush_display,      service_screen,         error_heading,
    clear_click, take_click};
static const PicoRfidOps rfid_ops = {pin_mode,  digital_write, spi_transfer,
                                     spi_order, spi_mode,      delay_ms};
static const PicoRfidApplicationOps rfid_application_ops = {
    clock_ms,      clear_click, take_click, filter_inserted, insert_picopak_screen,
    flush_display, log_message, error};
static const PicoPhaseOps phase_ops = {log_message,      clear_row,        clear_display,
                                       flush_display,    wait_resume,      refresh_page,
                                       show_ending_step, move_compartment, save_shuttle};
static const PicoUiServices ui_services = {.pause_until_resume = pause_until_resume,
                                           .wait_for_filter = wait_filter,
                                           .run_first_rinse = first_rinse,
                                           .run_rinse = rinse,
                                           .run_circulation_program = circulation_program,
                                           .run_rack_beer = rack_beer,
                                           .run_descale = descale,
                                           .run_still = still_program,
                                           .drain_reservoir = drain_reservoir,
                                           .drain_step_filter = drain_filter,
                                           .run_circulation = circulate,
                                           .execute_instruction = ui_instruction,
                                           .fetch_default_recipe_failed = fetch_default_failed,
                                           .buzzer = buzzer,
                                           .alarm_filter_missing = alarm_filter,
                                           .alarm_until_press = alarm_click,
                                           .stepper_move = ui_move_compartment,
                                           .stepper_torque = set_torque,
                                           .restore_idle = restore_idle,
                                           .flush_compartments = flush_compartments,
                                           .flush_compartments_until_press = flush_until_press,
                                           .heat_flush_compartments = heat_flush,
                                           .rfid_read_uid = read_uid};
static const PicoNetworkUi network_ui = {.event = network_event,
                                         .report_error = network_error,
                                         .choose_wifi_and_connect = choose_wifi,
                                         .diagnose_wifi = diagnose_wifi,
                                         .choose_language = choose_language,
                                         .choose_units = choose_units,
                                         .wait_for_continue = wait_continue,
                                         .choose_still = choose_still,
                                         .decode_recipe = decode_recipe,
                                         .recipe_logo_ready = recipe_logo_ready,
                                         .recipe_image_ready = recipe_image_ready};
static const PicoRecipeOps recipe_ops = {.log = log_message,
                                         .serial = serial_text,
                                         .assertion = assertion,
                                         .action_label = action_label,
                                         .compartment_label = compartment_label,
                                         .screen = recipe_screen,
                                         .prompt = recipe_prompt,
                                         .clear_row = clear_row,
                                         .advance_carousel = advance_page,
                                         .refresh_page = refresh_page,
                                         .set_recipe_name = set_recipe_name,
                                         .set_step_name = set_step_name,
                                         .take_click = take_click,
                                         .clear_click = clear_click,
                                         .populate_brew_menu = brew_menu,
                                         .populate_post_brew_menu = post_brew_menu,
                                         .choose_menu = choose_menu,
                                         .temperature_input = temperature_input,
                                         .time_input = time_input,
                                         .error = recipe_error,
                                         .wait_for_filter = wait_filter,
                                         .check_thermoblock_heating = check_heat,
                                         .move_home = move_home,
                                         .save_recovery = save_recovery,
#if PICO_STANDALONE
                                         .read_local_recovery = read_local_recovery,
#else
                                         .read_recovery = read_recovery,
#endif
                                         .delete_recovery = delete_recovery,
                                         .get_session_id = session_id,
                                         .report_state = report_state,
                                         .log_step = log_step,
                                         .log_progress = log_progress,
                                         .fetch_recipe = fetch_recipe,
                                         .fetch_default_recipe_failed = fetch_default_failed,
                                         .read_picopak = read_picopak,
                                         .required_actions = required_actions,
                                         .save_shuttle_scale = save_shuttle,
                                         .load_shuttle_scale = load_shuttle,
                                         .still_read_status = still_status,
                                         .still_pressure_limit = still_pressure,
                                         .still_stop = still_stop,
                                         .run_drain_instructions = drain_instructions,
                                         .title_label = title_label,
                                         .center_text = center_text,
                                         .timer = timer,
                                         .temperature_columns = temperature_columns,
                                         .temperature_summary = temperature_summary,
                                         .valve_position = valve_position,
                                         .circulation_status = circulation_status,
                                         .instruction_screen = instruction_screen,
                                         .instruction_sequence = instruction_sequence,
                                         .populate_actions = populate_actions,
                                         .select_action = select_action,
                                         .alarm_until_press_or_timeout = alarm_timeout,
                                         .report_error = report_error_to_server,
                                         .still_initialize = still_initialize,
                                         .still_has_home_ip = still_home,
                                         .still_choose_access_point = still_choose_ap,
                                         .still_permission = still_permission,
                                         .still_configure = still_configure,
                                         .still_connect_wifi = still_wifi,
                                         .still_start_once = still_start_once};
static const PicoProgramAssets program_assets = {&pico_ui_asset_program_primary,
                                                 &pico_ui_asset_program_secondary,
                                                 &pico_ui_asset_sousvide_secondary};
static const PicoRecipePages recipe_pages = {
    .normal = {pico_screen_brew_name_page, pico_screen_current_step_page,
               pico_screen_step_progress_page, pico_screen_temperature_page,
#if PICO_STANDALONE
               /* Local step recipes have no server-calculated ABV/IBU data. */
               pico_screen_program_status_page},
#else
               pico_screen_recipe_metrics_page},
#endif
    .cooking = {pico_screen_basic_brew_status, pico_screen_detailed_brew_status},
    .manual = {pico_screen_manual_overview_page, pico_screen_current_target_page,
               pico_screen_program_status_page},
    .holding = pico_ui_draw_hold_status,
#if PICO_STANDALONE
    .still = NULL};
#else
    .still = pico_screen_still_readings};
#endif
const PicoPageCallback *pico_application_default_pages(void)
{
    return recipe_pages.normal;
}

void pico_application_bind_services(PicoApplication *a, PicoControl *control,
                                    PicoActuators *actuators,
                                    const PicoApplicationHardware *hardware, void *hardware_context,
                                    const PicoNetworkPlatform *network_platform,
                                    void *network_context)
{
    a->control = control;
    a->actuators = actuators;
    a->hardware = hardware;
    a->hardware_context = hardware_context;
    a->settings.home_compartment = 6; /* Original initialized byte at 0x2002ac80. */
    pico_network_initialize(&a->network, &a->state, &a->settings, network_platform, network_context,
                            &network_ui, a);
    a->device = (PicoDevice){&a->state,
                             actuators,
                             &a->settings.legacy_compartment_mapping,
                             &a->settings.home_compartment,
                             &device_ops,
                             a};
    a->reader.ops = &rfid_ops;
    a->reader.context = a;
    a->rfid = (PicoRfidApplication){&a->reader, &rfid_application_ops, a};
    a->phase = (PicoPhaseControl){control,          actuators,
                                  &a->state.screen, &a->state.session,
                                  &a->calibration,  &a->settings.language,
                                  &phase_ops,       a};
    a->recipe = (PicoRecipeRuntime){&a->state,
                                    &a->phase,
                                    &a->settings.language,
                                    &a->settings.detailed_status,
                                    &a->settings.offline_mode,
                                    &a->settings.legacy_compartment_mapping,
                                    &program_assets,
                                    &recipe_pages,
                                    &recipe_ops,
                                    a,
                                    NULL
#if PICO_STANDALONE
                                    , 0
#endif
                                    };
    pico_ui_initialize(&a->ui, &a->state, &a->network, actuators, control, &ui_hardware, a,
                       &ui_services, a);
}

/* Original setup at 0x200131bc, with explicit owned application services. */
void pico_application_setup(PicoApplication *a)
{
    a->hardware->serial_begin(a->hardware_context, 115200);
    serial_text(a, "", true);
    serial_text(a, "***************************************************************", true);
    serial_text(a, "                        Pico PRO", true);
    serial_text(a, "***************************************************************", true);
#if PICO_STANDALONE
    log_message(a, "I", "MAIN", "PROFILE = standalone; local recipes and LAN control");
#else
    log_message(a, "I", "MAIN", "DATE_TIME = Dec 16 2019 12:59:40");
    log_message(a, "I", "MAIN", "VERSION_STR = 0.1.34");
    log_message(a, "I", "MAIN", "GIT_HASH = d0da766714f2012e98f8e43ecd6519895caf565e");
    log_message(a, "I", "MAIN", "GIT_BRANCH = dev");
#endif
    log_message(a, "I", "MAIN", "SSD_TYPE = SSD1325");
    pico_display_initialize();
    pico_screen_splash();
    pico_actuators_initialize(a->actuators);
    pico_device_temperature_inputs_initialize(&a->device);
    pico_encoder_initialize(&a->ui.encoder, 32, 31, 19, 19, 32, 31);
    pico_device_spi_initialize(&a->device);
    pico_device_filter_input_initialize(&a->device);
    pico_storage_load_preferences(&a->network);
    pico_screen_splash();
    pico_control_timers_initialize(a->control);
    a->network.platform->wifi_initialize(a->network.platform_context);
    if (pico_device_service_button(&a->device)) {
        delay_ms(a, 2000);
        pico_device_detect_stepper(&a->device);
        clear_click(a);
        pico_screen_device_information();
        pico_rfid_application_initialize(&a->rfid);
        pico_ui_service_menu();
        pico_display_clear();
        pico_screen_splash();
        a->network.platform->reset_mcu(a->network.platform_context);
    }
#if PICO_STANDALONE
    /* LAN connectivity is optional; production registration and cloud
     * availability never gate startup. Local API adapters also work offline. */
    pico_network_ensure_connection(&a->network);
    a->settings.offline_mode = 0;
    if (pico_local_recipes_initialize(&a->network) != PICO_LOCAL_STORE_OK)
        log_message(a, "W", "LOCAL", "Could not initialize PicoPak catalogue; existing files preserved");
    pico_local_control_ready(true);
#else
    if (!a->settings.offline_mode) {
        pico_network_ensure_connection(&a->network);
        if (a->network.platform->wifi_status(a->network.platform_context) == 3) {
            while (!pico_api_register(&a->network)) {
            }
        } else
            pico_report_error(8, 1, 0);
    }
#endif
    pico_api_report_state(&a->network);
    pico_device_detect_stepper(&a->device);
    pico_rfid_application_initialize(&a->rfid);
    if (pico_recipe_session_restore(&a->recipe, &a->state.recipe)) {
#if PICO_STANDALONE
        /* Looking at the offer must neither consume the checkpoint nor start
         * a history. Power can fail again while the user checks the machine. */
        if (!pico_ui_confirm_continue_or_abort()) {
            pico_storage_delete_restart(&a->network);
            a->recipe.recovery_phase = 0;
            goto final_startup;
        }
#else
        pico_storage_delete_restart(&a->network);
        if (!pico_ui_confirm_continue_or_abort())
            goto final_startup;
#endif
        if (a->state.recipe.program->session_type == 1) {
            pico_screen_loading(0);
            pico_recipe_run_control_program(&a->recipe, 7);
        } else {
#if PICO_STANDALONE
            if (!pico_recipe_session_start(&a->recipe, true))
                goto final_startup;
            a->state.device_state = 3;
#endif
            pico_recipe_run(&a->recipe, &a->state.recipe, true);
        }
        pico_recipe_rinse(&a->recipe);
    }
#if !PICO_STANDALONE
    pico_storage_delete_restart(&a->network);
#endif
final_startup:
#if !PICO_STANDALONE
    pico_update_run(&a->network);
    pico_recipe_run_required_maintenance(&a->recipe);
#else
    a->recipe.recovery_phase = 0;
    /* Firmware updates are an explicit physical deployment operation. */
    (void)a;
#endif
}

/* Original main menu0x20012e10 and loop tail entry0x20012e9c. */
void pico_application_loop(PicoApplication *a)
{
    static const uint32_t actions[] = {12, 13, 14, 15, 16, 17};
    pico_display_clear();
    pico_display_logo();
    pico_menu_populate_action_labels(actions, 6, a->settings.language, a->settings.detailed_status);
#if PICO_STANDALONE
    if (pico_local_control_stopped()) {
        pico_control_stop(a->control);
        pico_actuators_all_off(a->actuators);
        a->state.controller.phase = PICO_BREW_PHASE_IDLE;
        a->state.controller.status = 0;
        a->state.device_state = 2;
        pico_storage_delete_restart(&a->network);
    }
    pico_local_control_enter_menu();
#endif
    uint32_t selection = pico_menu_select_action(4, 0);
#if PICO_STANDALONE
    if (pico_local_control_has_pending()) {
        pico_local_control_run_pending();
        return;
    }
    if (pico_local_control_stopped())
        return;
    pico_local_control_leave_menu();
#endif
    switch (selection) {
    case 12:
        pico_recipe_run_brew_program(&a->recipe, &a->state.recipe);
        break;
    case 13:
        pico_recipe_run_cooking_program(&a->recipe, 3);
        break;
    case 14:
        pico_recipe_run_cooking_program(&a->recipe, 4);
        break;
    case 15:
        pico_recipe_manual_program(&a->recipe);
        break;
    case 16:
        pico_utilities_menu();
        break;
    case 17:
        pico_settings_menu();
        break;
    default:
        log_message(a, "E", "MENU", "Unsupported action");
        break;
    }
}
