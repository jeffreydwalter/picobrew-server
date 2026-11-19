/* Pico Pro 0.1.34 — menus. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Builds Turn Fan Off, Turn Fan On, Back labels; returns3.
 * Firmware address: 0x2000483c. */
uint32_t pico_menu_populate_still_fan(void)
{
    libc_strcpy(&g_menu_label_0, "Turn Fan Off");
    libc_strcpy(&g_menu_label_1, "Turn Fan On");
    libc_strcpy(&g_menu_label_2, "Back");
    return 3;
}

/* Zeroes 22 action slots and menu count.
 * Firmware address: 0x200048f0. */
/* Firmware entry: 0x200048f0. Reviewed source reconstruction. */
void pico_menu_initialize(PicoMenu *menu)
{
    for (pico_u32 index = 0; index < PICO_MENU_CAPACITY; ++index)
        menu->actions[index] = 0;
    menu->count = 0;
}

/* Bounds-checked indexed action lookup with getActionId log.
 * Firmware address: 0x20004da8. */
/* Firmware entry: 0x20004da8. Reviewed source reconstruction. */
pico_u32 pico_menu_action_at(PicoMenu *menu, int index)
{
    int valid = index >= 0 && index < (pico_i32)menu->count;
    pico_assert_nonfatal(valid, "0 <= index && index < _count");
    pico_u32 action_id = valid ? menu->actions[index] : 0;
    char log_buffer[128];
    libc_snprintf(log_buffer, sizeof log_buffer, "getActionId(%d) %d %s", index, action_id,
                  pico_action_name(action_id));
    pico_log_serial("I", "MENU", log_buffer);
    return action_id;
}

/* Validates ActionId and appends to 22-entry menu; explicit Menu::add diagnostics.
 * Firmware address: 0x20004e10. */
/* Firmware entry: 0x20004e10. Reviewed source reconstruction. */
void pico_menu_add_action(PicoMenu *menu, pico_u32 action_id)
{
    pico_assert_nonfatal(action_id < 50, "0 <= actionId && actionId < ActionId::_EnumCount");
    if (action_id >= 50) {
        pico_log_serial("E", "MENU", "Menu::add() Invalid actionId");
        return;
    }
    if (menu->count >= PICO_MENU_CAPACITY) {
        pico_log_serial("E", "MENU", "Menu::add() Max count exceeded");
        return;
    }
    menu->actions[menu->count++] = action_id;
}

/* Clears count then copies a bounded input list through menu_add_action.
 * Firmware address: 0x20004e64. */
/* Firmware entry: 0x20004e64. Reviewed source reconstruction. */
void pico_menu_set_actions(PicoMenu *menu, const pico_u32 *actions, pico_u32 count)
{
    menu->count = 0;
    pico_assert_nonfatal(actions != 0 && (pico_i32)count >= 0, "actions != NULL && count >= 0");
    if (actions == 0 || (pico_i32)count < 0)
        return;
    if (count >= PICO_MENU_CAPACITY) {
        /* The firmware logs even when count equals the exact capacity. */
        pico_log_serial("E", "MENU", "Max count exceeded");
        count = PICO_MENU_CAPACITY;
    }
    for (pico_u32 index = 0; index < count; ++index)
        pico_menu_add_action(menu, actions[index]);
    pico_assert_nonfatal(menu->count == count, "_count == count");
}

/* Loads action list and copies each localized label into22-byte display rows.
 * Firmware address: 0x20005cf0. */
void pico_menu_populate_action_labels(uint32_t *actions, uint32_t count, uint32_t language,
                                      uint32_t status_selector)
{
    uint32_t action_id;
    char *source;
    int index;

    pico_menu_set_actions((PicoMenu *)&g_active_menu, actions, count);
    for (index = 0; index < g_active_menu_count; index = index + 1) {
        action_id = pico_menu_action_at((PicoMenu *)&g_active_menu, index);
        source = pico_action_label(action_id, language, status_selector);
        pico_cstring_copy(&g_menu_label_0 + index * 0x16, 0x16, source);
    }
    return;
}

/* Copies actions/count from existing menu and localizes display rows.
 * Firmware address: 0x20005d14. */
void pico_menu_populate_from_menu(uint32_t *source_menu, uint32_t language,
                                  uint32_t status_selector)
{
    uint32_t action_id;
    char *source;
    int index;

    pico_menu_set_actions((PicoMenu *)&g_active_menu, source_menu, source_menu[0x16]);
    for (index = 0; index < g_active_menu_count; index = index + 1) {
        action_id = pico_menu_action_at((PicoMenu *)&g_active_menu, index);
        source = pico_action_label(action_id, language, status_selector);
        pico_cstring_copy(&g_menu_label_0 + index * 0x16, 0x16, source);
    }
    return;
}

