#include "ui_internal.h"
#include "picobrew/local_control.h"

#if !PICO_STANDALONE
static const uint32_t ui_utilities_actions[10] = {40, 41, 42, 43, 44, 45, 46, 48, 47, 39};
#endif

void pico_ui_draw_menu_items(uint32_t item_count, int selected_row, int first_row,
                             uint32_t first_visible_item)
{
    short display_row;
    short y;
    const uint8_t *bitmap;
    uint32_t index;

    ui_log("I", "MEN", "drawItems");
    for (index = first_visible_item; index < item_count; index = (index + 1) & 0xff) {
        pico_ui_active->menu_labels[index][21] = 0;
        display_row = (short)index - (short)first_visible_item;
        if (index + (uint32_t)first_row == (uint32_t)selected_row) {
            display_row = display_row + (short)first_row;
            y = display_row * 8;
            pico_graphics_fill_rectangle(&pico_ui_active->display, 0, y, 0x80, 8, 0);
            pico_graphics_fill_rectangle(&pico_ui_active->display, 8, y, 0x70, 8, 1);
            pico_graphics_bitmap(&pico_ui_active->display, 0, y,
                                 (const uint8_t *)"\x1f?\x7f\x7f\x7f\x7f?\x1f", 8, 8, 1);
            pico_graphics_text_color(&pico_ui_active->display, 0);
            pico_display_center_text(display_row, pico_ui_active->menu_labels[0] + index * 0x16);
            pico_graphics_text_color(&pico_ui_active->display, 1);
            if (item_count == 1) {
                bitmap = pico_ui_bitmap_footer_single;
            } else {
                bitmap = pico_ui_bitmap_footer_multiple;
            }
            pico_graphics_bitmap(&pico_ui_active->display, 0x78, y, bitmap, 8, 8, 1);
        } else {
            display_row = (short)first_row + display_row;
            pico_graphics_fill_rectangle(&pico_ui_active->display, 0, display_row * 8, 0x80, 8, 0);
            pico_display_center_text(display_row, pico_ui_active->menu_labels[0] + index * 0x16);
        }
    }
    return;
}

uint32_t pico_menu_choose_with_timeout(uint32_t first_menu_row, uint32_t choice_count,
                                       uint32_t initial_index, int use_timeout,
                                       uint8_t default_index, uint8_t redraw, int timeout_seconds)
{
#if !PICO_STANDALONE
    bool still_fault;
#endif
    uint32_t selection_started_ms;
    int input_result;
    uint32_t now_ms;
    uint32_t encoder_position;
    uint32_t visible_row_capacity;
    uint32_t selected_row;
    uint32_t first_row;
    uint32_t first_visible_item;
#if !PICO_STANDALONE
    uint32_t last_still_poll_ms;
#endif
    uint32_t visible_item_end;

    ui_assert((uint32_t)(choice_count != 0), "numOpts > 0");
    ui_assert((uint32_t)(initial_index < choice_count), "0 <= defOption && defOption < numOpts");
    if (choice_count <= initial_index) {
        initial_index = (choice_count - 1) & 0xff;
    }
    selection_started_ms = ui_millis();
    first_row = first_menu_row & 0xff;
    visible_row_capacity = 8 - first_menu_row;
    visible_item_end = visible_row_capacity;
    if ((int)choice_count < (int)visible_row_capacity) {
        visible_item_end = choice_count;
    }
    pico_encoder_set_range((PicoEncoder *)&pico_ui_active->encoder, first_menu_row,
                           first_menu_row + choice_count, (initial_index + first_row) & 0xff);
    input_result = pico_encoder_get_position((PicoEncoder *)&pico_ui_active->encoder);
    selected_row = (input_result + 1U) & 0xff;
    pico_encoder_clear_click((PicoEncoder *)&pico_ui_active->encoder);
    first_visible_item = 0;
#if !PICO_STANDALONE
    last_still_poll_ms = selection_started_ms;
#endif
    while (true) {
        ui_millis();
#if PICO_STANDALONE
        if (pico_local_control_has_pending())
            return UINT32_MAX;
        if (pico_local_control_stopped() && pico_local_control_busy())
            return UINT32_MAX;
#endif
        if (((pico_ui_active->app->controller.phase != '\x04') &&
             (input_result = pico_filter_is_inserted(pico_ui_active->actuators),
              input_result == 0)) &&
            (pico_ui_active->app->controller.status < 6)) {
            pico_ui_active->services->pause_until_resume(pico_ui_active->service_context);
            return (uint32_t)default_index;
        }
#if !PICO_STANDALONE
        if ((uint32_t)(pico_ui_active->app->controller.status - 9) < 2) {
            now_ms = ui_millis();
            if (2000 < now_ms - last_still_poll_ms) {
                still_fault = pico_still_blocked(pico_ui_active->network);
                if (still_fault) {
                    return 0;
                }
                last_still_poll_ms = now_ms;
            }
        }
#endif
        encoder_position = pico_encoder_get_position((PicoEncoder *)&pico_ui_active->encoder);
        encoder_position = encoder_position & 0xff;
        if ((use_timeout != 0) && (now_ms = ui_millis(), (uint32_t)(timeout_seconds * 1000) <
                                                             now_ms - selection_started_ms))
            break;
        if (encoder_position == selected_row) {
            selected_row = encoder_position;
            if ((redraw != '\0') &&
                (pico_ui_active->app->carousel.pages[pico_ui_active->app->carousel.page_index] !=
                 0)) {
                pico_display_clear();
                pico_ui_active->app->carousel.pages[pico_ui_active->app->carousel.page_index]();
                pico_ui_draw_menu_items(choice_count, encoder_position, first_row,
                                        first_visible_item & 0xff);
                pico_display_flush();
            }
        } else {
            if (selected_row < encoder_position) {
                selected_row = (selected_row + 1) & 0xff;
                if ((int)(first_menu_row + choice_count) <= (int)selected_row) {
                    selected_row = (choice_count + ((first_row - 1) & 0xff)) & 0xff;
                }
            } else if (selected_row == first_menu_row) {
                if ((int)selected_row < (int)(selected_row - 1)) {
                    selected_row = first_row;
                }
            } else {
                selected_row = (selected_row - 1) & 0xff;
            }
            pico_encoder_set_position((PicoEncoder *)&pico_ui_active->encoder, selected_row);
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
            if ((redraw != '\0') &&
                (pico_ui_active->app->carousel.pages[pico_ui_active->app->carousel.page_index] !=
                 0)) {
                pico_display_clear();
                pico_ui_active->app->carousel.pages[pico_ui_active->app->carousel.page_index]();
            }
            pico_ui_draw_menu_items(choice_count, selected_row, first_row,
                                    first_visible_item & 0xff);
            pico_display_flush();
            pico_encoder_set_position((PicoEncoder *)&pico_ui_active->encoder, selected_row);
        }
        input_result = pico_encoder_take_click((PicoEncoder *)&pico_ui_active->encoder);
        if (input_result != 0) {
            input_result = pico_encoder_get_position((PicoEncoder *)&pico_ui_active->encoder);
            return (input_result - first_row) & 0xff;
        }
    }
    return (uint32_t)default_index;
}

