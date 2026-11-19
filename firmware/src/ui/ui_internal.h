#ifndef PICOBREW_UI_INTERNAL_H
#define PICOBREW_UI_INTERNAL_H
#include "picobrew/ui.h"
#include "picobrew/text.h"
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct PicoInstructionText {
    uint32_t count;
    const char *lines[6];
} PicoInstructionText;
#include "ui_assets.h"
extern PicoUi *pico_ui_active;
extern const char *const pico_action_names[50];
extern const char *const pico_action_labels[50][3];
static inline uint32_t ui_millis(void)
{
    return pico_ui_active->hardware->millis(pico_ui_active->hardware_context);
}
static inline void ui_delay(uint32_t n)
{
    pico_ui_active->hardware->delay_ms(pico_ui_active->hardware_context, n);
}
static inline int ui_read(unsigned pin)
{
    return pico_ui_active->hardware->digital_read(pico_ui_active->hardware_context, pin);
}
static inline void ui_write(unsigned pin, unsigned value)
{
    pico_ui_active->hardware->digital_write(pico_ui_active->hardware_context, pin, value);
}
static inline void ui_pin_mode(unsigned pin, unsigned mode)
{
    pico_ui_active->hardware->pin_mode(pico_ui_active->hardware_context, pin, mode);
}
static inline uint8_t ui_spi(uint8_t v)
{
    return pico_ui_active->hardware->spi_transfer(pico_ui_active->hardware_context, v);
}
static inline void ui_interrupts_disable(void)
{
    pico_ui_active->hardware->interrupts_disable(pico_ui_active->hardware_context);
}
static inline bool ui_interrupts_enable(void)
{
    return pico_ui_active->hardware->interrupts_enable(pico_ui_active->hardware_context);
}
static inline void ui_attach(unsigned n, void (*h)(void), unsigned mode)
{
    pico_ui_active->hardware->attach_interrupt(pico_ui_active->hardware_context, n, h, mode);
}
static inline void ui_log(const char *level, const char *component, const char *message)
{
    pico_network_log(pico_ui_active->network, level, component, message);
}
static inline void ui_assert(bool condition, const char *expression)
{
    pico_network_assert(pico_ui_active->network, condition, expression);
}
static inline bool ui_empty(const char *s)
{
    return pico_text_is_empty(s);
}
static inline void ui_text_log(void *context, const char *level, const char *component,
                               const char *message)
{
    pico_network_log(context, level, component, message);
}
static inline uint32_t ui_cstring_copy(char *dst, uint32_t capacity, const char *src)
{
    return pico_text_copy(dst, capacity, src, ui_text_log, pico_ui_active->network);
}
/* Explicit bounds repair for original plain strcpy paths receiving remote
 * strings. Original CString calls instead use ui_cstring_copy below. */
static inline void ui_copy(char *dst, size_t capacity, const char *src)
{
    if (capacity) {
        size_t n = strlen(src);
        if (n >= capacity)
            n = capacity - 1;
        memcpy(dst, src, n);
        dst[n] = 0;
    }
}
char *ui_itoa(int value, char *destination, int base);
static inline int ui_int(double x)
{
    if (isnan(x))
        return 0;
    if (x >= 2147483648.0)
        return INT32_MAX;
    if (x <= -2147483648.0)
        return INT32_MIN;
    return (int32_t)x;
}
static inline uint32_t ui_uint(float x)
{
    if (isnan(x) || x <= 0)
        return 0;
    if (x >= 4294967296.0f)
        return UINT32_MAX;
    return (uint32_t)x;
}
static inline double ui_display_temperature(double f)
{
    return pico_ui_active->network->settings->temperature_unit_celsius ? (f - 32.0) / 1.8 : f;
}
#endif
