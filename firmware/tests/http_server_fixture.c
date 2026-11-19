#include "picobrew/http_server.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char key[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static const char auth[] = "Authorization: Bearer 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\r\n";
static PicoHttpServer server;
static unsigned cases, dispatched;
static char last_body[4096];
static size_t last_body_length;

typedef struct Wire {
    const uint8_t *input;
    size_t input_length, consumed;
    uint8_t output[16384];
    size_t produced, read_chunk, write_chunk, poll_bytes;
    unsigned poll_calls, accepted, closed, blocked_sends;
    int receive_result;
    bool block_writes;
} Wire;
static Wire wire;

static int accept_client(void *context)
{
    Wire *w = context;
    ++w->poll_calls;
    if (w->accepted)
        return PICO_HTTP_IO_AGAIN;
    ++w->accepted;
    return 0; /* Socket descriptor zero must be accepted. */
}
static int receive_client(void *context, int client, uint8_t *data, size_t capacity)
{
    Wire *w = context;
    assert(client == 0);
    ++w->poll_calls;
    if (w->receive_result)
        return w->receive_result == 1 ? 0 : w->receive_result;
    size_t count = w->input_length - w->consumed;
    if (!count)
        return PICO_HTTP_IO_AGAIN;
    if (count > capacity) count = capacity;
    if (count > w->read_chunk) count = w->read_chunk;
    memcpy(data, w->input + w->consumed, count);
    w->consumed += count;
    w->poll_bytes += count;
    return (int)count;
}
static int send_client(void *context, int client, const uint8_t *data, size_t length)
{
    Wire *w = context;
    assert(client == 0);
    ++w->poll_calls;
    if (w->block_writes || w->blocked_sends) {
        if (w->blocked_sends) --w->blocked_sends;
        return PICO_HTTP_IO_AGAIN;
    }
    if (length > w->write_chunk) length = w->write_chunk;
    assert(w->produced + length < sizeof(w->output));
    memcpy(w->output + w->produced, data, length);
    w->produced += length;
    w->poll_bytes += length;
    return (int)length;
}
static void close_client(void *context, int client)
{
    assert(client == 0);
    ++((Wire *)context)->closed;
    ++((Wire *)context)->poll_calls;
}
static const PicoHttpIo io = {accept_client, receive_client, send_client, close_client};
static const uint8_t asset[] = "browser-asset";
static const PicoHttpAsset assets[] = {{"/", "text/html; charset=utf-8", "gzip", asset, sizeof(asset)-1, NULL}};

static bool handler(const PicoHttpRequest *request, PicoHttpResponse *response,
                     char *arena, size_t capacity, void *context)
{
    (void)context;
    ++dispatched;
    last_body_length = request->body_length;
    assert(last_body_length < sizeof(last_body));
    memcpy(last_body, request->body, last_body_length);
    last_body[last_body_length] = '\0';
    /* Reentrant polling must not read or accept another request. */
    unsigned prior_calls = wire.poll_calls;
    pico_http_server_poll(&server, 1);
    assert(prior_calls == wire.poll_calls);
    if (strcmp(request->path, "/api/v1/health") != 0)
        assert(request->authenticated && request->auth_configured);
    const char *result = "{\"ok\":true}";
    if (strcmp(request->path, "/api/v1/echo") == 0) {
        response->body = (const uint8_t *)arena;
        response->body_length = request->body_length;
    } else {
        assert(strlen(result) < capacity);
        strcpy(arena, result);
        response->body = (const uint8_t *)arena;
        response->body_length = strlen(result);
    }
    return true;
}

static void setup(const void *input, size_t length, bool provision)
{
    memset(&wire, 0, sizeof(wire));
    wire.input = input;
    wire.input_length = length;
    wire.read_chunk = wire.write_chunk = 128;
    dispatched = 0;
    last_body_length = 0;
    pico_http_server_initialize(&server, &io, &wire, handler, NULL, assets, 1);
    if (provision)
        assert(pico_http_server_set_key(&server, key, strlen(key)));
}

static void poll(uint32_t now)
{
    wire.poll_bytes = wire.poll_calls = 0;
    pico_http_server_poll(&server, now);
    assert(wire.poll_bytes <= PICO_HTTP_IO_BUDGET);
    assert(wire.poll_calls <= PICO_HTTP_IO_CALL_BUDGET);
}

static void finish(void)
{
    for (unsigned i = 0; i < 5000 && !wire.closed; ++i)
        poll(i);
    assert(wire.closed == 1);
    assert(server.client == -1);
    assert(server.arena[0] == '\0');
    wire.output[wire.produced] = '\0';
}

