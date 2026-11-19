#ifndef PICOBREW_STATE_H
#define PICOBREW_STATE_H

#include "picobrew/pid.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { PICO_RECIPE_STEP_CAPACITY = 15, PICO_MENU_CAPACITY = 22, PICO_CAROUSEL_CAPACITY = 15 };

typedef enum PicoBrewPhase {
    PICO_BREW_PHASE_HEATING = 0,
    PICO_BREW_PHASE_HEATED_HOLD = 1,
    PICO_BREW_PHASE_COOLING = 2,
    PICO_BREW_PHASE_DRAINING = 3,
    PICO_BREW_PHASE_IDLE = 4,
    PICO_BREW_PHASE_UNHEATED_CIRCULATION = 5,
    PICO_BREW_PHASE_COOL_THERMOBLOCK = 6,
    PICO_BREW_PHASE_UNHEATED_PUMP_HOLD = 7,
    PICO_BREW_PHASE_INDEFINITE_HEATING = 8
} PicoBrewPhase;

typedef struct PicoStillTelemetry {
    uint8_t received;
    uint8_t reserved_01[3];
    uint32_t communication_failures;
    uint8_t temperature_units;
    uint8_t reserved_09[3];
    uint32_t received_ms;
    PICO_ALIGNAS(8) double temperature1;
    PICO_ALIGNAS(8) double temperature2;
    PICO_ALIGNAS(8) double temperature3;
    PICO_ALIGNAS(8) double reading4; /* Meaning not established. */
    PICO_ALIGNAS(8) double pressure;
    uint8_t pressure_status;
    uint8_t fan_on;
    uint8_t status3; /* Meaning not established. */
    uint8_t reserved_3b[5];
    PICO_ALIGNAS(8) double temperature2_limit;
    PICO_ALIGNAS(8) double temperature2_margin;
    PICO_ALIGNAS(8) double pressure_limit;
    uint32_t communication_failure_limit;
    uint32_t reserved_5c;
} PicoStillTelemetry;

typedef struct PicoBrewController {
    PicoStillTelemetry still;
    uint8_t compartment;
    uint8_t status;
    uint8_t phase;
    char phase_label[0x15];
    PICO_ALIGNAS(8) double wort_history[6];
    uint8_t drain_subphase;
    uint8_t reserved_0a9[7];
    PICO_ALIGNAS(8) PicoPid wort_pid;
    PICO_ALIGNAS(8) PicoPid thermoblock_pid;
    uint8_t shuttle_calibration_complete;
    union {
        uint8_t calibration_flag_1a1;
        uint8_t reprime_attempted; /* First automatic retry of controller status 5. */
    };
    uint8_t reserved_1a2[2];
    uint32_t trend_seconds;
    int32_t step_seconds;
    int32_t history_count;
    PICO_ALIGNAS(8) double previous_window_wort_fahrenheit;
    PICO_ALIGNAS(8) volatile double highest_wort_fahrenheit;
    uint8_t reserved_1c0[0x10];
    PICO_ALIGNAS(8) double wort_target_fahrenheit;
    PICO_ALIGNAS(8) double thermoblock_target_fahrenheit;
    PICO_ALIGNAS(8) double wort_pid_output;
    PICO_ALIGNAS(8) double thermoblock_pid_output;
    PICO_ALIGNAS(8) volatile double wort_fahrenheit;
    PICO_ALIGNAS(8) volatile double thermoblock_fahrenheit;
    uint32_t heater_cycle_started_ms;
    uint32_t shuttle_on_time_ms;
    uint32_t shuttle_cycle_started_ms;
    uint32_t heater_on_time_ms;
    uint32_t drain_phase_started_ms;
    uint32_t reserved_214;
    PICO_ALIGNAS(8) double active_shuttle_scale;
    PICO_ALIGNAS(8) double candidate_shuttle_scale;
} PicoBrewController;
typedef PicoBrewController PicoController;

typedef struct PicoControlSignals {
    uint8_t enabled;
    uint8_t slow_window_pending;
    uint8_t progress_row;
    uint8_t progress_start_column;
    uint8_t progress_end_column;
    uint8_t progress_glyph_index;
    uint8_t progress_active;
} PicoControlSignals;

