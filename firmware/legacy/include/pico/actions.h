#ifndef PICO_ACTIONS_H
#define PICO_ACTIONS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

char *pico_action_name(uint32_t action_id);
char *pico_action_label(uint32_t action_id, uint32_t language, uint32_t status_selector);

#endif /* PICO_ACTIONS_H */
