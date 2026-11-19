#include "picobrew/local_history.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { FILES = 28, FILE_BYTES = 3584, TRACE = 30000 };
static struct { uint8_t data[FILE_BYTES]; size_t size; bool present; } files[FILES];
static PicoApplication app;
static uint32_t now;
static bool storage_ready, device_ready, busy, stopped, masked, filter_inserted;
static int foreground, writes, reads, fault_write, corrupt_write, bad_io, trace[TRACE];
static bool allocation_failure;
static int allocations, allocation_attempts, frees, heap_live, heap_errors;
static size_t heap_bytes;
static void *heap_pointer;

void *hf_test_malloc(size_t bytes)
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
void hf_test_free(void *memory)
{
    if (!memory) return;
    if (memory != heap_pointer) ++heap_errors;
    heap_pointer = NULL; heap_bytes = 0; ++frees; --heap_live; free(memory);
}

static uint32_t clock_ms(void *context) { (void)context; return now; }
static void irq_off(void *context) { (void)context; masked = true; }
static bool irq_on(void *context) { (void)context; masked = false; return true; }
static int pin_read(void *context, uint8_t pin) { (void)context; (void)pin; return !filter_inserted; }
bool pico_local_control_foreground(void) { return foreground && !masked; }
bool pico_local_control_is_ready(void) { return device_ready; }
bool pico_local_control_busy(void) { return busy; }
bool pico_local_control_stopped(void) { return stopped; }
static const PicoApplicationHardware hardware = {
    .millis = clock_ms, .digital_read = pin_read,
    .interrupts_disable = irq_off, .interrupts_enable = irq_on
};
static bool ready(void *context) { (void)context; return storage_ready; }
static int file_index(const char *path)
{
    if (!strcmp(path, "/local/history.meta.a")) return 24;
    if (!strcmp(path, "/local/history.meta.b")) return 25;
    if (!strcmp(path, "/local/history.tail.a")) return 26;
    if (!strcmp(path, "/local/history.tail.b")) return 27;
    unsigned index; char extra;
    if (sscanf(path, "/local/history%u%c", &index, &extra) == 1 && index < 24) return (int)index;
    return -1;
}
static int file_stat(void *context, const char *name, size_t *length)
{
    (void)context; int index = file_index(name);
    if (masked || !foreground || !storage_ready) ++bad_io;
    if (index < 0) return -1;
    if (!files[index].present) return 1;
    *length = files[index].size; return 0;
}
static int32_t file_read(void *context, const char *name, uint8_t *data, size_t length)
{
    (void)context; int index = file_index(name); ++reads;
    if (masked || !foreground || !storage_ready) ++bad_io;
    if (index < 0 || !files[index].present || length != files[index].size) return -1;
    memcpy(data, files[index].data, length); return (int32_t)length;
}
static int file_replace(void *context, const char *name, const uint8_t *data, size_t length)
{
    (void)context; int index = file_index(name);
    if (masked || !foreground || !storage_ready) ++bad_io;
    if (index < 0 || length > FILE_BYTES) return -1;
    if (writes < TRACE) trace[writes] = index;
    ++writes; files[index].present = true;
    /* Faithful destructive open: the old logical file is gone immediately. */
    files[index].size = 0; memset(files[index].data, 0xff, FILE_BYTES);
    if (writes == fault_write) return -1;
    files[index].size = length; memcpy(files[index].data, data, length);
    if (writes == corrupt_write) files[index].data[length - 1] ^= 0x80;
    return 0;
}
static const PicoHistoryStorage storage = {ready, file_stat, file_read, file_replace};
void hf_reset(void)
{
    pico_history_bind(NULL, NULL, NULL);
    allocation_failure = false; allocations = allocation_attempts = frees = heap_live = heap_errors = 0;
    memset(files, 0, sizeof files); memset(&app, 0, sizeof app); now = 1000;
    storage_ready = device_ready = busy = filter_inserted = true; stopped = masked = false; foreground = 1;
    writes = reads = fault_write = corrupt_write = bad_io = 0;
    app.hardware = &hardware; app.state.controller.phase = 4;
    app.state.controller.wort_fahrenheit = 71.25;
    app.state.controller.thermoblock_fahrenheit = 83.5;
    app.state.controller.wort_target_fahrenheit = 152;
    app.state.controller.step_seconds = 3600;
    strcpy(app.state.screen.recipe_name, "Test Pale Ale");
    pico_history_bind(&app, &storage, NULL);
}
void hf_reboot(void)
{
    now = 1000; stopped = masked = false; foreground = 1; storage_ready = device_ready = true;
    fault_write = corrupt_write = 0;
    pico_history_bind(&app, &storage, NULL); pico_history_poll();
}
int hf_start(const char *command) { return pico_history_start(command); }
void hf_tick(uint32_t delta) { now += delta; pico_history_poll(); }
void hf_finish(int interrupted) { busy = false; pico_history_finish(interrupted != 0); }
void hf_busy(int value) { busy = value != 0; }
void hf_ready(int value) { device_ready = value != 0; }
void hf_storage(int value) { storage_ready = value != 0; }
void hf_foreground(int value) { foreground = value; }
void hf_phase(int phase, int step) { app.state.controller.phase = (uint8_t)phase; app.state.recipe.current_step = step; }
void hf_controller(int status, int compartment, int filter)
{
    app.state.controller.status = (uint8_t)status; app.state.controller.compartment = (uint8_t)compartment;
    filter_inserted = filter != 0;
}
void hf_values(double wort, double thermoblock, double target)
{
    app.state.controller.wort_fahrenheit = wort;
    app.state.controller.thermoblock_fahrenheit = thermoblock;
    app.state.controller.wort_target_fahrenheit = target;
}
void hf_name(const char *name) { snprintf(app.state.screen.recipe_name, sizeof app.state.screen.recipe_name, "%s", name); }
void hf_clock(uint32_t value) { now = value; }
void hf_stop(void) { stopped = true; }
int hf_writes(void) { return writes; }
int hf_reads(void) { return reads; }
int hf_bad_io(void) { return bad_io; }
int hf_trace(int index) { return index >= 0 && index < writes && index < TRACE ? trace[index] : -1; }
void hf_fail_write(int next) { fault_write = writes + next; }
void hf_corrupt_write(int next) { corrupt_write = writes + next; }
void hf_corrupt_file(int file, int offset)
{
    if (file >= 0 && file < FILES && offset >= 0 && (size_t)offset < files[file].size)
        files[file].data[offset] ^= 0x80;
}
int hf_file_count(void)
{
    int count = 0; for (int i = 0; i < FILES; ++i) if (files[i].present) ++count;
    return count;
}
int hf_file_size(int index) { return index >= 0 && index < FILES ? (int)files[index].size : -1; }
void hf_allocation_failure(int value) { allocation_failure = value != 0; }
int hf_heap(int which)
{
    switch (which) {
    case 0: return allocations; case 1: return allocation_attempts; case 2: return frees;
    case 3: return heap_live; case 4: return (int)heap_bytes; case 5: return heap_errors;
    }
    return -1;
}
uint32_t hf_file_generation(int index)
{
    if (index < 0 || index >= FILES || files[index].size < 12) return 0;
    const uint8_t *p = files[index].data + 8;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
