/* Pico Pro 0.1.34 — encoder. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Writes encoder step parameter at offset0x1c; callers select increments1 or5.
 * Firmware address: 0x2000458e. */
/* Firmware entry: 0x2000458e. Reviewed source reconstruction. */
void pico_encoder_set_step(PicoEncoder *encoder, int step)
{
    encoder->step = step;
}

/* Sets minimum, maximum, initial position and previous position.
 * Firmware address: 0x20004592. */
/* Firmware entry: 0x20004592. Reviewed source reconstruction. */
void pico_encoder_set_range(PicoEncoder *encoder, int minimum, int maximum, int initial_position)
{
    encoder->minimum = minimum;
    encoder->maximum = maximum;
    encoder->position = initial_position;
    encoder->previous_position = initial_position;
}

/* Sets encoder wraparound byte at offset0x28.
 * Firmware address: 0x2000459e. */
/* Firmware entry: 0x2000459e. Reviewed source reconstruction. */
void pico_encoder_enable_wrap(PicoEncoder *encoder)
{
    encoder->wrap_enabled = 255;
}

/* Stores both current and previous positions.
 * Firmware address: 0x200045ae. */
/* Firmware entry: 0x200045ae. Reviewed source reconstruction. */
void pico_encoder_set_position(PicoEncoder *encoder, int position)
{
    encoder->position = position;
    encoder->previous_position = position;
}

/* Clamps or wraps position between configured bounds and returns position.
 * Firmware address: 0x200045b6. */
/* Firmware entry: 0x200045b6. Reviewed source reconstruction. */
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

/* Consumes active-low click latch; returns0xff once and restores latch.
 * Firmware address: 0x200045ea. */
/* Firmware entry: 0x200045ea. Reviewed source reconstruction. */
int pico_encoder_take_click(PicoEncoder *encoder)
{
    if (encoder->click_latch == 0) {
        encoder->click_latch = 1;
        return 255;
    }
    return 0;
}

/* Resets click latch to inactive state.
 * Firmware address: 0x200045fa. */
/* Firmware entry: 0x200045fa. Reviewed source reconstruction. */
void pico_encoder_clear_click(PicoEncoder *encoder)
{
    encoder->click_latch = 1;
}

/* Configures rotary phase/button GPIO pins and interrupt handlers.
 * Firmware address: 0x20004640. */
void pico_encoder_initialize(PicoEncoder *encoder, uint8_t phase_apin, uint8_t phase_b_pin,
                             uint8_t button_pin, uint8_t button_interrupt,
                             uint8_t phase_a_interrupt, uint8_t phase_b_interrupt)
{
    pico_u8 initial_phase;
    bool phase_a;
    bool phase_b;
    uint32_t unresolved_input_register_00000025;

    encoder->click_latch = '\x01';
    encoder->position = 0;
    phase_a = digitalRead((uint32_t)encoder->phase_a_pin);
    phase_b = digitalRead((uint32_t)encoder->phase_b_pin);
    encoder->button_interrupt = button_interrupt;
    initial_phase = phase_b + phase_a * '\x02';
    encoder->previous_phase = initial_phase;
    encoder->last_stable_phase = initial_phase;
    encoder->phase_a_interrupt = phase_a_interrupt;
    encoder->phase_b_interrupt = phase_b_interrupt;
    encoder->phase_a_pin = phase_apin;
    encoder->phase_b_pin = phase_b_pin;
    encoder->button_pin = button_pin;
    pinMode(pico_join_u24_u8(unresolved_input_register_00000025, phase_apin), 0);
    pinMode((uint32_t)encoder->phase_b_pin, 0);
    pinMode((uint32_t)encoder->button_pin, 0);
    IntMasterDisable();
    attachInterrupt((uint32_t)encoder->phase_a_interrupt, &LAB_20004714_1, 4);
    attachInterrupt((uint32_t)encoder->phase_b_interrupt, &LAB_20004714_1, 4);
    attachInterrupt((uint32_t)encoder->button_interrupt, &LAB_20004740_1, 3);
    IntMasterEnable();
    return;
}

/* Samples quadrature phases, updates count by direction, applies wrap/clamp.
 * Firmware address: 0x200046b8. */
