/* Pico Pro 0.1.34 — actions. */
#include "pico/firmware.h"
#include <stdarg.h>

/* ActionId name table with explicit bounds assertion.
 * Firmware address: 0x20004400. */
char *pico_action_name(uint32_t action_id)
{
    char *action_name;

    pico_assert_nonfatal((uint32_t)(action_id < 0x33),
                         "0 <= actionId && actionId <= ActionId::_EnumCount");
    if (action_id < 0x33) {
        action_name = pico_action_names[action_id];
    } else {
        action_name = "<actionEnum>";
    }
    return action_name;
}

/* Localized label table; ToggleStatus resolves to detailed/basic status.
 * Firmware address: 0x2000442c. */
char *pico_action_label(uint32_t action_id, uint32_t language, uint32_t status_selector)
{
    pico_assert_nonfatal((uint32_t)(action_id < 0x33),
                         "0 <= actionId && actionId <= ActionId::_EnumCount");
    pico_assert_nonfatal((uint32_t)(language < 3), "0 <= language && language <= 2");
    if ((0x32 < action_id) || (2 < language)) {
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
