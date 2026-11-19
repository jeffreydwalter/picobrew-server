#include "ui_internal.h"
#include "picobrew/config.h"

void pico_screen_error_heading(void)
{
    pico_graphics_text_scale(&pico_ui_active->display, '\x02');
    pico_display_center_text(2, "ERROR");
    pico_graphics_text_scale(&pico_ui_active->display, '\x01');
    pico_display_center_text(5, "<Silence Alarm>");
    return;
}

void pico_screen_water_error_help(void)
{
    short last_row;
    char *last_line;

    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(1, "ERROR: Llene con agua");
        pico_display_center_text(2, "el reservorio si esta");
        pico_display_center_text(3, "vacio, de lo");
        pico_display_center_text(4, "contrario seleccione");
        pico_display_center_text(5, "salida y contacte");
        last_row = 6;
        last_line = "a soporte";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(1, "ERREUR: Remplir le ");
        pico_display_center_text(2, "reservoir d\'eau s\'il");
        pico_display_center_text(3, "est vide, autrement ");
        pico_display_center_text(4, "choisir sortez et ");
        pico_display_center_text(5, "contact le soutien");
        last_line = "technique";
        last_row = 6;
    } else {
        pico_display_center_text(1, "ERROR: Fill water");
        pico_display_center_text(2, "reservoir if empty,");
        pico_display_center_text(3, "otherwise select");
        pico_display_center_text(4, "exit and contact");
        last_row = 5;
        last_line = "support";
    }
    pico_display_center_text(last_row, last_line);
    return;
}

void pico_screen_still_connecting(const char *status_text)
{
    pico_display_clear();
    pico_display_center_text(2, status_text);
    pico_display_flush();
    pico_ui_active->app->control_signals.progress_row = 3;
    pico_ui_active->app->control_signals.progress_start_column = 5;
    pico_ui_active->app->control_signals.progress_end_column = 0x10;
    pico_ui_active->app->control_signals.progress_glyph_index = 0;
    pico_ui_active->app->control_signals.progress_active = 1;
    return;
}

void pico_screen_still_exiting(void)
{
    pico_display_clear();
    pico_display_center_text(2, "Exiting Still");
    pico_ui_active->app->control_signals.progress_row = 3;
    pico_ui_active->app->control_signals.progress_start_column = 5;
    pico_ui_active->app->control_signals.progress_end_column = 0x10;
    pico_ui_active->app->control_signals.progress_glyph_index = 0;
    pico_ui_active->app->control_signals.progress_active = 1;
    return;
}

void pico_screen_wifi_searching(void)
{
    char *last_line;

    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_string_at_cell(2, 10, "la busqueda");
        pico_display_string_at_cell(3, 10, "de  redes");
        last_line = "WiFi";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_string_at_cell(2, 10, "Recherche");
        pico_display_string_at_cell(3, 10, "de reseaux");
        last_line = "sans fil";
    } else {
        pico_display_string_at_cell(2, 10, "Searching");
        pico_display_string_at_cell(3, 10, "for WiFi");
        last_line = "networks";
    }
    pico_display_string_at_cell(4, 10, last_line);
    return;
}

void pico_screen_ending_process(void)
{
    const char *message;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        message = "Fin del proceso";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        message = "Fin du processus";
    } else {
        message = "Ending Process";
    }
    pico_display_center_text(2, message);
    pico_display_flush();
    return;
}

void pico_screen_preheating_steam(short row)
{
    char *temperature_line;

    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(row, "Precalentamiento");
        if (pico_ui_active->network->settings->temperature_unit_celsius == '\0') {
            temperature_line = "vapor - 210F";
        } else {
            temperature_line = "vapor - 99C";
        }
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(row, "Prechauffage");
        if (pico_ui_active->network->settings->temperature_unit_celsius == '\0') {
            temperature_line = "vapeur - 210F";
        } else {
            temperature_line = "vapeur - 99C";
        }
    } else {
        pico_display_center_text(row, "Preheating");
        if (pico_ui_active->network->settings->temperature_unit_celsius == '\0') {
            temperature_line = "steam - 210F";
        } else {
            temperature_line = "steam - 99C";
        }
    }
    pico_display_center_text(row + 1, temperature_line);
    return;
}

bool pico_screen_loading(int is_picopak)
{
    bool int_master_enable_result;
    char *loading_label;
    char message[24];

    if (pico_ui_active->network->settings->language == '\x01') {
        loading_label = "Cargando";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        loading_label = "Remplissage";
    } else {
        loading_label = "Loading";
    }
    strcpy(message, loading_label);
    if (is_picopak != 0) {
        strcat(message, " PicoPak");
    }
    ui_interrupts_disable();
    pico_display_center_text(3, message);
    pico_display_flush();
    pico_ui_active->app->control_signals.progress_row = 4;
    pico_ui_active->app->control_signals.progress_start_column = 2;
    pico_ui_active->app->control_signals.progress_end_column = 0x13;
    pico_ui_active->app->control_signals.progress_glyph_index = 0;
    pico_ui_active->app->control_signals.progress_active = 1;
    int_master_enable_result = ui_interrupts_enable();
    return int_master_enable_result;
}

