#ifndef PICOBREW_TEXT_H
#define PICOBREW_TEXT_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PicoTextLog)(void *context, const char *level, const char *component,
                            const char *message);

bool pico_text_is_empty(const char *text);
/* log is required whenever a null/empty/truncated argument can be supplied.
 * Returns the number copied, excluding NUL; warnings match original CString.
 * As in the original, source length is evaluated before destination checks. */
uint32_t pico_text_copy(char *destination, uint32_t capacity, const char *source, PicoTextLog log,
                        void *context);

#ifdef __cplusplus
}
#endif
#endif
