/* Pico Pro 0.1.34 — serial flash. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x20018108. */
void SLFS_end(void)
{
    return;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x2001810a. */
uint32_t SLFS_peek(int file, uint32_t argument_2)
{
    long sl_fs_read_result;
    uint32_t value;

    if ((*(int *)(file + 0x10) != 0) &&
        (*(uint32_t *)(file + 0x18) != *(uint32_t *)(file + 0x14))) {
        value = argument_2;
        sl_fs_read_result =
            sl_FsRead(*(int *)(file + 0x10), *(uint32_t *)(file + 0x18), (uint8_t *)&value, 1);
        *(long *)(file + 0x1c) = sl_fs_read_result;
        if (sl_fs_read_result == 1) {
            return value & 0xff;
        }
    }
    return 0xffffffff;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x20018136. */
uint32_t SLFS_read_byte(int file, uint32_t argument_2)
{
    long sl_fs_read_result;
    uint32_t value;

    if (((*(int *)(file + 0x10) != 0) && (*(char *)(file + 0x20) == '\0')) &&
        (*(uint32_t *)(file + 0x18) != *(uint32_t *)(file + 0x14))) {
        value = argument_2;
        sl_fs_read_result =
            sl_FsRead(*(int *)(file + 0x10), *(uint32_t *)(file + 0x18), (uint8_t *)&value, 1);
        *(long *)(file + 0x1c) = sl_fs_read_result;
        if (sl_fs_read_result == 1) {
            *(int *)(file + 0x18) = *(int *)(file + 0x18) + 1;
            return value & 0xff;
        }
    }
    return 0xffffffff;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x20018170. */
uint32_t SLFS_write_byte(int file)
{
    uint32_t value_1;
    long sl_fs_write_result;
    uint8_t byte_value_9;

    if ((*(int *)(file + 0x10) == 0) || (*(char *)(file + 0x20) == '\0')) {
        value_1 = 0xffffffff;
    } else {
        if (*(uint32_t *)(file + 0x18) != *(uint32_t *)(file + 0x14)) {
            sl_fs_write_result =
                sl_FsWrite(*(int *)(file + 0x10), *(uint32_t *)(file + 0x18), &byte_value_9, 1);
            *(long *)(file + 0x1c) = sl_fs_write_result;
            if (0 < sl_fs_write_result) {
                *(int *)(file + 0x18) = *(int *)(file + 0x18) + 1;
                return 1;
            }
        }
        value_1 = 0;
    }
    return value_1;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x200181b0. */
uint32_t SLFS_write_buffer(void *file, uint8_t *buffer, uint32_t count)
{
    uint32_t bytesWritten;

    if ((*(int *)((int)file + 0x10) == 0) || (*(char *)((int)file + 0x20) == '\0')) {
        return 0xffffffff;
    }
    if (*(uint32_t *)((int)file + 0x18) != *(uint32_t *)((int)file + 0x14)) {
        bytesWritten =
            sl_FsWrite(*(int *)((int)file + 0x10), *(uint32_t *)((int)file + 0x18), buffer, count);
        *(uint32_t *)((int)file + 0x1c) = bytesWritten;
        if (-1 < (int)bytesWritten) {
            *(uint32_t *)((int)file + 0x18) = *(int *)((int)file + 0x18) + bytesWritten;
            return bytesWritten;
        }
    }
    return 0;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x200181e8. */
bool SLFS_available(int file)
{
    if (*(int *)(file + 0x10) == 0) {
        return false;
    }
    if (*(char *)(file + 0x20) == '\0') {
        return *(int *)(file + 0x18) < *(int *)(file + 0x14);
    }
    return false;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x20018228. */
uint32_t SLFS_begin(void)
{
    int sl_start_result;
    uint32_t value_2;

    if (wifi_initialized != '\0') {
        return 1;
    }
    unresolved_simplelink_protocol_48();
    sl_start_result = sl_Start(0, 0, 0);
    value_2 = 0;
    if ((1 < sl_start_result + 2U) && (sl_start_result != -3)) {
        if (sl_start_result != 0) {
            sl_WlanSetMode('\0');
            sl_Stop(0);
            sl_Start(0, 0, 0);
        }
        sl_WlanDisconnect();
        value_2 = 1;
        unresolved_simplelink_protocol_31((char *)0x0, '\0');
        wifi_initialized = '\x01';
        sl_NetAppStop(1);
        sl_WlanRxStatStart();
    }
    return value_2;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x2001822c. */
uint32_t SLFS_open(int file, uint8_t *filename, uint32_t mode)
{
    short sl_fs_get_info_result;
    long sl_fs_open_result;
    uint32_t value_3;
    uint8_t buffer_34[4];
    uint32_t value_30;
    uint32_t value;

    if (*(int *)(file + 0x10) == 0) {
        sl_fs_open_result = sl_FsOpen(filename, mode, (uint32_t *)0x0, (long *)(file + 0x10));
        *(long *)(file + 0x1c) = sl_fs_open_result;
        if (sl_fs_open_result == 0) {
            *(uint32_t *)(file + 0x18) = 0;
            *(uint32_t *)(file + 0x14) = 0;
            if (mode == 0) {
                *(uint8_t *)(file + 0x20) = 0;
                sl_fs_get_info_result = sl_FsGetInfo(filename, 0, buffer_34);
                *(int *)(file + 0x1c) = (int)sl_fs_get_info_result;
                if (sl_fs_get_info_result == 0) {
                    *(uint32_t *)(file + 0x14) = value_30;
                    return 1;
                }
                sl_FsClose(*(long *)(file + 0x10), (uint8_t *)0x0, (uint8_t *)0x0, 0);
                *(uint32_t *)(file + 0x10) = 0;
                value_3 = 0;
            } else {
                value_3 = 1;
                *(uint8_t *)(file + 0x20) = 1;
                sl_fs_get_info_result = sl_FsGetInfo(filename, 0, buffer_34);
                *(int *)(file + 0x1c) = (int)sl_fs_get_info_result;
                if (sl_fs_get_info_result != 0) {
                    sl_FsClose(*(long *)(file + 0x10), (uint8_t *)0x0, (uint8_t *)0x0, 0);
                    *(uint32_t *)(file + 0x10) = 0;
                    return 0;
                }
                *(uint32_t *)(file + 0x18) = 0;
                *(uint32_t *)(file + 0x14) = value;
            }
            return value_3;
        }
    }
    return 0;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x200182b2. */
int SLFS_close(int file)
{
    short sl_fs_close_result;
    int value_2;

    value_2 = 0;
    if (*(int *)(file + 0x10) != 0) {
        *(uint32_t *)(file + 0x18) = 0;
        *(uint32_t *)(file + 0x14) = 0;
        sl_fs_close_result = sl_FsClose(*(int *)(file + 0x10), (uint8_t *)0x0, (uint8_t *)0x0, 0);
        value_2 = (int)sl_fs_close_result;
        *(int *)(file + 0x1c) = value_2;
        *(uint32_t *)(file + 0x10) = 0;
        *(uint8_t *)(file + 0x20) = 0;
        if (value_2 == 0) {
            value_2 = 1;
        }
    }
    return value_2;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x200182da. */
int SLFS_remove(int file, uint8_t *filename)
{
    short sl_fs_del_result;
    uint32_t value_2;
    int value_3;

    if (*(int *)(file + 0x10) != 0) {
        return 0;
    }
    sl_fs_del_result = sl_FsDel(filename, 0);
    value_2 = (uint32_t)sl_fs_del_result;
    *(uint32_t *)(file + 0x1c) = value_2;
    value_3 = 1 - value_2;
    if (1 < value_2) {
        value_3 = 0;
    }
    return value_3;
}

/* Pinned Energia libraries/WiFi/SLFS.cpp API and observed file-handle/size/offset fields
 * at0x10/0x14/0x18 plus sl_Fs* operations. Local image has modified return/error behavior, so
 * source body was not substituted. Firmware address: 0x200182fc. */
void initialize_global_serial_flash(void)
{
    g_serial_flash_unknown_03 = 1000;
    g_serial_flash_unknown_02 = 0;
    global_serial_flash = &g_serial_flash_unknown_01;
    g_serial_flash_unknown_04 = 0;
    g_serial_flash_unknown_05 = 0;
    g_serial_flash_unknown_06 = 0;
    g_serial_flash_unknown_07 = 0;
    g_serial_flash_unknown_08 = 0;
    return;
}
