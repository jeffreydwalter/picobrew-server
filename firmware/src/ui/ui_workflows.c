#include "ui_internal.h"
#include "picobrew/local_control.h"

void pico_ui_wait_for_resume(void)
{
    int encoder_take_click_result;

    do {
        while (encoder_take_click_result =
                   pico_encoder_take_click((PicoEncoder *)&pico_ui_active->encoder),
               encoder_take_click_result == 0) {
            if (pico_local_cancelled())
                return;
            pico_ui_draw_paused();
        }
        encoder_take_click_result = pico_filter_is_inserted(pico_ui_active->actuators);
    } while (!pico_local_cancelled() && encoder_take_click_result == 0);
    return;
}

void pico_ui_wait_for_footer_press(const char *text)
{
    int encoder_take_click_result;

    pico_ui_draw_footer_single(text);
    pico_display_flush();
    pico_encoder_clear_click((PicoEncoder *)&pico_ui_active->encoder);
    do {
        encoder_take_click_result =
            pico_encoder_take_click((PicoEncoder *)&pico_ui_active->encoder);
    } while (!pico_local_cancelled() && encoder_take_click_result == 0);
    return;
}

void pico_ui_prompt_deep_clean_setup(void)
{
    char *text;

    pico_ui_draw_deep_clean_heading();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(2, "Agregue agua al");
        pico_display_center_text(3, "barril y al");
        pico_display_center_text(4, "reservorio y luego");
        pico_display_center_text(5, "agregue la tableta");
        pico_display_center_text(6, "de limpieza al barril");
        text = "Comience a limpiar";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(2, "Ajoutez de l\'eau dans");
        pico_display_center_text(3, "le fut et le");
        pico_display_center_text(4, "reservoir et ajoutez");
        pico_display_center_text(5, "une pastille nettoy-");
        pico_display_center_text(6, "ante dans le fut");
        text = "Commencer";
    } else {
        pico_display_center_text(2, "Add water to keg");
        pico_display_center_text(3, "and reservoir");
        pico_display_center_text(4, "then add cleaning");
        pico_display_center_text(5, "tab to keg");
        text = "Start Clean";
    }
    pico_ui_wait_for_footer_press(text);
    return;
}

void pico_ui_wait_for_filter_insertion(void)
{
    int step_filter_is_inserted_result;

    step_filter_is_inserted_result = pico_filter_is_inserted(pico_ui_active->actuators);
    if (step_filter_is_inserted_result == 0) {
        pico_display_clear();
        pico_ui_draw_instruction_with_context(1,
                                              (uint32_t)pico_ui_active->network->settings->language,
                                              pico_ui_active->app->session.title);
        pico_display_flush();
        pico_ui_active->services->wait_for_filter(pico_ui_active->service_context);
    }
    pico_display_clear();
    return;
}

void pico_ui_enter_wifi_password(void)
{
    pico_ui_draw_password_heading();
    pico_ui_active->network->settings->wifi_password[0] = 0;
    pico_encoder_edit_text(pico_ui_active->network->settings->wifi_password, 0x40);
    pico_display_clear();
    return;
}

uint32_t pico_ui_choose_language(void)
{
    uint32_t menu_populate_languages_result;

    menu_populate_languages_result = pico_menu_populate_languages();
    pico_display_clear();
    pico_ui_draw_language_menu();
    menu_populate_languages_result = pico_menu_choose(3, menu_populate_languages_result, 0);
    if (menu_populate_languages_result != 1) {
        if (menu_populate_languages_result == 0) {
            pico_ui_active->network->settings->language = 0;
            return 0;
        }
        if (menu_populate_languages_result != 2) {
            return menu_populate_languages_result;
        }
    }
    pico_ui_active->network->settings->language = (char)menu_populate_languages_result;
    return menu_populate_languages_result;
}

uint32_t pico_ui_choose_units(void)
{
    uint32_t ui_draw_units_menu_result;

    pico_display_clear();
    ui_draw_units_menu_result = pico_ui_draw_units_menu();
    ui_draw_units_menu_result = pico_menu_choose(3, ui_draw_units_menu_result, 0);
    if (pico_local_cancelled())
        return pico_ui_active->network->settings->temperature_unit_celsius;
    pico_ui_active->network->settings->temperature_unit_celsius = ui_draw_units_menu_result != 0;
    return (uint32_t)(ui_draw_units_menu_result != 0);
}

