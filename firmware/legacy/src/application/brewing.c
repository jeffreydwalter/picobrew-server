/* Pico Pro 0.1.34 — brewing. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Explicit setBrewState log; switches actuators and phase-specific controller fields.
 * Firmware address: 0x2000cde0. */
void pico_controller_set_brew_phase(int controller, int brew_phase)
{
    char shuttle_update_calibration_result;
    uint32_t now_ms;
    uint32_t value_3;
    double __aeabi_dadd_result;
    char buffer_90[132];

    libc_snprintf(buffer_90, 0x80, "%s %d", "setBrewState", brew_phase);
    pico_log_serial("I", "BREW", buffer_90);
    g_screen_brew_state = 0;
    g_initialization_unknown_21 = 0;
    switch (brew_phase) {
    case 0:
    case 8:
        g_screen_brew_state = 1;
        shuttle_update_calibration_result = pico_shuttle_update_calibration(controller);
        if (shuttle_update_calibration_result == '\0') {
            value_3 = 0x4072c000;
        } else {
            value_3 = 0x4074a000;
        }
        *(uint32_t *)(controller + 0x1d8) = 0;
        *(uint32_t *)(controller + 0x1dc) = value_3;
        pico_label_heating((char *)(controller + 99));
        break;
    case 1:
        g_screen_brew_state = 2;
        g_initialization_unknown_21 = 1;
        __aeabi_dadd_result = __aeabi_dadd(*(double *)(controller + 0x220), 0.1);
        *(double *)(controller + 0x218) = __aeabi_dadd_result;
        pico_label_holding((char *)(controller + 99));
        break;
    case 2:
        g_screen_brew_state = 3;
        pico_heat_controller_stop(controller);
        pico_label_cooling((char *)(controller + 99));
        goto branch_1;
    case 3:
        pico_heat_controller_stop(controller);
        pico_label_draining((char *)(controller + 99));
        pico_drain_pump_on();
        pico_steam_pump_on();
        pico_shuttle_pump_on();
        delay(5000);
        now_ms = millis();
        *(uint32_t *)(controller + 0x210) = now_ms;
        pico_shuttle_pump_off();
        pico_steam_pump_off();
        pico_stepper_move_to_compartment(0);
        *(uint8_t *)(controller + 0x60) = 0;
        goto branch_2;
    default:
        pico_heat_controller_stop(controller);
        goto branch_2;
    case 5:
        pico_heat_controller_stop(controller);
        pico_label_holding((char *)(controller + 99));
    branch_1:
        pico_controller_start_circulation();
        goto branch_2;
    case 6:
        pico_label_holding((char *)(controller + 99));
        pico_heat_controller_stop(controller);
        pico_drain_pump_on();
        pico_shuttle_pump_on();
        goto branch_3;
    case 7:
        pico_heat_controller_stop(controller);
        pico_shuttle_pump_on();
        pico_label_holding((char *)(controller + 99));
        pico_drain_pump_on();
    branch_3:
        if (*(char *)(controller + 0x60) != '\x01') {
            pico_steam_pump_on();
        }
        goto branch_2;
    }
    pico_controller_start_circulation();
    pico_heat_controller_start(controller);
branch_2:
    *(char *)(controller + 0x62) = (char)brew_phase;
    return;
}

/* Clears controller status byte and re-enters current brew phase.
 * Firmware address: 0x2000cf68. */
void pico_controller_clear_status(int controller)
{
    pico_display_clear_row_from_column(0, 9);
    *(uint8_t *)(controller + 0x61) = 0;
    pico_controller_set_brew_phase(controller, (uint32_t) * (uint8_t *)(controller + 0x62));
    return;
}

/* Stops control/pumps, waits through paused UI, restores current phase.
 * Firmware address: 0x2000cf86. */
void pico_controller_pause_until_resume(int controller)
{
    pico_heat_controller_stop(controller);
    pico_steam_and_drain_pumps_off();
    pico_ui_wait_for_resume();
    pico_controller_set_brew_phase(controller, (uint32_t) * (uint8_t *)(controller + 0x62));
    if ((*(uint8_t *)(controller + 0x62) & 0xfd) == 0) {
        pico_display_refresh_current_page();
        delay(5000);
        return;
    }
    return;
}

/* Explicit endBrew_runFor milliseconds/drain/shuttle log and timed filter-presence handling.
 * Firmware address: 0x2000d148. */
void pico_end_brew_run_outputs_for(int duration_ms, char *program_title, int drain_enabled,
                                   int shuttle_enabled)
{
    uint32_t started_ms;
    uint32_t now_ms;
    int resumed_after_filter_removal;
    char *drain_state_text;
    char *shuttle_state_text;
    char log_message[128];

    shuttle_state_text = " ON";
    if (drain_enabled == 0) {
        drain_state_text = "OFF";
    } else {
        drain_state_text = " ON";
    }
    if (shuttle_enabled == 0) {
        shuttle_state_text = "OFF";
    }
    libc_snprintf(log_message, 0x80, "endBrew_runFor %d drain = %s shuttle = %s", duration_ms,
                  drain_state_text, shuttle_state_text);
    pico_log_serial("I", "RCTL", log_message);
    started_ms = millis();
    while (now_ms = millis(), now_ms < duration_ms + started_ms) {
        resumed_after_filter_removal =
            pico_pause_outputs_for_missing_filter(0, drain_enabled, shuttle_enabled);
        if (resumed_after_filter_removal != 0) {
            pico_screen_show_ending_program(program_title);
        }
    }
    return;
}

/* Disables controller, presents step-finish screen, drains as required and clears step state.
 * Firmware address: 0x2000d314. */
void pico_finish_brew_step(int controller, char *program_title)
{
    millis();
    pico_heat_controller_stop(controller);
    pico_screen_ending_step(program_title);
    pico_display_flush();
    pico_steam_pump_off();
    if (2 < g_session_type - 3U) {
        pico_drain_pump_on();
    }
    delay(1000);
    if (*(char *)(controller + 0x60) == '\x03') {
        pico_stepper_move_to_compartment(6);
        *(uint8_t *)(controller + 0x60) = 6;
        delay(30000);
    }
    *(uint8_t *)(controller + 0x61) = 0;
    *(uint8_t *)(controller + 0x62) = 4;
    *(uint8_t *)(controller + 0xa8) = 0;
    pico_display_clear();
    return;
}

/* Vacuum instructions then timed compartment sweep until user presses encoder.
 * Firmware address: 0x2000d484. */
void pico_vacuum_compartments(uint32_t argument_1, uint32_t compartment_id,
                              uint32_t first_compartment)
{
    int input_result;
    uint32_t phase_started_ms;
    uint32_t now_ms;
    uint32_t resumed_ms;
    uint32_t dwell_ms;

    pico_actuators_all_off();
    input_result = pico_ui_prompt_vacuum(0);
    if (input_result != 0) {
        pico_steam_pump_on();
        pico_ui_prompt_vacuum(1);
        phase_started_ms = millis();
        millis();
        dwell_ms = 3000;
        while (input_result = pico_encoder_take_click((PicoEncoder *)&g_encoder),
               input_result == 0) {
            now_ms = millis();
            input_result = pico_pause_outputs_for_missing_filter(1, 0, 1);
            if (input_result != 0) {
                pico_ui_prompt_vacuum(1);
                resumed_ms = millis();
                phase_started_ms = (phase_started_ms - now_ms) + resumed_ms;
            }
            now_ms = millis();
            if (dwell_ms < now_ms - phase_started_ms) {
                pico_steam_pump_off();
                pico_stepper_move_to_compartment(compartment_id);
                pico_steam_pump_on();
                phase_started_ms = millis();
                compartment_id = compartment_id + 1 & 0xff;
                if (6 < compartment_id) {
                    dwell_ms = dwell_ms * 3;
                    compartment_id = first_compartment;
                }
            }
        }
        pico_steam_pump_off();
        pico_drain_pump_off();
    }
    pico_display_clear();
    pico_restore_idle_hardware();
    return;
}

/* Runs circulation at compartment2 with wort-temp/elapsed display until encoder press.
 * Firmware address: 0x2000d524. */
void pico_run_circulation(uint32_t *wort_temperature_words)
{
    int encoder_take_click_result;
    uint32_t now_ms;
    uint32_t value_3;
    uint32_t value_4;

    pico_screen_show_program_notice();
    pico_stepper_move_to_compartment(2);
    pico_start_drain_and_steam_pumps();
    value_4 = 0;
    value_3 = 0;
    while (encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           encoder_take_click_result == 0) {
        encoder_take_click_result = pico_step_filter_is_inserted();
        if (encoder_take_click_result == 0) {
            pico_steam_and_drain_pumps_off();
            pico_prompt_insert_filter_with_alarm();
            pico_start_drain_and_steam_pumps();
        }
        now_ms = millis();
        if (value_4 < now_ms) {
            pico_ui_draw_circulation_status(*wort_temperature_words, wort_temperature_words[1],
                                            value_3);
            value_4 = now_ms + 1000;
            value_3 = value_3 + 1;
        }
    }
    pico_steam_and_drain_pumps_off();
    pico_restore_idle_hardware();
    return;
}

/* DRAIN RESERVOIR UI, shuttle/steam pump operation and filter-removal handling.
 * Firmware address: 0x2000d580. */
void pico_drain_reservoir(void)
{
    int encoder_take_click_result;

    pico_display_clear();
    millis();
    pico_ui_draw_drain_reservoir();
    pico_display_flush();
    pico_stepper_move_to_compartment(2);
    pico_shuttle_pump_on();
    delay(500);
    pico_steam_pump_on();
    do {
        pico_ui_draw_stop_footer();
        pico_display_flush();
        do {
            encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder);
            if (encoder_take_click_result != 0) {
                pico_shuttle_pump_off();
                delay(500);
                pico_steam_pump_off();
                pico_restore_idle_hardware();
                return;
            }
            encoder_take_click_result = pico_step_filter_is_inserted();
        } while (encoder_take_click_result != 0);
        pico_shuttle_pump_off();
        pico_steam_pump_off();
        pico_prompt_insert_filter_with_alarm();
        pico_steam_pump_on();
        delay(500);
        pico_shuttle_pump_on();
        pico_display_clear();
        pico_ui_draw_drain_reservoir();
    } while (true);
}

/* Sweeps compartments1..6, running pump3seconds in each, with minimum cycle/exit controls.
 * Firmware address: 0x2000d738. */
