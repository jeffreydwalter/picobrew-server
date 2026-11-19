#ifndef PICO_NETWORK_INTERNAL_H
#define PICO_NETWORK_INTERNAL_H
#include "picobrew/network.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline uint32_t net_now(PicoNetwork *n)
{
    return n->platform->millis(n->platform_context);
}
static inline void net_delay(PicoNetwork *n, uint32_t ms)
{
    n->platform->delay_ms(n->platform_context, ms);
}
static inline void net_serial(PicoNetwork *n, const char *s)
{
    n->platform->serial_write(n->platform_context, s);
}
static inline void net_line(PicoNetwork *n, const char *s)
{
    net_serial(n, s);
    net_serial(n, "\r\n");
}
static inline void net_ui(PicoNetwork *n, PicoNetworkUiEvent e, const char *s)
{
    n->ui->event(n->ui_context, e, s);
}
static inline void net_error(PicoNetwork *n, int e, bool wait, bool report)
{
    n->ui->report_error(n->ui_context, e, wait, report);
}
static inline bool net_put(PicoNetwork *n, const char *s)
{
    size_t length = strlen(s);
    return n->platform->client_write(n->platform_context, (const uint8_t *)s, length) == length;
}
static inline int32_t net_truncate_signed(double x)
{
    if (x != x)
        return 0;
    if (x <= -2147483648.0)
        return INT32_MIN;
    if (x >= 2147483647.0)
        return INT32_MAX;
    return (int32_t)x;
}
static inline uint32_t net_truncate_unsigned(float x)
{
    if (!(x > 0.0f))
        return 0;
    if ((double)x >= 4294967295.0)
        return UINT32_MAX;
    return (uint32_t)x;
}
static inline bool net_request_fits(int count, size_t capacity)
{
    return count >= 0 && (size_t)count < capacity;
}
static inline int net_prefix(PicoNetwork *n, char *out, size_t cap, const char *method)
{
    return snprintf(out, cap, "/API/pico/%s?uid=%s", method, n->app->device_uid);
}
static inline void net_copy(char *dst, size_t cap, const char *src)
{
    if (cap) {
        size_t size = strlen(src);
        if (size >= cap)
            size = cap - 1;
        memcpy(dst, src, size);
        dst[size] = 0;
    }
}
#endif
