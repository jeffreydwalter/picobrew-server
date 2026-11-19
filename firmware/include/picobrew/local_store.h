#ifndef PICOBREW_LOCAL_STORE_H
#define PICOBREW_LOCAL_STORE_H

#include "picobrew/network.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { PICO_LOCAL_RECIPE_CAPACITY = 16, PICO_LOCAL_RECIPE_NAME_SIZE = 32 };

typedef enum PicoLocalStoreResult {
    PICO_LOCAL_STORE_OK = 0,
    PICO_LOCAL_STORE_NOT_FOUND,
    PICO_LOCAL_STORE_INVALID,
    PICO_LOCAL_STORE_FULL,
    PICO_LOCAL_STORE_IO,
    PICO_LOCAL_STORE_CORRUPT
} PicoLocalStoreResult;

/* 0: exists, 1: definitely absent, negative: I/O failure. Missing and failed
 * reads must remain distinct: a failed catalog read must never create an empty
 * replacement. The native adapter uses sl_FsGetInfo, without an SLFS handle. */
typedef int (*PicoLocalFileStat)(void *context, const char *name, size_t *size);
typedef int32_t (*PicoLocalReadFile)(void *context, const char *name, uint8_t *data, size_t length);
void pico_local_store_bind(PicoNetwork *network, PicoLocalFileStat stat, void *context);
/* Native builds install a bounded block reader to avoid one NWP transaction
 * per byte. It uses the same context as stat and returns bytes or an error. */
void pico_local_store_set_reader(PicoLocalReadFile reader);

typedef struct PicoLocalRecipe {
    char id[21];
    char name[PICO_LOCAL_RECIPE_NAME_SIZE];
    uint8_t step_count;
    PicoRecipeStep steps[PICO_RECIPE_STEP_CAPACITY];
} PicoLocalRecipe;

bool pico_local_recipe_id_valid(const char *id);
bool pico_local_recipe_valid(const PicoLocalRecipe *recipe);
PicoLocalStoreResult pico_local_recipe_get(PicoNetwork *network, const char *id,
                                          PicoLocalRecipe *recipe);
PicoLocalStoreResult pico_local_recipe_put(PicoNetwork *network,
                                          const PicoLocalRecipe *recipe, bool *created);
PicoLocalStoreResult pico_local_recipe_delete(PicoNetwork *network, const char *id);
/* Returns each occupied slot by ordinal, without an unbounded directory scan. */
PicoLocalStoreResult pico_local_recipe_at(PicoNetwork *network, unsigned ordinal,
                                         PicoLocalRecipe *recipe);
PicoLocalStoreResult pico_local_recipe_visit(PicoNetwork *network,
    bool (*visitor)(void *context, const PicoLocalRecipe *recipe), void *context);
bool pico_local_recipe_load(PicoNetwork *network, const char *id, PicoRecipe *recipe,
                            char name[PICO_LOCAL_RECIPE_NAME_SIZE]);
bool pico_local_recipe_list(PicoNetwork *network, PicoRecipeListing *listing);
/* Boot migration handles an existing catalogue exactly once. Empty catalogues
 * receive defaults; occupied catalogues retain every user slot. Explicit
 * restore adds missing defaults only, in one capacity-checked catalogue commit. */
unsigned pico_local_starter_recipe_count(void);
PicoLocalStoreResult pico_local_recipes_initialize(PicoNetwork *network);
PicoLocalStoreResult pico_local_recipes_restore_defaults(PicoNetwork *network,
                                                        unsigned *added, unsigned *preserved);

/* Versioned, checksummed two-bank progress. Durations are the remaining
 * controller minutes, including PicoPak pass-through drain compensation.
 * Recovery always requires a matching recipe and an explicit user decision. */
typedef struct PicoLocalRecovery {
    char recipe_id[21];
    uint32_t recipe_signature;
    uint16_t active_minutes, drain_minutes;
    uint8_t session_type, step, phase, calibrated;
} PicoLocalRecovery;
uint32_t pico_local_recovery_select(const PicoRecipe *recipe);
PicoLocalStoreResult pico_local_recovery_get(PicoNetwork *network, PicoLocalRecovery *record);
bool pico_local_recovery_save(PicoNetwork *network, const PicoLocalRecovery *record);
bool pico_local_recovery_clear(PicoNetwork *network);

bool pico_local_session_new(PicoNetwork *network, char *identifier, size_t capacity);
bool pico_local_settings_load(PicoNetwork *network);
bool pico_local_settings_save(PicoNetwork *network);

/* Reporting events are a bounded in-memory ring, supplied by local_api.c.
 * Only the session sequence and recipes/settings are persisted here. */
void pico_local_event(PicoNetwork *network, const char *kind, const char *detail);

#ifdef __cplusplus
}
#endif
#endif
