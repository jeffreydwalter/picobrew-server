#ifndef PICOBREW_HTTP_SERVER_H
#define PICOBREW_HTTP_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PICO_HTTP_ARENA_CAPACITY = 4096,
    PICO_HTTP_PATH_CAPACITY = 128,
    PICO_HTTP_LINE_CAPACITY = 512,
    PICO_HTTP_HEADER_LIMIT = 1536,
    PICO_HTTP_IO_BUDGET = 512,
    PICO_HTTP_IO_CALL_BUDGET = 8,
    PICO_HTTP_KEY_LENGTH = 64,
    PICO_HTTP_IO_AGAIN = -1,
    PICO_HTTP_IO_ERROR = -2
};

typedef enum PicoHttpMethod {
    PICO_HTTP_GET,
    PICO_HTTP_POST,
    PICO_HTTP_PUT,
    PICO_HTTP_PATCH,
    PICO_HTTP_DELETE,
    PICO_HTTP_HEAD,
    PICO_HTTP_OPTIONS
} PicoHttpMethod;

typedef struct PicoHttpRequest {
    PicoHttpMethod method;
    char path[PICO_HTTP_PATH_CAPACITY];
    const char *body;
    size_t body_length;
    bool authenticated;
    bool auth_configured;
} PicoHttpRequest;

typedef struct PicoHttpResponse {
    unsigned status;
    const char *content_type;
    const uint8_t *body;
    size_t body_length;
    const char *content_encoding;
} PicoHttpResponse;

/* body aliases arena. A handler must consume/validate/persist its input before
 * overwriting arena with the response. A response may instead point to stable
 * immutable data. That memory must remain valid until the next request. */
typedef bool (*PicoHttpHandler)(const PicoHttpRequest *request, PicoHttpResponse *response,
                                char *arena, size_t capacity, void *context);

typedef struct PicoHttpAsset {
    const char *path;
    const char *content_type;
    const char *content_encoding;
    const uint8_t *data;
    size_t length;
    /* A non-NULL path streams an installed file instead of data. The expected
     * length belongs to this firmware build, not to the untrusted request. */
    const char *file_path;
} PicoHttpAsset;

/* File callbacks use their own handle, independent of the recovered SerFlash
 * stream. open verifies the expected length and returns a handle >=0 or ERROR.
 * It may perform at most two native operations (stat and open). read performs
 * one bounded read and must return exactly length; short/zero/error aborts the
 * response. close is called once for every successful open, including HEAD,
 * errors, deadlines and shutdown. File callbacks run only in foreground poll. */
typedef struct PicoHttpAssetIo {
    int (*open)(void *context, const char *path, size_t expected_length);
    int (*read)(void *context, int handle, size_t offset, uint8_t *data, size_t length);
    void (*close)(void *context, int handle);
} PicoHttpAssetIo;

/* All callbacks must be nonblocking. accept returns a descriptor >=0 or AGAIN.
 * receive/send return a positive count, AGAIN, or ERROR; receive=0 is EOF.
 * Descriptor zero is valid. No callback is invoked from an interrupt here. */
typedef struct PicoHttpIo {
    int (*accept)(void *context);
    int (*receive)(void *context, int client, uint8_t *data, size_t capacity);
    int (*send)(void *context, int client, const uint8_t *data, size_t length);
    void (*close)(void *context, int client);
} PicoHttpIo;

typedef struct PicoHttpServer {
    const PicoHttpIo *io;
    void *io_context;
    PicoHttpHandler handler;
    void *handler_context;
    const PicoHttpAsset *assets;
    size_t asset_count;
    const PicoHttpAssetIo *asset_io;
    void *asset_context;
    const PicoHttpAsset *pending_asset;
    PicoHttpRequest request;
    PicoHttpResponse response;
    char arena[PICO_HTTP_ARENA_CAPACITY];
    char line[PICO_HTTP_LINE_CAPACITY];
    char key[PICO_HTTP_KEY_LENGTH + 1];
    size_t line_length;
    size_t header_bytes;
    size_t content_length;
    size_t body_received;
    size_t header_length;
    size_t header_sent;
    size_t body_sent;
    size_t asset_chunk_length;
    size_t asset_chunk_sent;
    uint32_t started_ms;
    int client;
    int asset_handle;
    uint8_t stage;
    bool in_poll;
    bool saw_cr;
    bool http_11;
    bool has_host;
    bool has_length;
    bool has_authorization;
    bool has_content_type;
    bool content_type_json;
} PicoHttpServer;

/* Initialization and key configuration do not perform device I/O. */
void pico_http_server_initialize(PicoHttpServer *server, const PicoHttpIo *io, void *io_context,
                                  PicoHttpHandler handler, void *handler_context,
                                  const PicoHttpAsset *assets, size_t asset_count);
/* Configure immediately after initialize, before polling/accepting clients. */
void pico_http_server_set_asset_io(PicoHttpServer *server, const PicoHttpAssetIo *io,
                                    void *context);
/* Accept exactly 64 lowercase hex characters, optionally followed by LF/CRLF.
 * Invalid input clears the key, leaving protected endpoints unavailable. */
bool pico_http_server_set_key(PicoHttpServer *server, const char *key, size_t length);
void pico_http_server_poll(PicoHttpServer *server, uint32_t now_ms);
void pico_http_server_disconnect(PicoHttpServer *server);
const char *pico_http_method_name(PicoHttpMethod method);

#ifdef __cplusplus
}
#endif
#endif
