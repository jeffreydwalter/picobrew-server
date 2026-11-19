/* Test-only I/O fixture. Never included in the application library. */
#include "picobrew/actuators.h"

enum {
    EVENT_READ = 1,
    EVENT_WRITE,
    EVENT_MODE,
    EVENT_DELAY_US,
    EVENT_DELAY_MS,
    EVENT_PAUSE,
    EVENT_PROMPT
};
static uint32_t events[128][3];
static unsigned event_count;
static unsigned read_index;
static int read_values[2];
static uint8_t phase;
static bool filter_restored;

static void event(unsigned type, unsigned a, unsigned b)
{
    if (event_count < 128) {
        events[event_count][0] = type;
        events[event_count][1] = a;
        events[event_count][2] = b;
        ++event_count;
    }
}
static void mode(void *ctx, unsigned pin, unsigned value)
{
    (void)ctx;
    event(EVENT_MODE, pin, value);
}
static int read_pin(void *ctx, unsigned pin)
{
    (void)ctx;
    int value = filter_restored ? 0 : read_values[read_index++ == 0 ? 0 : 1];
    event(EVENT_READ, pin, (unsigned)value);
    return value;
}
static void write_pin(void *ctx, unsigned pin, unsigned value)
{
    (void)ctx;
    event(EVENT_WRITE, pin, value);
}
static void delay_us(void *ctx, uint32_t value)
{
    (void)ctx;
    event(EVENT_DELAY_US, value, 0);
}
static void delay_ms(void *ctx, uint32_t value)
{
    (void)ctx;
    event(EVENT_DELAY_MS, value, 0);
}
static void pause_brew(void *ctx)
{
    (void)ctx;
    event(EVENT_PAUSE, 0, 0);
    filter_restored = true;
}
static void prompt(void *ctx)
{
    (void)ctx;
    event(EVENT_PROMPT, 0, 0);
    filter_restored = true;
}
static uint8_t current_phase(void *ctx)
{
    (void)ctx;
    return phase;
}
static const PicoActuatorOps ops = {mode,     read_pin,   write_pin, delay_us,
                                    delay_ms, pause_brew, prompt,    current_phase};
static PicoActuators actuators = {&ops, 0};

void pico_test_actuator_configure(uint32_t current, uint32_t first, uint32_t second)
{
    event_count = 0;
    read_index = 0;
    phase = (uint8_t)current;
    read_values[0] = (int)first;
    read_values[1] = (int)second;
    filter_restored = false;
}

uint32_t pico_test_actuator_run(uint32_t operation, uint32_t resume_mask)
{
    switch (operation) {
    case 0:
        pico_actuators_initialize(&actuators);
        break;
    case 1:
        return pico_filter_is_inserted(&actuators);
    case 2:
        pico_actuators_all_off(&actuators);
        break;
    case 3:
        pico_actuators_off_except_drain(&actuators);
        break;
    case 4:
        pico_actuators_steam_and_drain_off(&actuators);
        break;
    case 5:
        pico_actuator_heater_on(&actuators);
        break;
    case 6:
        pico_actuator_heater_off(&actuators);
        break;
    case 7:
        pico_actuator_steam_on(&actuators);
        break;
    case 8:
        pico_actuator_steam_off(&actuators);
        break;
    case 9:
        pico_actuator_drain_on(&actuators);
        break;
    case 10:
        pico_actuator_drain_off(&actuators);
        break;
    case 11:
        pico_actuator_shuttle_on(&actuators);
        break;
    case 12:
        pico_actuator_shuttle_off(&actuators);
        break;
    case 13:
        return pico_actuator_drain_is_on(&actuators);
    case 14:
        pico_actuators_start_drain_and_steam(&actuators);
        break;
    case 15:
        return pico_actuators_pause_for_missing_filter(
            &actuators, (resume_mask & 1u) != 0, (resume_mask & 2u) != 0, (resume_mask & 4u) != 0);
    default:
        return 0xdeadbeef;
    }
    return 0;
}

uint32_t pico_test_actuator_event_count(void)
{
    return event_count;
}
uint32_t pico_test_actuator_event_word(uint32_t index, uint32_t word)
{
    return index < event_count && word < 3 ? events[index][word] : 0xffffffff;
}
