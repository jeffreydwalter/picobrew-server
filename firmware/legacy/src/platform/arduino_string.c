/* Pico Pro 0.1.34 — arduino string. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Pinned Energia WString.cpp methods and buffer/capacity/length fields0/4/8; realloc(size+1),
 * invalidation, reserve and copy flows match. Firmware address: 0x20019876. */
uint32_t *String_destroy(uint32_t *text)
{
    libc_free((void *)*text);
    return text;
}

/* Pinned Energia WString.cpp methods and buffer/capacity/length fields0/4/8; realloc(size+1),
 * invalidation, reserve and copy flows match. Firmware address: 0x20019884. */
void String_invalidate(uint32_t *text)
{
    if ((void *)*text != (void *)0x0) {
        libc_free((void *)*text);
    }
    *text = 0;
    text[2] = 0;
    text[1] = 0;
    return;
}

/* Pinned Energia WString.cpp methods and buffer/capacity/length fields0/4/8; realloc(size+1),
 * invalidation, reserve and copy flows match. Firmware address: 0x2001989a. */
bool String_change_buffer(uint32_t *text, int capacity)
{
    void *libc_realloc_result;

    libc_realloc_result = PICO_UNRESOLVED_RETURN(libc_realloc((void *)*text, capacity + 1));
    if (libc_realloc_result != (void *)0x0) {
        *text = libc_realloc_result;
        text[1] = capacity;
    }
    return libc_realloc_result != (void *)0x0;
}

/* Pinned Energia WString.cpp methods and buffer/capacity/length fields0/4/8; realloc(size+1),
 * invalidation, reserve and copy flows match. Firmware address: 0x200198b2. */
bool String_reserve(int *text, uint32_t capacity)
{
    bool string_change_buffer_result;

    if ((*text == 0) || ((uint32_t)text[1] < capacity)) {
        string_change_buffer_result = String_change_buffer(text, capacity);
        if (!string_change_buffer_result) {
            return string_change_buffer_result;
        }
        if (text[2] == 0) {
            *(uint8_t *)*text = 0;
        }
    }
    return true;
}

/* Pinned Energia WString.cpp methods and buffer/capacity/length fields0/4/8; realloc(size+1),
 * invalidation, reserve and copy flows match. Firmware address: 0x200198d4. */
int *String_copy(int *text, char *source, uint32_t length)
{
    bool string_reserve_result;

    string_reserve_result = String_reserve(text, length);
    if (string_reserve_result) {
        text[2] = length;
        libc_strcpy((char *)*text, source);
    } else {
        String_invalidate(text);
    }
    return text;
}

/* Pinned Energia WString.cpp methods and buffer/capacity/length fields0/4/8; realloc(size+1),
 * invalidation, reserve and copy flows match. Firmware address: 0x200198fa. */
int *String_assign_c_string(int *text, char *source)
{
    uint32_t text_length;

    if (source == (char *)0x0) {
        String_invalidate(text);
    } else {
        text_length = libc_strlen(source);
        String_copy(text, source, text_length);
    }
    return text;
}

/* Pinned Energia WString.cpp methods and buffer/capacity/length fields0/4/8; realloc(size+1),
 * invalidation, reserve and copy flows match. Firmware address: 0x2001991c. */
int *String_construct_c_string(int *text, char *source)
{
    *text = 0;
    text[1] = 0;
    text[2] = 0;
    String_assign_c_string(text, source);
    return text;
}

/* Pinned Energia WString.cpp methods and buffer/capacity/length fields0/4/8; realloc(size+1),
 * invalidation, reserve and copy flows match. Firmware address: 0x20019930. */
uint8_t String_character_at(int *text, uint32_t index)
{
    if ((index < (uint32_t)text[2]) && (*text != 0)) {
        return *(uint8_t *)(*text + index);
    }
    return 0;
}
