/* Pico Pro 0.1.34 — hardware control. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Enables heater while thermoblock below threshold; faults after15seconds if still cold.
 * Firmware address: 0x2000c880. */
uint32_t pico_check_thermoblock_heating(int controller)
{
    double *thermoblock_temperature_words;
    int temperature_below_threshold;
    uint32_t heat_started_ms;
    uint32_t now_ms;

    thermoblock_temperature_words = (double *)(controller + 0x1f8);
    temperature_below_threshold = __aeabi_dcmplt(*thermoblock_temperature_words, 230.0);
    if (temperature_below_threshold != 0) {
        pico_heater_on();
        heat_started_ms = millis();
        while (temperature_below_threshold = __aeabi_dcmplt(*thermoblock_temperature_words, 230.0),
               temperature_below_threshold != 0) {
            now_ms = millis();
            if ((&g_brewing_unknown_05 < (uint8_t *)(now_ms - heat_started_ms)) &&
                (temperature_below_threshold = __aeabi_dcmplt(*thermoblock_temperature_words, 90.0),
                 temperature_below_threshold != 0)) {
                pico_heat_controller_stop(controller);
                pico_actuators_off_except_drain();
                pico_report_error(0x13, 1, 1);
                return 0;
            }
        }
        pico_heater_off();
    }
    return 1;
}

/* Sends command0x37, accepts0x0c ready and faults on0x58.
 * Firmware address: 0x2000ca5c. */
bool pico_stepper_is_ready(void)
{
    uint8_t spi_command_read_byte_result;

    spi_command_read_byte_result = pico_spi_command_read_byte('7');
    if (spi_command_read_byte_result == 'X') {
        pico_report_error(0x12, 0, 1);
    }
    return spi_command_read_byte_result == '\f';
}

/* Polls ready up to151 times at300ms; reports processor error on exhaustion.
 * Firmware address: 0x2000ca7e. */
uint8_t pico_stepper_wait_ready(void)
{
    bool stepper_is_ready_result;
    int value_2;

    value_2 = 0x97;
    while (true) {
        stepper_is_ready_result = pico_stepper_is_ready();
        if (stepper_is_ready_result) {
            return 1;
        }
        value_2 = value_2 + -1;
        if (value_2 == 0)
            break;
        delay(300);
    }
    pico_display_clear();
    pico_report_error(4, 0, 1);
    return 0;
}

/* Requests buzzer state enabled; avoids toggle if already enabled.
 * Firmware address: 0x2000cb40. */
uint32_t pico_buzzer_on(void)
{
    uint8_t spi_command_read_byte_result;
    char *level;
    char *message;

    if (g_buzzer_enabled_as_char == '\x01') {
        pico_log_serial("W", "SPI", "setBuzzer() useless");
        return 1;
    }
    spi_command_read_byte_result = pico_spi_command_read_byte('\v');
    if (spi_command_read_byte_result == '\v') {
        spi_command_read_byte_result = pico_spi_command_read_byte('\v');
        pico_stepper_wait_ready();
        if (spi_command_read_byte_result == '\x02') {
            g_buzzer_enabled_as_char = 1;
            return 1;
        }
        if (spi_command_read_byte_result == '\x03') {
            g_buzzer_enabled_as_char = 0;
            return 1;
        }
        level = "W";
        message = "Unknown alarm state";
    } else {
        pico_stepper_wait_ready();
        level = "E";
        message = "Failed to toggle alarm";
    }
    pico_log_serial(level, "SPI", message);
    return 0;
}

/* Requests buzzer state disabled; avoids toggle if already disabled.
 * Firmware address: 0x2000cb46. */
int pico_buzzer_off(void)
{
    uint8_t spi_command_read_byte_result;
    char *level;
    char *message;

    if (g_buzzer_enabled_as_char == '\0') {
        pico_log_serial("W", "SPI", "setBuzzer() useless");
        return 1;
    }
    spi_command_read_byte_result = pico_spi_command_read_byte('\v');
    if (spi_command_read_byte_result == '\v') {
        spi_command_read_byte_result = pico_spi_command_read_byte('\v');
        pico_stepper_wait_ready();
        if (spi_command_read_byte_result == '\x02') {
            g_buzzer_enabled_as_char = 1;
            return 1;
        }
        if (spi_command_read_byte_result == '\x03') {
            g_buzzer_enabled_as_char = 0;
            return 1;
        }
        level = "W";
        message = "Unknown alarm state";
    } else {
        pico_stepper_wait_ready();
        level = "E";
        message = "Failed to toggle alarm";
    }
    pico_log_serial(level, "SPI", message);
    return 0;
}

