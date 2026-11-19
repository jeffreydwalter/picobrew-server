#include "ui_internal.h"

void pico_label_insert_step_filter(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Introduzca el filtro";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Inserez le filtre";
    } else {
        label = "Insert Step Filter";
    }
    strcpy(destination, label);
    return;
}

void pico_label_adjust_recipe(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Ajuste la receta";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Ajuster la recette";
    } else {
        label = "Adjust Recipe";
    }
    strcpy(destination, label);
    return;
}

void pico_label_done(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Hecho";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Termine";
    } else {
        label = "Done";
    }
    strcpy(destination, label);
    return;
}

void pico_label_confirm(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Confirmar";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Confirmer";
    } else {
        label = "Confirm";
    }
    strcpy(destination, label);
    return;
}

void pico_label_continue(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Continuar";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Termine";
    } else {
        label = "Continue";
    }
    strcpy(destination, label);
    return;
}

void pico_label_exit(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Salida";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Quitter";
    } else {
        label = "Exit";
    }
    strcpy(destination, label);
    return;
}

void pico_label_back(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "De Vuelta";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Retour";
    } else {
        label = "Back";
    }
    strcpy(destination, label);
    return;
}

void pico_label_ok(char *destination)
{
    char *source;

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "Esta bien";
    } else {
        source = "OK";
    }
    strcpy(destination, source);
    return;
}

void pico_label_yes(char *destination)
{
    char *source;

    if (pico_ui_active->network->settings->language == '\x01') {
        source = "Si";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        source = "Oui";
    } else {
        source = "Yes";
    }
    strcpy(destination, source);
    return;
}

void pico_label_no(char *destination)
{
    char *source;

    if (pico_ui_active->network->settings->language == '\x02') {
        source = "Non";
    } else {
        source = "No";
    }
    strcpy(destination, source);
    return;
}

void pico_label_compartment(char *destination, uint32_t compartment)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        switch (compartment) {
        case 1:
            label = "Macerado";
            break;
        case 2:
            label = "Calentar Agua";
            break;
        case 3:
            label = "Adjunto 1";
            break;
        case 4:
            label = "Adjunto 2";
            break;
        case 5:
            label = "Adjunto 4";
            break;
        case 6:
            label = "Adjunto 3";
            break;
        default:
            goto use_reservoir_label;
        }
        goto copy_label;
    }
    if (pico_ui_active->network->settings->language == '\x02') {
        switch (compartment) {
        case 1:
            label = "Brassin";
            break;
        case 2:
            label = "Chauffer l\'eau";
            break;
        case 3:
            label = "Ajout 1";
            break;
        case 4:
            label = "Ajout 2";
            break;
        case 5:
            label = "Ajout 4";
            break;
        case 6:
            label = "Ajout 3";
            break;
        default:
            goto use_reservoir_label;
        }
    } else {
        switch (compartment) {
        case 1:
            label = "Mash";
            break;
        case 2:
            label = "Heat Water";
            break;
        case 3:
            label = "Adjunct 1";
            break;
        case 4:
            label = "Adjunct 2";
            break;
        case 5:
            label = "Adjunct 4";
            break;
        case 6:
            label = "Adjunct 3";
            break;
        default:
        use_reservoir_label:
            label = "Reservoir";
        }
    }
copy_label:
    strcpy(destination, label);
    return;
}

void pico_label_service(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "DIAGNOSTICOS";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "DIAGNOSIQUES";
    } else {
        label = "SERVICE";
    }
    strcpy(destination, label);
    return;
}

void pico_label_heating(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "CALENTAMIENTO";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "CHAUFFAGE";
    } else {
        label = "HEATING";
    }
    strcpy(destination, label);
    return;
}

void pico_label_cooling(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "ENFRIAMIENTO";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "REFROIDISSEMENT";
    } else {
        label = "COOLING";
    }
    strcpy(destination, label);
    return;
}

void pico_label_holding(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "MANTENER";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "MAINTIEN";
    } else {
        label = "HOLDING";
    }
    strcpy(destination, label);
    return;
}

void pico_label_draining(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "DRENAR";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "VIDANGE";
    } else {
        label = "DRAINING";
    }
    strcpy(destination, label);
    return;
}

void pico_label_start_brewing(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Comienzo";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Commencer";
    } else {
        label = "Start Brewing";
    }
    strcpy(destination, label);
    return;
}

void pico_label_cancel(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "Cancele";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "Annuler";
    } else {
        label = "Cancel";
    }
    strcpy(destination, label);
    return;
}

void pico_label_next_uppercase(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "SIGUIENTE";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "PROCHAIN";
    } else {
        label = "NEXT";
    }
    strcpy(destination, label);
    return;
}

void pico_label_exit_uppercase(char *destination)
{
    char *label;

    if (pico_ui_active->network->settings->language == '\x01') {
        label = "SALIDA";
    } else if (pico_ui_active->network->settings->language == '\x02') {
        label = "QUITTER";
    } else {
        label = "EXIT";
    }
    strcpy(destination, label);
    return;
}