uint32_t pico_menu_choose_default_timeout(uint32_t first_menu_row, uint32_t choice_count,
                                          uint32_t initial_index, int use_timeout,
                                          uint8_t default_index, uint8_t redraw)
{
    uint32_t menu_choose_with_timeout_result;

    menu_choose_with_timeout_result = pico_menu_choose_with_timeout(
        first_menu_row, choice_count, initial_index, use_timeout, default_index, redraw, 0x3c);
    return menu_choose_with_timeout_result;
}

uint32_t pico_menu_choose(uint32_t first_menu_row, uint32_t choice_count, uint32_t initial_index)
{
    uint32_t menu_choose_default_timeout_result;

    menu_choose_default_timeout_result = pico_menu_choose_default_timeout(
        first_menu_row, choice_count, initial_index, 0, '\0', '\0');
    return menu_choose_default_timeout_result;
}

uint32_t pico_menu_select_action(uint32_t first_menu_row, int initial_index)
{
    uint32_t menu_choose_result;

    ui_assert((uint32_t)(0 < (int)pico_ui_active->app->menu.count),
              "pico_ui_active->app->menu.getCount() > 0");
    if (initial_index < 0) {
        menu_choose_result = 0;
    } else {
        menu_choose_result = (uint32_t)(initial_index < (int)pico_ui_active->app->menu.count);
    }
    ui_assert(menu_choose_result,
              "0 <= selected && selected < pico_ui_active->app->menu.getCount()");
    if ((int)pico_ui_active->app->menu.count <= initial_index) {
        initial_index = pico_ui_active->app->menu.count - 1;
    }
    menu_choose_result = pico_menu_choose(first_menu_row, pico_ui_active->app->menu.count & 0xff,
                                          initial_index & ~(initial_index >> 0x1f) & 0xff);
    if (pico_local_cancelled())
        return UINT32_MAX;
#if PICO_STANDALONE
    if (menu_choose_result >= pico_ui_active->app->menu.count)
        return UINT32_MAX;
#endif
    menu_choose_result =
        pico_menu_action_at((PicoMenu *)&pico_ui_active->app->menu, menu_choose_result);
    return menu_choose_result;
}

