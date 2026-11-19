#ifndef PICO_WIFI_H
#define PICO_WIFI_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void destroy_cached_wifi_clients(void);
uint32_t WiFi_initialize(void);
uint32_t WiFi_find_unused_socket(void);
void WiFi_enable_dhcp_if_unconfigured(uint32_t argument_1, uint32_t argument_2,
                                      uint32_t argument_3);
void WiFi_set_dns(uint32_t argument_1, int dnsAddress);
uint32_t WiFi_disconnect(void);
uint32_t *WiFi_get_mac_address(uint32_t argument_1, uint32_t *macAddress, uint32_t argument_3,
                               uint32_t argument_4);
uint32_t *WiFi_get_local_ip(uint32_t *resultAddress);
void WiFi_get_ip_configuration(uint32_t argument_1, int localAddress, int dnsAddress,
                               int gatewayAddress, int subnetMask);
int WiFi_scan_networks(void);
uint8_t *WiFi_scan_result_ssid(uint32_t argument_1, int networkIndex, uint32_t argument_3,
                               uint32_t argument_4);
uint32_t WiFi_scan_result_security(uint32_t argument_1, int networkIndex, uint32_t argument_3,
                                   uint32_t argument_4);
uint8_t WiFi_status(void);
uint8_t WiFi_begin_with_passphrase(uint32_t argument_1, char *ssid, uint32_t passphrase,
                                   char *argument_4);
int WiFi_host_by_name(uint32_t argument_1, char *hostname, int resultAddress);
void WiFi_register_connected_device(uint32_t *ipAddress, uint32_t *macAddress);
void WiFi_unregister_connected_device(void *macAddress);
uint8_t *initialize_global_wifi(void);

#endif /* PICO_WIFI_H */
