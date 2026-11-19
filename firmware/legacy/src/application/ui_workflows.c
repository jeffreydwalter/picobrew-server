/* Pico Pro 0.1.34 — ui workflows. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Keeps drawing pause screen until encoder press and step-filter-present condition.
 * Firmware address: 0x2000a414. */
void pico_ui_wait_for_resume(void)
{
    int encoder_take_click_result;

    do {
        while (encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder),
               encoder_take_click_result == 0) {
            pico_ui_draw_paused();
        }
        encoder_take_click_result = pico_step_filter_is_inserted();
    } while (encoder_take_click_result == 0);
    return;
}

/* Renders footer text, resets encoder button, waits for press.
 * Firmware address: 0x2000a434. */
void pico_ui_wait_for_footer_press(char *text)
{
    int encoder_take_click_result;

    pico_ui_draw_footer_single(text);
    pico_display_flush();
    pico_encoder_clear_click((PicoEncoder *)&g_encoder);
    do {
        encoder_take_click_result = pico_encoder_take_click((PicoEncoder *)&g_encoder);
    } while (encoder_take_click_result == 0);
    return;
}

/* Add water/cleaning tablet instructions then waits at Start Clean footer.
 * Firmware address: 0x2000af18. */
void pico_ui_prompt_deep_clean_setup(void)
{
    char *text;

    pico_ui_draw_deep_clean_heading();
    if (g_language == '\x01') {
        pico_display_center_text(2, "Agregue agua al");
        pico_display_center_text(3, "barril y al");
        pico_display_center_text(4, "reservorio y luego");
        pico_display_center_text(5, "agregue la tableta");
        pico_display_center_text(6, "de limpieza al barril");
        text = "Comience a limpiar";
    } else if (g_language == '\x02') {
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

/* Draws insertion instruction repeatedly until step-filter predicate changes.
 * Firmware address: 0x2000b17c. */
void pico_ui_wait_for_filter_insertion(void)
{
    int step_filter_is_inserted_result;

    step_filter_is_inserted_result = pico_step_filter_is_inserted();
    if (step_filter_is_inserted_result == 0) {
        pico_display_clear();
        pico_ui_draw_instruction_with_context(1, (uint32_t)g_language_as_uint8_t, g_session_title);
        pico_display_flush();
        pico_wait_for_step_filter();
    }
    pico_display_clear();
    return;
}

/* Clears display, draws password heading and invokes text editor.
 * Firmware address: 0x2000b1e8. */
void pico_ui_enter_wifi_password(void)
{
    pico_ui_draw_password_heading();
    g_wifi_password = 0;
    pico_encoder_edit_text(&g_wifi_password, 0x40);
    pico_display_clear();
    return;
}

/* Loads SSID/security for scanned-network index, prompts password unless open security.
 * Firmware address: 0x2000b208. */
void pico_ui_select_scanned_network(uint32_t network_index)
{
    uint32_t wi_fi_scan_result_security_result;
    char *source;
    uint32_t unresolved_input_r2;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_input_r3;
    uint32_t unresolved_return_r3;

    wi_fi_scan_result_security_result = WiFi_scan_result_security(
        &global_wifi, network_index, unresolved_input_r2, unresolved_input_r3);
    g_wifi_security_type = (char)wi_fi_scan_result_security_result;
    source = WiFi_scan_result_ssid(&global_wifi, network_index, unresolved_return_r2,
                                   unresolved_return_r3);
    libc_strcpy(&g_wifi_ssid, source);
    g_wifi_password = 0;
    if (g_wifi_security_type == '\a') {
        return;
    }
    pico_ui_enter_wifi_password();
    return;
}

/* Language menu selection updates global language0/1/2.
 * Firmware address: 0x2000c334. */
uint32_t pico_ui_choose_language(void)
{
    uint32_t menu_populate_languages_result;

    menu_populate_languages_result = pico_menu_populate_languages();
    pico_display_clear();
    pico_ui_draw_language_menu();
    menu_populate_languages_result = pico_menu_choose(3, menu_populate_languages_result, 0);
    if (menu_populate_languages_result != 1) {
        if (menu_populate_languages_result == 0) {
            g_language_as_uint8_t_2 = 0;
            return 0;
        }
        if (menu_populate_languages_result != 2) {
            return menu_populate_languages_result;
        }
    }
    g_language_as_uint8_t_2 = (char)menu_populate_languages_result;
    return menu_populate_languages_result;
}

/* Units menu selection updates metric preference.
 * Firmware address: 0x2000c36c. */
uint32_t pico_ui_choose_units(void)
{
    uint32_t ui_draw_units_menu_result;

    pico_display_clear();
    ui_draw_units_menu_result = pico_ui_draw_units_menu();
    ui_draw_units_menu_result = pico_menu_choose(3, ui_draw_units_menu_result, 0);
    g_temperature_unit_celsius_as_uint8_t_2 = ui_draw_units_menu_result != 0;
    return (uint32_t)(ui_draw_units_menu_result != 0);
}

/* CHOOSE A PROGRAM screen and program selection menu.
 * Firmware address: 0x2000c418. */
uint32_t pico_ui_choose_program(void)
{
    uint32_t menu_populate_still_actions_result;

    pico_display_clear();
    pico_ui_draw_subheader("CHOOSE A PROGRAM");
    menu_populate_still_actions_result = pico_menu_populate_still_actions(2);
    menu_populate_still_actions_result = pico_menu_choose(4, menu_populate_still_actions_result, 0);
    return menu_populate_still_actions_result;
}

/* Renders up to four indexed steps with menu-based advance/exit.
 * Firmware address: 0x2000c43c. */
uint32_t pico_ui_walk_program_steps(int program_id)
{
    uint32_t menu_populate_still_actions_result;
    uint32_t unresolved_input_r1;
    int value_2;
    uint32_t value_14;

    value_2 = 0;
    value_14 = unresolved_input_r1;
    do {
        value_14 = pico_join_u8_u24((char)value_2, (uint32_t)value_14);
        pico_screen_still_setup_step((uint8_t *)((int)&value_14 + 3));
        menu_populate_still_actions_result = pico_menu_populate_still_actions(program_id);
        menu_populate_still_actions_result =
            pico_menu_choose(7, menu_populate_still_actions_result, 0);
        if (menu_populate_still_actions_result != 0) {
            return menu_populate_still_actions_result;
        }
        value_2 = value_2 + 1;
    } while (value_2 != 4);
    return 0;
}

/* Localized reminder to run rinse cycle with single acknowledgement option.
 * Firmware address: 0x2000c564. */
void pico_ui_require_first_rinse(void)
{
    short row;
    char *text;

    pico_display_clear();
    if (g_language == '\x01') {
        pico_display_center_text(0, "CERVEZA CANCELADA");
        pico_display_center_text(3, "Descartar el mosto  ");
        pico_display_center_text(4, "y correr el ciclo");
        row = 5;
        text = "de lavado";
    } else if (g_language == '\x02') {
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
    pico_label_ok(&g_menu_label_0);
    pico_menu_choose(7, 1, 0);
    return;
}

/* Vacuum setup/start or active emptying instructions, selected by stage.
 * Firmware address: 0x2000c60c. */
int pico_ui_prompt_vacuum(int stage)
{
    char *localized_text;
    uint32_t selected_option;
    int continue_selected;
    char stop_label[28];

    pico_display_clear();
    localized_text = pico_title_label(2, (uint32_t)g_language_as_uint8_t);
    pico_ui_draw_header_at_row(localized_text, 0);
    if (stage == 0) {
        if (g_language_as_uint8_t == 1) {
            pico_display_center_text(2, "Utilice la vara");
            pico_display_center_text(3, "del barril NEGRA");
            pico_display_center_text(4, "para aspirar el");
            pico_display_center_text(5, "agua del");
            pico_display_center_text(6, "reservorio");
            localized_text = "Comience a aspirar";
        } else if (g_language_as_uint8_t == 2) {
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
        libc_strcpy(&g_menu_label_0, localized_text);
        pico_label_exit(&g_menu_label_1);
        selected_option = pico_menu_choose(7, 2, 0);
        continue_selected = 1 - selected_option;
        if (1 < selected_option) {
            continue_selected = 0;
        }
    } else {
        if (stage == 1) {
            if (g_language_as_uint8_t == 1) {
                pico_display_center_text(3, "Aspire el agua");
                pico_display_center_text(4, "del reservorio");
                localized_text = "hasta vaciarlo";
            } else if (g_language_as_uint8_t == 2) {
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
            pico_encoder_clear_click((PicoEncoder *)&g_encoder);
        }
        continue_selected = 1;
    }
    return continue_selected;
}

/* Loads Continue footer and waits for its selection.
 * Firmware address: 0x2000c7f8. */
uint32_t pico_ui_wait_for_continue(void)
{
    uint32_t menu_choose_result;

    pico_label_continue(&g_menu_label_0);
    menu_choose_result = pico_menu_choose(7, 1, 0);
    return menu_choose_result;
}

/* Shows missing-filter screen, sounds alarm until insertion, clears display.
 * Firmware address: 0x2000cc30. */
void pico_prompt_insert_filter_with_alarm(void)
{
    millis();
    pico_buzzer_on();
    pico_screen_reinsert_step_filter();
    pico_display_flush();
    pico_alarm_while_filter_missing();
    pico_display_clear();
    return;
}

/* Drain setup instructions, StartDrain action, pump toggle controls and abort log.
 * Firmware address: 0x2000d014. */
uint32_t pico_run_drain_instructions(uint32_t second_screen_id)
{
    bool drain_running;
    int selected_action;
    uint32_t result_action;
    uint32_t drain_menu_id;
    uint32_t first_screen_id;
    uint32_t next_screen_id;

    first_screen_id = 0xe;
    next_screen_id = second_screen_id;
    selected_action = pico_ui_run_instruction_sequence(&first_screen_id, 2, 0, g_session_program_id,
                                                       g_session_title, '\x01');
    if (selected_action == 0x17) {
        drain_running = pico_drain_pump_is_on();
        if (!drain_running) {
            pico_drain_pump_on();
        }
        while (true) {
            drain_running = pico_drain_pump_is_on();
            if (drain_running) {
                drain_menu_id = 0x11;
            } else {
                drain_menu_id = 0x12;
            }
            drain_menu_id = pico_execute_current_program_instruction(drain_menu_id, 0);
            if (1 < drain_menu_id - 0x1f)
                break;
            drain_running = digitalRead(8);
            selected_action = 1 - (uint32_t)drain_running;
            if (1 < drain_running) {
                selected_action = 0;
            }
            digitalWrite(8, selected_action);
        }
        pico_drain_pump_off();
        result_action = 0x17;
    } else {
        pico_log_serial("W", "RCTL", "Drain setup aborted by user");
        result_action = 0x22;
    }
    return result_action;
}

/* Stage-specific AP-name and security-choice UI; called by manual network editor.
 * Firmware address: 0x2000e524. */
void pico_ui_manual_network_step(int stage)
{
    char *text_1;

    pico_display_clear();
    if (stage == 1) {
        pico_ui_draw_header_at_row("ENTER AP NAME", 0);
        if (g_language == '\x01') {
            text_1 = "NOMBRE DEL AP";
        } else if (g_language == '\x02') {
            text_1 = "NOM DU PA";
        } else {
            text_1 = "ENTER AP NAME";
        }
        pico_ui_draw_header_at_row(text_1, 0);
        return;
    }
    if (stage == 0) {
        if ((g_language == '\x01') || (g_language != '\x02')) {
            pico_display_center_text(3, "Please Enter Name");
            text_1 = "of AP, case matters";
        } else {
            pico_display_center_text(3, "NOM DU PA,");
            text_1 = "sensible a la casse";
        }
        pico_display_center_text(4, text_1);
        pico_ui_wait_for_continue();
        return;
    }
    if (stage != 2) {
        return;
    }
    if (g_language == '\x01') {
        text_1 = "Seguridad?";
    } else if (g_language == '\x02') {
        text_1 = "Securite?";
    } else {
        text_1 = "Security Type?";
    }
    pico_display_center_text(2, text_1);
    libc_strcpy(&g_menu_label_0, "WPA/WPA2");
    libc_strcpy(&g_menu_label_1, "WEP");
    libc_strcpy(&g_menu_label_2, "Unsecure");
    return;
}

/* Asks user to log into picobrew.com and associate PicoPak using serial number on box.
 * Firmware address: 0x2000e85c. */
void pico_ui_prompt_picopak_association(void)
{
    char *text;

    pico_display_clear();
    if (g_language == '\x01') {
        pico_display_center_text(0, "ERROR DE PicoPak");
        pico_display_center_text(2, "Entra en la cuenta de");
        pico_display_center_text(3, "picobrew.com y");
        pico_display_center_text(4, "asocie el PicoPak");
        pico_display_center_text(5, "utilizando el numero");
        text = "de serie en la caja";
    } else if (g_language == '\x02') {
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
}

/* Finished/disconnect-hoses/discard-waste-water instructions and acknowledgement.
 * Firmware address: 0x2000e958. */
void pico_ui_show_rinse_finished(void)
{
    char *text;

    pico_ui_draw_startup_heading();
    if (g_language == '\x01') {
        pico_display_center_text(3, "Finalizado!");
        pico_display_center_text(4, "Desconecte las");
        pico_display_center_text(5, "mangueras y deseche");
        text = "el aguaantes de lavar";
    } else if (g_language == '\x02') {
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

/* Switches first-rinse stage into localized setup/progress/failure/completion screens.
 * Firmware address: 0x2000eadc. */
void pico_ui_first_rinse_step(uint32_t stage)
{
    short row;
    char *text_1;

    pico_display_clear();
    pico_ui_draw_first_rinse_heading();
    switch (stage) {
    case 0:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Agregue agua al");
            pico_display_center_text(4, "barril y al");
            pico_display_center_text(5, "reservorio");
            text_1 = "Comience a lavar";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Ajouter de l\'eau");
            pico_display_center_text(4, "dans le fut et");
            pico_display_center_text(5, "le reservoir");
            text_1 = "Commencer le rincage";
        } else {
            pico_display_center_text(3, "Add water to keg");
            pico_display_center_text(4, "and reservoir");
            pico_display_center_text(5, "Click when ready");
            text_1 = "Start Rinse";
        }
        libc_strcpy(&g_menu_label_0, text_1);
        pico_menu_choose(7, 1, 0);
        return;
    case 1:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Por favor inserte");
            pico_display_center_text(4, "el Filtro de Paso");
            row = 5;
            text_1 = "para comenzar";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Veillez inserer");
            pico_display_center_text(4, "le filtre pour");
            text_1 = "commencer";
            row = 5;
        } else {
            pico_display_center_text(3, "Whoops, please");
            pico_display_center_text(4, "insert the Step");
            row = 5;
            text_1 = "Filter to get started";
        }
        break;
    case 2:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Preparar el");
            row = 4;
            text_1 = "generador de vapor";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Preparation du");
            row = 4;
            text_1 = "generateur a vapeur";
        } else {
            pico_display_center_text(3, "Priming steam");
            row = 4;
            text_1 = "generator";
        }
        break;
    case 3:
        row = 3;
        if (g_language == '\x01') {
            pico_display_center_text(3, "Preparar la bomba");
            row = 4;
            text_1 = "de vapor";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Preparation de la");
            row = 4;
            text_1 = "pompe a vapeur";
        } else {
            text_1 = "Priming steam pump";
        }
        break;
    case 4:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Sistema de lavado");
            row = 4;
            text_1 = "con vapor y agua";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Systeme a l\'eau de");
            row = 4;
            text_1 = "rincage a la vapeur";
        } else {
            pico_display_center_text(3, "Steam flushing");
            row = 4;
            text_1 = "water system";
        }
        break;
    case 5:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Sistema de lavado del");
            row = 4;
            text_1 = "barril y drenaje";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Rincage du fut et");
            row = 4;
            text_1 = "vidange du systeme";
        } else {
            pico_display_center_text(3, "Flushing keg and");
            row = 4;
            text_1 = "drain system";
        }
        break;
    case 6:
        if (g_language == '\x01') {
            text_1 = "ERROR DE LA BOMBA";
        } else if (g_language == '\x02') {
            text_1 = "ERREUR DE LA POMPE";
        } else {
            text_1 = "SHUTTLE PUMP ERROR";
        }
        goto branch_1;
    case 7:
        if (g_language == '\x01') {
            text_1 = "ERROR DEL CALENTADOR";
        } else if (g_language == '\x02') {
            text_1 = "ERREUR DU RADIATEUR";
        } else {
            text_1 = "HEATER ERROR";
        }
    branch_1:
        pico_display_center_text(3, text_1);
        pico_screen_help_address();
        goto branch_2;
    case 8:
        pico_screen_rinse_complete();
        goto branch_2;
    default:
        return;
    }
    pico_display_center_text(row, text_1);
branch_2:
    pico_display_flush();
    return;
}

/* Loads localized Stop footer and waits for single-option encoder selection.
 * Firmware address: 0x2000ef10. */
void pico_ui_wait_for_stop(void)
{
    pico_label_done(&g_menu_label_0);
    pico_menu_choose(7, 1, 0);
    return;
}

/* Switches racking stage into connection/flush/pressure/transfer instructions with
 * acknowledgements. Firmware address: 0x2000f26c. */
void pico_ui_rack_beer_step(uint32_t stage)
{
    short short_value_1;
    char *text_2;

    pico_display_clear();
    pico_ui_draw_rack_beer_heading();
    switch (stage) {
    case 0:
        if (g_language == '\x01') {
            pico_display_center_text(2, "Conecte la vara del");
            pico_display_center_text(3, "barril a la esfera ");
            pico_display_center_text(4, "de seguro GRIS y");
            pico_display_center_text(5, "coloquela en el con-");
            text_2 = "tenedor de desechos";
        branch_1:
            pico_display_center_text(6, text_2);
            text_2 = "Comience a limpiar";
            goto branch_2;
        }
        if (g_language != '\x02') {
            pico_display_center_text(2, "Connect keg wand to");
            pico_display_center_text(3, "GRAY ball-lock, and");
            pico_display_center_text(4, "place in waste");
            pico_display_center_text(5, "container");
            text_2 = "Start Flush";
            goto branch_2;
        }
        pico_display_center_text(2, "Brancher le baton de");
        pico_display_center_text(3, "fut au connecteur");
        pico_display_center_text(4, "a bille GRIS et");
        pico_display_center_text(5, "placez-le dans le");
        text_2 = "contenant a dechets";
        goto branch_3;
    case 1:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Manguera para limpiar");
            pico_display_center_text(4, "limpiar el drenaje.");
            pico_display_center_text(5, "Haga clic");
            short_value_1 = 6;
            text_2 = "cuando termine";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Rincage du tuyau ");
            pico_display_center_text(4, "de drainage.");
            pico_display_center_text(5, "Cliquez lorsque");
            text_2 = "termine para limpiar";
            short_value_1 = 6;
        } else {
            pico_display_center_text(3, "Flushing drain hose");
            short_value_1 = 5;
            text_2 = "Click when done";
        }
        pico_display_center_text(short_value_1, text_2);
        pico_display_flush();
        g_device_state = 5;
        goto branch_4;
    case 2:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Retire la vara del");
            pico_display_center_text(4, "barril de la esfera");
            short_value_1 = 5;
            text_2 = "de seguro GRIS";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Enlevez le baton");
            pico_display_center_text(4, "fut du connecteur");
            text_2 = "de a bille GRIS";
            short_value_1 = 5;
        } else {
            pico_display_center_text(3, "Remove keg wand from");
            short_value_1 = 4;
            text_2 = "GRAY ball-lock";
        }
        break;
    case 3:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Conecte la esfera");
            pico_display_center_text(4, "de seguro GRIS al");
            pico_display_center_text(5, "conector de IN");
            short_value_1 = 6;
            text_2 = "del barril";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "branchez le connecteur");
            pico_display_center_text(4, "a bille GRIS sur le");
            pico_display_center_text(5, "raccord IN du fut ");
            short_value_1 = 6;
            text_2 = "baril";
        } else {
            pico_display_center_text(3, "Connect GRAY ball-");
            short_value_1 = 4;
            text_2 = "lock to keg IN post";
        }
        break;
    case 4:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Hale la valvula de");
            pico_display_center_text(4, "presion del barril");
            pico_display_center_text(5, "para liberar la ");
            short_value_1 = 6;
            text_2 = "presion";
        } else if (g_language == '\x02') {
            pico_display_center_text(3, "Appuyez sur la");
            pico_display_center_text(4, "soupape de pression");
            pico_display_center_text(5, "du fut pour");
            short_value_1 = 6;
            text_2 = "depressuriser";
        } else {
            pico_display_center_text(3, "Pull keg pressure");
            pico_display_center_text(4, "valve to release");
            short_value_1 = 5;
            text_2 = "any pressure";
        }
        break;
    case 5:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Conecte el tubo de");
            pico_display_center_text(4, "trasiego de SALIDA");
            pico_display_center_text(5, &g_ui_workflows_unknown_04);
            text_2 = "al desechos";
            goto branch_1;
        }
        if (g_language != '\x02') {
            pico_display_center_text(2, "Connect racking tube");
            pico_display_center_text(3, "to keg OUT post,");
            pico_display_center_text(4, "and direct tube");
            pico_display_center_text(5, "to waste");
            pico_display_center_text(6, "container");
            text_2 = "Start Flush";
            goto branch_2;
        }
        pico_display_center_text(3, "Branchez le tube");
        pico_display_center_text(4, "au raccord OUT");
        pico_display_center_text(5, "fut et placez tube");
        text_2 = "le dechets";
    branch_3:
        pico_display_center_text(6, text_2);
        text_2 = "Demarrer le rincage";
    branch_2:
        pico_ui_wait_for_footer_press(text_2);
        return;
    case 6:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Deje correr hasta");
            pico_display_center_text(4, "que turbidez en el");
            short_value_1 = 5;
            text_2 = "tubo de trasiego";
        } else if (g_language == '\x02') {
            pico_display_center_text(2, "Continuez jusqu\'a");
            pico_display_center_text(3, "la disparition");
            pico_display_center_text(4, "complete du mout");
            pico_display_center_text(5, "bouillant dans le");
            text_2 = "tube de decantation";
            short_value_1 = 6;
        } else {
            pico_display_center_text(3, "Run until no trub");
            pico_display_center_text(4, "is visible in");
            short_value_1 = 5;
            text_2 = "racking tube";
        }
        pico_display_center_text(short_value_1, text_2);
        goto branch_5;
    case 7:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Introduzca el tubo de");
            pico_display_center_text(4, "trasiego en el barril");
            pico_display_center_text(5, "que se servira para");
            pico_display_center_text(6, "transferir la cerveza");
            text_2 = "Iniciar proceso";
        } else if (g_language == '\x02') {
            pico_display_center_text(2, "Inserez le tube de");
            pico_display_center_text(3, "decantation dans");
            pico_display_center_text(4, "le fut de service");
            pico_display_center_text(5, "pour transferer");
            pico_display_center_text(6, "la biere");
            text_2 = "Demarrer l\'action";
        } else {
            pico_display_center_text(3, "Insert racking tube");
            pico_display_center_text(4, "in serving keg to");
            pico_display_center_text(5, "transfer beer");
            text_2 = "Start Transfer";
        }
        goto branch_2;
    case 8:
        if (g_language == '\x01') {
            pico_display_center_text(3, "Deje correr hasta");
            pico_display_center_text(4, "que se llene el");
            pico_display_center_text(5, "barril o entre");
            short_value_1 = 6;
            text_2 = "aire al tubo";
        } else if (g_language == '\x02') {
            pico_display_center_text(2, "Continuez jusqu\'a ce");
            pico_display_center_text(3, "que le fut soit");
            pico_display_center_text(4, "plein et que l\'air");
            pico_display_center_text(5, "penetre dans le");
            text_2 = "tube";
            short_value_1 = 6;
        } else {
            pico_display_center_text(3, "Run until keg is");
            pico_display_center_text(4, "full or air");
            short_value_1 = 5;
            text_2 = "enters tube";
        }
        pico_display_center_text(short_value_1, text_2);
        pico_display_flush();
        g_device_state = 2;
    branch_4:
        pico_api_report_device_state();
    branch_5:
        pico_ui_wait_for_stop();
        return;
    default:
        return;
    }
    pico_display_center_text(short_value_1, text_2);
    pico_ui_wait_for_continue();
    return;
}