bool pico_ui_choose_binary_option(const char *title)
{
    uint32_t screen_fill_reservoir_and_start_result;

    screen_fill_reservoir_and_start_result = pico_screen_fill_reservoir_and_start(title);
    screen_fill_reservoir_and_start_result =
        pico_menu_choose(7, screen_fill_reservoir_and_start_result, 0);
    return !pico_local_cancelled() && screen_fill_reservoir_and_start_result != 1;
}

bool pico_ui_confirm_at_row(uint32_t menu_row)
{
    uint32_t menu_choose_result;

    pico_label_no(pico_ui_active->menu_labels[0]);
    pico_label_yes(pico_ui_active->menu_labels[1]);
    menu_choose_result = pico_menu_choose(menu_row, 2, 0);
    return menu_choose_result == 1;
}

void pico_ui_service_menu(void)
{
#if !PICO_STANDALONE
    bool ui_confirm_at_row_result;
    int mcu_image1_exists_result;
#endif
    uint32_t menu_populate_service_result;

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
            pico_ui_active->services->drain_reservoir(pico_ui_active->service_context);
            break;
        case 2:
            pico_ui_active->services->drain_step_filter(pico_ui_active->service_context);
            break;
        case 3:
            pico_ui_active->services->run_circulation(pico_ui_active->service_context);
            break;
        case 4:
            pico_diagnostics_menu();
            break;
#if !PICO_STANDALONE
        case 5:
            pico_display_clear();
            pico_ui_draw_firmware_reset_question();
            ui_confirm_at_row_result = pico_ui_confirm_at_row(6);
            if (ui_confirm_at_row_result) {
                pico_display_clear();
                pico_ui_draw_clear_wifi_question();
                ui_confirm_at_row_result = pico_ui_confirm_at_row(6);
                if (ui_confirm_at_row_result) {
                    pico_storage_delete_wifi(pico_ui_active->network);
                }
                pico_display_clear();
                pico_screen_downloading_firmware();
                pico_display_flush();
                mcu_image1_exists_result =
                    pico_storage_recovery_image_exists(pico_ui_active->network);
                if (mcu_image1_exists_result == 0) {
                    pico_display_clear();
                    pico_display_center_text(2, "No Default Image");
                    pico_display_center_text(3, "please contact");
                    pico_display_center_text(4, "support");
                    pico_ui_wait_for_continue();
                } else {
                    pico_storage_delete_user_files(pico_ui_active->network);
                    ui_delay(1000);
                    pico_ui_active->network->platform->reset_mcu(
                        pico_ui_active->network->platform_context);
                }
            }
            break;
#endif
        default:
            return;
        }
    } while (true);
}

void pico_utilities_menu(void)
{
    uint32_t menu_select_action_result;

    do {
        pico_display_clear();
        pico_ui_draw_utilities_heading();
#if PICO_STANDALONE
        static const uint32_t local_actions[] = {40, 41, 42, 43, 44, 45, 46, 48, 39};
        pico_menu_populate_action_labels(local_actions, 9,
            pico_ui_active->network->settings->language,
            pico_ui_active->network->settings->detailed_status);
#else
        pico_menu_populate_action_labels(
            (uint32_t *)&ui_utilities_actions, 10,
            (uint32_t)pico_ui_active->network->settings->language,
            (uint32_t)pico_ui_active->network->settings->detailed_status);
#endif
        menu_select_action_result = pico_menu_select_action(3, 0);
        switch (menu_select_action_result) {
        case 0x27:
            return;
        case 0x28:
            pico_ui_active->services->run_first_rinse(pico_ui_active->service_context);
            break;
        case 0x29:
            pico_ui_active->services->run_rinse(pico_ui_active->service_context);
            break;
        case 0x2a:
            pico_ui_active->services->run_circulation_program(pico_ui_active->service_context);
            break;
        case 0x2b:
            pico_ui_active->services->run_rack_beer(pico_ui_active->service_context);
            break;
        case 0x2c:
            pico_ui_active->services->drain_step_filter(pico_ui_active->service_context);
            break;
        case 0x2d:
            pico_ui_active->services->drain_reservoir(pico_ui_active->service_context);
            break;
        case 0x2e:
            pico_ui_active->services->run_descale(pico_ui_active->service_context);
            break;
        case 0x2f:
#if !PICO_STANDALONE
            pico_ui_active->services->run_still(pico_ui_active->service_context);
#endif
            break;
        case 0x30:
            pico_ui_active->services->run_circulation(pico_ui_active->service_context);
            break;
        default:
            ui_log("E", "MENU", "Unsupported action");
            return;
        }
    } while (true);
}
