#ifndef PICO_NETWORK_ADDRESSES_H
#define PICO_NETWORK_ADDRESSES_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int IPAddress_printTo(int address, void *output);
uint32_t *IPAddress_construct_zero(uint32_t *address);
void IPAddress_construct_octets(uint32_t *address, uint8_t octet0, uint8_t octet1, uint8_t octet2,
                                uint8_t octet3);
int IPAddress_assign_word(int address, uint32_t value);
void initialize_global_any_ip_address(void);
int MACAddress_printTo(int address, void *output);
void initialize_global_any_mac_address(void);

#endif /* PICO_NETWORK_ADDRESSES_H */
