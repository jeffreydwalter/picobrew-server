#include "picobrew/phase.h"
#include <string.h>

static PicoBrewController controller;
static PicoControlSignals signals;
static PicoScreenState screen;
static PicoProgramContext session;
static PicoShuttleCalibration calibration;
static uint8_t language;
static uint8_t original_controller[0x228], original_aux[0x40];
static uint32_t ticks[64], tick_count, tick_index, enable_result;
static int32_t readings[2], resume_phase, resume_status;
static uint32_t read_index;
static bool restored;
static uint32_t events[256][10], event_count;
static PicoPhaseControl phase;
static PicoActuators actuators;

static uint32_t hash_string(const char *s)
{
    uint32_t hash = 2166136261u;
    while (*s != 0) {
        hash ^= (uint8_t)*s++;
        hash *= 16777619u;
    }
    return hash;
}

static void event(uint32_t tag, uint32_t a, uint32_t b, uint32_t c)
{
    if (event_count >= 256)
        __builtin_trap();
    uint32_t *e = events[event_count++];
    e[0] = tag;
    e[1] = a;
    e[2] = b;
    e[3] = c;
    e[4] = controller.phase | ((uint32_t)controller.status << 8) |
           ((uint32_t)controller.compartment << 16) |
           ((uint32_t)controller.shuttle_calibration_complete << 24);
    e[5] = screen.brew_state;
    e[6] = screen.heated_hold | ((uint32_t)signals.enabled << 8) |
           ((uint32_t)calibration.window_active << 16) |
           ((uint32_t)calibration.settling_complete << 24);
    e[7] = calibration.history_count;
    e[8] = hash_string(controller.phase_label);
    e[9] = controller.heater_on_time_ms;
}

static uint32_t now(void *context)
{
    (void)context;
    uint32_t value = ticks[tick_index < tick_count ? tick_index : tick_count - 1];
    ++tick_index;
    event(1, value, 0, 0);
    return value;
}
static void disable(void *context)
{
    (void)context;
    event(2, 0, 0, 0);
}
static bool enable(void *context)
{
    (void)context;
    event(3, 0, 0, 0);
    return enable_result != 0;
}
static int read_pin(void *context, unsigned pin)
{
    (void)context;
    if (pin != PICO_PIN_STEP_FILTER)
        __builtin_trap();
    int value = restored ? 0 : readings[read_index < 2 ? read_index : 1];
    ++read_index;
    event(4, pin, (uint32_t)value, 0);
    return value;
}
static void write_pin(void *context, unsigned pin, unsigned value)
{
    (void)context;
    event(5, pin, value, 0);
}
static void delay_us(void *context, uint32_t duration)
{
    (void)context;
    event(6, duration, 0, 0);
}
static void delay_ms(void *context, uint32_t duration)
{
    (void)context;
    event(7, duration, 0, 0);
}
static uint8_t current_phase(void *context)
{
    (void)context;
    return controller.phase;
}
static void actuator_pause(void *context)
{
    (void)context;
    pico_phase_pause_until_resume(&phase);
}
static void heater_off(void *context)
{
    (void)context;
    pico_actuator_heater_off(&actuators);
}
static void shuttle_off(void *context)
{
    (void)context;
    pico_actuator_shuttle_off(&actuators);
}
static void log_message(void *context, const char *level, const char *module, const char *message)
{
    (void)context;
    event(8, hash_string(level), hash_string(module), hash_string(message));
}
static void clear_row(void *context, uint16_t row, uint16_t column)
{
    (void)context;
    event(9, row, column, 0);
}
static void clear_display(void *context)
{
    (void)context;
    event(10, 0, 0, 0);
}
static void flush(void *context)
{
    (void)context;
    event(11, 0, 0, 0);
}
static void resume(void *context)
{
    (void)context;
    event(12, 0, 0, 0);
    restored = true;
    if (resume_phase >= 0)
        controller.phase = (uint8_t)resume_phase;
    if (resume_status >= 0)
        controller.status = (uint8_t)resume_status;
}
static void refresh(void *context)
{
    (void)context;
    event(13, 0, 0, 0);
}
static void ending(void *context, const char *title)
{
    (void)context;
    event(14, hash_string(title), 0, 0);
}
static void move_compartment(void *context, uint32_t compartment)
{
    (void)context;
    event(15, compartment, 0, 0);
}
static void save_scale(void *context, double candidate)
{
    (void)context;
    uint32_t words[2];
    memcpy(words, &candidate, sizeof(words));
    event(16, words[0], words[1], 0);
}

static const PicoControlOps control_ops = {.millis = now,
                                           .interrupts_disable = disable,
                                           .interrupts_enable = enable,
                                           .heater_off = heater_off,
                                           .shuttle_pump_off = shuttle_off};