uint32_t pico_ui_choose_program(void)
{
    uint32_t menu_populate_still_actions_result;

    pico_display_clear();
    pico_ui_draw_subheader("CHOOSE A PROGRAM");
    menu_populate_still_actions_result = pico_menu_populate_still_actions(2);
    menu_populate_still_actions_result = pico_menu_choose(4, menu_populate_still_actions_result, 0);
    return menu_populate_still_actions_result;
}

void pico_ui_require_first_rinse(void)
{
    short row;
    char *text;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(0, "CERVEZA CANCELADA");
        pico_display_center_text(3, "Descartar el mosto  ");
        pico_display_center_text(4, "y correr el ciclo");
        row = 5;
        text = "de lavado";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(0, "BRASSAGE ANNULE");
        pico_display_center_text(3, "Se debarrasser du");
        pico_display_center_text(4, "mout et demarrer");
        text = "le cycle de rincage";
        row = 5;
    } else {
        pico_display_center_text(0, "BREW CANCELED");
        pico_display_center_text(3, "Discard wort and");
        row = 4;
        text = "run rinse cycle";
    }
    pico_display_center_text(row, text);
    pico_ui_draw_separator(2, 1, 1);
    pico_label_ok(pico_ui_active->menu_labels[0]);
    pico_menu_choose(7, 1, 0);
    return;
}

int pico_ui_prompt_vacuum(int stage)
{
    const char *localized_text;
    uint32_t selected_option;
    int continue_selected;
    char stop_label[28];

    pico_display_clear();
    localized_text = pico_title_label(2, (uint32_t)pico_ui_active->network->settings->language);
    pico_ui_draw_header_at_row(localized_text, 0);
    if (stage == 0) {
        if (pico_ui_active->network->settings->language == 1) {
            pico_display_center_text(2, "Utilice la vara");
            pico_display_center_text(3, "del barril NEGRA");
            pico_display_center_text(4, "para aspirar el");
            pico_display_center_text(5, "agua del");
            pico_display_center_text(6, "reservorio");
            localized_text = "Comience a aspirar";
        } else if (pico_ui_active->network->settings->language == 2) {
            pico_display_center_text(2, "Utilisez le baton");
            pico_display_center_text(3, "de fut NOIR pour ");
            pico_display_center_text(4, "aspirer l\'eau du");
            pico_display_center_text(5, "reservoir");
            localized_text = "Commencer la succion";
        } else {
            pico_display_center_text(3, "Use BLACK keg wand");
            pico_display_center_text(4, "to vacuum water");
            pico_display_center_text(5, "from reservoir");
            localized_text = "Start Vacuum";
        }
        strcpy(pico_ui_active->menu_labels[0], localized_text);
        pico_label_exit(pico_ui_active->menu_labels[1]);
        selected_option = pico_menu_choose(7, 2, 0);
        continue_selected = 1 - selected_option;
        if (1 < selected_option) {
            continue_selected = 0;
        }
    } else {
        if (stage == 1) {
            if (pico_ui_active->network->settings->language == 1) {
                pico_display_center_text(3, "Aspire el agua");
                pico_display_center_text(4, "del reservorio");
                localized_text = "hasta vaciarlo";
            } else if (pico_ui_active->network->settings->language == 2) {
                pico_display_center_text(3, "Aspirez l\'eau du");
                pico_display_center_text(4, "reservoir jusqu\'a ce");
                localized_text = "qu\'il soit vide";
            } else {
                pico_display_center_text(3, "Vacuum water from");
                pico_display_center_text(4, "reservoir until");
                localized_text = "empty";
            }
            pico_display_center_text(5, localized_text);
            pico_label_done(stop_label);
            pico_ui_draw_footer_single(stop_label);
            pico_display_flush();
            pico_encoder_clear_click((PicoEncoder *)&pico_ui_active->encoder);
        }
        continue_selected = 1;
    }
    return continue_selected;
}

uint32_t pico_ui_wait_for_continue(void)
{
    uint32_t menu_choose_result;

    pico_label_continue(pico_ui_active->menu_labels[0]);
    menu_choose_result = pico_menu_choose(7, 1, 0);
    return menu_choose_result;
}

void pico_prompt_insert_filter_with_alarm(void)
{
    ui_millis();
    pico_ui_active->services->buzzer(pico_ui_active->service_context, true);
    pico_screen_reinsert_step_filter();
    pico_display_flush();
    pico_ui_active->services->alarm_filter_missing(pico_ui_active->service_context);
    pico_display_clear();
    return;
}

