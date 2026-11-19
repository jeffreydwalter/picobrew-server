#ifndef PICO_SIMPLELINK_FS_H
#define PICO_SIMPLELINK_FS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

long sl_FsOpen(uint8_t *pFileName, uint32_t AccessModeAndMaxSize, uint32_t *pToken,
               long *pFileHandle);
short sl_FsClose(long FileHdl, uint8_t *pCeritificateFileName, uint8_t *pSignature,
                 uint32_t SignatureLen);
long sl_FsRead(long FileHdl, uint32_t Offset, uint8_t *pData, uint32_t Len);
long sl_FsWrite(long FileHdl, uint32_t Offset, uint8_t *pData, uint32_t Len);
short sl_FsGetInfo(uint8_t *pFileName, uint32_t Token, void *pFsFileInfo);
short sl_FsDel(uint8_t *pFileName, uint32_t Token);

#endif /* PICO_SIMPLELINK_FS_H */