static void result(unsigned status, unsigned expected_dispatch)
{
    char prefix[32];
    snprintf(prefix, sizeof(prefix), "HTTP/1.1 %u ", status);
    assert(strncmp((const char *)wire.output, prefix, strlen(prefix)) == 0);
    assert(dispatched == expected_dispatch);
    assert(strstr((const char *)wire.output, "Connection: close\r\n"));
    assert(!strstr((const char *)wire.output, "Access-Control-Allow-Origin"));
    ++cases;
}

static void check(const char *request, bool provision, unsigned status, unsigned expected_dispatch)
{
    setup(request, strlen(request), provision);
    finish();
    result(status, expected_dispatch);
}

int main(void)
{
    char request[8192], body[4096], key_line[80];
    check("GET /api/v1/health HTTP/1.1\r\nHost: pico\r\n\r\n", false, 200, 1);
    check("GET /api/v1/status HTTP/1.1\r\nHost: pico\r\n\r\n", false, 503, 0);
    check("GET /api/v1/status HTTP/1.1\r\nHost: pico\r\n\r\n", true, 401, 0);
    snprintf(request, sizeof(request), "GET /api/v1/status HTTP/1.1\r\nHost: pico\r\n%s\r\n", auth);
    check(request, true, 200, 1);
    setup(request, strlen(request), true);
    wire.read_chunk = 1;
    wire.write_chunk = 1;
    wire.blocked_sends = 7;
    finish();
    result(200, 1);
    assert(strstr((const char *)wire.output, "\r\n\r\n{\"ok\":true}"));

    for (size_t length = 0; length < 68; ++length) {
        memset(key_line, 'a', sizeof(key_line));
        bool accepted = pico_http_server_set_key(&server, key_line, length);
        assert(accepted == (length == 64));
        ++cases;
    }
    snprintf(key_line, sizeof(key_line), "%s\n", key);
    assert(pico_http_server_set_key(&server, key_line, 65));
    snprintf(key_line, sizeof(key_line), "%s\r\n", key);
    assert(pico_http_server_set_key(&server, key_line, 66));
    key_line[0] = 'A';
    assert(!pico_http_server_set_key(&server, key_line, 66));
    assert(server.key[0] == '\0');
    cases += 3;

    check("GET / HTTP/1.1\r\nHost: pico\r\n\r\n", false, 200, 0);
    assert(strstr((const char *)wire.output, "Content-Encoding: gzip\r\n"));
    assert(strstr((const char *)wire.output, "\r\n\r\nbrowser-asset"));
    check("HEAD / HTTP/1.1\r\nHost: pico\r\n\r\n", false, 200, 0);
    assert(strcmp(strstr((const char *)wire.output, "\r\n\r\n"), "\r\n\r\n") == 0);
    check("GET /missing HTTP/1.1\r\nHost: pico\r\n\r\n", false, 404, 0);
    check("GET /api/v1/health HTTP/1.0\r\n\r\n", false, 200, 1);
    check("GET /api/v1/health HTTP/1.1\r\n\r\n", false, 400, 0);
    check("GET /api/v1/health HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n", false, 400, 0);
    check("GET /api/v1/health HTTP/1.1\nHost: pico\n\n", false, 400, 0);
    check("GET /api/v1/health HTTP/1.1\rX", false, 400, 0);
    check("GET /api/v1/health HTTP/1.1\r\n Transfer-Encoding: chunked\r\n\r\n", false, 400, 0);
    check("GET http://pico/api/v1/health HTTP/1.1\r\nHost: pico\r\n\r\n", false, 400, 0);
    check("GET /api/v1/health HTTP/2.0\r\nHost: pico\r\n\r\n", false, 400, 0);
    check("GET /api/v1/health HTTP/1.1\r\nHost: pico\r\nTransfer-Encoding: chunked\r\n\r\n", false, 400, 0);
    check("GET /api/v1/health HTTP/1.1\r\nHost: pico\r\nContent-Length: 0\r\ncontent-length: 0\r\n\r\n", false, 400, 0);
    check("GET /api/v1/health HTTP/1.1\r\nHost: pico\r\nContent-Length: +1\r\n\r\n", false, 400, 0);
    check("GET /api/v1/health HTTP/1.1\r\nHost: pico\r\nContent-Length: 9999999999999999999\r\n\r\n", false, 413, 0);
    check("GET /api/v1/health HTTP/1.1\r\nHost: pico\r\nExpect: 100-continue\r\n\r\n", false, 417, 0);
    check("GET /api/v1/health HTTP/1.1\r\nHost: pico\r\n\r\nGET / HTTP/1.0\r\n\r\n", false, 400, 0);

    const char *invalid_headers[] = {
        "Content-Length: 4096\r\nContent-Type: application/json\r\n",
        "Content-Length: 2\r\nContent-Type: text/plain\r\n",
        "Content-Type: application/json\r\n",
        "Content-Length: 2\r\nContent-Type: application/json\r\nContent-Type: application/json\r\n",
        "Content-Length: 2\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\n",
        "Content-Length: 2\r\nContent-Type: application/json; charset=latin1\r\n"
    };
    const unsigned statuses[] = {413, 415, 411, 400, 400, 415};
    for (unsigned i = 0; i < sizeof(statuses)/sizeof(statuses[0]); ++i) {
        snprintf(request, sizeof(request), "POST /api/v1/echo HTTP/1.1\r\nHost: pico\r\n%s%s\r\n{}", auth, invalid_headers[i]);
        check(request, true, statuses[i], 0);
    }
    snprintf(request, sizeof(request), "POST /api/v1/echo HTTP/1.1\r\nHost: pico\r\n%sContent-Length: 2\r\nContent-Type: application/json; charset=UTF-8\r\n\r\n{}", auth);
    check(request, true, 200, 1);
    assert(last_body_length == 2 && strcmp(last_body, "{}") == 0);
    assert(strstr((const char *)wire.output, "\r\n\r\n{}"));

    memset(body, ' ', sizeof(body)); body[0] = '{'; body[4094] = '}'; body[4095] = '\0';
    snprintf(request, sizeof(request), "PUT /api/v1/echo HTTP/1.1\r\nHost: pico\r\n%sContent-Length: 4095\r\nContent-Type: application/json\r\n\r\n%s", auth, body);
    check(request, true, 200, 1);
    assert(last_body_length == 4095 && memcmp(last_body, body, 4095) == 0);
    snprintf(request, sizeof(request), "DELETE /api/v1/recipe/one HTTP/1.1\r\nHost: pico\r\n%s\r\n", auth);
    check(request, true, 200, 1);
    snprintf(request, sizeof(request), "OPTIONS /api/v1/status HTTP/1.1\r\nHost: pico\r\n%s\r\n", auth);
    check(request, true, 405, 0);
    snprintf(request, sizeof(request), "GET /api/v1/status HTTP/1.1\r\nHost: pico\r\n%s%s\r\n", auth, auth);
    check(request, true, 400, 0);
    snprintf(request, sizeof(request), "GET /api/v1/status HTTP/1.1\r\nHost: pico\r\n%s\r\n", auth);
    char *token = strstr(request, "012345"); assert(token); token[63] = '0';
    check(request, true, 401, 0);

    memset(request, 'x', sizeof(request));
    memcpy(request, "GET /", 5); request[2000] = '\0';
    check(request, false, 431, 0);
    strcpy(request, "GET /api/v1/health HTTP/1.1\r\nHost: pico\r\n");
    for (unsigned i = 0; i < 100; ++i) strcat(request, "X-Fill: abcdefghijklmnop\r\n");
    strcat(request, "\r\n");
    check(request, false, 431, 0);

    /* Timeouts are absolute, wrap-safe, and close without dispatching. */
    setup("GET /api/v1/", 12, true);
    poll(0xfffffff0u); poll(0x00001380u);
    assert(wire.closed == 1 && dispatched == 0); ++cases;
    const char *health_request = "GET /api/v1/health HTTP/1.1\r\nHost: pico\r\n\r\n";
    setup(health_request, strlen(health_request), false);
    wire.block_writes = true;
    poll(0); poll(8001);
    assert(wire.closed == 1); ++cases;
    setup("", 0, true); wire.receive_result = 1; poll(0);
    assert(wire.closed == 1 && !dispatched); ++cases;
    setup("", 0, true); wire.receive_result = PICO_HTTP_IO_ERROR; poll(0);
    assert(wire.closed == 1 && !dispatched); ++cases;

    /* Deterministic malformed-byte corpus, run under ASan/UBSan by the runner. */
    unsigned random = 0x104a83bdu;
    for (unsigned n = 0; n < 1000; ++n) {
        size_t length = n % 600 + 1;
        for (size_t j = 0; j < length; ++j) {
            random = random * 1664525u + 1013904223u;
            request[j] = (char)(random >> 24);
        }
        setup(request, length, true);
        for (unsigned j = 0; j < 100 && !wire.closed; ++j) poll(j);
        if (!wire.closed) poll(5001);
        assert(wire.closed == 1 && dispatched == 0);
        ++cases;
    }
    printf("HTTP transport: %u cases passed; server state %lu bytes; I/O bounded to %u bytes/%u calls per poll.\n",
           cases, (unsigned long)sizeof(PicoHttpServer), PICO_HTTP_IO_BUDGET, PICO_HTTP_IO_CALL_BUDGET);
    return 0;
}
