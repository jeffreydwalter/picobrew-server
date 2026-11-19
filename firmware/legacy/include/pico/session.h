#ifndef PICO_SESSION_H
#define PICO_SESSION_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_end_brew_session(int logging_enabled);
int pico_start_session(int request_server_session);
int pico_parse_restart_session(uint32_t *recipe);
int pico_setup_instructions(int logging_enabled, char *program_title);
void pico_run_post_session_instructions(void);

#endif /* PICO_SESSION_H */
