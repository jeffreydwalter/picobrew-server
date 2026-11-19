/* Pico Pro 0.1.34 — sensors. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Reads active-low GPIO11 with100us second sample; interlock callers use filter-presence check.
 * Firmware address: 0x20004f64. */
int pico_step_filter_is_inserted(void)
{
    bool input_high;
    int inserted;

    input_high = digitalRead(0xb);
    if (input_high) {
        delayMicroseconds(100);
        input_high = digitalRead(0xb);
        inserted = 1 - (uint32_t)input_high;
        if (1 < input_high) {
            inserted = 0;
        }
        return inserted;
    }
    return 1;
}

/* Waits until GPIO11 filter-inserted predicate succeeds.
 * Firmware address: 0x20004f88. */
void pico_wait_for_step_filter(void)
{
    int inserted;

    while (inserted = pico_step_filter_is_inserted(), inserted == 0) {
        millis();
    }
    return;
}

/* Logical complement of step_filter_is_inserted.
 * Firmware address: 0x20004f98. */
uint32_t pico_step_filter_is_missing(void)
{
    uint32_t inserted;

    inserted = pico_step_filter_is_inserted();
    return (inserted ^ 1) & 0xff;
}

/* Configures GPIO11 as input for filter interlock.
 * Firmware address: 0x20005078. */
void pico_step_filter_gpio_initialize(void)
{
    pinMode(0xb, 0);
    return;
}

/* Configures analog inputs25 and26.
 * Firmware address: 0x20006666. */
uint32_t pico_temperature_inputs_initialize(void)
{
    pinMode(0x19, 0);
    pinMode(0x1a, 0);
    return 1;
}

/* Reads ADC26 and applies log/cubic thermistor equation then Fahrenheit conversion.
 * Firmware address: 0x20006680. */
float pico_temperature_read_pin26(void)
{
    uint32_t analog_read_result;
    int value;
    float __aeabi_d2f_result;
    double temperature_bits;
    double __aeabi_dmul_result;
    double __aeabi_dmul_result_2;

    analog_read_result = analogRead(0x1a);
    pinMode(0x1a, 0);
    temperature_bits = __aeabi_i2d(analog_read_result + 1);
    __aeabi_dmul_result = __aeabi_dmul(temperature_bits, 20000.0);
    temperature_bits = __aeabi_dsub(5049.863, temperature_bits);
    temperature_bits = __aeabi_ddiv(__aeabi_dmul_result, temperature_bits);
    value = __aeabi_d2iz(temperature_bits);
    temperature_bits = __aeabi_i2d(value);
    temperature_bits = libm_log(temperature_bits);
    __aeabi_d2f_result = __aeabi_d2f(temperature_bits);
    temperature_bits = __aeabi_f2d(__aeabi_d2f_result);
    __aeabi_dmul_result = __aeabi_dmul(temperature_bits, 0.0002276660423);
    __aeabi_dmul_result = __aeabi_dadd(__aeabi_dmul_result, 0.0006232719375);
    __aeabi_dmul_result_2 = __aeabi_dmul(temperature_bits, 7.184906863e-08);
    __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dmul_result_2, temperature_bits);
    temperature_bits = __aeabi_dmul(__aeabi_dmul_result_2, temperature_bits);
    temperature_bits = __aeabi_dadd(__aeabi_dmul_result, temperature_bits);
    temperature_bits = __aeabi_ddiv(1.0, temperature_bits);
    __aeabi_d2f_result = __aeabi_d2f(temperature_bits);
    temperature_bits = __aeabi_f2d(__aeabi_d2f_result);
    temperature_bits = __aeabi_dsub(temperature_bits, 273.15);
    __aeabi_d2f_result = __aeabi_d2f(temperature_bits);
    temperature_bits = __aeabi_f2d(__aeabi_d2f_result);
    temperature_bits = __aeabi_dmul(temperature_bits, 1.8);
    temperature_bits = __aeabi_dadd(temperature_bits, 32.0);
    __aeabi_d2f_result = __aeabi_d2f(temperature_bits);
    return __aeabi_d2f_result;
}

/* Reads ADC25 using second thermistor coefficients then Fahrenheit conversion.
 * Firmware address: 0x200067a0. */
float pico_temperature_read_pin25(void)
{
    uint32_t analog_read_result;
    int value;
    float __aeabi_d2f_result;
    double temperature_bits;
    double __aeabi_dmul_result;
    double __aeabi_dmul_result_2;

    analog_read_result = analogRead(0x19);
    pinMode(0x19, 0);
    temperature_bits = __aeabi_i2d(analog_read_result + 1);
    __aeabi_dmul_result = __aeabi_dmul(temperature_bits, 10000.0);
    temperature_bits = __aeabi_dsub(5049.863, temperature_bits);
    temperature_bits = __aeabi_ddiv(__aeabi_dmul_result, temperature_bits);
    value = __aeabi_d2iz(temperature_bits);
    temperature_bits = __aeabi_i2d(value);
    temperature_bits = libm_log(temperature_bits);
    __aeabi_d2f_result = __aeabi_d2f(temperature_bits);
    temperature_bits = __aeabi_f2d(__aeabi_d2f_result);
    __aeabi_dmul_result = __aeabi_dmul(temperature_bits, 0.0002351430103);
    __aeabi_dmul_result = __aeabi_dadd(__aeabi_dmul_result, 0.001127286052);
    __aeabi_dmul_result_2 = __aeabi_dmul(temperature_bits, 7.753973551e-08);
    __aeabi_dmul_result_2 = __aeabi_dmul(__aeabi_dmul_result_2, temperature_bits);
    temperature_bits = __aeabi_dmul(__aeabi_dmul_result_2, temperature_bits);
    temperature_bits = __aeabi_dadd(__aeabi_dmul_result, temperature_bits);
    temperature_bits = __aeabi_ddiv(1.0, temperature_bits);
    __aeabi_d2f_result = __aeabi_d2f(temperature_bits);
    temperature_bits = __aeabi_f2d(__aeabi_d2f_result);
    temperature_bits = __aeabi_dsub(temperature_bits, 273.15);
    __aeabi_d2f_result = __aeabi_d2f(temperature_bits);
    temperature_bits = __aeabi_f2d(__aeabi_d2f_result);
    temperature_bits = __aeabi_dmul(temperature_bits, 1.8);
    temperature_bits = __aeabi_dadd(temperature_bits, 32.0);
    __aeabi_d2f_result = __aeabi_d2f(temperature_bits);
    if ((g_session_type == 2) || (g_session_type == 4)) {
        __aeabi_d2f_result = __aeabi_fsub(__aeabi_d2f_result, 2.0);
    }
    return __aeabi_d2f_result;
}

/* Polls GPIO19 low until millisecond timeout and flushes display on success; physical role
 * unverified. Firmware address: 0x20009674. */
uint32_t pico_wait_gpio19_low(void)
{
    bool pin_high;
    uint32_t started_ms;
    uint32_t now_ms;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;

    started_ms = millis();
    do {
        pin_high = digitalRead(0x13);
        if (!pin_high) {
            pico_screen_service_status(0, unresolved_return_r1, unresolved_return_r2,
                                       unresolved_return_r3);
            pico_display_flush();
            return 1;
        }
        now_ms = millis();
    } while (started_ms - now_ms < 0x3e9);
    return 0;
}
