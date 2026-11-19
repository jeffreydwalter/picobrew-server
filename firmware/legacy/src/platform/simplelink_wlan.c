/* Pico Pro 0.1.34 — simplelink wlan. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_WLAN_WLANDISCONNECTCOMMAND': '0x8c81'} and
 * matching argument/control flow in pinned wlan.c. Firmware address: 0x20017c74. */
short sl_WlanDisconnect(void)
{
    int value_1;
    uint32_t unresolved_input_r1;
    uint32_t unresolved_input_r2;

    value_1 = unresolved_simplelink_protocol_26(0x8c81, unresolved_input_r1, unresolved_input_r2);
    return (short)value_1;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_WLAN_POLICYSETCOMMAND': '0x8c86'} and matching
 * argument/control flow in pinned wlan.c. Firmware address: 0x20017c7c. */
short sl_WlanPolicySet(uint8_t Type, uint8_t Policy, uint8_t *pVal, uint8_t val_len)
{
    int value_1;
    uint32_t unresolved_input_register_0000002d;
    short short_value;
    uint8_t byte_value;
    uint8_t byte_value_29;
    short buffer_28[4];
    uint8_t *pointer_20;

    unresolved_simplelink_protocol_28(buffer_28);
    buffer_28[0] = (short)pico_join_u24_u8(unresolved_input_register_0000002d, val_len);
    short_value = pico_join_u8_u8(PICO_PART_U8(short_value, 1), Type);
    byte_value = Policy;
    byte_value_29 = val_len;
    pointer_20 = pVal;
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_wlan_unknown_04,
                                                &short_value, buffer_28);
    if (value_1 == 0) {
        return short_value;
    }
    do {

    } while (true);
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_WLAN_PROFILEDELCOMMAND': '0x8c85'} and matching
 * argument/control flow in pinned wlan.c. Firmware address: 0x20017e48. */
short sl_WlanProfileDel(short index)
{
    int value_1;
    uint32_t unresolved_input_r1;
    uint32_t value;

    value = pico_join_u24_u8((int32_t)((uint32_t)unresolved_input_r1 >> 8), (char)index);
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_wlan_unknown_01, &value,
                                                (short *)0x0);
    if (value_1 == 0) {
        return (short)value;
    }
    do {

    } while (true);
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_WLAN_SCANRESULTSGETCOMMAND': '0x8c8c'} and
 * matching argument/control flow in pinned wlan.c. Firmware address: 0x20017e6c. */
short sl_WlanGetNetworkList(uint8_t Index, uint8_t count, void *pEntries)
{
    int value_1;
    uint32_t unresolved_input_register_00000025;
    uint16_t short_value_24;
    short short_value_20;
    short short_value;
    void *pointer_14;

    if (pico_join_u24_u8(unresolved_input_register_00000025, count) == 0) {
        short_value_24 = -0x7d2;
    } else {
        unresolved_simplelink_protocol_28(&short_value_20);
        short_value = (short)pico_join_u24_u8(unresolved_input_register_00000025, count) * 0x2c;
        short_value_24 = pico_join_u8_u8(count, Index);
        pointer_14 = pEntries;
        value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_wlan_unknown_03,
                                                    &short_value_24, &short_value_20);
        if (value_1 != 0) {
            do {

            } while (true);
        }
    }
    return short_value_24;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_WLAN_STARTRXSTATCOMMAND': '0x8cac'} and matching
 * argument/control flow in pinned wlan.c. Firmware address: 0x20017eb8. */
short sl_WlanRxStatStart(void)
{
    int value_1;
    uint32_t unresolved_input_r1;
    uint32_t unresolved_input_r2;

    value_1 = unresolved_simplelink_protocol_26(0x8cac, unresolved_input_r1, unresolved_input_r2);
    return (short)value_1;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_WLAN_SET_MODE': '0x8cb4'} and matching
 * argument/control flow in pinned wlan.c. Firmware address: 0x20017ec0. */
short sl_WlanSetMode(uint8_t mode)
{
    int value_1;
    uint32_t unresolved_input_r1;
    uint32_t value;

    value = pico_join_u24_u8((int32_t)((uint32_t)unresolved_input_r1 >> 8), mode);
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_wlan_unknown_02, &value,
                                                (short *)0x0);
    if (value_1 == 0) {
        return (short)value;
    }
    do {

    } while (true);
}
