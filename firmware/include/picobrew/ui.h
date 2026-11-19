#ifndef PICOBREW_UI_H
#define PICOBREW_UI_H

#include "picobrew/actuators.h"
#include "picobrew/control.h"
#include "picobrew/network.h"
#include "picobrew/state.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PICO_TEXT_LINE_CAPACITY = 6,
    PICO_DISPLAY_WIDTH = 128,
    PICO_DISPLAY_HEIGHT = 64,
    PICO_FRAMEBUFFER_SIZE = 1024,
    PICO_MENU_LABEL_LENGTH = 22
};

typedef struct PicoBitmap {
    const uint8_t *pixels;
    int32_t width, height;
} PicoBitmap;
typedef struct PicoTextScreen {
    uint32_t line_count;
    const char *lines[PICO_TEXT_LINE_CAPACITY];
} PicoTextScreen;
typedef struct PicoEncoder {
    uint8_t phase_a_pin, phase_b_pin, button_pin;
    uint8_t reserved_03[9];
    volatile uint8_t click_latch;
    uint8_t reserved_0d[3];
    int32_t minimum, maximum, previous_position, step;
    volatile int32_t position;
    uint8_t button_interrupt, phase_a_interrupt, phase_b_interrupt, reserved_27;
    uint8_t wrap_enabled, previous_phase, current_phase, last_stable_phase;
} PicoEncoder;
typedef struct PicoDisplay {
    int16_t width, height, cursor_x, cursor_y;
    uint16_t foreground, background;
    uint8_t text_scale, rotation, wrap, codepage_437;
    uint8_t command_data_pin, chip_select_pin, shared_chip_select_pin;
    uint8_t framebuffer[PICO_FRAMEBUFFER_SIZE];
} PicoDisplay;

typedef struct PicoUiHardware {
    uint32_t (*millis)(void *context);
    void (*delay_ms)(void *context, uint32_t milliseconds);
    void (*pin_mode)(void *context, unsigned pin, unsigned mode);
    int (*digital_read)(void *context, unsigned pin);
    void (*digital_write)(void *context, unsigned pin, unsigned value);
    void (*spi_begin)(void *context);
    uint8_t (*spi_transfer)(void *context, uint8_t value);
    void (*interrupts_disable)(void *context);
    bool (*interrupts_enable)(void *context);
    void (*attach_interrupt)(void *context, unsigned interrupt, void (*handler)(void),
                             unsigned mode);
} PicoUiHardware;

/* Recipe and mechanical workflows use their owning component; UI owns drawing
 * and interaction. Every referenced service is required in the complete app. */
typedef struct PicoUiServices {
    void (*pause_until_resume)(void *context);
    void (*wait_for_filter)(void *context);
    void (*run_first_rinse)(void *context);
    void (*run_rinse)(void *context);
    void (*run_circulation_program)(void *context);
    void (*run_rack_beer)(void *context);
    void (*run_descale)(void *context);
    void (*run_still)(void *context);
    void (*drain_reservoir)(void *context);
    void (*drain_step_filter)(void *context);
    void (*run_circulation)(void *context);
    uint32_t (*execute_instruction)(void *context, uint32_t instruction, bool sound_alarm);
    bool (*fetch_default_recipe_failed)(void *context, const char *identifier);
    bool (*buzzer)(void *context, bool enabled);
    void (*alarm_filter_missing)(void *context);
    void (*alarm_until_press)(void *context);
    void (*stepper_move)(void *context, uint8_t compartment);
    bool (*stepper_torque)(void *context, unsigned mode);
    void (*restore_idle)(void *context);
    void (*flush_compartments)(void *context, int cycles, int stop_on_click);
    void (*flush_compartments_until_press)(void *context);
    void (*heat_flush_compartments)(void *context, unsigned mode, double fahrenheit);
    bool (*rfid_read_uid)(void *context, char uid[22]);
} PicoUiServices;

typedef struct PicoUi {
    PicoAppState *app;
    PicoNetwork *network;
    PicoActuators *actuators;
    PicoControl *control;
    const PicoUiHardware *hardware;
    void *hardware_context;
    const PicoUiServices *services;
    void *service_context;
    PicoDisplay display;
    PicoEncoder encoder;
    char menu_labels[PICO_MENU_CAPACITY][PICO_MENU_LABEL_LENGTH];
} PicoUi;

/* The original appliance has one OLED and one rotary encoder. This explicit
 * binding supplies that singleton while retaining independently owned memory.
 */
