#ifndef PICO_CC3200_HASH_H
#define PICO_CC3200_HASH_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint32_t SHAMD5IntStatus(uint32_t ui32_base, bool b_masked);
void SHAMD5IntEnable(uint32_t ui32_base, uint32_t ui32_int_flags);
void SHAMD5IntDisable(uint32_t ui32_base, uint32_t ui32_int_flags);
void SHAMD5IntRegister(uint32_t ui32_base, PicoInterruptHandler *pfn_handler);
void SHAMD5ConfigSet(uint32_t ui32_base, uint32_t ui32_mode);
void SHAMD5DataWrite(uint32_t ui32_base, uint8_t *pui8_src);
void SHAMD5DataWriteMultiple(uint32_t ui32_base, uint8_t *pui8_data_src, uint32_t ui32_data_length);
void SHAMD5ResultRead(uint32_t ui32_base, uint8_t *pui8_dest);

#endif /* PICO_CC3200_HASH_H */