/* Switches descale stage into reservoir fill/start/empty/rinse instructions and confirmation.
 * Firmware address: 0x2000f928. */
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
        g_device_state = 9;
        pico_api_report_device_state();
        pico_ui_wait_for_footer_press("Run Program");
        break;
    case 1:
        pico_display_center_text(3, "INSERT");
        row = 4;
        text = "STEPFILTER";
        goto branch_1;
    case 2:
        pico_display_center_text(3, "Starting");
        pico_display_center_text(4, "Program");
        break;
    case 3:
        pico_display_center_text(3, "Click When");
        pico_display_center_text(4, "Reservoir Empty");
        pico_ui_draw_continue_footer();
        pico_encoder_clear_click((PicoEncoder *)&g_encoder);
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
    branch_1:
        pico_display_center_text(row, text);
        pico_display_flush();
    }
    return true;
}

/* Invokes supplied screen callback and asks confirmation at footer row7.
 * Firmware address: 0x2000fa40. */
bool pico_ui_confirm_screen(uint8_t *draw_screen)
{
    bool ui_confirm_at_row_result;

    pico_display_clear();
    (*(PicoOpaqueFunction *)draw_screen)();
    ui_confirm_at_row_result = pico_ui_confirm_at_row(7);
    return ui_confirm_at_row_result;
}

