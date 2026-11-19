#include "ui_internal.h"
#include "picobrew/local_control.h"

void pico_menu_populate_from_menu(const PicoMenu *menu, uint32_t language, uint32_t status_selector)
{
    pico_menu_populate_action_labels(menu->actions, menu->count, language, status_selector);
}
void pico_ui_select_scanned_network(uint32_t index)
{
    PicoNetwork *n = pico_ui_active->network;
    n->settings->wifi_security = n->platform->scan_security(n->platform_context, index);
    ui_copy(n->settings->wifi_ssid, sizeof n->settings->wifi_ssid,
            n->platform->scan_ssid(n->platform_context, index));
    n->settings->wifi_password[0] = 0;
    if (n->settings->wifi_security != 7)
        pico_ui_enter_wifi_password();
}
uint32_t pico_ui_walk_program_steps(int program_id)
{
    uint8_t step = 0;
    for (; step < 4; step++) {
        pico_screen_still_setup_step(&step);
        uint32_t count = pico_menu_populate_still_actions(program_id);
        uint32_t choice = pico_menu_choose(7, count, 0);
        if (choice)
            return choice;
    }
    return 0;
}
bool pico_ui_confirm_screen(void (*draw_screen)(void))
{
    pico_display_clear();
    draw_screen();
    return pico_ui_confirm_at_row(7);
}
int pico_ui_confirm_thermal_recovery(int status)
{
    pico_api_error(pico_ui_active->network, status == 6 ? 21 : 20);
    pico_ui_active->services->alarm_until_press(pico_ui_active->service_context);
    pico_display_clear();
    if (status == 6)
        pico_screen_thermal_recovery_help();
    else
        pico_screen_water_error_help();
    pico_display_flush();
    pico_label_continue(pico_ui_active->menu_labels[0]);
    pico_label_exit(pico_ui_active->menu_labels[1]);
    return pico_menu_choose(7, 2, 0) == 0;
}
uint32_t pico_ui_scan_and_connect_wifi(void)
{
    PicoNetwork *n = pico_ui_active->network;
    PicoControlSignals *signals = &pico_ui_active->app->control_signals;
    pico_display_clear();
    if (!n->platform->wifi_initialized(n->platform_context) &&
        !n->platform->wifi_initialize(n->platform_context))
        return 0;
    for (;;) {
        pico_display_clear();
        pico_display_device_bitmap();
        pico_screen_wifi_searching();
        pico_display_flush();
        signals->progress_start_column = 9;
        signals->progress_row = 1;
        signals->progress_end_column = 21;
        signals->progress_glyph_index = 0;
        signals->progress_active = 1;
        int count = n->platform->scan_networks(n->platform_context);
        pico_display_stop_progress_indicator(signals);
        if ((count & 255) == 0) {
            pico_screen_wifi_no_signal();
            pico_ui_wait_for_continue();
            return 0;
        }
        /* The OLED menu owns 22 labels. Reserve two slots for manual/rescan and
         * bound SDK error/oversized results instead of corrupting adjacent RAM. */
        if (count < 0)
            count = 0;
        if (count > 20)
            count = 20;
        for (int i = 0; i < count; i++)
            ui_copy(pico_ui_active->menu_labels[i], 22,
                    n->platform->scan_ssid(n->platform_context, (unsigned)i));
        pico_menu_append_wifi_search_actions(&count);
        pico_display_clear();
        pico_ui_draw_choose_network();
        uint32_t selected = pico_menu_choose(3, (unsigned)count & 255, 0);
        if (pico_local_cancelled() || selected >= (uint32_t)count)
            return 0;
        if (selected == (uint32_t)count - 1) {
            pico_display_clear();
            continue;
        }
        if (selected == (uint32_t)count - 2)
            pico_ui_enter_manual_network();
        else
            pico_ui_select_scanned_network(selected);
        if (pico_local_cancelled())
            return 0;
        pico_display_clear();
        bool connected = pico_network_connect_with_screen(n);
        if (connected && !pico_network_resolve_server(n)) {
            pico_display_stop_progress_indicator(signals);
            pico_diagnostic_wifi();
        }
        pico_display_stop_progress_indicator(signals);
        pico_display_clear();
        if (!connected) {
            pico_display_device_bitmap_variant_b();
            pico_display_flush();
            ui_delay(1000);
            return 0;
        }
        pico_display_device_bitmap_variant_a();
        pico_display_flush();
        return 1;
    }
}
void pico_ui_network_menu(void)
{
    for (;;) {
        uint32_t count = pico_menu_populate_network();
        pico_display_clear();
        pico_ui_draw_network_heading();
        switch (pico_menu_choose(3, count, 0)) {
        case 0:
            pico_display_clear();
            pico_ui_active->network->platform->wifi_disconnect(
                pico_ui_active->network->platform_context);
            if (pico_ui_scan_and_connect_wifi()) {
                pico_screen_wifi_connected();
                pico_storage_save_wifi(pico_ui_active->network);
            } else
                pico_screen_wifi_connection_failed();
            ui_delay(1000);
            break;
        case 1:
            pico_diagnostic_wifi();
            break;
        case 2:
            pico_ui_show_network_addresses();
            break;
        default:
            return;
        }
    }
}
void pico_settings_menu(void)
{
    for (;;) {
        pico_display_clear();
        uint32_t count = pico_menu_populate_settings();
        pico_ui_draw_settings_heading();
        switch (pico_menu_choose(3, count, 0)) {
        case 0:
            pico_ui_choose_language();
            pico_storage_save_display(pico_ui_active->network);
            break;
        case 1:
            pico_ui_choose_units();
            pico_storage_save_display(pico_ui_active->network);
            break;
        case 2:
            pico_ui_network_menu();
            break;
        case 3:
            pico_screen_device_information();
            break;
        default:
            return;
        }
    }
}
int pico_ui_adjust_recipe(PicoRecipe *recipe)
{
    for (;;) {
        pico_display_clear();
        bool adjustable =
#if PICO_STANDALONE
            false; /* Local recipe steps are edited explicitly through the API. */
#else
            recipe->abv_adjustment_reference != -1 || recipe->ibu_adjustment_reference != -1;
#endif
        uint32_t count =
            pico_ui_draw_recipe_choices(pico_ui_active->app->screen.recipe_name, adjustable);
        pico_display_clear();
        pico_display_refresh_current_page();
        uint32_t choice = pico_menu_choose(7, count, 0);
        if (choice == 0)
            return 1;
        if (choice != 1 || !adjustable)
            return 0;
        bool ibu_changed = false, abv_changed = false;
        pico_display_clear();
        if (recipe->ibu_adjustment_reference != -1) {
            pico_ui_draw_bitterness_adjustment();
            recipe->ibu_adjustment = pico_encoder_select_adjustment_level(
                recipe->ibu_adjustment_reference, &recipe->ibu_adjustment, &ibu_changed);
            pico_display_clear();
        }
        if (recipe->abv_adjustment_reference != -1) {
            pico_ui_draw_abv_adjustment();
            recipe->abv_adjustment = pico_encoder_select_adjustment_level(
                recipe->abv_adjustment_reference, &recipe->abv_adjustment, &abv_changed);
        }
        pico_display_clear();
        if (ibu_changed || abv_changed) {
            pico_display_clear();
            pico_screen_loading_picopak();
            int result = pico_api_fetch_recipe(pico_ui_active->network, recipe->identifier,
                                               recipe->ibu_adjustment, recipe->abv_adjustment);
            pico_display_stop_progress_indicator(&pico_ui_active->app->control_signals);
            if (result)
                return 0;
        }
    }
}
uint32_t pico_ui_select_recipe(PicoRecipe *recipe)
{
    PicoRecipeListing list = {0};
    bool last_page = false;
#if !PICO_STANDALONE
    bool catalog = true;
#endif
    uint32_t selected = 0;
    int menu_count = 0;
    for (;;) {
        pico_display_clear();
        pico_screen_loading_picopak();
#if PICO_STANDALONE
        bool loaded = pico_api_recipe_list(pico_ui_active->network, &list, false);
        if (!loaded || !list.loaded_items) {
            pico_display_stop_progress_indicator(&pico_ui_active->app->control_signals);
            pico_display_clear();
            pico_display_center_text(0, "PicoPak recipes");
            pico_display_center_text(2, loaded ? "No saved recipes" : "Recipes unavailable");
            pico_display_center_text(4, loaded ? "Send from Hub or" : "Check local storage");
            if (loaded) pico_display_center_text(5, "add in local web app");
            pico_label_back(pico_ui_active->menu_labels[0]);
            (void)pico_menu_choose(7, 1, 0);
            return 0;
        }
#else
        if (!catalog || !pico_api_recipe_list(pico_ui_active->network, &list, false)) {
            if (!pico_api_recipe_list(pico_ui_active->network, &list, true)) {
                pico_display_stop_progress_indicator(&pico_ui_active->app->control_signals);
                pico_ui_prompt_picopak_association();
                return 0;
            }
            catalog = false;
        }
#endif
        pico_display_stop_progress_indicator(&pico_ui_active->app->control_signals);
        for (int i = 0; i < list.menu_item_count; i++)
            ui_copy(pico_ui_active->menu_labels[i], 22, list.menu_labels[i]);
        menu_count = list.menu_item_count;
        last_page = list.total_items - list.page_offset < 20;
        if (!last_page)
            pico_menu_set_load_more_label(menu_count++);
        pico_label_exit(pico_ui_active->menu_labels[menu_count++]);
        pico_display_clear();
        pico_ui_draw_picopak_selection();
        selected = pico_menu_choose(3, (unsigned)menu_count & 255, 0);
        if (pico_local_cancelled() || selected >= (unsigned)menu_count)
            return 0;
        if (selected == (unsigned)menu_count - 1) {
#if PICO_STANDALONE
            return 0;
#else
            if (!catalog)
                return 0;
            catalog = false;
            continue;
#endif
        }
        if (!last_page && selected == (unsigned)menu_count - 2) {
            list.page_offset += (int)selected;
            continue;
        }
        break;
    }
    pico_display_clear();
    pico_screen_loading_picopak();
#if PICO_STANDALONE
    if (selected >= (unsigned)list.loaded_items)
        goto failed;
    ui_copy(recipe->identifier, sizeof recipe->identifier, list.identifiers[selected]);
#else
    if (catalog) {
        if (!pico_api_create_picopak(pico_ui_active->network, atoi(list.identifiers[selected])))
            goto failed;
    } else {
        char title[22];
        ui_copy(title, sizeof title, pico_ui_active->menu_labels[selected]);
        unsigned matches = 0;
        for (unsigned i = selected; i < (unsigned)list.loaded_items; i++)
            if (!strcmp(list.titles[i], title))
                ui_copy(pico_ui_active->menu_labels[matches++], 22, list.identifiers[i]);
        if (!matches)
            goto failed;
        if (matches < 2)
            selected = matches - 1;
        else {
            pico_display_stop_progress_indicator(&pico_ui_active->app->control_signals);
            pico_display_clear();
            pico_ui_draw_picopak_selection();
            pico_label_exit(pico_ui_active->menu_labels[matches]);
            /* Preserve original count behavior: the last matched ID is the
             * cancellation index; the separately written Exit label is hidden. */
            selected = pico_menu_choose(3, matches & 255, 0);
            if (selected == matches - 1)
                return 0;
        }
        ui_copy(recipe->identifier, sizeof recipe->identifier,
                pico_ui_active->menu_labels[selected]);
    }
#endif
    if (!pico_ui_active->services->fetch_default_recipe_failed(pico_ui_active->service_context,
                                                               recipe->identifier)) {
        pico_display_stop_progress_indicator(&pico_ui_active->app->control_signals);
        return 1;
    }
failed:
    pico_display_stop_progress_indicator(&pico_ui_active->app->control_signals);
    return 0;
}
