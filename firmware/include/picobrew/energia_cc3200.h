#ifndef PICOBREW_ENERGIA_CC3200_H
#define PICOBREW_ENERGIA_CC3200_H

/* Target integration uses actual pinned SDK declarations, not recovered ABI
 * signatures. This header is for C++ translation units in the Energia build.
 */
#include <Energia.h>
#include <IPAddress.h>
#include "picobrew/pico_http_client.h"
#include "picobrew/actuators.h"
#include "picobrew/control.h"

struct PicoEnergiaUi {
    void (*pause_until_resume)(void *context);
    void (*prompt_insert_filter)(void *context);
    void (*progress_dot)(void *context, uint16_t row, uint16_t column);
    void (*progress_clear)(void *context, uint16_t row, uint16_t start_column);
    void (*display_flush)(void *context);
    /* This must implement the original fatal heater indication and halt. */
    void (*halt_heater_malfunction)(void *context);
};

struct PicoEnergiaControl {
    PicoAppState *state;
    PicoControl control;
    PicoActuators actuators;
    const PicoEnergiaUi *ui;
    void *ui_context;
};

/* Bind only; no GPIO writes and no timer enabling. Every UI callback is
 * mandatory so an incomplete application cannot silently omit interlocks.
 */
bool pico_energia_bind_control(PicoEnergiaControl *runtime, PicoAppState *state,
                               const PicoEnergiaUi *ui, void *ui_context);

template <typename T> struct PicoSdkObjectStorage {
    alignas(T) unsigned char bytes[sizeof(T)];
    void *storage()
    {
        return static_cast<void *>(bytes);
    }
    T &object()
    {
        return *reinterpret_cast<T *>(bytes);
    }
};

/* Storage is constructed in original application order by the initializer.
 * Access object() only after pico_application_initialize has completed.
 * Construct these SDK objects exactly once. Their storage and the associated
 * initialization binding must remain alive for the entire application lifetime,
 * including any invocation of the registered WiFiClient destructor.
 */
struct PicoEnergiaObjects {
    PicoSdkObjectStorage<IPAddress> still_home_address;
    PicoSdkObjectStorage<IPAddress> still_active_address;
    PicoSdkObjectStorage<PicoHttpClient> http_client;
    PicoSdkObjectStorage<IPAddress> server_address;
};

struct PicoEnergiaInitialization {
    PicoEnergiaObjects *objects;
    void *rfid_reader;
    void *rfid_context;
    /* Pico's MFRC522 wiring/variant remains an explicit application binding. */
    void (*construct_rfid)(void *context, void *reader, uint8_t select_pin,
                           uint8_t shared_select_pin, uint8_t reset_pin);
    int destructor_registration_result;
};

PicoPlatformObjects pico_energia_platform_objects(PicoEnergiaInitialization *binding);
const PicoInitializationOps *pico_energia_initialization_ops();

#endif