/* Reports thermal status error, alarms and offers retry/abort choices.
 * Firmware address: 0x2000fa80. */
int pico_ui_confirm_thermal_recovery(int controller_status)
{
    uint32_t selected_option;
    int value_2;
    PicoOpaqueFunction *draw_recovery_screen;

    draw_recovery_screen = (PicoOpaqueFunction *)&g_ui_workflows_unknown_01;
    if (controller_status == 6) {
        value_2 = 0x15;
    } else {
        draw_recovery_screen = pico_screen_water_error_help + 1;
        value_2 = 0x14;
    }
    pico_api_report_error(value_2);
    pico_alarm_until_encoder_press();
    pico_display_clear();
    (*draw_recovery_screen)();
    pico_display_flush();
    pico_label_continue(&g_menu_label_0);
    pico_label_exit(&g_menu_label_1);
    selected_option = pico_menu_choose(7, 2, 0);
    value_2 = 1 - selected_option;
    if (1 < selected_option) {
        value_2 = 0;
    }
    return value_2;
}

/* Generic localized Would you like to prompt followed by Continue/Abort options.
 * Firmware address: 0x2000fab8. */
int pico_ui_confirm_continue_or_abort(void)
{
    uint32_t menu_choose_result;
    int value_2;

    pico_ui_draw_choice_question();
    pico_label_continue(&g_menu_label_0);
    pico_label_exit(&g_menu_label_1);
    menu_choose_result = pico_menu_choose(6, 2, 0);
    value_2 = 1 - menu_choose_result;
    if (1 < menu_choose_result) {
        value_2 = 0;
    }
    return value_2;
}

