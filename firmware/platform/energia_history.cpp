#include "picobrew/energia_history.h"
#include <WiFi.h>
#include <string.h>
extern "C" {
#include <utility/simplelink.h>
#include <utility/fs.h>
}

static bool ready(void *) { return WiFiClass::_initialized; }

static bool history_path(const char *path)
{
    if (!path || strncmp(path, "/local/history", 14)) return false;
    const char *suffix = path + 14;
    if (!strcmp(suffix, ".meta.a") || !strcmp(suffix, ".meta.b") ||
        !strcmp(suffix, ".tail.a") || !strcmp(suffix, ".tail.b")) return true;
    return strlen(suffix) == 2 && suffix[0] >= '0' && suffix[0] <= '2' &&
           suffix[1] >= '0' && suffix[1] <= '9' &&
           (suffix[0] - '0') * 10 + suffix[1] - '0' < PICO_HISTORY_CHUNKS;
}

static int file_stat(void *, const char *path, size_t *length)
{
    if (!ready(NULL) || !history_path(path) || !length) return -1;
    SlFsFileInfo_t info = {};
    int result = sl_FsGetInfo(reinterpret_cast<const _u8 *>(path), 0, &info);
    if (result == SL_FS_ERR_FILE_NOT_EXISTS) return 1;
    if (result != 0) return -1;
    *length = info.FileLen;
    return 0;
}

static int32_t read_file(void *, const char *path, uint8_t *data, size_t length)
{
    if (!ready(NULL) || !history_path(path) || !data || !length || length > 3584) return -1;
    _i32 handle = -1;
    if (sl_FsOpen(reinterpret_cast<const _u8 *>(path), FS_MODE_OPEN_READ, NULL, &handle) != 0)
        return -1;
    int result = sl_FsRead(handle, 0, data, static_cast<_u32>(length));
    int closed = sl_FsClose(handle, NULL, NULL, 0);
    return closed == 0 ? result : -1;
}

static int replace_file(void *, const char *path, const uint8_t *data, size_t length)
{
    if (!ready(NULL) || !history_path(path) || !data || !length || length > 3584) return -1;
    SlFsFileInfo_t info = {};
    int found = sl_FsGetInfo(reinterpret_cast<const _u8 *>(path), 0, &info);
    _u32 mode;
    if (found == SL_FS_ERR_FILE_NOT_EXISTS)
        mode = FS_MODE_OPEN_CREATE(strstr(path, ".meta.") ? 1024 : 3584, 0);
    else if (found == 0 && info.AllocatedLen >= length)
        mode = FS_MODE_OPEN_WRITE;
    else return -1;
    _i32 handle = -1;
    if (sl_FsOpen(reinterpret_cast<const _u8 *>(path), mode, NULL, &handle) != 0) return -1;
    int written = sl_FsWrite(handle, 0, const_cast<_u8 *>(data), static_cast<_u32>(length));
    int closed = sl_FsClose(handle, NULL, NULL, 0);
    return written == static_cast<int>(length) && closed == 0 ? 0 : -1;
}

static const PicoHistoryStorage storage = {ready, file_stat, read_file, replace_file};
const PicoHistoryStorage *pico_energia_history_storage(void) { return &storage; }
