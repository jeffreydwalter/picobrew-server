#ifndef PICO_CC3200_TIMER_H
#define PICO_CC3200_TIMER_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void SysTickIntHandler(void);
void SysTickEnable(void);
void SysTickIntEnable(void);
void SysTickPeriodSet(uint32_t ul_period);
void TimerEnable(uint32_t ul_base, uint32_t ul_timer);
void TimerConfigure(uint32_t ul_base, uint32_t ul_config);
void TimerLoadSet(uint32_t ul_base, uint32_t ul_timer, uint32_t ul_value);
void TimerIntRegister(uint32_t ul_base, uint32_t ul_timer, PicoInterruptHandler *pfn_handler);

#endif /* PICO_CC3200_TIMER_H */
