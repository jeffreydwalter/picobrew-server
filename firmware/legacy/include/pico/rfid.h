#ifndef PICO_RFID_H
#define PICO_RFID_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

bool pico_rfid_probe(void);
uint8_t pico_rfid_read_uid_until_click(char *uid_hex);
uint8_t pico_rfid_read_uid(char *uid_hex);
void pico_rfid_initialize(void);
uint8_t pico_rfid_wait_for_uid(char *uid_text, int capacity, uint32_t argument_3,
                               uint32_t argument_4);
uint8_t pico_rfid_read_picopak(char *uid_text, int capacity);

#endif /* PICO_RFID_H */
