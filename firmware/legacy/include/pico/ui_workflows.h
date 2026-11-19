#ifndef PICO_UI_WORKFLOWS_H
#define PICO_UI_WORKFLOWS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_ui_wait_for_resume(void);
void pico_ui_wait_for_footer_press(char *text);
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
bool pico_ui_confirm_screen(uint8_t *draw_screen);
int pico_ui_confirm_thermal_recovery(int controller_status);
int pico_ui_confirm_continue_or_abort(void);
void pico_ui_enter_manual_network(void);
uint32_t pico_ui_scan_and_connect_wifi(uint32_t argument_1, uint32_t argument_2,
                                       uint32_t argument_3);
int pico_ui_adjust_recipe(void *recipe);
void pico_ui_start_descale(void);
uint32_t pico_ui_select_recipe(int recipe);
uint32_t pico_ui_run_instruction_screen(uint32_t screen_id, int sound_alarm, int program_id,
                                        char *title, uint8_t use_context_title);
int pico_ui_run_instruction_sequence(uint32_t *screen_ids, int count, int sound_first_alarm,
                                     int program_id, char *title, uint8_t use_context_title);
uint32_t pico_get_temperature_input(uint32_t action_id, char *program_title, uint32_t fahrenheit);
int pico_get_time_input(uint32_t action_id, char *program_title, int minutes);

#endif /* PICO_UI_WORKFLOWS_H */
