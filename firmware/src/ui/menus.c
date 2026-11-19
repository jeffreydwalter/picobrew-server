#include "ui_internal.h"

uint32_t pico_menu_populate_still_fan(void)
{
    strcpy(pico_ui_active->menu_labels[0], "Turn Fan Off");
    strcpy(pico_ui_active->menu_labels[1], "Turn Fan On");
    strcpy(pico_ui_active->menu_labels[2], "Back");
    return 3;
}

void pico_menu_initialize(PicoMenu *menu)
{
    for (uint32_t index = 0; index < PICO_MENU_CAPACITY; ++index)
        menu->actions[index] = 0;
    menu->count = 0;
}

uint32_t pico_menu_action_at(PicoMenu *menu, int index)
{
    int valid = index >= 0 && index < (int32_t)menu->count;
    ui_assert(valid, "0 <= index && index < _count");
    uint32_t action_id = valid ? menu->actions[index] : 0;
    char log_buffer[128];
    snprintf(log_buffer, sizeof log_buffer, "getActionId(%d) %d %s", index, (int)action_id,
             pico_action_name(action_id));
    ui_log("I", "MENU", log_buffer);
    return action_id;
}

void pico_menu_add_action(PicoMenu *menu, uint32_t action_id)
{
    ui_assert(action_id < 50, "0 <= actionId && actionId < ActionId::_EnumCount");
    if (action_id >= 50) {
        ui_log("E", "MENU", "Menu::add() Invalid actionId");
        return;
    }
    if (menu->count >= PICO_MENU_CAPACITY) {
        ui_log("E", "MENU", "Menu::add() Max count exceeded");
        return;
    }
    menu->actions[menu->count++] = action_id;
}

void pico_menu_set_actions(PicoMenu *menu, const uint32_t *actions, uint32_t count)
{
    menu->count = 0;
    ui_assert(actions != 0 && (int32_t)count >= 0, "actions != NULL && count >= 0");
    if (actions == 0 || (int32_t)count < 0)
        return;
    if (count >= PICO_MENU_CAPACITY) {
        /* The firmware logs even when count equals the exact capacity. */
        ui_log("E", "MENU", "Max count exceeded");
        count = PICO_MENU_CAPACITY;
    }
    for (uint32_t index = 0; index < count; ++index)
        pico_menu_add_action(menu, actions[index]);
    ui_assert(menu->count == count, "_count == count");
}

void pico_menu_populate_action_labels(const uint32_t *actions, uint32_t count, uint32_t language,
                                      uint32_t status_selector)
{
    uint32_t action_id;
    const char *source;
    int index;

    pico_menu_set_actions((PicoMenu *)&pico_ui_active->app->menu, actions, count);
    for (index = 0; (uint32_t)index < pico_ui_active->app->menu.count; index = index + 1) {
        action_id = pico_menu_action_at((PicoMenu *)&pico_ui_active->app->menu, index);
        source = pico_action_label(action_id, language, status_selector);
        ui_cstring_copy(pico_ui_active->menu_labels[index], 22, source);
    }
    return;
}

uint32_t pico_menu_populate_hardware_tests(void)
{
    strcpy(pico_ui_active->menu_labels[0], "Full System");
    strcpy(pico_ui_active->menu_labels[1], "Shuttle Pump");
    strcpy(pico_ui_active->menu_labels[2], "Thermoblock");
    strcpy(pico_ui_active->menu_labels[3], "Temp Sensors");
    strcpy(pico_ui_active->menu_labels[4], "Rotary valve");
    strcpy(pico_ui_active->menu_labels[5], "Rotary valve w/Keg");
    strcpy(pico_ui_active->menu_labels[6], "Alarm");
    strcpy(pico_ui_active->menu_labels[7], "RFID Detect");
    strcpy(pico_ui_active->menu_labels[8], "Reset Shuttle Pump");
    strcpy(pico_ui_active->menu_labels[9], "Keg Pump");
    strcpy(pico_ui_active->menu_labels[10], "Drain Pump");
    strcpy(pico_ui_active->menu_labels[11], "High Torque");
    strcpy(pico_ui_active->menu_labels[12], "RV Pressure Test");
    strcpy(pico_ui_active->menu_labels[13], "Keg Pump Posistion");
    strcpy(pico_ui_active->menu_labels[14], "Help info");
    strcpy(pico_ui_active->menu_labels[15], "Back");
    return 0x10;
}

