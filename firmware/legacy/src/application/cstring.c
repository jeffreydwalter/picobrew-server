/* Pico Pro 0.1.34 — cstring. */
#include "pico/firmware.h"
#include <stdarg.h>

/* NULL or first byte zero predicate; explicit CString assertions at callers.
 * Firmware address: 0x20004888. */
/* Firmware entry: 0x20004888. Reviewed source reconstruction. */
int pico_cstring_is_null_or_empty(char *text)
{
    return text == 0 || text[0] == '\0';
}

/* Bounded copy with explicit NULL/capacity checks, NUL terminator, truncation logging.
 * Firmware address: 0x20004ad4. */
uint32_t pico_cstring_copy(char *destination, uint32_t capacity, char *source)
{
    uint32_t source_length;
    char *message;
    uint32_t copied_length;

    if (source == (char *)0x0) {
        source_length = 0;
    } else {
        source_length = libc_strlen(source);
    }
    if (destination == (char *)0x0) {
        pico_log_serial("W", "CSTR", "Invalid arg: dest is NULL");
        return 0;
    }
    if (capacity == 0) {
        message = "Invalid arg: destsize <= 0";
        copied_length = 0;
    branch_1:
        pico_log_serial("W", "CSTR", message);
        return copied_length;
    }
    *destination = '\0';
    if (source == (char *)0x0) {
        pico_log_serial("W", "CSTR", "Invalid arg: src is NULL");
        return 0;
    }
    if (source_length == 0) {
        pico_log_serial("W", "CSTR", "Invalid arg: srclen <= 0");
    } else {
        copied_length = capacity - 1;
        if (source_length < capacity - 1) {
            copied_length = source_length;
        }
        libc_strncpy(destination, source, copied_length);
        destination[copied_length] = '\0';
        if (copied_length != source_length) {
            message = "String truncated on copy";
            goto branch_1;
        }
    }
    return source_length;
}

/* Same NULL/empty predicate as20004888.
 * Firmware address: 0x200064c4. */
/* Firmware entry: 0x200064c4. Reviewed source reconstruction. */
int pico_cstring_is_null_or_empty_duplicate(char *text)
{
    return text == 0 || text[0] == '\0';
}
