#include "picobrew/local_telemetry.h"
#include "picobrew/local_control.h"
#include "picobrew/version.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { CONFIG_BYTES = 160, TX_BYTES = PICO_TELEMETRY_BUFFER_BYTES, HEADER_RESERVE = 256,
       REQUEST_TIMEOUT_MS = 5000, BACKOFF_MS = 30000, DNS_CACHE_MS = 300000 };
static struct {
    PicoApplication *application;
    const PicoHistoryStorage *storage;
    const PicoTelemetryTransport *transport;
    void *context, *snapshot_context;
    PicoTelemetrySnapshot snapshot;
    PicoTelemetryConfig config;
    PicoTelemetryStatus status;
    uint8_t *tx;
    char response[13];
    uint32_t generation, address, resolved_at, scheduled_at, started_at, backoff_at;
    size_t length, offset;
    unsigned bank, received;
    int socket;
    bool loaded, backing_off;
} telemetry;
static void add_count(uint32_t *counter, uint32_t amount);

const char *pico_telemetry_state_name(PicoTelemetryState state)
{
    static const char *const names[] = {"disabled", "offline", "waiting-idle", "ready",
        "connecting", "sending", "receiving", "backoff"};
    return (unsigned)state < sizeof names / sizeof names[0] ? names[state] : "disabled";
}
const char *pico_telemetry_error_name(PicoTelemetryError error)
{
    static const char *const names[] = {"none", "dns", "socket", "connect", "send", "receive",
        "http", "timeout", "config", "capacity"};
    return (unsigned)error < sizeof names / sizeof names[0] ? names[error] : "config";
}
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}
static uint32_t crc(const uint8_t *data, size_t length)
{
    uint32_t result = UINT32_MAX;
    for (size_t i = 0; i < length; ++i) {
        result ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            result = (result >> 1) ^ (0xedb88320u & (0u - (result & 1u)));
    }
    return ~result;
}
static bool ipv4(const char *host, uint32_t *address)
{
    uint32_t value = 0;
    for (unsigned part = 0; part < 4; ++part) {
        unsigned number = 0, digits = 0;
        while (*host >= '0' && *host <= '9') {
            number = number * 10 + (unsigned)(*host++ - '0');
            if (++digits > 3 || number > 255) return false;
        }
        if (!digits || (part < 3 ? *host++ != '.' : *host != 0)) return false;
        value = (value << 8) | number;
    }
    if (!value || value == UINT32_MAX) return false;
    if (address) *address = value;
    return true;
}
bool pico_telemetry_config_valid(const PicoTelemetryConfig *config)
{
    if (!config || !config->port || !memchr(config->host, 0, sizeof config->host)) return false;
    if (!config->host[0]) return !config->enabled;
    if (ipv4(config->host, NULL)) return true;
    unsigned label = 0; bool numeric = true; unsigned char previous = 0;
    for (const unsigned char *p = (const unsigned char *)config->host; *p; ++p) {
        unsigned ch = *p;
        if (ch == '.') {
            if (!label || previous == '-') return false;
            label = 0;
        } else {
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                  (ch >= '0' && ch <= '9') || ch == '-') || (!label && ch == '-') || ++label > 63) return false;
            if (!(ch >= '0' && ch <= '9')) numeric = false;
        }
        previous = (unsigned char)ch;
    }
    return label && previous != '-' && !numeric;
}
static bool storage_ready(void)
{
    return telemetry.application && telemetry.application->hardware && telemetry.application->hardware->millis &&
        telemetry.storage && telemetry.storage->ready && telemetry.storage->stat && telemetry.storage->read &&
        telemetry.storage->replace && pico_local_control_foreground() && telemetry.storage->ready(telemetry.context);
}
static uint32_t now_ms(void)
{
    return telemetry.application->hardware->millis(telemetry.application->hardware_context);
}
static void file_name(char *name, unsigned bank)
{
    strcpy(name, bank ? "/local/telemetry.b" : "/local/telemetry.a");
}
static bool read_config(unsigned bank, uint8_t *data, bool *absent)
{
    char name[24]; size_t length = 0; file_name(name, bank);
    int result = telemetry.storage->stat(telemetry.context, name, &length);
    if (absent) *absent = result == 1;
    return result == 0 && length == CONFIG_BYTES &&
        telemetry.storage->read(telemetry.context, name, data, CONFIG_BYTES) == CONFIG_BYTES;
}
static bool decode_config(const uint8_t *data, PicoTelemetryConfig *config, uint32_t *generation)
{
    if (memcmp(data, "PTM1", 4) || data[4] != 1 || data[5] > 1 || !get32(data + 8) ||
        get32(data + CONFIG_BYTES - 4) != crc(data, CONFIG_BYTES - 4)) return false;
    memset(config, 0, sizeof *config); config->enabled = data[5] != 0;
    config->port = (uint16_t)(data[6] | (unsigned)data[7] << 8);
    memcpy(config->host, data + 12, sizeof config->host);
    if (!pico_telemetry_config_valid(config)) return false;
    *generation = get32(data + 8); return true;
}
static bool load(void)
{
    if (telemetry.loaded) return true;
    if (!storage_ready()) return false;
    bool all_absent = true; uint8_t raw[CONFIG_BYTES]; PicoTelemetryConfig config;
    for (unsigned bank = 0; bank < 2; ++bank) {
        bool absent = false; uint32_t generation;
        bool valid = read_config(bank, raw, &absent) && decode_config(raw, &config, &generation);
        if (!absent) all_absent = false;
        if (valid && generation > telemetry.generation) {
            telemetry.config = config; telemetry.generation = generation; telemetry.bank = bank;
        }
    }
    telemetry.loaded = true; telemetry.scheduled_at = now_ms() - PICO_TELEMETRY_INTERVAL_MS;
    if (!all_absent && !telemetry.generation) {
        telemetry.status.storage_error = true; telemetry.status.last_error = PICO_TELEMETRY_ERROR_CONFIG;
    }
    telemetry.status.state = telemetry.config.enabled ? PICO_TELEMETRY_READY : PICO_TELEMETRY_DISABLED;
    return true;
}
static void close_socket(void)
{
    if (telemetry.socket >= 0 && telemetry.transport && telemetry.transport->close)
        telemetry.transport->close(telemetry.context, telemetry.socket);
    telemetry.socket = -1;
}
void pico_telemetry_bind(PicoApplication *application, const PicoHistoryStorage *storage,
                         const PicoTelemetryTransport *transport, void *context)
{
    close_socket(); free(telemetry.tx);
    memset(&telemetry, 0, sizeof telemetry); telemetry.application = application;
    telemetry.storage = storage; telemetry.transport = transport; telemetry.context = context;
    telemetry.socket = -1; telemetry.config.port = 8080;
}
void pico_telemetry_set_snapshot(PicoTelemetrySnapshot snapshot, void *context)
{
    telemetry.snapshot = snapshot; telemetry.snapshot_context = context;
}
void pico_telemetry_network_changed(void)
{
    if (!pico_local_control_foreground()) return;
    if (telemetry.socket >= 0) add_count(&telemetry.status.dropped, 1);
    close_socket(); telemetry.address = 0; telemetry.status.resolved = false;
    telemetry.backing_off = false;
    telemetry.status.state = telemetry.config.enabled ? PICO_TELEMETRY_OFFLINE : PICO_TELEMETRY_DISABLED;
}
void pico_telemetry_get_config(PicoTelemetryConfig *config) { load(); if (config) *config = telemetry.config; }
void pico_telemetry_get_status(PicoTelemetryStatus *status) { load(); if (status) *status = telemetry.status; }
PicoTelemetryConfigureResult pico_telemetry_configure(const PicoTelemetryConfig *config)
{
    if (!pico_telemetry_config_valid(config)) return PICO_TELEMETRY_CONFIG_INVALID;
    if (!pico_local_control_foreground() || pico_local_control_busy()) return PICO_TELEMETRY_CONFIG_BUSY;
    if (!load() || !storage_ready() || telemetry.generation == UINT32_MAX) return PICO_TELEMETRY_CONFIG_STORAGE;
    if (config->enabled == telemetry.config.enabled && config->port == telemetry.config.port &&
        !strcmp(config->host, telemetry.config.host) && !telemetry.status.storage_error) return PICO_TELEMETRY_CONFIG_OK;
    uint8_t raw[CONFIG_BYTES] = {0}, verification[CONFIG_BYTES]; char name[24];
    memcpy(raw, "PTM1", 4); raw[4] = 1; raw[5] = config->enabled;
    raw[6] = (uint8_t)config->port; raw[7] = (uint8_t)(config->port >> 8);
    put32(raw + 8, telemetry.generation + 1); memcpy(raw + 12, config->host, strlen(config->host) + 1);
    put32(raw + CONFIG_BYTES - 4, crc(raw, CONFIG_BYTES - 4)); file_name(name, !telemetry.bank);
    if (telemetry.storage->replace(telemetry.context, name, raw, sizeof raw) ||
        !read_config(!telemetry.bank, verification, NULL) || memcmp(raw, verification, sizeof raw)) {
        telemetry.status.storage_error = true; telemetry.status.last_error = PICO_TELEMETRY_ERROR_CONFIG;
        return PICO_TELEMETRY_CONFIG_STORAGE;
    }
    close_socket(); telemetry.config = *config; ++telemetry.generation; telemetry.bank = !telemetry.bank;
    if (!config->enabled) { free(telemetry.tx); telemetry.tx = NULL; }
    telemetry.address = 0; telemetry.status.resolved = false; telemetry.backing_off = false;
    telemetry.status.storage_error = false; telemetry.status.last_error = PICO_TELEMETRY_ERROR_NONE;
    telemetry.status.last_http_status = 0;
    telemetry.status.state = config->enabled ? PICO_TELEMETRY_READY : PICO_TELEMETRY_DISABLED;
    telemetry.scheduled_at = now_ms() - PICO_TELEMETRY_INTERVAL_MS;
    return PICO_TELEMETRY_CONFIG_OK;
}
static void add_count(uint32_t *counter, uint32_t amount)
{
    *counter = UINT32_MAX - *counter < amount ? UINT32_MAX : *counter + amount;
}
static void fail(PicoTelemetryError error, uint32_t now)
{
    close_socket(); add_count(&telemetry.status.failed, 1); add_count(&telemetry.status.dropped, 1);
    telemetry.status.last_error = error; telemetry.status.state = PICO_TELEMETRY_BACKOFF;
    telemetry.backing_off = true; telemetry.backoff_at = now;
}
static bool frame(void)
{
    char *body = (char *)telemetry.tx + HEADER_RESERVE;
    size_t capacity = TX_BYTES - HEADER_RESERVE;
    const char prefix[] = "{\"schema_version\":1,\"firmware_version\":\"" PICO_STANDALONE_VERSION "\",\"status\":";
    size_t prefix_length = sizeof prefix - 1, length = 0;
    memcpy(body, prefix, prefix_length);
    if (!telemetry.snapshot || !telemetry.snapshot(telemetry.snapshot_context, body + prefix_length,
            capacity - prefix_length - 1, &length) || !length || length >= capacity - prefix_length - 1) return false;
    length += prefix_length; body[length++] = '}';
    int header = snprintf((char *)telemetry.tx, HEADER_RESERVE,
        "POST /api/v1/telemetry HTTP/1.0\r\nHost: %s:%u\r\nContent-Type: application/json\r\n"
        "Content-Length: %u\r\nConnection: close\r\n\r\n", telemetry.config.host, telemetry.config.port, (unsigned)length);
    if (header < 0 || header >= HEADER_RESERVE || (size_t)header + length > TX_BYTES) return false;
    memmove(telemetry.tx + header, body, length);
    telemetry.length = (size_t)header + length; telemetry.offset = 0; telemetry.received = 0; return true;
}
static bool due(uint32_t now)
{
    uint32_t count = (now - telemetry.scheduled_at) / PICO_TELEMETRY_INTERVAL_MS;
    if (!count) return false;
    telemetry.scheduled_at += count * PICO_TELEMETRY_INTERVAL_MS;
    add_count(&telemetry.status.dropped, count - 1); return true;
}
void pico_telemetry_poll(void)
{
    if (!pico_local_control_foreground() || !load() || !telemetry.config.enabled) return;
    const PicoTelemetryTransport *t = telemetry.transport;
    if (!t || !t->ready || !t->resolve || !t->open || !t->connect || !t->send || !t->receive || !t->close) return;
    uint32_t now = now_ms(); bool scheduled = due(now);
    if (!t->ready(telemetry.context)) {
        if (telemetry.socket >= 0) fail(PICO_TELEMETRY_ERROR_CONNECT, now);
        /* A different SSID/subnet can map the same hostname to another host.
         * Never carry a DNS cache across an observed network disconnection. */
        telemetry.address = 0; telemetry.status.resolved = false;
        if (scheduled) add_count(&telemetry.status.dropped, 1);
        telemetry.status.state = PICO_TELEMETRY_OFFLINE; return;
    }
    if (telemetry.socket >= 0) {
        if (scheduled) add_count(&telemetry.status.dropped, 1);
        if (now - telemetry.started_at >= REQUEST_TIMEOUT_MS) { fail(PICO_TELEMETRY_ERROR_TIMEOUT, now); return; }
        if (telemetry.status.state == PICO_TELEMETRY_CONNECTING) {
            int result = t->connect(telemetry.context, telemetry.socket, telemetry.address, telemetry.config.port);
            if (result < 0) fail(PICO_TELEMETRY_ERROR_CONNECT, now);
            else if (!result) telemetry.status.state = PICO_TELEMETRY_SENDING;
        } else if (telemetry.status.state == PICO_TELEMETRY_SENDING) {
            size_t length = telemetry.length - telemetry.offset; if (length > 256) length = 256;
            int count = t->send(telemetry.context, telemetry.socket, telemetry.tx + telemetry.offset, length);
            if (count < 0 || (size_t)count > length) { fail(PICO_TELEMETRY_ERROR_SEND, now); return; }
            telemetry.offset += (unsigned)count;
            if (telemetry.offset == telemetry.length) telemetry.status.state = PICO_TELEMETRY_RECEIVING;
        } else {
            uint8_t data[32]; int count = t->receive(telemetry.context, telemetry.socket, data, sizeof data);
            if (count < 0 || count > (int)sizeof data) { fail(PICO_TELEMETRY_ERROR_RECEIVE, now); return; }
            for (int i = 0; i < count && telemetry.received < sizeof telemetry.response; ++i)
                telemetry.response[telemetry.received++] = (char)data[i];
            if (telemetry.received == sizeof telemetry.response) {
                const char *p = telemetry.response;
                if (memcmp(p, "HTTP/1.", 7) || (p[7] != '0' && p[7] != '1') || p[8] != ' ' ||
                    p[9] < '1' || p[9] > '5' || p[10] < '0' || p[10] > '9' || p[11] < '0' || p[11] > '9' ||
                    (p[12] != ' ' && p[12] != '\r')) { fail(PICO_TELEMETRY_ERROR_HTTP, now); return; }
                telemetry.status.last_http_status = (uint16_t)((p[9] - '0') * 100 + (p[10] - '0') * 10 + p[11] - '0');
                if (p[9] != '2') { fail(PICO_TELEMETRY_ERROR_HTTP, now); return; }
                close_socket(); add_count(&telemetry.status.sent, 1); telemetry.status.last_success_ms = now;
                telemetry.status.last_error = PICO_TELEMETRY_ERROR_NONE; telemetry.status.state = PICO_TELEMETRY_READY;
            }
        }
        return;
    }
    if (telemetry.backing_off && now - telemetry.backoff_at < BACKOFF_MS) {
        telemetry.status.state = PICO_TELEMETRY_BACKOFF;
        if (scheduled) add_count(&telemetry.status.dropped, 1);
        return;
    }
    telemetry.backing_off = false;
    if (!scheduled) return;
    uint32_t address;
    bool numeric = ipv4(telemetry.config.host, &address);
    if (!numeric && !telemetry.address && pico_local_control_busy()) {
        telemetry.status.state = PICO_TELEMETRY_WAITING_IDLE; add_count(&telemetry.status.dropped, 1); return;
    }
    /* One lazily allocated frame is reused for every upload. Allocation must
     * succeed before DNS or socket I/O; it never borrows the HTTP arena. */
    if (!telemetry.tx) telemetry.tx = malloc(TX_BYTES);
    if (!telemetry.tx) { fail(PICO_TELEMETRY_ERROR_CAPACITY, now); return; }
    if (numeric) {
        telemetry.address = address; telemetry.status.resolved = true;
    } else if (!telemetry.address || (now - telemetry.resolved_at >= DNS_CACHE_MS && !pico_local_control_busy())) {
        if (pico_local_control_busy()) {
            telemetry.status.state = PICO_TELEMETRY_WAITING_IDLE; add_count(&telemetry.status.dropped, 1); return;
        }
        if (t->resolve(telemetry.context, telemetry.config.host, &address) != 0 || !address) {
            telemetry.address = 0; telemetry.status.resolved = false; fail(PICO_TELEMETRY_ERROR_DNS, now_ms()); return;
        }
        telemetry.address = address; telemetry.resolved_at = now_ms(); telemetry.status.resolved = true;
    }
    if (!frame()) { fail(PICO_TELEMETRY_ERROR_CAPACITY, now_ms()); return; }
    telemetry.socket = t->open(telemetry.context);
    if (telemetry.socket < 0) { fail(PICO_TELEMETRY_ERROR_SOCKET, now_ms()); return; }
    telemetry.started_at = now_ms(); telemetry.status.state = PICO_TELEMETRY_CONNECTING;
}
