#ifndef PICO_PERSISTENCE_H
#define PICO_PERSISTENCE_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

bool pico_session_supports_recovery(uint32_t session_type);
uint32_t pico_mcu_image1_exists(void);
bool pico_file_write_text(uint8_t *filename, char *text, uint32_t capacity, uint32_t argument_4);
bool pico_recovery_save_state(char *recipe_identifier, int remaining_active_minutes,
                              int remaining_hold_minutes, int step_index, int resume_state,
                              uint32_t session_type, int adjustment_a, int adjustment_b,
                              char force_write, uint8_t calibration_complete);
bool pico_settings_save_wifi(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                             uint32_t argument_4);
bool pico_settings_save_display(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3);
uint32_t pico_file_delete(uint8_t *filename);
void pico_recovery_delete_state(void);
void pico_settings_delete_wifi(void);
void pico_settings_delete_user_files(void);
bool pico_settings_save_shuttle_scale(void);
bool pico_recovery_save_current_session(int remaining_active_minutes, int remaining_hold_minutes,
                                        int step_index, int resume_state, char force_write,
                                        uint8_t calibration_complete);
bool pico_file_close(void);

#endif /* PICO_PERSISTENCE_H */
