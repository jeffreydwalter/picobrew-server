#ifndef PICO_STORAGE_SETTINGS_H
#define PICO_STORAGE_SETTINGS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int pico_storage_read_text(char *filename, char *buffer, uint32_t capacity);
int pico_storage_read_restart_record(char *buffer);
uint32_t pico_storage_load_device_uid(void);
int pico_storage_load_shuttle_scale(void);
int pico_storage_load_wifi_credentials(void);
bool pico_reset_shuttle_scale(void);
void pico_storage_load_user_preferences(void);

#endif /* PICO_STORAGE_SETTINGS_H */
