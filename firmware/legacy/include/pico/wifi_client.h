#ifndef PICO_WIFI_CLIENT_H
#define PICO_WIFI_CLIENT_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint32_t WiFiClient_write_byte(void *client, uint8_t value);
int WiFiClient_read_byte(void *client);
int WiFiClient_peek(void *client);
bool WiFiClient_is_valid(void *client);
int WiFiClient_connect_hostname(void *client, char *hostname, uint16_t port);
int WiFiClient_ssl_connect_hostname(void *client, char *hostname, uint16_t port);
void WiFiClient_flush(void *client);
int WiFiClient_available(void *client);
int WiFiClient_read_buffer(void *client, uint8_t *buffer, uint32_t count);
void WiFiClient_stop(void *client);
uint32_t WiFiClient_write_buffer(void *client, uint8_t *buffer, uint32_t count);
int WiFiClient_connect_ipv4_blocking(void *client, void *ipAddress, uint16_t port);
int WiFiClient_connect_ipv4(void *client, void *ipAddress, uint16_t port);
int WiFiClient_ssl_connect_ipv4(void *client, void *ipAddress, uint16_t port);
void WiFiClient_construct(void *client);
void *WiFiClient_destroy(void *client);
uint8_t WiFiClient_connected(void *client);
uint8_t WiFiClient_status(void *client);

#endif /* PICO_WIFI_CLIENT_H */
