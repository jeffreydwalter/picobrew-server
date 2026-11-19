#ifndef PICO_NETWORK_H
#define PICO_NETWORK_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint16_t pico_parse_ipv4_address(int text, int ip_address);
uint32_t pico_wifi_ensure_started_thunk(void);
char pico_wifi_connect_access_point(void);
uint32_t pico_wifi_connect_stored_network(void);
int pico_wifi_connect_with_screen(void);
int pico_wifi_reconnect(void);
bool pico_wifi_resolve_server_address(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                      int argument_4);
uint8_t pico_wifi_ping_public_dns(void);
void pico_api_write_base_path(char *url);
void pico_api_append_device_uid(char *url);
void pico_http_write_request_headers(uint32_t argument_1, int argument_2, uint32_t argument_3,
                                     uint32_t argument_4);

#endif /* PICO_NETWORK_H */
