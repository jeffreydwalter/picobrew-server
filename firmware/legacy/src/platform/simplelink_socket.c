/* Pico Pro 0.1.34 — simplelink socket. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20017628. */
short sl_Socket(short domain, short type, short protocol)
{
    int value_1;
    uint16_t unresolved_input_register_00000026;
    uint16_t unresolved_input_register_0000002a;
    uint32_t value;
    uint32_t value_8;

    value_8 = pico_join_u16_u16(unresolved_input_register_0000002a, protocol);
    PICO_PART_U16(value, 0) = pico_join_u8_u8((char)type, (char)domain);
    PICO_PART_U8(value, 3) = (uint8_t)((uint16_t)unresolved_input_register_00000026 >> 8);
    PICO_PART_U24(value, 0) = pico_join_u8_u16((char)protocol, (uint16_t)value);
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_socket_unknown_03, &value,
                                                (short *)0x0);
    if (value_1 != 0) {
        do {

        } while (true);
    }
    if (-1 < value << 0x10) {
        PICO_PART_U16(value, 0) = (uint16_t)PICO_PART_U8(value, 2);
    }
    return (uint16_t)value;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x2001765c. */
short sl_Close(short socket_descriptor)
{
    int value_1;
    uint32_t unresolved_input_r1;
    uint32_t value;

    value =
        pico_join_u24_u8((int32_t)((uint32_t)unresolved_input_r1 >> 8), (char)socket_descriptor);
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_socket_unknown_04, &value,
                                                (short *)0x0);
    if (value_1 == 0) {
        return (short)value;
    }
    do {

    } while (true);
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20017680. */
short sl_Connect(short socket_descriptor, void *address, short address_length)
{
    short short_value_1;
    uint16_t unresolved_input_register_00000022;
    int value_2;
    uint32_t value_3;
    uint16_t short_value_24;
    uint8_t byte_value_22;
    uint8_t byte_value_21;
    short short_value_20;
    uint8_t byte_value;
    short short_value;
    uint8_t byte_value_2;

    byte_value_21 = 4;
    if (*(short *)address == 2) {
        short_value_24 = 0x9406;
    } else {
        if (*(short *)address != 9) {
            return -2;
        }
        short_value_24 = 0x9606;
    }
    byte_value_2 = (uint8_t)socket_descriptor;
    value_3 = (uint32_t)byte_value_2;
    byte_value_22 = 0xc;
    short_value = 0;
    unresolved_simplelink_protocol_41(address, (int)&short_value);
    value_3 = unresolved_simplelink_protocol_18(&short_value_20, 9, value_3 & 0xf);
    if (value_3 == 10) {
        short_value_1 = -2000;
    } else {
        value_2 = unresolved_simplelink_protocol_25(&short_value_24, &short_value, (short *)0x0);
        if (value_2 != 0) {
            do {

            } while (true);
        }
        if ((uint32_t)byte_value_2 !=
            pico_join_u16_u16(unresolved_input_register_00000022, socket_descriptor)) {
            do {

            } while (true);
        }
        short_value_1 = short_value;
        if (short_value == 0) {
            unresolved_simplelink_protocol_10(
                (uint8_t *)(g_simplelink_control_block_as_int + value_3 * 0xc + 0x10));
            if ((uint32_t)byte_value !=
                pico_join_u16_u16(unresolved_input_register_00000022, socket_descriptor)) {
                do {

                } while (true);
            }
            short_value_1 = short_value_20;
        }
        unresolved_simplelink_protocol_19(value_3);
    }
    return short_value_1;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x2001777c. */
short sl_Send(short socket_descriptor, void *buffer, short length, short flags)
{
    uint16_t unresolved_input_register_00000022;
    uint32_t value_1;
    uint32_t value_2;
    uint16_t unresolved_input_register_0000002e;
    uint32_t value_3;
    bool condition_4;
    short short_value_38;
    uint8_t byte_value_36;
    uint8_t byte_value_35;
    uint32_t value_34;
    short buffer_30[4];
    void *pointer_28;
    uint32_t *pointer_24;

    unresolved_simplelink_protocol_28(buffer_30);
    condition_4 = (socket_descriptor & 0xf0U) != 0x80;
    if (condition_4) {
        pointer_24 = (uint32_t *)0x0;
    } else {
        pointer_24 = &value_34;
        buffer_30[1] = 0xfffc;
        value_34 = pico_join_u16_u16(unresolved_input_register_0000002e, flags);
    }
    pointer_28 = buffer;
    value_1 = unresolved_simplelink_protocol_42(
        pico_join_u16_u16(unresolved_input_register_00000022, socket_descriptor),
        (uint32_t)(uint16_t)length);
    buffer_30[0] = (short)value_1;
    byte_value_36 = (uint8_t)socket_descriptor;
    value_3 = (uint32_t)byte_value_36;
    byte_value_35 = (uint8_t)flags & 0xf | byte_value_35;
    do {
        short_value_38 = buffer_30[0];
        value_2 = unresolved_simplelink_protocol_15(
            value_3, (uint16_t *)&g_simplelink_socket_unknown_02, &short_value_38, buffer_30);
        if (value_2 != 0)
            goto branch_1;
        pointer_28 = (void *)(value_1 + (int)pointer_28);
        value_1 = unresolved_simplelink_protocol_42(
            pico_join_u16_u16(unresolved_input_register_00000022, socket_descriptor),
            (int)buffer + ((uint32_t)(uint16_t)length - (int)pointer_28) & 0xffff);
        buffer_30[0] = (short)value_1;
    } while ((value_1 != 0) && (condition_4));
    value_2 = (uint32_t)(uint16_t)length;
branch_1:
    return (short)value_2;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20017820. */
uint32_t sl_Htonl(uint32_t value)
{
    uint32_t value_4;

    value_4 = pico_join_u8_u24(
        (char)value, pico_join_u8_u16((char)(value >> 8), pico_join_u8_u8((char)(value >> 0x10),
                                                                          (char)(value >> 0x18))));
    return value_4;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20017842. */
uint16_t sl_Htons(uint16_t value)
{
    uint16_t short_value_2;

    short_value_2 = pico_join_u8_u8((char)value, (char)(value >> 8));
    return short_value_2;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x200178e8. */
short sl_Recv(short socket_descriptor, void *buffer, short length, short flags)
{
    uint32_t value_1;
    uint16_t short_value;
    uint8_t byte_value;
    uint8_t byte_value_29;
    short short_value_28;
    short short_value_26;
    void *pointer;

    unresolved_simplelink_protocol_28(&short_value_28);
    byte_value = (uint8_t)socket_descriptor;
    short_value = length;
    byte_value_29 = (uint8_t)flags & 0xf;
    short_value_26 = length;
    pointer = buffer;
    value_1 = unresolved_simplelink_protocol_20((void *)(uint32_t)byte_value,
                                                (uint16_t *)&g_simplelink_socket_unknown_05,
                                                &short_value, &short_value_28);
    if (value_1 == 0) {
        value_1 = (uint32_t)short_value;
    }
    return (short)value_1;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_SOCKET_SETSOCKOPT': '0x9408'} and matching
 * argument/control flow in pinned socket.c. Firmware address: 0x2001792c. */
short sl_SetSockOpt(short sd, short level, short optname, void *optval, uint16_t optlen)
{
    int value_1;
    short short_value;
    uint8_t byte_value;
    uint8_t byte_value_29;
    uint16_t buffer_28[4];
    void *pointer_20;

    unresolved_simplelink_protocol_28(buffer_28);
    buffer_28[0] = optlen;
    short_value = pico_join_u8_u8((char)level, (char)sd);
    byte_value_29 = (uint8_t)optlen;
    byte_value = (uint8_t)optname;
    pointer_20 = optval;
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_socket_unknown_06,
                                                &short_value, (short *)buffer_28);
    if (value_1 == 0) {
        return short_value;
    }
    do {

    } while (true);
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_SOCKET_SELECT': '0x9407'} and matching
 * argument/control flow in pinned socket.c. Firmware address: 0x20017978. */
short sl_Select(short nfds, void *readsds, void *writesds, void *exceptsds, void *timeout)
{
    uint32_t value_1;
    int value_2;
    uint16_t buffer_24[2];
    uint16_t short_value_20;
    uint16_t short_value;
    uint16_t short_value_2;
    uint8_t byte_value;
    uint16_t short_value_18;
    uint16_t short_value_16;
    uint16_t short_value_14;
    uint16_t short_value_12;

    short_value_2 = nfds & 0xff;
    byte_value = 0;
    short_value_18 = 0;
    short_value_16 = 0;
    if (readsds != (void *)0x0) {
        short_value_18 = (uint16_t) * (uint32_t *)readsds;
    }
    if (writesds != (void *)0x0) {
        short_value_16 = (uint16_t) * (uint32_t *)writesds;
    }
    if (timeout == (void *)0x0) {
        short_value_12 = 0xffff;
    } else {
        value_1 = *(uint32_t *)timeout;
        if (0xfffe < value_1) {
            value_1 = 0xffff;
        }
        short_value_12 = (uint16_t)value_1;
        value_1 = *(uint32_t *)((int)timeout + 4) >> 10;
        *(uint32_t *)((int)timeout + 4) = value_1;
        if (value_1 < 0xffff) {
            short_value_14 = (uint16_t)value_1;
            goto branch_1;
        }
    }
    short_value_14 = 0xffff;
branch_1:
    value_1 = unresolved_simplelink_protocol_18(buffer_24, 10, 8);
    if (value_1 == 10) {
        short_value_2 = 0xf830;
    } else {
        value_2 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_socket_unknown_01,
                                                    &short_value_2, (short *)0x0);
        if (value_2 != 0) {
            do {

            } while (true);
        }
        if (short_value_2 == 0) {
            unresolved_simplelink_protocol_10(
                (uint8_t *)(g_simplelink_control_block_as_int + value_1 * 0xc + 0x10));
            short_value_2 = buffer_24[0];
            if (-1 < (int)((uint32_t)buffer_24[0] << 0x10)) {
                if (readsds != (void *)0x0) {
                    *(uint32_t *)readsds = (uint32_t)short_value_20;
                }
                if (writesds != (void *)0x0) {
                    *(uint32_t *)writesds = (uint32_t)short_value;
                }
            }
        }
        unresolved_simplelink_protocol_19(value_1);
    }
    return short_value_2;
}
