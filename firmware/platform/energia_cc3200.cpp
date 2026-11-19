#include "picobrew/energia_cc3200.h"
#include "picobrew/config.h"
#include "picobrew/sensors.h"

#include <new>
extern "C" {
#include <driverlib/interrupt.h>
#include <driverlib/prcm.h>
#include <driverlib/timer.h>
int __aeabi_atexit(void *object, void (*destructor)(void *), void *dso);
extern void *__dso_handle;
}

static PicoEnergiaControl *runtime(void *context)
{
    return static_cast<PicoEnergiaControl *>(context);
}
static uint32_t clock_ms(void *)
{
    return static_cast<uint32_t>(millis());
}
static void interrupts_disable(void *)
{
    IntMasterDisable();
}
static bool interrupts_enable(void *)
{
    return IntMasterEnable();
}
static uint32_t timer_status(void *, uint32_t base, bool masked)
{
    return TimerIntStatus(base, masked);
}
static void timer_clear(void *, uint32_t base, uint32_t flags)
{
    TimerIntClear(base, flags);
}
static void pin_mode(void *, unsigned pin, unsigned mode)
{
    pinMode(static_cast<uint8_t>(pin), static_cast<uint8_t>(mode));
}
static int digital_read(void *, unsigned pin)
{
    return digitalRead(static_cast<uint8_t>(pin));
}
static void digital_write(void *, unsigned pin, unsigned value)
{
    digitalWrite(static_cast<uint8_t>(pin), static_cast<uint8_t>(value));
}
static void delay_us(void *, uint32_t duration)
{
    delayMicroseconds(duration);
}
static void delay_ms(void *, uint32_t duration)
{
    delay(duration);
}
static void pause_brew(void *context)
{
    PicoEnergiaControl *r = runtime(context);
    r->ui->pause_until_resume(r->ui_context);
}
static void prompt_filter(void *context)
{
    PicoEnergiaControl *r = runtime(context);
    r->ui->prompt_insert_filter(r->ui_context);
}
static uint8_t brew_phase(void *context)
{
    return runtime(context)->state->controller.phase;
}
static float read_wort(void *context)
{
    uint32_t count = analogRead(PICO_PIN_WORT_SENSOR);
    pinMode(PICO_PIN_WORT_SENSOR, INPUT);
    return pico_temperature_pin25_from_adc(
        count, static_cast<int>(runtime(context)->state->session.session_type));
}
static float read_thermoblock(void *)
{
    uint32_t count = analogRead(PICO_PIN_THERMOBLOCK_SENSOR);
    pinMode(PICO_PIN_THERMOBLOCK_SENSOR, INPUT);
    return pico_temperature_pin26_from_adc(count);
}
static void heater_on(void *context)
{
    pico_actuator_heater_on(&runtime(context)->actuators);
}
static void heater_off(void *context)
{
    pico_actuator_heater_off(&runtime(context)->actuators);
}
static void shuttle_on(void *context)
{
    pico_actuator_shuttle_on(&runtime(context)->actuators);
}
static void shuttle_off(void *context)
{
    pico_actuator_shuttle_off(&runtime(context)->actuators);
}
static void steam_off(void *context)
{
    pico_actuator_steam_off(&runtime(context)->actuators);
}
static void drain_off(void *context)
{
    pico_actuator_drain_off(&runtime(context)->actuators);
}
static void progress_dot(void *context, uint16_t row, uint16_t column)
{
    PicoEnergiaControl *r = runtime(context);
    r->ui->progress_dot(r->ui_context, row, column);
}
static void progress_clear(void *context, uint16_t row, uint16_t start_column)
{
    PicoEnergiaControl *r = runtime(context);
    r->ui->progress_clear(r->ui_context, row, start_column);
}
static void flush_display(void *context)
{
    PicoEnergiaControl *r = runtime(context);
    r->ui->display_flush(r->ui_context);
}
static void halt_heater(void *context)
{
    PicoEnergiaControl *r = runtime(context);
    r->ui->halt_heater_malfunction(r->ui_context);
    /* The original fatal path does not return. A wrongly returning application
     * handler must not allow the control loop to resume after this fault. */
    for (;;) {
    }
}
static void clock_enable(void *, uint32_t peripheral, uint32_t mode)
{
    PRCMPeripheralClkEnable(peripheral, mode);
}
static void peripheral_reset(void *, uint32_t peripheral)
{
    PRCMPeripheralReset(peripheral);
}
static void timer_configure(void *, uint32_t base, uint32_t config)
{
    TimerConfigure(base, config);
}
static void timer_load(void *, uint32_t base, uint32_t timer, uint32_t ticks)
{
    TimerLoadSet(base, timer, ticks);
}
static void timer_register(void *, uint32_t base, uint32_t timer, void (*handler)(void))
{
    TimerIntRegister(base, timer, handler);
}
static void timer_interrupt_enable(void *, uint32_t base, uint32_t flags)
{
    TimerIntEnable(base, flags);
}
static void timer_enable(void *, uint32_t base, uint32_t timer)
{
    TimerEnable(base, timer);
}

