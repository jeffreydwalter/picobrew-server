#ifndef PICO_CC3200_CLOCK_H
#define PICO_CC3200_CLOCK_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void PRCMMCUReset(bool b_include_subsystem);
uint32_t PRCMSysResetCauseGet(void);
void PRCMPeripheralClkEnable(uint32_t ul_peripheral, uint32_t ul_clk_flags);
void PRCMPeripheralClkDisable(uint32_t ul_peripheral, uint32_t ul_clk_flags);
uint32_t PRCMPeripheralClockGet(uint32_t ul_peripheral);
void PRCMPeripheralReset(uint32_t ul_peripheral);
bool PRCMPeripheralStatusGet(uint32_t ul_peripheral);
void PRCMCC3200MCUInit(void);

#endif /* PICO_CC3200_CLOCK_H */
