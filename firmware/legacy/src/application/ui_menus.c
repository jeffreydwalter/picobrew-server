/* Pico Pro 0.1.34 — ui menus. */
#include "pico/firmware.h"
#include <stdarg.h>

/* drawItems log, rendered text rows, highlight and selection arrows.
 * Firmware address: 0x2000a0a4. */
void pico_ui_draw_menu_items(uint32_t item_count, int selected_row, int first_row,
                             uint32_t first_visible_item)
{
    short short_value_1;
    short y;
    uint8_t *bitmap;
    uint32_t index;

    pico_log_serial("I", "MEN", "drawItems");
    for (index = first_visible_item; index < item_count; index = index + 1 & 0xff) {
        (&g_ui_menus_unknown_04)[index * 0x16] = 0;
        short_value_1 = (short)index - (short)first_visible_item;
        if (index + first_row == selected_row) {
            short_value_1 = short_value_1 + (short)first_row;
            y = short_value_1 * 8;
            GFX_fill_rectangle(&g_display, 0, y, 0x80, 8, 0);
            GFX_fill_rectangle(&g_display, 8, y, 0x70, 8, 1);
            GFX_draw_bitmap(&g_display, 0, y, "\x1f?\x7f\x7f\x7f\x7f?\x1f", 8, 8, 1);
            GFX_set_text_color(&g_display, 0);
            pico_display_center_text(short_value_1, &g_menu_label_0 + index * 0x16);
            GFX_set_text_color(&g_display, 1);
            if (item_count == 1) {
                bitmap = &g_ui_menus_unknown_01;
            } else {
                bitmap = &g_ui_menus_unknown_03;
            }
            GFX_draw_bitmap(&g_display, 0x78, y, bitmap, 8, 8, 1);
        } else {
            short_value_1 = (short)first_row + short_value_1;
            GFX_fill_rectangle(&g_display, 0, short_value_1 * 8, 0x80, 8, 0);
            pico_display_center_text(short_value_1, &g_menu_label_0 + index * 0x16);
        }
    }
    return;
}

/* Encoder-based scrolling selection, optional timeout, redraw and default option.
 * Firmware address: 0x2000c090. */
