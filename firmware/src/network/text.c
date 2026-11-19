#include "picobrew/text.h"
#include <string.h>

bool pico_text_is_empty(const char *text)
{
    return !text || !*text;
}

uint32_t pico_text_copy(char *destination, uint32_t capacity, const char *source, PicoTextLog log,
                        void *context)
{
    uint32_t length = source ? (uint32_t)strlen(source) : 0;
    if (!destination) {
        log(context, "W", "CSTR", "Invalid arg: dest is NULL");
        return 0;
    }
    if (!capacity) {
        log(context, "W", "CSTR", "Invalid arg: destsize <= 0");
        return 0;
    }
    *destination = 0;
    if (!source) {
        log(context, "W", "CSTR", "Invalid arg: src is NULL");
        return 0;
    }
    if (!length) {
        log(context, "W", "CSTR", "Invalid arg: srclen <= 0");
        return 0;
    }
    uint32_t copied = length < capacity - 1 ? length : capacity - 1;
    strncpy(destination, source, copied);
    destination[copied] = 0;
    if (copied != length)
        log(context, "W", "CSTR", "String truncated on copy");
    return copied;
}
