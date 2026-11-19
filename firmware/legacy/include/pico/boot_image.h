#ifndef PICO_BOOT_IMAGE_H
#define PICO_BOOT_IMAGE_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int boot_info_write(long argument_1, uint32_t argument_2, uint32_t argument_3);
int boot_info_read(long argument_1, uint32_t argument_2, uint32_t argument_3);
uint32_t boot_info_toggle_application_slot(long activate, uint32_t argument_2, uint32_t argument_3);
uint32_t boot_info_select_factory_slot(long argument_1, uint32_t argument_2, uint32_t argument_3);

#endif /* PICO_BOOT_IMAGE_H */
