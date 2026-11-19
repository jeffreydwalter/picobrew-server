/* Pico Pro 0.1.34 — diagnostics. */
#include "pico/firmware.h"
#include <stdarg.h>

/* SENSORS screen shows step filter IN/OUT and temperature readings until encoder press.
 * Firmware address: 0x2000a818. */
void pico_diagnostic_sensors(void)
{
    bool digital_read_result;
    int encoder_take_click_result;
    uint32_t now_ms_2;
    char *text;
    uint32_t now_ms;

    pico_display_clear();
    pico_ui_draw_header("SENSORS");
    pico_ui_draw_footer_single("Done");
    pico_display_text_at_cell(3, 0, "Step Filter: ");
    pico_display_flush();
    now_ms = 0;
    while (encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           encoder_take_click_result == 0) {
        pico_display_clear_row(3);
        digital_read_result = digitalRead(0xb);
        if (digital_read_result) {
            text = "Step Filter: OUT ";
        } else {
            text = "Step Filter: IN  ";
        }
        pico_display_text_at_cell(3, 0, text);
        now_ms_2 = millis();
        if (1000 < now_ms_2 - now_ms) {
            pico_display_temperature_columns(
                5, &g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_low,
                g_wort_temperature_fahrenheit_high,
                (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                          g_steam_temperature_fahrenheit_low),
                0.0);
            now_ms = millis();
        }
        pico_display_flush();
    }
    return;
}

/* Draws error; optionally shuts down outputs, reports to API, and permits acknowledgement or halts.
 * Firmware address: 0x2000c814. */
void pico_report_error(int error_code, int allow_continue, int report_to_server)
{
    char wi_fi_status_result;

    pico_display_stop_progress_indicator(&g_progress_indicator);
    if (allow_continue == 0) {
        pico_heat_controller_stop(0x2002d2f8);
        pico_steam_pump_off();
    }
    pico_display_clear();
    pico_ui_draw_error(error_code);
    pico_display_flush();
    if (((report_to_server != 0) &&
         (wi_fi_status_result = WiFi_status(), wi_fi_status_result == '\x03')) &&
        (error_code != 7)) {
        pico_api_report_error(error_code);
    }
    if (allow_continue != 0) {
        pico_encoder_clear_click((PicoEncoder *)&g_encoder);
        pico_ui_wait_for_continue();
        pico_display_clear();
        return;
    }
    do {
        millis();
    } while (true);
}

/* Runs drain pump with elapsed-time display until encoder press.
 * Firmware address: 0x2000d098. */
void pico_diagnostic_drain_pump(void)
{
    uint32_t now_ms;
    int encoder_take_click_result;

    pico_display_clear();
    pico_ui_draw_header("Drain Pump");
    pico_drain_pump_on();
    now_ms = millis();
    pico_ui_draw_stop_footer();
    pico_display_center_text(3, "Drain Pump active");
    while (encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           encoder_take_click_result == 0) {
        pico_display_update_timer(5, now_ms, 0);
    }
    pico_drain_pump_off();
    return;
}

/* Runs GPIO27 pump at compartment2 with elapsed time until encoder press.
 * Firmware address: 0x2000d604. */
void pico_diagnostic_steam_pump(void)
{
    uint32_t now_ms;
    int encoder_take_click_result;

    pico_display_clear();
    pico_ui_draw_header("Keg Pump");
    pico_display_center_text(3, "Resetting");
    pico_display_center_text(4, "Rotary Valve");
    pico_display_flush();
    pico_stepper_move_to_compartment(2);
    now_ms = millis();
    pico_steam_pump_on();
    pico_display_clear();
    pico_ui_draw_header("Keg Pump");
    pico_display_center_text(3, "Keg Pump active");
    pico_ui_draw_stop_footer();
    while (encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           encoder_take_click_result == 0) {
        pico_pause_outputs_for_missing_filter(1, 0, 0);
        pico_display_update_timer(5, now_ms, 0);
    }
    pico_steam_pump_off();
    pico_restore_idle_hardware();
    return;
}

/* Drives heater GPIO5 for15seconds while displaying temperatures.
 * Firmware address: 0x2000d640. */