void pico_screen_updating_firmware(void)
{
    short last_row;
    char *last_line;

    last_row = 4;
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(4, "Actualizacion");
        last_row = 5;
        last_line = "del firmware";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(4, "Mise a jour du");
        last_line = "micrologiciel";
        last_row = 5;
    } else {
        last_line = "Updating firmware";
    }
    pico_display_center_text(last_row, last_line);
    return;
}

void pico_screen_downloading_firmware(void)
{
    short last_row;
    char *last_line;

    last_row = 4;
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(4, "Cargando nuevo");
        last_row = 5;
        last_line = "firmware";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(4, "Telechargement du");
        last_line = "nouveau micrologiciel";
        last_row = 5;
    } else {
        last_line = "Loading new firmware";
    }
    pico_display_center_text(last_row, last_line);
    return;
}

void pico_screen_firmware_update_failed(void)
{
    char *last_line;

    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(4, "Actualizacion");
        last_line = "fallida del firmware";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(4, "Echec de la mise a");
        last_line = "jour du micrologiciel";
    } else {
        pico_display_center_text(4, "Updating firmware");
        last_line = "failed";
    }
    pico_display_center_text(5, last_line);
    return;
}

void pico_screen_rinse_complete(void)
{
    short last_row;
    char *last_line;

    last_row = 3;
    if (pico_ui_active->network->settings->language == '\x01') {
        last_line = "Lave completamente";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(3, "Rincez completement");
        last_line = "Bon brassage Pico!";
        last_row = 4;
    } else {
        pico_display_center_text(3, "Rinse complete");
        last_row = 4;
        last_line = "Happy Pico brewing!";
    }
    pico_display_center_text(last_row, last_line);
    return;
}

void pico_screen_help_address(void)
{
    short url_row;
    char *text;

    if (pico_ui_active->network->settings->language == '\x01') {
        text = "Contacte";
    } else {
        if (pico_ui_active->network->settings->language != '\x02') {
            pico_display_center_text(4, "Go to");
            url_row = 5;
            goto draw_help_url;
        }
        text = "Consultez";
    }
    pico_display_center_text(3, text);
    url_row = 4;
draw_help_url:
#if PICO_STANDALONE
    pico_display_center_text(url_row, "Open Pico's local IP");
#else
    pico_display_center_text(url_row, "picobrew.com/help");
#endif
    pico_display_flush();
    return;
}

void pico_screen_show_help_temporarily(void)
{
    pico_display_clear();
    pico_screen_help_address();
    pico_display_flush();
    ui_delay(5000);
    return;
}

void pico_screen_ending_first_rinse(void)
{
    const char *message;

    pico_display_clear();
    pico_display_logo();
    if (pico_ui_active->network->settings->language == '\x01') {
        message = "Finalizacion lavado";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        message = "Fin du nettoyage";
    } else {
        message = "Ending First-Rinse";
    }
    pico_display_center_text(4, message);
    pico_display_flush();
    return;
}

void pico_screen_still_setup_step(uint8_t *setup_step)
{
    char *last_line;

    pico_display_clear();
    pico_display_draw_program_frame();
    switch (*setup_step) {
    case 0:
        pico_display_center_text(2, "Add 2 inches");
        pico_display_center_text(3, "of water into");
        last_line = "the step filter";
        break;
    case 1:
        pico_display_center_text(2, "Insert the");
        pico_display_center_text(3, "Step Filter and");
        last_line = "Lid into Pico";
        break;
    case 2:
        pico_display_center_text(2, "Attach the IN");
        pico_display_center_text(3, "and OUT hoses");
        last_line = "to PicoStill";
        break;
    case 3:
        pico_display_center_text(2, "Fill water");
        pico_display_center_text(3, "reservoir if");
        last_line = "empty";
        break;
    default:
        return;
    }
    pico_display_center_text(4, last_line);
    return;
}

void pico_screen_draw_program_notice(void)
{
    const char *title;

    pico_display_clear();
    title = pico_title_label(3, (uint32_t)pico_ui_active->network->settings->language);
    pico_display_draw_titled_frame(title);
    return;
}

void pico_screen_show_program_notice(void)
{
    const char *message;

    pico_screen_draw_program_notice();
    message = pico_title_label(4, (uint32_t)pico_ui_active->network->settings->language);
    pico_display_center_text(3, message);
    pico_display_flush();
    return;
}

void pico_screen_reinsert_step_filter(void)
{
    char *last_line;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_draw_titled_frame("ERROR");
        pico_display_center_text(3, "Reintroduzca el");
        last_line = "filtro de paso";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_draw_titled_frame("ERREUR");
        pico_display_center_text(3, "Reinserez");
        last_line = "le filtre";
    } else {
        pico_display_draw_titled_frame("ERROR");
        pico_display_center_text(3, "Reinsert");
        last_line = "StepFilter";
    }
    pico_display_center_text(4, last_line);
    return;
}

