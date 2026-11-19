#include "ui_internal.h"
#include "picobrew/config.h"

void pico_ui_draw_footer_option(const char *text, int has_more_options)
{
    const uint8_t *bitmap;

    pico_graphics_fill_rectangle(&pico_ui_active->display, 0, 0x38, 0x80, 8, 0);
    pico_graphics_fill_rectangle(&pico_ui_active->display, 8, 0x38, 0x70, 8, 1);
    pico_graphics_bitmap(&pico_ui_active->display, 0, 0x38,
                         (const uint8_t *)"\x1f?\x7f\x7f\x7f\x7f?\x1f", 8, 8, 1);
    pico_graphics_text_color(&pico_ui_active->display, 0);
    pico_display_center_text(7, text);
    pico_graphics_text_color(&pico_ui_active->display, 1);
    if (has_more_options == 0) {
        bitmap = pico_ui_bitmap_footer_single;
    } else {
        bitmap = pico_ui_bitmap_footer_multiple;
    }
    pico_graphics_bitmap(&pico_ui_active->display, 0x78, 0x38, bitmap, 8, 8, 1);
    return;
}

void pico_ui_draw_footer_single(const char *text)
{
    pico_ui_draw_footer_option(text, 0);
    return;
}

void pico_ui_draw_paused(void)
{
    int step_filter_is_inserted_result;
    char *text;
    char label_text[32];

    pico_display_clear();
    pico_display_draw_titled_frame(pico_ui_active->app->screen.recipe_name);
    if (pico_ui_active->network->settings->language == '\x01') {
        text = "PAUSADO";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "EN PAUSE";
    } else {
        text = "PAUSED";
    }
    pico_display_center_text(3, text);
    step_filter_is_inserted_result = pico_filter_is_inserted(pico_ui_active->actuators);
    if (step_filter_is_inserted_result == 0) {
        pico_label_insert_step_filter(label_text);
    } else {
        pico_label_continue(label_text);
    }
    pico_ui_draw_footer_single(label_text);
    pico_display_flush();
    return;
}

void pico_ui_draw_stop_footer(void)
{
    char label_text[32];

    pico_label_done(label_text);
    pico_ui_draw_footer_single(label_text);
    return;
}

void pico_ui_draw_continue_footer(void)
{
    char label_text[32];

    pico_label_continue(label_text);
    pico_ui_draw_footer_single(label_text);
    pico_display_flush();
    return;
}

void pico_ui_draw_separator(short margin_columns, short row, int line_count)
{
    short y0;
    int index;

    for (index = 1; index <= line_count; index = index + 1) {
        y0 = row * 8 + (short)index;
        pico_graphics_line(&pico_ui_active->display, margin_columns * 6, y0,
                           margin_columns * -6 + 0x80, y0, 1);
    }
    return;
}

void pico_ui_draw_header(const char *text)
{
    pico_display_center_text(0, text);
    pico_ui_draw_separator(2, 1, 1);
    pico_display_flush();
    return;
}

void pico_ui_draw_header_at_row(const char *text, short row)
{
    pico_display_center_text(row, text);
    pico_ui_draw_separator(2, row + 1, 1);
    pico_display_flush();
    return;
}

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

void pico_ui_draw_descale_rinse_preparation(void)
{
    pico_ui_draw_header_at_row("DESCALE", 0);
#if PICO_STANDALONE
    pico_display_center_text(2, "Use distilled water");
    pico_display_center_text(3, "in the reservoir.");
    pico_display_center_text(5, "Run Descale when");
    pico_display_center_text(6, "maintenance is due.");
#else
    pico_display_center_text(2, "If you ONLY use");
    pico_display_center_text(3, "distilled water in");
    pico_display_center_text(4, "reservoir please ");
    pico_display_center_text(5, "update setting on");
    pico_display_center_text(6, "picobrew.com");
#endif
    return;
}

void pico_ui_draw_subheader(const char *text)
{
    pico_ui_draw_header_at_row(text, 1);
    return;
}

void pico_ui_draw_heater_malfunction(void)
{
    pico_ui_draw_subheader("HEATER MALFUNCTION");
    pico_display_center_text(3, "ERROR CODE 24");
    pico_display_center_text(4, "UNPLUG PICO");
    pico_display_center_text(5, "IMMEDIATELY AND");
#if PICO_STANDALONE
    pico_display_center_text(6, "CHECK HEATER");
    pico_display_center_text(7, "BEFORE RESTART");
#else
    pico_display_center_text(6, "EMAIL SUPPORT");
    pico_display_center_text(7, "info@picobrew.com");
#endif
    pico_display_flush();
    return;
}

