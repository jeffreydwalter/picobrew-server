/* Pico Pro 0.1.34 — simplelink device. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Starts network processor, opens SPI unless caller supplies handle, sets initialization callback
 * and waits for start response. Pinned SimpleLink device.c sl_Start API and flow match. Firmware
 * address: 0x20015c20. */
int sl_Start(int interfaceHandle, uint32_t deviceName, int initializationCallback)
{
    int *pointer_1;
    short short_value_2;
    int nwp_spi_open_result;
    uint32_t value_4;
    uint32_t value;

    value = deviceName;
    NwpPowerOnPreamble();
    unresolved_simplelink_protocol_22();
    pointer_1 = g_simplelink_control_block;
    if (interfaceHandle == 0) {
        nwp_spi_open_result = nwp_spi_open();
        *pointer_1 = nwp_spi_open_result;
    } else {
        *g_simplelink_control_block = interfaceHandle;
    }
    value_4 = unresolved_simplelink_protocol_18(&value, 0xe, 8);
    if (value_4 == 10) {
        short_value_2 = -2000;
    } else if (*g_simplelink_control_block < 0) {
        short_value_2 = -0x7d4;
    } else {
        NwpPowerOff();
        nwp_register_interrupt_handler((PicoInterruptHandler *)&LAB_20015e88_1);
        g_simplelink_control_block[3] = initializationCallback;
        NwpPowerOn();
        short_value_2 = 0;
        if (initializationCallback == 0) {
            unresolved_simplelink_protocol_10(
                (uint8_t *)(g_simplelink_control_block + value_4 * 3 + 4));
            unresolved_simplelink_protocol_19((uint32_t) *
                                              (uint8_t *)(g_simplelink_control_block + 0x2b));
            short_value_2 = (short)(char)(&g_simplelink_device_unknown_02)[value & 7];
        }
    }
    return (int)short_value_2;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_DEVICE_STOP_COMMAND': '0x8473'} and matching
 * argument/control flow in pinned device.c. Firmware address: 0x20015d58. */
short sl_Stop(uint16_t timeout)
{
    uint16_t unresolved_input_register_00000022;
    uint32_t nwp_spi_close_result;
    uint32_t value_2;
    int value_3;
    short unresolved_input_r1;
    uint32_t value_20;
    short buffer[4];

    value_20 = pico_join_u16_u16(unresolved_input_register_00000022, timeout);
    if (value_20 == 0) {
        nwp_register_interrupt_handler((PicoInterruptHandler *)0x0);
        NwpPowerOff();
        nwp_spi_close_result = nwp_spi_close();
        buffer[0] = (short)nwp_spi_close_result;
    } else {
        value_20 = pico_join_u16_u16(unresolved_input_register_00000022, timeout);
        buffer[0] = unresolved_input_r1;
        value_2 = unresolved_simplelink_protocol_18(buffer, 0xe, 8);
        if ((char)value_2 == 10) {
            return -2000;
        }
        value_3 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_device_unknown_01,
                                                    &value_20, (short *)0x0);
        if (value_3 != 0) {
            do {

            } while (true);
        }
        if ((short)value_20 == 0) {
            unresolved_simplelink_protocol_10(
                (uint8_t *)((char)value_2 * 0xc + 0x10 + g_simplelink_control_block_as_int));
            value_20 = pico_join_u16_u16(PICO_PART_U16(value_20, 2), buffer[0]);
        } else {
            buffer[0] = 0;
        }
        unresolved_simplelink_protocol_19(value_2);
        nwp_register_interrupt_handler((PicoInterruptHandler *)0x0);
        NwpPowerOff();
        nwp_spi_close();
    }
    unresolved_simplelink_protocol_02();
    return buffer[0];
}
