#include "ui_internal.h"
#include "picobrew/config.h"

void pico_copy_firmware_version(char *destination)
{
#if PICO_STANDALONE
    strcpy(destination, "v" PICO_STANDALONE_VERSION);
#else
    strcpy(destination, "v" PICO_LEGACY_VERSION);
#endif
    return;
}

void pico_append_program_state_label(char *destination)
{
    strcat(destination, pico_ui_active->app->recipe.program->state_label);
    return;
}

void pico_append_program_name(char *destination)
{
    strcat(destination, pico_ui_active->app->recipe.program->name);
    return;
}
