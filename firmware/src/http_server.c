#include "picobrew/http_server.h"
#include <stdio.h>
#include <string.h>

enum { IDLE, REQUEST_LINE, HEADERS, BODY, COMPLETE, OPENING_ASSET, SENDING };
enum { REQUEST_TIMEOUT_MS = 5000, RESPONSE_TIMEOUT_MS = 8000 };

static char ascii_lower(char c)
{
    return c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;
}

static bool same_casefold(const char *a, const char *b)
{
    while (*a && *b && ascii_lower(*a) == ascii_lower(*b)) {
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

const char *pico_http_method_name(PicoHttpMethod method)
{
    static const char *const names[] = {"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"};
    return (unsigned)method < sizeof(names) / sizeof(names[0]) ? names[method] : "";
}

static const char *reason(unsigned status)
{
    switch (status) {
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 411: return "Length Required";
    case 413: return "Content Too Large";
    case 415: return "Unsupported Media Type";
    case 417: return "Expectation Failed";
    case 422: return "Unprocessable Content";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 501: return "Not Implemented";
    case 503: return "Service Unavailable";
    default: return "Internal Server Error";
    }
}

void pico_http_server_initialize(PicoHttpServer *s, const PicoHttpIo *io, void *io_context,
                                  PicoHttpHandler handler, void *handler_context,
                                  const PicoHttpAsset *assets, size_t asset_count)
{
    memset(s, 0, sizeof(*s));
    s->io = io;
    s->io_context = io_context;
    s->handler = handler;
    s->handler_context = handler_context;
    s->assets = assets;
    s->asset_count = asset_count;
    s->client = -1;
    s->asset_handle = -1;
}

void pico_http_server_set_asset_io(PicoHttpServer *s, const PicoHttpAssetIo *io, void *context)
{
    s->asset_io = io;
    s->asset_context = context;
}

bool pico_http_server_set_key(PicoHttpServer *s, const char *key, size_t length)
{
    memset(s->key, 0, sizeof(s->key));
    if (!key)
        return false;
    if (length == PICO_HTTP_KEY_LENGTH + 1 && key[length - 1] == '\n')
        --length;
    else if (length == PICO_HTTP_KEY_LENGTH + 2 && key[length - 2] == '\r' && key[length - 1] == '\n')
        length -= 2;
    if (length != PICO_HTTP_KEY_LENGTH)
        return false;
    for (size_t i = 0; i < length; ++i)
        if (!((key[i] >= '0' && key[i] <= '9') || (key[i] >= 'a' && key[i] <= 'f')))
            return false;
    memcpy(s->key, key, length);
    return true;
}

static void close_asset(PicoHttpServer *s)
{
    if (s->asset_handle >= 0)
        s->asset_io->close(s->asset_context, s->asset_handle);
    s->asset_handle = -1;
    s->pending_asset = NULL;
    s->asset_chunk_length = s->asset_chunk_sent = 0;
}

void pico_http_server_disconnect(PicoHttpServer *s)
{
    close_asset(s);
    if (s->client >= 0)
        s->io->close(s->io_context, s->client);
    s->client = -1;
    s->stage = IDLE;
    /* Erase authorization/body material left by the previous client. */
    memset(&s->request, 0, sizeof(s->request));
    memset(&s->response, 0, sizeof(s->response));
    memset(s->arena, 0, sizeof(s->arena));
    memset(s->line, 0, sizeof(s->line));
}

static void begin_request(PicoHttpServer *s, int client, uint32_t now)
{
    memset(&s->request, 0, sizeof(s->request));
    memset(&s->response, 0, sizeof(s->response));
    s->client = client;
    s->stage = REQUEST_LINE;
    s->started_ms = now;
    s->line_length = s->header_bytes = s->content_length = s->body_received = 0;
    s->header_length = s->header_sent = s->body_sent = 0;
    s->asset_chunk_length = s->asset_chunk_sent = 0;
    s->pending_asset = NULL;
    s->saw_cr = s->http_11 = s->has_host = s->has_length = false;
    s->has_authorization = s->has_content_type = s->content_type_json = false;
    s->request.body = s->arena;
    s->request.auth_configured = s->key[0] != '\0';
}

static bool safe_header_value(const char *text)
{
    if (!text || !*text)
        return false;
    for (; *text; ++text)
        if ((unsigned char)*text < 32 || (unsigned char)*text > 126)
            return false;
    return true;
}

static void prepare_response(PicoHttpServer *s, uint32_t now)
{
    PicoHttpResponse *r = &s->response;
    if (r->status < 200 || r->status > 599 || !safe_header_value(r->content_type) ||
        (r->content_encoding && !safe_header_value(r->content_encoding)) ||
        (r->body_length && !r->body)) {
        r->status = 500;
        r->content_type = "application/json";
        r->content_encoding = NULL;
        r->body = (const uint8_t *)"{\"error\":\"invalid_response\"}";
        r->body_length = strlen((const char *)r->body);
    }
    int count = snprintf(s->line, sizeof(s->line),
        "HTTP/1.1 %u %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n"
        "Connection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
        "Content-Security-Policy: default-src 'self'; object-src 'none'; frame-ancestors 'none'; base-uri 'none'\r\n"
        "%s%s%s%s\r\n",
        r->status, reason(r->status), r->content_type, (unsigned long)r->body_length,
        r->content_encoding ? "Content-Encoding: " : "",
        r->content_encoding ? r->content_encoding : "",
        r->content_encoding ? "\r\n" : "",
        r->status == 401 ? "WWW-Authenticate: Bearer realm=\"pico\"\r\n" : "");
    if (count < 0 || (size_t)count >= sizeof(s->line)) {
        pico_http_server_disconnect(s);
        return;
    }
    s->header_length = (size_t)count;
    s->header_sent = s->body_sent = 0;
    s->started_ms = now;
    s->stage = SENDING;
}

static void error_response(PicoHttpServer *s, unsigned status, const char *error, uint32_t now)
{
    int size = snprintf(s->arena, sizeof(s->arena), "{\"error\":\"%s\"}", error);
    s->response = (PicoHttpResponse){status, "application/json", (const uint8_t *)s->arena,
                                    size > 0 ? (size_t)size : 0, NULL};
    prepare_response(s, now);
}

static void asset_unavailable(PicoHttpServer *s, uint32_t now)
{
    close_asset(s);
    if (strcmp(s->request.path, "/") == 0 || strcmp(s->request.path, "/index.html") == 0) {
        static const char fallback[] =
            "<!doctype html><html lang=\"en\"><meta charset=\"utf-8\">"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<title>Pico Local</title><h1>Pico Local</h1>"
            "<p>The browser files are missing or do not match this firmware. "
            "Install the matching web files with the firmware installer.</p>"
            "<p>The local API remains available. <a href=\"/api/v1/health\">Device health</a></p></html>";
        s->response = (PicoHttpResponse){503, "text/html; charset=utf-8",
                                        (const uint8_t *)fallback, sizeof(fallback) - 1, NULL};
        prepare_response(s, now);
    } else {
        error_response(s, 503, "web_asset_unavailable", now);
    }
}

static bool authorized(const PicoHttpServer *s, const char *value)
{
    if (!s->key[0] || strlen(value) != PICO_HTTP_KEY_LENGTH + 7)
        return false;
    static const char prefix[] = "bearer ";
    unsigned difference = 0;
    for (size_t i = 0; i < 7; ++i)
        difference |= (unsigned char)ascii_lower(value[i]) ^ (unsigned char)prefix[i];
    for (size_t i = 0; i < PICO_HTTP_KEY_LENGTH; ++i)
        difference |= (unsigned char)value[i + 7] ^ (unsigned char)s->key[i];
    return difference == 0;
}

static bool parse_request_line(PicoHttpServer *s)
{
    char *method_end = strchr(s->line, ' ');
    if (!method_end)
        return false;
    *method_end++ = '\0';
    char *path_end = strchr(method_end, ' ');
    if (!path_end)
        return false;
    *path_end++ = '\0';
    if (strcmp(path_end, "HTTP/1.1") == 0)
        s->http_11 = true;
    else if (strcmp(path_end, "HTTP/1.0") != 0)
        return false;
    bool method_found = false;
    for (unsigned i = PICO_HTTP_GET; i <= PICO_HTTP_OPTIONS; ++i)
        if (strcmp(s->line, pico_http_method_name((PicoHttpMethod)i)) == 0) {
            s->request.method = (PicoHttpMethod)i;
            method_found = true;
            break;
        }
    size_t length = strlen(method_end);
    if (!method_found || !length || method_end[0] != '/' || length >= sizeof(s->request.path))
        return false;
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)method_end[i] <= 32 || (unsigned char)method_end[i] >= 127 ||
            method_end[i] == '\\' || method_end[i] == '#')
            return false;
    memcpy(s->request.path, method_end, length + 1);
    s->stage = HEADERS;
    return true;
}