typedef struct PicoProgramContext {
    char identifier[32];
    uint32_t program_id;
    uint32_t temperature;
    uint32_t duration;
    uint32_t location;
    uint32_t session_type;
    const char *title;
    const char *name;
    const char *state_label;
    const char *logo_text;
    const char *interrupted_text;
    const void *bitmap_primary;
    const void *bitmap_secondary;
    const uint32_t *actions;
    uint32_t action_count;
} PicoProgramContext;

typedef struct PicoRecipeStep {
    char name[22];
    uint8_t location;
    union {
        uint8_t kind;
        uint8_t target_fahrenheit;
    };
    union {
        uint32_t duration;
        uint32_t drain_minutes;
    };
    union {
        uint32_t temperature;
        uint32_t active_minutes;
    };
} PicoRecipeStep;

typedef struct PicoRecipe {
    PicoProgramContext *program;
    char identifier[21];
    char secondary_identifier[21]; /* Cleared next to the main ID; role incomplete. */
    uint8_t reserved_2e[2];
    PicoRecipeStep steps[PICO_RECIPE_STEP_CAPACITY];
    uint8_t step_count;
    uint8_t reserved_211[3];
    int32_t integer_metric;
    PICO_ALIGNAS(8) double decimal_metric;
    int32_t abv_adjustment;
    int32_t ibu_adjustment;
    int32_t abv_adjustment_reference;
    int32_t current_step;
    int32_t ibu_adjustment_reference;
} PicoRecipe;

typedef struct PicoMenu {
    uint32_t actions[PICO_MENU_CAPACITY];
    uint32_t count;
} PicoMenu;

typedef void (*PicoPageCallback)(void);
typedef struct PicoPageCarousel {
    uint32_t page_index;
    uint32_t page_count;
    uint32_t last_update_ms;
    uint32_t interval_ms;
    PicoPageCallback pages[PICO_CAROUSEL_CAPACITY];
} PicoPageCarousel;

typedef struct PicoScreenState {
    char recipe_name[32];
    char step_name[32];
    uint32_t progress_component;
    uint32_t brew_state;
    uint8_t heated_hold;
} PicoScreenState;

typedef struct PicoAppState {
    PicoBrewController controller;
    PicoControlSignals control_signals;
    PicoScreenState screen;
    PicoPageCarousel carousel;
    PicoProgramContext session;
    PicoRecipe recipe;
    PicoMenu menu;
    uint8_t progress_unknown_04;
    uint32_t progress_unknown_08;
    uint32_t progress_unknown_0c;
    uint8_t device_state;
    char device_uid[33];
    uint8_t buzzer_enabled;
    uint32_t stepper_capabilities;
} PicoAppState;

/* SDK objects have their native SDK types and lifetimes. Application code
 * supplies their pointers rather than allocating guessed byte layouts. */
typedef struct PicoPlatformObjects {
    void *still_home_address;
    void *still_active_address;
    void *http_client;
    void *rfid_reader;
    void *server_address;
} PicoPlatformObjects;

typedef struct PicoInitializationOps {
    void (*construct_zero_address)(void *context, void *address);
    void (*construct_http_client)(void *context, void *client);
    void (*register_http_client_destructor)(void *context, void *client);
    void (*construct_rfid)(void *context, void *reader, uint8_t select_pin,
                           uint8_t shared_select_pin, uint8_t reset_pin);
    void (*construct_address_octets)(void *context, void *address, uint8_t a, uint8_t b, uint8_t c,
                                     uint8_t d);
    uint32_t (*millis)(void *context);
} PicoInitializationOps;

/* All operations and five callbacks must be supplied by the integration.
 * This reproduces the global constructor, not a full clear/reset API. Fresh
 * state must first receive C static/BSS zero initialization. Untouched fields
 * remain unchanged on reentry, as in the original function. SDK object
 * construction/destruction semantics belong to the platform implementation. */
void pico_application_initialize(PicoAppState *state, const PicoPlatformObjects *objects,
                                 const PicoInitializationOps *ops, void *context,
                                 const PicoPageCallback pages[5]);
void pico_still_clear_telemetry(PicoStillTelemetry *telemetry);

#ifdef __cplusplus
}
#endif
#endif
