#ifndef PICO_NETWORK_API_H
#define PICO_NETWORK_API_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int pico_wifi_check_status(int background, int resolve_server);
void pico_api_close_connection(void);
int pico_read_api_response(char *buffer, uint32_t capacity, uint32_t *length, int close_after,
                           uint8_t terminator);
int pico_wifi_wait_for_local_ip(uint32_t timeout_ms, int show_error);
int pico_wifi_resolve_server(void);
void pico_api_show_error(char *context);
int pico_api_open_request(char *request, int keep_open, int background);
void pico_api_report_device_state(void);
int pico_api_report_error(int error_code);
uint32_t pico_wifi_ensure_connection(void);
int pico_api_read_hash_response(char *buffer, uint32_t capacity, uint32_t *length);
uint32_t pico_api_register_device(void);
int pico_api_get_session_id(char *session_id, uint32_t capacity);
uint32_t pico_api_create_picopak(int recipe_id);
int pico_api_get_required_actions(uint32_t *action_ids, uint32_t capacity);
int pico_api_read_hex_image(uint8_t *image, uint8_t terminator);
int pico_api_read_recipe_bitmap(uint8_t *image);
uint32_t pico_api_fetch_recipe(char *recipe_identifier, int ibu_adjustment, int abv_adjustment);
bool pico_api_fetch_default_recipe_failed(char *recipe_identifier);
uint32_t pico_api_wait_for_hash(void);
int pico_api_make_wifi_call(char *request, int wait_for_response, int background);
int pico_wifi_log_event(char *step_name, char *event_name);
int pico_wifi_log_step(char *step_name);
int pico_wifi_log_progress(char *step_name);
uint32_t pico_api_parse_recipe_list(int *total_items, int *page_offset, int *menu_item_count,
                                    int item_identifiers, char *request_and_record_buffer,
                                    int item_titles, int *loaded_items);
int pico_api_get_associated_picopaks(int *total_items, int *page_offset, int *menu_item_count,
                                     int item_identifiers, int item_titles, int *loaded_items);
int pico_api_get_recipe_list(int *total_items, int *page_offset, int *menu_item_count,
                             int item_identifiers);

#endif /* PICO_NETWORK_API_H */
