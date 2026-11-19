#ifndef PICO_CSTRING_H
#define PICO_CSTRING_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int pico_cstring_is_null_or_empty(char *text);
uint32_t pico_cstring_copy(char *destination, uint32_t capacity, char *source);
int pico_cstring_is_null_or_empty_duplicate(char *text);

#endif /* PICO_CSTRING_H */
