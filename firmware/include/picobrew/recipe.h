#ifndef PICOBREW_RECIPE_H
#define PICOBREW_RECIPE_H

#include "picobrew/phase.h"
#if PICO_STANDALONE
#include "picobrew/local_store.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PicoRecipeScreen {
    PICO_RECIPE_CLEAR,
    PICO_RECIPE_FLUSH,
    PICO_RECIPE_LOGO,
    PICO_RECIPE_CHANGING_STEP,
    PICO_RECIPE_SETTING_UP_STEP,
    PICO_RECIPE_TITLED_FRAME,
    PICO_RECIPE_PREHEATING_STEAM,
    PICO_RECIPE_STARTING_PROGRAM,
    PICO_RECIPE_ENDING_PROGRAM,
    PICO_RECIPE_LOADING,
    PICO_RECIPE_LOADING_PACK,
    PICO_RECIPE_STOP_PROGRESS,
    PICO_RECIPE_PAUSED,
    PICO_RECIPE_SKIP_CONFIRMATION,
    PICO_RECIPE_STOP_CONFIRMATION,
    PICO_RECIPE_STILL_COMMUNICATION_FAILURE,
    PICO_RECIPE_STILL_TEMPERATURE_WARNING,
    PICO_RECIPE_STILL_COMPLETION,
    PICO_RECIPE_STILL_FAN_MENU,
    PICO_RECIPE_REQUIRE_FIRST_RINSE,
    PICO_RECIPE_BREW_COMPLETE,
    PICO_RECIPE_WAIT_CLICK,
    PICO_RECIPE_RINSE_FINISHED,
    PICO_RECIPE_DEEP_CLEAN_SETUP,
    PICO_RECIPE_ENDING_PROCESS,
    PICO_RECIPE_PROGRAM_NOTICE,
    PICO_RECIPE_DRAIN_RESERVOIR,
    PICO_RECIPE_DRAIN_FILTER,
    PICO_RECIPE_STOP_FOOTER,
    PICO_RECIPE_HEADER,
    PICO_RECIPE_FIRST_RINSE_STEP,
    PICO_RECIPE_ENDING_FIRST_RINSE,
    PICO_RECIPE_RACK_BEER_STEP,
    PICO_RECIPE_DESCALE_HEADING,
    PICO_RECIPE_WAIT_STOP,
    PICO_RECIPE_INSTRUCTION,
    PICO_RECIPE_INSTRUCTION_HEADER,
    PICO_RECIPE_START_DESCALE,
    PICO_RECIPE_CONFIRM_STILL_CLEANED,
    PICO_RECIPE_WAIT_FILTER_INSERTION
} PicoRecipeScreen;

typedef enum PicoRecipePrompt {
    PICO_RECIPE_CONFIRM_AT_ROW,
    PICO_RECIPE_CONFIRM_THERMAL_RECOVERY,
    PICO_RECIPE_SETUP_CHOICE,
    PICO_RECIPE_SELECT_RECIPE,
    PICO_RECIPE_ADJUST_RECIPE,
    PICO_RECIPE_CONFIRM_CLEANING,
    PICO_RECIPE_VACUUM,
    PICO_RECIPE_DESCALE_STEP,
    PICO_RECIPE_CHOOSE_STILL_PROGRAM,
    PICO_RECIPE_WALK_STILL_STEPS,
    PICO_RECIPE_WAIT_CONTINUE
} PicoRecipePrompt;

typedef struct PicoProgramAssets {
    const void *primary;
    const void *secondary;
    const void *sousvide_secondary;
} PicoProgramAssets;

typedef struct PicoRecipePages {
    PicoPageCallback normal[5];
    PicoPageCallback cooking[2];
    PicoPageCallback manual[3];
    PicoPageCallback holding;
    PicoPageCallback still;
} PicoRecipePages;

/* All callbacks used on an executed path are required. These interfaces are
 * application services, not substitute successful implementations. Fields in
 * state and the pointed-to settings must remain stable while this context runs.
 */
