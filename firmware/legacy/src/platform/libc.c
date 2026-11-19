/* Pico Pro 0.1.34 — libc. */
#include "pico/firmware.h"
#include <stdarg.h>

/* movs r1,#0 then tail-call strtod2001cfa0; _strtod_r body returns IEEE-754 double in r0/r1.
 * Firmware address: 0x2001ae9c. */
double libc_atof(char *text)
{
    bool condition_1;
    bool condition_2;
    bool condition_3;
    int *reentrancy;
    uint8_t *pointer_4;
    uint32_t *pointer_5;
    uint32_t text_length;
    int libc_strncmp_result;
    int *pointer_8;
    uint32_t *value;
    uint32_t *pointer_9;
    uint8_t *pointer_10;
    uint8_t *pointer_11;
    int value_12;
    uint8_t *pointer_13;
    uint32_t value_14;
    int value_15;
    uint32_t value_16;
    uint32_t value_17;
    double *pointer_18;
    int value_19;
    uint32_t value_20;
    uint32_t *value_00;
    uint32_t value_21;
    uint32_t __aeabi_d2uiz_result;
    uint32_t value_23;
    char *right;
    int value_24;
    uint32_t shift;
    int value_25;
    double __aeabi_dmul_result;
    double __aeabi_ui2d_result;
    uint64_t __aeabi_ui2d_result_2;
    uint32_t *pointer_88;
    int value_84;
    int value_78;
    uint64_t __aeabi_dmul_result_2;
    int value_2;
    uint32_t value_48;
    int value_44;
    uint8_t *pointer;
    uint32_t *pointer_38;
    uint32_t value_34;
    uint32_t value_30;
    uint32_t value_3;

    reentrancy = libc_reentrancy;
    __aeabi_ui2d_result_2 = 0.0;
    pointer_38 = (uint32_t *)0x0;
    pointer = (uint8_t *)text;
branch_1:
    value_17 = (uint32_t)*pointer;
    switch (value_17) {
    case 0:
        goto switchD_2001c088_caseD_0;
    default:
        value_84 = 0;
        goto joined_r0x2001c138;
    case 9:
    case 10:
    case 0xb:
    case 0xc:
    case 0xd:
    case 0x20:
        goto switchD_2001c088_caseD_9;
    case 0x2b:
        value_84 = 0;
        break;
    case 0x2d:
        value_84 = 1;
    }
    value_17 = (uint32_t)pointer[1];
    pointer = pointer + 1;
    if (value_17 != 0) {
    joined_r0x2001c138:
        if (value_17 != 0x30) {
            condition_1 = false;
            goto branch_2;
        }
        if ((pointer[1] != 0x58) && (pointer[1] != 0x78))
            goto branch_3;
        value_17 = unresolved_c_library_14(libc_reentrancy, (int *)&pointer,
                                           (uint32_t *)&g_libc_unknown_02, &value_34, &pointer_38,
                                           value_84);
        value_20 = value_17 & 7;
        if (value_20 == 0)
            goto branch_4;
        if (value_20 == 6) {
            __aeabi_ui2d_result_2 = 0.0;
            value_84 = 0;
        } else {
            if (pointer_38 != (uint32_t *)0x0) {
                unresolved_floating_point_05(&value_30, 0x35, (int)pointer_38);
                unresolved_c_library_18((int)reentrancy, pointer_38);
            }
            switch (value_20) {
            case 0:
            case 6:
                __aeabi_ui2d_result_2 = 0.0;
                break;
            case 1:
            case 5:
                __aeabi_ui2d_result_2 = (double)pico_join_u32_u32(
                    value_3 & 0xffefffff | (value_34 + 0x433) * 0x100000, value_30);
                break;
            case 2:
                __aeabi_ui2d_result_2 = (double)pico_join_u32_u32(value_3, value_30);
                break;
            case 3:
                __aeabi_ui2d_result_2 = INFINITY;
                break;
            case 4:
                __aeabi_ui2d_result_2 = NAN;
            }
            if ((int)(value_17 << 0x1c) < 0) {
                __aeabi_ui2d_result_2 =
                    (double)((uint64_t)__aeabi_ui2d_result_2 | 0x8000000000000000);
            }
        }
        goto branch_5;
    }
    goto switchD_2001c088_caseD_0;
switchD_2001c088_caseD_9:
    pointer = pointer + 1;
    goto branch_1;
branch_3:
    do {
        pointer = pointer + 1;
        value_17 = (uint32_t)*pointer;
    } while (value_17 == 0x30);
    if (value_17 == 0) {
    branch_4:
        __aeabi_ui2d_result_2 = 0.0;
        goto branch_5;
    }
    condition_1 = true;
branch_2:
    pointer_88 = (uint32_t *)pointer;
    value_20 = 0;
    __aeabi_d2uiz_result = 0;
    value_19 = 0;
    if (value_17 - 0x30 < 10) {
        do {
            pointer = pointer + 1;
            if (value_19 < 9) {
                __aeabi_d2uiz_result = (value_17 + __aeabi_d2uiz_result * 10) - 0x30;
            } else {
                value_20 = (value_17 + value_20 * 10) - 0x30;
            }
            value_17 = (uint32_t)*pointer;
            value_19 = value_19 + 1;
        } while (value_17 - 0x30 < 10);
    }
    pointer_10 = pointer;
    pointer_5 = (uint32_t *)unresolved_c_library_16();
    right = (char *)*pointer_5;
    pointer_5 = (uint32_t *)unresolved_c_library_16();
    text_length = libc_strlen((char *)*pointer_5);
    libc_strncmp_result = libc_strncmp((char *)pointer_10, right, text_length);
    pointer_10 = pointer;
    value_24 = value_19;
    if (libc_strncmp_result == 0) {
        pointer_5 = (uint32_t *)unresolved_c_library_16();
        value_17 = libc_strlen((char *)*pointer_5);
        pointer = pointer_10 + value_17;
        value_17 = (uint32_t)pointer_10[value_17];
        if (value_19 != 0) {
            value_12 = 0;
            libc_strncmp_result = 0;
            goto branch_6;
        }
        if (value_17 == 0x30) {
            value_15 = 0;
            pointer_10 = pointer;
            while (true) {
                pointer = pointer_10 + 1;
                value_17 = (uint32_t)pointer_10[1];
                value_15 = value_15 + 1;
                if (value_17 != 0x30)
                    break;
                pointer_10 = pointer;
            }
        } else {
            value_15 = 0;
        }
        if (value_17 - 0x31 < 9) {
            pointer_88 = (uint32_t *)pointer;
            text_length = value_17 - 0x30;
            value_24 = 0;
            value_12 = 1;
            libc_strncmp_result = value_15;
            while (true) {
                value_15 = value_24;
                if (text_length != 0) {
                    libc_strncmp_result = libc_strncmp_result + value_12;
                    if (value_12 != 1) {
                        value_12 = value_12 + value_24 + -1;
                        do {
                            value_25 = value_15 + 1;
                            if (value_15 < 9) {
                                __aeabi_d2uiz_result = __aeabi_d2uiz_result * 10;
                            } else if (value_25 < 0x11) {
                                value_20 = value_20 * 10;
                            }
                            value_24 = value_12;
                            value_15 = value_25;
                        } while (value_25 != value_12);
                    }
                    value_15 = value_24 + 1;
                    if (value_24 < 9) {
                        __aeabi_d2uiz_result = text_length + __aeabi_d2uiz_result * 10;
                        value_12 = 0;
                    } else {
                        if (value_15 < 0x11) {
                            value_20 = text_length + value_20 * 10;
                        }
                        value_12 = 0;
                    }
                }
                value_17 = (uint32_t)pointer[1];
                value_24 = value_15;
                pointer = pointer + 1;
            branch_6:
                text_length = value_17 - 0x30;
                if (9 < text_length)
                    break;
                value_12 = value_12 + 1;
            }
            condition_2 = true;
            value_15 = value_12;
            goto branch_7;
        }
        if ((value_17 & 0xffffffdf) == 0x45) {
            libc_strncmp_result = 0;
            value_24 = 0;
            condition_2 = true;
            goto branch_8;
        }
        condition_2 = true;
    branch_9:
        if ((value_15 != 0) || (condition_1))
            goto branch_4;
        if (!condition_2) {
            switch (value_17) {
            case 0x49:
            case 0x69:
                pointer_10 = &g_libc_unknown_03;
                do {
                    pointer_10 = pointer_10 + 1;
                    if (*pointer_10 == 0) {
                        pointer_10 = &g_libc_unknown_04;
                        goto branch_10;
                    }
                    pointer = pointer + 1;
                    value_17 = (uint32_t)*pointer;
                    if (value_17 - 0x41 < 0x1a) {
                        value_17 = value_17 + 0x20;
                    }
                } while (value_17 == *pointer_10);
                break;
            case 0x4e:
            case 0x6e:
                pointer_10 = &g_libc_unknown_05;
                do {
                    pointer_10 = pointer_10 + 1;
                    if (*pointer_10 == 0) {
                        if (pointer[1] == 0x28) {
                            pointer = pointer + 1;
                            value_19 = unresolved_c_library_15(
                                (int *)&pointer, (uint32_t *)&g_libc_unknown_06, &value_30);
                            if (value_19 == 5) {
                                __aeabi_ui2d_result_2 =
                                    (double)(pico_join_u32_u32(value_3, value_30) |
                                             0x7ff0000000000000);
                                goto branch_5;
                            }
                        }
                        __aeabi_ui2d_result_2 = -NAN;
                        goto branch_5;
                    }
                    pointer = pointer + 1;
                    value_17 = (uint32_t)*pointer;
                    if (value_17 - 0x41 < 0x1a) {
                        value_17 = value_17 + 0x20;
                    }
                } while (value_17 == *pointer_10);
            }
        }
    } else {
        libc_strncmp_result = 0;
        condition_2 = false;
        value_15 = 0;
    branch_7:
        if ((value_17 & 0xffffffdf) != 0x45) {
        branch_11:
            pointer_11 = (uint8_t *)0x0;
        joined_r0x2001c792:
            if (value_24 == 0)
                goto branch_9;
            value_17 = (int)pointer_11 - libc_strncmp_result;
            __aeabi_ui2d_result_2 = __aeabi_ui2d(__aeabi_d2uiz_result);
            libc_strncmp_result = value_24;
            if (0xf < value_24) {
                libc_strncmp_result = 0x10;
            }
            if (value_19 == 0) {
                value_19 = value_24;
            }
            if (9 < libc_strncmp_result) {
                __aeabi_dmul_result =
                    __aeabi_dmul(__aeabi_ui2d_result_2,
                                 *(double *)(&g_libc_unknown_08 + libc_strncmp_result * 8));
                __aeabi_ui2d_result = __aeabi_ui2d(value_20);
                __aeabi_ui2d_result_2 = __aeabi_dadd(__aeabi_dmul_result, __aeabi_ui2d_result);
            }
            if (value_24 < 0x10) {
                if (value_17 == 0)
                    goto branch_5;
                if ((int)value_17 < 1) {
                    if (-0x17 < (int)value_17) {
                        __aeabi_ui2d_result_2 = __aeabi_ddiv(
                            __aeabi_ui2d_result_2, *(double *)(&g_libc_unknown_09 + value_17 * -8));
                        goto branch_5;
                    }
                } else {
                    if ((int)value_17 < 0x17) {
                        __aeabi_ui2d_result_2 = __aeabi_dmul(
                            *(double *)(&g_libc_unknown_09 + value_17 * 8), __aeabi_ui2d_result_2);
                        goto branch_5;
                    }
                    if ((int)value_17 <= 0x25 - value_24) {
                        __aeabi_dmul_result =
                            __aeabi_dmul(*(double *)(&g_libc_unknown_09 + (0xf - value_24) * 8),
                                         __aeabi_ui2d_result_2);
                        __aeabi_ui2d_result_2 = __aeabi_dmul(
                            __aeabi_dmul_result,
                            *(double *)(&g_libc_unknown_09 + (value_17 - (0xf - value_24)) * 8));
                        goto branch_5;
                    }
                }
            }
            value_20 = (value_24 - libc_strncmp_result) + value_17;
            if ((int)value_20 < 1) {
                if (value_20 != 0) {
                    text_length = -value_20 & 0xf;
                    if (text_length != 0) {
                        __aeabi_ui2d_result_2 =
                            __aeabi_ddiv(__aeabi_ui2d_result_2,
                                         *(double *)(&g_libc_unknown_09 + text_length * 8));
                    }
                    value_20 = (int)-value_20 >> 4;
                    if (value_20 != 0) {
                        if ((int)value_20 < 0x20) {
                            if ((value_20 & 0x10) == 0) {
                                value_78 = 0;
                            } else {
                                value_78 = 0x6a;
                            }
                            if (0 < (int)value_20) {
                                pointer_18 = (double *)&g_libc_unknown_07;
                                do {
                                    if ((int)(value_20 << 0x1f) < 0) {
                                        __aeabi_ui2d_result_2 =
                                            __aeabi_dmul(__aeabi_ui2d_result_2, *pointer_18);
                                    }
                                    value_20 = (int)value_20 >> 1;
                                    pointer_18 = pointer_18 + 1;
                                } while (value_20 != 0);
                            }
                            if (value_78 != 0) {
                                libc_strncmp_result = -(
                                    (PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7fffffff) >> 0x14);
                                value_20 = libc_strncmp_result + 0x6b;
                                if (0 < (int)value_20) {
                                    if ((int)value_20 < 0x20) {
                                        __aeabi_ui2d_result_2 = (double)pico_join_u32_u32(
                                            PICO_PART_U32(__aeabi_ui2d_result_2, 4),
                                            -1 << (value_20 & 0xff) &
                                                (uint32_t)__aeabi_ui2d_result_2);
                                    } else if ((int)value_20 < 0x35) {
                                        __aeabi_ui2d_result_2 =
                                            (double)((uint64_t)(-1 << (libc_strncmp_result + 0x4bU &
                                                                       0xff) &
                                                                PICO_PART_U32(__aeabi_ui2d_result_2,
                                                                              4))
                                                     << 0x20);
                                    } else {
                                        __aeabi_ui2d_result_2 = 4.008336720017946e-292;
                                    }
                                }
                            }
                            libc_strncmp_result = __aeabi_dcmpeq(__aeabi_ui2d_result_2, 0.0);
                            if (libc_strncmp_result == 0)
                                goto branch_12;
                        }
                        *reentrancy = 0x22;
                        __aeabi_ui2d_result_2 = 0.0;
                        goto branch_5;
                    }
                }
            branch_13:
                value_78 = 0;
            branch_12:
                pointer_8 = unresolved_c_library_21(reentrancy, (int)pointer_88, value_19, value_24,
                                                    __aeabi_d2uiz_result);
                if (pointer_8 != (int *)0x0) {
                    value_20 = -value_17;
                    if (-1 < (int)value_17) {
                        value_20 = 0;
                    }
                    value_17 = value_17 & ~((int)value_17 >> 0x1f);
                    pointer_88 = (uint32_t *)0x0;
                    value_00 = (uint32_t *)0x0;
                branch_14:
                    value = (uint32_t *)unresolved_c_library_17(reentrancy, pointer_8[1]);
                    if (value == (uint32_t *)0x0)
                        goto branch_15;
                    libc_memcpy(value + 3, pointer_8 + 3, (pointer_8[4] + 2) * 4);
                    __aeabi_d2uiz_result = PICO_PART_U32(__aeabi_ui2d_result_2, 4);
                    pointer_38 = (uint32_t *)unresolved_floating_point_03(
                        reentrancy, &value_30, (uint32_t)__aeabi_ui2d_result_2,
                        PICO_PART_U32(__aeabi_ui2d_result_2, 4), (int *)&value_34,
                        (int *)&value_30);
                    if ((pointer_38 == (uint32_t *)0x0) ||
                        (value_00 = (uint32_t *)unresolved_c_library_24(reentrancy, 1),
                         value_00 == (uint32_t *)0x0))
                        goto branch_15;
                    if ((int)value_34 < 0) {
                        text_length = value_17 - value_34;
                        value_23 = value_20;
                    } else {
                        text_length = value_17;
                        value_23 = value_20 + value_34;
                    }
                    value_19 = (value_34 - value_78) + value_30;
                    value_24 = 0x36 - value_30;
                    if (value_19 < -0x3fd) {
                        value_14 = -value_19 - 0x3fd;
                        value_24 = value_24 - value_14;
                        if ((int)value_14 < 0x20) {
                            value_21 = 1 << (value_14 & 0xff);
                            value_14 = 0;
                        } else {
                            value_21 = 1;
                            value_14 = 1 << (-value_19 - 0x41dU & 0xff);
                        }
                    } else {
                        value_14 = 0;
                        value_21 = 1;
                    }
                    shift = value_24 + value_23;
                    if ((int)shift < (int)value_23) {
                        value_16 = shift;
                    } else {
                        value_16 = value_23;
                    }
                    text_length = text_length + value_24 + value_78;
                    if ((int)text_length <= (int)value_16) {
                        value_16 = text_length;
                    }
                    if (0 < (int)value_16) {
                        shift = shift - value_16;
                        text_length = text_length - value_16;
                        value_23 = value_23 - value_16;
                    }
                    if (value_20 != 0) {
                        value_00 = (uint32_t *)unresolved_c_library_23(reentrancy, (int *)value_00,
                                                                       value_20);
                        if ((value_00 == (uint32_t *)0x0) ||
                            (pointer_9 = (uint32_t *)unresolved_c_library_22(
                                 reentrancy, (int)value_00, (int)pointer_38),
                             pointer_9 == (uint32_t *)0x0))
                            goto branch_15;
                        unresolved_c_library_18((int)reentrancy, pointer_38);
                        pointer_38 = pointer_9;
                    }
                    if ((((((0 < (int)shift) && (pointer_38 = (uint32_t *)unresolved_c_library_27(
                                                     reentrancy, pointer_38, shift),
                                                 pointer_38 == (uint32_t *)0x0)) ||
                           ((value_17 != 0 && (value = (uint32_t *)unresolved_c_library_23(
                                                   reentrancy, (int *)value, value_17),
                                               value == (uint32_t *)0x0)))) ||
                          ((0 < (int)text_length && (value = (uint32_t *)unresolved_c_library_27(
                                                         reentrancy, value, text_length),
                                                     value == (uint32_t *)0x0)))) ||
                         ((0 < (int)value_23 && (value_00 = (uint32_t *)unresolved_c_library_27(
                                                     reentrancy, value_00, value_23),
                                                 value_00 == (uint32_t *)0x0)))) ||
                        (pointer_88 = (uint32_t *)unresolved_c_library_29(
                             reentrancy, (int)pointer_38, (int)value),
                         pointer_88 == (uint32_t *)0x0))
                        goto branch_15;
                    text_length = pointer_88[3];
                    pointer_88[3] = 0;
                    value_19 = unresolved_c_library_24_2((int)pointer_88, (int)value_00);
                    if (value_19 < 0) {
                        if (((((text_length == 0) && ((uint32_t)__aeabi_ui2d_result_2 == 0)) &&
                              (((uint64_t)__aeabi_ui2d_result_2 & 0xfffff00000000) == 0)) &&
                             (0x6b00000 <
                              (PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000))) &&
                            ((pointer_88[5] != 0 || (1 < (int)pointer_88[4])))) {
                            pointer_88 =
                                (uint32_t *)unresolved_c_library_27(reentrancy, pointer_88, 1);
                            value_19 = unresolved_c_library_24_2((int)pointer_88, (int)value_00);
                            if (value_19 < 1)
                                goto branch_16;
                        branch_17:
                            if ((value_78 == 0) ||
                                (0x6b00000 <
                                 (PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000))) {
                                __aeabi_ui2d_result_2 = (double)pico_join_u32_u32(
                                    ~(~((PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000) -
                                            0x100000 >>
                                        0x14)
                                      << 0x14),
                                    0xffffffff);
                                goto branch_16;
                            }
                            if ((PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000) < 0x3700001)
                                goto branch_18;
                        branch_19:
                            __aeabi_dmul_result =
                                __aeabi_dmul(__aeabi_ui2d_result_2, 1.232595164407831e-32);
                            PICO_PART_U32(__aeabi_ui2d_result_2, 4) =
                                (uint32_t)((uint64_t)__aeabi_dmul_result >> 0x20);
                            if ((PICO_PART_U32(__aeabi_ui2d_result_2, 4) == 0) &&
                                (PICO_PART_U32(__aeabi_ui2d_result_2, 0) =
                                     PICO_SUB_U32(__aeabi_dmul_result, 0),
                                 (uint32_t)__aeabi_ui2d_result_2 == 0)) {
                                *reentrancy = 0x22;
                                __aeabi_ui2d_result_2 = __aeabi_dmul_result;
                                goto branch_20;
                            }
                            goto branch_21;
                        }
                    } else {
                        if (value_19 != 0) {
                            __aeabi_dmul_result =
                                unresolved_floating_point_04((int)pointer_88, (int)value_00);
                            value_19 = __aeabi_dcmple(__aeabi_dmul_result, 2.0);
                            if (value_19 == 0) {
                                __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dmul_result, 0.5);
                                __aeabi_dmul_result = __aeabi_dmul_result_2;
                                if (text_length == 0) {
                                    __aeabi_dmul_result = (double)pico_join_u32_u32(
                                        (int)((uint64_t)__aeabi_dmul_result_2 >> 0x20) +
                                            -0x80000000,
                                        PICO_SUB_U32(__aeabi_dmul_result_2, 0));
                                }
                            } else if (text_length == 0) {
                                if ((uint32_t)__aeabi_ui2d_result_2 == 0) {
                                    if (((uint64_t)__aeabi_ui2d_result_2 & 0xfffff00000000) == 0) {
                                        value_19 = __aeabi_dcmplt(__aeabi_dmul_result, 1.0);
                                        if (value_19 == 0) {
                                            __aeabi_dmul_result_2 =
                                                __aeabi_dmul(__aeabi_dmul_result, 0.5);
                                            value_48 = PICO_SUB_U32(__aeabi_dmul_result_2, 0);
                                            value_44 =
                                                (int)((uint64_t)__aeabi_dmul_result_2 >> 0x20) +
                                                -0x80000000;
                                        } else {
                                            value_48 = 0;
                                            value_44 = -0x40200000;
                                            __aeabi_dmul_result_2 = 0.5;
                                        }
                                        __aeabi_dmul_result =
                                            (double)pico_join_u32_u32(value_44, value_48);
                                        goto branch_22;
                                    }
                                } else if (((uint32_t)__aeabi_ui2d_result_2 == 1) &&
                                           (PICO_PART_U32(__aeabi_ui2d_result_2, 4) == 0))
                                    goto branch_18;
                                __aeabi_dmul_result_2 = 1.0;
                                __aeabi_dmul_result = -1.0;
                            } else {
                                __aeabi_dmul_result_2 = 1.0;
                                __aeabi_dmul_result = 1.0;
                            }
                        branch_22:
                            value_23 = PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000;
                            if (value_23 == 0x7fe00000) {
                                PICO_PART_U32(__aeabi_ui2d_result_2, 4) =
                                    PICO_PART_U32(__aeabi_ui2d_result_2, 4) + 0xfcb00000;
                                __aeabi_ui2d_result = (double)unresolved_floating_point_01(
                                    (uint32_t)__aeabi_ui2d_result_2,
                                    PICO_PART_U32(__aeabi_ui2d_result_2, 4));
                                __aeabi_dmul_result =
                                    __aeabi_dmul(__aeabi_dmul_result, __aeabi_ui2d_result);
                                __aeabi_dmul_result =
                                    __aeabi_dadd(__aeabi_ui2d_result_2, __aeabi_dmul_result);
                                PICO_PART_U32(__aeabi_ui2d_result_2, 4) =
                                    (uint32_t)((uint64_t)__aeabi_dmul_result >> 0x20);
                                if ((PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000) <
                                    0x7ca00000) {
                                    __aeabi_d2uiz_result =
                                        PICO_PART_U32(__aeabi_ui2d_result_2, 4) + 0x3500000;
                                    PICO_PART_U32(__aeabi_ui2d_result_2, 0) =
                                        PICO_SUB_U32(__aeabi_dmul_result, 0);
                                    __aeabi_ui2d_result_2 = (double)pico_join_u32_u32(
                                        __aeabi_d2uiz_result, (uint32_t)__aeabi_ui2d_result_2);
                                    goto branch_23;
                                }
                                if ((__aeabi_d2uiz_result == 0x7fefffff) &&
                                    ((uint32_t)__aeabi_ui2d_result_2 == 0xffffffff))
                                    goto branch_15;
                                __aeabi_ui2d_result_2 = 1.79769313486232e+308;
                            } else {
                                if ((value_78 != 0) && (value_23 < 0x6a00001)) {
                                    value_19 = __aeabi_dcmple(__aeabi_dmul_result_2, 2147483647.0);
                                    if (value_19 != 0) {
                                        __aeabi_d2uiz_result = __aeabi_d2uiz(__aeabi_dmul_result_2);
                                        if (__aeabi_d2uiz_result == 0) {
                                            __aeabi_dmul_result_2 = 1.0;
                                        } else {
                                            __aeabi_dmul_result_2 =
                                                __aeabi_ui2d(__aeabi_d2uiz_result);
                                        }
                                        if (text_length == 0) {
                                            value_2 = PICO_PART_U32(__aeabi_dmul_result_2, 4) +
                                                      -0x80000000;
                                        } else {
                                            value_2 = PICO_PART_U32(__aeabi_dmul_result_2, 4);
                                        }
                                        __aeabi_dmul_result = (double)pico_join_u32_u32(
                                            value_2, (uint32_t)__aeabi_dmul_result_2);
                                    }
                                    __aeabi_dmul_result = (double)pico_join_u32_u32(
                                        ((int)((uint64_t)__aeabi_dmul_result >> 0x20) + 0x6b00000) -
                                            value_23,
                                        PICO_SUB_U32(__aeabi_dmul_result, 0));
                                }
                                __aeabi_ui2d_result = (double)unresolved_floating_point_01(
                                    (uint32_t)__aeabi_ui2d_result_2,
                                    PICO_PART_U32(__aeabi_ui2d_result_2, 4));
                                __aeabi_dmul_result =
                                    __aeabi_dmul(__aeabi_dmul_result, __aeabi_ui2d_result);
                                __aeabi_dmul_result =
                                    __aeabi_dadd(__aeabi_ui2d_result_2, __aeabi_dmul_result);
                                PICO_PART_U32(__aeabi_ui2d_result_2, 4) =
                                    (uint32_t)((uint64_t)__aeabi_dmul_result >> 0x20);
                                __aeabi_d2uiz_result = PICO_PART_U32(__aeabi_ui2d_result_2, 4);
                                __aeabi_ui2d_result_2 = __aeabi_dmul_result;
                            branch_23:
                                if ((value_78 == 0) &&
                                    (value_23 == (__aeabi_d2uiz_result & 0x7ff00000))) {
                                    value_19 = __aeabi_d2iz(__aeabi_dmul_result_2);
                                    __aeabi_dmul_result = __aeabi_i2d(value_19);
                                    __aeabi_dmul_result =
                                        __aeabi_dsub(__aeabi_dmul_result_2, __aeabi_dmul_result);
                                    if ((text_length == 0) &&
                                        (((uint32_t)__aeabi_ui2d_result_2 == 0 &&
                                          ((__aeabi_d2uiz_result & 0xfffff) == 0)))) {
                                        value_19 = __aeabi_dcmplt(__aeabi_dmul_result, 0.24999995);
                                    joined_r0x2001cb42:
                                        if (value_19 == 0)
                                            goto branch_24;
                                    } else {
                                        value_19 = __aeabi_dcmplt(__aeabi_dmul_result, 0.4999999);
                                        if (value_19 == 0) {
                                            value_19 =
                                                __aeabi_dcmpgt(__aeabi_dmul_result, 0.5000001);
                                            goto joined_r0x2001cb42;
                                        }
                                    }
                                    goto branch_20;
                                }
                            }
                        branch_24:
                            unresolved_c_library_18((int)reentrancy, pointer_38);
                            unresolved_c_library_18((int)reentrancy, value);
                            unresolved_c_library_18((int)reentrancy, value_00);
                            unresolved_c_library_18((int)reentrancy, pointer_88);
                            goto branch_14;
                        }
                        if (text_length == 0) {
                            if ((((uint64_t)__aeabi_ui2d_result_2 & 0xfffff00000000) == 0) &&
                                ((uint32_t)__aeabi_ui2d_result_2 == 0))
                                goto branch_17;
                        } else if ((PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0xfffff) == 0xfffff) {
                            if ((value_78 == 0) ||
                                (0x6a00000 <
                                 (PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000))) {
                                value_17 = 0xffffffff;
                            } else {
                                value_17 = -1 << (0x6b - ((PICO_PART_U32(__aeabi_ui2d_result_2, 4) &
                                                           0x7ff00000) >>
                                                          0x14) &
                                                  0xff);
                            }
                            if ((uint32_t)__aeabi_ui2d_result_2 == value_17) {
                                if ((PICO_PART_U32(__aeabi_ui2d_result_2, 4) != 0x7fefffff) ||
                                    ((uint32_t)__aeabi_ui2d_result_2 != 0xffffffff)) {
                                    __aeabi_ui2d_result_2 =
                                        (double)((uint64_t)((PICO_PART_U32(__aeabi_ui2d_result_2,
                                                                           4) &
                                                             0x7ff00000) +
                                                            0x100000)
                                                 << 0x20);
                                    goto branch_16;
                                }
                                goto branch_15;
                            }
                        }
                        if (value_14 == 0) {
                            value_14 = value_21 & (uint32_t)__aeabi_ui2d_result_2;
                        } else {
                            value_14 = value_14 & PICO_PART_U32(__aeabi_ui2d_result_2, 4);
                        }
                        if (value_14 == 0)
                            goto branch_16;
                        if (text_length == 0) {
                            __aeabi_dmul_result = unresolved_c_library_07_2(
                                (uint32_t)__aeabi_ui2d_result_2,
                                PICO_PART_U32(__aeabi_ui2d_result_2, 4), value_78);
                            __aeabi_ui2d_result_2 =
                                __aeabi_dsub(__aeabi_ui2d_result_2, __aeabi_dmul_result);
                            value_19 = __aeabi_dcmpeq(__aeabi_ui2d_result_2, 0.0);
                            if (value_19 != 0) {
                            branch_18:
                                *reentrancy = 0x22;
                                __aeabi_ui2d_result_2 = 0.0;
                                goto branch_20;
                            }
                        } else {
                            __aeabi_dmul_result = unresolved_c_library_07_2(
                                (uint32_t)__aeabi_ui2d_result_2,
                                PICO_PART_U32(__aeabi_ui2d_result_2, 4), value_78);
                            __aeabi_ui2d_result_2 =
                                __aeabi_dadd(__aeabi_ui2d_result_2, __aeabi_dmul_result);
                        }
                    }
                branch_16:
                    __aeabi_dmul_result = __aeabi_ui2d_result_2;
                    if (value_78 != 0)
                        goto branch_19;
                branch_21:
                    __aeabi_ui2d_result_2 = __aeabi_dmul_result;
                    goto branch_20;
                }
            } else {
                if ((value_20 & 0xf) != 0) {
                    __aeabi_ui2d_result_2 =
                        __aeabi_dmul(*(double *)(&g_libc_unknown_09 + (value_20 & 0xf) * 8),
                                     __aeabi_ui2d_result_2);
                }
                if ((value_20 & 0xfffffff0) == 0)
                    goto branch_13;
                if ((int)(value_20 & 0xfffffff0) < 0x135) {
                    value_20 = (int)value_20 >> 4;
                    if ((int)value_20 < 2) {
                        libc_strncmp_result = 0;
                    } else {
                        pointer_18 = (double *)&g_libc_unknown_10;
                        libc_strncmp_result = 0;
                        do {
                            text_length = value_20 & 1;
                            libc_strncmp_result = libc_strncmp_result + 1;
                            value_20 = (int)value_20 >> 1;
                            if (text_length != 0) {
                                __aeabi_ui2d_result_2 =
                                    __aeabi_dmul(__aeabi_ui2d_result_2, *pointer_18);
                            }
                            pointer_18 = pointer_18 + 1;
                        } while (1 < (int)value_20);
                    }
                    __aeabi_ui2d_result_2 = (double)pico_join_u32_u32(
                        PICO_PART_U32(__aeabi_ui2d_result_2, 4) + -0x3500000,
                        (uint32_t)__aeabi_ui2d_result_2);
                    __aeabi_dmul_result =
                        __aeabi_dmul(*(double *)(&g_libc_unknown_10 + libc_strncmp_result * 2),
                                     __aeabi_ui2d_result_2);
                    PICO_PART_U32(__aeabi_ui2d_result_2, 4) =
                        (uint32_t)((uint64_t)__aeabi_dmul_result >> 0x20);
                    if (0x7ca00000 < (PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000))
                        goto branch_25;
                    if ((PICO_PART_U32(__aeabi_ui2d_result_2, 4) & 0x7ff00000) < 0x7c900001) {
                        PICO_PART_U32(__aeabi_ui2d_result_2, 0) =
                            PICO_SUB_U32(__aeabi_dmul_result, 0);
                        __aeabi_ui2d_result_2 = (double)pico_join_u32_u32(
                            PICO_PART_U32(__aeabi_ui2d_result_2, 4) + 0x3500000,
                            (uint32_t)__aeabi_ui2d_result_2);
                        value_78 = 0;
                    } else {
                        value_78 = 0;
                        __aeabi_ui2d_result_2 = 1.79769313486232e+308;
                    }
                    goto branch_12;
                }
            }
        branch_25:
            __aeabi_ui2d_result_2 = INFINITY;
            *reentrancy = 0x22;
            goto branch_5;
        }
        if (value_24 != 0) {
        branch_26:
            pointer_10 = pointer;
            value_17 = (uint32_t)pointer[1];
            if (value_17 == 0x2b) {
                condition_3 = false;
            branch_27:
                value_17 = (uint32_t)pointer[2];
                pointer = pointer + 2;
            } else {
                if (value_17 == 0x2d) {
                    condition_3 = true;
                    goto branch_27;
                }
                condition_3 = false;
                pointer = pointer + 1;
            }
            if (value_17 - 0x30 < 10) {
                if (value_17 == 0x30) {
                    do {
                        pointer = pointer + 1;
                        value_17 = (uint32_t)*pointer;
                    } while (value_17 == 0x30);
                }
                pointer_10 = pointer;
                if (8 < value_17 - 0x31)
                    goto branch_11;
                pointer_11 = (uint8_t *)(value_17 - 0x30);
                value_17 = (uint32_t)pointer[1];
                pointer_13 = pointer + 1;
                if (value_17 - 0x30 < 10) {
                    pointer_4 = pointer + 2;
                    do {
                        pointer_13 = pointer_4;
                        value_12 = value_17 + (int)pointer_11 * 10;
                        value_17 = (uint32_t)*pointer_13;
                        pointer_11 = (uint8_t *)(value_12 + -0x30);
                        pointer_4 = pointer_13 + 1;
                    } while (value_17 - 0x30 < 10);
                }
                pointer = pointer_13;
                if ((int)pointer - (int)pointer_10 < 9) {
                    if (0x4e1e < (int)pointer_11) {
                        pointer_11 = &g_libc_unknown_01;
                    }
                } else {
                    pointer_11 = &g_libc_unknown_01;
                }
                if (condition_3) {
                    pointer_11 = (uint8_t *)-(int)pointer_11;
                }
            } else {
                pointer_11 = (uint8_t *)0x0;
                pointer = pointer_10;
            }
            goto joined_r0x2001c792;
        }
    branch_8:
        if ((value_15 != 0) || (condition_1))
            goto branch_26;
    }