void pico_ui_draw_descale_heading(void)
{
    pico_ui_draw_subheader("DESCALE");
    return;
}

void pico_ui_draw_language_menu(void)
{
    char *source;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "CONFIGURE EL IDIOMA";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        source = "CHOISIR LA LANGUE";
    } else {
        source = "SET LANGUAGE";
    }
    strcpy(label_text, source);
    pico_ui_draw_subheader(label_text);
    return;
}

uint32_t pico_ui_draw_units_menu(void)
{
    char *source;
    char label_text[32];

    pico_display_clear_row(1);
    if (pico_ui_active->network->settings->language == '\x01') {
        strcpy(label_text, "DEFINIR UNIDADES");
        strcpy(pico_ui_active->menu_labels[0], "Imperial");
        source = "Metrico";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        strcpy(label_text, "Choisir l\'unite");
        strcpy(pico_ui_active->menu_labels[0], "Imperial");
        source = "Metrique";
    } else {
        strcpy(label_text, "SET UNITS");
        strcpy(pico_ui_active->menu_labels[0], "Imperial");
        source = "Metric";
    }
    strcpy(pico_ui_active->menu_labels[1], source);
    pico_ui_draw_subheader(label_text);
    return 2;
}

void pico_ui_draw_drain_filter(void)
{
    char *text;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        strcpy(label_text, "FILTRO DEL DRENAJE");
        pico_display_center_text(3, "Bomba de Drenaje");
        text = "activa";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        strcpy(label_text, "VIDANGE DU FILTRE");
        pico_display_center_text(3, "Pompe de vidange");
        text = "active";
    } else {
        strcpy(label_text, "DRAIN STEP FILTER");
        text = "Drain Pump active";
    }
    pico_display_center_text(4, text);
    pico_ui_draw_subheader(label_text);
    return;
}

void pico_ui_draw_drain_reservoir(void)
{
    char *text;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        strcpy(label_text, "DRENE EL RESERVORIO");
        pico_display_center_text(3, "Haga clic");
        text = "cuando termine";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        strcpy(label_text, "VIDANGER LE RESERVOIR");
        pico_display_center_text(3, "Cliquez lorsque");
        text = "termine";
    } else {
        strcpy(label_text, "DRAIN RESERVOIR");
        pico_display_center_text(3, "Shuttle Pump active");
        text = "Click when done";
    }
    pico_display_center_text(5, text);
    pico_ui_draw_subheader(label_text);
    return;
}

void pico_ui_draw_choice_question(void)
{
    char *text;
    char question_text[32];

    pico_display_clear();
    pico_label_continue(question_text);
    strcat(question_text, pico_ui_active->app->session.name);
    strcat(question_text, "?");
    if (pico_ui_active->network->settings->language == '\x01') {
        text = "Desea";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "Voulez-vous";
    } else {
        text = "Would you like to";
    }
    pico_display_center_text(3, text);
    pico_ui_draw_subheader(pico_ui_active->app->session.interrupted_text);
    pico_display_center_text(4, question_text);
    return;
}

void pico_ui_draw_first_rinse_heading(void)
{
    char *source;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "PRIMER LAVADO";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        source = "PREMIER RINCER";
    } else {
        source = "FIRST RINSE";
    }
    strcpy(label_text, source);
    pico_ui_draw_subheader(label_text);
    return;
}

void pico_ui_draw_choose_network(void)
{
    char *source;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "ELIJA LA RED";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        source = "CHOISIR UN RESEAU";
    } else {
        source = "CHOOSE NETWORK";
    }
    strcpy(label_text, source);
    pico_ui_draw_subheader(label_text);
    return;
}

void pico_ui_draw_wifi_test_heading(void)
{
    char *text;

    if (pico_ui_active->network->settings->language == '\x01') {
        text = "VERIFIQUE EL WIFI";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "TEST SANS FIL";
    } else {
        text = "TEST WIFI";
    }
    pico_ui_draw_subheader(text);
    return;
}