void pico_flush_compartments(uint32_t minimum_cycles, int stop_automatically)
{
    uint32_t pump_started_ms;
    uint32_t now_ms;
    int button_pressed;
    uint32_t compartment_id;
    uint8_t completed_cycles;

    pico_display_clear();
    millis();
    pico_ui_draw_header("TEST ROTARY VALVE");
    pico_display_flush();
    pico_drain_pump_on();
    pico_stepper_move_to_compartment(0);
    completed_cycles = 0;
    do {
        if (minimum_cycles <= completed_cycles) {
            if (stop_automatically != 0) {
            branch_1:
                pico_display_clear();
                pico_steam_and_drain_pumps_off();
                pico_restore_idle_hardware();
                return;
            }
            pico_ui_draw_stop_footer();
            pico_display_flush();
        }
        compartment_id = 1;
        do {
            pico_stepper_move_to_compartment(compartment_id & 0xff);
            pump_started_ms = millis();
            pico_display_valve_position(3, compartment_id);
            pico_display_center_text(5, "(Keg Pump Active)");
            pico_display_flush();
            pico_steam_pump_on();
            while (now_ms = millis(), now_ms - pump_started_ms < 3000) {
                button_pressed = pico_encoder_take_click((PicoEncoder *)&g_encoder);
                if ((button_pressed != 0) && (minimum_cycles <= completed_cycles))
                    goto branch_1;
            }
            pico_steam_pump_off();
            compartment_id = compartment_id + 1;
            pico_display_clear_row(5);
            pico_display_flush();
        } while (compartment_id != 7);
        completed_cycles = completed_cycles + 1;
    } while (true);
}

/* Compartment flush wrapper with no minimum cycles and user exit enabled.
 * Firmware address: 0x2000d7ec. */
void pico_flush_compartments_until_press(void)
{
    pico_flush_compartments(0, 0);
    return;
}

/* Reads wort temperature, primes/circulates as needed, chooses heating/cooling/hold brew phase.
 * Firmware address: 0x2000d7f8. */
void pico_prepare_brew_step_temperature(int controller, int target_fahrenheit, char *program_title)
{
    double *wort_temperature_words;
    double *thermoblock_temperature_words;
    uint32_t phase_started_ms;
    int step_filter_is_inserted_result;
    float temperature_read_pin25_result;
    uint32_t now_ms;
    double __aeabi_f2d_result;
    double right;
    double right_00;

    millis();
    phase_started_ms = millis();
    step_filter_is_inserted_result = pico_step_filter_is_inserted();
    if (step_filter_is_inserted_result == 0) {
        pico_steam_and_drain_pumps_off();
        pico_display_clear();
        pico_prompt_insert_filter_with_alarm();
        pico_display_clear();
        pico_display_draw_titled_frame(program_title);
        pico_screen_changing_step();
        pico_display_flush();
    }
    pico_controller_start_circulation();
    if (target_fahrenheit != 0) {
        temperature_read_pin25_result = pico_temperature_read_pin25();
        __aeabi_f2d_result = __aeabi_f2d(temperature_read_pin25_result);
        wort_temperature_words = (double *)(controller + 0x1f0);
        *wort_temperature_words = __aeabi_f2d_result;
        right = __aeabi_i2d(target_fahrenheit);
        step_filter_is_inserted_result = __aeabi_dcmple(__aeabi_f2d_result, right);
        if (step_filter_is_inserted_result != 0) {
            thermoblock_temperature_words = (double *)(controller + 0x1f8);
            step_filter_is_inserted_result = __aeabi_dcmplt(*thermoblock_temperature_words, 210.0);
            if (step_filter_is_inserted_result != 0) {
                pico_display_clear();
                pico_display_draw_titled_frame(program_title);
                pico_screen_preheating_steam(3);
                pico_display_flush();
                *(uint32_t *)(controller + 0x218) = 0x9999999a;
                *(uint32_t *)(controller + 0x21c) = 0x3fb99999;
                do {
                    pico_heat_controller_start(controller);
                    do {
                        step_filter_is_inserted_result =
                            __aeabi_dcmplt(*thermoblock_temperature_words, 210.0);
                        if ((step_filter_is_inserted_result == 0) ||
                            (step_filter_is_inserted_result =
                                 __aeabi_dcmple(*wort_temperature_words, right),
                             step_filter_is_inserted_result == 0))
                            goto branch_1;
                        now_ms = millis();
                        if ((&g_brewing_unknown_05 < (uint8_t *)(now_ms - phase_started_ms)) &&
                            (step_filter_is_inserted_result =
                                 __aeabi_dcmplt(*thermoblock_temperature_words, 90.0),
                             step_filter_is_inserted_result != 0)) {
                            pico_heat_controller_stop(controller);
                            pico_actuators_off_except_drain();
                            pico_report_error(0x13, 1, 1);
                            *(uint8_t *)(controller + 0x61) = 3;
                            *(uint8_t *)(controller + 0x62) = 6;
                            return;
                        }
                        now_ms = millis();
                        if ((&g_brewing_unknown_05 < (uint8_t *)(now_ms - phase_started_ms)) &&
                            (step_filter_is_inserted_result =
                                 pico_encoder_take_click((PicoEncoder *)&g_encoder),
                             step_filter_is_inserted_result != 0))
                            goto branch_1;
                        step_filter_is_inserted_result = pico_step_filter_is_inserted();
                    } while (step_filter_is_inserted_result != 0);
                    pico_heat_controller_stop(controller);
                    pico_steam_and_drain_pumps_off();
                    pico_display_clear();
                    pico_prompt_insert_filter_with_alarm();
                    pico_display_clear();
                    pico_controller_start_circulation();
                    pico_display_clear();
                    pico_display_draw_titled_frame(program_title);
                    pico_screen_preheating_steam(3);
                    pico_display_flush();
                } while (true);
            }
        }
        do {
            phase_started_ms = millis();
            do {
                now_ms = millis();
                if (&g_brewing_unknown_01 < (uint8_t *)(now_ms - phase_started_ms))
                    goto branch_1;
                step_filter_is_inserted_result = pico_step_filter_is_inserted();
            } while (step_filter_is_inserted_result != 0);
            pico_steam_and_drain_pumps_off();
            pico_prompt_insert_filter_with_alarm();
            pico_controller_start_circulation();
            pico_display_clear();
            pico_screen_changing_step();
            pico_display_flush();
        } while (true);
    }
    step_filter_is_inserted_result = 5;
branch_2:
    pico_controller_set_brew_phase(controller, step_filter_is_inserted_result);
    return;
branch_1:
    temperature_read_pin25_result = pico_temperature_read_pin25();
    __aeabi_f2d_result = __aeabi_f2d(temperature_read_pin25_result);
    *wort_temperature_words = __aeabi_f2d_result;
    right_00 = __aeabi_i2d(target_fahrenheit + 5);
    __aeabi_f2d_result = *wort_temperature_words;
    step_filter_is_inserted_result = __aeabi_dcmpgt(*wort_temperature_words, right_00);
    if (step_filter_is_inserted_result == 0) {
        step_filter_is_inserted_result = __aeabi_dcmplt(__aeabi_f2d_result, right);
        if (step_filter_is_inserted_result == 0) {
            step_filter_is_inserted_result = 1;
        } else {
            step_filter_is_inserted_result = 0;
        }
    } else {
        step_filter_is_inserted_result = 2;
    }
    goto branch_2;
}

/* Heats thermoblock and flushes each compartment using descending thermal thresholds.
 * Firmware address: 0x2000da08. */
void pico_heat_flush_compartments(int steam_enabled, uint32_t argument_2,
                                  uint32_t target_fahrenheit_low, uint32_t target_fahrenheit_high)
{
    uint32_t value_1;
    uint32_t value_2;
    int __aeabi_d2iz_result;
    int __aeabi_d2iz_result_2;
    char *itoa_result;
    uint32_t heat_started_ms;
    uint32_t now_ms;
    uint32_t value_8;
    uint32_t compartment_id;
    double __aeabi_i2d_result;
    double right;
    char number_text[20];
    char temperature_label[28];

    pico_drain_pump_on();
    compartment_id = 1;
    do {
        pico_display_clear();
        pico_ui_draw_header("RV Pressure Test");
        pico_display_center_text(4, "Moving Squid");
        pico_display_flush();
        pico_stepper_move_to_compartment(compartment_id & 0xff);
        pico_display_valve_position(3, compartment_id);
        pico_display_clear_row(4);
        libc_strcpy(temperature_label, "Heating to");
        __aeabi_d2iz_result =
            __aeabi_d2iz((double)pico_join_u32_u32(target_fahrenheit_high, target_fahrenheit_low));
        __aeabi_i2d_result = __aeabi_i2d(__aeabi_d2iz_result);
        __aeabi_i2d_result = __aeabi_dsub(__aeabi_i2d_result, 20.0);
        __aeabi_d2iz_result_2 = __aeabi_d2iz(__aeabi_i2d_result);
        itoa_result = itoa(__aeabi_d2iz_result_2, number_text, 10);
        libc_strcat(temperature_label, itoa_result);
        pico_display_center_text(4, temperature_label);
        heat_started_ms = millis();
        value_2 = g_steam_temperature_fahrenheit_high;
        value_1 = g_steam_temperature_fahrenheit_low;
        __aeabi_i2d_result = __aeabi_dsub(
            (double)pico_join_u32_u32(target_fahrenheit_high, target_fahrenheit_low), 20.0);
        __aeabi_d2iz_result_2 =
            __aeabi_dcmplt((double)pico_join_u32_u32(value_2, value_1), __aeabi_i2d_result);
        if (__aeabi_d2iz_result_2 != 0) {
            pico_heater_on();
            while (__aeabi_d2iz_result_2 =
                       __aeabi_dcmplt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                                g_steam_temperature_fahrenheit_low),
                                      __aeabi_i2d_result),
                   __aeabi_d2iz_result_2 != 0) {
                now_ms = millis();
                if ((&g_brewing_unknown_05 < (uint8_t *)(now_ms - heat_started_ms)) &&
                    (__aeabi_d2iz_result_2 = __aeabi_dcmplt(
                         (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                   g_steam_temperature_fahrenheit_low),
                         90.0),
                     __aeabi_d2iz_result_2 != 0)) {
                    pico_actuators_off_except_drain();
                    pico_report_error(0x13, 0, 0);
                    return;
                }
                pico_display_temperature_columns(
                    5, 0, g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_high,
                    (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                              g_steam_temperature_fahrenheit_low),
                    0.0);
                pico_display_flush();
            }
            pico_heater_off();
        }
        pico_display_clear_row(4);
        libc_strcpy(temperature_label, "Heating to");
        itoa_result = itoa(__aeabi_d2iz_result, number_text, 10);
        libc_strcat(temperature_label, itoa_result);
        pico_display_center_text(4, temperature_label);
        heat_started_ms = millis();
        while ((__aeabi_d2iz_result = __aeabi_dcmplt(
                    (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                              g_steam_temperature_fahrenheit_low),
                    (double)pico_join_u32_u32(target_fahrenheit_high, target_fahrenheit_low)),
                __aeabi_d2iz_result != 0 &&
                    (now_ms = millis(),
                     (uint8_t *)(now_ms - heat_started_ms) < &g_brewing_unknown_02))) {
            pico_display_temperature_columns(
                5, 0, g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_high,
                (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                          g_steam_temperature_fahrenheit_low),
                0.0);
            pico_display_flush();
        }
        pico_display_clear_row(4);
        pico_display_center_text(4, "SHUTTLE ON");
        if (steam_enabled != 0) {
            pico_steam_pump_on();
        }
        pico_shuttle_pump_on();
        millis();
        __aeabi_d2iz_result = 0;
        right = (double)pico_join_u32_u32(target_fahrenheit_high, target_fahrenheit_low);
        while (true) {
            millis();
            while (__aeabi_d2iz_result_2 =
                       __aeabi_dcmpgt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                                g_steam_temperature_fahrenheit_low),
                                      right),
                   __aeabi_d2iz_result_2 != 0) {
                pico_display_temperature_columns(
                    5, 0, g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_high,
                    (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                              g_steam_temperature_fahrenheit_low),
                    0.0);
                pico_display_flush();
            }
            __aeabi_d2iz_result = __aeabi_d2iz_result + 1;
            millis();
            if (__aeabi_d2iz_result == 4)
                break;
            if (__aeabi_d2iz_result == 2) {
                right = __aeabi_i2d_result;
            } else {
                if (__aeabi_d2iz_result == 3) {
                    value_8 = 0x403e0000;
                } else {
                    value_8 = 0x40240000;
                }
                right = __aeabi_dsub(
                    (double)pico_join_u32_u32(target_fahrenheit_high, target_fahrenheit_low),
                    (double)((uint64_t)value_8 << 0x20));
            }
        }
        compartment_id = compartment_id + 1;
        pico_shuttle_pump_off();
        pico_steam_pump_off();
    } while (compartment_id != 7);
    pico_drain_pump_off();
    pico_display_clear();
    pico_restore_idle_hardware();
    return;
}