uint32_t pico_menu_choose_with_timeout(uint32_t first_menu_row, uint32_t choice_count,
                                       uint32_t initial_index, int use_timeout,
                                       uint8_t default_index, uint8_t redraw, int timeout_seconds)
{
    bool still_fault;
    uint32_t selection_started_ms;
    int input_result;
    uint32_t now_ms;
    uint32_t encoder_position;
    uint32_t visible_row_capacity;
    uint32_t selected_row;
    uint32_t first_row;
    uint32_t first_visible_item;
    uint32_t last_still_poll_ms;
    uint32_t visible_item_end;

    pico_assert_nonfatal((uint32_t)(choice_count != 0), "numOpts > 0");
    pico_assert_nonfatal((uint32_t)(initial_index < choice_count),
                         "0 <= defOption && defOption < numOpts");
    if (choice_count <= initial_index) {
        initial_index = choice_count - 1 & 0xff;
    }
    selection_started_ms = millis();
    first_row = first_menu_row & 0xff;
    visible_row_capacity = 8 - first_menu_row;
    visible_item_end = visible_row_capacity;
    if ((int)choice_count < (int)visible_row_capacity) {
        visible_item_end = choice_count;
    }
    pico_encoder_set_range((PicoEncoder *)&g_encoder, first_menu_row, first_menu_row + choice_count,
                           initial_index + first_row & 0xff);
    input_result = pico_encoder_get_position((PicoEncoder *)&g_encoder);
    selected_row = input_result + 1U & 0xff;
    pico_encoder_clear_click((PicoEncoder *)&g_encoder);
    first_visible_item = 0;
    last_still_poll_ms = selection_started_ms;
    while (true) {
        millis();
        if (((g_controller_brew_phase != '\x04') &&
             (input_result = pico_step_filter_is_inserted(), input_result == 0)) &&
            (g_controller_status < 6)) {
            pico_controller_pause_until_resume(0x2002d2f8);
            return (uint32_t)default_index;
        }
        if (g_controller_status - 9 < 2) {
            now_ms = millis();
            if (2000 < now_ms - last_still_poll_ms) {
                still_fault = pico_still_has_blocking_status(&g_controller);
                if (still_fault) {
                    return 0;
                }
                last_still_poll_ms = now_ms;
            }
        }
        encoder_position = pico_encoder_get_position((PicoEncoder *)&g_encoder);
        encoder_position = encoder_position & 0xff;
        if ((use_timeout != 0) &&
            (now_ms = millis(), (uint32_t)(timeout_seconds * 1000) < now_ms - selection_started_ms))
            break;
        if (encoder_position == selected_row) {
            selected_row = encoder_position;
            if ((redraw != '\0') && ((&g_carousel_page_index)[g_carousel_page_index + 4] != 0)) {
                pico_display_clear();
                (*(PicoOpaqueFunction *)(&g_carousel_page_index)[g_carousel_page_index + 4])();
                pico_ui_draw_menu_items(choice_count, encoder_position, first_row,
                                        first_visible_item & 0xff);
                pico_display_flush();
            }
        } else {
            if (selected_row < encoder_position) {
                selected_row = selected_row + 1 & 0xff;
                if ((int)(first_menu_row + choice_count) <= (int)selected_row) {
                    selected_row = choice_count + (first_row - 1 & 0xff) & 0xff;
                }
            } else if (selected_row == first_menu_row) {
                if ((int)selected_row < (int)(selected_row - 1)) {
                    selected_row = first_row;
                }
            } else {
                selected_row = selected_row - 1 & 0xff;
            }
            pico_encoder_set_position((PicoEncoder *)&g_encoder, selected_row);
            encoder_position = selected_row - first_menu_row;
            first_visible_item = encoder_position + 1;
            if ((int)(encoder_position + 1) < (int)visible_item_end) {
                first_visible_item = visible_item_end;
            }
            if ((int)choice_count < (int)first_visible_item) {
                first_visible_item = choice_count;
            }
            first_visible_item = first_visible_item + (first_menu_row - 8);
            first_visible_item = first_visible_item & ~((int)first_visible_item >> 0x1f);
            if ((int)encoder_position <= (int)first_visible_item) {
                first_visible_item = encoder_position;
            }
            visible_item_end = visible_row_capacity + first_visible_item;
            if ((int)choice_count <= (int)(visible_row_capacity + first_visible_item)) {
                visible_item_end = choice_count;
            }
            if ((redraw != '\0') && ((&g_carousel_page_index)[g_carousel_page_index + 4] != 0)) {
                pico_display_clear();
                (*(PicoOpaqueFunction *)(&g_carousel_page_index)[g_carousel_page_index + 4])();
            }
            pico_ui_draw_menu_items(choice_count, selected_row, first_row,
                                    first_visible_item & 0xff);
            pico_display_flush();
            pico_encoder_set_position((PicoEncoder *)&g_encoder, selected_row);
        }
        input_result = pico_encoder_take_click((PicoEncoder *)&g_encoder);
        if (input_result != 0) {
            input_result = pico_encoder_get_position((PicoEncoder *)&g_encoder);
            return input_result - first_row & 0xff;
        }
    }
    return (uint32_t)default_index;
}

/* Selection wrapper supplying60-second timeout.
 * Firmware address: 0x2000c2a8. */
uint32_t pico_menu_choose_default_timeout(uint32_t first_menu_row, uint32_t choice_count,
                                          uint32_t initial_index, int use_timeout,
                                          uint8_t default_index, uint8_t redraw)
{
    uint32_t menu_choose_with_timeout_result;

    menu_choose_with_timeout_result = pico_menu_choose_with_timeout(
        first_menu_row, choice_count, initial_index, use_timeout, default_index, redraw, 0x3c);
    return menu_choose_with_timeout_result;
}

/* Selection wrapper without timeout/redraw.
 * Firmware address: 0x2000c2c2. */
uint32_t pico_menu_choose(uint32_t first_menu_row, uint32_t choice_count, uint32_t initial_index)
{
    uint32_t menu_choose_default_timeout_result;

    menu_choose_default_timeout_result = pico_menu_choose_default_timeout(
        first_menu_row, choice_count, initial_index, 0, '\0', '\0');
    return menu_choose_default_timeout_result;
}

/* Selects menu row then resolves action ID via menu action table.
 * Firmware address: 0x2000c2d4. */
