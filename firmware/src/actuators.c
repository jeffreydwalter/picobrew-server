#include "picobrew/actuators.h"
#include "picobrew/local_control.h"

/* These operations retain the original I/O order and application interlock.
 * Original entries and validation coverage are recorded in the recovery map.
 */
static void write_pin(PicoActuators *actuators, unsigned pin, unsigned value)
{
    if (value && pico_local_cancelled())
        return;
    actuators->ops->digital_write(actuators->context, pin, value);
}

void pico_actuators_initialize(PicoActuators *actuators)
{
    actuators->ops->pin_mode(actuators->context, PICO_PIN_STEAM_PUMP, 1);
    actuators->ops->pin_mode(actuators->context, PICO_PIN_DRAIN_PUMP, 1);
    actuators->ops->pin_mode(actuators->context, PICO_PIN_SHUTTLE_PUMP, 1);
    actuators->ops->pin_mode(actuators->context, PICO_PIN_HEATER, 1);
    write_pin(actuators, PICO_PIN_HEATER, 0);
    write_pin(actuators, PICO_PIN_STEAM_PUMP, 0);
    write_pin(actuators, PICO_PIN_DRAIN_PUMP, 0);
    write_pin(actuators, PICO_PIN_SHUTTLE_PUMP, 0);
}

bool pico_filter_is_inserted(PicoActuators *actuators)
{
    if (actuators->ops->digital_read(actuators->context, PICO_PIN_STEP_FILTER) == 0)
        return true;
    actuators->ops->delay_microseconds(actuators->context, 100);
    return actuators->ops->digital_read(actuators->context, PICO_PIN_STEP_FILTER) == 0;
}

static void pump_on(PicoActuators *actuators, unsigned pin)
{
    if (pico_local_cancelled())
        return;
    /* Phase 4 is the original idle state. The pause handler is called before
     * the GPIO write and may block until the filter is restored and UI resumes.
     */
    if (actuators->ops->current_phase(actuators->context) != 4 &&
        !pico_filter_is_inserted(actuators)) {
#if PICO_STANDALONE
        if (!pico_local_control_foreground()) {
            pico_local_control_trip_filter();
            pico_actuators_all_off(actuators);
            return;
        }
#endif
        actuators->ops->pause_until_resume(actuators->context);
    }
    write_pin(actuators, pin, 1);
}

void pico_actuator_heater_on(PicoActuators *a)
{
    write_pin(a, PICO_PIN_HEATER, 1);
}
void pico_actuator_heater_off(PicoActuators *a)
{
    write_pin(a, PICO_PIN_HEATER, 0);
}
void pico_actuator_steam_on(PicoActuators *a)
{
    pump_on(a, PICO_PIN_STEAM_PUMP);
}
void pico_actuator_steam_off(PicoActuators *a)
{
    write_pin(a, PICO_PIN_STEAM_PUMP, 0);
}
void pico_actuator_drain_on(PicoActuators *a)
{
    pump_on(a, PICO_PIN_DRAIN_PUMP);
}
void pico_actuator_drain_off(PicoActuators *a)
{
    write_pin(a, PICO_PIN_DRAIN_PUMP, 0);
}
void pico_actuator_shuttle_on(PicoActuators *a)
{
    pump_on(a, PICO_PIN_SHUTTLE_PUMP);
}
void pico_actuator_shuttle_off(PicoActuators *a)
{
    write_pin(a, PICO_PIN_SHUTTLE_PUMP, 0);
}

bool pico_actuator_drain_is_on(PicoActuators *actuators)
{
    return actuators->ops->digital_read(actuators->context, PICO_PIN_DRAIN_PUMP) != 0;
}

void pico_actuators_steam_and_drain_off(PicoActuators *actuators)
{
    write_pin(actuators, PICO_PIN_STEAM_PUMP, 0);
    write_pin(actuators, PICO_PIN_DRAIN_PUMP, 0);
}

void pico_actuators_all_off(PicoActuators *actuators)
{
    write_pin(actuators, PICO_PIN_HEATER, 0);
    write_pin(actuators, PICO_PIN_SHUTTLE_PUMP, 0);
    write_pin(actuators, PICO_PIN_STEAM_PUMP, 0);
    write_pin(actuators, PICO_PIN_DRAIN_PUMP, 0);
}

void pico_actuators_off_except_drain(PicoActuators *actuators)
{
    write_pin(actuators, PICO_PIN_HEATER, 0);
    write_pin(actuators, PICO_PIN_SHUTTLE_PUMP, 0);
    write_pin(actuators, PICO_PIN_STEAM_PUMP, 0);
}

void pico_actuators_start_drain_and_steam(PicoActuators *actuators)
{
    if (pico_local_cancelled())
        return;
    if (actuators->ops->current_phase(actuators->context) != 4 &&
        !pico_filter_is_inserted(actuators))
        actuators->ops->pause_until_resume(actuators->context);
    write_pin(actuators, PICO_PIN_DRAIN_PUMP, 1);
#if PICO_STANDALONE
    pico_local_control_delay(500);
#else
    actuators->ops->delay_milliseconds(actuators->context, 500);
#endif
    write_pin(actuators, PICO_PIN_STEAM_PUMP, 1);
}

bool pico_actuators_pause_for_missing_filter(PicoActuators *actuators, bool resume_steam,
                                             bool resume_drain, bool resume_shuttle)
{
    if (pico_local_cancelled())
        return false;
    if (pico_filter_is_inserted(actuators))
        return false;
    pico_actuator_heater_off(actuators);
    pico_actuators_steam_and_drain_off(actuators);
    pico_actuator_shuttle_off(actuators);
    actuators->ops->prompt_insert_filter(actuators->context);
    if (resume_steam)
        pico_actuator_steam_on(actuators);
    if (resume_drain)
        pico_actuator_drain_on(actuators);
    if (resume_shuttle)
        pico_actuator_shuttle_on(actuators);
    return true;
}