/* Rinse setup, timed drain and compartment vacuum, state reporting and completion instructions.
 * Firmware address: 0x2000deec. */
void pico_run_rinse_cycle(uint32_t argument_1)
{
    char *title;
    int ui_run_instruction_sequence_result;
    uint32_t now_ms_2;
    uint32_t now_ms_3;
    uint32_t now_ms;
    uint32_t value_5;

    pico_display_clear();
    g_device_state = 6;
    value_5 = (uint32_t)g_legacy_compartment_mapping;
    title = pico_title_label(1, (uint32_t)g_language_as_uint8_t);
    ui_run_instruction_sequence_result =
        pico_ui_run_instruction_sequence((uint32_t *)&g_brewing_unknown_11, 2, 0, 0, title, '\0');
    if (ui_run_instruction_sequence_result == 0x17) {
        pico_ui_wait_for_filter_insertion();
        pico_ui_draw_instruction_with_header(0x17, (uint32_t)g_language_as_uint8_t, title);
        pico_api_report_device_state();
        now_ms_2 = millis();
        pico_drain_pump_on();
        pico_stepper_move_to_compartment(value_5);
        while (now_ms_3 = millis(), (uint8_t *)(now_ms_3 - now_ms_2) < &g_brewing_unknown_08) {
            now_ms_3 = millis();
            ui_run_instruction_sequence_result = pico_pause_outputs_for_missing_filter(0, 1, 0);
            if (ui_run_instruction_sequence_result != 0) {
                pico_ui_draw_instruction_with_header(0x17, (uint32_t)g_language_as_uint8_t, title);
                now_ms = millis();
                now_ms_2 = (now_ms_2 - now_ms_3) + now_ms;
            }
            pico_display_update_timer(5, now_ms_2, 0x3c);
        }
        pico_drain_pump_off();
        pico_steam_pump_off();
        pico_vacuum_compartments(argument_1, value_5 + 1 & 0xff, value_5);
        g_device_state = 2;
        pico_api_report_device_state();
        pico_ui_run_instruction_screen(0x18, 0, 0, title, '\0');
    } else {
        pico_log_serial("W", "RCTL", "Rinse setup aborted by user");
        g_device_state = 2;
    }
    return;
}

/* Runs drain/shuttle circulation120seconds with compartment changes, compares thermoblock cooling;
 * pointer ABI remains incomplete. Firmware address: 0x2000ed9c. */
bool pico_first_rinse_cool_and_flush(void)
{
    uint32_t value_1;
    uint32_t value_2;
    uint32_t now_ms_4;
    uint32_t now_ms_3;
    int pause_outputs_for_missing_filter_result;
    uint32_t now_ms;
    uint8_t *unresolved_input_r3;
    uint32_t now_ms_2;
    double right;

    value_2 = g_steam_temperature_fahrenheit_high;
    value_1 = g_steam_temperature_fahrenheit_low;
    pico_ui_first_rinse_step(5);
    pico_steam_pump_off();
    pico_shuttle_pump_on();
    pico_drain_pump_on();
    now_ms_4 = millis();
    now_ms_2 = now_ms_4;
    while (now_ms_3 = millis(), (uint8_t *)(now_ms_3 - now_ms_4) < &g_brewing_unknown_09) {
        now_ms_3 = millis();
        pause_outputs_for_missing_filter_result = pico_pause_outputs_for_missing_filter(0, 1, 1);
        if (pause_outputs_for_missing_filter_result != 0) {
            pico_ui_first_rinse_step(5);
            now_ms = millis();
            now_ms_4 = (now_ms_4 - now_ms_3) + now_ms;
            now_ms = millis();
            now_ms_2 = (now_ms_2 - now_ms_3) + now_ms;
        }
        now_ms_3 = millis();
        if (&g_brewing_unknown_03 < (uint8_t *)(now_ms_3 - now_ms_2)) {
            pico_stepper_move_to_compartment((uint32_t)*unresolved_input_r3);
            now_ms_2 = millis();
            pause_outputs_for_missing_filter_result = *(int *)unresolved_input_r3 + 1;
            if (6 < pause_outputs_for_missing_filter_result) {
                pause_outputs_for_missing_filter_result = 1;
            }
            *(int *)unresolved_input_r3 = pause_outputs_for_missing_filter_result;
            pico_display_update_timer(6, now_ms_4, 0x78);
        }
        pico_display_update_timer(6, now_ms_4, 0x78);
    }
    right = __aeabi_dsub((double)pico_join_u32_u32(value_2, value_1), 85.0);
    pause_outputs_for_missing_filter_result =
        __aeabi_dcmplt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                 g_steam_temperature_fahrenheit_low),
                       right);
    return pause_outputs_for_missing_filter_result != 0;
}

/* Enables heater to thermoblock threshold, detects30-second heating failure, turns off heater and
 * waits30seconds. Firmware address: 0x2000ee78. */
uint32_t pico_first_rinse_heat_test(void)
{
    uint32_t now_ms;
    int __aeabi_dcmplt_result;
    uint32_t now_ms_2;
    uint32_t value_4;

    millis();
    pico_heater_on();
    pico_display_flush();
    now_ms = millis();
    do {
        __aeabi_dcmplt_result =
            __aeabi_dcmplt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                     g_steam_temperature_fahrenheit_low),
                           230.0);
        if (__aeabi_dcmplt_result == 0) {
            value_4 = 1;
        branch_1:
            pico_heater_off();
            now_ms = millis();
            pico_ui_first_rinse_step(3);
            while (now_ms_2 = millis(), (uint8_t *)(now_ms_2 - now_ms) < &g_brewing_unknown_07) {
                pico_display_update_timer(6, now_ms, 0x1e);
            }
            return value_4;
        }
        now_ms_2 = millis();
        if ((&g_brewing_unknown_07 < (uint8_t *)(now_ms_2 - now_ms)) &&
            (__aeabi_dcmplt_result =
                 __aeabi_dcmplt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                          g_steam_temperature_fahrenheit_low),
                                130.0),
             __aeabi_dcmplt_result != 0)) {
            value_4 = 0;
            goto branch_1;
        }
        pico_display_update_timer(6, now_ms, 0);
    } while (true);
}

/* Draws drain-filter screen, runs drain pump until user Stop, then disables drain.
 * Firmware address: 0x2000ef2c. */
void pico_drain_step_filter(void)
{
    pico_display_clear();
    pico_ui_draw_drain_filter();
    pico_drain_pump_on();
    pico_ui_wait_for_stop();
    pico_drain_pump_off();
    return;
}

/* Complete first-rinse orchestration: heat/steam/shuttle tests, timed circulation, cooling and
 * error reporting. Firmware address: 0x2000ef48. */
void pico_run_first_rinse(void)
{
    bool first_rinse_cool_and_flush_result;
    int step_filter_is_inserted_result;
    uint32_t now_ms_2;
    uint32_t now_ms;
    int pause_outputs_for_missing_filter_result;
    uint32_t now_ms_3;
    uint32_t now_ms_4;
    uint32_t value;

    pico_display_clear();
    pico_ui_first_rinse_step(0);
    step_filter_is_inserted_result = pico_step_filter_is_inserted();
    if (step_filter_is_inserted_result == 0) {
        pico_ui_first_rinse_step(1);
        pico_wait_for_step_filter();
    }
    pico_ui_first_rinse_step(2);
    g_device_state = 6;
    pico_api_report_device_state();
    value = 2;
    pico_stepper_move_to_compartment(1);
    step_filter_is_inserted_result = pico_first_rinse_heat_test();
    pico_ui_first_rinse_step(4);
    pico_steam_pump_on();
    pico_shuttle_pump_on();
    pico_drain_pump_on();
    pico_display_flush();
    now_ms_2 = millis();
    now_ms_4 = now_ms_2;
    while (now_ms = millis(), (uint8_t *)(now_ms - now_ms_2) < &g_brewing_unknown_10) {
        now_ms = millis();
        pause_outputs_for_missing_filter_result = pico_pause_outputs_for_missing_filter(1, 0, 1);
        if (pause_outputs_for_missing_filter_result != 0) {
            pico_ui_first_rinse_step(4);
            now_ms_3 = millis();
            now_ms_2 = (now_ms_2 - now_ms) + now_ms_3;
            now_ms_3 = millis();
            now_ms_4 = (now_ms_4 - now_ms) + now_ms_3;
        }
        pause_outputs_for_missing_filter_result =
            __aeabi_dcmplt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                     g_steam_temperature_fahrenheit_low),
                           225.0);
        if (pause_outputs_for_missing_filter_result == 0) {
            pause_outputs_for_missing_filter_result =
                __aeabi_dcmpgt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                         g_steam_temperature_fahrenheit_low),
                               230.0);
            if (pause_outputs_for_missing_filter_result != 0) {
                pico_heater_off();
            }
        } else {
            pico_heater_on();
        }
        now_ms = millis();
        if (&g_brewing_unknown_03 < (uint8_t *)(now_ms - now_ms_4)) {
            pico_heater_off();
            pico_steam_pump_off();
            pico_shuttle_pump_off();
            pico_stepper_move_to_compartment(value & 0xff);
            pico_steam_pump_on();
            pico_shuttle_pump_on();
            now_ms_4 = millis();
            value = value + 1;
            if (6 < (int)value) {
                value = 1;
            }
            pico_display_update_timer(6, now_ms_2, 0x82);
        }
        pico_display_temperature_summary(
            g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_high,
            g_steam_temperature_fahrenheit_low, g_steam_temperature_fahrenheit_high, 0.0, 5);
        pico_display_update_timer(6, now_ms_2, 0x82);
    }
    pico_heater_off();
    first_rinse_cool_and_flush_result = pico_first_rinse_cool_and_flush();
    if (!first_rinse_cool_and_flush_result) {
        pico_shuttle_pump_off();
        pico_steam_pump_off();
        pico_drain_pump_off();
        pico_first_rinse_heat_test();
        first_rinse_cool_and_flush_result = pico_first_rinse_cool_and_flush();
    }
    pico_screen_ending_first_rinse();
    pico_shuttle_pump_off();
    pico_drain_pump_off();
    pico_vacuum_compartments(&g_active_program_context_as_uint32_t, value & 0xff, 1);
    if (first_rinse_cool_and_flush_result == false) {
        pico_ui_first_rinse_step(6);
        pico_api_report_error(0x16);
        if (step_filter_is_inserted_result == 0) {
            pico_api_report_error(0x13);
        }
        pico_ui_wait_for_stop();
        if (step_filter_is_inserted_result != 0)
            goto branch_1;
    } else if (step_filter_is_inserted_result != 0) {
        pico_ui_first_rinse_step(8);
        goto branch_1;
    }
    pico_ui_first_rinse_step(7);
    pico_api_report_error(0x13);
    pico_ui_wait_for_stop();
