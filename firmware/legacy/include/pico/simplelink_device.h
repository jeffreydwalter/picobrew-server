#ifndef PICO_SIMPLELINK_DEVICE_H
#define PICO_SIMPLELINK_DEVICE_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int sl_Start(int interfaceHandle, uint32_t deviceName, int initializationCallback);
short sl_Stop(uint16_t timeout);

#endif /* PICO_SIMPLELINK_DEVICE_H */