static bool json_content_type(char *value)
{
    char *parameter = strchr(value, ';');
    if (parameter) {
        *parameter++ = '\0';
        while (*parameter == ' ' || *parameter == '\t')
            ++parameter;
        if (!same_casefold(parameter, "charset=utf-8") && !same_casefold(parameter, "charset=\"utf-8\""))
            return false;
    }
    size_t length = strlen(value);
    while (length && (value[length - 1] == ' ' || value[length - 1] == '\t'))
        value[--length] = '\0';
    return same_casefold(value, "application/json");
}

/* Return HTTP error status, zero if valid. */
static unsigned parse_header(PicoHttpServer *s)
{
    char *colon = strchr(s->line, ':');
    if (!colon || colon == s->line)
        return 400;
    for (char *p = s->line; p != colon; ++p)
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || *p == '-'))
            return 400;
    *colon++ = '\0';
    while (*colon == ' ' || *colon == '\t')
        ++colon;
    size_t length = strlen(colon);
    while (length && (colon[length - 1] == ' ' || colon[length - 1] == '\t'))
        colon[--length] = '\0';
    if (same_casefold(s->line, "Transfer-Encoding"))
        return 400;
    if (same_casefold(s->line, "Expect"))
        return 417;
    if (same_casefold(s->line, "Host")) {
        if (s->has_host || !length)
            return 400;
        for (size_t i = 0; i < length; ++i)
            if ((unsigned char)colon[i] <= 32 || colon[i] == '/' || colon[i] == '\\' || colon[i] == '@')
                return 400;
        s->has_host = true;
    } else if (same_casefold(s->line, "Content-Length")) {
        if (s->has_length || !length)
            return 400;
        s->has_length = true;
        size_t total = 0;
        for (size_t i = 0; i < length; ++i) {
            if (colon[i] < '0' || colon[i] > '9')
                return 400;
            if (total > (PICO_HTTP_ARENA_CAPACITY - 1) / 10)
                return 413;
            total = total * 10 + (size_t)(colon[i] - '0');
            if (total >= PICO_HTTP_ARENA_CAPACITY)
                return 413;
        }
        s->content_length = total;
    } else if (same_casefold(s->line, "Authorization")) {
        if (s->has_authorization)
            return 400;
        s->has_authorization = true;
        s->request.authenticated = authorized(s, colon);
    } else if (same_casefold(s->line, "Content-Type")) {
        if (s->has_content_type)
            return 400;
        s->has_content_type = true;
        s->content_type_json = json_content_type(colon);
    }
    return 0;
}

