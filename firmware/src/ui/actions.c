#include "ui_internal.h"

const char *pico_action_name(uint32_t action_id)
{
    const char *action_name;

    ui_assert((uint32_t)(action_id < 50), "0 <= actionId && actionId <= ActionId::_EnumCount");
    if (action_id < 50) {
        action_name = pico_action_names[action_id];
    } else {
        action_name = "<actionEnum>";
    }
    return action_name;
}

const char *pico_action_label(uint32_t action_id, uint32_t language, uint32_t status_selector)
{
    ui_assert((uint32_t)(action_id < 50), "0 <= actionId && actionId <= ActionId::_EnumCount");
    ui_assert((uint32_t)(language < 3), "0 <= language && language <= 2");
    if ((49 < action_id) || (2 < language)) {
        return "<action>";
    }
    if (action_id == 9) {
        if (status_selector == 0) {
            action_id = 8;
        } else {
            action_id = 7;
        }
    }
    return pico_action_labels[action_id][language];
}