switchD_2001c088_caseD_0:
    __aeabi_ui2d_result_2 = 0.0;
    value_84 = 0;
    goto branch_5;
branch_15:
    *reentrancy = 0x22;
    __aeabi_ui2d_result_2 = INFINITY;
branch_20:
    unresolved_c_library_18((int)reentrancy, pointer_38);
    unresolved_c_library_18((int)reentrancy, value);
    unresolved_c_library_18((int)reentrancy, value_00);
    unresolved_c_library_18((int)reentrancy, pointer_8);
    unresolved_c_library_18((int)reentrancy, pointer_88);
    goto branch_5;
    while (true) {
        pointer = pointer + 1;
        value_17 = (uint32_t)*pointer;
        if (value_17 - 0x41 < 0x1a) {
            value_17 = value_17 + 0x20;
        }
        if (value_17 != *pointer_10)
            break;
    branch_10:
        pointer_10 = pointer_10 + 1;
        if (*pointer_10 == 0)
            break;
    }
    __aeabi_ui2d_result_2 = INFINITY;
branch_5:
    if (value_84 == 0) {
        return __aeabi_ui2d_result_2;
    }
    return (double)pico_join_u32_u32((int)((uint64_t)__aeabi_ui2d_result_2 >> 0x20) + -0x80000000,
                                     PICO_SUB_U32(__aeabi_ui2d_result_2, 0));
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001aea4. */
int libc_atoi(char *text)
{
    uint8_t byte_value_1;
    uint8_t byte_value_2;
    uint32_t value_3;
    uint8_t *pointer_4;
    uint32_t value_5;
    int value_6;
    int value_7;
    uint32_t value_8;
    uint32_t value_9;

    while (true) {
        pointer_4 = (uint8_t *)(text + 1);
        value_5 = (uint32_t)(uint8_t)*text;
        byte_value_1 = *(uint8_t *)(g_libc_unknown_11 + value_5 + 1);
        byte_value_2 = byte_value_1 & 8;
        if ((byte_value_1 & 8) == 0)
            break;
        text = (char *)pointer_4;
    }
    if (value_5 == 0x2d) {
        pointer_4 = (uint8_t *)(text + 2);
        value_5 = (uint32_t)(uint8_t)text[1];
        byte_value_2 = 1;
    } else if (value_5 == 0x2b) {
        value_5 = (uint32_t)(uint8_t)text[1];
        pointer_4 = (uint8_t *)(text + 2);
    }
    if (byte_value_2 == 0) {
        value_9 = 0x7fffffff;
    } else {
        value_9 = 0x80000000;
    }
    value_8 = value_9 / 10;
    value_7 = 0;
    value_3 = 0;
    do {
        byte_value_1 = *(uint8_t *)(g_libc_unknown_11 + value_5 + 1);
        if ((byte_value_1 & 4) == 0) {
            if ((byte_value_1 & 3) == 0) {
            branch_1:
                if (value_7 == -1) {
                    if (byte_value_2 == 0) {
                        value_3 = 0x7fffffff;
                    } else {
                        value_3 = 0x80000000;
                    }
                    *libc_reentrancy_as_uint32_t_pointer = 0x22;
                } else if (byte_value_2 != 0) {
                    value_3 = -value_3;
                }
                return value_3;
            }
            if ((byte_value_1 & 3) == 1) {
                value_6 = 0x37;
            } else {
                value_6 = 0x57;
            }
            value_6 = value_5 - value_6;
        } else {
            value_6 = value_5 - 0x30;
        }
        if (9 < value_6)
            goto branch_1;
        if (value_7 != -1) {
            if ((value_8 <= value_3 && value_3 != value_8) ||
                ((value_3 == value_8 && ((int)(value_9 % 10) < value_6)))) {
                value_7 = -1;
            } else {
                value_3 = value_3 * 10 + value_6;
                value_7 = 1;
            }
        }
        value_5 = (uint32_t)*pointer_4;
        pointer_4 = pointer_4 + 1;
    } while (true);
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001aecc. */
void *libc_malloc(uint32_t count)
{
    int *pointer_1;

    pointer_1 = unresolved_c_library_03(libc_reentrancy, count);
    return pointer_1;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001aedc. */
void libc_free(void *allocation)
{
    unresolved_c_library_12(libc_reentrancy, (int)allocation);
    return;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001b424. */
int libc_memcmp(void *left, void *right, uint32_t count)
{
    int value_1;
    uint8_t *pointer_2;
    uint32_t value_3;
    uint8_t *pointer_4;
    uint32_t value_5;
    bool condition_6;

    if (3 < count) {
        if ((((uint32_t)left | (uint32_t)right) & 3) != 0)
            goto branch_1;
        pointer_2 = left;
        pointer_4 = right;
        do {
            left = pointer_2;
            right = pointer_4;
            if (*(int *)pointer_2 != *(int *)pointer_4)
                break;
            count = count - 4;
            left = pointer_2 + 4;
            right = pointer_4 + 4;
            pointer_2 = pointer_2 + 4;
            pointer_4 = pointer_4 + 4;
        } while (3 < count);
    }
    if (count == 0) {
        return 0;
    }
branch_1:
    value_5 = (uint32_t) * (uint8_t *)left;
    value_3 = (uint32_t) * (uint8_t *)right;
    if (value_5 == value_3) {
        value_1 = 0;
        do {
            condition_6 = value_1 == count - 1;
            value_1 = value_1 + 1;
            if (condition_6) {
                return 0;
            }
            left = (void *)((int)left + 1);
            value_5 = (uint32_t) * (uint8_t *)left;
            right = (void *)((int)right + 1);
            value_3 = (uint32_t) * (uint8_t *)right;
        } while (value_5 == value_3);
    }
    return value_5 - value_3;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001b48c. */
void *libc_memcpy(void *destination, void *source, uint32_t count)
{
    uint8_t *pointer_1;
    uint32_t *pointer_2;
    uint32_t *pointer_3;
    uint8_t *pointer_4;
    uint32_t *pointer_5;
    uint8_t *pointer_6;
    uint32_t value_7;
    uint32_t value_8;
    bool condition_9;

    pointer_3 = destination;
    if ((((uint32_t)source | (uint32_t)destination) & 3) != 0) {
        if (count < 8) {
            value_8 = count - 4;
            pointer_1 = destination;
            if (3 < count) {
                do {
                    pointer_6 = source;
                    pointer_4 = pointer_1;
                    condition_9 = value_8 != 0;
                    value_8 = value_8 - 1;
                    *pointer_4 = *pointer_6;
                    pointer_1 = pointer_4 + 1;
                    source = pointer_6 + 1;
                } while (condition_9);
                pointer_4[1] = pointer_6[1];
                pointer_4[2] = pointer_6[2];
                pointer_4[3] = pointer_6[3];
                return destination;
            }
            goto branch_1;
        }
        if ((((uint32_t)source & 3) != 0) && (value_8 = (uint32_t)destination & 3, value_8 != 0)) {
            count = count - (4 - value_8);
            pointer_2 = destination;
            pointer_5 = source;
            if (((uint32_t)destination & 1) != 0) {
                pointer_5 = (uint32_t *)((int)source + 1);
                pointer_2 = (uint32_t *)((int)destination + 1);
                *(uint8_t *)destination = *(uint8_t *)source;
            }
            pointer_3 = pointer_2;
            source = pointer_5;
            if ((value_8 * -0x40000000 & 0x80000000) != 0) {
                source = (void *)((int)pointer_5 + 2);
                pointer_3 = (uint32_t *)((int)pointer_2 + 2);
                *(uint16_t *)pointer_2 = *(uint16_t *)pointer_5;
            }
        }
    }
    while (0x3f < count) {
        *pointer_3 = *(uint32_t *)source;
        pointer_3[1] = *(uint32_t *)((int)source + 4);
        pointer_3[2] = *(uint32_t *)((int)source + 8);
        pointer_3[3] = *(uint32_t *)((int)source + 0xc);
        pointer_3[4] = *(uint32_t *)((int)source + 0x10);
        pointer_3[5] = *(uint32_t *)((int)source + 0x14);
        pointer_3[6] = *(uint32_t *)((int)source + 0x18);
        pointer_3[7] = *(uint32_t *)((int)source + 0x1c);
        pointer_3[8] = *(uint32_t *)((int)source + 0x20);
        pointer_3[9] = *(uint32_t *)((int)source + 0x24);
        pointer_3[10] = *(uint32_t *)((int)source + 0x28);
        pointer_3[0xb] = *(uint32_t *)((int)source + 0x2c);
        pointer_3[0xc] = *(uint32_t *)((int)source + 0x30);
        pointer_3[0xd] = *(uint32_t *)((int)source + 0x34);
        pointer_3[0xe] = *(uint32_t *)((int)source + 0x38);
        pointer_3[0xf] = *(uint32_t *)((int)source + 0x3c);
        pointer_3 = pointer_3 + 0x10;
        source = (uint32_t *)((int)source + 0x40);
        count = count - 0x40;
    }
    value_7 = count - 0x10;
    if (0xffffffcf < count - 0x40) {
        do {
            *pointer_3 = *(uint32_t *)source;
            pointer_3[1] = *(uint32_t *)((int)source + 4);
            pointer_2 = (uint32_t *)((int)source + 0xc);
            pointer_5 = pointer_3 + 3;
            pointer_3[2] = *(uint32_t *)((int)source + 8);
            source = (void *)((int)source + 0x10);
            pointer_3 = pointer_3 + 4;
            *pointer_5 = *pointer_2;
            condition_9 = 0xf < value_7;
            value_7 = value_7 - 0x10;
        } while (condition_9);
    }
    value_8 = value_7 + 0xc;
    pointer_5 = pointer_3;
    pointer_2 = source;
    if (0xfffffff3 < value_7) {
        do {
            source = pointer_2 + 1;
            *pointer_5 = *pointer_2;
            condition_9 = 3 < value_8;
            value_8 = value_8 - 4;
            pointer_3 = pointer_5 + 1;
            pointer_5 = pointer_5 + 1;
            pointer_2 = source;
        } while (condition_9);
    }
branch_1:
    if (value_8 != 0xfffffffc) {
        pointer_2 = pointer_3;
        pointer_5 = source;
        if ((value_8 & 1) != 0) {
            pointer_5 = (uint32_t *)((int)source + 1);
            pointer_2 = (uint32_t *)((int)pointer_3 + 1);
            *(uint8_t *)pointer_3 = *(uint8_t *)source;
        }
        if ((value_8 & 2) != 0) {
            *(uint16_t *)pointer_2 = *(uint16_t *)pointer_5;
        }
    }
    return destination;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001b5c0. */
void *libc_memmove(void *destination, void *source, uint32_t count)
{
    int value_1;
    uint32_t *pointer_2;
    uint32_t *pointer_3;
    uint32_t *pointer_4;
    uint8_t *pointer_5;
    uint8_t *pointer_6;
    uint32_t value_7;
    uint32_t value_8;

    if ((source < destination) &&
        (pointer_5 = (uint8_t *)((int)source + count), destination < pointer_5)) {
        pointer_6 = (uint8_t *)((int)destination + count);
        if (count != 0) {
            do {
                count = count - 1;
                pointer_5 = pointer_5 + -1;
                pointer_6 = pointer_6 + -1;
                *pointer_6 = *pointer_5;
            } while (count != 0);
        }
    } else {
        pointer_2 = destination;
        if (0xf < count) {
            if ((((uint32_t)destination | (uint32_t)source) & 3) != 0)
                goto branch_1;
            value_7 = count - 0x10 >> 4;
            pointer_3 = source;
            do {
                *pointer_2 = *pointer_3;
                pointer_2[1] = pointer_3[1];
                pointer_2[2] = pointer_3[2];
                pointer_2[3] = pointer_3[3];
                pointer_2 = pointer_2 + 4;
                pointer_3 = pointer_3 + 4;
            } while (pointer_2 != (uint32_t *)((int)destination + value_7 * 0x10 + 0x10));
            value_8 = count & 0xf;
            value_1 = (value_7 + 1) * 0x10;
            source = (void *)((int)source + value_1);
            pointer_2 = (uint32_t *)(value_1 + (int)destination);
            if (value_8 < 4) {
                count = value_8;
                if (value_8 == 0) {
                    return destination;
                }
                goto branch_1;
            }
            value_7 = value_8;
            pointer_3 = pointer_2;
            pointer_4 = source;
            do {
                value_7 = value_7 - 4;
                *pointer_3 = *pointer_4;
                pointer_3 = pointer_3 + 1;
                pointer_4 = pointer_4 + 1;
            } while (3 < value_7);
            value_1 = (value_8 - 4 & 0xfffffffc) + 4;
            count = count & 3;
            source = (void *)((int)source + value_1);
            pointer_2 = (uint32_t *)((int)pointer_2 + value_1);
        }
        if (count != 0) {
        branch_1:
            pointer_3 = pointer_2;
            do {
                pointer_4 = (uint32_t *)((int)pointer_3 + 1);
                *(uint8_t *)pointer_3 = *(uint8_t *)source;
                source = (uint32_t *)((int)source + 1);
                pointer_3 = pointer_4;
            } while (pointer_4 != (uint32_t *)(count + (int)pointer_2));
            return destination;
        }
    }
    return destination;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001b680. */
void *libc_memset(void *destination_2, int value, uint32_t count)
{
    uint32_t *pointer_1;
    uint32_t *destination;
    uint32_t *pointer_3;
    uint32_t value_4;
    uint32_t value_5;

    if (((uint32_t)destination_2 & 3) == 0) {
        destination = destination_2;
    } else {
        if (count == 0) {
            return destination_2;
        }
        pointer_1 = destination_2;
        while (true) {
            count = count - 1;
            destination = (uint32_t *)((int)pointer_1 + 1);
            *(char *)pointer_1 = (char)value;
            if (((uint32_t)destination & 3) == 0)
                break;
            if (count == 0) {
                return destination_2;
            }
            pointer_1 = destination;
        }
    }
    if (3 < count) {
        value_4 = value & 0xffU | (value & 0xffU) << 8;
        value_4 = value_4 | value_4 << 0x10;
        if (0xf < count) {
            value_5 = count - 0x10 >> 4;
            pointer_1 = destination;
            do {
                *pointer_1 = value_4;
                pointer_1[1] = value_4;
                pointer_1[2] = value_4;
                pointer_1[3] = value_4;
                pointer_1 = pointer_1 + 4;
            } while (pointer_1 != destination + value_5 * 4 + 4);
            count = count & 0xf;
            destination = destination + (value_5 + 1) * 4;
            if (count < 4)
                goto branch_1;
        }
        value_5 = count;
        pointer_1 = destination;
        do {
            value_5 = value_5 - 4;
            *pointer_1 = value_4;
            pointer_1 = pointer_1 + 1;
        } while (3 < value_5);
        destination = (uint32_t *)((int)destination + (count - 4 & 0xfffffffc) + 4);
        count = count & 3;
    }
branch_1:
    if (count != 0) {
        pointer_1 = destination;
        do {
            pointer_3 = (uint32_t *)((int)pointer_1 + 1);
            *(char *)pointer_1 = (char)value;
            pointer_1 = pointer_3;
        } while (pointer_3 != (uint32_t *)(count + (int)destination));
    }
    return destination_2;
}

/* Wrapper supplies global reentrancy context and branches to realloc implementation; called by
 * String_change_buffer(size+1). Firmware address: 0x2001b71c. */
void *libc_realloc(void *allocation, uint32_t count)
{
    int *pointer_1;

    pointer_1 = unresolved_c_library_06(libc_reentrancy, allocation, count);
    return pointer_1;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001bb1c. */
int libc_snprintf(char *destination, uint32_t capacity, char *format, ...)
{
    int *pointer_1;
    int value_2;
    uint32_t unresolved_input_r3;
    char *buffer_88[2];
    int value_80;
    uint16_t short_value;
    uint16_t short_value_2;
    char *text_78;
    int value_74;
    uint32_t value_4;

    pointer_1 = libc_reentrancy;
    if ((int)capacity < 0) {
        *libc_reentrancy = 0x8b;
        value_2 = -1;
    } else {
        if (capacity == 0) {
            value_80 = 0;
        } else {
            value_80 = capacity - 1;
        }
        short_value = 0x208;
        short_value_2 = 0xffff;
        buffer_88[0] = destination;
        text_78 = destination;
        value_74 = value_80;
        value_4 = unresolved_input_r3;
        value_2 =
            unresolved_c_library_08(libc_reentrancy, (int *)buffer_88, (uint8_t *)format, &value_4);
        if (value_2 < -1) {
            *pointer_1 = 0x8b;
        }
        if (capacity != 0) {
            *buffer_88[0] = '\0';
        }
    }
    return value_2;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001bb84. */
char *libc_strcat(char *destination, char *source)
{
    char byte_value_1;
    uint32_t *pointer_2;

    pointer_2 = (uint32_t *)destination;
    if ((((uint32_t)destination & 3) == 0) &&
        ((*(uint32_t *)destination + 0xfefefeff & ~*(uint32_t *)destination & 0x80808080) == 0)) {
        do {
            pointer_2 = pointer_2 + 1;
        } while ((*pointer_2 + 0xfefefeff & ~*pointer_2 & 0x80808080) == 0);
    }
    byte_value_1 = (char)*pointer_2;
    while (byte_value_1 != '\0') {
        pointer_2 = (uint32_t *)((int)pointer_2 + 1);
        byte_value_1 = *(char *)pointer_2;
    }
    libc_strcpy((char *)pointer_2, source);
    return destination;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001bbd0. */
int libc_strcmp(char *left, char *right)
{
    uint32_t *pointer_1;
    int value_2;
    uint8_t *pointer_3;
    uint8_t *pointer_4;
    uint32_t *pointer_5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t value_8;
    uint32_t value_9;
    uint32_t value_10;

    if ((((uint32_t)left | (uint32_t)right) & 7) != 0) {
        pointer_1 = (uint32_t *)left;
        if (((uint32_t)left & 3) != 0) {
            pointer_1 = (uint32_t *)((uint32_t)left & 0xfffffffc) + 1;
            value_6 = *(uint32_t *)((uint32_t)left & 0xfffffffc);
            pointer_3 = (uint8_t *)right;
            if (((uint32_t)left & 1) == 0) {
            branch_1:
                pointer_4 = pointer_3 + 1;
                value_7 = value_6 >> 0x10 & 0xff;
                value_2 = value_7 - *pointer_3;
                if (value_2 != 0) {
                    return value_2;
                }
                if (value_7 == 0) {
                    return 0;
                }
            } else {
                pointer_4 = (uint8_t *)right;
                if (((uint32_t)left & 2) == 0) {
                    pointer_3 = (uint8_t *)(right + 1);
                    value_7 = value_6 >> 8 & 0xff;
                    value_2 = value_7 - (uint8_t)*right;
                    if (value_2 != 0) {
                        return value_2;
                    }
                    if (value_7 == 0) {
                        return 0;
                    }
                    goto branch_1;
                }
            }
            right = (char *)(pointer_4 + 1);
            value_2 = (value_6 >> 0x18) - (uint32_t)*pointer_4;
            if ((value_2 != 0) || (value_6 >> 0x18 == 0)) {
                return value_2;
            }
        }
        value_6 = (uint32_t)right & 3;
        if (value_6 != 0) {
            pointer_5 = (uint32_t *)((uint32_t)right & 0xfffffffc);
            if (value_6 == 2) {
                value_6 = *pointer_5;
                do {
                    pointer_5 = pointer_5 + 1;
                    value_8 = *pointer_1;
                    value_6 = value_6 >> 0x10;
                    value_10 = pico_join_u8_u24(
                        -((char)(value_8 >> 0x18) == '\0'),
                        pico_join_u8_u16(-((char)(value_8 >> 0x10) == '\0'),
                                         pico_join_u8_u8(-((char)(value_8 >> 8) == '\0'),
                                                         -((char)value_8 == '\0'))));
                    value_7 = value_8 & 0xffff;
                    if (value_10 * -0x10000 != 0 || value_7 != value_6)
                        goto branch_2;
                    value_6 = *pointer_5;
                    value_7 = value_7 ^ value_8;
                    value_8 = value_6 * 0x10000;
                    pointer_1 = pointer_1 + 1;
                } while (value_10 == 0 && value_7 == value_8);
            } else if (value_6 < 2) {
                value_6 = *pointer_5;
                do {
                    pointer_5 = pointer_5 + 1;
                    value_8 = *pointer_1;
                    value_6 = value_6 >> 8;
                    value_10 = pico_join_u8_u24(
                        -((char)(value_8 >> 0x18) == '\0'),
                        pico_join_u8_u16(-((char)(value_8 >> 0x10) == '\0'),
                                         pico_join_u8_u8(-((char)(value_8 >> 8) == '\0'),
                                                         -((char)value_8 == '\0'))));
                    value_7 = value_8 & 0xffffff;
                    if (value_10 * -0x100 != 0 || value_7 != value_6)
                        goto branch_2;
                    value_6 = *pointer_5;
                    value_7 = value_7 ^ value_8;
                    value_8 = value_6 * 0x1000000;
                    pointer_1 = pointer_1 + 1;
                } while (value_10 == 0 && value_7 == value_8);
            } else {
                value_6 = *pointer_5;
                do {
                    pointer_5 = pointer_5 + 1;
                    value_8 = *pointer_1;
                    value_6 = value_6 >> 0x18;
                    value_10 = pico_join_u8_u24(
                        -((char)(value_8 >> 0x18) == '\0'),
                        pico_join_u8_u16(-((char)(value_8 >> 0x10) == '\0'),
                                         pico_join_u8_u8(-((char)(value_8 >> 8) == '\0'),
                                                         -((char)value_8 == '\0'))));
                    value_7 = value_8 & 0xff;
                    if (value_10 * -0x1000000 != 0 || value_7 != value_6)
                        goto branch_2;
                    value_6 = *pointer_5;
                    value_7 = value_7 ^ value_8;
                    value_8 = value_6 * 0x100;
                    pointer_1 = pointer_1 + 1;
                } while (value_10 == 0 && value_7 == value_8);
            }
            value_7 = value_7 << 0x18 | (value_7 >> 8 & 0xff) << 0x10 |
                      (value_7 >> 0x10 & 0xff) << 8 | value_7 >> 0x18;
            value_6 =
                (value_8 >> 8 & 0xff) << 0x10 | (value_8 >> 0x10 & 0xff) << 8 | value_8 >> 0x18;
            goto branch_3;
        }
        left = (char *)pointer_1;
        pointer_5 = (uint32_t *)right;
        if (((uint32_t)pointer_1 & 4) != 0) {
            left = (char *)(pointer_1 + 1);
            value_6 = *pointer_1;
            pointer_5 = (uint32_t *)((int)right + 4);
            value_8 = *(uint32_t *)right;
            value_10 =
                pico_join_u8_u24(-((char)(value_6 >> 0x18) == '\0'),
                                 pico_join_u8_u16(-((char)(value_6 >> 0x10) == '\0'),
                                                  pico_join_u8_u8(-((char)(value_6 >> 8) == '\0'),
                                                                  -((char)value_6 == '\0'))));
            if (value_6 != value_8 || value_10 != 0) {
            branch_4:
                value_7 = value_6 << 0x18 | (value_6 >> 8 & 0xff) << 0x10 |
                          (value_6 >> 0x10 & 0xff) << 8 | value_6 >> 0x18;
                value_6 = value_8 << 0x18 | (value_8 >> 8 & 0xff) << 0x10 |
                          (value_8 >> 0x10 & 0xff) << 8 | value_8 >> 0x18;
                goto branch_3;
            }
        }
        right = (char *)pointer_5;
        if (((uint32_t)pointer_5 & 4) != 0) {
            value_6 = *pointer_5;
            pointer_1 = pointer_5 + 1;
            do {
                value_7 = *(uint32_t *)left;
                value_8 = *(uint32_t *)((int)left + 4);
                value_10 = pico_join_u8_u24(
                    -((char)(value_7 >> 0x18) == '\0'),
                    pico_join_u8_u16(-((char)(value_7 >> 0x10) == '\0'),
                                     pico_join_u8_u8(-((char)(value_7 >> 8) == '\0'),
                                                     -((char)value_7 == '\0'))));
                if (value_7 != value_6 || value_10 != 0)
                    goto branch_2;
                value_9 = *pointer_1;
                value_6 = pointer_1[1];
                value_10 = pico_join_u8_u24(
                    -((char)(value_8 >> 0x18) == '\0'),
                    pico_join_u8_u16(-((char)(value_8 >> 0x10) == '\0'),
                                     pico_join_u8_u8(-((char)(value_8 >> 8) == '\0'),
                                                     -((char)value_8 == '\0'))));
                left = (char *)((int)left + 8);
                pointer_1 = pointer_1 + 2;
            } while (value_8 == value_9 && value_10 == 0);
            value_7 = value_8 << 0x18 | (value_8 >> 8 & 0xff) << 0x10 |
                      (value_8 >> 0x10 & 0xff) << 8 | value_8 >> 0x18;
            value_6 = value_9 << 0x18 | (value_9 >> 8 & 0xff) << 0x10 |
                      (value_9 >> 0x10 & 0xff) << 8 | value_9 >> 0x18;
            goto branch_3;
        }
    }
    do {
        value_6 = *(uint32_t *)left;
        value_7 = *(uint32_t *)((int)left + 4);
        value_8 = *(uint32_t *)right;
        value_9 = *(uint32_t *)((int)right + 4);
        value_10 =
            pico_join_u8_u24(-((char)(value_6 >> 0x18) == '\0'),
                             pico_join_u8_u16(-((char)(value_6 >> 0x10) == '\0'),
                                              pico_join_u8_u8(-((char)(value_6 >> 8) == '\0'),
                                                              -((char)value_6 == '\0'))));
        if (value_6 != value_8 || value_10 != 0)
            goto branch_4;
        value_10 =
            pico_join_u8_u24(-((char)(value_7 >> 0x18) == '\0'),
                             pico_join_u8_u16(-((char)(value_7 >> 0x10) == '\0'),
                                              pico_join_u8_u8(-((char)(value_7 >> 8) == '\0'),
                                                              -((char)value_7 == '\0'))));
        left = (char *)((int)left + 8);
        right = (char *)((int)right + 8);
    } while (value_7 == value_9 && value_10 == 0);
    value_7 = value_7 << 0x18 | (value_7 >> 8 & 0xff) << 0x10 | (value_7 >> 0x10 & 0xff) << 8 |
              value_7 >> 0x18;
    value_6 = value_9 << 0x18 | (value_9 >> 8 & 0xff) << 0x10 | (value_9 >> 0x10 & 0xff) << 8 |
              value_9 >> 0x18;
branch_3:
    value_8 = value_10 << 0x18 | (value_10 >> 8 & 0xff) << 0x10 | (value_10 >> 0x10 & 0xff) << 8 |
              value_10 >> 0x18;
    if (value_8 != 0) {
        value_8 = 0x18 - pico_count_leading_zeros_u32(value_8);
        value_7 = value_7 >> (value_8 & 0xff);
        value_6 = value_6 >> (value_8 & 0xff);
    }
    value_2 = 1;
    if (value_7 <= value_6) {
        value_2 = -(uint32_t)(value_7 < value_6);
    }
    return value_2;
branch_2:
    value_7 = value_7 << 0x18 | (value_7 >> 8 & 0xff) << 0x10 | (value_7 >> 0x10 & 0xff) << 8 |
              value_7 >> 0x18;
    value_6 = value_6 << 0x18 | (value_6 >> 8 & 0xff) << 0x10 | (value_6 >> 0x10 & 0xff) << 8 |
              value_6 >> 0x18;
    goto branch_3;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001bdec. */
char *libc_strcpy(char *destination, char *source)
{
    char byte_value_1;
    uint16_t short_value_2;
    uint32_t *pointer_3;
    uint32_t *pointer_4;
    uint32_t value_5;
    uint32_t value_6;
    uint32_t *pointer_7;
    uint32_t *pointer_8;
    char *text_9;
    bool condition_10;

    text_9 = destination;
    if ((((uint32_t)destination ^ (uint32_t)source) & 3) != 0) {
        do {
            byte_value_1 = *source;
            *text_9 = byte_value_1;
            source = source + 1;
            text_9 = text_9 + 1;
        } while (byte_value_1 != '\0');
        return destination;
    }
    pointer_8 = (uint32_t *)destination;
    if (((uint32_t)source & 3) != 0) {
        pointer_4 = (uint32_t *)source;
        if (((uint32_t)source & 1) != 0) {
            pointer_4 = (uint32_t *)(source + 1);
            byte_value_1 = *source;
            pointer_8 = (uint32_t *)(destination + 1);
            *destination = byte_value_1;
            if (byte_value_1 == '\0') {
                return destination;
            }
        }
        source = (char *)pointer_4;
        if (((uint32_t)pointer_4 & 2) != 0) {
            source = (char *)((int)pointer_4 + 2);
            short_value_2 = (uint16_t)*pointer_4;
            condition_10 = (short_value_2 & 0xff) != 0;
            if (condition_10) {
                *(uint16_t *)pointer_8 = short_value_2;
                pointer_8 = (uint32_t *)((int)pointer_8 + 2);
            } else {
                *(char *)pointer_8 = (char)*pointer_4;
            }
            if (!condition_10 || (short_value_2 & 0xff00) == 0) {
                return destination;
            }
        }
    }
    value_5 = *(uint32_t *)source;
    pointer_4 = (uint32_t *)((int)source + 4);
    pointer_7 = pointer_8;
    if (((uint32_t)source & 4) != 0) {
        if ((value_5 + 0xfefefeff & ~value_5 & 0x80808080) != 0)
            goto branch_1;
        pointer_7 = pointer_8 + 1;
        *pointer_8 = value_5;
        pointer_4 = (uint32_t *)((int)source + 8);
        value_5 = *(uint32_t *)((int)source + 4);
    }
    while (true) {
        pointer_3 = pointer_4 + 1;
        value_6 = *pointer_4;
        pointer_8 = pointer_7;
        if ((value_5 + 0xfefefeff & ~value_5 & 0x80808080) != 0)
            break;
        pointer_8 = pointer_7 + 1;
        *pointer_7 = value_5;
        if ((value_6 + 0xfefefeff & ~value_6 & 0x80808080) != 0) {
            value_5 = value_6;
            break;
        }
        pointer_4 = pointer_4 + 2;
        value_5 = *pointer_3;
        pointer_7 = pointer_7 + 2;
        *pointer_8 = value_6;
    }
branch_1:
    do {
        *(char *)pointer_8 = (char)value_5;
        value_6 = value_5 & 0xff;
        value_5 = value_5 >> 8 | value_5 << 0x18;
        pointer_8 = (uint32_t *)((int)pointer_8 + 1);
    } while (value_6 != 0);
    return destination;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001bea8. */
uint32_t libc_strlen(char *text)
{
    uint32_t value_1;
    uint32_t *pointer_2;
    uint32_t value_3;
    bool condition_4;
    bool condition_5;

    pointer_2 = (uint32_t *)((uint32_t)text & 0xfffffffc);
    value_1 = -((uint32_t)text & 3);
    value_3 = *pointer_2;
    if (((uint32_t)text & 3) != 0) {
        value_3 = value_3 | 0xffffffffU >> ((value_1 + 4) * 8 & 0xff);
    }
    while (pointer_2 = pointer_2 + 1, (value_3 + 0xfefefeff & ~value_3 & 0x80808080) == 0) {
        value_1 = value_1 + 4;
        value_3 = *pointer_2;
    }
    condition_4 = (value_3 & 0xff) != 0;
    if (condition_4) {
        value_1 = value_1 + 1;
    }
    condition_5 = (value_3 & 0xff00) != 0;
    if (condition_4 && condition_5) {
        value_1 = value_1 + 1;
    }
    if ((condition_4 && condition_5) && (value_3 & 0xff0000) != 0) {
        value_1 = value_1 + 1;
    }
    return value_1;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001bf08. */
int libc_strncmp(char *left, char *right, uint32_t count)
{
    uint32_t value_1;
    uint8_t *pointer_2;
    uint32_t value_3;
    uint8_t *pointer_4;

    if (count == 0) {
        return 0;
    }
    if ((((((uint32_t)left | (uint32_t)right) & 3) == 0) && (3 < count)) &&
        (value_3 = *(uint32_t *)left, value_3 == *(uint32_t *)right)) {
        count = count - 4;
        if (count == 0) {
            return 0;
        }
        if ((value_3 + 0xfefefeff & ~value_3 & 0x80808080) != 0) {
            return 0;
        }
        pointer_2 = (uint8_t *)left;
        pointer_4 = (uint8_t *)right;
        while (true) {
            left = (char *)(pointer_2 + 4);
            right = (char *)(pointer_4 + 4);
            if (count < 4)
                break;
            value_3 = *(uint32_t *)(pointer_2 + 4);
            if (value_3 != *(uint32_t *)(pointer_4 + 4))
                goto branch_1;
            count = count - 4;
            if (count == 0) {
                return 0;
            }
            if ((value_3 + 0xfefefeff & ~value_3 & 0x80808080) != 0) {
                return 0;
            }
            pointer_2 = (uint8_t *)left;
            pointer_4 = (uint8_t *)right;
        }
        if (count == 0) {
            value_3 = (uint32_t)pointer_2[4];
            value_1 = (uint32_t)pointer_4[4];
            goto branch_2;
        }
    }
branch_1:
    value_3 = (uint32_t)(uint8_t)*left;
    value_1 = (uint32_t)(uint8_t)*right;
    if (value_3 == value_1) {
        if (count == 1) {
            return 0;
        }
        if (value_3 == 0) {
            return 0;
        }
        pointer_2 = (uint8_t *)(right + 2);
        while (true) {
            left = left + 1;
            value_3 = (uint32_t)(uint8_t)*left;
            value_1 = (uint32_t)pointer_2[-1];
            if (value_3 != value_1)
                break;
            if (pointer_2 == (uint8_t *)(right + count)) {
                return 0;
            }
            pointer_2 = pointer_2 + 1;
            if (value_3 == 0) {
                return 0;
            }
        }
    }
branch_2:
    return value_3 - value_1;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001bfbc. */
char *libc_strncpy(char *destination, char *source, uint32_t count)
{
    char byte_value_1;
    uint32_t *pointer_2;
    uint32_t *pointer_4;
    uint32_t *pointer_3;
    uint32_t value_5;

    if (((((uint32_t)destination | (uint32_t)source) & 3) == 0) && (3 < count)) {
        pointer_4 = (uint32_t *)destination;
        pointer_3 = (uint32_t *)source;
        do {
            value_5 = *pointer_3;
            source = (char *)pointer_3;
            pointer_2 = pointer_4;
            if ((value_5 + 0xfefefeff & ~value_5 & 0x80808080) != 0)
                break;
            count = count - 4;
            *pointer_4 = value_5;
            source = (char *)(pointer_3 + 1);
            pointer_2 = pointer_4 + 1;
            pointer_4 = pointer_4 + 1;
            pointer_3 = pointer_3 + 1;
        } while (3 < count);
    } else {
        pointer_2 = (uint32_t *)destination;
    }
    if (count != 0) {
        byte_value_1 = (char)*(uint32_t *)source;
        *(char *)pointer_2 = byte_value_1;
        while (true) {
            pointer_2 = (uint32_t *)((int)pointer_2 + 1);
            count = count - 1;
            if (byte_value_1 == '\0')
                break;
            if (count == 0) {
                return destination;
            }
            source = (char *)((int)source + 1);
            byte_value_1 = *source;
            *(char *)pointer_2 = byte_value_1;
        }
        if (count != 0) {
            pointer_4 = pointer_2;
            do {
                pointer_3 = (uint32_t *)((int)pointer_4 + 1);
                *(char *)pointer_4 = '\0';
                pointer_4 = pointer_3;
            } while (pointer_3 != (uint32_t *)(count + (int)pointer_2));
        }
    }
    return destination;
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x2001d1fc. */
uint32_t libc_strtoul(char *text, char **endPointer, int base)
{
    uint8_t byte_value_1;
    uint8_t byte_value_2;
    uint32_t value_3;
    uint8_t *pointer_4;
    uint32_t value_5;
    uint8_t *pointer_6;
    int value_7;
    int value_8;
    uint32_t value_9;
    uint32_t value_10;

    pointer_6 = (uint8_t *)text;
    while (true) {
        pointer_4 = pointer_6 + 1;
        value_5 = (uint32_t)*pointer_6;
        byte_value_1 = *(uint8_t *)(g_libc_unknown_11 + value_5 + 1);
        byte_value_2 = byte_value_1 & 8;
        if ((byte_value_1 & 8) == 0)
            break;
        pointer_6 = pointer_4;
    }
    if (value_5 == 0x2d) {
        pointer_4 = pointer_6 + 2;
        value_5 = (uint32_t)pointer_6[1];
        byte_value_2 = 1;
    } else if (value_5 == 0x2b) {
        value_5 = (uint32_t)pointer_6[1];
        pointer_4 = pointer_6 + 2;
    }
    if ((base & 0xffffffefU) == 0) {
        if (value_5 == 0x30) {
            if ((*pointer_4 & 0xdf) == 0x58) {
                value_5 = (uint32_t)pointer_4[1];
                base = 0x10;
                value_10 = 0xf;
                pointer_4 = pointer_4 + 2;
                value_9 = 0xfffffff;
                goto branch_1;
            }
            if (base == 0) {
                base = 8;
                value_10 = 7;
                value_9 = 0x1fffffff;
                goto branch_1;
            }
        } else if (base == 0) {
            base = 10;
            value_10 = 5;
            value_9 = 0x19999999;
            goto branch_1;
        }
    }
    value_9 = (uint32_t)(0xffffffff / (uint64_t)(uint32_t)base);
    value_10 = ~(base * value_9);
branch_1:
    value_8 = 0;
    value_3 = 0;
    do {
        byte_value_1 = *(uint8_t *)(g_libc_unknown_11 + value_5 + 1);
        if ((byte_value_1 & 4) == 0) {
            if ((byte_value_1 & 3) == 0) {
            branch_2:
                if (value_8 < 0) {
                    *libc_reentrancy_as_uint32_t_pointer = 0x22;
                    value_3 = 0xffffffff;
                } else if (byte_value_2 != 0) {
                    value_3 = -value_3;
                }
                if (endPointer != (char **)0x0) {
                    if (value_8 != 0) {
                        text = (char *)(pointer_4 + -1);
                    }
                    *endPointer = text;
                }
                return value_3;
            }
            if ((byte_value_1 & 3) == 1) {
                value_7 = 0x37;
            } else {
                value_7 = 0x57;
            }
            value_7 = value_5 - value_7;
        } else {
            value_7 = value_5 - 0x30;
        }
        if (base <= value_7)
            goto branch_2;
        if (((value_8 < 0) || (value_9 <= value_3 && value_3 != value_9)) ||
            ((value_3 == value_9 && ((int)value_10 < value_7)))) {
            value_8 = -1;
        } else {
            value_3 = base * value_3 + value_7;
            value_8 = 1;
        }
        value_5 = (uint32_t)*pointer_4;
        pointer_4 = pointer_4 + 1;
    } while (true);
}

/* Observed standard C operation, arguments, wrapper and callsites; original source symbol inferred
 * semantically. Firmware address: 0x200200c4. */
void *libc_memchr(void *buffer, int value, uint32_t count)
{
    uint8_t byte_value_1;
    uint32_t value_2;
    uint32_t value_3;
    int value_4;
    uint32_t *pointer_5;
    uint32_t value_6;
    bool condition_7;

    value_2 = value & 0xff;
    if (((uint32_t)buffer & 3) == 0) {
    branch_1:
        if (3 < count) {
            value_6 = value_2 | value_2 << 8;
            pointer_5 = buffer;
            do {
                value_3 = *pointer_5 ^ (value_6 | value_6 << 0x10);
                buffer = pointer_5;
                if ((value_3 + 0xfefefeff & ~value_3 & 0x80808080) != 0)
                    break;
                count = count - 4;
                buffer = pointer_5 + 1;
                pointer_5 = pointer_5 + 1;
            } while (3 < count);
        }
        if (count == 0) {
            buffer = (uint32_t *)0x0;
        } else if ((uint8_t) * (uint32_t *)buffer != value_2) {
            value_4 = 0;
            do {
                buffer = (void *)((int)buffer + 1);
                condition_7 = value_4 == count - 1;
                value_4 = value_4 + 1;
                if (condition_7)
                    goto branch_2;
            } while (*(uint8_t *)buffer != value_2);
        }
    } else if (count == 0) {
    branch_2:
        buffer = (uint32_t *)0x0;
    } else {
        byte_value_1 = *(uint8_t *)buffer;
        while (byte_value_1 != value_2) {
            count = count - 1;
            buffer = (void *)((int)buffer + 1);
            if (((uint32_t)buffer & 3) == 0)
                goto branch_1;
            if (count == 0)
                goto branch_2;
            byte_value_1 = *(uint8_t *)buffer;
        }
    }
    return buffer;
}
