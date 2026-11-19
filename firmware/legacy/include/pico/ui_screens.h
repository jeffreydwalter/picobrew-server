#ifndef PICO_UI_SCREENS_H
#define PICO_UI_SCREENS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_ui_draw_footer_option(char *text, int has_more_options);
void pico_ui_draw_hold_status(void);
void pico_ui_draw_footer_single(char *text);
void pico_ui_draw_paused(void);
void pico_ui_draw_stop_footer(void);
void pico_ui_draw_circulation_status(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3);
void pico_ui_draw_continue_footer(void);
void pico_ui_draw_separator(short margin_columns, short row, int line_count);
void pico_ui_draw_header(char *text);
void pico_ui_draw_header_at_row(char *text, short row);
void pico_ui_draw_descale_preparation(void);
void pico_ui_draw_descale_rinse_preparation(void);
void pico_ui_draw_subheader(char *text);
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
void pico_ui_draw_instruction_screen(uint32_t screen_id, uint32_t language, char *title,
                                     int use_context_title);
void pico_ui_draw_instruction_with_header(uint32_t screen_id, uint32_t language, char *title);
void pico_ui_draw_instruction_with_context(uint32_t screen_id, uint32_t language, char *title);
void pico_ui_draw_password_heading(void);
void pico_ui_draw_rack_beer_heading(void);
void pico_ui_draw_stop_confirmation(void);
void pico_ui_draw_skip_step_confirmation(void);
void pico_ui_draw_error(int argument_1);
void pico_ui_draw_insert_picopak(void);
void pico_ui_draw_abv_adjustment(void);
void pico_ui_draw_bitterness_adjustment(void);
void pico_ui_draw_brew_complete(char *recipe_title);
int pico_ui_draw_recipe_choices(char *recipe_title, int allow_adjustments);
void pico_ui_draw_picopak_selection(void);
void pico_ui_draw_registration_code(char *registration_code);
void pico_ui_show_wifi_test_result(uint32_t diagnostic_stage);
char *pico_title_label(uint32_t title_id, uint32_t language);
bool pico_title_id_is_valid(uint32_t title_id);
char *pico_brew_display_state_label(uint32_t phase, uint32_t language);

#endif /* PICO_UI_SCREENS_H */
