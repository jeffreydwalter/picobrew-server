/* Pico Pro 0.1.34 — wiring gpio. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Pinned Energia wiring digital/interrupt source and observed pin tables, GPIO
 * bases40004000/40005000/40006000/40007000 and per-port callback tables. Firmware address:
 * 0x20018dbc. */
void gpio_port_a0_interrupt(void)
{
    uint32_t interrupt_flags;
    int value_1;

    interrupt_flags = GPIOIntStatus(0x40004000, true);
    GPIOIntClear(0x40004000, interrupt_flags);
    value_1 = 0;
    do {
        if (((int)(interrupt_flags << 0x1f) < 0) &&
            (*(PicoOpaqueFunction **)((int)&gpio_port_a0_callbacks + value_1) !=
             (PicoOpaqueFunction *)0x0)) {
            (**(PicoOpaqueFunction **)((int)&gpio_port_a0_callbacks + value_1))();
        }
        value_1 = value_1 + 4;
        interrupt_flags = interrupt_flags >> 1;
    } while (value_1 != 0x20);
    return;
}

/* Pinned Energia wiring digital/interrupt source and observed pin tables, GPIO
 * bases40004000/40005000/40006000/40007000 and per-port callback tables. Firmware address:
 * 0x20018dcc. */
void gpio_port_a1_interrupt(void)
{
    uint32_t interrupt_flags;
    int value_1;

    interrupt_flags = GPIOIntStatus(0x40005000, true);
    GPIOIntClear(0x40005000, interrupt_flags);
    value_1 = 0;
    do {
        if (((int)(interrupt_flags << 0x1f) < 0) &&
            (*(PicoOpaqueFunction **)(value_1 + 0x200365e0) != (PicoOpaqueFunction *)0x0)) {
            (**(PicoOpaqueFunction **)(value_1 + 0x200365e0))();
        }
        value_1 = value_1 + 4;
        interrupt_flags = interrupt_flags >> 1;
    } while (value_1 != 0x20);
    return;
}

/* Pinned Energia wiring digital/interrupt source and observed pin tables, GPIO
 * bases40004000/40005000/40006000/40007000 and per-port callback tables. Firmware address:
 * 0x20018ddc. */
void gpio_port_a2_interrupt(void)
{
    uint32_t interrupt_flags;
    int value_1;

    interrupt_flags = GPIOIntStatus(0x40006000, true);
    GPIOIntClear(0x40006000, interrupt_flags);
    value_1 = 0;
    do {
        if (((int)(interrupt_flags << 0x1f) < 0) &&
            (*(PicoOpaqueFunction **)(value_1 + 0x20036600) != (PicoOpaqueFunction *)0x0)) {
            (**(PicoOpaqueFunction **)(value_1 + 0x20036600))();
        }
        value_1 = value_1 + 4;
        interrupt_flags = interrupt_flags >> 1;
    } while (value_1 != 0x20);
    return;
}

/* Pinned Energia wiring digital/interrupt source and observed pin tables, GPIO
 * bases40004000/40005000/40006000/40007000 and per-port callback tables. Firmware address:
 * 0x20018dec. */
void gpio_port_a3_interrupt(void)
{
    uint32_t interrupt_flags;
    int value_1;

    interrupt_flags = GPIOIntStatus(0x40007000, true);
    GPIOIntClear(0x40007000, interrupt_flags);
    value_1 = 0;
    do {
        if (((int)(interrupt_flags << 0x1f) < 0) &&
            (*(PicoOpaqueFunction **)(value_1 + 0x20036620) != (PicoOpaqueFunction *)0x0)) {
            (**(PicoOpaqueFunction **)(value_1 + 0x20036620))();
        }
        value_1 = value_1 + 4;
        interrupt_flags = interrupt_flags >> 1;
    } while (value_1 != 0x20);
    return;
}

/* Pinned Energia wiring digital/interrupt source and observed pin tables, GPIO
 * bases40004000/40005000/40006000/40007000 and per-port callback tables. Firmware address:
 * 0x20018dfc. */
