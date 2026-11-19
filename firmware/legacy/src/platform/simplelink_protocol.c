/* Pico Pro 0.1.34 — simplelink protocol. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Original routine retained; precise purpose is not yet established.
 * Firmware address: 0x20015c1c. */
uint32_t thunk_FUN_20017574(void)
{
    int value_1;

    value_1 = 0;
    do {
        if (*(PicoOpaqueFunction **)((int)&g_simplelink_protocol_unknown_37 + value_1) !=
            (PicoOpaqueFunction *)0x0) {
            if (*(char *)(g_simplelink_control_block_as_int + 0x97) !=
                g_simplelink_protocol_unknown_31) {
                (**(PicoOpaqueFunction **)((int)&g_simplelink_protocol_unknown_37 + value_1))(0);
            }
            *(uint32_t *)((int)&g_simplelink_protocol_unknown_37 + value_1) = 0;
            *(uint32_t *)((int)&g_simplelink_protocol_unknown_38 + value_1) = 0;
        }
        value_1 = value_1 + 8;
    } while (value_1 != 0x28);
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x20015df0. */
void unresolved_simplelink_protocol_01(short *argument_1)
{
    if (*argument_1 != 8) {
        if (*argument_1 == 0x73) {
            unresolved_simplelink_protocol_13();
            **(uint32_t **)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) *
                                0xc +
                            g_simplelink_control_block_as_int + 0x14) =
                *(uint32_t *)(argument_1 + 4);
            unresolved_simplelink_protocol_11(
                (uint8_t *)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) *
                                0xc +
                            g_simplelink_control_block_as_int + 0x10));
            unresolved_simplelink_protocol_16();
            return;
        }
        return;
    }
    unresolved_simplelink_protocol_13();
    if (*(PicoOpaqueFunction **)(g_simplelink_control_block_as_int + 0xc) ==
        (PicoOpaqueFunction *)0x0) {
        **(uint32_t **)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
                        g_simplelink_control_block_as_int + 0x14) = *(uint32_t *)(argument_1 + 4);
        unresolved_simplelink_protocol_11(
            (uint8_t *)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
                        g_simplelink_control_block_as_int + 0x10));
    } else {
        (**(PicoOpaqueFunction **)(g_simplelink_control_block_as_int + 0xc))(
            (int)(char)(&g_simplelink_device_unknown_02)[*(uint32_t *)(argument_1 + 4) & 7]);
    }
    unresolved_simplelink_protocol_16();
    if (*(int *)(g_simplelink_control_block_as_int + 0xc) == 0) {
        return;
    }
    unresolved_simplelink_protocol_19((uint32_t) *
                                      (uint8_t *)(g_simplelink_control_block_as_int + 0xac));
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20015e04. */
void unresolved_simplelink_protocol_02(void)
{
    uint8_t *pointer_1;
    int value_2;
    int value_3;
    int value_4;

    value_3 = g_simplelink_control_block_as_int;
    *(uint8_t *)(g_simplelink_control_block_as_int + 0x93) = 0;
    value_3 = unresolved_simplelink_protocol_37((uint8_t *)(value_3 + 0x94), 0);
    if (value_3 != 0) {
        do {

        } while (true);
    }
    value_3 =
        unresolved_simplelink_protocol_37((uint8_t *)(g_simplelink_control_block_as_int + 0x95), 0);
    if (value_3 != 0) {
        do {

        } while (true);
    }
    value_3 =
        unresolved_simplelink_protocol_37((uint8_t *)(g_simplelink_control_block_as_int + 0x91), 0);
    if (value_3 != 0) {
        do {

        } while (true);
    }
    value_3 =
        unresolved_simplelink_protocol_37((uint8_t *)(g_simplelink_control_block_as_int + 4), 0);
    if (value_3 != 0) {
        do {

        } while (true);
    }
    value_3 =
        unresolved_simplelink_protocol_37((uint8_t *)(g_simplelink_control_block_as_int + 0x90), 0);
    if (value_3 != 0) {
        do {

        } while (true);
    }
    value_3 = 0;
    do {
        value_4 = unresolved_simplelink_protocol_37(
            (uint8_t *)(value_3 * 0xc + 0x10 + g_simplelink_control_block_as_int), 0);
        value_2 = g_simplelink_control_block_as_int;
        if (value_4 != 0) {
            do {

            } while (true);
        }
        value_3 = value_3 + 1;
    } while (value_3 != 10);
    pointer_1 = (uint8_t *)(g_simplelink_control_block_as_int + 0x88);
    g_simplelink_control_block_as_int = value_4;
    *pointer_1 = 0;
    *(uint8_t *)(value_2 + 0x89) = 10;
    *(uint8_t *)(value_2 + 0x8a) = 10;
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20015ecc. */
uint32_t unresolved_simplelink_protocol_03(uint16_t *argument_1, short *argument_2,
                                           void *argument_3)
{
    uint16_t short_value_1;
    uint16_t short_value_2;
    bool condition_3;
    int *pointer_4;
    int nwp_spi_write_result;
    uint16_t short_value_6;
    int value_7;
    uint32_t value_8;

    pointer_4 = g_simplelink_control_block;
    if (argument_1 == (uint16_t *)0x0) {
        do {

        } while (true);
    }
    g_simplelink_control_block[0x28] = (int)argument_3;
    short_value_1 = *argument_1;
    pointer_4[0x27] = (int)argument_1;
    pointer_4[0x29] = (int)argument_2;
    *(uint16_t *)((int)pointer_4 + 6) = short_value_1;
    if (argument_2 == (short *)0x0) {
        short_value_6 = *(uint8_t *)(argument_1 + 1) + 3 & 0x1fc;
    } else {
        short_value_6 = (*(uint8_t *)(argument_1 + 1) + 3 & 0xfffc) + (*argument_2 + 3U & 0xfffc);
    }
    *(uint16_t *)(pointer_4 + 2) = short_value_6;
    if ((argument_2 == (short *)0x0) ||
        (short_value_2 = argument_2[1], -1 < (int)((uint32_t)short_value_2 << 0x10))) {
        condition_3 = false;
    } else {
        condition_3 = false;
        if (*argument_2 != 0) {
            argument_2[1] = -short_value_2;
            condition_3 = true;
            *(uint16_t *)(pointer_4 + 2) = short_value_6 - short_value_2;
        }
    }
    nwp_spi_write_result = nwp_spi_write(*pointer_4, &g_simplelink_protocol_unknown_12, 4);
    if ((short)nwp_spi_write_result != 4) {
        do {

        } while (true);
    }
    nwp_spi_write_result = nwp_spi_write(*g_simplelink_control_block,
                                         (void *)((int)g_simplelink_control_block + 6), 4);
    if ((short)nwp_spi_write_result != 4) {
        do {

        } while (true);
    }
    if (((argument_3 != (void *)0x0) && (*(uint8_t *)(argument_1 + 1) != 0)) &&
        (value_8 = *(uint8_t *)(argument_1 + 1) + 3 & 0x1fc,
         nwp_spi_write_result = nwp_spi_write(*g_simplelink_control_block, argument_3, value_8),
         value_8 != (int)(short)nwp_spi_write_result)) {
        do {

        } while (true);
    }
    if (condition_3) {
        value_7 = (int)(short)(argument_2[1] + 3U & 0xfffc);
        nwp_spi_write_result =
            nwp_spi_write(*g_simplelink_control_block, *(void **)(argument_2 + 6), value_7);
        if (value_7 != (short)nwp_spi_write_result) {
            do {

            } while (true);
        }
    } else if (argument_2 == (short *)0x0) {
        return 0;
    }
    if (*argument_2 != 0) {
        if ((*(uint8_t *)(argument_1 + 1) & 3) != 0) {
            do {

            } while (true);
        }
        value_7 = (int)(short)(*argument_2 + 3U & 0xfffc);
        nwp_spi_write_result =
            nwp_spi_write(*g_simplelink_control_block, *(void **)(argument_2 + 4), value_7);
        if (value_7 != (short)nwp_spi_write_result) {
            do {

            } while (true);
        }
    }
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x20015fd4. */
void unresolved_simplelink_protocol_04(void)
{
    uint32_t *pointer_1;
    PicoOpaqueFunction *pointer_2;
    int value_3;

    pointer_2 = *(PicoOpaqueFunction **)(g_simplelink_control_block_as_int + 0xb0);
    pointer_1 = *(uint32_t **)(g_simplelink_control_block_as_int + 0xa8);
    if (pointer_2 != (PicoOpaqueFunction *)0x0) {
        value_3 = 0;
        do {
            if ((&g_simplelink_protocol_unknown_16)[value_3 * 2] == (short)*pointer_1) {
                pointer_1 = pointer_1 + 1;
                *pointer_1 = (uint32_t)(uint8_t)(&g_simplelink_protocol_unknown_17)[value_3 * 4];
                break;
            }
            value_3 = value_3 + 1;
        } while (value_3 != 0xf);
        (*pointer_2)(pointer_1);
    }
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20016014. */
void unresolved_simplelink_protocol_05(uint8_t *argument_1)
{
    int value_1;
    uint8_t byte_value_2;
    PicoOpaqueFunction *pointer_3;

    value_1 = g_simplelink_control_block_as_int;
    if (((uint32_t)argument_1 & 0x400) == 0) {
        pointer_3 = (PicoOpaqueFunction *)0x0;
        if (argument_1 == &g_simplelink_protocol_unknown_02) {
            byte_value_2 = 3;
        } else if ((argument_1 + -0x100a < &g_simplelink_protocol_unknown_01) ||
                   (argument_1 == &g_simplelink_protocol_unknown_06)) {
            byte_value_2 = 0;
        } else {
            pointer_3 =
                (PicoOpaqueFunction
                     *)(&g_simplelink_protocol_unknown_18)[((uint32_t)argument_1 & 0x3fff) >> 0xb];
            if ((argument_1 == &g_simplelink_protocol_unknown_08) ||
                (byte_value_2 = 2, argument_1 == &g_simplelink_protocol_unknown_09)) {
                byte_value_2 = 2;
                pointer_3 = (PicoOpaqueFunction *)&LAB_200173b8_1;
            } else if (argument_1 == &g_simplelink_protocol_unknown_07) {
                pointer_3 = unresolved_simplelink_protocol_35 + 1;
            }
        }
    } else {
        byte_value_2 = 1;
        pointer_3 = (PicoOpaqueFunction *)0x0;
    }
    *(uint8_t *)(g_simplelink_control_block_as_int + 0xb4) = byte_value_2;
    *(PicoOpaqueFunction **)(value_1 + 0xb0) = pointer_3;
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20016090. */
uint32_t unresolved_simplelink_protocol_06(uint32_t *argument_1, char *argument_2)
{
    int nwp_spi_write_result;
    uint32_t *pointer_2;
    uint32_t value_3;
    char byte_value_4;
    uint32_t value_5;

    nwp_spi_write_result =
        nwp_spi_write(*g_simplelink_control_block, &g_simplelink_protocol_unknown_13, 4);
    if ((short)nwp_spi_write_result != 4) {
        do {

        } while (true);
    }
    nwp_spi_write_result = nwp_spi_read(*g_simplelink_control_block, argument_1, 4);
    if ((short)nwp_spi_write_result != 4) {
        do {

        } while (true);
    }
    value_5 = 0;
    while (true) {
        value_3 = *argument_1;
        if ((((value_3 & 4) != 0) &&
             ((value_3 & 0x7fff7f7f) ==
              (*(uint8_t *)((int)g_simplelink_control_block + 0x96) & 3 | 0x2bcd5c3c))) ||
            ((value_3 & 0x7fff7f7f) == 0x2bcd5c3a)) {
            value_5 = value_5 & 3;
            if (value_5 == 0) {
                nwp_spi_write_result = nwp_spi_read(*g_simplelink_control_block, argument_1, 4);
                if ((short)nwp_spi_write_result != 4) {
                    do {

                    } while (true);
                }
            } else {
                *argument_1 = argument_1[1];
                nwp_spi_write_result =
                    nwp_spi_read(*g_simplelink_control_block,
                                 (uint32_t *)((4 - value_5) + (int)argument_1), value_5);
                if (value_5 != (int)(short)nwp_spi_write_result) {
                    do {

                    } while (true);
                }
            }
            while (
                ((value_3 = *argument_1,
                  (value_3 & 4) != 0 &&
                      ((value_3 & 0x7fff7f7f) ==
                       (*(uint8_t *)((int)g_simplelink_control_block + 0x96) & 3 | 0x2bcd5c3c))) ||
                 ((value_3 & 0x7fff7f7f) == 0x2bcd5c3a))) {
                nwp_spi_write_result = nwp_spi_read(*g_simplelink_control_block, argument_1, 4);
                if ((short)nwp_spi_write_result != 4) {
                    do {

                    } while (true);
                }
            }
            nwp_spi_write_result = *g_simplelink_control_block;
            *(char *)((int)g_simplelink_control_block + 0x96) =
                *(char *)((int)g_simplelink_control_block + 0x96) + '\x01';
            nwp_spi_write_result = nwp_spi_read(nwp_spi_write_result, argument_1 + 1, 4);
            if ((short)nwp_spi_write_result != 4) {
                do {

                } while (true);
            }
            byte_value_4 = '\0';
            if (value_5 != 0) {
                byte_value_4 = '\x04' - (char)value_5;
            }
            *argument_2 = byte_value_4;
            return 0;
        }
        if (value_5 == 2000) {
            do {

            } while (true);
        }
        if (((value_5 & 3) == 0) &&
            (nwp_spi_write_result = nwp_spi_read(*g_simplelink_control_block, argument_1 + 1, 4),
             (short)nwp_spi_write_result != 4))
            break;
        nwp_spi_write_result = 0;
        pointer_2 = argument_1;
        do {
            nwp_spi_write_result = nwp_spi_write_result + 1;
            *(uint8_t *)pointer_2 = *(uint8_t *)((int)pointer_2 + 1);
            pointer_2 = (uint32_t *)((int)pointer_2 + 1);
        } while (nwp_spi_write_result != 7);
        *(uint8_t *)((int)argument_1 + 7) = 0;
        value_5 = value_5 + 1;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x200161e0. */
uint32_t unresolved_simplelink_protocol_07(uint16_t *argument_1, void *argument_2,
                                           short *argument_3)
{
    uint8_t byte_value_1;
    int value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t *pointer_6;
    uint32_t *pointer_7;
    uint32_t buffer[8];

    byte_value_1 = *(uint8_t *)(g_simplelink_control_block_as_int + 0x92);
    pointer_6 = (uint32_t *)(g_simplelink_control_block_as_int + 0x9c);
    pointer_7 = buffer;
    do {
        value_4 = pointer_6[1];
        *pointer_7 = *pointer_6;
        pointer_7[1] = value_4;
        pointer_7 = pointer_7 + 2;
        pointer_6 = pointer_6 + 2;
    } while (pointer_6 != (uint32_t *)(g_simplelink_control_block_as_int + 0xb4));
    *pointer_7 = *pointer_6;
    *(uint8_t *)(g_simplelink_control_block_as_int + 0x92) = 0;
    value_4 = unresolved_simplelink_protocol_03(argument_1, argument_3, argument_2);
    value_2 = g_simplelink_control_block_as_int;
    *(uint8_t *)(g_simplelink_control_block_as_int + 0x92) = byte_value_1;
    pointer_7 = buffer;
    pointer_6 = (uint32_t *)(value_2 + 0x9c);
    do {
        value_3 = *pointer_7;
        value_5 = pointer_7[1];
        pointer_7 = pointer_7 + 2;
        *pointer_6 = value_3;
        pointer_6[1] = value_5;
        pointer_6 = pointer_6 + 2;
    } while (pointer_7 != buffer + 6);
    *pointer_6 = *pointer_7;
    return value_4;
}

/* Purpose incomplete.
 * Firmware address: 0x20016254. */
void unresolved_simplelink_protocol_08(uint8_t *argument_1, uint32_t argument_2)
{
    uint8_t byte_value_1;
    uint32_t value_2;
    uint32_t value_3;

    value_2 = (uint32_t)*argument_1;
    byte_value_1 = *(uint8_t *)(value_2 * 0xc + g_simplelink_control_block_as_int + 0x1a);
    if ((byte_value_1 == 10) || (value_2 == argument_2)) {
        *argument_1 = byte_value_1;
        return;
    }
    do {
        value_3 = value_2;
        if (9 < value_3) {
            return;
        }
        value_2 =
            (uint32_t) * (uint8_t *)(value_3 * 0xc + g_simplelink_control_block_as_int + 0x1a);
    } while (value_2 != argument_2);
    *(uint8_t *)(value_3 * 0xc + g_simplelink_control_block_as_int + 0x1a) =
        *(uint8_t *)(value_2 * 0xc + g_simplelink_control_block_as_int + 0x1a);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20016298. */
int unresolved_simplelink_protocol_09(uint8_t *argument_1, uint32_t argument_2)
{
    uint8_t byte_value_1;
    int value_2;
    short short_value_3;
    uint32_t value_4;
    uint32_t value_5;
    int value_6;
    uint32_t value_7;

    value_2 = g_simplelink_control_block_as_int;
    byte_value_1 = *(uint8_t *)(g_simplelink_control_block_as_int + 0x8a);
    do {
        value_4 = (uint32_t)byte_value_1;
        if (9 < value_4) {
            short_value_3 = -3;
        branch_1:
            return (int)short_value_3;
        }
        value_5 =
            (uint32_t) * (uint8_t *)(value_4 * 0xc + g_simplelink_control_block_as_int + 0x19);
        value_7 =
            (uint32_t) * (uint8_t *)(value_4 * 0xc + g_simplelink_control_block_as_int + 0x18);
        if ((int)(value_5 << 0x18) < 0) {
            argument_1 = (uint8_t *)((uint32_t)argument_1 & 0xfdff);
        }
        if (((value_7 == 0xf) && (argument_2 == value_5)) &&
            ((argument_1 + -0x100a < &g_simplelink_protocol_unknown_01 ||
              (argument_1 == &g_simplelink_protocol_unknown_06)))) {
        branch_2:
            *(uint8_t *)(value_2 + 0xac) = byte_value_1;
            short_value_3 = 0;
            goto branch_1;
        }
        value_6 = (value_7 - 8) * 8;
        if (((uint8_t *)(uint32_t) * (uint16_t *)(&g_simplelink_protocol_unknown_14 + value_6) ==
             argument_1) &&
            (((argument_2 == (value_5 & 0xf) && (argument_2 < 8)) || ((value_5 & 0xf) == 8)))) {
            *(uint32_t *)(g_simplelink_control_block_as_int + 0xb0) =
                *(uint32_t *)(&g_simplelink_protocol_unknown_15 + value_6);
            goto branch_2;
        }
        byte_value_1 = *(uint8_t *)(value_4 * 0xc + g_simplelink_control_block_as_int + 0x1a);
    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016324. */
void unresolved_simplelink_protocol_10(uint8_t *argument_1)
{
    int value_1;

    value_1 = unresolved_simplelink_protocol_40(argument_1, 0x22, 0x11, 0xff);
    if (value_1 == 0) {
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016336. */
void unresolved_simplelink_protocol_11(uint8_t *argument_1)
{
    int value_1;

    value_1 = unresolved_simplelink_protocol_37(argument_1, 0x22);
    if (value_1 == 0) {
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016344. */
void unresolved_simplelink_protocol_12(uint8_t *argument_1)
{
    int value_1;

    value_1 = unresolved_simplelink_protocol_40(argument_1, 0x33, 0x44, 0xff);
    if (value_1 == 0) {
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016358. */
void unresolved_simplelink_protocol_13(void)
{
    int value_1;

    value_1 = unresolved_simplelink_protocol_40(
        (uint8_t *)(g_simplelink_control_block_as_int + 0x90), 0x33, 0x44, 0xff);
    if (value_1 == 0) {
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016374. */
void unresolved_simplelink_protocol_14(uint8_t *argument_1)
{
    int value_1;

    value_1 = unresolved_simplelink_protocol_37(argument_1, 0x33);
    if (value_1 == 0) {
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016384. */
int unresolved_simplelink_protocol_15(uint32_t argument_1, uint16_t *argument_2, void *argument_3,
                                      short *argument_4)
{
    int value_1;
    short short_value_2;
    uint32_t value_3;
    uint32_t value_4;

    value_4 = 1 << (argument_1 & 0xf);
    do {
        unresolved_simplelink_protocol_12((uint8_t *)(g_simplelink_control_block_as_int + 0x94));
        unresolved_simplelink_protocol_40((uint8_t *)(g_simplelink_control_block_as_int + 0x95),
                                          0x22, 0x11, 0);
        if ((value_4 & *(uint8_t *)(g_simplelink_control_block_as_int + 0x99)) != 0) {
            unresolved_simplelink_protocol_14(
                (uint8_t *)(g_simplelink_control_block_as_int + 0x94));
            short_value_2 = -1;
        branch_1:
            return (int)short_value_2;
        }
        if (*(uint8_t *)(g_simplelink_control_block_as_int + 0x93) < 3) {
            if ((value_4 & *(uint8_t *)(g_simplelink_control_block_as_int + 0x98)) != 0) {
                unresolved_simplelink_protocol_14(
                    (uint8_t *)(g_simplelink_control_block_as_int + 0x94));
                short_value_2 = -0xb;
                goto branch_1;
            }
            unresolved_simplelink_protocol_10(
                (uint8_t *)(g_simplelink_control_block_as_int + 0x95));
        }
        if (2 < *(uint8_t *)(g_simplelink_control_block_as_int + 0x93)) {
            unresolved_simplelink_protocol_12((uint8_t *)(g_simplelink_control_block_as_int + 4));
            value_1 = g_simplelink_control_block_as_int;
            if (*(uint8_t *)(g_simplelink_control_block_as_int + 0x93) < 3) {
                do {

                } while (true);
            }
            *(uint8_t *)(g_simplelink_control_block_as_int + 0x93) =
                *(uint8_t *)(g_simplelink_control_block_as_int + 0x93) - 1;
            unresolved_simplelink_protocol_14((uint8_t *)(value_1 + 0x94));
            value_3 = unresolved_simplelink_protocol_03(argument_2, argument_4, argument_3);
            short_value_2 = (short)value_3;
            unresolved_simplelink_protocol_14((uint8_t *)(g_simplelink_control_block_as_int + 4));
            goto branch_1;
        }
        unresolved_simplelink_protocol_14((uint8_t *)(g_simplelink_control_block_as_int + 0x94));
    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016440. */
void unresolved_simplelink_protocol_16(void)
{
    int value_1;

    value_1 = unresolved_simplelink_protocol_37(
        (uint8_t *)(g_simplelink_control_block_as_int + 0x90), 0x33);
    if (value_1 == 0) {
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016458. */
uint32_t unresolved_simplelink_protocol_17(uint8_t argument_1, uint32_t argument_2)
{
    uint8_t byte_value_1;
    uint8_t byte_value_2;
    int value_3;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t value_6;

    unresolved_simplelink_protocol_13();
    value_3 = g_simplelink_control_block_as_int;
    byte_value_1 = *(uint8_t *)(g_simplelink_control_block_as_int + 0x88);
    value_4 = (uint32_t)byte_value_1;
    if (value_4 < 10) {
        byte_value_2 = *(uint8_t *)(value_4 * 0xc + g_simplelink_control_block_as_int + 0x1a);
        if (9 < byte_value_2) {
            byte_value_2 = 10;
        }
        *(uint8_t *)(g_simplelink_control_block_as_int + 0x88) = byte_value_2;
        value_3 = value_4 * 0xc + value_3;
        *(uint8_t *)(value_3 + 0x18) = argument_1;
        if (argument_2 < 8) {
            *(char *)(value_3 + 0x19) = (char)argument_2;
        }
        value_6 = 1 << (argument_2 & 0xff);
        value_5 = 1 << argument_1;
        while (((value_3 = g_simplelink_control_block_as_int,
                 argument_2 < 8 &&
                     ((value_6 & *(uint32_t *)(g_simplelink_control_block_as_int + 0x8c)) != 0)) ||
                (((value_5 & *(uint32_t *)(g_simplelink_control_block_as_int + 0x8c)) != 0 &&
                  (argument_2 == 8))))) {
            *(uint8_t *)(g_simplelink_control_block_as_int + value_4 * 0xc + 0x1a) =
                *(uint8_t *)(g_simplelink_control_block_as_int + 0x89);
            *(uint8_t *)(value_3 + 0x89) = byte_value_1;
            unresolved_simplelink_protocol_16();
            unresolved_simplelink_protocol_10(
                (uint8_t *)(value_4 * 0xc + 0x10 + g_simplelink_control_block_as_int));
            unresolved_simplelink_protocol_13();
        }
        if (argument_2 < 8) {
            value_5 = value_6;
        }
        *(uint32_t *)(g_simplelink_control_block_as_int + 0x8c) =
            *(uint32_t *)(g_simplelink_control_block_as_int + 0x8c) | value_5;
        *(uint8_t *)(value_4 * 0xc + value_3 + 0x1a) = *(uint8_t *)(value_3 + 0x8a);
        *(uint8_t *)(value_3 + 0x8a) = byte_value_1;
        unresolved_simplelink_protocol_16();
    } else {
        unresolved_simplelink_protocol_16();
        value_4 = 10;
    }
    return value_4;
}

/* Purpose incomplete.
 * Firmware address: 0x2001651c. */
uint32_t unresolved_simplelink_protocol_18(uint32_t argument_1, uint8_t argument_2,
                                           uint32_t argument_3)
{
    uint32_t value_1;

    value_1 = unresolved_simplelink_protocol_17(argument_2, argument_3);
    if (value_1 != 10) {
        unresolved_simplelink_protocol_13();
        *(uint32_t *)(value_1 * 0xc + g_simplelink_control_block_as_int + 0x14) = argument_1;
        unresolved_simplelink_protocol_16();
    }
    return value_1;
}

/* Purpose incomplete.
 * Firmware address: 0x2001654c. */
void unresolved_simplelink_protocol_19(uint32_t argument_1)
{
    uint8_t byte_value_1;
    uint8_t byte_value_2;
    int value_3;
    int value_4;
    uint32_t value_5;

    unresolved_simplelink_protocol_13();
    byte_value_2 = *(uint8_t *)(g_simplelink_control_block_as_int + 0x89);
    do {
        value_5 = (uint32_t)byte_value_2;
        if (9 < value_5) {
        branch_1:
            value_3 = g_simplelink_control_block_as_int;
            value_4 = argument_1 * 0xc + g_simplelink_control_block_as_int;
            value_5 = *(uint8_t *)(value_4 + 0x19) & 0xf;
            if (7 < value_5) {
                value_5 = (uint32_t) * (uint8_t *)(value_4 + 0x18);
            }
            *(uint32_t *)(g_simplelink_control_block_as_int + 0x8c) =
                *(uint32_t *)(g_simplelink_control_block_as_int + 0x8c) & ~(1 << value_5);
            value_4 = value_3 + argument_1 * 0xc;
            *(uint32_t *)(value_4 + 0x14) = 0;
            *(uint8_t *)(value_4 + 0x18) = 0;
            *(uint8_t *)(value_4 + 0x19) = 8;
            unresolved_simplelink_protocol_08((uint8_t *)(value_3 + 0x8a), argument_1);
            value_3 = g_simplelink_control_block_as_int;
            *(uint8_t *)(argument_1 * 0xc + g_simplelink_control_block_as_int + 0x1a) =
                *(uint8_t *)(g_simplelink_control_block_as_int + 0x88);
            *(char *)(value_3 + 0x88) = (char)argument_1;
            unresolved_simplelink_protocol_16();
            return;
        }
        value_3 = value_5 * 0xc + g_simplelink_control_block_as_int;
        value_4 = argument_1 * 0xc + g_simplelink_control_block_as_int;
        if ((*(char *)(value_3 + 0x18) == *(char *)(value_4 + 0x18)) &&
            ((byte_value_2 = *(uint8_t *)(value_3 + 0x19),
              (byte_value_2 & 0xf) == 8 ||
                  ((byte_value_1 = *(uint8_t *)(value_4 + 0x19),
                    (byte_value_1 & 0xf) < 8 && (((byte_value_2 ^ byte_value_1) & 0xf) == 0)))))) {
            unresolved_simplelink_protocol_08((uint8_t *)(g_simplelink_control_block_as_int + 0x89),
                                              value_5);
            unresolved_simplelink_protocol_11(
                (uint8_t *)(value_5 * 0xc + 0x10 + g_simplelink_control_block_as_int));
            goto branch_1;
        }
        byte_value_2 = *(uint8_t *)(value_5 * 0xc + g_simplelink_control_block_as_int + 0x1a);
    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016618. */
int unresolved_simplelink_protocol_20(void *argument_1, uint16_t *argument_2, void *argument_3,
                                      short *argument_4)
{
    int value_1;
    uint32_t value_2;
    uint8_t *pointer_3;
    void *pointer_28;
    uint16_t *pointer_24;
    void *pointer_20;

    if (*(int *)(argument_4 + 6) == 0) {
        do {

        } while (true);
    }
    if (argument_4[1] == 0) {
        do {

        } while (true);
    }
    if (((uint32_t)argument_1 & 0xf) < 8) {
        pointer_28 = argument_1;
        pointer_24 = argument_2;
        pointer_20 = argument_3;
        value_2 = unresolved_simplelink_protocol_17(0xf, (uint32_t)argument_1 & 0xf);
        if (value_2 == 10) {
            pointer_3 = &g_simplelink_protocol_unknown_10;
        } else {
            unresolved_simplelink_protocol_13();
            pointer_24 = *(uint16_t **)(argument_4 + 6);
            *(void ***)(value_2 * 0xc + g_simplelink_control_block_as_int + 0x14) = &pointer_28;
            pointer_28 = argument_3;
            unresolved_simplelink_protocol_16();
            unresolved_simplelink_protocol_12(
                (uint8_t *)(g_simplelink_control_block_as_int + 0x94));
            unresolved_simplelink_protocol_40((uint8_t *)(g_simplelink_control_block_as_int + 0x95),
                                              0x22, 0x11, 0);
            if (*(uint8_t *)(g_simplelink_control_block_as_int + 0x93) < 2) {
                unresolved_simplelink_protocol_10(
                    (uint8_t *)(g_simplelink_control_block_as_int + 0x95));
            }
            unresolved_simplelink_protocol_12((uint8_t *)(g_simplelink_control_block_as_int + 4));
            value_1 = g_simplelink_control_block_as_int;
            if (*(uint8_t *)(g_simplelink_control_block_as_int + 0x93) < 2) {
                do {

                } while (true);
            }
            *(uint8_t *)(g_simplelink_control_block_as_int + 0x93) =
                *(uint8_t *)(g_simplelink_control_block_as_int + 0x93) - 1;
            unresolved_simplelink_protocol_14((uint8_t *)(value_1 + 0x94));
            pointer_3 =
                (uint8_t *)unresolved_simplelink_protocol_03(argument_2, argument_4, argument_3);
            unresolved_simplelink_protocol_14((uint8_t *)(g_simplelink_control_block_as_int + 4));
            if (pointer_3 == (uint8_t *)0x0) {
                unresolved_simplelink_protocol_10(
                    (uint8_t *)(g_simplelink_control_block_as_int + value_2 * 0xc + 0x10));
            }
            unresolved_simplelink_protocol_19(value_2);
        }
    } else {
        pointer_3 = &g_simplelink_protocol_unknown_11;
    }
    return (int)(short)pointer_3;
}

/* Purpose incomplete.
 * Firmware address: 0x200166f0. */
void unresolved_simplelink_protocol_21(void *argument_1, uint32_t argument_2)
{
    libc_memset(argument_1, 0, argument_2);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x200166f8. */
void unresolved_simplelink_protocol_22(void)
{
    uint8_t *pointer_1;
    int value_2;
    int value_3;
    int value_4;

    g_simplelink_control_block_as_uint8_t_pointer = &g_simplelink_protocol_unknown_32;
    unresolved_simplelink_protocol_21(&g_simplelink_protocol_unknown_32, 0xbc);
    g_simplelink_protocol_unknown_31_as_uint8_t = 0;
    value_2 = unresolved_simplelink_protocol_37(
        g_simplelink_control_block_as_uint8_t_pointer + 0x91, 0x11);
    if (value_2 != 0) {
        do {

        } while (true);
    }
    unresolved_simplelink_protocol_40(g_simplelink_control_block_as_uint8_t_pointer + 0x91, 0x22,
                                      0x11, 0);
    value_2 =
        unresolved_simplelink_protocol_37(g_simplelink_control_block_as_uint8_t_pointer + 4, 0x33);
    if (value_2 != 0) {
        do {

        } while (true);
    }
    value_2 = unresolved_simplelink_protocol_37(
        g_simplelink_control_block_as_uint8_t_pointer + 0x90, 0x33);
    if (value_2 != 0) {
        do {

        } while (true);
    }
    unresolved_simplelink_protocol_21(g_simplelink_control_block_as_uint8_t_pointer + 0x10, 0x78);
    g_simplelink_control_block_as_uint8_t_pointer[0x88] = 0;
    value_2 = 0;
    do {
        pointer_1 = g_simplelink_control_block_as_uint8_t_pointer;
        value_4 = value_2 * 0xc;
        value_2 = value_2 + 1;
        g_simplelink_control_block_as_uint8_t_pointer[value_4 + 0x19] = 8;
        pointer_1[value_4 + 0x1a] = (char)value_2;
        value_3 = unresolved_simplelink_protocol_37(pointer_1 + value_4 + 0x10, 0x11);
        if (value_3 != 0) {
            do {

            } while (true);
        }
        unresolved_simplelink_protocol_40(
            g_simplelink_control_block_as_uint8_t_pointer + value_4 + 0x10, 0x22, 0x11, 0);
        pointer_1 = g_simplelink_control_block_as_uint8_t_pointer;
    } while (value_2 != 10);
    g_simplelink_control_block_as_uint8_t_pointer[0x8a] = 10;
    pointer_1[0x89] = 10;
    pointer_1[0x93] = 1;
    value_2 = unresolved_simplelink_protocol_37(pointer_1 + 0x94, 0x33);
    if (value_2 != 0) {
        do {

        } while (true);
    }
    value_2 = unresolved_simplelink_protocol_37(
        g_simplelink_control_block_as_uint8_t_pointer + 0x95, 0x11);
    if (value_2 == 0) {
        g_simplelink_protocol_unknown_29 = 0;
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x200167d0. */
uint32_t unresolved_simplelink_protocol_23(uint32_t argument_1, uint32_t argument_2,
                                           uint32_t argument_3, uint32_t argument_4)
{
    char byte_value_1;
    int *pointer_2;
    int value_3;
    uint32_t index_2;
    uint32_t value_5;
    uint8_t *pointer_6;
    short index;
    uint32_t value_8;
    uint32_t value_9;
    uint16_t short_value_10;
    uint32_t value_30;
    uint32_t value;
    uint32_t value_28;
    uint32_t value_24;

    pointer_2 = g_simplelink_control_block;
    g_simplelink_control_block[0x2a] = 0;
    pointer_2[0x2c] = 0;
    value_30 = argument_1;
    value = argument_2;
    value_28 = argument_3;
    value_24 = argument_4;
    value_3 = unresolved_simplelink_protocol_06(&value_28, (char *)((int)&value_30 + 3));
    index_2 = value_28;
    pointer_2 = g_simplelink_control_block;
    if (value_3 != 0) {
        do {

        } while (true);
    }
    pointer_6 = (uint8_t *)(value_28 & 0xffff);
    index = PICO_PART_U16(value_28, 2);
    if (pointer_6 != &NMI) {
        *(uint8_t *)(g_simplelink_control_block + 0x26) = PICO_PART_U8(value_24, 3);
        *(uint8_t *)((int)pointer_2 + 0x93) = (uint8_t)value_24;
        *(uint8_t *)((int)pointer_2 + 0x99) = PICO_PART_U8(value_24, 2);
        if (1 < (uint8_t)value_24) {
            unresolved_simplelink_protocol_11((uint8_t *)((int)pointer_2 + 0x95));
        }
    }
    unresolved_simplelink_protocol_05(pointer_6);
    byte_value_1 = (char)g_simplelink_control_block[0x2d];
    if (byte_value_1 == '\x01') {
        index_2 = *(uint8_t *)(g_simplelink_control_block[0x27] + 3) + 3 & 0x1fc;
        value_3 = nwp_spi_read(*g_simplelink_control_block,
                               (uint32_t *)g_simplelink_control_block[0x28], index_2);
        pointer_2 = g_simplelink_control_block;
        if (index_2 != (int)(short)value_3) {
            do {

            } while (true);
        }
        value_3 = g_simplelink_control_block[0x29];
        if ((value_3 != 0) && (short_value_10 = *(uint16_t *)(value_3 + 2), short_value_10 != 0)) {
            value_8 = (value_28 >> 0x10) -
                          (uint32_t) * (uint8_t *)(g_simplelink_control_block[0x27] + 3) &
                      0xffff;
            index_2 = value_8 - 4;
            index = (short)index_2;
            *(short *)(value_3 + 4) = index;
            if (index < 1) {
                *(uint16_t *)(value_3 + 2) = 0;
            } else {
                if ((short)short_value_10 < index) {
                    value_5 = short_value_10 & 0xfffffffc;
                    value_9 = (uint32_t)short_value_10;
                } else {
                    value_5 = index_2 & 0xfffc;
                    value_9 = index_2;
                }
                if ((3 < value_5) &&
                    (value_3 = nwp_spi_read(*pointer_2, *(uint32_t **)(value_3 + 0xc),
                                            (int)(short)value_5),
                     (int)(short)value_5 != (int)(short)value_3)) {
                    do {

                    } while (true);
                }
                if ((value_9 & 3) != 0) {
                    value_3 = nwp_spi_read(*g_simplelink_control_block, &value, 4);
                    if ((short)value_3 != 4) {
                        do {

                        } while (true);
                    }
                    libc_memcpy(
                        (void *)(*(int *)(g_simplelink_control_block[0x29] + 0xc) + value_5),
                        &value, value_9 & 3);
                    index_2 = value_8 - 8;
                }
                if ((short)*(uint16_t *)(g_simplelink_control_block[0x29] + 2) < (short)index_2) {
                    for (index_2 =
                             (index_2 & 0xffff) -
                             (*(uint16_t *)(g_simplelink_control_block[0x29] + 2) & 0xfffffffc);
                         (index_2 & 0xffff) != 0; index_2 = (index_2 & 0xffff) - 4) {
                        value_3 = nwp_spi_read(*g_simplelink_control_block, &value, 4);
                        if ((short)value_3 != 4) {
                            do {

                            } while (true);
                        }
                    }
                }
            }
        }
    } else {
        if (byte_value_1 == '\0') {
            if (pointer_6 == &g_simplelink_protocol_unknown_05) {
                index = 0xc;
            } else if (pointer_6 == &g_simplelink_protocol_unknown_06) {
                index = 0x18;
            } else {
                index = 4;
            }
            value_3 = nwp_spi_read(*g_simplelink_control_block, &value_24, 4);
            if ((short)value_3 != 4) {
                do {

                } while (true);
            }
            if (7 < (PICO_PART_U8(value_24, 2) & 0xf)) {
                do {

                } while (true);
            }
            unresolved_simplelink_protocol_13();
            value_3 = unresolved_simplelink_protocol_09(pointer_6, value_24 >> 0x10 & 0xf);
            if (value_3 != 0) {
                do {

                } while (true);
            }
            **(uint32_t **)g_simplelink_control_block
                 [(uint32_t) * (uint8_t *)(g_simplelink_control_block + 0x2b) * 3 + 5] = value_24;
            if ((index != 4) &&
                (value_3 = nwp_spi_read(
                     *g_simplelink_control_block,
                     (uint32_t
                          *)(*(int *)g_simplelink_control_block
                                 [(uint32_t) * (uint8_t *)(g_simplelink_control_block + 0x2b) * 3 +
                                  5] +
                             4),
                     (uint32_t)(uint16_t)(index - 4U)),
                 (uint16_t)(index - 4U) != (uint16_t)value_3)) {
                do {

                } while (true);
            }
            if (0 < (short)value_24) {
                index_2 = value_24 & 3;
                value_9 = value_24 & 3;
                value_8 = value_24 & 0xfffc;
                if ((3 < value_8) &&
                    (value_3 = nwp_spi_read(
                         *g_simplelink_control_block,
                         *(uint32_t *
                               *)(g_simplelink_control_block
                                      [(uint32_t) * (uint8_t *)(g_simplelink_control_block + 0x2b) *
                                           3 +
                                       5] +
                                  4),
                         value_8),
                     (short)value_8 != (short)value_3)) {
                    do {

                    } while (true);
                }
                if (index_2 != 0) {
                    value_3 = nwp_spi_read(*g_simplelink_control_block, &value, 4);
                    if ((short)value_3 != 4) {
                        do {

                        } while (true);
                    }
                    libc_memcpy(
                        (void *)(*(int *)(g_simplelink_control_block
                                              [(uint32_t) *
                                                   (uint8_t *)(g_simplelink_control_block + 0x2b) *
                                                   3 +
                                               5] +
                                          4) +
                                 value_8),
                        &value, value_9);
                }
            }
            unresolved_simplelink_protocol_11(
                (uint8_t *)(g_simplelink_control_block +
                            (uint32_t) * (uint8_t *)(g_simplelink_control_block + 0x2b) * 3 + 4));
        } else {
            if (byte_value_1 != '\x02')
                goto branch_1;
            g_simplelink_control_block[0x2a] = (int)&g_simplelink_protocol_unknown_33;
            unresolved_simplelink_protocol_21(&g_simplelink_protocol_unknown_33, 0xa8);
            g_simplelink_protocol_unknown_33 = value_28;
            g_simplelink_protocol_unknown_34 = value_24;
            value_8 = (uint16_t)(index - 4) + 3 & 0xfffffffc;
            if (value_8 < 0xa1) {
                short_value_10 = index - 1U & 0xfffc;
            } else {
                short_value_10 = 0xa0;
            }
            if (((uint16_t)(index - 4) != 0) &&
                (value_3 = nwp_spi_read(*g_simplelink_control_block,
                                        (uint32_t *)&g_simplelink_protocol_unknown_35,
                                        (int)(short)short_value_10),
                 (int)(short)short_value_10 != (int)(short)value_3)) {
                do {

                } while (true);
            }
            if (0xa0 < value_8) {
                for (index = (index - 1U & 0xfffc) - 0xa0; index != 0; index = index + -4) {
                    value_3 = nwp_spi_read(*g_simplelink_control_block, &value, 4);
                    if ((short)value_3 != 4) {
                        do {

                        } while (true);
                    }
                }
            }
            unresolved_simplelink_protocol_13();
            if (((uint8_t *)(index_2 & 0xfdff) == &g_simplelink_protocol_unknown_03) ||
                (pointer_6 == &g_simplelink_protocol_unknown_04)) {
                index_2 = g_simplelink_protocol_unknown_36 & 0xf;
            } else {
                index_2 = 8;
            }
            unresolved_simplelink_protocol_09(pointer_6, index_2);
        }
        unresolved_simplelink_protocol_16();
    }
branch_1:
    index_2 = value_30 >> 0x18;
    if ((index_2 == 0) || (value_3 = nwp_spi_read(*g_simplelink_control_block, &value_28, index_2),
                           index_2 == (int)(short)value_3)) {
        NwpUnMaskInterrupt();
        return 0;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016b38. */
uint32_t unresolved_simplelink_protocol_24(uint32_t argument_1, uint32_t argument_2)
{
    uint32_t value_1;
    int value_2;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r1_00;
    uint32_t unresolved_return_r1_01;
    uint32_t unresolved_return_r1_02;

    while (true) {
        while (true) {
            if (*(char *)(g_simplelink_control_block_as_uint32_t + 0x92) != '\x01') {
                unresolved_simplelink_protocol_14(
                    (uint8_t *)(g_simplelink_control_block_as_uint32_t + 4));
                if (*(uint8_t *)(g_simplelink_control_block_as_uint32_t + 0x97) !=
                    g_simplelink_protocol_unknown_31_as_uint8_t_2) {
                    unresolved_simplelink_protocol_38(0x20016c5d, 0);
                }
                return 0;
            }
            if ((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_uint32_t + 0x97) !=
                (uint32_t)g_simplelink_protocol_unknown_31_as_uint8_t_2)
                break;
            unresolved_simplelink_protocol_10(
                (uint8_t *)(g_simplelink_control_block_as_uint32_t + 0x91));
            argument_2 = unresolved_return_r1_02;
        }
        value_2 = unresolved_simplelink_protocol_23(
            g_simplelink_control_block_as_uint32_t, argument_2,
            (uint32_t) * (uint8_t *)(g_simplelink_control_block_as_uint32_t + 0x97),
            (uint32_t)g_simplelink_protocol_unknown_31_as_uint8_t_2);
        value_1 = g_simplelink_control_block_as_uint32_t;
        if (value_2 != 0)
            break;
        *(char *)(g_simplelink_control_block_as_uint32_t + 0x97) =
            *(char *)(g_simplelink_control_block_as_uint32_t + 0x97) + '\x01';
        if (*(char *)(value_1 + 0xb4) == '\x01') {
            *(uint8_t *)(value_1 + 0x92) = 0;
            unresolved_simplelink_protocol_40((uint8_t *)(value_1 + 0x91), 0x22, 0x11, 0);
            argument_2 = unresolved_return_r1_00;
        } else {
            argument_2 = unresolved_return_r1;
            if (*(char *)(value_1 + 0xb4) == '\x02') {
                unresolved_simplelink_protocol_04();
                *(uint32_t *)(g_simplelink_control_block_as_uint32_t + 0xa8) = 0;
                argument_2 = unresolved_return_r1_01;
            }
        }
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016bc8. */
int unresolved_simplelink_protocol_25(uint16_t *argument_1, void *argument_2, short *argument_3)
{
    int value_1;
    uint32_t unresolved_return_r1;
    bool condition_2;
    int value;

    unresolved_simplelink_protocol_12((uint8_t *)(g_simplelink_control_block_as_int + 4));
    *(uint8_t *)(g_simplelink_control_block_as_int + 0x92) = 1;
    value_1 = unresolved_simplelink_protocol_03(argument_1, argument_3, argument_2);
    if (value_1 == 0) {
        if (g_simplelink_protocol_unknown_29_as_char == '\0') {
            g_simplelink_protocol_unknown_29_as_char = '\x01';
            value = 4000;
            do {
                condition_2 = value != 0;
                value = value + -1;
            } while (condition_2);
        }
        value_1 = unresolved_simplelink_protocol_24(0, unresolved_return_r1);
    } else {
        unresolved_simplelink_protocol_14((uint8_t *)(g_simplelink_control_block_as_int + 4));
    }
    return (int)(short)value_1;
}

/* Purpose incomplete.
 * Firmware address: 0x20016c30. */
int unresolved_simplelink_protocol_26(uint16_t argument_1, uint32_t argument_2, uint32_t argument_3)
{
    int value_1;
    uint32_t value_10;
    struct {
        uint32_t header;
        uint32_t payload;
    } command;

    value_10 = 0;
    command.header = pico_join_u8_u24(4, (uint32_t)argument_1);
    command.payload = argument_3;
    value_1 =
        unresolved_simplelink_protocol_25((uint16_t *)&command.header, &value_10, (short *)0x0);
    if (value_1 == 0) {
        return (int)(short)value_10;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20016c5c. */
uint32_t unresolved_simplelink_protocol_27(void)
{
    char byte_value_1;
    uint32_t value_2;
    int value_3;
    uint32_t unresolved_return_r1;

    unresolved_simplelink_protocol_12((uint8_t *)(g_simplelink_control_block_as_uint32_t + 4));
    if ((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_uint32_t + 0x97) !=
        (uint32_t)g_simplelink_protocol_unknown_31_as_uint8_t_2) {
        value_3 = unresolved_simplelink_protocol_23(
            g_simplelink_control_block_as_uint32_t, unresolved_return_r1,
            (uint32_t) * (uint8_t *)(g_simplelink_control_block_as_uint32_t + 0x97),
            (uint32_t)g_simplelink_protocol_unknown_31_as_uint8_t_2);
        value_2 = g_simplelink_control_block_as_uint32_t;
        if (value_3 != 0) {
            do {

            } while (true);
        }
        *(char *)(g_simplelink_control_block_as_uint32_t + 0x97) =
            *(char *)(g_simplelink_control_block_as_uint32_t + 0x97) + '\x01';
        byte_value_1 = *(char *)(value_2 + 0xb4);
        if (byte_value_1 == '\x02') {
            if (*(int *)(value_2 + 0xa8) == 0) {
                do {

                } while (true);
            }
            unresolved_simplelink_protocol_04();
            *(uint32_t *)(g_simplelink_control_block_as_uint32_t + 0xa8) = 0;
        } else if ((byte_value_1 != '\x03') && (byte_value_1 != '\0')) {
            do {

            } while (true);
        }
    }
    unresolved_simplelink_protocol_14((uint8_t *)(g_simplelink_control_block_as_uint32_t + 4));
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x20016cc8. */
void unresolved_simplelink_protocol_28(void *argument_1)
{
    unresolved_simplelink_protocol_21(argument_1, 0x10);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20016cce. */
uint32_t unresolved_simplelink_protocol_29(int argument_1)
{
    uint32_t value_1;
    uint32_t value_2;

    if (argument_1 == 0) {
        return 0;
    }
    value_2 = 0;
    do {
        value_1 = value_2 & 0xffff;
        value_2 = value_2 + 1;
    } while (*(char *)(argument_1 + value_2 + -1) != '\0');
    return value_1;
}

/* Purpose incomplete.
 * Firmware address: 0x20016ce8. */
uint32_t unresolved_simplelink_protocol_30(uint32_t argument_1, uint8_t argument_2)
{
    uint32_t value_1;
    int value_2;
    uint32_t buffer[6];

    buffer[0] = 0x100;
    buffer[1] = 0x400;
    buffer[2] = 0x1000;
    buffer[3] = 0x4000;
    buffer[4] = 0x10000;
    value_2 = 0;
    do {
        if (argument_1 <= buffer[value_2] * 0xff)
            break;
        value_2 = value_2 + 1;
    } while (value_2 != 5);
    value_1 = argument_1 / buffer[value_2];
    if (argument_1 != buffer[value_2] * value_1) {
        value_1 = value_1 + 1;
    }
    return value_1 & 0xff | (uint32_t)argument_2 << 0x10 | 0x3000 | value_2 << 8;
}

/* Purpose incomplete.
 * Firmware address: 0x2001704c. */
int unresolved_simplelink_protocol_31(char *argument_1, uint8_t argument_2)
{
    short sl_net_app_mdnsregister_unregister_service_result;

    sl_net_app_mdnsregister_unregister_service_result =
        sl_NetAppMDNSRegisterUnregisterService(argument_1, argument_2, (char *)0x0, '\0', 0, 0, 0);
    return (int)sl_net_app_mdnsregister_unregister_service_result;
}

/* Purpose incomplete.
 * Firmware address: 0x20017064. */
void unresolved_simplelink_protocol_32(int argument_1)
{
    uint16_t short_value_1;
    uint32_t count;
    int value_2;
    uint16_t *pointer_3;
    uint32_t value_4;

    value_2 = (uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
              g_simplelink_control_block_as_int;
    if (*(char *)(value_2 + 0x19) < '\0') {
        return;
    }
    pointer_3 = *(uint16_t **)(value_2 + 0x14);
    short_value_1 = *(uint16_t *)(argument_1 + 10);
    **(uint32_t **)(pointer_3 + 2) = *(uint32_t *)(argument_1 + 0x10);
    **(uint32_t **)(pointer_3 + 4) = *(uint32_t *)(argument_1 + 0xc);
    *pointer_3 = *(uint16_t *)(argument_1 + 8);
    value_4 = (uint32_t) * *(uint16_t **)(pointer_3 + 6);
    count = (uint32_t)short_value_1;
    if (value_4 < short_value_1) {
        count = value_4;
    }
    **(uint16_t **)(pointer_3 + 6) = (uint16_t)count;
    libc_memcpy(*(void **)(pointer_3 + 8), (void *)(argument_1 + 0x14), count);
    unresolved_simplelink_protocol_11(
        (uint8_t *)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
                    g_simplelink_control_block_as_int + 0x10));
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001716c. */
void unresolved_simplelink_protocol_33(int argument_1)
{
    uint32_t *pointer_1;
    int value_2;
    uint32_t *pointer_4;
    uint32_t *pointer_3;

    unresolved_simplelink_protocol_13();
    value_2 = (uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
              g_simplelink_control_block_as_int;
    if (*(char *)(value_2 + 0x19) < '\0') {
        pointer_1 = *(uint32_t **)(value_2 + 0x14);
        pointer_3 = (uint32_t *)(argument_1 + 8);
        do {
            pointer_4 = pointer_3 + 1;
            *pointer_1 = *pointer_3;
            pointer_1 = pointer_1 + 1;
            pointer_3 = pointer_4;
        } while (pointer_4 != (uint32_t *)(argument_1 + 0x1c));
    } else {
        pointer_1 = *(uint32_t **)(value_2 + 0x14);
        *pointer_1 = *(uint32_t *)(argument_1 + 8);
        pointer_1[1] = *(uint32_t *)(argument_1 + 0xc);
    }
    unresolved_simplelink_protocol_11(
        (uint8_t *)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
                    g_simplelink_control_block_as_int + 0x10));
    unresolved_simplelink_protocol_16();
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x200171cc. */
void unresolved_simplelink_protocol_34(int argument_1, uint32_t *argument_2)
{
    *argument_2 = *(uint32_t *)(argument_1 + 0xc);
    argument_2[1] = *(uint32_t *)(argument_1 + 8);
    *(uint16_t *)(argument_2 + 2) = *(uint16_t *)(argument_1 + 2);
    *(uint16_t *)((int)argument_2 + 10) = *(uint16_t *)(argument_1 + 4);
    *(uint16_t *)(argument_2 + 3) = *(uint16_t *)(argument_1 + 6);
    argument_2[4] = *(uint32_t *)(argument_1 + 0x10);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x200171e8. */
void unresolved_simplelink_protocol_35(int argument_1)
{
    uint32_t *pointer_1;
    uint32_t *pointer_3;
    uint32_t *pointer_2;
    uint32_t buffer_24[5];

    if (g_simplelink_protocol_unknown_30 == (PicoOpaqueFunction *)0x0) {
        unresolved_simplelink_protocol_13();
        pointer_1 = *(uint32_t **)((uint32_t) *
                                       (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
                                   g_simplelink_control_block_as_int + 0x14);
        if (pointer_1 != (uint32_t *)0x0) {
            pointer_2 = (uint32_t *)(argument_1 + 8);
            do {
                pointer_3 = pointer_2 + 1;
                *pointer_1 = *pointer_2;
                pointer_1 = pointer_1 + 1;
                pointer_2 = pointer_3;
            } while (pointer_3 != (uint32_t *)(argument_1 + 0x1c));
            unresolved_simplelink_protocol_11(
                (uint8_t *)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) *
                                0xc +
                            g_simplelink_control_block_as_int + 0x10));
        }
        unresolved_simplelink_protocol_16();
    } else {
        unresolved_simplelink_protocol_34(argument_1 + 8, buffer_24);
        ((PicoOpaqueFunction *)(uintptr_t)g_simplelink_protocol_unknown_30)(buffer_24);
    }
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20017368. */
uint16_t unresolved_simplelink_protocol_36(uint8_t *argument_1)
{
    int value_1;
    uint16_t buffer_30[4];
    uint32_t value_28;
    uint32_t value_24;
    uint16_t short_value_20;
    uint8_t buffer[30];

    buffer_30[0] = *argument_1 + 3 & 0xfffc;
    value_28 = *(uint32_t *)(argument_1 + 4);
    buffer_30[1] = 0;
    value_24 = 0;
    short_value_20 = *(uint16_t *)argument_1;
    libc_memcpy(buffer, *(void **)(argument_1 + 8), (uint32_t)argument_1[1]);
    value_1 = unresolved_simplelink_protocol_07((uint16_t *)&g_simplelink_protocol_unknown_19,
                                                &short_value_20, (short *)buffer_30);
    if (value_1 == 0) {
        return short_value_20;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x2001754c. */
uint32_t unresolved_simplelink_protocol_37(uint8_t *argument_1, uint8_t argument_2)
{
    *argument_1 = argument_2;
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x20017554. */
uint32_t unresolved_simplelink_protocol_38(uint32_t argument_1, uint32_t argument_2)
{
    int value_1;

    value_1 = 0;
    do {
        if (*(int *)((int)&g_simplelink_protocol_unknown_37 + value_1) == 0) {
            *(uint32_t *)((int)&g_simplelink_protocol_unknown_37 + value_1) = argument_1;
            *(uint32_t *)((int)&g_simplelink_protocol_unknown_38 + value_1) = argument_2;
            return 0;
        }
        value_1 = value_1 + 8;
    } while (value_1 != 0x28);
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x20017574. */
uint32_t unresolved_simplelink_protocol_39(void)
{
    int value_1;

    value_1 = 0;
    do {
        if (*(PicoOpaqueFunction **)((int)&g_simplelink_protocol_unknown_37 + value_1) !=
            (PicoOpaqueFunction *)0x0) {
            if (*(char *)(g_simplelink_control_block_as_int + 0x97) !=
                g_simplelink_protocol_unknown_31) {
                (**(PicoOpaqueFunction **)((int)&g_simplelink_protocol_unknown_37 + value_1))(0);
            }
            *(uint32_t *)((int)&g_simplelink_protocol_unknown_37 + value_1) = 0;
            *(uint32_t *)((int)&g_simplelink_protocol_unknown_38 + value_1) = 0;
        }
        value_1 = value_1 + 8;
    } while (value_1 != 0x28);
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x200175b4. */
int unresolved_simplelink_protocol_40(uint8_t *argument_1, uint32_t argument_2, uint8_t argument_3,
                                      uint32_t argument_4)
{
    char byte_value_1;

    do {
        if (argument_4 == 0) {
            byte_value_1 = -1;
        branch_1:
            return (int)byte_value_1;
        }
        if (*argument_1 == argument_2) {
            *argument_1 = argument_3;
            byte_value_1 = '\0';
            goto branch_1;
        }
        if (argument_4 != 0xff) {
            argument_4 = argument_4 - 1 & 0xff;
        }
        unresolved_simplelink_protocol_39();
    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x200175e2. */
void unresolved_simplelink_protocol_41(short *argument_1, int argument_2)
{
    *(char *)(argument_2 + 3) = (char)*argument_1 << 4;
    *(short *)(argument_2 + 4) = argument_1[1];
    if (*argument_1 == 2) {
        *(uint32_t *)(argument_2 + 8) = *(uint32_t *)(argument_1 + 2);
        return;
    }
    if (*argument_1 == 9) {
        *(uint32_t *)(argument_2 + 6) = *(uint32_t *)(argument_1 + 4);
        *(short *)(argument_2 + 10) = argument_1[6];
    }
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001760c. */
uint32_t unresolved_simplelink_protocol_42(uint32_t argument_1, uint32_t argument_2)
{
    if ((uint8_t)(&g_simplelink_protocol_unknown_20)[(argument_1 & 0xff) >> 4] + 0x546 <
        argument_2) {
        argument_2 = (uint8_t)(&g_simplelink_protocol_unknown_20)[(argument_1 & 0xff) >> 4] + 0x546;
    }
    return argument_2;
}

/* Purpose incomplete.
 * Firmware address: 0x2001772c. */
void unresolved_simplelink_protocol_43(int argument_1)
{
    int value_1;

    unresolved_simplelink_protocol_13();
    value_1 = g_simplelink_control_block_as_int;
    if ((*(uint8_t *)(argument_1 + 10) & 0xf) < 9) {
        *(uint8_t *)(*(int *)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) *
                                  0xc +
                              g_simplelink_control_block_as_int + 0x14) +
                     2) = *(uint8_t *)(argument_1 + 10);
        **(uint16_t **)((uint32_t) * (uint8_t *)(value_1 + 0xac) * 0xc + value_1 + 0x14) =
            *(uint16_t *)(argument_1 + 8);
        unresolved_simplelink_protocol_11(
            (uint8_t *)((uint32_t) * (uint8_t *)(value_1 + 0xac) * 0xc + value_1 + 0x10));
        unresolved_simplelink_protocol_16();
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20017858. */
void unresolved_simplelink_protocol_44(int argument_1)
{
    uint32_t *pointer_1;

    unresolved_simplelink_protocol_13();
    if ((*(uint8_t *)(argument_1 + 10) & 0xf) < 9) {
        pointer_1 = *(uint32_t **)((uint32_t) *
                                       (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
                                   g_simplelink_control_block_as_int + 0x14);
        *pointer_1 = *(uint32_t *)(argument_1 + 8);
        pointer_1[1] = *(uint32_t *)(argument_1 + 0xc);
        pointer_1[2] = *(uint32_t *)(argument_1 + 0x10);
        unresolved_simplelink_protocol_11(
            (uint8_t *)((uint32_t) * (uint8_t *)(g_simplelink_control_block_as_int + 0xac) * 0xc +
                        g_simplelink_control_block_as_int + 0x10));
        unresolved_simplelink_protocol_16();
        return;
    }
    do {

    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20017a50. */
void unresolved_simplelink_protocol_45(uint32_t argument_1, uint32_t *argument_2)
{
    *argument_2 = *argument_2 | 1 << (argument_1 & 0xf);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20017a62. */
bool unresolved_simplelink_protocol_46(uint32_t argument_1, uint32_t *argument_2)
{
    return (1 << (argument_1 & 0xf) & *argument_2) != 0;
}

/* Purpose incomplete.
 * Firmware address: 0x20017a78. */
void unresolved_simplelink_protocol_47(uint32_t *argument_1)
{
    *argument_1 = 0;
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20017aa8. */
void unresolved_simplelink_protocol_48(void)
{
    int value_1;

    PRCMPeripheralClkEnable(5, 1);
    PRCMPeripheralReset(5);
    unresolved_ti_drivers_05(0x3e, (PicoInterruptHandler *)&LAB_20017a80_1);
    unresolved_ti_drivers_05(0x3f, (PicoInterruptHandler *)&LAB_20017a98_1);
    uDMAEnable();
    libc_memset(&g_simplelink_protocol_unknown_40, 0, 0x400);
    uDMAControlBaseSet(&g_simplelink_protocol_unknown_40);
    value_1 = 0;
    do {
        *(uint32_t *)((int)&g_simplelink_protocol_unknown_39 + value_1) = 0;
        value_1 = value_1 + 4;
    } while (value_1 != 0x100);
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20017b00. */
int unresolved_simplelink_protocol_49(void *argument_1, uint32_t argument_2, uint32_t *argument_3,
                                      uint8_t *argument_4, uint32_t *argument_5)
{
    uint8_t byte_value_1;
    char byte_value_2;
    int value_3;
    uint32_t value_4_2;
    uint32_t value_4;
    uint16_t short_value_0;
    uint32_t value;
    uint16_t short_value;
    uint8_t byte_value_8;
    uint8_t buffer_7[3];
    uint32_t value_4_3;
    uint8_t buffer_0[164];

    libc_memset(&short_value_0, 0, 0xb0);
    if (0x20 < (int)argument_2) {
        do {

        } while (true);
    }
    if (argument_1 != (void *)0x0) {
        byte_value_2 = (char)argument_2;
        short_value_0 = pico_join_u8_u8(byte_value_2, (uint8_t)short_value_0);
        if (argument_5 == (uint32_t *)0x0) {
            value_4 = 0x4008c80;
            libc_memcpy(buffer_7, argument_1, argument_2);
            PICO_PART_U24(value_4, 0) = pico_join_u8_u16(byte_value_2 + '\t', (uint16_t)value_4);
            if (argument_4 != (uint8_t *)0x0) {
                short_value_0 = pico_join_u8_u8(PICO_PART_U8(short_value_0, 1), *argument_4);
                byte_value_8 = argument_4[8];
                if (0x3f < byte_value_8)
                    goto branch_1;
                PICO_PART_U24(value_4, 0) =
                    pico_join_u8_u16(byte_value_2 + '\t' + byte_value_8, (uint16_t)value_4);
                if (*(void **)(argument_4 + 4) != (void *)0x0) {
                    libc_memcpy(buffer_7 + (argument_2 & 0xff), *(void **)(argument_4 + 4),
                                (uint32_t)byte_value_8);
                }
            }
        branch_2:
            if (argument_3 == (uint32_t *)0x0) {
                unresolved_simplelink_protocol_21(&value, 6);
            } else {
                value = *argument_3;
                short_value = *(uint16_t *)(argument_3 + 1);
            }
            value_3 = unresolved_simplelink_protocol_25((uint16_t *)&value_4, &short_value_0,
                                                        (short *)0x0);
            if (value_3 != 0) {
                do {

                } while (true);
            }
            goto branch_3;
        }
        value_4 = 0x4008c82;
        libc_memcpy(buffer_0, argument_1, argument_2);
        PICO_PART_U24(value_4, 0) = pico_join_u8_u16(byte_value_2 + '\x10', (uint16_t)value_4);
        if (argument_4 != (uint8_t *)0x0) {
            byte_value_1 = argument_4[8];
            value_4_2 = (uint32_t)byte_value_1;
            if (value_4_2 != 0) {
                short_value_0 = pico_join_u8_u8(PICO_PART_U8(short_value_0, 1), *argument_4);
                if (0x3f < value_4_2)
                    goto branch_1;
                if (*(void **)(argument_4 + 4) == (void *)0x0) {
                    do {

                    } while (true);
                }
                byte_value_8 = byte_value_1;
                libc_memcpy(buffer_0 + (argument_2 & 0xff), *(void **)(argument_4 + 4), value_4_2);
                PICO_PART_U24(value_4, 0) =
                    pico_join_u8_u16(byte_value_1 + byte_value_2 + '\x10', (uint16_t)value_4);
            }
        }
        byte_value_1 = *(uint8_t *)(argument_5 + 1);
        value_4_2 = (uint32_t)byte_value_1;
        value_4_3 = argument_5[4];
        buffer_7[2] = *(uint8_t *)((int)argument_5 + 0xd);
        if (value_4_2 < 0x21) {
            buffer_7[0] = byte_value_1;
            if (value_4_2 != 0) {
                libc_memcpy(buffer_0 + (uint32_t)PICO_PART_U8(short_value_0, 1) +
                                (uint32_t)byte_value_8,
                            (void *)*argument_5, value_4_2);
                PICO_PART_U24(value_4, 0) =
                    pico_join_u8_u16(byte_value_1 + PICO_PART_U8(value_4, 2), (uint16_t)value_4);
            }
            byte_value_1 = *(uint8_t *)(argument_5 + 3);
            value_4_2 = (uint32_t)byte_value_1;
            if (value_4_2 < 0x21) {
                buffer_7[1] = byte_value_1;
                if (value_4_2 != 0) {
                    libc_memcpy(buffer_0 + (uint32_t)PICO_PART_U8(short_value_0, 1) +
                                    (uint32_t)byte_value_8 + (uint32_t)buffer_7[0],
                                (void *)argument_5[2], value_4_2);
                    PICO_PART_U24(value_4, 0) = pico_join_u8_u16(
                        byte_value_1 + PICO_PART_U8(value_4, 2), (uint16_t)value_4);
                }
                goto branch_2;
            }
        }
    }
branch_1:
    short_value_0 = -0x7d3;
branch_3:
    return (int)short_value_0;
}

/* Purpose incomplete.
 * Firmware address: 0x20017cbc. */
int unresolved_simplelink_protocol_50(void *argument_1, uint32_t argument_2, uint32_t *argument_3,
                                      char *argument_4, uint32_t *argument_5, uint8_t argument_6)
{
    uint8_t byte_value_1;
    char byte_value_2_2;
    int value_3;
    uint32_t value_4_2;
    uint32_t value_8;
    uint16_t short_value_4;
    uint8_t byte_value_2;
    uint32_t value_1;
    uint16_t short_value;
    uint8_t byte_value;
    uint8_t byte_value_3;
    uint8_t buffer_9[5];
    uint32_t value_4;
    uint8_t buffer_0[164];

    byte_value_2 = argument_6;
    if (argument_1 != (void *)0x0) {
        if (0x20 < (int)argument_2) {
            do {

            } while (true);
        }
        byte_value_2_2 = (char)argument_2;
        short_value_4 = pico_join_u8_u8(byte_value_2_2, (uint8_t)short_value_4);
        if (argument_5 == (uint32_t *)0x0) {
            value_8 = 0x4008c83;
            libc_memcpy(buffer_9, argument_1, argument_2);
            PICO_PART_U24(value_8, 0) = pico_join_u8_u16(byte_value_2_2 + '\v', (uint16_t)value_8);
            if (argument_4 == (char *)0x0) {
                short_value_4 = (uint16_t)PICO_PART_U8(short_value_4, 1) << 8;
                byte_value = 0;
            } else {
                short_value_4 = pico_join_u8_u8(PICO_PART_U8(short_value_4, 1), *argument_4);
                byte_value = argument_4[8];
                if (*argument_4 == '\x01') {
                    byte_value_3 = 0;
                }
                if (0x3f < byte_value)
                    goto branch_1;
                PICO_PART_U24(value_8, 0) =
                    pico_join_u8_u16(byte_value_2_2 + '\v' + byte_value, (uint16_t)value_8);
                if (*(void **)(argument_4 + 4) != (void *)0x0) {
                    libc_memcpy(buffer_9 + (argument_2 & 0xff), *(void **)(argument_4 + 4),
                                (uint32_t)byte_value);
                }
            }
        branch_2:
            if (argument_3 == (uint32_t *)0x0) {
                unresolved_simplelink_protocol_21(&value_1, 6);
            } else {
                value_1 = *argument_3;
                short_value = *(uint16_t *)(argument_3 + 1);
            }
            value_3 = unresolved_simplelink_protocol_25((uint16_t *)&value_8, &short_value_4,
                                                        (short *)0x0);
            if (value_3 != 0) {
                do {

                } while (true);
            }
            goto branch_3;
        }
        value_8 = 0x4008c67;
        libc_memcpy(buffer_0, argument_1, argument_2);
        PICO_PART_U24(value_8, 0) = pico_join_u8_u16(byte_value_2_2 + '\x14', (uint16_t)value_8);
        if (argument_4 == (char *)0x0) {
        branch_4:
            byte_value = 0;
        } else {
            byte_value = argument_4[8];
            value_4_2 = (uint32_t)byte_value;
            if (value_4_2 == 0)
                goto branch_4;
            short_value_4 = pico_join_u8_u8(PICO_PART_U8(short_value_4, 1), *argument_4);
            if (*argument_4 == '\x01') {
                byte_value_3 = 0;
            }
            if (0x3f < value_4_2)
                goto branch_1;
            PICO_PART_U24(value_8, 0) =
                pico_join_u8_u16(byte_value_2_2 + '\x14' + byte_value, (uint16_t)value_8);
            if (*(void **)(argument_4 + 4) == (void *)0x0) {
                do {

                } while (true);
            }
            libc_memcpy(buffer_0 + (argument_2 & 0xff), *(void **)(argument_4 + 4), value_4_2);
        }
        byte_value_1 = *(uint8_t *)(argument_5 + 1);
        value_4_2 = (uint32_t)byte_value_1;
        value_4 = argument_5[4];
        buffer_9[2] = *(uint8_t *)((int)argument_5 + 0xd);
        if (value_4_2 < 0x21) {
            buffer_9[0] = byte_value_1;
            if (value_4_2 != 0) {
                libc_memcpy(buffer_0 + (uint32_t)PICO_PART_U8(short_value_4, 1) +
                                (uint32_t)byte_value,
                            (void *)*argument_5, value_4_2);
                PICO_PART_U24(value_8, 0) =
                    pico_join_u8_u16(byte_value_1 + PICO_PART_U8(value_8, 2), (uint16_t)value_8);
            }
            byte_value_1 = *(uint8_t *)(argument_5 + 3);
            value_4_2 = (uint32_t)byte_value_1;
            if (value_4_2 < 0x21) {
                buffer_9[1] = byte_value_1;
                if (value_4_2 != 0) {
                    libc_memcpy(buffer_0 + (uint32_t)PICO_PART_U8(short_value_4, 1) +
                                    (uint32_t)byte_value + (uint32_t)buffer_9[0],
                                (void *)argument_5[2], value_4_2);
                    PICO_PART_U24(value_8, 0) = pico_join_u8_u16(
                        byte_value_1 + PICO_PART_U8(value_8, 2), (uint16_t)value_8);
                }
                goto branch_2;
            }
        }
    }
branch_1:
    short_value_4 = -0x7d3;
branch_3:
    return (int)short_value_4;
}

/* Purpose incomplete.
 * Firmware address: 0x20017ee4. */
void unresolved_simplelink_protocol_51(uint32_t *argument_1)
{
    void *libc_memcpy_result;
    uint32_t count;
    uint32_t buffer_14[2];

    buffer_14[0] = 0;
    switch (*argument_1) {
    case 1:
        count = (uint32_t) * (uint8_t *)((int)argument_1 + 5);
        wifi_connection_status = 3;
        if (count < 0x21) {
            libc_memcpy_result = PICO_UNRESOLVED_RETURN(libc_memcpy(
                &g_simplelink_protocol_unknown_28, (void *)((int)argument_1 + 6), count));
            *(uint8_t *)((int)libc_memcpy_result + count) = 0;
            g_simplelink_protocol_unknown_27 = *(uint8_t *)((int)argument_1 + 0x47);
            g_simplelink_protocol_unknown_26 = *(uint8_t *)(argument_1 + 0x12);
            g_simplelink_protocol_unknown_25 = *(uint8_t *)((int)argument_1 + 0x49);
            g_simplelink_protocol_unknown_24 = *(uint8_t *)((int)argument_1 + 0x4a);
            g_simplelink_protocol_unknown_23 = *(uint8_t *)((int)argument_1 + 0x4b);
            g_simplelink_protocol_unknown_22 = *(uint8_t *)(argument_1 + 0x13);
        }
        break;
    case 2:
        wifi_connection_status = 6;
        libc_memset(&g_simplelink_protocol_unknown_28, 0, 0x20);
        libc_memset(&g_simplelink_protocol_unknown_22, 0, 6);
        return;
    case 5:
        WiFi_register_connected_device(buffer_14, argument_1 + 9);
        break;
    case 6:
        WiFi_unregister_connected_device(argument_1 + 9);
        g_simplelink_protocol_unknown_21 = g_simplelink_protocol_unknown_21 + -1;
    }
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20017f94. */
void unresolved_simplelink_protocol_52(uint32_t *argument_1, uint32_t argument_2,
                                       uint32_t argument_3)
{
    uint32_t value_1;
    uint32_t *pointer_2;
    uint32_t network_address;

    uint32_t value_8;

    value_1 = *argument_1;
    if (value_1 != 0) {
        if (value_1 < 3) {
            value_1 = argument_1[1];
            pointer_2 = &g_wifi_unknown_07_as_uint32_t;
        } else {
            if (value_1 != 3) {
                return;
            }
            value_1 = argument_1[1];
            PICO_PART_U8(network_address, 3) = (uint8_t)value_1;
            PICO_PART_U24(network_address, 0) =
                pico_join_u8_u16((char)(value_1 >> 8),
                                 pico_join_u8_u8((char)(value_1 >> 0x10), (char)(value_1 >> 0x18)));
            value_8 = argument_3;
            WiFi_register_connected_device((uint32_t *)&PICO_PART_U8(network_address, 0),
                                           argument_1 + 3);
            pointer_2 = (uint32_t *)&g_simplelink_protocol_unknown_21_as_uint32_t;
            value_1 = g_simplelink_protocol_unknown_21_as_uint32_t + 1;
        }
        *pointer_2 = value_1;
    }
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20017fe0. */
void unresolved_simplelink_protocol_53(void)
{
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20017fe2. */
void unresolved_simplelink_protocol_54(void)
{
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x20017fe4. */
int unresolved_simplelink_protocol_55(uint32_t *argument_1, uint8_t *argument_2, int argument_3)
{
    uint8_t *pointer_1;

    pointer_1 = argument_2 + argument_3;
    for (; argument_2 != pointer_1; argument_2 = argument_2 + 1) {
        (**(PicoOpaqueFunction **)*argument_1)(argument_1, *argument_2);
    }
    return argument_3;
}
