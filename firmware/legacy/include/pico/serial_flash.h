#ifndef PICO_SERIAL_FLASH_H
#define PICO_SERIAL_FLASH_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void SLFS_end(void);
uint32_t SLFS_peek(int file, uint32_t argument_2);
uint32_t SLFS_read_byte(int file, uint32_t argument_2);
uint32_t SLFS_write_byte(int file);
uint32_t SLFS_write_buffer(void *file, uint8_t *buffer, uint32_t count);
bool SLFS_available(int file);
uint32_t SLFS_begin(void);
uint32_t SLFS_open(int file, uint8_t *filename, uint32_t mode);
int SLFS_close(int file);
int SLFS_remove(int file, uint8_t *filename);
void initialize_global_serial_flash(void);

#endif /* PICO_SERIAL_FLASH_H */