void pico_menu_append_wifi_search_actions(int *option_count)
{
    int row_index;
    char *destination;
    char *search_again_label;
    bool is_french;

    row_index = *option_count;
    if (pico_ui_active->network->settings->language == '\x01') {
        *option_count = row_index + 1;
        strcpy(pico_ui_active->menu_labels[0] + row_index * 0x16, "Ingresar info WiFi");
        row_index = *option_count;
        search_again_label = ">Busque nuevamente<";
        *option_count = row_index + 1;
        destination = pico_ui_active->menu_labels[0] + row_index * 0x16;
    } else {
        is_french = pico_ui_active->network->settings->language == '\x02';
        *option_count = row_index + 1;
        if (is_french) {
            strcpy(pico_ui_active->menu_labels[0] + row_index * 0x16, "Entrez les info WiFi");
            row_index = *option_count;
            search_again_label = ">Chercher de nouveau<";
            *option_count = row_index + 1;
            destination = pico_ui_active->menu_labels[0] + row_index * 0x16;
        } else {
            strcpy(pico_ui_active->menu_labels[0] + row_index * 0x16, "> Enter WiFi Info <");
            row_index = *option_count;
            search_again_label = "> Search again <";
            *option_count = row_index + 1;
            destination = pico_ui_active->menu_labels[0] + row_index * 0x16;
        }
    }
    strcpy(destination, search_again_label);
    return;
}

uint32_t pico_menu_populate_service(void)
{
    if (pico_ui_active->network->settings->language == '\x01') {
        strcpy(pico_ui_active->menu_labels[0], "Verificar el Wi-Fi");
        strcpy(pico_ui_active->menu_labels[1], "Drenar el reservorio");
        strcpy(pico_ui_active->menu_labels[2], "Drenar un filtro");
        strcpy(pico_ui_active->menu_labels[3], "Hacer circular");
        strcpy(pico_ui_active->menu_labels[4], "Pico de prueba");
    } else if (pico_ui_active->network->settings->language == '\x02') {
        strcpy(pico_ui_active->menu_labels[0], "Test sans fil");
        strcpy(pico_ui_active->menu_labels[1], "Vider le reservoir");
        strcpy(pico_ui_active->menu_labels[2], "Filtre de vidange");
        strcpy(pico_ui_active->menu_labels[3], "Faire circuler");
        strcpy(pico_ui_active->menu_labels[4], "Verificar Pico");
    } else {
        strcpy(pico_ui_active->menu_labels[0], "Test WiFi");
        strcpy(pico_ui_active->menu_labels[1], "Drain Reservoir");
        strcpy(pico_ui_active->menu_labels[2], "Drain Step Filter");
        strcpy(pico_ui_active->menu_labels[3], "Circulate");
        strcpy(pico_ui_active->menu_labels[4], "Test Pico");
    }
#if PICO_STANDALONE
    /* Stock factory recovery downloads its replacement from PicoBrew. */
    pico_label_exit(pico_ui_active->menu_labels[5]);
    return 6;
#else
    const char *label = pico_ui_active->network->settings->language == '\x01'
                            ? "Reinicio por defecto" : "Factory Reset";
    strcpy(pico_ui_active->menu_labels[5], label);
    pico_label_exit(pico_ui_active->menu_labels[6]);
    return 7;
#endif
}

uint32_t pico_menu_populate_network(void)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        strcpy(pico_ui_active->menu_labels[0], "Conectar al WiFi");
        strcpy(pico_ui_active->menu_labels[1], "Verificar el WiFi");
        label = "Estado";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        strcpy(pico_ui_active->menu_labels[0], "Brancher au reseau");
        strcpy(pico_ui_active->menu_labels[1], "Test WiFi");
        label = "Etat";
    } else {
        strcpy(pico_ui_active->menu_labels[0], "Connect to WiFi");
        strcpy(pico_ui_active->menu_labels[1], "Test WiFi");
        label = "Status";
    }
    strcpy(pico_ui_active->menu_labels[2], label);
    pico_label_back(pico_ui_active->menu_labels[3]);
    return 4;
}