uint32_t pico_menu_select_action(uint32_t first_menu_row, int initial_index)
{
    uint32_t menu_choose_result;

    pico_assert_nonfatal((uint32_t)(0 < (int)g_active_menu_count_as_uint32_t),
                         "g_menu.getCount() > 0");
    if (initial_index < 0) {
        menu_choose_result = 0;
    } else {
        menu_choose_result = (uint32_t)(initial_index < (int)g_active_menu_count_as_uint32_t);
    }
    pico_assert_nonfatal(menu_choose_result, "0 <= selected && selected < g_menu.getCount()");
    if ((int)g_active_menu_count_as_uint32_t <= initial_index) {
        initial_index = g_active_menu_count_as_uint32_t - 1;
    }
    menu_choose_result = pico_menu_choose(first_menu_row, g_active_menu_count_as_uint32_t & 0xff,
                                          initial_index & ~(initial_index >> 0x1f) & 0xff);
    menu_choose_result = pico_menu_action_at((PicoMenu *)&g_active_menu, menu_choose_result);
    return menu_choose_result;
}

/* Builds a two-choice footer and returns whether selected index differs from1.
 * Firmware address: 0x2000c3fc. */
bool pico_ui_choose_binary_option(char *argument_1)
{
    uint32_t screen_fill_reservoir_and_start_result;

    screen_fill_reservoir_and_start_result = pico_screen_fill_reservoir_and_start(argument_1);
    screen_fill_reservoir_and_start_result =
        pico_menu_choose(7, screen_fill_reservoir_and_start_result, 0);
    return screen_fill_reservoir_and_start_result != 1;
}

/* Builds yes/no choices and returns selected confirmation outcome.
 * Firmware address: 0x2000f8fc. */
bool pico_ui_confirm_at_row(uint32_t menu_row)
{
    uint32_t menu_choose_result;

    pico_label_no(&g_menu_label_0);
    pico_label_yes(&g_menu_label_1);
    menu_choose_result = pico_menu_choose(menu_row, 2, 0);
    return menu_choose_result == 1;
}

/* Network menu dispatches connection setup, WiFi test and address/status display.
 * Firmware address: 0x2000ff98. */
void pico_ui_network_menu(void)
{
    uint32_t menu_populate_network_result;
    uint32_t wi_fi_disconnect_result;
    int ui_scan_and_connect_wifi_result;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r1_00;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r2_00;
    uint32_t unresolved_return_r2_01;
    uint32_t unresolved_return_r3;
    uint32_t unresolved_return_r3_00;
    uint64_t screen_wifi_connected_result;

    while (true) {
        while (true) {
            while (true) {
                menu_populate_network_result = pico_menu_populate_network();
                pico_display_clear();
                pico_ui_draw_network_heading();
                menu_populate_network_result = pico_menu_choose(3, menu_populate_network_result, 0);
                if (menu_populate_network_result != 1)
                    break;
                pico_diagnostic_wifi();
            }
            if (menu_populate_network_result != 0)
                break;
            pico_display_clear();
            wi_fi_disconnect_result = WiFi_disconnect();
            ui_scan_and_connect_wifi_result = pico_ui_scan_and_connect_wifi(
                wi_fi_disconnect_result, unresolved_return_r1, unresolved_return_r2);
            if (ui_scan_and_connect_wifi_result == 0) {
                pico_screen_wifi_connection_failed(0, unresolved_return_r1_00,
                                                   unresolved_return_r2_00, unresolved_return_r3);
            } else {
                screen_wifi_connected_result = PICO_UNRESOLVED_RETURN(pico_screen_wifi_connected());
                pico_settings_save_wifi((int)screen_wifi_connected_result,
                                        (int)((uint64_t)screen_wifi_connected_result >> 0x20),
                                        unresolved_return_r2_01, unresolved_return_r3_00);
            }
            delay(1000);
        }
        if (menu_populate_network_result != 2)
            break;
        pico_ui_show_network_addresses();
    }
    return;
}

/* Main SettingsMenu tail-call target.
 * Firmware address: 0x2000fff8. */