void pico_diagnostic_heater(void)
{
    uint32_t now_ms;
    uint32_t now_ms_2;
    uint32_t now_ms_3;

    pico_display_clear();
    pico_ui_draw_header("THERMOBLOCK STEAM");
    pico_display_center_text(3, "Resetting");
    pico_display_center_text(4, "Rotary Valve");
    pico_display_flush();
    pico_stepper_move_to_compartment(2);
    pico_display_clear();
    pico_ui_draw_header("THERMOBLOCK STEAM");
    now_ms = millis();
    digitalWrite(5, 1);
    now_ms_3 = 0;
    while (now_ms_2 = millis(), (uint8_t *)(now_ms_2 - now_ms) < &g_brewing_unknown_05) {
        now_ms_2 = millis();
        if (1000 < now_ms_2 - now_ms_3) {
            pico_display_temperature_columns(
                4, &g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_low,
                g_wort_temperature_fahrenheit_high,
                (double)pico_join_u32_u32(g_steam_temperature_fahrenheit_high,
                                          g_steam_temperature_fahrenheit_low),
                0.0);
            pico_display_flush();
            now_ms_3 = millis();
        }
    }
    digitalWrite(5, 0);
    pico_restore_idle_hardware();
    return;
}

/* Cycles rotary valve through compartments0..6 until encoder press.
 * Firmware address: 0x2000d6e8. */
void pico_diagnostic_stepper(void)
{
    int encoder_take_click_result;
    uint32_t value_2;

    pico_display_clear();
    pico_ui_draw_header("TEST ROTARY VALVE");
    pico_display_center_text(3, "Moving Rotary");
    pico_ui_draw_stop_footer();
    pico_display_flush();
    value_2 = 0;
    while (true) {
        pico_stepper_move_to_compartment(value_2 & 0xff);
        encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder);
        if (encoder_take_click_result != 0)
            break;
        value_2 = value_2 + 1;
        if (6 < (int)value_2) {
            value_2 = 0;
        }
    }
    pico_display_clear();
    pico_restore_idle_hardware();
    return;
}

/* Disables control, shuts down critical outputs, sounds alarm and halts on heater malfunction.
 * Firmware address: 0x2000e054. */
void pico_halt_heater_malfunction(void)
{
    IntMasterEnable();
    g_heat_controller_enabled = 0;
    pico_actuators_off_except_drain();
    pico_buzzer_on();
    IntMasterDisable();
    pico_display_clear();
    pico_ui_draw_heater_malfunction();
    pico_api_report_error(0x18);
    do {
        millis();
    } while (true);
}

/* Sends torque mode and reports COULD NOT SET HIGH TORQUE on failure.
 * Firmware address: 0x2000e49c. */
void pico_ui_toggle_stepper_torque(void)
{
    int stepper_set_torque_mode_result;
    char *text;

    pico_display_clear();
    pico_ui_draw_header("SETTING HIGH TORQUE");
    pico_display_flush();
    stepper_set_torque_mode_result = pico_stepper_set_torque_mode(10);
    if (stepper_set_torque_mode_result == 0) {
        pico_display_center_text(3, "HIGH TORQUE FAIL");
        text = "COULD NOT SET";
    } else {
        pico_display_clear();
        pico_display_center_text(3, "HIGH TORQUE SET");
        pico_display_center_text(4, "CLICK 4 STEPPER KEG");
        pico_ui_wait_for_continue();
        pico_flush_compartments(1, 1);
        pico_display_clear();
        pico_ui_draw_header("SETTING LOW TORQUE");
        pico_display_center_text(3, "PASSED WITH");
        text = "HIGH TORQUE";
    }
    pico_display_center_text(4, text);
    pico_ui_wait_for_continue();
    return;
}

/* Mash/Pass Thru/Adj1..4/Exit menu dispatches rotary valve movement.
 * Firmware address: 0x2000ea0c. */