uint32_t pico_menu_populate_settings(void)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        strcpy(pico_ui_active->menu_labels[0], "Idioma");
        strcpy(pico_ui_active->menu_labels[1], "Unidades");
        strcpy(pico_ui_active->menu_labels[2], "Red");
        label = "Acerca de";
    } else {
        if (pico_ui_active->network->settings->language != '\x02') {
            strcpy(pico_ui_active->menu_labels[0], "Language");
            strcpy(pico_ui_active->menu_labels[1], "Units");
            strcpy(pico_ui_active->menu_labels[2], "Network");
            strcpy(pico_ui_active->menu_labels[3], "About");
            strcpy(pico_ui_active->menu_labels[4], "Back");
            return 5;
        }
        strcpy(pico_ui_active->menu_labels[0], "Langue");
        strcpy(pico_ui_active->menu_labels[1], "Unites");
        strcpy(pico_ui_active->menu_labels[2], "Reseau");
        label = "A propos";
    }
    strcpy(pico_ui_active->menu_labels[3], label);
    pico_label_back(pico_ui_active->menu_labels[4]);
    return 5;
}

uint32_t pico_menu_populate_languages(void)
{
    strcpy(pico_ui_active->menu_labels[0], "English");
    strcpy(pico_ui_active->menu_labels[1], "Espa\244ol");
    strcpy(pico_ui_active->menu_labels[2], "Francais");
    pico_label_back(pico_ui_active->menu_labels[3]);
    return 4;
}

uint32_t pico_menu_populate_in_brew_options(void)
{
    const PicoProgramContext *program = pico_ui_active->app->recipe.program;
    pico_ui_active->network->platform->serial_write(pico_ui_active->network->platform_context,
                                                    "populateInBrewOptions\r\n");
    for (unsigned index = 0; index < program->action_count && index < PICO_MENU_CAPACITY; index++) {
        const char *label =
            pico_action_label(program->actions[index], pico_ui_active->network->settings->language,
                              pico_ui_active->network->settings->detailed_status);
        pico_ui_active->network->platform->serial_write(pico_ui_active->network->platform_context,
                                                        label);
        pico_ui_active->network->platform->serial_write(pico_ui_active->network->platform_context,
                                                        "\r\n");
        ui_copy(pico_ui_active->menu_labels[index], 22, label);
    }
    return program->action_count & 255;
}

void pico_menu_set_load_more_label(int row_index)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = ">Cargue Mas<";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "*Mettre davantage";
    } else {
        label = ">Load More<";
    }
    strcpy(pico_ui_active->menu_labels[0] + row_index * 0x16, label);
    return;
}

uint32_t pico_menu_populate_post_brew(void)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        strcpy(pico_ui_active->menu_labels[0], "Drenar");
        label = "Mantenga la temp";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        strcpy(pico_ui_active->menu_labels[0], "Drainer");
        label = "Maintenir la temp";
    } else {
        strcpy(pico_ui_active->menu_labels[0], "Drain");
        label = "Hold Temp";
    }
    strcpy(pico_ui_active->menu_labels[1], label);
    pico_label_exit(pico_ui_active->menu_labels[2]);
    return 3;
}

uint32_t pico_menu_populate_still_actions(int menu_kind)
{
    uint32_t option_count;

    if (menu_kind == 1) {
        option_count = pico_menu_populate_still_fan();
        return option_count;
    }
    if (menu_kind == 0) {
        pico_label_next_uppercase(pico_ui_active->menu_labels[0]);
        pico_label_exit_uppercase(pico_ui_active->menu_labels[1]);
    } else {
        if (menu_kind != 2) {
            return 0;
        }
        strcpy(pico_ui_active->menu_labels[0], "Manual");
        pico_label_exit(pico_ui_active->menu_labels[1]);
    }
    return 2;
}
