/* Appliance wiring and boot/menu orchestration tests; excluded from firmware. */
#include "picobrew/application.h"
#include <string.h>
PicoApplication application_fixture_app;
PicoControl application_fixture_control;
PicoActuators application_fixture_actuators;
uint32_t application_fixture_addresses[8];
uint32_t device_fixture_millis(void *c);
void device_fixture_delay(void *c, uint32_t m);
void device_fixture_pin_mode(void *c, uint8_t p, uint8_t m);
int device_fixture_read(void *c, uint8_t p);
void device_fixture_write(void *c, uint8_t p, uint8_t v);
void device_fixture_mode(void *c, uint8_t m);
uint8_t device_fixture_transfer(void *c, uint8_t v);
void device_fixture_disable(void *c);
void application_fixture_attach(void *c, unsigned p, void (*h)(void), unsigned m)
{
    (void)c;
    (void)p;
    (void)h;
    (void)m;
}
bool application_fixture_enable(void *c)
{
    (void)c;
    return true;
}
void application_fixture_serial_begin(void *c, uint32_t b)
{
    (void)c;
    (void)b;
}
void application_fixture_serial(void *c, const char *s)
{
    (void)c;
    (void)s;
}
bool application_fixture_wifi_init(void *c)
{
    (void)c;
    return true;
}
int application_fixture_wifi_status(void *c)
{
    (void)c;
    return 3;
}
void application_fixture_reset(void *c)
{
    (void)c;
}
static const PicoApplicationHardware hardware = {device_fixture_millis,
                                                 device_fixture_delay,
                                                 device_fixture_pin_mode,
                                                 device_fixture_read,
                                                 device_fixture_write,
                                                 device_fixture_disable,
                                                 device_fixture_transfer,
                                                 device_fixture_mode,
                                                 device_fixture_mode,
                                                 device_fixture_disable,
                                                 application_fixture_enable,
                                                 application_fixture_attach,
                                                 application_fixture_serial_begin};
static const PicoNetworkPlatform network_platform = {.serial_write = application_fixture_serial,
                                                     .wifi_initialize =
                                                         application_fixture_wifi_init,
                                                     .wifi_status = application_fixture_wifi_status,
                                                     .reset_mcu = application_fixture_reset};
void application_fixture_bind(void)
{
    PicoApplication *a = &application_fixture_app;
    memset(a, 0, sizeof *a);
    a->state.recipe.program = &a->state.session;
    application_fixture_control.controller = &a->state.controller;
    application_fixture_control.signals = &a->state.control_signals;
    pico_application_bind_services(a, &application_fixture_control, &application_fixture_actuators,
                                   &hardware, 0, &network_platform, 0);
    application_fixture_addresses[0] = (uint32_t)a;
    application_fixture_addresses[1] = (uint32_t)&a->settings.offline_mode;
    application_fixture_addresses[2] = (uint32_t)&a->state.session.session_type;
    application_fixture_addresses[3] = (uint32_t)&a->settings.language;
    application_fixture_addresses[4] = (uint32_t)&a->settings.detailed_status;
    application_fixture_addresses[5] = (uint32_t)&a->ui;
    application_fixture_addresses[6] = (uint32_t)&a->ui.display.framebuffer;
    application_fixture_addresses[7] = (uint32_t)&a->state.recipe;
}
static bool complete(const void *table, size_t size)
{
    const uint8_t *bytes = table;
    for (size_t offset = 0; offset < size; offset += 4) {
        uint32_t address;
        memcpy(&address, bytes + offset, 4);
        if (!address)
            return false;
    }
    return true;
}
bool application_fixture_bindings_complete(void)
{
    PicoApplication *a = &application_fixture_app;
    return complete(a->device.ops, sizeof *a->device.ops) &&
           complete(a->reader.ops, sizeof *a->reader.ops) &&
           complete(a->rfid.ops, sizeof *a->rfid.ops) &&
           complete(a->ui.hardware, sizeof *a->ui.hardware) &&
           complete(a->ui.services, sizeof *a->ui.services) &&
           complete(a->network.ui, sizeof *a->network.ui) &&
           complete(a->phase.ops, sizeof *a->phase.ops) &&
           complete(a->recipe.ops, sizeof *a->recipe.ops);
}
void application_fixture_network_clear(void)
{
    PicoApplication *a = &application_fixture_app;
    a->network.ui->event(a->network.ui_context, PICO_NET_UI_CLEAR, 0);
}
void application_fixture_choose_language(void)
{
    PicoApplication *a = &application_fixture_app;
    a->network.ui->choose_language(a->network.ui_context);
}
