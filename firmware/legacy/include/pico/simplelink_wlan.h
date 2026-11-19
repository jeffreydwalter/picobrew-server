#ifndef PICO_SIMPLELINK_WLAN_H
#define PICO_SIMPLELINK_WLAN_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

short sl_WlanDisconnect(void);
short sl_WlanPolicySet(uint8_t Type, uint8_t Policy, uint8_t *pVal, uint8_t val_len);
short sl_WlanProfileDel(short index);
short sl_WlanGetNetworkList(uint8_t Index, uint8_t count, void *pEntries);
short sl_WlanRxStatStart(void);
short sl_WlanSetMode(uint8_t mode);

#endif /* PICO_SIMPLELINK_WLAN_H */
