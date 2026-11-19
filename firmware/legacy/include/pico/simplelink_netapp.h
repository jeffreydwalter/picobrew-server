#ifndef PICO_SIMPLELINK_NETAPP_H
#define PICO_SIMPLELINK_NETAPP_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

short sl_NetAppStop(uint32_t AppBitMap);
short sl_NetAppMDNSRegisterUnregisterService(char *pServiceName, uint8_t service_name_len,
                                             char *pText, uint8_t text_len, uint16_t Port,
                                             uint32_t TTL, uint32_t Options);
short sl_NetAppDnsGetHostByName(char *hostname, uint16_t usNameLen, uint32_t *out_ip_addr,
                                uint8_t family);
int sl_NetAppPingStart(uint32_t *pingParameters, int addressFamily, uint32_t *report, int callback);

#endif /* PICO_SIMPLELINK_NETAPP_H */