void pico_ui_choose_compartment(void)
{
    uint32_t now_ms;
    int encoder_take_click_result;
    uint32_t menu_choose_result;

    pico_display_clear();
    pico_ui_draw_header("Keg Pump Posistion");
    libc_strcpy(&g_menu_label_0, "Mash");
    libc_strcpy(&g_menu_label_1, "Pass Thru");
    libc_strcpy(&g_menu_label_2, "Adj 1");
    libc_strcpy(&g_menu_label_3, "Adj 2");
    libc_strcpy(&g_menu_label_4, "Adj 4");
    libc_strcpy(&g_menu_label_5, "Adj 3");
    libc_strcpy(&g_menu_label_6, "Exit");
    menu_choose_result = pico_menu_choose(3, 7, 0);
    if (menu_choose_result == 6) {
        return;
    }
    pico_display_clear();
    pico_ui_draw_header("Keg Pump Posistion");
    pico_display_center_text(4, &g_menu_label_0 + menu_choose_result * 0x16);
    pico_display_flush();
    pico_stepper_move_to_compartment(menu_choose_result + 1 & 0xff);
    now_ms = millis();
    pico_steam_pump_on();
    pico_display_clear();
    pico_ui_draw_header("Keg Pump");
    pico_display_center_text(3, "Keg Pump active");
    pico_ui_draw_stop_footer();
    while (encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           encoder_take_click_result == 0) {
        pico_pause_outputs_for_missing_filter(1, 0, 0);
        pico_display_update_timer(5, now_ms, 0);
    }
    pico_steam_pump_off();
    pico_restore_idle_hardware();
    return;
}

/* Shuttle Pump heading; positions valve, enables shuttle until Stop, then restores idle.
 * Firmware address: 0x2000f158. */
void pico_diagnostic_shuttle_pump(void)
{
    pico_display_clear();
    pico_ui_draw_header("Shuttle Pump");
    pico_display_center_text(3, "Resetting");
    pico_display_center_text(4, "Rotary Valve");
    pico_display_flush();
    pico_stepper_move_to_compartment(2);
    pico_display_clear();
    pico_ui_draw_header("Shuttle Pump");
    pico_display_center_text(3, "Shuttle Pump active");
    pico_shuttle_pump_on();
    pico_ui_wait_for_stop();
    pico_shuttle_pump_off();
    pico_restore_idle_hardware();
    return;
}

/* ALARM/Sounding alarm display, buzzer on, wait Stop, buzzer off.
 * Firmware address: 0x2000f1b8. */
int pico_diagnostic_buzzer(void)
{
    int buzzer_off_result;

    pico_display_clear();
    pico_ui_draw_header("ALARM");
    pico_display_center_text(3, "Sounding alarm");
    pico_display_flush();
    pico_buzzer_on();
    pico_ui_wait_for_stop();
    buzzer_off_result = pico_buzzer_off();
    return buzzer_off_result;
}

/* Reads tag UID and displays No tag detected on failure.
 * Firmware address: 0x2000f1e8. */
void pico_diagnostic_rfid(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                          uint32_t argument_4)
{
    char rfid_read_uid_result;
    uint32_t value_20;
    uint32_t value;
    uint32_t value_18;
    uint32_t value_14;

    value_20 = argument_1;
    value = argument_2;
    value_18 = argument_3;
    value_14 = argument_4;
    libc_memset(&value_20, 0, 0x15);
    pico_display_clear();
    pico_ui_draw_header("RFID Detect");
    pico_ui_draw_footer_single("Exit Test");
    rfid_read_uid_result = pico_rfid_read_uid((char *)&value_20);
    if (rfid_read_uid_result == '\0') {
        libc_strcpy((char *)&value_20, "No tag detected");
    }
    pico_display_center_text(4, (char *)&value_20);
    pico_ui_wait_for_stop();
    return;
}

/* Calls each hardware diagnostic in sequence including shuttle, heater, sensors, stepper, RFID,
 * alarm and pumps. Firmware address: 0x2000f234. */
void pico_diagnostic_run_all(void)
{
    int diagnostic_buzzer_result;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;

    pico_display_clear();
    pico_diagnostic_shuttle_pump();
    pico_diagnostic_heater();
    pico_diagnostic_sensors();
    pico_diagnostic_stepper();
    pico_flush_compartments_until_press();
    diagnostic_buzzer_result = pico_diagnostic_buzzer();
    pico_diagnostic_rfid(diagnostic_buzzer_result, unresolved_return_r1, unresolved_return_r2,
                         unresolved_return_r3);
    pico_reset_shuttle_scale();
    pico_diagnostic_steam_pump();
    pico_diagnostic_drain_pump();
    pico_display_clear();
    return;
}

