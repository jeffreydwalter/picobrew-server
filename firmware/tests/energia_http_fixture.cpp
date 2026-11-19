#include "picobrew/energia_http_server.h"
#include <WiFi.h>
#include <utility/simplelink.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

bool WiFiClass::_initialized;
WiFiClass WiFi;
static unsigned cases, calls, socket_calls, accepted_nonblocking, reads, file_opens, file_closes, socket_closes;
static uint32_t now_ms;
static int wifi_status, stat_result, open_result, read_result, close_result, accept_result;
static const char key[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static uint8_t file_data[8192];
static size_t file_size;
static int file_handle;
static const char *request;
static size_t consumed, produced, largest_read;
static uint8_t response[8192];

uint32_t millis() { return now_ms; }
uint8_t WiFiClass::status() { return (uint8_t)wifi_status; }
extern "C" {
_i16 sl_NetAppStop(_u32 app) { assert(app == SL_NET_APP_HTTP_SERVER_ID); ++calls; return 0; }
_i16 sl_Socket(_i16 family, _i16 type, _i16 protocol)
{
    assert(family == SL_AF_INET && type == SL_SOCK_STREAM && protocol == SL_IPPROTO_TCP);
    ++calls; ++socket_calls; return 3;
}
_i16 sl_SetSockOpt(_i16 descriptor, _i16 level, _i16 option, const void *value, SlSocklen_t length)
{
    assert(level == SL_SOL_SOCKET && option == SL_SO_NONBLOCKING);
    assert(length == sizeof(SlSockNonblocking_t));
    assert(((const SlSockNonblocking_t *)value)->NonblockingEnabled == 1);
    if (descriptor == 0) ++accepted_nonblocking;
    ++calls; return 0;
}
_u16 sl_Htons(_u16 value) { return (_u16)((value << 8) | (value >> 8)); }
_i16 sl_Bind(_i16 descriptor, const SlSockAddr_t *raw, SlSocklen_t length)
{
    const SlSockAddrIn_t *address = (const SlSockAddrIn_t *)raw;
    assert(descriptor == 3 && length == sizeof(*address));
    assert(address->sin_family == SL_AF_INET && address->sin_port == sl_Htons(80));
    assert(address->sin_addr.s_addr == 0); ++calls; return 0;
}
_i16 sl_Listen(_i16 descriptor, _i16 backlog) { assert(descriptor == 3 && backlog == 1); ++calls; return 0; }
_i16 sl_Accept(_i16 descriptor, SlSockAddr_t *, SlSocklen_t *)
{
    assert(descriptor == 3); ++calls;
    int result = accept_result; accept_result = SL_EAGAIN; return (_i16)result;
}
_i16 sl_Recv(_i16 descriptor, void *data, _i16 length, _i16 flags)
{
    assert(descriptor == 0 && length <= 128 && flags == 0); ++calls;
    size_t count = request ? strlen(request) - consumed : 0;
    if (!count) return SL_EAGAIN;
    if (count > (size_t)length) count = (size_t)length;
    memcpy(data, request + consumed, count); consumed += count; return (_i16)count;
}
_i16 sl_Send(_i16 descriptor, const void *data, _i16 length, _i16 flags)
{
    assert(descriptor == 0 && flags == 0 && length > 0);
    assert(produced + (size_t)length < sizeof(response));
    ++calls; memcpy(response + produced, data, (size_t)length); produced += (size_t)length;
    return length;
}
_i16 sl_Close(_i16) { ++calls; ++socket_closes; return 0; }
_i16 sl_FsGetInfo(const _u8 *path, _u32 token, SlFsFileInfo_t *info)
{
    assert(path && !token); ++calls;
    if (!stat_result) info->FileLen = (_u32)file_size;
    return (_i16)stat_result;
}
_i32 sl_FsOpen(const _u8 *path, _u32 mode, _u32 *token, _i32 *handle)
{
    assert(path && mode == FS_MODE_OPEN_READ && token == NULL); ++calls;
    if (!open_result) { *handle = file_handle; ++file_opens; }
    return open_result;
}
_i32 sl_FsRead(_i32 handle, _u32 offset, _u8 *data, _u32 length)
{
    assert(handle == file_handle && offset <= file_size && length <= sizeof(file_data));
    ++calls; ++reads;
    if (length > largest_read) largest_read = length;
    if (read_result) return read_result;
    size_t count = length < file_size - offset ? length : file_size - offset;
    memcpy(data, file_data + offset, count);
    return (_i32)count;
}
_i16 sl_FsClose(_i32 handle, const _u8 *certificate, const _u8 *signature, _u32 length)
{
    assert(handle == file_handle && !certificate && !signature && !length);
    ++calls; ++file_closes; return (_i16)close_result;
}
}

static void reset()
{
    calls = socket_calls = accepted_nonblocking = reads = file_opens = file_closes = socket_closes = 0;
    now_ms = 0;
    wifi_status = WL_CONNECTED;
    stat_result = open_result = read_result = close_result = 0;
    accept_result = SL_EAGAIN;
    WiFiClass::_initialized = false;
    file_handle = 8;
    request = NULL;
    consumed = produced = largest_read = 0;
    memset(response, 0, sizeof(response));
    memcpy(file_data, key, 64); file_size = 64;
}

int main()
{
    PicoEnergiaHttpServer server;
    reset();
    pico_energia_http_initialize(&server, NULL, NULL, NULL, 0, 80);
    assert(calls == 0 && server.listener == -1 && server.http.client == -1);
    pico_energia_http_poll(&server);
    assert(calls == 0); ++cases;
    WiFiClass::_initialized = true;
    pico_energia_http_poll(&server);
    assert(server.listener == 3 && server.key_loaded && strcmp(server.http.key, key) == 0);
    assert(socket_calls == 1 && reads == 1 && file_closes == 1); ++cases;
    accept_result = 0; now_ms = 1;
    pico_energia_http_poll(&server);
    assert(server.http.client == 0 && accepted_nonblocking == 1); ++cases;
    unsigned before = calls;
    server.in_poll = true;
    pico_energia_http_poll(&server);
    assert(calls == before); server.in_poll = false; ++cases;
    wifi_status = 0;
    pico_energia_http_poll(&server);
    assert(server.listener == -1 && server.http.client == -1 && socket_closes == 2); ++cases;

    wifi_status = WL_CONNECTED;
    pico_energia_http_poll(&server);
    assert(socket_calls == 2);
    accept_result = -9;
    pico_energia_http_poll(&server);
    assert(server.listener == -1); ++cases;
    now_ms = 999;
    pico_energia_http_poll(&server);
    assert(socket_calls == 2); ++cases;
    now_ms = 1001;
    pico_energia_http_poll(&server);
    assert(socket_calls == 3 && server.listener == 3); ++cases;
    pico_energia_http_shutdown(&server);

    size_t size = 77;
    assert(pico_energia_local_file_stat(NULL, "/local/recipe", &size) == 0 && size == 64); ++cases;
    stat_result = SL_FS_ERR_FILE_NOT_EXISTS;
    assert(pico_energia_local_file_stat(NULL, "/local/recipe", &size) == 1); ++cases;
    const int errors[] = {-2, -3, -7, -9, -12, -19, -30, -39, -53};
    for (int error : errors) {
        stat_result = error; size = 71;
        assert(pico_energia_local_file_stat(NULL, "/local/recipe", &size) == error && size == 71);
        ++cases;
    }
    stat_result = 0;
    uint8_t output[512];
    memset(file_data, 0x71, sizeof(output)); file_size = sizeof(output);
    reads = file_closes = 0;
    assert(pico_energia_local_file_read(NULL, "/local/recipe", output, sizeof(output)) == 512);
    assert(reads == 1 && file_closes == 1 && memcmp(output, file_data, sizeof(output)) == 0); ++cases;
    read_result = -22;
    assert(pico_energia_local_file_read(NULL, "/local/recipe", output, sizeof(output)) == -22);
    assert(file_closes == 2); ++cases;
    read_result = 0; close_result = -30;
    assert(pico_energia_local_file_read(NULL, "/local/recipe", output, sizeof(output)) == -30); ++cases;
    close_result = 0; open_result = -11; before = file_closes;
    assert(pico_energia_local_file_read(NULL, "/local/recipe", output, sizeof(output)) == -11);
    assert(file_closes == before); ++cases;
    before = calls;
    assert(pico_energia_local_file_read(NULL, "/local/recipe", output, 4097) < 0);
    assert(calls == before); ++cases;

    for (unsigned mode = 0; mode < 7; ++mode) {
        reset(); WiFiClass::_initialized = true;
        switch (mode) {
        case 0: stat_result = SL_FS_ERR_FILE_NOT_EXISTS; break;
        case 1: stat_result = -39; break;
        case 2: file_data[0] = 'Z'; break;
        case 3: file_size = 67; break;
        case 4: open_result = -6; break;
        case 5: read_result = 63; break;
        case 6: close_result = -7; break;
        }
        pico_energia_http_initialize(&server, NULL, NULL, NULL, 0, 80);
        pico_energia_http_poll(&server);
        assert(server.listener == 3 && server.key_loaded && server.http.key[0] == '\0');
        pico_energia_http_shutdown(&server); ++cases;
    }
    reset(); WiFiClass::_initialized = true;
    file_data[64] = '\r'; file_data[65] = '\n'; file_size = 66;
    pico_energia_http_initialize(&server, NULL, NULL, NULL, 0, 80);
    pico_energia_http_poll(&server);
    assert(strcmp(server.http.key, key) == 0); ++cases;
    pico_energia_http_shutdown(&server);

    const PicoHttpAsset asset = {"/", "text/html; charset=utf-8", "gzip", NULL, 6501, "/local/web/i.gz"};
    reset(); WiFiClass::_initialized = true;
    for (size_t i = 0; i < 6501; ++i) file_data[i] = (uint8_t)(i * 29u);
    file_size = 6501; file_handle = 0;
    request = "GET / HTTP/1.1\r\nHost: pico\r\n\r\n";
    accept_result = 0;
    pico_energia_http_initialize(&server, NULL, NULL, &asset, 1, 80);
    server.key_loaded = true; /* Isolate the asset handle from the key-load test. */
    for (unsigned i = 0; i < 100 && !socket_closes; ++i) {
        now_ms = i;
        pico_energia_http_poll(&server);
    }
    const char *body = strstr((const char *)response, "\r\n\r\n");
    assert(body && strncmp((const char *)response, "HTTP/1.1 200 ", 13) == 0);
    body += 4;
    assert(produced - (size_t)(body - (const char *)response) == 6501);
    assert(memcmp(body, file_data, 6501) == 0);
    assert(file_opens == 1 && file_closes == 1 && reads == 13 && largest_read == 512);
    assert(server.http.asset_handle == -1);
    pico_energia_http_shutdown(&server); ++cases;

    for (unsigned mode = 0; mode < 4; ++mode) {
        reset(); WiFiClass::_initialized = true;
        file_size = 6501; request = "GET / HTTP/1.1\r\nHost: pico\r\n\r\n"; accept_result = 0;
        switch (mode) {
        case 0: stat_result = SL_FS_ERR_FILE_NOT_EXISTS; break;
        case 1: stat_result = -39; break;
        case 2: file_size = 6500; break;
        case 3: open_result = -6; break;
        }
        pico_energia_http_initialize(&server, NULL, NULL, &asset, 1, 80);
        server.key_loaded = true;
        for (unsigned i = 0; i < 100 && !socket_closes; ++i) pico_energia_http_poll(&server);
        assert(strncmp((const char *)response, "HTTP/1.1 503 ", 13) == 0);
        assert(file_opens == 0 && file_closes == 0 && reads == 0);
        pico_energia_http_shutdown(&server); ++cases;
    }

    reset(); WiFiClass::_initialized = true;
    file_size = 6501; request = "GET / HTTP/1.1\r\nHost: pico\r\n\r\n"; accept_result = 0;
    pico_energia_http_initialize(&server, NULL, NULL, &asset, 1, 80);
    server.key_loaded = true;
    pico_energia_http_poll(&server);
    assert(file_opens == 1 && file_closes == 0 && server.http.asset_handle == file_handle);
    wifi_status = 0;
    pico_energia_http_poll(&server);
    assert(file_closes == 1 && socket_closes == 2 && server.http.asset_handle == -1);
    pico_energia_http_shutdown(&server);
    assert(file_closes == 1); ++cases;
    printf("Native HTTP adapter: %u mocked SimpleLink cases passed.\n", cases);
}
