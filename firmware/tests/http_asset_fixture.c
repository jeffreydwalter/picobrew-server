#include "picobrew/http_server.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Exercise the real HTTP state machine with a file larger than its entire
 * response arena. Neither the fixture nor the server may return a whole-file
 * pointer to the transport. File handle zero is deliberately valid. */
static PicoHttpServer server;
static unsigned cases;
static uint8_t file_data[6501];
static PicoHttpAsset assets[] = {
    {"/", "text/html; charset=utf-8", "gzip", NULL, sizeof(file_data), "/local/web/index.html.gz"},
    {"/app.js", "application/javascript; charset=utf-8", "gzip", NULL, sizeof(file_data), "/local/web/app.js.gz"}
};
static struct {
    const char *request;
    size_t consumed, produced, read_offset, write_chunk, poll_bytes;
    uint8_t output[8192];
    unsigned accepted, socket_closes, file_opens, file_closes, file_reads;
    unsigned poll_calls, poll_file_reads, blocked_sends, sends;
    unsigned fail_read, short_read, zero_read, fail_send, zero_send;
    bool open_error, block_sends;
} wire;

static int accept_client(void *context)
{
    assert(context == &wire);
    ++wire.poll_calls;
    if (wire.accepted)
        return PICO_HTTP_IO_AGAIN;
    ++wire.accepted;
    return 0;
}

static int receive_client(void *context, int client, uint8_t *data, size_t capacity)
{
    assert(context == &wire && client == 0);
    ++wire.poll_calls;
    size_t count = strlen(wire.request) - wire.consumed;
    if (!count)
        return PICO_HTTP_IO_AGAIN;
    if (count > capacity)
        count = capacity;
    memcpy(data, wire.request + wire.consumed, count);
    wire.consumed += count;
    wire.poll_bytes += count;
    return (int)count;
}

static int send_client(void *context, int client, const uint8_t *data, size_t length)
{
    assert(context == &wire && client == 0);
    ++wire.poll_calls;
    ++wire.sends;
    if (wire.fail_send == wire.sends)
        return PICO_HTTP_IO_ERROR;
    if (wire.zero_send == wire.sends)
        return 0;
    if (wire.block_sends || wire.blocked_sends) {
        if (wire.blocked_sends)
            --wire.blocked_sends;
        return PICO_HTTP_IO_AGAIN;
    }
    if (length > wire.write_chunk)
        length = wire.write_chunk;
    assert(wire.produced + length < sizeof(wire.output));
    memcpy(wire.output + wire.produced, data, length);
    wire.produced += length;
    wire.poll_bytes += length;
    return (int)length;
}

static void close_client(void *context, int client)
{
    assert(context == &wire && client == 0);
    ++wire.poll_calls;
    ++wire.socket_closes;
}

static int open_asset(void *context, const char *path, size_t expected_length)
{
    assert(context == &wire);
    assert(strcmp(path, "/local/web/index.html.gz") == 0 ||
           strcmp(path, "/local/web/app.js.gz") == 0);
    assert(expected_length == assets[0].length);
    wire.poll_calls += 2; /* Match the native stat/open budget. */
    if (wire.open_error)
        return PICO_HTTP_IO_ERROR;
    assert(wire.file_opens == 0);
    ++wire.file_opens;
    return 0;
}

static int read_asset(void *context, int handle, size_t offset, uint8_t *data, size_t length)
{
    assert(context == &wire && handle == 0);
    ++wire.poll_calls;
    ++wire.poll_file_reads;
    ++wire.file_reads;
    assert(wire.file_opens == 1 && !wire.file_closes);
    assert(offset == wire.read_offset && length > 0 && length <= PICO_HTTP_IO_BUDGET);
    assert(offset + length <= assets[0].length);
    assert(data >= (uint8_t *)server.arena && data + length <= (uint8_t *)server.arena + sizeof(server.arena));
    if (wire.fail_read == wire.file_reads)
        return PICO_HTTP_IO_ERROR;
    if (wire.zero_read == wire.file_reads)
        return 0;
    if (wire.short_read == wire.file_reads)
        --length;
    memcpy(data, file_data + offset, length);
    wire.read_offset += length;
    return (int)length;
}

static void close_asset(void *context, int handle)
{
    assert(context == &wire && handle == 0);
    assert(wire.file_opens == 1 && wire.file_closes == 0);
    ++wire.poll_calls;
    ++wire.file_closes;
}

static const PicoHttpIo io = {accept_client, receive_client, send_client, close_client};
static const PicoHttpAssetIo asset_io = {open_asset, read_asset, close_asset};

static void setup(const char *request)
{
    memset(&wire, 0, sizeof(wire));
    wire.request = request;
    wire.write_chunk = 73;
    assets[0].length = assets[1].length = sizeof(file_data);
    pico_http_server_initialize(&server, &io, &wire, NULL, NULL, assets, 2);
    pico_http_server_set_asset_io(&server, &asset_io, &wire);
}

