/* Native appliance entry: SDK ResetISR -> constructors -> main -> setup/loop.
 * Test fixtures are not linked into this image. */
#include "picobrew/application.h"
#include "picobrew/config.h"
#include "picobrew/energia_cc3200.h"
#include "picobrew/energia_network.h"
#if PICO_STANDALONE
#include "picobrew/energia_http_server.h"
#include "picobrew/energia_history.h"
#include "picobrew/energia_telemetry.h"
#include "picobrew/local_api.h"
#include "picobrew/local_control.h"
#include "picobrew/web_assets.h"
#endif
#include <SPI.h>
extern "C" {
#include <driverlib/interrupt.h>
}

static PicoApplication application;
static PicoEnergiaControl control_runtime;
static PicoEnergiaObjects native_objects;
static PicoEnergiaInitialization initialization;
static PicoEnergiaNetwork network_runtime;
#if PICO_STANDALONE
static PicoLocalApi local_api;
static PicoEnergiaHttpServer local_http;
static void service_local_http(void *context)
{
    pico_energia_http_poll(static_cast<PicoEnergiaHttpServer *>(context));
}
static bool telemetry_snapshot(void *context, char *output, size_t capacity, size_t *length)
{
    return pico_local_api_handle(static_cast<PicoLocalApi *>(context), "GET", "/api/v1/status",
                                 NULL, 0, output, capacity, length) == 200;
}
#endif

static uint32_t clock_ms(void *)
{
    return millis();
}
static void delay_ms(void *, uint32_t value)
{
    delay(value);
}
static void pin_mode(void *, uint8_t pin, uint8_t mode)
{
    pinMode(pin, mode);
}
static int read_pin(void *, uint8_t pin)
{
    return digitalRead(pin);
}
static void write_pin(void *, uint8_t pin, uint8_t value)
{
    digitalWrite(pin, value);
}
static void spi_begin(void *)
{
    SPI.begin();
}
static uint8_t spi_transfer(void *, uint8_t value)
{
    return SPI.transfer(value);
}
static void spi_order(void *, uint8_t value)
{
    SPI.setBitOrder(value);
}
static void spi_mode(void *, uint8_t value)
{
    SPI.setDataMode(value);
}
static void disable_interrupts(void *)
{
    IntMasterDisable();
}
static bool enable_interrupts(void *)
{
    return IntMasterEnable();
}
static void attach_interrupt(void *, unsigned pin, void (*handler)(void), unsigned mode)
{
    attachInterrupt(static_cast<uint8_t>(pin), handler, static_cast<int>(mode));
}
static void serial_begin(void *, uint32_t baud)
{
    Serial.begin(baud);
}
static const PicoApplicationHardware hardware = {
    clock_ms,          delay_ms,         pin_mode,    read_pin, write_pin,
    spi_begin,         spi_transfer,     spi_order,   spi_mode, disable_interrupts,
    enable_interrupts, attach_interrupt, serial_begin};
static void pause_brew(void *context)
{
    pico_phase_pause_until_resume(&static_cast<PicoApplication *>(context)->phase);
}
static void prompt_filter(void *)
{
    pico_prompt_insert_filter_with_alarm();
}
static void progress_dot(void *, uint16_t row, uint16_t column)
{
    pico_display_text_at_cell(static_cast<short>(row), static_cast<short>(column), ".");
}
static void progress_clear(void *, uint16_t row, uint16_t column)
{
    pico_display_clear_row_from_column(static_cast<short>(row), static_cast<short>(column));
}
static void display_flush(void *)
{
    pico_display_flush();
}
static void halt_heater(void *)
{
    pico_halt_heater_malfunction();
}
static const PicoEnergiaUi control_ui = {pause_brew,     prompt_filter, progress_dot,
                                         progress_clear, display_flush, halt_heater};
static void construct_reader(void *, void *reader, uint8_t select, uint8_t shared,
                             uint8_t auxiliary)
{
    pico_rfid_construct(static_cast<PicoRfid *>(reader), select, shared, auxiliary);
}

struct PicoFirmwareConstruction {
    PicoFirmwareConstruction()
    {
        /* Binding installs only real services; outputs and timers remain off
         * until the recovered setup sequence reaches their initializers. */
        if (!pico_energia_bind_control(&control_runtime, &application.state, &control_ui,
                                       &application))
            for (;;) {
            }
        initialization.objects = &native_objects;
        initialization.rfid_reader = &application.reader;
        initialization.construct_rfid = construct_reader;
        PicoPlatformObjects objects = pico_energia_platform_objects(&initialization);
        pico_application_initialize(&application.state, &objects, pico_energia_initialization_ops(),
                                    &initialization, pico_application_default_pages());
#if !PICO_STANDALONE
        network_runtime.client = &native_objects.http_client.object();
#endif
        pico_application_bind_services(&application, &control_runtime.control,
                                       &control_runtime.actuators, &hardware, NULL,
                                       pico_energia_network_platform(), &network_runtime);
#if PICO_STANDALONE
        application.network.server_host = NULL;
        application.network.server_port = 0;
#else
        application.network.server_host = PICO_SERVER_HOST;
        application.network.server_port = PICO_SERVER_PORT;
#endif
    }
};
static PicoFirmwareConstruction construction;

void setup()
{
#if PICO_STANDALONE
    pico_local_api_initialize(&local_api, &application);
    pico_local_store_bind(&application.network, pico_energia_local_file_stat, NULL);
    pico_local_store_set_reader(pico_energia_local_file_read);
    pico_energia_http_initialize(&local_http, pico_local_api_http_handler, &local_api,
                                 pico_web_assets, pico_web_assets_count, PICO_LOCAL_API_PORT);
    pico_local_control_bind(&application, service_local_http, &local_http);
    pico_history_bind(&application, pico_energia_history_storage(), NULL);
    pico_telemetry_bind(&application, pico_energia_telemetry_storage(),
                        pico_energia_telemetry_transport(), NULL);
    pico_telemetry_set_snapshot(telemetry_snapshot, &local_api);
#endif
    pico_application_setup(&application);
}
void loop()
{
    pico_application_loop(&application);
}