uint32_t pico_run_drain_instructions(uint32_t second_screen_id)
{
    int drain_running;
    int selected_action;
    uint32_t result_action;
    uint32_t drain_menu_id;
    uint32_t screen_ids[2];

    screen_ids[0] = 14;
    screen_ids[1] = second_screen_id;
    selected_action =
        pico_ui_run_instruction_sequence(screen_ids, 2, 0, pico_ui_active->app->session.program_id,
                                         pico_ui_active->app->session.title, '\x01');
    if (selected_action == 0x17) {
        drain_running = pico_actuator_drain_is_on(pico_ui_active->actuators);
        if (!drain_running) {
            pico_actuator_drain_on(pico_ui_active->actuators);
        }
        while (true) {
            if (pico_local_cancelled())
                return 34;
            drain_running = pico_actuator_drain_is_on(pico_ui_active->actuators);
            if (drain_running) {
                drain_menu_id = 0x11;
            } else {
                drain_menu_id = 0x12;
            }
            drain_menu_id = pico_ui_active->services->execute_instruction(
                pico_ui_active->service_context, drain_menu_id, false);
            if (1 < drain_menu_id - 0x1f)
                break;
            drain_running = ui_read(8);
            selected_action = 1 - (uint32_t)drain_running;
            if (1 < drain_running) {
                selected_action = 0;
            }
#if PICO_STANDALONE
            if (selected_action)
                pico_actuator_drain_on(pico_ui_active->actuators);
            else
                pico_actuator_drain_off(pico_ui_active->actuators);
#else
            ui_write(8, selected_action);
#endif
        }
        pico_actuator_drain_off(pico_ui_active->actuators);
        result_action = 0x17;
    } else {
        ui_log("W", "RCTL", "Drain setup aborted by user");
        result_action = 0x22;
    }
    return result_action;
}

void pico_ui_manual_network_step(int stage)
{
    char *text;

    pico_display_clear();
    if (stage == 1) {
        pico_ui_draw_header_at_row("ENTER AP NAME", 0);
        if (pico_ui_active->network->settings->language == '\x01') {
            text = "NOMBRE DEL AP";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            text = "NOM DU PA";
        } else {
            text = "ENTER AP NAME";
        }
        pico_ui_draw_header_at_row(text, 0);
        return;
    }
    if (stage == 0) {
        if ((pico_ui_active->network->settings->language == '\x01') ||
            (pico_ui_active->network->settings->language != '\x02')) {
            pico_display_center_text(3, "Please Enter Name");
            text = "of AP, case matters";
        } else {
            pico_display_center_text(3, "NOM DU PA,");
            text = "sensible a la casse";
        }
        pico_display_center_text(4, text);
        pico_ui_wait_for_continue();
        return;
    }
    if (stage != 2) {
        return;
    }
    if (pico_ui_active->network->settings->language == '\x01') {
        text = "Seguridad?";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        text = "Securite?";
    } else {
        text = "Security Type?";
    }
    pico_display_center_text(2, text);
    strcpy(pico_ui_active->menu_labels[0], "WPA/WPA2");
    strcpy(pico_ui_active->menu_labels[1], "WEP");
    strcpy(pico_ui_active->menu_labels[2], "Unsecure");
    return;
}

void pico_ui_prompt_picopak_association(void)
{
#if PICO_STANDALONE
    pico_display_clear();
    pico_display_center_text(0, "LOCAL RECIPES");
    pico_display_center_text(2, "No recipes available");
    pico_display_center_text(4, "Add a recipe using");
    pico_display_center_text(5, "the local web app");
    pico_ui_wait_for_continue();
#else
    char *text;

    pico_display_clear();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(0, "ERROR DE PicoPak");
        pico_display_center_text(2, "Entra en la cuenta de");
        pico_display_center_text(3, "picobrew.com y");
        pico_display_center_text(4, "asocie el PicoPak");
        pico_display_center_text(5, "utilizando el numero");
        text = "de serie en la caja";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(0, "ERREUR de PicoPak");
        pico_display_center_text(2, "Ouvrir une session");
        pico_display_center_text(3, "a picobrew.com et");
        pico_display_center_text(4, "associer le PicoPak");
        pico_display_center_text(5, "utilisant le numero");
        text = "de serie sur la boite";
    } else {
        pico_display_center_text(0, "PicoPak ERROR");
        pico_display_center_text(2, "Please login to");
        pico_display_center_text(3, "picobrew.com and");
        pico_display_center_text(4, "associate the PicoPak");
        pico_display_center_text(5, "using the serial");
        text = "number on the box";
    }
    pico_display_center_text(6, text);
    pico_ui_draw_separator(2, 1, 1);
    pico_ui_wait_for_continue();
    return;