static bool api_path(const char *path)
{
    return strncmp(path, "/api/", 5) == 0 || strcmp(path, "/api") == 0;
}

static unsigned finish_headers(PicoHttpServer *s)
{
    if (s->http_11 && !s->has_host)
        return 400;
    bool mutation = s->request.method == PICO_HTTP_POST || s->request.method == PICO_HTTP_PUT ||
                    s->request.method == PICO_HTTP_PATCH;
    if (mutation && !s->has_length)
        return 411;
    if ((mutation || s->content_length) && !s->content_type_json)
        return 415;
    if ((s->request.method == PICO_HTTP_GET || s->request.method == PICO_HTTP_HEAD ||
         s->request.method == PICO_HTTP_OPTIONS) && s->content_length)
        return 400;
    bool public_health = s->request.method == PICO_HTTP_GET &&
                         strcmp(s->request.path, "/api/v1/health") == 0;
    if (api_path(s->request.path) && !public_health) {
        if (!s->request.auth_configured)
            return 503;
        if (!s->request.authenticated)
            return 401;
    }
    s->stage = s->content_length ? BODY : COMPLETE;
    s->arena[0] = '\0';
    return 0;
}

static void consume(PicoHttpServer *s, uint8_t byte, uint32_t now)
{
    if (s->stage == BODY) {
        s->arena[s->body_received++] = (char)byte;
        if (s->body_received == s->content_length) {
            s->arena[s->body_received] = '\0';
            s->stage = COMPLETE;
        }
        return;
    }
    if (++s->header_bytes > PICO_HTTP_HEADER_LIMIT) {
        error_response(s, 431, "headers_too_large", now);
        return;
    }
    if (s->saw_cr) {
        s->saw_cr = false;
        if (byte != '\n') {
            error_response(s, 400, "invalid_line_ending", now);
            return;
        }
        s->line[s->line_length] = '\0';
        unsigned error = 0;
        if (s->stage == REQUEST_LINE) {
            if (!parse_request_line(s))
                error = 400;
        } else if (s->line_length) {
            error = parse_header(s);
        } else {
            error = finish_headers(s);
        }
        s->line_length = 0;
        if (error)
            error_response(s, error, error == 401 ? "unauthorized" :
                           error == 503 ? "api_key_not_provisioned" : "invalid_request", now);
        return;
    }
    if (byte == '\r') {
        s->saw_cr = true;
        return;
    }
    if (byte < 32 && byte != '\t') {
        error_response(s, 400, "invalid_header_character", now);
        return;
    }
    if (byte >= 127 || s->line_length + 1 >= sizeof(s->line)) {
        error_response(s, byte >= 127 ? 400 : 431, "invalid_header", now);
        return;
    }
    s->line[s->line_length++] = (char)byte;
}