void pico_settings_menu(void)
{
    uint32_t menu_populate_settings_result;
    uint32_t ui_choose_language_result;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r1_00;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r2_00;
    uint32_t value_3;

branch_1:
    pico_display_clear();
    menu_populate_settings_result = pico_menu_populate_settings();
    pico_ui_draw_settings_heading();
    menu_populate_settings_result = pico_menu_choose(3, menu_populate_settings_result, 0);
    switch (menu_populate_settings_result) {
    case 0:
        ui_choose_language_result = pico_ui_choose_language();
        menu_populate_settings_result = unresolved_return_r1;
        value_3 = unresolved_return_r2;
        break;
    case 1:
        ui_choose_language_result = pico_ui_choose_units();
        menu_populate_settings_result = unresolved_return_r1_00;
        value_3 = unresolved_return_r2_00;
        break;
    case 2:
        pico_ui_network_menu();
        goto branch_1;
    case 3:
        goto switchD_20010016_caseD_3;
    default:
        return;
    }
    pico_settings_save_display(ui_choose_language_result, menu_populate_settings_result, value_3);
    goto branch_1;
switchD_20010016_caseD_3:
    pico_screen_device_information();
    goto branch_1;
}

/* Service menu includes network, reservoir/filter drain, circulation, diagnostics and factory
 * firmware reset. Firmware address: 0x2001003c. */
void pico_ui_service_menu(void)
{
    bool ui_confirm_at_row_result;
    uint32_t menu_populate_service_result;
    int mcu_image1_exists_result;

    do {
        pico_display_clear();
        menu_populate_service_result = pico_menu_populate_service();
        pico_ui_draw_current_program_heading();
        menu_populate_service_result = pico_menu_choose(3, menu_populate_service_result, 0);
        switch (menu_populate_service_result) {
        case 0:
            pico_ui_network_menu();
            break;
        case 1:
            pico_drain_reservoir();
            break;
        case 2:
            pico_drain_step_filter();
            break;
        case 3:
            pico_run_circulation(&g_wort_temperature_fahrenheit_low);
            break;
        case 4:
            pico_diagnostics_menu();
            break;
        case 5:
            pico_display_clear();
            pico_ui_draw_firmware_reset_question();
            ui_confirm_at_row_result = pico_ui_confirm_at_row(6);
            if (ui_confirm_at_row_result) {
                pico_display_clear();
                pico_ui_draw_clear_wifi_question();
                ui_confirm_at_row_result = pico_ui_confirm_at_row(6);
                if (ui_confirm_at_row_result) {
                    pico_settings_delete_wifi();
                }
                pico_display_clear();
                pico_screen_downloading_firmware();
                pico_display_flush();
                mcu_image1_exists_result = pico_mcu_image1_exists();
                if (mcu_image1_exists_result == 0) {
                    pico_display_clear();
                    pico_display_center_text(2, "No Default Image");
                    pico_display_center_text(3, "please contact");
                    pico_display_center_text(4, "support");
                    pico_ui_wait_for_continue();
                } else {
                    pico_settings_delete_user_files();
                    delay(1000);
                    PRCMMCUReset(true);
                }
            }
            break;
        default:
            return;
        }
    } while (true);
}

/* Main UtilitiesMenu tail-call target dispatching recovered action IDs.
 * Firmware address: 0x20012670. */
void pico_utilities_menu(void)
{
    uint32_t menu_select_action_result;

    do {
        pico_display_clear();
        pico_ui_draw_utilities_heading();
        pico_menu_populate_action_labels((uint32_t *)&g_ui_menus_unknown_02, 10,
                                         (uint32_t)g_language_as_uint8_t,
                                         (uint32_t)g_detailed_status);
        menu_select_action_result = pico_menu_select_action(3, 0);
        switch (menu_select_action_result) {
        case 0x27:
            return;
        case 0x28:
            pico_run_first_rinse();
            break;
        case 0x29:
            pico_run_rinse_cycle(&g_active_program_context_as_uint32_t);
            break;
        case 0x2a:
            pico_run_circulation_program();
            break;
        case 0x2b:
            pico_run_rack_beer();
            break;
        case 0x2c:
            pico_drain_step_filter();
            break;
        case 0x2d:
            pico_drain_reservoir();
            break;
        case 0x2e:
            pico_run_descale_program();
            break;
        case 0x2f:
            pico_run_still_program();
            break;
        case 0x30:
            pico_run_circulation(&g_wort_temperature_fahrenheit_low);
            break;
        default:
            pico_log_serial("E", "MENU", "Unsupported action");
            return;
        }
    } while (true);
}
