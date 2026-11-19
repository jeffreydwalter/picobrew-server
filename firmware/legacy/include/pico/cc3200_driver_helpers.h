#ifndef PICO_CC3200_DRIVER_HELPERS_H
#define PICO_CC3200_DRIVER_HELPERS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void ADCEnable(uint32_t baseAddress);
void ADCDisable(uint32_t baseAddress);
void ADCTimerEnable(uint32_t baseAddress);
void ADCTimerDisable(uint32_t baseAddress);
uint32_t ADCFIFOLvlGet(uint32_t baseAddress, uint32_t channel);
uint32_t ADCFIFORead(uint32_t baseAddress, uint32_t channel);
void GPIOIntClear(uint32_t port, uint32_t interruptFlags);
long GPIOPinRead(uint32_t port, uint8_t pins);
bool IntMasterEnable(void);
bool IntMasterDisable(void);
void PinConfigurePeripheral2mA(uint32_t pin, uint32_t mode);
void PinConfigurePeripheral6mA(uint32_t pin, uint32_t mode);
void PinTypeADC(uint32_t pin, uint32_t mode);
void TimerIntEnable(uint32_t baseAddress, uint32_t interruptFlags);
uint32_t TimerIntStatus(uint32_t baseAddress, bool masked);
void TimerIntClear(uint32_t baseAddress, uint32_t interruptFlags);
void UARTIntEnable(uint32_t baseAddress, uint32_t interruptFlags);
void UARTIntDisable(uint32_t baseAddress, uint32_t interruptFlags);
uint32_t UARTIntStatus(uint32_t baseAddress, bool masked);
void UARTIntClear(uint32_t baseAddress, uint32_t interruptFlags);

#endif /* PICO_CC3200_DRIVER_HELPERS_H */
