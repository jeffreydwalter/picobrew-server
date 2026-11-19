#ifndef PICOBREW_LOCAL_TELEMETRY_H
#define PICOBREW_LOCAL_TELEMETRY_H
#include "picobrew/local_history.h"
#ifdef __cplusplus
extern "C" {
#endif

enum { PICO_TELEMETRY_HOST_CAPACITY = 128, PICO_TELEMETRY_INTERVAL_MS = 15000,
       PICO_TELEMETRY_BUFFER_BYTES = 1152 };
typedef struct PicoTelemetryConfig {
    char host[PICO_TELEMETRY_HOST_CAPACITY];
    uint16_t port;
    bool enabled;
} PicoTelemetryConfig;
typedef enum PicoTelemetryState {
    PICO_TELEMETRY_DISABLED, PICO_TELEMETRY_OFFLINE, PICO_TELEMETRY_WAITING_IDLE,
    PICO_TELEMETRY_READY, PICO_TELEMETRY_CONNECTING, PICO_TELEMETRY_SENDING,
    PICO_TELEMETRY_RECEIVING, PICO_TELEMETRY_BACKOFF
} PicoTelemetryState;
typedef enum PicoTelemetryError {
    PICO_TELEMETRY_ERROR_NONE, PICO_TELEMETRY_ERROR_DNS, PICO_TELEMETRY_ERROR_SOCKET,
    PICO_TELEMETRY_ERROR_CONNECT, PICO_TELEMETRY_ERROR_SEND, PICO_TELEMETRY_ERROR_RECEIVE,
    PICO_TELEMETRY_ERROR_HTTP, PICO_TELEMETRY_ERROR_TIMEOUT, PICO_TELEMETRY_ERROR_CONFIG,
    PICO_TELEMETRY_ERROR_CAPACITY
} PicoTelemetryError;
typedef struct PicoTelemetryStatus {
    uint32_t sent, failed, dropped, last_success_ms;
    uint16_t last_http_status;
    PicoTelemetryState state;
    PicoTelemetryError last_error;
    bool resolved, storage_error;
} PicoTelemetryStatus;
typedef struct PicoTelemetryTransport {
    bool (*ready)(void *context);
    /* resolve is the only potentially blocking callback and is called ONLY
     * when the appliance is idle. Address is host-order IPv4. */
    int (*resolve)(void *context, const char *host, uint32_t *address);
    int (*open)(void *context); /* descriptor >=0; nonblocking before return */
    int (*connect)(void *context, int socket, uint32_t address, uint16_t port); /*0=connected,1=pending,<0=error*/
    int (*send)(void *context, int socket, const uint8_t *data, size_t length); /*0=again,>0=bytes,<0=error*/
    int (*receive)(void *context, int socket, uint8_t *data, size_t capacity); /*0=again,>0=bytes,<0=closed/error*/
    void (*close)(void *context, int socket);
} PicoTelemetryTransport;
typedef enum PicoTelemetryConfigureResult {
    PICO_TELEMETRY_CONFIG_OK = 0, PICO_TELEMETRY_CONFIG_INVALID,
    PICO_TELEMETRY_CONFIG_BUSY, PICO_TELEMETRY_CONFIG_STORAGE
} PicoTelemetryConfigureResult;
/* Serialize the same bounded JSON object as GET /api/v1/status. A false
 * result drops this upload; the callback never executes a control command. */
typedef bool (*PicoTelemetrySnapshot)(void *context, char *output, size_t capacity, size_t *length);

void pico_telemetry_bind(PicoApplication *application, const PicoHistoryStorage *storage,
                         const PicoTelemetryTransport *transport, void *context);
void pico_telemetry_set_snapshot(PicoTelemetrySnapshot snapshot, void *context);
/* Foreground hook BEFORE an SSID/NWP change; avoids retaining a DNS result or
 * descriptor even when a very short offline transition bypasses polling. */
void pico_telemetry_network_changed(void);
void pico_telemetry_poll(void);
void pico_telemetry_get_config(PicoTelemetryConfig *config);
void pico_telemetry_get_status(PicoTelemetryStatus *status);
PicoTelemetryConfigureResult pico_telemetry_configure(const PicoTelemetryConfig *config);
bool pico_telemetry_config_valid(const PicoTelemetryConfig *config);
const char *pico_telemetry_state_name(PicoTelemetryState state);
const char *pico_telemetry_error_name(PicoTelemetryError error);

#ifdef __cplusplus
}
#endif
#endif
