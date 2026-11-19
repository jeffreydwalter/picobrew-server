/* Pico Pro 0.1.34 — timers. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Configures timer0 at80M ticks and timer1 at1.6M ticks, registers handlers, enables both.
 * Firmware address: 0x20005080. */
bool pico_control_timers_initialize(void)
{
    bool int_master_enable_result;

    IntMasterDisable();
    PRCMPeripheralClkEnable(0xe, 1);
    PRCMPeripheralReset(0xe);
    TimerConfigure(0x40030000, 0x22);
    TimerLoadSet(0x40030000, 0xff, 80000000);
    TimerIntRegister(0x40030000, 0xff, pico_temperature_control_timer_isr + 1);
    TimerIntEnable(0x40030000, 1);
    PRCMPeripheralClkEnable(0xf, 1);
    PRCMPeripheralReset(0xf);
    TimerConfigure(0x40031000, 0x22);
    TimerLoadSet(0x40031000, 0xff, 1600000);
    TimerIntRegister(0x40031000, 0xff, pico_heater_pump_timer_isr + 1);
    TimerIntEnable(0x40031000, 1);
    TimerEnable(0x40031000, 0xff);
    TimerEnable(0x40030000, 0xff);
    int_master_enable_result = IntMasterEnable();
    return int_master_enable_result;
}
