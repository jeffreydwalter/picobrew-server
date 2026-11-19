/* Pico Pro 0.1.34 — c library. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Purpose incomplete.
 * Firmware address: 0x2001ae90. */
uint32_t unresolved_c_library_01(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3)
{
    uint32_t *libc_malloc_result;
    uint32_t value_2;
    uint32_t value_3;

    if (g_c_library_unknown_13 == (uint32_t *)0x0) {
        g_c_library_unknown_13 = (uint32_t *)&g_c_library_unknown_14;
    }
    value_3 = g_c_library_unknown_13[1];
    libc_malloc_result = g_c_library_unknown_13;
    if (0x1f < (int)value_3) {
        libc_malloc_result = PICO_UNRESOLVED_RETURN(libc_malloc(400));
        if (libc_malloc_result == (uint32_t *)0x0) {
            return 0xffffffff;
        }
        *libc_malloc_result = g_c_library_unknown_13;
        libc_malloc_result[1] = 0;
        value_3 = 0;
        g_c_library_unknown_13 = libc_malloc_result;
        libc_malloc_result[0x62] = 0;
        libc_malloc_result[99] = 0;
    }
    libc_malloc_result[value_3 + 0x22] = argument_1;
    value_2 = 1 << (value_3 & 0xff);
    libc_malloc_result[0x62] = libc_malloc_result[0x62] | value_2;
    libc_malloc_result[value_3 + 0x42] = argument_3;
    libc_malloc_result[99] = value_2 | libc_malloc_result[99];
    libc_malloc_result[1] = value_3 + 1;
    libc_malloc_result[value_3 + 2] = argument_2;
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x2001aec0. */
uint32_t unresolved_c_library_02(void)
{
    return libc_reentrancy_as_uint32_t;
}

/* Purpose incomplete.
 * Firmware address: 0x2001aeec. */
int *unresolved_c_library_03(int *argument_1, uint32_t argument_2)
{
    void *pointer_1;
    int value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t *pointer_5;
    int value_6;
    uint32_t value_7;
    uint32_t value_8;
    uint32_t *index_2;
    uint8_t *pointer_10;
    uint8_t *pointer_11;
    uint8_t *index;
    uint32_t value_13;
    uint32_t *pointer_14;
    uint8_t *pointer_15;
    uint32_t value_16;

    if (argument_2 + 0xb < 0x17) {
        value_13 = 0x10;
        if (0x10 < argument_2)
            goto branch_1;
    } else {
        value_13 = argument_2 + 0xb & 0xfffffff8;
        if (((int)value_13 < 0) || (value_13 < argument_2)) {
        branch_1:
            *argument_1 = 0xc;
            return (int *)0x0;
        }
    }
    unresolved_c_library_04();
    pointer_5 = g_c_library_unknown_19;
    if (value_13 < 0x1f8) {
        value_16 = value_13 >> 3;
        index = (uint8_t *)(&g_c_library_unknown_18)[value_16 * 2];
        if ((index != &g_c_library_unknown_15 + value_16 * 8) ||
            (pointer_11 = index + 8, index = *(uint8_t **)(index + 0x14), pointer_11 != index)) {
            value_13 = *(uint32_t *)(index + 4);
            value_6 = *(int *)(index + 0xc);
            value_2 = *(int *)(index + 8);
            value_16 = *(uint32_t *)(index + (value_13 & 0xfffffffc) + 4);
            *(int *)(value_2 + 0xc) = value_6;
            *(int *)(value_6 + 8) = value_2;
            *(uint32_t *)(index + (value_13 & 0xfffffffc) + 4) = value_16 | 1;
            unresolved_c_library_05();
            return (int *)(index + 8);
        }
        value_16 = value_16 + 2;
    } else {
        value_16 = value_13 >> 9;
        if (value_16 == 0) {
            value_2 = 0x7e;
            value_6 = 0x3f;
        } else if (value_16 < 5) {
            value_6 = (value_13 >> 6) + 0x38;
            value_2 = value_6 * 2;
        } else if (value_16 < 0x15) {
            value_6 = value_16 + 0x5b;
            value_2 = value_6 * 2;
        } else if (value_16 < 0x55) {
            value_6 = (value_13 >> 0xc) + 0x6e;
            value_2 = value_6 * 2;
        } else if (value_16 < 0x155) {
            value_6 = (value_13 >> 0xf) + 0x77;
            value_2 = value_6 * 2;
        } else if (value_16 < 0x555) {
            value_6 = (value_13 >> 0x12) + 0x7c;
            value_2 = value_6 * 2;
        } else {
            value_2 = 0xfc;
            value_6 = 0x7e;
        }
        for (index = (uint8_t *)(&g_c_library_unknown_18)[value_2];
             &g_c_library_unknown_15 + value_2 * 4 != index; index = *(uint8_t **)(index + 0xc)) {
            value_16 = *(uint32_t *)(index + 4) & 0xfffffffc;
            if (0xf < (int)(value_16 - value_13)) {
                value_6 = value_6 + -1;
                break;
            }
            if (-1 < (int)(value_16 - value_13)) {
                value_6 = *(int *)(index + 0xc);
                value_13 = *(uint32_t *)(index + value_16 + 4);
                value_2 = *(int *)(index + 8);
                *(int *)(value_2 + 0xc) = value_6;
                *(int *)(value_6 + 8) = value_2;
                *(uint32_t *)(index + value_16 + 4) = value_13 | 1;
                unresolved_c_library_05();
                return (int *)(index + 8);
            }
        }
        value_16 = value_6 + 1;
    }
    if ((uint8_t **)g_c_library_unknown_19 == &g_c_library_unknown_17) {
        value_4 = g_c_library_unknown_16;
    } else {
        value_3 = g_c_library_unknown_19[1];
        value_4 = value_3 & 0xfffffffc;
        value_7 = value_4 - value_13;
        if (0xf < (int)value_7) {
            pointer_14 = (uint32_t *)((int)g_c_library_unknown_19 + value_13);
            g_c_library_unknown_19[1] = value_13 | 1;
            g_c_library_unknown_19 = pointer_14;
            g_c_library_unknown_20 = pointer_14;
            pointer_14[3] = &g_c_library_unknown_17;
            pointer_14[2] = &g_c_library_unknown_17;
            pointer_14[1] = value_7 | 1;
            *(uint32_t *)((int)pointer_14 + value_7) = value_7;
            unresolved_c_library_05();
            return pointer_5 + 2;
        }
        g_c_library_unknown_20 = &g_c_library_unknown_17;
        g_c_library_unknown_19 = &g_c_library_unknown_17;
        if (-1 < (int)value_7) {
            *(uint32_t *)((int)pointer_5 + value_4 + 4) =
                *(uint32_t *)((int)pointer_5 + value_4 + 4) | 1;
            unresolved_c_library_05();
            return pointer_5 + 2;
        }
        if (value_4 < 0x200) {
            value_3 = value_3 >> 3;
            index = (&g_c_library_unknown_17)[value_3 * 2];
            pointer_5[2] = index;
            value_4 = 1 << ((int)value_3 >> 2 & 0xffU) | g_c_library_unknown_16;
            pointer_5[3] = &g_c_library_unknown_15 + value_3 * 8;
            g_c_library_unknown_16 = value_4;
            (&g_c_library_unknown_17)[value_3 * 2] = (uint8_t *)pointer_5;
            *(uint32_t **)(index + 0xc) = pointer_5;
        } else {
            value_7 = value_3 >> 9;
            if (value_7 < 5) {
                value_6 = (value_3 >> 6) + 0x38;
                value_2 = value_6 * 2;
            } else if (value_7 < 0x15) {
                value_6 = value_7 + 0x5b;
                value_2 = value_6 * 2;
            } else if (value_7 < 0x55) {
                value_6 = (value_3 >> 0xc) + 0x6e;
                value_2 = value_6 * 2;
            } else if (value_7 < 0x155) {
                value_6 = (value_3 >> 0xf) + 0x77;
                value_2 = value_6 * 2;
            } else if (value_7 < 0x555) {
                value_6 = (value_3 >> 0x12) + 0x7c;
                value_2 = value_6 * 2;
            } else {
                value_2 = 0xfc;
                value_6 = 0x7e;
            }
            index = (&g_c_library_unknown_17)[value_2];
            if (index == &g_c_library_unknown_15 + value_2 * 4) {
                g_c_library_unknown_16 = g_c_library_unknown_16 | 1 << (value_6 >> 2 & 0xffU);
                pointer_11 = index;
            } else {
                do {
                    pointer_11 = index;
                    if ((*(uint32_t *)(index + 4) & 0xfffffffc) <= value_4)
                        break;
                    index = *(uint8_t **)(index + 8);
                    pointer_11 = index;
                } while (&g_c_library_unknown_15 + value_2 * 4 != index);
                index = *(uint8_t **)(pointer_11 + 0xc);
            }
            value_4 = g_c_library_unknown_16;
            pointer_5[3] = index;
            pointer_5[2] = pointer_11;
            *(uint32_t **)(index + 8) = pointer_5;
            *(uint32_t **)(pointer_11 + 0xc) = pointer_5;
        }
    }
    index = g_c_library_unknown_17;
    value_3 = 1 << ((int)value_16 >> 2 & 0xffU);
    if (value_3 <= value_4) {
        if ((value_4 & value_3) == 0) {
            value_16 = value_16 & 0xfffffffc;
            do {
                value_3 = value_3 << 1;
                value_16 = value_16 + 4;
            } while ((value_4 & value_3) == 0);
        }
        do {
            pointer_14 = (uint32_t *)(&g_c_library_unknown_15 + value_16 * 8);
            pointer_5 = pointer_14;
            value_4 = value_16;
            do {
                for (index_2 = (uint32_t *)pointer_5[3]; pointer_5 != index_2;
                     index_2 = (uint32_t *)index_2[3]) {
                    value_7 = index_2[1] & 0xfffffffc;
                    value_8 = value_7 - value_13;
                    if (0xf < (int)value_8) {
                        value_6 = index_2[3];
                        value_2 = index_2[2];
                        pointer_5 = (uint32_t *)((int)index_2 + value_13);
                        index_2[1] = value_13 | 1;
                        *(int *)(value_2 + 0xc) = value_6;
                        *(int *)(value_6 + 8) = value_2;
                        g_c_library_unknown_19 = pointer_5;
                        g_c_library_unknown_20 = pointer_5;
                        pointer_5[3] = &g_c_library_unknown_17;
                        pointer_5[2] = &g_c_library_unknown_17;
                        pointer_5[1] = value_8 | 1;
                        *(uint32_t *)((int)pointer_5 + value_8) = value_8;
                        unresolved_c_library_05();
                        return index_2 + 2;
                    }
                    if (-1 < (int)value_8) {
                        value_6 = index_2[3];
                        value_2 = index_2[2];
                        *(uint32_t *)((int)index_2 + value_7 + 4) =
                            *(uint32_t *)((int)index_2 + value_7 + 4) | 1;
                        *(int *)(value_2 + 0xc) = value_6;
                        *(int *)(value_6 + 8) = value_2;
                        unresolved_c_library_05();
                        return index_2 + 2;
                    }
                }
                value_4 = value_4 + 1;
                pointer_5 = pointer_5 + 2;
            } while ((value_4 & 3) != 0);
            do {
                value_7 = value_16 & 3;
                pointer_5 = pointer_14 + -2;
                value_16 = value_16 - 1;
                if (value_7 == 0) {
                    g_c_library_unknown_16 = g_c_library_unknown_16 & ~value_3;
                    break;
                }
                pointer_14 = (uint32_t *)*pointer_14;
            } while (pointer_14 == pointer_5);
            value_3 = value_3 * 2;
            if ((g_c_library_unknown_16 < value_3) || (value_16 = value_4, value_3 == 0))
                break;
            for (; (g_c_library_unknown_16 & value_3) == 0; value_3 = value_3 << 1) {
                value_16 = value_16 + 4;
            }
        } while (true);
    }
    value_16 = *(uint32_t *)(g_c_library_unknown_17 + 4) & 0xfffffffc;
    if (value_13 <= value_16) {
        value_4 = value_16 - value_13;
        pointer_11 = g_c_library_unknown_17;
        if (0xf < (int)value_4)
            goto branch_2;
    }
    pointer_15 = g_c_library_unknown_17 + value_16;
    if (g_c_library_unknown_22 == (uint8_t *)0xffffffff) {
        value_4 = value_13 + g_c_library_unknown_26 + 0x10;
    } else {
        value_4 =
            (uint32_t)(&g_c_library_unknown_02 + value_13 + g_c_library_unknown_26) & 0xfffff000;
    }
    pointer_11 = PICO_UNRESOLVED_RETURN(unresolved_c_library_07(argument_1, value_4));
    if ((pointer_11 == (uint8_t *)0xffffffff) ||
        ((pointer_11 < pointer_15 && (index != &g_c_library_unknown_15)))) {
        value_16 = *(uint32_t *)(g_c_library_unknown_17 + 4) & 0xfffffffc;
        pointer_11 = g_c_library_unknown_17;
    } else {
        pointer_10 = g_c_library_unknown_27 + value_4;
        g_c_library_unknown_27 = pointer_10;
        if ((pointer_15 == pointer_11) && (((uint32_t)pointer_15 & 0xfff) == 0)) {
            *(uint32_t *)(g_c_library_unknown_17 + 4) = value_4 + value_16 | 1;
        } else {
            if (g_c_library_unknown_22 == (uint8_t *)0xffffffff) {
                g_c_library_unknown_22 = pointer_11;
            } else {
                g_c_library_unknown_27 = pointer_10 + ((int)pointer_11 - (int)pointer_15);
            }
            value_3 = (uint32_t)pointer_11 & 7;
            if (value_3 == 0) {
                value_2 = 0x1000;
            } else {
                pointer_11 = pointer_11 + (8 - value_3);
                value_2 = (int)&g_c_library_unknown_01 - value_3;
            }
            value_2 = value_2 - ((uint32_t)(pointer_11 + value_4) & 0xfff);
            pointer_1 = PICO_UNRESOLVED_RETURN(unresolved_c_library_07(argument_1, value_2));
            if (pointer_1 == (void *)0xffffffff) {
                value_4 = 1;
                value_2 = 0;
            } else {
                value_4 = (int)pointer_1 + (value_2 - (int)pointer_11) | 1;
            }
            pointer_10 = g_c_library_unknown_27 + value_2;
            g_c_library_unknown_17 = pointer_11;
            *(uint32_t *)(pointer_11 + 4) = value_4;
            g_c_library_unknown_27 = pointer_10;
            if (index != &g_c_library_unknown_15) {
                if (value_16 < 0x10) {
                    *(uint32_t *)(pointer_11 + 4) = 1;
                    value_16 = 0;
                    goto branch_3;
                }
                value_16 = value_16 - 0xc & 0xfffffff8;
                *(uint32_t *)(index + 4) = value_16 | *(uint32_t *)(index + 4) & 1;
                *(uint32_t *)(index + value_16 + 4) = 5;
                *(uint32_t *)(index + value_16 + 8) = 5;
                if (0xf < value_16) {
                    unresolved_c_library_12(argument_1, (int)(index + 8));
                    pointer_10 = g_c_library_unknown_27;
                }
            }
        }
        if (g_c_library_unknown_25 < pointer_10) {
            g_c_library_unknown_25 = pointer_10;
        }
        if (g_c_library_unknown_24 < pointer_10) {
            g_c_library_unknown_24 = pointer_10;
        }
        value_16 = *(uint32_t *)(g_c_library_unknown_17 + 4) & 0xfffffffc;
        pointer_11 = g_c_library_unknown_17;
    }
branch_3:
    value_4 = value_16 - value_13;
    if ((value_16 < value_13) || ((int)value_4 < 0x10)) {
        unresolved_c_library_05();
        return (int *)0x0;
    }
branch_2:
    g_c_library_unknown_17 = pointer_11 + value_13;
    *(uint32_t *)(pointer_11 + 4) = value_13 | 1;
    *(uint32_t *)(g_c_library_unknown_17 + 4) = value_4 | 1;
    unresolved_c_library_05();
    return (int *)(pointer_11 + 8);
}

/* Purpose incomplete.
 * Firmware address: 0x2001b714. */
void unresolved_c_library_04(void)
{
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001b718. */
void unresolved_c_library_05(void)
{
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001b72c. */
int *unresolved_c_library_06(int *argument_1, int *argument_2, uint32_t argument_3)
{
    int value_1;
    int *pointer_2;
    uint32_t value_3;
    int *pointer_4;
    uint32_t value_5;
    uint32_t value_6;
    int value_7;
    uint32_t value_8;
    uint32_t value_9;
    int *pointer_10;

    if (argument_2 == (int *)0x0) {
        pointer_2 = unresolved_c_library_03(argument_1, argument_3);
        return pointer_2;
    }
    unresolved_c_library_04();
    value_5 = argument_2[-1];
    value_8 = value_5 & 0xfffffffc;
    pointer_2 = argument_2 + -2;
    if (argument_3 + 0xb < 0x17) {
        value_9 = 0x10;
    } else {
        value_9 = argument_3 + 0xb & 0xfffffff8;
        if ((int)value_9 < 0)
            goto branch_1;
    }
    if (value_9 < argument_3) {
    branch_1:
        *argument_1 = 0xc;
        return (int *)0x0;
    }
    value_6 = value_8;
    if ((int)value_9 <= (int)value_8) {
    branch_2:
        value_8 = value_6 - value_9;
        if (value_8 < 0x10) {
            pointer_2[1] = value_5 & 1 | value_6;
            *(uint32_t *)((int)pointer_2 + value_6 + 4) =
                *(uint32_t *)((int)pointer_2 + value_6 + 4) | 1;
        } else {
            pointer_2[1] = value_9 | value_5 & 1;
            *(uint32_t *)((int)pointer_2 + value_9 + 4) = value_8 | 1;
            *(uint32_t *)((int)pointer_2 + value_8 + value_9 + 4) =
                *(uint32_t *)((int)pointer_2 + value_8 + value_9 + 4) | 1;
            unresolved_c_library_12(argument_1, (int)pointer_2 + value_9 + 8);
        }
        unresolved_c_library_05();
        return argument_2;
    }
    value_1 = (int)pointer_2 + value_8;
    if (g_c_library_unknown_17_as_int == value_1) {
        value_3 = *(uint32_t *)(g_c_library_unknown_17_as_int + 4) & 0xfffffffc;
        if ((int)(value_9 + 0x10) <= (int)(value_3 + value_8)) {
            g_c_library_unknown_17_as_int = (int)pointer_2 + value_9;
            *(uint32_t *)(g_c_library_unknown_17_as_int + 4) = (value_3 + value_8) - value_9 | 1;
            argument_2[-1] = value_9 | argument_2[-1] & 1U;
            unresolved_c_library_05();
            return argument_2;
        }
        value_1 = g_c_library_unknown_17_as_int;
    } else if ((*(uint32_t *)((*(uint32_t *)(value_1 + 4) & 0xfffffffe) + value_1 + 4) & 1) == 0) {
        value_3 = *(uint32_t *)(value_1 + 4) & 0xfffffffc;
        value_6 = value_3 + value_8;
        if ((int)value_9 <= (int)value_6) {
            value_7 = *(int *)(value_1 + 0xc);
            value_1 = *(int *)(value_1 + 8);
            *(int *)(value_1 + 0xc) = value_7;
            *(int *)(value_7 + 8) = value_1;
            goto branch_2;
        }
    } else {
        value_3 = 0;
        value_1 = 0;
    }
    if (-1 < (int)(value_5 << 0x1f)) {
        pointer_10 = (int *)((int)pointer_2 - argument_2[-2]);
        value_6 = (pointer_10[1] & 0xfffffffcU) + value_8;
        if (value_1 != 0) {
            if (value_1 == g_c_library_unknown_17_as_int) {
                if ((int)(value_9 + 0x10) <= (int)(value_3 + value_6)) {
                    value_7 = pointer_10[3];
                    pointer_2 = pointer_10 + 2;
                    value_1 = *pointer_2;
                    value_8 = value_8 - 4;
                    *(int *)(value_1 + 0xc) = value_7;
                    *(int *)(value_7 + 8) = value_1;
                    if (value_8 < 0x25) {
                        if (value_8 < 0x14) {
                            pointer_4 = pointer_2;
                        } else {
                            pointer_10[2] = *argument_2;
                            pointer_10[3] = argument_2[1];
                            if (value_8 < 0x1c) {
                                argument_2 = argument_2 + 2;
                                pointer_4 = pointer_10 + 4;
                            } else {
                                pointer_10[4] = argument_2[2];
                                pointer_10[5] = argument_2[3];
                                if (value_8 == 0x24) {
                                    pointer_10[6] = argument_2[4];
                                    pointer_10[7] = argument_2[5];
                                    argument_2 = argument_2 + 6;
                                    pointer_4 = pointer_10 + 8;
                                } else {
                                    argument_2 = argument_2 + 4;
                                    pointer_4 = pointer_10 + 6;
                                }
                            }
                        }
                        *pointer_4 = *argument_2;
                        pointer_4[1] = argument_2[1];
                        pointer_4[2] = argument_2[2];
                    } else {
                        libc_memmove(pointer_2, argument_2, value_8);
                    }
                    g_c_library_unknown_17_as_int = (int)pointer_10 + value_9;
                    *(uint32_t *)(g_c_library_unknown_17_as_int + 4) =
                        (value_3 + value_6) - value_9 | 1;
                    pointer_10[1] = value_9 | pointer_10[1] & 1U;
                    unresolved_c_library_05();
                    return pointer_2;
                }
            } else {
                value_3 = value_3 + value_6;
                if ((int)value_9 <= (int)value_3) {
                    value_7 = *(int *)(value_1 + 0xc);
                    value_1 = *(int *)(value_1 + 8);
                    *(int *)(value_1 + 0xc) = value_7;
                    *(int *)(value_7 + 8) = value_1;
                    pointer_2 = pointer_10 + 2;
                    value_1 = *pointer_2;
                    value_7 = pointer_10[3];
                    *(int *)(value_1 + 0xc) = value_7;
                    value_8 = value_8 - 4;
                    *(int *)(value_7 + 8) = value_1;
                    value_6 = value_3;
                    if (value_8 < 0x25) {
                        if (value_8 < 0x14) {
                            pointer_4 = pointer_2;
                        } else {
                            pointer_10[2] = *argument_2;
                            pointer_10[3] = argument_2[1];
                            if (value_8 < 0x1c) {
                                argument_2 = argument_2 + 2;
                                pointer_4 = pointer_10 + 4;
                            } else {
                                pointer_10[4] = argument_2[2];
                                pointer_10[5] = argument_2[3];
                                if (value_8 == 0x24) {
                                    pointer_10[6] = argument_2[4];
                                    pointer_10[7] = argument_2[5];
                                    argument_2 = argument_2 + 6;
                                    pointer_4 = pointer_10 + 8;
                                } else {
                                    argument_2 = argument_2 + 4;
                                    pointer_4 = pointer_10 + 6;
                                }
                            }
                        }
                        *pointer_4 = *argument_2;
                        pointer_4[1] = argument_2[1];
                        pointer_4[2] = argument_2[2];
                        value_5 = pointer_10[1];
                        argument_2 = pointer_2;
                        pointer_2 = pointer_10;
                    } else {
                        libc_memmove(pointer_2, argument_2, value_8);
                        value_5 = pointer_10[1];
                        argument_2 = pointer_2;
                        pointer_2 = pointer_10;
                    }
                    goto branch_2;
                }
            }
        }
        if ((int)value_9 <= (int)value_6) {
            value_7 = pointer_10[3];
            pointer_2 = pointer_10 + 2;
            value_1 = *pointer_2;
            value_8 = value_8 - 4;
            *(int *)(value_1 + 0xc) = value_7;
            *(int *)(value_7 + 8) = value_1;
            if (value_8 < 0x25) {
                if (value_8 < 0x14) {
                    pointer_4 = pointer_2;
                } else {
                    pointer_10[2] = *argument_2;
                    pointer_10[3] = argument_2[1];
                    if (value_8 < 0x1c) {
                        argument_2 = argument_2 + 2;
                        pointer_4 = pointer_10 + 4;
                    } else {
                        pointer_10[4] = argument_2[2];
                        pointer_10[5] = argument_2[3];
                        if (value_8 == 0x24) {
                            pointer_10[6] = argument_2[4];
                            pointer_10[7] = argument_2[5];
                            argument_2 = argument_2 + 6;
                            pointer_4 = pointer_10 + 8;
                        } else {
                            argument_2 = argument_2 + 4;
                            pointer_4 = pointer_10 + 6;
                        }
                    }
                }
                *pointer_4 = *argument_2;
                pointer_4[1] = argument_2[1];
                pointer_4[2] = argument_2[2];
                value_5 = pointer_10[1];
                argument_2 = pointer_2;
                pointer_2 = pointer_10;
            } else {
                libc_memmove(pointer_2, argument_2, value_8);
                value_5 = pointer_10[1];
                argument_2 = pointer_2;
                pointer_2 = pointer_10;
            }
            goto branch_2;
        }
    }
    pointer_10 = unresolved_c_library_03(argument_1, argument_3);
    if (pointer_10 != (int *)0x0) {
        value_5 = argument_2[-1];
        if (pointer_10 + -2 == (int *)((value_5 & 0xfffffffe) + (int)pointer_2)) {
            value_6 = value_8 + (pointer_10[-1] & 0xfffffffcU);
            goto branch_2;
        }
        value_8 = value_8 - 4;
        if (value_8 < 0x25) {
            if (value_8 < 0x14) {
                pointer_2 = argument_2;
                pointer_4 = pointer_10;
            } else {
                *pointer_10 = *argument_2;
                pointer_10[1] = argument_2[1];
                if (value_8 < 0x1c) {
                    pointer_4 = pointer_10 + 2;
                    pointer_2 = argument_2 + 2;
                } else {
                    pointer_10[2] = argument_2[2];
                    pointer_10[3] = argument_2[3];
                    if (value_8 == 0x24) {
                        pointer_10[4] = argument_2[4];
                        pointer_10[5] = argument_2[5];
                        pointer_2 = argument_2 + 6;
                        pointer_4 = pointer_10 + 6;
                    } else {
                        pointer_4 = pointer_10 + 4;
                        pointer_2 = argument_2 + 4;
                    }
                }
            }
            *pointer_4 = *pointer_2;
            pointer_4[1] = pointer_2[1];
            pointer_4[2] = pointer_2[2];
        } else {
            libc_memmove(pointer_10, argument_2, value_8);
        }
        unresolved_c_library_12(argument_1, (int)argument_2);
    }
    unresolved_c_library_05();
    return pointer_10;
}

/* Calls _sbrk at2001bb04. Both return paths2001bb0c and2001bb16 preserve r0. Only side effect on
 * failure is copying global errno into *reentrancy. Returns the prior break or(void*)-1. Firmware
 * address: 0x2001baf8. */
void *unresolved_c_library_07(int *reentrancy, int increment)
{
    void *_sbrk_result;

    g_c_library_unknown_28 = 0;
    _sbrk_result = PICO_UNRESOLVED_RETURN(_sbrk(increment));
    if ((_sbrk_result == (void *)0xffffffff) && (g_c_library_unknown_28 != 0)) {
        *reentrancy = g_c_library_unknown_28;
        return (void *)0xffffffff;
    }
    return _sbrk_result;
}

/* Purpose incomplete.
 * Firmware address: 0x2001c020. */
double unresolved_c_library_07_2(uint32_t argument_1, uint32_t argument_2, int argument_3)
{
    int value_1;
    double __aeabi_dmul_result;

    __aeabi_dmul_result = (double)unresolved_floating_point_01(argument_1, argument_2);
    if ((argument_3 == 0) || (value_1 = 0x6b - ((argument_2 & 0x7fffffff) >> 0x14), value_1 < 1)) {
        return __aeabi_dmul_result;
    }
    __aeabi_dmul_result = __aeabi_dmul(
        __aeabi_dmul_result, (double)((uint64_t)(value_1 * 0x100000 + 0x3ff00000) << 0x20));
    return __aeabi_dmul_result;
}

/* Purpose incomplete.
 * Firmware address: 0x2001d214. */
int unresolved_c_library_08(int *argument_1, int *argument_2, uint8_t *argument_3,
                            uint32_t *argument_4)
{
    uint32_t *pointer_1;
    char *text;
    uint32_t text_length;
    void *libc_memchr_result;
    int __aeabi_dcmpeq_result;
    int *pointer_5;
    uint8_t *pointer_6;
    uint8_t *pointer_7;
    int *pointer_8;
    char unresolved_return_r2;
    uint8_t *pointer_9;
    uint32_t *pointer_10;
    char *text_00;
    uint16_t *pointer_11;
    uint32_t value_12;
    int value_13;
    uint32_t text_length_2;
    char *text_15;
    int libm_fpclassify_result;
    uint32_t value_17;
    char *text_18;
    uint32_t value_19;
    char byte_value_20;
    bool condition_21;
    uint64_t value_22;
    uint32_t value_124;
    uint32_t value_120;
    uint32_t value_3;
    int value_110;
    uint32_t value;
    uint32_t *pointer_108;
    uint32_t value_104;
    uint32_t value_8;
    uint32_t value_0;
    uint32_t value_2;
    int value_4;
    char *text_0;
    uint32_t value_4_2;
    char byte_value_9;
    uint8_t byte_value_8;
    uint8_t byte_value_7;
    uint32_t value_4_3;
    uint32_t value_0_2;
    uint8_t byte_value;
    uint8_t byte_value_2;
    char buffer[6];
    char *text_4;
    uint8_t buffer_0[4];
    int *pointer;
    int value_98;
    int value_94;
    uint8_t buffer_90[38];
    char byte_value_3;
    uint8_t byte_value_69;
    int buffer_68[16];
    uint8_t buffer_28[4];

    pointer_1 = (uint32_t *)unresolved_c_library_16();
    text = (char *)*pointer_1;
    text_length = libc_strlen(text);
    if (((int)((uint32_t) * (uint16_t *)(argument_2 + 3) << 0x18) < 0) && (argument_2[4] == 0)) {
        pointer_5 = unresolved_c_library_03(argument_1, 0x40);
        *argument_2 = (int)pointer_5;
        argument_2[4] = (int)pointer_5;
        if (pointer_5 == (int *)0x0) {
            *argument_1 = 0xc;
            return -1;
        }
        argument_2[5] = 0x40;
    }
    value_0 = 0;
    value_2 = 0;
    value_104 = 0;
    value_94 = 0;
    value_98 = 0;
    text_0 = (char *)0x0;
    value_4 = 0;
    value_110 = 0;
    pointer_5 = buffer_68;
    pointer_108 = argument_4;
    pointer = buffer_68;
branch_1:
    do {
        pointer_9 = argument_3;
        if ((*argument_3 != 0) && (*argument_3 != 0x25)) {
            do {
                pointer_9 = pointer_9 + 1;
                if (*pointer_9 == 0)
                    break;
            } while (*pointer_9 != 0x25);
            libm_fpclassify_result = (int)pointer_9 - (int)argument_3;
            if (libm_fpclassify_result != 0) {
                *pointer_5 = (int)argument_3;
                value_98 = value_98 + 1;
                value_94 = value_94 + libm_fpclassify_result;
                pointer_5[1] = libm_fpclassify_result;
                if (value_98 < 8) {
                    pointer_5 = pointer_5 + 2;
                } else {
                    value_13 =
                        unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                    if (value_13 != 0)
                        goto branch_2;
                    pointer_5 = buffer_68;
                }
                value_110 = value_110 + libm_fpclassify_result;
            }
        }
        if (*pointer_9 == 0) {
        branch_3:
            if (value_94 != 0) {
                unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
            }
        branch_2:
            if ((*(uint16_t *)(argument_2 + 3) & 0x40) != 0) {
                value_110 = -1;
            }
            return value_110;
        }
        value_19 = (uint32_t)pointer_9[1];
        byte_value_9 = '\0';
        value = 0;
        value_124 = 0;
        pointer_9 = pointer_9 + 1;
    branch_4:
        value_120 = 0xffffffff;
        argument_3 = pointer_9;
    branch_5:
        argument_3 = argument_3 + 1;
    branch_6:
        text_length_2 = value_120;
        switch (value_19) {
        case 0x20:
            goto switchD_2001d2da_caseD_20;
        default:
            if (value_19 == 0)
                goto branch_3;
            byte_value_20 = '\0';
            value_3 = 1;
            byte_value_9 = '\0';
            value_120 = 0;
            value_8 = 0;
            text_length_2 = 1;
            text_00 = buffer_90;
            buffer_90[0] = (char)value_19;
            goto branch_7;
        case 0x23:
            value_124 = value_124 | 1;
            value_19 = (uint32_t)*argument_3;
            goto branch_5;
        case 0x2a:
            value = *pointer_108;
            pointer_108 = pointer_108 + 1;
            if ((int)value < 0) {
                value = -value;
                goto switchD_2001d2da_caseD_2d;
            }
            value_19 = (uint32_t)*argument_3;
            goto branch_5;
        case 0x2b:
            value_19 = (uint32_t)*argument_3;
            byte_value_9 = '+';
            goto branch_5;
        case 0x2d:
        switchD_2001d2da_caseD_2d:
            value_124 = value_124 | 4;
            value_19 = (uint32_t)*argument_3;
            goto branch_5;
        case 0x2e:
            goto switchD_2001d2da_caseD_2e;
        case 0x30:
            value_124 = value_124 | 0x80;
            value_19 = (uint32_t)*argument_3;
            goto branch_5;
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
            goto switchD_2001d2da_caseD_31;
        case 0x44:
            value_124 = value_124 | 0x10;
            if ((int)(value_124 << 0x1a) < 0)
                goto branch_8;
        branch_9:
            if (((value_124 & 0x10) == 0) && ((int)(value_124 << 0x19) < 0)) {
                value_12 = (uint32_t)(short)(uint16_t)*pointer_108;
                pointer_108 = pointer_108 + 1;
                value_17 = (int)value_12 >> 0x1f;
                goto branch_10;
            }
            value_12 = *pointer_108;
            value_17 = (int)value_12 >> 0x1f;
            pointer_108 = pointer_108 + 1;
            if ((int)value_17 < 0)
                goto branch_11;
        branch_12:
            libm_fpclassify_result = 1;
            break;
        case 0x45:
        case 0x47:
        case 0x65:
        case 0x66:
        case 0x67:
            pointer_108 = (uint32_t *)((int)pointer_108 + 7U & 0xfffffff8);
            if ((int)(value_124 << 0x1c) < 0) {
                value_0 = *pointer_108;
                value_2 = pointer_108[1];
            } else {
                value_0 = *pointer_108;
                value_2 = pointer_108[1];
            }
            pointer_108 = pointer_108 + 2;
            libm_fpclassify_result = libm_fpclassify((double)pico_join_u32_u32(value_2, value_0));
            if (libm_fpclassify_result == 1) {
                libm_fpclassify_result =
                    __aeabi_dcmplt((double)pico_join_u32_u32(value_2, value_0), 0.0);
                if (libm_fpclassify_result != 0) {
                    byte_value_9 = '-';
                }
                value_3 = 3;
                text_00 = &g_c_library_unknown_04;
                value_120 = 0;
                if (value_19 < 0x48) {
                    text_00 = "INF";
                }
                text_length_2 = 3;
                value_8 = 0;
                value_124 = value_124 & 0xffffff7f;
                byte_value_20 = byte_value_9;
                goto branch_13;
            }
            libm_fpclassify_result = libm_fpclassify((double)pico_join_u32_u32(value_2, value_0));
            if (libm_fpclassify_result == 0) {
                text_00 = &g_c_library_unknown_06;
                value_3 = 3;
                if (value_19 < 0x48) {
                    text_00 = &g_c_library_unknown_05;
                }
                text_length_2 = 3;
                value_120 = 0;
                value_8 = 0;
                value_124 = value_124 & 0xffffff7f;
                byte_value_20 = byte_value_9;
                goto branch_13;
            }
            value_12 = value_19 & 0xffffffdf;
            if (value_120 == 0xffffffff) {
                value_120 = 6;
            } else if ((value_12 == 0x47) && (value_120 == 0)) {
                value_120 = 1;
            }
            if ((int)value_2 < 0) {
                value_4_2 = value_2 + 0x80000000;
                libm_fpclassify_result = 0x2d;
            } else {
                value_4_2 = value_2;
                libm_fpclassify_result = 0;
            }
            text_length_2 = value_120;
            if ((value_19 == 0x66) || (value_19 == 0x46)) {
                text_00 = (char *)unresolved_c_library_10(argument_1, &value_4_3, value_0,
                                                          value_4_2, 3, value_120,
                                                          (int *)&value_4_3, &value_0_2, &text_4);
            } else {
                if (value_12 == 0x45) {
                    text_length_2 = value_120 + 1;
                }
                text_00 = (char *)unresolved_c_library_10(argument_1, &value_4_3, value_0,
                                                          value_4_2, 2, text_length_2,
                                                          (int *)&value_4_3, &value_0_2, &text_4);
                if (((value_19 == 0x67) || (value_19 == 0x47)) && ((value_124 & 1) == 0)) {
                    text_18 = text_4;
                    goto branch_14;
                }
            }
            text_15 = (char *)((int)text_00 + text_length_2);
            if (value_12 == 0x46) {
                if (((char)*(int *)text_00 == '0') &&
                    (value_13 = __aeabi_dcmpeq((double)pico_join_u32_u32(value_4_2, value_0), 0.0),
                     value_13 == 0)) {
                    value_4_3 = 1 - text_length_2;
                    text_15 = text_15 + value_4_3;
                } else {
                    text_15 = text_15 + value_4_3;
                }
            }
            value_13 = __aeabi_dcmpeq((double)pico_join_u32_u32(value_4_2, value_0), 0.0);
            if (value_13 == 0) {
                for (; text_18 = text_4, text_4 < text_15; text_4 = text_4 + 1) {
                    *text_4 = '0';
                }
            } else {
                text_18 = text_15;
            }
        branch_14:
            value_104 = (int)text_18 - (int)text_00;
            text_length_2 = value_4_3;
            if (value_12 == 0x47) {
                if (((int)value_4_3 < -3) || ((int)value_120 < (int)value_4_3)) {
                    value_19 = value_19 - 2;
                    goto branch_15;
                }
            branch_16:
                value_8 = value_4_3;
                if ((int)value_4_3 < (int)value_104) {
                    if ((int)value_4_3 < 1) {
                        value_13 = 2 - value_4_3;
                    } else {
                        value_13 = 1;
                    }
                    text_length_2 = value_13 + value_104;
                    value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                    value_19 = 0x67;
                } else if ((value_124 & 1) == 0) {
                    value_3 = value_4_3 & ~((int)value_4_3 >> 0x1f);
                    value_19 = 0x67;
                } else {
                    text_length_2 = value_4_3 + 1;
                    value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                    value_19 = 0x67;
                }
            } else if (value_19 < 0x66) {
            branch_15:
                value_4_3 = value_4_3 - 1;
                if ((int)value_4_3 < 0) {
                    byte_value_2 = 0x2d;
                    text_length_2 = -value_4_3;
                } else {
                    byte_value_2 = 0x2b;
                    text_length_2 = value_4_3;
                }
                byte_value = (uint8_t)value_19;
                if ((int)text_length_2 < 10) {
                    buffer[1] = (char)text_length_2 + '0';
                    buffer[0] = '0';
                    text_18 = buffer + 2;
                } else {
                    pointer_9 = buffer_0 + 2;
                    do {
                        pointer_6 = pointer_9;
                        value_12 = (int)text_length_2 / 10;
                        *pointer_6 = (char)text_length_2 + (char)value_12 * -10 + 0x30;
                        pointer_9 = pointer_6 + -1;
                        text_length_2 = value_12;
                    } while (9 < (int)value_12);
                    text_length_2 = value_12 + 0x30 & 0xff;
                    pointer_6[-1] = (uint8_t)(value_12 + 0x30);
                    if (pointer_6 + -1 < buffer_0 + 3) {
                        pointer_7 = &byte_value_2;
                        pointer_9 = pointer_6;
                        while (true) {
                            pointer_7 = pointer_7 + 1;
                            *pointer_7 = (char)text_length_2;
                            if (pointer_9 == buffer_0 + 3)
                                break;
                            text_length_2 = (uint32_t)*pointer_9;
                            pointer_9 = pointer_9 + 1;
                        }
                        text_18 = (char *)(((int)buffer_28 * 2 - (int)pointer_6) + -0xf6);
                    } else {
                        text_18 = buffer;
                    }
                }
                value_4 = (int)text_18 - (int)&byte_value;
                text_length_2 = value_104 + value_4;
                if (((int)value_104 < 2) && ((value_124 & 1) == 0)) {
                    value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                    value_8 = 0;
                } else {
                    text_length_2 = text_length_2 + 1;
                    value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                    value_8 = 0;
                }
            } else {
                if (value_19 != 0x66)
                    goto branch_16;
                value_8 = value_4_3;
                if ((int)value_4_3 < 1) {
                    if ((value_120 == 0) && (-1 < (int)(value_124 << 0x1f))) {
                        value_3 = 1;
                        text_length_2 = 1;
                    } else {
                        text_length_2 = value_120 + 2;
                        value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                    }
                } else if ((value_120 == 0) && (-1 < (int)(value_124 << 0x1f))) {
                    value_3 = value_4_3 & ~((int)value_4_3 >> 0x1f);
                } else {
                    text_length_2 = value_120 + 1 + value_4_3;
                    value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                }
            }
            if (libm_fpclassify_result == 0) {
                value_120 = 0;
                value_124 = value_124 | 0x100;
                byte_value_20 = byte_value_9;
                goto branch_13;
            }
            byte_value_20 = '-';
            byte_value_9 = '-';
            value_120 = 0;
            value_124 = value_124 | 0x100;
            goto branch_17;
        case 0x4c:
            value_124 = value_124 | 8;
            value_19 = (uint32_t)*argument_3;
            goto branch_5;
        case 0x4f:
            value_124 = value_124 | 0x10;
        case 0x6f:
            if ((value_124 & 0x20) == 0) {
                if (((value_124 & 0x10) == 0) && ((value_124 & 0x40) != 0)) {
                    value_12 = (uint32_t)(uint16_t)*pointer_108;
                    pointer_108 = pointer_108 + 1;
                    value_17 = 0;
                    libm_fpclassify_result = 0;
                } else {
                    libm_fpclassify_result = 0;
                    value_12 = *pointer_108;
                    pointer_108 = pointer_108 + 1;
                    value_17 = 0;
                }
            } else {
                pointer_10 = (uint32_t *)((int)pointer_108 + 7U & 0xfffffff8);
                pointer_108 = pointer_10 + 2;
                value_12 = *pointer_10;
                value_17 = pointer_10[1];
                libm_fpclassify_result = 0;
            }
        branch_18:
            byte_value_9 = '\0';
            break;
        case 0x55:
            value_124 = value_124 | 0x10;
        case 0x75:
            if ((int)(value_124 << 0x1a) < 0) {
                pointer_10 = (uint32_t *)((int)pointer_108 + 7U & 0xfffffff8);
                pointer_108 = pointer_10 + 2;
                value_12 = *pointer_10;
                value_17 = pointer_10[1];
                libm_fpclassify_result = 1;
            } else if (((int)(value_124 << 0x1b) < 0) || ((value_124 & 0x40) == 0)) {
                value_12 = *pointer_108;
                pointer_108 = pointer_108 + 1;
                libm_fpclassify_result = 1;
                value_17 = 0;
            } else {
                value_12 = (uint32_t)(uint16_t)*pointer_108;
                pointer_108 = pointer_108 + 1;
                value_17 = 0;
                libm_fpclassify_result = 1;
            }
            goto branch_18;
        case 0x58:
            text_0 = "0123456789ABCDEF";
            if (-1 < (int)(value_124 << 0x1a))
                goto branch_19;
        branch_20:
            pointer_10 = (uint32_t *)((int)pointer_108 + 7U & 0xfffffff8);
            pointer_108 = pointer_10 + 2;
            value_12 = *pointer_10;
            value_17 = pointer_10[1];
            goto branch_21;
        case 99:
            buffer_90[0] = (uint8_t)*pointer_108;
            text_length_2 = 1;
            pointer_108 = pointer_108 + 1;
            byte_value_20 = '\0';
            value_3 = 1;
            byte_value_9 = '\0';
            value_120 = 0;
            value_8 = 0;
            text_00 = buffer_90;
            goto branch_7;
        case 100:
        case 0x69:
            if (-1 < (int)(value_124 << 0x1a))
                goto branch_9;
        branch_8:
            pointer_108 = (uint32_t *)((int)pointer_108 + 7U & 0xfffffff8);
            value_12 = *pointer_108;
            value_17 = pointer_108[1];
            pointer_108 = pointer_108 + 2;
        branch_10:
            if (-1 < (int)value_17)
                goto branch_12;
        branch_11:
            condition_21 = value_12 != 0;
            value_12 = -value_12;
            value_17 = -value_17 - (uint32_t)condition_21;
            byte_value_9 = '-';
            libm_fpclassify_result = 1;
            break;
        case 0x68:
            value_124 = value_124 | 0x40;
            value_19 = (uint32_t)*argument_3;
            goto branch_5;
        case 0x6c:
            value_19 = (uint32_t)*argument_3;
            if (value_19 == 0x6c) {
                value_19 = (uint32_t)argument_3[1];
                value_124 = value_124 | 0x20;
                argument_3 = argument_3 + 1;
            } else {
                value_124 = value_124 | 0x10;
            }
            goto branch_5;
        case 0x6e:
            if ((value_124 & 0x20) == 0) {
                if ((value_124 & 0x10) == 0) {
                    if ((value_124 & 0x40) == 0) {
                        pointer_8 = (int *)*pointer_108;
                        pointer_108 = pointer_108 + 1;
                        *pointer_8 = value_110;
                    } else {
                        pointer_11 = (uint16_t *)*pointer_108;
                        pointer_108 = pointer_108 + 1;
                        *pointer_11 = (short)value_110;
                    }
                } else {
                    pointer_8 = (int *)*pointer_108;
                    pointer_108 = pointer_108 + 1;
                    *pointer_8 = value_110;
                }
            } else {
                pointer_8 = (int *)*pointer_108;
                pointer_108 = pointer_108 + 1;
                *pointer_8 = value_110;
                pointer_8[1] = value_110 >> 0x1f;
            }
            goto branch_1;
        case 0x70:
            value_12 = *pointer_108;
            text_0 = "0123456789abcdef";
            pointer_108 = pointer_108 + 1;
            value_19 = 0x78;
            value_124 = value_124 | 2;
            byte_value_8 = 0x30;
            value_17 = 0;
            byte_value_7 = 0x78;
            libm_fpclassify_result = 2;
            goto branch_18;
        case 0x71:
            value_124 = value_124 | 0x20;
            value_19 = (uint32_t)*argument_3;
            goto branch_5;
        case 0x73:
            text_00 = (char *)*pointer_108;
            byte_value_9 = '\0';
            pointer_108 = pointer_108 + 1;
            if ((int *)text_00 == (int *)0x0) {
                if (5 < value_120) {
                    text_length_2 = 6;
                }
                value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                text_00 = "(null)";
                value_120 = 0;
                value_8 = 0;
                byte_value_20 = '\0';
            } else if ((int)value_120 < 0) {
                value_120 = 0;
                text_length_2 = libc_strlen(text_00);
                value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                value_8 = 0;
                byte_value_20 = byte_value_9;
            } else {
                libc_memchr_result = PICO_UNRESOLVED_RETURN(libc_memchr(text_00, 0, value_120));
                byte_value_20 = byte_value_9;
                if (libc_memchr_result == (void *)0x0) {
                    value_3 = value_120 & ~((int)value_120 >> 0x1f);
                    value_8 = 0;
                    value_120 = 0;
                } else {
                    if ((int)libc_memchr_result - (int)text_00 < (int)value_120) {
                        text_length_2 = (int)libc_memchr_result - (int)text_00;
                    }
                    value_3 = text_length_2 & ~((int)text_length_2 >> 0x1f);
                    value_120 = 0;
                    value_8 = 0;
                }
            }
            goto branch_13;
        case 0x78:
            text_0 = "0123456789abcdef";
            if ((int)(value_124 << 0x1a) < 0)
                goto branch_20;
        branch_19:
            if ((int)(value_124 << 0x1b) < 0) {
                value_12 = *pointer_108;
                pointer_108 = pointer_108 + 1;
                value_17 = 0;
            } else if ((int)(value_124 << 0x19) < 0) {
                value_12 = (uint32_t)(uint16_t)*pointer_108;
                pointer_108 = pointer_108 + 1;
                value_17 = 0;
            } else {
                value_12 = *pointer_108;
                pointer_108 = pointer_108 + 1;
                value_17 = 0;
            }
        branch_21:
            if (((value_124 & 1) == 0) || (value_12 == 0 && value_17 == 0)) {
                libm_fpclassify_result = 2;
            } else {
                value_124 = value_124 | 2;
                byte_value_8 = 0x30;
                libm_fpclassify_result = 2;
                byte_value_7 = (char)value_19;
            }
            goto branch_18;
        }
        byte_value_20 = byte_value_9;
        value_22 = pico_join_u32_u32(value_17, value_12);
        if (-1 < (int)value_120) {
            value_124 = value_124 & 0xffffff7f;
        }
        if ((value_12 == 0 && value_17 == 0) && (value_120 == 0)) {
            if ((libm_fpclassify_result == 0) && ((value_124 & 1) != 0)) {
                text_00 = &byte_value_69;
                byte_value_69 = '0';
                text_length_2 = (int)buffer_68 - (int)text_00;
            } else {
                text_00 = (char *)buffer_68;
            }
        } else if (libm_fpclassify_result == 1) {
            if (value_17 == 0 && value_12 < 10) {
                byte_value_69 = (char)value_12 + '0';
                pointer_8 = (int *)&byte_value_69;
                goto branch_22;
            }
            pointer_8 = (int *)&byte_value_69;
            do {
                text_00 = (char *)pointer_8;
                text_length_2 = (uint32_t)(value_22 >> 0x20);
                soft_unsigned_long_long_divide_wrapper((uint32_t)value_22, text_length_2, 10, 0);
                *text_00 = unresolved_return_r2 + '0';
                value_22 = soft_unsigned_long_long_divide_wrapper((uint32_t)value_22, text_length_2,
                                                                  10, 0);
                pointer_8 = (int *)((int)text_00 + -1);
            } while (value_22 != 0);
            text_length_2 = (int)buffer_68 - (int)text_00;
        } else {
            text_00 = &byte_value_69;
            if (libm_fpclassify_result == 2) {
                pointer_8 = (int *)&byte_value_69;
                do {
                    text_00 = (char *)pointer_8;
                    text_length_2 = value_12 & 0xf;
                    value_12 = value_12 >> 4 | value_17 << 0x1c;
                    value_17 = value_17 >> 4;
                    *text_00 = text_0[text_length_2];
                    pointer_8 = (int *)((int)text_00 + -1);
                } while (value_12 != 0 || value_17 != 0);
                text_length_2 = (int)buffer_68 - (int)text_00;
                goto branch_23;
            }
            do {
                pointer_8 = (int *)text_00;
                text_length_2 = value_12 >> 3 | value_17 << 0x1d;
                value_17 = value_17 >> 3;
                libm_fpclassify_result = (value_12 & 7) + 0x30;
                *(char *)pointer_8 = (char)libm_fpclassify_result;
                text_00 = (char *)((int)pointer_8 + -1);
                value_12 = text_length_2;
            } while (text_length_2 != 0 || value_17 != 0);
            if (((int)(value_124 << 0x1f) < 0) && (libm_fpclassify_result != 0x30)) {
                text_length_2 = (int)buffer_68 - (int)text_00;
                *(char *)((int)pointer_8 + -1) = '0';
            } else {
            branch_22:
                text_00 = (char *)pointer_8;
                text_length_2 = (int)buffer_68 - (int)text_00;
            }
        }
    branch_23:
        value_3 = value_120;
        if ((int)value_120 <= (int)text_length_2) {
            value_3 = text_length_2;
        }
        value_8 = 0;
    branch_13:
        if (byte_value_20 != '\0') {
        branch_17:
            value_3 = value_3 + 1;
        }
    branch_7:
        if ((value_124 & 2) != 0) {
            value_3 = value_3 + 2;
        }
        if (((value_124 & 0x84) == 0) &&
            (libm_fpclassify_result = value - value_3, 0 < libm_fpclassify_result)) {
            if (0x10 < libm_fpclassify_result) {
                do {
                    value_13 = libm_fpclassify_result;
                    value_98 = value_98 + 1;
                    value_94 = value_94 + 0x10;
                    *pointer_5 = (int)"                ";
                    pointer_5[1] = 0x10;
                    if (value_98 < 8) {
                        pointer_5 = pointer_5 + 2;
                    } else {
                        libm_fpclassify_result =
                            unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                        if (libm_fpclassify_result != 0)
                            goto branch_2;
                        pointer_5 = buffer_68;
                    }
                    libm_fpclassify_result = value_13 + -0x10;
                } while (0x10 < libm_fpclassify_result);
                libm_fpclassify_result = value_13 + -0x10;
            }
            value_98 = value_98 + 1;
            value_94 = value_94 + libm_fpclassify_result;
            *pointer_5 = (int)"                ";
            pointer_5[1] = libm_fpclassify_result;
            if (value_98 < 8) {
                pointer_5 = pointer_5 + 2;
                byte_value_20 = byte_value_9;
            } else {
                libm_fpclassify_result =
                    unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                if (libm_fpclassify_result != 0)
                    goto branch_2;
                pointer_5 = buffer_68;
                byte_value_20 = byte_value_9;
            }
        }
        if (byte_value_20 != '\0') {
            value_98 = value_98 + 1;
            value_94 = value_94 + 1;
            *pointer_5 = (int)&byte_value_9;
            pointer_5[1] = 1;
            if (value_98 < 8) {
                pointer_5 = pointer_5 + 2;
            } else {
                libm_fpclassify_result =
                    unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                if (libm_fpclassify_result != 0)
                    goto branch_2;
                pointer_5 = buffer_68;
            }
        }
        if ((value_124 & 2) != 0) {
            value_98 = value_98 + 1;
            value_94 = value_94 + 2;
            *pointer_5 = (int)&byte_value_8;
            pointer_5[1] = 2;
            if (value_98 < 8) {
                pointer_5 = pointer_5 + 2;
            } else {
                libm_fpclassify_result =
                    unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                if (libm_fpclassify_result != 0)
                    goto branch_2;
                pointer_5 = buffer_68;
            }
        }
        if (((value_124 & 0x84) == 0x80) &&
            (libm_fpclassify_result = value - value_3, 0 < libm_fpclassify_result)) {
            if (0x10 < libm_fpclassify_result) {
                do {
                    while (true) {
                        value_13 = libm_fpclassify_result;
                        value_98 = value_98 + 1;
                        value_94 = value_94 + 0x10;
                        *pointer_5 = (int)"0000000000000000INF";
                        pointer_5[1] = 0x10;
                        if (value_98 < 8)
                            break;
                        libm_fpclassify_result =
                            unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                        if (libm_fpclassify_result != 0)
                            goto branch_2;
                        pointer_5 = buffer_68;
                        libm_fpclassify_result = value_13 + -0x10;
                        if (value_13 + -0x10 < 0x11)
                            goto branch_24;
                    }
                    pointer_5 = pointer_5 + 2;
                    libm_fpclassify_result = value_13 + -0x10;
                } while (0x10 < value_13 + -0x10);
            branch_24:
                libm_fpclassify_result = value_13 + -0x10;
            }
            value_98 = value_98 + 1;
            value_94 = value_94 + libm_fpclassify_result;
            *pointer_5 = (int)"0000000000000000INF";
            pointer_5[1] = libm_fpclassify_result;
            if (value_98 < 8) {
                pointer_5 = pointer_5 + 2;
            } else {
                libm_fpclassify_result =
                    unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                if (libm_fpclassify_result != 0)
                    goto branch_2;
                pointer_5 = buffer_68;
            }
        }
        libm_fpclassify_result = value_120 - text_length_2;
        if (0 < libm_fpclassify_result) {
            if (0x10 < libm_fpclassify_result) {
                do {
                    while (true) {
                        value_13 = libm_fpclassify_result;
                        value_98 = value_98 + 1;
                        value_94 = value_94 + 0x10;
                        *pointer_5 = (int)"0000000000000000INF";
                        pointer_5[1] = 0x10;
                        if (value_98 < 8)
                            break;
                        libm_fpclassify_result =
                            unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                        if (libm_fpclassify_result != 0)
                            goto branch_2;
                        pointer_5 = buffer_68;
                        libm_fpclassify_result = value_13 + -0x10;
                        if (value_13 + -0x10 < 0x11)
                            goto branch_25;
                    }
                    pointer_5 = pointer_5 + 2;
                    libm_fpclassify_result = value_13 + -0x10;
                } while (0x10 < value_13 + -0x10);
            branch_25:
                libm_fpclassify_result = value_13 + -0x10;
            }
            value_98 = value_98 + 1;
            value_94 = value_94 + libm_fpclassify_result;
            *pointer_5 = (int)"0000000000000000INF";
            pointer_5[1] = libm_fpclassify_result;
            if (value_98 < 8) {
                pointer_5 = pointer_5 + 2;
            } else {
                libm_fpclassify_result =
                    unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                if (libm_fpclassify_result != 0)
                    goto branch_2;
                pointer_5 = buffer_68;
            }
        }
        libm_fpclassify_result = value_94;
        if ((int)(value_124 << 0x17) < 0) {
            if ((int)value_19 < 0x66) {
                if (((int)value_104 < 2) && (-1 < (int)(value_124 << 0x1f))) {
                    value_94 = value_94 + 1;
                    *pointer_5 = (int)text_00;
                    pointer_5[1] = 1;
                    libm_fpclassify_result = value_98;
                joined_r0x2001deba:
                    value_98 = libm_fpclassify_result + 1;
                    if (value_98 < 8) {
                        pointer_5 = pointer_5 + 2;
                        libm_fpclassify_result = value_98;
                        value_13 = value_94;
                    } else {
                        libm_fpclassify_result =
                            unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                        if (libm_fpclassify_result != 0)
                            goto branch_2;
                        pointer_5 = buffer_68;
                        libm_fpclassify_result = value_98;
                        value_13 = value_94;
                    }
                } else {
                    value_98 = value_98 + 1;
                    value_94 = value_94 + 1;
                    *pointer_5 = (int)text_00;
                    pointer_5[1] = 1;
                    if (value_98 < 8) {
                        pointer_5 = pointer_5 + 2;
                    } else {
                        libm_fpclassify_result =
                            unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                        if (libm_fpclassify_result != 0)
                            goto branch_2;
                        pointer_5 = buffer_68;
                    }
                    *pointer_5 = (int)text;
                    value_98 = value_98 + 1;
                    value_94 = value_94 + text_length;
                    pointer_5[1] = text_length;
                    if (value_98 < 8) {
                        pointer_5 = pointer_5 + 2;
                    } else {
                        libm_fpclassify_result =
                            unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                        if (libm_fpclassify_result != 0)
                            goto branch_2;
                        pointer_5 = buffer_68;
                    }
                    value_13 = value_94;
                    libm_fpclassify_result = value_98;
                    __aeabi_dcmpeq_result =
                        __aeabi_dcmpeq((double)pico_join_u32_u32(value_2, value_0), 0.0);
                    if (__aeabi_dcmpeq_result == 0) {
                        value_94 = value_13 + (value_104 - 1);
                        *pointer_5 = (int)((int)text_00 + 1);
                        pointer_5[1] = value_104 - 1;
                        goto joined_r0x2001deba;
                    }
                    __aeabi_dcmpeq_result = value_104 - 1;
                    if (0 < __aeabi_dcmpeq_result) {
                        value_98 = libm_fpclassify_result;
                        value_94 = value_13;
                        if (0x10 < __aeabi_dcmpeq_result) {
                            do {
                                while (true) {
                                    libm_fpclassify_result = __aeabi_dcmpeq_result;
                                    value_98 = value_98 + 1;
                                    value_94 = value_94 + 0x10;
                                    *pointer_5 = (int)"0000000000000000INF";
                                    pointer_5[1] = 0x10;
                                    if (value_98 < 8)
                                        break;
                                    value_13 = unresolved_floating_point_07(argument_1, argument_2,
                                                                            (int *)&pointer);
                                    if (value_13 != 0)
                                        goto branch_2;
                                    pointer_5 = buffer_68;
                                    __aeabi_dcmpeq_result = libm_fpclassify_result + -0x10;
                                    if (libm_fpclassify_result + -0x10 < 0x11)
                                        goto branch_26;
                                }
                                pointer_5 = pointer_5 + 2;
                                __aeabi_dcmpeq_result = libm_fpclassify_result + -0x10;
                            } while (0x10 < libm_fpclassify_result + -0x10);
                        branch_26:
                            __aeabi_dcmpeq_result = libm_fpclassify_result + -0x10;
                        }
                        value_94 = value_94 + __aeabi_dcmpeq_result;
                        *pointer_5 = (int)"0000000000000000INF";
                        pointer_5[1] = __aeabi_dcmpeq_result;
                        libm_fpclassify_result = value_98;
                        goto joined_r0x2001deba;
                    }
                }
                value_94 = value_13 + value_4;
                *pointer_5 = (int)&byte_value;
                pointer_5[1] = value_4;
                value_98 = libm_fpclassify_result;
                goto joined_r0x2001deea;
            }
            value_13 = __aeabi_dcmpeq((double)pico_join_u32_u32(value_2, value_0), 0.0);
            if (value_13 == 0) {
                if (0 < (int)value_4_3) {
                    value_19 = value_104;
                    if ((int)value_8 <= (int)value_104) {
                        value_19 = value_8;
                    }
                    if (0 < (int)value_19) {
                        *pointer_5 = (int)text_00;
                        value_98 = value_98 + 1;
                        value_94 = libm_fpclassify_result + value_19;
                        pointer_5[1] = value_19;
                        if (value_98 < 8) {
                            pointer_5 = pointer_5 + 2;
                            libm_fpclassify_result = value_94;
                        } else {
                            libm_fpclassify_result = unresolved_floating_point_07(
                                argument_1, argument_2, (int *)&pointer);
                            if (libm_fpclassify_result != 0)
                                goto branch_2;
                            pointer_5 = buffer_68;
                            libm_fpclassify_result = value_94;
                        }
                    }
                    value_13 = value_8 - (value_19 & ~((int)value_19 >> 0x1f));
                    if (0 < value_13) {
                        value_94 = libm_fpclassify_result;
                        if (0x10 < value_13) {
                            do {
                                value_98 = value_98 + 1;
                                value_94 = value_94 + 0x10;
                                *pointer_5 = (int)"0000000000000000INF";
                                pointer_5[1] = 0x10;
                                if (value_98 < 8) {
                                    pointer_5 = pointer_5 + 2;
                                } else {
                                    libm_fpclassify_result = unresolved_floating_point_07(
                                        argument_1, argument_2, (int *)&pointer);
                                    if (libm_fpclassify_result != 0)
                                        goto branch_2;
                                    pointer_5 = buffer_68;
                                }
                                value_13 = value_13 + -0x10;
                            } while (0x10 < value_13);
                        }
                        value_98 = value_98 + 1;
                        value_94 = value_94 + value_13;
                        *pointer_5 = (int)"0000000000000000INF";
                        pointer_5[1] = value_13;
                        if (value_98 < 8) {
                            pointer_5 = pointer_5 + 2;
                            libm_fpclassify_result = value_94;
                        } else {
                            libm_fpclassify_result = unresolved_floating_point_07(
                                argument_1, argument_2, (int *)&pointer);
                            if (libm_fpclassify_result != 0)
                                goto branch_2;
                            pointer_5 = buffer_68;
                            libm_fpclassify_result = value_94;
                        }
                    }
                    if (((int)value_4_3 < (int)value_104) || ((int)(value_124 << 0x1f) < 0)) {
                        *pointer_5 = (int)text;
                        pointer_5[1] = text_length;
                        value_98 = value_98 + 1;
                        value_94 = libm_fpclassify_result + text_length;
                        if (value_98 < 8) {
                            pointer_5 = pointer_5 + 2;
                            libm_fpclassify_result = value_94;
                        } else {
                            libm_fpclassify_result = unresolved_floating_point_07(
                                argument_1, argument_2, (int *)&pointer);
                            if (libm_fpclassify_result != 0)
                                goto branch_2;
                            pointer_5 = buffer_68;
                            libm_fpclassify_result = value_94;
                        }
                    }
                    text_18 = (char *)((int)text_00 + (value_104 - (int)((int)text_00 + value_8)));
                    text_15 = (char *)(value_104 - value_4_3);
                    if ((int)text_15 < (int)text_18) {
                        text_18 = text_15;
                    }
                    if (0 < (int)text_18) {
                        *pointer_5 = (int)((int)text_00 + value_8);
                        value_98 = value_98 + 1;
                        value_94 = libm_fpclassify_result + (int)text_18;
                        pointer_5[1] = (int)text_18;
                        if (value_98 < 8) {
                            pointer_5 = pointer_5 + 2;
                            libm_fpclassify_result = value_94;
                        } else {
                            libm_fpclassify_result = unresolved_floating_point_07(
                                argument_1, argument_2, (int *)&pointer);
                            if (libm_fpclassify_result != 0)
                                goto branch_2;
                            text_15 = (char *)(value_104 - value_4_3);
                            pointer_5 = buffer_68;
                            libm_fpclassify_result = value_94;
                        }
                    }
                    value_13 = (int)text_15 - ((uint32_t)text_18 & ~((int)text_18 >> 0x1f));
                    if (0 < value_13) {
                        if (0x10 < value_13) {
                            value_94 = libm_fpclassify_result;
                            do {
                                value_98 = value_98 + 1;
                                value_94 = value_94 + 0x10;
                                *pointer_5 = (int)"0000000000000000INF";
                                pointer_5[1] = 0x10;
                                if (value_98 < 8) {
                                    pointer_5 = pointer_5 + 2;
                                } else {
                                    libm_fpclassify_result = unresolved_floating_point_07(
                                        argument_1, argument_2, (int *)&pointer);
                                    if (libm_fpclassify_result != 0)
                                        goto branch_2;
                                    pointer_5 = buffer_68;
                                }
                                value_13 = value_13 + -0x10;
                            } while (0x10 < value_13);
                            goto branch_27;
                        }
                        goto branch_28;
                    }
                    goto branch_29;
                }
                *pointer_5 = (int)&g_c_library_unknown_07;
                value_98 = value_98 + 1;
                value_94 = libm_fpclassify_result + 1;
                pointer_5[1] = 1;
                if (value_98 < 8) {
                    pointer_5 = pointer_5 + 2;
                } else {
                    libm_fpclassify_result =
                        unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                    if (libm_fpclassify_result != 0)
                        goto branch_2;
                    pointer_5 = buffer_68;
                }
                if ((value_4_3 == 0) && (value_104 == 0)) {
                    libm_fpclassify_result = value_94;
                    if (-1 < (int)(value_124 << 0x1f))
                        goto branch_29;
                }
                *pointer_5 = (int)text;
                value_98 = value_98 + 1;
                value_94 = value_94 + text_length;
                pointer_5[1] = text_length;
                if (value_98 < 8) {
                    pointer_5 = pointer_5 + 2;
                } else {
                    libm_fpclassify_result =
                        unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                    if (libm_fpclassify_result != 0)
                        goto branch_2;
                    pointer_5 = buffer_68;
                }
                libm_fpclassify_result = -value_4_3;
                if (value_4_3 != 0 && -1 < libm_fpclassify_result) {
                    if (value_4_3 != 0xfffffff0 && 0xf < libm_fpclassify_result) {
                        do {
                            value_98 = value_98 + 1;
                            value_94 = value_94 + 0x10;
                            *pointer_5 = (int)"0000000000000000INF";
                            pointer_5[1] = 0x10;
                            if (value_98 < 8) {
                                pointer_5 = pointer_5 + 2;
                            } else {
                                value_13 = unresolved_floating_point_07(argument_1, argument_2,
                                                                        (int *)&pointer);
                                if (value_13 != 0)
                                    goto branch_2;
                                pointer_5 = buffer_68;
                            }
                            libm_fpclassify_result = libm_fpclassify_result + -0x10;
                        } while (0x10 < libm_fpclassify_result);
                    }
                    value_98 = value_98 + 1;
                    value_94 = value_94 + libm_fpclassify_result;
                    *pointer_5 = (int)"0000000000000000INF";
                    pointer_5[1] = libm_fpclassify_result;
                    if (value_98 < 8) {
                        pointer_5 = pointer_5 + 2;
                    } else {
                        libm_fpclassify_result =
                            unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                        if (libm_fpclassify_result != 0)
                            goto branch_2;
                        pointer_5 = buffer_68;
                    }
                }
                *pointer_5 = (int)text_00;
                value_98 = value_98 + 1;
                libm_fpclassify_result = value_94 + value_104;
                pointer_5[1] = value_104;
                value_94 = libm_fpclassify_result;
                if (value_98 < 8) {
                    pointer_5 = pointer_5 + 2;
                    goto branch_29;
                }
                goto branch_30;
            }
            *pointer_5 = (int)&g_c_library_unknown_07;
            value_98 = value_98 + 1;
            value_94 = libm_fpclassify_result + 1;
            pointer_5[1] = 1;
            if (value_98 < 8) {
                pointer_5 = pointer_5 + 2;
            } else {
                libm_fpclassify_result =
                    unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                if (libm_fpclassify_result != 0)
                    goto branch_2;
                pointer_5 = buffer_68;
            }
            if ((int)value_104 <= (int)value_4_3) {
                libm_fpclassify_result = value_94;
                if (-1 < (int)(value_124 << 0x1f))
                    goto branch_29;
            }
            *pointer_5 = (int)text;
            value_98 = value_98 + 1;
            value_94 = value_94 + text_length;
            pointer_5[1] = text_length;
            if (value_98 < 8) {
                pointer_5 = pointer_5 + 2;
            } else {
                libm_fpclassify_result =
                    unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                if (libm_fpclassify_result != 0)
                    goto branch_2;
                pointer_5 = buffer_68;
            }
            value_13 = value_104 - 1;
            libm_fpclassify_result = value_94;
            if (0 < value_13) {
                if (0x10 < value_13) {
                    do {
                        value_98 = value_98 + 1;
                        value_94 = value_94 + 0x10;
                        *pointer_5 = (int)"0000000000000000INF";
                        pointer_5[1] = 0x10;
                        if (value_98 < 8) {
                            pointer_5 = pointer_5 + 2;
                        } else {
                            libm_fpclassify_result = unresolved_floating_point_07(
                                argument_1, argument_2, (int *)&pointer);
                            if (libm_fpclassify_result != 0)
                                goto branch_2;
                            pointer_5 = buffer_68;
                        }
                        value_13 = value_13 + -0x10;
                    } while (0x10 < value_13);
                branch_27:
                    libm_fpclassify_result = value_94;
                }
            branch_28:
                value_94 = libm_fpclassify_result + value_13;
                *pointer_5 = (int)"0000000000000000INF";
                pointer_5[1] = value_13;
                goto joined_r0x2001deea;
            }
        } else {
            *pointer_5 = (int)text_00;
            value_94 = value_94 + text_length_2;
            pointer_5[1] = text_length_2;
        joined_r0x2001deea:
            value_98 = value_98 + 1;
            if (value_98 < 8) {
                pointer_5 = pointer_5 + 2;
                libm_fpclassify_result = value_94;
            } else {
            branch_30:
                libm_fpclassify_result =
                    unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                if (libm_fpclassify_result != 0)
                    goto branch_2;
                pointer_5 = buffer_68;
                libm_fpclassify_result = value_94;
            }
        }
    branch_29:
        if ((int)(value_124 << 0x1d) < 0) {
            value_13 = value - value_3;
            if (0 < value_13) {
                value_94 = libm_fpclassify_result;
                if (0x10 < value_13) {
                    do {
                        while (true) {
                            libm_fpclassify_result = value_13;
                            value_98 = value_98 + 1;
                            value_94 = value_94 + 0x10;
                            *pointer_5 = (int)"                ";
                            pointer_5[1] = 0x10;
                            if (value_98 < 8)
                                break;
                            value_13 = unresolved_floating_point_07(argument_1, argument_2,
                                                                    (int *)&pointer);
                            if (value_13 != 0)
                                goto branch_2;
                            pointer_5 = buffer_68;
                            value_13 = libm_fpclassify_result + -0x10;
                            if (libm_fpclassify_result + -0x10 < 0x11)
                                goto branch_31;
                        }
                        pointer_5 = pointer_5 + 2;
                        value_13 = libm_fpclassify_result + -0x10;
                    } while (0x10 < libm_fpclassify_result + -0x10);
                branch_31:
                    value_13 = libm_fpclassify_result + -0x10;
                }
                value_98 = value_98 + 1;
                value_94 = value_94 + value_13;
                *pointer_5 = (int)"                ";
                pointer_5[1] = value_13;
                libm_fpclassify_result = value_94;
                if (7 < value_98) {
                    libm_fpclassify_result =
                        unresolved_floating_point_07(argument_1, argument_2, (int *)&pointer);
                    if (libm_fpclassify_result != 0)
                        goto branch_2;
                    libm_fpclassify_result = value_94;
                }
            }
        }
        if ((int)value_3 < (int)value) {
            value_3 = value;
        }
        value_110 = value_110 + value_3;
        if ((libm_fpclassify_result != 0) && (libm_fpclassify_result = unresolved_floating_point_07(
                                                  argument_1, argument_2, (int *)&pointer),
                                              libm_fpclassify_result != 0))
            goto branch_2;
        value_98 = 0;
        pointer_5 = buffer_68;
    } while (true);
switchD_2001d2da_caseD_31:
    text_length_2 = value_19 - 0x30;
    value = 0;
    do {
        pointer_9 = argument_3 + 1;
        value_19 = (uint32_t)*argument_3;
        value = text_length_2 + value * 10;
        text_length_2 = value_19 - 0x30;
        argument_3 = pointer_9;
    } while (text_length_2 < 10);
    goto branch_6;
switchD_2001d2da_caseD_2e:
    value_19 = (uint32_t)*argument_3;
    pointer_9 = argument_3 + 1;
    if (value_19 == 0x2a) {
        value_120 = *pointer_108;
        value_19 = (uint32_t)argument_3[1];
        pointer_108 = pointer_108 + 1;
        argument_3 = pointer_9;
        if ((int)value_120 < 0)
            goto branch_4;
        goto branch_5;
    }
    text_length_2 = value_19 - 0x30;
    if (text_length_2 < 10) {
        value_120 = 0;
        do {
            argument_3 = pointer_9 + 1;
            value_19 = (uint32_t)*pointer_9;
            value_120 = text_length_2 + value_120 * 10;
            text_length_2 = value_19 - 0x30;
            pointer_9 = argument_3;
        } while (text_length_2 < 10);
        value_120 = value_120 | (int)value_120 >> 0x1f;
    } else {
        value_120 = 0;
        argument_3 = pointer_9;
    }
    goto branch_6;
switchD_2001d2da_caseD_20:
    value_19 = (uint32_t)*argument_3;
    if (byte_value_9 == '\0') {
        byte_value_9 = ' ';
    }
    goto branch_5;
}

/* Purpose incomplete.
 * Firmware address: 0x2001e7b4. */
uint32_t unresolved_c_library_09(int argument_1, int argument_2)
{
    int value_1;
    uint32_t *pointer_3;
    uint32_t *pointer_2;
    uint32_t *pointer_4;
    uint32_t *pointer_5;
    int value_6;
    int value_7;
    uint32_t value_8;
    uint32_t value_9;
    uint32_t *pointer_10;
    uint32_t *pointer_11;
    uint32_t value_12;

    value_6 = *(int *)(argument_2 + 0x10);
    if (*(int *)(argument_1 + 0x10) < value_6) {
        return 0;
    }
    value_7 = value_6 + -1;
    pointer_5 = (uint32_t *)(argument_2 + 0x14);
    pointer_11 = (uint32_t *)(argument_1 + 0x14);
    value_9 = pointer_11[value_7] / (pointer_5[value_7] + 1);
    pointer_10 = pointer_5 + value_7;
    if (value_9 != 0) {
        value_12 = 0;
        value_1 = 0;
        pointer_2 = pointer_5;
        pointer_4 = pointer_11;
        do {
            pointer_3 = pointer_2 + 1;
            value_12 = (*pointer_2 & 0xffff) * value_9 + value_12;
            value_8 = (*pointer_2 >> 0x10) * value_9 + (value_12 >> 0x10);
            value_12 = (value_1 - (value_12 & 0xffff)) + (*pointer_4 & 0xffff);
            value_1 = ((*pointer_4 >> 0x10) - (value_8 & 0xffff)) + ((int)value_12 >> 0x10);
            *pointer_4 = value_12 & 0xffff | value_1 * 0x10000;
            value_1 = value_1 >> 0x10;
            value_12 = value_8 >> 0x10;
            pointer_2 = pointer_3;
            pointer_4 = pointer_4 + 1;
        } while (pointer_3 <= pointer_10);
        if (pointer_11[value_7] == 0) {
            if ((pointer_11 < pointer_11 + value_6 + -2) && (pointer_11[value_6 + -2] == 0)) {
                pointer_2 = pointer_11 + value_6 + -3;
                do {
                    value_7 = value_7 + -1;
                    if (pointer_2 <= pointer_11)
                        break;
                    value_12 = *pointer_2;
                    pointer_2 = pointer_2 + -1;
                } while (value_12 == 0);
            }
            *(int *)(argument_1 + 0x10) = value_7;
        }
    }
    value_6 = unresolved_c_library_24_2(argument_1, argument_2);
    if (-1 < value_6) {
        value_9 = value_9 + 1;
        value_6 = 0;
        pointer_2 = pointer_11;
        do {
            pointer_4 = pointer_5 + 1;
            value_12 = (value_6 - (*pointer_5 & 0xffff)) + (*pointer_2 & 0xffff);
            value_6 = ((*pointer_2 >> 0x10) - (*pointer_5 >> 0x10)) + ((int)value_12 >> 0x10);
            *pointer_2 = value_12 & 0xffff | value_6 * 0x10000;
            value_6 = value_6 >> 0x10;
            pointer_2 = pointer_2 + 1;
            pointer_5 = pointer_4;
        } while (pointer_4 <= pointer_10);
        if (pointer_11[value_7] == 0) {
            if ((pointer_11 < pointer_11 + value_7 + -1) && (pointer_11[value_7 + -1] == 0)) {
                pointer_5 = pointer_11 + value_7 + -2;
                do {
                    value_7 = value_7 + -1;
                    if (pointer_5 <= pointer_11)
                        break;
                    value_12 = *pointer_5;
                    pointer_5 = pointer_5 + -1;
                } while (value_12 == 0);
            }
            *(int *)(argument_1 + 0x10) = value_7;
        }
    }
    return value_9;
}

/* Purpose incomplete.
 * Firmware address: 0x2001e8e8. */
int *unresolved_c_library_10(int *argument_1, uint32_t argument_2, uint32_t argument_3,
                             uint32_t argument_4, uint32_t argument_5, uint32_t argument_6,
                             int *argument_7, uint32_t *argument_8, uint32_t *argument_9)
{
    bool condition_1;
    bool condition_2;
    bool condition_3;
    int __aeabi_dcmpeq_result;
    char *text_5;
    int __aeabi_dcmplt_result;
    uint32_t *pointer_7;
    uint32_t *value;
    uint32_t value_8;
    uint32_t *pointer_9;
    uint32_t *pointer_10;
    uint32_t unresolved_return_r1;
    uint32_t value_11;
    char byte_value_12;
    uint32_t value_13;
    int *pointer_14;
    int *pointer_15;
    int __aeabi_dcmplt_result_2;
    uint32_t value_17;
    uint32_t *value_00;
    uint32_t value_18;
    double *pointer_19;
    int *pointer_20;
    double __aeabi_ui2d_result;
    double __aeabi_i2d_result_2;
    double __aeabi_i2d_result;
    uint32_t value_78;
    uint32_t __aeabi_d2iz_result;
    uint32_t value_70;
    int *pointer;
    uint32_t *pointer_68;
    uint32_t value_60;
    uint32_t value_2;
    uint32_t value_54;
    uint64_t __aeabi_ddiv_result;
    uint32_t value_34;
    int value_30;
    int buffer[2];

    value_17 = argument_5;
    pointer_10 = (uint32_t *)argument_1[0x10];
    if (pointer_10 != (uint32_t *)0x0) {
        value_13 = argument_1[0x11];
        pointer_10[1] = value_13;
        pointer_10[2] = 1 << (value_13 & 0xff);
        unresolved_c_library_18((int)argument_1, pointer_10);
        argument_1[0x10] = 0;
    }
    if ((int)argument_4 < 0) {
        argument_4 = argument_4 & 0x7fffffff;
        *argument_8 = 1;
    } else {
        *argument_8 = 0;
    }
    if ((argument_4 & 0x7ff00000) == 0x7ff00000) {
        *argument_7 = (int)&g_c_library_unknown_03;
        if (argument_3 == 0) {
            if ((argument_4 & 0xfffff) == 0) {
                text_5 = "Infinity";
            } else {
                text_5 = &g_c_library_unknown_09;
            }
        } else {
            text_5 = &g_c_library_unknown_09;
        }
        if (argument_9 == (uint32_t *)0x0) {
            return (int *)text_5;
        }
        if (*(char *)((int)text_5 + 3) == '\0') {
            pointer_15 = (int *)((int)text_5 + 3);
        } else {
            pointer_15 = (int *)((int)text_5 + 8);
        }
        *argument_9 = pointer_15;
        return (int *)text_5;
    }
    __aeabi_dcmpeq_result = __aeabi_dcmpeq((double)pico_join_u32_u32(argument_4, argument_3), 0.0);
    if (__aeabi_dcmpeq_result != 0) {
        *argument_7 = 1;
        if (argument_9 == (uint32_t *)0x0) {
            return (int *)&g_c_library_unknown_07;
        }
        *argument_9 = &g_c_library_unknown_08;
        return (int *)&g_c_library_unknown_07;
    }
    pointer_68 = (uint32_t *)unresolved_floating_point_03(
        argument_1, unresolved_return_r1, argument_3, argument_4, buffer, &value_30);
    __aeabi_dcmpeq_result = value_30;
    if (argument_4 >> 0x14 == 0) {
        __aeabi_dcmplt_result_2 = buffer[0] + value_30;
        if (__aeabi_dcmplt_result_2 < -0x411) {
            value_13 = argument_3 << (-__aeabi_dcmplt_result_2 - 0x412U & 0xff);
        } else {
            value_13 = argument_4 << (-__aeabi_dcmplt_result_2 - 0x3f2U & 0xff) |
                       argument_3 >> (__aeabi_dcmplt_result_2 + 0x412U & 0xff);
        }
        __aeabi_ui2d_result = __aeabi_ui2d(value_13);
        __aeabi_dcmplt_result_2 = __aeabi_dcmplt_result_2 + -1;
        value_11 = (int)((uint64_t)__aeabi_ui2d_result >> 0x20) + 0xfe100000;
        condition_3 = true;
        value_13 = PICO_SUB_U32(__aeabi_ui2d_result, 0);
    } else {
        __aeabi_dcmplt_result_2 = (argument_4 >> 0x14) - 0x3ff;
        value_11 = argument_4 & 0xfffff | 0x3ff00000;
        condition_3 = false;
        value_13 = argument_3;
    }
    __aeabi_ui2d_result = __aeabi_dsub((double)pico_join_u32_u32(value_11, value_13), 1.5);
    __aeabi_ui2d_result = __aeabi_dmul(__aeabi_ui2d_result, 0.289529654602168);
    __aeabi_ui2d_result = __aeabi_dadd(__aeabi_ui2d_result, 0.1760912590558);
    __aeabi_i2d_result_2 = __aeabi_i2d(__aeabi_dcmplt_result_2);
    __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_i2d_result_2, 0.301029995663981);
    __aeabi_ui2d_result = __aeabi_dadd(__aeabi_ui2d_result, __aeabi_i2d_result_2);
    __aeabi_d2iz_result = __aeabi_d2iz(__aeabi_ui2d_result);
    __aeabi_dcmplt_result = __aeabi_dcmplt(__aeabi_ui2d_result, 0.0);
    if (__aeabi_dcmplt_result != 0) {
        __aeabi_i2d_result_2 = __aeabi_i2d(__aeabi_d2iz_result);
        __aeabi_dcmplt_result = __aeabi_dcmpeq(__aeabi_i2d_result_2, __aeabi_ui2d_result);
        if (__aeabi_dcmplt_result == 0) {
            __aeabi_d2iz_result = __aeabi_d2iz_result - 1;
        }
    }
    if (__aeabi_d2iz_result < 0x17) {
        __aeabi_dcmplt_result =
            __aeabi_dcmpgt(*(double *)(&g_libc_unknown_09 + __aeabi_d2iz_result * 8),
                           (double)pico_join_u32_u32(argument_4, argument_3));
        if (__aeabi_dcmplt_result == 0) {
            condition_2 = false;
        } else {
            __aeabi_d2iz_result = __aeabi_d2iz_result - 1;
            condition_2 = false;
        }
    } else {
        condition_2 = true;
    }
    value_78 = (__aeabi_dcmpeq_result - __aeabi_dcmplt_result_2) - 1;
    if ((int)value_78 < 0) {
        value_70 = -value_78;
        value_78 = 0;
    } else {
        value_70 = 0;
    }
    if ((int)__aeabi_d2iz_result < 0) {
        value_70 = value_70 - __aeabi_d2iz_result;
        value_60 = -__aeabi_d2iz_result;
        value_2 = 0;
    } else {
        value_2 = __aeabi_d2iz_result;
        value_78 = value_78 + __aeabi_d2iz_result;
        value_60 = 0;
    }
    if (9 < argument_5) {
        argument_5 = 0;
    switchD_2001ea9c_default:
        argument_1[0x11] = 0;
        pointer = unresolved_c_library_17(argument_1, 0);
        value_13 = 0xffffffff;
        argument_6 = 0;
        argument_1[0x10] = (int)pointer;
        value_54 = 0xffffffff;
        condition_1 = true;
        goto branch_1;
    }
    if (5 < (int)argument_5) {
        argument_5 = argument_5 - 4;
    }
    switch (argument_5) {
    case 2:
        condition_1 = false;
        break;
    case 3:
        condition_1 = false;
        goto branch_2;
    case 4:
        condition_1 = true;
        break;
    case 5:
        condition_1 = true;
    branch_2:
        value_54 = argument_6 + __aeabi_d2iz_result;
        value_13 = value_54 + 1;
        if ((int)value_13 < 1)
            goto branch_3;
        goto branch_4;
    default:
        goto switchD_2001ea9c_default;
    }
    if ((int)argument_6 < 1) {
        argument_6 = 1;
        value_54 = 1;
        value_13 = 1;
    branch_3:
        value_11 = 0;
        argument_1[0x11] = 0;
    } else {
        value_54 = argument_6;
        value_13 = argument_6;
    branch_4:
        value_11 = 0;
        argument_1[0x11] = 0;
        if (0x17 < value_13) {
            __aeabi_dcmpeq_result = 4;
            value_18 = 1;
            do {
                value_11 = value_18;
                __aeabi_dcmpeq_result = __aeabi_dcmpeq_result * 2;
                value_18 = value_11 + 1;
            } while (__aeabi_dcmpeq_result + 0x14U <= value_13);
            argument_1[0x11] = value_11;
        }
    }
    pointer_15 = unresolved_c_library_17(argument_1, value_11);
    argument_1[0x10] = (int)pointer_15;
    pointer = pointer_15;
    if ((value_13 < 0xf) && ((int)value_17 < 6)) {
        if ((int)__aeabi_d2iz_result < 1) {
            if (__aeabi_d2iz_result == 0) {
                __aeabi_ui2d_result = (double)pico_join_u32_u32(argument_4, argument_3);
                __aeabi_dcmpeq_result = 2;
            } else {
                __aeabi_ui2d_result = __aeabi_dmul(
                    (double)pico_join_u32_u32(argument_4, argument_3),
                    *(double *)(&g_libc_unknown_09 + (-__aeabi_d2iz_result & 0xf) * 8));
                __aeabi_dcmplt_result_2 = (int)-__aeabi_d2iz_result >> 4;
                if (__aeabi_dcmplt_result_2 == 0) {
                    __aeabi_dcmpeq_result = 2;
                } else {
                    pointer_19 = (double *)&g_libc_unknown_10;
                    __aeabi_dcmpeq_result = 2;
                    do {
                        if (__aeabi_dcmplt_result_2 << 0x1f < 0) {
                            __aeabi_dcmpeq_result = __aeabi_dcmpeq_result + 1;
                            __aeabi_ui2d_result = __aeabi_dmul(__aeabi_ui2d_result, *pointer_19);
                        }
                        __aeabi_dcmplt_result_2 = __aeabi_dcmplt_result_2 >> 1;
                        pointer_19 = pointer_19 + 1;
                    } while (__aeabi_dcmplt_result_2 != 0);
                }
            }
        } else {
            value_17 = (int)__aeabi_d2iz_result >> 4;
            __aeabi_i2d_result_2 =
                *(double *)(&g_libc_unknown_09 + (__aeabi_d2iz_result & 0xf) * 8);
            __aeabi_ui2d_result = *(double *)(&g_libc_unknown_09 + (__aeabi_d2iz_result & 0xf) * 8);
            if ((int)(value_17 << 0x1b) < 0) {
                __aeabi_ddiv_result =
                    __aeabi_ddiv((double)pico_join_u32_u32(argument_4, argument_3), 1e+256);
                value_17 = value_17 & 0xf;
                __aeabi_dcmpeq_result = 3;
            } else {
                __aeabi_dcmpeq_result = 2;
                __aeabi_ddiv_result = (double)pico_join_u32_u32(argument_4, argument_3);
            }
            if (value_17 != 0) {
                pointer_19 = (double *)&g_libc_unknown_10;
                do {
                    if ((int)(value_17 << 0x1f) < 0) {
                        __aeabi_dcmpeq_result = __aeabi_dcmpeq_result + 1;
                        __aeabi_ui2d_result = __aeabi_dmul(__aeabi_ui2d_result, *pointer_19);
                    }
                    value_17 = (int)value_17 >> 1;
                    pointer_19 = pointer_19 + 1;
                    __aeabi_i2d_result_2 = __aeabi_ui2d_result;
                } while (value_17 != 0);
            }
            __aeabi_ui2d_result = __aeabi_ddiv(__aeabi_ddiv_result, __aeabi_i2d_result_2);
        }
        if ((!condition_2) || (__aeabi_dcmplt_result_2 = __aeabi_dcmplt(__aeabi_ui2d_result, 1.0),
                               __aeabi_dcmplt_result_2 == 0)) {
            __aeabi_i2d_result_2 = __aeabi_i2d(__aeabi_dcmpeq_result);
            __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_i2d_result_2, __aeabi_ui2d_result);
            __aeabi_i2d_result_2 = __aeabi_dadd(__aeabi_i2d_result_2, 7.0);
            value_8 = PICO_SUB_U32(__aeabi_i2d_result_2, 0);
            __aeabi_dcmpeq_result = (int)((uint64_t)__aeabi_i2d_result_2 >> 0x20) + -0x3400000;
            if (value_13 == 0)
                goto branch_5;
            value_34 = __aeabi_d2iz_result;
            value_17 = value_13;
        branch_6:
            if (condition_1) {
                __aeabi_i2d_result_2 =
                    __aeabi_ddiv(0.5, *(double *)(&g_c_library_unknown_11 + value_17 * 8));
                __aeabi_i2d_result_2 =
                    __aeabi_dsub(__aeabi_i2d_result_2,
                                 (double)pico_join_u32_u32(__aeabi_dcmpeq_result, value_8));
                __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_ui2d_result);
                __aeabi_i2d_result = __aeabi_i2d(__aeabi_dcmpeq_result);
                __aeabi_ui2d_result = __aeabi_dsub(__aeabi_ui2d_result, __aeabi_i2d_result);
                value_11 = __aeabi_dcmpeq_result + 0x30;
                *(char *)pointer_15 = (char)value_11;
                pointer_20 = (int *)((int)pointer_15 + 1);
                __aeabi_dcmpeq_result = __aeabi_dcmpgt(__aeabi_i2d_result_2, __aeabi_ui2d_result);
                if (__aeabi_dcmpeq_result != 0)
                    goto branch_7;
                __aeabi_i2d_result = __aeabi_dsub(1.0, __aeabi_ui2d_result);
                __aeabi_dcmpeq_result = __aeabi_dcmpgt(__aeabi_i2d_result_2, __aeabi_i2d_result);
                if (__aeabi_dcmpeq_result == 0) {
                    if (1 < (int)value_17) {
                        pointer_14 = pointer_20;
                        do {
                            __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_i2d_result_2, 10.0);
                            __aeabi_ui2d_result = __aeabi_dmul(__aeabi_ui2d_result, 10.0);
                            __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_ui2d_result);
                            __aeabi_i2d_result = __aeabi_i2d(__aeabi_dcmpeq_result);
                            __aeabi_ui2d_result =
                                __aeabi_dsub(__aeabi_ui2d_result, __aeabi_i2d_result);
                            value_11 = __aeabi_dcmpeq_result + 0x30;
                            pointer_20 = (int *)((int)pointer_14 + 1);
                            *(char *)pointer_14 = (char)value_11;
                            __aeabi_dcmpeq_result =
                                __aeabi_dcmplt(__aeabi_ui2d_result, __aeabi_i2d_result_2);
                            if (__aeabi_dcmpeq_result != 0) {
                                __aeabi_d2iz_result = value_34;
                                goto branch_8;
                            }
                            __aeabi_i2d_result = __aeabi_dsub(1.0, __aeabi_ui2d_result);
                            __aeabi_dcmpeq_result =
                                __aeabi_dcmplt(__aeabi_i2d_result, __aeabi_i2d_result_2);
                            if (__aeabi_dcmpeq_result != 0)
                                goto branch_9;
                            pointer_14 = pointer_20;
                        } while (pointer_20 != (int *)(value_17 + (int)pointer_15));
                    }
                    goto branch_1;
                }
            branch_9:
                value_11 = value_11 & 0xff;
                __aeabi_d2iz_result = value_34;
            } else {
                __aeabi_i2d_result_2 =
                    __aeabi_dmul(*(double *)(&g_libc_unknown_09 + (value_17 - 1) * 8),
                                 (double)pico_join_u32_u32(__aeabi_dcmpeq_result, value_8));
                __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_ui2d_result);
                __aeabi_i2d_result = __aeabi_i2d(__aeabi_dcmpeq_result);
                __aeabi_ui2d_result = __aeabi_dsub(__aeabi_ui2d_result, __aeabi_i2d_result);
                *(char *)pointer_15 = (char)__aeabi_dcmpeq_result + '0';
                pointer_20 = (int *)((int)pointer_15 + 1);
                if (value_17 != 1) {
                    pointer_14 = pointer_15;
                    do {
                        __aeabi_ui2d_result = __aeabi_dmul(__aeabi_ui2d_result, 10.0);
                        __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_ui2d_result);
                        __aeabi_i2d_result = __aeabi_i2d(__aeabi_dcmpeq_result);
                        __aeabi_ui2d_result = __aeabi_dsub(__aeabi_ui2d_result, __aeabi_i2d_result);
                        pointer_14 = (int *)((int)pointer_14 + 1);
                        *(char *)pointer_14 = (char)__aeabi_dcmpeq_result + '0';
                    } while (pointer_14 != (int *)((int)pointer_15 + (value_17 - 1)));
                    pointer_20 = (int *)((int)pointer_20 + (value_17 - 1));
                }
                __aeabi_i2d_result = __aeabi_dadd(__aeabi_i2d_result_2, 0.5);
                __aeabi_dcmpeq_result = __aeabi_dcmplt(__aeabi_i2d_result, __aeabi_ui2d_result);
                if (__aeabi_dcmpeq_result == 0) {
                    __aeabi_i2d_result_2 = __aeabi_dsub(0.5, __aeabi_i2d_result_2);
                    __aeabi_dcmpeq_result =
                        __aeabi_dcmpgt(__aeabi_i2d_result_2, __aeabi_ui2d_result);
                    if (__aeabi_dcmpeq_result == 0)
                        goto branch_1;
                    pointer_15 = pointer_20;
                    do {
                        pointer_20 = pointer_15;
                        pointer_15 = (int *)((int)pointer_20 + -1);
                    } while (*(char *)((int)pointer_20 + -1) == '0');
                branch_7:
                    __aeabi_d2iz_result = value_34;
                    goto branch_8;
                }
                __aeabi_d2iz_result = value_34;
                value_11 = (uint32_t) * (uint8_t *)((int)pointer_20 + -1);
            }
        branch_10:
            while (pointer_14 = (int *)((int)pointer_20 + -1), value_11 == 0x39) {
                if (pointer_15 == pointer_14) {
                    *(char *)pointer = '0';
                    __aeabi_d2iz_result = __aeabi_d2iz_result + 1;
                    *(char *)pointer_14 = '1';
                    goto branch_8;
                }
                value_11 = (uint32_t) * (uint8_t *)((int)pointer_20 + -2);
                pointer_20 = pointer_14;
            }
            *(char *)pointer_14 = (char)value_11 + '\x01';
            goto branch_8;
        }
        if (value_13 != 0) {
            if (0 < (int)value_54) {
                __aeabi_ui2d_result = __aeabi_dmul(__aeabi_ui2d_result, 10.0);
                __aeabi_i2d_result_2 = __aeabi_i2d(__aeabi_dcmpeq_result + 1);
                __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_ui2d_result, __aeabi_i2d_result_2);
                __aeabi_i2d_result_2 = __aeabi_dadd(__aeabi_i2d_result_2, 7.0);
                value_8 = PICO_SUB_U32(__aeabi_i2d_result_2, 0);
                value_34 = __aeabi_d2iz_result - 1;
                __aeabi_dcmpeq_result = (int)((uint64_t)__aeabi_i2d_result_2 >> 0x20) + -0x3400000;
                value_17 = value_54;
                goto branch_6;
            }
            goto branch_1;
        }
        __aeabi_i2d_result_2 = __aeabi_i2d(__aeabi_dcmpeq_result);
        __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_ui2d_result, __aeabi_i2d_result_2);
        __aeabi_i2d_result_2 = __aeabi_dadd(__aeabi_i2d_result_2, 7.0);
        value_8 = PICO_SUB_U32(__aeabi_i2d_result_2, 0);
        __aeabi_dcmpeq_result = (int)((uint64_t)__aeabi_i2d_result_2 >> 0x20) + -0x3400000;
    branch_5:
        __aeabi_ui2d_result = __aeabi_dsub(__aeabi_ui2d_result, 5.0);
        __aeabi_dcmplt_result_2 = __aeabi_dcmpgt(
            __aeabi_ui2d_result, (double)pico_join_u32_u32(__aeabi_dcmpeq_result, value_8));
        if (__aeabi_dcmplt_result_2 == 0) {
            __aeabi_dcmpeq_result = __aeabi_dcmplt(
                __aeabi_ui2d_result,
                (double)pico_join_u32_u32(__aeabi_dcmpeq_result + -0x80000000, value_8));
            if (__aeabi_dcmpeq_result == 0)
                goto branch_1;
            goto branch_11;
        }
        pointer_7 = (uint32_t *)0x0;
        pointer_9 = (uint32_t *)0x0;
    branch_12:
        __aeabi_d2iz_result = __aeabi_d2iz_result + 1;
        *(char *)pointer = '1';
        pointer_20 = (int *)((int)pointer + 1);
        value_00 = pointer_9;
    branch_13:
        unresolved_c_library_18((int)argument_1, pointer_7);
        if (value_00 == (uint32_t *)0x0)
            goto branch_8;
    } else {
    branch_1:
        if ((-1 < buffer[0]) && ((int)__aeabi_d2iz_result < 0xf)) {
            __aeabi_ui2d_result = *(double *)(&g_libc_unknown_09 + __aeabi_d2iz_result * 8);
            if ((-1 < (int)argument_6) || (0 < (int)value_13)) {
                __aeabi_i2d_result_2 = __aeabi_ddiv(
                    (double)pico_join_u32_u32(argument_4, argument_3), __aeabi_ui2d_result);
                __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_i2d_result_2);
                __aeabi_i2d_result_2 = __aeabi_i2d(__aeabi_dcmpeq_result);
                __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_i2d_result_2, __aeabi_ui2d_result);
                __aeabi_i2d_result_2 = __aeabi_dsub(
                    (double)pico_join_u32_u32(argument_4, argument_3), __aeabi_i2d_result_2);
                *(char *)pointer = (char)__aeabi_dcmpeq_result + '0';
                pointer_20 = (int *)((int)pointer + 1);
                if (value_13 != 1) {
                    __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_i2d_result_2, 10.0);
                    __aeabi_dcmpeq_result = __aeabi_dcmpeq(__aeabi_i2d_result_2, 0.0);
                    if (__aeabi_dcmpeq_result == 0) {
                        pointer_15 = (int *)((int)pointer + 2);
                        do {
                            pointer_20 = pointer_15;
                            __aeabi_i2d_result =
                                __aeabi_ddiv(__aeabi_i2d_result_2, __aeabi_ui2d_result);
                            __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_i2d_result);
                            __aeabi_i2d_result = __aeabi_i2d(__aeabi_dcmpeq_result);
                            __aeabi_i2d_result =
                                __aeabi_dmul(__aeabi_i2d_result, __aeabi_ui2d_result);
                            __aeabi_i2d_result_2 =
                                __aeabi_dsub(__aeabi_i2d_result_2, __aeabi_i2d_result);
                            *(char *)((int)pointer_20 + -1) = (char)__aeabi_dcmpeq_result + '0';
                            if (pointer_20 == (int *)((int)pointer + value_13))
                                goto branch_14;
                            __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_i2d_result_2, 10.0);
                            __aeabi_dcmpeq_result = __aeabi_dcmpeq(__aeabi_i2d_result_2, 0.0);
                            pointer_15 = (int *)((int)pointer_20 + 1);
                        } while (__aeabi_dcmpeq_result == 0);
                    }
                    goto branch_8;
                }
            branch_14:
                __aeabi_i2d_result_2 = __aeabi_dadd(__aeabi_i2d_result_2, __aeabi_i2d_result_2);
                __aeabi_dcmplt_result_2 = __aeabi_dcmplt(__aeabi_ui2d_result, __aeabi_i2d_result_2);
                if (__aeabi_dcmplt_result_2 == 0) {
                    __aeabi_dcmplt_result_2 =
                        __aeabi_dcmpeq(__aeabi_ui2d_result, __aeabi_i2d_result_2);
                    if ((__aeabi_dcmplt_result_2 == 0) || (-1 < __aeabi_dcmpeq_result << 0x1f))
                        goto branch_8;
                }
                value_11 = (uint32_t) * (uint8_t *)((int)pointer_20 + -1);
                pointer_15 = pointer;
                goto branch_10;
            }
            if (value_13 == 0) {
                __aeabi_ui2d_result = __aeabi_dmul(__aeabi_ui2d_result, 5.0);
                __aeabi_dcmpeq_result = __aeabi_dcmpge(
                    __aeabi_ui2d_result, (double)pico_join_u32_u32(argument_4, argument_3));
                value_00 = (uint32_t *)0x0;
                pointer_7 = (uint32_t *)0x0;
                pointer_9 = (uint32_t *)0x0;
                value = (uint32_t *)0x0;
                if (__aeabi_dcmpeq_result == 0)
                    goto branch_12;
            } else {
            branch_11:
                value_00 = (uint32_t *)0x0;
                value = (uint32_t *)0x0;
            }
        branch_15:
            __aeabi_d2iz_result = ~argument_6;
            pointer_7 = value;
            pointer_20 = pointer;
            goto branch_13;
        }
        value_17 = value_70;
        value_11 = value_60;
        if (condition_1) {
            if ((int)argument_5 < 2) {
                if (condition_3) {
                    value_18 = buffer[0] + 0x433;
                } else {
                    value_18 = 0x36 - value_30;
                }
            } else {
                value_18 = value_13 - 1;
                if ((int)value_60 < (int)value_18) {
                    value_2 = value_2 + (value_18 - value_60);
                    value_11 = 0;
                    value_60 = value_18;
                } else {
                    value_11 = value_60 - value_18;
                }
                if ((int)value_13 < 0) {
                    value_18 = 0;
                    value_17 = value_70 - value_13;
                } else {
                    value_18 = value_13;
                }
            }
            value_70 = value_70 + value_18;
            value_78 = value_78 + value_18;
            value_00 = (uint32_t *)unresolved_c_library_24(argument_1, 1);
        } else {
            value_00 = (uint32_t *)0x0;
        }
        if ((value_17 != 0) && (0 < (int)value_78)) {
            value_18 = value_78;
            if ((int)value_17 <= (int)value_78) {
                value_18 = value_17;
            }
            value_70 = value_70 - value_18;
            value_78 = value_78 - value_18;
            value_17 = value_17 - value_18;
        }
        if (0 < (int)value_60) {
            if (condition_1) {
                if (0 < (int)value_11) {
                    value_00 =
                        (uint32_t *)unresolved_c_library_23(argument_1, (int *)value_00, value_11);
                    pointer_7 = (uint32_t *)unresolved_c_library_22(argument_1, (int)value_00,
                                                                    (int)pointer_68);
                    unresolved_c_library_18((int)argument_1, pointer_68);
                    pointer_68 = pointer_7;
                }
                if (value_60 - value_11 != 0) {
                    pointer_68 = (uint32_t *)unresolved_c_library_23(argument_1, (int *)pointer_68,
                                                                     value_60 - value_11);
                }
            } else {
                pointer_68 =
                    (uint32_t *)unresolved_c_library_23(argument_1, (int *)pointer_68, value_60);
            }
        }
        value = (uint32_t *)unresolved_c_library_24(argument_1, 1);
        if (0 < (int)value_2) {
            value = (uint32_t *)unresolved_c_library_23(argument_1, (int *)value, value_2);
        }
        if (((int)argument_5 < 2) && (argument_3 == 0)) {
            if ((argument_4 & 0xfffff) == 0) {
                condition_3 = false;
                if ((argument_4 & 0x7ff00000) != 0) {
                    value_70 = value_70 + 1;
                    value_78 = value_78 + 1;
                    condition_3 = true;
                }
            } else {
                condition_3 = false;
            }
        } else {
            condition_3 = false;
        }
        if (value_2 == 0) {
            __aeabi_dcmpeq_result = 1;
        } else {
            __aeabi_dcmpeq_result = unresolved_c_library_20(value[value[4] + 4]);
            __aeabi_dcmpeq_result = 0x20 - __aeabi_dcmpeq_result;
        }
        value_11 = value_78 + __aeabi_dcmpeq_result & 0x1f;
        if (value_11 == 0) {
            __aeabi_dcmpeq_result = 0x1c;
        branch_16:
            value_70 = value_70 + __aeabi_dcmpeq_result;
            value_78 = value_78 + __aeabi_dcmpeq_result;
            value_17 = value_17 + __aeabi_dcmpeq_result;
        } else if ((int)(0x20 - value_11) < 5) {
            if (0x20 - value_11 != 4) {
                __aeabi_dcmpeq_result = 0x3c - value_11;
                goto branch_16;
            }
        } else {
            __aeabi_dcmpeq_result = 0x1c - value_11;
            value_70 = value_70 + __aeabi_dcmpeq_result;
            value_78 = value_78 + __aeabi_dcmpeq_result;
            value_17 = value_17 + __aeabi_dcmpeq_result;
        }
        if (0 < (int)value_70) {
            pointer_68 = (uint32_t *)unresolved_c_library_27(argument_1, pointer_68, value_70);
        }
        if (0 < (int)value_78) {
            value = (uint32_t *)unresolved_c_library_27(argument_1, value, value_78);
        }
        if (condition_2) {
            __aeabi_dcmpeq_result = unresolved_c_library_24_2((int)pointer_68, (int)value);
            if (__aeabi_dcmpeq_result < 0) {
                __aeabi_d2iz_result = __aeabi_d2iz_result - 1;
                pointer_68 =
                    (uint32_t *)unresolved_c_library_19(argument_1, (int *)pointer_68, 10, 0);
                value_13 = value_54;
                if (condition_1) {
                    value_00 =
                        (uint32_t *)unresolved_c_library_19(argument_1, (int *)value_00, 10, 0);
                }
            }
        }
        if (((int)value_13 < 1) && (2 < (int)argument_5)) {
            if (value_13 == 0) {
                pointer_7 = (uint32_t *)unresolved_c_library_19(argument_1, (int *)value, 5, 0);
                __aeabi_dcmpeq_result = unresolved_c_library_24_2((int)pointer_68, (int)pointer_7);
                pointer_9 = value_00;
                value = pointer_7;
                if (0 < __aeabi_dcmpeq_result)
                    goto branch_12;
            }
            goto branch_15;
        }
        if (condition_1) {
            pointer_7 = value_00;
            if (0 < (int)value_17) {
                pointer_7 = (uint32_t *)unresolved_c_library_27(argument_1, value_00, value_17);
            }
            if (condition_3) {
                pointer_9 = (uint32_t *)unresolved_c_library_17(argument_1, pointer_7[1]);
                libc_memcpy(pointer_9 + 3, pointer_7 + 3, (pointer_7[4] + 2) * 4);
                value_00 = (uint32_t *)unresolved_c_library_27(argument_1, pointer_9, 1);
            } else {
                value_00 = pointer_7;
            }
            pointer_15 = pointer;
        branch_17:
            pointer_20 = (int *)((int)pointer_15 + 1);
            value_11 = unresolved_c_library_09((int)pointer_68, (int)value);
            __aeabi_dcmpeq_result = unresolved_c_library_24_2((int)pointer_68, (int)pointer_7);
            pointer_14 = unresolved_c_library_29(argument_1, (int)value, (int)value_00);
            value_17 = value_11 + 0x30;
            if (pointer_14[3] == 0) {
                __aeabi_dcmplt_result_2 =
                    unresolved_c_library_24_2((int)pointer_68, (int)pointer_14);
                unresolved_c_library_18((int)argument_1, pointer_14);
                if (((__aeabi_dcmplt_result_2 != 0) || (argument_5 != 0)) ||
                    ((argument_3 & 1) != 0))
                    goto branch_18;
                if (value_17 != 0x39) {
                    if (0 < __aeabi_dcmpeq_result) {
                        value_17 = value_11 + 0x31;
                    }
                    *(char *)pointer_15 = (char)value_17;
                    pointer_20 = (int *)((int)pointer_15 + 1);
                    goto branch_19;
                }
            branch_20:
                byte_value_12 = '9';
                *(char *)pointer_15 = '9';
                pointer_20 = (int *)((int)pointer_15 + 1);
                goto branch_21;
            }
            unresolved_c_library_18((int)argument_1, pointer_14);
            __aeabi_dcmplt_result_2 = 1;
        branch_18:
            if ((__aeabi_dcmpeq_result < 0) ||
                (((__aeabi_dcmpeq_result == 0 && (argument_5 == 0)) && ((argument_3 & 1) == 0)))) {
                if (0 < __aeabi_dcmplt_result_2) {
                    pointer_68 = (uint32_t *)unresolved_c_library_27(argument_1, pointer_68, 1);
                    __aeabi_dcmpeq_result = unresolved_c_library_24_2((int)pointer_68, (int)value);
                    if ((0 < __aeabi_dcmpeq_result) ||
                        ((__aeabi_dcmpeq_result == 0 && ((value_17 & 1) != 0)))) {
                        if (value_17 == 0x39)
                            goto branch_20;
                        value_17 = value_11 + 0x31;
                    }
                }
                *(char *)pointer_15 = (char)value_17;
                pointer_20 = (int *)((int)pointer_15 + 1);
                goto branch_19;
            }
            if (0 < __aeabi_dcmplt_result_2) {
                if (value_17 == 0x39)
                    goto branch_20;
                *(char *)pointer_15 = (char)value_11 + '1';
                pointer_20 = (int *)((int)pointer_15 + 1);
                goto branch_19;
            }
            *(char *)pointer_15 = (char)value_17;
            if (pointer_20 != (int *)((int)pointer + value_13)) {
                pointer_68 =
                    (uint32_t *)unresolved_c_library_19(argument_1, (int *)pointer_68, 10, 0);
                pointer_15 = pointer_20;
                if (pointer_7 == value_00) {
                    pointer_7 =
                        (uint32_t *)unresolved_c_library_19(argument_1, (int *)pointer_7, 10, 0);
                    value_00 = pointer_7;
                } else {
                    pointer_7 =
                        (uint32_t *)unresolved_c_library_19(argument_1, (int *)pointer_7, 10, 0);
                    value_00 =
                        (uint32_t *)unresolved_c_library_19(argument_1, (int *)value_00, 10, 0);
                }
                goto branch_17;
            }
        } else {
            __aeabi_dcmpeq_result = 0;
            while (true) {
                value_17 = unresolved_c_library_09((int)pointer_68, (int)value);
                value_17 = value_17 + 0x30;
                *(char *)((int)pointer + __aeabi_dcmpeq_result) = (char)value_17;
                __aeabi_dcmpeq_result = __aeabi_dcmpeq_result + 1;
                if ((int)value_13 <= __aeabi_dcmpeq_result)
                    break;
                pointer_68 =
                    (uint32_t *)unresolved_c_library_19(argument_1, (int *)pointer_68, 10, 0);
            }
            if ((int)value_13 < 1) {
                pointer_20 = (int *)((int)pointer + 1);
            } else {
                pointer_20 = (int *)((int)pointer + value_13);
            }
            pointer_7 = (uint32_t *)0x0;
        }
        pointer_68 = (uint32_t *)unresolved_c_library_27(argument_1, pointer_68, 1);
        __aeabi_dcmpeq_result = unresolved_c_library_24_2((int)pointer_68, (int)value);
        if ((0 < __aeabi_dcmpeq_result) ||
            ((__aeabi_dcmpeq_result == 0 && ((value_17 & 1) != 0)))) {
            byte_value_12 = *(char *)((int)pointer_20 + -1);
        branch_21:
            do {
                pointer_15 = (int *)((int)pointer_20 + -1);
                if (byte_value_12 != '9')
                    goto code_r0x2001ee52;
                if (pointer == pointer_15) {
                    __aeabi_d2iz_result = __aeabi_d2iz_result + 1;
                    *(char *)pointer = '1';
                    goto branch_19;
                }
                byte_value_12 = *(char *)((int)pointer_20 + -2);
                pointer_20 = pointer_15;
            } while (true);
        }
        pointer_15 = pointer_20;
        do {
            pointer_20 = pointer_15;
            pointer_15 = (int *)((int)pointer_20 + -1);
        } while (*(char *)((int)pointer_20 + -1) == '0');
    branch_19:
        unresolved_c_library_18((int)argument_1, value);
        if (value_00 == (uint32_t *)0x0)
            goto branch_8;
        if ((pointer_7 != (uint32_t *)0x0) && (pointer_7 != value_00)) {
            unresolved_c_library_18((int)argument_1, pointer_7);
        }
    }
    unresolved_c_library_18((int)argument_1, value_00);
