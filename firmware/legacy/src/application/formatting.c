/* Pico Pro 0.1.34 — formatting. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Formats integer/fractional float parts and zero-pads requested decimal digits.
 * Firmware address: 0x20005950. */
char *pico_format_float_decimal(float argument_1, char *destination, uint32_t decimal_places)
{
    int __aeabi_f2iz_result;
    float __aeabi_i2f_result;
    float right;
    uint8_t digit_index;
    uint32_t __aeabi_f2uiz_result;
    uint32_t value_5;
    char fraction_text[16];

    __aeabi_f2iz_result = __aeabi_f2iz(argument_1);
    itoa(__aeabi_f2iz_result, destination, 10);
    __aeabi_f2iz_result = __aeabi_fcmplt(argument_1, 0.0);
    if (__aeabi_f2iz_result != 0) {
        argument_1 = (float)((int)argument_1 + -0x80000000);
    }
    if (decimal_places != 0) {
        libc_strcat(destination, ".");
        __aeabi_f2uiz_result = 1;
        for (digit_index = 0; digit_index < decimal_places; digit_index = digit_index + 1) {
            __aeabi_f2uiz_result = __aeabi_f2uiz_result * 10;
        }
        __aeabi_f2iz_result = __aeabi_f2iz(argument_1);
        __aeabi_i2f_result = __aeabi_i2f(__aeabi_f2iz_result);
        __aeabi_i2f_result = __aeabi_fsub(argument_1, __aeabi_i2f_result);
        right = __aeabi_ui2f(__aeabi_f2uiz_result);
        __aeabi_i2f_result = __aeabi_fmul(__aeabi_i2f_result, right);
        __aeabi_f2uiz_result = __aeabi_f2uiz(__aeabi_i2f_result);
        itoa(__aeabi_f2uiz_result, fraction_text, 10);
        digit_index = 0;
        while (true) {
            __aeabi_f2uiz_result = libc_strlen(fraction_text);
            value_5 = (uint32_t)digit_index;
            digit_index = digit_index + 1;
            if ((int)(decimal_places - __aeabi_f2uiz_result) <= (int)value_5)
                break;
            libc_strcat(destination, "0");
        }
        libc_strcat(destination, fraction_text);
    }
    return destination;
}
