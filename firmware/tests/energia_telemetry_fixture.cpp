#include "picobrew/energia_telemetry.h"
#include <WiFi.h>
#include <cstring>
extern "C" {
#include <utility/simplelink.h>
#include <utility/fs.h>
#include <utility/netapp.h>
#include <utility/socket.h>
}
bool WiFiClass::_initialized = true;
WiFiClass WiFi;
static int wifi_state = WL_CONNECTED;
int WiFiClass::status() { return wifi_state; }
static int result[12], trace[40], calls, bad;
static unsigned mode, capacity, port, address;
static void note(int code) { if (calls < 40) trace[calls] = code; ++calls; }
extern "C" _i32 sl_FsGetInfo(const _u8 *, _u32, SlFsFileInfo_t *info)
{
    note(0); info->FileLen = 160; info->AllocatedLen = capacity; return result[0];
}
extern "C" _i32 sl_FsOpen(const _u8 *, _u32 value, _u32 *, _i32 *handle)
{
    note(1); mode = value; *handle = 77; return result[1];
}
extern "C" _i32 sl_FsRead(_i32 handle, _u32 offset, _u8 *data, _u32 length)
{
    note(2); if (handle != 77 || offset) ++bad;
    std::memset(data, 0, length); return result[2] == 9999 ? (_i32)length : result[2];
}
extern "C" _i32 sl_FsWrite(_i32 handle, _u32 offset, _u8 *, _u32 length)
{
    note(3); if (handle != 77 || offset) ++bad;
    return result[3] == 9999 ? (_i32)length : result[3];
}
extern "C" _i32 sl_FsClose(_i32 handle, const _u8 *, const _u8 *, _u32)
{
    note(4); if (handle != 77) ++bad; return result[4];
}
extern "C" _i16 sl_Socket(_i16 family, _i16 type, _i16 protocol)
{
    note(5); if (family != SL_AF_INET || type != SL_SOCK_STREAM || protocol != SL_IPPROTO_TCP) ++bad;
    return (_i16)result[5];
}
extern "C" _i16 sl_SetSockOpt(_i16 socket, _i16 level, _i16 option, const void *value, _i16 size)
{
    note(6);
    if (socket != 7 || level != SL_SOL_SOCKET || option != SL_SO_NONBLOCKING ||
        size != sizeof(SlSockNonblocking_t) || !static_cast<const SlSockNonblocking_t *>(value)->NonblockingEnabled) ++bad;
    return (_i16)result[6];
}
extern "C" _u16 sl_Htons(_u16 value) { return (_u16)((value << 8) | (value >> 8)); }
extern "C" _u32 sl_Htonl(_u32 value)
{
    return ((value & 0xffu) << 24) | ((value & 0xff00u) << 8) | ((value >> 8) & 0xff00u) | (value >> 24);
}
extern "C" _i16 sl_Connect(_i16 socket, const SlSockAddr_t *value, _i16 size)
{
    note(7); const SlSockAddrIn_t *target = reinterpret_cast<const SlSockAddrIn_t *>(value);
    if (socket != 7 || size != sizeof(*target) || target->sin_family != SL_AF_INET) ++bad;
    port = sl_Htons(target->sin_port); address = sl_Htonl(target->sin_addr.s_addr); return (_i16)result[7];
}
extern "C" _i16 sl_Send(_i16 socket, const void *, _i16 length, _i16 flags)
{
    note(8); if (socket != 7 || flags) ++bad;
    return (_i16)(result[8] == 9999 ? length : result[8]);
}
extern "C" _i16 sl_Recv(_i16 socket, void *data, _i16 length, _i16 flags)
{
    note(9); if (socket != 7 || flags) ++bad;
    std::memset(data, 0, (size_t)length); return (_i16)(result[9] == 9999 ? length : result[9]);
}
extern "C" _i16 sl_Close(_i16 socket) { note(10); if (socket != 7) ++bad; return (_i16)result[10]; }
extern "C" _i16 sl_NetAppDnsGetHostByName(_i8 *name, _u16 length, _u32 *value, _u8 family)
{
    note(11); if (std::strcmp(reinterpret_cast<char *>(name), "nas.local") || length != 9 || family != SL_AF_INET) ++bad;
    *value = 0x0a000109; return (_i16)result[11];
}
extern "C" void nt_reset(void)
{
    WiFiClass::_initialized = true; wifi_state = WL_CONNECTED; calls = bad = 0;
    std::memset(result, 0, sizeof result); result[0] = -11; result[2] = result[3] = result[8] = result[9] = 9999;
    result[5] = 7; capacity = 3584; mode = port = address = 0;
}
extern "C" void nt_result(int which, int value) { if (which >= 0 && which < 12) result[which] = value; }
extern "C" void nt_network(int initialized, int state) { WiFiClass::_initialized = initialized != 0; wifi_state = state; }
extern "C" int nt_trace(int index) { return index >= 0 && index < calls && index < 40 ? trace[index] : -1; }
extern "C" int nt_calls(void) { return calls; }
extern "C" int nt_bad(void) { return bad; }
extern "C" unsigned nt_mode(void) { return mode; }
extern "C" unsigned nt_port(void) { return port; }
extern "C" unsigned nt_address(void) { return address; }
extern "C" int nt_ready(void) { return pico_energia_telemetry_transport()->ready(NULL); }
extern "C" int nt_open(void) { return pico_energia_telemetry_transport()->open(NULL); }
extern "C" int nt_connect(void) { return pico_energia_telemetry_transport()->connect(NULL, 7, 0x0a000109, 8080); }
extern "C" int nt_send(void)
{
    uint8_t data[32] = {}; return pico_energia_telemetry_transport()->send(NULL, 7, data, sizeof data);
}
extern "C" int nt_receive(void)
{
    uint8_t data[32]; return pico_energia_telemetry_transport()->receive(NULL, 7, data, sizeof data);
}
extern "C" void nt_close(void) { pico_energia_telemetry_transport()->close(NULL, 7); }
extern "C" int nt_resolve(void) { uint32_t value; return pico_energia_telemetry_transport()->resolve(NULL, "nas.local", &value); }
extern "C" int nt_write(const char *path, size_t length)
{
    static uint8_t data[160]; return pico_energia_telemetry_storage()->replace(NULL, path, data, length);
}
