/* Pico Pro 0.1.34 — programs. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Prints program/session fields and localized allowed action list.
 * Firmware address: 0x20004900. */
void pico_program_log_configuration(PicoProgramContext *context, uint32_t language)
{
    char *action_label;
    int action_index;
    char log_buffer[128];

    Print_println_string(&global_serial0, "Session: ");
    Print_print_string(&global_serial0, "  id = ");
    Print_println_string(&global_serial0, context->identifier);
    Print_print_string(&global_serial0, "  programId = ");
    Print_print_string(&global_serial0,
                       *(char **)(&g_programs_unknown_02 + context->program_id * 4));
    Print_print_string(&global_serial0, " (");
    Print_print_signed_alias(&global_serial0, context->program_id, 10);
    Print_println_string(&global_serial0, ")");
    Print_print_string(&global_serial0, "  temperature = ");
    Print_println_signed(&global_serial0, context->temperature, 10);
    Print_print_string(&global_serial0, "  duration    = ");
    Print_println_signed(&global_serial0, context->duration, 10);
    Print_print_string(&global_serial0, "  location    = ");
    Print_println_signed(&global_serial0, context->location, 10);
    Print_print_string(&global_serial0, "  type = ");
    Print_print_string(&global_serial0,
                       *(char **)(&g_programs_unknown_10 + context->session_type * 4));
    Print_print_string(&global_serial0, " (");
    Print_print_signed_alias(&global_serial0, context->session_type, 10);
    Print_println_string(&global_serial0, ")");
    Print_print_string(&global_serial0, "  title = ");
    Print_println_string(&global_serial0, context->title);
    Print_print_string(&global_serial0, "  name = ");
    Print_println_string(&global_serial0, context->name);
    Print_print_string(&global_serial0, "  state = ");
    Print_println_string(&global_serial0, context->state_label);
    Print_print_string(&global_serial0, "  logoText = ");
    Print_println_string(&global_serial0, context->logo_text);
    libc_snprintf(log_buffer, 0x80, "\"%s\" has %d actions",
                  *(char **)(&g_programs_unknown_10 + context->session_type * 4),
                  context->action_count);
    Print_println_string(&global_serial0, log_buffer);
    for (action_index = 0; action_index < (int)context->action_count;
         action_index = action_index + 1) {
        action_label = pico_action_label(context->actions[action_index], language, 0);
        libc_snprintf(log_buffer, 0x80, "  - %d \"%s\"", context->actions[action_index],
                      action_label);
        Print_println_string(&global_serial0, log_buffer);
    }
    return;
}

/* Validates program/language, chooses localized labels, session type, default values and action
 * table. Firmware address: 0x20004bc0. */
void pico_program_initialize_context(PicoProgramContext *context, uint32_t program_id,
                                     uint32_t language)
{
    int value_1;
    pico_u32 value_2;
    pico_u32 *pointer_3;

    pico_assert_nonfatal((uint32_t)(language < 3), "language >= 0 && language <= 2");
    pico_assert_nonfatal((uint32_t)(program_id < 7),
                         "_programId >= 0 && _programId < ProgramId::_EnumCount");
    if (2 < language) {
        language = 0;
    }
    if (6 < program_id) {
        program_id = 1;
    }
    context->identifier[0] = '\0';
    context->temperature = 0;
    context->duration = 0;
    context->location = 0;
    context->session_type = 0;
    context->actions = (pico_u32 *)0x0;
    context->action_count = 0;
    value_1 = program_id * 3 + language;
    context->program_id = program_id;
    context->title = (&g_programs_unknown_08)[value_1];
    context->name = (&g_programs_unknown_12)[value_1];
    context->state_label = (&g_programs_unknown_01)[value_1];
    context->logo_text = (&g_programs_unknown_13)[value_1];
    context->interrupted_text = (&g_programs_unknown_16)[value_1];
    context->bitmap_primary = &g_programs_unknown_06;
    context->bitmap_secondary = &g_programs_unknown_07;
    if (program_id != 1) {
        if (program_id == 2) {
            value_2 = 1;
        } else {
            if (program_id != 5) {
                if (program_id == 3) {
                    context->session_type = 2;
                    context->temperature = 0x87;
                    context->duration = 0x69;
                    context->bitmap_secondary = &g_programs_unknown_14;
                } else if (program_id == 4) {
                    context->temperature = 0x78;
                    context->session_type = 4;
                    context->duration = 0x5a;
                } else if (program_id == 6) {
                    context->session_type = 5;
                    context->temperature = 0x3c;
                } else {
                    Print_println_string(&global_serial0, "ERROR unsuported programId");
                }
                goto branch_1;
            }
            value_2 = 3;
        }
        context->session_type = value_2;
    }
branch_1:
    value_2 = context->session_type;
    if (value_2 == 0) {
        context->actions = (pico_u32 *)&g_programs_unknown_05;
        value_2 = 6;
    } else {
        if (value_2 == 1) {
            pointer_3 = (pico_u32 *)&g_programs_unknown_11;
        } else if (value_2 == 2) {
            pointer_3 = (pico_u32 *)&g_programs_unknown_15;
        } else if (value_2 == 3) {
            pointer_3 = (pico_u32 *)&g_programs_unknown_04;
        } else {
            if (value_2 != 4) {
                if (value_2 != 5) {
                    Print_println_string(&global_serial0, "ERROR unsuported session types");
                    goto branch_2;
                }
                context->actions = (pico_u32 *)&g_programs_unknown_03;
                value_2 = 3;
                goto branch_3;
            }
            pointer_3 = (pico_u32 *)&g_programs_unknown_09;
        }
        context->actions = pointer_3;
        value_2 = 5;
    }
branch_3:
    context->action_count = value_2;
branch_2:
    pico_program_log_configuration(context, language);
    return;
}