branch_1:
    pico_display_flush();
    pico_stepper_move_home();
    g_device_state = 2;
    pico_api_report_device_state();
    pico_ui_wait_for_stop();
    return;
}

/* Executes rack-beer instruction stages0..8 with drain-pump on/off around transfer stages.
 * Firmware address: 0x2000f800. */
void pico_run_rack_beer(void)
{
    pico_display_clear();
    pico_ui_rack_beer_step(0);
    pico_drain_pump_on();
    pico_ui_rack_beer_step(1);
    pico_drain_pump_off();
    pico_ui_rack_beer_step(2);
    pico_ui_rack_beer_step(3);
    pico_ui_rack_beer_step(4);
    pico_ui_rack_beer_step(5);
    pico_drain_pump_on();
    pico_ui_rack_beer_step(6);
    pico_drain_pump_off();
    pico_ui_rack_beer_step(7);
    pico_drain_pump_on();
    pico_ui_rack_beer_step(8);
    pico_drain_pump_off();
    return;
}

/* Descaling program and thermal/flush sequence.
 * Firmware address: 0x20011420. */
void pico_run_descale_program(void)
{
    uint32_t value_1;
    uint32_t value_2;
    bool ui_descale_step_result;
    int step_filter_is_inserted_result_2;
    int step_filter_is_inserted_result;

    pico_ui_descale_step(0);
    pico_display_clear();
    pico_ui_draw_descale_heading();
    while (step_filter_is_inserted_result_2 = pico_step_filter_is_inserted(),
           step_filter_is_inserted_result_2 == 0) {
        pico_ui_descale_step(1);
    }
    pico_ui_descale_step(6);
    step_filter_is_inserted_result_2 = 0;
    pico_stepper_move_to_compartment(1);
    pico_reprime_steam_pump(0x2002d2f8, "PRIME", "Descaling", 0, '\0');
    pico_actuators_all_off();
    pico_ui_descale_step(2);
    do {
        millis();
        pico_ui_descale_step(3);
        while (true) {
            do {
                pico_shuttle_pump_on();
                step_filter_is_inserted_result = pico_step_filter_is_inserted();
                if (step_filter_is_inserted_result == 0) {
                    millis();
                    pico_shuttle_pump_off();
                    pico_heater_off();
                    while (step_filter_is_inserted_result = pico_step_filter_is_inserted(),
                           step_filter_is_inserted_result == 0) {
                        pico_ui_descale_step(1);
                    }
                    pico_ui_descale_step(3);
                    millis();
                }
                pico_display_temperature_columns(
                    5, 0, g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_high,
                    (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                              g_steam_temperature_fahrenheit_low),
                    0.0);
                pico_display_flush();
                value_2 = g_steam_temperature_fahrenheit_high;
                value_1 = g_steam_temperature_fahrenheit_low;
                step_filter_is_inserted_result =
                    __aeabi_dcmpgt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                             g_steam_temperature_fahrenheit_low),
                                   130.0);
                if (step_filter_is_inserted_result == 0) {
                    step_filter_is_inserted_result =
                        __aeabi_dcmplt((double)pico_join_u32_u32(value_2, value_1), 120.0);
                    if (step_filter_is_inserted_result != 0) {
                        pico_heater_on();
                    }
                } else {
                    pico_heater_off();
                }
                step_filter_is_inserted_result = pico_encoder_take_click((PicoEncoder *)&g_encoder);
            } while (step_filter_is_inserted_result == 0);
            millis();
            pico_shuttle_pump_off();
            pico_heater_off();
            ui_descale_step_result = pico_ui_descale_step(4);
            if (ui_descale_step_result)
                break;
            pico_ui_descale_step(3);
            millis();
        }
        millis();
        pico_actuators_all_off();
        millis();
        if (step_filter_is_inserted_result_2 == 0) {
            pico_ui_descale_step(5);
        }
        step_filter_is_inserted_result_2 = step_filter_is_inserted_result_2 + 1;
    } while (step_filter_is_inserted_result_2 != 2);
    pico_restore_idle_hardware();
    pico_actuators_all_off();
    g_device_state = 2;
    pico_api_report_device_state();
    return;
}

/* Runs heating/holding/draining phases, controller status recovery and action menu.
 * Firmware address: 0x20011590. */
