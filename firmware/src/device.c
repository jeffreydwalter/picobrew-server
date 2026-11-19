#include "picobrew/device.h"
#include <stdio.h>
#include "picobrew/local_control.h"

/* The auxiliary controller uses the shared SPI bus in mode 1. These logical
 * Energia pin identifiers are verified against the firmware's board tables. */
enum { RFID_SELECT = 4, AUXILIARY_SELECT = 18, SERVICE_BUTTON = 19 };

static uint32_t now(PicoDevice *d)
{
    return d->ops->millis(d->context);
}
static void delay_ms(PicoDevice *d, uint32_t ms)
{
    d->ops->delay_ms(d->context, ms);
}
static void select_auxiliary(PicoDevice *d)
{
    d->ops->spi_mode(d->context, 1);
    d->ops->digital_write(d->context, RFID_SELECT, 1);
    d->ops->digital_write(d->context, AUXILIARY_SELECT, 0);
}
static void release_auxiliary(PicoDevice *d)
{
    d->ops->digital_write(d->context, RFID_SELECT, 1);
    d->ops->digital_write(d->context, AUXILIARY_SELECT, 1);
    d->ops->spi_mode(d->context, 0);
}

/* Original 0x2000621c. */
void pico_device_spi_initialize(PicoDevice *d)
{
    d->ops->pin_mode(d->context, RFID_SELECT, 1);
    d->ops->pin_mode(d->context, AUXILIARY_SELECT, 1);
    d->ops->digital_write(d->context, RFID_SELECT, 1);
    d->ops->digital_write(d->context, AUXILIARY_SELECT, 1);
    d->state->buzzer_enabled = 0;
    d->state->stepper_capabilities = 0;
}

/* Original 0x2000624c. */
uint8_t pico_device_spi_selected_byte(PicoDevice *d, uint8_t value)
{
    select_auxiliary(d);
    delay_ms(d, 100);
    uint8_t result = d->ops->spi_transfer(d->context, value);
    release_auxiliary(d);
    return result;
}

/* Original 0x2000629c; unconditional interrupt enable matches the image. */
uint8_t pico_device_spi_command(PicoDevice *d, uint8_t command)
{
    d->ops->interrupts_disable(d->context);
    select_auxiliary(d);
    delay_ms(d, 1);
    d->ops->spi_transfer(d->context, command);
    delay_ms(d, 1);
    uint8_t result = d->ops->spi_transfer(d->context, 0xff);
    delay_ms(d, 1);
    release_auxiliary(d);
    d->ops->interrupts_enable(d->context);
    return result;
}

/* Original 0x2000ca5c. */
bool pico_device_stepper_ready(PicoDevice *d)
{
    uint8_t response = pico_device_spi_command(d, 0x37);
    if (response == 0x58)
        d->ops->report_error(d->context, 18, false, true);
    return response == 12;
}

/* Original 0x2000ca7e: 151 probes and at most 150 intervening delays. */
bool pico_device_wait_stepper(PicoDevice *d)
{
    for (unsigned remaining = 151; remaining != 0; --remaining) {
        if (pico_local_cancelled())
            return false;
        if (pico_device_stepper_ready(d))
            return true;
        if (remaining != 1)
            delay_ms(d, 300);
    }
    d->ops->display_clear(d->context);
    d->ops->report_error(d->context, 4, false, true);
    return false;
}

/* Original entry points 0x2000cb40/on and 0x2000cb46/off share one body. */
bool pico_device_buzzer(PicoDevice *d, bool enabled)
{
    if (d->state->buzzer_enabled == (uint8_t)enabled) {
        d->ops->log(d->context, "W", "SPI", "setBuzzer() useless");
        return true;
    }
    uint8_t response = pico_device_spi_command(d, 11);
    if (response == 11) {
        response = pico_device_spi_command(d, 11);
        pico_device_wait_stepper(d);
        if (response == 2 || response == 3) {
            d->state->buzzer_enabled = response == 2;
            return true;
        }
        d->ops->log(d->context, "W", "SPI", "Unknown alarm state");
    } else {
        pico_device_wait_stepper(d);
        d->ops->log(d->context, "E", "SPI", "Failed to toggle alarm");
    }
    return false;
}

/* Original 0x2000ccac. */
uint32_t pico_device_stepper_capabilities(PicoDevice *d, uint32_t *detail)
{
    if (pico_device_spi_command(d, 0x29) != 0x29) {
        pico_device_wait_stepper(d);
        return UINT32_MAX;
    }
    uint8_t capabilities = pico_device_spi_command(d, 0x29);
    *detail = pico_device_spi_selected_byte(d, 0x29);
    return capabilities;
}

/* Original 0x2000ccd8. Detection does not clear a previously set legacy flag
 * when a later query supports new mapping; preserve that original behavior. */
void pico_device_detect_stepper(PicoDevice *d)
{
    uint32_t detail = 0;
    delay_ms(d, 1000);
    uint32_t capabilities = pico_device_stepper_capabilities(d, &detail);
    if (capabilities == UINT32_MAX) {
        pico_device_wait_stepper(d);
        capabilities = pico_device_stepper_capabilities(d, &detail);
    }
    if ((capabilities & 1u) == 0) {
        *d->legacy_compartment_mapping = 1;
        *d->home_compartment = 0;
    }
    d->state->stepper_capabilities = capabilities;
}

