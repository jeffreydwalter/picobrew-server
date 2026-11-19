/* Pico Pro 0.1.34 — strings. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Copies embedded version string v0.1.34.
 * Firmware address: 0x20005544. */
void pico_copy_firmware_version(char *destination)
{
    libc_strcpy(destination, "v0.1.34");
    return;
}

/* Appends program state label at offset0x3c; program logger explicitly labels this field state.
 * Firmware address: 0x200084f0. */
void pico_append_program_state_label(char *destination)
{
    libc_strcat(destination, *(char **)(g_active_program_context_as_int + 0x3c));
    return;
}

/* Appends program name at offset0x38 after Start/Starting/Ending prefixes; program logger
 * explicitly names field. Firmware address: 0x20008500. */
void pico_append_program_name(char *destination)
{
    libc_strcat(destination, *(char **)(g_active_program_context_as_int + 0x38));
    return;
}
