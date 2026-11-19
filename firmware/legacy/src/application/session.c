/* Pico Pro 0.1.34 — session. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Explicit endBrewSession BEGIN/END; heat/cool/drain shutdown and state reset.
 * Firmware address: 0x20010368. */
void pico_end_brew_session(int logging_enabled)
{
    bool condition_1;
    uint32_t now_ms;
    int __aeabi_dcmplt_result;
    uint8_t *pointer_4;
    uint32_t value_5;

    pico_log_serial("I", "RCTL", "endBrewSession() BEGIN");
    digitalWrite(5, 0);
    digitalWrite(0x18, 0);
    digitalWrite(0x1b, 0);
    pico_recovery_delete_state();
    if (g_session_type == 3) {
        pico_still_stop(0x2002d580);
    }
    pico_screen_show_ending_program(g_session_unknown_02);
    pico_stepper_move_to_compartment(6);
    now_ms = millis();
    pico_shuttle_pump_on();
    pico_screen_show_ending_program(g_session_unknown_02);
    __aeabi_dcmplt_result =
        __aeabi_dcmplt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                 g_steam_temperature_fahrenheit_low),
                       250.0);
    if (__aeabi_dcmplt_result != 0) {
        pico_heater_on();
        do {
            __aeabi_dcmplt_result =
                __aeabi_dcmplt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                         g_steam_temperature_fahrenheit_low),
                               250.0);
            if (__aeabi_dcmplt_result == 0)
                break;
            pointer_4 = (uint8_t *)millis();
        } while (pointer_4 < &g_session_unknown_01 + now_ms);
        pico_heater_off();
    }
    pico_end_brew_run_outputs_for(3000, g_session_unknown_02, 1, 1);
    now_ms = millis();
    pico_log_serial("I", "RCTL", "!fullRotation || steamTemp <= 220");
    condition_1 = false;
    value_5 = 1;
branch_1:
    do {
        pointer_4 = (uint8_t *)millis();
        if (&g_session_unknown_01 + now_ms <= pointer_4)
            goto branch_2;
        pico_stepper_move_to_compartment(value_5);
        pico_shuttle_pump_on();
        value_5 = value_5 + 1 & 0xff;
        pico_end_brew_run_outputs_for(1000, g_session_unknown_02, 1, 1);
        if (value_5 < 6) {
            if (!condition_1)
                goto branch_1;
        } else {
            value_5 = 1;
            condition_1 = true;
        }
        __aeabi_dcmplt_result =
            __aeabi_dcmpgt((double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                                     g_steam_temperature_fahrenheit_low),
                           220.0);
        if (__aeabi_dcmplt_result == 0) {
        branch_2:
            pico_shuttle_pump_off();
            pico_log_serial("I", "RCTL", "drain only for 5s");
            pico_end_brew_run_outputs_for(5000, g_session_unknown_02, 1, 0);
            pico_drain_pump_off();
            if (g_shuttle_calibration_complete_as_char == '\0') {
                g_shuttle_scale_low = 0x9999999a;
                g_shuttle_scale_high = 0x3fc99999;
                pico_settings_save_shuttle_scale();
            }
            g_thermoblock_target_fahrenheit_low = 0;
            g_thermoblock_target_fahrenheit_high = 0x4072c000;
            g_controller_brew_phase_as_uint8_t = 4;
            g_shuttle_calibration_complete_as_char = 0;
            g_controller_status_as_uint8_t = 0;
            pico_storage_load_shuttle_scale();
            g_carousel_page_callbacks_as_uint32_t = 0x20009ac9;
            g_brewing_unknown_19 = &LAB_20009bc0_1;
            g_initialization_unknown_24 = 0x20009be5;
            g_brewing_unknown_20_as_uint32_t = 0x20009c25;
            g_initialization_unknown_23 = &LAB_20008968_1;
            g_carousel_page_count = 5;
            g_temperature_unknown_12 = 0;
            g_device_state = 2;
            if (logging_enabled != 0) {
                pico_api_report_device_state();
            }
            pico_stepper_move_home();
            pico_log_serial("I", "RCTL", "endBrewSession() END");
            return;
        }
    } while (true);
}

/* Starts optional server session and resets session timing/state.
 * Firmware address: 0x2001085c. */
int pico_start_session(int request_server_session)
{
    int api_get_session_id_result;

    pico_display_clear();
    pico_screen_loading(0);
    if ((request_server_session == 0) ||
        (api_get_session_id_result = pico_api_get_session_id(&g_session, 0x20),
         api_get_session_id_result != 0)) {
        pico_display_stop_progress_indicator(&g_progress_indicator);
        api_get_session_id_result = 1;
    } else {
        pico_display_stop_progress_indicator(&g_progress_indicator);
        api_get_session_id_result = 0;
    }
    return api_get_session_id_result;
}