int pico_run_brew_step(void *controller, uint32_t target_fahrenheit, uint32_t active_minutes,
                       uint32_t hold_minutes, uint8_t compartment_id, char *program_title,
                       char *step_name, uint8_t logging_enabled, int remaining_minutes,
                       int step_index, int session_type)
{
    double *thermoblock_target;
    bool condition_or_confirmed;
    bool step_complete;
    uint32_t selected_value;
    float hold_seconds;
    uint32_t phase_started_ms;
    uint32_t now_ms;
    int operation_result;
    uint8_t *drain_elapsed_ms;
    uint32_t resumed_ms;
    uint32_t original_compartment_id;
    int phase_or_total_minutes;
    char *action_message;
    uint8_t *drain_interval_ms;
    char status_or_drain_subphase;
    uint8_t next_controller_status;
    uint32_t thermoblock_target_low;
    uint32_t thermoblock_target_high;
    double __aeabi_ui2d_result;
    double __aeabi_i2d_result;
    uint32_t last_log_ms;
    uint32_t last_still_poll_ms;
    char log_message[132];

    original_compartment_id = (uint32_t)compartment_id;
    pico_menu_populate_in_brew_options();
    *(uint8_t *)((int)controller + 0x60) = compartment_id;
    __aeabi_ui2d_result = __aeabi_ui2d(target_fahrenheit);
    *(double *)((int)controller + 0x1d0) = __aeabi_ui2d_result;
    *(int *)((int)controller + 0x1a8) = remaining_minutes * 0x3c;
    libc_snprintf(log_message, 0x80, "%s %dF %d min %d \"%s\"", "runstep", target_fahrenheit,
                  active_minutes, original_compartment_id, step_name);
    pico_log_serial("I", "BREW", log_message);
    pico_recovery_save_current_session(active_minutes, hold_minutes, step_index, 0, '\x01',
                                       *(uint8_t *)((int)controller + 0x1a0));
    if (*(char *)((int)controller + 0x60) == '\0') {
        selected_value = 2;
    branch_1:
        *(char *)((int)controller + 0x60) = (char)selected_value;
        pico_stepper_move_to_compartment(selected_value);
        phase_or_total_minutes = 6;
    } else {
        if (*(char *)((int)controller + 0x60) == '\a') {
            selected_value = 1;
            goto branch_1;
        }
        if ((active_minutes != 0xffffffff) || ((session_type != 2 && (session_type != 4)))) {
            pico_stepper_move_to_compartment(original_compartment_id);
            pico_prepare_brew_step_temperature((int)controller, target_fahrenheit, program_title);
            goto branch_2;
        }
        pico_stepper_move_to_compartment(1);
        *(uint8_t *)((int)controller + 0x60) = 1;
        phase_or_total_minutes = 8;
    }
    pico_controller_set_brew_phase((int)controller, phase_or_total_minutes);
branch_2:
    if (((*(char *)((int)controller + 0x60) == '\x01') && (hold_minutes != 0)) &&
        (session_type == 0)) {
        hold_seconds = __aeabi_i2f(hold_minutes * 0x3c);
        __aeabi_ui2d_result = __aeabi_f2d(hold_seconds);
        __aeabi_ui2d_result = __aeabi_ddiv(__aeabi_ui2d_result, 45.0);
        __aeabi_ui2d_result = libm_ceil(__aeabi_ui2d_result);
        __aeabi_ui2d_result = __aeabi_dmul(__aeabi_ui2d_result, 0.5);
        __aeabi_ui2d_result = libm_ceil(__aeabi_ui2d_result);
        __aeabi_i2d_result = __aeabi_i2d(hold_minutes);
        __aeabi_ui2d_result = __aeabi_dadd(__aeabi_i2d_result, __aeabi_ui2d_result);
        hold_minutes = __aeabi_d2iz(__aeabi_ui2d_result);
    }
    pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
    phase_started_ms = millis();
    phase_or_total_minutes = active_minutes + hold_minutes;
    last_still_poll_ms = 0;
    last_log_ms = 0;
    do {
        pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
        now_ms = millis();
        if (&g_brewing_unknown_06 < (uint8_t *)(now_ms - last_log_ms)) {
            if (logging_enabled != '\0') {
                pico_wifi_log_progress(step_name);
            }
            last_log_ms = millis();
        }
        if ((session_type == 3) && (now_ms = millis(), 2000 < now_ms - last_still_poll_ms)) {
            operation_result = pico_still_read_status(controller);
            status_or_drain_subphase = *(char *)((int)controller + 0x61);
            if (operation_result == 0) {
                if ((status_or_drain_subphase != '\b') &&
                    (*(int *)((int)controller + 0x58) <= *(int *)((int)controller + 4))) {
                    pico_heat_controller_stop((int)controller);
                    next_controller_status = 8;
                branch_3:
                    *(uint8_t *)((int)controller + 0x61) = next_controller_status;
                }
            } else {
                if (status_or_drain_subphase != '\n') {
                    step_complete = pico_still_check_pressure_limit((int)controller);
                    if (step_complete) {
                        pico_heat_controller_stop((int)controller);
                        next_controller_status = 10;
                        goto branch_3;
                    }
                    if (status_or_drain_subphase == '\t')
                        goto branch_4;
                }
                operation_result = __aeabi_dcmpgt(*(double *)((int)controller + 0x18),
                                                  *(double *)((int)controller + 0x40));
                if (operation_result != 0) {
                    pico_heat_controller_stop((int)controller);
                    next_controller_status = 9;
                    goto branch_3;
                }
            }
        branch_4:
            last_still_poll_ms = millis();
        }
        thermoblock_target = (double *)((int)controller + 0x1d8);
        thermoblock_target_low = *(uint32_t *)thermoblock_target;
        thermoblock_target_high = *(uint32_t *)((int)controller + 0x1dc);
        operation_result = __aeabi_dcmpeq(*thermoblock_target, 300.0);
        if ((operation_result == 0) && (0xb3 < target_fahrenheit)) {
            thermoblock_target_high = 0x40754000;
            thermoblock_target_low = 0;
        }
        *(uint32_t *)thermoblock_target = thermoblock_target_low;
        *(uint32_t *)((int)controller + 0x1dc) = thermoblock_target_high;
        switch (*(uint8_t *)((int)controller + 0x62)) {
        case 0:
            pico_shuttle_update_calibration((int)controller);
        case 2:
            __aeabi_i2d_result = *(double *)((int)controller + 0x1f0);
            __aeabi_ui2d_result = *(double *)((int)controller + 0x1f0);
            if (*(char *)((int)controller + 0x62) == '\0') {
                __aeabi_i2d_result = __aeabi_ui2d(target_fahrenheit);
                operation_result = __aeabi_dcmpge(__aeabi_ui2d_result, __aeabi_i2d_result);
            } else {
                __aeabi_ui2d_result = __aeabi_i2d(target_fahrenheit + 5);
                operation_result = __aeabi_dcmple(__aeabi_i2d_result, __aeabi_ui2d_result);
            }
            if (operation_result == 0) {
                step_complete = false;
            } else {
                pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
                if ((int)active_minutes < 1) {
                    if (0 < (int)hold_minutes) {
                        operation_result = 3;
                        goto branch_5;
                    }
                    step_complete = true;
                } else {
                    operation_result = 1;
                branch_5:
                    pico_controller_set_brew_phase((int)controller, operation_result);
                    step_complete = false;
                }
                pico_display_clear_row_from_column(4, 9);
                pico_display_clear_row_from_column(5, 9);
                phase_started_ms = millis();
            }
            if (*(int *)((int)controller + 0x1a8) / 0x3c + 1 < phase_or_total_minutes) {
                *(int *)((int)controller + 0x1a8) = phase_or_total_minutes * 0x3c;
            }
            goto branch_6;
        case 1:
        case 5:
        case 7:
            operation_result = *(int *)((int)controller + 0x1a8);
            now_ms = millis();
            if (operation_result / 0x3c + 1U <
                phase_or_total_minutes - (now_ms - phase_started_ms) / 60000) {
                now_ms = millis();
                *(uint32_t *)((int)controller + 0x1a8) =
                    (phase_or_total_minutes - (now_ms - phase_started_ms) / 60000) * 0x3c;
            }
            pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
            now_ms = millis();
            pico_recovery_save_current_session(active_minutes - (now_ms - phase_started_ms) / 60000,
                                               hold_minutes, step_index, 0, '\0',
                                               *(uint8_t *)((int)controller + 0x1a0));
            now_ms = millis();
            if ((now_ms - phase_started_ms) / 60000 < active_minutes) {
            branch_7:
                step_complete = false;
            } else {
                if (0 < (int)hold_minutes) {
                    pico_controller_set_brew_phase((int)controller, 3);
                    pico_recovery_save_current_session(0, hold_minutes, step_index, 0, '\x01',
                                                       *(uint8_t *)((int)controller + 0x1a0));
                    phase_started_ms = millis();
                    goto branch_7;
                }
                step_complete = true;
            }
            goto branch_6;
        case 3:
            if ((original_compartment_id != 1) || (session_type != 0))
                goto switchD_20011956_default;
            switch (*(uint8_t *)((int)controller + 0xa8)) {
            case 0:
                now_ms = millis();
                selected_value = now_ms - *(int *)((int)controller + 0x210);
                condition_or_confirmed = 1999 < selected_value;
                step_complete = selected_value == 2000;
                break;
            case 1:
                now_ms = millis();
                drain_interval_ms = &g_brewing_unknown_07;
                drain_elapsed_ms = (uint8_t *)(now_ms - *(int *)((int)controller + 0x210));
                goto branch_8;
            case 2:
            case 4:
                now_ms = millis();
                drain_elapsed_ms = (uint8_t *)(now_ms - *(int *)((int)controller + 0x210));
                condition_or_confirmed = &g_brewing_unknown_04 < drain_elapsed_ms;
                step_complete = drain_elapsed_ms == &g_brewing_unknown_05;
                break;
            case 3:
                now_ms = millis();
                drain_interval_ms = &g_session_unknown_01;
                drain_elapsed_ms = (uint8_t *)(now_ms - *(int *)((int)controller + 0x210));
            branch_8:
                if (drain_interval_ms < drain_elapsed_ms) {
                    pico_drain_pump_on();
                    goto branch_9;
                }
                goto switchD_20011956_default;
            case 5:
                now_ms = millis();
                if (&g_session_unknown_01 <
                    (uint8_t *)(now_ms - *(int *)((int)controller + 0x210))) {
                    pico_drain_pump_on();
                    now_ms = millis();
                    *(uint32_t *)((int)controller + 0x210) = now_ms;
                    *(char *)((int)controller + 0xa8) = *(char *)((int)controller + 0xa8) + '\x01';
                }
            case 6:
                now_ms = millis();
                if (&g_brewing_unknown_05 <
                    (uint8_t *)(now_ms - *(int *)((int)controller + 0x210))) {
                    pico_drain_pump_off();
                    now_ms = millis();
                    status_or_drain_subphase = '\0';
                    *(uint32_t *)((int)controller + 0x210) = now_ms;
                    goto branch_10;
                }
            default:
                goto switchD_20011956_default;
            }
            if (condition_or_confirmed && !step_complete) {
                pico_drain_pump_off();
            branch_9:
                now_ms = millis();
                *(uint32_t *)((int)controller + 0x210) = now_ms;
                status_or_drain_subphase = *(char *)((int)controller + 0xa8) + '\x01';
            branch_10:
                *(char *)((int)controller + 0xa8) = status_or_drain_subphase;
            }
        switchD_20011956_default:
            now_ms = millis();
            pico_recovery_save_current_session(
                0, hold_minutes - (now_ms - phase_started_ms) / 60000, step_index, 0, '\0',
                *(uint8_t *)((int)controller + 0x1a0));
            operation_result = *(int *)((int)controller + 0x1a8);
            now_ms = millis();
            if (operation_result / 0x3c + 1U < hold_minutes - (now_ms - phase_started_ms) / 60000) {
                now_ms = millis();
                *(uint32_t *)((int)controller + 0x1a8) =
                    (hold_minutes - (now_ms - phase_started_ms) / 60000) * 0x3c;
            }
            pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
            now_ms = millis();
            step_complete = hold_minutes <= (now_ms - phase_started_ms) / 60000;
            goto branch_6;
        default:
            pico_prepare_brew_step_temperature((int)controller, target_fahrenheit, program_title);
            break;
        case 6:
            operation_result = __aeabi_dcmplt(*(double *)((int)controller + 0x1f8), 210.9);
            if ((operation_result != 0) ||
                (now_ms = millis(),
                 &g_brewing_unknown_07 < (uint8_t *)(now_ms - phase_started_ms))) {
                pico_controller_set_brew_phase((int)controller, 7);
            }
            pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
            break;
        case 8:
            selected_value = pico_menu_populate_post_brew();
            selected_value =
                pico_menu_choose_with_timeout(7, selected_value, 0, 1, '\x01', '\x01', 0x14);
            if (selected_value != 1)
                goto switchD_20011c68_caseD_4;
        }
        step_complete = false;
    branch_6:
        switch (*(uint8_t *)((int)controller + 0x61)) {
        case 1:
            operation_result = __aeabi_dcmplt(*(double *)((int)controller + 0x1f0), 215.0);
            if (operation_result != 0) {
                __aeabi_ui2d_result = __aeabi_i2d(target_fahrenheit);
                operation_result =
                    __aeabi_dcmplt(*(double *)((int)controller + 0x1f0), __aeabi_ui2d_result);
                if ((operation_result != 0) || (target_fahrenheit == 0))
                    goto branch_11;
            }
            break;
        case 2:
            operation_result = __aeabi_dcmplt(*(double *)((int)controller + 0x1f8), 350.0);
            if (operation_result != 0)
                goto branch_11;
            break;
        case 3:
            pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
            pico_finish_brew_step((int)controller, program_title);
            pico_report_error(0xf, 1, 1);
            return 0;
        case 4:
            pico_controller_clear_status((int)controller);
            pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
            break;
        case 5:
        case 6:
        case 7:
            now_ms = millis();
            if ((*(uint8_t *)((int)controller + 0x61) == 5) &&
                (*(char *)((int)controller + 0x1a1) == '\0')) {
                *(uint8_t *)((int)controller + 0x1a1) = 1;
                pico_wifi_log_step("Re-Prime Steam Pump");
            branch_12:
                selected_value =
                    pico_reprime_steam_pump((int)controller, program_title, step_name, 1, '\x01');
                if (selected_value == 0) {
                    pico_wifi_log_step("Thermoblock Issue");
                    goto switchD_20011c68_caseD_4;
                }
                pico_wifi_log_step(step_name);
            branch_13:
                resumed_ms = millis();
                phase_started_ms = (phase_started_ms - now_ms) + resumed_ms;
                pico_display_clear();
                pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
                goto branch_11;
            }
            operation_result =
                pico_ui_confirm_thermal_recovery((uint32_t) * (uint8_t *)((int)controller + 0x61));
            if (operation_result != 0) {
                if (*(char *)((int)controller + 0x61) == '\x05') {
                    pico_wifi_log_step("Re-Prime Steam Pump");
                    goto branch_12;
                }
                goto branch_13;
            }
            goto branch_14;
        case 8:
            if (*(int *)((int)controller + 4) < *(int *)((int)controller + 0x58))
                goto branch_11;
            pico_ui_show_still_communication_failure();
            break;
        case 9:
            pico_ui_show_still_temperature_warning();
            goto branch_11;
        case 10:
            pico_ui_show_still_completion();
        branch_11:
            pico_controller_clear_status((int)controller);
        }
        now_ms = millis();
        operation_result = pico_handle_filter_removal();
        if (operation_result != 0) {
        branch_14:
            pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
            goto switchD_20011c68_caseD_4;
        }
        resumed_ms = millis();
        phase_started_ms = (phase_started_ms - now_ms) + resumed_ms;
        operation_result = pico_encoder_take_click((PicoEncoder *)&g_encoder);
        if (operation_result == 0)
            goto branch_15;
        Print_print_string(&global_serial0, "Encoder Press in Brew\n");
        selected_value = pico_menu_populate_in_brew_options();
        selected_value = pico_menu_choose_default_timeout(7, selected_value, 0, 1, '\0', '\x01');
        pico_assert_nonfatal((uint32_t)(g_step_action_ids != 0), "actions != NULL");
        pico_assert_nonfatal((uint32_t)((int)selected_value < g_step_action_count),
                             "0 <= index && index < actionsCount");
        if ((g_step_action_ids == 0) || (g_step_action_count <= (int)selected_value))
            goto switchD_20011c68_default;
        switch (*(uint32_t *)(g_step_action_ids + (short)selected_value * 4)) {
        case 1:
            action_message = "Selected 0: Continue";
            goto branch_16;
        case 2:
            pico_assert_nonfatal((uint32_t)(session_type == 3),
                                 "sessionType == SessionTypes::Still");
            if ((session_type == 3) == 0) {
            branch_17:
                pico_heat_controller_stop((int)controller);
                now_ms = millis();
                pico_steam_pump_off();
                pico_drain_step_filter();
                pico_controller_set_brew_phase((int)controller,
                                               (uint32_t) * (uint8_t *)((int)controller + 0x62));
                if ((*(uint8_t *)((int)controller + 0x62) & 0xfd) == 0) {
                    pico_display_clear();
                    pico_display_refresh_current_page();
                    delay(5000);
                }
                resumed_ms = millis();
                phase_started_ms = (phase_started_ms - now_ms) + resumed_ms;
                last_log_ms = 0;
            } else {
                Print_print_string(&global_serial0, "Selected 1: Still Fan State");
                pico_display_clear();
                pico_ui_still_fan_menu();
            }
            break;
        case 3:
            Print_print_string(&global_serial0, "Selected 1: Skip Step");
            pico_ui_draw_skip_step_confirmation();
            condition_or_confirmed = pico_ui_confirm_at_row(5);
            if (condition_or_confirmed)
                goto branch_18;
            break;
        case 4:
        switchD_20011c68_caseD_4:
            pico_finish_brew_step((int)controller, program_title);
            return 0;
        case 5:
            Print_print_string(&global_serial0, "Selected 3: Pause");
            now_ms = millis();
            pico_controller_pause_until_resume((int)controller);
            resumed_ms = millis();
            phase_started_ms = (resumed_ms + phase_started_ms) - now_ms;
            last_log_ms = 0;
            break;
        case 6:
            if (session_type != 3)
                goto branch_17;
            action_message = "Selected 2: BUG Drain -> Continue";
        branch_16:
            Print_print_string(&global_serial0, action_message);
            break;
        case 7:
        case 8:
        case 9:
            Print_print_string(&global_serial0, "Selected 4: Change ProView Display");
            pico_assert_nonfatal(
                (uint32_t)(session_type != 1),
                "sessionType == SessionTypes::Brewing || sessionType != SessionTypes::Cleaning");
            g_detailed_status = g_detailed_status ^ 1;
            break;
        case 10:
            selected_value = __aeabi_d2iz(*(double *)((int)controller + 0x1d0));
            selected_value = pico_get_temperature_input(0x12, g_session_title, selected_value);
            target_fahrenheit = selected_value & 0xff;
            __aeabi_ui2d_result = __aeabi_ui2d(target_fahrenheit);
            *(double *)((int)controller + 0x1d0) = __aeabi_ui2d_result;
            break;
        case 0xb:
            Print_print_string(&global_serial0, "Selected 5: Exit Session");
            pico_ui_draw_stop_confirmation();
            condition_or_confirmed = pico_ui_confirm_at_row(5);
            if (condition_or_confirmed)
                goto switchD_20011c68_caseD_4;
        }
    switchD_20011c68_default:
        pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
    branch_15:
    } while (!step_complete);
    pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
branch_18:
    pico_finish_brew_step((int)controller, program_title);
    return 1;
}

