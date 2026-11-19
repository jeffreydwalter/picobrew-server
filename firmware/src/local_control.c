#include "picobrew/config.h"
#if PICO_STANDALONE
#include "picobrew/local_control.h"
#include "picobrew/local_history.h"
#include "picobrew/local_telemetry.h"
#include <limits.h>
#include <string.h>

static struct {
    PicoApplication *application;
    void (*poll)(void *);
    void *context;
    uint32_t last_poll;
    bool ready, serving, at_menu, running;
    volatile bool stopped;
    volatile bool filter_trip;
    volatile unsigned bus_selected;
    unsigned pending;
    char recipe_id[21];
    const char *active_command;
} local;

static const char *const commands[] = {
    "", "brew", "sous-vide", "cold-brew", "manual", "rinse", "first-rinse",
    "descale", "circulate", "drain-reservoir", "drain-filter", "rack-beer",
    "utilities", "settings"
};

bool pico_local_control_foreground(void)
{
#if defined(__arm__) || defined(__thumb__)
    uint32_t exception, mask;
    __asm__ volatile("mrs %0, ipsr" : "=r"(exception));
    __asm__ volatile("mrs %0, primask" : "=r"(mask));
    return exception == 0 && mask == 0;
#else
    return true;
#endif
}

void pico_local_control_bind(PicoApplication *a, void (*poll)(void *), void *context)
{
    memset(&local, 0, sizeof local);
    local.application = a;
    local.poll = poll;
    local.context = context;
    local.active_command = "idle";
}

void pico_local_control_ready(bool ready) { local.ready = ready; }
void pico_local_control_bus_select(unsigned pin, bool active)
{
    unsigned mask = pin == 4 ? 1u : pin == 18 ? 2u : 0u;
    if (active) local.bus_selected |= mask;
    else local.bus_selected &= ~mask;
}
bool pico_local_control_is_ready(void) { return local.ready; }
bool pico_local_control_stopped(void) { return local.stopped; }
const char *pico_local_control_stop_reason(void)
{
    return !local.stopped ? "none" : local.filter_trip ? "filter-removed" : "requested";
}
void pico_local_control_trip_filter(void)
{
    local.filter_trip = true;
    local.stopped = true;
    local.pending = 0;
    if (local.application && local.application->control)
        local.application->control->signals->enabled = 0;
}
bool pico_local_outputs_allowed(void) { return !local.stopped; }
bool pico_local_control_has_pending(void) { return local.pending != 0; }
bool pico_local_control_busy(void)
{
    return !local.ready || !local.at_menu || local.running || local.pending != 0;
}
const char *pico_local_control_command_name(void)
{
    return local.active_command ? local.active_command : "initializing";
}
void pico_local_control_enter_menu(void)
{
    /* Any workflow that returned without its normal history finalizer was
     * abandoned. Settings and maintenance never start a new history. */
    pico_history_finish(true);
    local.at_menu = true;
    local.running = false;
    local.active_command = "idle";
}
void pico_local_control_leave_menu(void)
{
    local.at_menu = false;
    local.active_command = "front-panel";
}

void pico_local_service(void)
{
    if (!local.ready || !local.poll || local.serving || local.bus_selected || !pico_local_control_foreground())
        return;
    PicoApplication *a = local.application;
    uint32_t now = a->hardware->millis(a->hardware_context);
    if ((uint32_t)(now - local.last_poll) < 5)
        return;
    local.last_poll = now;
    local.serving = true;
    pico_history_poll();
    local.poll(local.context);
    pico_telemetry_poll();
    local.serving = false;
}

void pico_local_control_delay(uint32_t milliseconds)
{
    PicoApplication *a = local.application;
    if (!a)
        return;
    if (!pico_local_control_foreground() || local.bus_selected) {
        a->hardware->delay_ms(a->hardware_context, milliseconds);
        return;
    }
    /* Preserve elapsed delays while allowing bounded foreground network work.
     * During a latched stop, workflow delays no longer postpone unwinding.
     */
    while (milliseconds && !local.stopped) {
        uint32_t slice = milliseconds > 10 ? 10 : milliseconds;
        a->hardware->delay_ms(a->hardware_context, slice);
        milliseconds -= slice;
        pico_local_service();
    }
}

PicoLocalControlResult pico_local_control_encoder(int delta, bool click)
{
    if (!local.ready || !local.application || !pico_local_control_foreground())
        return PICO_LOCAL_CONTROL_NOT_READY;
    if (delta < -100 || delta > 100)
        return PICO_LOCAL_CONTROL_INVALID;
    PicoEncoder *e = &local.application->ui.encoder;
    /* This operation runs only in the foreground. Protect the encoder update
     * from its rotation ISR; do not emulate GPIO edges or bypass UI choices.
     */
    local.application->hardware->interrupts_disable(local.application->hardware_context);
    int64_t next = (int64_t)e->position + (int64_t)delta;
    if (next < e->minimum) next = e->minimum;
    if (next > e->maximum) next = e->maximum;
    e->position = (int32_t)next;
    if (click) e->click_latch = 0;
    local.application->hardware->interrupts_enable(local.application->hardware_context);
    return PICO_LOCAL_CONTROL_OK;
}

