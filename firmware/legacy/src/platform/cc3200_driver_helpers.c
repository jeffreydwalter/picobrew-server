/* Pico Pro 0.1.34 — cc3200 driver helpers. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x20019942. */
void ADCEnable(uint32_t baseAddress)
{
    *(uint32_t *)baseAddress = *(uint32_t *)baseAddress | 1;
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x2001994c. */
void ADCDisable(uint32_t baseAddress)
{
    *(uint32_t *)baseAddress = *(uint32_t *)baseAddress & 0xfffffffe;
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x200199ac. */
void ADCTimerEnable(uint32_t baseAddress)
{
    *(uint32_t *)(baseAddress + 0x68) = *(uint32_t *)(baseAddress + 0x68) | 0x2000000;
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x200199b6. */
void ADCTimerDisable(uint32_t baseAddress)
{
    *(uint32_t *)(baseAddress + 0x68) = *(uint32_t *)(baseAddress + 0x68) & 0xfdffffff;
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x200199c0. */
uint32_t ADCFIFOLvlGet(uint32_t baseAddress, uint32_t channel)
{
    return *(uint32_t *)(baseAddress + 0x94 + channel) & 7;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x200199ca. */
uint32_t ADCFIFORead(uint32_t baseAddress, uint32_t channel)
{
    return *(uint32_t *)(baseAddress + 0x74 + channel);
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x20019aa0. */
void GPIOIntClear(uint32_t port, uint32_t interruptFlags)
{
    *(uint32_t *)(port + 0x41c) = interruptFlags;
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x20019ac0. */
long GPIOPinRead(uint32_t port, uint8_t pins)
{
    uint32_t unresolved_input_register_00000025;

    return *(long *)(port + pico_join_u24_u8(unresolved_input_register_00000025, pins) * 4);
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x20019ad4. */
bool IntMasterEnable(void)
{
    uint32_t cpucpsie_result;

    cpucpsie_result = CPUcpsie();
    return PICO_SUB_U8(cpucpsie_result, 0);
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x20019ade. */
bool IntMasterDisable(void)
{
    uint32_t cpucpsid_result;

    cpucpsid_result = CPUcpsid();
    return PICO_SUB_U8(cpucpsid_result, 0);
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x20019cd0. */
void PinConfigurePeripheral2mA(uint32_t pin, uint32_t mode)
{
    PinModeSet(pin, mode);
    PinConfigSet(pin, 0x20, 0);
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x20019ce6. */
void PinConfigurePeripheral6mA(uint32_t pin, uint32_t mode)
{
    PinModeSet(pin, mode);
    PinConfigSet(pin, 0x60, 0);
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x20019d18. */
void PinTypeADC(uint32_t pin, uint32_t mode)
{
    PinConfigSet(pin, 0x20, 0x10000000);
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x2001a588. */
void TimerIntEnable(uint32_t baseAddress, uint32_t interruptFlags)
{
    *(uint32_t *)(baseAddress + 0x18) = interruptFlags | *(uint32_t *)(baseAddress + 0x18);
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x2001a590. */
uint32_t TimerIntStatus(uint32_t baseAddress, bool masked)
{
    uint32_t unresolved_input_register_00000025;

    if (pico_join_u24_u8(unresolved_input_register_00000025, masked) == 0) {
        return *(uint32_t *)(baseAddress + 0x1c);
    }
    return *(uint32_t *)(baseAddress + 0x20);
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x2001a59a. */
void TimerIntClear(uint32_t baseAddress, uint32_t interruptFlags)
{
    *(uint32_t *)(baseAddress + 0x24) = interruptFlags;
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x2001a64c. */
void UARTIntEnable(uint32_t baseAddress, uint32_t interruptFlags)
{
    *(uint32_t *)(baseAddress + 0x38) = interruptFlags | *(uint32_t *)(baseAddress + 0x38);
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x2001a654. */
void UARTIntDisable(uint32_t baseAddress, uint32_t interruptFlags)
{
    *(uint32_t *)(baseAddress + 0x38) = *(uint32_t *)(baseAddress + 0x38) & ~interruptFlags;
    return;
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x2001a65e. */
uint32_t UARTIntStatus(uint32_t baseAddress, bool masked)
{
    uint32_t unresolved_input_register_00000025;

    if (pico_join_u24_u8(unresolved_input_register_00000025, masked) == 0) {
        return *(uint32_t *)(baseAddress + 0x3c);
    }
    return *(uint32_t *)(baseAddress + 0x40);
}

/* Driver register offset and operation match the pinned TI driver source. Generic pin names retain
 * ambiguity between identical peripheral-specific aliases. Firmware address: 0x2001a668. */
void UARTIntClear(uint32_t baseAddress, uint32_t interruptFlags)
{
    *(uint32_t *)(baseAddress + 0x44) = interruptFlags;
    return;
}