/* Explicit playAlarmWhileStepFilterMissing log; toggles alarm each second.
 * Firmware address: 0x2000cc00. */
void pico_alarm_while_filter_missing(void)
{
    bool condition_1;
    uint32_t step_filter_is_missing_result;
    uint32_t now_ms;
    uint32_t value_4;

    pico_log_serial("I", "SCR", "playAlarmWhileStepFilterMissing()");
    pico_log_serial("I", "SCR", "playAlarmWhile()");
    pico_assert_nonfatal(1, "predicate != NULL");
    pico_buzzer_off();
    condition_1 = false;
    value_4 = 0;
    while (step_filter_is_missing_result = pico_step_filter_is_missing(),
           step_filter_is_missing_result != 0) {
        millis();
        now_ms = millis();
        if (value_4 < now_ms) {
            if (condition_1) {
                pico_buzzer_off();
            } else {
                pico_buzzer_on();
            }
            value_4 = now_ms + 1000;
            condition_1 = (bool)(condition_1 ^ 1);
        }
    }
    if (condition_1) {
        pico_buzzer_off();
        return;
    }
    return;
}

/* Alternates buzzer until encoder button is pressed.
 * Firmware address: 0x2000cc50. */
void pico_alarm_until_encoder_press(void)
{
    bool condition_1;
    uint32_t now_ms;
    int encoder_take_click_result;
    uint32_t now_ms_2;

    now_ms = millis();
    pico_encoder_clear_click((PicoEncoder *)&g_encoder);
    pico_display_clear();
    pico_screen_error_heading();
    pico_display_flush();
    pico_buzzer_on();
    condition_1 = true;
    while (encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           encoder_take_click_result == 0) {
        now_ms_2 = millis();
        if (1000 < now_ms_2 - now_ms) {
            if (condition_1) {
                pico_buzzer_off();
            } else {
                pico_buzzer_on();
            }
            now_ms = millis();
            condition_1 = (bool)(condition_1 ^ 1);
        }
    }
    if (condition_1) {
        pico_buzzer_off();
        return;
    }
    return;
}

/* Command0x29 exchanges two response bytes, one returned and one output parameter.
 * Firmware address: 0x2000ccac. */
uint32_t pico_stepper_query_capabilities(uint32_t *reply_detail)
{
    uint8_t spi_command_read_byte_result;
    uint8_t spi_command_read_byte_result_2;
    uint8_t detail_reply;

    spi_command_read_byte_result = pico_spi_command_read_byte(')');
    if (spi_command_read_byte_result == ')') {
        spi_command_read_byte_result_2 = pico_spi_command_read_byte(')');
        detail_reply = pico_spi_transfer_selected_byte(')');
        *reply_detail = (uint32_t)detail_reply;
        return (uint32_t)spi_command_read_byte_result_2;
    }
    pico_stepper_wait_ready();
    return 0xffffffff;
}

/* Uses capability bit0 to choose legacy compartment mapping.
 * Firmware address: 0x2000ccd8. */
void pico_detect_stepper_configuration(void)
{
    uint32_t capabilities;
    uint32_t reply_detail;

    reply_detail = 0;
    delay(1000);
    capabilities = pico_stepper_query_capabilities(&reply_detail);
    if (capabilities == 0xffffffff) {
        pico_stepper_wait_ready();
        capabilities = pico_stepper_query_capabilities(&reply_detail);
    }
    if ((capabilities & 1) == 0) {
        g_legacy_compartment_mapping_as_uint8_t = 1;
        g_hardware_control_unknown_03 = 0;
    }
    g_stepper_capabilities_as_uint32_t = capabilities;
    return;
}

