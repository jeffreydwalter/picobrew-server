/* Minimal Cortex-M boundary fixture. Never linked into appliance firmware. */
#include "picobrew/local_control.h"
#include <string.h>
static PicoApplication app;
static PicoControl control;
static PicoActuators actuators;
volatile uint32_t arm_poll_count, arm_pause_count, arm_high_writes, arm_outputs;
volatile uint32_t arm_history_polls;
volatile uint32_t arm_telemetry_polls;
void pico_history_poll(void) { ++arm_history_polls; }
void pico_telemetry_poll(void) { ++arm_telemetry_polls; }
void pico_history_finish(bool interrupted) { (void)interrupted; }
static uint32_t tick;
static bool missing;
static uint32_t millis(void *c) { (void)c; tick+=10; return tick; }
static void poll(void *c) { (void)c; ++arm_poll_count; pico_local_service(); }
static int read_pin(void *c, unsigned pin) { (void)c; (void)pin; return missing; }
static void write_pin(void *c, unsigned pin, unsigned value)
{
    (void)c;
    if (value) { ++arm_high_writes; arm_outputs |= 1u << pin; }
    else arm_outputs &= ~(1u << pin);
}
static void delay(void *c, uint32_t ms) { (void)c; tick+=ms; }
static void pause(void *c) { (void)c; ++arm_pause_count; }
static uint8_t phase(void *c) { (void)c; return app.state.controller.phase; }
static const PicoActuatorOps actuator_ops={.digital_read=read_pin, .digital_write=write_pin,
    .delay_microseconds=delay, .pause_until_resume=pause, .current_phase=phase};
static const PicoApplicationHardware hardware={.millis=millis};
void arm_prepare(void)
{
    memset(&app,0,sizeof app); arm_poll_count=arm_pause_count=arm_high_writes=arm_outputs=0;
    arm_history_polls=arm_telemetry_polls=0;
    tick=0; missing=false;
    control.signals=&app.state.control_signals; control.controller=&app.state.controller;
    actuators.ops=&actuator_ops;
    app.control=&control; app.actuators=&actuators; app.hardware=&hardware;
    app.state.controller.phase=PICO_BREW_PHASE_HEATING;
    pico_local_control_bind(&app,poll,0); pico_local_control_ready(true);
}
void arm_filter_probe(void)
{
    missing=true; app.state.control_signals.enabled=1;
    arm_outputs=(1u<<PICO_PIN_HEATER)|(1u<<PICO_PIN_DRAIN_PUMP)|(1u<<PICO_PIN_STEAM_PUMP);
    pico_actuator_shuttle_on(&actuators);
}
uint32_t arm_signals_enabled(void) { return app.state.control_signals.enabled; }