void pico_ui_bind(PicoUi *ui);
PicoUi *pico_ui_context(void);
void pico_ui_initialize(PicoUi *ui, PicoAppState *app, PicoNetwork *network,
                        PicoActuators *actuators, PicoControl *control,
                        const PicoUiHardware *hardware, void *hardware_context,
                        const PicoUiServices *services, void *service_context);
void pico_encoder_rotation_isr(void);
void pico_encoder_button_isr(void);
void pico_ui_network_event(void *context, PicoNetworkUiEvent event, const char *text);
void pico_ui_recipe_logo_ready(void *context, const uint8_t image[1024]);
void pico_ui_recipe_image_ready(void *context, unsigned index, const uint8_t image[1024]);
extern const PicoBitmap pico_ui_asset_program_primary;
extern const PicoBitmap pico_ui_asset_program_secondary;
extern const PicoBitmap pico_ui_asset_sousvide_secondary;

/* Graphics operations are public so host tools can inspect/render the exact
 * native font, monochrome layout and clipping rules without an OLED. */
void pico_graphics_pixel(PicoDisplay *display, int x, int y, int color);
void pico_graphics_bitmap(PicoDisplay *display, int16_t x, int16_t y, const uint8_t *pixels,
                          int16_t width, int16_t height, uint16_t color);
void pico_graphics_fill_rectangle(PicoDisplay *display, int16_t x, int16_t y, int16_t width,
                                  int16_t height, uint16_t color);
void pico_graphics_line(PicoDisplay *display, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        uint16_t color);
void pico_graphics_round_rectangle(PicoDisplay *display, int16_t x, int16_t y, int16_t width,
                                   int16_t height, int16_t radius, uint16_t color);