/* TEST PICO menu dispatches all individual hardware diagnostics.
 * Firmware address: 0x2000f858. */
void pico_diagnostics_menu(void)
{
    uint32_t menu_populate_hardware_tests_result;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;

    do {
        pico_display_clear();
        menu_populate_hardware_tests_result = pico_menu_populate_hardware_tests();
        pico_ui_draw_header("TEST PICO");
        menu_populate_hardware_tests_result =
            pico_menu_choose(3, menu_populate_hardware_tests_result, 0);
        switch (menu_populate_hardware_tests_result) {
        case 0:
            pico_diagnostic_run_all();
            break;
        case 1:
            pico_diagnostic_shuttle_pump();
            break;
        case 2:
            pico_diagnostic_heater();
            break;
        case 3:
            pico_diagnostic_sensors();
            break;
        case 4:
            pico_diagnostic_stepper();
            break;
        case 5:
            pico_flush_compartments_until_press();
            break;
        case 6:
            pico_diagnostic_buzzer();
            break;
        case 7:
            pico_diagnostic_rfid(menu_populate_hardware_tests_result, unresolved_return_r1,
                                 unresolved_return_r2, unresolved_return_r3);
            break;
        case 8:
            pico_reset_shuttle_scale();
            break;
        case 9:
            pico_diagnostic_steam_pump();
            break;
        case 10:
            pico_diagnostic_drain_pump();
            break;
        case 0xb:
            pico_ui_toggle_stepper_torque();
            break;
        case 0xc:
            pico_heat_flush_compartments(1, unresolved_return_r1, 0, 0x40740000);
            break;
        case 0xd:
            pico_ui_choose_compartment();
            break;
        case 0xe:
            pico_screen_show_help_temporarily();
            break;
        default:
            return;
        }
    } while (true);
}

/* Checks association/local IP, pings five times, checks web-service connection and displays
 * results. Firmware address: 0x2000fccc. */
void pico_diagnostic_wifi(void)
{
    char wi_fi_status_result;
    uint8_t wifi_ping_public_dns_result;
    uint32_t wifi_check_status_result;
    char *source;
    int api_report_error_result;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r2_00;
    uint32_t value_5;
    uint32_t unresolved_return_r3;
    uint32_t unresolved_return_r3_00;
    uint32_t value_6;
    uint64_t ui_draw_wifi_test_heading_result;
    char buffer[12];
    char buffer_20[4];
    int value;

    pico_display_clear();
    wi_fi_status_result = WiFi_status();
    if (wi_fi_status_result != '\x03') {
        pico_wifi_ensure_connection();
    }
    wi_fi_status_result = WiFi_status();
    if (wi_fi_status_result == '\x03') {
        pico_ui_show_wifi_test_result(0);
        wifi_check_status_result = pico_wifi_check_status(0, 1);
        delay(500);
        pico_display_clear();
        if (wifi_check_status_result == 3) {
            pico_ui_show_wifi_test_result(1);
        } else if ((2 < wifi_check_status_result) && (wifi_check_status_result - 9 < 2)) {
            pico_display_clear();
            ui_draw_wifi_test_heading_result =
                PICO_UNRESOLVED_RETURN(pico_ui_draw_wifi_test_heading());
            value_5 = unresolved_return_r2_00;
            value_6 = unresolved_return_r3_00;
            goto branch_1;
        }
        WiFi_get_local_ip((uint32_t *)buffer_20);
        if (value == any_ip_address_value) {
            pico_ui_show_wifi_test_result(2);
        } else {
            pico_ui_show_wifi_test_result(3);
            wifi_ping_public_dns_result = pico_wifi_ping_public_dns();
            source = itoa((uint32_t)wifi_ping_public_dns_result, buffer, 10);
            libc_strcpy(buffer_20, source);
            libc_strcat(buffer_20, " of 5 returned");
            pico_display_center_text(5, buffer_20);
            pico_display_flush();
            delay(0x5dc);
            pico_ui_show_wifi_test_result(4);
            pico_ui_show_wifi_test_result(5);
            api_report_error_result = pico_api_report_error(0x11);
            if (api_report_error_result == 0) {
                pico_display_clear_row(5);
                pico_display_center_text(5, "PicoBrew: NO");
            }
            pico_ui_wait_for_stop();
            pico_display_clear();
        }
    } else {
        ui_draw_wifi_test_heading_result = PICO_UNRESOLVED_RETURN(pico_display_clear());
        value_5 = unresolved_return_r2;
        value_6 = unresolved_return_r3;
    branch_1:
        pico_screen_wifi_connection_failed(
            (int)ui_draw_wifi_test_heading_result,
            (int)((uint64_t)ui_draw_wifi_test_heading_result >> 0x20), value_5, value_6);
        pico_ui_wait_for_continue();
    }
    return;
}

