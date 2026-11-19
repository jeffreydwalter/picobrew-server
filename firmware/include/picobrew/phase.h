#ifndef PICOBREW_PHASE_H
#define PICOBREW_PHASE_H

#include "picobrew/actuators.h"
#include "picobrew/control.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The original calibration globals form one three-sample history and two
 * timed-window flags. Fresh storage receives static/BSS zero initialization.
 * history_count must be in 0..2; arbitrary corrupt history indices are outside
 * the recovered routine's valid-state contract.
 */
typedef struct PicoShuttleCalibration {
    float candidate_history[3];
    int32_t history_count;
    uint8_t settling_complete;
    uint32_t window_started_ms;
    uint8_t window_active;
} PicoShuttleCalibration;

/* These are real UI, stepper, logging, and settings boundaries. Every callback
 * used by the selected path is required. wait_for_resume may update controller
 * state; the caller rereads the phase after it returns, as in the original.
 * save_shuttle_scale must persist the candidate value with the original
 * SS.txt formatting when implementing the original storage behavior.
 */
typedef struct PicoPhaseOps {
    void (*log_message)(void *context, const char *level, const char *module, const char *message);
    void (*display_clear_row)(void *context, uint16_t row, uint16_t column);
    void (*display_clear)(void *context);
    void (*display_flush)(void *context);
    void (*wait_for_resume)(void *context);
    void (*refresh_current_page)(void *context);
    void (*show_ending_step)(void *context, const char *program_title);
    void (*move_to_compartment)(void *context, uint32_t compartment);
    void (*save_shuttle_scale)(void *context, double candidate);
} PicoPhaseOps;

typedef struct PicoPhaseControl {
    PicoControl *control;
    PicoActuators *actuators;
    PicoScreenState *screen;
    const PicoProgramContext *session;
    PicoShuttleCalibration *calibration;
    const uint8_t *language;
    const PicoPhaseOps *ops;
    void *context;
} PicoPhaseControl;

/* The actuator current_phase callback must read control->controller->phase.
 * Its pause_until_resume callback should invoke pico_phase_pause_until_resume
 * on this same instance. Control output callbacks must use these actuators.
 */
uint8_t pico_phase_update_shuttle_calibration(PicoPhaseControl *phase);
void pico_phase_start_circulation(PicoPhaseControl *phase);
void pico_phase_set(PicoPhaseControl *phase, int32_t brew_phase);
void pico_phase_clear_status(PicoPhaseControl *phase);
void pico_phase_pause_until_resume(PicoPhaseControl *phase);
void pico_phase_finish_step(PicoPhaseControl *phase, const char *program_title);

#ifdef __cplusplus
}
#endif
#endif
