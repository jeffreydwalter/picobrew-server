/* Pico Pro 0.1.34 — boot image. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Reads/writes an8-byte boot-info file with SimpleLink filesystem, selection byte at2002e5b0 and
 * magic0xabcddcba at2002e5b4. Boot slot transitions are explicit; no claim of complete updater
 * behavior. Firmware address: 0x20018308. */
int boot_info_write(long argument_1, uint32_t argument_2, uint32_t argument_3)
{
    long sl_fs_open_result;
    int status;
    long fileHandle;
    uint32_t fileToken;
    uint32_t value_10;

    fileHandle = argument_1;
    fileToken = argument_2;
    value_10 = argument_3;
    sl_fs_open_result =
        sl_FsOpen((uint8_t *)"/sys/mcubootinfo.bin", 0x1000, &fileToken, &fileHandle);
    if (sl_fs_open_result == 0) {
        sl_fs_open_result = sl_FsWrite(fileHandle, 0, &boot_info_selected_image, 8);
        status = -(uint32_t)(sl_fs_open_result < 1);
        sl_FsClose(fileHandle, (uint8_t *)0x0, (uint8_t *)0x0, 0);
    } else {
        status = -1;
    }
    return status;
}

/* Reads/writes an8-byte boot-info file with SimpleLink filesystem, selection byte at2002e5b0 and
 * magic0xabcddcba at2002e5b4. Boot slot transitions are explicit; no claim of complete updater
 * behavior. Firmware address: 0x20018354. */
int boot_info_read(long argument_1, uint32_t argument_2, uint32_t argument_3)
{
    long sl_fs_open_result;
    int status;
    long fileHandle;
    uint32_t fileToken;
    uint32_t value_10;

    fileHandle = argument_1;
    fileToken = argument_2;
    value_10 = argument_3;
    sl_fs_open_result = sl_FsOpen((uint8_t *)"/sys/mcubootinfo.bin", 0, &fileToken, &fileHandle);
    if (sl_fs_open_result == 0) {
        sl_fs_open_result = sl_FsRead(fileHandle, 0, &boot_info_selected_image, 8);
        status = -(uint32_t)(sl_fs_open_result < 1);
        sl_FsClose(fileHandle, (uint8_t *)0x0, (uint8_t *)0x0, 0);
    } else {
        status = -1;
    }
    return status;
}

/* Reads/writes an8-byte boot-info file with SimpleLink filesystem, selection byte at2002e5b0 and
 * magic0xabcddcba at2002e5b4. Boot slot transitions are explicit; no claim of complete updater
 * behavior. Firmware address: 0x2001839c. */
uint32_t boot_info_toggle_application_slot(long activate, uint32_t argument_2, uint32_t argument_3)
{
    int boot_info_read_result;
    uint32_t unresolved_return_r1;
    uint32_t value_2;

    boot_info_read_result = boot_info_read(activate, argument_2, argument_3);
    if (activate != 1) {
        return 1;
    }
    boot_info_magic = 0xabcddcba;
    if (boot_info_selected_image_as_char == '\x01') {
        value_2 = 2;
    } else {
        value_2 = 1;
    }
    boot_info_selected_image_as_char = (char)value_2;
    boot_info_write(boot_info_read_result, unresolved_return_r1, value_2);
    return 0;
}

/* Reads/writes an8-byte boot-info file with SimpleLink filesystem, selection byte at2002e5b0 and
 * magic0xabcddcba at2002e5b4. Boot slot transitions are explicit; no claim of complete updater
 * behavior. Firmware address: 0x200183d0. */
uint32_t boot_info_select_factory_slot(long argument_1, uint32_t argument_2, uint32_t argument_3)
{
    int boot_info_read_result;
    uint32_t unresolved_return_r1;

    boot_info_read_result = boot_info_read(argument_1, argument_2, argument_3);
    boot_info_magic = 0xabcddcba;
    boot_info_selected_image = 0;
    boot_info_write(boot_info_read_result, unresolved_return_r1, 0xabcddcba);
    return 0;
}