/* Edits SSID, selects security, edits password if needed.
 * Firmware address: 0x2000fac8. */
void pico_ui_enter_manual_network(void)
{
    uint32_t menu_choose_result;

    pico_ui_manual_network_step(0);
    g_wifi_ssid = 0;
    pico_ui_manual_network_step(1);
    pico_encoder_edit_text(&g_wifi_ssid, 0x21);
    pico_ui_manual_network_step(2);
    menu_choose_result = pico_menu_choose(3, 3, 0);
    if (menu_choose_result == 1) {
        g_wifi_security_type = '\x05';
    } else if (menu_choose_result == 2) {
        g_wifi_security_type = '\a';
    } else {
        g_wifi_security_type = '\x02';
    }
    pico_display_clear();
    g_wifi_password = 0;
    if (g_wifi_security_type != '\a') {
        pico_ui_enter_wifi_password();
    }
    pico_display_clear();
    return;
}

/* Scans SSIDs, adds manual/rescan options, selects network, connects and checks DNS.
 * Firmware address: 0x2000fb30. */
uint32_t pico_ui_scan_and_connect_wifi(uint32_t argument_1, uint32_t argument_2,
                                       uint32_t argument_3)
{
    uint32_t value_1;
    int wi_fi_initialize_result;
    uint32_t wi_fi_scan_networks_result;
    char *source;
    int wifi_resolve_server_result;
    uint32_t unresolved_return_r2;
    uint32_t value_5;
    uint32_t unresolved_return_r3;
    uint32_t value_6;
    uint32_t value;
    uint32_t value_18;

    value = argument_2;
    value_18 = argument_3;
    pico_display_clear();
    value = 0;
    if ((wifi_initialized != '\0') ||
        (wi_fi_initialize_result = WiFi_initialize(), wi_fi_initialize_result != 0)) {
        while (true) {
            pico_display_clear();
            pico_display_device_bitmap();
            pico_screen_wifi_searching();
            pico_display_flush();
            g_progress_start_column = 9;
            g_progress_indicator = 1;
            g_progress_end_column = 0x15;
            g_progress_glyph_index = 0;
            g_progress_active = 1;
            wi_fi_scan_networks_result = WiFi_scan_networks();
            pico_display_stop_progress_indicator(&g_progress_indicator);
            if ((wi_fi_scan_networks_result & 0xff) == 0)
                break;
            value = 0;
            value_5 = unresolved_return_r2;
            value_6 = unresolved_return_r3;
            while (value_1 = value, (int)value < (int)(wi_fi_scan_networks_result & 0xff)) {
                source = WiFi_scan_result_ssid(&global_wifi, value & 0xff, value_5, value_6);
                libc_strncpy(&g_menu_label_0 + value_1 * 0x16, source, 0x15);
                value_5 = 0;
                value_6 = value + 1;
                (&g_ui_menus_unknown_04)[value * 0x16] = 0;
                value = value_6;
            }
            pico_menu_append_wifi_search_actions((int *)&value);
            pico_display_clear();
            pico_ui_draw_choose_network();
            wi_fi_scan_networks_result = pico_menu_choose(3, value & 0xff, 0);
            if (wi_fi_scan_networks_result != value - 1) {
                if (wi_fi_scan_networks_result == value - 2) {
                    pico_ui_enter_manual_network();
                } else {
                    pico_ui_select_scanned_network(wi_fi_scan_networks_result);
                }
                pico_display_clear();
                wi_fi_initialize_result = pico_wifi_connect_with_screen();
                if ((wi_fi_initialize_result != 0) &&
                    (wifi_resolve_server_result = pico_wifi_resolve_server(),
                     wifi_resolve_server_result == 0)) {
                    pico_display_stop_progress_indicator(&g_progress_indicator);
                    pico_diagnostic_wifi();
                }
                pico_display_stop_progress_indicator(&g_progress_indicator);
                pico_display_clear();
                if (wi_fi_initialize_result == 0) {
                    pico_display_device_bitmap_variant_b();
                    pico_display_flush();
                    delay(1000);
                    return 0;
                }
                pico_display_device_bitmap_variant_a();
                pico_display_flush();
                return 1;
            }
            pico_display_clear();
        }
        pico_screen_wifi_no_signal();
        pico_ui_wait_for_continue();
    }
    return 0;
}