void pico_ui_draw_utilities_heading(void)
{
    char *source;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "HERRAMIENTAS";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        source = "SERVICES";
    } else {
        source = "UTILITIES";
    }
    strcpy(label_text, source);
    pico_ui_draw_subheader(label_text);
    return;
}

void pico_ui_draw_settings_heading(void)
{
    char *source;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "CONFIGURACION";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        source = "PARAMETRES";
    } else {
        source = "SETTINGS";
    }
    strcpy(label_text, source);
    pico_ui_draw_subheader(label_text);
    return;
}

void pico_ui_draw_current_program_heading(void)
{
    char label_text[32];

    pico_label_service(label_text);
    pico_ui_draw_subheader(label_text);
    return;
}

void pico_ui_draw_network_heading(void)
{
    char *source;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "RED";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        source = "RESEAU";
    } else {
        source = "NETWORK";
    }
    strcpy(label_text, source);
    pico_ui_draw_subheader(label_text);
    return;
}

void pico_ui_draw_factory_reset_heading(void)
{
    char *text;

    if (pico_ui_active->network->settings->language == '\x01') {
        text = "REINICIO DE LA PLANTA";
    } else {
        if (pico_ui_active->network->settings->language == '\x02') {
            pico_ui_draw_header_at_row("REINITIALISATION", 0);
            return;
        }
        text = "FACTORY RESET";
    }
    pico_ui_draw_subheader(text);
    return;
}

void pico_ui_draw_firmware_reset_question(void)
{
    short row;
    char *text;

    pico_ui_draw_factory_reset_heading();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(3, "Reiniciar Pico");
        pico_display_center_text(4, "al firmware de");
        row = 5;
        text = "la Planta?";
    } else if (pico_ui_active->network->settings->language == '\x02') {
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

void pico_ui_draw_clear_wifi_question(void)
{
    short row;
    char *text;

    pico_ui_draw_factory_reset_heading();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(3, "Retirar las");
        pico_display_center_text(4, "credenciales de");
        row = 5;
        text = "Wi-Fi almacenadas?";
    } else if (pico_ui_active->network->settings->language == '\x02') {
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

void pico_ui_draw_startup_heading(void)
{
    pico_ui_draw_header_at_row(pico_ui_active->app->recipe.program->title, 0);
    return;
}

void pico_ui_draw_deep_clean_heading(void)
{
    char *source;
    char label_text[32];

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "LIMPIEZA PROFUNDA";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        source = "NETTOYAGE";
    } else {
        source = "DEEP CLEAN";
    }
    strcpy(label_text, source);
    pico_ui_draw_header_at_row(label_text, 0);
    return;
}

void pico_ui_draw_instruction_with_header(uint32_t screen_id, uint32_t language, const char *title)
{
    pico_ui_draw_instruction_screen(screen_id, language, title, 0);
    return;
}

void pico_ui_draw_instruction_with_context(uint32_t screen_id, uint32_t language, const char *title)
{
    pico_ui_draw_instruction_screen(screen_id, language, title, 1);
    return;
}

void pico_ui_draw_password_heading(void)
{
    char *text;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        text = "CONTRASENA";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "MOT DE PASSE";
    } else {
        text = "ENTER PASSWORD";
    }
    pico_ui_draw_header_at_row(text, 0);
    return;
}

void pico_ui_draw_rack_beer_heading(void)
{
    char *text;

    if (pico_ui_active->network->settings->language == '\x01') {
        text = "EMBARRILAR CERVEZA";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "ENFUTAGE DE LA BIERE";
    } else {
        text = "RACK BEER";
    }
    pico_display_center_text(0, text);
    pico_ui_draw_separator(2, 1, 1);
    return;
}

void pico_ui_draw_stop_confirmation(void)
{
    char *text;
    char label_text[24];

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(1, "Esta seguro");
        text = "para detener:";
    } else {
        if (pico_ui_active->network->settings->language != '\x02') {
            pico_display_center_text(1, "Are you sure you");
            pico_display_center_text(2, "want to");
            strcpy(label_text, "stop ");
            goto append_program_label;
        }
        pico_display_center_text(1, "Etes-vous certain");
        text = "de vouloir arreter";
    }
    pico_display_center_text(2, text);
    label_text[0] = '\0';
append_program_label:
    pico_append_program_state_label(label_text);
    strcat(label_text, "?");
    pico_display_center_text(3, label_text);
    pico_ui_draw_separator(2, 4, 1);
    return;
}