#endif
}

void pico_ui_show_rinse_finished(void)
{
    char *text;

    pico_ui_draw_startup_heading();
    if (pico_ui_active->network->settings->language == '\x01') {
        pico_display_center_text(3, "Finalizado!");
        pico_display_center_text(4, "Desconecte las");
        pico_display_center_text(5, "mangueras y deseche");
        text = "el aguaantes de lavar";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        pico_display_center_text(2, "Termine!");
        pico_display_center_text(3, "Debranchez les");
        pico_display_center_text(4, "tuyaux et eliminez");
        pico_display_center_text(5, "l\'eau souillee");
        text = "avant de nettoyer";
    } else {
        pico_display_center_text(3, "Finished!");
        pico_display_center_text(4, "Disconnect Hoses and");
        pico_display_center_text(5, "dump waste water");
        text = "before rinse";
    }
    pico_display_center_text(6, text);
    pico_ui_wait_for_continue();
    return;
}

void pico_ui_first_rinse_step(uint32_t stage)
{
    short row;
    char *text;

    pico_display_clear();
    pico_ui_draw_first_rinse_heading();
    switch (stage) {
    case 0:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Agregue agua al");
            pico_display_center_text(4, "barril y al");
            pico_display_center_text(5, "reservorio");
            text = "Comience a lavar";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Ajouter de l\'eau");
            pico_display_center_text(4, "dans le fut et");
            pico_display_center_text(5, "le reservoir");
            text = "Commencer le rincage";
        } else {
            pico_display_center_text(3, "Add water to keg");
            pico_display_center_text(4, "and reservoir");
            pico_display_center_text(5, "Click when ready");
            text = "Start Rinse";
        }
        strcpy(pico_ui_active->menu_labels[0], text);
        pico_menu_choose(7, 1, 0);
        return;
    case 1:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Por favor inserte");
            pico_display_center_text(4, "el Filtro de Paso");
            row = 5;
            text = "para comenzar";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Veillez inserer");
            pico_display_center_text(4, "le filtre pour");
            text = "commencer";
            row = 5;
        } else {
            pico_display_center_text(3, "Whoops, please");
            pico_display_center_text(4, "insert the Step");
            row = 5;
            text = "Filter to get started";
        }
        break;
    case 2:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Preparar el");
            row = 4;
            text = "generador de vapor";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Preparation du");
            row = 4;
            text = "generateur a vapeur";
        } else {
            pico_display_center_text(3, "Priming steam");
            row = 4;
            text = "generator";
        }
        break;
    case 3:
        row = 3;
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Preparar la bomba");
            row = 4;
            text = "de vapor";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Preparation de la");
            row = 4;
            text = "pompe a vapeur";
        } else {
            text = "Priming steam pump";
        }
        break;
    case 4:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Sistema de lavado");
            row = 4;
            text = "con vapor y agua";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Systeme a l\'eau de");
            row = 4;
            text = "rincage a la vapeur";
        } else {
            pico_display_center_text(3, "Steam flushing");
            row = 4;
            text = "water system";
        }
        break;
    case 5:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Sistema de lavado del");
            row = 4;
            text = "barril y drenaje";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Rincage du fut et");
            row = 4;
            text = "vidange du systeme";
        } else {
            pico_display_center_text(3, "Flushing keg and");
            row = 4;
            text = "drain system";
        }
        break;
    case 6:
        if (pico_ui_active->network->settings->language == '\x01') {
            text = "ERROR DE LA BOMBA";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            text = "ERREUR DE LA POMPE";
        } else {
            text = "SHUTTLE PUMP ERROR";
        }
        goto draw_failure_help;
    case 7:
        if (pico_ui_active->network->settings->language == '\x01') {
            text = "ERROR DEL CALENTADOR";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            text = "ERREUR DU RADIATEUR";
        } else {
            text = "HEATER ERROR";
        }
    draw_failure_help:
        pico_display_center_text(3, text);
        pico_screen_help_address();
        goto flush_screen;
    case 8:
        pico_screen_rinse_complete();
        goto flush_screen;
    default:
        return;
    }
    pico_display_center_text(row, text);