/* Command0x3b sends one mode byte; caller displays HIGH TORQUE failure on rejection.
 * Firmware address: 0x2000cd24. */
uint32_t pico_stepper_set_torque_mode(uint32_t torque_mode)
{
    uint8_t spi_command_read_byte_result;
    uint8_t spi_command_read_byte_result_2;

    pico_stepper_wait_ready();
    spi_command_read_byte_result = pico_spi_command_read_byte(';');
    if (spi_command_read_byte_result == ';') {
        spi_command_read_byte_result_2 = pico_spi_command_read_byte((uint8_t)torque_mode);
        if (spi_command_read_byte_result_2 == torque_mode) {
            return 1;
        }
        pico_stepper_wait_ready();
    }
    return 0;
}

/* Explicit moveStepperToCompartment log, legacy remapping, command0x21 handshake.
 * Firmware address: 0x2000cd4c. */
uint32_t pico_stepper_move_to_compartment(uint32_t compartment_id)
{
    uint8_t spi_command_read_byte_result;
    uint8_t spi_command_read_byte_result_2;
    uint32_t mapped_compartment;
    char log_message[64];

    libc_snprintf(log_message, 0x40, "moveStepperToCompartment %d", compartment_id);
    pico_log_serial("I", "STEP", log_message);
    spi_command_read_byte_result = pico_spi_command_read_byte('!');
    if (spi_command_read_byte_result == '!') {
        if (g_legacy_compartment_mapping_as_char == '\0') {
            mapped_compartment = compartment_id;
        } else if (compartment_id == 0) {
            mapped_compartment = 6;
        } else {
            mapped_compartment = compartment_id - 1 & 0xff;
        }
        spi_command_read_byte_result = (uint8_t)(mapped_compartment + 1);
        spi_command_read_byte_result_2 = pico_spi_command_read_byte(spi_command_read_byte_result);
        if ((uint32_t)spi_command_read_byte_result_2 == mapped_compartment + 1) {
            spi_command_read_byte_result_2 =
                pico_spi_command_read_byte(spi_command_read_byte_result);
            if (spi_command_read_byte_result_2 < 7) {
                delay((uint32_t)spi_command_read_byte_result_2 * 500);
                pico_stepper_wait_ready();
                g_controller_compartment = (char)compartment_id;
                return 1;
            }
            pico_report_error(5, 0, 1);
            return 0;
        }
    }
    pico_stepper_wait_ready();
    return 0;
}

/* Enables GPIO27 after ensuring filter present; paired shutdown helper confirmed.
 * Firmware address: 0x2000cfbc. */
void pico_steam_pump_on(void)
{
    int step_filter_is_inserted_result;

    if ((g_controller_brew_phase != '\x04') &&
        (step_filter_is_inserted_result = pico_step_filter_is_inserted(),
         step_filter_is_inserted_result == 0)) {
        pico_controller_pause_until_resume(0x2002d2f8);
    }
    digitalWrite(0x1b, 1);
    return;
}

/* Enables GPIO8; endBrew_runFor identifies this output as drain.
 * Firmware address: 0x2000cfe8. */
void pico_drain_pump_on(void)
{
    int step_filter_is_inserted_result;

    if ((g_controller_brew_phase != '\x04') &&
        (step_filter_is_inserted_result = pico_step_filter_is_inserted(),
         step_filter_is_inserted_result == 0)) {
        pico_controller_pause_until_resume(0x2002d2f8);
    }
    digitalWrite(8, 1);
    return;
}

/* Enables GPIO24; endBrew_runFor identifies this output as shuttle.
 * Firmware address: 0x2000d0e0. */
void pico_shuttle_pump_on(void)
{
    int step_filter_is_inserted_result;

    if ((g_controller_brew_phase != '\x04') &&
        (step_filter_is_inserted_result = pico_step_filter_is_inserted(),
         step_filter_is_inserted_result == 0)) {
        pico_controller_pause_until_resume(0x2002d2f8);
    }
    digitalWrite(0x18, 1);
    return;
}

/* On missing filter, disables pumps/heater, waits for replacement, then restores selected outputs.
 * Firmware address: 0x2000d10c. */
