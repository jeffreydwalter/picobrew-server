#include "picobrew/local_telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PicoApplication app;
static struct { uint8_t data[160]; size_t length; bool present; } files[2];
static uint32_t now;
static bool busy, foreground, fs_ready, connected;
static int writes, fail_write, corrupt_write, dns_calls, open_calls, connect_calls,
    send_calls, receive_calls, close_calls, bad_context, dns_result, open_result,
    connect_result, send_result, receive_result, receive_chunk, snapshot_calls;
static char transmitted[2048], response[256], snapshot_json[1024];
static size_t transmitted_length, response_offset;
static uint32_t last_address;
static bool allocation_failure;
static int allocations, allocation_attempts, frees, heap_live, heap_errors;
static size_t heap_bytes;
static void *heap_pointer;

void *tf_test_malloc(size_t bytes)
{
    ++allocation_attempts;
    if (allocation_failure) return NULL;
    void *memory = malloc(bytes);
    if (memory) {
        if (heap_pointer) ++heap_errors;
        heap_pointer = memory; heap_bytes = bytes; ++allocations; ++heap_live;
    }
    return memory;
}
void tf_test_free(void *memory)
{
    if (!memory) return;
    if (memory != heap_pointer) ++heap_errors;
    heap_pointer = NULL; heap_bytes = 0; ++frees; --heap_live; free(memory);
}

bool pico_local_control_foreground(void) { return foreground; }
bool pico_local_control_busy(void) { return busy; }
static uint32_t clock_ms(void *context) { (void)context; return now; }
static const PicoApplicationHardware hardware = {.millis = clock_ms};
static bool storage_ready(void *context) { (void)context; return fs_ready; }
static int index_of(const char *path)
{
    if (!strcmp(path, "/local/telemetry.a")) return 0;
    if (!strcmp(path, "/local/telemetry.b")) return 1;
    return -1;
}
static int file_stat(void *context, const char *path, size_t *length)
{
    (void)context; int index = index_of(path);
    if (!foreground || !fs_ready) ++bad_context;
    if (index < 0) return -1;
    if (!files[index].present) return 1;
    *length = files[index].length; return 0;
}
static int32_t file_read(void *context, const char *path, uint8_t *data, size_t length)
{
    (void)context; int index = index_of(path);
    if (!foreground || !fs_ready) ++bad_context;
    if (index < 0 || !files[index].present || files[index].length != length) return -1;
    memcpy(data, files[index].data, length); return (int32_t)length;
}
static int file_replace(void *context, const char *path, const uint8_t *data, size_t length)
{
    (void)context; int index = index_of(path);
    if (!foreground || !fs_ready || busy) ++bad_context;
    if (index < 0 || length != 160) return -1;
    ++writes; files[index].present = true; files[index].length = 0;
    if (writes == fail_write) return -1;
    memcpy(files[index].data, data, length); files[index].length = length;
    if (writes == corrupt_write) files[index].data[159] ^= 0x80;
    return 0;
}
static bool network_ready(void *context) { (void)context; return connected; }
static int resolve(void *context, const char *host, uint32_t *address)
{
    (void)context; (void)host; ++dns_calls;
    if (busy || !foreground || !connected) ++bad_context;
    *address = 0x0a000101u + (uint32_t)dns_calls; return dns_result;
}
static int open_socket(void *context)
{
    (void)context; ++open_calls;
    if (!foreground || !connected) ++bad_context;
    transmitted_length = 0; response_offset = 0; return open_result;
}
static int connect_socket(void *context, int socket, uint32_t address, uint16_t port)
{
    (void)context; ++connect_calls; last_address = address;
    if (!foreground || socket != 7 || !port) ++bad_context;
    return connect_result;
}
static int send_socket(void *context, int socket, const uint8_t *data, size_t length)
{
    (void)context; ++send_calls;
    if (!foreground || socket != 7 || length > 256) ++bad_context;
    if (send_result <= 0) return send_result;
    size_t count = length < (size_t)send_result ? length : (size_t)send_result;
    if (transmitted_length + count >= sizeof transmitted) return -1;
    memcpy(transmitted + transmitted_length, data, count); transmitted_length += count;
    transmitted[transmitted_length] = 0; return (int)count;
}
static int receive_socket(void *context, int socket, uint8_t *data, size_t capacity)
{
    (void)context; ++receive_calls;
    if (!foreground || socket != 7 || capacity > 32) ++bad_context;
    if (receive_result <= 0) return receive_result;
    size_t left = strlen(response) - response_offset;
    if (!left) return -1;
    size_t count = left < capacity ? left : capacity;
    if (count > (size_t)receive_chunk) count = (size_t)receive_chunk;
    memcpy(data, response + response_offset, count); response_offset += count; return (int)count;
}
static void close_socket(void *context, int socket)
{
    (void)context; ++close_calls; if (!foreground || socket != 7) ++bad_context;
}
static bool snapshot(void *context, char *output, size_t capacity, size_t *length)
{
    (void)context; ++snapshot_calls;
    size_t size = strlen(snapshot_json);
    if (size >= capacity) return false;
    memcpy(output, snapshot_json, size + 1); *length = size; return true;
}
static const PicoHistoryStorage storage = {storage_ready, file_stat, file_read, file_replace};
static const PicoTelemetryTransport transport = {network_ready, resolve, open_socket,
    connect_socket, send_socket, receive_socket, close_socket};
