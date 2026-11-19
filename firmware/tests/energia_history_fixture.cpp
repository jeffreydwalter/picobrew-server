#include "picobrew/energia_history.h"
#include <WiFi.h>
#include <cstring>
extern "C" {
#include <utility/simplelink.h>
#include <utility/fs.h>
}
bool WiFiClass::_initialized = true;
static int result[5], trace[20], calls, bad_handle;
static unsigned allocated, last_mode;
static void note(int call) { if (calls < 20) trace[calls] = call; ++calls; }
extern "C" _i32 sl_FsGetInfo(const _u8 *, _u32 token, SlFsFileInfo_t *info)
{
    note(0); if (token) ++bad_handle;
    info->FileLen = 64; info->AllocatedLen = allocated; return result[0];
}
extern "C" _i32 sl_FsOpen(const _u8 *, _u32 mode, _u32 *token, _i32 *handle)
{
    note(1); if (token) ++bad_handle;
    last_mode = mode; *handle = 77; return result[1];
}
extern "C" _i32 sl_FsRead(_i32 handle, _u32 offset, _u8 *data, _u32 length)
{
    note(2); if (handle != 77 || offset) ++bad_handle;
    std::memset(data, 0x35, length); return result[2] == 9999 ? (_i32)length : result[2];
}
extern "C" _i32 sl_FsWrite(_i32 handle, _u32 offset, _u8 *, _u32 length)
{
    note(3); if (handle != 77 || offset) ++bad_handle;
    return result[3] == 9999 ? (_i32)length : result[3];
}
extern "C" _i32 sl_FsClose(_i32 handle, const _u8 *certificate, const _u8 *signature, _u32 signature_length)
{
    note(4); if (handle != 77 || certificate || signature || signature_length) ++bad_handle;
    return result[4];
}
extern "C" void hb_reset(void)
{
    WiFiClass::_initialized = true;
    std::memset(result, 0, sizeof result); result[0] = -11; result[2] = result[3] = 9999;
    calls = bad_handle = 0; allocated = 3584; last_mode = 0;
}
extern "C" void hb_result(int field, int value) { if (field >= 0 && field < 5) result[field] = value; }
extern "C" void hb_ready(int value) { WiFiClass::_initialized = value != 0; }
extern "C" void hb_capacity(unsigned value) { allocated = value; }
extern "C" int hb_calls(void) { return calls; }
extern "C" int hb_trace(int index) { return index >= 0 && index < calls && index < 20 ? trace[index] : -1; }
extern "C" unsigned hb_mode(void) { return last_mode; }
extern "C" int hb_bad_handle(void) { return bad_handle; }
extern "C" int hb_read(const char *path, size_t length)
{
    static uint8_t data[4096];
    return pico_energia_history_storage()->read(NULL, path, data, length);
}
extern "C" int hb_write(const char *path, size_t length)
{
    static uint8_t data[4096];
    return pico_energia_history_storage()->replace(NULL, path, data, length);
}
