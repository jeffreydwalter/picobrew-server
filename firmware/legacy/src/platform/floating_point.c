/* Pico Pro 0.1.34 — floating point. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Purpose incomplete.
 * Firmware address: 0x20020744. */
uint64_t unresolved_floating_point_01(uint32_t argument_1, uint32_t argument_2)
{
    uint32_t value_1;

    value_1 = (argument_2 & 0x7ff00000) + 0xfcc00000;
    if (0 < (int)value_1) {
        return (uint64_t)value_1 << 0x20;
    }
    value_1 = (int)-value_1 >> 0x14;
    if ((int)value_1 < 0x14) {
        return (uint64_t)(uint32_t)(0x80000 >> (value_1 & 0xff)) << 0x20;
    }
    if ((int)value_1 < 0x33) {
        value_1 = 1 << (0x33 - value_1 & 0xff);
    } else {
        value_1 = 1;
    }
    return (uint64_t)value_1;
}

/* Purpose incomplete.
 * Firmware address: 0x20020784. */
uint64_t unresolved_floating_point_02(int argument_1, int *argument_2)
{
    int value_1;
    uint32_t value_2;
    uint32_t value_3;
    int value_4;
    uint32_t value_5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t value_8;
    bool condition_9;

    value_7 = argument_1 + 0x14;
    value_4 = value_7 + *(int *)(argument_1 + 0x10) * 4;
    value_6 = *(uint32_t *)(value_4 + -4);
    value_1 = unresolved_c_library_20(value_6);
    *argument_2 = 0x20 - value_1;
    value_2 = value_4 - 4;
    if (value_1 < 0xb) {
        condition_9 = value_7 < value_2;
        value_7 = 0xb - value_1;
        if (condition_9) {
            value_2 = *(uint32_t *)(value_4 + -8);
        }
        value_5 = value_7 & 0xff;
        if (condition_9) {
            value_7 = value_2 >> (value_7 & 0xff);
        }
        if (!condition_9) {
            value_7 = 0;
        }
        return pico_join_u32_u32(value_6 >> value_5,
                                 value_7 | value_6 << (value_1 + 0x15U & 0xff)) |
               0x3ff0000000000000;
    }
    if (value_7 < value_2) {
        value_2 = value_4 - 8;
        value_5 = *(uint32_t *)(value_4 + -8);
    } else {
        value_5 = 0;
    }
    value_8 = value_1 - 0xb;
    if (value_8 == 0) {
        return pico_join_u32_u32(value_6, value_5) | 0x3ff0000000000000;
    }
    value_3 = value_2;
    if (value_7 < value_2) {
        value_3 = *(uint32_t *)(value_2 - 4);
    }
    if (value_7 < value_2) {
        value_3 = value_3 >> (0x2bU - value_1 & 0xff);
    } else {
        value_3 = 0;
    }
    return pico_join_u32_u32(value_6 << (value_8 & 0xff) | 0x3ff00000 |
                                 value_5 >> (0x2bU - value_1 & 0xff),
                             value_3 | value_5 << (value_8 & 0xff));
}

/* Purpose incomplete.
 * Firmware address: 0x20020838. */
int *unresolved_floating_point_03(int *argument_1, uint32_t argument_2, uint32_t argument_3,
                                  uint32_t argument_4, int *argument_5, int *argument_6)
{
    int *pointer_1;
    uint32_t value_2;
    int value_3;
    uint32_t value_4;
    int value_5;
    uint32_t value_6;
    uint32_t value_28;
    uint32_t buffer_24[2];

    value_6 = (argument_4 & 0x7fffffff) >> 0x14;
    pointer_1 = unresolved_c_library_17(argument_1, 1);
    buffer_24[0] = argument_4 & 0xfffff;
    if (value_6 != 0) {
        buffer_24[0] = buffer_24[0] | 0x100000;
    }
    if (argument_3 == 0) {
        value_3 = unresolved_c_library_21_2(buffer_24);
        value_5 = 1;
        pointer_1[5] = buffer_24[0];
        value_2 = value_3 + 0x20;
        pointer_1[4] = 1;
    } else {
        value_28 = argument_3;
        value_2 = unresolved_c_library_21_2(&value_28);
        if (value_2 == 0) {
            pointer_1[5] = value_28;
        } else {
            value_4 = buffer_24[0] << (0x20 - value_2 & 0xff);
            buffer_24[0] = buffer_24[0] >> (value_2 & 0xff);
            pointer_1[5] = value_28 | value_4;
        }
        if (buffer_24[0] == 0) {
            value_5 = 1;
        } else {
            value_5 = 2;
        }
        pointer_1[6] = buffer_24[0];
        pointer_1[4] = value_5;
    }
    if (value_6 == 0) {
        *argument_5 = value_2 - 0x432;
        value_3 = unresolved_c_library_20(pointer_1[value_5 + 4]);
        *argument_6 = value_5 * 0x20 - value_3;
        return pointer_1;
    }
    *argument_5 = (value_6 - 0x433) + value_2;
    *argument_6 = 0x35 - value_2;
    return pointer_1;
}

