#ifndef PICOBREW_APPLICATION_H
#define PICOBREW_APPLICATION_H
#include "picobrew/device.h"
#include "picobrew/rfid.h"
#include "picobrew/recipe.h"
#include "picobrew/ui.h"
#ifdef __cplusplus
extern "C" {
#endif

/* The native SDK supplies electrical I/O and time. All appliance workflows
 * are bound to the recovered modules by application.c. */
typedef struct PicoApplicationHardware {
    uint32_t (*millis)(void *context);
    void (*delay_ms)(void *context, uint32_t milliseconds);
    void (*pin_mode)(void *context, uint8_t pin, uint8_t mode);
    int (*digital_read)(void *context, uint8_t pin);
    void (*digital_write)(void *context, uint8_t pin, uint8_t value);
    void (*spi_begin)(void *context);
    uint8_t (*spi_transfer)(void *context, uint8_t value);
    void (*spi_bit_order)(void *context, uint8_t order);
    void (*spi_mode)(void *context, uint8_t mode);
    void (*interrupts_disable)(void *context);
    bool (*interrupts_enable)(void *context);
    void (*attach_interrupt)(void *context, unsigned interrupt, void (*handler)(void),
                             unsigned mode);
    void (*serial_begin)(void *context, uint32_t baud);
} PicoApplicationHardware;

typedef struct PicoApplication {
    PicoAppState state;
    PicoSettings settings;
    PicoNetwork network;
    PicoUi ui;
    PicoDevice device;
    PicoRfid reader;
    PicoRfidApplication rfid;
    PicoShuttleCalibration calibration;
    PicoPhaseControl phase;
    PicoRecipeRuntime recipe;
    PicoControl *control;
    PicoActuators *actuators;
    const PicoApplicationHardware *hardware;
    void *hardware_context;
} PicoApplication;

/* Bind once to fresh static/BSS storage, after pico_application_initialize has
 * constructed SDK objects and initialized state. This does not enable outputs.
 */
void pico_application_bind_services(PicoApplication *application, PicoControl *control,
                                    PicoActuators *actuators,
                                    const PicoApplicationHardware *hardware, void *hardware_context,
                                    const PicoNetworkPlatform *network_platform,
                                    void *network_context);
const PicoPageCallback *pico_application_default_pages(void);
void pico_application_setup(PicoApplication *application);
void pico_application_loop(PicoApplication *application);
#ifdef __cplusplus
}
#endif
#endif