typedef struct PicoRecipeOps {
    void (*log)(void *context, const char *level, const char *module, const char *message);
    void (*serial)(void *context, const char *text, bool newline);
    void (*assertion)(void *context, bool condition, const char *expression);
    const char *(*action_label)(void *context, uint32_t action, uint32_t language, uint32_t status);
    void (*compartment_label)(void *context, char *destination, uint32_t compartment);
    void (*screen)(void *context, PicoRecipeScreen screen, uint32_t argument, const char *text);
    int (*prompt)(void *context, PicoRecipePrompt prompt, uint32_t argument, const char *text);
    void (*clear_row)(void *context, uint16_t row, uint16_t column);
    void (*advance_carousel)(void *context);
    void (*refresh_page)(void *context);
    void (*set_recipe_name)(void *context, const char *name);
    void (*set_step_name)(void *context, uint32_t location, const char *name);
    bool (*take_click)(void *context);
    void (*clear_click)(void *context);
    uint32_t (*populate_brew_menu)(void *context);
    uint32_t (*populate_post_brew_menu)(void *context);
    uint32_t (*choose_menu)(void *context, uint32_t row, uint32_t count, uint32_t initial,
                            int use_timeout, uint8_t default_index, uint8_t redraw,
                            uint32_t timeout_seconds);
    uint32_t (*temperature_input)(void *context, uint32_t label, const char *title,
                                  uint32_t initial);
    uint32_t (*time_input)(void *context, uint32_t label, const char *title, uint32_t initial);
    void (*error)(void *context, uint32_t code, bool logging, bool display);
    void (*wait_for_filter)(void *context);
    bool (*check_thermoblock_heating)(void *context);
    void (*move_home)(void *context);
    void (*save_recovery)(void *context, uint32_t active_minutes, uint32_t drain_minutes,
                          int32_t step_index, uint32_t reserved, bool force, uint8_t calibrated);
    bool (*read_recovery)(void *context, char *record);
#if PICO_STANDALONE
    bool (*read_local_recovery)(void *context, PicoLocalRecovery *record);
#endif
    void (*delete_recovery)(void *context);
    bool (*get_session_id)(void *context, char *identifier, uint32_t capacity);
    void (*report_state)(void *context);
    void (*log_step)(void *context, const char *name);
    void (*log_progress)(void *context, const char *name);
    int (*fetch_recipe)(void *context, const char *identifier, int32_t ibu, int32_t abv);
    bool (*fetch_default_recipe_failed)(void *context, const char *identifier);
    bool (*read_picopak)(void *context, char *identifier, uint32_t capacity);
    int (*required_actions)(void *context, uint32_t *actions, uint32_t capacity);
    void (*save_shuttle_scale)(void *context, double scale);
    void (*load_shuttle_scale)(void *context);
    bool (*still_read_status)(void *context, PicoStillTelemetry *telemetry);
    bool (*still_pressure_limit)(void *context, PicoStillTelemetry *telemetry);
    void (*still_stop)(void *context);
    uint32_t (*run_drain_instructions)(void *context, uint32_t instruction);
    const char *(*title_label)(void *context, uint32_t id, uint32_t language);
    void (*center_text)(void *context, uint16_t row, const char *text);
    void (*timer)(void *context, uint16_t row, uint32_t started_ms, uint32_t duration_seconds);
    void (*temperature_columns)(void *context, uint16_t row, double wort, double steam,
                                double target);
    void (*temperature_summary)(void *context, double wort, double steam, double target,
                                uint16_t row);
    void (*valve_position)(void *context, uint16_t row, uint32_t position);
    void (*circulation_status)(void *context, double wort, uint32_t elapsed_seconds);
    uint32_t (*instruction_screen)(void *context, uint32_t screen, int alarm, uint32_t program,
                                   const char *title, bool use_context_title);
    int (*instruction_sequence)(void *context, const uint32_t *screens, int count, int alarm,
                                uint32_t program, const char *title, bool use_context_title);
    void (*populate_actions)(void *context, const uint32_t *actions, uint32_t count);
    uint32_t (*select_action)(void *context, uint32_t row, int32_t initial);
    void (*alarm_until_press_or_timeout)(void *context, uint32_t timeout_ms);
    void (*report_error)(void *context, uint32_t code);
    void (*still_initialize)(void *context);
    bool (*still_has_home_ip)(void *context);
    bool (*still_choose_access_point)(void *context);
    bool (*still_permission)(void *context, const char *method);
    bool (*still_configure)(void *context);
    bool (*still_connect_wifi)(void *context, bool home);
    bool (*still_start_once)(void *context);
} PicoRecipeOps;

typedef struct PicoRecipeRuntime {
    PicoAppState *state;
    PicoPhaseControl *phase;
    uint8_t *language;
    uint8_t *detailed_status;
    uint8_t *offline;
    uint8_t *legacy_compartment;
    const PicoProgramAssets *assets;
    const PicoRecipePages *pages;
    const PicoRecipeOps *ops;
    void *context;
    const char *active_title;
#if PICO_STANDALONE
    /* Zero means a new step; otherwise the saved phase plus one. */
    uint8_t recovery_phase;
#endif
} PicoRecipeRuntime;

typedef struct PicoStepRequest {
    uint32_t target_fahrenheit;
    uint32_t active_minutes;
    uint32_t drain_minutes;
    uint8_t compartment;
    const char *program_title;
    const char *step_name;
    uint8_t logging_enabled;
    int32_t remaining_minutes;
    int32_t step_index;
    uint32_t session_type;
} PicoStepRequest;

