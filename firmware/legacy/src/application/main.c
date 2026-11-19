/* Pico Pro 0.1.34 — main. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Main menu actions12..17 dispatch to brewing, cooking, manual, utilities and settings.
 * Firmware address: 0x20012e10. */
/* Firmware entry: 0x20012e10. Reviewed source reconstruction.
 * The six actions are verified against the original table at 0x200285ac.
 */
void pico_loop_body(void)
{
    static uint32_t main_actions[] = {PICO_ACTION_StartBrewing,  PICO_ACTION_StartSousVide,
                                      PICO_ACTION_StartColdBrew, PICO_ACTION_StartManualBrew,
                                      PICO_ACTION_UtilitiesMenu, PICO_ACTION_SettingsMenu};

    pico_display_clear();
    pico_display_logo();
    pico_menu_populate_action_labels(main_actions, 6, g_language, g_detailed_status);

    switch (pico_menu_select_action(4, 0)) {
    case PICO_ACTION_StartBrewing:
        pico_run_brew_program(&g_active_program_context);
        return;
    case PICO_ACTION_StartSousVide:
        pico_sousvide_coldbrew_program(3);
        return;
    case PICO_ACTION_StartColdBrew:
        pico_sousvide_coldbrew_program(4);
        return;
    case PICO_ACTION_StartManualBrew:
        pico_manual_brew_program();
        return;
    case PICO_ACTION_UtilitiesMenu:
        pico_utilities_menu();
        return;
    case PICO_ACTION_SettingsMenu:
        pico_settings_menu();
        return;
    default:
        pico_log_serial("E", "MENU", "Unsupported action");
        return;
    }
}

/* Energia loop wrapper tail-calls main menu body.
 * Firmware address: 0x20012e9c. */
/* Firmware entry: 0x20012e9c. The four-byte entry tail-calls the menu loop. */
void pico_loop(void)
{
    pico_loop_body();
}

/* Application initialization, build logging, stored preferences, connectivity, restart and update
 * handling. Firmware address: 0x200131bc. */
void pico_setup(void)
{
    char wi_fi_status_result;
    int wait_gpio19_low_result;
    char buffer_88[128];

    HardwareSerial_begin(0x20036664, &g_main_unknown_01);
    Print_println_string(&global_serial0, "");
    Print_println_string(&global_serial0,
                         "***************************************************************");
    Print_println_string(&global_serial0, "                        Pico PRO");
    Print_println_string(&global_serial0,
                         "***************************************************************");
    libc_snprintf(buffer_88, 0x80, "%s = %s %s", "DATE_TIME", "Dec 16 2019", "12:59:40");
    pico_log_serial("I", "MAIN", buffer_88);
    libc_snprintf(buffer_88, 0x80, "%s = %s", "VERSION_STR", "0.1.34");
    pico_log_serial("I", "MAIN", buffer_88);
    libc_snprintf(buffer_88, 0x80, "%s = %s", "GIT_HASH",
                  "d0da766714f2012e98f8e43ecd6519895caf565e");
    pico_log_serial("I", "MAIN", buffer_88);
    libc_snprintf(buffer_88, 0x80, "%s = %s", "GIT_BRANCH", "dev");
    pico_log_serial("I", "MAIN", buffer_88);
    libc_snprintf(buffer_88, 0x80, "%s = %s", "SSD_TYPE", "SSD1325");
    pico_log_serial("I", "MAIN", buffer_88);
    pico_display_initialize();
    pico_screen_splash();
    pico_actuators_initialize();
    pico_temperature_inputs_initialize();
    pico_encoder_initialize((PicoEncoder *)&g_encoder, ' ', '\x1f', '\x13', '\x13', ' ', '\x1f');
    pico_spi_chip_selects_initialize();
    pico_step_filter_gpio_initialize();
    pico_storage_load_user_preferences();
    pico_screen_splash();
    pico_control_timers_initialize();
    pico_wifi_ensure_started_thunk();
    wait_gpio19_low_result = pico_wait_gpio19_low();
    if (wait_gpio19_low_result != 0) {
        delay(2000);
        pico_detect_stepper_configuration();
        pico_encoder_clear_click((PicoEncoder *)&g_encoder);
        pico_screen_device_information();
        pico_rfid_initialize();
        pico_ui_service_menu();
        pico_display_clear();
        pico_screen_splash();
        PRCMMCUReset(true);
    }
    if (g_offline_mode == '\0') {
        pico_wifi_ensure_connection();
        wi_fi_status_result = WiFi_status();
        if (wi_fi_status_result == '\x03') {
            do {
                wait_gpio19_low_result = pico_api_register_device();
            } while (wait_gpio19_low_result == 0);
        } else {
            pico_report_error(8, 1, 0);
        }
    }
    pico_api_report_device_state();
    pico_detect_stepper_configuration();
    pico_rfid_initialize();
    wait_gpio19_low_result = pico_parse_restart_session(&g_active_program_context_as_uint32_t);
    if (wait_gpio19_low_result != 0) {
        pico_recovery_delete_state();
        wait_gpio19_low_result = pico_ui_confirm_continue_or_abort();
        if (wait_gpio19_low_result == 0)
            goto branch_1;
        if (*(int *)(g_active_program_context_as_uint32_t + 0x30) == 1) {
            pico_screen_loading(0);
            pico_run_control_program(7);
        } else {
            pico_run_recipe_steps(&g_active_program_context_as_uint32_t, 1);
        }
        pico_run_rinse_cycle(&g_active_program_context_as_uint32_t);
    }
    pico_recovery_delete_state();
branch_1:
    pico_update_firmware();
    pico_run_required_maintenance();
    return;
}