void pico_ui_draw_skip_step_confirmation(void)
{
    char *text;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(1, "Esta seguro de que");
        pico_display_center_text(2, "desea saltarse");
        text = "el paso actual?";
    } else if (pico_ui_active->network->settings->language == '\x02') {
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

void pico_ui_draw_error(int error_code)
{
    pico_display_clear();
    unsigned selected_language = pico_ui_active->network->settings->language;
    pico_display_center_text(3, selected_language == 1   ? "Introduzca el codigo"
                                : selected_language == 2 ? "Inscrire le code"
                                                         : "Enter Code");
    unsigned language = pico_ui_active->network->settings->language;
    if (language > 2)
        language = 0;
    unsigned index =
        error_code == 99 ? 25 : (error_code >= 0 && error_code < 25 ? (unsigned)error_code : 0);
    const char *first = pico_ui_error_text[index][language][0],
               *second = pico_ui_error_text[index][language][1];
    pico_display_center_text(0, first);
    pico_display_center_text(1, second);
    pico_graphics_text_scale(&pico_ui_active->display, 2);
    char number[16];
    ui_itoa(error_code, number, 10);
    pico_display_center_text(4, number);
    pico_graphics_text_scale(&pico_ui_active->display, 1);
#if PICO_STANDALONE
    pico_display_center_text(6, "Record error code");
#else
    pico_display_center_text(6, "at picobrew.com/err");
#endif
    pico_ui_draw_separator(3, *second ? 2 : 1, 2);
}

void pico_ui_draw_insert_picopak(void)
{
    char *label;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Por favor inserte";
    } else {
        if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Veillez inserer");
            label = "associer le PicoPak";
            goto draw_pack_name;
        }
        label = "Please insert";
    }
    pico_display_center_text(3, label);
    label = "ERROR DE PicoPak";
draw_pack_name:
    pico_display_center_text(4, label + 9);
    if (pico_ui_active->network->settings->language == '\x01') {
        label = "CERVEZA";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "BRASSER";
    } else {
        label = "BREW";
    }
    pico_display_center_text(1, label);
    pico_ui_draw_separator(3, 2, 2);
    return;
}

void pico_ui_draw_abv_adjustment(void)
{
    char *text;

    if (pico_ui_active->network->settings->language == '\x01') {
        text = "Ajuste el ABV";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "AJUSTEZ LE ABV";
    } else {
        text = "ADJUST ABV";
    }
    pico_display_center_text(1, text);
    pico_ui_draw_separator(3, 2, 2);
    pico_display_text_at_cell(6, 0, " Min            Max  ");
    return;
}

void pico_ui_draw_bitterness_adjustment(void)
{
    char *text;

    if (pico_ui_active->network->settings->language == '\x01') {
        text = "AJUSTAR DEL AMARGURA";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "AJUSTER L\'AMERTUME";
    } else {
        text = "ADJUST BITTERNESS";
    }
    pico_display_center_text(1, text);
    pico_ui_draw_separator(1, 2, 2);
    pico_display_text_at_cell(6, 0, " Min            Max  ");
    return;
}

void pico_ui_draw_brew_complete(const char *recipe_title)
{
    short last_text_row;
    char *last_instruction_line;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(2, "CERVEZA FINALIZADA");
        pico_display_center_text(4, "barril fria para 20C");
        pico_display_center_text(5, "(68F),fermentar la");
        last_text_row = 6;
        last_instruction_line = "levadura add";
    } else if (pico_ui_active->network->settings->language == '\x02') {
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

int pico_ui_draw_recipe_choices(const char *recipe_title, int allow_adjustments)
{
    int choice_count;

    pico_label_start_brewing(pico_ui_active->menu_labels[0]);
    if (allow_adjustments == 0) {
        choice_count = 1;
    } else {
        pico_label_adjust_recipe(pico_ui_active->menu_labels[1]);
        choice_count = 2;
    }
    pico_label_cancel(pico_ui_active->menu_labels[0] + choice_count * 0x16);
    pico_display_center_text(1, recipe_title);
    pico_ui_draw_separator(3, 2, 2);
    return choice_count + 1;
}

void pico_ui_draw_picopak_selection(void)
{
    char *text;

    if (pico_ui_active->network->settings->language == '\x01') {
        text = "Seleccione PicoPak";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "Choisir le PicoPak";
    } else {
        text = "Select PicoPak";
    }
    pico_display_center_text(1, text);
    pico_ui_draw_separator(3, 2, 2);
    return;
}

void pico_ui_draw_registration_code(const char *registration_code)
{
#if PICO_STANDALONE
    (void)registration_code;
    pico_display_clear();
    pico_display_center_text(1, "LOCAL CONTROL");
    pico_display_center_text(3, "Connect to WiFi");
    pico_display_center_text(5, "Open Pico's local IP");
    pico_display_center_text(7, "in your browser");
#else
    char *text;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(1, "REGISTRE PICO");
        text = "Introduzca el codigo";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(1, "ENREGISTRER LE PICO");
        text = "Inscrire le code";
    } else {
        pico_display_center_text(1, "REGISTER PICO");
        text = "Enter code";
    }
    pico_display_center_text(3, text);
    pico_display_center_text(7, "picobrew.com/newpico");
    pico_ui_draw_separator(3, 2, 2);
    pico_graphics_text_scale(&pico_ui_active->display, '\x02');
    pico_display_center_text(4, registration_code);
    pico_graphics_text_scale(&pico_ui_active->display, '\x01');
#endif
    return;
}

