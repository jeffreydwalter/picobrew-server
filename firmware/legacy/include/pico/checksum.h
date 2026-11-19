#ifndef PICO_CHECKSUM_H
#define PICO_CHECKSUM_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_hash_interrupt_handler(void);
int pico_validate_ota_checksum(char *expected_hex, int nibble_sum);

#endif /* PICO_CHECKSUM_H */