static const PicoActuatorOps actuator_ops = {.digital_read = read_pin,
                                             .digital_write = write_pin,
                                             .delay_microseconds = delay_us,
                                             .delay_milliseconds = delay_ms,
                                             .pause_until_resume = actuator_pause,
                                             .current_phase = current_phase};
static const PicoPhaseOps phase_ops = {log_message, clear_row, clear_display,    flush,     resume,
                                       refresh,     ending,    move_compartment, save_scale};
static PicoControl control = {&controller, &signals, &control_ops, 0};
static PicoActuators actuators = {&actuator_ops, 0};
static PicoPhaseControl phase = {&control,     &actuators, &screen,    &session,
                                 &calibration, &language,  &phase_ops, 0};

static uint32_t read32(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}
static double read64(const uint8_t *p)
{
    double v;
    memcpy(&v, p, 8);
    return v;
}
static void write32(uint8_t *p, uint32_t v)
{
    memcpy(p, &v, 4);
}
static void write64(uint8_t *p, double v)
{
    memcpy(p, &v, 8);
}
static void load_pid(PicoPid *p, const uint8_t *r, volatile double *input, double *output,
                     double *setpoint)
{
    p->displayedProportionalGain = read64(r);
    p->displayedIntegralGain = read64(r + 8);
    p->displayedDerivativeGain = read64(r + 0x10);
    p->proportionalGain = read64(r + 0x18);
    p->integralGain = read64(r + 0x20);
    p->derivativeGain = read64(r + 0x28);
    p->direction = (int32_t)read32(r + 0x30);
    p->input = input;
    p->output = output;
    p->setpoint = setpoint;
    p->lastUpdateMs = read32(r + 0x40);
    p->integralTerm = read64(r + 0x48);
    p->previousInput = read64(r + 0x50);
    p->sampleIntervalMs = (int32_t)read32(r + 0x58);
    p->minimumOutput = read64(r + 0x60);
    p->maximumOutput = read64(r + 0x68);
    p->automatic = r[0x70] != 0;
}
static void store_pid(uint8_t *r, const PicoPid *p)
{
    write64(r, p->displayedProportionalGain);
    write64(r + 8, p->displayedIntegralGain);
    write64(r + 0x10, p->displayedDerivativeGain);
    write64(r + 0x18, p->proportionalGain);
    write64(r + 0x20, p->integralGain);
    write64(r + 0x28, p->derivativeGain);
    write32(r + 0x30, p->direction);
    write32(r + 0x40, p->lastUpdateMs);
    write64(r + 0x48, p->integralTerm);
    write64(r + 0x50, p->previousInput);
    write32(r + 0x58, p->sampleIntervalMs);
    write64(r + 0x60, p->minimumOutput);
    write64(r + 0x68, p->maximumOutput);
    r[0x70] = p->automatic;
}

void phase_fixture_reset(const uint8_t *raw, const uint8_t *aux, const uint32_t *clock_values,
                         const uint32_t *configuration)
{
    memcpy(original_controller, raw, sizeof(original_controller));
    memcpy(original_aux, aux, sizeof(original_aux));
    memset(&controller, 0, sizeof(controller));
    memset(&screen, 0, sizeof(screen));
    memset(&session, 0, sizeof(session));
    memset(&calibration, 0, sizeof(calibration));
    memcpy(&signals, aux, sizeof(signals));
    screen.brew_state = read32(aux + 8);
    screen.heated_hold = aux[12];
    memcpy(calibration.candidate_history, aux + 0x10, 12);
    calibration.history_count = (int32_t)read32(aux + 0x1c);
    calibration.settling_complete = aux[0x20];
    calibration.window_started_ms = read32(aux + 0x24);
    calibration.window_active = aux[0x28];
    session.session_type = read32(aux + 0x2c);
    language = aux[0x30];
    tick_count = configuration[0];
    enable_result = configuration[1];
    readings[0] = (int32_t)configuration[2];
    readings[1] = (int32_t)configuration[3];
    resume_phase = (int32_t)configuration[4];
    resume_status = (int32_t)configuration[5];
    memcpy(ticks, clock_values, tick_count * 4);
    tick_index = read_index = event_count = 0;
    restored = false;
    controller.compartment = raw[0x60];
    controller.status = raw[0x61];
    controller.phase = raw[0x62];
    memcpy(controller.phase_label, raw + 0x63, sizeof(controller.phase_label));
    memcpy(controller.wort_history, raw + 0x78, 48);
    controller.drain_subphase = raw[0xa8];
    controller.shuttle_calibration_complete = raw[0x1a0];
    controller.calibration_flag_1a1 = raw[0x1a1];
    controller.trend_seconds = read32(raw + 0x1a4);
    controller.step_seconds = (int32_t)read32(raw + 0x1a8);
    controller.history_count = (int32_t)read32(raw + 0x1ac);
    controller.previous_window_wort_fahrenheit = read64(raw + 0x1b0);
    controller.highest_wort_fahrenheit = read64(raw + 0x1b8);
    controller.wort_target_fahrenheit = read64(raw + 0x1d0);
    controller.thermoblock_target_fahrenheit = read64(raw + 0x1d8);
    controller.wort_pid_output = read64(raw + 0x1e0);
    controller.thermoblock_pid_output = read64(raw + 0x1e8);
    controller.wort_fahrenheit = read64(raw + 0x1f0);
    controller.thermoblock_fahrenheit = read64(raw + 0x1f8);
    controller.heater_cycle_started_ms = read32(raw + 0x200);
    controller.shuttle_on_time_ms = read32(raw + 0x204);
    controller.shuttle_cycle_started_ms = read32(raw + 0x208);
    controller.heater_on_time_ms = read32(raw + 0x20c);
    controller.drain_phase_started_ms = read32(raw + 0x210);
    controller.active_shuttle_scale = read64(raw + 0x218);
    controller.candidate_shuttle_scale = read64(raw + 0x220);
    load_pid(&controller.wort_pid, raw + 0xb0, &controller.wort_fahrenheit,
             &controller.wort_pid_output, &controller.wort_target_fahrenheit);
    load_pid(&controller.thermoblock_pid, raw + 0x128, &controller.thermoblock_fahrenheit,
             &controller.thermoblock_pid_output, &controller.thermoblock_target_fahrenheit);
}

