#ifndef PICO_PICOSTILL_H
#define PICO_PICOSTILL_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_still_write_request(uint32_t action_id);
int pico_still_call(uint32_t action_id, char *response, uint32_t capacity);
bool pico_still_send_wifi_credentials(void);
short pico_still_get_ip_address(void);
int pico_still_connect_to_lan(void);
uint32_t pico_still_read_status(uint8_t *controller);
bool pico_still_has_blocking_status(uint8_t *controller);
uint32_t pico_still_select_access_point_2(void);
void pico_ui_confirm_still_cleaned(void);
int pico_still_connect_wifi_network(int network_mode);
void pico_ui_show_still_communication_failure(void);
void pico_ui_show_still_completion(void);
void pico_ui_show_still_temperature_warning(void);
bool pico_still_wait_for_response(int network_mode, uint32_t initial_delay_seconds,
                                  uint32_t timeout_seconds);
uint8_t pico_still_configure(void);
bool pico_still_start_session_request(void);
bool pico_still_start_session_once(int still_controller);
uint32_t pico_still_stop(int still_controller);
void pico_still_set_fan(int fan_enabled);
void pico_ui_still_fan_menu(void);
int pico_api_can_use_still(char *still_uid);
int pico_api_has_cleaned_ack(char *still_uid);
int pico_api_set_cleaned_ack(char *still_uid);

#endif /* PICO_PICOSTILL_H */
