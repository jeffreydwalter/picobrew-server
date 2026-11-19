#ifndef PICO_FORMATTING_H
#define PICO_FORMATTING_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

char *pico_format_float_decimal(float argument_1, char *destination, uint32_t decimal_places);

#endif /* PICO_FORMATTING_H */
