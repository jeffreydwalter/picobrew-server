#ifndef PICOBREW_LOCAL_API_H
#define PICOBREW_LOCAL_API_H

#include "picobrew/application.h"
#include "picobrew/http_server.h"
#include "picobrew/local_store.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { PICO_LOCAL_EVENT_CAPACITY = 8 };
typedef struct PicoLocalEvent {
    uint32_t sequence;
    uint32_t uptime_ms;
    char kind[16];
    char detail[48];
} PicoLocalEvent;
typedef struct PicoLocalApi {
    PicoApplication *application;
    PicoLocalEvent events[PICO_LOCAL_EVENT_CAPACITY];
    uint32_t next_sequence;
    uint8_t event_head;
    uint8_t event_count;
    bool auth_configured;
} PicoLocalApi;

/* One appliance is bound; initialization does not perform I/O or enable an
 * output. The transport authenticates API calls before reaching the dispatcher.
 * Body and response may alias: all request data is consumed before output. */
void pico_local_api_initialize(PicoLocalApi *api, PicoApplication *application);
int pico_local_api_handle(PicoLocalApi *api, const char *method, const char *path,
                          const char *body, size_t body_length, char *response,
                          size_t capacity, size_t *response_length);
bool pico_local_api_http_handler(const PicoHttpRequest *request, PicoHttpResponse *response,
                                 char *arena, size_t capacity, void *context);

#ifdef __cplusplus
}
#endif
#endif
