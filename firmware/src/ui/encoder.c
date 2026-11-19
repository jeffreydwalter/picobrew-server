#include "ui_internal.h"
#include "picobrew/local_control.h"

void pico_encoder_set_step(PicoEncoder *encoder, int step)
{
    encoder->step = step;
}

void pico_encoder_set_range(PicoEncoder *encoder, int minimum, int maximum, int initial_position)
{
    encoder->minimum = minimum;
    encoder->maximum = maximum;
    encoder->position = initial_position;
    encoder->previous_position = initial_position;
}

void pico_encoder_enable_wrap(PicoEncoder *encoder)
{
    encoder->wrap_enabled = 255;
}

void pico_encoder_set_position(PicoEncoder *encoder, int position)
{
    encoder->position = position;
    encoder->previous_position = position;
}

int pico_encoder_get_position(PicoEncoder *encoder)
{
    if (encoder->wrap_enabled) {
        if (encoder->position < encoder->minimum)
            encoder->position = encoder->maximum;
        else if (encoder->position > encoder->maximum)
            encoder->position = encoder->minimum;
    } else {
        if (encoder->position > encoder->maximum)
            encoder->position = encoder->maximum;
        if (encoder->position < encoder->minimum)
            encoder->position = encoder->minimum;
    }
    return encoder->position;
}

int pico_encoder_take_click(PicoEncoder *encoder)
{
    pico_local_poll();
    if (encoder->click_latch == 0) {
        encoder->click_latch = 1;
        return 255;
    }
    return 0;
}

void pico_encoder_clear_click(PicoEncoder *encoder)
{
    encoder->click_latch = 1;
}

void pico_encoder_handle_rotation(PicoEncoder *encoder)
{
    bool phase_a;
    bool phase_b;
    uint8_t phase;
    int position;
    int32_t next_position;

    phase_a = ui_read((uint32_t)encoder->phase_a_pin);
    phase_b = ui_read((uint32_t)encoder->phase_b_pin);
    phase = phase_b + phase_a * '\x02';
    encoder->current_phase = phase;
    if ((encoder->last_stable_phase == phase) || ((phase != '\0' && (phase != '\x03'))))
        goto update_phase_and_clamp;
    if (encoder->previous_phase == '\x01') {
        position = encoder->position;
        if (phase == '\0') {
        step_backward:
            next_position = position + -1;
        } else {
        step_forward:
            next_position = position + 1;
        }
        encoder->position = next_position;
    } else if (encoder->previous_phase == '\x02') {
        position = encoder->position;
        if (phase == '\0')
            goto step_forward;
        goto step_backward;
    }
    encoder->last_stable_phase = phase;
update_phase_and_clamp:
    encoder->previous_phase = phase;
    position = pico_encoder_get_position(encoder);
    encoder->position = position;
    return;
}

void pico_encoder_reset_state(void)
{
    pico_ui_active->encoder.position = 0;
    pico_ui_active->encoder.minimum = 0;
    pico_ui_active->encoder.maximum = 0;
    pico_ui_active->encoder.wrap_enabled = 0;
    pico_ui_active->encoder.step = 1;
    return;
}

void pico_display_wait_for_click(void)
{
    int clicked;

    pico_display_flush();
    pico_encoder_clear_click((PicoEncoder *)&pico_ui_active->encoder);
    while (clicked = pico_encoder_take_click((PicoEncoder *)&pico_ui_active->encoder),
           clicked == 0) {
        if (pico_local_cancelled())
            break;
        ui_millis();
    }
    return;
}