/* Start/adjust/cancel loop with ABV and bitterness controls, re-fetching recipe after changes.
 * Firmware address: 0x20010e1c. */
int pico_ui_adjust_recipe(void *recipe)
{
    uint32_t value_1;
    uint32_t ui_draw_recipe_choices_result;
    int api_fetch_recipe_result;
    uint32_t unresolved_input_r1;
    uint32_t value_14;

    value_14 = unresolved_input_r1;
    while (true) {
        pico_display_clear();
        if (*(int *)((int)recipe + 0x228) == -1) {
            ui_draw_recipe_choices_result = (uint32_t)(*(int *)((int)recipe + 0x230) != -1);
        } else {
            ui_draw_recipe_choices_result = 1;
        }
        ui_draw_recipe_choices_result =
            pico_ui_draw_recipe_choices(&g_screen_recipe_name, ui_draw_recipe_choices_result);
        pico_display_clear();
        pico_display_refresh_current_page();
        ui_draw_recipe_choices_result = pico_menu_choose(7, ui_draw_recipe_choices_result, 0);
        if (ui_draw_recipe_choices_result == 0) {
            return 1;
        }
        if (ui_draw_recipe_choices_result != 1) {
            return 0;
        }
        if ((*(int *)((int)recipe + 0x228) == -1) && (*(int *)((int)recipe + 0x230) == -1))
            break;
        pico_display_clear();
        value_1 = (uint32_t)value_14;
        value_14 = (uint32_t)(value_1 & 0xffff);
        if (*(int *)((int)recipe + 0x230) != -1) {
            pico_ui_draw_bitterness_adjustment();
            ui_draw_recipe_choices_result = pico_encoder_select_adjustment_level(
                recipe, *(uint32_t *)((int)recipe + 0x230), (uint32_t *)((int)recipe + 0x224),
                (int)&value_14 + 2);
            *(uint32_t *)((int)recipe + 0x224) = ui_draw_recipe_choices_result;
            pico_display_clear();
        }
        if (*(int *)((int)recipe + 0x228) != -1) {
            pico_ui_draw_abv_adjustment();
            ui_draw_recipe_choices_result = pico_encoder_select_adjustment_level(
                recipe, *(uint32_t *)((int)recipe + 0x228), (uint32_t *)((int)recipe + 0x220),
                (int)&value_14 + 3);
            *(uint32_t *)((int)recipe + 0x220) = ui_draw_recipe_choices_result;
        }
        pico_display_clear();
        if ((PICO_PART_U8(value_14, 2) != '\0') || (PICO_PART_U8(value_14, 3) != '\0')) {
            pico_display_clear();
            pico_screen_loading_picopak();
            api_fetch_recipe_result =
                pico_api_fetch_recipe((char *)((int)recipe + 4), *(int *)((int)recipe + 0x224),
                                      *(int *)((int)recipe + 0x220));
            if (api_fetch_recipe_result != 0) {
                pico_display_stop_progress_indicator(&g_progress_indicator);
                return 0;
            }
            pico_display_stop_progress_indicator(&g_progress_indicator);
        }
    }
    return 0;
}