void pico_encoder_handle_rotation(PicoEncoder *encoder)
{
    bool phase_a;
    bool phase_b;
    pico_u8 phase;
    int encoder_get_position_result;
    pico_i32 value_5;

    phase_a = digitalRead((uint32_t)encoder->phase_a_pin);
    phase_b = digitalRead((uint32_t)encoder->phase_b_pin);
    phase = phase_b + phase_a * '\x02';
    encoder->current_phase = phase;
    if ((encoder->last_stable_phase == phase) || ((phase != '\0' && (phase != '\x03'))))
        goto branch_1;
    if (encoder->previous_phase == '\x01') {
        encoder_get_position_result = encoder->position;
        if (phase == '\0') {
        branch_2:
            value_5 = encoder_get_position_result + -1;
        } else {
        branch_3:
            value_5 = encoder_get_position_result + 1;
        }
        encoder->position = value_5;
    } else if (encoder->previous_phase == '\x02') {
        encoder_get_position_result = encoder->position;
        if (phase == '\0')
            goto branch_3;
        goto branch_2;
    }
    encoder->last_stable_phase = phase;
branch_1:
    encoder->previous_phase = phase;
    encoder_get_position_result = pico_encoder_get_position(encoder);
    encoder->position = encoder_get_position_result;
    return;
}

/* Zeroes encoder position/bounds/wrap and sets step1.
 * Firmware address: 0x2000474c. */
void pico_encoder_reset_state(void)
{
    g_encoder_position = 0;
    g_encoder_minimum = 0;
    g_encoder_maximum = 0;
    g_encoder_wrap_enabled = 0;
    g_encoder_step = 1;
    return;
}

/* Flushes display, clears encoder latch and polls until button click.
 * Firmware address: 0x20004f44. */
void pico_display_wait_for_click(void)
{
    int clicked;

    pico_display_flush();
    pico_encoder_clear_click((PicoEncoder *)&g_encoder);
    while (clicked = pico_encoder_take_click((PicoEncoder *)&g_encoder), clicked == 0) {
        millis();
    }
    return;
}

/* Interactive rotary character selector supporting delete, finish, navigation and bounded text
 * output. Firmware address: 0x20009c98. */
void pico_encoder_edit_text(char *text, int capacity)
{
    int clicked_or_character;
    int selected_character;
    int displayed_character;
    uint8_t *pointer_4;
    uint32_t text_length;
    uint32_t new_length;
    int character_window_index;
    int next_character_index;
    char character_table[100];

    libc_memcpy(character_table,
                " a "
                "aabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&\'()*+,-./"
                ":;<=>?@[\\]^_`{|}~ Imperial",
                99);
    pico_encoder_set_range((PicoEncoder *)&g_encoder, 0, 0x62, 4);
    pico_encoder_enable_wrap((PicoEncoder *)&g_encoder);
    pico_display_clear_rows(3, 7);
    text_length = 0;
    do {
        while (clicked_or_character = pico_encoder_take_click((PicoEncoder *)&g_encoder),
               clicked_or_character != 0) {
            clicked_or_character = pico_encoder_get_position((PicoEncoder *)&g_encoder);
            new_length = text_length;
            if (clicked_or_character == 3) {
                if (text_length == 0) {
                    *text = '\0';
                } else {
                    new_length = text_length - 1 & 0xff;
                    text[new_length] = '\0';
                }
            } else {
                if (clicked_or_character < 3) {
                    g_encoder_wrap_enabled = 0;
                    return;
                }
                if ((int)text_length < capacity + -1) {
                    new_length = text_length + 1 & 0xff;
                    text[text_length] = character_table[clicked_or_character];
                    text[new_length] = '\0';
                }
            }
            pico_display_clear_rows(2, 4);
            pico_display_center_text(2, text);
            pico_display_flush();
            text_length = new_length;
            if (clicked_or_character < 3) {
                g_encoder_wrap_enabled = 0;
                return;
            }
        }
        selected_character = pico_encoder_get_position((PicoEncoder *)&g_encoder);
        character_window_index = (selected_character + 0x5a) % 99;
        pico_display_clear_rows(5, 7);
        clicked_or_character = 0;
        next_character_index = character_window_index;
        do {
            displayed_character = next_character_index % 99;
            if (displayed_character == 3) {
                pointer_4 = "";
            branch_1:
                GFX_draw_bitmap(&g_display, (short)clicked_or_character, 0x30, pointer_4, 8, 8, 1);
            } else {
                if (displayed_character == 0x62) {
                    pointer_4 = "";
                    goto branch_1;
                }
                if (displayed_character == 1) {
                    pointer_4 = "";
                    goto branch_1;
                }
                pico_display_character_on_row((short)clicked_or_character, 6,
                                              character_table[displayed_character]);
            }
            clicked_or_character = clicked_or_character + 6;
            next_character_index = next_character_index + 1;
        } while (clicked_or_character != 0x36);
        character_window_index = character_window_index + 10;
        clicked_or_character = 0x4e;
        do {
            next_character_index = character_window_index % 99;
            if (next_character_index == 3) {
                pointer_4 = "";
            branch_2:
                GFX_draw_bitmap(&g_display, (short)clicked_or_character, 0x30, pointer_4, 8, 8, 1);
            } else {
                if (next_character_index == 0x62) {
                    pointer_4 = "";
                    goto branch_2;
                }
                if (next_character_index == 1) {
                    pointer_4 = "";
                    goto branch_2;
                }
                pico_display_character_on_row((short)clicked_or_character, 6,
                                              character_table[next_character_index]);
            }
            clicked_or_character = clicked_or_character + 6;
            character_window_index = character_window_index + 1;
        } while (clicked_or_character != 0x7e);
        GFX_set_text_size(&g_display, '\x02');
        GFX_fill_rectangle(&g_display, 0x39, 0x28, 0x11, 0x1b, 1);
        GFX_set_text_color(&g_display, 0);
        if (selected_character == 3) {
            pointer_4 = "";
        branch_3:
            GFX_draw_bitmap(&g_display, 0x3b, 0x2b, pointer_4, 0x10, 0x10, 0);
        } else {
            if (selected_character < 4) {
                if (-1 < selected_character) {
                    pointer_4 = "";
                    goto branch_3;
                }
            } else if (selected_character == 0x62) {
                pointer_4 = "";
                goto branch_3;
            }
            pico_display_character_at_pixels(0x3d, 0x2b, character_table[selected_character]);
        }
        GFX_set_text_color(&g_display, 1);
        GFX_set_text_size(&g_display, '\x01');
        pico_display_flush();
    } while (true);
}

