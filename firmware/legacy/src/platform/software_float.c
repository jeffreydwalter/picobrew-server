/* Pico Pro 0.1.34 — software float. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Analysis reconstruction only. In these software floating-point routines,
 * decompiler casts can stand for IEEE-754 bit reinterpretation rather than
 * C numeric conversion. Those casts still require instruction-level review.
 * These bodies must not be used as an executable arithmetic library. */

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2001a830. */
double libm_ceil(double value)
{
    int __aeabi_dcmpgt_result;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t value_6;
    bool condition_7;
    double __aeabi_dadd_result;

    value_5 = (uint32_t)((uint64_t)value >> 0x20);
    value_4 = PICO_SUB_U32(value, 0);
    value_3 = (value_5 & 0x7fffffff) >> 0x14;
    value_2 = value_3 - 0x3ff;
    if ((int)value_2 < 0x14) {
        if ((int)value_2 < 0) {
            __aeabi_dadd_result = __aeabi_dadd(value, 1e+300);
            __aeabi_dcmpgt_result = __aeabi_dcmpgt(__aeabi_dadd_result, 0.0);
            if (__aeabi_dcmpgt_result != 0) {
                if ((int64_t)value < 0) {
                    value = -0.0;
                } else if (value == 0.0) {
                    value = 0.0;
                } else {
                    value = 1.0;
                }
            }
        } else {
            value_3 = 0xfffff >> (value_2 & 0xff);
            if ((value_3 & value_5) == 0 && value_4 == 0) {
                return value;
            }
            __aeabi_dadd_result = __aeabi_dadd(value, 1e+300);
            __aeabi_dcmpgt_result = __aeabi_dcmpgt(__aeabi_dadd_result, 0.0);
            if (__aeabi_dcmpgt_result != 0) {
                if (value_5 != 0 && -1 < (int64_t)value) {
                    value_5 = (0x100000 >> (value_2 & 0xff)) + value_5;
                }
                value = (double)((uint64_t)(value_5 & ~value_3) << 0x20);
            }
        }
        return value;
    }
    if ((int)value_2 < 0x34) {
        value_6 = 0xffffffff >> (value_3 - 0x413 & 0xff);
        if ((value_6 & value_4) != 0) {
            __aeabi_dadd_result = __aeabi_dadd(value, 1e+300);
            __aeabi_dcmpgt_result = __aeabi_dcmpgt(__aeabi_dadd_result, 0.0);
            if (__aeabi_dcmpgt_result == 0) {
                return value;
            }
            if (value_5 != 0 && -1 < (int64_t)value) {
                if (value_2 != 0x14) {
                    value_2 = 1 << (0x433 - value_3 & 0xff);
                    condition_7 = pico_carry_u32(value_2, value_4);
                    value_4 = value_2 + value_4;
                    value = (double)pico_join_u32_u32(value_5, value_4);
                    if (!condition_7)
                        goto branch_1;
                }
                value = (double)pico_join_u32_u32(value_5 + 1, value_4);
            }
        branch_1:
            return (double)pico_join_u32_u32((int)((uint64_t)value >> 0x20),
                                             PICO_SUB_U32(value, 0) & ~value_6);
        }
    } else if (value_2 == 0x400) {
        __aeabi_dadd_result = __aeabi_dadd(value, value);
        return __aeabi_dadd_result;
    }
    return value;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2001a950. */
double libm_log(double value)
{
    char byte_value_1;
    int libm_fpclassify_result;
    uint32_t *pointer_3;
    double libm_ieee754_log_result;
    double libm_quiet_nan_result;

    libm_ieee754_log_result = libm_ieee754_log(value);
    if (((g_software_float_unknown_01 == -1) ||
         (libm_fpclassify_result = libm_fpclassify(value), libm_fpclassify_result == 0)) ||
        (libm_fpclassify_result = __aeabi_dcmpgt(value, 0.0),
         byte_value_1 = g_software_float_unknown_01, libm_fpclassify_result != 0)) {
        return libm_ieee754_log_result;
    }
    if (g_software_float_unknown_01 == '\0') {
        libm_quiet_nan_result = -3.4028234663852886e+38;
    } else {
        libm_quiet_nan_result = -INFINITY;
    }
    libm_fpclassify_result = __aeabi_dcmpeq(value, 0.0);
    if (libm_fpclassify_result == 0) {
        if ((byte_value_1 == '\x02') ||
            (libm_fpclassify_result = unresolved_integer_arithmetic_01(),
             libm_fpclassify_result == 0)) {
            pointer_3 = (uint32_t *)unresolved_c_library_02();
            *pointer_3 = 0x21;
        }
        libm_quiet_nan_result = libm_quiet_nan();
    } else if ((byte_value_1 == '\x02') ||
               (libm_fpclassify_result = unresolved_integer_arithmetic_01(),
                libm_fpclassify_result == 0)) {
        pointer_3 = (uint32_t *)unresolved_c_library_02();
        *pointer_3 = 0x22;
    }
    return libm_quiet_nan_result;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2001aa38. */
double libm_ieee754_log(double value)
{
    int __aeabi_dcmpeq_result;
    int value_2;
    uint32_t value_3;
    uint32_t value_4;
    double __aeabi_ddiv_result;
    double __aeabi_dadd_result;
    double __aeabi_dmul_result_3;
    double __aeabi_dmul_result_2;
    double __aeabi_ddiv_result_2;
    double __aeabi_dmul_result;

    if ((int64_t)value < 0x10000000000000) {
        if (((uint64_t)value & 0x7fffffff00000000) == 0 && PICO_SUB_U32(value, 0) == 0) {
            __aeabi_ddiv_result = __aeabi_ddiv(-18014398509481984.0, 0.0);
            return __aeabi_ddiv_result;
        }
        if ((int64_t)value < 0) {
            __aeabi_ddiv_result = __aeabi_dsub(value, value);
            __aeabi_ddiv_result = __aeabi_ddiv(__aeabi_ddiv_result, 0.0);
            return __aeabi_ddiv_result;
        }
        value = __aeabi_dmul(value, 18014398509481984.0);
        value_2 = -0x36;
        if (0x7fefffff < (int)((uint64_t)value >> 0x20)) {
        branch_1:
            __aeabi_ddiv_result = __aeabi_dadd(value, value);
            return __aeabi_ddiv_result;
        }
    } else {
        value_2 = 0;
        if (0x7fefffffffffffff < (int64_t)value)
            goto branch_1;
    }
    value_4 = (uint32_t)((uint64_t)value >> 0x20);
    value_3 = value_4 & 0xfffff;
    __aeabi_ddiv_result =
        __aeabi_dsub((double)pico_join_u32_u32(value_3 + 0x95f64 & 0x100000 ^ 0x3ff00000 | value_3,
                                               PICO_SUB_U32(value, 0)),
                     1.0);
    value_2 = ((int)value_4 >> 0x14) + -0x3ff + value_2 + (value_3 + 0x95f64 >> 0x14);
    if (2 < (value_3 + 2 & 0xfffff)) {
        __aeabi_dadd_result = __aeabi_dadd(__aeabi_ddiv_result, 2.0);
        __aeabi_ddiv_result_2 = __aeabi_ddiv(__aeabi_ddiv_result, __aeabi_dadd_result);
        __aeabi_dadd_result = __aeabi_i2d(value_2);
        __aeabi_dmul_result_3 = __aeabi_dmul(__aeabi_ddiv_result_2, __aeabi_ddiv_result_2);
        __aeabi_dmul_result = __aeabi_dmul(__aeabi_dmul_result_3, __aeabi_dmul_result_3);
        __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dmul_result, 0.14798198605116586);
        __aeabi_dmul_result_2 = __aeabi_dadd(__aeabi_dmul_result_2, 0.1818357216161805);
        __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dmul_result_2, __aeabi_dmul_result);
        __aeabi_dmul_result_2 = __aeabi_dadd(__aeabi_dmul_result_2, 0.2857142874366239);
        __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dmul_result_2, __aeabi_dmul_result);
        __aeabi_dmul_result_2 = __aeabi_dadd(__aeabi_dmul_result_2, 0.6666666666666735);
        __aeabi_dmul_result_3 = __aeabi_dmul(__aeabi_dmul_result_2, __aeabi_dmul_result_3);
        __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dmul_result, 0.15313837699209373);
        __aeabi_dmul_result_2 = __aeabi_dadd(__aeabi_dmul_result_2, 0.22222198432149784);
        __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dmul_result_2, __aeabi_dmul_result);
        __aeabi_dmul_result_2 = __aeabi_dadd(__aeabi_dmul_result_2, 0.3999999999940942);
        __aeabi_dmul_result = __aeabi_dmul(__aeabi_dmul_result_2, __aeabi_dmul_result);
        __aeabi_dmul_result_3 = __aeabi_dadd(__aeabi_dmul_result_3, __aeabi_dmul_result);
        if ((int)(0x6b851 - value_3 | value_3 - 0x6147a) < 1) {
            if (value_2 != 0) {
                __aeabi_dmul_result = __aeabi_dmul(__aeabi_dadd_result, 0.6931471803691238);
                __aeabi_dmul_result_3 = __aeabi_dsub(__aeabi_ddiv_result, __aeabi_dmul_result_3);
                __aeabi_ddiv_result_2 = __aeabi_dmul(__aeabi_dmul_result_3, __aeabi_ddiv_result_2);
                goto branch_2;
            }
            __aeabi_dadd_result = __aeabi_dsub(__aeabi_ddiv_result, __aeabi_dmul_result_3);
            __aeabi_dadd_result = __aeabi_dmul(__aeabi_dadd_result, __aeabi_ddiv_result_2);
        } else {
            __aeabi_dmul_result = __aeabi_dmul(__aeabi_ddiv_result, 0.5);
            __aeabi_dmul_result = __aeabi_dmul(__aeabi_dmul_result, __aeabi_ddiv_result);
            if (value_2 != 0) {
                __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dadd_result, 0.6931471803691238);
                __aeabi_dmul_result_3 = __aeabi_dadd(__aeabi_dmul_result, __aeabi_dmul_result_3);
                __aeabi_ddiv_result_2 = __aeabi_dmul(__aeabi_dmul_result_3, __aeabi_ddiv_result_2);
                __aeabi_dadd_result = __aeabi_dmul(__aeabi_dadd_result, 1.9082149292705877e-10);
                __aeabi_dadd_result = __aeabi_dadd(__aeabi_ddiv_result_2, __aeabi_dadd_result);
                __aeabi_dadd_result = __aeabi_dsub(__aeabi_dmul_result, __aeabi_dadd_result);
                __aeabi_ddiv_result = __aeabi_dsub(__aeabi_dadd_result, __aeabi_ddiv_result);
                __aeabi_ddiv_result = __aeabi_dsub(__aeabi_dmul_result_2, __aeabi_ddiv_result);
                return __aeabi_ddiv_result;
            }
            __aeabi_dadd_result = __aeabi_dadd(__aeabi_dmul_result, __aeabi_dmul_result_3);
            __aeabi_dadd_result = __aeabi_dmul(__aeabi_dadd_result, __aeabi_ddiv_result_2);
            __aeabi_dadd_result = __aeabi_dsub(__aeabi_dmul_result, __aeabi_dadd_result);
        }
        __aeabi_ddiv_result = __aeabi_dsub(__aeabi_ddiv_result, __aeabi_dadd_result);
        return __aeabi_ddiv_result;
    }
    __aeabi_dcmpeq_result = __aeabi_dcmpeq(__aeabi_ddiv_result, 0.0);
    if (__aeabi_dcmpeq_result != 0) {
        if (value_2 == 0) {
            return 0.0;
        }
        __aeabi_ddiv_result = __aeabi_i2d(value_2);
        __aeabi_dadd_result = __aeabi_dmul(__aeabi_ddiv_result, 0.6931471803691238);
        __aeabi_ddiv_result = __aeabi_dmul(__aeabi_ddiv_result, 1.9082149292705877e-10);
        __aeabi_ddiv_result = __aeabi_dadd(__aeabi_dadd_result, __aeabi_ddiv_result);
        return __aeabi_ddiv_result;
    }
    __aeabi_dadd_result = __aeabi_dmul(__aeabi_ddiv_result, __aeabi_ddiv_result);
    __aeabi_ddiv_result_2 = __aeabi_dmul(__aeabi_ddiv_result, 0.3333333333333333);
    __aeabi_ddiv_result_2 = __aeabi_dsub(0.5, __aeabi_ddiv_result_2);
    __aeabi_ddiv_result_2 = __aeabi_dmul(__aeabi_dadd_result, __aeabi_ddiv_result_2);
    if (value_2 == 0) {
        __aeabi_ddiv_result = __aeabi_dsub(__aeabi_ddiv_result, __aeabi_ddiv_result_2);
        return __aeabi_ddiv_result;
    }
    __aeabi_dadd_result = __aeabi_i2d(value_2);
    __aeabi_dmul_result = __aeabi_dmul(__aeabi_dadd_result, 0.6931471803691238);
branch_2:
    __aeabi_dadd_result = __aeabi_dmul(__aeabi_dadd_result, 1.9082149292705877e-10);
    __aeabi_dadd_result = __aeabi_dsub(__aeabi_ddiv_result_2, __aeabi_dadd_result);
    __aeabi_ddiv_result = __aeabi_dsub(__aeabi_dadd_result, __aeabi_ddiv_result);
    __aeabi_ddiv_result = __aeabi_dsub(__aeabi_dmul_result, __aeabi_ddiv_result);
    return __aeabi_ddiv_result;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2001ae30. */