void pico_screen_changing_step(void)
{
    const char *message;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        message = "Cambiar paso";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        message = "Changement d\'etape";
    } else {
        message = "Changing step";
    }
    pico_display_center_text(3, message);
    pico_display_draw_titled_frame(pico_ui_active->app->screen.recipe_name);
    return;
}

void pico_screen_setting_up_step(const char *title)
{
    const char *message;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        message = "Configurar paso";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        message = "Mettre en place";
    } else {
        message = "Setting up step";
    }
    pico_display_center_text(3, message);
    pico_display_draw_titled_frame(title);
    return;
}

void pico_screen_ending_step(const char *title)
{
    const char *message;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        message = "Finalizar paso";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        message = "Terminer l\'etape";
    } else {
        message = "Ending step";
    }
    pico_display_center_text(3, message);
    pico_display_draw_titled_frame(title);
    return;
}

uint32_t pico_screen_fill_reservoir_and_start(const char *title)
{
    char *localized_line;

    pico_display_clear();
    pico_display_draw_titled_frame(title);
    if (pico_ui_active->network->settings->language == '\x01') {
        localized_line = "Inicio ";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        localized_line = "Demarrer ";
    } else {
        localized_line = "Start ";
    }
    strcpy(pico_ui_active->menu_labels[0], localized_line);
    pico_append_program_name(pico_ui_active->menu_labels[0]);
    pico_label_exit(pico_ui_active->menu_labels[1]);
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(3, "Llenar el reservorio");
        localized_line = "con agua destilada";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(3, "Remplissez reservoir");
        localized_line = "d\'eau distillee";
    } else {
        pico_display_center_text(3, "Fill Reservoir with");
        localized_line = "distilled water";
    }
    pico_display_center_text(4, localized_line);
    return 2;
}

void pico_screen_starting_program(const char *title)
{
    char *starting_prefix;
    char message[24];

    pico_display_clear();
    pico_display_draw_titled_frame(title);
    if (pico_ui_active->network->settings->language == '\x01') {
        starting_prefix = "Iniciar ";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        starting_prefix = "Demarrer ";
    } else {
        starting_prefix = "Starting ";
    }
    strcpy(message, starting_prefix);
    pico_append_program_name(message);
    pico_display_center_text(3, message);
    return;
}

void pico_screen_ending_program(const char *title)
{
    char *ending_prefix;
    char message[24];

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        ending_prefix = "Finalizar ";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        ending_prefix = "Terminer ";
    } else {
        ending_prefix = "Ending ";
    }
    strcpy(message, ending_prefix);
    pico_append_program_name(message);
    pico_display_center_message(message);
    pico_display_draw_titled_frame(title);
    return;
}

void pico_screen_show_ending_program(const char *title)
{
    pico_display_clear();
    pico_screen_ending_program(title);
    pico_display_flush();
    return;
}

bool pico_screen_loading_picopak(void)
{
    bool screen_loading_result;

    screen_loading_result = pico_screen_loading(1);
    return screen_loading_result;
}

void pico_screen_splash(void)
{
    uint32_t version_length;
    char message_text[32];

    pico_graphics_bitmap(&pico_ui_active->display, 0, 0x10, pico_ui_bitmap_splash, 0x80, 0x1e, 1);
    pico_display_clear_row(7);
    pico_copy_firmware_version(message_text);
    version_length = strlen(message_text);
    pico_display_text_at_cell(7,
                              (uint16_t)(0x15 - version_length) &
                                  ~(uint16_t)((int)(0x15 - version_length) >> 0x1f) & 0xff,
                              message_text);
    pico_display_flush();
    return;
}

void pico_screen_service_status(void)
{
    char service_label[32];
    pico_label_service(service_label);
    pico_display_clear_row(6);
    pico_display_center_text(6, service_label);
}

void pico_screen_wifi_no_signal(void)
{
    const char *message;
    char message_text[32];

    pico_display_clear_row(6);
    if (pico_ui_active->network->settings->language == '\x01') {
        message = "No hay se\244al WiFi";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        message = "Pas de signal WiFi";
    } else {
        message = "No WiFi signal found";
    }
    strcpy(message_text, message);
    pico_display_text_at_cell(6, 0, message_text);
    return;
}

void pico_screen_wifi_connected(void)
{
    const char *message;

    pico_display_clear_row(6);
    if (pico_ui_active->network->settings->language == '\x01') {
        message = "Conectado";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        message = "Connecte";
    } else {
        message = "Connected";
    }
    pico_display_center_text(6, message);
    return;
}

void pico_screen_wifi_connection_failed(void)
{
    char translated_message[28];
    const char *message;
    pico_display_clear_row(6);
    if (pico_ui_active->network->settings->language == 1) {
        strcpy(translated_message, "Conexi\xa2n fallida");
        message = translated_message;
    } else if (pico_ui_active->network->settings->language == 2) {
        message = "Echec de la connexion";
    } else {
        message = "Connection failed";
    }
    pico_display_center_text(6, message);
}
