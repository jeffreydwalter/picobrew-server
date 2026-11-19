#ifndef PICO_SCREENS_H
#define PICO_SCREENS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_screen_error_heading(void);
void pico_screen_water_error_help(void);
void pico_screen_device_information(void);
void pico_screen_still_connecting(char *status_text);
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
void pico_screen_get_instruction_lines(PicoTextScreen *screen, uint32_t argument_2,
                                       uint32_t argument_3);
void pico_screen_still_setup_step(uint8_t *setup_step);
void pico_screen_draw_program_notice(void);
void pico_screen_show_program_notice(void);
void pico_screen_reinsert_step_filter(void);
void pico_screen_changing_step(void);
void pico_screen_setting_up_step(char *title);
void pico_screen_ending_step(char *title);
void pico_screen_still_readings(void);
uint32_t pico_screen_fill_reservoir_and_start(char *title);
void pico_screen_starting_program(char *title);
void pico_screen_ending_program(char *title);
void pico_screen_show_ending_program(char *title);
bool pico_screen_loading_picopak(void);
void pico_screen_splash(void);
void pico_screen_service_status(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                uint32_t argument_4);
void pico_screen_wifi_no_signal(void);
void pico_screen_wifi_connected(void);
void pico_screen_wifi_connection_failed(uint32_t argument_1, uint32_t argument_2,
                                        uint32_t argument_3, uint32_t argument_4);
void pico_screen_brew_name_page(void);
void pico_screen_recipe_metrics_page(void);
void pico_screen_step_progress_page(void);

#endif /* PICO_SCREENS_H */
