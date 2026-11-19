#ifndef PICO_ENCODER_H
#define PICO_ENCODER_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_encoder_set_step(PicoEncoder *encoder, int step);
void pico_encoder_set_range(PicoEncoder *encoder, int minimum, int maximum, int initial_position);
void pico_encoder_enable_wrap(PicoEncoder *encoder);
void pico_encoder_set_position(PicoEncoder *encoder, int position);
int pico_encoder_get_position(PicoEncoder *encoder);
int pico_encoder_take_click(PicoEncoder *encoder);
void pico_encoder_clear_click(PicoEncoder *encoder);
void pico_encoder_initialize(PicoEncoder *encoder, uint8_t phase_apin, uint8_t phase_b_pin,
                             uint8_t button_pin, uint8_t button_interrupt,
                             uint8_t phase_a_interrupt, uint8_t phase_b_interrupt);
void pico_encoder_handle_rotation(PicoEncoder *encoder);
void pico_encoder_reset_state(void);
void pico_display_wait_for_click(void);
void pico_encoder_edit_text(char *text, int capacity);
uint32_t pico_encoder_select_adjustment_level(uint32_t argument_1, uint32_t baseline_level,
                                              uint32_t *stored_level, uint32_t changed);

#endif /* PICO_ENCODER_H */
