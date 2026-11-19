/* Pico Pro 0.1.34 — initialization. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Constructs network/RFID/PID objects and initializes recipe/menu/controller/UI globals.
 * Firmware address: 0x200090f8. */
void pico_initialize_application_globals(void)
{
    uint32_t *step_cursor;
    int offset;
    int next_offset;
    uint32_t unresolved_input_stack_ffffffc4;

    IPAddress_construct_zero((uint32_t *)&g_still_controller);
    IPAddress_construct_zero((uint32_t *)&g_initialization_unknown_22);
    g_screen_recipe_name = 0;
    g_screen_step_name = 0;
    g_screen_progress_component = 0;
    g_screen_brew_state = 0;
    g_initialization_unknown_21 = 0;
    WiFiClient_construct(&g_http_client);
    unresolved_c_library_01(&g_http_client, 0x200156b5, &g_initialization_unknown_25);
    MFRC522_construct(0x2002d570, 4, 0x12, 0);
    g_device_uid = 0;
    g_device_state = 2;
    IPAddress_construct_octets((uint32_t *)&g_server_ip_address, 0, 0, 0, 0);
    g_carousel_last_update_ms = 0;
    offset = 0;
    do {
        next_offset = offset + 4;
        *(uint32_t *)((int)&g_carousel_page_callbacks_as_uint32_t + offset) = 0;
        offset = next_offset;
    } while (next_offset != 0x3c);
    g_carousel_interval_ms = &g_brewing_unknown_03;
    g_carousel_page_callbacks_as_uint32_t = 0x20009ac9;
    g_brewing_unknown_19 = &LAB_20009bc0_1;
    g_initialization_unknown_24 = 0x20009be5;
    g_brewing_unknown_20_as_uint32_t = 0x20009c25;
    g_carousel_page_count = 5;
    g_initialization_unknown_23 = &LAB_20008968_1;
    g_progress_indicator = 0;
    g_progress_start_column = 0;
    g_progress_end_column = 0;
    g_progress_glyph_index = 0;
    g_initialization_unknown_02 = 0;
    g_initialization_unknown_03 = 0;
    g_initialization_unknown_04 = 0;
    g_progress_active = 0;
    g_initialization_unknown_05 = 0;
    g_initialization_unknown_06 = 0x4057c000;
    g_initialization_unknown_11 = 5;
    g_initialization_unknown_07 = 0;
    g_initialization_unknown_08 = 0x40140000;
    g_initialization_unknown_09 = 0;
    g_initialization_unknown_10 = 0x40240000;
    pico_still_clear_telemetry(&g_controller);
    PID_disable(&g_initialization_unknown_13);
    PID_disable(&g_initialization_unknown_14);
    g_controller_compartment = 2;
    g_controller_status_as_uint8_t = 0;
    g_wort_temperature_fahrenheit_low = 0;
    g_wort_temperature_fahrenheit_high = 0x40518000;
    g_steam_temperature_fahrenheit_low = 0;
    g_steam_temperature_fahrenheit_high = 0x4054c000;
    g_thermoblock_target_fahrenheit_low = 0;
    g_thermoblock_target_fahrenheit_high = 0x4072c000;
    g_initialization_unknown_19 = 0x9999999a;
    g_initialization_unknown_20 = 0x3fc99999;
    g_shuttle_scale_low = 0x9999999a;
    g_shuttle_scale_high = 0x3fc99999;
    g_initialization_unknown_18 = 0;
    g_temperature_trend_seconds = 0;
    g_controller_brew_phase_as_uint8_t = 4;
    g_temperature_unknown_12 = 0;
    g_shuttle_calibration_complete = 0;
    g_initialization_unknown_15 = 0;
    g_initialization_unknown_12 = 0;
    g_step_time_seconds_as_uint32_t = 0;
    pico_still_clear_telemetry(&g_controller);
    PID_construct(&g_initialization_unknown_13, &g_wort_temperature_fahrenheit_low,
                  &g_initialization_unknown_16, &g_wort_target_fahrenheit_low, 130.0, 0, 0x40490000,
                  0, 0x403e0000, 0, unresolved_input_stack_ffffffc4, 0, 0x40a77000);
    PID_construct(&g_initialization_unknown_14, &g_steam_temperature_fahrenheit_low,
                  &g_initialization_unknown_17, &g_thermoblock_target_fahrenheit_low, 150.0, 0,
                  0x40490000, 0, 0x40540000, 0, unresolved_input_stack_ffffffc4, 0, 0x40a77000);
    pico_menu_initialize((PicoMenu *)&g_active_menu);
    step_cursor = (uint32_t *)&g_initialization_unknown_01;
    g_active_program_context = &g_session;
    do {
        *(uint8_t *)(step_cursor + -8) = 0;
        *(uint8_t *)((int)step_cursor + -9) = 0;
        step_cursor[-2] = 0;
        step_cursor[-1] = 0;
        *(uint8_t *)((int)step_cursor + -10) = 2;
        step_cursor = step_cursor + 8;
    } while (step_cursor != &g_recipe_ibu_adjustment_reference_as_uint32_t);
    offset = 0x2002bcd7;
    do {
        next_offset = offset + 1;
        *(uint8_t *)(offset + 5) = 0;
        *(uint8_t *)(offset + 0x1a) = 0;
        offset = next_offset;
    } while (next_offset != 0x2002bcec);
    g_recipe_abv_adjustment_as_uint32_t = 0xffffffff;
    g_recipe_ibu_adjustment_as_uint32_t = 0xffffffff;
    g_current_recipe_step_index = 0;
    g_recipe_integer_metric_as_uint32_t = 0;
    g_recipe_decimal_metric_low = 0;
    g_recipe_decimal_metric_high = 0;
    g_buzzer_enabled = 0;
    g_stepper_capabilities = 0;
    return;
}