int pico_recipe_delimited_field(const char *input, char *output, int32_t *cursor, char delimiter,
                                int32_t input_limit);
/* response must address length+1 bytes and be NUL terminated at response[length].
 * Embedded NULs, fields of 100 bytes or more, and absent separators fail safely.
 * The checked parser preserves valid-record behavior of the original firmware;
 * the historical delimiter helper above requires caller-sized output storage.
 */
bool pico_recipe_parse_response(PicoRecipeRuntime *runtime, const char *response, int32_t length);
void pico_recipe_program_initialize(PicoRecipeRuntime *runtime, PicoProgramContext *program,
                                    uint32_t program_id, uint32_t language);
void pico_recipe_program_log(PicoRecipeRuntime *runtime, const PicoProgramContext *program,
                             uint32_t language);
void pico_recipe_initialize_cleaning(PicoRecipeRuntime *runtime, PicoRecipe *recipe,
                                     int32_t temperature);
void pico_recipe_prepare_temperature(PicoRecipeRuntime *runtime, int32_t target_fahrenheit,
                                     const char *program_title);
int pico_recipe_run_step(PicoRecipeRuntime *runtime, const PicoStepRequest *request);
bool pico_recipe_run(PicoRecipeRuntime *runtime, PicoRecipe *recipe, bool logging);
void pico_recipe_end_outputs(PicoRecipeRuntime *runtime, uint32_t duration_ms, const char *title,
                             bool drain, bool shuttle);
void pico_recipe_session_end(PicoRecipeRuntime *runtime, bool logging);
bool pico_recipe_session_start(PicoRecipeRuntime *runtime, bool server_session);
bool pico_recipe_session_restore(PicoRecipeRuntime *runtime, PicoRecipe *recipe);
bool pico_recipe_setup(PicoRecipeRuntime *runtime, bool logging, const char *title);
void pico_recipe_post_session(PicoRecipeRuntime *runtime);
bool pico_recipe_pause_instructions(PicoRecipeRuntime *runtime, uint32_t program_id);
void pico_recipe_prompt_parameters(PicoRecipeRuntime *runtime);
bool pico_recipe_run_control_program(PicoRecipeRuntime *runtime, uint32_t device_state);
void pico_recipe_run_brew_program(PicoRecipeRuntime *runtime, PicoRecipe *recipe);
void pico_recipe_run_cooking_program(PicoRecipeRuntime *runtime, uint32_t program_id);
void pico_recipe_run_deep_clean(PicoRecipeRuntime *runtime);
void pico_recipe_run_required_maintenance(PicoRecipeRuntime *runtime);
uint32_t pico_recipe_location_value(PicoRecipeRuntime *runtime, uint32_t location);
uint32_t pico_recipe_instruction(PicoRecipeRuntime *runtime, uint32_t instruction, int option);
bool pico_recipe_handle_filter_removal(PicoRecipeRuntime *runtime);
bool pico_recipe_prime_steam(PicoRecipeRuntime *runtime, const char *title);
uint32_t pico_recipe_reprime_steam(PicoRecipeRuntime *runtime, const char *title, const char *step,
                                   bool logging, bool restart_circulation);
void pico_recipe_restore_idle(PicoRecipeRuntime *runtime);
void pico_recipe_vacuum(PicoRecipeRuntime *runtime, uint32_t compartment, uint32_t first);
void pico_recipe_circulate(PicoRecipeRuntime *runtime);
void pico_recipe_drain_reservoir(PicoRecipeRuntime *runtime);
void pico_recipe_flush_compartments(PicoRecipeRuntime *runtime, uint32_t minimum_cycles,
                                    bool stop_automatically);
void pico_recipe_flush_until_press(PicoRecipeRuntime *runtime);
void pico_recipe_heat_flush_compartments(PicoRecipeRuntime *runtime, bool steam, double target);
void pico_recipe_rinse(PicoRecipeRuntime *runtime);
bool pico_recipe_first_rinse_cool(PicoRecipeRuntime *runtime, int32_t *next_compartment);
bool pico_recipe_first_rinse_heat(PicoRecipeRuntime *runtime);
void pico_recipe_drain_filter(PicoRecipeRuntime *runtime);
void pico_recipe_first_rinse(PicoRecipeRuntime *runtime);
void pico_recipe_rack_beer(PicoRecipeRuntime *runtime);
void pico_recipe_descale(PicoRecipeRuntime *runtime);
void pico_recipe_manual_program(PicoRecipeRuntime *runtime);
void pico_recipe_circulation_program(PicoRecipeRuntime *runtime);
uint32_t pico_recipe_still_program(PicoRecipeRuntime *runtime);

#ifdef __cplusplus
}
#endif
#endif
