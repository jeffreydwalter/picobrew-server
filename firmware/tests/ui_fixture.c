/* Trapping observation points: test fixtures only, never device firmware. */
#include "picobrew/ui.h"
#include <stddef.h>
#include <string.h>
#define TRAP() __asm__ volatile("bkpt 0")
uint32_t ui_fixture_millis(void *context)
{
    (void)context;
    TRAP();
    return 0;
}
void ui_fixture_delay(void *context, uint32_t ms)
{
    (void)context;
    (void)ms;
    TRAP();
}
void ui_fixture_pin_mode(void *context, unsigned pin, unsigned mode)
{
    (void)context;
    (void)pin;
    (void)mode;
    TRAP();
}
int ui_fixture_read(void *context, unsigned pin)
{
    (void)context;
    (void)pin;
    TRAP();
    return 0;
}
void ui_fixture_write(void *context, unsigned pin, unsigned value)
{
    (void)context;
    (void)pin;
    (void)value;
    TRAP();
}
void ui_fixture_spi_begin(void *context)
{
    (void)context;
    TRAP();
}
uint8_t ui_fixture_spi(void *context, uint8_t value)
{
    (void)context;
    (void)value;
    TRAP();
    return 0;
}
void ui_fixture_disable(void *context)
{
    (void)context;
    TRAP();
}
bool ui_fixture_enable(void *context)
{
    (void)context;
    TRAP();
    return false;
}
void ui_fixture_attach(void *context, unsigned irq, void (*handler)(void), unsigned mode)
{
    (void)context;
    (void)irq;
    (void)handler;
    (void)mode;
    TRAP();
}
bool ui_fixture_buzzer(void *context, bool enabled)
{
    (void)context;
    (void)enabled;
    TRAP();
    return false;
}
void ui_fixture_pause(void *context)
{
    (void)context;
    TRAP();
}
void ui_fixture_wait_filter(void *context)
{
    (void)context;
    TRAP();
}
void ui_fixture_alarm(void *context)
{
    (void)context;
    TRAP();
}
uint32_t ui_fixture_instruction(void *context, uint32_t id, bool sound)
{
    (void)context;
    (void)id;
    (void)sound;
    TRAP();
    return 0;
}
bool ui_fixture_default_recipe(void *context, const char *id)
{
    (void)context;
    (void)id;
    TRAP();
    return false;
}
extern PicoNetwork fixture_network;
extern PicoAppState fixture_app;
extern PicoSettings fixture_settings;
extern void fixture_bind(void);
PicoUi ui_fixture;
PicoActuators ui_fixture_actuators;
PicoControl ui_fixture_control;
const PicoUiHardware ui_fixture_hardware = {
    ui_fixture_millis, ui_fixture_delay,     ui_fixture_pin_mode, ui_fixture_read,
    ui_fixture_write,  ui_fixture_spi_begin, ui_fixture_spi,      ui_fixture_disable,
    ui_fixture_enable, ui_fixture_attach};
const PicoUiServices ui_fixture_services = {.pause_until_resume = ui_fixture_pause,
                                            .buzzer = ui_fixture_buzzer,
                                            .wait_for_filter = ui_fixture_wait_filter,
                                            .alarm_until_press = ui_fixture_alarm,
                                            .alarm_filter_missing = ui_fixture_alarm,
                                            .execute_instruction = ui_fixture_instruction,
                                            .fetch_default_recipe_failed =
                                                ui_fixture_default_recipe};
const uint32_t ui_fixture_offsets[] = {offsetof(PicoUi, display),
                                       offsetof(PicoUi, encoder),
                                       offsetof(PicoUi, menu_labels),
                                       offsetof(PicoDisplay, framebuffer),
                                       offsetof(PicoDisplay, cursor_x),
                                       offsetof(PicoDisplay, text_scale),
                                       offsetof(PicoSettings, language),
                                       offsetof(PicoSettings, temperature_unit_celsius),
                                       offsetof(PicoSettings, detailed_status),
                                       offsetof(PicoAppState, recipe),
                                       offsetof(PicoAppState, screen),
                                       offsetof(PicoAppState, session),
                                       offsetof(PicoAppState, carousel),
                                       offsetof(PicoAppState, control_signals),
                                       sizeof(PicoEncoder),
                                       sizeof(PicoAppState),
                                       sizeof(PicoUi)};
void ui_fixture_bind(void)
{
    fixture_bind();
    pico_ui_initialize(&ui_fixture, &fixture_app, &fixture_network, &ui_fixture_actuators,
                       &ui_fixture_control, &ui_fixture_hardware, NULL, &ui_fixture_services, NULL);
    fixture_app.recipe.program = &fixture_app.session;
    fixture_app.session.title = "Pico Program";
    fixture_app.session.name = "Brewing";
    fixture_app.session.state_label = "Brew";
    fixture_app.session.logo_text = "NOW BREWING";
    fixture_app.session.interrupted_text = "Continue or abort";
    fixture_app.session.bitmap_primary = &pico_ui_asset_program_primary;
    fixture_app.session.bitmap_secondary = &pico_ui_asset_program_secondary;
    fixture_app.session.duration = 90;
    fixture_app.session.temperature = 153;
    fixture_app.session.session_type = 2;
    strcpy(fixture_app.screen.recipe_name, "Test Brew");
    strcpy(fixture_app.screen.step_name, "Heat water");
    strcpy(fixture_app.recipe.steps[0].name, "Step One");
    fixture_app.recipe.current_step = 0;
    fixture_app.controller.wort_target_fahrenheit = 154.5;
    fixture_app.controller.phase = 4;
    pico_graphics_text_colors(&ui_fixture.display, 1, 0);
    pico_graphics_rotation(&ui_fixture.display, 2);
    strcpy(ui_fixture.menu_labels[0], "First");
    strcpy(ui_fixture.menu_labels[1], "Second");
    strcpy(ui_fixture.menu_labels[2], "Third");
}

const uint32_t ui_fixture_menu_offset = offsetof(PicoAppState, menu);

/* This image is an emulator fixture, never an appliance installation image. */
void pico_ui_validation_entry(void)
{
    for (;;) {
    }
}