/* Validates LocationId then reads four-byte lookup value.
 * Firmware address: 0x20005fdc. */
uint32_t pico_location_table_value(uint32_t location_id)
{
    bool valid;

    valid = pico_title_id_is_valid(location_id);
    pico_assert_nonfatal((uint32_t)valid, "LocationId::isValid(locationId)");
    return *(uint32_t *)(&g_programs_unknown_17 + location_id * 4);
}

/* Forwards instruction ID/option with active program and display context.
 * Firmware address: 0x20006128. */
uint32_t pico_execute_current_program_instruction(uint32_t instruction_id, int instruction_option)
{
    uint32_t ui_run_instruction_screen_result;

    ui_run_instruction_screen_result = pico_ui_run_instruction_screen(
        instruction_id, instruction_option, g_session_program_id, g_session_title, '\x01');
    return ui_run_instruction_screen_result;
}

/* Invokes temperature and time controls storing targetF/minutes in session.
 * Firmware address: 0x20006144. */
void pico_prompt_manual_temperature_duration(void)
{
    g_session_target_fahrenheit =
        pico_get_temperature_input(0x12, g_session_title, g_session_target_fahrenheit);
    g_session_hold_minutes = pico_get_time_input(0x14, g_session_title, g_session_hold_minutes);
    return;
}

/* Explicit executePauseCustomInstructions log and SousVide/Coffee session assertions.
 * Firmware address: 0x20006168. */
bool pico_execute_pause_custom_instructions(int program_id)
{
    uint32_t execute_current_program_instruction_result;
    int sound_alarm;
    char log_buffer[132];

    if (g_session_type == 2) {
        execute_current_program_instruction_result = 1;
    } else {
        execute_current_program_instruction_result = (uint32_t)(g_session_type == 4);
    }
    pico_assert_nonfatal(
        execute_current_program_instruction_result,
        "session.type == SessionTypes::SousVide || session.type == SessionTypes::Coffee");
    pico_assert_nonfatal((uint32_t)(g_session_program_id == program_id),
                         "session.programId == programId");
    libc_snprintf(log_buffer, 0x80, "%s %s", "executePauseCustomInstructions",
                  *(char **)(&g_programs_unknown_02 + program_id * 4));
    pico_log_serial("I", "SVIDE", log_buffer);
    if (g_session_program_id == 4) {
        execute_current_program_instruction_result = 0xe;
        sound_alarm = 1;
    } else {
        if (g_session_program_id != 3) {
            execute_current_program_instruction_result = 0;
            goto branch_1;
        }
        execute_current_program_instruction_result = pico_execute_current_program_instruction(9, 1);
        pico_display_clear();
        if (execute_current_program_instruction_result != 0x17)
            goto branch_1;
        execute_current_program_instruction_result = 10;
        sound_alarm = 0;
    }
    execute_current_program_instruction_result = pico_execute_current_program_instruction(
        execute_current_program_instruction_result, sound_alarm);
    pico_display_clear();
branch_1:
    pico_encoder_clear_click((PicoEncoder *)&g_encoder);
    return execute_current_program_instruction_result != 0x22;
}

/* Initializes program2 DEEP CLEAN and six compartment steps with mode-dependent duration/drain
 * defaults. Firmware address: 0x20008244. */
void pico_recipe_initialize_cleaning_steps(int *recipe, int cleaning_mode)
{
    int value_1;
    int *pointer_2;
    uint32_t compartment;

    pico_program_initialize_context((PicoProgramContext *)*recipe, 2,
                                    (uint32_t)g_language_as_uint8_t);
    pico_screen_set_recipe_name((PicoScreenState *)&g_screen_recipe_name,
                                *(char **)(*recipe + 0x34));
    *(uint8_t *)(recipe + 0x84) = 6;
    pointer_2 = recipe + 0xc;
    for (compartment = 1; (int)(compartment - 1) < (int)(uint32_t) * (uint8_t *)(recipe + 0x84);
         compartment = compartment + 1) {
        *(char *)((int)pointer_2 + 0x17) = (char)cleaning_mode;
        pointer_2[6] = 0;
        if (cleaning_mode == 0) {
            value_1 = 1;
        } else {
            value_1 = 5;
        }
        pointer_2[7] = value_1;
        *(char *)((int)pointer_2 + 0x16) = (char)compartment;
        pico_label_compartment((char *)pointer_2, compartment & 0xff);
        pointer_2 = pointer_2 + 8;
    }
    if (cleaning_mode == 0) {
        value_1 = 2;
    } else {
        value_1 = 5;
    }
    recipe[(*(uint8_t *)(recipe + 0x84) - 1) * 8 + 0x12] = value_1;
    return;
}
