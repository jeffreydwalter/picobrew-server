/* Pico Pro 0.1.34 — simplelink netapp. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NETAPP_STOP_COMMAND': '0x9c61'} and matching
 * argument/control flow in pinned netapp.c. Firmware address: 0x20016fac. */
short sl_NetAppStop(uint32_t AppBitMap)
{
    int value_1;
    uint32_t buffer[3];

    buffer[0] = AppBitMap;
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_netapp_unknown_01, buffer,
                                                (short *)0x0);
    if (value_1 == 0) {
        return (short)buffer[0];
    }
    do {

    } while (true);
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NETAPP_MDNSREGISTERSERVICE': '0x9c34'} and
 * matching argument/control flow in pinned netapp.c. Firmware address: 0x20016fd0. */
short sl_NetAppMDNSRegisterUnregisterService(char *pServiceName, uint8_t service_name_len,
                                             char *pText, uint8_t text_len, uint16_t Port,
                                             uint32_t TTL, uint32_t Options)
{
    int value_1;
    uint32_t unresolved_input_register_00000025;
    uint32_t count;
    uint32_t unresolved_input_register_0000002d;
    uint32_t count_00;
    uint16_t short_value;
    uint16_t short_value_2;
    uint32_t value_138;
    uint32_t value_134;
    short buffer_130[4];
    uint8_t *pointer_128;
    uint8_t buffer_120[260];

    count_00 = pico_join_u24_u8(unresolved_input_register_0000002d, text_len);
    count = pico_join_u24_u8(unresolved_input_register_00000025, service_name_len);
    value_134 = Options;
    short_value_2 = Port;
    short_value = pico_join_u8_u8(text_len, service_name_len);
    value_138 = TTL;
    if ((int)(count_00 + count) < 0xff) {
        unresolved_simplelink_protocol_21(buffer_120, 0xff);
        libc_memcpy(buffer_120, pServiceName, count);
        if (count_00 != 0) {
            libc_memcpy(buffer_120 + count, pText, count_00);
        }
        unresolved_simplelink_protocol_28(buffer_130);
        buffer_130[0] = (short)(count_00 + count);
        pointer_128 = buffer_120;
        value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_netapp_unknown_03,
                                                    &short_value, buffer_130);
        if (value_1 != 0) {
            do {

            } while (true);
        }
    } else {
        short_value = -1;
    }
    return short_value;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NETAPP_DNSGETHOSTBYNAME': '0x9c20'} and matching
 * argument/control flow in pinned netapp.c. Firmware address: 0x200170c0. */
short sl_NetAppDnsGetHostByName(char *hostname, uint16_t usNameLen, uint32_t *out_ip_addr,
                                uint8_t family)
{
    uint32_t value_1;
    uint32_t count;
    uint32_t unresolved_input_register_0000002d;
    int value_2;
    uint16_t short_value_40;
    uint8_t byte_value;
    uint16_t buffer[4];
    char *text_34;
    uint16_t buffer_2[2];
    uint8_t buffer_28[20];

    unresolved_simplelink_protocol_28(buffer);
    short_value_40 = usNameLen;
    byte_value = family;
    buffer[0] = usNameLen;
    text_34 = hostname;
    value_1 = unresolved_simplelink_protocol_17(0xb, 8);
    if (value_1 == 10) {
        short_value_40 = 0xf830;
    } else {
        unresolved_simplelink_protocol_13();
        value_2 = value_1 * 0xc + g_simplelink_control_block_as_int;
        *(uint16_t **)(value_2 + 0x14) = buffer_2;
        if (pico_join_u24_u8(unresolved_input_register_0000002d, family) == 3) {
            *(uint8_t *)(value_2 + 0x19) = *(uint8_t *)(value_2 + 0x19) | 0x80;
        }
        unresolved_simplelink_protocol_16();
        value_2 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_netapp_unknown_02,
                                                    &short_value_40, (short *)buffer);
        if (value_2 != 0) {
            do {

            } while (true);
        }
        if (short_value_40 == 0) {
            unresolved_simplelink_protocol_10(
                (uint8_t *)(g_simplelink_control_block_as_int + value_1 * 0xc + 0x10));
            short_value_40 = buffer_2[0];
            if (buffer_2[0] == 0) {
                if (pico_join_u24_u8(unresolved_input_register_0000002d, family) == 2) {
                    count = 4;
                } else {
                    count = 0x10;
                }
                libc_memcpy(out_ip_addr, buffer_28, count);
            }
        }
        unresolved_simplelink_protocol_19(value_1);
    }
    return short_value_40;
}

