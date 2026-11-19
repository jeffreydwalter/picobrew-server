#ifndef PICOBREW_LOCAL_CONTROL_H
#define PICOBREW_LOCAL_CONTROL_H

#include "picobrew/application.h"
#include "picobrew/config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PicoLocalControlResult {
    PICO_LOCAL_CONTROL_OK = 0,
    PICO_LOCAL_CONTROL_INVALID,
    PICO_LOCAL_CONTROL_BUSY,
    PICO_LOCAL_CONTROL_STOPPED,
    PICO_LOCAL_CONTROL_NOT_READY
} PicoLocalControlResult;

/* One appliance and one foreground network service. Binding performs no I/O.
 * The service is disabled until setup has initialized the device peripherals.
 */
void pico_local_control_bind(PicoApplication *application,
                             void (*poll)(void *context), void *context);
void pico_local_control_ready(bool ready);
void pico_local_service(void);
/* Shared display/RFID/auxiliary chip-select bookkeeping. */
void pico_local_control_bus_select(unsigned pin, bool active);

/* Commands are queued; blocking appliance workflows never run inside HTTP
 * request handling. Stop immediately inhibits heater/pump output and remains
 * latched until a separate clear-stop at the main-menu boundary.
 */
PicoLocalControlResult pico_local_control_command(const char *command,
                                                 const char *recipe_id);
PicoLocalControlResult pico_local_control_encoder(int delta, bool click);
bool pico_local_control_busy(void);
bool pico_local_control_stopped(void);
bool pico_local_control_is_ready(void);
const char *pico_local_control_command_name(void);
bool pico_local_outputs_allowed(void);
bool pico_local_control_foreground(void);
/* ISR-safe fault latch: no UI, networking, allocation or IRQ-enable operation. */
void pico_local_control_trip_filter(void);
const char *pico_local_control_stop_reason(void);

/* Compile-time no-ops keep the recovered production profile unchanged. */
static inline bool pico_local_cancelled(void)
{
#if PICO_STANDALONE
    return pico_local_control_stopped();
#else
    return false;
#endif
}
static inline void pico_local_poll(void)
{
#if PICO_STANDALONE
    pico_local_service();
#endif
}

/* Application integration; not REST endpoints. */
void pico_local_control_enter_menu(void);
void pico_local_control_leave_menu(void);
bool pico_local_control_run_pending(void);
bool pico_local_control_has_pending(void);
void pico_local_control_delay(uint32_t milliseconds);
void pico_local_control_brew(PicoApplication *application, const char *recipe_id);

#ifdef __cplusplus
}
#endif
#endif
