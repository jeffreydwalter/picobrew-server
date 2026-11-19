/* Pico Pro 0.1.34 — actuators. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Configures outputs27,8,24,5 and drives all low.
 * Firmware address: 0x20005ecc. */
void pico_actuators_initialize(void)
{
    pinMode(0x1b, 1);
    pinMode(8, 1);
    pinMode(0x18, 1);
    pinMode(5, 1);
    digitalWrite(5, 0);
    digitalWrite(0x1b, 0);
    digitalWrite(8, 0);
    digitalWrite(0x18, 0);
    return;
}

/* GPIO27 low; pump role supported by app steam-priming path.
 * Firmware address: 0x20005f12. */
void pico_steam_pump_off(void)
{
    digitalWrite(0x1b, 0);
    return;
}

/* GPIO8 low; app endBrew_runFor drain argument enables this pin.
 * Firmware address: 0x20005f1a. */
void pico_drain_pump_off(void)
{
    digitalWrite(8, 0);
    return;
}

/* GPIO24 low; app endBrew_runFor shuttle argument enables this pin.
 * Firmware address: 0x20005f22. */
void pico_shuttle_pump_off(void)
{
    digitalWrite(0x18, 0);
    return;
}

/* Drives GPIO27 and GPIO8 low.
 * Firmware address: 0x20005f2a. */
void pico_steam_and_drain_pumps_off(void)
{
    digitalWrite(0x1b, 0);
    digitalWrite(8, 0);
    return;
}

/* GPIO5 high; app heat-to-target and heater diagnostic call this routine.
 * Firmware address: 0x20005f40. */
void pico_heater_on(void)
{
    digitalWrite(5, 1);
    return;
}

/* GPIO5 low; paired with heater_on in heat-to-target flow.
 * Firmware address: 0x20005f48. */
void pico_heater_off(void)
{
    digitalWrite(5, 0);
    return;
}

/* Sets heater, shuttle, steam and drain GPIO outputs low.
 * Firmware address: 0x20005f88. */
void pico_actuators_all_off(void)
{
    digitalWrite(5, 0);
    digitalWrite(0x18, 0);
    digitalWrite(0x1b, 0);
    digitalWrite(8, 0);
    return;
}

/* Sets heater/shuttle/steam outputs low while leaving drain unchanged.
 * Firmware address: 0x20005fae. */
void pico_actuators_off_except_drain(void)
{
    digitalWrite(5, 0);
    digitalWrite(0x18, 0);
    digitalWrite(0x1b, 0);
    return;
}

/* Reads GPIO8 output state.
 * Firmware address: 0x20005fcc. */
bool pico_drain_pump_is_on(void)
{
    bool pin_high;

    pin_high = digitalRead(8);
    return pin_high;
}