/* Iterates32-byte recipe step records and finalizes completed/canceled session.
 * Firmware address: 0x20011e68. */
uint32_t pico_run_recipe_steps(void *recipe, int logging_enabled)
{
    uint8_t recipe_step_count;
    char future_compartment;
    uint32_t initial_wort_temperature_low;
    uint32_t initial_wort_temperature_high;
    bool continue_selected;
    int step_index_or_setup_result;
    int remaining_minutes;
    char *step_record;
    uint32_t completion_result;
    char *step_name_or_event;
    char *future_step_record;
    int future_step_index_or_result;
    double __aeabi_i2d_result;
    double __aeabi_i2d_result_2;
    char step_name[20];
    uint8_t step_name_terminator;

    step_index_or_setup_result = pico_setup_instructions(logging_enabled, &g_screen_recipe_name);
    completion_result = 0;
    if (step_index_or_setup_result != 0) {
        step_index_or_setup_result = *(int *)((int)recipe + 0x22c);
        step_record = (char *)((int)recipe + step_index_or_setup_result * 0x20 + 0x30);
        step_name_or_event = step_record;
        for (; step_index_or_setup_result < (int)(uint32_t) * (uint8_t *)((int)recipe + 0x210);
             step_index_or_setup_result = step_index_or_setup_result + 1) {
            *(int *)((int)recipe + 0x22c) = step_index_or_setup_result;
            pico_screen_set_step((PicoScreenState *)&g_screen_recipe_name,
                                 (uint32_t)(uint8_t)step_record[0x16], step_name_or_event);
            if ((uint8_t)step_record[0x16] < 9) {
                if (step_record[0x16] == 8) {
                    pico_actuators_all_off();
                    continue_selected =
                        pico_execute_pause_custom_instructions(*(int *)(*(int *)recipe + 0x20));
                    if (!continue_selected) {
                        return 0;
                    }
                } else {
                    step_record[0x15] = '\0';
                    libc_strncpy(step_name, step_name_or_event, 0x14);
                    step_name_terminator = 0;
                    pico_screen_changing_step();
                    pico_display_flush();
                    if (logging_enabled != 0) {
                        pico_wifi_log_step(step_name_or_event);
                    }
                    pico_screen_setting_up_step(&g_screen_recipe_name);
                    pico_display_flush();
                    initial_wort_temperature_high = g_wort_temperature_fahrenheit_high;
                    initial_wort_temperature_low = g_wort_temperature_fahrenheit_low;
                    recipe_step_count = *(uint8_t *)((int)recipe + 0x210);
                    remaining_minutes = 0;
                    future_step_record = step_name_or_event;
                    for (future_step_index_or_result = step_index_or_setup_result;
                         future_step_index_or_result < (int)(uint32_t)recipe_step_count;
                         future_step_index_or_result = future_step_index_or_result + 1) {
                        remaining_minutes = remaining_minutes +
                                            *(int *)(future_step_record + 0x18) +
                                            *(int *)(future_step_record + 0x1c);
                        future_compartment = future_step_record[0x16];
                        if (future_compartment == '\x02') {
                            if (*(int *)(*(int *)recipe + 0x30) == 0) {
                                remaining_minutes = remaining_minutes + 8;
                            } else if (*(int *)(*(int *)recipe + 0x30) - 2U < 3) {
                                __aeabi_i2d_result = __aeabi_i2d(remaining_minutes);
                                __aeabi_i2d_result_2 =
                                    __aeabi_i2d((uint32_t)(uint8_t)future_step_record[0x17]);
                                __aeabi_i2d_result_2 = __aeabi_dsub(
                                    __aeabi_i2d_result_2,
                                    (double)pico_join_u32_u32(initial_wort_temperature_high,
                                                              initial_wort_temperature_low));
                                __aeabi_i2d_result_2 = __aeabi_ddiv(__aeabi_i2d_result_2, 10.0);
                                __aeabi_i2d_result_2 = __aeabi_dmul(__aeabi_i2d_result_2, 8.0);
                                __aeabi_i2d_result =
                                    __aeabi_dadd(__aeabi_i2d_result, __aeabi_i2d_result_2);
                                remaining_minutes = __aeabi_d2iz(__aeabi_i2d_result);
                            }
                        } else if (((future_compartment == '\x03') ||
                                    (future_compartment == '\x01')) &&
                                   (*(int *)(*(int *)recipe + 0x30) == 0)) {
                            remaining_minutes = remaining_minutes + 7;
                        }
                        future_step_record = future_step_record + 0x20;
                    }
                    future_step_index_or_result = pico_run_brew_step(
                        &g_controller, (uint32_t)(uint8_t)step_record[0x17],
                        *(uint32_t *)(step_record + 0x1c), *(uint32_t *)(step_record + 0x18),
                        step_record[0x16], &g_screen_recipe_name, step_name,
                        (uint8_t)logging_enabled, remaining_minutes, step_index_or_setup_result,
                        *(int *)(*(int *)recipe + 0x30));
                    if (future_step_index_or_result == 0) {
                        pico_log_serial("W", "RCTL", "runStep() returned false");
                        completion_result = 1;
                        if (logging_enabled == 0)
                            goto branch_1;
                        step_name_or_event = "Brew Canceled";
                        goto branch_2;
                    }
                }
            }
            step_record = step_record + 0x20;
            step_name_or_event = step_name_or_event + 0x20;
        }
        if (logging_enabled == 0) {
            completion_result = 0;
        } else {
            step_name_or_event = "Brew Complete";
            completion_result = 0;
        branch_2:
            pico_wifi_log_step(step_name_or_event);
        }
    branch_1:
        *(uint32_t *)((int)recipe + 0x22c) = 0;
        pico_end_brew_session(logging_enabled);
        completion_result = completion_result ^ 1;
    }
    return completion_result;
}

/* Explicit manualBrewProgram log and ManualBrew session assertion.
 * Firmware address: 0x20012020. */
/* Firmware entry: 0x20012020. Reviewed source reconstruction.
 * One PicoMenu occupies the original 88-byte action array and following count.
 * Instruction arrays are verified from image addresses 0x2002aa10/0x2002aca4.
 * The three display callbacks remain address references because their tiny
 * entry points have not yet been recovered as separate named functions.
 */
