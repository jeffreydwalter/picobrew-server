#ifndef PICO_MENUS_H
#define PICO_MENUS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint32_t pico_menu_populate_still_fan(void);
void pico_menu_initialize(PicoMenu *menu);
pico_u32 pico_menu_action_at(PicoMenu *menu, int index);
void pico_menu_add_action(PicoMenu *menu, pico_u32 action_id);
void pico_menu_set_actions(PicoMenu *menu, const pico_u32 *actions, pico_u32 count);
void pico_menu_populate_action_labels(uint32_t *actions, uint32_t count, uint32_t language,
                                      uint32_t status_selector);
void pico_menu_populate_from_menu(uint32_t *source_menu, uint32_t language,
                                  uint32_t status_selector);
uint32_t pico_menu_populate_hardware_tests(void);
void pico_menu_append_wifi_search_actions(int *option_count);
uint32_t pico_menu_populate_service(void);
uint32_t pico_menu_populate_network(void);
uint32_t pico_menu_populate_settings(void);
uint32_t pico_menu_populate_languages(void);
uint32_t pico_menu_populate_in_brew_options(void);
void pico_menu_set_load_more_label(int row_index);
uint32_t pico_menu_populate_post_brew(void);
uint32_t pico_menu_populate_still_actions(int menu_kind);

#endif /* PICO_MENUS_H */
