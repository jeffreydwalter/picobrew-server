/* Pico Pro 0.1.34 — simplelink fs. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NVMEM_FILEOPEN': '0xa43c'} and matching
 * argument/control flow in pinned fs.c. Firmware address: 0x20016d40. */
long sl_FsOpen(uint8_t *pFileName, uint32_t AccessModeAndMaxSize, uint32_t *pToken,
               long *pFileHandle)
{
    uint32_t value_1;
    uint32_t value_2;
    uint32_t value_30;
    uint32_t value_3;
    uint16_t buffer_28[4];
    uint8_t *pointer_20;
    uint32_t value;

    value_1 = unresolved_simplelink_protocol_29((int)pFileName);
    buffer_28[0] = (short)value_1 + 4U & 0xfffc;
    buffer_28[1] = 0;
    value = 0;
    if (pToken == (uint32_t *)0x0) {
        value_3 = 0;
    } else {
        value_3 = *pToken;
    }
    value_30 = AccessModeAndMaxSize;
    pointer_20 = pFileName;
    value_1 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_fs_unknown_06, &value_30,
                                                (short *)buffer_28);
    *pFileHandle = value_30;
    if (pToken != (uint32_t *)0x0) {
        *pToken = value_3;
    }
    value_2 = *pFileHandle;
    value_1 = value_1 & ~((int)value_2 >> 0x20);
    if ((int)value_2 < 0) {
        value_1 = value_2;
    }
    return value_1;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NVMEM_FILECLOSE': '0xa43d'} and matching
 * argument/control flow in pinned fs.c. Firmware address: 0x20016d98. */
short sl_FsClose(long FileHdl, uint8_t *pCeritificateFileName, uint8_t *pSignature,
                 uint32_t SignatureLen)
{
    uint32_t value_1;
    int value_2;
    long value_34;
    uint32_t value_30;
    uint32_t value;
    uint16_t short_value_28;
    short short_value_26;
    uint8_t *pointer_20;
    uint8_t *pointer;

    libc_memset(&value_34, 0, 0xc);
    value_34 = FileHdl;
    if (pCeritificateFileName != (uint8_t *)0x0) {
        value_1 = unresolved_simplelink_protocol_29((int)pCeritificateFileName);
        value_30 = value_1 + 4 & 0xfffffffc;
    }
    short_value_28 = (short)SignatureLen + 3U & 0xfffc;
    short_value_26 = (short)value_30;
    if ((pCeritificateFileName != (uint8_t *)0x0) && ((value_30 & 0xffff) != 0)) {
        short_value_26 = -short_value_26;
    }
    value = SignatureLen;
    pointer_20 = pSignature;
    pointer = pCeritificateFileName;
    value_2 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_fs_unknown_04, &value_34,
                                                (short *)&short_value_28);
    if (value_2 == 0) {
        return (short)value_34;
    }
    do {

    } while (true);
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NVMEM_FILEREADCOMMAND': '0xa440'} and matching
 * argument/control flow in pinned fs.c. Firmware address: 0x20016dfc. */
long sl_FsRead(long FileHdl, uint32_t Offset, uint8_t *pData, uint32_t Len)
{
    uint32_t value_1;
    int value_2;
    uint32_t value_3;
    int value_4;
    long value_34;
    uint32_t value_30;
    uint16_t short_value;
    short short_value_28;
    uint16_t short_value_26;
    uint32_t value_20;
    uint8_t *pointer;

    value_3 = 0x5b4;
    if (Len < 0x5b5) {
        value_3 = Len & 0xffff;
    }
    short_value_28 = 0;
    value_20 = 0;
    short_value_26 = (uint16_t)value_3;
    value_4 = 0;
    value_30 = Offset;
    pointer = pData;
    while (true) {
        value_1 = value_30;
        value_34 = FileHdl;
        short_value = short_value_26;
        value_2 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_fs_unknown_02,
                                                    &value_34, &short_value_28);
        if (value_2 != 0) {
            return (int)(short)value_2;
        }
        value_2 = (int)(short)value_34;
        if (value_2 < 0)
            break;
        value_4 = value_4 + value_2;
        Len = Len - value_3;
        value_30 = value_1 + value_3;
        pointer = pointer + value_3;
        value_3 = 0x5b4;
        if (Len < 0x5b5) {
            value_3 = Len & 0xffff;
        }
        short_value_26 = (uint16_t)value_3;
        if (value_3 == 0) {
            return value_4;
        }
    }
    if (value_4 == 0) {
        return value_2;
    }
    return value_4;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NVMEM_FILEWRITECOMMAND': '0xa441'} and matching
 * argument/control flow in pinned fs.c. Firmware address: 0x20016e84. */
