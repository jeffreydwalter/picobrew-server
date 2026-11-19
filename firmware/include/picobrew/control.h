#ifndef PICOBREW_CONTROL_H
#define PICOBREW_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include "picobrew/state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hardware/UI boundaries used by the recovered controller. The operations are
 * required: absent implementations are integration errors, not success stubs.
 * shuttle_pump_on must preserve the original filter/pause interlock. The fatal
 * heater callback must not return. The context belongs to the integrating app.
 */
typedef struct PicoControlOps {
    uint32_t (*millis)(void *context);
    void (*interrupts_disable)(void *context);
    /* Matches TI IntMasterEnable: true means interrupts were disabled before
     * this call; the value is not a success/failure status. */
    bool (*interrupts_enable)(void *context);
    uint32_t (*timer_interrupt_status)(void *context, uint32_t timer_base, bool masked);
    void (*timer_interrupt_clear)(void *context, uint32_t timer_base, uint32_t flags);
    float (*read_wort_fahrenheit)(void *context);
    float (*read_thermoblock_fahrenheit)(void *context);
    void (*heater_on)(void *context);
    void (*heater_off)(void *context);
    void (*shuttle_pump_on)(void *context);
    void (*shuttle_pump_off)(void *context);
    void (*steam_pump_off)(void *context);
    void (*drain_pump_off)(void *context);
    void (*progress_dot)(void *context, uint16_t row, uint16_t column);
    void (*progress_clear)(void *context, uint16_t row, uint16_t start_column);
    void (*display_flush)(void *context);
    void (*halt_heater_malfunction)(void *context);
    void (*peripheral_clock_enable)(void *context, uint32_t peripheral, uint32_t mode);
    void (*peripheral_reset)(void *context, uint32_t peripheral);
    void (*timer_configure)(void *context, uint32_t timer_base, uint32_t configuration);
    void (*timer_load_set)(void *context, uint32_t timer_base, uint32_t timer, uint32_t ticks);
    void (*timer_interrupt_register)(void *context, uint32_t timer_base, uint32_t timer,
                                     void (*handler)(void));
    void (*timer_interrupt_enable)(void *context, uint32_t timer_base, uint32_t flags);
    void (*timer_enable)(void *context, uint32_t timer_base, uint32_t timer);
} PicoControlOps;

typedef struct PicoControl {
    PicoBrewController *controller;
    PicoControlSignals *signals;
    const PicoControlOps *ops;
    void *context;
} PicoControl;

void pico_control_record_temperature_sample(PicoControl *control);
void pico_control_start(PicoControl *control);
bool pico_control_stop(PicoControl *control);
int pico_control_classify_temperature_progress(PicoControl *control, double temperature);
void pico_control_temperature_tick(PicoControl *control);
void pico_control_outputs_tick(PicoControl *control);

/* One hardware instance owns these vector callbacks. Binding must happen before
 * enabling the timers; the context and operations must remain alive thereafter.
 */
void pico_control_bind_interrupts(PicoControl *control);
void pico_temperature_control_timer_isr(void);
void pico_heater_pump_timer_isr(void);
bool pico_control_timers_initialize(PicoControl *control);

#ifdef __cplusplus
}
#endif
#endif
