/* Pico Pro 0.1.34 — ui screens. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Draws selected row at y=56, footer text, and one of two arrow bitmaps.
 * Firmware address: 0x2000a1b8. */
void pico_ui_draw_footer_option(char *text, int has_more_options)
{
    uint8_t *bitmap;

    GFX_fill_rectangle(&g_display, 0, 0x38, 0x80, 8, 0);
    GFX_fill_rectangle(&g_display, 8, 0x38, 0x70, 8, 1);
    GFX_draw_bitmap(&g_display, 0, 0x38, "\x1f?\x7f\x7f\x7f\x7f?\x1f", 8, 8, 1);
    GFX_set_text_color(&g_display, 0);
    pico_display_center_text(7, text);
    GFX_set_text_color(&g_display, 1);
    if (has_more_options == 0) {
        bitmap = &g_ui_menus_unknown_01;
    } else {
        bitmap = &g_ui_menus_unknown_03;
    }
    GFX_draw_bitmap(&g_display, 0x78, 0x38, bitmap, 8, 8, 1);
    return;
}

/* HOLDING TEMP and TOTAL TIME labels; converts Fahrenheit for metric preference.
 * Firmware address: 0x2000a240. */
void pico_ui_draw_hold_status(void)
{
    uint32_t value_1;
    uint32_t value_2;
    int __aeabi_d2iz_result;
    char *itoa_result;
    double __aeabi_dsub_result;
    char buffer_38[4];
    char buffer_34[12];
    char buffer_28[28];

    value_2 = g_wort_target_fahrenheit_high;
    value_1 = g_wort_target_fahrenheit_low;
    __aeabi_dsub_result =
        (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low);
    if (g_temperature_unit_celsius == '\0') {
        libc_strcpy(buffer_38, "F");
    } else {
        libc_strcpy(buffer_38, "C");
        __aeabi_dsub_result = __aeabi_dsub((double)pico_join_u32_u32(value_2, value_1), 32.0);
        __aeabi_dsub_result = __aeabi_ddiv(__aeabi_dsub_result, 1.8);
    }
    libc_strcpy(buffer_28, "HOLDING TEMP:");
    __aeabi_d2iz_result = __aeabi_d2iz(__aeabi_dsub_result);
    itoa_result = itoa(__aeabi_d2iz_result, buffer_34, 10);
    libc_strcat(buffer_28, itoa_result);
    libc_strcat(buffer_28, buffer_38);
    pico_display_center_text(3, buffer_28);
    libc_strcpy(buffer_28, "TOTAL TIME:");
    __aeabi_d2iz_result = g_step_time_seconds / 0xe10;
    itoa_result = itoa(__aeabi_d2iz_result, buffer_34, 10);
    libc_strcat(buffer_28, itoa_result);
    libc_strcat(buffer_28, ":");
    __aeabi_d2iz_result = __aeabi_d2iz_result * -0x3c;
    if (g_step_time_seconds / 0x3c + __aeabi_d2iz_result < 1) {
        __aeabi_d2iz_result = 0;
    } else {
        __aeabi_d2iz_result = __aeabi_d2iz_result + g_step_time_seconds / 0x3c;
        if (9 < __aeabi_d2iz_result)
            goto branch_1;
    }
    libc_strcat(buffer_28, "0");
branch_1:
    itoa_result = itoa(__aeabi_d2iz_result, buffer_34, 10);
    libc_strcat(buffer_28, itoa_result);
    libc_strcat(buffer_28, ":");
    __aeabi_d2iz_result = g_step_time_seconds % 0x3c;
    if (__aeabi_d2iz_result < 10) {
        libc_strcat(buffer_28, "0");
    }
    itoa_result = itoa(__aeabi_d2iz_result, buffer_34, 10);
    libc_strcat(buffer_28, itoa_result);
    pico_display_center_text(4, buffer_28);
    pico_display_draw_titled_frame(&g_screen_recipe_name);
    pico_ui_draw_footer_option("Drain", 1);
    return;
}

/* Footer wrapper selecting the single-option arrow mode.
 * Firmware address: 0x2000a3a8. */
void pico_ui_draw_footer_single(char *text)
{
    pico_ui_draw_footer_option(text, 0);
    return;
}

/* Localized PAUSED title with context-dependent continue text.
 * Firmware address: 0x2000a3b0. */
void pico_ui_draw_paused(void)
{
    int step_filter_is_inserted_result;
    char *text;
    char buffer_20[16];

    pico_display_clear();
    pico_display_draw_titled_frame(&g_screen_recipe_name);
    if (g_language == '\x01') {
        text = "PAUSADO";
    } else if (g_language == '\x02') {
        text = "EN PAUSE";
    } else {
        text = "PAUSED";
    }
    pico_display_center_text(3, text);
    step_filter_is_inserted_result = pico_step_filter_is_inserted();
    if (step_filter_is_inserted_result == 0) {
        pico_label_insert_step_filter(buffer_20);
    } else {
        pico_label_continue(buffer_20);
    }
    pico_ui_draw_footer_single(buffer_20);
    pico_display_flush();
    return;
}

/* Copies localized stop label through helper0x20007e98 then renders footer.
 * Firmware address: 0x2000a454. */
void pico_ui_draw_stop_footer(void)
{
    char buffer_20[16];

    pico_label_done(buffer_20);
    pico_ui_draw_footer_single(buffer_20);
    return;
}

/* Shows Wort Temp and Ellapsed using temperature and elapsed seconds.
 * Firmware address: 0x2000a468. */