flush_screen:
    pico_display_flush();
    return;
}

void pico_ui_wait_for_stop(void)
{
    pico_label_done(pico_ui_active->menu_labels[0]);
    pico_menu_choose(7, 1, 0);
    return;
}

void pico_ui_rack_beer_step(uint32_t stage)
{
    short row;
    char *text;

    pico_display_clear();
    pico_ui_draw_rack_beer_heading();
    switch (stage) {
    case 0:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(2, "Conecte la vara del");
            pico_display_center_text(3, "barril a la esfera ");
            pico_display_center_text(4, "de seguro GRIS y");
            pico_display_center_text(5, "coloquela en el con-");
            text = "tenedor de desechos";
        spanish_flush_footer:
            pico_display_center_text(6, text);
            text = "Comience a limpiar";
            goto wait_for_start;
        }
        if (pico_ui_active->network->settings->language != '\x02') {
            pico_display_center_text(2, "Connect keg wand to");
            pico_display_center_text(3, "GRAY ball-lock, and");
            pico_display_center_text(4, "place in waste");
            pico_display_center_text(5, "container");
            text = "Start Flush";
            goto wait_for_start;
        }
        pico_display_center_text(2, "Brancher le baton de");
        pico_display_center_text(3, "fut au connecteur");
        pico_display_center_text(4, "a bille GRIS et");
        pico_display_center_text(5, "placez-le dans le");
        text = "contenant a dechets";
        goto french_flush_footer;
    case 1:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Manguera para limpiar");
            pico_display_center_text(4, "limpiar el drenaje.");
            pico_display_center_text(5, "Haga clic");
            row = 6;
            text = "cuando termine";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Rincage du tuyau ");
            pico_display_center_text(4, "de drainage.");
            pico_display_center_text(5, "Cliquez lorsque");
            text = "termine para limpiar";
            row = 6;
        } else {
            pico_display_center_text(3, "Flushing drain hose");
            row = 5;
            text = "Click when done";
        }
        pico_display_center_text(row, text);
        pico_display_flush();
        pico_ui_active->app->device_state = 5;
        goto report_process_state;
    case 2:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Retire la vara del");
            pico_display_center_text(4, "barril de la esfera");
            row = 5;
            text = "de seguro GRIS";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Enlevez le baton");
            pico_display_center_text(4, "fut du connecteur");
            text = "de a bille GRIS";
            row = 5;
        } else {
            pico_display_center_text(3, "Remove keg wand from");
            row = 4;
            text = "GRAY ball-lock";
        }
        break;
    case 3:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Conecte la esfera");
            pico_display_center_text(4, "de seguro GRIS al");
            pico_display_center_text(5, "conector de IN");
            row = 6;
            text = "del barril";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "branchez le connecteur");
            pico_display_center_text(4, "a bille GRIS sur le");
            pico_display_center_text(5, "raccord IN du fut ");
            row = 6;
            text = "baril";
        } else {
            pico_display_center_text(3, "Connect GRAY ball-");
            row = 4;
            text = "lock to keg IN post";
        }
        break;
    case 4:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Hale la valvula de");
            pico_display_center_text(4, "presion del barril");
            pico_display_center_text(5, "para liberar la ");
            row = 6;
            text = "presion";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(3, "Appuyez sur la");
            pico_display_center_text(4, "soupape de pression");
            pico_display_center_text(5, "du fut pour");
            row = 6;
            text = "depressuriser";
        } else {
            pico_display_center_text(3, "Pull keg pressure");
            pico_display_center_text(4, "valve to release");
            row = 5;
            text = "any pressure";
        }
        break;
    case 5:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Conecte el tubo de");
            pico_display_center_text(4, "trasiego de SALIDA");
            pico_display_center_text(5, "del barril y dir\303\255jalo");
            text = "al desechos";
            goto spanish_flush_footer;
        }
        if (pico_ui_active->network->settings->language != '\x02') {
            pico_display_center_text(2, "Connect racking tube");
            pico_display_center_text(3, "to keg OUT post,");
            pico_display_center_text(4, "and direct tube");
            pico_display_center_text(5, "to waste");
            pico_display_center_text(6, "container");
            text = "Start Flush";
            goto wait_for_start;
        }
        pico_display_center_text(3, "Branchez le tube");
        pico_display_center_text(4, "au raccord OUT");
        pico_display_center_text(5, "fut et placez tube");
        text = "le dechets";
    french_flush_footer:
        pico_display_center_text(6, text);
        text = "Demarrer le rincage";
    wait_for_start:
        pico_ui_wait_for_footer_press(text);
        return;
    case 6:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Deje correr hasta");
            pico_display_center_text(4, "que turbidez en el");
            row = 5;
            text = "tubo de trasiego";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(2, "Continuez jusqu\'a");
            pico_display_center_text(3, "la disparition");
            pico_display_center_text(4, "complete du mout");
            pico_display_center_text(5, "bouillant dans le");
            text = "tube de decantation";
            row = 6;
        } else {
            pico_display_center_text(3, "Run until no trub");
            pico_display_center_text(4, "is visible in");
            row = 5;
            text = "racking tube";
        }
        pico_display_center_text(row, text);
        goto wait_for_stop;
    case 7:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Introduzca el tubo de");
            pico_display_center_text(4, "trasiego en el barril");
            pico_display_center_text(5, "que se servira para");
            pico_display_center_text(6, "transferir la cerveza");
            text = "Iniciar proceso";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(2, "Inserez le tube de");
            pico_display_center_text(3, "decantation dans");
            pico_display_center_text(4, "le fut de service");
            pico_display_center_text(5, "pour transferer");
            pico_display_center_text(6, "la biere");
            text = "Demarrer l\'action";
        } else {
            pico_display_center_text(3, "Insert racking tube");
            pico_display_center_text(4, "in serving keg to");
            pico_display_center_text(5, "transfer beer");
            text = "Start Transfer";
        }
        goto wait_for_start;
    case 8:
        if (pico_ui_active->network->settings->language == '\x01') {
            pico_display_center_text(3, "Deje correr hasta");
            pico_display_center_text(4, "que se llene el");
            pico_display_center_text(5, "barril o entre");
            row = 6;
            text = "aire al tubo";
        } else if (pico_ui_active->network->settings->language == '\x02') {
            pico_display_center_text(2, "Continuez jusqu\'a ce");
            pico_display_center_text(3, "que le fut soit");
            pico_display_center_text(4, "plein et que l\'air");
            pico_display_center_text(5, "penetre dans le");
            text = "tube";
            row = 6;
        } else {
            pico_display_center_text(3, "Run until keg is");
            pico_display_center_text(4, "full or air");
            row = 5;
            text = "enters tube";
        }
        pico_display_center_text(row, text);
        pico_display_flush();
        pico_ui_active->app->device_state = 2;
    report_process_state:
        pico_api_report_state(pico_ui_active->network);
    wait_for_stop:
        pico_ui_wait_for_stop();
        return;
    default:
        return;
    }
    pico_display_center_text(row, text);
    pico_ui_wait_for_continue();
    return;
}

