#ifndef PICO_PROGRAMS_H
#define PICO_PROGRAMS_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void pico_program_log_configuration(PicoProgramContext *context, uint32_t language);
void pico_program_initialize_context(PicoProgramContext *context, uint32_t program_id,
                                     uint32_t language);
uint32_t pico_location_table_value(uint32_t location_id);
uint32_t pico_execute_current_program_instruction(uint32_t instruction_id, int instruction_option);
void pico_prompt_manual_temperature_duration(void);
bool pico_execute_pause_custom_instructions(int program_id);
void pico_recipe_initialize_cleaning_steps(int *recipe, int cleaning_mode);

#endif /* PICO_PROGRAMS_H */
