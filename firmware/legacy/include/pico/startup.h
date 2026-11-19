#ifndef PICO_STARTUP_H
#define PICO_STARTUP_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void NmiSR(void);
void FaultISR(void);
void BusFaultHandler(void);
void IntDefaultHandler(void);
void ResetISR(void);
void *_sbrk(intptr_t __delta);

#endif /* PICO_STARTUP_H */