bool pico_ui_descale_step(uint32_t stage)
{
    bool ui_confirm_at_row_result;
    short row;
    char *text;

    pico_display_clear();
    pico_ui_draw_descale_heading();
    switch (stage) {
    case 0:
        pico_display_center_text(3, "Fill Reservoir, add");
        pico_display_center_text(4, "descaling soultion,");
        pico_display_center_text(5, "insert StepFilter");
        pico_display_flush();
        pico_ui_active->app->device_state = 9;
        pico_api_report_state(pico_ui_active->network);
        pico_ui_wait_for_footer_press("Run Program");
        break;
    case 1:
        pico_display_center_text(3, "INSERT");
        row = 4;
        text = "STEPFILTER";
        goto draw_status_line;
    case 2:
        pico_display_center_text(3, "Starting");
        pico_display_center_text(4, "Program");
        break;
    case 3:
        pico_display_center_text(3, "Click When");
        pico_display_center_text(4, "Reservoir Empty");
        pico_ui_draw_continue_footer();
        pico_encoder_clear_click((PicoEncoder *)&pico_ui_active->encoder);
        break;
    case 4:
        pico_display_center_text(3, "Is Reservoir");
        pico_display_center_text(4, "Empty?");
        ui_confirm_at_row_result = pico_ui_confirm_at_row(7);
        return ui_confirm_at_row_result;
    case 5:
        pico_display_center_text(3, "Empty StepFilter and");
        pico_display_center_text(4, "fill reservoir with");
        pico_display_center_text(5, "clean water.");
        pico_ui_wait_for_continue();
        break;
    case 6:
        text = "Setting up...";
        row = 3;
    draw_status_line:
        pico_display_center_text(row, text);
        pico_display_flush();
    }
    return true;
}