uint32_t attachInterrupt(uint32_t pin, uint32_t callback, uint32_t mode)
{
    uint8_t uc_pins;
    bool int_master_enable_result;
    int value_2;
    uint8_t *port;
    uint32_t interrupt_flags;
    uint32_t value_3;

    uc_pins = (&digital_pin_bit_masks)[pin];
    interrupt_flags = (uint32_t)uc_pins;
    port = *(uint8_t **)(&gpio_port_base_addresses +
                         (uint32_t)(uint8_t)(&digital_pin_port_indices)[pin] * 4);
    switch (mode) {
    case 0:
        value_3 = 2;
        break;
    default:
        return pin;
    case 2:
        value_3 = 4;
        break;
    case 3:
        value_3 = 0;
        break;
    case 4:
        value_3 = 1;
    }
    IntMasterDisable();
    GPIOIntClear((uint32_t)port, interrupt_flags);
    GPIOIntTypeSet((uint32_t)port, uc_pins, value_3);
    GPIOIntEnable((uint32_t)port, interrupt_flags);
    value_2 = 0;
    do {
        if ((int)(interrupt_flags << 0x1f) < 0)
            break;
        value_2 = value_2 + 1;
        interrupt_flags = interrupt_flags >> 1;
    } while (value_2 != 8);
    if (port == &mmio_wiring_gpio_unknown_02) {
        *(uint32_t *)(value_2 * 4 + 0x200365e0) = callback;
        GPIOIntRegister(0x40005000, gpio_port_a1_interrupt + 1);
        value_3 = 0x11;
    } else if (&mmio_wiring_gpio_unknown_02 < port) {
        if (port == &mmio_wiring_gpio_unknown_03) {
            *(uint32_t *)(value_2 * 4 + 0x20036600) = callback;
            GPIOIntRegister(0x40006000, gpio_port_a2_interrupt + 1);
            value_3 = 0x12;
        } else {
            if (port != &mmio_wiring_gpio_unknown_04)
                goto branch_1;
            *(uint32_t *)(value_2 * 4 + 0x20036620) = callback;
            GPIOIntRegister(0x40007000, gpio_port_a3_interrupt + 1);
            value_3 = 0x13;
        }
    } else {
        if (port != &mmio_wiring_gpio_unknown_01)
            goto branch_1;
        (&gpio_port_a0_callbacks)[value_2] = callback;
        GPIOIntRegister(0x40004000, (PicoInterruptHandler *)&FUN_20018dbc_1);
        value_3 = 0x10;
    }
    IntEnable(value_3);
branch_1:
    int_master_enable_result = IntMasterEnable();
    return (uint32_t)int_master_enable_result;
}

/* Pinned Energia wiring digital/interrupt source and observed pin tables, GPIO
 * bases40004000/40005000/40006000/40007000 and per-port callback tables. Firmware address:
 * 0x20019054. */
void pinMode(int pin, int mode)
{
    uint8_t uc_pins;
    uint16_t short_value_1;
    uint32_t ul_pin_io;
    uint32_t value_2;

    uc_pins = (&digital_pin_bit_masks)[pin];
    if ((uint8_t)(&digital_pin_port_indices)[pin] == 0) {
        return;
    }
    value_2 = *(uint32_t *)(&gpio_port_base_addresses +
                            (uint32_t)(uint8_t)(&digital_pin_port_indices)[pin] * 4);
    short_value_1 = *(uint16_t *)(&digital_pin_hardware_numbers + pin * 2);
    PinTypeGPIO((uint32_t)short_value_1, 0, false);
    if (mode == 0) {
        ul_pin_io = 0;
    branch_1:
        GPIODirModeSet(value_2, uc_pins, ul_pin_io);
        return;
    }
    if (mode == 2) {
        GPIODirModeSet(value_2, uc_pins, 0);
        value_2 = 0x100;
    } else {
        if (mode != 3) {
            ul_pin_io = 1;
            goto branch_1;
        }
        GPIODirModeSet(value_2, uc_pins, 0);
        value_2 = 0x200;
    }
    PinConfigSet((uint32_t)short_value_1, 0x20, value_2);
    return;
}

/* Pinned Energia wiring digital/interrupt source and observed pin tables, GPIO
 * bases40004000/40005000/40006000/40007000 and per-port callback tables. Firmware address:
 * 0x200190cc. */
bool digitalRead(int pin)
{
    uint8_t byte_value_1;
    long gpiopin_read_result;

    byte_value_1 = (&digital_pin_port_indices)[pin];
    if (byte_value_1 == 0) {
        return (bool)byte_value_1;
    }
    gpiopin_read_result =
        GPIOPinRead(*(uint32_t *)(&gpio_port_base_addresses + (uint32_t)byte_value_1 * 4),
                    (&digital_pin_bit_masks)[pin]);
    return gpiopin_read_result != 0;
}

/* Pinned Energia wiring digital/interrupt source and observed pin tables, GPIO
 * bases40004000/40005000/40006000/40007000 and per-port callback tables. Firmware address:
 * 0x200190fc. */
void digitalWrite(int pin, int value)
{
    uint32_t value_1;

    value_1 = (uint32_t)(uint8_t)(&digital_pin_bit_masks)[pin];
    if (value == 0) {
        value_1 = 0;
    }
    if ((uint8_t)(&digital_pin_port_indices)[pin] == 0) {
        return;
    }
    *(uint32_t *)(*(int *)(&gpio_port_base_addresses +
                           (uint32_t)(uint8_t)(&digital_pin_port_indices)[pin] * 4) +
                  (uint32_t)(uint8_t)(&digital_pin_bit_masks)[pin] * 4) = value_1;
    return;
}