static void dispatch(PicoHttpServer *s, uint32_t now)
{
    s->request.body_length = s->body_received;
    if (api_path(s->request.path)) {
        if (s->request.method == PICO_HTTP_HEAD || s->request.method == PICO_HTTP_OPTIONS) {
            error_response(s, 405, "method_not_allowed", now);
            return;
        }
        s->response = (PicoHttpResponse){200, "application/json", (const uint8_t *)s->arena, 0, NULL};
        if (!s->handler || !s->handler(&s->request, &s->response, s->arena,
                                       sizeof(s->arena), s->handler_context)) {
            error_response(s, 500, "handler_failed", now);
            return;
        }
        /* The response may use all of arena, but may never escape its end. */
        uintptr_t body = (uintptr_t)s->response.body;
        uintptr_t start = (uintptr_t)s->arena;
        if (body >= start && body < start + sizeof(s->arena) &&
            s->response.body_length > start + sizeof(s->arena) - body) {
            error_response(s, 500, "response_too_large", now);
            return;
        }
        prepare_response(s, now);
        return;
    }
    if (s->request.method != PICO_HTTP_GET && s->request.method != PICO_HTTP_HEAD) {
        error_response(s, 405, "method_not_allowed", now);
        return;
    }
    for (size_t i = 0; i < s->asset_count; ++i) {
        const PicoHttpAsset *asset = &s->assets[i];
        if (strcmp(asset->path, s->request.path) == 0) {
            s->response = (PicoHttpResponse){200, asset->content_type, asset->data,
                                            asset->length, asset->content_encoding};
            if (asset->file_path) {
                s->pending_asset = asset;
                s->response.body = (const uint8_t *)s->arena;
                s->stage = OPENING_ASSET;
                return;
            }
            prepare_response(s, now);
            return;
        }
    }
    error_response(s, 404, "not_found", now);
}