int pico_ui_confirm_continue_or_abort(void)
{
    uint32_t menu_choose_result;
    int continue_selected;

    pico_ui_draw_choice_question();
    pico_label_continue(pico_ui_active->menu_labels[0]);
    pico_label_exit(pico_ui_active->menu_labels[1]);
    menu_choose_result = pico_menu_choose(6, 2, 0);
    continue_selected = 1 - menu_choose_result;
    if (1 < menu_choose_result) {
        continue_selected = 0;
    }
    return continue_selected;
}

void pico_ui_enter_manual_network(void)
{
    uint32_t menu_choose_result;

    pico_ui_manual_network_step(0);
    pico_ui_active->network->settings->wifi_ssid[0] = 0;
    pico_ui_manual_network_step(1);
    pico_encoder_edit_text(pico_ui_active->network->settings->wifi_ssid, 0x21);
    pico_ui_manual_network_step(2);
    menu_choose_result = pico_menu_choose(3, 3, 0);
    if (menu_choose_result == 1) {
        pico_ui_active->network->settings->wifi_security = '\x05';
    } else if (menu_choose_result == 2) {
        pico_ui_active->network->settings->wifi_security = '\a';
    } else {
        pico_ui_active->network->settings->wifi_security = '\x02';
    }
    pico_display_clear();
    pico_ui_active->network->settings->wifi_password[0] = 0;
    if (pico_ui_active->network->settings->wifi_security != '\a') {
        pico_ui_enter_wifi_password();
    }
    pico_display_clear();
    return;
}

void pico_ui_start_descale(void)
{
    bool ui_confirm_screen_result;

    ui_confirm_screen_result = pico_ui_confirm_screen(pico_ui_draw_descale_preparation);
    if (ui_confirm_screen_result) {
        pico_ui_active->services->run_descale(pico_ui_active->service_context);
        return;
    }
    pico_display_clear();
    pico_ui_draw_descale_rinse_preparation();
    pico_ui_wait_for_continue();
    return;
}

uint32_t pico_ui_run_instruction_screen(uint32_t screen_id, int sound_alarm, int program_id,
                                        const char *title, uint8_t use_context_title)
{
    if (pico_local_cancelled())
        return 34;
    uint32_t menu_select_action_result;
    PicoMenu instruction_menu;

    pico_display_clear_thunk();
    pico_ui_draw_instruction_screen(screen_id,
                                    (uint32_t)pico_ui_active->network->settings->language, title,
                                    (uint32_t)use_context_title);
    if (sound_alarm != 0) {
        pico_alarm_until_press_or_timeout(pico_ui_active->network->settings->language,
                                          pico_ui_active->network->settings->detailed_status,
                                          60000);
    }
    pico_menu_initialize(&instruction_menu);
    switch (screen_id) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 7:
    case 8:
        pico_menu_add_action(&instruction_menu, 0x17);
        if (program_id == 4) {
            menu_select_action_result = 0x19;
            goto add_stage_action;
        }
        break;
    default:
        ui_log("E", "SCR", "Unsuported screen");
        break;
    case 9:
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
    case 0x10:
    case 0x15:
    case 0x16:
        menu_select_action_result = 0x17;
        goto add_stage_action;
    case 10:
        menu_select_action_result = 0x1a;
    add_stage_action:
        pico_menu_add_action(&instruction_menu, menu_select_action_result);
        break;
    case 0xb:
        pico_menu_add_action(&instruction_menu, 0x1d);
        if (program_id == 3) {
            menu_select_action_result = 0x1c;
            goto add_stage_action;
        }
        break;
    case 0x11:
        menu_select_action_result = 0x1f;
        goto add_rinse_actions;
    case 0x12:
        menu_select_action_result = 0x20;
    add_rinse_actions:
        pico_menu_add_action(&instruction_menu, menu_select_action_result);
        menu_select_action_result = 0x1e;
        goto add_final_action;
    case 0x18:
        menu_select_action_result = 0x18;
        goto add_final_action;
    }
    menu_select_action_result = 0x22;
add_final_action:
    pico_menu_add_action(&instruction_menu, menu_select_action_result);
    pico_menu_populate_from_menu(&instruction_menu,
                                 (uint32_t)pico_ui_active->network->settings->language,
                                 (uint32_t)pico_ui_active->network->settings->detailed_status);
    menu_select_action_result = pico_menu_select_action(7, 0);
    return menu_select_action_result;
}