void pico_ui_show_wifi_test_result(uint32_t diagnostic_stage)
{
    short text_row;
    char *message;

    pico_display_clear();
    pico_ui_draw_wifi_test_heading();
    switch (diagnostic_stage) {
    case 0:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Comprobacion de");
            text_row = 4;
            message = "conexi\303\263n Wifi";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Verification de");
            message = "la connexion Wifi";
            text_row = 4;
        } else {
            pico_display_center_text(3, "Checking WiFi");
            text_row = 4;
            message = "Connection";
        }
        break;
    case 1:
#if PICO_STANDALONE
        pico_display_center_text(3, "CHECK LOCAL NETWORK");
        text_row = 5;
        message = "No cloud required";
#else
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "NO SE PUEDE ");
            pico_display_center_text(4, "RESOLVER DNS");
            text_row = 5;
            message = "de picobrew.com";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Ne peut resoudre");
            pico_display_center_text(4, "le DNS de");
            message = "picobrew.com";
            text_row = 5;
        } else {
            pico_display_center_text(3, "CANNOT RESOLVE DNS");
            text_row = 4;
            message = "of picobrew.com";
        }
#endif
        goto draw_result_and_wait;
    case 2:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "NO HAY IP LOCAL,");
            pico_display_center_text(4, "VERIFICACION DE LA");
            text_row = 5;
            message = "DHCP EN EL ROUTER";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "AUCUN IP LOCAL");
            pico_display_center_text(4, "V\303\211RIFIER PARAMETRES");
            text_row = 5;
            message = "DHCP DU ROUTEUR";
        } else {
            pico_display_center_text(3, "NO LOCAL IP CHECK");
            pico_display_center_text(4, "DHCP SETTINGS ON");
            text_row = 5;
            message = "ROUTER";
        }
    draw_result_and_wait:
        pico_display_center_text(text_row, message);
        pico_ui_wait_for_continue();
        return;
    case 3:
        text_row = 3;
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Servidor que");
            text_row = 4;
            message = "produzca Ping";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            message = "Ping au serveur";
        } else {
            message = "Pinging server";
        }
        break;
    case 4:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Revise la conexion");
            message = "a los servicios web";
        } else {
            if (pico_ui_active->network->settings->language != '\x02') {
                pico_display_center_text(3, "Checking connection");
                pico_display_center_text(4, "to PicoBrew");
                text_row = 5;
                message = "web services";
                break;
            }
            pico_display_center_text(3, "Verifier la connexion");
            message = "aux services web";
        }
        pico_display_center_text(4, message);
        text_row = 5;
        message = "de Picobrew";
        break;
    case 5:
        pico_display_center_text(3, pico_ui_active->network->settings->wifi_ssid);
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(4, "Internet: SI");
            message = "PicoBrew: SI";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(4, "Internet: OUI");
            message = "PicoBrew: OUI";
        } else {
            pico_display_center_text(4, "Internet: YES");
            message = "PicoBrew: YES";
        }
        pico_display_center_text(5, message);
        return;
    default:
        return;
    }
    pico_display_center_text(text_row, message);
    pico_display_flush();
    return;
}

bool pico_title_id_is_valid(uint32_t title_id)
{
    return title_id < 6;
}
