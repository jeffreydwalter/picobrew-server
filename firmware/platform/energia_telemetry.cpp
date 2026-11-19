#include "picobrew/energia_telemetry.h"
#include <WiFi.h>
#include <string.h>
extern "C" {
#include <utility/simplelink.h>
#include <utility/fs.h>
#include <utility/netapp.h>
#include <utility/socket.h>
}

static bool fs_ready(void *) { return WiFiClass::_initialized; }
static bool connected(void *)
{
    if (!WiFiClass::_initialized) return false;
    int state = WiFi.status();
    return state == WL_CONNECTED || state == WL_AP_MODE;
}
static bool config_path(const char *path)
{
    return path && (!strcmp(path, "/local/telemetry.a") || !strcmp(path, "/local/telemetry.b"));
}
static int file_stat(void *, const char *path, size_t *length)
{
    if (!fs_ready(NULL) || !config_path(path) || !length) return -1;
    SlFsFileInfo_t info = {};
    int result = sl_FsGetInfo(reinterpret_cast<const _u8 *>(path), 0, &info);
    if (result == SL_FS_ERR_FILE_NOT_EXISTS) return 1;
    if (result != 0) return -1;
    *length = info.FileLen; return 0;
}
static int32_t read_file(void *, const char *path, uint8_t *data, size_t length)
{
    if (!fs_ready(NULL) || !config_path(path) || !data || length != 160) return -1;
    _i32 handle = -1;
    if (sl_FsOpen(reinterpret_cast<const _u8 *>(path), FS_MODE_OPEN_READ, NULL, &handle) != 0) return -1;
    int result = sl_FsRead(handle, 0, data, static_cast<_u32>(length));
    int closed = sl_FsClose(handle, NULL, NULL, 0); return closed == 0 ? result : -1;
}
static int replace_file(void *, const char *path, const uint8_t *data, size_t length)
{
    if (!fs_ready(NULL) || !config_path(path) || !data || length != 160) return -1;
    SlFsFileInfo_t info = {};
    int found = sl_FsGetInfo(reinterpret_cast<const _u8 *>(path), 0, &info);
    _u32 mode;
    if (found == SL_FS_ERR_FILE_NOT_EXISTS) mode = FS_MODE_OPEN_CREATE(160, 0);
    else if (found == 0 && info.AllocatedLen >= length) mode = FS_MODE_OPEN_WRITE;
    else return -1;
    _i32 handle = -1;
    if (sl_FsOpen(reinterpret_cast<const _u8 *>(path), mode, NULL, &handle) != 0) return -1;
    int written = sl_FsWrite(handle, 0, const_cast<_u8 *>(data), static_cast<_u32>(length));
    int closed = sl_FsClose(handle, NULL, NULL, 0);
    return written == static_cast<int>(length) && closed == 0 ? 0 : -1;
}
static int resolve(void *, const char *host, uint32_t *address)
{
    if (!connected(NULL)) return -1;
    _u32 result = 0;
    int status = sl_NetAppDnsGetHostByName(reinterpret_cast<_i8 *>(const_cast<char *>(host)),
                                          static_cast<_u16>(strlen(host)), &result, SL_AF_INET);
    if (status != 0 || !result) return -1;
    *address = result; return 0;
}
static int open_socket(void *)
{
    int socket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_IPPROTO_TCP);
    if (socket < 0) return -1;
    SlSockNonblocking_t nonblocking = {}; nonblocking.NonblockingEnabled = 1;
    if (sl_SetSockOpt(static_cast<_i16>(socket), SL_SOL_SOCKET, SL_SO_NONBLOCKING,
                     &nonblocking, sizeof nonblocking) != 0) {
        sl_Close(static_cast<_i16>(socket)); return -1;
    }
    return socket;
}
static int connect_socket(void *, int socket, uint32_t address, uint16_t port)
{
    SlSockAddrIn_t target = {}; target.sin_family = SL_AF_INET;
    target.sin_port = sl_Htons(port); target.sin_addr.s_addr = sl_Htonl(address);
    int result = sl_Connect(static_cast<_i16>(socket), reinterpret_cast<SlSockAddr_t *>(&target), sizeof target);
    if (result == 0 || result == SL_EISCONN) return 0;
    return result == SL_EALREADY || result == SL_EAGAIN ? 1 : -1;
}
static int send_socket(void *, int socket, const uint8_t *data, size_t length)
{
    int result = sl_Send(static_cast<_i16>(socket), data, static_cast<_i16>(length), 0);
    return result == SL_EAGAIN ? 0 : result <= 0 ? -1 : result;
}
static int receive_socket(void *, int socket, uint8_t *data, size_t capacity)
{
    int result = sl_Recv(static_cast<_i16>(socket), data, static_cast<_i16>(capacity), 0);
    return result == SL_EAGAIN ? 0 : result <= 0 ? -1 : result;
}
static void close_socket(void *, int socket)
{
    if (WiFiClass::_initialized) sl_Close(static_cast<_i16>(socket));
}
static const PicoHistoryStorage storage = {fs_ready, file_stat, read_file, replace_file};
static const PicoTelemetryTransport transport = {
    connected, resolve, open_socket, connect_socket, send_socket, receive_socket, close_socket
};
const PicoHistoryStorage *pico_energia_telemetry_storage(void) { return &storage; }
const PicoTelemetryTransport *pico_energia_telemetry_transport(void) { return &transport; }
