/* Pico Pro 0.1.34 — simplelink netcfg. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_DEVICE_NETCFG_SET_COMMAND': '0x8432'} and
 * matching argument/control flow in pinned netcfg.c. Firmware address: 0x20017490. */
long sl_NetCfgSet(uint8_t config_id, uint8_t config_opt, uint8_t config_len, uint8_t *pValues)
{
    uint32_t unresolved_input_register_00000021;
    int value_1;
    uint32_t unresolved_input_register_00000025;
    uint32_t unresolved_input_register_00000029;
    short short_value_30;
    uint16_t short_value;
    uint16_t short_value_2;
    short short_value_3;
    uint16_t buffer_28[4];
    uint8_t *pointer_20;

    unresolved_simplelink_protocol_28(buffer_28);
    short_value_3 = (short)pico_join_u24_u8(unresolved_input_register_00000029, config_len);
    buffer_28[0] = short_value_3 + 3U & 0xfffc;
    short_value = (uint16_t)pico_join_u24_u8(unresolved_input_register_00000021, config_id);
    short_value_2 = (uint16_t)pico_join_u24_u8(unresolved_input_register_00000025, config_opt);
    pointer_20 = pValues;
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_netcfg_unknown_02,
                                                &short_value_30, (short *)buffer_28);
    if (value_1 == 0) {
        return (int)short_value_30;
    }
    do {

    } while (true);
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_DEVICE_NETCFG_GET_COMMAND': '0x8433'} and
 * matching argument/control flow in pinned netcfg.c. Firmware address: 0x200174d4. */
long sl_NetCfgGet(uint8_t config_id, uint8_t *pConfigOpt, uint8_t *pConfigLen, uint8_t *pValues)
{
    uint32_t unresolved_input_register_00000021;
    int value_1;
    short short_value_30;
    uint16_t short_value;
    uint16_t short_value_2;
    uint16_t short_value_3;
    short short_value_28;
    uint16_t short_value_26;
    short short_value_24;
    uint8_t *pointer;

    if (*pConfigLen == '\0') {
        value_1 = -0x7d2;
    } else {
        unresolved_simplelink_protocol_28(&short_value_28);
        short_value_3 = (uint16_t)*pConfigLen;
        short_value = (uint16_t)pico_join_u24_u8(unresolved_input_register_00000021, config_id);
        if (pConfigOpt != (uint8_t *)0x0) {
            short_value_2 = (uint16_t)*pConfigOpt;
        }
        short_value_26 = short_value_3;
        pointer = pValues;
        value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_netcfg_unknown_01,
                                                    &short_value_30, &short_value_28);
        if (value_1 != 0) {
            do {

            } while (true);
        }
        if (pConfigOpt != (uint8_t *)0x0) {
            *pConfigOpt = (uint8_t)short_value_2;
        }
        if ((short)short_value_26 < short_value_24) {
            *pConfigLen = (uint8_t)short_value_26;
            if (pico_join_u24_u8(unresolved_input_register_00000021, config_id) == 2) {
                value_1 = 0;
            } else {
                value_1 = -0x7d1;
            }
        } else {
            *pConfigLen = (uint8_t)short_value_24;
            value_1 = (int)short_value_30;
        }
    }
    return value_1;
}
