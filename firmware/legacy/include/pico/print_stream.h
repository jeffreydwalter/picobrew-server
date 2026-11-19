#ifndef PICO_PRINT_STREAM_H
#define PICO_PRINT_STREAM_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint32_t Print_write_buffer(void *self, uint8_t *buffer, uint32_t count);
uint32_t Print_write_string(void *self, char *text);
uint32_t Print_print_String(void *self, void *text);
uint32_t Print_print_string(void *self, char *text);
uint32_t Print_print_char(void *self, char value);
uint32_t Print_println(void *self);
uint32_t Print_println_String(void *self, void *text);
uint32_t Print_println_string(void *self, char *text);
uint32_t Print_print_number(void *self, uint32_t value, uint8_t radix);
uint32_t Print_print_signed(void *self, int value, int radix);
uint32_t Print_print_signed_alias(void *self, int value, int radix);
uint32_t Print_println_signed(void *self, long value, int radix);
uint32_t Print_print_unsigned(void *self, uint32_t value, int radix);
uint32_t Print_print_unsigned_alias_a(void *self, uint32_t value, int radix);
uint32_t Print_print_unsigned_alias_b(void *self, uint32_t value, int radix);
uint32_t Print_println_unsigned(void *self, uint32_t value, int radix);
uint32_t Print_print_float(void *self, float value, uint8_t digits);
uint32_t Print_println_float(void *self, float value, uint8_t digits);

#endif /* PICO_PRINT_STREAM_H */
