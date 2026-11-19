/* Pico Pro 0.1.34 — screens. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Draws ERROR in large text and an error instruction line.
 * Firmware address: 0x20005324. */
void pico_screen_error_heading(void)
{
    GFX_set_text_size(&g_display, '\x02');
    pico_display_center_text(2, "ERROR");
    GFX_set_text_size(&g_display, '\x01');
    pico_display_center_text(5, "<Silence Alarm>");
    return;
}

/* Localized error instructions to fill water and contact support.
 * Firmware address: 0x20005358. */
void pico_screen_water_error_help(void)
{
    short last_row;
    char *last_line;

    if (g_language == '\x01') {
        pico_display_center_text(1, "ERROR: Llene con agua");
        pico_display_center_text(2, "el reservorio si esta");
        pico_display_center_text(3, "vacio, de lo");
        pico_display_center_text(4, "contrario seleccione");
        pico_display_center_text(5, "salida y contacte");
        last_row = 6;
        last_line = "a soporte";
    } else if (g_language == '\x02') {
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

/* Displays logo/version, device UID and formatted MAC, then waits for click.
 * Firmware address: 0x20005d38. */
void pico_screen_device_information(void)
{
    char *source;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;
    int mac_index;
    uint32_t mac_byte;
    uint8_t mac_address[8];
    char hex_buffer[12];
    char line[152];

    libc_memset(line, 0, 0x96);
    WiFi_get_mac_address(&global_wifi, (uint32_t *)mac_address, unresolved_return_r2,
                         unresolved_return_r3);
    pico_display_clear();
    pico_display_logo();
    pico_copy_firmware_version(line);
    pico_display_center_text(3, line);
    libc_strcpy(line, "ID:");
    libc_strcat(line, &g_device_uid);
    mac_index = 0;
    pico_display_text_at_cell(5, 0, line);
    line[0] = '\0';
    do {
        mac_byte = (uint32_t)mac_address[mac_index];
        if (mac_byte < 0x10) {
            libc_strcat(line, "0");
        }
        source = itoa(mac_byte, hex_buffer, 0x10);
        libc_strcat(line, source);
        if (mac_index != 5) {
            libc_strcat(line, ":");
        }
        mac_index = mac_index + 1;
    } while (mac_index != 6);
    pico_display_center_text(7, line);
    pico_display_flush();
    pico_display_wait_for_click();
    return;
}

/* Clears screen, centers status message and enables progress animation.
 * Firmware address: 0x20006494. */
void pico_screen_still_connecting(char *status_text)
{
    pico_display_clear();
    pico_display_center_text(2, status_text);
    pico_display_flush();
    g_progress_indicator = 3;
    g_progress_start_column = 5;
    g_progress_end_column = 0x10;
    g_progress_glyph_index = 0;
    g_progress_active = 1;
    return;
}

/* Displays Exiting Still and enables progress animation.
 * Firmware address: 0x200065ec. */
void pico_screen_still_exiting(void)
{
    pico_display_clear();
    pico_display_center_text(2, "Exiting Still");
    g_progress_indicator = 3;
    g_progress_start_column = 5;
    g_progress_end_column = 0x10;
    g_progress_glyph_index = 0;
    g_progress_active = 1;
    return;
}

/* Localized Searching for WiFi networks text.
 * Firmware address: 0x200069c4. */
void pico_screen_wifi_searching(void)
{
    char *last_line;

    if (g_language == '\x01') {
        pico_display_string_at_cell(2, 10, "la busqueda");
        pico_display_string_at_cell(3, 10, "de  redes");
        last_line = "WiFi";
    } else if (g_language == '\x02') {
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

/* Localized Ending Process message and flush.
 * Firmware address: 0x20006ae4. */
void pico_screen_ending_process(void)
{
    char *message;

    pico_display_clear();
    if (g_language == '\x01') {
        message = "Fin del proceso";
    } else if (g_language == '\x02') {
        message = "Fin du processus";
    } else {
        message = "Ending Process";
    }
    pico_display_center_text(2, message);
    pico_display_flush();
    return;
}

/* Localized preheating text with210F/99C based on units.
 * Firmware address: 0x20006b4c. */
void pico_screen_preheating_steam(short row)
{
    char *temperature_line;

    if (g_language == '\x01') {
        pico_display_center_text(row, "Precalentamiento");
        if (g_temperature_unit_celsius == '\0') {
            temperature_line = "vapor - 210F";
        } else {
            temperature_line = "vapor - 99C";
        }
    } else if (g_language == '\x02') {
        pico_display_center_text(row, "Prechauffage");
        if (g_temperature_unit_celsius == '\0') {
            temperature_line = "vapeur - 210F";
        } else {
            temperature_line = "vapeur - 99C";
        }
    } else {
        pico_display_center_text(row, "Preheating");
        if (g_temperature_unit_celsius == '\0') {
            temperature_line = "steam - 210F";
        } else {
            temperature_line = "steam - 99C";
        }
    }
    pico_display_center_text(row + 1, temperature_line);
    return;
}

/* Localized Loading optionally PicoPak, with progress animation.
 * Firmware address: 0x20006c0c. */
bool pico_screen_loading(int is_picopak)
{
    bool int_master_enable_result;
    char *loading_label;
    char message[24];

    if (g_language == '\x01') {
        loading_label = "Cargando";
    } else if (g_language == '\x02') {
        loading_label = "Remplissage";
    } else {
        loading_label = "Loading";
    }
    libc_strcpy(message, loading_label);
    if (is_picopak != 0) {
        libc_strcat(message, " PicoPak");
    }
    IntMasterDisable();
    pico_display_center_text(3, message);
    pico_display_flush();
    g_progress_indicator = 4;
    g_progress_start_column = 2;
    g_progress_end_column = 0x13;
    g_progress_glyph_index = 0;
    g_progress_active = 1;
    int_master_enable_result = IntMasterEnable();
    return int_master_enable_result;
}

/* Localized Updating firmware message.
 * Firmware address: 0x20006c80. */
void pico_screen_updating_firmware(void)
{
    short last_row;
    char *last_line;

    last_row = 4;
    if (g_language == '\x01') {
        pico_display_center_text(4, "Actualizacion");
        last_row = 5;
        last_line = "del firmware";
    } else if (g_language == '\x02') {
        pico_display_center_text(4, "Mise a jour du");
        last_line = "micrologiciel";
        last_row = 5;
    } else {
        last_line = "Updating firmware";
    }
    pico_display_center_text(last_row, last_line);
    return;
}

/* Localized Loading new firmware message.
 * Firmware address: 0x20006ccc. */
void pico_screen_downloading_firmware(void)
{
    short last_row;
    char *last_line;

    last_row = 4;
    if (g_language == '\x01') {
        pico_display_center_text(4, "Cargando nuevo");
        last_row = 5;
        last_line = "firmware";
    } else if (g_language == '\x02') {
        pico_display_center_text(4, "Telechargement du");
        last_line = "nouveau micrologiciel";
        last_row = 5;
    } else {
        last_line = "Loading new firmware";
    }
    pico_display_center_text(last_row, last_line);
    return;
}

/* Localized firmware update failure message.
 * Firmware address: 0x20006d18. */
void pico_screen_firmware_update_failed(void)
{
    char *last_line;

    if (g_language == '\x01') {
        pico_display_center_text(4, "Actualizacion");
        last_line = "fallida del firmware";
    } else if (g_language == '\x02') {
        pico_display_center_text(4, "Echec de la mise a");
        last_line = "jour du micrologiciel";
    } else {
        pico_display_center_text(4, "Updating firmware");
        last_line = "failed";
    }
    pico_display_center_text(5, last_line);
    return;
}

/* Localized Rinse complete/Happy Pico brewing message.
 * Firmware address: 0x20006d70. */
void pico_screen_rinse_complete(void)
{
    short last_row;
    char *last_line;

    last_row = 3;
    if (g_language == '\x01') {
        last_line = "Lave completamente";
    } else if (g_language == '\x02') {
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

/* Localized Go to/Consultez prefix and picobrew.com/help.
 * Firmware address: 0x20006dbc. */
void pico_screen_help_address(void)
{
    short url_row;
    char *text;

    if (g_language == '\x01') {
        text = "Contacte";
    } else {
        if (g_language != '\x02') {
            pico_display_center_text(4, "Go to");
            url_row = 5;
            goto branch_1;
        }
        text = "Consultez";
    }
    pico_display_center_text(3, text);
    url_row = 4;
branch_1:
    pico_display_center_text(url_row, "picobrew.com/help");
    pico_display_flush();
    return;
}

/* Clears, displays help address, flushes and waits5seconds.
 * Firmware address: 0x20006e08. */
void pico_screen_show_help_temporarily(void)
{
    pico_display_clear();
    pico_screen_help_address();
    pico_display_flush();
    delay(5000);
    return;
}

/* Localized Ending First-Rinse with logo and flush.
 * Firmware address: 0x20006e24. */
void pico_screen_ending_first_rinse(void)
{
    char *message;

    pico_display_clear();
    pico_display_logo();
    if (g_language == '\x01') {
        message = "Finalizacion lavado";
    } else if (g_language == '\x02') {
        message = "Fin du nettoyage";
    } else {
        message = "Ending First-Rinse";
    }
    pico_display_center_text(4, message);
    pico_display_flush();
    return;
}

/* Explicit getScreenIdLines diagnostics and localized instruction selection by screen ID.
 * Firmware address: 0x20006e64. */
void pico_screen_get_instruction_lines(PicoTextScreen *screen, uint32_t argument_2,
                                       uint32_t argument_3)
{
    uint32_t value_1;
    char *first_line;
    char *second_line;
    PicoTextScreen *pointer_4;
    char *text_5;
    uint32_t value_6;
    char *text_7;
    uint32_t value_8;

    if (0x18 < argument_2) {
        pico_log_serial("E", "SCREEN", "getScreenIdLines() invalid ScreenId arg");
        return;
    }
    pointer_4 = screen;
    value_6 = argument_2;
    value_8 = argument_3;
    if (argument_3 < 3) {
        if (argument_2 == 0) {
            if (argument_3 == 1) {
                first_line = "Introduzca el filtro";
                second_line = "vacio y la tapa";
                value_1 = 2;
                goto branch_1;
            }
            if (argument_3 == 2) {
                first_line = "Inserez le filtre";
                second_line = "avec son couvercle";
                value_1 = argument_3;
                goto branch_1;
            }
            if (argument_3 != 0) {
                return;
            }
            goto branch_2;
        }
    } else {
        pico_log_serial("E", "SCREEN", "getScreenIdLines() invalid language arg");
        argument_3 = value_8;
        if (argument_2 == 0) {
        branch_2:
            first_line = "Insert empty";
            second_line = "Step Filter & Lid";
            value_1 = 2;
            argument_2 = value_6;
            goto branch_1;
        }
        argument_3 = 0;
    }
    if (argument_2 == 1) {
        first_line = "Insert step filter";
        second_line = "and lid";
        value_1 = 2;
        argument_2 = value_6;
        argument_3 = value_8;
    } else {
        if (argument_2 == 2) {
            first_line = "Remove step filter";
            goto branch_3;
        }
        if (argument_2 == 3) {
            first_line = "Empty and clean";
            second_line = "step filter";
            value_1 = 2;
            argument_2 = value_6;
            argument_3 = value_8;
        } else {
            if (argument_2 == 4) {
                return;
            }
            if (argument_2 == 7) {
                first_line = "Fill step filter";
                second_line = "with 5L of water";
                value_1 = 2;
                argument_2 = value_6;
                argument_3 = value_8;
            } else {
                if (argument_2 == 5) {
                    text_5 = "in step filter";
                    first_line = "Place Brew Bag with";
                    second_line = "coffee and 4.5L water";
                    goto branch_4;
                }
                value_1 = argument_3;
                if (argument_2 == 8) {
                    if (argument_3 == 1) {
                        first_line = "Agregue 5QT. de agua";
                        second_line = "al barril y conecte";
                        value_1 = 2;
                        argument_2 = value_6;
                        argument_3 = value_8;
                    } else if (argument_3 == 2) {
                        first_line = "Ajoutez 5 QT. d\'eau";
                        second_line = "dans le fut. Connectez";
                        argument_2 = value_6;
                        argument_3 = value_8;
                    } else {
                        if (argument_3 != 0) {
                            return;
                        }
                        first_line = "Add 5 QUARTS water";
                        second_line = "to Keg and connect";
                        value_1 = 2;
                        argument_2 = value_6;
                        argument_3 = value_8;
                    }
                } else if (argument_2 == 9) {
                    if (argument_3 == 1) {
                        first_line = "Introduzca el";
                        second_line = "adaptor Sous Vide";
                        value_1 = 2;
                        argument_2 = value_6;
                        argument_3 = value_8;
                    } else if (argument_3 == 2) {
                        first_line = "Inserez l\'adaptateur";
                        second_line = "Sous Vide";
                        argument_2 = value_6;
                        argument_3 = value_8;
                    } else {
                        if (argument_3 != 0) {
                            return;
                        }
                        first_line = "Insert";
                        second_line = "Sous Vide Adapter";
                        value_1 = 2;
                        argument_2 = value_6;
                        argument_3 = value_8;
                    }
                } else if (argument_2 == 10) {
                    if (argument_3 == 1) {
                        first_line = "Agregue la comida";
                        second_line = "y cierre la tapa";
                        value_1 = 2;
                        argument_2 = value_6;
                        argument_3 = value_8;
                    } else if (argument_3 == 2) {
                        first_line = "Ajouter les aliments.";
                        second_line = "Fermer le couvercle";
                        argument_2 = value_6;
                        argument_3 = value_8;
                    } else {
                        if (argument_3 != 0) {
                            return;
                        }
                        first_line = "Add food & close lid";
                        second_line = "Insert Step Filter";
                        value_1 = 2;
                        argument_2 = value_6;
                        argument_3 = value_8;
                    }
                } else {
                    if (argument_2 == 0xb) {
                        return;
                    }
                    if (argument_2 == 0xc) {
                        first_line = "Connect IN & OUT";
                        second_line = "ball-locks to keg";
                        value_1 = 2;
                        argument_2 = value_6;
                        argument_3 = value_8;
                    } else {
                        if (argument_2 != 0xd) {
                            if (argument_2 == 0xe) {
                                if (argument_3 != 1) {
                                    if (argument_3 == 2) {
                                        first_line = "Deconnectez";
                                        second_line = "les tuyaux";
                                        argument_2 = value_6;
                                        argument_3 = value_8;
                                        goto branch_1;
                                    }
                                    if (argument_3 != 0) {
                                        return;
                                    }
                                }
                                first_line = "Disconnect Hoses";
                                goto branch_3;
                            }
                            if (argument_2 == 0xf) {
                                text_5 = "place in container";
                                first_line = "Connect keg wand to";
                                second_line = "GRAY ball-lock, and";
                            branch_4:
                                pico_screen_add_lines(screen, 3, first_line, second_line, text_5,
                                                      value_6, value_8);
                                return;
                            }
                            if (argument_2 == 0x10) {
                                first_line = "Remove blue inline";
                                second_line = "filter screen";
                                value_1 = 2;
                                argument_2 = value_6;
                                argument_3 = value_8;
                                goto branch_1;
                            }
                            if (argument_2 - 0x11 < 3) {
                                return;
                            }
                            if (argument_2 == 0x14) {
                                first_line = "Step Ended";
                                goto branch_3;
                            }
                            if (argument_2 == 0x15) {
                                if (argument_3 == 1) {
                                    text_5 = "agua y conecte las";
                                    text_7 = "varas del barril";
                                    first_line = "Llene el filtro de";
                                    second_line = "paso con 1 pulgada de";
                                } else {
                                    if (argument_3 != 2) {
                                        if (argument_3 != 0) {
                                            return;
                                        }
                                        text_5 = "attach keg wands";
                                        first_line = "Fill step filter";
                                        second_line = "with 1\" water and";
                                        goto branch_4;
                                    }
                                    text_5 = "attacher les batons";
                                    text_7 = "a fut";
                                    first_line = "Remplir le filtre";
                                    second_line = "de 2.5cm d\'eau et";
                                }
                            } else {
                                if (argument_2 != 0x16) {
                                    if (argument_2 == 0x17) {
                                        if (argument_3 == 1) {
                                            first_line = &g_screens_unknown_02;
                                            second_line = "de drenaje";
                                            value_1 = 2;
                                            argument_2 = value_6;
                                            argument_3 = value_8;
                                        } else {
                                            if (argument_3 != 2) {
                                                if (argument_3 != 0) {
                                                    return;
                                                }
                                                first_line = "Clearing drain line";
                                                goto branch_3;
                                            }
                                            first_line = "Nettoyage de la";
                                            second_line = "ligne de drainage";
                                            argument_2 = value_6;
                                            argument_3 = value_8;
                                        }
                                    } else {
                                        if (argument_2 != 0x18) {
                                            return;
                                        }
                                        if (argument_3 == 1) {
                                            first_line = "Lave completamente";
                                        branch_3:
                                            pico_screen_add_lines(screen, 1, first_line);
                                            return;
                                        }
                                        if (argument_3 == 2) {
                                            first_line = "Rinsage fini";
                                            second_line = "Bon brassage Pico!";
                                            argument_2 = value_6;
                                            argument_3 = value_8;
                                        } else {
                                            if (argument_3 != 0) {
                                                return;
                                            }
                                            first_line = "Rinse complete";
                                            second_line = "Happy Pico brewing!";
                                            value_1 = 2;
                                            argument_2 = value_6;
                                            argument_3 = value_8;
                                        }
                                    }
                                    goto branch_1;
                                }
                                if (argument_3 == 1) {
                                    text_5 = "barril en el conten-";
                                    text_7 = "edor de desechos";
                                    first_line = "Coloque la manguera";
                                    second_line = "GRIS con la vara del";
                                } else {
                                    if (argument_3 != 2) {
                                        if (argument_3 != 0) {
                                            return;
                                        }
                                        text_5 = "waste container";
                                        first_line = "Place GRAY hose";
                                        second_line = "with keg wand in";
                                        goto branch_4;
                                    }
                                    text_5 = "dans le contenant";
                                    text_7 = "a dechets";
                                    first_line = "Placez le tuyau GRIS";
                                    second_line = "avec le baton a fut";
                                }
                            }
                            pico_screen_add_lines(screen, 4, first_line, second_line, text_5,
                                                  text_7, value_8);
                            return;
                        }
                        if (argument_3 != 1) {
                            if (argument_3 == 2) {
                                first_line = "Connectez les";
                                second_line = "tuyaux ensemble";
                                argument_2 = value_6;
                                argument_3 = value_8;
                                goto branch_1;
                            }
                            if (argument_3 != 0) {
                                return;
                            }
                        }
                        first_line = "Connect Hoses using";
                        second_line = "Keg Adapter Kit";
                        value_1 = 2;
                        argument_2 = value_6;
                        argument_3 = value_8;
                    }
                }
            }
        }
    }
branch_1:
    pico_screen_add_lines(screen, value_1, first_line, second_line, pointer_4, argument_2,
                          argument_3);
    return;
}

/* Four setup steps: water/filter/hose/reservoir instructions for PicoStill.
 * Firmware address: 0x200072ac. */
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

/* Obtains localized program notice label index3 and draws title frame.
 * Firmware address: 0x20007358. */
void pico_screen_draw_program_notice(void)
{
    char *title;

    pico_display_clear();
    title = pico_title_label(3, (uint32_t)g_language_as_uint8_t);
    pico_display_draw_titled_frame(title);
    return;
}

/* Adds localized notice label index4 on row3 and flushes.
 * Firmware address: 0x20007374. */
void pico_screen_show_program_notice(void)
{
    char *message;

    pico_screen_draw_program_notice();
    message = pico_title_label(4, (uint32_t)g_language_as_uint8_t);
    pico_display_center_text(3, message);
    pico_display_flush();
    return;
}

/* Localized ERROR/Reinsert StepFilter screen.
 * Firmware address: 0x20007398. */
void pico_screen_reinsert_step_filter(void)
{
    char *last_line;

    pico_display_clear();
    if (g_language == '\x01') {
        pico_display_draw_titled_frame("ERROR");
        pico_display_center_text(3, "Reintroduzca el");
        last_line = "filtro de paso";
    } else if (g_language == '\x02') {
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

/* Localized Changing step message beneath current recipe title.
 * Firmware address: 0x20007410. */
void pico_screen_changing_step(void)
{
    char *message;

    pico_display_clear();
    if (g_language == '\x01') {
        message = "Cambiar paso";
    } else if (g_language == '\x02') {
        message = "Changement d\'etape";
    } else {
        message = "Changing step";
    }
    pico_display_center_text(3, message);
    pico_display_draw_titled_frame(&g_screen_recipe_name);
    return;
}

/* Localized Setting up step and supplied title.
 * Firmware address: 0x20007454. */
void pico_screen_setting_up_step(char *title)
{
    char *message;

    pico_display_clear();
    if (g_language == '\x01') {
        message = "Configurar paso";
    } else if (g_language == '\x02') {
        message = "Mettre en place";
    } else {
        message = "Setting up step";
    }
    pico_display_center_text(3, message);
    pico_display_draw_titled_frame(title);
    return;
}

/* Localized Ending step and supplied title.
 * Firmware address: 0x20007494. */
void pico_screen_ending_step(char *title)
{
    char *message;

    pico_display_clear();
    if (g_language == '\x01') {
        message = "Finalizar paso";
    } else if (g_language == '\x02') {
        message = "Terminer l\'etape";
    } else {
        message = "Ending step";
    }
    pico_display_center_text(3, message);
    pico_display_draw_titled_frame(title);
    return;
}

/* Displays PicoStill T1/T2/T3 temperatures, pressure, fan and target; converts units.
 * Firmware address: 0x20007698. */
void pico_screen_still_readings(void)
{
    double number_1;
    double left;
    double left_00;
    uint32_t value_2;
    uint32_t value_3;
    uint32_t value_4;
    uint32_t value_5;
    uint32_t value_6;
    uint32_t value_7;
    char byte_value_8;
    char byte_value_9;
    int __aeabi_dcmpeq_result;
    char *itoa_result;
    float __aeabi_d2f_result;
    double temperature2_bits;
    double value;
    double __aeabi_dmul_result;
    double __aeabi_dsub_result;
    uint64_t __aeabi_ddiv_result;
    char unit_text[4];
    char number_text[12];
    char target_line[24];
    char reading_line[28];

    byte_value_9 = g_temperature_unit_celsius;
    byte_value_8 = g_still_fan_on;
    value_7 = g_still_pressure_high;
    value_6 = g_still_pressure_low;
    value_5 = g_still_temperature3_high;
    value_4 = g_still_temperature3_low;
    value_3 = g_still_temperature2_high;
    value_2 = g_still_temperature2_low;
    value = (double)pico_join_u32_u32(g_still_temperature1_high, g_still_temperature1_low);
    temperature2_bits =
        (double)pico_join_u32_u32(g_still_temperature2_high, g_still_temperature2_low);
    __aeabi_dmul_result =
        (double)pico_join_u32_u32(g_still_temperature3_high, g_still_temperature3_low);
    if (g_still_temperature_units != g_temperature_unit_celsius) {
        if (g_temperature_unit_celsius == '\0') {
            temperature2_bits = __aeabi_dmul(
                (double)pico_join_u32_u32(g_still_temperature1_high, g_still_temperature1_low),
                1.8);
            value = __aeabi_dadd(temperature2_bits, 32.0);
            temperature2_bits = __aeabi_dmul((double)pico_join_u32_u32(value_3, value_2), 1.8);
            temperature2_bits = __aeabi_dadd(temperature2_bits, 32.0);
            __aeabi_dmul_result = __aeabi_dmul((double)pico_join_u32_u32(value_5, value_4), 1.8);
            __aeabi_dmul_result = __aeabi_dadd(__aeabi_dmul_result, 32.0);
        } else {
            temperature2_bits = __aeabi_dsub(
                (double)pico_join_u32_u32(g_still_temperature1_high, g_still_temperature1_low),
                32.0);
            value = __aeabi_ddiv(temperature2_bits, 1.8);
            temperature2_bits = __aeabi_dsub((double)pico_join_u32_u32(value_3, value_2), 32.0);
            temperature2_bits = __aeabi_ddiv(temperature2_bits, 1.8);
            __aeabi_dmul_result = __aeabi_dsub((double)pico_join_u32_u32(value_5, value_4), 32.0);
            __aeabi_dmul_result = __aeabi_ddiv(__aeabi_dmul_result, 1.8);
        }
    }
    __aeabi_ddiv_result = (double)pico_join_u32_u32(g_wort_temperature_fahrenheit_high,
                                                    g_wort_temperature_fahrenheit_low);
    __aeabi_dsub_result =
        (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low);
    number_1 =
        (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low);
    left_00 =
        (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low);
    left = (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low);
    if (byte_value_9 == '\0') {
        libc_strcpy(unit_text, "F");
    } else {
        libc_strcpy(unit_text, "C");
        __aeabi_dsub_result = __aeabi_dsub(__aeabi_ddiv_result, 32.0);
        __aeabi_ddiv_result = __aeabi_ddiv(__aeabi_dsub_result, 1.8);
        __aeabi_dcmpeq_result = __aeabi_dcmpeq(left, 0.0);
        __aeabi_dsub_result = number_1;
        if (__aeabi_dcmpeq_result == 0) {
            __aeabi_dsub_result = __aeabi_dsub(left_00, 32.0);
            __aeabi_dsub_result = __aeabi_ddiv(__aeabi_dsub_result, 1.8);
        }
    }
    __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_ddiv_result);
    itoa_result = itoa(__aeabi_dcmpeq_result, number_text, 10);
    libc_strcpy(target_line, itoa_result);
    libc_strcat(target_line, unit_text);
    if (g_language == '\x01') {
        itoa_result = " ->OBJETIVO:";
    } else if (g_language == '\x02') {
        itoa_result = " ---> CIBLE:";
    } else {
        itoa_result = " --> TARGET:";
    }
    libc_strcat(target_line, itoa_result);
    __aeabi_dcmpeq_result = __aeabi_d2iz(__aeabi_dsub_result);
    itoa_result = itoa(__aeabi_dcmpeq_result, number_text, 10);
    libc_strcat(target_line, itoa_result);
    libc_strcat(target_line, unit_text);
    pico_display_center_text(2, "Still Readings");
    libc_strcpy(reading_line, "T1:");
    __aeabi_d2f_result = __aeabi_d2f(value);
    itoa_result = pico_format_float_decimal(__aeabi_d2f_result, number_text, 1);
    libc_strcat(reading_line, itoa_result);
    libc_strcat(reading_line, unit_text);
    libc_strcat(reading_line, "  T2:");
    __aeabi_d2f_result = __aeabi_d2f(temperature2_bits);
    itoa_result = pico_format_float_decimal(__aeabi_d2f_result, number_text, 1);
    libc_strcat(reading_line, itoa_result);
    libc_strcat(reading_line, unit_text);
    pico_display_center_text(3, reading_line);
    libc_strcpy(reading_line, "T3:");
    __aeabi_d2f_result = __aeabi_d2f(__aeabi_dmul_result);
    itoa_result = pico_format_float_decimal(__aeabi_d2f_result, number_text, 1);
    libc_strcat(reading_line, itoa_result);
    libc_strcat(reading_line, unit_text);
    libc_strcat(reading_line, " ");
    __aeabi_d2f_result = __aeabi_d2f((double)pico_join_u32_u32(value_7, value_6));
    itoa_result = pico_format_float_decimal(__aeabi_d2f_result, number_text, 2);
    libc_strcat(reading_line, itoa_result);
    libc_strcat(reading_line, "psi");
    pico_display_center_text(4, reading_line);
    itoa_result = "Fan: OFF";
    if (byte_value_8 != '\0') {
        itoa_result = "Fan: ON";
    }
    libc_strcpy(reading_line, itoa_result);
    pico_display_center_text(5, reading_line);
    pico_display_center_text(7, target_line);
    pico_display_draw_titled_frame(&g_screen_recipe_name);
    return;
}

/* Builds Start/Exit choices and distilled-water reservoir instruction.
 * Firmware address: 0x20008510. */
uint32_t pico_screen_fill_reservoir_and_start(char *title)
{
    char *localized_line;

    pico_display_clear();
    pico_display_draw_titled_frame(title);
    if (g_language == '\x01') {
        localized_line = "Inicio ";
    } else if (g_language == '\x02') {
        localized_line = "Demarrer ";
    } else {
        localized_line = "Start ";
    }
    libc_strcpy(&g_menu_label_0, localized_line);
    pico_append_program_name(&g_menu_label_0);
    pico_label_exit(&g_menu_label_1);
    if (g_language == '\x01') {
        pico_display_center_text(3, "Llenar el reservorio");
        localized_line = "con agua destilada";
    } else if (g_language == '\x02') {
        pico_display_center_text(3, "Remplissez reservoir");
        localized_line = "d\'eau distillee";
    } else {
        pico_display_center_text(3, "Fill Reservoir with");
        localized_line = "distilled water";
    }
    pico_display_center_text(4, localized_line);
    return 2;
}

/* Localized Starting plus active program verb label and supplied title.
 * Firmware address: 0x200085b0. */
void pico_screen_starting_program(char *title)
{
    char *starting_prefix;
    char message[24];

    pico_display_clear();
    pico_display_draw_titled_frame(title);
    if (g_language == '\x01') {
        starting_prefix = "Iniciar ";
    } else if (g_language == '\x02') {
        starting_prefix = "Demarrer ";
    } else {
        starting_prefix = "Starting ";
    }
    libc_strcpy(message, starting_prefix);
    pico_append_program_name(message);
    pico_display_center_text(3, message);
    return;
}

/* Localized Ending plus active program verb label and supplied title.
 * Firmware address: 0x20008600. */
void pico_screen_ending_program(char *title)
{
    char *ending_prefix;
    char message[24];

    pico_display_clear();
    if (g_language == '\x01') {
        ending_prefix = "Finalizar ";
    } else if (g_language == '\x02') {
        ending_prefix = "Terminer ";
    } else {
        ending_prefix = "Ending ";
    }
    libc_strcpy(message, ending_prefix);
    pico_append_program_name(message);
    pico_display_center_message(message);
    pico_display_draw_titled_frame(title);
    return;
}

/* Clears, draws ending-program screen and flushes.
 * Firmware address: 0x2000864c. */
void pico_screen_show_ending_program(char *title)
{
    pico_display_clear();
    pico_screen_ending_program(title);
    pico_display_flush();
    return;
}

/* Tail wrapper selects Loading PicoPak mode.
 * Firmware address: 0x200086f4. */
bool pico_screen_loading_picopak(void)
{
    bool screen_loading_result;

    screen_loading_result = pico_screen_loading(1);
    return screen_loading_result;
}

/* Draws128x30 bitmap and firmware version aligned at bottom right.
 * Firmware address: 0x20009588. */
void pico_screen_splash(void)
{
    uint32_t version_length;
    char buffer_20[32];

    GFX_draw_bitmap(&g_display, 0, 0x10, "", 0x80, 0x1e, 1);
    pico_display_clear_row(7);
    pico_copy_firmware_version(buffer_20);
    version_length = libc_strlen(buffer_20);
    pico_display_text_at_cell(7,
                              (uint16_t)(0x15 - version_length) &
                                  ~(uint16_t)((int)(0x15 - version_length) >> 0x1f) & 0xff,
                              buffer_20);
    pico_display_flush();
    return;
}

/* Displays localized SERVICE heading on status row6.
 * Firmware address: 0x20009658. */
void pico_screen_service_status(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3,
                                uint32_t argument_4)
{
    uint32_t service_label;
    uint32_t value;
    uint32_t value_18;
    uint32_t value_14;

    service_label = argument_1;
    value = argument_2;
    value_18 = argument_3;
    value_14 = argument_4;
    pico_label_service((char *)&service_label);
    pico_display_clear_row(6);
    pico_display_center_text(6, (char *)&service_label);
    return;
}

/* Prepares localized No WiFi signal found text after clearing status row.
 * Firmware address: 0x200096a0. */
void pico_screen_wifi_no_signal(void)
{
    char *message;
    char buffer_20[16];

    pico_display_clear_row(6);
    if (g_language == '\x01') {
        message = &g_screens_unknown_03;
    } else if (g_language == '\x02') {
        message = "Pas de signal WiFi";
    } else {
        message = "No WiFi signal found";
    }
    libc_strcpy(buffer_20, message);
    pico_display_text_at_cell(6, 0, buffer_20);
    return;
}

/* Localized Connected message on status row6.
 * Firmware address: 0x200096e4. */
void pico_screen_wifi_connected(void)
{
    char *message;

    pico_display_clear_row(6);
    if (g_language == '\x01') {
        message = "Conectado";
    } else if (g_language == '\x02') {
        message = "Connecte";
    } else {
        message = "Connected";
    }
    pico_display_center_text(6, message);
    return;
}

/* Localized Connection failed message on status row6.
 * Firmware address: 0x20009720. */
void pico_screen_wifi_connection_failed(uint32_t argument_1, uint32_t argument_2,
                                        uint32_t argument_3, uint32_t argument_4)
{
    char *message;
    uint32_t message_buffer;
    uint32_t value;
    uint32_t value_18;
    uint32_t value_14;

    message = (char *)&message_buffer;
    message_buffer = argument_1;
    value = argument_2;
    value_18 = argument_3;
    value_14 = argument_4;
    pico_display_clear_row(6);
    if (g_language == '\x01') {
        libc_strcpy(message, &g_screens_unknown_01);
    } else if (g_language == '\x02') {
        message = "Echec de la connexion";
    } else {
        message = "Connection failed";
    }
    pico_display_center_text(6, message);
    return;
}

/* Framed recipe name with NOW BREWING or detailed temperature summary.
 * Firmware address: 0x20009ac8. */
void pico_screen_brew_name_page(void)
{
    pico_display_draw_content_frame();
    pico_display_center_text(3, &g_screen_recipe_name);
    if (g_detailed_status_as_char == '\0') {
        pico_display_center_text(7, "NOW BREWING");
        return;
    }
    pico_display_temperature_summary(
        g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_high,
        g_steam_temperature_fahrenheit_low, g_steam_temperature_fahrenheit_high,
        (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low), 7);
    return;
}

/* Draws recipe decimal/integer metrics and optional temperature summary.
 * Firmware address: 0x20009be4. */
void pico_screen_recipe_metrics_page(void)
{
    pico_display_recipe_decimal_metric();
    pico_display_recipe_integer_metric();
    pico_display_draw_content_frame();
    if (g_detailed_status_as_char != '\0') {
        pico_display_temperature_summary(
            g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_high,
            g_steam_temperature_fahrenheit_low, g_steam_temperature_fahrenheit_high,
            (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low),
            7);
    }
    return;
}

/* Selects progress bitmap by component, draws current step name and optional temperatures.
 * Firmware address: 0x20009c24. */
void pico_screen_step_progress_page(void)
{
    if (g_screen_progress_component_as_int < 8) {
        GFX_draw_bitmap_alias(&g_display, 0, 0x10,
                              (&g_screens_unknown_04)[g_screen_progress_component_as_int], 0x80,
                              0x1e, 1);
    }
    pico_display_draw_titled_frame(&g_recipe_steps + g_current_recipe_step_index_as_int * 0x20);
    if (g_detailed_status_as_char != '\0') {
        pico_display_temperature_summary(
            g_wort_temperature_fahrenheit_low, g_wort_temperature_fahrenheit_high,
            g_steam_temperature_fahrenheit_low, g_steam_temperature_fahrenheit_high,
            (double)pico_join_u16_u48((short)((uint32_t)g_wort_target_fahrenheit_high >> 0x10),
                                      pico_join_u16_u32((short)g_wort_target_fahrenheit_high,
                                                        g_wort_target_fahrenheit_low)),
            7);
    }
    return;
}