/* Shows descale preparation and confirmation, then runs descale program or alternate instructions.
 * Firmware address: 0x20011564. */
void pico_ui_start_descale(void)
{
    bool ui_confirm_screen_result;

    ui_confirm_screen_result = pico_ui_confirm_screen(pico_ui_draw_descale_preparation + 1);
    if (ui_confirm_screen_result) {
        pico_run_descale_program();
        return;
    }
    pico_display_clear();
    pico_ui_draw_descale_rinse_preparation();
    pico_ui_wait_for_continue();
    return;
}

/* Paged recipe/PicoPak list selection and server-side PicoPak creation.
 * Firmware address: 0x20012b94. */
uint32_t pico_ui_select_recipe(int recipe)
{
    bool last_page;
    bool show_recipe_catalog;
    int api_get_recipe_list_result;
    uint32_t selected_item;
    int libc_strcmp_result;
    int value_6;
    int total_items;
    uint32_t menu_item_count;
    int loaded_items;
    int page_offset;
    char selected_title[24];
    char item_identifiers[420];
    char item_titles[484];

    total_items = 0;
    menu_item_count = 0;
    loaded_items = 0;
    page_offset = 0;
    show_recipe_catalog = true;
    while (true) {
        while (true) {
            pico_display_clear();
            pico_screen_loading_picopak();
            if ((!show_recipe_catalog) ||
                (api_get_recipe_list_result = pico_api_get_recipe_list(
                     &total_items, &page_offset, (int *)&menu_item_count, (int)item_identifiers),
                 api_get_recipe_list_result == 0)) {
                api_get_recipe_list_result = pico_api_get_associated_picopaks(
                    &total_items, &page_offset, (int *)&menu_item_count, (int)item_identifiers,
                    (int)item_titles, &loaded_items);
                if (api_get_recipe_list_result == 0) {
                    pico_display_stop_progress_indicator(&g_progress_indicator);
                    pico_ui_prompt_picopak_association();
                    return 0;
                }
                show_recipe_catalog = false;
            }
            pico_display_stop_progress_indicator(&g_progress_indicator);
            last_page = total_items - page_offset < 0x14;
            if (!last_page) {
                pico_menu_set_load_more_label(menu_item_count);
                menu_item_count = menu_item_count + 1;
            }
            pico_label_exit(&g_menu_label_0 + menu_item_count * 0x16);
            menu_item_count = menu_item_count + 1;
            pico_display_clear();
            pico_ui_draw_picopak_selection();
            selected_item = pico_menu_choose(3, menu_item_count & 0xff, 0);
            if (selected_item != menu_item_count - 1)
                break;
            if (!show_recipe_catalog) {
                return 0;
            }
            show_recipe_catalog = false;
        }
        if ((selected_item != menu_item_count - 2) || (last_page))
            break;
        page_offset = selected_item + page_offset;
    }
    pico_display_clear();
    pico_screen_loading_picopak();
    if (show_recipe_catalog) {
        api_get_recipe_list_result = libc_atoi(item_identifiers + selected_item * 0x16);
        api_get_recipe_list_result = pico_api_create_picopak(api_get_recipe_list_result);
        if (api_get_recipe_list_result == 0)
            goto branch_1;
    } else {
        value_6 = selected_item * 0x16;
        libc_strcpy(selected_title, &g_menu_label_0 + value_6);
        api_get_recipe_list_result = 0;
        menu_item_count = 0;
        for (; (int)selected_item < loaded_items; selected_item = selected_item + 1) {
            libc_strcmp_result =
                libc_strcmp(item_titles + api_get_recipe_list_result + value_6, selected_title);
            if (libc_strcmp_result == 0) {
                libc_strcmp_result = menu_item_count * 0x16;
                menu_item_count = menu_item_count + 1;
                libc_strcpy(&g_menu_label_0 + libc_strcmp_result,
                            item_identifiers + api_get_recipe_list_result + value_6);
            }
            api_get_recipe_list_result = api_get_recipe_list_result + 0x16;
        }
        if ((int)menu_item_count < 2) {
            selected_item = menu_item_count - 1;
        } else {
            pico_display_stop_progress_indicator(&g_progress_indicator);
            pico_display_clear();
            pico_ui_draw_picopak_selection();
            pico_label_exit(&g_menu_label_0 + menu_item_count * 0x16);
            selected_item = pico_menu_choose(3, menu_item_count & 0xff, 0);
            if (selected_item == menu_item_count - 1) {
                return 0;
            }
        }
        libc_strcpy((char *)(recipe + 4), &g_menu_label_0 + selected_item * 0x16);
    }
    show_recipe_catalog = pico_api_fetch_default_recipe_failed((char *)(recipe + 4));
    if (!show_recipe_catalog) {
        pico_display_stop_progress_indicator(&g_progress_indicator);
        return 1;
    }
branch_1:
    pico_display_stop_progress_indicator(&g_progress_indicator);
    return 0;
}