uint32_t pico_pause_outputs_for_missing_filter(int resume_steam, int resume_drain,
                                               int resume_shuttle)
{
    int filter_present;

    filter_present = pico_step_filter_is_inserted();
    if (filter_present != 0) {
        return 0;
    }
    pico_heater_off();
    pico_steam_and_drain_pumps_off();
    pico_shuttle_pump_off();
    pico_prompt_insert_filter_with_alarm();
    if (resume_steam != 0) {
        pico_steam_pump_on();
    }
    if (resume_drain != 0) {
        pico_drain_pump_on();
    }
    if (resume_shuttle != 0) {
        pico_shuttle_pump_on();
    }
    return 1;
}

/* Acknowledges timer0x40031000 and schedules heater/shuttle on-times while control enabled.
 * Firmware address: 0x2000d1c0. */
bool pico_heater_pump_timer_isr(void)
{
    bool int_master_enable_result;
    uint32_t now_ms;
    uint32_t interrupt_flags;
    int __aeabi_dcmpgt_result;
    double right;

    now_ms = millis();
    IntMasterDisable();
    interrupt_flags = TimerIntStatus(0x40031000, true);
    TimerIntClear(0x40031000, interrupt_flags);
    if ((g_heat_controller_enabled_as_char != '\0') && (g_controller_status_as_char == '\0')) {
        if (3000 < now_ms - g_heater_cycle_started_ms) {
            g_heater_cycle_started_ms = g_heater_cycle_started_ms + 3000;
            g_shuttle_pulse_started_ms = g_heater_cycle_started_ms;
        }
        right = __aeabi_ui2d(now_ms - g_heater_cycle_started_ms);
        __aeabi_dcmpgt_result = __aeabi_dcmpgt(
            (double)pico_join_u32_u32(g_hardware_control_unknown_07, g_initialization_unknown_17),
            right);
        if (__aeabi_dcmpgt_result == 0) {
            pico_heater_off();
        } else {
            pico_heater_on();
            g_heater_on_time_ms = g_heater_on_time_ms + 0x14;
        }
        if (now_ms - g_shuttle_pulse_started_ms < g_shuttle_on_time_ms) {
            pico_shuttle_pump_on();
        } else {
            pico_shuttle_pump_off();
        }
        if (500 < now_ms - g_shuttle_pulse_started_ms) {
            g_shuttle_pulse_started_ms = g_shuttle_pulse_started_ms + 500;
        }
    }
    int_master_enable_result = IntMasterEnable();
    return int_master_enable_result;
}

/* Enables drain, waits500ms, enables steam pump after filter check.
 * Firmware address: 0x2000d288. */
void pico_start_drain_and_steam_pumps(void)
{
    int step_filter_is_inserted_result;

    if ((g_controller_brew_phase != '\x04') &&
        (step_filter_is_inserted_result = pico_step_filter_is_inserted(),
         step_filter_is_inserted_result == 0)) {
        pico_controller_pause_until_resume(0x2002d2f8);
    }
    digitalWrite(8, 1);
    delay(500);
    digitalWrite(0x1b, 1);
    return;
}

/* Chooses steam-only or combined drain/steam circulation based on session type.
 * Firmware address: 0x2000d2c4. */
void pico_controller_start_circulation(void)
{
    if (g_session_type - 3U < 3) {
        pico_drain_pump_off();
        pico_steam_pump_on();
        return;
    }
    pico_start_drain_and_steam_pumps();
    return;
}

/* Stops outputs and sounds missing-filter alarm, then restores brew phase.
 * Firmware address: 0x2000d2e8. */
uint32_t pico_handle_filter_removal(void)
{
    int step_filter_is_inserted_result;

    step_filter_is_inserted_result = pico_step_filter_is_inserted();
    if (step_filter_is_inserted_result == 0) {
        pico_heat_controller_stop(0x2002d2f8);
        pico_steam_and_drain_pumps_off();
        pico_prompt_insert_filter_with_alarm();
        pico_controller_set_brew_phase(0x2002d2f8, (uint32_t)g_controller_brew_phase_as_uint8_t_2);
    }
    return 0;
}

