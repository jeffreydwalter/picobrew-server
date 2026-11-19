#ifndef PICOBREW_LOCAL_HISTORY_H
#define PICOBREW_LOCAL_HISTORY_H

#include "picobrew/application.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PICO_HISTORY_SAMPLE_MS = 15000,
    PICO_HISTORY_CHECKPOINT_MS = 60000,
    PICO_HISTORY_CHUNK_RECORDS = 176,
    PICO_HISTORY_BUFFER_BYTES = 32 + PICO_HISTORY_CHUNK_RECORDS * 20,
    PICO_HISTORY_CHUNKS = 24,
    PICO_HISTORY_CAPACITY = 8,
    PICO_HISTORY_RECORD_CAPACITY = PICO_HISTORY_CHUNKS * PICO_HISTORY_CHUNK_RECORDS,
    PICO_HISTORY_PAGE_RECORDS = 32,
    PICO_HISTORY_READY = 1,
    PICO_HISTORY_BUSY = 2,
    PICO_HISTORY_STOPPED = 4,
    PICO_HISTORY_FILTER_INSERTED = 8,
    PICO_HISTORY_SAMPLING_GAP = 16
};

typedef enum PicoHistoryState {
    PICO_HISTORY_EMPTY = 0,
    PICO_HISTORY_RECORDING = 1,
    PICO_HISTORY_COMPLETED = 2,
    PICO_HISTORY_INTERRUPTED = 3
} PicoHistoryState;

/* Independent native file handles, never the recovered SerFlash stream.
 * stat: 0=present, 1=absent, negative=I/O error. read returns exact bytes read.
 * replace closes the file, returns 0 only after a complete write and close.
 * Existing files are rewritten without delete/recreate/format. No callback is
 * called in an ISR or while interrupts are masked. */
typedef struct PicoHistoryStorage {
    bool (*ready)(void *context);
    int (*stat)(void *context, const char *path, size_t *length);
    int32_t (*read)(void *context, const char *path, uint8_t *data, size_t length);
    int (*replace)(void *context, const char *path, const uint8_t *data, size_t length);
} PicoHistoryStorage;

/* Celsius conversion belongs to the browser. INT16_MIN means an unavailable
 * temperature; all other values are tenths of one degree Fahrenheit.
 * The serialized record is exactly20 bytes, independent of C struct padding. */
typedef struct PicoHistorySample {
    uint32_t elapsed_ms;
    int32_t step_seconds;
    int16_t wort_tenths_f;
    int16_t thermoblock_tenths_f;
    int16_t target_tenths_f;
    uint8_t phase;
    uint8_t controller_status;
    uint8_t compartment;
    uint8_t current_step;
    uint8_t flags;
    uint8_t device_state;
} PicoHistorySample;

typedef struct PicoHistoryMetadata {
    uint32_t id;
    uint32_t duration_ms;
    uint32_t record_count;
    uint32_t persisted_record_count;
    uint32_t gap_count;
    uint32_t first_elapsed_ms;
    PicoHistoryState state;
    bool available;
    bool truncated;
    bool storage_error;
    bool recovered_after_restart;
    char command[16];
    char recipe_name[32];
} PicoHistoryMetadata;

/* bind performs no I/O. start commits a new catalog entry, reclaiming only the
 * oldest completed/interrupted history if the fixed catalog/chunks are full.
 * An active history's beginning is never reclaimed to extend its own tail.
 * finish is idempotent. It records a final snapshot and commits the live tail.
 * poll must also run when Wi-Fi is disconnected: logging depends only on the
 * initialized network processor's filesystem, never on an HTTP connection. */
void pico_history_bind(PicoApplication *application, const PicoHistoryStorage *storage,
                       void *storage_context);
bool pico_history_start(const char *command);
void pico_history_finish(bool interrupted);
void pico_history_poll(void);
/* Latest/current metadata; inventory order is newest first. */
void pico_history_metadata(PicoHistoryMetadata *metadata);
size_t pico_history_count(void);
bool pico_history_at(size_t newest_ordinal, PicoHistoryMetadata *metadata);
bool pico_history_find(uint32_t id, PicoHistoryMetadata *metadata);
/* 0=success,1=history id changed/absent,2=bad offset,negative=storage failure.
 * The caller supplies a page buffer of at least capacity samples (max32).
 * count=0 at record_count is valid. No partial page is returned on error. */
int pico_history_read(uint32_t id, uint32_t offset, PicoHistorySample *samples,
                      size_t capacity, size_t *count);

#ifdef __cplusplus
}
#endif
#endif