/* Purpose incomplete.
 * Firmware address: 0x200208f4. */
double unresolved_floating_point_04(int argument_1, int argument_2)
{
    int value_1;
    int value_2;
    int value_3;
    uint64_t value_4;
    uint64_t value_5;
    double __aeabi_ddiv_result;
    int value_20;
    int buffer[2];

    value_4 = unresolved_floating_point_02(argument_1, &value_20);
    value_1 = (int)(value_4 >> 0x20);
    value_5 = unresolved_floating_point_02(argument_2, buffer);
    value_2 = (int)(value_5 >> 0x20);
    value_3 =
        (value_20 - buffer[0]) + (*(int *)(argument_1 + 0x10) - *(int *)(argument_2 + 0x10)) * 0x20;
    if (value_3 < 1) {
        value_2 = value_2 + value_3 * -0x100000;
    } else {
        value_1 = value_1 + value_3 * 0x100000;
    }
    __aeabi_ddiv_result = __aeabi_ddiv((double)pico_join_u32_u32(value_1, (int)value_4),
                                       (double)pico_join_u32_u32(value_2, (int)value_5));
    return __aeabi_ddiv_result;
}

/* Purpose incomplete.
 * Firmware address: 0x20020944. */
void unresolved_floating_point_05(uint32_t *argument_1, int argument_2, int argument_3)
{
    uint32_t *pointer_1;
    uint32_t *pointer_2;
    uint32_t *pointer_3;
    uint32_t *pointer_4;
    uint32_t *pointer_5;

    pointer_2 = (uint32_t *)(argument_3 + 0x14);
    pointer_4 = pointer_2 + *(int *)(argument_3 + 0x10);
    pointer_5 = argument_1 + (argument_2 + -1 >> 5) + 1;
    if (pointer_2 < pointer_4) {
        pointer_1 = argument_1;
        do {
            pointer_3 = pointer_2 + 1;
            *pointer_1 = *pointer_2;
            pointer_1 = pointer_1 + 1;
            pointer_2 = pointer_3;
        } while (pointer_3 < pointer_4);
        argument_1 = (uint32_t *)((int)argument_1 +
                                  ((int)pointer_4 + (-0x15 - argument_3) & 0xfffffffcU) + 4);
    }
    for (; argument_1 < pointer_5; argument_1 = argument_1 + 1) {
        *argument_1 = 0;
    }
    return;
}

/* Purpose incomplete.
 * Firmware address: 0x2002098c. */
uint32_t unresolved_floating_point_06(int argument_1, uint32_t argument_2)
{
    uint32_t value_1;
    int value_2;
    int value_3;
    int *pointer_4;
    int *pointer_5;

    value_3 = *(int *)(argument_1 + 0x10);
    value_2 = (int)argument_2 >> 5;
    pointer_5 = (int *)(argument_1 + 0x14);
    if (value_3 < value_2) {
        pointer_4 = pointer_5 + value_3;
    } else if ((value_2 < value_3) && (value_1 = argument_2 & 0x1f, value_1 != 0)) {
        pointer_4 = pointer_5 + value_2;
        if (((uint32_t)pointer_5[value_2] >> value_1) << value_1 != pointer_5[value_2]) {
            return 1;
        }
    } else {
        pointer_4 = pointer_5 + value_2;
    }
    if (pointer_4 <= pointer_5) {
        return 0;
    }
    value_2 = pointer_4[-1];
    pointer_4 = pointer_4 + -1;
    while (true) {
        if (value_2 != 0) {
            return 1;
        }
        if (pointer_4 <= pointer_5)
            break;
        pointer_4 = pointer_4 + -1;
        value_2 = *pointer_4;
    }
    return 0;
}