/* Populates16explicit service-test labels for pumps/heater/sensors/valve/RFID.
 * Firmware address: 0x200068d8. */
uint32_t pico_menu_populate_hardware_tests(void)
{
    libc_strcpy(&g_menu_label_0, "Full System");
    libc_strcpy(&g_menu_label_1, "Shuttle Pump");
    libc_strcpy(&g_menu_label_2, "Thermoblock");
    libc_strcpy(&g_menu_label_3, "Temp Sensors");
    libc_strcpy(&g_menu_label_4, "Rotary valve");
    libc_strcpy(&g_menu_label_5, "Rotary valve w/Keg");
    libc_strcpy(&g_menu_label_6, "Alarm");
    libc_strcpy(&g_menu_label_7, "RFID Detect");
    libc_strcpy(&g_menu_label_8, "Reset Shuttle Pump");
    libc_strcpy(&g_menu_label_9, "Keg Pump");
    libc_strcpy(&g_menu_label_10, "Drain Pump");
    libc_strcpy(&g_menu_label_11, "High Torque");
    libc_strcpy(&g_menu_label_12, "RV Pressure Test");
    libc_strcpy(&g_menu_label_13, "Keg Pump Posistion");
    libc_strcpy(&g_menu_label_14, "Help info");
    libc_strcpy(&g_menu_label_15, "Back");
    return 0x10;
}

/* Appends Enter WiFi Info and Search again labels, incrementing caller count.
 * Firmware address: 0x20006a54. */
void pico_menu_append_wifi_search_actions(int *option_count)
{
    int row_index;
    char *destination;
    char *search_again_label;
    bool is_french;

    row_index = *option_count;
    if (g_language == '\x01') {
        *option_count = row_index + 1;
        libc_strcpy(&g_menu_label_0 + row_index * 0x16, "Ingresar info WiFi");
        row_index = *option_count;
        search_again_label = ">Busque nuevamente<";
        *option_count = row_index + 1;
        destination = &g_menu_label_0 + row_index * 0x16;
    } else {
        is_french = g_language == '\x02';
        *option_count = row_index + 1;
        if (is_french) {
            libc_strcpy(&g_menu_label_0 + row_index * 0x16, "Entrez les info WiFi");
            row_index = *option_count;
            search_again_label = ">Chercher de nouveau<";
            *option_count = row_index + 1;
            destination = &g_menu_label_0 + row_index * 0x16;
        } else {
            libc_strcpy(&g_menu_label_0 + row_index * 0x16, "> Enter WiFi Info <");
            row_index = *option_count;
            search_again_label = "> Search again <";
            *option_count = row_index + 1;
            destination = &g_menu_label_0 + row_index * 0x16;
        }
    }
    libc_strcpy(destination, search_again_label);
    return;
}

/* Builds WiFi/drain/circulate/test/reset/exit service menu.
 * Firmware address: 0x20007f48. */
uint32_t pico_menu_populate_service(void)
{
    char *label;

    if (g_language == '\x01') {
        libc_strcpy(&g_menu_label_0, "Verificar el Wi-Fi");
        libc_strcpy(&g_menu_label_1, "Drenar el reservorio");
        libc_strcpy(&g_menu_label_2, "Drenar un filtro");
        libc_strcpy(&g_menu_label_3, "Hacer circular");
        libc_strcpy(&g_menu_label_4, "Pico de prueba");
        label = "Reinicio por defecto";
    } else if (g_language == '\x02') {
        libc_strcpy(&g_menu_label_0, "Test sans fil");
        libc_strcpy(&g_menu_label_1, "Vider le reservoir");
        libc_strcpy(&g_menu_label_2, "Filtre de vidange");
        libc_strcpy(&g_menu_label_3, "Faire circuler");
        libc_strcpy(&g_menu_label_4, "Verificar Pico");
        label = "Factory Reset";
    } else {
        libc_strcpy(&g_menu_label_0, "Test WiFi");
        libc_strcpy(&g_menu_label_1, "Drain Reservoir");
        libc_strcpy(&g_menu_label_2, "Drain Step Filter");
        libc_strcpy(&g_menu_label_3, "Circulate");
        libc_strcpy(&g_menu_label_4, "Test Pico");
        label = "Factory Reset";
    }
    libc_strcpy(&g_menu_label_5, label);
    pico_label_exit(&g_menu_label_6);
    return 7;
}

/* Builds Connect to WiFi, Test WiFi, Status, Back menu.
 * Firmware address: 0x2000807c. */
