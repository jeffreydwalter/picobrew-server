#ifndef PICO_CC3200_INTERRUPTS_H
#define PICO_CC3200_INTERRUPTS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint32_t CPUcpsid(void);
uint32_t CPUcpsie(void);
void IntVTableBaseSet(uint32_t ul_vtable_base);
void IntRegister(uint32_t ul_interrupt, PicoInterruptHandler *pfn_handler);
void IntUnregister(uint32_t ul_interrupt);
void IntPrioritySet(uint32_t ul_interrupt, uint8_t uc_priority);
void IntEnable(uint32_t ul_interrupt);
void IntDisable(uint32_t ul_interrupt);
void IntPendClear(uint32_t ul_interrupt);

#endif /* PICO_CC3200_INTERRUPTS_H */
