/* Test boundaries only; not part of the production archive. */
#include "picobrew/device.h"
PicoAppState device_fixture_state;
PicoDevice device_fixture_device;
PicoControl device_fixture_control;
uint8_t device_fixture_legacy, device_fixture_home;
uint32_t device_fixture_detail, device_fixture_addresses[8];
uint32_t device_fixture_millis(void *c)
{
    (void)c;
    return 0;
}
void device_fixture_delay(void *c, uint32_t m)
{
    (void)c;
    (void)m;
}
void device_fixture_pin_mode(void *c, uint8_t p, uint8_t m)
{
    (void)c;
    (void)p;
    (void)m;
}
int device_fixture_read(void *c, uint8_t p)
{
    (void)c;
    (void)p;
    return 0;
}
void device_fixture_write(void *c, uint8_t p, uint8_t v)
{
    (void)c;
    (void)p;
    (void)v;
}
void device_fixture_mode(void *c, uint8_t m)
{
    (void)c;
    (void)m;
}
uint8_t device_fixture_transfer(void *c, uint8_t v)
{
    (void)c;
    (void)v;
    return 0;
}
void device_fixture_disable(void *c)
{
    (void)c;
}
void device_fixture_enable(void *c)
{
    (void)c;
}
void device_fixture_log(void *c, const char *l, const char *m, const char *s)
{
    (void)c;
    (void)l;
    (void)m;
    (void)s;
}
void device_fixture_error(void *c, unsigned e, bool a, bool r)
{
    (void)c;
    (void)e;
    (void)a;
    (void)r;
}
void device_fixture_clear(void *c)
{
    (void)c;
}
void device_fixture_flush(void *c)
{
    (void)c;
}
void device_fixture_service(void *c)
{
    (void)c;
}
void device_fixture_heading(void *c)
{
    (void)c;
}
void device_fixture_clear_click(void *c)
{
    (void)c;
}
bool device_fixture_click(void *c)
{
    (void)c;
    return false;
}
static const PicoDeviceOps ops = {
    device_fixture_millis,      device_fixture_delay,   device_fixture_pin_mode,
    device_fixture_read,        device_fixture_write,   device_fixture_mode,
    device_fixture_transfer,    device_fixture_disable, device_fixture_enable,
    device_fixture_log,         device_fixture_error,   device_fixture_clear,
    device_fixture_flush,       device_fixture_service, device_fixture_heading,
    device_fixture_clear_click, device_fixture_click};
void device_fixture_bind(void)
{
    device_fixture_device = (PicoDevice){&device_fixture_state, 0,    &device_fixture_legacy,
                                         &device_fixture_home,  &ops, 0};
    device_fixture_control.controller = &device_fixture_state.controller;
    device_fixture_addresses[0] = (uint32_t)&device_fixture_state.controller;
    device_fixture_addresses[1] = (uint32_t)&device_fixture_state.buzzer_enabled;
    device_fixture_addresses[2] = (uint32_t)&device_fixture_state.stepper_capabilities;
    device_fixture_addresses[3] = (uint32_t)&device_fixture_legacy;
    device_fixture_addresses[4] = (uint32_t)&device_fixture_home;
    device_fixture_addresses[5] = (uint32_t)&device_fixture_detail;
    device_fixture_addresses[6] = (uint32_t)&device_fixture_device;
    device_fixture_addresses[7] = (uint32_t)&device_fixture_control;
}