/* Sets compartment2, starts circulation, checks thermoblock heating, primes for10seconds.
 * Firmware address: 0x2000d380. */
int pico_prime_steam_pump(int controller, char *program_title)
{
    int heat_check_succeeded;

    heat_check_succeeded = pico_step_filter_is_inserted();
    if (heat_check_succeeded == 0) {
        pico_display_clear();
        pico_prompt_insert_filter_with_alarm();
    }
    pico_display_clear();
    pico_display_draw_titled_frame(program_title);
    pico_display_center_text(3, "Priming steam");
    pico_display_center_text(4, "generator");
    pico_display_flush();
    pico_stepper_move_to_compartment(2);
    *(uint8_t *)(controller + 0x60) = 2;
    pico_controller_start_circulation();
    heat_check_succeeded = pico_check_thermoblock_heating(controller);
    if (heat_check_succeeded != 0) {
        pico_display_clear();
        pico_display_draw_titled_frame(program_title);
        pico_display_center_text(3, "Priming steam");
        pico_display_center_text(4, "pump");
        pico_display_flush();
        delay(10000);
        pico_steam_pump_off();
        pico_drain_pump_off();
    }
    return heat_check_succeeded;
}

/* Moves to configured default compartment.
 * Firmware address: 0x2000d40c. */
void pico_stepper_move_home(void)
{
    pico_stepper_move_to_compartment((uint32_t)g_hardware_control_unknown_03_as_uint8_t);
    return;
}

/* Resets runtime state and returns rotary valve to configured home compartment.
 * Firmware address: 0x2000d418. */
void pico_restore_idle_hardware(void)
{
    pico_screen_ending_process();
    pico_stepper_move_home();
    return;
}

/* Acknowledges timer0x40030000; samples wort/thermoblock, updates PID/timers and thermal faults.
 * Firmware address: 0x2000e088. */
