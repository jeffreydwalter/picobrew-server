#ifndef PICO_HARDWARE_CONTROL_H
#define PICO_HARDWARE_CONTROL_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint32_t pico_check_thermoblock_heating(int controller);
bool pico_stepper_is_ready(void);
uint8_t pico_stepper_wait_ready(void);
uint32_t pico_buzzer_on(void);
int pico_buzzer_off(void);
void pico_alarm_while_filter_missing(void);
void pico_alarm_until_encoder_press(void);
uint32_t pico_stepper_query_capabilities(uint32_t *reply_detail);
void pico_detect_stepper_configuration(void);
uint32_t pico_stepper_set_torque_mode(uint32_t torque_mode);
uint32_t pico_stepper_move_to_compartment(uint32_t compartment_id);
void pico_steam_pump_on(void);
void pico_drain_pump_on(void);
void pico_shuttle_pump_on(void);
uint32_t pico_pause_outputs_for_missing_filter(int resume_steam, int resume_drain,
                                               int resume_shuttle);
bool pico_heater_pump_timer_isr(void);
void pico_start_drain_and_steam_pumps(void);
void pico_controller_start_circulation(void);
uint32_t pico_handle_filter_removal(void);
int pico_prime_steam_pump(int controller, char *program_title);
void pico_stepper_move_home(void);
void pico_restore_idle_hardware(void);
bool pico_temperature_control_timer_isr(uint32_t argument_1, uint32_t argument_2);
uint32_t pico_reprime_steam_pump(int controller, char *program_title, char *step_name,
                                 int logging_enabled, char restart_circulation);
void pico_alarm_until_press_or_timeout(uint32_t language, uint32_t status_selector, int timeout_ms);

#endif /* PICO_HARDWARE_CONTROL_H */