static void bind(void)
{
    pico_telemetry_bind(&app, &storage, &transport, NULL);
    pico_telemetry_set_snapshot(snapshot, NULL);
}
void tf_reset(void)
{
    foreground = true; pico_telemetry_bind(NULL, NULL, NULL, NULL);
    allocation_failure = false; allocations = allocation_attempts = frees = heap_live = heap_errors = 0;
    memset(files, 0, sizeof files); memset(&app, 0, sizeof app); app.hardware = &hardware;
    now = 1000; busy = false; foreground = fs_ready = connected = true;
    writes = fail_write = corrupt_write = dns_calls = open_calls = connect_calls = send_calls = receive_calls = close_calls = bad_context = snapshot_calls = 0;
    dns_result = connect_result = 0; open_result = 7; send_result = 256; receive_result = 1; receive_chunk = 32;
    strcpy(response, "HTTP/1.1 204 No Content\r\n\r\n");
    strcpy(snapshot_json, "{\"device_uid\":\"pico-123\",\"session_id\":\"local-123-1\",\"uptime_ms\":42,\"phase\":1,\"wort_f\":153.2,\"target_f\":154.0}");
    transmitted_length = response_offset = 0; last_address = 0; bind();
}
void tf_reboot(void) { now = 1000; fail_write = corrupt_write = 0; bind(); }
void tf_tick(uint32_t delta) { now += delta; pico_telemetry_poll(); }
void tf_clock(uint32_t value) { now = value; }
void tf_busy(int value) { busy = value != 0; }
void tf_foreground(int value) { foreground = value != 0; }
void tf_network(int value) { connected = value != 0; }
void tf_storage(int value) { fs_ready = value != 0; }
int tf_config(const char *host, int port, int enabled)
{
    PicoTelemetryConfig config; memset(&config, 0, sizeof config);
    if (strlen(host) >= sizeof config.host) return PICO_TELEMETRY_CONFIG_INVALID;
    strcpy(config.host, host); config.port = (uint16_t)port; config.enabled = enabled != 0;
    return pico_telemetry_configure(&config);
}
void tf_behavior(int which, int value)
{
    switch (which) {
    case 0: dns_result = value; break; case 1: open_result = value; break;
    case 2: connect_result = value; break; case 3: send_result = value; break;
    case 4: receive_result = value; break; case 5: receive_chunk = value; break;
    }
}
int tf_counter(int which)
{
    switch (which) {
    case 0: return dns_calls; case 1: return open_calls; case 2: return connect_calls;
    case 3: return send_calls; case 4: return receive_calls; case 5: return close_calls;
    case 6: return writes; case 7: return bad_context; case 8: return snapshot_calls;
    }
    return -1;
}
void tf_response(const char *value) { snprintf(response, sizeof response, "%s", value); }
void tf_snapshot(const char *value) { snprintf(snapshot_json, sizeof snapshot_json, "%s", value); }
const char *tf_transmitted(void) { return transmitted; }
uint32_t tf_address(void) { return last_address; }
void tf_fail_write(int after) { fail_write = writes + after; }
void tf_corrupt_write(int after) { corrupt_write = writes + after; }
void tf_allocation_failure(int value) { allocation_failure = value != 0; }
int tf_heap(int which)
{
    switch (which) {
    case 0: return allocations; case 1: return allocation_attempts; case 2: return frees;
    case 3: return heap_live; case 4: return (int)heap_bytes; case 5: return heap_errors;
    }
    return -1;
}