bool pico_temperature_control_timer_isr(uint32_t argument_1, uint32_t argument_2)
{
    bool int_master_enable_result;
    uint32_t now_ms;
    uint32_t interrupt_flags;
    float temperature_read_pin25_result;
    int __aeabi_dcmplt_result;
    uint8_t byte_value_5;
    uint32_t value_6;
    uint32_t value_7;
    uint32_t value_8;
    uint8_t *unresolved_saved_r10;
    double __aeabi_f2d_result;
    uint32_t value_30;
    uint32_t value;

    now_ms = millis();
    IntMasterDisable();
    interrupt_flags = TimerIntStatus(0x40030000, true);
    TimerIntClear(0x40030000, interrupt_flags);
    if (g_progress_active_as_char != '\0') {
        byte_value_5 = g_progress_glyph_index_as_uint8_t + g_progress_start_column_as_uint8_t;
        g_progress_glyph_index_as_uint8_t = g_progress_glyph_index_as_uint8_t + 1;
        pico_display_text_at_cell((uint16_t)g_progress_indicator_as_uint8_t, (uint16_t)byte_value_5,
                                  ".");
        if ((uint32_t)g_progress_glyph_index_as_uint8_t +
                (uint32_t)g_progress_start_column_as_uint8_t ==
            (uint32_t)g_progress_end_column_as_uint8_t) {
            pico_display_clear_row_from_column((uint16_t)g_progress_indicator_as_uint8_t,
                                               (uint16_t)g_progress_start_column_as_uint8_t);
            g_progress_glyph_index_as_uint8_t = 0;
        }
        pico_display_flush();
    }
    temperature_read_pin25_result = pico_temperature_read_pin25();
    __aeabi_f2d_result = __aeabi_f2d(temperature_read_pin25_result);
    g_wort_temperature_fahrenheit_high = (uint32_t)((uint64_t)__aeabi_f2d_result >> 0x20);
    g_wort_temperature_fahrenheit_low = PICO_SUB_U32(__aeabi_f2d_result, 0);
    temperature_read_pin25_result = pico_temperature_read_pin26();
    __aeabi_f2d_result = __aeabi_f2d(temperature_read_pin25_result);
    g_steam_temperature_fahrenheit_high = (uint32_t)((uint64_t)__aeabi_f2d_result >> 0x20);
    g_steam_temperature_fahrenheit_low = PICO_SUB_U32(__aeabi_f2d_result, 0);
    if (g_controller_brew_phase == '\b') {
        g_step_time_seconds = g_step_time_seconds + 1;
    } else if (g_step_time_seconds != 0) {
        g_step_time_seconds = g_step_time_seconds + -1;
    }
    value_30 = argument_1;
    value = argument_2;
    if (g_heat_controller_enabled_as_char == '\0')
        goto branch_1;
    unresolved_saved_r10 = &g_controller;
    if (g_controller_status != 0)
        goto branch_1;
    value_30 = g_wort_temperature_fahrenheit_low;
    value = g_wort_temperature_fahrenheit_high;
    __aeabi_dcmplt_result =
        __aeabi_dcmplt((double)pico_join_u32_u32(g_wort_temperature_fahrenheit_high,
                                                 g_wort_temperature_fahrenheit_low),
                       32.0);
    if ((__aeabi_dcmplt_result == 0) &&
        (__aeabi_dcmplt_result = __aeabi_dcmplt(__aeabi_f2d_result, 32.0),
         __aeabi_dcmplt_result == 0)) {
        __aeabi_dcmplt_result = __aeabi_dcmpgt(__aeabi_f2d_result, 385.0);
        if (__aeabi_dcmplt_result == 0)
            goto branch_2;
        goto branch_3;
    }
    pico_heat_controller_stop(0x2002d2f8);
    pico_steam_pump_off();
    pico_drain_pump_off();
    g_controller_status = 3;
    do {
        IntMasterDisable();
    branch_4:
        value_8 = (uint32_t)g_controller_status;
        unresolved_saved_r10 = &g_controller;
        __aeabi_f2d_result = (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                       g_steam_temperature_fahrenheit_low);
        if (value_8 == 0) {
            g_temperature_trend_seconds_as_uint32_t = g_temperature_trend_seconds_as_uint32_t + 1;
            if ((int)g_temperature_trend_seconds_as_uint32_t < 0x3c) {
                __aeabi_f2d_result = (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                               g_steam_temperature_fahrenheit_low);
                if ((int)g_temperature_trend_seconds_as_uint32_t % 0x1e == 0) {
                    g_hardware_control_unknown_04 = g_wort_temperature_fahrenheit_low;
                    g_hardware_control_unknown_05 = g_wort_temperature_fahrenheit_high;
                    pico_heat_record_temperature_sample(0x2002d2f8);
                    __aeabi_f2d_result = (double)pico_join_u32_u32(
                        g_steam_temperature_fahrenheit_high, g_steam_temperature_fahrenheit_low);
                }
            } else {
                pico_heat_record_temperature_sample(0x2002d2f8);
                value_7 = g_wort_temperature_fahrenheit_high;
                value_6 = g_wort_temperature_fahrenheit_low;
                g_temperature_trend_seconds_as_uint32_t = value_8;
                __aeabi_dcmplt_result = __aeabi_dcmpgt(
                    (double)pico_join_u32_u32(g_wort_temperature_fahrenheit_high,
                                              g_wort_temperature_fahrenheit_low),
                    (double)pico_join_u32_u32(g_temperature_unknown_14, g_temperature_unknown_13));
                if (__aeabi_dcmplt_result == 0) {
                } else {
                    g_temperature_unknown_13 = value_6;
                    g_temperature_unknown_14 = value_7;
                }
                __aeabi_dcmplt_result =
                    __aeabi_dcmpgt((double)pico_join_u32_u32(value_7, value_6),
                                   (double)pico_join_u32_u32(g_hardware_control_unknown_05,
                                                             g_hardware_control_unknown_04));
                if (__aeabi_dcmplt_result == 0) {
                } else {
                    g_hardware_control_unknown_04 = value_6;
                    g_hardware_control_unknown_05 = value_7;
                }
                __aeabi_dcmplt_result = pico_heat_classify_temperature_progress(
                    g_hardware_control_unknown_04, g_hardware_control_unknown_05, value_6, value_7);
                if (__aeabi_dcmplt_result == 2) {
                    pico_heat_controller_stop(0x2002d2f8);
                    pico_steam_pump_off();
                    pico_drain_pump_off();
                    g_controller_status = 6;
                } else if (__aeabi_dcmplt_result == 3) {
                    pico_heat_controller_stop(0x2002d2f8);
                    pico_steam_pump_off();
                    pico_drain_pump_off();
                    g_controller_status = 7;
                } else {
                    __aeabi_f2d_result = (double)pico_join_u32_u32(
                        g_steam_temperature_fahrenheit_high, g_steam_temperature_fahrenheit_low);
                    if (__aeabi_dcmplt_result != 1)
                        goto branch_1;
                    pico_heat_controller_stop(0x2002d2f8);
                    pico_steam_pump_off();
                    pico_drain_pump_off();
                    g_controller_status = 5;
                }
                IntMasterDisable();
                __aeabi_f2d_result = (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                               g_steam_temperature_fahrenheit_low);
            }
        }
    branch_1:
        g_steam_temperature_fahrenheit_high = (uint32_t)((uint64_t)__aeabi_f2d_result >> 0x20);
        g_steam_temperature_fahrenheit_low = PICO_SUB_U32(__aeabi_f2d_result, 0);
        if (g_heat_controller_enabled_as_char != '\0') {
        branch_5:
            g_steam_temperature_fahrenheit_high = (uint32_t)((uint64_t)__aeabi_f2d_result >> 0x20);
            g_steam_temperature_fahrenheit_low = PICO_SUB_U32(__aeabi_f2d_result, 0);
            int_master_enable_result = IntMasterEnable();
            return int_master_enable_result;
        }
        __aeabi_dcmplt_result = __aeabi_dcmpgt(__aeabi_f2d_result, 385.0);
        __aeabi_f2d_result = (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                       g_steam_temperature_fahrenheit_low);
        if (__aeabi_dcmplt_result == 0)
            goto branch_5;
    branch_3:
        pico_halt_heater_malfunction();
    branch_2:
        __aeabi_dcmplt_result = __aeabi_dcmpgt((double)pico_join_u32_u32(value, value_30), 215.0);
        if ((__aeabi_dcmplt_result == 0) || (__aeabi_dcmplt_result = __aeabi_dcmpgt(
                                                 *(double *)(unresolved_saved_r10 + 0x1d0), 195.0),
                                             __aeabi_dcmplt_result == 0)) {
            PID_compute(&g_initialization_unknown_13);
            PID_compute(&g_initialization_unknown_14);
            now_ms = now_ms - g_heater_cycle_started_ms;
            if (3000 < now_ms) {
                g_heater_cycle_started_ms = g_heater_cycle_started_ms + 3000;
                g_shuttle_pulse_started_ms = g_heater_cycle_started_ms;
            }
            __aeabi_dcmplt_result = __aeabi_dcmplt(
                (double)pico_join_u32_u32(g_initialization_unknown_20, g_initialization_unknown_19),
                0.0);
            if (__aeabi_dcmplt_result == 0) {
                value_6 = g_initialization_unknown_19;
                value_7 = g_initialization_unknown_20;
            } else {
                value_6 = 0;
                value_7 = 0;
            }
            g_initialization_unknown_19 = value_6;
            g_initialization_unknown_20 = value_7;
            __aeabi_f2d_result =
                __aeabi_ddiv((double)pico_join_u32_u32(g_hardware_control_unknown_06,
                                                       g_initialization_unknown_16),
                             3000.0);
            __aeabi_f2d_result =
                __aeabi_dmul(__aeabi_f2d_result, (double)pico_join_u32_u32(value_7, value_6));
            __aeabi_f2d_result = __aeabi_dmul(__aeabi_f2d_result, 500.0);
            g_shuttle_on_time_ms = __aeabi_d2uiz(__aeabi_f2d_result);
            if (g_shuttle_on_time_ms < 0x16) {
                g_shuttle_on_time_ms = 0x15;
            }
            goto branch_4;
        }
        pico_heat_controller_stop((int)unresolved_saved_r10);
        unresolved_saved_r10[0x61] = 1;
    } while (true);
}

