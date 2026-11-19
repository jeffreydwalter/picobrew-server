#ifndef PICO_TRANSLATIONS_H
#define PICO_TRANSLATIONS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_label_insert_step_filter(char *destination);
void pico_label_adjust_recipe(char *destination);
void pico_label_done(char *destination);
void pico_label_confirm(char *destination);
void pico_label_continue(char *destination);
void pico_label_exit(char *destination);
void pico_label_back(char *destination);
void pico_label_ok(char *destination);
void pico_label_yes(char *destination);
void pico_label_no(char *destination);
void pico_label_compartment(char *destination, uint32_t compartment);
void pico_label_service(char *destination);
void pico_label_heating(char *destination);
void pico_label_cooling(char *destination);
void pico_label_holding(char *destination);
void pico_label_draining(char *destination);
void pico_label_start_brewing(char *destination);
void pico_label_cancel(char *destination);
void pico_label_next_uppercase(char *destination);
void pico_label_exit_uppercase(char *destination);

#endif /* PICO_TRANSLATIONS_H */