/* Rotary selection of0..10 visual boxes with baseline marker and changed output.
 * Firmware address: 0x20009f8c. */
uint32_t pico_encoder_select_adjustment_level(uint32_t argument_1, uint32_t baseline_level,
                                              uint32_t *stored_level, uint32_t changed)
{
    int encoder_position;
    uint32_t selected_level;
    uint32_t box_index;
    int displayed_position;
    int initial_position;
    short baseline_marker_x;

    if (*stored_level == 0xffffffff) {
        *stored_level = baseline_level;
    }
    selected_level = *stored_level;
    initial_position = selected_level - 1;
    pico_encoder_set_range((PicoEncoder *)&g_encoder, -1, 9, initial_position);
    for (box_index = 0; (int)box_index <= initial_position; box_index = box_index + 1) {
        pico_display_adjustment_box(2, box_index & 0xff);
    }
    for (selected_level = selected_level & ~((int)selected_level >> 0x1f); (int)selected_level < 10;
         selected_level = selected_level + 1) {
        pico_display_adjustment_box(0, selected_level & 0xff);
    }
    if (baseline_level == 0) {
        baseline_marker_x = 0xf;
    } else {
        baseline_marker_x = (short)baseline_level * 8 + 0xf;
    }
    GFX_fill_triangle(&g_display, baseline_marker_x + 3, 0x1f, baseline_marker_x + -3, 0x1f,
                      baseline_marker_x, 0x25, 1);
    pico_display_flush();
    pico_encoder_clear_click((PicoEncoder *)&g_encoder);
    displayed_position = initial_position;
    while (encoder_position = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           encoder_position == 0) {
        encoder_position = pico_encoder_get_position((PicoEncoder *)&g_encoder);
        if (encoder_position != displayed_position) {
            if (encoder_position < displayed_position) {
                displayed_position = displayed_position + -1;
            } else {
                displayed_position = displayed_position + 1;
            }
            for (selected_level = 0; (int)selected_level <= displayed_position;
                 selected_level = selected_level + 1) {
                pico_display_adjustment_box(2, selected_level & 0xff);
            }
            for (; (int)selected_level < 10; selected_level = selected_level + 1) {
                pico_display_adjustment_box(0, selected_level & 0xff);
            }
            pico_display_flush();
            pico_encoder_set_position((PicoEncoder *)&g_encoder, displayed_position);
        }
    }
    displayed_position = pico_encoder_get_position((PicoEncoder *)&g_encoder);
    *(bool *)changed = initial_position != displayed_position;
    initial_position = pico_encoder_get_position((PicoEncoder *)&g_encoder);
    if ((initial_position < 0) ||
        (initial_position = pico_encoder_get_position((PicoEncoder *)&g_encoder),
         initial_position < 9)) {
        initial_position = pico_encoder_get_position((PicoEncoder *)&g_encoder);
        if (initial_position < 0) {
            selected_level = 0;
        } else {
            initial_position = pico_encoder_get_position((PicoEncoder *)&g_encoder);
            selected_level = initial_position + 1U & 0xff;
        }
    } else {
        selected_level = 10;
    }
    return selected_level;
}
