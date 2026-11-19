#ifndef PICO_ARDUINO_STRING_H
#define PICO_ARDUINO_STRING_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint32_t *String_destroy(uint32_t *text);
void String_invalidate(uint32_t *text);
bool String_change_buffer(uint32_t *text, int capacity);
bool String_reserve(int *text, uint32_t capacity);
int *String_copy(int *text, char *source, uint32_t length);
int *String_assign_c_string(int *text, char *source);
int *String_construct_c_string(int *text, char *source);
uint8_t String_character_at(int *text, uint32_t index);

#endif /* PICO_ARDUINO_STRING_H */