void pico_manual_brew_program(void)
{
    static uint32_t setup_screens[] = {16, 13, 7, 1};
    static uint32_t cleanup_screens[] = {2, 3, 1};
    PicoMenu menu;
    char step_name[64];

    pico_log_serial("I", "RCTL", "manualBrewProgram");
    pico_program_initialize_context((PicoProgramContext *)&g_session, 6, g_language);
    pico_assert_nonfatal(g_session_type == 5, "session.type == SessionTypes::ManualBrew");
    g_session_location = 1;
    pico_screen_set_recipe_name((PicoScreenState *)&g_screen_recipe_name, g_session_title);
    char *program_title = g_session_title;
    pico_menu_initialize(&menu);

    if (pico_ui_run_instruction_sequence(setup_screens, 4, 0, g_session_program_id, g_session_title,
                                         1) != PICO_ACTION_Next) {
        pico_log_serial("W", "RCTL", "User aborted");
        return;
    }
    if (!pico_start_session(1)) {
        pico_log_serial("W", "RCTL", "startSession() failed");
        return;
    }
    if (!pico_setup_instructions(1, program_title)) {
        pico_log_serial("W", "RCTL", "setupInstructions() failed");
        return;
    }

    PicoPageCarousel *carousel = (PicoPageCarousel *)&g_carousel_page_index;
    carousel->pages[0] = (void (*)(void))(uintptr_t)UINT32_C(0x2000477d);
    carousel->pages[1] = (void (*)(void))(uintptr_t)UINT32_C(0x20007e79);
    carousel->page_count = 3;
    carousel->pages[2] = (void (*)(void))(uintptr_t)UINT32_C(0x20009b75);

    int selected_menu_index = 0;
    int step_index = 0;
    for (;;) {
        pico_wifi_log_step("Waiting Instructions");
        pico_display_clear();
        menu.count = 0;
        pico_menu_add_action(&menu, PICO_ACTION_SetTargetTemperature);
        pico_menu_add_action(&menu, PICO_ACTION_SetHoldDuration);
        pico_menu_add_action(&menu, PICO_ACTION_StartStep);
        pico_menu_add_action(&menu, PICO_ACTION_ExitAsEndManualBrew);
        pico_ui_draw_instruction_screen(19, g_language, program_title, 1);
        pico_menu_populate_from_menu((uint32_t *)&menu, g_language, g_detailed_status);

        uint32_t action = pico_menu_select_action(7, selected_menu_index);
        if (action == PICO_ACTION_SetTargetTemperature) {
            g_session_target_fahrenheit = pico_get_temperature_input(
                PICO_ACTION_SetTargetTemperature, program_title, g_session_target_fahrenheit);
            selected_menu_index = 1;
            continue;
        }
        if (action == PICO_ACTION_SetHoldDuration) {
            g_session_hold_minutes = pico_get_time_input(PICO_ACTION_SetHoldDuration, program_title,
                                                         g_session_hold_minutes);
            selected_menu_index = 2;
            continue;
        }
        if (action == PICO_ACTION_StartStep) {
            libc_snprintf(step_name, sizeof step_name, "Step %d", step_index + 1);
            uint32_t compartment_id = pico_location_table_value(g_session_location);
            pico_screen_set_step((PicoScreenState *)&g_screen_recipe_name, compartment_id,
                                 step_name);
            pico_screen_setting_up_step(g_session_title);
            pico_display_flush();
            pico_wifi_log_step(step_name);

            int step_succeeded = pico_run_brew_step(
                &g_controller, g_session_target_fahrenheit & 255, g_session_hold_minutes, 0,
                (uint8_t)compartment_id, (char *)&g_screen_recipe_name, step_name, 1,
                g_session_hold_minutes, step_index, g_session_type);
            if (!step_succeeded) {
                pico_log_serial("W", "RCTL", "runStep() returned false");
            } else {
                pico_ui_draw_instruction_screen(20, g_language, program_title, 1);
                pico_alarm_until_press_or_timeout(g_language, g_detailed_status, 60000);
            }
            selected_menu_index = 0;
            ++step_index;
            continue;
        }
        if (action == PICO_ACTION_ExitAsEndManualBrew) {
            break;
        }

        /* The original logs this even for the generic Exit action. */
        pico_log_serial("W", "RCTL", "Unexpected action in manualBrewProgram");
        if (action == PICO_ACTION_Exit) {
            break;
        }
    }

    pico_end_brew_session(1);
    if (pico_run_drain_instructions(12) != PICO_ACTION_Next) {
        pico_log_serial("W", "RCTL", "User aborted drain");
        return;
    }
    if (pico_ui_run_instruction_sequence(cleanup_screens, 3, 0, g_session_program_id,
                                         g_session_title, 1) != PICO_ACTION_Next) {
        pico_log_serial("W", "RCTL", "User aborted");
        return;
    }
    pico_run_circulation(&g_wort_temperature_fahrenheit_low);
}

/* Still discovery/authorization and Heating/Maintain control program.
 * Firmware address: 0x20012568. */
uint32_t pico_run_still_program(void)
{
    char still_configure_result;
    bool still_has_home_ip_result;
    int still_select_access_point_2_result;
    char *text;
    uint32_t ui_choose_program_result;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    int unresolved_return_r3;
    uint8_t byte_value_5;

    pico_still_initialize_connection(0x2002d580);
    still_has_home_ip_result = pico_still_has_home_ip(0x2002d580, unresolved_return_r1,
                                                      unresolved_return_r2, unresolved_return_r3);
    pico_assert_nonfatal((uint32_t)!still_has_home_ip_result, "!still.hasHomeIP()");
    ui_choose_program_result = pico_ui_choose_program();
    if (ui_choose_program_result == 0) {
        ui_choose_program_result = pico_get_temperature_input(0x12, g_session_title, 0x87);
        byte_value_5 = (uint8_t)ui_choose_program_result;
    } else {
        if (ui_choose_program_result != 1) {
            return ui_choose_program_result;
        }
        byte_value_5 = 0xb4;
    }
    ui_choose_program_result = pico_ui_walk_program_steps(0);
    if (ui_choose_program_result != 0) {
        return ui_choose_program_result;
    }
    still_select_access_point_2_result = pico_still_select_access_point_2();
    if (still_select_access_point_2_result == 0) {
        text = "No Still found";
    } else {
        still_select_access_point_2_result = pico_api_can_use_still(&g_still_identifier);
        if (still_select_access_point_2_result != 0) {
            still_select_access_point_2_result = pico_api_has_cleaned_ack(&g_still_identifier);
            if (still_select_access_point_2_result == 0) {
                pico_ui_confirm_still_cleaned();
                pico_api_set_cleaned_ack(&g_still_identifier);
            }
            still_configure_result = pico_still_configure();
            if (still_configure_result == '\0') {
                pico_still_connect_wifi_network(1);
                pico_display_stop_progress_indicator(&g_progress_indicator);
            } else {
                pico_display_stop_progress_indicator(&g_progress_indicator);
                pico_display_clear();
                pico_display_center_text(2, "Starting Still");
                g_progress_indicator = 3;
                g_progress_start_column = 5;
                g_progress_end_column = 0x10;
                g_progress_glyph_index = 0;
                g_progress_active = 1;
                still_has_home_ip_result = pico_still_start_session_once(0x2002d580);
                if (still_has_home_ip_result) {
                    pico_program_initialize_context(g_active_program_context_as_uint32_t, 5,
                                                    (uint32_t)g_language_as_uint8_t);
                    pico_screen_set_recipe_name(
                        (PicoScreenState *)&g_screen_recipe_name,
                        ((PicoProgramContext *)(uintptr_t)g_active_program_context_as_uint32_t)
                            ->title);
                    g_brewing_unknown_17 = 0;
                    g_brewing_unknown_18 = 0x7fffffff;
                    g_recipe_step_count = 0;
                    g_brewing_unknown_16 = byte_value_5;
                    libc_strcpy(&g_recipe_steps, "Heating");
                    ui_choose_program_result = g_recipe_step_count + 1;
                    (&g_brewing_unknown_15)[(uint32_t)g_recipe_step_count * 0x20] = 2;
                    g_recipe_step_count = (uint8_t)ui_choose_program_result;
                    libc_strcpy(&g_recipe_steps + (ui_choose_program_result & 0xff) * 0x20,
                                "Maintain");
                    pico_display_stop_progress_indicator(&g_progress_indicator);
                    pico_display_clear();
                    g_carousel_page_callbacks_as_uint32_t = 0x20007699;
                    g_carousel_page_count = 3;
                    g_brewing_unknown_19_as_uint32_t = 0x20007699;
                    g_brewing_unknown_20_as_uint32_t = 0x20007699;
                    pico_log_serial("I", "RCTL", "runProgram()");
                    still_select_access_point_2_result = pico_start_session(0);
                    ui_choose_program_result = 0;
                    if (still_select_access_point_2_result != 0) {
                        g_device_state = 8;
                        pico_log_serial("I", "RCTL", "runControlProgram() BEGIN");
                        ui_choose_program_result =
                            pico_run_recipe_steps(&g_active_program_context_as_uint32_t, 0);
                        pico_log_serial("I", "RCTL", "runControlProgram() END");
                        pico_display_clear();
                        g_device_state = 2;
                        pico_run_post_session_instructions();
                        if (g_session_type - 2U < 2) {
                            pico_assert_nonfatal(
                                1, "locState == PICO_SOUS_VIDE || locState == PICO_DISTILL");
                            pico_vacuum_compartments(&g_active_program_context_as_uint32_t,
                                                     (uint32_t)g_legacy_compartment_mapping,
                                                     (uint32_t)g_legacy_compartment_mapping);
                        }
                        if (g_session_type == 1) {
                            pico_assert_nonfatal(
                                0, "locState == PICO_CLEANING || locState == PICO_SOUS_VIDE");
                            pico_ui_show_rinse_finished();
                            pico_run_rinse_cycle(&g_active_program_context_as_uint32_t);
                        }
                        Print_println_string(&global_serial0, "runProgram(): DONE");
                    }
                    return ui_choose_program_result;
                }
                pico_still_connect_wifi_network(1);
                pico_display_stop_progress_indicator(&g_progress_indicator);
                pico_display_clear();
            }
            pico_display_clear();
            pico_display_center_text(1, "Connection delay.");
            pico_display_center_text(2, "Wait one minute");
            pico_display_center_text(3, "and try again. If");
            pico_display_center_text(4, "problem persists,");
            pico_display_center_text(5, "restart Still.");
            ui_choose_program_result = pico_ui_wait_for_continue();
            return ui_choose_program_result;
        }
        text = "Cannot Use Still";
    }
    pico_display_clear();
    pico_display_center_text(1, text);
    ui_choose_program_result = pico_ui_wait_for_continue();
    return ui_choose_program_result;
}

/* Explicit runProgram and runControlProgram BEGIN/END logs.
 * Firmware address: 0x200125c8. */
uint32_t pico_run_control_program(int program_id)
{
    int start_session_result;
    uint32_t run_recipe_steps_result;
    uint32_t value_3;

    pico_log_serial("I", "RCTL", "runProgram()");
    start_session_result = pico_start_session(1);
    run_recipe_steps_result = 0;
    if (start_session_result != 0) {
        g_device_state = (uint8_t)program_id;
        pico_log_serial("I", "RCTL", "runControlProgram() BEGIN");
        run_recipe_steps_result = pico_run_recipe_steps(&g_active_program_context_as_uint32_t, 1);
        pico_log_serial("I", "RCTL", "runControlProgram() END");
        pico_display_clear();
        g_device_state = 2;
        pico_api_report_device_state();
        pico_run_post_session_instructions();
        if (g_session_type - 2U < 2) {
            if (program_id == 4) {
                value_3 = 1;
            } else {
                value_3 = (uint32_t)(program_id == 8);
            }
            pico_assert_nonfatal(value_3, "locState == PICO_SOUS_VIDE || locState == PICO_DISTILL");
            pico_vacuum_compartments(&g_active_program_context_as_uint32_t,
                                     (uint32_t)g_legacy_compartment_mapping,
                                     (uint32_t)g_legacy_compartment_mapping);
        }
        if (g_session_type == 1) {
            if (program_id == 7) {
                value_3 = 1;
            } else {
                value_3 = (uint32_t)(program_id == 4);
            }
            pico_assert_nonfatal(value_3,
                                 "locState == PICO_CLEANING || locState == PICO_SOUS_VIDE");
            pico_ui_show_rinse_finished();
            pico_run_rinse_cycle(&g_active_program_context_as_uint32_t);
        }
        Print_println_string(&global_serial0, "runProgram(): DONE");
    }
    return run_recipe_steps_result;
}