void pico_ui_draw_circulation_status(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3)
{
    int param4;
    uint32_t param4_00;
    uint8_t param5;
    uint32_t value_1;
    uint32_t param5_00;
    double __aeabi_dsub_result;
    char buffer_30[32];

    pico_screen_draw_program_notice();
    if (g_temperature_unit_celsius == '\0') {
        param4 = __aeabi_d2iz((double)pico_join_u32_u32(argument_2, argument_1));
        param5 = 'F';
    } else {
        __aeabi_dsub_result = __aeabi_dsub((double)pico_join_u32_u32(argument_2, argument_1), 32.0);
        __aeabi_dsub_result = __aeabi_ddiv(__aeabi_dsub_result, 1.8);
        param4 = __aeabi_d2iz(__aeabi_dsub_result);
        param5 = 'C';
    }
    libc_snprintf(buffer_30, 0x20, "%-10s  %3d%c", "Wort Temp:", param4, param5);
    pico_display_text_at_cell(3, 3, buffer_30);
    value_1 = (argument_3 % 0xe10) / 0x3c;
    param4_00 = argument_3 / 0xe10;
    param5_00 = value_1;
    if (argument_3 / 0xe10 == 0) {
        param5_00 = (argument_3 % 0xe10) % 0x3c;
        param4_00 = value_1;
    }
    libc_snprintf(buffer_30, 0x20, "%-10s %02d:%02d", "Ellapsed:", param4_00, param5_00);
    pico_display_text_at_cell(4, 3, buffer_30);
    pico_ui_draw_stop_footer();
    pico_display_flush();
    return;
}

/* Copies localized continue label through0x20007ef0 then renders footer.
 * Firmware address: 0x2000a520. */
void pico_ui_draw_continue_footer(void)
{
    char buffer_20[16];

    pico_label_continue(buffer_20);
    pico_ui_draw_footer_single(buffer_20);
    pico_display_flush();
    return;
}

/* Draws horizontal lines below a text row with column margin.
 * Firmware address: 0x2000a758. */
void pico_ui_draw_separator(short margin_columns, short row, int line_count)
{
    short y0;
    int index;

    for (index = 1; index <= line_count; index = index + 1) {
        y0 = row * 8 + (short)index;
        GFX_draw_line(&g_display, margin_columns * 6, y0, margin_columns * -6 + 0x80, y0, 1);
    }
    return;
}

/* Renders title on row0, separator at row1, and flushes display.
 * Firmware address: 0x2000a7a4. */
void pico_ui_draw_header(char *text)
{
    pico_display_center_text(0, text);
    pico_ui_draw_separator(2, 1, 1);
    pico_display_flush();
    return;
}

/* Draws header text, separator on next row, flushes display.
 * Firmware address: 0x2000a8b8. */
void pico_ui_draw_header_at_row(char *text, short row)
{
    pico_display_center_text(row, text);
    pico_ui_draw_separator(2, row + 1, 1);
    pico_display_flush();
    return;
}

/* DESCALE header and first descale instruction text block.
 * Firmware address: 0x2000a8d8. */
void pico_ui_draw_descale_preparation(void)
{
    pico_ui_draw_header_at_row("DESCALE", 0);
    pico_display_center_text(2, "If you DO NOT use");
    pico_display_center_text(3, "distilled water in");
    pico_display_center_text(4, "reservoir Pico needs");
    pico_display_center_text(5, "to be descaled.");
    pico_display_center_text(6, "Run Descale Now?");
    return;
}

/* DESCALE header and second instruction text block.
 * Firmware address: 0x2000a928. */
void pico_ui_draw_descale_rinse_preparation(void)
{
    pico_ui_draw_header_at_row("DESCALE", 0);
    pico_display_center_text(2, "If you ONLY use");
    pico_display_center_text(3, "distilled water in");
    pico_display_center_text(4, "reservoir please ");
    pico_display_center_text(5, "update setting on");
    pico_display_center_text(6, "picobrew.com");
    return;
}

/* Header wrapper selecting text row1.
 * Firmware address: 0x2000a978. */
void pico_ui_draw_subheader(char *text)
{
    pico_ui_draw_header_at_row(text, 1);
    return;
}

/* HEATER MALFUNCTION heading and recovery instructions.
 * Firmware address: 0x2000a980. */
void pico_ui_draw_heater_malfunction(void)
{
    pico_ui_draw_subheader("HEATER MALFUNCTION");
    pico_display_center_text(3, "ERROR CODE 24");
    pico_display_center_text(4, "UNPLUG PICO");
    pico_display_center_text(5, "IMMEDIATELY AND");
    pico_display_center_text(6, "EMAIL SUPPORT");
    pico_display_center_text(7, "info@picobrew.com");
    pico_display_flush();
    return;
}

/* Renders DESCALE subheader.
 * Firmware address: 0x2000a9d0. */
void pico_ui_draw_descale_heading(void)
{
    pico_ui_draw_subheader("DESCALE");
    return;
}

/* Localized SET LANGUAGE heading.
 * Firmware address: 0x2000aa10. */
