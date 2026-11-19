#ifndef PICOBREW_DEVICE_H
#define PICOBREW_DEVICE_H

#include "picobrew/actuators.h"
#include "picobrew/control.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PicoDeviceOps {
    uint32_t (*millis)(void *context);
    void (*delay_ms)(void *context, uint32_t duration);
    void (*pin_mode)(void *context, uint8_t pin, uint8_t mode);
    int (*digital_read)(void *context, uint8_t pin);
    void (*digital_write)(void *context, uint8_t pin, uint8_t value);
    void (*spi_mode)(void *context, uint8_t mode);
    uint8_t (*spi_transfer)(void *context, uint8_t value);
    void (*interrupts_disable)(void *context);
    void (*interrupts_enable)(void *context);
    void (*log)(void *context, const char *level, const char *module, const char *message);
    void (*report_error)(void *context, unsigned error, bool acknowledge, bool report);
    void (*display_clear)(void *context);
    void (*display_flush)(void *context);
    void (*service_screen)(void *context);
    void (*error_heading)(void *context);
    void (*encoder_clear_click)(void *context);
    bool (*encoder_take_click)(void *context);
} PicoDeviceOps;

typedef struct PicoDevice {
    PicoAppState *state;
    PicoActuators *actuators;
    uint8_t *legacy_compartment_mapping;
    uint8_t *home_compartment;
    const PicoDeviceOps *ops;
    void *context;
} PicoDevice;

void pico_device_spi_initialize(PicoDevice *device);
uint8_t pico_device_spi_selected_byte(PicoDevice *device, uint8_t value);
uint8_t pico_device_spi_command(PicoDevice *device, uint8_t command);
bool pico_device_stepper_ready(PicoDevice *device);
bool pico_device_wait_stepper(PicoDevice *device);
bool pico_device_buzzer(PicoDevice *device, bool enabled);
uint32_t pico_device_stepper_capabilities(PicoDevice *device, uint32_t *detail);
void pico_device_detect_stepper(PicoDevice *device);
bool pico_device_set_torque(PicoDevice *device, uint32_t mode);
bool pico_device_move_compartment(PicoDevice *device, uint32_t compartment);
void pico_device_move_home(PicoDevice *device);
void pico_device_filter_input_initialize(PicoDevice *device);
bool pico_device_temperature_inputs_initialize(PicoDevice *device);
void pico_device_wait_for_filter(PicoDevice *device);
bool pico_device_service_button(PicoDevice *device);
void pico_device_alarm_while_filter_missing(PicoDevice *device);
void pico_device_alarm_until_encoder_press(PicoDevice *device);
bool pico_device_check_thermoblock_heating(PicoDevice *device, PicoControl *control);

#ifdef __cplusplus
}
#endif
#endif
