#ifndef PICOBREW_ENERGIA_HTTP_SERVER_H
#define PICOBREW_ENERGIA_HTTP_SERVER_H
#include "picobrew/http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PICO_LOCAL_API_KEY_FILE "/local/api.key"

typedef struct PicoEnergiaHttpServer {
    PicoHttpServer http;
    int listener;
    uint16_t port;
    uint32_t last_attempt_ms;
    bool attempted;
    bool key_loaded;
    bool in_poll;
} PicoEnergiaHttpServer;

/* Initialization binds callbacks only. File/socket I/O starts in poll after
 * the existing application has initialized the network processor and WiFi.
 * Installed web assets use one independent native SLFS read handle. Each poll
 * reads at most 512 asset bytes into the existing HTTP arena, and all client
 * completion/error/deadline/shutdown paths release that handle. */
void pico_energia_http_initialize(PicoEnergiaHttpServer *server, PicoHttpHandler handler,
                                   void *context, const PicoHttpAsset *assets,
                                   size_t asset_count, uint16_t port);
void pico_energia_http_poll(PicoEnergiaHttpServer *server);
void pico_energia_http_shutdown(PicoEnergiaHttpServer *server);

/* Separate filesystem handle: never interferes with the global SerFlash
 * stream used by recovered settings/restart code. Only the SDK's explicit
 * not-found error maps to1. Other negative errors must fail closed. */
int pico_energia_local_file_stat(void *context, const char *path, size_t *size);
/* One bounded native read, with an independent handle and close on every
 * opened path. Returns bytes read, or a negative read/close error. */
int32_t pico_energia_local_file_read(void *context, const char *path, uint8_t *data,
                                    size_t length);

#ifdef __cplusplus
}
#endif
#endif