void pico_graphics_fill_triangle(PicoDisplay *display, int16_t x0, int16_t y0, int16_t x1,
                                 int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void pico_graphics_write(PicoDisplay *display, uint8_t character);
void pico_graphics_text(PicoDisplay *display, const char *text);
void pico_graphics_cursor(PicoDisplay *display, int16_t x, int16_t y);
void pico_graphics_text_color(PicoDisplay *display, uint16_t color);
void pico_graphics_text_colors(PicoDisplay *display, uint16_t foreground, uint16_t background);
void pico_graphics_text_scale(PicoDisplay *display, uint8_t scale);
void pico_graphics_rotation(PicoDisplay *display, uint8_t rotation);
void pico_oled_command(PicoDisplay *display, uint8_t command);
void pico_oled_initialize(PicoDisplay *display);
void pico_oled_invert(PicoDisplay *display, bool invert);

void pico_display_clear(void);
void pico_display_flush(void);
void pico_display_center_bitmap(short y, const PicoBitmap *bitmap);
void pico_display_center_message(const char *text);
void pico_display_construct(void);
void pico_display_refresh_current_page(void);
void pico_screen_set_recipe_name(PicoScreenState *screen, const char *recipe_name);
void pico_screen_set_step(PicoScreenState *screen, uint32_t component, const char *step_name);
void pico_screen_add_lines(PicoTextScreen *screen, int count, ...);
void pico_display_clear_thunk(void);
void pico_display_advance_carousel(PicoPageCarousel *carousel);
const uint8_t *pico_display_digit_bitmap(int digit);
void pico_display_initialize(void);
void pico_display_text_at_cell(short row, short column, const char *text);
void pico_display_string_at_cell(short row, short column, const char *text);
void pico_display_center_text(short row, const char *text);
void pico_display_text_at_pixels(short y, short x, const char *text);
void pico_display_device_bitmap(void);
void pico_display_logo(void);
void pico_display_right_aligned_text(short row, short column, const char *text,
                                     uint32_t field_width, char padding_character);
void pico_display_character_on_row(short x, short row, char character);
void pico_display_character_at_pixels(short x, short y, char character);
void pico_display_draw_content_frame(void);
void pico_display_draw_titled_frame(const char *title);
void pico_display_draw_program_frame(void);
void pico_display_recipe_decimal_metric(void);
void pico_display_recipe_integer_metric(void);
void pico_display_target_temperature(short row);
void pico_display_large_timer(int total_minutes, uint32_t seconds, int show_seconds);
void pico_display_large_temperature(double fahrenheit, int show_decimal);
void pico_display_large_timer_with_seconds(int total_minutes, uint32_t seconds);
void pico_display_large_temperature_with_decimal(double fahrenheit);
void pico_display_stop_progress_indicator(PicoControlSignals *signals);
void pico_display_device_bitmap_variant_a(void);
void pico_display_device_bitmap_variant_b(void);
void pico_display_clear_row(short row);
void pico_display_update_timer(short row, int started_ms, int duration_seconds);
void pico_display_clear_rows(uint32_t first_row, uint32_t last_row);
void pico_display_valve_position(short row, uint32_t position);
void pico_display_temperature_columns(short row, double wort, double steam, double target);
void pico_display_temperature_summary(double wort, double steam, double target, uint16_t row);
void pico_display_clear_row_from_column(short row, short column);
void pico_display_adjustment_box(int box_style, uint32_t box_index);

void pico_encoder_set_step(PicoEncoder *encoder, int step);
void pico_encoder_set_range(PicoEncoder *encoder, int minimum, int maximum, int initial_position);
void pico_encoder_enable_wrap(PicoEncoder *encoder);
void pico_encoder_set_position(PicoEncoder *encoder, int position);
int pico_encoder_get_position(PicoEncoder *encoder);
int pico_encoder_take_click(PicoEncoder *encoder);
void pico_encoder_clear_click(PicoEncoder *encoder);
void pico_encoder_initialize(PicoEncoder *encoder, uint8_t phase_a_pin, uint8_t phase_b_pin,
                             uint8_t button_pin, uint8_t button_interrupt,
                             uint8_t phase_a_interrupt, uint8_t phase_b_interrupt);
void pico_encoder_handle_rotation(PicoEncoder *encoder);
void pico_encoder_reset_state(void);
void pico_display_wait_for_click(void);
void pico_encoder_edit_text(char *text, int capacity);
uint32_t pico_encoder_select_adjustment_level(uint32_t baseline_level, int32_t *stored_level,
                                              bool *changed);

void pico_screen_error_heading(void);
void pico_screen_water_error_help(void);
void pico_screen_device_information(void);
void pico_screen_still_connecting(const char *status_text);
void pico_screen_still_exiting(void);
void pico_screen_wifi_searching(void);
void pico_screen_ending_process(void);
void pico_screen_preheating_steam(short row);
bool pico_screen_loading(int is_picopak);
void pico_screen_updating_firmware(void);
void pico_screen_downloading_firmware(void);
void pico_screen_firmware_update_failed(void);
void pico_screen_rinse_complete(void);
void pico_screen_help_address(void);
void pico_screen_show_help_temporarily(void);
void pico_screen_ending_first_rinse(void);
void pico_screen_get_instruction_lines(PicoTextScreen *screen, uint32_t screen_id,
                                       uint32_t language);
void pico_screen_still_setup_step(uint8_t *setup_step);
void pico_screen_draw_program_notice(void);
void pico_screen_show_program_notice(void);
void pico_screen_reinsert_step_filter(void);
void pico_screen_changing_step(void);
void pico_screen_setting_up_step(const char *title);
void pico_screen_ending_step(const char *title);
void pico_screen_still_readings(void);
uint32_t pico_screen_fill_reservoir_and_start(const char *title);
void pico_screen_starting_program(const char *title);
void pico_screen_ending_program(const char *title);
void pico_screen_show_ending_program(const char *title);
bool pico_screen_loading_picopak(void);
void pico_screen_splash(void);
void pico_screen_service_status(void);
void pico_screen_wifi_no_signal(void);
void pico_screen_wifi_connected(void);
void pico_screen_wifi_connection_failed(void);
void pico_screen_brew_name_page(void);
void pico_screen_recipe_metrics_page(void);
void pico_screen_step_progress_page(void);

uint32_t pico_menu_populate_still_fan(void);
void pico_menu_initialize(PicoMenu *menu);
uint32_t pico_menu_action_at(PicoMenu *menu, int index);
void pico_menu_add_action(PicoMenu *menu, uint32_t action_id);
void pico_menu_set_actions(PicoMenu *menu, const uint32_t *actions, uint32_t count);
void pico_menu_populate_action_labels(const uint32_t *actions, uint32_t count, uint32_t language,
                                      uint32_t status_selector);
void pico_menu_populate_from_menu(const PicoMenu *source_menu, uint32_t language,
                                  uint32_t status_selector);
uint32_t pico_menu_populate_hardware_tests(void);
void pico_menu_append_wifi_search_actions(int *option_count);
uint32_t pico_menu_populate_service(void);
uint32_t pico_menu_populate_network(void);
uint32_t pico_menu_populate_settings(void);
uint32_t pico_menu_populate_languages(void);
uint32_t pico_menu_populate_in_brew_options(void);
void pico_menu_set_load_more_label(int row_index);
uint32_t pico_menu_populate_post_brew(void);
uint32_t pico_menu_populate_still_actions(int menu_kind);

void pico_label_insert_step_filter(char *destination);
void pico_label_adjust_recipe(char *destination);
void pico_label_done(char *destination);
void pico_label_confirm(char *destination);
void pico_label_continue(char *destination);
void pico_label_exit(char *destination);
void pico_label_back(char *destination);
void pico_label_ok(char *destination);
void pico_label_yes(char *destination);
void pico_label_no(char *destination);
void pico_label_compartment(char *destination, uint32_t compartment);
void pico_label_service(char *destination);
void pico_label_heating(char *destination);
void pico_label_cooling(char *destination);
void pico_label_holding(char *destination);
void pico_label_draining(char *destination);
void pico_label_start_brewing(char *destination);
void pico_label_cancel(char *destination);
void pico_label_next_uppercase(char *destination);
void pico_label_exit_uppercase(char *destination);

void pico_ui_draw_menu_items(uint32_t item_count, int selected_row, int first_row,
                             uint32_t first_visible_item);
uint32_t pico_menu_choose_with_timeout(uint32_t first_menu_row, uint32_t choice_count,
                                       uint32_t initial_index, int use_timeout,
                                       uint8_t default_index, uint8_t redraw, int timeout_seconds);
uint32_t pico_menu_choose_default_timeout(uint32_t first_menu_row, uint32_t choice_count,
                                          uint32_t initial_index, int use_timeout,
                                          uint8_t default_index, uint8_t redraw);
uint32_t pico_menu_choose(uint32_t first_menu_row, uint32_t choice_count, uint32_t initial_index);
uint32_t pico_menu_select_action(uint32_t first_menu_row, int initial_index);
bool pico_ui_choose_binary_option(const char *title);
bool pico_ui_confirm_at_row(uint32_t menu_row);
void pico_ui_network_menu(void);
void pico_settings_menu(void);
void pico_ui_service_menu(void);
void pico_utilities_menu(void);

void pico_ui_draw_footer_option(const char *text, int has_more_options);
void pico_ui_draw_hold_status(void);
void pico_ui_draw_footer_single(const char *text);
void pico_ui_draw_paused(void);
void pico_ui_draw_stop_footer(void);
void pico_ui_draw_circulation_status(double wort, uint32_t elapsed_seconds);
void pico_ui_draw_continue_footer(void);
void pico_ui_draw_separator(short margin_columns, short row, int line_count);
void pico_ui_draw_header(const char *text);
void pico_ui_draw_header_at_row(const char *text, short row);
void pico_ui_draw_descale_preparation(void);
void pico_ui_draw_descale_rinse_preparation(void);
void pico_ui_draw_subheader(const char *text);
void pico_ui_draw_heater_malfunction(void);
void pico_ui_draw_descale_heading(void);
void pico_ui_draw_language_menu(void);
uint32_t pico_ui_draw_units_menu(void);
void pico_ui_draw_drain_filter(void);
void pico_ui_draw_drain_reservoir(void);
void pico_ui_draw_choice_question(void);
void pico_ui_draw_first_rinse_heading(void);
void pico_ui_draw_choose_network(void);
void pico_ui_draw_wifi_test_heading(void);
void pico_ui_draw_utilities_heading(void);
void pico_ui_draw_settings_heading(void);
void pico_ui_draw_current_program_heading(void);
void pico_ui_draw_network_heading(void);
void pico_ui_draw_factory_reset_heading(void);
void pico_ui_draw_firmware_reset_question(void);
void pico_ui_draw_clear_wifi_question(void);
void pico_ui_draw_startup_heading(void);
void pico_ui_draw_deep_clean_heading(void);
void pico_ui_draw_instruction_screen(uint32_t screen_id, uint32_t language, const char *title,
                                     int use_context_title);
void pico_ui_draw_instruction_with_header(uint32_t screen_id, uint32_t language, const char *title);
void pico_ui_draw_instruction_with_context(uint32_t screen_id, uint32_t language,
                                           const char *title);
void pico_ui_draw_password_heading(void);
void pico_ui_draw_rack_beer_heading(void);
void pico_ui_draw_stop_confirmation(void);
void pico_ui_draw_skip_step_confirmation(void);
void pico_ui_draw_error(int error_code);
void pico_ui_draw_insert_picopak(void);
void pico_ui_draw_abv_adjustment(void);
void pico_ui_draw_bitterness_adjustment(void);
void pico_ui_draw_brew_complete(const char *recipe_title);
int pico_ui_draw_recipe_choices(const char *recipe_title, int allow_adjustments);
void pico_ui_draw_picopak_selection(void);
void pico_ui_draw_registration_code(const char *registration_code);
void pico_ui_show_wifi_test_result(uint32_t diagnostic_stage);
const char *pico_title_label(uint32_t title_id, uint32_t language);
bool pico_title_id_is_valid(uint32_t title_id);
const char *pico_brew_display_state_label(uint32_t phase, uint32_t language);

void pico_ui_wait_for_resume(void);
void pico_ui_wait_for_footer_press(const char *text);
void pico_ui_prompt_deep_clean_setup(void);
void pico_ui_wait_for_filter_insertion(void);
void pico_ui_enter_wifi_password(void);
void pico_ui_select_scanned_network(uint32_t network_index);
uint32_t pico_ui_choose_language(void);
uint32_t pico_ui_choose_units(void);
uint32_t pico_ui_choose_program(void);
uint32_t pico_ui_walk_program_steps(int program_id);
void pico_ui_require_first_rinse(void);
int pico_ui_prompt_vacuum(int stage);
uint32_t pico_ui_wait_for_continue(void);
void pico_prompt_insert_filter_with_alarm(void);
uint32_t pico_run_drain_instructions(uint32_t second_screen_id);
void pico_ui_manual_network_step(int stage);
void pico_ui_prompt_picopak_association(void);
void pico_ui_show_rinse_finished(void);
void pico_ui_first_rinse_step(uint32_t stage);
void pico_ui_wait_for_stop(void);
void pico_ui_rack_beer_step(uint32_t stage);
bool pico_ui_descale_step(uint32_t stage);
bool pico_ui_confirm_screen(void (*draw_screen)(void));
int pico_ui_confirm_thermal_recovery(int controller_status);
int pico_ui_confirm_continue_or_abort(void);
void pico_ui_enter_manual_network(void);
uint32_t pico_ui_scan_and_connect_wifi(void);
int pico_ui_adjust_recipe(PicoRecipe *recipe);
void pico_ui_start_descale(void);
uint32_t pico_ui_select_recipe(PicoRecipe *recipe);
uint32_t pico_ui_run_instruction_screen(uint32_t screen_id, int sound_alarm, int program_id,
                                        const char *title, uint8_t use_context_title);
int pico_ui_run_instruction_sequence(const uint32_t *screen_ids, int count, int sound_first_alarm,
                                     int program_id, const char *title, uint8_t use_context_title);
uint32_t pico_get_temperature_input(uint32_t action_id, const char *program_title,
                                    uint32_t fahrenheit);
int pico_get_time_input(uint32_t action_id, const char *program_title, int minutes);

const char *pico_action_name(uint32_t action_id);
const char *pico_action_label(uint32_t action_id, uint32_t language, uint32_t status_selector);

void pico_copy_firmware_version(char *destination);
void pico_append_program_state_label(char *destination);
void pico_append_program_name(char *destination);

char *pico_format_float_decimal(float value, char *destination, uint32_t decimal_places);

void pico_diagnostic_sensors(void);
void pico_report_error(int error_code, int allow_continue, int report_to_server);
void pico_diagnostic_drain_pump(void);
void pico_diagnostic_steam_pump(void);
void pico_diagnostic_heater(void);
void pico_diagnostic_stepper(void);
void pico_halt_heater_malfunction(void);
void pico_alarm_until_press_or_timeout(uint32_t language, uint32_t status_selector, int timeout_ms);
void pico_ui_toggle_stepper_torque(void);
void pico_ui_choose_compartment(void);
void pico_diagnostic_shuttle_pump(void);
int pico_diagnostic_buzzer(void);
void pico_diagnostic_rfid(void);
void pico_diagnostic_run_all(void);
void pico_diagnostics_menu(void);
void pico_diagnostic_wifi(void);
void pico_ui_show_network_addresses(void);

void pico_ui_confirm_still_cleaned(void);
void pico_ui_show_still_communication_failure(void);
void pico_ui_show_still_completion(void);
void pico_ui_show_still_temperature_warning(void);
void pico_ui_still_fan_menu(void);
void pico_screen_thermal_recovery_help(void);
void pico_screen_basic_brew_status(void);
void pico_screen_detailed_brew_status(void);
void pico_screen_temperature_page(void);
void pico_screen_current_step_page(void);
void pico_screen_program_status_page(void);
void pico_screen_manual_overview_page(void);
void pico_screen_current_target_page(void);
void pico_ui_draw_deep_clean_confirmation(void);
int pico_ui_choose_still(void *context, const char identifiers[][22], size_t count);

#ifdef __cplusplus
}
#endif
#endif