/* Explicit parseWasBrewing log; colon-delimited session restoration fields.
 * Firmware address: 0x20010bcc. */
int pico_parse_restart_session(uint32_t *recipe)
{
    bool session_supports_recovery_result;
    int storage_read_restart_record_result;
    uint32_t libc_atoi_result;
    PicoProgramContext *context;
    int libc_atoi_result_2;
    char *output;
    uint32_t language;
    int value;
    char buffer_88[12];
    char buffer[100];

    pico_log_serial("I", "RCTL", "parseWasBrewing");
    storage_read_restart_record_result = pico_storage_read_restart_record(buffer);
    if (storage_read_restart_record_result != 0) {
        value = 0;
        storage_read_restart_record_result =
            pico_parse_delimited_field(buffer, buffer_88, &value, ":", 10);
        if (storage_read_restart_record_result != 0) {
            libc_atoi_result = libc_atoi(buffer_88);
            session_supports_recovery_result = pico_session_supports_recovery(libc_atoi_result);
            if (!session_supports_recovery_result) {
                pico_log_serial("W", "RCTL", "Cannot restore this session type");
                return 0;
            }
            output = (char *)(recipe + 1);
            storage_read_restart_record_result =
                pico_parse_delimited_field(buffer, output, &value, ":", value + 0x15);
            if ((storage_read_restart_record_result != 0) &&
                (storage_read_restart_record_result =
                     pico_parse_delimited_field(buffer, buffer_88, &value, ":", value + 10),
                 storage_read_restart_record_result != 0)) {
                storage_read_restart_record_result = libc_atoi(buffer_88);
                recipe[0x88] = storage_read_restart_record_result;
                storage_read_restart_record_result =
                    pico_parse_delimited_field(buffer, buffer_88, &value, ":", value + 10);
                if (storage_read_restart_record_result != 0) {
                    storage_read_restart_record_result = libc_atoi(buffer_88);
                    recipe[0x89] = storage_read_restart_record_result;
                    if (libc_atoi_result == 0) {
                        pico_display_clear();
                        pico_screen_loading_picopak();
                        storage_read_restart_record_result =
                            pico_api_fetch_recipe(output, recipe[0x89], recipe[0x88]);
                        if (storage_read_restart_record_result != 0) {
                            pico_display_stop_progress_indicator(&g_progress_indicator);
                            return 0;
                        }
                        pico_display_stop_progress_indicator(&g_progress_indicator);
                    } else {
                        if (libc_atoi_result != 1) {
                            Print_println_string(&global_serial0,
                                                 "Cannot restore this session type");
                            return 0;
                        }
                        pico_recipe_initialize_cleaning_steps(&g_active_program_context_as_uint32_t,
                                                              0x87);
                    }
                    storage_read_restart_record_result =
                        pico_parse_delimited_field(buffer, buffer_88, &value, ":", value + 10);
                    if (storage_read_restart_record_result != 0) {
                        storage_read_restart_record_result = libc_atoi(buffer_88);
                        recipe[0x8b] = storage_read_restart_record_result;
                        if (((storage_read_restart_record_result <
                              (int)(uint32_t) * (uint8_t *)(recipe + 0x84)) &&
                             (-1 < storage_read_restart_record_result)) &&
                            (storage_read_restart_record_result = pico_parse_delimited_field(
                                 buffer, buffer_88, &value, ":", value + 10),
                             storage_read_restart_record_result != 0)) {
                            libc_atoi_result_2 = recipe[0x8b];
                            storage_read_restart_record_result = libc_atoi(buffer_88);
                            recipe[libc_atoi_result_2 * 8 + 0x13] =
                                storage_read_restart_record_result;
                            storage_read_restart_record_result = pico_parse_delimited_field(
                                buffer, buffer_88, &value, ":", value + 10);
                            if (storage_read_restart_record_result != 0) {
                                libc_atoi_result_2 = recipe[0x8b];
                                storage_read_restart_record_result = libc_atoi(buffer_88);
                                recipe[libc_atoi_result_2 * 8 + 0x12] =
                                    storage_read_restart_record_result;
                                storage_read_restart_record_result = pico_parse_delimited_field(
                                    buffer, buffer_88, &value, ":", value + 10);
                                if (storage_read_restart_record_result != 0) {
                                    storage_read_restart_record_result = libc_atoi(buffer_88);
                                    if (storage_read_restart_record_result != 0) {
                                        *(uint8_t *)((int)recipe + (recipe[0x8b] + 2) * 0x20 + 7) =
                                            0;
                                    }
                                    storage_read_restart_record_result = pico_parse_delimited_field(
                                        buffer, buffer_88, &value, ":", value + 10);
                                    if (storage_read_restart_record_result != 0) {
                                        libc_atoi_result_2 = libc_atoi(buffer_88);
                                        if (libc_atoi_result_2 != 0) {
                                            g_shuttle_calibration_complete = 1;
                                        }
                                        context = (PicoProgramContext *)*recipe;
                                        language = (uint32_t)g_language_as_uint8_t;
                                        if (libc_atoi_result == 0) {
                                            libc_atoi_result = 1;
                                        } else if (libc_atoi_result == 1) {
                                            libc_atoi_result = 2;
                                        } else if (libc_atoi_result == 3) {
                                            libc_atoi_result = 5;
                                        } else if (libc_atoi_result == 2) {
                                            libc_atoi_result = 3;
                                        } else if (libc_atoi_result == 4) {
                                            libc_atoi_result = 4;
                                        } else if (libc_atoi_result == 5) {
                                            libc_atoi_result = 6;
                                        } else {
                                            Print_println_string(&global_serial0,
                                                                 "ERROR unexpected session type");
                                            libc_atoi_result = 0;
                                        }
                                        pico_program_initialize_context(context, libc_atoi_result,
                                                                        language);
                                        pico_cstring_copy((char *)context, 0x20, output);
                                        return storage_read_restart_record_result;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* Runs setup sequence and initial thermoblock checks, ending session on failure.
 * Firmware address: 0x200112b0. */
int pico_setup_instructions(int logging_enabled, char *program_title)
{
    bool ui_choose_binary_option_result;
    int step_filter_is_inserted_result;

    if (g_offline_mode != '\0') {
        Print_println_string(&global_serial0, "setupInstructions without logging (g_offlineMode)");
        logging_enabled = 0;
    }
    g_session_unknown_02 = program_title;
    pico_screen_starting_program(program_title);
    pico_display_flush();
    if (logging_enabled != 0) {
        pico_api_report_device_state();
    }
    ui_choose_binary_option_result = pico_ui_choose_binary_option(program_title);
    if (ui_choose_binary_option_result) {
        step_filter_is_inserted_result = pico_step_filter_is_inserted();
        if (step_filter_is_inserted_result == 0) {
            pico_ui_draw_paused();
            pico_wait_for_step_filter();
        }
        pico_display_clear();
        step_filter_is_inserted_result = pico_prime_steam_pump(0x2002d2f8, program_title);
        if (step_filter_is_inserted_result != 0) {
            return step_filter_is_inserted_result;
        }
        if (logging_enabled != 0) {
            pico_wifi_log_step("Thermoblock Issue");
        }
    }
    pico_end_brew_session(logging_enabled);
    return 0;
}

/* Explicit runPostSessionInstructions BEGIN/END and optional holding/drain instructions.
 * Firmware address: 0x20012290. */
void pico_run_post_session_instructions(void)
{
    uint32_t execute_current_program_instruction_result;
    int step_filter_is_inserted_result;

    pico_log_serial("I", "RCTL", "runPostSessionInstructions() BEGIN");
    if ((g_session_type == 2) || (g_session_type == 4)) {
        execute_current_program_instruction_result =
            pico_execute_current_program_instruction(0xb, 1);
        if (execute_current_program_instruction_result == 0x1c) {
            step_filter_is_inserted_result = pico_step_filter_is_inserted();
            if (step_filter_is_inserted_result != 0) {
                g_carousel_page_callbacks_as_uint32_t = 0x2000a241;
                g_carousel_page_count = 1;
                pico_display_advance_carousel((PicoPageCarousel *)&g_carousel_page_index);
                pico_run_brew_step(&g_controller,
                                   (uint32_t)(uint8_t)g_session_target_fahrenheit_as_uint8_t,
                                   0xffffffff, 0, '\x01', &g_screen_recipe_name, "Holding", '\x01',
                                   g_session_hold_minutes, 2, g_session_type);
                pico_end_brew_session(1);
            }
        } else if (execute_current_program_instruction_result == 0x1d) {
            pico_run_drain_instructions(0xf);
        }
    }
    pico_log_serial("I", "RCTL", "runPostSessionInstructions() END");
    return;
}
