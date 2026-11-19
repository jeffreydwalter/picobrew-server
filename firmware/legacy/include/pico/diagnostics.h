#ifndef PICO_DIAGNOSTICS_H
#define PICO_DIAGNOSTICS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_diagnostic_sensors(void);
void pico_report_error(int error_code, int allow_continue, int report_to_server);
void pico_diagnostic_drain_pump(void);
void pico_diagnostic_steam_pump(void);
void pico_diagnostic_heater(void);
void pico_diagnostic_stepper(void);
void pico_halt_heater_malfunction(void);
void pico_ui_toggle_stepper_torque(void);
void pico_ui_choose_compartment(void);
void pico_diagnostic_shuttle_pump(void);
int pico_diagnostic_buzzer(void);
void pico_diagnostic_rfid(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                          uint32_t argument_4);
void pico_diagnostic_run_all(void);
void pico_diagnostics_menu(void);
void pico_diagnostic_wifi(void);
void pico_ui_show_network_addresses(void);

#endif /* PICO_DIAGNOSTICS_H */
