/* Pico Pro 0.1.34 — print stream. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x2001961e. */
uint32_t Print_write_buffer(void *self, uint8_t *buffer, uint32_t count)
{
    int value_1;
    uint32_t value_2;
    uint8_t *pointer_3;

    pointer_3 = buffer + count;
    value_2 = 0;
    for (; buffer != pointer_3; buffer = buffer + 1) {
        value_1 = (*(PicoOpaqueFunction *)**(uint32_t **)self)(self, *buffer);
        value_2 = value_2 + value_1;
    }
    return value_2;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x2001964c. */
uint32_t Print_write_string(void *self, char *text)
{
    uint32_t text_length;

    text_length = libc_strlen(text);
    text_length = (**(PicoOpaqueFunction **)(*(int *)self + 4))(self, text, text_length);
    return text_length;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x20019666. */
uint32_t Print_print_String(void *self, void *text)
{
    uint8_t string_character_at_result;
    int value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t value_5;
    PicoOpaqueFunction *pointer_6;

    value_4 = 0;
    value_5 = 0;
    while (true) {
        value_3 = value_4 & 0xffff;
        value_4 = value_4 + 1;
        if (*(uint32_t *)((int)text + 8) <= value_3)
            break;
        pointer_6 = (PicoOpaqueFunction *)**(uint32_t **)self;
        string_character_at_result = String_character_at(text, value_3);
        value_2 = (*pointer_6)(self, string_character_at_result);
        value_5 = value_5 + value_2;
    }
    return value_5;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x2001969a. */
uint32_t Print_print_string(void *self, char *text)
{
    uint32_t print_write_string_result;

    print_write_string_result = Print_write_string(self, text);
    return print_write_string_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x200196a2. */
uint32_t Print_print_char(void *self, char value)
{
    uint32_t value_1;

    value_1 = (*(PicoOpaqueFunction *)**(uint32_t **)self)();
    return value_1;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x200196ac. */
uint32_t Print_println(void *self)
{
    uint32_t print_print_char_result_2;
    uint32_t print_print_char_result;

    print_print_char_result_2 = Print_print_char(self, '\r');
    print_print_char_result = Print_print_char(self, '\n');
    return print_print_char_result + print_print_char_result_2;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x200196c4. */
uint32_t Print_println_String(void *self, void *text)
{
    uint32_t print_print_string_result;
    uint32_t print_println_result;

    print_print_string_result = Print_print_String(self, text);
    print_println_result = Print_println(self);
    return print_println_result + print_print_string_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x200196d8. */
uint32_t Print_println_string(void *self, char *text)
{
    uint32_t print_write_string_result;
    uint32_t print_println_result;

    print_write_string_result = Print_write_string(self, text);
    print_println_result = Print_println(self);
    return print_println_result + print_write_string_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x200196ec. */
uint32_t Print_print_number(void *self, uint32_t value, uint8_t radix)
{
    uint32_t value_1;
    uint32_t unresolved_input_register_00000029;
    uint32_t value_2;
    uint32_t value_3;
    char *text;
    char byte_value_4;
    char buffer_15[9];

    value_2 = pico_join_u24_u8(unresolved_input_register_00000029, radix);
    buffer_15[1] = 0;
    if (value_2 < 2) {
        value_2 = 10;
    }
    text = buffer_15;
    while (true) {
        value_3 = value / value_2;
        value_1 = value - value_3 * value_2;
        byte_value_4 = (char)value_1;
        if ((value_1 & 0xff) < 10) {
            byte_value_4 = byte_value_4 + '0';
        } else {
            byte_value_4 = byte_value_4 + '7';
        }
        *text = byte_value_4;
        if (value_3 == 0)
            break;
        value = value_3;
        text = text + -1;
    }
    value_2 = Print_write_string(self, text);
    return value_2;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x20019728. */
uint32_t Print_print_signed(void *self, int value, int radix)
{
    uint32_t print_print_char_result;
    uint32_t print_print_number_result;
    uint8_t radix_00;

    if (radix == 0) {
        print_print_char_result = (*(PicoOpaqueFunction *)**(uint32_t **)self)(self, value & 0xff);
        return print_print_char_result;
    }
    if (radix == 10) {
        radix_00 = '\n';
        if (value < 0) {
            print_print_char_result = Print_print_char(self, '-');
            print_print_number_result = Print_print_number(self, -value, '\n');
            return print_print_number_result + print_print_char_result;
        }
    } else {
        radix_00 = (uint8_t)radix;
    }
    print_print_char_result = Print_print_number(self, value, radix_00);
    return print_print_char_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x20019764. */
uint32_t Print_print_signed_alias(void *self, int value, int radix)
{
    uint32_t print_print_char_result;
    uint32_t print_print_number_result;
    uint8_t radix_00;

    if (radix == 0) {
        print_print_char_result = (*(PicoOpaqueFunction *)**(uint32_t **)self)(self, value & 0xff);
        return print_print_char_result;
    }
    if (radix == 10) {
        radix_00 = '\n';
        if (value < 0) {
            print_print_char_result = Print_print_char(self, '-');
            print_print_number_result = Print_print_number(self, -value, '\n');
            return print_print_number_result + print_print_char_result;
        }
    } else {
        radix_00 = (uint8_t)radix;
    }
    print_print_char_result = Print_print_number(self, value, radix_00);
    return print_print_char_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x20019768. */
uint32_t Print_println_signed(void *self, long value, int radix)
{
    uint32_t print_print_signed_result;
    uint32_t print_println_result;

    print_print_signed_result = Print_print_signed(self, value, radix);
    print_println_result = Print_println(self);
    return print_println_result + print_print_signed_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x2001977c. */
uint32_t Print_print_unsigned(void *self, uint32_t value, int radix)
{
    uint32_t print_print_number_result;

    if (radix == 0) {
        print_print_number_result =
            (*(PicoOpaqueFunction *)**(uint32_t **)self)(self, value & 0xff);
        return print_print_number_result;
    }
    print_print_number_result = Print_print_number(self, value, (uint8_t)radix);
    return print_print_number_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x20019788. */
uint32_t Print_print_unsigned_alias_a(void *self, uint32_t value, int radix)
{
    uint32_t print_print_number_result;

    if (radix == 0) {
        print_print_number_result =
            (*(PicoOpaqueFunction *)**(uint32_t **)self)(self, value & 0xff);
        return print_print_number_result;
    }
    print_print_number_result = Print_print_number(self, value, (uint8_t)radix);
    return print_print_number_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x2001978c. */
uint32_t Print_print_unsigned_alias_b(void *self, uint32_t value, int radix)
{
    uint32_t print_print_number_result;

    if (radix == 0) {
        print_print_number_result =
            (*(PicoOpaqueFunction *)**(uint32_t **)self)(self, value & 0xff);
        return print_print_number_result;
    }
    print_print_number_result = Print_print_number(self, value, (uint8_t)radix);
    return print_print_number_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x20019790. */
uint32_t Print_println_unsigned(void *self, uint32_t value, int radix)
{
    uint32_t print_print_unsigned_result;
    uint32_t print_println_result;

    print_print_unsigned_result = Print_print_unsigned(self, value, radix);
    print_println_result = Print_println(self);
    return print_println_result + print_print_unsigned_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x200197a4. */
uint32_t Print_print_float(void *self, float value, uint8_t digits)
{
    int __aeabi_fcmplt_result;
    uint32_t print_print_char_result;
    uint32_t __aeabi_f2uiz_result;
    float __aeabi_ui2f_result;
    float __aeabi_fdiv_result;
    uint32_t unresolved_input_register_00000029;
    uint32_t value_6;
    uint8_t index;

    value_6 = pico_join_u24_u8(unresolved_input_register_00000029, digits);
    __aeabi_fcmplt_result = __aeabi_fcmplt(value, 0.0);
    if (__aeabi_fcmplt_result == 0) {
        print_print_char_result = 0;
    } else {
        print_print_char_result = Print_print_char(self, '-');
        value = (float)((int)value + -0x80000000);
    }
    __aeabi_fdiv_result = 0.5;
    for (index = 0; index < value_6; index = index + 1) {
        __aeabi_fdiv_result = __aeabi_fdiv(__aeabi_fdiv_result, 10.0);
    }
    __aeabi_fdiv_result = __aeabi_fadd(value, __aeabi_fdiv_result);
    __aeabi_f2uiz_result = __aeabi_f2uiz(__aeabi_fdiv_result);
    __aeabi_ui2f_result = __aeabi_ui2f(__aeabi_f2uiz_result);
    __aeabi_fdiv_result = __aeabi_fsub(__aeabi_fdiv_result, __aeabi_ui2f_result);
    __aeabi_f2uiz_result = Print_print_number(self, __aeabi_f2uiz_result, '\n');
    print_print_char_result = print_print_char_result + __aeabi_f2uiz_result;
    if (value_6 != 0) {
        __aeabi_f2uiz_result = Print_write_string(self, ".");
        print_print_char_result = print_print_char_result + __aeabi_f2uiz_result;
    }
    for (; value_6 != 0; value_6 = value_6 - 1 & 0xff) {
        __aeabi_fdiv_result = __aeabi_fmul(__aeabi_fdiv_result, 10.0);
        __aeabi_fcmplt_result = __aeabi_f2iz(__aeabi_fdiv_result);
        __aeabi_f2uiz_result = Print_print_signed(self, __aeabi_fcmplt_result, 10);
        print_print_char_result = print_print_char_result + __aeabi_f2uiz_result;
        __aeabi_ui2f_result = __aeabi_i2f(__aeabi_fcmplt_result);
        __aeabi_fdiv_result = __aeabi_fsub(__aeabi_fdiv_result, __aeabi_ui2f_result);
    }
    return print_print_char_result;
}

/* Source/control-flow match to Energia cores/cc3200/Print.cpp or inline Print.h method; virtual
 * byte/buffer write dispatch. Firmware address: 0x20019860. */
uint32_t Print_println_float(void *self, float value, uint8_t digits)
{
    uint32_t print_print_float_result;
    uint32_t print_println_result;

    print_print_float_result = Print_print_float(self, value, digits);
    print_println_result = Print_println(self);
    return print_println_result + print_print_float_result;
}
