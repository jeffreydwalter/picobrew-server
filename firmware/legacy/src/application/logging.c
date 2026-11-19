/* Pico Pro 0.1.34 — logging. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Formats severity/component prefix and writes message to Serial.
 * Firmware address: 0x20004a94. */
void pico_log_serial(char *level, char *component, char *message)
{
    millis();
    libc_snprintf(&g_log_prefix_buffer, 0x80, "[%s][%s] ", level, component);
    Print_print_string(&global_serial0, &g_log_prefix_buffer);
    Print_println_string(&global_serial0, message);
    return;
}

/* Failed condition increments assertion counter and prints expression without aborting.
 * Firmware address: 0x20004b7c. */
void pico_assert_nonfatal(int condition, char *expression)
{
    if (condition == 0) {
        g_assertion_count = g_assertion_count + 1;
        Print_print_string(&global_serial0, "***** ASSERT [");
        Print_print_string(&global_serial0, expression);
        Print_println_string(&global_serial0, "] *****");
        return;
    }
    return;
}

/* Prints four IPAddress octets at offsets4..7 separated by literal dots.
 * Firmware address: 0x2000632c. */
void pico_log_ipv4_address(void *ip_address)
{
    Print_print_unsigned_alias_a(&global_serial0, (uint32_t) * (uint8_t *)((int)ip_address + 4),
                                 10);
    Print_print_string(&global_serial0, ".");
    Print_print_unsigned_alias_a(&global_serial0, (uint32_t) * (uint8_t *)((int)ip_address + 5),
                                 10);
    Print_print_string(&global_serial0, ".");
    Print_print_unsigned_alias_a(&global_serial0, (uint32_t) * (uint8_t *)((int)ip_address + 6),
                                 10);
    Print_print_string(&global_serial0, ".");
    Print_print_unsigned_alias_a(&global_serial0, (uint32_t) * (uint8_t *)((int)ip_address + 7),
                                 10);
    return;
}
