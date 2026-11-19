
#include "picobrew/state.h"
#include <string.h>
#include <stdint.h>
static PicoAppState state;
static uint8_t initial[8192];
static uint8_t snapshots[8][8192];
static uint32_t events[8][5], event_count, clocks[2], clock_index;
static double pid_io[3];
static PicoPid lone_pid;
static uint8_t lone_initial[0x78];
static void page0(void) {}
static void page1(void) {}
static void page2(void) {}
static void page3(void) {}
static void page4(void) {}
static PicoPageCallback pages[5] = {page0, page1, page2, page3, page4};
static uint32_t callbacks[5] = {0x20009ac9, 0x20009bc1, 0x20009c25, 0x20008969, 0x20009be5};
static uint32_t get32(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}
static void put32(uint8_t *p, uint32_t v)
{
    memcpy(p, &v, 4);
}
static uint32_t pointer_id(const volatile void *p)
{
    if (p == &state.controller.wort_fahrenheit)
        return 0x2002d4e8;
    if (p == &state.controller.thermoblock_fahrenheit)
        return 0x2002d4f0;
    if (p == &state.controller.wort_pid_output)
        return 0x2002d4d8;
    if (p == &state.controller.thermoblock_pid_output)
        return 0x2002d4e0;
    if (p == &state.controller.wort_target_fahrenheit)
        return 0x2002d4c8;
    if (p == &state.controller.thermoblock_target_fahrenheit)
        return 0x2002d4d0;
    if (p == &state.session)
        return 0x2002c108;
    for (unsigned i = 0; i < 3; ++i)
        if (p == &pid_io[i])
            return 0x20082000 + 8 * i;
    return (uint32_t)(uintptr_t)p;
}
static void load_pid(PicoPid *pid, const uint8_t *wire)
{
    memcpy(&pid->displayedProportionalGain, wire + 0, 8);
    memcpy(&pid->displayedIntegralGain, wire + 8, 8);
    memcpy(&pid->displayedDerivativeGain, wire + 16, 8);
    memcpy(&pid->proportionalGain, wire + 24, 8);
    memcpy(&pid->integralGain, wire + 32, 8);
    memcpy(&pid->derivativeGain, wire + 40, 8);
    memcpy(&pid->direction, wire + 48, 4);
    memcpy(&pid->lastUpdateMs, wire + 64, 4);
    memcpy(&pid->integralTerm, wire + 72, 8);
    memcpy(&pid->previousInput, wire + 80, 8);
    memcpy(&pid->sampleIntervalMs, wire + 88, 4);
    memcpy(&pid->minimumOutput, wire + 96, 8);
    memcpy(&pid->maximumOutput, wire + 104, 8);
    memcpy(&pid->automatic, wire + 112, 1);

    pid->input = (volatile double *)(uintptr_t)get32(wire + 0x34);
    pid->output = (double *)(uintptr_t)get32(wire + 0x38);
    pid->setpoint = (double *)(uintptr_t)get32(wire + 0x3c);
}
static void save_pid(const PicoPid *pid, uint8_t *wire)
{
    memcpy(wire + 0, &pid->displayedProportionalGain, 8);
    memcpy(wire + 8, &pid->displayedIntegralGain, 8);
    memcpy(wire + 16, &pid->displayedDerivativeGain, 8);
    memcpy(wire + 24, &pid->proportionalGain, 8);
    memcpy(wire + 32, &pid->integralGain, 8);
    memcpy(wire + 40, &pid->derivativeGain, 8);
    memcpy(wire + 48, &pid->direction, 4);
    memcpy(wire + 64, &pid->lastUpdateMs, 4);
    memcpy(wire + 72, &pid->integralTerm, 8);
    memcpy(wire + 80, &pid->previousInput, 8);
    memcpy(wire + 88, &pid->sampleIntervalMs, 4);
    memcpy(wire + 96, &pid->minimumOutput, 8);
    memcpy(wire + 104, &pid->maximumOutput, 8);
    memcpy(wire + 112, &pid->automatic, 1);

    put32(wire + 0x34, pointer_id(pid->input));
    put32(wire + 0x38, pointer_id(pid->output));
    put32(wire + 0x3c, pointer_id(pid->setpoint));
}
static void load_state(const uint8_t *wire)
{
    memset(&state, 0, sizeof state);
    memcpy(initial, wire, 8192);
    memcpy((void *)&state.controller.still, wire + 5880, 96);
    memcpy((void *)&state.controller.compartment, wire + 5976, 1);
    memcpy((void *)&state.controller.status, wire + 5977, 1);
    memcpy((void *)&state.controller.phase, wire + 5978, 1);
    memcpy((void *)&state.controller.phase_label, wire + 5979, 21);
    memcpy((void *)&state.controller.wort_history, wire + 6000, 48);
    memcpy((void *)&state.controller.drain_subphase, wire + 6048, 1);
    memcpy((void *)&state.controller.reserved_0a9, wire + 6049, 7);
    memcpy((void *)&state.controller.shuttle_calibration_complete, wire + 6296, 1);
    memcpy((void *)&state.controller.calibration_flag_1a1, wire + 6297, 1);
    memcpy((void *)&state.controller.reserved_1a2, wire + 6298, 2);
    memcpy((void *)&state.controller.trend_seconds, wire + 6300, 4);
    memcpy((void *)&state.controller.step_seconds, wire + 6304, 4);
    memcpy((void *)&state.controller.history_count, wire + 6308, 4);
    memcpy((void *)&state.controller.previous_window_wort_fahrenheit, wire + 6312, 8);
    memcpy((void *)&state.controller.highest_wort_fahrenheit, wire + 6320, 8);
    memcpy((void *)&state.controller.reserved_1c0, wire + 6328, 16);
    memcpy((void *)&state.controller.wort_target_fahrenheit, wire + 6344, 8);
    memcpy((void *)&state.controller.thermoblock_target_fahrenheit, wire + 6352, 8);
    memcpy((void *)&state.controller.wort_pid_output, wire + 6360, 8);
    memcpy((void *)&state.controller.thermoblock_pid_output, wire + 6368, 8);
    memcpy((void *)&state.controller.wort_fahrenheit, wire + 6376, 8);
    memcpy((void *)&state.controller.thermoblock_fahrenheit, wire + 6384, 8);
    memcpy((void *)&state.controller.heater_cycle_started_ms, wire + 6392, 4);
    memcpy((void *)&state.controller.shuttle_on_time_ms, wire + 6396, 4);
    memcpy((void *)&state.controller.shuttle_cycle_started_ms, wire + 6400, 4);
    memcpy((void *)&state.controller.heater_on_time_ms, wire + 6404, 4);
    memcpy((void *)&state.controller.drain_phase_started_ms, wire + 6408, 4);
    memcpy((void *)&state.controller.reserved_214, wire + 6412, 4);
    memcpy((void *)&state.controller.active_shuttle_scale, wire + 6416, 8);
    memcpy((void *)&state.controller.candidate_shuttle_scale, wire + 6424, 8);
    memcpy((void *)&state.menu, wire + 116, 92);
    memcpy((void *)&state.recipe.identifier, wire + 220, 21);
    memcpy((void *)&state.recipe.secondary_identifier, wire + 241, 21);
    memcpy((void *)&state.recipe.reserved_2e, wire + 262, 2);
    memcpy((void *)&state.recipe.steps, wire + 264, 480);
    memcpy((void *)&state.recipe.step_count, wire + 744, 1);
    memcpy((void *)&state.recipe.reserved_211, wire + 745, 3);
    memcpy((void *)&state.recipe.integer_metric, wire + 748, 4);
    memcpy((void *)&state.recipe.decimal_metric, wire + 752, 8);
    memcpy((void *)&state.recipe.abv_adjustment, wire + 760, 4);
    memcpy((void *)&state.recipe.ibu_adjustment, wire + 764, 4);
    memcpy((void *)&state.recipe.abv_adjustment_reference, wire + 768, 4);
    memcpy((void *)&state.recipe.current_step, wire + 772, 4);
    memcpy((void *)&state.recipe.ibu_adjustment_reference, wire + 776, 4);
    memcpy((void *)&state.control_signals.progress_row, wire + 784, 1);
    memcpy((void *)&state.control_signals.progress_start_column, wire + 785, 1);
    memcpy((void *)&state.control_signals.progress_end_column, wire + 786, 1);
    memcpy((void *)&state.control_signals.progress_glyph_index, wire + 787, 1);
    memcpy((void *)&state.progress_unknown_04, wire + 788, 1);
    memcpy((void *)&state.progress_unknown_08, wire + 792, 4);
    memcpy((void *)&state.progress_unknown_0c, wire + 796, 4);
    memcpy((void *)&state.control_signals.progress_active, wire + 800, 1);
    memcpy((void *)&state.screen, wire + 6436, 76);
    memcpy((void *)&state.carousel.page_index, wire + 7632, 4);
    memcpy((void *)&state.carousel.page_count, wire + 7636, 4);
    memcpy((void *)&state.carousel.last_update_ms, wire + 7640, 4);
    memcpy((void *)&state.carousel.interval_ms, wire + 7644, 4);
    memcpy((void *)&state.device_state, wire + 7836, 1);
    memcpy((void *)&state.device_uid, wire + 7837, 33);
    memcpy((void *)&state.buzzer_enabled, wire + 7872, 1);
    memcpy((void *)&state.stepper_capabilities, wire + 7876, 4);

    load_pid(&state.controller.wort_pid, wire + 6056);
    load_pid(&state.controller.thermoblock_pid, wire + 6176);
    state.recipe.program = (PicoProgramContext *)(uintptr_t)get32(wire + 0xd8);
    for (unsigned i = 0; i < 15; ++i)
        state.carousel.pages[i] = (PicoPageCallback)(uintptr_t)get32(wire + 0x1de0 + 4 * i);
}
static void save_state(uint8_t *wire)
{
    memcpy(wire, initial, 8192);
    memcpy(wire + 5880, (const void *)&state.controller.still, 96);
    memcpy(wire + 5976, (const void *)&state.controller.compartment, 1);
    memcpy(wire + 5977, (const void *)&state.controller.status, 1);
    memcpy(wire + 5978, (const void *)&state.controller.phase, 1);
    memcpy(wire + 5979, (const void *)&state.controller.phase_label, 21);
    memcpy(wire + 6000, (const void *)&state.controller.wort_history, 48);
    memcpy(wire + 6048, (const void *)&state.controller.drain_subphase, 1);
    memcpy(wire + 6049, (const void *)&state.controller.reserved_0a9, 7);
    memcpy(wire + 6296, (const void *)&state.controller.shuttle_calibration_complete, 1);
    memcpy(wire + 6297, (const void *)&state.controller.calibration_flag_1a1, 1);
    memcpy(wire + 6298, (const void *)&state.controller.reserved_1a2, 2);
    memcpy(wire + 6300, (const void *)&state.controller.trend_seconds, 4);
    memcpy(wire + 6304, (const void *)&state.controller.step_seconds, 4);
    memcpy(wire + 6308, (const void *)&state.controller.history_count, 4);
    memcpy(wire + 6312, (const void *)&state.controller.previous_window_wort_fahrenheit, 8);
    memcpy(wire + 6320, (const void *)&state.controller.highest_wort_fahrenheit, 8);
    memcpy(wire + 6328, (const void *)&state.controller.reserved_1c0, 16);
    memcpy(wire + 6344, (const void *)&state.controller.wort_target_fahrenheit, 8);
    memcpy(wire + 6352, (const void *)&state.controller.thermoblock_target_fahrenheit, 8);
    memcpy(wire + 6360, (const void *)&state.controller.wort_pid_output, 8);
    memcpy(wire + 6368, (const void *)&state.controller.thermoblock_pid_output, 8);
    memcpy(wire + 6376, (const void *)&state.controller.wort_fahrenheit, 8);
    memcpy(wire + 6384, (const void *)&state.controller.thermoblock_fahrenheit, 8);
    memcpy(wire + 6392, (const void *)&state.controller.heater_cycle_started_ms, 4);
    memcpy(wire + 6396, (const void *)&state.controller.shuttle_on_time_ms, 4);
    memcpy(wire + 6400, (const void *)&state.controller.shuttle_cycle_started_ms, 4);
    memcpy(wire + 6404, (const void *)&state.controller.heater_on_time_ms, 4);
    memcpy(wire + 6408, (const void *)&state.controller.drain_phase_started_ms, 4);
    memcpy(wire + 6412, (const void *)&state.controller.reserved_214, 4);
    memcpy(wire + 6416, (const void *)&state.controller.active_shuttle_scale, 8);
    memcpy(wire + 6424, (const void *)&state.controller.candidate_shuttle_scale, 8);
    memcpy(wire + 116, (const void *)&state.menu, 92);
    memcpy(wire + 220, (const void *)&state.recipe.identifier, 21);
    memcpy(wire + 241, (const void *)&state.recipe.secondary_identifier, 21);
    memcpy(wire + 262, (const void *)&state.recipe.reserved_2e, 2);
    memcpy(wire + 264, (const void *)&state.recipe.steps, 480);
    memcpy(wire + 744, (const void *)&state.recipe.step_count, 1);
    memcpy(wire + 745, (const void *)&state.recipe.reserved_211, 3);
    memcpy(wire + 748, (const void *)&state.recipe.integer_metric, 4);
    memcpy(wire + 752, (const void *)&state.recipe.decimal_metric, 8);
    memcpy(wire + 760, (const void *)&state.recipe.abv_adjustment, 4);
    memcpy(wire + 764, (const void *)&state.recipe.ibu_adjustment, 4);
    memcpy(wire + 768, (const void *)&state.recipe.abv_adjustment_reference, 4);
    memcpy(wire + 772, (const void *)&state.recipe.current_step, 4);
    memcpy(wire + 776, (const void *)&state.recipe.ibu_adjustment_reference, 4);
    memcpy(wire + 784, (const void *)&state.control_signals.progress_row, 1);
    memcpy(wire + 785, (const void *)&state.control_signals.progress_start_column, 1);
    memcpy(wire + 786, (const void *)&state.control_signals.progress_end_column, 1);
    memcpy(wire + 787, (const void *)&state.control_signals.progress_glyph_index, 1);
    memcpy(wire + 788, (const void *)&state.progress_unknown_04, 1);
    memcpy(wire + 792, (const void *)&state.progress_unknown_08, 4);
    memcpy(wire + 796, (const void *)&state.progress_unknown_0c, 4);
    memcpy(wire + 800, (const void *)&state.control_signals.progress_active, 1);
    memcpy(wire + 6436, (const void *)&state.screen, 76);
    memcpy(wire + 7632, (const void *)&state.carousel.page_index, 4);
    memcpy(wire + 7636, (const void *)&state.carousel.page_count, 4);
    memcpy(wire + 7640, (const void *)&state.carousel.last_update_ms, 4);
    memcpy(wire + 7644, (const void *)&state.carousel.interval_ms, 4);
    memcpy(wire + 7836, (const void *)&state.device_state, 1);
    memcpy(wire + 7837, (const void *)&state.device_uid, 33);
    memcpy(wire + 7872, (const void *)&state.buzzer_enabled, 1);
    memcpy(wire + 7876, (const void *)&state.stepper_capabilities, 4);

    save_pid(&state.controller.wort_pid, wire + 6056);
    save_pid(&state.controller.thermoblock_pid, wire + 6176);
    put32(wire + 0xd8, pointer_id(state.recipe.program));
    for (unsigned i = 0; i < 15; ++i) {
        uint32_t v = (uint32_t)(uintptr_t)state.carousel.pages[i];
        for (unsigned j = 0; j < 5; ++j)
            if (state.carousel.pages[i] == pages[j])
                v = callbacks[j];
        put32(wire + 0x1de0 + 4 * i, v);
    }
}
static void event(uint32_t id, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    unsigned i = event_count++;
    if (i >= 8)
        return;
    events[i][0] = id;
    events[i][1] = a;
    events[i][2] = b;
    events[i][3] = c;
    events[i][4] = d;
    save_state(snapshots[i]);
}
static void zero_ip(void *c, void *o)
{
    (void)c;
    event(1, (uint32_t)(uintptr_t)o, 0, 0, 0);
}
static void http(void *c, void *o)
{
    (void)c;
    event(2, (uint32_t)(uintptr_t)o, 0, 0, 0);
}
static void dtor(void *c, void *o)
{
    (void)c;
    event(3, (uint32_t)(uintptr_t)o, 0, 0, 0);
}
static void rfid(void *c, void *o, uint8_t a, uint8_t b, uint8_t d)
{
    (void)c;
    event(4, (uint32_t)(uintptr_t)o, a, b, d);
}
static void ip(void *c, void *o, uint8_t a, uint8_t b, uint8_t d, uint8_t e)
{
    (void)c;
    event(5, (uint32_t)(uintptr_t)o, a, b, ((uint32_t)d << 8) | e);
}
static uint32_t clock_source(void *c)
{
    (void)c;
    event(6, 0, 0, 0, 0);
    return clocks[clock_index < 2 ? clock_index++ : 1];
}
void pico_state_test_init(const uint8_t *wire, uint32_t t0, uint32_t t1)
{
    load_state(wire);
    event_count = clock_index = 0;
    clocks[0] = t0;
    clocks[1] = t1;
    PicoPlatformObjects objects = {(void *)0x2002d580, (void *)0x2002d58c, (void *)0x2002c168,
                                   (void *)0x2002d570, (void *)0x2002dac8};
    PicoInitializationOps ops = {zero_ip, http, dtor, rfid, ip, clock_source};
    pico_application_initialize(&state, &objects, &ops, 0, pages);
}
void pico_state_test_state(uint8_t *out)
{
    save_state(out);
}
uint32_t pico_state_test_event_count(void)
{
    return event_count;
}
void pico_state_test_event(uint32_t n, uint32_t *out, uint8_t *memory)
{
    memcpy(out, events[n], 20);
    memcpy(memory, snapshots[n], 8192);
}
void pico_state_test_pid(unsigned op, const uint8_t *wire, double input, double output,
                         double setpoint, double a, double b, double c, int mode, uint32_t now,
                         uint8_t *out, double *io)
{
    memcpy(lone_initial, wire, 0x78);
    load_pid(&lone_pid, wire);
    pid_io[0] = input;
    pid_io[1] = output;
    pid_io[2] = setpoint;
    lone_pid.input = &pid_io[0];
    lone_pid.output = &pid_io[1];
    lone_pid.setpoint = &pid_io[2];
    switch (op) {
    case 0:
        pico_pid_construct(&lone_pid, &pid_io[0], &pid_io[1], &pid_io[2], a, b, c, mode, 3000.0,
                           now);
        break;
    case 1:
        pico_pid_compute(&lone_pid, now);
        break;
    case 2:
        pico_pid_set_tunings(&lone_pid, a, b, c);
        break;
    case 3:
        pico_pid_set_output_limits(&lone_pid, a, b);
        break;
    case 4:
        pico_pid_set_mode(&lone_pid, mode);
        break;
    case 5:
        pico_pid_set_direction(&lone_pid, mode);
        break;
    case 6:
        pico_pid_initialize_integral(&lone_pid);
        break;
    case 7:
        pico_pid_disable(&lone_pid);
        break;
    }
    memcpy(out, lone_initial, 0x78);
    save_pid(&lone_pid, out);
    memcpy(io, pid_io, 24);
}
/* Fixed-layout packet keeps the test bridge independent of host/ARM argument
 * register allocation. Doubles are copied to aligned C locals before use. */
void pico_state_test_pid_packet(const uint8_t *request, uint8_t *result)
{
    double values[6], io[3];
    memcpy(values, request + 124, 48);
    pico_state_test_pid(get32(request), request + 4, values[0], values[1], values[2], values[3],
                        values[4], values[5], (int32_t)get32(request + 172), get32(request + 176),
                        result, io);
    memcpy(result + 120, io, 24);
}
