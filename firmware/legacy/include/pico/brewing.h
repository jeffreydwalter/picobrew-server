#ifndef PICO_BREWING_H
#define PICO_BREWING_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_controller_set_brew_phase(int controller, int brew_phase);
void pico_controller_clear_status(int controller);
void pico_controller_pause_until_resume(int controller);
void pico_end_brew_run_outputs_for(int duration_ms, char *program_title, int drain_enabled,
                                   int shuttle_enabled);
void pico_finish_brew_step(int controller, char *program_title);
void pico_vacuum_compartments(uint32_t argument_1, uint32_t compartment_id,
                              uint32_t first_compartment);
void pico_run_circulation(uint32_t *wort_temperature_words);
void pico_drain_reservoir(void);
void pico_flush_compartments(uint32_t minimum_cycles, int stop_automatically);
void pico_flush_compartments_until_press(void);
void pico_prepare_brew_step_temperature(int controller, int target_fahrenheit, char *program_title);
void pico_heat_flush_compartments(int steam_enabled, uint32_t argument_2,
                                  uint32_t target_fahrenheit_low, uint32_t target_fahrenheit_high);
void pico_run_rinse_cycle(uint32_t argument_1);
bool pico_first_rinse_cool_and_flush(void);
uint32_t pico_first_rinse_heat_test(void);
void pico_drain_step_filter(void);
void pico_run_first_rinse(void);
void pico_run_rack_beer(void);
void pico_run_descale_program(void);
int pico_run_brew_step(void *controller, uint32_t target_fahrenheit, uint32_t active_minutes,
                       uint32_t hold_minutes, uint8_t compartment_id, char *program_title,
                       char *step_name, uint8_t logging_enabled, int remaining_minutes,
                       int step_index, int session_type);
uint32_t pico_run_recipe_steps(void *recipe, int logging_enabled);
void pico_manual_brew_program(void);
uint32_t pico_run_still_program(void);
uint32_t pico_run_control_program(int program_id);
void pico_run_deep_clean_program(void);
void pico_run_required_maintenance(void);
void pico_run_circulation_program(void);
void pico_sousvide_coldbrew_program(uint32_t program_id);
void pico_run_brew_program(void *recipe);

#endif /* PICO_BREWING_H */
