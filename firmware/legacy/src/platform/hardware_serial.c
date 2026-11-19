/* Pico Pro 0.1.34 — hardware serial. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x20019128. */
int HardwareSerial_available(int serial)
{
    uint32_t availableBytes;

    availableBytes = *(uint32_t *)(serial + 0x220);
    if (availableBytes < *(uint32_t *)(serial + 0x224)) {
        availableBytes = availableBytes + *(int *)(serial + 0x21c);
    }
    return availableBytes - *(uint32_t *)(serial + 0x224);
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x2001913e. */
uint32_t HardwareSerial_peek(int serial)
{
    uint32_t receiveHead;

    if (*(int *)(serial + 0x224) == *(int *)(serial + 0x220)) {
        receiveHead = 0xffffffff;
    } else {
        receiveHead = (uint32_t) * (uint8_t *)(serial + *(int *)(serial + 0x224) + 0x11c);
    }
    return receiveHead;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x20019156. */
void HardwareSerial_read(int *serial)
{
    int value_1;

    value_1 = (**(PicoOpaqueFunction **)(*serial + 0x10))();
    if (-1 < value_1) {
        serial[0x89] =
            (serial[0x89] + 1U) - serial[0x87] * ((serial[0x89] + 1U) / (uint32_t)serial[0x87]);
    }
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x2001917c. */
void HardwareSerial_flush(int serial)
{
    if (*(int *)(serial + 0x118) == *(int *)(serial + 0x114)) {
        return;
    }
    do {

    } while (true);
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x2001918c. */
void HardwareSerial_construct(uint32_t *serial)
{
    serial[1] = 0;
    serial[2] = 1000;
    serial[0x45] = 0;
    serial[0x46] = 0;
    serial[0x88] = 0;
    serial[0x89] = 0;
    serial[0x8a] = 0;
    *serial = &g_hardware_serial_unknown_06;
    serial[0x44] = 0x100;
    serial[0x87] = 0x100;
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x200191f8. */
void HardwareSerial_clear(int serial)
{
    if (*(int *)(serial + 0x118) == *(int *)(serial + 0x114)) {
        *(uint32_t *)(serial + 0x118) = 0;
        *(uint32_t *)(serial + 0x114) = 0;
        *(uint32_t *)(serial + 0x224) = 0;
        *(uint32_t *)(serial + 0x220) = 0;
        return;
    }
    do {

    } while (true);
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x2001921c. */
void HardwareSerial_prime_transmit(int serial, uint32_t uartBase)
{
    bool uartspace_avail_result;
    uint32_t value_2;

    if (*(int *)(serial + 0x118) == *(int *)(serial + 0x114)) {
        return;
    }
    while (*(int *)(serial + 0x118) != *(int *)(serial + 0x114)) {
        uartspace_avail_result = UARTSpaceAvail(uartBase);
        if (uartspace_avail_result) {
            UARTIntDisable(
                *(uint32_t *)(&g_hardware_serial_unknown_07 + *(int *)(serial + 0x228) * 4), 0x20);
            while ((uartspace_avail_result = UARTSpaceAvail(uartBase),
                    uartspace_avail_result &&
                        (*(int *)(serial + 0x118) != *(int *)(serial + 0x114)))) {
                UARTCharPutNonBlocking(uartBase,
                                       *(uint8_t *)(*(int *)(serial + 0x118) + serial + 0x10));
                value_2 = *(int *)(serial + 0x118) + 1;
                *(uint32_t *)(serial + 0x118) =
                    value_2 -
                    *(uint32_t *)(serial + 0x110) * (value_2 / *(uint32_t *)(serial + 0x110));
            }
            UARTIntEnable(
                *(uint32_t *)(&g_hardware_serial_unknown_07 + *(int *)(serial + 0x228) * 4), 0x20);
        }
    }
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x200192a4. */
uint32_t HardwareSerial_write_byte(int serial, uint8_t value)
{
    uint32_t value_1;
    int value_2;

    value_1 = *(int *)(serial + 0x114) + 1;
    value_2 = value_1 - *(uint32_t *)(serial + 0x110) * (value_1 / *(uint32_t *)(serial + 0x110));
    if (value_2 != *(int *)(serial + 0x118)) {
        *(uint8_t *)(*(int *)(serial + 0x114) + serial + 0x10) = value;
        *(int *)(serial + 0x114) = value_2;
        HardwareSerial_prime_transmit(
            serial, *(uint32_t *)(&g_hardware_serial_unknown_07 + *(int *)(serial + 0x228) * 4));
        return 1;
    }
    do {

    } while (true);
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x200192e0. */
void HardwareSerial_begin(int serial, uint32_t baudRate)
{
    *(uint32_t *)(serial + 0x22c) = baudRate;
    PRCMPeripheralReset(
        *(uint32_t *)(&g_hardware_serial_unknown_03 + *(int *)(serial + 0x228) * 4));
    PRCMPeripheralClkEnable(
        *(uint32_t *)(&g_hardware_serial_unknown_03 + *(int *)(serial + 0x228) * 4), 1);
    PinConfigurePeripheral2mA(
        *(uint32_t *)(&g_hardware_serial_unknown_01 + *(int *)(serial + 0x228) * 8),
        *(uint32_t *)(&g_hardware_serial_unknown_04 + *(int *)(serial + 0x228) * 4));
    PinConfigurePeripheral2mA(
        *(uint32_t *)(&g_hardware_serial_unknown_02 + *(int *)(serial + 0x228) * 8),
        *(uint32_t *)(&g_hardware_serial_unknown_04 + *(int *)(serial + 0x228) * 4));
    UARTConfigSetExpClk(*(uint32_t *)(&g_hardware_serial_unknown_07 + *(int *)(serial + 0x228) * 4),
                        80000000, *(uint32_t *)(serial + 0x22c), 0x60);
    HardwareSerial_clear(serial);
    IntEnable(*(uint32_t *)(&g_hardware_serial_unknown_05 + *(int *)(serial + 0x228) * 4));
    UARTEnable(*(uint32_t *)(&g_hardware_serial_unknown_07 + *(int *)(serial + 0x228) * 4));
    UARTIntEnable(*(uint32_t *)(&g_hardware_serial_unknown_07 + *(int *)(serial + 0x228) * 4),
                  0x70);
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x20019438. */
void UARTIntHandler(void)
{
    bool uartchars_avail_result;
    uint32_t interrupt_flags;
    long uartchar_get_non_blocking_result;
    int value_3;

    interrupt_flags = UARTIntStatus(
        *(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_14 * 4), true);
    UARTIntClear(*(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_14 * 4),
                 interrupt_flags);
    if (((int)(interrupt_flags << 0x1a) < 0) &&
        (HardwareSerial_prime_transmit(0x20036664, *(uint32_t *)(&g_hardware_serial_unknown_07 +
                                                                 g_hardware_serial_unknown_14 * 4)),
         g_hardware_serial_unknown_09 == g_hardware_serial_unknown_08)) {
        UARTIntDisable(
            *(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_14 * 4), 0x20);
    }
    if ((interrupt_flags & 0x50) != 0) {
        while (uartchars_avail_result = UARTCharsAvail(
                   *(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_14 * 4)),
               uartchars_avail_result) {
            uartchar_get_non_blocking_result = UARTCharGetNonBlocking(
                *(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_14 * 4));
            value_3 = (g_hardware_serial_unknown_12 + 1U) -
                      g_hardware_serial_unknown_11 *
                          ((g_hardware_serial_unknown_12 + 1U) / g_hardware_serial_unknown_11);
            if (value_3 == g_hardware_serial_unknown_13) {
                return;
            }
            (&g_hardware_serial_unknown_10)[g_hardware_serial_unknown_12] =
                (char)uartchar_get_non_blocking_result;
            g_hardware_serial_unknown_12 = value_3;
        }
    }
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x20019444. */
void UARTIntHandler1(void)
{
    bool uartchars_avail_result;
    uint32_t interrupt_flags;
    long uartchar_get_non_blocking_result;
    int value_3;

    interrupt_flags = UARTIntStatus(
        *(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_23 * 4), true);
    UARTIntClear(*(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_23 * 4),
                 interrupt_flags);
    if (((int)(interrupt_flags << 0x1a) < 0) &&
        (HardwareSerial_prime_transmit(0x20036894, *(uint32_t *)(&g_hardware_serial_unknown_07 +
                                                                 g_hardware_serial_unknown_23 * 4)),
         g_hardware_serial_unknown_19 == g_hardware_serial_unknown_18)) {
        UARTIntDisable(
            *(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_23 * 4), 0x20);
    }
    if ((interrupt_flags & 0x50) != 0) {
        while (uartchars_avail_result = UARTCharsAvail(
                   *(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_23 * 4)),
               uartchars_avail_result) {
            uartchar_get_non_blocking_result = UARTCharGetNonBlocking(
                *(uint32_t *)(&g_hardware_serial_unknown_07 + g_hardware_serial_unknown_23 * 4));
            value_3 = (g_hardware_serial_unknown_21 + 1U) -
                      g_hardware_serial_unknown_20 *
                          ((g_hardware_serial_unknown_21 + 1U) / g_hardware_serial_unknown_20);
            if (value_3 == g_hardware_serial_unknown_22) {
                return;
            }
            *(char *)(g_hardware_serial_unknown_21 + 0x200369b0) =
                (char)uartchar_get_non_blocking_result;
            g_hardware_serial_unknown_21 = value_3;
        }
    }
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x20019450. */
void serialEvent(void)
{
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x20019452. */
void serialEvent1(void)
{
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x20019454. */
void serialEventRun(void)
{
    int hardware_serial_available_result;

    hardware_serial_available_result = HardwareSerial_available(0x20036664);
    if (hardware_serial_available_result != 0) {
        serialEvent();
    }
    hardware_serial_available_result = HardwareSerial_available(0x20036894);
    if (hardware_serial_available_result != 0) {
        serialEvent1();
    }
    return;
}

/* Pinned Energia HardwareSerial.cpp methods match UART register calls and256-byte transmit/receive
 * ring buffers; self fields0x220/0x224 contain receive indices. Firmware address: 0x20019478. */
void initialize_global_serial_ports(void)
{
    HardwareSerial_construct((uint32_t *)&global_serial0);
    g_hardware_serial_unknown_15 = 0;
    g_hardware_serial_unknown_16 = 1000;
    g_hardware_serial_unknown_18_as_uint32_t = 0;
    g_hardware_serial_unknown_19_as_uint32_t = 0;
    g_hardware_serial_unknown_21_as_uint32_t = 0;
    g_hardware_serial_unknown_22_as_uint32_t = 0;
    global_serial1 = &g_hardware_serial_unknown_06;
    g_hardware_serial_unknown_23_as_uint32_t = 1;
    g_hardware_serial_unknown_17 = 0x100;
    g_hardware_serial_unknown_20_as_uint32_t = 0x100;
    return;
}
