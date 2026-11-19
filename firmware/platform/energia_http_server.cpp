#include "picobrew/energia_http_server.h"
#include <WiFi.h>
#include <string.h>
extern "C" {
#include <utility/simplelink.h>
#include <utility/fs.h>
#include <utility/netapp.h>
#include <utility/socket.h>
}

static bool foreground_interruptible()
{
#if defined(__arm__) || defined(__thumb__)
    uint32_t exception, masked;
    __asm__ volatile("mrs %0, ipsr" : "=r"(exception));
    __asm__ volatile("mrs %0, primask" : "=r"(masked));
    return exception == 0 && masked == 0;
#else
    return true;
#endif
}

static bool nonblocking(int descriptor)
{
    SlSockNonblocking_t option = {};
    option.NonblockingEnabled = 1;
    return sl_SetSockOpt(static_cast<_i16>(descriptor), SL_SOL_SOCKET, SL_SO_NONBLOCKING,
                         &option, sizeof(option)) == 0;
}

static int accept_client(void *context)
{
    PicoEnergiaHttpServer *s = static_cast<PicoEnergiaHttpServer *>(context);
    if (s->listener < 0)
        return PICO_HTTP_IO_AGAIN;
    SlSockAddrIn_t peer = {};
    SlSocklen_t size = sizeof(peer);
    int client = sl_Accept(static_cast<_i16>(s->listener),
                           reinterpret_cast<SlSockAddr_t *>(&peer), &size);
    if (client == SL_EAGAIN)
        return PICO_HTTP_IO_AGAIN;
    if (client < 0) {
        sl_Close(static_cast<_i16>(s->listener));
        s->listener = -1;
        s->last_attempt_ms = millis();
        return PICO_HTTP_IO_ERROR;
    }
    if (!nonblocking(client)) {
        sl_Close(static_cast<_i16>(client));
        return PICO_HTTP_IO_ERROR;
    }
    return client;
}

static int receive_client(void *, int client, uint8_t *data, size_t capacity)
{
    int result = sl_Recv(static_cast<_i16>(client), data, static_cast<_i16>(capacity), 0);
    if (result == SL_EAGAIN)
        return PICO_HTTP_IO_AGAIN;
    return result < 0 ? PICO_HTTP_IO_ERROR : result;
}

static int send_client(void *, int client, const uint8_t *data, size_t length)
{
    /* No paced retry loop: EAGAIN and partial sends return to the foreground
     * scheduler, allowing brewing/control work between network attempts. */
    int result = sl_Send(static_cast<_i16>(client), data, static_cast<_i16>(length), 0);
    if (result == SL_EAGAIN)
        return PICO_HTTP_IO_AGAIN;
    return result < 0 ? PICO_HTTP_IO_ERROR : result;
}

static void close_client(void *, int client)
{
    if (WiFiClass::_initialized)
        sl_Close(static_cast<_i16>(client));
}

static const PicoHttpIo io = {accept_client, receive_client, send_client, close_client};

static int open_asset(void *, const char *path, size_t expected_length)
{
    if (!path || !WiFiClass::_initialized)
        return PICO_HTTP_IO_ERROR;
    SlFsFileInfo_t info = {};
    if (sl_FsGetInfo(reinterpret_cast<const _u8 *>(path), 0, &info) != 0 ||
        static_cast<size_t>(info.FileLen) != expected_length)
        return PICO_HTTP_IO_ERROR;
    _i32 handle = -1;
    if (sl_FsOpen(reinterpret_cast<const _u8 *>(path), FS_MODE_OPEN_READ, NULL, &handle) != 0)
        return PICO_HTTP_IO_ERROR;
    return static_cast<int>(handle);
}

static int read_asset(void *, int handle, size_t offset, uint8_t *data, size_t length)
{
    if (!WiFiClass::_initialized || handle < 0 || !data || length > PICO_HTTP_IO_BUDGET)
        return PICO_HTTP_IO_ERROR;
    return static_cast<int>(sl_FsRead(static_cast<_i32>(handle), static_cast<_u32>(offset),
                                     data, static_cast<_u32>(length)));
}

static void close_asset(void *, int handle)
{
    if (WiFiClass::_initialized)
        sl_FsClose(static_cast<_i32>(handle), NULL, NULL, 0);
}

static const PicoHttpAssetIo asset_io = {open_asset, read_asset, close_asset};

