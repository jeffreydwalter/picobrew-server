#ifndef PICO_SIMPLELINK_NETCFG_H
#define PICO_SIMPLELINK_NETCFG_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

long sl_NetCfgSet(uint8_t config_id, uint8_t config_opt, uint8_t config_len, uint8_t *pValues);
long sl_NetCfgGet(uint8_t config_id, uint8_t *pConfigOpt, uint8_t *pConfigLen, uint8_t *pValues);

#endif /* PICO_SIMPLELINK_NETCFG_H */
