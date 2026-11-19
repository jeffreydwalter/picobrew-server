#ifndef PICO_STRINGS_H
#define PICO_STRINGS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_copy_firmware_version(char *destination);
void pico_append_program_state_label(char *destination);
void pico_append_program_name(char *destination);

#endif /* PICO_STRINGS_H */