/* Renders instruction and runs appropriate action menu/confirmation for screen ID.
 * Firmware address: 0x20013484. */
uint32_t pico_ui_run_instruction_screen(uint32_t screen_id, int sound_alarm, int program_id,
                                        char *title, uint8_t use_context_title)
{
    uint32_t menu_select_action_result;
    int value_2;
    PicoMenu value_74;

    pico_display_clear_thunk();
    pico_ui_draw_instruction_screen(screen_id, (uint32_t)g_language_as_uint8_t, title,
                                    (uint32_t)use_context_title);
    if (sound_alarm != 0) {
        pico_alarm_until_press_or_timeout((uint32_t)g_language_as_uint8_t,
                                          (uint32_t)g_detailed_status, 60000);
    }
    value_2 = 0;
    do {
        *(uint32_t *)((int)value_74.actions + value_2) = 0;
        value_2 = value_2 + 4;
    } while (value_2 != 0x58);
    value_74.count = 0;
    switch (screen_id) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 7:
    case 8:
        pico_menu_add_action(&value_74, 0x17);
        if (program_id == 4) {
            menu_select_action_result = 0x19;
            goto branch_1;
        }
        break;
    default:
        pico_log_serial("E", "SCR", "Unsuported screen");
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
        goto branch_1;
    case 10:
        menu_select_action_result = 0x1a;
    branch_1:
        pico_menu_add_action(&value_74, menu_select_action_result);
        break;
    case 0xb:
        pico_menu_add_action(&value_74, 0x1d);
        if (program_id == 3) {
            menu_select_action_result = 0x1c;
            goto branch_1;
        }
        break;
    case 0x11:
        menu_select_action_result = 0x1f;
        goto branch_2;
    case 0x12:
        menu_select_action_result = 0x20;
    branch_2:
        pico_menu_add_action(&value_74, menu_select_action_result);
        menu_select_action_result = 0x1e;
        goto branch_3;
    case 0x18:
        menu_select_action_result = 0x18;
        goto branch_3;
    }
    menu_select_action_result = 0x22;