int libm_fpclassify(double value)
{
    uint32_t value_1;
    int value_2;
    uint32_t value_3;

    value_1 = PICO_SUB_U32(value, 0);
    value_3 = (uint32_t)((uint64_t)value >> 0x20) & 0x7fffffff;
    if (((uint64_t)value & 0x7fffffff00000000) == 0) {
        if (value_1 == 0) {
            value_2 = 2;
        } else {
            value_2 = 3;
        }
    } else {
        if (value_3 - 0x100000 < 0x7fe00000) {
            return 4;
        }
        if (value_3 < 0x100000) {
            value_2 = 3;
        } else if (value_3 == 0x7ff00000) {
            value_2 = 1 - value_1;
            if (1 < value_1) {
                value_2 = 0;
            }
        } else {
            value_2 = 0;
        }
    }
    return value_2;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2001ae84. */
double libm_quiet_nan(void)
{
    return NAN;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20020b54. */
double __aeabi_dsub(double right, double right_2)
{
    int value_1;
    int value_2;
    double number_3;
    uint8_t byte_value_4;
    double number_5;
    uint32_t value_6;
    uint32_t value_7;
    int value_8;
    uint32_t value_9;
    uint32_t value_10;
    uint32_t value_11;
    int value_12;
    uint32_t value_13;
    uint32_t value_14;
    uint32_t value_15;
    uint32_t value_16;
    uint32_t value_17;
    bool condition_18;
    bool condition_19;

    value_14 = (uint32_t)((uint64_t)right_2 >> 0x20);
    value_8 = PICO_SUB_U32(right_2, 0);
    value_15 = (uint32_t)((uint64_t)right >> 0x20);
    value_12 = PICO_SUB_U32(right, 0);
    number_3 = -right_2;
    value_13 = value_15 << 1;
    condition_18 = ((value_15 ^ value_14) & 0x7fffffff) == 0;
    condition_19 = condition_18 && value_12 == value_8;
    if (!condition_18 || value_12 != value_8) {
        condition_19 = value_13 == 0 && value_12 == 0;
    }
    if (!condition_19) {
        condition_19 = value_14 << 1 == 0 && value_8 == 0;
    }
    value_1 = (int)value_13 >> 0x15;
    if (!condition_19) {
        condition_19 = value_1 == -1;
    }
    value_2 = (int)(value_14 << 1) >> 0x15;
    if (!condition_19) {
        condition_19 = value_2 == -1;
    }
    if (condition_19) {
        if (value_1 == -1 || value_2 == -1) {
            if (value_1 == -1) {
                number_3 = right;
            }
            value_15 = (uint32_t)((uint64_t)number_3 >> 0x20);
            value_12 = PICO_SUB_U32(number_3, 0);
            number_5 = -right_2;
            if (value_1 != -1 || value_2 != -1) {
                number_5 = number_3;
            }
            condition_19 = ((uint64_t)number_3 & 0xfffff00000000) == 0;
            condition_18 = value_12 == 0 && condition_19;
            if (value_12 == 0 && condition_19) {
                condition_18 =
                    PICO_SUB_U32(number_5, 0) == 0 && ((uint64_t)number_5 & 0xfffff00000000) == 0;
            }
            if (!condition_18 || value_15 != (uint32_t)((uint64_t)number_5 >> 0x20)) {
                value_15 = value_15 | 0x80000;
            }
            return (double)pico_join_u32_u32(value_15, value_12);
        }
        if (((value_15 ^ value_14) & 0x7fffffff) != 0 || value_12 != value_8) {
            if (value_13 == 0 && value_12 == 0) {
                right = -right_2;
            }
            return right;
        }
        if (value_15 != (value_14 ^ 0x80000000)) {
            return 0.0;
        }
        if ((value_15 & 0x7fffffff) >> 0x14 == 0) {
            condition_19 = ((uint64_t)right & 0x80000000) != 0;
            value_14 = value_15 * 2 + (uint32_t)condition_19;
            if (pico_carry_u32(value_15, value_15) ||
                pico_carry_u32(value_15 * 2, (uint32_t)condition_19)) {
                value_14 = value_14 | 0x80000000;
            }
            return (double)pico_join_u32_u32(value_14, value_12 << 1);
        }
        if (value_13 < 0xffc00000) {
            return (double)pico_join_u32_u32(value_15 + 0x100000, value_12);
        }
        value_15 = value_15 & 0x80000000;
    branch_1:
        return (double)((uint64_t)(value_15 | 0x7ff00000) << 0x20);
    }
    value_13 = (value_15 & 0x7fffffff) >> 0x14;
    value_15 = (value_14 & 0x7fffffff) >> 0x14;
    value_14 = value_15 - value_13;
    condition_19 = value_14 != 0;
    if (value_15 < value_13) {
        value_14 = -value_14;
    }
    number_3 = right;
    number_5 = -right_2;
    if (condition_19 && value_13 <= value_15) {
        value_13 = value_13 + value_14;
        number_3 = -right_2;
        number_5 = right;
    }
    value_9 = PICO_SUB_U32(number_5, 0);
    value_15 = PICO_SUB_U32(number_3, 0);
    if (0x36 < value_14) {
        return number_3;
    }
    value_7 = (uint32_t)((uint64_t)number_3 >> 0x20) & 0xfffff | 0x100000;
    if (((uint64_t)number_3 & 0x8000000000000000) != 0) {
        condition_19 = value_15 != 0;
        value_15 = -value_15;
        value_7 = -value_7 - (uint32_t)condition_19;
    }
    value_11 = (uint32_t)((uint64_t)number_5 >> 0x20) & 0xfffff | 0x100000;
    if (((uint64_t)number_5 & 0x8000000000000000) != 0) {
        condition_19 = value_9 != 0;
        value_9 = -value_9;
        value_11 = -value_11 - (uint32_t)condition_19;
    }
    if (value_13 == value_14) {
        value_11 = value_11 ^ 0x100000;
        if (value_13 == 0) {
            value_7 = value_7 ^ 0x100000;
            value_13 = 1;
        } else {
            value_14 = value_14 - 1;
        }
    }
    value_17 = -value_14 + 0x20;
    if ((int)value_14 < 0x21) {
        value_16 = value_9 << (value_17 & 0xff);
        value_9 = value_9 >> (value_14 & 0xff);
        value_6 = value_15 + value_9;
        value_10 = value_11 << (value_17 & 0xff);
        value_17 = value_6 + value_10;
        value_7 = value_7 + pico_carry_u32(value_15, value_9) +
                  ((int)value_11 >> (value_14 & 0xff)) +
                  (uint32_t)pico_carry_u32(value_6, value_10);
    } else {
        value_16 = value_11 << (-value_14 + 0x40 & 0xff);
        if (value_9 != 0) {
            value_16 = value_16 | 2;
        }
        value_11 = (int)value_11 >> (value_14 - 0x20 & 0xff);
        value_17 = value_15 + value_11;
        value_7 = value_7 + ((int)value_11 >> 0x1f) + (uint32_t)pico_carry_u32(value_15, value_11);
    }
    value_15 = value_7 & 0x80000000;
    value_14 = value_7;
    if ((int)value_7 < 0) {
        condition_19 = value_16 == 0;
        value_16 = -value_16;
        value_14 = -value_17;
        value_17 = -(uint32_t)!condition_19 - value_17;
        value_14 = -(uint32_t)(condition_19 <= value_14) - value_7;
    }
    if (0xfffff < value_14) {
        value_9 = value_13 - 1;
        if (0x1fffff < value_14) {
            value_9 = value_14 & 1;
            value_14 = value_14 >> 1;
            byte_value_4 = (uint8_t)value_17;
            value_17 = (uint32_t)(value_9 != 0) << 0x1f | value_17 >> 1;
            value_16 = (uint32_t)(byte_value_4 & 1) << 0x1f | value_16 >> 1;
            value_9 = value_13;
            if (0xffbfffff < value_13 << 0x15)
                goto branch_1;
        }
    branch_2:
        condition_19 = 0x7fffffff < value_16;
        if (value_16 == 0x80000000) {
            condition_19 = (value_17 & 1) != 0;
        }
        return (double)pico_join_u32_u32(
            value_14 + value_9 * 0x100000 +
                    (uint32_t)pico_carry_u32(value_17, (uint32_t)condition_19) |
                value_15,
            value_17 + condition_19);
    }
    condition_18 = (value_16 & 0x80000000) != 0;
    value_16 = value_16 << 1;
    value_9 = value_17 * 2;
    condition_19 = pico_carry_u32(value_17, value_17);
    value_17 = value_17 * 2 + (uint32_t)condition_18;
    value_14 =
        value_14 * 2 + (uint32_t)(condition_19 || pico_carry_u32(value_9, (uint32_t)condition_18));
    value_9 = value_13 - 2;
    if ((value_14 & 0x100000) != 0)
        goto branch_2;
    value_11 = value_17;
    value_13 = value_14;
    if (value_14 == 0) {
        value_11 = 0;
        value_13 = value_17;
    }
    value_12 = pico_count_leading_zeros_u32(value_13);
    if (value_14 == 0) {
        value_12 = value_12 + 0x20;
    }
    value_6 = value_12 - 0xb;
    condition_18 = pico_sub_overflow_i32(value_6, 0x20);
    value_14 = value_12 - 0x2b;
    condition_19 = value_14 == 0;
    value_17 = value_14;
    if ((int)value_6 < 0x20) {
        condition_18 = pico_add_overflow_i32(value_14, 0xc);
        value_17 = value_12 - 0x1f;
        condition_19 = value_17 == 0;
        if (!condition_19 && -0xd < (int)value_14) {
            value_11 = value_13 << (value_6 & 0xff);
            value_13 = value_13 >> (0xc - value_17 & 0xff);
            goto branch_3;
        }
        value_14 = value_6;
    }
    if (condition_19 || (int)value_17 < 0 != condition_18) {
        value_16 = 0x20 - value_14;
    }
    value_13 = value_13 << (value_14 & 0xff);
    if (condition_19 || (int)value_17 < 0 != condition_18) {
        value_13 = value_13 | value_11 >> (value_16 & 0xff);
        value_11 = value_11 << (value_14 & 0xff);
    }
branch_3:
    if ((int)value_6 <= (int)value_9) {
        return (double)pico_join_u32_u32(value_13 + (value_9 - value_6) * 0x100000 | value_15,
                                         value_11);
    }
    value_14 = ~(value_9 - value_6);
    if (0x1e < (int)value_14) {
        return (double)(pico_join_u32_u32(value_7, value_13 >> (value_14 - 0x1f & 0xff)) &
                        0x80000000ffffffff);
    }
    if (value_14 - 0x13 == 0 || (int)(value_14 - 0x1f) < -0xc) {
        value_14 = value_14 + 1;
        return (double)pico_join_u32_u32(value_15 | value_13 >> (value_14 & 0xff),
                                         value_11 >> (value_14 & 0xff) |
                                             value_13 << (0x20 - value_14 & 0xff));
    }
    value_15 = 0xc - (value_14 - 0x13);
    return (double)(pico_join_u32_u32(value_7, value_11 >> (0x20 - value_15 & 0xff) |
                                                   value_13 << (value_15 & 0xff)) &
                    0x80000000ffffffff);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20020b58. */
double __aeabi_dadd(double right, double right_2)
{
    int value_1;
    int value_2;
    double number_3;
    uint8_t byte_value_4;
    uint32_t value_5;
    uint32_t value_6;
    int value_7;
    uint32_t value_8;
    uint32_t value_9;
    uint32_t value_10;
    int value_11;
    uint32_t value_12;
    uint32_t value_13;
    uint32_t value_14;
    uint32_t value_15;
    uint32_t value_16;
    bool condition_17;
    bool condition_18;

    value_13 = (uint32_t)((uint64_t)right_2 >> 0x20);
    value_7 = PICO_SUB_U32(right_2, 0);
    value_14 = (uint32_t)((uint64_t)right >> 0x20);
    value_11 = PICO_SUB_U32(right, 0);
    value_12 = value_14 << 1;
    condition_17 = ((value_14 ^ value_13) & 0x7fffffff) == 0;
    condition_18 = condition_17 && value_11 == value_7;
    if (!condition_17 || value_11 != value_7) {
        condition_18 = value_12 == 0 && value_11 == 0;
    }
    if (!condition_18) {
        condition_18 = value_13 << 1 == 0 && value_7 == 0;
    }
    value_1 = (int)value_12 >> 0x15;
    if (!condition_18) {
        condition_18 = value_1 == -1;
    }
    value_2 = (int)(value_13 << 1) >> 0x15;
    if (!condition_18) {
        condition_18 = value_2 == -1;
    }
    if (condition_18) {
        if (value_1 == -1 || value_2 == -1) {
            number_3 = right_2;
            if (value_1 == -1) {
                number_3 = right;
            }
            value_14 = (uint32_t)((uint64_t)number_3 >> 0x20);
            value_11 = PICO_SUB_U32(number_3, 0);
            if (value_1 != -1 || value_2 != -1) {
                right_2 = number_3;
            }
            condition_18 = ((uint64_t)number_3 & 0xfffff00000000) == 0;
            condition_17 = value_11 == 0 && condition_18;
            if (value_11 == 0 && condition_18) {
                condition_17 =
                    PICO_SUB_U32(right_2, 0) == 0 && ((uint64_t)right_2 & 0xfffff00000000) == 0;
            }
            if (!condition_17 || value_14 != (uint32_t)((uint64_t)right_2 >> 0x20)) {
                value_14 = value_14 | 0x80000;
            }
            return (double)pico_join_u32_u32(value_14, value_11);
        }
        if (((value_14 ^ value_13) & 0x7fffffff) != 0 || value_11 != value_7) {
            if (value_12 == 0 && value_11 == 0) {
                right = right_2;
            }
            return right;
        }
        if (value_14 != value_13) {
            return 0.0;
        }
        if ((value_14 & 0x7fffffff) >> 0x14 == 0) {
            condition_18 = ((uint64_t)right & 0x80000000) != 0;
            value_13 = value_14 * 2 + (uint32_t)condition_18;
            if (pico_carry_u32(value_14, value_14) ||
                pico_carry_u32(value_14 * 2, (uint32_t)condition_18)) {
                value_13 = value_13 | 0x80000000;
            }
            return (double)pico_join_u32_u32(value_13, value_11 << 1);
        }
        if (value_12 < 0xffc00000) {
            return (double)pico_join_u32_u32(value_14 + 0x100000, value_11);
        }
        value_14 = value_14 & 0x80000000;
    branch_1:
        return (double)((uint64_t)(value_14 | 0x7ff00000) << 0x20);
    }
    value_12 = (value_14 & 0x7fffffff) >> 0x14;
    value_14 = (value_13 & 0x7fffffff) >> 0x14;
    value_13 = value_14 - value_12;
    condition_18 = value_13 != 0;
    if (value_14 < value_12) {
        value_13 = -value_13;
    }
    number_3 = right;
    if (condition_18 && value_12 <= value_14) {
        value_12 = value_12 + value_13;
        number_3 = right_2;
        right_2 = right;
    }
    value_8 = PICO_SUB_U32(right_2, 0);
    value_14 = PICO_SUB_U32(number_3, 0);
    if (0x36 < value_13) {
        return number_3;
    }
    value_6 = (uint32_t)((uint64_t)number_3 >> 0x20) & 0xfffff | 0x100000;
    if (((uint64_t)number_3 & 0x8000000000000000) != 0) {
        condition_18 = value_14 != 0;
        value_14 = -value_14;
        value_6 = -value_6 - (uint32_t)condition_18;
    }
    value_10 = (uint32_t)((uint64_t)right_2 >> 0x20) & 0xfffff | 0x100000;
    if (((uint64_t)right_2 & 0x8000000000000000) != 0) {
        condition_18 = value_8 != 0;
        value_8 = -value_8;
        value_10 = -value_10 - (uint32_t)condition_18;
    }
    if (value_12 == value_13) {
        value_10 = value_10 ^ 0x100000;
        if (value_12 == 0) {
            value_6 = value_6 ^ 0x100000;
            value_12 = 1;
        } else {
            value_13 = value_13 - 1;
        }
    }
    value_16 = -value_13 + 0x20;
    if ((int)value_13 < 0x21) {
        value_15 = value_8 << (value_16 & 0xff);
        value_8 = value_8 >> (value_13 & 0xff);
        value_5 = value_14 + value_8;
        value_9 = value_10 << (value_16 & 0xff);
        value_16 = value_5 + value_9;
        value_6 = value_6 + pico_carry_u32(value_14, value_8) +
                  ((int)value_10 >> (value_13 & 0xff)) + (uint32_t)pico_carry_u32(value_5, value_9);
    } else {
        value_15 = value_10 << (-value_13 + 0x40 & 0xff);
        if (value_8 != 0) {
            value_15 = value_15 | 2;
        }
        value_10 = (int)value_10 >> (value_13 - 0x20 & 0xff);
        value_16 = value_14 + value_10;
        value_6 = value_6 + ((int)value_10 >> 0x1f) + (uint32_t)pico_carry_u32(value_14, value_10);
    }
    value_14 = value_6 & 0x80000000;
    value_13 = value_6;
    if ((int)value_6 < 0) {
        condition_18 = value_15 == 0;
        value_15 = -value_15;
        value_13 = -value_16;
        value_16 = -(uint32_t)!condition_18 - value_16;
        value_13 = -(uint32_t)(condition_18 <= value_13) - value_6;
    }
    if (0xfffff < value_13) {
        value_8 = value_12 - 1;
        if (0x1fffff < value_13) {
            value_8 = value_13 & 1;
            value_13 = value_13 >> 1;
            byte_value_4 = (uint8_t)value_16;
            value_16 = (uint32_t)(value_8 != 0) << 0x1f | value_16 >> 1;
            value_15 = (uint32_t)(byte_value_4 & 1) << 0x1f | value_15 >> 1;
            value_8 = value_12;
            if (0xffbfffff < value_12 << 0x15)
                goto branch_1;
        }
    branch_2:
        condition_18 = 0x7fffffff < value_15;
        if (value_15 == 0x80000000) {
            condition_18 = (value_16 & 1) != 0;
        }
        return (double)pico_join_u32_u32(
            value_13 + value_8 * 0x100000 +
                    (uint32_t)pico_carry_u32(value_16, (uint32_t)condition_18) |
                value_14,
            value_16 + condition_18);
    }
    condition_17 = (value_15 & 0x80000000) != 0;
    value_15 = value_15 << 1;
    value_8 = value_16 * 2;
    condition_18 = pico_carry_u32(value_16, value_16);
    value_16 = value_16 * 2 + (uint32_t)condition_17;
    value_13 =
        value_13 * 2 + (uint32_t)(condition_18 || pico_carry_u32(value_8, (uint32_t)condition_17));
    value_8 = value_12 - 2;
    if ((value_13 & 0x100000) != 0)
        goto branch_2;
    value_10 = value_16;
    value_12 = value_13;
    if (value_13 == 0) {
        value_10 = 0;
        value_12 = value_16;
    }
    value_11 = pico_count_leading_zeros_u32(value_12);
    if (value_13 == 0) {
        value_11 = value_11 + 0x20;
    }
    value_5 = value_11 - 0xb;
    condition_17 = pico_sub_overflow_i32(value_5, 0x20);
    value_13 = value_11 - 0x2b;
    condition_18 = value_13 == 0;
    value_16 = value_13;
    if ((int)value_5 < 0x20) {
        condition_17 = pico_add_overflow_i32(value_13, 0xc);
        value_16 = value_11 - 0x1f;
        condition_18 = value_16 == 0;
        if (!condition_18 && -0xd < (int)value_13) {
            value_10 = value_12 << (value_5 & 0xff);
            value_12 = value_12 >> (0xc - value_16 & 0xff);
            goto branch_3;
        }
        value_13 = value_5;
    }
    if (condition_18 || (int)value_16 < 0 != condition_17) {
        value_15 = 0x20 - value_13;
    }
    value_12 = value_12 << (value_13 & 0xff);
    if (condition_18 || (int)value_16 < 0 != condition_17) {
        value_12 = value_12 | value_10 >> (value_15 & 0xff);
        value_10 = value_10 << (value_13 & 0xff);
    }
branch_3:
    if ((int)value_5 <= (int)value_8) {
        return (double)pico_join_u32_u32(value_12 + (value_8 - value_5) * 0x100000 | value_14,
                                         value_10);
    }
    value_13 = ~(value_8 - value_5);
    if (0x1e < (int)value_13) {
        return (double)(pico_join_u32_u32(value_6, value_12 >> (value_13 - 0x1f & 0xff)) &
                        0x80000000ffffffff);
    }
    if (value_13 - 0x13 == 0 || (int)(value_13 - 0x1f) < -0xc) {
        value_13 = value_13 + 1;
        return (double)pico_join_u32_u32(value_14 | value_12 >> (value_13 & 0xff),
                                         value_10 >> (value_13 & 0xff) |
                                             value_12 << (0x20 - value_13 & 0xff));
    }
    value_14 = 0xc - (value_13 - 0x13);
    return (double)(pico_join_u32_u32(value_6, value_10 >> (0x20 - value_14 & 0xff) |
                                                   value_12 << (value_14 & 0xff)) &
                    0x80000000ffffffff);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20020dd0. */
double __aeabi_ui2d(uint32_t value)
{
    uint32_t value_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    int value_5;
    uint32_t value_6;
    uint32_t unresolved_input_r12;
    bool condition_7;
    bool condition_8;

    if (value == 0) {
        return 0.0;
    }
    value_1 = 0;
    value_5 = pico_count_leading_zeros_u32(value);
    value_6 = value_5 + 0x15;
    condition_8 = pico_sub_overflow_i32(value_6, 0x20);
    value_3 = value_5 - 0xb;
    condition_7 = value_3 == 0;
    value_4 = value_3;
    if (value_6 < 0x20) {
        condition_8 = pico_add_overflow_i32(value_3, 0xc);
        value_4 = value_5 + 1;
        condition_7 = value_4 == 0;
        if (!condition_7 && -0xd < (int)value_3) {
            value_1 = value << value_6;
            value_2 = value >> (0xc - value_4 & 0xff);
            goto branch_1;
        }
        value_3 = value_6;
    }
    if (condition_7 || (int)value_4 < 0 != condition_8) {
        unresolved_input_r12 = 0x20 - value_3;
    }
    value_2 = value << (value_3 & 0xff);
    if (condition_7 || (int)value_4 < 0 != condition_8) {
        value_2 = value_2 | 0U >> (unresolved_input_r12 & 0xff);
        value_1 = 0 << (value_3 & 0xff);
    }
branch_1:
    if (value_6 < 0x433) {
        return (double)pico_join_u32_u32(value_2 + (0x432 - value_6) * 0x100000, value_1);
    }
    value_3 = ~(0x432 - value_6);
    if (0x1e < (int)value_3) {
        return (double)(uint64_t)(value_2 >> (value_3 - 0x1f & 0xff));
    }
    if (value_3 - 0x13 == 0 || (int)(value_3 - 0x1f) < -0xc) {
        value_3 = value_3 + 1;
        return (double)pico_join_u32_u32(value_2 >> (value_3 & 0xff),
                                         value_1 >> (value_3 & 0xff) |
                                             value_2 << (0x20 - value_3 & 0xff));
    }
    value_3 = 0xc - (value_3 - 0x13);
    return (double)(uint64_t)(value_1 >> (0x20 - value_3 & 0xff) | value_2 << (value_3 & 0xff));
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20020df0. */
double __aeabi_i2d(int value)
{
    uint32_t value_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    int value_5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t unresolved_input_r12;
    bool condition_8;
    bool condition_9;

    if (value == 0) {
        return 0.0;
    }
    value_7 = value & 0x80000000;
    value_2 = value;
    if ((int)value_7 < 0) {
        value_2 = -value;
    }
    value_1 = 0;
    value_5 = pico_count_leading_zeros_u32(value_2);
    value_6 = value_5 + 0x15;
    condition_9 = pico_sub_overflow_i32(value_6, 0x20);
    value_3 = value_5 - 0xb;
    condition_8 = value_3 == 0;
    value_4 = value_3;
    if (value_6 < 0x20) {
        condition_9 = pico_add_overflow_i32(value_3, 0xc);
        value_4 = value_5 + 1;
        condition_8 = value_4 == 0;
        if (!condition_8 && -0xd < (int)value_3) {
            value_1 = value_2 << value_6;
            value_2 = value_2 >> (0xc - value_4 & 0xff);
            goto branch_1;
        }
        value_3 = value_6;
    }
    if (condition_8 || (int)value_4 < 0 != condition_9) {
        unresolved_input_r12 = 0x20 - value_3;
    }
    value_2 = value_2 << (value_3 & 0xff);
    if (condition_8 || (int)value_4 < 0 != condition_9) {
        value_2 = value_2 | 0U >> (unresolved_input_r12 & 0xff);
        value_1 = 0 << (value_3 & 0xff);
    }
branch_1:
    if (value_6 < 0x433) {
        return (double)pico_join_u32_u32(value_2 + (0x432 - value_6) * 0x100000 | value_7, value_1);
    }
    value_3 = ~(0x432 - value_6);
    if (0x1e < (int)value_3) {
        return (double)(pico_join_u32_u32(value, value_2 >> (value_3 - 0x1f & 0xff)) &
                        0x80000000ffffffff);
    }
    if (value_3 - 0x13 == 0 || (int)(value_3 - 0x1f) < -0xc) {
        value_3 = value_3 + 1;
        return (double)pico_join_u32_u32(value_7 | value_2 >> (value_3 & 0xff),
                                         value_1 >> (value_3 & 0xff) |
                                             value_2 << (0x20 - value_3 & 0xff));
    }
    value_7 = 0xc - (value_3 - 0x13);
    return (double)(pico_join_u32_u32(value, value_1 >> (0x20 - value_7 & 0xff) |
                                                 value_2 << (value_7 & 0xff)) &
                    0x80000000ffffffff);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20020e14. */
double __aeabi_f2d(float value)
{
    uint32_t value_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t value_5;
    int value_6;
    uint32_t value_7;
    uint32_t unresolved_input_r3;
    uint32_t value_8;
    uint32_t unresolved_input_r12;
    bool condition_9;
    bool condition_10;

    value_5 = (int)value << 1;
    condition_9 = value_5 == 0;
    value_8 = (uint32_t)(((uint32_t)value & 0x80000000) != 0) << 0x1f;
    value_4 = (uint32_t)((int)value_5 >> 3) >> 1;
    value_3 = value_8 | value_4;
    value_2 = (int)value << 0x1d;
    if (!condition_9) {
        unresolved_input_r3 = value_5 & 0xff000000;
        condition_9 = unresolved_input_r3 == 0;
    }
    if (!condition_9) {
        condition_9 = unresolved_input_r3 == 0xff000000;
    }
    if (!condition_9) {
        return (double)(pico_join_u32_u32(value_3, value_2) ^ 0x3800000000000000);
    }
    if (value_5 == 0 || unresolved_input_r3 == 0xff000000) {
        return (double)pico_join_u32_u32(value_3, value_2);
    }
    value_1 = value_2;
    value_5 = value_4;
    if (value_4 == 0) {
        value_1 = 0;
        value_5 = value_2;
    }
    value_6 = pico_count_leading_zeros_u32(value_5);
    if (value_4 == 0) {
        value_6 = value_6 + 0x20;
    }
    value_7 = value_6 - 0xb;
    condition_10 = pico_sub_overflow_i32(value_7, 0x20);
    value_4 = value_6 - 0x2b;
    condition_9 = value_4 == 0;
    value_2 = value_4;
    if ((int)value_7 < 0x20) {
        condition_10 = pico_add_overflow_i32(value_4, 0xc);
        value_2 = value_6 - 0x1f;
        condition_9 = value_2 == 0;
        if (!condition_9 && -0xd < (int)value_4) {
            value_1 = value_5 << (value_7 & 0xff);
            value_5 = value_5 >> (0xc - value_2 & 0xff);
            goto branch_1;
        }
        value_4 = value_7;
    }
    if (condition_9 || (int)value_2 < 0 != condition_10) {
        unresolved_input_r12 = 0x20 - value_4;
    }
    value_5 = value_5 << (value_4 & 0xff);
    if (condition_9 || (int)value_2 < 0 != condition_10) {
        value_5 = value_5 | value_1 >> (unresolved_input_r12 & 0xff);
        value_1 = value_1 << (value_4 & 0xff);
    }
branch_1:
    if ((int)value_7 < 0x381) {
        return (double)pico_join_u32_u32(value_5 + (0x380 - value_7) * 0x100000 | value_8, value_1);
    }
    value_4 = ~(0x380 - value_7);
    if (0x1e < (int)value_4) {
        return (double)(pico_join_u32_u32(value_3, value_5 >> (value_4 - 0x1f & 0xff)) &
                        0x80000000ffffffff);
    }
    if (value_4 - 0x13 == 0 || (int)(value_4 - 0x1f) < -0xc) {
        value_4 = value_4 + 1;
        return (double)pico_join_u32_u32(value_8 | value_5 >> (value_4 & 0xff),
                                         value_1 >> (value_4 & 0xff) |
                                             value_5 << (0x20 - value_4 & 0xff));
    }
    value_8 = 0xc - (value_4 - 0x13);
    return (double)(pico_join_u32_u32(value_3, value_1 >> (0x20 - value_8 & 0xff) |
                                                   value_5 << (value_8 & 0xff)) &
                    0x80000000ffffffff);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20020ebc. */
double __aeabi_dmul(double left, double right)
{
    uint64_t value_1;
    uint64_t value_2;
    int64_t value_3;
    uint32_t value_4;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;
    uint32_t value_5;
    uint32_t value_6;
    int value_7;
    uint32_t value_8;
    uint32_t unresolved_saved_r5;
    uint32_t value_9;
    uint32_t value_10;
    uint32_t unresolved_return_r12;
    uint32_t value_11;
    bool condition_12;
    bool condition_13;
    bool condition_14;

    value_4 = (uint32_t)((uint64_t)right >> 0x20);
    value_9 = (uint32_t)((uint64_t)left >> 0x20);
    value_10 = 0x7ff;
    value_6 = value_9 >> 0x14 & 0x7ff;
    condition_12 = value_6 == 0;
    if (!condition_12) {
        unresolved_saved_r5 = value_4 >> 0x14 & 0x7ff;
        condition_12 = unresolved_saved_r5 == 0;
    }
    if (!condition_12) {
        condition_12 = value_6 == 0x7ff;
    }
    if (!condition_12) {
        condition_12 = unresolved_saved_r5 == 0x7ff;
    }
    if (condition_12) {
        left = (double)soft_double_multiply_special(PICO_SUB_U32(left, 0), value_9,
                                                    PICO_SUB_U32(right, 0), value_4);
        right = (double)pico_join_u32_u32(unresolved_return_r3, unresolved_return_r2);
        value_10 = unresolved_return_r12;
    }
    value_5 = (uint32_t)((uint64_t)right >> 0x20);
    value_4 = (uint32_t)((uint64_t)left >> 0x20);
    value_9 = PICO_SUB_U32(left, 0);
    value_7 = value_6 + unresolved_saved_r5;
    value_6 = value_4 ^ value_5;
    value_4 = value_4 & ~(value_10 << 0x15);
    value_5 = value_5 & ~(value_10 << 0x15);
    condition_12 = (value_4 & 0xfffff) == 0;
    condition_13 = value_9 == 0 && condition_12;
    if (value_9 != 0 || !condition_12) {
        condition_13 = PICO_SUB_U32(right, 0) == 0 && (value_5 & 0xfffff) == 0;
    }
    value_4 = value_4 | 0x100000;
    value_5 = value_5 | 0x100000;
    if (condition_13) {
        value_9 = value_9 | PICO_SUB_U32(right, 0);
        value_5 = (value_6 & 0x80000000 | value_4) ^ value_5;
        value_4 = value_10 >> 1;
        condition_13 = pico_sub_overflow_i32(value_7, value_4);
        value_8 = value_7 - value_4;
        condition_12 = value_8 == 0;
        value_6 = value_8;
        if (!condition_12 && (int)value_4 <= value_7) {
            condition_13 = pico_sub_overflow_i32(value_10, value_8);
            value_6 = value_10 - value_8;
            condition_12 = value_10 == value_8;
        }
        if (!condition_12 && (int)value_6 < 0 == condition_13) {
            return (double)pico_join_u32_u32(value_5 | value_8 * 0x100000, value_9);
        }
        value_5 = value_5 | 0x100000;
        value_4 = 0;
        condition_13 = pico_sub_overflow_i32(value_8, 1);
        value_8 = value_8 - 1;
        condition_12 = value_8 == 0;
        value_10 = value_8;
    } else {
        value_1 = ((uint64_t)left & 0xffffffff) * ((uint64_t)right & 0xffffffff);
        value_2 = ((uint64_t)left & 0xffffffff) * (uint64_t)value_5 +
                  (uint64_t)value_4 * ((uint64_t)right & 0xffffffff) + (value_1 >> 0x20);
        value_11 = (uint32_t)value_2;
        value_3 = (uint64_t)value_4 * (uint64_t)value_5 + (value_2 >> 0x20);
        value_10 = (uint32_t)value_3;
        value_9 = (uint32_t)((uint64_t)value_3 >> 0x20);
        if ((int)value_1 != 0) {
            value_11 = value_11 | 1;
        }
        value_8 = (value_7 + -0x3ff) - (uint32_t)(value_9 < 0x200);
        if (value_9 < 0x200) {
            condition_12 = (value_11 & 0x80000000) != 0;
            value_11 = value_11 << 1;
            value_3 = pico_join_u32_u32(
                value_9 * 2 + (uint32_t)(pico_carry_u32(value_10, value_10) ||
                                         pico_carry_u32(value_10 * 2, (uint32_t)condition_12)),
                value_10 * 2 + (uint32_t)condition_12);
        }
        value_5 = value_6 & 0x80000000 | (int)((uint64_t)value_3 >> 0x20) << 0xb |
                  (uint32_t)value_3 >> 0x15;
        value_9 = (uint32_t)value_3 << 0xb | value_11 >> 0x15;
        value_4 = value_11 * 0x800;
        condition_14 = 0xfc < value_8;
        condition_13 = pico_sub_overflow_i32(value_8, 0xfd);
        value_6 = value_8 - 0xfd;
        condition_12 = value_6 == 0;
        value_10 = value_6;
        if (condition_14 && !condition_12) {
            condition_14 = 0x6ff < value_6;
            condition_13 = pico_sub_overflow_i32(value_6, 0x700);
            value_10 = value_8 - 0x7fd;
            condition_12 = value_6 == 0x700;
        }
        if (!condition_14 || condition_12) {
            condition_12 = 0x7fffffff < value_4;
            if (value_4 == 0x80000000) {
                condition_12 = (value_11 >> 0x15 & 1) != 0;
            }
            return (double)pico_join_u32_u32(
                value_5 + value_8 * 0x100000 +
                    (uint32_t)pico_carry_u32(value_9, (uint32_t)condition_12),
                value_9 + condition_12);
        }
    }
    if (!condition_12 && (int)value_10 < 0 == condition_13) {
        return (double)((uint64_t)(value_5 & 0x80000000 | 0x7ff00000) << 0x20);
    }
    if ((int)value_8 < -0x35) {
        return (double)((uint64_t)(value_5 & 0x80000000) << 0x20);
    }
    value_10 = -value_8;
    value_6 = value_10 - 0x20;
    if (0x1f < (int)value_10) {
        value_8 = value_9 >> (value_6 & 0xff) | value_5 << (0x20 - value_6 & 0xff);
        value_10 = (value_5 >> (value_6 & 0xff) & ~((value_5 & 0x80000000) >> (value_6 & 0xff))) -
                   ((int)value_8 >> 0x1f);
        if ((value_4 == 0 && value_9 << (0x20 - value_6 & 0xff) == 0) &&
            (value_8 & 0x7fffffff) == 0) {
            value_10 = value_10 & ~(value_8 >> 0x1f);
        }
        return (double)(pico_join_u32_u32(value_5, value_10) & 0x80000000ffffffff);
    }
    if (value_10 - 0x14 == 0 || (int)value_6 < -0xc) {
        value_11 = value_9 << (value_8 + 0x20 & 0xff);
        value_9 = value_9 >> (value_10 & 0xff) | value_5 << (value_8 + 0x20 & 0xff);
        value_6 = value_9 + -((int)value_11 >> 0x1f);
        if (value_4 == 0 && (value_11 & 0x7fffffff) == 0) {
            value_6 = value_6 & ~(value_11 >> 0x1f);
        }
        return (double)pico_join_u32_u32(
            (value_5 & 0x80000000) + ((value_5 & 0x7fffffff) >> (value_10 & 0xff)) +
                (uint32_t)pico_carry_u32(value_9, -((int)value_11 >> 0x1f)),
            value_6);
    }
    value_10 = 0xc - (value_10 - 0x14);
    value_6 = value_9 << (value_10 & 0xff);
    value_10 = value_9 >> (0x20 - value_10 & 0xff) | value_5 << (value_10 & 0xff);
    value_9 = value_10 + -((int)value_6 >> 0x1f);
    if (value_4 == 0 && (value_6 & 0x7fffffff) == 0) {
        value_9 = value_9 & ~(value_6 >> 0x1f);
    }
    return (double)pico_join_u32_u32(
        (value_5 & 0x80000000) + (uint32_t)pico_carry_u32(value_10, -((int)value_6 >> 0x1f)),
        value_9);
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x20021098. */
uint64_t soft_double_multiply_special(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                      uint32_t argument_4)
{
    bool condition_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t unresolved_saved_r4;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t unresolved_input_r12;
    bool condition_6;

    value_4 = unresolved_input_r12 & argument_4 >> 0x14;
    if (unresolved_saved_r4 == unresolved_input_r12 || value_4 == unresolved_input_r12) {
        condition_1 = (argument_2 & 0x7fffffff) == 0;
        condition_6 = argument_1 == 0 && condition_1;
        value_5 = argument_4;
        value_2 = argument_3;
        if (argument_1 != 0 || !condition_1) {
            condition_6 = argument_3 == 0 && (argument_4 & 0x7fffffff) == 0;
            value_5 = argument_2;
            value_2 = argument_1;
        }
        value_3 = value_5;
        if (((condition_6) || ((unresolved_saved_r4 == unresolved_input_r12 &&
                                (value_2 != 0 || (value_5 & 0xfffff) != 0)))) ||
            ((value_4 == unresolved_input_r12 &&
              (value_2 = argument_3, value_3 = argument_4,
               argument_3 != 0 || (argument_4 & 0xfffff) != 0)))) {
            return pico_join_u32_u32(value_3, value_2) | 0x7ff8000000000000;
        }
        return (uint64_t)((value_5 ^ argument_4) & 0x80000000 | 0x7ff00000) << 0x20;
    }
    condition_1 = (argument_2 & 0x7fffffff) == 0;
    condition_6 = argument_1 == 0 && condition_1;
    if (argument_1 != 0 || !condition_1) {
        condition_6 = argument_3 == 0 && (argument_4 & 0x7fffffff) == 0;
    }
    if (condition_6) {
        return (uint64_t)((argument_2 ^ argument_4) & 0x80000000) << 0x20;
    }
    if (unresolved_saved_r4 == 0) {
        value_5 = argument_2 & 0x80000000;
        do {
            value_2 = argument_1 & 0x80000000;
            argument_1 = argument_1 << 1;
            argument_2 = argument_2 * 2 + (uint32_t)(value_2 != 0);
        } while ((argument_2 & 0x100000) == 0);
        argument_2 = argument_2 | value_5;
        if (value_4 != 0) {
            return pico_join_u32_u32(argument_2, argument_1);
        }
    }
    do {
        value_4 = argument_3 & 0x80000000;
        argument_3 = argument_3 << 1;
        argument_4 = argument_4 * 2 + (uint32_t)(value_4 != 0);
    } while ((argument_4 & 0x100000) == 0);
    return pico_join_u32_u32(argument_2, argument_1);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021110. */
double __aeabi_ddiv(double left, double right)
{
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;
    uint32_t value_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    int value_5;
    uint32_t unresolved_saved_r5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t value_8;
    uint32_t value_9;
    uint32_t value_10;
    uint32_t unresolved_return_r12;
    uint32_t value_11;
    bool condition_12;
    bool condition_13;
    bool condition_14;

    value_11 = (uint32_t)((uint64_t)right >> 0x20);
    value_8 = (uint32_t)((uint64_t)left >> 0x20);
    value_10 = 0x7ff;
    value_4 = value_8 >> 0x14 & 0x7ff;
    condition_12 = value_4 == 0;
    if (!condition_12) {
        unresolved_saved_r5 = value_11 >> 0x14 & 0x7ff;
        condition_12 = unresolved_saved_r5 == 0;
    }
    if (!condition_12) {
        condition_12 = value_4 == 0x7ff;
    }
    if (!condition_12) {
        condition_12 = unresolved_saved_r5 == 0x7ff;
    }
    if (condition_12) {
        left = (double)soft_double_divide_special(PICO_SUB_U32(left, 0), value_8,
                                                  PICO_SUB_U32(right, 0), value_11);
        right = (double)pico_join_u32_u32(unresolved_return_r3, unresolved_return_r2);
        value_10 = unresolved_return_r12;
    }
    value_1 = (uint32_t)((uint64_t)right >> 0x20);
    value_7 = PICO_SUB_U32(right, 0);
    value_6 = (uint32_t)((uint64_t)left >> 0x20);
    value_8 = PICO_SUB_U32(left, 0);
    value_5 = value_4 - unresolved_saved_r5;
    value_11 = value_6 ^ value_1;
    if (value_7 == 0 && ((uint64_t)right & 0xfffff00000000) == 0) {
        value_11 = value_11 & 0x80000000 | value_6 & 0xfffff;
        condition_13 = pico_add_overflow_i32(value_5, value_10 >> 1);
        value_4 = value_5 + (value_10 >> 1);
        condition_12 = (int)value_4 < 0;
        condition_14 = value_4 == 0;
        if (!condition_14 && condition_12 == condition_13) {
            condition_13 = pico_sub_overflow_i32(value_10, value_4);
            condition_12 = (int)(value_10 - value_4) < 0;
            condition_14 = value_10 == value_4;
        }
        if (!condition_14 && condition_12 == condition_13) {
            return (double)pico_join_u32_u32(value_11 | value_4 * 0x100000, value_8);
        }
        value_11 = value_11 | 0x100000;
        value_6 = 0;
        condition_14 = pico_sub_overflow_i32(value_4, 1);
        value_4 = value_4 - 1;
        condition_12 = value_4 == 0;
        value_10 = value_4;
    } else {
        value_1 = (value_1 & 0xfffff) << 8 | 0x10000000 | value_7 >> 0x18;
        value_10 = value_7 << 8;
        value_6 = (value_6 & 0xfffff) << 8 | 0x10000000 | value_8 >> 0x18;
        value_8 = value_8 * 0x100;
        value_11 = value_11 & 0x80000000;
        condition_12 = value_1 <= value_6;
        if (value_6 == value_1) {
            condition_12 = value_10 <= value_8;
        }
        value_5 = value_5 + (uint32_t)condition_12;
        value_4 = value_5 + 0x3fd;
        if (condition_12 == false) {
            value_1 = value_1 >> 1;
            value_10 = (uint32_t)((value_7 >> 0x18 & 1) != 0) << 0x1f | (value_7 & 0xffffff) << 7;
        }
        value_9 = value_8 - value_10;
        value_7 = (value_6 - value_1) - (uint32_t)(value_8 < value_10);
        value_2 = value_1 >> 1;
        value_1 = (uint32_t)((value_1 & 1) != 0) << 0x1f | value_10 >> 1;
        value_8 = 0x100000;
        value_10 = 0x80000;
        while (true) {
            condition_12 = value_1 <= value_9;
            if (value_2 < value_7 || value_7 - value_2 < (uint32_t)condition_12) {
                value_9 = value_9 - value_1;
                value_8 = value_8 | value_10;
                value_7 = (value_7 - value_2) - (uint32_t)!condition_12;
            }
            value_3 = value_2 >> 1;
            value_6 = (uint32_t)((value_2 & 1) != 0) << 0x1f | value_1 >> 1;
            condition_12 = value_6 <= value_9;
            if (value_3 < value_7 || value_7 - value_3 < (uint32_t)condition_12) {
                value_9 = value_9 - value_6;
                value_8 = value_8 | value_10 >> 1;
                value_7 = (value_7 - value_3) - (uint32_t)!condition_12;
            }
            value_1 = value_2 >> 2;
            value_3 = (uint32_t)((value_3 & 1) != 0) << 0x1f | value_6 >> 1;
            condition_12 = value_3 <= value_9;
            if (value_1 < value_7 || value_7 - value_1 < (uint32_t)condition_12) {
                value_9 = value_9 - value_3;
                value_8 = value_8 | value_10 >> 2;
                value_7 = (value_7 - value_1) - (uint32_t)!condition_12;
            }
            value_2 = value_2 >> 3;
            value_1 = (uint32_t)((value_1 & 1) != 0) << 0x1f | value_3 >> 1;
            condition_12 = value_1 <= value_9;
            if (value_2 < value_7 || value_7 - value_2 < (uint32_t)condition_12) {
                value_9 = value_9 - value_1;
                value_8 = value_8 | value_10 >> 3;
                value_7 = (value_7 - value_2) - (uint32_t)!condition_12;
            }
            value_6 = value_7 | value_9;
            if (value_6 == 0)
                break;
            value_7 = value_7 << 4 | value_9 >> 0x1c;
            value_9 = value_9 << 4;
            value_2 = value_2 << 3 | value_1 >> 0x1d;
            value_1 = (value_3 >> 1) << 3;
            value_10 = value_10 >> 4;
            if (value_10 == 0) {
                if ((value_11 & 0x100000) != 0)
                    goto branch_1;
                value_11 = value_11 | value_8;
                value_8 = 0;
                value_10 = 0x80000000;
            }
        }
        if ((value_11 & 0x100000) == 0) {
            value_11 = value_11 | value_8;
            value_8 = 0;
        }
    branch_1:
        condition_13 = 0xfc < value_4;
        condition_14 = pico_sub_overflow_i32(value_4, 0xfd);
        value_3 = value_5 + 0x300;
        condition_12 = value_3 == 0;
        value_10 = value_3;
        if (condition_13 && !condition_12) {
            condition_13 = 0x6ff < value_3;
            condition_14 = pico_sub_overflow_i32(value_3, 0x700);
            value_10 = value_5 - 0x400;
            condition_12 = value_3 == 0x700;
        }
        if (!condition_13 || condition_12) {
            condition_12 = value_2 <= value_7;
            if (value_7 == value_2) {
                condition_12 = value_1 <= value_9;
            }
            if (value_7 == value_2 && value_9 == value_1) {
                condition_12 = (value_8 & 1) != 0;
            }
            return (double)pico_join_u32_u32(
                value_11 + value_4 * 0x100000 +
                    (uint32_t)pico_carry_u32(value_8, (uint32_t)condition_12),
                value_8 + condition_12);
        }
    }
    if (!condition_12 && (int)value_10 < 0 == condition_14) {
        return (double)((uint64_t)(value_11 & 0x80000000 | 0x7ff00000) << 0x20);
    }
    if ((int)value_4 < -0x35) {
        return (double)((uint64_t)(value_11 & 0x80000000) << 0x20);
    }
    value_10 = -value_4;
    value_7 = value_10 - 0x20;
    if (0x1f < (int)value_10) {
        value_4 = value_8 >> (value_7 & 0xff) | value_11 << (0x20 - value_7 & 0xff);
        value_10 = (value_11 >> (value_7 & 0xff) & ~((value_11 & 0x80000000) >> (value_7 & 0xff))) -
                   ((int)value_4 >> 0x1f);
        if ((value_6 == 0 && value_8 << (0x20 - value_7 & 0xff) == 0) &&
            (value_4 & 0x7fffffff) == 0) {
            value_10 = value_10 & ~(value_4 >> 0x1f);
        }
        return (double)(pico_join_u32_u32(value_11, value_10) & 0x80000000ffffffff);
    }
    if (value_10 - 0x14 == 0 || (int)value_7 < -0xc) {
        value_7 = value_8 << (value_4 + 0x20 & 0xff);
        value_8 = value_8 >> (value_10 & 0xff) | value_11 << (value_4 + 0x20 & 0xff);
        value_4 = value_8 + -((int)value_7 >> 0x1f);
        if (value_6 == 0 && (value_7 & 0x7fffffff) == 0) {
            value_4 = value_4 & ~(value_7 >> 0x1f);
        }
        return (double)pico_join_u32_u32(
            (value_11 & 0x80000000) + ((value_11 & 0x7fffffff) >> (value_10 & 0xff)) +
                (uint32_t)pico_carry_u32(value_8, -((int)value_7 >> 0x1f)),
            value_4);
    }
    value_10 = 0xc - (value_10 - 0x14);
    value_4 = value_8 << (value_10 & 0xff);
    value_10 = value_8 >> (0x20 - value_10 & 0xff) | value_11 << (value_10 & 0xff);
    value_8 = value_10 + -((int)value_4 >> 0x1f);
    if (value_6 == 0 && (value_4 & 0x7fffffff) == 0) {
        value_8 = value_8 & ~(value_4 >> 0x1f);
    }
    return (double)pico_join_u32_u32(
        (value_11 & 0x80000000) + (uint32_t)pico_carry_u32(value_10, -((int)value_4 >> 0x1f)),
        value_8);
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x2002127e. */
uint64_t soft_double_divide_special(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                    uint32_t argument_4)
{
    uint32_t value_1;
    bool condition_2;
    uint32_t unresolved_saved_r4;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t unresolved_input_r12;
    bool condition_5;

    value_3 = unresolved_input_r12 & argument_4 >> 0x14;
    if (unresolved_saved_r4 != unresolved_input_r12 || value_3 != unresolved_input_r12) {
        if (unresolved_saved_r4 == unresolved_input_r12) {
            if (argument_1 == 0 && (argument_2 & 0xfffff) == 0) {
                if (value_3 != unresolved_input_r12) {
                branch_1:
                    return (uint64_t)((argument_2 ^ argument_4) & 0x80000000 | 0x7ff00000) << 0x20;
                }
                argument_1 = argument_3;
                argument_2 = argument_4;
            }
        } else if (value_3 == unresolved_input_r12) {
            if (argument_3 == 0 && (argument_4 & 0xfffff) == 0) {
            branch_2:
                return (uint64_t)((argument_2 ^ argument_4) & 0x80000000) << 0x20;
            }
            argument_1 = argument_3;
            argument_2 = argument_4;
        } else {
            condition_2 = (argument_2 & 0x7fffffff) == 0;
            condition_5 = argument_1 == 0 && condition_2;
            if (argument_1 != 0 || !condition_2) {
                condition_5 = argument_3 == 0 && (argument_4 & 0x7fffffff) == 0;
            }
            if (!condition_5) {
                if (unresolved_saved_r4 == 0) {
                    value_4 = argument_2 & 0x80000000;
                    do {
                        value_1 = argument_1 & 0x80000000;
                        argument_1 = argument_1 << 1;
                        argument_2 = argument_2 * 2 + (uint32_t)(value_1 != 0);
                    } while ((argument_2 & 0x100000) == 0);
                    argument_2 = argument_2 | value_4;
                    if (value_3 != 0) {
                        return pico_join_u32_u32(argument_2, argument_1);
                    }
                }
                do {
                    value_3 = argument_3 & 0x80000000;
                    argument_3 = argument_3 << 1;
                    argument_4 = argument_4 * 2 + (uint32_t)(value_3 != 0);
                } while ((argument_4 & 0x100000) == 0);
                return pico_join_u32_u32(argument_2, argument_1);
            }
            if (argument_1 != 0 || (argument_2 & 0x7fffffff) != 0)
                goto branch_1;
            if (argument_3 != 0 || (argument_4 & 0x7fffffff) != 0)
                goto branch_2;
        }
    }
    return pico_join_u32_u32(argument_2, argument_1) | 0x7ff8000000000000;
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x200212f0. */
uint32_t soft_double_compare_core(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                  uint32_t argument_4)
{
    uint32_t value_1;
    bool condition_2;
    bool condition_3;

    if (((int)(argument_2 << 1) >> 0x15 == -1 || (int)(argument_4 << 1) >> 0x15 == -1) &&
        ((((int)(argument_2 << 1) >> 0x15 == -1 &&
           (argument_1 != 0 || (argument_2 & 0xfffff) != 0)) ||
          (((int)(argument_4 << 1) >> 0x15 == -1 &&
            (argument_3 != 0 || (argument_4 & 0xfffff) != 0)))))) {
        return 1;
    }
    condition_2 = (argument_2 & 0x7fffffff) == 0;
    condition_3 = argument_1 == 0 && condition_2;
    if (argument_1 == 0 && condition_2) {
        condition_3 = argument_3 == 0 && (argument_4 & 0x7fffffff) == 0;
    }
    if (!condition_3) {
        condition_3 = argument_2 == argument_4;
    }
    if (condition_3 && argument_1 == argument_3) {
        return 0;
    }
    value_1 = argument_2 ^ argument_4;
    condition_2 = value_1 == 0;
    if (-1 < (int)value_1) {
        condition_2 = argument_2 == argument_4;
    }
    condition_3 = -1 < (int)value_1 && argument_4 <= argument_2;
    if (condition_2) {
        condition_3 = argument_3 <= argument_1;
    }
    value_1 = (int)argument_4 >> 0x1f;
    if (!condition_3) {
        value_1 = ~value_1;
    }
    return value_1 | 1;
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x2002136c. */
void soft_double_compare_reversed_flags(uint32_t argument_1, uint32_t argument_2,
                                        uint32_t argument_3, uint32_t argument_4)
{
    soft_double_compare_flags(argument_3, argument_4, argument_1, argument_2);
    return;
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x2002137c. */
uint32_t soft_double_compare_flags(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                   uint32_t argument_4)
{
    soft_double_compare_core(argument_1, argument_2, argument_3, argument_4);
    return argument_1;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2002138c. */
int __aeabi_dcmpeq(double left, double right)
{
    char unresolved_input_ZR;

    soft_double_compare_flags(PICO_SUB_U32(left, 0), (uint32_t)((uint64_t)left >> 0x20),
                              PICO_SUB_U32(right, 0), (uint32_t)((uint64_t)right >> 0x20));
    return (uint32_t)(unresolved_input_ZR != '\0');
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x200213a0. */
int __aeabi_dcmplt(double left, double right)
{
    char unresolved_input_CY;

    soft_double_compare_flags(PICO_SUB_U32(left, 0), (uint32_t)((uint64_t)left >> 0x20),
                              PICO_SUB_U32(right, 0), (uint32_t)((uint64_t)right >> 0x20));
    return (uint32_t)(unresolved_input_CY == '\0');
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x200213b4. */
int __aeabi_dcmple(double left, double right)
{
    uint8_t unresolved_input_ZR;
    uint8_t unresolved_input_CY;

    soft_double_compare_flags(PICO_SUB_U32(left, 0), (uint32_t)((uint64_t)left >> 0x20),
                              PICO_SUB_U32(right, 0), (uint32_t)((uint64_t)right >> 0x20));
    return (uint32_t)(!(bool)unresolved_input_CY || (bool)unresolved_input_ZR);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x200213c8. */
int __aeabi_dcmpge(double left, double right)
{
    uint8_t unresolved_input_ZR;
    uint8_t unresolved_input_CY;

    soft_double_compare_reversed_flags(PICO_SUB_U32(left, 0), (uint32_t)((uint64_t)left >> 0x20),
                                       PICO_SUB_U32(right, 0), (uint32_t)((uint64_t)right >> 0x20));
    return (uint32_t)(!(bool)unresolved_input_CY || (bool)unresolved_input_ZR);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x200213dc. */
int __aeabi_dcmpgt(double left, double right)
{
    char unresolved_input_CY;

    soft_double_compare_reversed_flags(PICO_SUB_U32(left, 0), (uint32_t)((uint64_t)left >> 0x20),
                                       PICO_SUB_U32(right, 0), (uint32_t)((uint64_t)right >> 0x20));
    return (uint32_t)(unresolved_input_CY == '\0');
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x200213f0. */
int __aeabi_d2iz(double value)
{
    uint32_t value_1;
    uint32_t value_2;
    int value_3;
    uint32_t value_4;

    value_2 = (uint32_t)((uint64_t)value >> 0x20);
    value_3 = value_2 * 2 + 0x200000;
    if (value_2 * 2 < 0xffe00000) {
        if (-1 < value_3) {
            return 0;
        }
        value_1 = value_3 >> 0x15;
        value_4 = -value_1 - 0x3e1;
        if (value_1 < 0xfffffc20 && value_4 != 0) {
            value_2 =
                (value_2 << 0xb | 0x80000000 | PICO_SUB_U32(value, 0) >> 0x15) >> (value_4 & 0xff);
            if (((uint64_t)value & 0x8000000000000000) != 0) {
                value_2 = -value_2;
            }
            return value_2;
        }
    } else if (PICO_SUB_U32(value, 0) != 0 || ((uint64_t)value & 0xfffff00000000) != 0) {
        return 0;
    }
    value_2 = value_2 & 0x80000000;
    if (((uint64_t)value & 0x8000000000000000) == 0) {
        value_2 = 0x7fffffff;
    }
    return value_2;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021440. */
uint32_t __aeabi_d2uiz(double value)
{
    int value_1;
    uint32_t value_2;
    int value_3;

    value_1 = (int)((uint64_t)value >> 0x20);
    value_2 = value_1 * 2;
    if (((uint64_t)value & 0x8000000000000000) != 0) {
        return 0;
    }
    value_3 = value_2 + 0x200000;
    if (value_2 < 0xffe00000) {
        if (-1 < value_3) {
            return 0;
        }
        value_2 = -(value_3 >> 0x15) - 0x3e1;
        if (-1 < (int)value_2) {
            return (value_1 << 0xb | 0x80000000U | PICO_SUB_U32(value, 0) >> 0x15) >>
                   (value_2 & 0xff);
        }
    } else if (PICO_SUB_U32(value, 0) != 0 || ((uint64_t)value & 0xfffff00000000) != 0) {
        return 0;
    }
    return 0xffffffff;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021480. */
float __aeabi_d2f(double value)
{
    float number_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t value_6;
    uint32_t unresolved_input_r12;
    bool condition_7;
    bool condition_8;

    value_2 = (uint32_t)((uint64_t)value >> 0x20);
    value_6 = PICO_SUB_U32(value, 0);
    value_3 = value_2 * 2;
    condition_8 = value_3 < 0x70000000;
    value_5 = value_3 + 0x90000000;
    value_4 = value_5;
    if (!condition_8) {
        unresolved_input_r12 = value_3 + 0x8fe00000;
        value_4 = unresolved_input_r12;
    }
    condition_7 = value_4 == 0;
    if (!condition_8 && value_5 >= 0x200000) {
        condition_7 = unresolved_input_r12 == 0x1fc00000;
    }
    if (((condition_8 || value_5 < 0x200000) || 0x1fc00000 < unresolved_input_r12) || condition_7) {
        if (((uint64_t)value & 0x4000000000000000) != 0) {
            if (((int)value_3 >> 0x15 == -1) &&
                (value_6 != 0 || ((uint64_t)value & 0xfffff00000000) != 0)) {
                return NAN;
            }
            return (float)(value_2 & 0x80000000 | 0x7f800000);
        }
        if ((int)value_5 < -0x2e00000) {
            return (float)(value_2 & 0x80000000);
        }
        value_3 = 0x18 - (value_3 + 0x92e00000 >> 0x15);
        value_4 = value_6 >> (value_3 & 0xff);
        if (value_6 << (0x20 - value_3 & 0xff) != 0) {
            value_4 = value_4 | 1;
        }
        value_6 = value_2 & 0x1fffff | 0x100000;
        value = (double)pico_join_u32_u32(value_2, value_4 | value_6 << (0x20 - value_3 & 0xff));
        value_5 = (value_6 >> (value_3 & 0xff)) << 1;
    }
    value_4 = PICO_SUB_U32(value, 0) * 8;
    number_1 = (float)(((uint32_t)((uint64_t)value >> 0x20) & 0x80000000 |
                        PICO_SUB_U32(value, 0) >> 0x1d) +
                       value_5 * 4 + (uint32_t)(0x7fffffff < value_4));
    if (value_4 == 0x80000000) {
        number_1 = (float)((uint32_t)number_1 & 0xfffffffe);
    }
    return number_1;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021528. */
float __aeabi_fsub(float left, float right)
{
    int value_1;
    int value_2;
    float number_3;
    float number_4;
    uint32_t value_5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t value_8;
    uint32_t unresolved_input_r3;
    uint32_t value_9;
    bool condition_10;

    number_4 = -right;
    value_5 = (int)left << 1;
    condition_10 = value_5 == 0;
    if (!condition_10) {
        unresolved_input_r3 = (int)right << 1;
        condition_10 = unresolved_input_r3 == 0;
    }
    if (!condition_10) {
        condition_10 = value_5 == unresolved_input_r3;
    }
    value_1 = (int)value_5 >> 0x18;
    if (!condition_10) {
        condition_10 = value_1 == -1;
    }
    if (!condition_10) {
        condition_10 = (int)unresolved_input_r3 >> 0x18 == -1;
    }
    if (condition_10) {
        value_2 = ((int)right << 1) >> 0x18;
        if (value_1 == -1 || value_2 == -1) {
            number_3 = number_4;
            if (value_1 == -1) {
                number_3 = left;
            }
            if (value_1 != -1 || value_2 != -1) {
                number_4 = number_3;
            }
            if ((((uint32_t)number_3 & 0x7fffff) != 0 || ((uint32_t)number_4 & 0x7fffff) != 0) ||
                number_3 != number_4) {
                number_3 = (float)((uint32_t)number_3 | 0x400000);
            }
            return number_3;
        }
        if ((((uint32_t)left ^ (uint32_t)right) & 0x7fffffff) != 0) {
            if (value_5 == 0) {
                left = number_4;
            }
            return left;
        }
        if (left != number_4) {
            return 0.0;
        }
        if ((value_5 & 0xff000000) == 0) {
            number_4 = (float)((int)left << 1);
            if (((uint32_t)left & 0x80000000) != 0) {
                number_4 = (float)((uint32_t)number_4 | 0x80000000);
            }
            return number_4;
        }
        if (value_5 < 0xfe000000) {
            return (float)((int)left + 0x800000);
        }
        value_9 = (uint32_t)left & 0x80000000;
    branch_1:
        return (float)(value_9 | 0x7f800000);
    }
    value_6 = (uint32_t)pico_abs_f32(left) >> 0x17;
    value_5 = unresolved_input_r3 >> 0x18;
    value_9 = value_5 - value_6;
    number_3 = number_4;
    value_7 = value_6;
    if (value_9 != 0 && value_6 <= value_5) {
        value_7 = value_6 + value_9;
        number_3 = left;
        left = number_4;
    }
    if (value_5 < value_6) {
        value_9 = -value_9;
    }
    if (0x19 < value_9) {
        return left;
    }
    value_5 = (uint32_t)left & 0xffffff | 0x800000;
    if (((uint32_t)left & 0x80000000) != 0) {
        value_5 = -value_5;
    }
    value_6 = (uint32_t)number_3 & 0xffffff | 0x800000;
    if (((uint32_t)number_3 & 0x80000000) != 0) {
        value_6 = -value_6;
    }
    if (value_7 == value_9) {
        value_6 = value_6 ^ 0x800000;
        if (value_7 == 0) {
            value_5 = value_5 ^ 0x800000;
            value_7 = 1;
        } else {
            value_9 = value_9 - 1;
        }
    }
    value_5 = value_5 + ((int)value_6 >> (value_9 & 0xff));
    value_6 = value_6 << (0x20 - value_9 & 0xff);
    value_9 = value_5 & 0x80000000;
    if ((int)value_5 < 0) {
        condition_10 = value_6 != 0;
        value_6 = -value_6;
        value_5 = -value_5 - (uint32_t)condition_10;
    }
    if (value_5 < 0x800000) {
        value_8 = value_6 & 0x80000000;
        value_6 = value_6 << 1;
        value_5 = value_5 * 2 + (uint32_t)(value_8 != 0);
        value_8 = value_7 - 2;
        if ((value_5 & 0x800000) == 0) {
            value_7 = pico_count_leading_zeros_u32(value_5) - 8;
            value_5 = value_5 << (value_7 & 0xff);
            if ((int)value_8 < (int)value_7) {
                value_5 = value_5 >> (-(value_8 - value_7) & 0xff);
            } else {
                value_5 = value_5 + (value_8 - value_7) * 0x800000;
            }
            return (float)(value_5 | value_9);
        }
    } else {
        value_8 = value_7 - 1;
        if (0xffffff < value_5) {
            value_8 = value_5 & 1;
            value_5 = value_5 >> 1;
            value_6 = (uint32_t)(value_8 != 0) << 0x1f | value_6 >> 1;
            value_8 = value_7;
            if (0xfd < value_7)
                goto branch_1;
        }
    }
    value_5 = value_5 + value_8 * 0x800000 + (uint32_t)(0x7fffffff < value_6);
    if (value_6 == 0x80000000) {
        value_5 = value_5 & 0xfffffffe;
    }
    return (float)(value_5 | value_9);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2002152c. */
float __aeabi_fadd(float left, float right)
{
    int value_1;
    int value_2;
    float number_3;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t unresolved_input_r3;
    uint32_t value_8;
    bool condition_9;

    value_4 = (int)left << 1;
    condition_9 = value_4 == 0;
    if (!condition_9) {
        unresolved_input_r3 = (int)right << 1;
        condition_9 = unresolved_input_r3 == 0;
    }
    if (!condition_9) {
        condition_9 = value_4 == unresolved_input_r3;
    }
    value_1 = (int)value_4 >> 0x18;
    if (!condition_9) {
        condition_9 = value_1 == -1;
    }
    if (!condition_9) {
        condition_9 = (int)unresolved_input_r3 >> 0x18 == -1;
    }
    if (condition_9) {
        value_2 = ((int)right << 1) >> 0x18;
        if (value_1 == -1 || value_2 == -1) {
            number_3 = right;
            if (value_1 == -1) {
                number_3 = left;
            }
            if (value_1 != -1 || value_2 != -1) {
                right = number_3;
            }
            if ((((uint32_t)number_3 & 0x7fffff) != 0 || ((uint32_t)right & 0x7fffff) != 0) ||
                number_3 != right) {
                number_3 = (float)((uint32_t)number_3 | 0x400000);
            }
            return number_3;
        }
        if ((((uint32_t)left ^ (uint32_t)right) & 0x7fffffff) != 0) {
            if (value_4 == 0) {
                left = right;
            }
            return left;
        }
        if (left != right) {
            return 0.0;
        }
        if ((value_4 & 0xff000000) == 0) {
            number_3 = (float)((int)left << 1);
            if (((uint32_t)left & 0x80000000) != 0) {
                number_3 = (float)((uint32_t)number_3 | 0x80000000);
            }
            return number_3;
        }
        if (value_4 < 0xfe000000) {
            return (float)((int)left + 0x800000);
        }
        value_8 = (uint32_t)left & 0x80000000;
    branch_1:
        return (float)(value_8 | 0x7f800000);
    }
    value_5 = (uint32_t)pico_abs_f32(left) >> 0x17;
    value_4 = unresolved_input_r3 >> 0x18;
    value_8 = value_4 - value_5;
    number_3 = right;
    value_6 = value_5;
    if (value_8 != 0 && value_5 <= value_4) {
        value_6 = value_5 + value_8;
        number_3 = left;
        left = right;
    }
    if (value_4 < value_5) {
        value_8 = -value_8;
    }
    if (0x19 < value_8) {
        return left;
    }
    value_4 = (uint32_t)left & 0xffffff | 0x800000;
    if (((uint32_t)left & 0x80000000) != 0) {
        value_4 = -value_4;
    }
    value_5 = (uint32_t)number_3 & 0xffffff | 0x800000;
    if (((uint32_t)number_3 & 0x80000000) != 0) {
        value_5 = -value_5;
    }
    if (value_6 == value_8) {
        value_5 = value_5 ^ 0x800000;
        if (value_6 == 0) {
            value_4 = value_4 ^ 0x800000;
            value_6 = 1;
        } else {
            value_8 = value_8 - 1;
        }
    }
    value_4 = value_4 + ((int)value_5 >> (value_8 & 0xff));
    value_5 = value_5 << (0x20 - value_8 & 0xff);
    value_8 = value_4 & 0x80000000;
    if ((int)value_4 < 0) {
        condition_9 = value_5 != 0;
        value_5 = -value_5;
        value_4 = -value_4 - (uint32_t)condition_9;
    }
    if (value_4 < 0x800000) {
        value_7 = value_5 & 0x80000000;
        value_5 = value_5 << 1;
        value_4 = value_4 * 2 + (uint32_t)(value_7 != 0);
        value_7 = value_6 - 2;
        if ((value_4 & 0x800000) == 0) {
            value_6 = pico_count_leading_zeros_u32(value_4) - 8;
            value_4 = value_4 << (value_6 & 0xff);
            if ((int)value_7 < (int)value_6) {
                value_4 = value_4 >> (-(value_7 - value_6) & 0xff);
            } else {
                value_4 = value_4 + (value_7 - value_6) * 0x800000;
            }
            return (float)(value_4 | value_8);
        }
    } else {
        value_7 = value_6 - 1;
        if (0xffffff < value_4) {
            value_7 = value_4 & 1;
            value_4 = value_4 >> 1;
            value_5 = (uint32_t)(value_7 != 0) << 0x1f | value_5 >> 1;
            value_7 = value_6;
            if (0xfd < value_6)
                goto branch_1;
        }
    }
    value_4 = value_4 + value_7 * 0x800000 + (uint32_t)(0x7fffffff < value_5);
    if (value_5 == 0x80000000) {
        value_4 = value_4 & 0xfffffffe;
    }
    return (float)(value_4 | value_8);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2002168c. */
float __aeabi_ui2f(uint32_t value)
{
    float number_1;
    uint32_t value_2;
    uint32_t value_3;
    int value_4;

    if (value == 0) {
        return 0.0;
    }
    value_2 = pico_count_leading_zeros_u32(value);
    value_3 = value_2 - 8;
    value_4 = value_3 * -0x800000 + 0x4a800000;
    if (value_2 < 8) {
        value_3 = value << value_2 + 0x18;
        number_1 = (float)(value_4 +
                           ((value >> (0x20 - (value_2 + 0x18) & 0xff)) - ((int)value_3 >> 0x1f)));
        if ((value_3 & 0x7fffffff) == 0) {
            number_1 = (float)((uint32_t)number_1 & ~(value_3 >> 0x1f));
        }
        return number_1;
    }
    value_2 = 0 << (value_3 & 0xff);
    number_1 = (float)(value_4 + (value << (value_3 & 0xff)) + (0U >> (0x20 - value_3 & 0xff)) +
                       (uint32_t)(0x7fffffff < value_2));
    if (value_2 == 0x80000000) {
        number_1 = (float)((uint32_t)number_1 & 0xfffffffe);
    }
    return number_1;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021694. */
float __aeabi_i2f(int value)
{
    float number_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    int value_5;

    value_4 = value & 0x80000000;
    if ((int)value_4 < 0) {
        value = -value;
    }
    if (value == 0) {
        return 0.0;
    }
    value_2 = pico_count_leading_zeros_u32(value);
    value_3 = value_2 - 8;
    value_5 = ((value_4 | 0x4b000000) - 0x800000) + value_3 * -0x800000;
    if (value_2 < 8) {
        value_4 = value << value_2 + 0x18;
        number_1 = (float)(value_5 + (((uint32_t)value >> (0x20 - (value_2 + 0x18) & 0xff)) -
                                      ((int)value_4 >> 0x1f)));
        if ((value_4 & 0x7fffffff) == 0) {
            number_1 = (float)((uint32_t)number_1 & ~(value_4 >> 0x1f));
        }
        return number_1;
    }
    value_4 = 0 << (value_3 & 0xff);
    number_1 = (float)(value_5 + (value << (value_3 & 0xff)) + (0U >> (0x20 - value_3 & 0xff)) +
                       (uint32_t)(0x7fffffff < value_4));
    if (value_4 == 0x80000000) {
        number_1 = (float)((uint32_t)number_1 & 0xfffffffe);
    }
    return number_1;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x2002173c. */
float __aeabi_fmul(float left, float right)
{
    int64_t value_1;
    float number_2;
    float number_3;
    uint32_t value_4;
    uint32_t value_5;
    int value_6;
    uint32_t value_7;
    int value_8;
    uint32_t unresolved_input_r3;
    int value_9;
    uint32_t value_10;
    bool condition_11;
    bool condition_12;

    value_5 = (uint32_t)left >> 0x17 & 0xff;
    condition_11 = value_5 == 0;
    if (!condition_11) {
        unresolved_input_r3 = (uint32_t)right >> 0x17 & 0xff;
        condition_11 = unresolved_input_r3 == 0;
    }
    if (!condition_11) {
        condition_11 = value_5 == 0xff;
    }
    if (!condition_11) {
        condition_11 = unresolved_input_r3 == 0xff;
    }
    if (condition_11) {
        unresolved_input_r3 = (uint32_t)right >> 0x17 & 0xff;
        if (value_5 == 0xff || unresolved_input_r3 == 0xff) {
            number_2 = right;
            if (left != 0.0 && left != -0.0) {
                number_2 = left;
            }
            number_3 = number_2;
            if (((((left == 0.0 || left == -0.0) || right == 0.0) || right == -0.0) ||
                 ((value_5 == 0xff && (((uint32_t)number_2 & 0x7fffff) != 0)))) ||
                ((unresolved_input_r3 == 0xff &&
                  (number_3 = right, ((uint32_t)right & 0x7fffff) != 0)))) {
                return (float)((uint32_t)number_3 | 0x7fc00000);
            }
            value_5 = (uint32_t)number_2 ^ (uint32_t)right;
            goto branch_1;
        }
        if (pico_abs_f32(left) == 0.0 || pico_abs_f32(right) == 0.0) {
            return (float)(((uint32_t)left ^ (uint32_t)right) & 0x80000000);
        }
        condition_11 = value_5 == 0;
        value_4 = (uint32_t)left & 0x80000000;
        while (true) {
            if (condition_11) {
                left = (float)((int)left << 1);
                condition_11 = ((uint32_t)left & 0x800000) == 0;
            }
            if (!condition_11)
                break;
            value_5 = value_5 - 1;
        }
        left = (float)((uint32_t)left | value_4);
        condition_11 = unresolved_input_r3 == 0;
        value_4 = (uint32_t)right & 0x80000000;
        while (true) {
            if (condition_11) {
                right = (float)((int)right << 1);
                condition_11 = ((uint32_t)right & 0x800000) == 0;
            }
            if (!condition_11)
                break;
            unresolved_input_r3 = unresolved_input_r3 - 1;
        }
        right = (float)((uint32_t)right | value_4);
    }
    value_6 = value_5 + unresolved_input_r3;
    value_5 = (uint32_t)left ^ (uint32_t)right;
    condition_11 = (int)left << 9 == 0;
    if (!condition_11) {
        right = (float)((int)right << 9);
        condition_11 = right == 0.0;
    }
    if (condition_11) {
        if ((int)left << 9 == 0) {
            right = (float)((int)right << 9);
        }
        value_5 = value_5 & 0x80000000 | (uint32_t)left & 0x7fffff | (uint32_t)right >> 9;
        condition_12 = pico_sub_overflow_i32(value_6, 0x7f);
        value_8 = value_6 + -0x7f;
        condition_11 = value_8 == 0;
        value_9 = value_8;
        if (!condition_11 && 0x7e < value_6) {
            condition_12 = pico_sub_overflow_i32(0xff, value_8);
            value_9 = -value_8 + 0xff;
            condition_11 = -value_8 == -0xff;
        }
        if (!condition_11 && value_9 < 0 == condition_12) {
            return (float)(value_5 | value_8 * 0x800000);
        }
        value_5 = value_5 | 0x800000;
        value_10 = 0;
        condition_12 = pico_sub_overflow_i32(value_8, 1);
        value_7 = value_6 - 0x80;
        condition_11 = value_7 == 0;
        value_4 = value_7;
    } else {
        value_1 = (uint64_t)(((uint32_t)left & 0x7fffff) << 4 | 0x8000000) *
                  (uint64_t)((uint32_t)right >> 5 | 0x8000000);
        condition_11 = (uint32_t)((uint64_t)value_1 >> 0x20) < 0x800000;
        if (condition_11) {
            value_1 = value_1 * 2;
        }
        value_10 = (uint32_t)value_1;
        value_5 = value_5 & 0x80000000 | (uint32_t)((uint64_t)value_1 >> 0x20);
        value_7 = (value_6 + -0x7f) - (uint32_t)condition_11;
        condition_12 = pico_sub_overflow_i32(value_7, 0xfd);
        condition_11 = value_7 == 0xfd;
        value_4 = value_7 - 0xfd;
        if (value_7 < 0xfe) {
            number_2 = (float)(value_5 + value_7 * 0x800000 + (uint32_t)(0x7fffffff < value_10));
            if (value_10 == 0x80000000) {
                number_2 = (float)((uint32_t)number_2 & 0xfffffffe);
            }
            return number_2;
        }
    }
    if (condition_11 || (int)value_4 < 0 != condition_12) {
        if ((int)value_7 < -0x18) {
            return (float)(value_5 & 0x80000000);
        }
        value_4 = (value_5 << 1) >> (-value_7 & 0xff);
        value_7 = value_5 << (value_7 + 0x20 & 0xff);
        number_2 = (float)(((uint32_t)((value_5 & 0x80000000) != 0) << 0x1f | value_4 >> 1) +
                           (uint32_t)((uint8_t)value_4 & 1));
        if (value_10 == 0 && (value_7 & 0x7fffffff) == 0) {
            number_2 = (float)((uint32_t)number_2 & ~(value_7 >> 0x1f));
        }
        return number_2;
    }
branch_1:
    return (float)(value_5 & 0x80000000 | 0x7f800000);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x200218a4. */
float __aeabi_fdiv(float left, float right)
{
    float number_1;
    uint32_t value_2;
    uint32_t value_3;
    int value_4;
    uint32_t value_5;
    int value_6;
    uint32_t unresolved_input_r3;
    uint32_t value_7;
    int value_8;
    uint32_t value_9;
    bool condition_10;
    bool condition_11;

    value_3 = (uint32_t)left >> 0x17 & 0xff;
    condition_10 = value_3 == 0;
    if (!condition_10) {
        unresolved_input_r3 = (uint32_t)right >> 0x17 & 0xff;
        condition_10 = unresolved_input_r3 == 0;
    }
    if (!condition_10) {
        condition_10 = value_3 == 0xff;
    }
    if (!condition_10) {
        condition_10 = unresolved_input_r3 == 0xff;
    }
    if (condition_10) {
        unresolved_input_r3 = (uint32_t)right >> 0x17 & 0xff;
        if (value_3 == 0xff) {
            if (((uint32_t)left & 0x7fffff) != 0) {
            branch_1:
                return (float)((uint32_t)left | 0x7fc00000);
            }
            if (unresolved_input_r3 == 0xff) {
                left = right;
                goto branch_1;
            }
        } else {
            if (unresolved_input_r3 == 0xff) {
                if (((uint32_t)right & 0x7fffff) == 0) {
                branch_2:
                    return (float)(((uint32_t)left ^ (uint32_t)right) & 0x80000000);
                }
                left = right;
                goto branch_1;
            }
            if (pico_abs_f32(left) != 0.0 && pico_abs_f32(right) != 0.0) {
                condition_10 = value_3 == 0;
                value_9 = (uint32_t)left & 0x80000000;
                while (true) {
                    if (condition_10) {
                        left = (float)((int)left << 1);
                        condition_10 = ((uint32_t)left & 0x800000) == 0;
                    }
                    if (!condition_10)
                        break;
                    value_3 = value_3 - 1;
                }
                left = (float)((uint32_t)left | value_9);
                condition_10 = unresolved_input_r3 == 0;
                value_9 = (uint32_t)right & 0x80000000;
                while (true) {
                    if (condition_10) {
                        right = (float)((int)right << 1);
                        condition_10 = ((uint32_t)right & 0x800000) == 0;
                    }
                    if (!condition_10)
                        break;
                    unresolved_input_r3 = unresolved_input_r3 - 1;
                }
                right = (float)((uint32_t)right | value_9);
                goto branch_3;
            }
            if (pico_abs_f32(left) == 0.0) {
                if (pico_abs_f32(right) != 0.0)
                    goto branch_2;
                goto branch_1;
            }
        }
        value_3 = (uint32_t)left ^ (uint32_t)right;
    } else {
    branch_3:
        value_4 = value_3 - unresolved_input_r3;
        if (((uint32_t)right & 0x7fffff) == 0) {
            value_3 = ((uint32_t)left ^ (uint32_t)right) & 0x80000000 | (uint32_t)left & 0x7fffff;
            condition_11 = pico_add_overflow_i32(value_4, 0x7f);
            value_6 = value_4 + 0x7f;
            condition_10 = value_6 == 0;
            value_8 = value_6;
            if (!condition_10 && -0x80 < value_4) {
                condition_11 = pico_sub_overflow_i32(0xff, value_6);
                value_8 = -value_6 + 0xff;
                condition_10 = -value_6 == -0xff;
            }
            if (!condition_10 && value_8 < 0 == condition_11) {
                return (float)(value_3 | value_6 * 0x800000);
            }
            value_3 = value_3 | 0x800000;
            value_7 = 0;
            condition_11 = pico_sub_overflow_i32(value_6, 1);
            value_5 = value_4 + 0x7e;
            condition_10 = value_5 == 0;
            value_9 = value_5;
        } else {
            value_2 = ((uint32_t)right & 0x7fffff) << 5 | 0x10000000;
            value_7 = ((uint32_t)left & 0x7fffff) << 5 | 0x10000000;
            value_3 = ((uint32_t)left ^ (uint32_t)right) & 0x80000000;
            condition_10 = value_2 <= value_7;
            if (!condition_10) {
                value_7 = value_7 << 1;
            }
            value_5 = value_4 + 0x7d + (uint32_t)condition_10;
            value_9 = 0x800000;
            do {
                if (value_2 <= value_7) {
                    value_7 = value_7 - value_2;
                    value_3 = value_3 | value_9;
                }
                if (value_2 >> 1 <= value_7) {
                    value_7 = value_7 - (value_2 >> 1);
                    value_3 = value_3 | value_9 >> 1;
                }
                if (value_2 >> 2 <= value_7) {
                    value_7 = value_7 - (value_2 >> 2);
                    value_3 = value_3 | value_9 >> 2;
                }
                if (value_2 >> 3 <= value_7) {
                    value_7 = value_7 - (value_2 >> 3);
                    value_3 = value_3 | value_9 >> 3;
                }
                value_7 = value_7 * 0x10;
                condition_10 = value_7 == 0;
                if (!condition_10) {
                    value_9 = value_9 >> 4;
                    condition_10 = value_9 == 0;
                }
            } while (!condition_10);
            condition_11 = pico_sub_overflow_i32(value_5, 0xfd);
            condition_10 = value_5 == 0xfd;
            value_9 = value_5 - 0xfd;
            if (value_5 < 0xfe) {
                number_1 = (float)(value_3 + value_5 * 0x800000 + (uint32_t)(value_2 <= value_7));
                if (value_7 == value_2) {
                    number_1 = (float)((uint32_t)number_1 & 0xfffffffe);
                }
                return number_1;
            }
        }
        if (condition_10 || (int)value_9 < 0 != condition_11) {
            if ((int)value_5 < -0x18) {
                return (float)(value_3 & 0x80000000);
            }
            value_9 = (value_3 << 1) >> (-value_5 & 0xff);
            value_5 = value_3 << (value_5 + 0x20 & 0xff);
            number_1 = (float)(((uint32_t)((value_3 & 0x80000000) != 0) << 0x1f | value_9 >> 1) +
                               (uint32_t)((uint8_t)value_9 & 1));
            if (value_7 == 0 && (value_5 & 0x7fffffff) == 0) {
                number_1 = (float)((uint32_t)number_1 & ~(value_5 >> 0x1f));
            }
            return number_1;
        }
    }
    return (float)(value_3 & 0x80000000 | 0x7f800000);
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x200219ec. */
uint32_t soft_float_compare_core(uint32_t argument_1, uint32_t argument_2)
{
    uint32_t value_1;
    bool condition_2;
    uint32_t value_3;
    uint32_t value_4;
    bool condition_5;

    value_3 = argument_1 * 2;
    value_4 = argument_2 * 2;
    if (((int)value_3 >> 0x18 == -1 || (int)value_4 >> 0x18 == -1) &&
        ((((int)value_3 >> 0x18 == -1 && ((argument_1 & 0x7fffff) != 0)) ||
          (((int)value_4 >> 0x18 == -1 && ((argument_2 & 0x7fffff) != 0)))))) {
        return 1;
    }
    condition_2 = (argument_2 & 0x7fffffff) == 0;
    condition_5 = value_3 == 0 && condition_2;
    value_1 = 0;
    if (value_3 != 0 || !condition_2) {
        value_1 = argument_1 ^ argument_2;
        condition_5 = value_1 == 0;
    }
    condition_2 = -1 < (int)value_1;
    if (condition_2) {
        argument_1 = value_3 + argument_2 * -2;
        condition_5 = argument_1 == 0;
    }
    if ((condition_2 && value_4 <= value_3) && !condition_5) {
        argument_1 = (int)argument_2 >> 0x1f;
    }
    if (!condition_2 || value_4 > value_3) {
        argument_1 = ~((int)argument_2 >> 0x1f);
    }
    if (!condition_5) {
        argument_1 = argument_1 | 1;
    }
    return argument_1;
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x20021a54. */
uint64_t soft_float_compare_flags(uint32_t argument_1, uint32_t argument_2)
{
    soft_float_compare_core(argument_1, argument_2);
    return pico_join_u32_u32(argument_2, argument_1);
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021a64. */
int __aeabi_fcmpeq(float left, float right)
{
    char unresolved_input_ZR;

    soft_float_compare_flags((uint32_t)left, (uint32_t)right);
    return (uint32_t)(unresolved_input_ZR != '\0');
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021a78. */
int __aeabi_fcmplt(float left, float right)
{
    char unresolved_input_CY;

    soft_float_compare_flags((uint32_t)left, (uint32_t)right);
    return (uint32_t)(unresolved_input_CY == '\0');
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021ac8. */
int __aeabi_f2iz(float value)
{
    int value_1;
    uint32_t value_2;

    if ((uint32_t)((int)value << 1) < 0x7f000000) {
        return 0;
    }
    value_1 = -((uint32_t)pico_abs_f32(value) >> 0x17);
    value_2 = value_1 + 0x9e;
    if ((uint32_t)pico_abs_f32(value) >> 0x17 < 0x9f && value_2 != 0) {
        value_2 = ((int)value << 8 | 0x80000000U) >> (value_2 & 0xff);
        if (((uint32_t)value & 0x80000000) != 0) {
            value_2 = -value_2;
        }
        return value_2;
    }
    if ((value_1 == -0xff) && (((uint32_t)value & 0x7fffff) != 0)) {
        return 0;
    }
    value_2 = (uint32_t)value & 0x80000000;
    if (value_2 == 0) {
        value_2 = 0x7fffffff;
    }
    return value_2;
}

/* Semantic identification from IEEE-754 sign/exponent/mantissa operations and concrete
 * arithmetic/comparison call sites; ABI is ARM soft-float. Firmware address: 0x20021b14. */
uint32_t __aeabi_f2uiz(float value)
{
    uint32_t value_1;

    if ((((uint32_t)value & 0x80000000) != 0) || ((uint32_t)((int)value << 1) < 0x7f000000)) {
        return 0;
    }
    value_1 = -((uint32_t)pico_abs_f32(value) >> 0x17) + 0x9e;
    if (-1 < (int)value_1) {
        return ((int)value << 8 | 0x80000000U) >> (value_1 & 0xff);
    }
    if ((-((uint32_t)pico_abs_f32(value) >> 0x17) == -0xff) &&
        (((uint32_t)value & 0x7fffff) != 0)) {
        return 0;
    }
    return 0xffffffff;
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x20021b54. */
uint64_t soft_unsigned_long_long_divide_wrapper(uint32_t argument_1, uint32_t argument_2,
                                                uint32_t argument_3, uint32_t argument_4)
{
    uint64_t soft_unsigned_long_long_divmod_result;
    int buffer_8[2];

    if ((argument_4 != 0) || (argument_3 != 0)) {
        soft_unsigned_long_long_divmod_result = soft_unsigned_long_long_divmod(
            argument_1, argument_2, argument_3, argument_4, buffer_8);
        return soft_unsigned_long_long_divmod_result;
    }
    if (argument_2 != 0 || argument_1 != 0) {
        argument_2 = 0xffffffff;
        argument_1 = 0xffffffff;
    }
    return pico_join_u32_u32(argument_2, argument_1);
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x20021b80. */
void soft_signed_long_long_divmod(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                  uint32_t argument_4, int *argument_5)
{
    uint32_t value_1;
    int64_t soft_signed_long_long_division_result;

    soft_signed_long_long_division_result =
        soft_signed_long_long_division(argument_1, argument_2, argument_3, argument_4);
    soft_signed_long_long_division_result =
        soft_signed_long_long_division_result * pico_join_u32_u32(argument_4, argument_3);
    value_1 = (uint32_t)soft_signed_long_long_division_result;
    *argument_5 = argument_1 - value_1;
    argument_5[1] = (argument_2 - (int)((uint64_t)soft_signed_long_long_division_result >> 0x20)) -
                    (uint32_t)(argument_1 < value_1);
    return;
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x20021bb0. */
uint64_t soft_unsigned_long_long_divmod(uint32_t numerator_low, uint32_t numerator_high,
                                        uint32_t denominator_low, uint32_t denominator_high,
                                        int *remainder)
{
    int64_t value_1;
    uint32_t value_2;
    uint64_t soft_unsigned_long_long_division_result;

    soft_unsigned_long_long_division_result = soft_unsigned_long_long_division(
        numerator_low, numerator_high, denominator_low, denominator_high);
    value_1 = soft_unsigned_long_long_division_result *
              pico_join_u32_u32(denominator_high, denominator_low);
    value_2 = (uint32_t)value_1;
    *remainder = numerator_low - value_2;
    remainder[1] =
        (numerator_high - (int)((uint64_t)value_1 >> 0x20)) - (uint32_t)(numerator_low < value_2);
    return soft_unsigned_long_long_division_result;
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x20021be4. */
uint64_t soft_signed_long_long_division(uint32_t argument_1, uint32_t argument_2,
                                        uint32_t argument_3, uint32_t argument_4)
{
    int64_t value_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    int value_5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t value_8;
    uint32_t value_9;
    uint32_t value_10;
    uint32_t value_11;
    bool condition_12;

    if ((int)argument_2 < 0) {
        condition_12 = argument_1 != 0;
        argument_1 = -argument_1;
        argument_2 = -argument_2 - (uint32_t)condition_12;
        value_8 = 0xffffffff;
    } else {
        value_8 = 0;
    }
    if ((int)argument_4 < 0) {
        value_8 = ~value_8;
        condition_12 = argument_3 != 0;
        argument_3 = -argument_3;
        argument_4 = -argument_4 - (uint32_t)condition_12;
    }
    if (argument_4 == 0) {
        if (argument_2 < argument_3) {
            value_5 = pico_count_leading_zeros_u32(argument_3);
            if (value_5 != 0) {
                argument_3 = argument_3 << value_5;
                argument_2 = argument_2 << value_5 | argument_1 >> (0x20U - value_5 & 0xff);
                argument_1 = argument_1 << value_5;
            }
            value_2 = argument_3 >> 0x10;
            value_3 = argument_2 / value_2;
            value_9 = argument_1 >> 0x10 | (argument_2 - value_2 * value_3) * 0x10000;
            value_6 = (argument_3 & 0xffff) * value_3;
            value_10 = value_3;
            if (value_9 < value_6) {
                condition_12 = pico_carry_u32(value_9, argument_3);
                value_9 = value_9 + argument_3;
                value_10 = value_3 - 1;
                if ((!condition_12) && (value_9 < value_6)) {
                    value_10 = value_3 - 2;
                    value_9 = value_9 + argument_3;
                }
            }
            value_7 = (value_9 - value_6) / value_2;
            value_6 = argument_1 & 0xffff | ((value_9 - value_6) - value_2 * value_7) * 0x10000;
            value_3 = (argument_3 & 0xffff) * value_7;
            value_2 = value_7;
            if (value_6 < value_3) {
                value_2 = value_7 - 1;
                if ((!pico_carry_u32(value_6, argument_3)) && (value_6 + argument_3 < value_3)) {
                    value_2 = value_7 - 2;
                }
            }
            value_2 = value_2 | value_10 << 0x10;
            value_10 = 0;
        } else {
            if (argument_3 == 0) {
                argument_3 = 1 / 0;
            }
            value_5 = pico_count_leading_zeros_u32(argument_3);
            if (value_5 == 0) {
                value_9 = argument_2 - argument_3;
                value_3 = argument_3 >> 0x10;
                value_11 = argument_3 & 0xffff;
                value_10 = 1;
            } else {
                argument_3 = argument_3 << value_5;
                value_10 = argument_2 >> (0x20U - value_5 & 0xff);
                value_3 = argument_3 >> 0x10;
                value_7 = value_10 / value_3;
                value_9 = argument_2 << value_5 | argument_1 >> (0x20U - value_5 & 0xff);
                value_11 = argument_3 & 0xffff;
                value_10 = value_9 >> 0x10 | (value_10 - value_3 * value_7) * 0x10000;
                value_6 = value_11 * value_7;
                argument_1 = argument_1 << value_5;
                value_2 = value_7;
                if (value_10 < value_6) {
                    condition_12 = pico_carry_u32(value_10, argument_3);
                    value_10 = value_10 + argument_3;
                    value_2 = value_7 - 1;
                    if ((!condition_12) && (value_10 < value_6)) {
                        value_2 = value_7 - 2;
                        value_10 = value_10 + argument_3;
                    }
                }
                value_7 = (value_10 - value_6) / value_3;
                value_9 = value_9 & 0xffff | ((value_10 - value_6) - value_3 * value_7) * 0x10000;
                value_6 = value_11 * value_7;
                value_10 = value_7;
                if (value_9 < value_6) {
                    condition_12 = pico_carry_u32(value_9, argument_3);
                    value_9 = value_9 + argument_3;
                    value_10 = value_7 - 1;
                    if ((!condition_12) && (value_9 < value_6)) {
                        value_10 = value_7 - 2;
                        value_9 = value_9 + argument_3;
                    }
                }
                value_9 = value_9 - value_6;
                value_10 = value_10 | value_2 << 0x10;
            }
            value_2 = value_9 / value_3;
            value_7 = argument_1 >> 0x10 | (value_9 - value_3 * value_2) * 0x10000;
            value_9 = value_11 * value_2;
            value_6 = value_2;
            if (value_7 < value_9) {
                condition_12 = pico_carry_u32(value_7, argument_3);
                value_7 = value_7 + argument_3;
                value_6 = value_2 - 1;
                if ((!condition_12) && (value_7 < value_9)) {
                    value_6 = value_2 - 2;
                    value_7 = value_7 + argument_3;
                }
            }
            value_4 = (value_7 - value_9) / value_3;
            value_3 = argument_1 & 0xffff | ((value_7 - value_9) - value_3 * value_4) * 0x10000;
            value_2 = value_4;
            if (value_3 < value_11 * value_4) {
                value_2 = value_4 - 1;
                if ((!pico_carry_u32(value_3, argument_3)) &&
                    (value_3 + argument_3 < value_11 * value_4)) {
                    value_2 = value_4 - 2;
                }
            }
            value_2 = value_2 | value_6 << 0x10;
        }
    } else if (argument_2 < argument_4) {
        value_10 = 0;
        value_2 = 0;
    } else {
        value_5 = pico_count_leading_zeros_u32(argument_4);
        if (value_5 == 0) {
            if ((argument_4 < argument_2) || (argument_3 <= argument_1)) {
                value_10 = 0;
                value_2 = 1;
            } else {
                value_2 = 0;
                value_10 = 0;
            }
        } else {
            value_10 = 0x20 - value_5;
            value_11 = argument_3 >> (value_10 & 0xff) | argument_4 << value_5;
            value_9 = value_11 >> 0x10;
            value_3 = argument_2 >> (value_10 & 0xff);
            value_2 = value_3 / value_9;
            value_6 = argument_2 << value_5 | argument_1 >> (value_10 & 0xff);
            value_3 = value_6 >> 0x10 | (value_3 - value_9 * value_2) * 0x10000;
            value_7 = (value_11 & 0xffff) * value_2;
            value_10 = value_2;
            if (value_3 < value_7) {
                condition_12 = pico_carry_u32(value_3, value_11);
                value_3 = value_3 + value_11;
                value_10 = value_2 - 1;
                if ((!condition_12) && (value_3 < value_7)) {
                    value_10 = value_2 - 2;
                    value_3 = value_3 + value_11;
                }
            }
            value_4 = (value_3 - value_7) / value_9;
            value_3 = value_6 & 0xffff | ((value_3 - value_7) - value_9 * value_4) * 0x10000;
            value_6 = (value_11 & 0xffff) * value_4;
            value_2 = value_4;
            if (value_3 < value_6) {
                condition_12 = pico_carry_u32(value_3, value_11);
                value_3 = value_3 + value_11;
                value_2 = value_4 - 1;
                if ((!condition_12) && (value_3 < value_6)) {
                    value_2 = value_4 - 2;
                    value_3 = value_3 + value_11;
                }
            }
            value_2 = value_2 | value_10 << 0x10;
            value_1 = (uint64_t)value_2 * (uint64_t)(argument_3 << value_5);
            value_9 = (uint32_t)((uint64_t)value_1 >> 0x20);
            value_10 = 0;
            if ((value_3 - value_6 < value_9) ||
                ((value_3 - value_6 == value_9 && (argument_1 << value_5 < (uint32_t)value_1)))) {
                value_2 = value_2 - 1;
                value_10 = 0;
            }
        }
    }
    if (value_8 != 0) {
        condition_12 = value_2 != 0;
        value_2 = -value_2;
        value_10 = -value_10 - (uint32_t)condition_12;
    }
    return pico_join_u32_u32(value_10, value_2);
}

/* IEEE-754 bit operations and branches identify shared helper behavior, but these internal entries
 * are not assumed to have ordinary C argument/return ABIs. Firmware address: 0x20021e7c. */
uint64_t soft_unsigned_long_long_division(uint32_t argument_1, uint32_t argument_2,
                                          uint32_t argument_3, uint32_t argument_4)
{
    int64_t value_1;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t value_8;
    int value_9;
    uint32_t value_10;
    bool condition_11;

    if (argument_4 == 0) {
        if (argument_3 <= argument_2) {
            if (argument_3 == 0) {
                argument_3 = 1 / 0;
            }
            value_9 = pico_count_leading_zeros_u32(argument_3);
            if (value_9 == 0) {
                value_3 = argument_2 - argument_3;
                value_5 = argument_3 >> 0x10;
                value_8 = argument_3 & 0xffff;
                value_2 = 1;
            } else {
                argument_3 = argument_3 << value_9;
                value_10 = argument_2 >> (0x20U - value_9 & 0xff);
                value_5 = argument_3 >> 0x10;
                value_6 = value_10 / value_5;
                value_2 = argument_1 >> (0x20U - value_9 & 0xff) | argument_2 << value_9;
                value_8 = argument_3 & 0xffff;
                value_4 = value_2 >> 0x10 | (value_10 - value_5 * value_6) * 0x10000;
                value_3 = value_8 * value_6;
                argument_1 = argument_1 << value_9;
                value_10 = value_6;
                if (value_4 < value_3) {
                    condition_11 = pico_carry_u32(value_4, argument_3);
                    value_4 = value_4 + argument_3;
                    value_10 = value_6 - 1;
                    if ((!condition_11) && (value_4 < value_3)) {
                        value_10 = value_6 - 2;
                        value_4 = value_4 + argument_3;
                    }
                }
                value_7 = (value_4 - value_3) / value_5;
                value_3 = value_2 & 0xffff | ((value_4 - value_3) - value_5 * value_7) * 0x10000;
                value_6 = value_8 * value_7;
                value_2 = value_7;
                if (value_3 < value_6) {
                    condition_11 = pico_carry_u32(value_3, argument_3);
                    value_3 = value_3 + argument_3;
                    value_2 = value_7 - 1;
                    if ((!condition_11) && (value_3 < value_6)) {
                        value_2 = value_7 - 2;
                        value_3 = value_3 + argument_3;
                    }
                }
                value_3 = value_3 - value_6;
                value_2 = value_2 | value_10 << 0x10;
            }
            value_6 = value_3 / value_5;
            value_3 = argument_1 >> 0x10 | (value_3 - value_5 * value_6) * 0x10000;
            value_4 = value_8 * value_6;
            value_10 = value_6;
            if (value_3 < value_4) {
                condition_11 = pico_carry_u32(value_3, argument_3);
                value_3 = value_3 + argument_3;
                value_10 = value_6 - 1;
                if ((!condition_11) && (value_3 < value_4)) {
                    value_10 = value_6 - 2;
                    value_3 = value_3 + argument_3;
                }
            }
            value_6 = (value_3 - value_4) / value_5;
            value_5 = argument_1 & 0xffff | ((value_3 - value_4) - value_5 * value_6) * 0x10000;
            value_3 = value_6;
            if (value_5 < value_8 * value_6) {
                value_3 = value_6 - 1;
                if ((!pico_carry_u32(value_5, argument_3)) &&
                    (value_5 + argument_3 < value_8 * value_6)) {
                    value_3 = value_6 - 2;
                }
            }
            return pico_join_u32_u32(value_2, value_3 | value_10 << 0x10);
        }
        value_9 = pico_count_leading_zeros_u32(argument_3);
        if (value_9 != 0) {
            argument_3 = argument_3 << value_9;
            argument_2 = argument_1 >> (0x20U - value_9 & 0xff) | argument_2 << value_9;
            argument_1 = argument_1 << value_9;
        }
        value_2 = argument_3 >> 0x10;
        value_3 = argument_2 / value_2;
        value_8 = argument_1 >> 0x10 | (argument_2 - value_2 * value_3) * 0x10000;
        value_5 = (argument_3 & 0xffff) * value_3;
        value_10 = value_3;
        if (value_8 < value_5) {
            condition_11 = pico_carry_u32(value_8, argument_3);
            value_8 = value_8 + argument_3;
            value_10 = value_3 - 1;
            if ((!condition_11) && (value_8 < value_5)) {
                value_10 = value_3 - 2;
                value_8 = value_8 + argument_3;
            }
        }
        value_6 = (value_8 - value_5) / value_2;
        value_5 = argument_1 & 0xffff | ((value_8 - value_5) - value_2 * value_6) * 0x10000;
        value_3 = (argument_3 & 0xffff) * value_6;
        value_2 = value_6;
        if (value_5 < value_3) {
            value_2 = value_6 - 1;
            if ((!pico_carry_u32(value_5, argument_3)) && (value_5 + argument_3 < value_3)) {
                value_2 = value_6 - 2;
            }
        }
        value_2 = value_2 | value_10 << 0x10;
    } else {
        if (argument_2 < argument_4) {
            return 0;
        }
        value_9 = pico_count_leading_zeros_u32(argument_4);
        if (value_9 == 0) {
            if ((argument_4 < argument_2) || (argument_3 <= argument_1)) {
                return 1;
            }
            value_2 = 0;
        } else {
            value_2 = 0x20 - value_9;
            value_3 = argument_3 >> (value_2 & 0xff) | argument_4 << value_9;
            value_8 = value_3 >> 0x10;
            value_10 = argument_2 >> (value_2 & 0xff);
            value_4 = value_10 / value_8;
            value_5 = argument_2 << value_9 | argument_1 >> (value_2 & 0xff);
            value_2 = value_5 >> 0x10 | (value_10 - value_8 * value_4) * 0x10000;
            value_6 = (value_3 & 0xffff) * value_4;
            value_10 = value_4;
            if (value_2 < value_6) {
                condition_11 = pico_carry_u32(value_2, value_3);
                value_2 = value_2 + value_3;
                value_10 = value_4 - 1;
                if ((!condition_11) && (value_2 < value_6)) {
                    value_10 = value_4 - 2;
                    value_2 = value_2 + value_3;
                }
            }
            value_4 = (value_2 - value_6) / value_8;
            value_5 = value_5 & 0xffff | ((value_2 - value_6) - value_8 * value_4) * 0x10000;
            value_8 = (value_3 & 0xffff) * value_4;
            value_2 = value_4;
            if (value_5 < value_8) {
                condition_11 = pico_carry_u32(value_5, value_3);
                value_5 = value_5 + value_3;
                value_2 = value_4 - 1;
                if ((!condition_11) && (value_5 < value_8)) {
                    value_2 = value_4 - 2;
                    value_5 = value_5 + value_3;
                }
            }
            value_2 = value_2 | value_10 << 0x10;
            value_1 = (uint64_t)value_2 * (uint64_t)(argument_3 << value_9);
            value_10 = (uint32_t)((uint64_t)value_1 >> 0x20);
            if ((value_5 - value_8 < value_10) ||
                ((value_5 - value_8 == value_10 && (argument_1 << value_9 < (uint32_t)value_1)))) {
                value_2 = value_2 - 1;
            }
        }
    }
    return (uint64_t)value_2;
}