int pico_ui_run_instruction_sequence(const uint32_t *screen_ids, int count, int sound_first_alarm,
                                     int program_id, const char *title, uint8_t use_context_title)
{
    ui_assert(screen_ids != NULL && count > 0, "screenIds != NULL && screenIdsCount > 0");
    if (screen_ids == NULL)
        return 0;
    uint32_t action = 0;
    for (int index = 0; index < count; ++index) {
        bool sound_alarm = sound_first_alarm != 0 && index == 0;
        action = pico_ui_run_instruction_screen(screen_ids[index], sound_alarm, program_id, title,
                                                use_context_title);
        if (action != 0x17)
            return (int)action;
    }
    return (int)action;
}

uint32_t pico_get_temperature_input(uint32_t action_id, const char *program_title,
                                    uint32_t fahrenheit)
{
    const char *action_label;
    int button_pressed;

    char footer_text[24];
    char log_message[128];

    action_label =
        pico_action_label(action_id, (uint32_t)pico_ui_active->network->settings->language,
                          (uint32_t)pico_ui_active->network->settings->detailed_status);
    ui_cstring_copy(footer_text, 0x16, action_label);
    pico_encoder_set_step((PicoEncoder *)&pico_ui_active->encoder, 1);
    pico_encoder_set_range((PicoEncoder *)&pico_ui_active->encoder, 0x3c, 0xcb, fahrenheit);
    while (button_pressed = pico_encoder_take_click((PicoEncoder *)&pico_ui_active->encoder),
           button_pressed == 0) {
        if (pico_local_cancelled())
            break;
        fahrenheit = pico_encoder_get_position((PicoEncoder *)&pico_ui_active->encoder);
        pico_display_clear_thunk();
        pico_display_center_bitmap(0xb, &pico_ui_asset_temperature_header);
        pico_display_center_bitmap(0x2c, &pico_ui_asset_temperature_footer);
        pico_display_draw_titled_frame(program_title);

        pico_display_large_temperature((double)fahrenheit, 0);
        pico_ui_draw_footer_single(footer_text);
        pico_display_flush();
    }
    snprintf(log_message, 0x80, "%s \"%s\" %dF", "getTemperatureInput", program_title,
             (int)fahrenheit);
    ui_log("I", "UI", log_message);
    return fahrenheit;
}

int pico_get_time_input(uint32_t action_id, const char *program_title, int minutes)
{
    const char *action_label;
    int button_or_increment;
    char footer_text[24];
    char log_message[128];

    action_label =
        pico_action_label(action_id, (uint32_t)pico_ui_active->network->settings->language,
                          (uint32_t)pico_ui_active->network->settings->detailed_status);
    ui_cstring_copy(footer_text, 0x16, action_label);
    if (minutes < 0x3c) {
        button_or_increment = 1;
    } else {
        button_or_increment = 5;
    }
    pico_encoder_set_step((PicoEncoder *)&pico_ui_active->encoder, button_or_increment);
    pico_encoder_set_range((PicoEncoder *)&pico_ui_active->encoder, 0, 0x253, minutes);
    while (button_or_increment = pico_encoder_take_click((PicoEncoder *)&pico_ui_active->encoder),
           button_or_increment == 0) {
        if (pico_local_cancelled())
            break;
        minutes = pico_encoder_get_position((PicoEncoder *)&pico_ui_active->encoder);
        if (minutes < 0x3c) {
            button_or_increment = 1;
        } else {
            button_or_increment = 5;
        }
        pico_encoder_set_step((PicoEncoder *)&pico_ui_active->encoder, button_or_increment);
        pico_display_clear_thunk();
        pico_display_center_bitmap(0xb, &pico_ui_asset_temperature_header);
        pico_display_center_bitmap(0x2c, &pico_ui_asset_temperature_footer);
        pico_display_draw_titled_frame(program_title);
        pico_display_large_timer(minutes, 0, 0);
        pico_ui_draw_footer_single(footer_text);
        pico_display_flush();
    }
    pico_encoder_set_step((PicoEncoder *)&pico_ui_active->encoder, 1);
    snprintf(log_message, 0x80, "%s \"%s\" %d min", "getTimeInput", program_title, minutes);
    ui_log("I", "UI", log_message);
    return minutes;
}