branch_3:
    pico_menu_add_action(&value_74, menu_select_action_result);
    pico_menu_populate_from_menu(value_74.actions, (uint32_t)g_language_as_uint8_t,
                                 (uint32_t)g_detailed_status);
    menu_select_action_result = pico_menu_select_action(7, 0);
    return menu_select_action_result;
}

/* Traverses supplied screen IDs until final completion or user cancel.
 * Firmware address: 0x20013578. */
int pico_ui_run_instruction_sequence(uint32_t *screen_ids, int count, int sound_first_alarm,
                                     int program_id, char *title, uint8_t use_context_title)
{
    int value_1;
    uint32_t ui_run_instruction_screen_result;
    uint32_t index;

    index = 0;
    if (screen_ids != (uint32_t *)0x0) {
        index = (uint32_t)(0 < count);
    }
    pico_assert_nonfatal(index, "screenIds != NULL && screenIdsCount > 0");
    if (screen_ids == (uint32_t *)0x0) {
        value_1 = 0;
    } else {
        value_1 = 0;
        for (index = 0; (int)index < count; index = index + 1) {
            if (sound_first_alarm == 0) {
                value_1 = 0;
            } else {
                value_1 = 1 - index;
                if (1 < index) {
                    value_1 = 0;
                }
            }
            ui_run_instruction_screen_result = pico_ui_run_instruction_screen(
                screen_ids[index], value_1, program_id, title, use_context_title);
            if (ui_run_instruction_screen_result != 0x17) {
                return ui_run_instruction_screen_result;
            }
            value_1 = 0x17;
        }
    }
    return value_1;
}

/* Explicit getTemperatureInput log, encoder limits60..203 and Fahrenheit formatting.
 * Firmware address: 0x200135d8. */
uint32_t pico_get_temperature_input(uint32_t action_id, char *program_title, uint32_t fahrenheit)
{
    char *action_label;
    int button_pressed;
    double __aeabi_i2d_result;
    char footer_text[24];
    char log_message[128];

    action_label =
        pico_action_label(action_id, (uint32_t)g_language_as_uint8_t, (uint32_t)g_detailed_status);
    pico_cstring_copy(footer_text, 0x16, action_label);
    pico_encoder_set_step((PicoEncoder *)&g_encoder, 1);
    pico_encoder_set_range((PicoEncoder *)&g_encoder, 0x3c, 0xcb, fahrenheit);
    while (button_pressed = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           button_pressed == 0) {
        fahrenheit = pico_encoder_get_position((PicoEncoder *)&g_encoder);
        pico_display_clear_thunk();
        pico_display_center_bitmap(0xb, (uint32_t *)&g_ui_workflows_unknown_03);
        pico_display_center_bitmap(0x2c, (uint32_t *)&g_ui_workflows_unknown_02);
        pico_display_draw_titled_frame(program_title);
        __aeabi_i2d_result = __aeabi_i2d(fahrenheit);
        pico_display_large_temperature(PICO_SUB_U32(__aeabi_i2d_result, 0),
                                       (int)((uint64_t)__aeabi_i2d_result >> 0x20), 0);
        pico_ui_draw_footer_single(footer_text);
        pico_display_flush();
    }
    libc_snprintf(log_message, 0x80, "%s \"%s\" %dF", "getTemperatureInput", program_title,
                  fahrenheit);
    pico_log_serial("I", "UI", log_message);
    return fahrenheit;
}

/* Explicit getTimeInput log, minute range0..595 and step size1 or5.
 * Firmware address: 0x20013694. */
int pico_get_time_input(uint32_t action_id, char *program_title, int minutes)
{
    char *action_label;
    int button_or_increment;
    char footer_text[24];
    char log_message[128];

    action_label =
        pico_action_label(action_id, (uint32_t)g_language_as_uint8_t, (uint32_t)g_detailed_status);
    pico_cstring_copy(footer_text, 0x16, action_label);
    if (minutes < 0x3c) {
        button_or_increment = 1;
    } else {
        button_or_increment = 5;
    }
    pico_encoder_set_step((PicoEncoder *)&g_encoder, button_or_increment);
    pico_encoder_set_range((PicoEncoder *)&g_encoder, 0, 0x253, minutes);
    while (button_or_increment = pico_encoder_take_click((PicoEncoder *)&g_encoder),
           button_or_increment == 0) {
        minutes = pico_encoder_get_position((PicoEncoder *)&g_encoder);
        if (minutes < 0x3c) {
            button_or_increment = 1;
        } else {
            button_or_increment = 5;
        }
        pico_encoder_set_step((PicoEncoder *)&g_encoder, button_or_increment);
        pico_display_clear_thunk();
        pico_display_center_bitmap(0xb, (uint32_t *)&g_ui_workflows_unknown_03);
        pico_display_center_bitmap(0x2c, (uint32_t *)&g_ui_workflows_unknown_02);
        pico_display_draw_titled_frame(program_title);
        pico_display_large_timer(minutes, 0, 0);
        pico_ui_draw_footer_single(footer_text);
        pico_display_flush();
    }
    pico_encoder_set_step((PicoEncoder *)&g_encoder, 1);
    libc_snprintf(log_message, 0x80, "%s \"%s\" %d min", "getTimeInput", program_title, minutes);
    pico_log_serial("I", "UI", log_message);
    return minutes;
}