/* Thermoblock heat check and shuttle operation; optional progress logging and pump restoration.
 * Firmware address: 0x20011338. */
uint32_t pico_reprime_steam_pump(int controller, char *program_title, char *step_name,
                                 int logging_enabled, char restart_circulation)
{
    int remaining_log_intervals;
    uint32_t heat_check_succeeded;
    uint32_t last_log_ms;
    int thermoblock_still_hot;
    uint32_t now_ms;
    uint32_t result;

    remaining_log_intervals = pico_step_filter_is_inserted();
    if (remaining_log_intervals == 0) {
        pico_display_clear();
        pico_prompt_insert_filter_with_alarm();
    }
    if (restart_circulation != '\0') {
        pico_controller_start_circulation();
    }
    pico_display_clear();
    pico_display_draw_titled_frame(program_title);
    pico_display_center_text(3, "Priming steam");
    pico_display_center_text(4, "pump");
    pico_display_flush();
    heat_check_succeeded = pico_check_thermoblock_heating(controller);
    result = heat_check_succeeded;
    if (heat_check_succeeded != 0) {
        last_log_ms = millis();
        pico_shuttle_pump_on();
        remaining_log_intervals = 0x10;
        do {
            do {
                thermoblock_still_hot = __aeabi_dcmpgt(*(double *)(controller + 0x1f8), 230.0);
                if (thermoblock_still_hot == 0)
                    goto branch_1;
            } while ((logging_enabled == 0) ||
                     (now_ms = millis(),
                      (uint8_t *)(now_ms - last_log_ms) < &g_hardware_control_unknown_01));
            pico_wifi_log_progress(step_name);
            last_log_ms = millis();
            remaining_log_intervals = remaining_log_intervals + -1;
        } while (remaining_log_intervals != 0);
    branch_1:
        pico_shuttle_pump_off();
        if (restart_circulation != '\0') {
            pico_controller_start_circulation();
            delay(5000);
        }
        pico_display_clear();
        result = (uint32_t) * (uint8_t *)(controller + 0x1a0);
        if (result == 0) {
            *(uint32_t *)(controller + 0x218) = 0x9999999a;
            *(uint32_t *)(controller + 0x21c) = 0x3fc99999;
            *(uint32_t *)(controller + 0x220) = 0x9999999a;
            *(uint32_t *)(controller + 0x224) = 0x3fc99999;
            result = heat_check_succeeded;
        }
    }
    return result;
}