void pico_http_server_poll(PicoHttpServer *s, uint32_t now)
{
    if (s->in_poll || !s->io || !s->io->accept || !s->io->receive || !s->io->send || !s->io->close)
        return;
    s->in_poll = true;
    size_t budget = PICO_HTTP_IO_BUDGET;
    /* Reserve file close and socket close on EOF/error/completion. Asset open
     * costs two operations (native stat/open). At most one 512-byte flash read
     * is performed per poll, reusing arena after request parsing is complete. */
    unsigned calls = PICO_HTTP_IO_CALL_BUDGET - 2;
    unsigned asset_reads = 1;
    if (s->stage == IDLE) {
        --calls;
        int client = s->io->accept(s->io_context);
        if (client < 0)
            goto done;
        begin_request(s, client, now);
    }
    if ((uint32_t)(now - s->started_ms) >
        (s->stage >= OPENING_ASSET ? RESPONSE_TIMEOUT_MS : REQUEST_TIMEOUT_MS)) {
        /* Close rather than extending a slow client's deadline with an error. */
        pico_http_server_disconnect(s);
        goto done;
    }
    while (budget && calls && s->stage < OPENING_ASSET) {
        uint8_t input[128];
        size_t wanted = budget < sizeof(input) ? budget : sizeof(input);
        --calls;
        int count = s->io->receive(s->io_context, s->client, input, wanted);
        if (count == PICO_HTTP_IO_AGAIN)
            break;
        if (count <= 0 || (size_t)count > wanted) {
            pico_http_server_disconnect(s);
            goto done;
        }
        budget -= (size_t)count;
        for (int i = 0; i < count; ++i) {
            if (s->stage == COMPLETE) {
                error_response(s, 400, "pipelining_not_supported", now);
                break;
            }
            consume(s, input[i], now);
            if (s->stage == SENDING)
                break;
        }
        if (s->stage == COMPLETE) {
            dispatch(s, now);
            break;
        }
    }
    if (s->stage == OPENING_ASSET && calls >= 3) {
        if (!s->asset_io || !s->asset_io->open || !s->asset_io->read || !s->asset_io->close) {
            asset_unavailable(s, now);
        } else {
            calls -= 2;
            s->asset_handle = s->asset_io->open(s->asset_context,
                s->pending_asset->file_path, s->pending_asset->length);
            if (s->asset_handle < 0) {
                asset_unavailable(s, now);
            } else if (s->request.method == PICO_HTTP_HEAD || !s->response.body_length) {
                prepare_response(s, now);
            } else {
                size_t wanted = s->response.body_length;
                if (wanted > PICO_HTTP_IO_BUDGET)
                    wanted = PICO_HTTP_IO_BUDGET;
                --calls;
                --asset_reads;
                int count = s->asset_io->read(s->asset_context, s->asset_handle, 0,
                                             (uint8_t *)s->arena, wanted);
                if (count != (int)wanted) {
                    asset_unavailable(s, now);
                } else {
                    s->asset_chunk_length = wanted;
                    s->asset_chunk_sent = 0;
                    prepare_response(s, now);
                }
            }
        }
    }
    while (budget && calls && s->stage == SENDING) {
        const uint8_t *data;
        size_t left;
        if (s->header_sent < s->header_length) {
            data = (const uint8_t *)s->line + s->header_sent;
            left = s->header_length - s->header_sent;
        } else if (s->request.method != PICO_HTTP_HEAD && s->body_sent < s->response.body_length) {
            if (s->asset_handle >= 0) {
                if (s->asset_chunk_sent == s->asset_chunk_length) {
                    if (!asset_reads)
                        break;
                    size_t wanted = s->response.body_length - s->body_sent;
                    if (wanted > PICO_HTTP_IO_BUDGET)
                        wanted = PICO_HTTP_IO_BUDGET;
                    --calls;
                    --asset_reads;
                    int count = s->asset_io->read(s->asset_context, s->asset_handle,
                        s->body_sent, (uint8_t *)s->arena, wanted);
                    if (count != (int)wanted) {
                        /* Headers/body may already be on the wire. A truncated
                         * Content-Length response fails closed; do not append
                         * JSON or retry a file that may have changed. */
                        pico_http_server_disconnect(s);
                        break;
                    }
                    s->asset_chunk_length = wanted;
                    s->asset_chunk_sent = 0;
                    if (!calls)
                        break;
                }
                data = (const uint8_t *)s->arena + s->asset_chunk_sent;
                left = s->asset_chunk_length - s->asset_chunk_sent;
            } else {
                data = s->response.body + s->body_sent;
                left = s->response.body_length - s->body_sent;
            }
        } else {
            pico_http_server_disconnect(s);
            break;
        }
        if (left > budget)
            left = budget;
        --calls;
        int count = s->io->send(s->io_context, s->client, data, left);
        if (count == PICO_HTTP_IO_AGAIN)
            break;
        if (count <= 0 || (size_t)count > left) {
            pico_http_server_disconnect(s);
            break;
        }
        budget -= (size_t)count;
        if (s->header_sent < s->header_length)
            s->header_sent += (size_t)count;
        else {
            s->body_sent += (size_t)count;
            if (s->asset_handle >= 0)
                s->asset_chunk_sent += (size_t)count;
        }
    }
done:
    s->in_poll = false;
}
