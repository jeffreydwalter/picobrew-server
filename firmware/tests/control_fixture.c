#include "picobrew/control.h"
#include <setjmp.h>
#include <string.h>

static PicoBrewController controller;
static PicoControlSignals signals;
static uint8_t original_controller[0x228];
static uint32_t clock_values[32], clock_count, clock_position;
static float wort_sample, thermoblock_sample;
static uint32_t events[512][4], event_count;
static jmp_buf fatal_jump;
static uint32_t enable_result;

static void event(uint32_t tag, uint32_t a, uint32_t b, uint32_t c)
{
    if (event_count >= 512)
        __builtin_trap();
    events[event_count][0] = tag;
    events[event_count][1] = a;
    events[event_count][2] = b;
    events[event_count][3] = c;
    ++event_count;
}
static uint32_t now(void *ctx)
{
    (void)ctx;
    uint32_t n = clock_values[clock_position < clock_count ? clock_position : clock_count - 1];
    ++clock_position;
    event(1, n, 0, 0);
    return n;
}
static void disable(void *ctx)
{
    (void)ctx;
    event(2, 0, 0, 0);
}
static bool enable(void *ctx)
{
    (void)ctx;
    event(3, 0, 0, 0);
    return enable_result != 0;
}
static uint32_t status(void *ctx, uint32_t base, bool masked)
{
    (void)ctx;
    event(4, base, masked, 0);
    return 0x35;
}
static void clear(void *ctx, uint32_t base, uint32_t flags)
{
    (void)ctx;
    event(5, base, flags, 0);
}
static float wort(void *ctx)
{
    (void)ctx;
    event(6, 0, 0, 0);
    return wort_sample;
}
static float thermoblock(void *ctx)
{
    (void)ctx;
    event(7, 0, 0, 0);
    return thermoblock_sample;
}
#define OUTPUT(name, tag)                                                                          \
    static void name(void *ctx)                                                                    \
    {                                                                                              \
        (void)ctx;                                                                                 \
        event(tag, 0, 0, 0);                                                                       \
    }
OUTPUT(heater_on, 8)
OUTPUT(heater_off, 9)
OUTPUT(shuttle_on, 10)
OUTPUT(shuttle_off, 11) OUTPUT(steam_off, 12)
    OUTPUT(drain_off, 13) static void dot(void *ctx, uint16_t row, uint16_t col)
{
    (void)ctx;
    event(14, row, col, 0);
}
static void progress_clear(void *ctx, uint16_t row, uint16_t col)
{
    (void)ctx;
    event(15, row, col, 0);
}
OUTPUT(flush, 16)
static void fatal(void *ctx)
{
    (void)ctx;
    event(17, 0, 0, 0);
    longjmp(fatal_jump, 1);
}
static void peripheral_enable(void *ctx, uint32_t peripheral, uint32_t mode)
{
    (void)ctx;
    event(18, peripheral, mode, 0);
}
static void peripheral_reset(void *ctx, uint32_t peripheral)
{
    (void)ctx;
    event(19, peripheral, 0, 0);
}
static void configure(void *ctx, uint32_t base, uint32_t mode)
{
    (void)ctx;
    event(20, base, mode, 0);
}
static void load(void *ctx, uint32_t base, uint32_t timer, uint32_t ticks)
{
    (void)ctx;
    event(21, base, timer, ticks);
}
static void register_handler(void *ctx, uint32_t base, uint32_t timer, void (*handler)(void))
{
    (void)ctx;
    event(22, base, timer,
          handler == pico_temperature_control_timer_isr ? 0
          : handler == pico_heater_pump_timer_isr       ? 1
                                                        : 99);
}
static void enable_timer_interrupt(void *ctx, uint32_t base, uint32_t flags)
{
    (void)ctx;
    event(23, base, flags, 0);
}
static void enable_timer(void *ctx, uint32_t base, uint32_t timer)
{
    (void)ctx;
    event(24, base, timer, 0);
}
static const PicoControlOps ops = {now,
                                   disable,
                                   enable,
                                   status,
                                   clear,
                                   wort,
                                   thermoblock,
                                   heater_on,
                                   heater_off,
                                   shuttle_on,
                                   shuttle_off,
                                   steam_off,
                                   drain_off,
                                   dot,
                                   progress_clear,
                                   flush,
                                   fatal,
                                   peripheral_enable,
                                   peripheral_reset,
                                   configure,
                                   load,
                                   register_handler,
                                   enable_timer_interrupt,
                                   enable_timer};
static PicoControl control = {&controller, &signals, &ops, 0};

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
    p->displayedProportionalGain = read64(r + 0);
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
    write64(r + 0, p->displayedProportionalGain);
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

void control_fixture_reset(const uint8_t *raw, const uint8_t *raw_signals, const uint32_t *ticks,
                           uint32_t count, uint32_t wort_bits, uint32_t thermo_bits,
                           uint32_t enabled_result)
{
    memcpy(original_controller, raw, 0x228);
    memset(&controller, 0, sizeof(controller));
    memcpy(&signals, raw_signals, sizeof(signals));
    memcpy(clock_values, ticks, count * 4);
    clock_count = count;
    clock_position = 0;
    memcpy(&wort_sample, &wort_bits, 4);
    memcpy(&thermoblock_sample, &thermo_bits, 4);
    enable_result = enabled_result;
    event_count = 0;
    controller.compartment = raw[0x60];
    controller.status = raw[0x61];
    controller.phase = raw[0x62];
    memcpy(controller.wort_history, raw + 0x78, 6 * 8);
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
    controller.active_shuttle_scale = read64(raw + 0x218);
    load_pid(&controller.wort_pid, raw + 0xb0, &controller.wort_fahrenheit,
             &controller.wort_pid_output, &controller.wort_target_fahrenheit);
    load_pid(&controller.thermoblock_pid, raw + 0x128, &controller.thermoblock_fahrenheit,
             &controller.thermoblock_pid_output, &controller.thermoblock_target_fahrenheit);
    pico_control_bind_interrupts(&control);
}

int control_fixture_run(int operation, double temperature)
{
    if (setjmp(fatal_jump))
        return -777;
    switch (operation) {
    case 0:
        pico_control_temperature_tick(&control);
        break;
    case 1:
        pico_control_outputs_tick(&control);
        break;
    case 2:
        pico_control_start(&control);
        break;
    case 3:
        return pico_control_stop(&control);
    case 4:
        pico_control_record_temperature_sample(&control);
        break;
    case 5:
        return pico_control_classify_temperature_progress(&control, temperature);
    case 6:
        return pico_control_timers_initialize(&control);
    case 7:
        pico_temperature_control_timer_isr();
        break;
    case 8:
        pico_heater_pump_timer_isr();
        break;
    default:
        return -999;
    }
    return 0;
}

void control_fixture_result(uint8_t *raw, uint8_t *raw_signals)
{
    memcpy(raw, original_controller, 0x228);
    memcpy(raw_signals, &signals, sizeof(signals));
    raw[0x60] = controller.compartment;
    raw[0x61] = controller.status;
    raw[0x62] = controller.phase;
    memcpy(raw + 0x78, controller.wort_history, 6 * 8);
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
    write64(raw + 0x218, controller.active_shuttle_scale);
    store_pid(raw + 0xb0, &controller.wort_pid);
    store_pid(raw + 0x128, &controller.thermoblock_pid);
}
uint32_t control_fixture_event_count(void)
{
    return event_count;
}
const uint32_t *control_fixture_events(void)
{
    return &events[0][0];
}