/* Confirms cleaning, builds135F cleaning recipe, waits Start Clean, runs control program7.
 * Firmware address: 0x20012600. */
void pico_run_deep_clean_program(void)
{
    bool ui_confirm_screen_result;

    ui_confirm_screen_result = pico_ui_confirm_screen(&LAB_2000a9dc_1);
    if (ui_confirm_screen_result) {
        pico_recipe_initialize_cleaning_steps(&g_active_program_context_as_uint32_t, 0x87);
        pico_display_clear();
        pico_ui_prompt_deep_clean_setup();
        g_carousel_page_callbacks_as_uint32_t = 0x20009ac9;
        g_brewing_unknown_19 = &LAB_20009bc0_1;
        g_carousel_page_count = 2;
        pico_run_control_program(7);
        return;
    }
    return;
}

/* Fetches server-required actions, dispatches action7 cleaning and action9 descaling.
 * Firmware address: 0x20012624. */
void pico_run_required_maintenance(void)
{
    int api_get_required_actions_result;
    int index;
    uint32_t buffer_38[11];

    api_get_required_actions_result = pico_api_get_required_actions(buffer_38, 10);
    for (index = 0; index < api_get_required_actions_result; index = index + 1) {
        if (buffer_38[index] == 7) {
            pico_run_deep_clean_program();
        } else if (buffer_38[index] == 9) {
            pico_ui_start_descale();
        }
    }
    return;
}

/* Utility wrapper runs circulation with current wort-temperature pointer.
 * Firmware address: 0x20012658. */
void pico_run_circulation_program(void)
{
    pico_recipe_initialize_cleaning_steps(&g_active_program_context_as_uint32_t, 0x87);
    pico_display_clear();
    pico_ui_prompt_deep_clean_setup();
    g_carousel_page_callbacks_as_uint32_t = 0x20009ac9;
    g_brewing_unknown_19 = &LAB_20009bc0_1;
    g_carousel_page_count = 2;
    pico_run_control_program(7);
    return;
}

/* SousVide/Coffee session assertions and Heating/Maintain step creation.
 * Firmware address: 0x20012730. */
void pico_sousvide_coldbrew_program(uint32_t program_id)
{
    uint32_t execute_current_program_instruction_result;
    uint32_t selected_action;
    int value_3;
    uint8_t byte_value_4;
    int value_5;
    uint32_t *pointer_6;
    pico_u32 value_7;

    pico_program_initialize_context((PicoProgramContext *)&g_session, program_id,
                                    (uint32_t)g_language_as_uint8_t);
    if (g_session_type == 2) {
        execute_current_program_instruction_result = 1;
    } else {
        execute_current_program_instruction_result = (uint32_t)(g_session_type == 4);
    }
    pico_assert_nonfatal(
        execute_current_program_instruction_result,
        "session.type == SessionTypes::SousVide || session.type == SessionTypes::Coffee");
    if (program_id == 3) {
        value_5 = 2;
        pointer_6 = &_data;
    } else {
        if (program_id != 4) {
            pico_log_serial("E", "MENU", "Unsupported program");
            return;
        }
        pointer_6 = &g_brewing_unknown_14;
        value_5 = 3;
    }
    pico_prompt_manual_temperature_duration();
    value_3 = 0;
    do {
        execute_current_program_instruction_result =
            pico_execute_current_program_instruction(pointer_6[value_3], 0);
        switch (execute_current_program_instruction_result) {
        case 0x17:
            value_3 = value_3 + 1;
            if (value_5 <= value_3) {
                pico_log_serial("W", "MENU", "Falling through ActionId::StartCooking");
                goto switchD_20012796_caseD_1a;
            }
            break;
        case 0x19:
            pico_prompt_manual_temperature_duration();
            break;
        case 0x1a:
            goto switchD_20012796_caseD_1a;
        case 0x22:
            return;
        }
    } while (true);
switchD_20012796_caseD_1a:
    if (program_id == 3) {
        pico_assert_nonfatal((uint32_t)(((PicoProgramContext *)(uintptr_t)
                                             g_active_program_context_as_picoprogramcontext_pointer)
                                            ->session_type == 2),
                             "session.type == SessionTypes::SousVide");
        pico_assert_nonfatal((uint32_t)(((PicoProgramContext *)(uintptr_t)
                                             g_active_program_context_as_picoprogramcontext_pointer)
                                            ->program_id == 3),
                             "session.programId == ProgramId::SousVideDefault");
        Print_println_string(&global_serial0, "Selected session parameters");
        pico_program_log_configuration(g_active_program_context_as_picoprogramcontext_pointer,
                                       (uint32_t)g_language_as_uint8_t);
        value_7 = ((PicoProgramContext *)(uintptr_t)
                       g_active_program_context_as_picoprogramcontext_pointer)
                      ->duration;
        byte_value_4 = (uint8_t)((PicoProgramContext *)(uintptr_t)
                                     g_active_program_context_as_picoprogramcontext_pointer)
                           ->temperature;
        pico_screen_set_recipe_name((PicoScreenState *)&g_screen_recipe_name,
                                    ((PicoProgramContext *)(uintptr_t)
                                         g_active_program_context_as_picoprogramcontext_pointer)
                                        ->title);
        g_brewing_unknown_17 = 0;
        g_brewing_unknown_18 = 1;
        g_recipe_step_count = 0;
        g_brewing_unknown_16 = byte_value_4;
        libc_strcpy(&g_recipe_steps, "Heating");
        execute_current_program_instruction_result = g_recipe_step_count + 2;
        *(uint8_t *)(execute_current_program_instruction_result * 0x20 + 0x2002bcde) = 2;
        (&g_brewing_unknown_15)[(g_recipe_step_count + 1 & 0xff) * 0x20] = 8;
    } else {
        if (program_id != 4)
            goto branch_1;
        pico_assert_nonfatal((uint32_t)(((PicoProgramContext *)(uintptr_t)
                                             g_active_program_context_as_picoprogramcontext_pointer)
                                            ->session_type == 4),
                             "session.type == SessionTypes::Coffee");
        pico_assert_nonfatal((uint32_t)(((PicoProgramContext *)(uintptr_t)
                                             g_active_program_context_as_picoprogramcontext_pointer)
                                            ->program_id == 4),
                             "session.programId == ProgramId::CoffeeColdBrew");
        Print_println_string(&global_serial0, "Selected session parameters");
        pico_program_log_configuration(g_active_program_context_as_picoprogramcontext_pointer,
                                       (uint32_t)g_language_as_uint8_t);
        value_7 = ((PicoProgramContext *)(uintptr_t)
                       g_active_program_context_as_picoprogramcontext_pointer)
                      ->duration;
        byte_value_4 = (uint8_t)((PicoProgramContext *)(uintptr_t)
                                     g_active_program_context_as_picoprogramcontext_pointer)
                           ->temperature;
        pico_screen_set_recipe_name((PicoScreenState *)&g_screen_recipe_name,
                                    ((PicoProgramContext *)(uintptr_t)
                                         g_active_program_context_as_picoprogramcontext_pointer)
                                        ->title);
        g_brewing_unknown_17 = 0;
        g_brewing_unknown_18 = 0;
        g_recipe_step_count = 0;
        g_brewing_unknown_16 = byte_value_4;
        libc_strcpy(&g_recipe_steps, "Heating");
        (&g_brewing_unknown_15)[(uint32_t)g_recipe_step_count * 0x20] = 1;
        execute_current_program_instruction_result = g_recipe_step_count + 1;
    }
    selected_action = execute_current_program_instruction_result & 0xff;
    g_recipe_step_count = (uint8_t)execute_current_program_instruction_result;
    (&g_brewing_unknown_16)[selected_action * 0x20] = byte_value_4;
    (&g_brewing_unknown_17)[selected_action * 8] = 0;
    (&g_brewing_unknown_18)[selected_action * 8] = value_7;
    libc_strcpy(&g_recipe_steps + selected_action * 0x20, "Maintain");
    execute_current_program_instruction_result = (uint32_t)g_recipe_step_count;
    g_recipe_step_count = g_recipe_step_count + 1;
    (&g_brewing_unknown_15)[execute_current_program_instruction_result * 0x20] = 1;
branch_1:
    pico_ui_wait_for_filter_insertion();
    g_carousel_page_callbacks = &LAB_200074d8_1;
    g_carousel_page_count = 2;
    g_brewing_unknown_19 = &LAB_20008808_1;
    pico_run_control_program(4);
    return;
}

/* Main brewing program: retrieve recipe, execute steps and post-brew rinse handling.
 * Firmware address: 0x20012d38. */
void pico_run_brew_program(void *recipe)
{
    char rfid_read_succeeded;
    bool recipe_fetch_failed;
    int selection_succeeded;
    uint32_t brew_completed;
    char *recipe_identifier;

    *(uint32_t *)((int)recipe + 0x22c) = 0;
    pico_program_initialize_context(*(PicoProgramContext **)recipe, 1,
                                    (uint32_t)g_language_as_uint8_t);
    pico_display_clear();
    pico_display_logo();
    g_carousel_page_callbacks_as_uint32_t = 0x20009ac9;
    g_brewing_unknown_19 = &LAB_20009bc0_1;
    g_initialization_unknown_24 = 0x20009be5;
    g_brewing_unknown_20_as_uint32_t = 0x20009c25;
    g_initialization_unknown_23 = &LAB_20008968_1;
    g_carousel_page_count = 5;
    recipe_identifier = (char *)((int)recipe + 4);
    *(uint32_t *)((int)recipe + 0x220) = 0xffffffff;
    *(uint32_t *)((int)recipe + 0x224) = 0xffffffff;
    rfid_read_succeeded = pico_rfid_read_picopak(recipe_identifier, 0x15);
    if (rfid_read_succeeded == '\0') {
        pico_display_clear();
        selection_succeeded = pico_ui_select_recipe((int)recipe);
        if (selection_succeeded == 0) {
            return;
        }
    } else {
        recipe_fetch_failed = pico_api_fetch_default_recipe_failed(recipe_identifier);
        if (recipe_fetch_failed) {
            pico_display_clear();
            selection_succeeded = pico_ui_select_recipe((int)recipe);
            if (selection_succeeded == 0) {
                return;
            }
        }
    }
    pico_cstring_copy(*(char **)recipe, 0x20, recipe_identifier);
    selection_succeeded = pico_ui_adjust_recipe(recipe);
    if (selection_succeeded != 0) {
        g_device_state = 3;
        brew_completed = pico_run_recipe_steps(recipe, 1);
        if (brew_completed == 0) {
            pico_ui_require_first_rinse();
        } else {
            pico_ui_draw_brew_complete(&g_screen_recipe_name);
            pico_display_wait_for_click();
        }
        pico_run_rinse_cycle(recipe);
    }
    return;
}
