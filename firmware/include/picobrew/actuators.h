#ifndef PICOBREW_ACTUATORS_H
#define PICOBREW_ACTUATORS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Energia logical pin numbers, matched against the original firmware. */
enum {
    PICO_PIN_HEATER = 5,
    PICO_PIN_DRAIN_PUMP = 8,
    PICO_PIN_STEP_FILTER = 11,
    PICO_PIN_SHUTTLE_PUMP = 24,
    PICO_PIN_WORT_SENSOR = 25,
    PICO_PIN_THERMOBLOCK_SENSOR = 26,
    PICO_PIN_STEAM_PUMP = 27
};

typedef struct PicoActuatorOps {
    void (*pin_mode)(void *context, unsigned pin, unsigned mode);
    int (*digital_read)(void *context, unsigned pin);
    void (*digital_write)(void *context, unsigned pin, unsigned value);
    void (*delay_microseconds)(void *context, uint32_t duration);
    void (*delay_milliseconds)(void *context, uint32_t duration);
    /* Complete original pause/resume and missing-filter UI behavior must be
     * supplied by the application. These are required integration boundaries.
     */
    void (*pause_until_resume)(void *context);
    void (*prompt_insert_filter)(void *context);
    uint8_t (*current_phase)(void *context);
} PicoActuatorOps;

typedef struct PicoActuators {
    const PicoActuatorOps *ops;
    void *context;
} PicoActuators;

void pico_actuators_initialize(PicoActuators *actuators);
bool pico_filter_is_inserted(PicoActuators *actuators);
void pico_actuators_all_off(PicoActuators *actuators);
void pico_actuators_off_except_drain(PicoActuators *actuators);
void pico_actuators_steam_and_drain_off(PicoActuators *actuators);
void pico_actuator_heater_on(PicoActuators *actuators);
void pico_actuator_heater_off(PicoActuators *actuators);
void pico_actuator_steam_on(PicoActuators *actuators);
void pico_actuator_steam_off(PicoActuators *actuators);
void pico_actuator_drain_on(PicoActuators *actuators);
void pico_actuator_drain_off(PicoActuators *actuators);
void pico_actuator_shuttle_on(PicoActuators *actuators);
void pico_actuator_shuttle_off(PicoActuators *actuators);
bool pico_actuator_drain_is_on(PicoActuators *actuators);
void pico_actuators_start_drain_and_steam(PicoActuators *actuators);
bool pico_actuators_pause_for_missing_filter(PicoActuators *actuators, bool resume_steam,
                                             bool resume_drain, bool resume_shuttle);

#ifdef __cplusplus
}
#endif
#endif
