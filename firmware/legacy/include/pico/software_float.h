#ifndef PICO_SOFTWARE_FLOAT_H
#define PICO_SOFTWARE_FLOAT_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

double libm_ceil(double value);
double libm_log(double value);
double libm_ieee754_log(double value);
int libm_fpclassify(double value);
double libm_quiet_nan(void);
double __aeabi_dsub(double right, double right_2);
double __aeabi_dadd(double right, double right_2);
double __aeabi_ui2d(uint32_t value);
double __aeabi_i2d(int value);
double __aeabi_f2d(float value);
double __aeabi_dmul(double left, double right);
uint64_t soft_double_multiply_special(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                      uint32_t argument_4);
double __aeabi_ddiv(double left, double right);
uint64_t soft_double_divide_special(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                    uint32_t argument_4);
uint32_t soft_double_compare_core(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                  uint32_t argument_4);
void soft_double_compare_reversed_flags(uint32_t argument_1, uint32_t argument_2,
                                        uint32_t argument_3, uint32_t argument_4);
uint32_t soft_double_compare_flags(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                   uint32_t argument_4);
int __aeabi_dcmpeq(double left, double right);
int __aeabi_dcmplt(double left, double right);
int __aeabi_dcmple(double left, double right);
int __aeabi_dcmpge(double left, double right);
int __aeabi_dcmpgt(double left, double right);
int __aeabi_d2iz(double value);
uint32_t __aeabi_d2uiz(double value);
float __aeabi_d2f(double value);
float __aeabi_fsub(float left, float right);
float __aeabi_fadd(float left, float right);
float __aeabi_ui2f(uint32_t value);
float __aeabi_i2f(int value);
float __aeabi_fmul(float left, float right);
float __aeabi_fdiv(float left, float right);
uint32_t soft_float_compare_core(uint32_t argument_1, uint32_t argument_2);
uint64_t soft_float_compare_flags(uint32_t argument_1, uint32_t argument_2);
int __aeabi_fcmpeq(float left, float right);
int __aeabi_fcmplt(float left, float right);
int __aeabi_f2iz(float value);
uint32_t __aeabi_f2uiz(float value);
uint64_t soft_unsigned_long_long_divide_wrapper(uint32_t argument_1, uint32_t argument_2,
                                                uint32_t argument_3, uint32_t argument_4);
void soft_signed_long_long_divmod(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                  uint32_t argument_4, int *argument_5);
uint64_t soft_unsigned_long_long_divmod(uint32_t numerator_low, uint32_t numerator_high,
                                        uint32_t denominator_low, uint32_t denominator_high,
                                        int *remainder);
uint64_t soft_signed_long_long_division(uint32_t argument_1, uint32_t argument_2,
                                        uint32_t argument_3, uint32_t argument_4);
uint64_t soft_unsigned_long_long_division(uint32_t argument_1, uint32_t argument_2,
                                          uint32_t argument_3, uint32_t argument_4);

#endif /* PICO_SOFTWARE_FLOAT_H */
