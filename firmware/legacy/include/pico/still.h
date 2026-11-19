#ifndef PICO_STILL_H
#define PICO_STILL_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_still_clear_telemetry(uint8_t *telemetry);
bool pico_still_check_pressure_limit(int telemetry);
void pico_still_log_error(char *message);
void pico_still_initialize_connection(int connection);
void pico_still_set_access_point_name(int connection, char *still_identifier);
bool pico_still_has_home_ip(int connection, uint32_t argument_2, uint32_t argument_3,
                            int argument_4);
void pico_still_select_access_point(int connection, int access_point, uint32_t argument_3);
void pico_still_parse_telemetry(uint8_t *telemetry, char *response);

#endif /* PICO_STILL_H */
