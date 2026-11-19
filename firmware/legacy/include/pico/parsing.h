#ifndef PICO_PARSING_H
#define PICO_PARSING_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

int pico_parse_delimited_field(char *input, char *output, int *cursor, char *delimiter,
                               int input_limit);
uint32_t pico_recipe_parse_server_response(char *response, long response_length);
void pico_url_write_escape(char *output, char *escape, int offset);
char *pico_url_encode(char *input, char *output);

#endif /* PICO_PARSING_H */