int phase_fixture_run(uint32_t operation, int32_t new_phase)
{
    switch (operation) {
    case 0:
        pico_phase_set(&phase, new_phase);
        break;
    case 1:
        pico_phase_clear_status(&phase);
        break;
    case 2:
        pico_phase_pause_until_resume(&phase);
        break;
    case 3:
        pico_phase_start_circulation(&phase);
        break;
    case 4:
        pico_phase_finish_step(&phase, "Test Program");
        break;
    case 5:
        return pico_phase_update_shuttle_calibration(&phase);
    default:
        __builtin_trap();
    }
    return 0;
}

void phase_fixture_result(uint8_t *raw, uint8_t *aux)
{
    memcpy(raw, original_controller, sizeof(original_controller));
    memcpy(aux, original_aux, sizeof(original_aux));
    memcpy(aux, &signals, sizeof(signals));
    write32(aux + 8, screen.brew_state);
    aux[12] = screen.heated_hold;
    memcpy(aux + 0x10, calibration.candidate_history, 12);
    write32(aux + 0x1c, calibration.history_count);
    aux[0x20] = calibration.settling_complete;
    write32(aux + 0x24, calibration.window_started_ms);
    aux[0x28] = calibration.window_active;
    write32(aux + 0x2c, session.session_type);
    aux[0x30] = language;
    raw[0x60] = controller.compartment;
    raw[0x61] = controller.status;
    raw[0x62] = controller.phase;
    memcpy(raw + 0x63, controller.phase_label, sizeof(controller.phase_label));
    memcpy(raw + 0x78, controller.wort_history, 48);
    raw[0xa8] = controller.drain_subphase;
    raw[0x1a0] = controller.shuttle_calibration_complete;
    raw[0x1a1] = controller.calibration_flag_1a1;
    write32(raw + 0x1a4, controller.trend_seconds);
    write32(raw + 0x1a8, controller.step_seconds);
    write32(raw + 0x1ac, controller.history_count);
    write64(raw + 0x1b0, controller.previous_window_wort_fahrenheit);
    write64(raw + 0x1b8, controller.highest_wort_fahrenheit);
    write64(raw + 0x1d0, controller.wort_target_fahrenheit);
    write64(raw + 0x1d8, controller.thermoblock_target_fahrenheit);
    write64(raw + 0x1e0, controller.wort_pid_output);
    write64(raw + 0x1e8, controller.thermoblock_pid_output);
    write64(raw + 0x1f0, controller.wort_fahrenheit);
    write64(raw + 0x1f8, controller.thermoblock_fahrenheit);
    write32(raw + 0x200, controller.heater_cycle_started_ms);
    write32(raw + 0x204, controller.shuttle_on_time_ms);
    write32(raw + 0x208, controller.shuttle_cycle_started_ms);
    write32(raw + 0x20c, controller.heater_on_time_ms);
    write32(raw + 0x210, controller.drain_phase_started_ms);
    write64(raw + 0x218, controller.active_shuttle_scale);
    write64(raw + 0x220, controller.candidate_shuttle_scale);
    store_pid(raw + 0xb0, &controller.wort_pid);
    store_pid(raw + 0x128, &controller.thermoblock_pid);
}
uint32_t phase_fixture_event_count(void)
{
    return event_count;
}
const uint32_t *phase_fixture_events(void)
{
    return &events[0][0];
}