static bool valid_id(const char *id)
{
    if (!id || !*id || strlen(id) > 20)
        return false;
    for (; *id; ++id)
        if (!((*id >= 'a' && *id <= 'z') || (*id >= 'A' && *id <= 'Z') ||
              (*id >= '0' && *id <= '9') || *id == '-' || *id == '_'))
            return false;
    return true;
}

PicoLocalControlResult pico_local_control_command(const char *command, const char *recipe_id)
{
    PicoApplication *a = local.application;
    if (!local.ready || !a || !pico_local_control_foreground())
        return PICO_LOCAL_CONTROL_NOT_READY;
    if (!command)
        return PICO_LOCAL_CONTROL_INVALID;
    if (!strcmp(command, "stop")) {
        /* The latch is set before stopping timers/outputs. All future output
         * writes are inhibited until a separate idle-only clear-stop request.
         */
        local.stopped = true;
        local.pending = 0;
        pico_control_stop(a->control);
        pico_actuators_all_off(a->actuators);
        return PICO_LOCAL_CONTROL_OK;
    }
    if (!strcmp(command, "clear-stop")) {
        if (pico_local_control_busy() || a->state.control_signals.enabled ||
            a->state.controller.phase != PICO_BREW_PHASE_IDLE)
            return PICO_LOCAL_CONTROL_BUSY;
        pico_actuators_all_off(a->actuators);
        local.stopped = false;
        local.filter_trip = false;
        return PICO_LOCAL_CONTROL_OK;
    }
    unsigned selection;
    for (selection = 1; selection < sizeof commands / sizeof commands[0]; ++selection)
        if (!strcmp(command, commands[selection]))
            break;
    if (selection == sizeof commands / sizeof commands[0] ||
        (selection == 1 && recipe_id && *recipe_id && !valid_id(recipe_id)))
        return PICO_LOCAL_CONTROL_INVALID;
    if (local.stopped)
        return PICO_LOCAL_CONTROL_STOPPED;
    if (pico_local_control_busy())
        return PICO_LOCAL_CONTROL_BUSY;
    local.recipe_id[0] = 0;
    if (selection == 1 && recipe_id)
        strcpy(local.recipe_id, recipe_id);
    local.pending = selection;
    /* Let the existing blocking main menu finish. The application checks the
     * pending command before executing the menu's selected action.
     */
    a->ui.encoder.click_latch = 0;
    return PICO_LOCAL_CONTROL_OK;
}

void pico_local_control_brew(PicoApplication *a, const char *id)
{
    if (!id || !*id) {
        pico_recipe_run_brew_program(&a->recipe, &a->state.recipe);
        return;
    }
    pico_recipe_program_initialize(&a->recipe, &a->state.session, 1, a->settings.language);
    a->state.recipe.program = &a->state.session;
    a->state.recipe.current_step = 0;
    for (unsigned i = 0; i < 5; ++i)
        a->state.carousel.pages[i] = a->recipe.pages->normal[i];
    a->state.carousel.page_count = 5;
    a->state.carousel.page_index = 0;
    if (pico_api_fetch_recipe(&a->network, id, -1, -1) != 0)
        return;
    if (!pico_recipe_session_start(&a->recipe, true))
        return;
    a->state.device_state = 3;
    pico_recipe_run(&a->recipe, &a->state.recipe, true);
    if (!local.stopped)
        pico_recipe_rinse(&a->recipe);
}

bool pico_local_control_run_pending(void)
{
    if (!local.pending || local.stopped)
        return false;
    unsigned command = local.pending;
    local.pending = 0;
    local.running = true;
    local.at_menu = false;
    local.active_command = commands[command];
    PicoApplication *a = local.application;
    switch (command) {
    case 1: pico_local_control_brew(a, local.recipe_id); break;
    case 2: pico_recipe_run_cooking_program(&a->recipe, 3); break;
    case 3: pico_recipe_run_cooking_program(&a->recipe, 4); break;
    case 4: pico_recipe_manual_program(&a->recipe); break;
    case 5: pico_recipe_rinse(&a->recipe); break;
    case 6: pico_recipe_first_rinse(&a->recipe); break;
    case 7: pico_recipe_descale(&a->recipe); break;
    case 8: pico_recipe_circulate(&a->recipe); break;
    case 9: pico_recipe_drain_reservoir(&a->recipe); break;
    case 10: pico_recipe_drain_filter(&a->recipe); break;
    case 11: pico_recipe_rack_beer(&a->recipe); break;
    case 12: pico_utilities_menu(); break;
    case 13: pico_settings_menu(); break;
    default: break;
    }
    if (local.stopped) {
        pico_control_stop(a->control);
        pico_actuators_all_off(a->actuators);
        a->state.controller.phase = PICO_BREW_PHASE_IDLE;
        a->state.controller.status = 0;
        a->state.device_state = 2;
    }
    local.running = false;
    pico_history_finish(true);
    local.active_command = "idle";
    return true;
}
#endif