uint32_t pico_menu_populate_network(void)
{
    char *label;

    if (g_language == '\x01') {
        libc_strcpy(&g_menu_label_0, "Conectar al WiFi");
        libc_strcpy(&g_menu_label_1, "Verificar el WiFi");
        label = "Estado";
    } else if (g_language == '\x02') {
        libc_strcpy(&g_menu_label_0, "Brancher au reseau");
        libc_strcpy(&g_menu_label_1, "Test WiFi");
        label = "Etat";
    } else {
        libc_strcpy(&g_menu_label_0, "Connect to WiFi");
        libc_strcpy(&g_menu_label_1, "Test WiFi");
        label = "Status";
    }
    libc_strcpy(&g_menu_label_2, label);
    pico_label_back(&g_menu_label_3);
    return 4;
}

/* Builds Language, Units, Network, About, Back menu.
 * Firmware address: 0x200082b0. */
uint32_t pico_menu_populate_settings(void)
{
    char *label;

    if (g_language == '\x01') {
        libc_strcpy(&g_menu_label_0, "Idioma");
        libc_strcpy(&g_menu_label_1, "Unidades");
        libc_strcpy(&g_menu_label_2, "Red");
        label = "Acerca de";
    } else {
        if (g_language != '\x02') {
            libc_strcpy(&g_menu_label_0, "Language");
            libc_strcpy(&g_menu_label_1, "Units");
            libc_strcpy(&g_menu_label_2, "Network");
            libc_strcpy(&g_menu_label_3, "About");
            libc_strcpy(&g_menu_label_4, "Back");
            return 5;
        }
        libc_strcpy(&g_menu_label_0, "Langue");
        libc_strcpy(&g_menu_label_1, "Unites");
        libc_strcpy(&g_menu_label_2, "Reseau");
        label = "A propos";
    }
    libc_strcpy(&g_menu_label_3, label);
    pico_label_back(&g_menu_label_4);
    return 5;
}

/* Builds English/Español/Francais/Back menu.
 * Firmware address: 0x200083a4. */
uint32_t pico_menu_populate_languages(void)
{
    libc_strcpy(&g_menu_label_0, "English");
    libc_strcpy(&g_menu_label_1, &g_menus_unknown_01);
    libc_strcpy(&g_menu_label_2, "Francais");
    pico_label_back(&g_menu_label_3);
    return 4;
}

/* Explicit populateInBrewOptions log; localizes active program's action list.
 * Firmware address: 0x200083e0. */
uint32_t pico_menu_populate_in_brew_options(void)
{
    int context;
    uint32_t action_count;
    char *text;
    int action_index;

    context = g_active_program_context_as_int;
    Print_println_string(&global_serial0, "populateInBrewOptions");
    for (action_index = 0;
         action_count = *(uint32_t *)(context + 0x54), action_index < (int)action_count;
         action_index = action_index + 1) {
        text = pico_action_label(*(uint32_t *)(*(int *)(context + 0x50) + action_index * 4),
                                 (uint32_t)g_language_as_uint8_t, (uint32_t)g_detailed_status);
        Print_println_string(&global_serial0, text);
        libc_strcpy(&g_menu_label_0 + action_index * 0x16, text);
    }
    return action_count & 0xff;
}

/* Sets selected menu row to localized Load More label.
 * Firmware address: 0x200086bc. */
void pico_menu_set_load_more_label(int row_index)
{
    char *label;

    if (g_language == '\x01') {
        label = ">Cargue Mas<";
    } else if (g_language == '\x02') {
        label = "*Mettre davantage";
    } else {
        label = ">Load More<";
    }
    libc_strcpy(&g_menu_label_0 + row_index * 0x16, label);
    return;
}

/* Builds Drain/Hold Temp/Exit menu.
 * Firmware address: 0x200086fc. */
uint32_t pico_menu_populate_post_brew(void)
{
    char *label;

    if (g_language == '\x01') {
        libc_strcpy(&g_menu_label_0, "Drenar");
        label = "Mantenga la temp";
    } else if (g_language == '\x02') {
        libc_strcpy(&g_menu_label_0, "Drainer");
        label = "Maintenir la temp";
    } else {
        libc_strcpy(&g_menu_label_0, "Drain");
        label = "Hold Temp";
    }
    libc_strcpy(&g_menu_label_1, label);
    pico_label_exit(&g_menu_label_2);
    return 3;
}

/* Selects still fan menu, Next/Exit setup menu or Manual/Exit menu.
 * Firmware address: 0x200087bc. */
uint32_t pico_menu_populate_still_actions(int menu_kind)
{
    uint32_t option_count;

    if (menu_kind == 1) {
        option_count = pico_menu_populate_still_fan();
        return option_count;
    }
    if (menu_kind == 0) {
        pico_label_next_uppercase(&g_menu_label_0);
        pico_label_exit_uppercase(&g_menu_label_1);
    } else {
        if (menu_kind != 2) {
            return 0;
        }
        libc_strcpy(&g_menu_label_0, "Manual");
        pico_label_exit(&g_menu_label_1);
    }
    return 2;
}
