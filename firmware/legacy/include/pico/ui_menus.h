#ifndef PICO_UI_MENUS_H
#define PICO_UI_MENUS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_ui_draw_menu_items(uint32_t item_count, int selected_row, int first_row,
                             uint32_t first_visible_item);
uint32_t pico_menu_choose_with_timeout(uint32_t first_menu_row, uint32_t choice_count,
                                       uint32_t initial_index, int use_timeout,
                                       uint8_t default_index, uint8_t redraw, int timeout_seconds);
uint32_t pico_menu_choose_default_timeout(uint32_t first_menu_row, uint32_t choice_count,
                                          uint32_t initial_index, int use_timeout,
                                          uint8_t default_index, uint8_t redraw);
uint32_t pico_menu_choose(uint32_t first_menu_row, uint32_t choice_count, uint32_t initial_index);
uint32_t pico_menu_select_action(uint32_t first_menu_row, int initial_index);
bool pico_ui_choose_binary_option(char *argument_1);
bool pico_ui_confirm_at_row(uint32_t menu_row);
void pico_ui_network_menu(void);
void pico_settings_menu(void);
void pico_ui_service_menu(void);
void pico_utilities_menu(void);

#endif /* PICO_UI_MENUS_H */