/* Displays IP, DNS, gateway and subnet mask.
 * Firmware address: 0x2000fdc0. */
void pico_ui_show_network_addresses(void)
{
    char wi_fi_status_result;
    char *itoa_result;
    int value_3;
    uint32_t value_54;
    uint8_t buffer_50[4];
    uint32_t value_2;
    uint8_t buffer_48[4];
    uint32_t value_44;
    uint8_t buffer_40[4];
    uint32_t value;
    uint8_t buffer_38[4];
    char buffer_34[12];
    char buffer_28[28];

    pico_display_clear();
    wi_fi_status_result = WiFi_status();
    if (wi_fi_status_result != '\x03') {
        pico_wifi_ensure_connection();
    }
    value_3 = 0;
    IPAddress_construct_octets(&value_54, 0, 0, 0, 0);
    IPAddress_construct_octets(&value_2, 0, 0, 0, 0);
    IPAddress_construct_octets(&value_44, 0, 0, 0, 0);
    IPAddress_construct_octets(&value, 0, 0, 0, 0);
    WiFi_get_ip_configuration(&global_wifi, (int)&value_54, (int)&value_2, (int)&value_44,
                              (int)&value);
    pico_display_clear();
    pico_display_center_text(0, "WIFI STATUS");
    pico_ui_draw_separator(2, 1, 1);
    pico_display_center_text(2, &g_wifi_ssid);
    libc_strcpy(buffer_28, "  IP: ");
    do {
        itoa_result = itoa((uint32_t)buffer_50[value_3], buffer_34, 10);
        libc_strcat(buffer_28, itoa_result);
        value_3 = value_3 + 1;
        libc_strcat(buffer_28, ".");
    } while (value_3 != 3);
    itoa_result = itoa((uint32_t)buffer_50[3], buffer_34, 10);
    libc_strcat(buffer_28, itoa_result);
    pico_display_text_at_cell(4, 0, buffer_28);
    libc_strcpy(buffer_28, " DNS: ");
    value_3 = 0;
    do {
        itoa_result = itoa((uint32_t)buffer_48[value_3], buffer_34, 10);
        libc_strcat(buffer_28, itoa_result);
        value_3 = value_3 + 1;
        libc_strcat(buffer_28, ".");
    } while (value_3 != 3);
    itoa_result = itoa((uint32_t)buffer_48[3], buffer_34, 10);
    libc_strcat(buffer_28, itoa_result);
    pico_display_text_at_cell(5, 0, buffer_28);
    libc_strcpy(buffer_28, "Gate: ");
    value_3 = 0;
    do {
        itoa_result = itoa((uint32_t)buffer_40[value_3], buffer_34, 10);
        libc_strcat(buffer_28, itoa_result);
        value_3 = value_3 + 1;
        libc_strcat(buffer_28, ".");
    } while (value_3 != 3);
    itoa_result = itoa((uint32_t)buffer_40[3], buffer_34, 10);
    libc_strcat(buffer_28, itoa_result);
    pico_display_text_at_cell(6, 0, buffer_28);
    libc_strcpy(buffer_28, "Mask: ");
    value_3 = 0;
    do {
        itoa_result = itoa((uint32_t)buffer_38[value_3], buffer_34, 10);
        libc_strcat(buffer_28, itoa_result);
        value_3 = value_3 + 1;
        libc_strcat(buffer_28, ".");
    } while (value_3 != 3);
    itoa_result = itoa((uint32_t)buffer_38[3], buffer_34, 10);
    libc_strcat(buffer_28, itoa_result);
    pico_display_text_at_cell(7, 0, buffer_28);
    pico_display_flush();
    pico_display_wait_for_click();
    return;
}