/* Unique command descriptors0x9e21/0x9c21/0x9c23 match IPv6/IPv4 ping-start and stop operations in
 * pinned SimpleLink netapp.c; callback signature retained as decompiled. Firmware address:
 * 0x20017254. */
int sl_NetAppPingStart(uint32_t *pingParameters, int addressFamily, uint32_t *report, int callback)
{
    int value_1;
    int value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t *pointer_5;
    uint32_t *pointer_6;
    uint32_t value_50;
    short buffer[10];
    uint32_t value_38;
    uint16_t short_value_34;
    uint16_t short_value_32;
    uint32_t value_30;
    uint32_t value;
    uint32_t buffer_28[4];

    value_50 = 0x4200000;
    if (pingParameters[4] == 0) {
        value_1 = unresolved_simplelink_protocol_26(0x9c23, addressFamily, report);
        PICO_PART_U16(value_38, 0) = (uint16_t)value_1;
        goto branch_1;
    }
    pointer_5 = pingParameters + 4;
    if (addressFamily == 2) {
        buffer_28[0] = *pointer_5;
        value_50 = 0x4209c21;
    } else {
        value_50 = 0x4209e21;
        pointer_6 = buffer_28;
        do {
            value_3 = pointer_5[1];
            *pointer_6 = *pointer_5;
            pointer_6[1] = value_3;
            pointer_6 = pointer_6 + 2;
            pointer_5 = pointer_5 + 2;
        } while (pointer_5 != pingParameters + 8);
    }
    value_38 = *pingParameters;
    short_value_34 = *(uint16_t *)(pingParameters + 1);
    short_value_32 = *(uint16_t *)((int)pingParameters + 6);
    value_30 = pingParameters[2];
    value = pingParameters[3];
    if (callback == 0) {
        value_4 = unresolved_simplelink_protocol_17(0xd, 8);
        if (value_4 == 10) {
            PICO_PART_U16(value_38, 0) = -2000;
            goto branch_1;
        }
        value_2 = unresolved_simplelink_protocol_40(
            (uint8_t *)(g_simplelink_control_block_as_int + 0x90), 0x33, 0x44, 0xff);
        value_1 = g_simplelink_control_block_as_int;
        if (value_2 != 0) {
            do {

            } while (true);
        }
        *(short **)(value_4 * 0xc + g_simplelink_control_block_as_int + 0x14) = buffer;
        g_simplelink_protocol_unknown_30_as_int = callback;
        value_1 = unresolved_simplelink_protocol_37((uint8_t *)(value_1 + 0x90), 0x33);
        if (value_1 != 0) {
            do {

            } while (true);
        }
    } else {
        value_4 = 10;
        g_simplelink_protocol_unknown_30_as_int = callback;
    }
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&value_50, &value_38, (short *)0x0);
    if (value_1 != 0) {
        do {

        } while (true);
    }
    if ((uint16_t)value_38 < 2) {
        if (callback == 0) {
            unresolved_simplelink_protocol_10(
                (uint8_t *)(g_simplelink_control_block_as_int + value_4 * 0xc + 0x10));
            if (buffer[0] == 0) {
                unresolved_simplelink_protocol_34((int)buffer, report);
            }
        branch_2:
            unresolved_simplelink_protocol_19(value_4);
        }
    } else if (callback == 0)
        goto branch_2;
branch_1:
    return (int)(short)(uint16_t)value_38;
}