int pico_energia_local_file_stat(void *, const char *path, size_t *size)
{
    if (!path || !size || !WiFiClass::_initialized)
        return -1;
    SlFsFileInfo_t info = {};
    int result = sl_FsGetInfo(reinterpret_cast<const _u8 *>(path), 0, &info);
    if (result == SL_FS_ERR_FILE_NOT_EXISTS)
        return 1;
    if (result != 0)
        return result < 0 ? result : -1;
    *size = static_cast<size_t>(info.FileLen);
    return 0;
}

int32_t pico_energia_local_file_read(void *, const char *path, uint8_t *data, size_t length)
{
    if (!path || !data || !WiFiClass::_initialized || length > PICO_HTTP_ARENA_CAPACITY)
        return -1;
    _i32 handle = -1;
    int opened = sl_FsOpen(reinterpret_cast<const _u8 *>(path), FS_MODE_OPEN_READ, NULL, &handle);
    if (opened != 0)
        return opened < 0 ? opened : -1;
    int count = length ? sl_FsRead(handle, 0, data, static_cast<_u32>(length)) : 0;
    int closed = sl_FsClose(handle, NULL, NULL, 0);
    if (count < 0)
        return count;
    if (closed != 0)
        return closed < 0 ? closed : -1;
    return count;
}

static void load_key(PicoEnergiaHttpServer *s)
{
    char key[PICO_HTTP_KEY_LENGTH + 2] = {};
    size_t length = 0;
    s->key_loaded = true;
    if (pico_energia_local_file_stat(NULL, PICO_LOCAL_API_KEY_FILE, &length) != 0 ||
        length < PICO_HTTP_KEY_LENGTH || length > sizeof(key))
        return;
    _i32 handle = -1;
    if (sl_FsOpen(reinterpret_cast<const _u8 *>(PICO_LOCAL_API_KEY_FILE), FS_MODE_OPEN_READ,
                   NULL, &handle) != 0)
        return;
    int count = sl_FsRead(handle, 0, reinterpret_cast<_u8 *>(key), static_cast<_u32>(length));
    int closed = sl_FsClose(handle, NULL, NULL, 0);
    if (count == static_cast<int>(length) && closed == 0)
        pico_http_server_set_key(&s->http, key, length);
    memset(key, 0, sizeof(key));
}

static bool start_listener(PicoEnergiaHttpServer *s)
{
    if (s->port == 80)
        sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
    int descriptor = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_IPPROTO_TCP);
    if (descriptor < 0)
        return false;
    SlSockAddrIn_t address = {};
    address.sin_family = SL_AF_INET;
    address.sin_port = sl_Htons(s->port);
    address.sin_addr.s_addr = 0;
    if (!nonblocking(descriptor) ||
        sl_Bind(static_cast<_i16>(descriptor), reinterpret_cast<SlSockAddr_t *>(&address),
                 sizeof(address)) != 0 || sl_Listen(static_cast<_i16>(descriptor), 1) != 0) {
        sl_Close(static_cast<_i16>(descriptor));
        return false;
    }
    s->listener = descriptor;
    return true;
}

void pico_energia_http_initialize(PicoEnergiaHttpServer *s, PicoHttpHandler handler,
                                   void *context, const PicoHttpAsset *assets,
                                   size_t asset_count, uint16_t port)
{
    memset(s, 0, sizeof(*s));
    s->listener = -1;
    s->port = port;
    pico_http_server_initialize(&s->http, &io, s, handler, context, assets, asset_count);
    pico_http_server_set_asset_io(&s->http, &asset_io, s);
}

void pico_energia_http_shutdown(PicoEnergiaHttpServer *s)
{
    pico_http_server_disconnect(&s->http);
    if (s->listener >= 0 && WiFiClass::_initialized)
        sl_Close(static_cast<_i16>(s->listener));
    s->listener = -1;
    s->attempted = false;
}

void pico_energia_http_poll(PicoEnergiaHttpServer *s)
{
    if (s->in_poll || !foreground_interruptible())
        return;
    s->in_poll = true;
    if (!WiFiClass::_initialized) {
        pico_energia_http_shutdown(s);
        s->in_poll = false;
        return;
    }
    int status = WiFi.status();
    if (status != WL_CONNECTED && status != WL_AP_MODE) {
        pico_energia_http_shutdown(s);
        s->in_poll = false;
        return;
    }
    if (!s->key_loaded)
        load_key(s);
    uint32_t now = millis();
    if (s->listener < 0 && (!s->attempted || static_cast<uint32_t>(now - s->last_attempt_ms) >= 1000)) {
        s->attempted = true;
        s->last_attempt_ms = now;
        start_listener(s);
    }
    if (s->listener >= 0)
        pico_http_server_poll(&s->http, now);
    s->in_poll = false;
}