void pico_ui_draw_language_menu(void)
{
    char *source;
    char buffer_20[16];

    if (g_language == '\x01') {
        source = "CONFIGURE EL IDIOMA";
    } else if (g_language == '\x02') {
        source = "CHOISIR LA LANGUE";
    } else {
        source = "SET LANGUAGE";
    }
    libc_strcpy(buffer_20, source);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Localized SET UNITS title and Imperial/Metric choices; returns2.
 * Firmware address: 0x2000aa4c. */
uint32_t pico_ui_draw_units_menu(void)
{
    char *source;
    char buffer_20[16];

    pico_display_clear_row(1);
    if (g_language == '\x01') {
        libc_strcpy(buffer_20, "DEFINIR UNIDADES");
        libc_strcpy(&g_menu_label_0, "Imperial");
        source = "Metrico";
    } else if (g_language == '\x02') {
        libc_strcpy(buffer_20, "Choisir l\'unite");
        libc_strcpy(&g_menu_label_0, "Imperial");
        source = "Metrique";
    } else {
        libc_strcpy(buffer_20, "SET UNITS");
        libc_strcpy(&g_menu_label_0, "Imperial");
        source = "Metric";
    }
    libc_strcpy(&g_menu_label_1, source);
    pico_ui_draw_subheader(buffer_20);
    return 2;
}

/* Localized DRAIN STEP FILTER screen and active drain-pump text.
 * Firmware address: 0x2000aad8. */
void pico_ui_draw_drain_filter(void)
{
    char *text;
    char buffer_20[16];

    if (g_language == '\x01') {
        libc_strcpy(buffer_20, "FILTRO DEL DRENAJE");
        pico_display_center_text(3, "Bomba de Drenaje");
        text = "activa";
    } else if (g_language == '\x02') {
        libc_strcpy(buffer_20, "VIDANGE DU FILTRE");
        pico_display_center_text(3, "Pompe de vidange");
        text = "active";
    } else {
        libc_strcpy(buffer_20, "DRAIN STEP FILTER");
        text = "Drain Pump active";
    }
    pico_display_center_text(4, text);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Localized DRAIN RESERVOIR screen and active shuttle-pump text.
 * Firmware address: 0x2000ab50. */
void pico_ui_draw_drain_reservoir(void)
{
    char *text;
    char buffer_20[16];

    if (g_language == '\x01') {
        libc_strcpy(buffer_20, "DRENE EL RESERVORIO");
        pico_display_center_text(3, "Haga clic");
        text = "cuando termine";
    } else if (g_language == '\x02') {
        libc_strcpy(buffer_20, "VIDANGER LE RESERVOIR");
        pico_display_center_text(3, "Cliquez lorsque");
        text = "termine";
    } else {
        libc_strcpy(buffer_20, "DRAIN RESERVOIR");
        pico_display_center_text(3, "Shuttle Pump active");
        text = "Click when done";
    }
    pico_display_center_text(5, text);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Localized Would you like to prompt under a context-generated title.
 * Firmware address: 0x2000abd4. */
void pico_ui_draw_choice_question(void)
{
    char *text;
    char buffer_28[32];

    pico_display_clear();
    pico_label_continue(buffer_28);
    libc_strcat(buffer_28, g_ui_screens_unknown_10);
    libc_strcat(buffer_28, "?");
    if (g_language == '\x01') {
        text = "Desea";
    } else if (g_language == '\x02') {
        text = "Voulez-vous";
    } else {
        text = "Would you like to";
    }
    pico_display_center_text(3, text);
    pico_ui_draw_subheader(g_ui_screens_unknown_11);
    pico_display_center_text(4, buffer_28);
    return;
}

/* Localized FIRST RINSE heading.
 * Firmware address: 0x2000ac3c. */
void pico_ui_draw_first_rinse_heading(void)
{
    char *source;
    char buffer_20[16];

    if (g_language == '\x01') {
        source = "PRIMER LAVADO";
    } else if (g_language == '\x02') {
        source = "PREMIER RINCER";
    } else {
        source = "FIRST RINSE";
    }
    libc_strcpy(buffer_20, source);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Localized CHOOSE NETWORK heading.
 * Firmware address: 0x2000ac78. */
void pico_ui_draw_choose_network(void)
{
    char *source;
    char buffer_20[16];

    if (g_language == '\x01') {
        source = "ELIJA LA RED";
    } else if (g_language == '\x02') {
        source = "CHOISIR UN RESEAU";
    } else {
        source = "CHOOSE NETWORK";
    }
    libc_strcpy(buffer_20, source);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Localized TEST WIFI heading.
 * Firmware address: 0x2000acb4. */
void pico_ui_draw_wifi_test_heading(void)
{
    char *text;

    if (g_language == '\x01') {
        text = "VERIFIQUE EL WIFI";
    } else if (g_language == '\x02') {
        text = "TEST SANS FIL";
    } else {
        text = "TEST WIFI";
    }
    pico_ui_draw_subheader(text);
    return;
}

/* Localized UTILITIES heading.
 * Firmware address: 0x2000ace0. */
void pico_ui_draw_utilities_heading(void)
{
    char *source;
    char buffer_20[16];

    if (g_language == '\x01') {
        source = "HERRAMIENTAS";
    } else if (g_language == '\x02') {
        source = "SERVICES";
    } else {
        source = "UTILITIES";
    }
    libc_strcpy(buffer_20, source);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Localized SETTINGS heading.
 * Firmware address: 0x2000ad1c. */
void pico_ui_draw_settings_heading(void)
{
    char *source;
    char buffer_20[16];

    if (g_language == '\x01') {
        source = "CONFIGURACION";
    } else if (g_language == '\x02') {
        source = "PARAMETRES";
    } else {
        source = "SETTINGS";
    }
    libc_strcpy(buffer_20, source);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Obtains program title from0x20008378 and renders subheader.
 * Firmware address: 0x2000ad58. */
void pico_ui_draw_current_program_heading(void)
{
    char buffer_20[16];

    pico_label_service(buffer_20);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Localized NETWORK heading.
 * Firmware address: 0x2000ad6c. */
void pico_ui_draw_network_heading(void)
{
    char *source;
    char buffer_20[16];

    if (g_language == '\x01') {
        source = "RED";
    } else if (g_language == '\x02') {
        source = "RESEAU";
    } else {
        source = "NETWORK";
    }
    libc_strcpy(buffer_20, source);
    pico_ui_draw_subheader(buffer_20);
    return;
}

/* Localized FACTORY RESET heading.
 * Firmware address: 0x2000ada8. */
void pico_ui_draw_factory_reset_heading(void)
{
    char *text;

    if (g_language == '\x01') {
        text = "REINICIO DE LA PLANTA";
    } else {
        if (g_language == '\x02') {
            pico_ui_draw_header_at_row("REINITIALISATION", 0);
            return;
        }
        text = "FACTORY RESET";
    }
    pico_ui_draw_subheader(text);
    return;
}

/* Asks whether to reset Pico to factory firmware.
 * Firmware address: 0x2000add8. */
void pico_ui_draw_firmware_reset_question(void)
{
    short row;
    char *text;

    pico_ui_draw_factory_reset_heading();
    if (g_language == '\x01') {
        pico_display_center_text(3, "Reiniciar Pico");
        pico_display_center_text(4, "al firmware de");
        row = 5;
        text = "la Planta?";
    } else if (g_language == '\x02') {
        pico_display_center_text(3, "Reinitialiser au");
        pico_display_center_text(4, "micrologiciel de");
        text = "Pico par defaut?";
        row = 5;
    } else {
        pico_display_center_text(3, "Reset Pico to");
        row = 4;
        text = "Factory firmware?";
    }
    pico_display_center_text(row, text);
    return;
}

/* Asks whether to remove stored WiFi credentials.
 * Firmware address: 0x2000ae4c. */
void pico_ui_draw_clear_wifi_question(void)
{
    short row;
    char *text;

    pico_ui_draw_factory_reset_heading();
    if (g_language == '\x01') {
        pico_display_center_text(3, "Retirar las");
        pico_display_center_text(4, "credenciales de");
        row = 5;
        text = "Wi-Fi almacenadas?";
    } else if (g_language == '\x02') {
        pico_display_center_text(2, "Supprimer les");
        pico_display_center_text(3, "informations");
        pico_display_center_text(4, "d\'identification");
        text = "sans fil archivees?";
        row = 5;
    } else {
        pico_display_center_text(3, "Remove stored WiFi");
        row = 4;
        text = "credentials?";
    }
    pico_display_center_text(row, text);
    return;
}

/* Fixed startup title rendered at row0.
 * Firmware address: 0x2000aecc. */
void pico_ui_draw_startup_heading(void)
{
    pico_ui_draw_header_at_row(*(char **)(g_active_program_context_as_int + 0x34), 0);
    return;
}

/* Localized DEEP CLEAN heading.
 * Firmware address: 0x2000aedc. */
void pico_ui_draw_deep_clean_heading(void)
{
    char *source;
    char buffer_20[16];

    if (g_language == '\x01') {
        source = "LIMPIEZA PROFUNDA";
    } else if (g_language == '\x02') {
        source = "NETTOYAGE";
    } else {
        source = "DEEP CLEAN";
    }
    libc_strcpy(buffer_20, source);
    pico_ui_draw_header_at_row(buffer_20, 0);
    return;
}

/* drawScreen log, ScreenId bounds check, localized instruction lines and optional bitmap/status
 * fields. Firmware address: 0x2000aff8. */
void pico_ui_draw_instruction_screen(uint32_t screen_id, uint32_t language, char *title,
                                     int use_context_title)
{
    int param4;
    short y;
    uint8_t param5;
    uint8_t *bitmap;
    int index;
    int param5_00;
    double __aeabi_i2d_result;
    short height;
    PicoTextScreen value_4;
    char buffer_8[32];
    char buffer_98[132];

    pico_assert_nonfatal((uint32_t)(screen_id < 0x19),
                         "0 <= screenId && screenId < ScreenId::_EnumCount");
    libc_snprintf(buffer_98, 0x80, "drawScreen %d %20s %s", language, title,
                  (&g_ui_screens_unknown_01)[screen_id]);
    pico_log_serial("I", "LANG", buffer_98);
    if (use_context_title == 0) {
        pico_ui_draw_header_at_row(title, 0);
    } else {
        pico_display_draw_titled_frame(title);
    }
    value_4.line_count = 0;
    pico_screen_get_instruction_lines(&value_4, screen_id, language);
    for (index = 0; index < (int)value_4.line_count; index = index + 1) {
        pico_display_center_text((short)index + 3, value_4.lines[index]);
    }
    if (screen_id == 0xb) {
        height = 0x10;
        bitmap = &g_ui_screens_unknown_07;
        y = 0x17;
    } else if (screen_id == 0x11) {
        height = 0x10;
        bitmap = &g_ui_screens_unknown_08;
        y = 0x17;
    } else {
        if (screen_id != 0x12) {
            if (screen_id != 0x13) {
                return;
            }
            index = g_session_hold_minutes / 0x3c;
            param5_00 = g_session_hold_minutes % 0x3c;
            if (g_temperature_unit_celsius == '\0') {
                param5 = 'F';
                param4 = g_session_target_fahrenheit_as_int;
            } else {
                __aeabi_i2d_result = __aeabi_i2d(g_session_target_fahrenheit_as_int);
                __aeabi_i2d_result = __aeabi_dsub(__aeabi_i2d_result, 32.0);
                __aeabi_i2d_result = __aeabi_ddiv(__aeabi_i2d_result, 1.8);
                param4 = __aeabi_d2iz(__aeabi_i2d_result);
                param5 = 'C';
            }
            libc_snprintf(buffer_8, 0x20, "%-10s %3d%c", "Target:", param4, param5);
            pico_display_center_text(3, buffer_8);
            libc_snprintf(buffer_8, 0x20, "%-10s %02d:%02d", "Hold Time:", index, param5_00);
            pico_display_center_text(4, buffer_8);
            return;
        }
        height = 0x1e;
        bitmap = &g_ui_screens_unknown_06;
        y = 0x10;
    }
    GFX_draw_bitmap_alias(&g_display, 0, y, bitmap, 0x80, height, 1);
    return;
}

/* Instruction renderer wrapper with normal title mode.
 * Firmware address: 0x2000b170. */
void pico_ui_draw_instruction_with_header(uint32_t screen_id, uint32_t language, char *title)
{
    pico_ui_draw_instruction_screen(screen_id, language, title, 0);
    return;
}

/* Instruction renderer wrapper with program-context title mode.
 * Firmware address: 0x2000b176. */
void pico_ui_draw_instruction_with_context(uint32_t screen_id, uint32_t language, char *title)
{
    pico_ui_draw_instruction_screen(screen_id, language, title, 1);
    return;
}

/* Localized ENTER PASSWORD heading.
 * Firmware address: 0x2000b1b0. */
void pico_ui_draw_password_heading(void)
{
    char *text;

    pico_display_clear();
    if (g_language == '\x01') {
        text = "CONTRASENA";
    } else if (g_language == '\x02') {
        text = "MOT DE PASSE";
    } else {
        text = "ENTER PASSWORD";
    }
    pico_ui_draw_header_at_row(text, 0);
    return;
}

/* Localized RACK BEER heading.
 * Firmware address: 0x2000b24c. */
void pico_ui_draw_rack_beer_heading(void)
{
    char *text;

    if (g_language == '\x01') {
        text = "EMBARRILAR CERVEZA";
    } else if (g_language == '\x02') {
        text = "ENFUTAGE DE LA BIERE";
    } else {
        text = "RACK BEER";
    }
    pico_display_center_text(0, text);
    pico_ui_draw_separator(2, 1, 1);
    return;
}

/* Localized Are you sure you want to stop prompt.
 * Firmware address: 0x2000b28c. */
void pico_ui_draw_stop_confirmation(void)
{
    char *text;
    char buffer_20[24];

    pico_display_clear();
    if (g_language == '\x01') {
        pico_display_center_text(1, "Esta seguro");
        text = "para detener:";
    } else {
        if (g_language != '\x02') {
            pico_display_center_text(1, "Are you sure you");
            pico_display_center_text(2, "want to");
            libc_strcpy(buffer_20, "stop ");
            goto branch_1;
        }
        pico_display_center_text(1, "Etes-vous certain");
        text = "de vouloir arreter";
    }
    pico_display_center_text(2, text);
    buffer_20[0] = '\0';
branch_1:
    pico_append_program_state_label(buffer_20);
    libc_strcat(buffer_20, "?");
    pico_display_center_text(3, buffer_20);
    pico_ui_draw_separator(2, 4, 1);
    return;
}

/* Localized skip-current-step confirmation.
 * Firmware address: 0x2000b328. */
void pico_ui_draw_skip_step_confirmation(void)
{
    char *text;

    pico_display_clear();
    if (g_language == '\x01') {
        pico_display_center_text(1, "Esta seguro de que");
        pico_display_center_text(2, "desea saltarse");
        text = "el paso actual?";
    } else if (g_language == '\x02') {
        pico_display_center_text(1, "Etes-vous certain");
        pico_display_center_text(2, "de vouloir sauter");
        text = "cette etape?";
    } else {
        pico_display_center_text(1, "Are you sure you");
        pico_display_center_text(2, "want to");
        text = "skip current step?";
    }
    pico_display_center_text(3, text);
    pico_ui_draw_separator(2, 4, 1);
    return;
}

/* Switches error code into localized hardware/network/PicoPak error messages.
 * Firmware address: 0x2000b3b4. */
void pico_ui_draw_error(int argument_1)
{
    short row;
    char *itoa_result;
    char *source;
    char buffer[12];
    char buffer_50[32];
    char buffer_30[36];

    buffer_30[0] = '\0';
    buffer_30[1] = '\0';
    buffer_30[2] = '\0';
    buffer_30[3] = '\0';
    libc_memset(buffer_30 + 4, 0, 0x1a);
    pico_display_clear();
    if (g_language == '\x01') {
        itoa_result = "Introduzca el codigo";
    } else if (g_language == '\x02') {
        itoa_result = "Inscrire le code";
    } else {
        itoa_result = "Enter Code";
    }
    pico_display_center_text(3, itoa_result);
    if (argument_1 == 9) {
        if (g_language == '\x01') {
            libc_strcpy(buffer_50, "WIFI INICIALIZADO");
            itoa_result = buffer_30;
            source = "INCORRECTAMENTE";
        } else if (g_language == '\x02') {
            libc_strcpy(buffer_50, "WIFI EST");
            itoa_result = buffer_30;
            source = "INITIALISE MAL";
        } else {
            libc_strcpy(buffer_50, "WIFI INITIALIZED");
            itoa_result = buffer_30;
            source = "INCORRECTLY";
        }
        goto branch_1;
    }
    if (argument_1 < 10) {
        if (argument_1 == 4) {
            if (g_language == '\x01') {
                libc_strcpy(buffer_50, "STEPPER MSP");
                itoa_result = buffer_30;
                source = "NO RESPONDE";
            } else if (g_language == '\x02') {
                libc_strcpy(buffer_50, "STEPPER MSP");
                itoa_result = buffer_30;
                source = "sans reponse";
            } else {
                libc_strcpy(buffer_50, "STEPPER PROCESSOR");
                itoa_result = buffer_30;
                source = "UNRESPONSIVE";
            }
            goto branch_1;
        }
        if (argument_1 < 5) {
            if (argument_1 == 2) {
                if (g_language == '\x01') {
                branch_2:
                    libc_strcpy(buffer_50, "FALLA AL INICIAR");
                    itoa_result = buffer_30;
                    source = "LA RFID";
                    goto branch_1;
                }
                if (g_language == '\x02') {
                branch_3:
                    libc_strcpy(buffer_50, "ECHEC DE L\'INIT");
                    itoa_result = buffer_30;
                    source = "DU RFID";
                    goto branch_1;
                }
                itoa_result = "WIFI FAILED TO";
            } else {
                if (2 < argument_1) {
                    if (g_language == '\x01') {
                        libc_strcpy(buffer_50, "NO SE PUDO RESOLVER");
                        itoa_result = buffer_30;
                        source = "EL DNS";
                    } else if (g_language == '\x02') {
                        libc_strcpy(buffer_50, "INCAPABLE DE RESOUDRE");
                        itoa_result = buffer_30;
                        source = "LE DNS";
                    } else {
                        libc_strcpy(buffer_50, "COULD NOT RESOLVE");
                        itoa_result = buffer_30;
                        source = "DNS";
                    }
                    goto branch_1;
                }
                if (argument_1 != 1)
                    goto branch_4;
                if (g_language == '\x01')
                    goto branch_2;
                if (g_language == '\x02')
                    goto branch_3;
                itoa_result = "RFID FAILED TO";
            }
            libc_strcpy(buffer_50, itoa_result);
            itoa_result = buffer_30;
            source = "INITIALIZE";
            goto branch_1;
        }
        if (argument_1 == 6) {
            if (g_language != '\x01') {
                if (g_language == '\x02') {
                    libc_strcpy(buffer_50, "PICO N\'EST PAS DANS");
                    itoa_result = buffer_30;
                    source = "LA BASE DE DONNEES";
                } else {
                    libc_strcpy(buffer_50, "PICO NOT IN");
                    itoa_result = buffer_30;
                    source = "DATABASE";
                }
                goto branch_1;
            }
            itoa_result = "PICO NO SE ENCUENTRA";
            goto branch_5;
        }
        if (argument_1 < 6) {
            if (g_language == '\x01') {
                libc_strcpy(buffer_50, "MSP ENVIO LA");
                itoa_result = buffer_30;
                source = "COMUNICACION MAL";
            } else if (g_language == '\x02') {
                libc_strcpy(buffer_50, "LE MSP A ENVOYE UNE");
                itoa_result = buffer_30;
                source = "COMMUNICATION MAL";
            } else {
                libc_strcpy(buffer_50, "PROCESSOR SENT WRONG");
                itoa_result = buffer_30;
                source = "COMMUNICATION";
            }
            goto branch_1;
        }
        if (argument_1 == 7) {
            if (g_language == '\x01') {
                libc_strcpy(buffer_50, "COMUNICACION");
                itoa_result = buffer_30;
                source = "DEL SERVIDOR";
                goto branch_1;
            }
            if (g_language == '\x02') {
                libc_strcpy(buffer_50, "COMMUNICATION");
                itoa_result = buffer_30;
                source = "DU SERVEUR";
                goto branch_1;
            }
            itoa_result = "SERVER COMMUNICATION";
            goto branch_6;
        }
        if (argument_1 == 8) {
            if (g_language == '\x01') {
                libc_strcpy(buffer_50, "NO CONECTADO AL");
                itoa_result = buffer_30;
                source = "PUNTO DE ACCESO";
            } else if (g_language == '\x02') {
                libc_strcpy(buffer_50, "N\'EST PAS CONNECTE");
                itoa_result = buffer_30;
                source = "A UN POINT D\'ACCES";
            } else {
                libc_strcpy(buffer_50, "NOT CONNECTED TO");
                itoa_result = buffer_30;
                source = "ACCESS POINT";
            }
            goto branch_1;
        }
    } else {
        if (argument_1 == 0xe) {
            if (g_language != '\x01') {
                if (g_language == '\x02') {
                    libc_strcpy(buffer_50, &g_ui_screens_unknown_02);
                    itoa_result = buffer_30;
                    source = "PRESENT DANS LA DB";
                } else {
                    libc_strcpy(buffer_50, "RFID NOT FOUND");
                    itoa_result = buffer_30;
                    source = "IN DATABASE";
                }
                goto branch_1;
            }
            itoa_result = "RFID NO SE ENCUENTRA";
        branch_5:
            libc_strcpy(buffer_50, itoa_result);
            itoa_result = buffer_30;
            source = "EN LA BASE DE DATOS";
            goto branch_1;
        }
        if (argument_1 < 0xf) {
            if (argument_1 == 0xb) {
                if (g_language == '\x01') {
                    libc_strcpy(buffer_50, "NO TIENE DIRECCION");
                    itoa_result = buffer_30;
                    source = "IP ASIGNADA";
                } else if (g_language == '\x02') {
                    libc_strcpy(buffer_50, "LE PICO N\'A PAS");
                    itoa_result = buffer_30;
                    source = "D\'ADRESSE IP";
                } else {
                    libc_strcpy(buffer_50, "PICO NOT ASSIGNED");
                    itoa_result = buffer_30;
                    source = "IP ADDRESS";
                }
                goto branch_1;
            }
            if (argument_1 < 0xb) {
                if (g_language == '\x01') {
                    libc_strcpy(buffer_50, "PUNTO DE ACCESO AL");
                    itoa_result = buffer_30;
                    source = "WIFI DESCONECTADO";
                } else if (g_language == '\x02') {
                    libc_strcpy(buffer_50, "LE POINT D\'ACCES DU");
                    itoa_result = buffer_30;
                    source = "WIFI EST DECONNECTE";
                } else {
                    libc_strcpy(buffer_50, "WIFI ACCESS POINT");
                    itoa_result = buffer_30;
                    source = "DISCONNECTED";
                }
                goto branch_1;
            }
            if (argument_1 == 0xc) {
                if (g_language == '\x01') {
                    libc_strcpy(buffer_50, "ERROR DE LECTURA");
                    itoa_result = buffer_30;
                    source = "DE LA RFID";
                    goto branch_1;
                }
                if (g_language == '\x02') {
                    libc_strcpy(buffer_50, "ERREUR DE");
                    itoa_result = buffer_30;
                    source = "LECTURE RFID";
                    goto branch_1;
                }
                itoa_result = "RFID READ";
            } else {
                if (argument_1 != 0xd)
                    goto branch_4;
                if (g_language == '\x01') {
                    libc_strcpy(buffer_50, "ERROR EN LA RECETA DE");
                    itoa_result = buffer_30;
                    source = "ANALISIS DE LECTURA";
                    goto branch_1;
                }
                if (g_language == '\x02') {
                    libc_strcpy(buffer_50, "ERREUR DE RECETTE");
                    itoa_result = buffer_30;
                    source = "PARSING";
                    goto branch_1;
                }
                itoa_result = "PARSING RECIPE";
            }
        branch_6:
            libc_strcpy(buffer_50, itoa_result);
            itoa_result = buffer_30;
            source = "ERROR";
            goto branch_1;
        }
        if (argument_1 == 0x12) {
            if (g_language == '\x01') {
                libc_strcpy(buffer_50, "VALVULA ROTATORIA");
                itoa_result = buffer_30;
                source = "ATASCADA";
            } else if (g_language == '\x02') {
                libc_strcpy(buffer_50, "LA VALVE ROTATIVE");
                itoa_result = buffer_30;
                source = "EST BLOQUEE";
            } else {
                libc_strcpy(buffer_50, "ROTARY VALVE");
                itoa_result = buffer_30;
                source = "SEIZED";
            }
            goto branch_1;
        }
        if (argument_1 < 0x13) {
            if (argument_1 == 0xf) {
                if (g_language == '\x01') {
                    libc_strcpy(buffer_50, "FALLA DEL SENSOR");
                    itoa_result = buffer_30;
                    source = "DE TEMPERATURA";
                } else if (g_language == '\x02') {
                    libc_strcpy(buffer_50, "ECHEC TEMPORAIRE");
                    itoa_result = buffer_30;
                    source = "DU CAPTEUR";
                } else {
                    libc_strcpy(buffer_50, "TEMP SENSOR");
                    itoa_result = buffer_30;
                    source = "FAILURE";
                }
                goto branch_1;
            }
        } else {
            if (argument_1 == 0x13) {
                if (g_language == '\x01') {
                    libc_strcpy(buffer_50, "EL BLOQUE");
                    itoa_result = buffer_30;
                    source = "NO CALIENTA";
                } else if (g_language == '\x02') {
                    libc_strcpy(buffer_50, "LE BLOC THERMIQUE");
                    itoa_result = buffer_30;
                    source = "NE CHAUFFE PAS";
                } else {
                    libc_strcpy(buffer_50, "THERMO BLOCK");
                    itoa_result = buffer_30;
                    source = "NOT HEATING";
                }
                goto branch_1;
            }
            if (argument_1 == 99) {
                itoa_result = buffer_50;
                source = "TEST_ERROR";
                goto branch_1;
            }
        }
    }
branch_4:
    itoa_result = buffer_50;
    if (g_language == '\x01') {
        source = "ERROR DE HARDWARE";
    } else if (g_language == '\x02') {
        source = "ERREUR DU MATERIEL";
    } else {
        source = "HARDWARE ERROR";
    }
branch_1:
    libc_strcpy(itoa_result, source);
    pico_display_center_text(0, buffer_50);
    pico_display_center_text(1, buffer_30);
    GFX_set_text_size(&g_display, '\x02');
    itoa_result = itoa(argument_1, buffer, 10);
    pico_display_center_text(4, itoa_result);
    GFX_set_text_size(&g_display, '\x01');
    pico_display_center_text(6, "at picobrew.com/err");
    if (buffer_30[0] == '\0') {
        row = 1;
    } else {
        row = 2;
    }
    pico_ui_draw_separator(3, row, 2);
    return;
}

/* Localized insert-associated-PicoPak prompt.
 * Firmware address: 0x2000b99c. */
void pico_ui_draw_insert_picopak(void)
{
    char *text_1;

    pico_display_clear();
    if (g_language == '\x01') {
        text_1 = "Por favor inserte";
    } else {
        if (g_language == '\x02') {
            pico_display_center_text(3, "Veillez inserer");
            text_1 = "associer le PicoPak";
            goto branch_1;
        }
        text_1 = "Please insert";
    }
    pico_display_center_text(3, text_1);
    text_1 = "ERROR DE PicoPak";
branch_1:
    pico_display_center_text(4, text_1 + 9);
    if (g_language == '\x01') {
        text_1 = "CERVEZA";
    } else if (g_language == '\x02') {
        text_1 = "BRASSER";
    } else {
        text_1 = "BREW";
    }
    pico_display_center_text(1, text_1);
    pico_ui_draw_separator(3, 2, 2);
    return;
}

/* Localized ADJUST ABV with Min/Max scale.
 * Firmware address: 0x2000b9f0. */
void pico_ui_draw_abv_adjustment(void)
{
    char *text;

    if (g_language == '\x01') {
        text = "Ajuste el ABV";
    } else if (g_language == '\x02') {
        text = "AJUSTEZ LE ABV";
    } else {
        text = "ADJUST ABV";
    }
    pico_display_center_text(1, text);
    pico_ui_draw_separator(3, 2, 2);
    pico_display_text_at_cell(6, 0, " Min            Max  ");
    return;
}

/* Localized ADJUST BITTERNESS with Min/Max scale.
 * Firmware address: 0x2000ba34. */
void pico_ui_draw_bitterness_adjustment(void)
{
    char *text;

    if (g_language == '\x01') {
        text = "AJUSTAR DEL AMARGURA";
    } else if (g_language == '\x02') {
        text = "AJUSTER L\'AMERTUME";
    } else {
        text = "ADJUST BITTERNESS";
    }
    pico_display_center_text(1, text);
    pico_ui_draw_separator(1, 2, 2);
    pico_display_text_at_cell(6, 0, " Min            Max  ");
    return;
}

/* Localized BREWING COMPLETE and cooling/yeast instructions.
 * Firmware address: 0x2000ba78. */
void pico_ui_draw_brew_complete(char *recipe_title)
{
    short last_text_row;
    char *last_instruction_line;

    pico_display_clear();
    if (g_language == '\x01') {
        pico_display_center_text(2, "CERVEZA FINALIZADA");
        pico_display_center_text(4, "barril fria para 20C");
        pico_display_center_text(5, "(68F),fermentar la");
        last_text_row = 6;
        last_instruction_line = "levadura add";
    } else if (g_language == '\x02') {
        pico_display_center_text(2, "BRASSAGE COMPLETE");
        pico_display_center_text(4, "Refroidir le fut a");
        pico_display_center_text(5, "20C (68F) avant");
        pico_display_center_text(6, "d\'ajouter la levure");
        last_instruction_line = "pour fermenter";
        last_text_row = 7;
    } else {
        pico_display_center_text(2, "BREWING COMPLETE");
        pico_display_center_text(4, "Cool keg to 68 F");
        pico_display_center_text(5, "then add yeast");
        last_text_row = 6;
        last_instruction_line = "to ferment";
    }
    pico_display_center_text(last_text_row, last_instruction_line);
    pico_display_center_text(0, recipe_title);
    pico_ui_draw_separator(3, 1, 2);
    return;
}

/* Builds recipe start/optional adjustment/exit choices and returns count.
 * Firmware address: 0x2000bb3c. */
int pico_ui_draw_recipe_choices(char *recipe_title, int allow_adjustments)
{
    int choice_count;

    pico_label_start_brewing(&g_menu_label_0);
    if (allow_adjustments == 0) {
        choice_count = 1;
    } else {
        pico_label_adjust_recipe(&g_menu_label_1);
        choice_count = 2;
    }
    pico_label_cancel(&g_menu_label_0 + choice_count * 0x16);
    pico_display_center_text(1, recipe_title);
    pico_ui_draw_separator(3, 2, 2);
    return choice_count + 1;
}

/* Localized Select PicoPak heading.
 * Firmware address: 0x2000bb80. */
void pico_ui_draw_picopak_selection(void)
{
    char *text;

    if (g_language == '\x01') {
        text = "Seleccione PicoPak";
    } else if (g_language == '\x02') {
        text = "Choisir le PicoPak";
    } else {
        text = "Select PicoPak";
    }
    pico_display_center_text(1, text);
    pico_ui_draw_separator(3, 2, 2);
    return;
}

/* Localized REGISTER PICO and supplied code at picobrew.com/newpico.
 * Firmware address: 0x2000bbc0. */
void pico_ui_draw_registration_code(char *registration_code)
{
    char *text;

    pico_display_clear();
    if (g_language == '\x01') {
        pico_display_center_text(1, "REGISTRE PICO");
        text = "Introduzca el codigo";
    } else if (g_language == '\x02') {
        pico_display_center_text(1, "ENREGISTRER LE PICO");
        text = "Inscrire le code";
    } else {
        pico_display_center_text(1, "REGISTER PICO");
        text = "Enter code";
    }
    pico_display_center_text(3, text);
    pico_display_center_text(7, "picobrew.com/newpico");
    pico_ui_draw_separator(3, 2, 2);
    GFX_set_text_size(&g_display, '\x02');
    pico_display_center_text(4, registration_code);
    GFX_set_text_size(&g_display, '\x01');
    return;
}

/* Switches diagnostic stage into WiFi/DHCP/DNS/ping/server result text; performs no network test
 * itself. Firmware address: 0x2000e610. */
void pico_ui_show_wifi_test_result(uint32_t diagnostic_stage)
{
    short short_value_1;
    char *text_2;

    pico_display_clear();
    pico_ui_draw_wifi_test_heading();
    switch (diagnostic_stage) {
    case 0:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Comprobacion de");
            short_value_1 = 4;
            text_2 = &g_ui_screens_unknown_03;
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Verification de");
            text_2 = "la connexion Wifi";
            short_value_1 = 4;
        } else {
            pico_display_center_text(3, "Checking WiFi");
            short_value_1 = 4;
            text_2 = "Connection";
        }
        break;
    case 1:
        if (g_language == '\x01') {
            pico_display_center_text(3, "NO SE PUEDE ");
            pico_display_center_text(4, "RESOLVER DNS");
            short_value_1 = 5;
            text_2 = "de picobrew.com";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Ne peut resoudre");
            pico_display_center_text(4, "le DNS de");
            text_2 = "picobrew.com";
            short_value_1 = 5;
        } else {
            pico_display_center_text(3, "CANNOT RESOLVE DNS");
            short_value_1 = 4;
            text_2 = "of picobrew.com";
        }
        goto branch_1;
    case 2:
        if (g_language == '\x01') {
            pico_display_center_text(3, "NO HAY IP LOCAL,");
            pico_display_center_text(4, "VERIFICACION DE LA");
            short_value_1 = 5;
            text_2 = "DHCP EN EL ROUTER";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "AUCUN IP LOCAL");
            pico_display_center_text(4, &g_ui_screens_unknown_04);
            short_value_1 = 5;
            text_2 = "DHCP DU ROUTEUR";
        } else {
            pico_display_center_text(3, "NO LOCAL IP CHECK");
            pico_display_center_text(4, "DHCP SETTINGS ON");
            short_value_1 = 5;
            text_2 = "ROUTER";
        }
    branch_1:
        pico_display_center_text(short_value_1, text_2);
        pico_ui_wait_for_continue();
        return;
    case 3:
        short_value_1 = 3;
        if (g_language == '\x01') {
            pico_display_center_text(3, "Servidor que");
            short_value_1 = 4;
            text_2 = "produzca Ping";
        } else if (g_language == '\x02') {
            text_2 = "Ping au serveur";
        } else {
            text_2 = "Pinging server";
        }
        break;
    case 4:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Revise la conexion");
            text_2 = "a los servicios web";
        } else {
            if (g_language != '\x02') {
                pico_display_center_text(3, "Checking connection");
                pico_display_center_text(4, "to PicoBrew");
                short_value_1 = 5;
                text_2 = "web services";
                break;
            }
            pico_display_center_text(3, "Verifier la connexion");
            text_2 = "aux services web";
        }
        pico_display_center_text(4, text_2);
        short_value_1 = 5;
        text_2 = "de Picobrew";
        break;
    case 5:
        pico_display_center_text(3, &g_wifi_ssid);
        if (g_language == '\x01') {
            pico_display_center_text(4, "Internet: SI");
            text_2 = "PicoBrew: SI";
        } else if (g_language == '\x02') {
            pico_display_center_text(4, "Internet: OUI");
            text_2 = "PicoBrew: OUI";
        } else {
            pico_display_center_text(4, "Internet: YES");
            text_2 = "PicoBrew: YES";
        }
        pico_display_center_text(5, text_2);
        return;
    default:
        return;
    }
    pico_display_center_text(short_value_1, text_2);
    pico_display_flush();
    return;
}

/* TitleId bounds and language-indexed label lookup.
 * Firmware address: 0x200133b4. */
char *pico_title_label(uint32_t title_id, uint32_t language)
{
    pico_assert_nonfatal((uint32_t)(title_id < 6),
                         "0 <= titleId && titleId <= TitleId::_EnumCount");
    pico_assert_nonfatal((uint32_t)(language < 3), "0 <= language && language <= 2");
    if (5 < title_id) {
        return "<titleId>";
    }
    if (2 < language) {
        language = 0;
    }
    return (&g_ui_screens_unknown_09)[language + title_id * 3];
}

/* Returns id<6, matching six-entry TitleId table; inferred helper role.
 * Firmware address: 0x20013404. */
bool pico_title_id_is_valid(uint32_t title_id)
{
    return title_id < 6;
}

/* Four-entry localized brew-state text table; distinct from nine controller brew phases.
 * Firmware address: 0x20013410. */
char *pico_brew_display_state_label(uint32_t phase, uint32_t language)
{
    pico_assert_nonfatal((uint32_t)(phase < 4), "isValid(brewStateId)");
    pico_assert_nonfatal((uint32_t)(language < 3), "0 <= language && language <= 2");
    if (2 < language) {
        language = 0;
    }
    if (phase < 4 == 0) {
        return "<brewState>";
    }
    return (&g_ui_screens_unknown_05)[language + phase * 3];
}