static void poll(uint32_t now)
{
    wire.poll_calls = wire.poll_bytes = wire.poll_file_reads = 0;
    pico_http_server_poll(&server, now);
    assert(wire.poll_calls <= PICO_HTTP_IO_CALL_BUDGET);
    assert(wire.poll_bytes <= PICO_HTTP_IO_BUDGET);
    assert(wire.poll_file_reads <= 1);
}

static void finish(void)
{
    for (unsigned i = 0; i < 2000 && !wire.socket_closes; ++i)
        poll(i);
    assert(wire.socket_closes == 1);
    assert(wire.file_closes == wire.file_opens);
    assert(server.client == -1 && server.asset_handle == -1);
    assert(server.arena[0] == '\0');
    wire.output[wire.produced] = 0;
}

static const uint8_t *body(unsigned status)
{
    char prefix[32];
    snprintf(prefix, sizeof(prefix), "HTTP/1.1 %u ", status);
    assert(strncmp((const char *)wire.output, prefix, strlen(prefix)) == 0);
    const char *start = strstr((const char *)wire.output, "\r\n\r\n");
    assert(start);
    return (const uint8_t *)start + 4;
}

int main(void)
{
    const char *get = "GET / HTTP/1.1\r\nHost: pico\r\n\r\n";
    const char *head = "HEAD / HTTP/1.1\r\nHost: pico\r\n\r\n";
    for (size_t i = 0; i < sizeof(file_data); ++i)
        file_data[i] = (uint8_t)(i * 37u + i / 11u);

    setup(get);
    wire.blocked_sends = 3;
    finish();
    const uint8_t *payload = body(200);
    assert(strstr((const char *)wire.output, "Content-Length: 6501\r\n"));
    assert(strstr((const char *)wire.output, "Content-Encoding: gzip\r\n"));
    assert(wire.produced - (size_t)(payload - wire.output) == sizeof(file_data));
    assert(memcmp(payload, file_data, sizeof(file_data)) == 0);
    assert(wire.file_reads == (sizeof(file_data) + 511) / 512);
    ++cases;

    setup(head);
    finish();
    payload = body(200);
    assert(wire.produced == (size_t)(payload - wire.output));
    assert(wire.file_opens == 1 && wire.file_closes == 1 && wire.file_reads == 0);
    assert(strstr((const char *)wire.output, "Content-Length: 6501\r\n"));
    ++cases;

    setup(get);
    assets[0].length = 0;
    finish();
    payload = body(200);
    assert(wire.produced == (size_t)(payload - wire.output));
    assert(wire.file_reads == 0);
    ++cases;

    for (unsigned failure = 0; failure < 5; ++failure) {
        setup(get);
        switch (failure) {
        case 0: wire.open_error = true; break;
        case 1: wire.fail_read = 1; break;
        case 2: wire.short_read = 1; break;
        case 3: wire.zero_read = 1; break;
        case 4: pico_http_server_set_asset_io(&server, NULL, NULL); break;
        }
        finish();
        payload = body(503);
        assert(strstr((const char *)payload, "browser files are missing"));
        assert(!strstr((const char *)wire.output, "Content-Encoding:"));
        ++cases;
    }

    setup("GET /app.js HTTP/1.1\r\nHost: pico\r\n\r\n");
    wire.open_error = true;
    finish();
    assert(strcmp((const char *)body(503), "{\"error\":\"web_asset_unavailable\"}") == 0);
    ++cases;

    /* Once the 200 headers are sent, read errors close the stream. Appending an
     * error object or silently padding it would corrupt the declared entity. */
    for (unsigned failure = 0; failure < 3; ++failure) {
        setup(get);
        if (failure == 0) wire.fail_read = 3;
        if (failure == 1) wire.short_read = 3;
        if (failure == 2) wire.zero_read = 3;
        finish();
        payload = body(200);
        assert(wire.produced - (size_t)(payload - wire.output) == 1024);
        assert(memcmp(payload, file_data, 1024) == 0);
        assert(wire.file_reads == 3);
        ++cases;
    }

    for (unsigned failure = 0; failure < 2; ++failure) {
        setup(get);
        if (failure) wire.fail_send = 6;
        else wire.zero_send = 6;
        finish();
        assert(wire.file_opens == 1 && wire.file_closes == 1);
        ++cases;
    }

    setup(get);
    wire.block_sends = true;
    poll(0);
    assert(wire.file_opens == 1 && !wire.file_closes);
    poll(8001);
    assert(wire.socket_closes == 1 && wire.file_closes == 1);
    ++cases;

    setup(get);
    wire.block_sends = true;
    poll(0);
    pico_http_server_disconnect(&server);
    pico_http_server_disconnect(&server);
    assert(wire.socket_closes == 1 && wire.file_closes == 1);
    ++cases;

    printf("Flash HTTP assets: %u cases passed; bounded streaming, exact bytes, HEAD, errors and handle cleanup.\n", cases);
    return 0;
}