/* Purpose incomplete.
 * Firmware address: 0x200209ec. */
uint32_t unresolved_floating_point_07(int *argument_1, int *argument_2, int *argument_3)
{
    uint16_t short_value_1;
    void *destination;
    int *destination_00;
    uint32_t value_2;
    int value_3;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t *pointer_6;
    uint32_t count;
    uint32_t value_7;
    void *source;

    pointer_6 = (uint32_t *)*argument_3;
    if (argument_3[2] == 0) {
    branch_1:
        argument_3[1] = 0;
        return 0;
    }
    source = (void *)0x0;
    destination = (void *)*argument_2;
    value_4 = argument_2[2];
    value_5 = 0;
    do {
        if (value_5 != 0) {
            value_7 = value_5;
            if (value_4 <= value_5) {
                if ((*(uint16_t *)(argument_2 + 3) & 0x480) == 0) {
                    value_7 = value_4;
                } else {
                    count = (int)destination - argument_2[4];
                    value_4 = (argument_2[5] * 3) / 2;
                    value_2 = value_5 + 1 + count;
                    if (value_4 < value_2) {
                        value_4 = value_2;
                    }
                    if ((int)((uint32_t) * (uint16_t *)(argument_2 + 3) << 0x15) < 0) {
                        destination_00 = unresolved_c_library_03(argument_1, value_4);
                        if (destination_00 == (int *)0x0)
                            goto branch_2;
                        libc_memcpy(destination_00, (void *)argument_2[4], count);
                        *(uint16_t *)(argument_2 + 3) =
                            *(uint16_t *)(argument_2 + 3) & 0xfb7f | 0x80;
                    } else {
                        destination_00 =
                            unresolved_c_library_06(argument_1, (int *)argument_2[4], value_4);
                        if (destination_00 == (int *)0x0) {
                            unresolved_c_library_12(argument_1, argument_2[4]);
                        branch_2:
                            short_value_1 = *(uint16_t *)(argument_2 + 3);
                            *argument_1 = 0xc;
                            *(uint16_t *)(argument_2 + 3) = short_value_1 | 0x40;
                            argument_3[2] = 0;
                            argument_3[1] = 0;
                            return 0xffffffff;
                        }
                    }
                    destination = (void *)((int)destination_00 + count);
                    argument_2[5] = value_4;
                    argument_2[4] = (int)destination_00;
                    *argument_2 = (int)destination;
                    argument_2[2] = value_4 - count;
                }
            }
            libc_memmove(destination, source, value_7);
            value_3 = argument_3[2];
            value_4 = argument_2[2] - value_7;
            destination = (void *)(*argument_2 + value_7);
            argument_2[2] = value_4;
            *argument_2 = (int)destination;
            argument_3[2] = value_3 - value_5;
            if (value_3 - value_5 == 0)
                goto branch_1;
        }
        source = (void *)*pointer_6;
        value_5 = pointer_6[1];
        pointer_6 = pointer_6 + 2;
    } while (true);
}

/* Purpose incomplete.
 * Firmware address: 0x20020aec. */
int *unresolved_floating_point_08(int *argument_1, int argument_2, int argument_3)
{
    int *destination;
    uint32_t count;
    int *pointer_1;

    destination = unresolved_c_library_03(argument_1, argument_3 * argument_2);
    if (destination != (int *)0x0) {
        count = (destination[-1] & 0xfffffffcU) - 4;
        if (0x24 < count) {
            libc_memset(destination, 0, count);
            return destination;
        }
        if (count < 0x14) {
            pointer_1 = destination;
        } else {
            *destination = 0;
            destination[1] = 0;
            if (count < 0x1c) {
                pointer_1 = destination + 2;
            } else {
                destination[2] = 0;
                destination[3] = 0;
                if (count == 0x24) {
                    destination[4] = 0;
                    destination[5] = 0;
                    pointer_1 = destination + 6;
                } else {
                    pointer_1 = destination + 4;
                }
            }
        }
        *pointer_1 = 0;
        pointer_1[1] = 0;
        pointer_1[2] = 0;
    }
    return destination;
}
