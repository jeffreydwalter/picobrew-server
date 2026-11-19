#ifndef PICO_LOGGING_H
#define PICO_LOGGING_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_log_serial(char *level, char *component, char *message);
void pico_assert_nonfatal(int condition, char *expression);
void pico_log_ipv4_address(void *ip_address);

#endif /* PICO_LOGGING_H */