long sl_FsWrite(long FileHdl, uint32_t Offset, uint8_t *pData, uint32_t Len)
{
    uint32_t value_1;
    int value_2;
    uint32_t value_3;
    int value_4;
    long value_34;
    uint32_t value_30;
    short short_value;
    short buffer_28[4];
    uint8_t *pointer_20;
    uint32_t value;

    value_3 = 0x5b4;
    if (Len < 0x5b5) {
        value_3 = Len & 0xffff;
    }
    buffer_28[1] = 0;
    value = 0;
    buffer_28[0] = (short)value_3;
    value_4 = 0;
    value_30 = Offset;
    pointer_20 = pData;
    while (true) {
        value_1 = value_30;
        value_34 = FileHdl;
        short_value = buffer_28[0];
        value_2 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_fs_unknown_03,
                                                    &value_34, buffer_28);
        if (value_2 != 0) {
            return (int)(short)value_2;
        }
        value_2 = (int)(short)value_34;
        if (value_2 < 0)
            break;
        value_4 = value_4 + value_2;
        Len = Len - value_3;
        value_30 = value_1 + value_3;
        pointer_20 = pointer_20 + value_3;
        value_3 = 0x5b4;
        if (Len < 0x5b5) {
            value_3 = Len & 0xffff;
        }
        buffer_28[0] = (short)value_3;
        if (value_3 == 0) {
            return value_4;
        }
    }
    if (value_4 == 0) {
        return value_2;
    }
    return value_4;
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NVMEM_FILEGETINFOCOMMAND': '0xa442'} and matching
 * argument/control flow in pinned fs.c. Firmware address: 0x20016f0c. */
short sl_FsGetInfo(uint8_t *pFileName, uint32_t Token, void *pFsFileInfo)
{
    uint32_t value_1;
    int value_2;
    uint16_t buffer[4];
    uint8_t *pointer_34;
    uint32_t value_30;
    uint32_t value;
    uint32_t value_28;
    uint32_t value_24;
    uint32_t value_20;
    uint32_t value_3;
    uint32_t value_18;
    uint32_t value_14;

    value_1 = unresolved_simplelink_protocol_29((int)pFileName);
    buffer[0] = (short)value_1 + 4U & 0xfffc;
    buffer[1] = 0;
    value_30 = 0;
    pointer_34 = pFileName;
    value = Token;
    value_2 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_fs_unknown_05, &value,
                                                (short *)buffer);
    if (value_2 == 0) {
        *(uint16_t *)pFsFileInfo = PICO_PART_U16(value, 2);
        *(uint32_t *)((int)pFsFileInfo + 4) = value_28;
        *(uint32_t *)((int)pFsFileInfo + 8) = value_24;
        *(uint32_t *)((int)pFsFileInfo + 0xc) = value_20;
        *(uint32_t *)((int)pFsFileInfo + 0x10) = value_3;
        *(uint32_t *)((int)pFsFileInfo + 0x14) = value_18;
        *(uint32_t *)((int)pFsFileInfo + 0x18) = value_14;
        return (short)value;
    }
    do {

    } while (true);
}

/* Unique SimpleLink command opcode(s) {'SL_OPCODE_NVMEM_FILEDELCOMMAND': '0xa443'} and matching
 * argument/control flow in pinned fs.c. Firmware address: 0x20016f6c. */
short sl_FsDel(uint8_t *pFileName, uint32_t Token)
{
    uint32_t value_1;
    int value_2;
    uint32_t value_24;
    uint16_t buffer_20[4];
    uint8_t *pointer_18;
    uint32_t value_14;

    value_1 = unresolved_simplelink_protocol_29((int)pFileName);
    buffer_20[0] = (short)value_1 + 4U & 0xfffc;
    buffer_20[1] = 0;
    value_14 = 0;
    value_24 = Token;
    pointer_18 = pFileName;
    value_2 = unresolved_simplelink_protocol_25((uint16_t *)&g_simplelink_fs_unknown_01, &value_24,
                                                (short *)buffer_20);
    if (value_2 == 0) {
        return (short)value_24;
    }
    do {

    } while (true);
}
