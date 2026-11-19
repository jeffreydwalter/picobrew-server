#ifndef PICO_FIRMWARE_UPDATE_H
#define PICO_FIRMWARE_UPDATE_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint32_t pico_open_firmware_destination(int recovery_image, uint32_t argument_2,
                                        uint32_t argument_3);
int pico_check_firmware_update(void);
uint32_t pico_write_firmware_chunk(void *data, uint32_t length);
int pico_receive_firmware_image(int destination);
int pico_download_firmware(void);
void pico_update_firmware(void);

#endif /* PICO_FIRMWARE_UPDATE_H */
