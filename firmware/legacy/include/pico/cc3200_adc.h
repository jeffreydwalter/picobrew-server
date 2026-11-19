#ifndef PICO_CC3200_ADC_H
#define PICO_CC3200_ADC_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void ADCChannelEnable(uint32_t ul_base, uint32_t ul_channel);
void ADCChannelDisable(uint32_t ul_base, uint32_t ul_channel);
void ADCTimerConfig(uint32_t ul_base, uint32_t ul_value);

#endif /* PICO_CC3200_ADC_H */