/* Explicit playAlarmUntilEncoderPressOrTimeout log, encoder/time predicate.
 * Firmware address: 0x20013460. */
void pico_alarm_until_press_or_timeout(uint32_t language, uint32_t status_selector, int timeout_ms)
{
    bool alarm_on;
    int pulse_duration_ms;
    uint32_t now_ms;
    uint32_t alarm_started_ms;
    char *silence_label;
    uint32_t next_toggle_ms;

    silence_label = pico_action_label(0x31, language, status_selector);
    pico_ui_draw_footer_single(silence_label);
    pico_display_flush();
    pico_log_serial("I", "SCR", "playAlarmUntilEncoderPressOrTimeout()");
    alarm_started_ms = millis();
    pico_log_serial("I", "SCR", "playAlarmWhile()");
    pico_assert_nonfatal(1, "predicate != NULL");
    pico_buzzer_off();
    alarm_on = false;
    next_toggle_ms = 0;
    while (pulse_duration_ms = (*(PicoOpaqueFunction *)&g_hardware_control_unknown_02)(
               alarm_started_ms + timeout_ms),
           pulse_duration_ms != 0) {
        millis();
        now_ms = millis();
        if (next_toggle_ms < now_ms) {
            if (alarm_on) {
                pico_buzzer_off();
                pulse_duration_ms = 4000;
            } else {
                pico_buzzer_on();
                pulse_duration_ms = 0x2ee;
            }
            next_toggle_ms = now_ms + pulse_duration_ms;
            alarm_on = (bool)(alarm_on ^ 1);
        }
    }
    if (alarm_on) {
        pico_buzzer_off();
        return;
    }
    return;
}