/* Original 0x2000cd24; comparison is to the full requested value. */
bool pico_device_set_torque(PicoDevice *d, uint32_t mode)
{
    pico_device_wait_stepper(d);
    if (pico_device_spi_command(d, 0x3b) != 0x3b)
        return false;
    if ((uint32_t)pico_device_spi_command(d, (uint8_t)mode) == mode)
        return true;
    pico_device_wait_stepper(d);
    return false;
}

/* Original 0x2000cd4c. The full command/echo/dwell handshake is retained. */
bool pico_device_move_compartment(PicoDevice *d, uint32_t compartment)
{
    if (pico_local_cancelled())
        return false;
    char message[64];
    snprintf(message, sizeof message, "moveStepperToCompartment %d", (int)compartment);
    d->ops->log(d->context, "I", "STEP", message);
    if (pico_device_spi_command(d, 0x21) == 0x21) {
        uint32_t mapped = compartment;
        if (*d->legacy_compartment_mapping)
            mapped = compartment == 0 ? 6 : (compartment - 1u) & 0xffu;
        uint8_t command = (uint8_t)(mapped + 1u);
        if ((uint32_t)pico_device_spi_command(d, command) == mapped + 1u) {
            uint8_t dwell = pico_device_spi_command(d, command);
            if (dwell < 7) {
                delay_ms(d, (uint32_t)dwell * 500u);
                pico_device_wait_stepper(d);
                d->state->controller.compartment = (uint8_t)compartment;
                return true;
            }
            d->ops->report_error(d->context, 5, false, true);
            return false;
        }
    }
    pico_device_wait_stepper(d);
    return false;
}

void pico_device_move_home(PicoDevice *d)
{
    pico_device_move_compartment(d, *d->home_compartment);
}

void pico_device_filter_input_initialize(PicoDevice *d)
{
    d->ops->pin_mode(d->context, PICO_PIN_STEP_FILTER, 0);
}

bool pico_device_temperature_inputs_initialize(PicoDevice *d)
{
    d->ops->pin_mode(d->context, PICO_PIN_WORT_SENSOR, 0);
    d->ops->pin_mode(d->context, PICO_PIN_THERMOBLOCK_SENSOR, 0);
    return true;
}

void pico_device_wait_for_filter(PicoDevice *d)
{
    while (!pico_local_cancelled() && !pico_filter_is_inserted(d->actuators))
        now(d);
}

/* Original 0x20009674 contains the reversed subtraction. It is retained and
 * separately documented rather than silently treating it as a normal timeout. */
bool pico_device_service_button(PicoDevice *d)
{
    uint32_t started = now(d);
    do {
        if (d->ops->digital_read(d->context, SERVICE_BUTTON) == 0) {
            d->ops->service_screen(d->context);
            d->ops->display_flush(d->context);
            return true;
        }
    } while ((uint32_t)(started - now(d)) <= 1000u);
    return false;
}

void pico_device_alarm_while_filter_missing(PicoDevice *d)
{
    d->ops->log(d->context, "I", "SCR", "playAlarmWhileStepFilterMissing()");
    d->ops->log(d->context, "I", "SCR", "playAlarmWhile()");
    pico_device_buzzer(d, false);
    bool on = false;
    uint32_t next = 0;
    while (!pico_local_cancelled() && !pico_filter_is_inserted(d->actuators)) {
        now(d);
        uint32_t current = now(d);
        if (next < current) {
            pico_device_buzzer(d, !on);
            next = current + 1000;
            on = !on;
        }
    }
    if (on)
        pico_device_buzzer(d, false);
}

void pico_device_alarm_until_encoder_press(PicoDevice *d)
{
    uint32_t started = now(d);
    d->ops->encoder_clear_click(d->context);
    d->ops->display_clear(d->context);
    d->ops->error_heading(d->context);
    d->ops->display_flush(d->context);
    pico_device_buzzer(d, true);
    bool on = true;
    while (!pico_local_cancelled() && !d->ops->encoder_take_click(d->context)) {
        if ((uint32_t)(now(d) - started) > 1000u) {
            pico_device_buzzer(d, !on);
            started = now(d);
            on = !on;
        }
    }
    if (on)
        pico_device_buzzer(d, false);
}

bool pico_device_check_thermoblock_heating(PicoDevice *d, PicoControl *control)
{
    if (pico_local_cancelled())
        return false;
    if (control->controller->thermoblock_fahrenheit < 230.0) {
        pico_actuator_heater_on(d->actuators);
        uint32_t started = now(d);
        while (control->controller->thermoblock_fahrenheit < 230.0) {
            if (pico_local_cancelled())
                return false;
            if ((uint32_t)(now(d) - started) > 15000u &&
                control->controller->thermoblock_fahrenheit < 90.0) {
                pico_control_stop(control);
                pico_actuators_off_except_drain(d->actuators);
                d->ops->report_error(d->context, 19, true, true);
                return false;
            }
        }
        pico_actuator_heater_off(d->actuators);
    }
    return true;
}