branch_8:
    unresolved_c_library_18((int)argument_1, pointer_68);
    *(char *)pointer_20 = '\0';
    *argument_7 = __aeabi_d2iz_result + 1;
    if (argument_9 == (uint32_t *)0x0) {
        return pointer;
    }
    *argument_9 = pointer_20;
    return pointer;
code_r0x2001ee52:
    *(char *)pointer_15 = byte_value_12 + '\x01';
    goto branch_19;
}

/* Purpose incomplete.
 * Firmware address: 0x2001f720. */
uint32_t unresolved_c_library_11(int *argument_1, int argument_2)
{
    void *pointer_1;
    uint32_t value_2;
    int value_3;

    unresolved_c_library_04();
    value_2 = *(uint32_t *)(g_c_library_unknown_17_as_int + 4) & 0xfffffffc;
    value_3 = ((value_2 - argument_2) + 0xfef & 0xfffff000) - 0x1000;
    if ((0xfff < value_3) &&
        (pointer_1 = PICO_UNRESOLVED_RETURN(unresolved_c_library_07(argument_1, 0)),
         pointer_1 == (void *)(g_c_library_unknown_17_as_int + value_2))) {
        pointer_1 = PICO_UNRESOLVED_RETURN(unresolved_c_library_07(argument_1, -value_3));
        if (pointer_1 != (void *)0xffffffff) {
            g_c_library_unknown_27_as_int = g_c_library_unknown_27_as_int - value_3;
            *(uint32_t *)(g_c_library_unknown_17_as_int + 4) = value_2 - value_3 | 1;
            unresolved_c_library_05();
            return 1;
        }
        pointer_1 = PICO_UNRESOLVED_RETURN(unresolved_c_library_07(argument_1, 0));
        if (0xf < (int)pointer_1 - g_c_library_unknown_17_as_int) {
            g_c_library_unknown_27_as_int = (int)pointer_1 - g_c_library_unknown_22_as_int;
            *(uint32_t *)(g_c_library_unknown_17_as_int + 4) =
                (int)pointer_1 - g_c_library_unknown_17_as_int | 1;
        }
    }
    unresolved_c_library_05();
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x2001f7bc. */
void unresolved_c_library_12(int *argument_1, int argument_2)
{
    uint32_t *pointer_1;
    uint32_t value_2;
    int value_3;
    int value_4;
    uint8_t *pointer_5;
    uint32_t value_6;
    uint8_t *pointer_7;
    int value_8;
    uint32_t value_9;
    bool condition_10;

    if (argument_2 == 0) {
        return;
    }
    unresolved_c_library_04();
    value_2 = *(uint32_t *)(argument_2 + -4);
    value_6 = value_2 & 0xfffffffe;
    value_8 = argument_2 + -8;
    value_4 = value_8 + value_6;
    value_9 = *(uint32_t *)(value_4 + 4) & 0xfffffffc;
    if (g_c_library_unknown_17 == value_4) {
        value_9 = value_9 + value_6;
        if (-1 < (int)(value_2 << 0x1f)) {
            value_8 = value_8 - *(int *)(argument_2 + -8);
            value_9 = value_9 + *(int *)(argument_2 + -8);
            value_4 = *(int *)(value_8 + 8);
            value_3 = *(int *)(value_8 + 0xc);
            *(int *)(value_4 + 0xc) = value_3;
            *(int *)(value_3 + 8) = value_4;
        }
        condition_10 = g_c_library_unknown_21 <= value_9;
        *(uint32_t *)(value_8 + 4) = value_9 | 1;
        g_c_library_unknown_17 = value_8;
        if (condition_10) {
            unresolved_c_library_11(argument_1, g_c_library_unknown_26);
        }
    } else {
        *(uint32_t *)(value_4 + 4) = value_9;
        if ((value_2 & 1) == 0) {
            value_8 = value_8 - *(int *)(argument_2 + -8);
            pointer_1 = *(uint32_t **)(value_8 + 8);
            value_6 = value_6 + *(int *)(argument_2 + -8);
            if (pointer_1 == &g_c_library_unknown_17) {
                condition_10 = true;
            } else {
                value_3 = *(int *)(value_8 + 0xc);
                pointer_1[3] = value_3;
                *(uint32_t **)(value_3 + 8) = pointer_1;
                condition_10 = false;
            }
        } else {
            condition_10 = false;
        }
        if (-1 < *(int *)(value_4 + value_9 + 4) << 0x1f) {
            value_6 = value_6 + value_9;
            pointer_1 = *(uint32_t **)(value_4 + 8);
            if ((!condition_10) && (pointer_1 == &g_c_library_unknown_17)) {
                g_c_library_unknown_19_as_int = value_8;
                g_c_library_unknown_20_as_int = value_8;
                *(int **)(value_8 + 0xc) = &g_c_library_unknown_17;
                *(int **)(value_8 + 8) = &g_c_library_unknown_17;
                *(uint32_t *)(value_8 + 4) = value_6 | 1;
                *(uint32_t *)(value_8 + value_6) = value_6;
                goto branch_1;
            }
            value_4 = *(int *)(value_4 + 0xc);
            pointer_1[3] = value_4;
            *(uint32_t **)(value_4 + 8) = pointer_1;
        }
        *(uint32_t *)(value_8 + 4) = value_6 | 1;
        *(uint32_t *)(value_8 + value_6) = value_6;
        if (!condition_10) {
            if (value_6 < 0x200) {
                value_6 = value_6 >> 3;
                value_4 = (&g_c_library_unknown_17)[value_6 * 2];
                *(int *)(value_8 + 8) = value_4;
                g_c_library_unknown_16 = g_c_library_unknown_16 | 1 << ((int)value_6 >> 2 & 0xffU);
                *(uint8_t **)(value_8 + 0xc) = &g_c_library_unknown_15 + value_6 * 8;
                (&g_c_library_unknown_17)[value_6 * 2] = value_8;
                *(int *)(value_4 + 0xc) = value_8;
                unresolved_c_library_05();
                return;
            }
            value_2 = value_6 >> 9;
            if (value_2 < 5) {
                value_4 = (value_6 >> 6) + 0x38;
                value_3 = value_4 * 2;
            } else if (value_2 < 0x15) {
                value_4 = value_2 + 0x5b;
                value_3 = value_4 * 2;
            } else if (value_2 < 0x55) {
                value_4 = (value_6 >> 0xc) + 0x6e;
                value_3 = value_4 * 2;
            } else if (value_2 < 0x155) {
                value_4 = (value_6 >> 0xf) + 0x77;
                value_3 = value_4 * 2;
            } else if (value_2 < 0x555) {
                value_4 = (value_6 >> 0x12) + 0x7c;
                value_3 = value_4 * 2;
            } else {
                value_3 = 0xfc;
                value_4 = 0x7e;
            }
            pointer_5 = (uint8_t *)(&g_c_library_unknown_17)[value_3];
            if (pointer_5 == &g_c_library_unknown_15 + value_3 * 4) {
                g_c_library_unknown_16 = g_c_library_unknown_16 | 1 << (value_4 >> 2 & 0xffU);
                pointer_7 = pointer_5;
            } else {
                do {
                    if ((*(uint32_t *)(pointer_5 + 4) & 0xfffffffc) <= value_6)
                        break;
                    pointer_5 = *(uint8_t **)(pointer_5 + 8);
                } while (&g_c_library_unknown_15 + value_3 * 4 != pointer_5);
                pointer_7 = *(uint8_t **)(pointer_5 + 0xc);
            }
            *(uint8_t **)(value_8 + 0xc) = pointer_7;
            *(uint8_t **)(value_8 + 8) = pointer_5;
            *(int *)(pointer_7 + 8) = value_8;
            *(int *)(pointer_5 + 0xc) = value_8;
        }
    }
branch_1:
    unresolved_c_library_05();
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001f958. */
void unresolved_c_library_13(int argument_1, uint32_t argument_2)
{
    int value_1;
    uint32_t *pointer_2;
    uint32_t *pointer_3;
    uint32_t *pointer_4;
    uint32_t value_5;
    uint32_t *pointer_6;
    uint32_t *pointer_7;
    uint32_t *pointer_8;
    uint32_t value_9;

    value_1 = (int)argument_2 >> 5;
    pointer_8 = (uint32_t *)(argument_1 + 0x14);
    if (value_1 < *(int *)(argument_1 + 0x10)) {
        value_9 = argument_2 & 0x1f;
        pointer_3 = pointer_8 + *(int *)(argument_1 + 0x10);
        pointer_7 = pointer_8 + value_1;
        if (value_9 == 0) {
            if (pointer_3 <= pointer_7)
                goto branch_1;
            pointer_4 = pointer_7;
            do {
                pointer_6 = pointer_4 + 1;
                *pointer_8 = *pointer_4;
                pointer_8 = pointer_8 + 1;
                pointer_4 = pointer_6;
            } while (pointer_6 < pointer_3);
            value_1 = (~(uint32_t)pointer_7 + (int)pointer_3 & 0xfffffffc) + 4;
        } else {
            value_5 = pointer_8[value_1] >> value_9;
            if (pointer_7 + 1 < pointer_3) {
                pointer_4 = pointer_7 + 1;
                pointer_6 = pointer_8;
                do {
                    *pointer_6 = value_5 | *pointer_4 << (0x20 - value_9 & 0xff);
                    pointer_2 = pointer_4 + 1;
                    value_5 = *pointer_4 >> value_9;
                    pointer_4 = pointer_2;
                    pointer_6 = pointer_6 + 1;
                } while (pointer_2 < pointer_3);
                pointer_3 = (uint32_t *)(((int)pointer_3 + (-5 - (int)pointer_7) & 0xfffffffcU) +
                                         4 + (int)pointer_8);
            } else {
                pointer_3 = pointer_8;
            }
            *pointer_3 = value_5;
            if (value_5 != 0) {
                pointer_3 = pointer_3 + 1;
            }
            value_1 = (int)pointer_3 - (int)pointer_8;
        }
        *(int *)(argument_1 + 0x10) = value_1 >> 2;
        if (value_1 >> 2 != 0) {
            return;
        }
    } else {
    branch_1:
        *(uint32_t *)(argument_1 + 0x10) = 0;
    }
    *(uint32_t *)(argument_1 + 0x14) = 0;
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2001f9f8. */
uint32_t unresolved_c_library_14(int *argument_1, int *argument_2, uint32_t *argument_3,
                                 uint32_t *argument_4, uint32_t *argument_5, int argument_6)
{
    uint8_t byte_value_1;
    bool condition_2;
    uint32_t *pointer_3;
    char *text;
    uint32_t text_length;
    int libc_strncmp_result_2;
    uint32_t *pointer_6;
    uint32_t value_7;
    int libc_strncmp_result;
    uint32_t value_9;
    uint8_t *index;
    uint8_t *pointer_11;
    uint32_t *pointer_12;
    uint8_t *left;
    uint32_t value_13;
    int value_14;
    uint8_t *pointer_15;
    uint32_t value_40;
    uint32_t *pointer;
    uint32_t *pointer_30;

    pointer_3 = (uint32_t *)unresolved_c_library_16();
    text = (char *)*pointer_3;
    text_length = libc_strlen(text);
    libc_strncmp_result = *argument_2;
    value_7 = (uint32_t) * (uint8_t *)(libc_strncmp_result + 2);
    byte_value_1 = text[text_length - 1];
    left = (uint8_t *)(libc_strncmp_result + 2);
    if (value_7 == 0x30) {
        value_14 = 0;
        pointer_15 = (uint8_t *)(libc_strncmp_result + 3);
        do {
            left = pointer_15;
            value_7 = (uint32_t)*left;
            value_14 = value_14 + 1;
            pointer_15 = left + 1;
        } while (value_7 == 0x30);
    } else {
        value_14 = 0;
    }
    if ((&g_c_library_unknown_10)[value_7] == '\0') {
        libc_strncmp_result = libc_strncmp((char *)left, text, text_length);
        if (libc_strncmp_result == 0) {
            value_7 = (uint32_t)left[text_length];
            value_9 = (uint32_t)(uint8_t)(&g_c_library_unknown_10)[value_7];
            pointer_15 = left + text_length;
            if (value_9 != 0) {
                left = pointer_15;
                if (value_7 == 0x30) {
                    do {
                        left = left + 1;
                    } while (*left == 0x30);
                    value_9 = (uint32_t)(uint8_t)(&g_c_library_unknown_10)[*left];
                }
                libc_strncmp_result = 1 - value_9;
                if (1 < value_9) {
                    libc_strncmp_result = 0;
                }
                value_14 = 1;
                goto branch_1;
            }
            libc_strncmp_result = 1;
            value_40 = 0;
        } else {
            value_7 = (uint32_t)*left;
            libc_strncmp_result = 1;
            value_40 = 0;
            pointer_15 = left;
        }
    } else {
        libc_strncmp_result = 0;
        value_9 = (uint32_t)(uint8_t)(&g_c_library_unknown_10)[*left];
        pointer_15 = (uint8_t *)0x0;
    branch_1:
        index = left;
        if (value_9 != 0) {
            do {
                index = index + 1;
            } while ((&g_c_library_unknown_10)[*index] != '\0');
        }
        libc_strncmp_result_2 = libc_strncmp((char *)index, text, text_length);
        if (libc_strncmp_result_2 == 0) {
            if (pointer_15 == (uint8_t *)0x0) {
                value_7 = (uint32_t)index[text_length];
                pointer_15 = index + text_length;
                index = pointer_15;
                if ((&g_c_library_unknown_10)[value_7] != '\0') {
                    do {
                        index = index + 1;
                        value_7 = (uint32_t)*index;
                    } while ((&g_c_library_unknown_10)[value_7] != '\0');
                }
            } else {
                value_7 = (uint32_t)*index;
            }
        } else {
            value_7 = (uint32_t)*index;
            if (pointer_15 == (uint8_t *)0x0) {
                value_40 = 0;
                pointer_15 = index;
                goto branch_2;
            }
        }
        value_40 = ((int)pointer_15 - (int)index) * 4;
        pointer_15 = index;
    }
branch_2:
    if ((value_7 == 0x50) || (value_7 == 0x70)) {
        value_7 = (uint32_t)pointer_15[1];
        if (value_7 == 0x2b) {
            condition_2 = false;
        branch_3:
            value_7 = (uint32_t)pointer_15[2];
            index = pointer_15 + 2;
        } else {
            if (value_7 == 0x2d) {
                condition_2 = true;
                goto branch_3;
            }
            index = pointer_15 + 1;
            condition_2 = false;
        }
        if (0x18 < (uint8_t)(&g_c_library_unknown_10)[value_7] - 1)
            goto branch_4;
        value_9 = (uint32_t)(uint8_t)(&g_c_library_unknown_10)[index[1]];
        libc_strncmp_result_2 = (uint8_t)(&g_c_library_unknown_10)[value_7] - 0x10;
        pointer_11 = index + 1;
        if (value_9 - 1 < 0x19) {
            index = index + 2;
            do {
                pointer_11 = index;
                libc_strncmp_result_2 = value_9 + libc_strncmp_result_2 * 10;
                value_9 = (uint32_t)(uint8_t)(&g_c_library_unknown_10)[*pointer_11];
                libc_strncmp_result_2 = libc_strncmp_result_2 + -0x10;
                index = pointer_11 + 1;
            } while (value_9 - 1 < 0x19);
        }
        if (condition_2) {
            libc_strncmp_result_2 = -libc_strncmp_result_2;
        }
        value_40 = value_40 + libc_strncmp_result_2;
    } else {
    branch_4:
        pointer_11 = pointer_15;
    }
    *argument_2 = (int)pointer_11;
    if (libc_strncmp_result != 0) {
        if (value_14 == 0) {
            return 6;
        }
        return 0;
    }
    value_7 = 0;
    for (index = pointer_15 + (-1 - (int)left); 7 < (int)index;
         index = (uint8_t *)((int)index >> 1)) {
        value_7 = value_7 + 1;
    }
    pointer = (uint32_t *)unresolved_c_library_17(argument_1, value_7);
    pointer_30 = pointer + 5;
    pointer_6 = pointer_30;
    if (left < pointer_15) {
        value_7 = 0;
        value_9 = 0;
        do {
            if ((pointer_15[-1] == byte_value_1) &&
                (index = pointer_15 + -1 + (1 - text_length), left <= index)) {
                libc_strncmp_result = libc_strncmp((char *)index, text, text_length);
                if (libc_strncmp_result != 0)
                    goto branch_5;
            } else {
            branch_5:
                if (value_9 == 0x20) {
                    *pointer_6 = value_7;
                    value_7 = 0;
                    pointer_6 = pointer_6 + 1;
                    value_9 = 0;
                    value_13 = 4;
                } else {
                    value_13 = value_9 + 4;
                }
                value_7 = value_7 | ((uint8_t)(&g_c_library_unknown_10)[pointer_15[-1]] & 0xf)
                                        << (value_9 & 0xff);
                index = pointer_15 + -1;
                value_9 = value_13;
            }
            pointer_15 = index;
        } while (left < pointer_15);
    } else {
        value_7 = 0;
    }
    *pointer_6 = value_7;
    text_length = (int)pointer_6 + (4 - (int)pointer_30) >> 2;
    pointer[4] = text_length;
    libc_strncmp_result = unresolved_c_library_20(value_7);
    value_7 = *argument_3;
    libc_strncmp_result = text_length * 0x20 - libc_strncmp_result;
    if ((int)value_7 < libc_strncmp_result) {
        text_length = libc_strncmp_result - value_7;
        libc_strncmp_result = unresolved_floating_point_06((int)pointer, text_length);
        if (libc_strncmp_result == 0) {
            value_9 = 0;
        } else {
            value_13 = text_length - 1;
            value_9 = 1;
            if ((1 << (value_13 & 0x1f) & pointer_30[(int)value_13 >> 5]) != 0) {
                if (((int)value_13 < 2) || (libc_strncmp_result = unresolved_floating_point_06(
                                                (int)pointer, text_length - 2),
                                            libc_strncmp_result == 0)) {
                    value_9 = 2;
                } else {
                    value_9 = 3;
                }
            }
        }
        value_40 = value_40 + text_length;
        unresolved_c_library_13((int)pointer, text_length);
    } else if (libc_strncmp_result < (int)value_7) {
        pointer =
            (uint32_t *)unresolved_c_library_27(argument_1, pointer, value_7 - libc_strncmp_result);
        value_40 = value_40 - (value_7 - libc_strncmp_result);
        pointer_30 = pointer + 5;
        value_9 = 0;
    } else {
        value_9 = 0;
    }
    if ((int)argument_3[2] < (int)value_40)
        goto branch_6;
    text_length = argument_3[1];
    if ((int)value_40 < (int)text_length) {
        value_13 = text_length - value_40;
        if ((int)value_7 <= (int)value_13) {
            value_9 = argument_3[3];
            if (value_9 == 2) {
                if (argument_6 == 0)
                    goto branch_7;
            } else if (value_9 == 3) {
                if (argument_6 != 0)
                    goto branch_7;
            } else if ((value_9 == 1) && (value_13 == value_7)) {
                if (1 < (int)value_7) {
                    libc_strncmp_result = unresolved_floating_point_06((int)pointer, value_7 - 1);
                    if (libc_strncmp_result == 0)
                        goto branch_8;
                    text_length = argument_3[1];
                }
            branch_7:
                *argument_4 = text_length;
                pointer[4] = 1;
                *pointer_30 = 1;
                *argument_5 = pointer;
                return 0x62;
            }
        branch_8:
            unresolved_c_library_18((int)argument_1, pointer);
            *argument_5 = 0;
            return 0x50;
        }
        text_length = value_13 - 1;
        if (value_9 == 0) {
            if (0 < (int)text_length) {
                value_9 = unresolved_floating_point_06((int)pointer, text_length);
            }
        } else {
            value_9 = 1;
        }
        if ((1 << (text_length & 0x1f) & pointer_30[(int)text_length >> 5]) != 0) {
            value_9 = value_9 | 2;
        }
        unresolved_c_library_13((int)pointer, value_13);
        value_40 = argument_3[1];
        value_7 = value_7 - value_13;
        text_length = 2;
    } else {
        text_length = 1;
    }
    if (value_9 == 0)
        goto branch_9;
    value_13 = argument_3[3];
    if (value_13 == 2) {
        argument_6 = 1 - argument_6;
    branch_10:
        if (argument_6 != 0) {
        branch_11:
            value_9 = pointer[4];
            pointer_6 = pointer_30;
            do {
                pointer_12 = pointer_6 + 1;
                if (*pointer_6 != 0xffffffff) {
                    *pointer_6 = *pointer_6 + 1;
                    goto branch_12;
                }
                *pointer_6 = 0;
                pointer_6 = pointer_12;
            } while (pointer_12 < pointer_30 + value_9);
            if ((int)value_9 < (int)pointer[2]) {
                value_13 = value_9;
            } else {
                pointer_6 = (uint32_t *)unresolved_c_library_17(argument_1, pointer[1] + 1);
                libc_memcpy(pointer_6 + 3, pointer + 3, (pointer[4] + 2) * 4);
                unresolved_c_library_18((int)argument_1, pointer);
                value_13 = pointer_6[4];
                pointer_30 = pointer_6 + 5;
                pointer = pointer_6;
            }
            pointer[4] = value_13 + 1;
            pointer[value_13 + 5] = 1;
        branch_12:
            if (text_length == 2) {
                if (*argument_3 - 1 == value_7) {
                    if ((1 << (value_7 & 0x1f) & pointer_30[(int)value_7 >> 5]) == 0) {
                        text_length = 0x22;
                    } else {
                        text_length = 0x21;
                    }
                } else {
                    text_length = 0x22;
                }
            } else {
                if (((int)value_9 < (int)pointer[4]) ||
                    (((value_7 & 0x1f) != 0 &&
                      (libc_strncmp_result = unresolved_c_library_20(pointer_30[value_9 - 1]),
                       libc_strncmp_result < (int)(0x20 - (value_7 & 0x1f)))))) {
                    unresolved_c_library_13((int)pointer, 1);
                    value_40 = value_40 + 1;
                    if ((int)argument_3[2] < (int)value_40) {
                    branch_6:
                        unresolved_c_library_18((int)argument_1, pointer);
                        *argument_5 = 0;
                        return 0xa3;
                    }
                }
                text_length = 0x21;
            }
            goto branch_9;
        }
    } else {
        if (value_13 == 3)
            goto branch_10;
        if (((value_13 == 1) && ((int)(value_9 << 0x1e) < 0)) &&
            ((int)((*pointer_30 | value_9) << 0x1f) < 0))
            goto branch_11;
    }
    text_length = text_length | 0x10;
branch_9:
    *argument_5 = pointer;
    *argument_4 = value_40;
    return text_length;
}

/* Purpose incomplete.
 * Firmware address: 0x2001ff28. */
uint32_t unresolved_c_library_15(int *argument_1, uint32_t *argument_2, uint32_t *argument_3)
{
    uint8_t byte_value_1;
    uint32_t *pointer_2;
    uint32_t *pointer_3;
    int value_4;
    uint32_t value_5;
    uint32_t value_6;
    uint32_t *pointer_7;
    uint32_t *pointer_8;
    int value_9;
    int value_10;
    uint32_t *pointer_11;
    uint32_t value_12;
    int value_13;
    uint32_t *pointer_30;

    pointer_30 = argument_3 + ((int)*argument_2 >> 5);
    value_6 = *argument_2 & 0x1f;
    if (value_6 != 0) {
        pointer_30 = pointer_30 + 1;
    }
    value_4 = *argument_1;
    pointer_30[-1] = 0;
    value_13 = 0;
    value_9 = 0;
    value_10 = 0;
    pointer_11 = pointer_30 + -1;
    byte_value_1 = *(uint8_t *)(value_4 + 1);
    pointer_2 = pointer_11;
    pointer_8 = pointer_11;
    while (value_5 = (uint32_t)byte_value_1, value_5 != 0) {
        byte_value_1 = (&g_c_library_unknown_10)[value_5];
        if (byte_value_1 == 0) {
            if (0x20 < value_5) {
                if (value_5 != 0x29) {
                    return 4;
                }
                *argument_1 = value_4 + 2;
                break;
            }
            if (value_9 < value_10) {
                if ((pointer_2 < pointer_8) && (value_13 < 8)) {
                    value_5 = *pointer_2;
                    pointer_7 = pointer_2;
                    do {
                        value_12 = pointer_7[1] << ((8 - value_13) * -4 + 0x20U & 0xff) | value_5;
                        value_5 = pointer_7[1] >> ((8 - value_13) * 4 & 0xffU);
                        *pointer_7 = value_12;
                        pointer_7 = pointer_7 + 1;
                        *pointer_7 = value_5;
                    } while (pointer_7 < pointer_8);
                }
                if (argument_3 < pointer_2) {
                    pointer_8 = pointer_2 + -1;
                    pointer_2[-1] = 0;
                    value_13 = 0;
                    pointer_2 = pointer_8;
                    value_9 = value_10;
                } else {
                    value_13 = 8;
                }
            }
        } else {
            value_13 = value_13 + 1;
            value_10 = value_10 + 1;
            if (value_13 < 9) {
                value_5 = *pointer_2 << 4;
            } else {
                if (pointer_2 <= argument_3)
                    goto branch_1;
                value_5 = 0;
                pointer_2[-1] = 0;
                value_13 = 1;
                pointer_2 = pointer_2 + -1;
            }
            *pointer_2 = byte_value_1 & 0xf | value_5;
        }
    branch_1:
        byte_value_1 = *(uint8_t *)(value_4 + 2);
        value_4 = value_4 + 1;
    }
    if (value_10 == 0) {
        return 4;
    }
    if ((pointer_2 < pointer_8) && (value_13 < 8)) {
        value_5 = *pointer_2;
        pointer_7 = pointer_2;
        do {
            value_12 = pointer_7[1] << ((8 - value_13) * -4 + 0x20U & 0xff) | value_5;
            value_5 = pointer_7[1] >> ((8 - value_13) * 4 & 0xffU);
            *pointer_7 = value_12;
            pointer_7 = pointer_7 + 1;
            *pointer_7 = value_5;
        } while (pointer_7 < pointer_8);
    }
    if (argument_3 < pointer_2) {
        pointer_8 = argument_3;
        do {
            pointer_3 = pointer_2 + 1;
            pointer_7 = pointer_8 + 1;
            *pointer_8 = *pointer_2;
            pointer_2 = pointer_3;
            pointer_8 = pointer_7;
        } while (pointer_3 <= pointer_11);
        do {
            pointer_2 = pointer_7 + 1;
            *pointer_7 = 0;
            pointer_7 = pointer_2;
        } while (pointer_2 <= pointer_11);
    } else if (value_6 != 0) {
        value_6 = pointer_30[-1] & 0xffffffffU >> (0x20 - value_6 & 0xff);
        pointer_30[-1] = value_6;
        goto joined_r0x2001ffec;
    }
    value_6 = pointer_30[-1];
joined_r0x2001ffec:
    while (true) {
        if (value_6 != 0) {
            return 5;
        }
        if (argument_3 == pointer_11)
            break;
        pointer_11 = pointer_11 + -1;
        value_6 = *pointer_11;
    }
    *pointer_11 = 1;
    return 5;
}

/* Purpose incomplete.
 * Firmware address: 0x200200bc. */
uint8_t *unresolved_c_library_16(void)
{
    return &g_c_library_unknown_23;
}

/* Purpose incomplete.
 * Firmware address: 0x20020154. */
int *unresolved_c_library_17(int *argument_1, uint32_t argument_2)
{
    int *pointer_1;
    int *pointer_2;
    int value_3;

    pointer_2 = (int *)argument_1[0x13];
    if (pointer_2 == (int *)0x0) {
        pointer_2 = unresolved_floating_point_08(argument_1, 4, 0x21);
        argument_1[0x13] = (int)pointer_2;
        if (pointer_2 == (int *)0x0) {
            return (int *)0x0;
        }
    }
    pointer_1 = (int *)pointer_2[argument_2];
    if (pointer_1 == (int *)0x0) {
        value_3 = 1 << (argument_2 & 0xff);
        pointer_1 = unresolved_floating_point_08(argument_1, 1, (value_3 + 5) * 4);
        if (pointer_1 == (int *)0x0) {
            return (int *)0x0;
        }
        pointer_1[1] = argument_2;
        pointer_1[2] = value_3;
    } else {
        pointer_2[argument_2] = *pointer_1;
    }
    pointer_1[4] = 0;
    pointer_1[3] = 0;
    return pointer_1;
}

/* Purpose incomplete.
 * Firmware address: 0x200201a0. */
void unresolved_c_library_18(int argument_1, uint32_t *argument_2)
{
    int value_1;

    if (argument_2 != (uint32_t *)0x0) {
        value_1 = *(int *)(argument_1 + 0x4c);
        *argument_2 = *(uint32_t *)(value_1 + argument_2[1] * 4);
        *(uint32_t **)(value_1 + argument_2[1] * 4) = argument_2;
    }
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x200201b4. */
int *unresolved_c_library_19(int *argument_1, int *argument_2, int argument_3, uint32_t argument_4)
{
    int *pointer_1;
    uint32_t value_2;
    int value_3;
    uint32_t *pointer_4;
    uint32_t value_5;
    int value_6;

    value_6 = argument_2[4];
    value_3 = 0;
    pointer_4 = (uint32_t *)(argument_2 + 5);
    do {
        value_5 = argument_3 * (*pointer_4 & 0xffff) + argument_4;
        value_2 = argument_3 * (*pointer_4 >> 0x10) + (value_5 >> 0x10);
        value_3 = value_3 + 1;
        *pointer_4 = (value_5 & 0xffff) + value_2 * 0x10000;
        argument_4 = value_2 >> 0x10;
        pointer_4 = pointer_4 + 1;
    } while (value_3 < value_6);
    pointer_1 = argument_2;
    if (argument_4 != 0) {
        if (argument_2[2] <= value_6) {
            pointer_1 = unresolved_c_library_17(argument_1, argument_2[1] + 1);
            libc_memcpy(pointer_1 + 3, argument_2 + 3, (argument_2[4] + 2) * 4);
            value_3 = argument_1[0x13];
            *argument_2 = *(int *)(value_3 + argument_2[1] * 4);
            *(int **)(value_3 + argument_2[1] * 4) = argument_2;
        }
        pointer_1[value_6 + 5] = argument_4;
        pointer_1[4] = value_6 + 1;
    }
    return pointer_1;
}

/* Allocates Bigint at20020154, places initial mantissa word, then multiplies/adds decimal digits
 * through200201b4. Epilogue200202ce leaves returned allocation/result pointer in r0. Every loop
 * call feeds its r0 pointer into the next call. Firmware address: 0x2002024c. */
int *unresolved_c_library_21(int *reentrancy, int text_address, int digits_before_decimal,
                             int total_digits, int initial_digits)
{
    int value_1;
    int *pointer_2;
    uint32_t value_3;
    uint8_t *pointer_4;
    uint8_t *pointer_5;
    int value_6;
    uint8_t *pointer_7;

    value_1 = (total_digits + 8) / 9;
    if (value_1 < 2) {
        value_3 = 0;
    } else {
        value_6 = 1;
        value_3 = 0;
        do {
            value_6 = value_6 * 2;
            value_3 = value_3 + 1;
        } while (value_6 < value_1);
    }
    pointer_2 = unresolved_c_library_17(reentrancy, value_3);
    pointer_2[5] = initial_digits;
    pointer_2[4] = 1;
    if (digits_before_decimal < 10) {
        pointer_4 = (uint8_t *)(text_address + 10);
        digits_before_decimal = 9;
    } else {
        pointer_4 = (uint8_t *)(text_address + 9);
        do {
            pointer_7 = pointer_4 + 1;
            pointer_2 = unresolved_c_library_19(reentrancy, pointer_2, 10, *pointer_4 - 0x30);
            pointer_4 = pointer_7;
        } while (pointer_7 != (uint8_t *)(text_address + digits_before_decimal));
        pointer_4 = (uint8_t *)(text_address + 9) + digits_before_decimal + -8;
    }
    if (digits_before_decimal < total_digits) {
        pointer_7 = pointer_4;
        do {
            pointer_5 = pointer_7 + 1;
            pointer_2 = unresolved_c_library_19(reentrancy, pointer_2, 10, *pointer_7 - 0x30);
            pointer_7 = pointer_5;
        } while (pointer_5 != pointer_4 + (total_digits - digits_before_decimal));
    }
    return pointer_2;
}

/* Purpose incomplete.
 * Firmware address: 0x200202e4. */
int unresolved_c_library_20(uint32_t argument_1)
{
    int value_1;

    if ((argument_1 & 0xffff0000) == 0) {
        argument_1 = argument_1 << 0x10;
        value_1 = 0x10;
    } else {
        value_1 = 0;
    }
    if ((argument_1 & 0xff000000) == 0) {
        argument_1 = argument_1 << 8;
        value_1 = value_1 + 8;
    }
    if ((argument_1 & 0xf0000000) == 0) {
        argument_1 = argument_1 << 4;
        value_1 = value_1 + 4;
    }
    if ((argument_1 & 0xc0000000) == 0) {
        argument_1 = argument_1 << 2;
        value_1 = value_1 + 2;
    }
    if ((int)argument_1 < 0) {
        return value_1;
    }
    if ((int)(argument_1 << 1) < 0) {
        return value_1 + 1;
    }
    return 0x20;
}

/* Purpose incomplete.
 * Firmware address: 0x20020324. */
int unresolved_c_library_21_2(uint32_t *argument_1)
{
    int value_1;
    uint32_t value_2;

    value_2 = *argument_1;
    if ((value_2 & 7) == 0) {
        value_1 = 0;
        if ((value_2 & 0xffff) == 0) {
            value_2 = value_2 >> 0x10;
            value_1 = 0x10;
        }
        if ((value_2 & 0xff) == 0) {
            value_2 = value_2 >> 8;
            value_1 = value_1 + 8;
        }
        if ((value_2 & 0xf) == 0) {
            value_2 = value_2 >> 4;
            value_1 = value_1 + 4;
        }
        if ((value_2 & 3) == 0) {
            value_2 = value_2 >> 2;
            value_1 = value_1 + 2;
        }
        if (-1 < (int)(value_2 << 0x1f)) {
            value_2 = value_2 >> 1;
            if (value_2 == 0) {
                return 0x20;
            }
            value_1 = value_1 + 1;
        }
        *argument_1 = value_2;
        return value_1;
    }
    if ((int)(value_2 << 0x1f) < 0) {
        return 0;
    }
    if ((int)(value_2 << 0x1e) < 0) {
        *argument_1 = value_2 >> 1;
        return 1;
    }
    *argument_1 = value_2 >> 2;
    return 2;
}

/* Calls Bigint allocator20020154 at20020386. Writes value at[r0+0x14] and word count1 at[r0+0x10];
 * pop at20020390 preserves allocation pointer r0. Firmware address: 0x20020380. */
int *unresolved_c_library_24(int *reentrancy, int value)
{
    int *pointer_1;

    pointer_1 = unresolved_c_library_17(reentrancy, 1);
    pointer_1[5] = value;
    pointer_1[4] = 1;
    return pointer_1;
}

/* Purpose incomplete.
 * Firmware address: 0x20020394. */
int *unresolved_c_library_22(int *argument_1, int argument_2, int argument_3)
{
    int *pointer_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t *pointer_4;
    uint32_t *pointer_5;
    uint32_t *pointer_6;
    int value_7;
    uint32_t *pointer_8;
    uint32_t value_9;
    int value_10;
    int value_11;
    uint32_t *pointer_12;
    uint32_t *pointer_13;
    int value_14;
    uint32_t value_15;

    value_10 = *(int *)(argument_2 + 0x10);
    value_14 = *(int *)(argument_3 + 0x10);
    value_7 = argument_2;
    value_11 = value_10;
    if (value_10 < value_14) {
        value_7 = argument_3;
        value_11 = value_14;
        value_14 = value_10;
        argument_3 = argument_2;
    }
    value_3 = *(uint32_t *)(value_7 + 4);
    value_10 = value_11 + value_14;
    if (*(int *)(value_7 + 8) < value_10) {
        value_3 = value_3 + 1;
    }
    pointer_1 = unresolved_c_library_17(argument_1, value_3);
    pointer_12 = (uint32_t *)(pointer_1 + 5);
    pointer_13 = pointer_12 + value_10;
    if (pointer_12 < pointer_13) {
        pointer_5 = pointer_12;
        do {
            pointer_8 = pointer_5 + 1;
            *pointer_5 = 0;
            pointer_5 = pointer_8;
        } while (pointer_8 < pointer_13);
    }
    pointer_8 = (uint32_t *)(value_7 + 0x14);
    pointer_5 = (uint32_t *)(argument_3 + 0x14U);
    while (pointer_5 < (uint32_t *)(argument_3 + 0x14U) + value_14) {
        value_3 = *pointer_5;
        value_9 = value_3 & 0xffff;
        if (value_9 != 0) {
            value_3 = 0;
            pointer_4 = pointer_8;
            pointer_6 = pointer_12;
            while (true) {
                value_15 = value_3 + value_9 * (*pointer_4 & 0xffff) + (*pointer_6 & 0xffff);
                value_2 =
                    value_9 * (*pointer_4 >> 0x10) + (*pointer_6 >> 0x10) + (value_15 >> 0x10);
                value_3 = value_2 >> 0x10;
                *pointer_6 = value_15 & 0xffff | value_2 * 0x10000;
                if (pointer_8 + value_11 <= pointer_4 + 1)
                    break;
                pointer_4 = pointer_4 + 1;
                pointer_6 = pointer_6 + 1;
            }
            pointer_6[1] = value_3;
            value_3 = *pointer_5;
        }
        value_3 = value_3 >> 0x10;
        if (value_3 != 0) {
            value_2 = *pointer_12;
            value_15 = 0;
            pointer_4 = pointer_12;
            pointer_6 = pointer_8;
            value_9 = value_2;
            while (true) {
                value_15 = value_15 + value_3 * (uint16_t)*pointer_6 + (value_2 >> 0x10);
                *pointer_4 = value_9 & 0xffff | value_15 * 0x10000;
                value_2 = pointer_4[1];
                value_9 = value_3 * (*pointer_6 >> 0x10) + (value_2 & 0xffff) + (value_15 >> 0x10);
                value_15 = value_9 >> 0x10;
                if (pointer_8 + value_11 <= pointer_6 + 1)
                    break;
                pointer_4 = pointer_4 + 1;
                pointer_6 = pointer_6 + 1;
            }
            pointer_4[1] = value_9;
        }
        pointer_12 = pointer_12 + 1;
        pointer_5 = pointer_5 + 1;
    }
    if (0 < value_10) {
        value_3 = pointer_13[-1];
        pointer_13 = pointer_13 + -1;
        while ((value_3 == 0 && (value_10 = value_10 + -1, value_10 != 0))) {
            pointer_13 = pointer_13 + -1;
            value_3 = *pointer_13;
        }
    }
    pointer_1[4] = value_10;
    return pointer_1;
}

/* Purpose incomplete.
 * Firmware address: 0x200204d0. */
int *unresolved_c_library_23(int *argument_1, int *argument_2, uint32_t argument_3)
{
    int *pointer_1;
    int value_2;
    int value_3;
    int *pointer_4;

    if ((argument_3 & 3) != 0) {
        argument_2 = unresolved_c_library_19(
            argument_1, argument_2, *(int *)(&g_c_library_unknown_12 + ((argument_3 & 3) - 1) * 4),
            0);
    }
    value_3 = (int)argument_3 >> 2;
    if (value_3 != 0) {
        pointer_4 = (int *)argument_1[0x12];
        if (pointer_4 == (int *)0x0) {
            pointer_4 = unresolved_c_library_17(argument_1, 1);
            pointer_4[5] = 0x271;
            pointer_4[4] = 1;
            argument_1[0x12] = (int)pointer_4;
            *pointer_4 = 0;
        }
        while (true) {
            value_2 = value_3 << 0x1f;
            value_3 = value_3 >> 1;
            pointer_1 = argument_2;
            if ((value_2 < 0) &&
                (pointer_1 = unresolved_c_library_22(argument_1, (int)argument_2, (int)pointer_4),
                 argument_2 != (int *)0x0)) {
                value_2 = argument_1[0x13];
                *argument_2 = *(int *)(value_2 + argument_2[1] * 4);
                *(int **)(value_2 + argument_2[1] * 4) = argument_2;
            }
            argument_2 = pointer_1;
            if (value_3 == 0)
                break;
            pointer_1 = (int *)*pointer_4;
            if (pointer_1 == (int *)0x0) {
                pointer_1 = unresolved_c_library_22(argument_1, (int)pointer_4, (int)pointer_4);
                *pointer_4 = (int)pointer_1;
                *pointer_1 = 0;
            }
            pointer_4 = pointer_1;
        }
    }
    return argument_2;
}

/* Allocates shifted Bigint, copies shifted limbs, recycles input into reentrancy freelist.
 * At20020602 writes result word count through r0; epilogue2002060a preserves that result pointer.
 * Both aligned and unaligned shift paths converge there. Firmware address: 0x2002056c. */
int *unresolved_c_library_27(int *reentrancy, uint32_t *value, uint32_t shift)
{
    int *pointer_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t index;
    int value_5;
    int value_6;
    uint32_t *pointer_8;
    uint32_t *pointer_7;
    uint32_t *pointer_9;
    uint32_t *pointer_10;
    int value_11;
    uint32_t *pointer_12;

    value_3 = value[4];
    value_6 = (int)shift >> 5;
    value_11 = value_6 + value_3 + 1;
    value_2 = value[1];
    for (index = value[2]; (int)index < value_11; index = index * 2) {
        value_2 = value_2 + 1;
    }
    pointer_1 = unresolved_c_library_17(reentrancy, value_2);
    pointer_9 = (uint32_t *)(pointer_1 + 5);
    if (0 < value_6) {
        value_5 = 0;
        pointer_7 = pointer_9;
        do {
            value_5 = value_5 + 1;
            *pointer_7 = 0;
            pointer_7 = pointer_7 + 1;
        } while (value_5 != value_6);
        pointer_9 = pointer_9 + value_5;
    }
    pointer_7 = value + 5;
    index = shift & 0x1f;
    pointer_12 = pointer_7 + value[4];
    if (index == 0) {
        do {
            pointer_8 = pointer_7 + 1;
            pointer_10 = pointer_9 + 1;
            *pointer_9 = *pointer_7;
            if (pointer_12 <= pointer_8)
                break;
            pointer_7 = pointer_7 + 2;
            pointer_9 = pointer_9 + 2;
            *pointer_10 = *pointer_8;
        } while (pointer_7 < pointer_12);
    } else {
        value_2 = 0;
        while (true) {
            *pointer_9 = value_2 | *pointer_7 << index;
            value_2 = *pointer_7 >> (0x20 - index & 0xff);
            if (pointer_12 <= pointer_7 + 1)
                break;
            pointer_7 = pointer_7 + 1;
            pointer_9 = pointer_9 + 1;
        }
        pointer_9[1] = value_2;
        if (value_2 != 0) {
            value_11 = value_6 + value_3 + 2;
        }
    }
    value_6 = reentrancy[0x13];
    value_2 = value[1];
    index = *(uint32_t *)(value_6 + value_2 * 4);
    pointer_1[4] = value_11 + -1;
    *value = index;
    *(uint32_t **)(value_6 + value_2 * 4) = value;
    return pointer_1;
}

/* Purpose incomplete.
 * Firmware address: 0x20020628. */
int unresolved_c_library_24_2(int argument_1, int argument_2)
{
    uint32_t *pointer_1;
    int value_2;
    int value_3;
    uint32_t *pointer_4;

    value_3 = *(int *)(argument_2 + 0x10);
    value_2 = *(int *)(argument_1 + 0x10) - value_3;
    if (value_2 != 0) {
        return value_2;
    }
    pointer_1 = (uint32_t *)(argument_2 + 0x14 + value_3 * 4);
    pointer_4 = (uint32_t *)(argument_1 + 0x14U) + value_3;
    do {
        pointer_4 = pointer_4 + -1;
        pointer_1 = pointer_1 + -1;
        if (*pointer_4 != *pointer_1) {
            if (*pointer_4 < *pointer_1) {
                value_2 = -1;
            } else {
                value_2 = 1;
            }
            return value_2;
        }
    } while ((uint32_t *)(argument_1 + 0x14U) < pointer_4);
    return 0;
}

/* Allocates difference Bigint, records sign and subtracts limbs. Nonzero path stores word count
 * through r0 at20020720 then returns. Equal-operands path allocates zero Bigint and returns same r0
 * at20020740. Firmware address: 0x20020670. */
int *unresolved_c_library_29(int *reentrancy, int left_address, int right_address)
{
    int value_1;
    int *pointer_2;
    int value_3;
    uint32_t value_4;
    uint32_t *pointer_5;
    uint32_t *pointer_6;
    uint32_t *pointer_7;
    int value_8;
    uint32_t *pointer_9;
    uint32_t *pointer_11;
    uint32_t *pointer_10;
    int value_12;
    uint32_t *pointer_13;

    value_1 = unresolved_c_library_24_2(left_address, right_address);
    if (value_1 == 0) {
        pointer_2 = unresolved_c_library_17(reentrancy, 0);
        pointer_2[5] = 0;
        pointer_2[4] = 1;
        return pointer_2;
    }
    value_8 = left_address;
    if (value_1 < 0) {
        value_8 = right_address;
        right_address = left_address;
    }
    pointer_2 = unresolved_c_library_17(reentrancy, *(uint32_t *)(value_8 + 4));
    value_12 = *(int *)(value_8 + 0x10);
    value_3 = *(int *)(right_address + 0x10);
    pointer_2[3] = (uint32_t)(value_1 < 0);
    pointer_13 = (uint32_t *)(value_8 + 0x14) + value_12;
    value_1 = 0;
    pointer_5 = (uint32_t *)(pointer_2 + 5);
    pointer_7 = (uint32_t *)(value_8 + 0x14);
    pointer_10 = (uint32_t *)(right_address + 0x14);
    do {
        pointer_11 = pointer_10 + 1;
        pointer_9 = pointer_7 + 1;
        value_4 = (value_1 + (*pointer_7 & 0xffff)) - (*pointer_10 & 0xffff);
        value_1 = ((*pointer_7 >> 0x10) - (*pointer_10 >> 0x10)) + ((int)value_4 >> 0x10);
        value_4 = value_4 & 0xffff | value_1 * 0x10000;
        pointer_6 = pointer_5 + 1;
        *pointer_5 = value_4;
        value_1 = value_1 >> 0x10;
        pointer_5 = pointer_6;
        pointer_7 = pointer_9;
        pointer_10 = pointer_11;
    } while (pointer_11 < (uint32_t *)(right_address + 0x14) + value_3);
    if (pointer_9 < pointer_13) {
        do {
            pointer_10 = pointer_7 + 1;
            value_4 = value_1 + (*pointer_7 & 0xffff);
            value_1 = (*pointer_7 >> 0x10) + ((int)value_4 >> 0x10);
            value_4 = value_4 & 0xffff | value_1 * 0x10000;
            *pointer_5 = value_4;
            value_1 = value_1 >> 0x10;
            pointer_5 = pointer_5 + 1;
            pointer_7 = pointer_10;
        } while (pointer_10 < pointer_13);
        pointer_6 = (uint32_t *)((~(uint32_t)pointer_9 + (int)pointer_13 & 0xfffffffc) + 4 +
                                 (int)pointer_6);
    }
    pointer_6 = pointer_6 + -1;
    while (value_4 == 0) {
        pointer_6 = pointer_6 + -1;
        value_12 = value_12 + -1;
        value_4 = *pointer_6;
    }
    pointer_2[4] = value_12;
    return pointer_2;
}