static const PicoActuatorOps actuator_ops = {pin_mode, digital_read, digital_write, delay_us,
                                             delay_ms, pause_brew,   prompt_filter, brew_phase};
static const PicoControlOps control_ops = {clock_ms,
                                           interrupts_disable,
                                           interrupts_enable,
                                           timer_status,
                                           timer_clear,
                                           read_wort,
                                           read_thermoblock,
                                           heater_on,
                                           heater_off,
                                           shuttle_on,
                                           shuttle_off,
                                           steam_off,
                                           drain_off,
                                           progress_dot,
                                           progress_clear,
                                           flush_display,
                                           halt_heater,
                                           clock_enable,
                                           peripheral_reset,
                                           timer_configure,
                                           timer_load,
                                           timer_register,
                                           timer_interrupt_enable,
                                           timer_enable};

bool pico_energia_bind_control(PicoEnergiaControl *r, PicoAppState *state, const PicoEnergiaUi *ui,
                               void *ui_context)
{
    if (!r || !state || !ui || !ui->pause_until_resume || !ui->prompt_insert_filter ||
        !ui->progress_dot || !ui->progress_clear || !ui->display_flush ||
        !ui->halt_heater_malfunction)
        return false;
    r->state = state;
    r->ui = ui;
    r->ui_context = ui_context;
    r->actuators = PicoActuators{&actuator_ops, r};
    r->control = PicoControl{&state->controller, &state->control_signals, &control_ops, r};
    return true;
}

static void construct_zero_address(void *, void *storage)
{
    ::new (storage) IPAddress();
}
static void construct_http(void *, void *storage)
{
#if PICO_STANDALONE
    /* The initializer retains its recovered callback shape, but the local
     * profile has no outbound client. Its reserved storage is never accessed.
     * Avoid retaining WiFiClient's entire virtual method table in SRAM. */
    (void)storage;
#else
    ::new (storage) PicoHttpClient();
#endif
}
#if !PICO_STANDALONE
static void destroy_http(void *object)
{
    static_cast<PicoHttpClient *>(object)->~PicoHttpClient();
}
#endif
static void register_http_destructor(void *context, void *client)
{
    PicoEnergiaInitialization *binding = static_cast<PicoEnergiaInitialization *>(context);
#if PICO_STANDALONE
    (void)client;
    binding->destructor_registration_result = 0;
#else
    binding->destructor_registration_result = __aeabi_atexit(client, destroy_http, &__dso_handle);
#endif
}
static void construct_rfid(void *context, void *reader, uint8_t select_pin,
                           uint8_t shared_select_pin, uint8_t reset_pin)
{
    PicoEnergiaInitialization *binding = static_cast<PicoEnergiaInitialization *>(context);
    binding->construct_rfid(binding->rfid_context, reader, select_pin, shared_select_pin,
                            reset_pin);
}
static void construct_address(void *, void *storage, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    ::new (storage) IPAddress(a, b, c, d);
}
static const PicoInitializationOps initialization_ops = {construct_zero_address,   construct_http,
                                                         register_http_destructor, construct_rfid,
                                                         construct_address,        clock_ms};

PicoPlatformObjects pico_energia_platform_objects(PicoEnergiaInitialization *binding)
{
    return PicoPlatformObjects{binding->objects->still_home_address.storage(),
                               binding->objects->still_active_address.storage(),
                               binding->objects->http_client.storage(), binding->rfid_reader,
                               binding->objects->server_address.storage()};
}

const PicoInitializationOps *pico_energia_initialization_ops()
{
    return &initialization_ops;
}
