/* Pico Pro 0.1.34 — translations. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Copies localized Insert Step Filter label.
 * Firmware address: 0x20006b20. */
void pico_label_insert_step_filter(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "Introduzca el filtro";
    } else if (g_language == '\x02') {
        label = "Inserez le filtre";
    } else {
        label = "Insert Step Filter";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized Adjust Recipe label.
 * Firmware address: 0x20006be0. */
void pico_label_adjust_recipe(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "Ajuste la receta";
    } else if (g_language == '\x02') {
        label = "Ajuster la recette";
    } else {
        label = "Adjust Recipe";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized Done text.
 * Firmware address: 0x20007e98. */
void pico_label_done(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "Hecho";
    } else if (g_language == '\x02') {
        label = "Termine";
    } else {
        label = "Done";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized Confirm text.
 * Firmware address: 0x20007ec4. */
void pico_label_confirm(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "Confirmar";
    } else if (g_language == '\x02') {
        label = "Confirmer";
    } else {
        label = "Confirm";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies firmware's localized Continue text, including its French Terminé wording.
 * Firmware address: 0x20007ef0. */
void pico_label_continue(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "Continuar";
    } else if (g_language == '\x02') {
        label = "Termine";
    } else {
        label = "Continue";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized Exit text.
 * Firmware address: 0x20007f1c. */
void pico_label_exit(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "Salida";
    } else if (g_language == '\x02') {
        label = "Quitter";
    } else {
        label = "Exit";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized Back text.
 * Firmware address: 0x20008050. */
void pico_label_back(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "De Vuelta";
    } else if (g_language == '\x02') {
        label = "Retour";
    } else {
        label = "Back";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies OK or Spanish Esta bien.
 * Firmware address: 0x20008108. */
void pico_label_ok(char *destination)
{
    char *source;

    if (g_language == '\x01') {
        source = "Esta bien";
    } else {
        source = "OK";
    }
    libc_strcpy(destination, source);
    return;
}

/* Copies localized Yes text.
 * Firmware address: 0x20008124. */
void pico_label_yes(char *destination)
{
    char *source;

    if (g_language == '\x01') {
        source = "Si";
    } else if (g_language == '\x02') {
        source = "Oui";
    } else {
        source = "Yes";
    }
    libc_strcpy(destination, source);
    return;
}

/* Copies localized No text.
 * Firmware address: 0x20008150. */
void pico_label_no(char *destination)
{
    char *source;

    if (g_language == '\x02') {
        source = "Non";
    } else {
        source = "No";
    }
    libc_strcpy(destination, source);
    return;
}

/* Selects Mash/Heat Water/Adjunct1..4/Reservoir label by compartment ID.
 * Firmware address: 0x2000816c. */
void pico_label_compartment(char *destination, uint32_t compartment)
{
    char *label;

    if (g_language == '\x01') {
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
            goto switchD_20008180_default;
        }
        goto branch_1;
    }
    if (g_language == '\x02') {
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
            goto switchD_20008180_default;
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
        switchD_20008180_default:
            label = "Reservoir";
        }
    }
branch_1:
    libc_strcpy(destination, label);
    return;
}

/* Copies localized SERVICE heading.
 * Firmware address: 0x20008378. */
void pico_label_service(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "DIAGNOSTICOS";
    } else if (g_language == '\x02') {
        label = "DIAGNOSIQUES";
    } else {
        label = "SERVICE";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized HEATING status.
 * Firmware address: 0x20008440. */
void pico_label_heating(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "CALENTAMIENTO";
    } else if (g_language == '\x02') {
        label = "CHAUFFAGE";
    } else {
        label = "HEATING";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized COOLING status.
 * Firmware address: 0x2000846c. */
void pico_label_cooling(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "ENFRIAMIENTO";
    } else if (g_language == '\x02') {
        label = "REFROIDISSEMENT";
    } else {
        label = "COOLING";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized HOLDING status.
 * Firmware address: 0x20008498. */
void pico_label_holding(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "MANTENER";
    } else if (g_language == '\x02') {
        label = "MAINTIEN";
    } else {
        label = "HOLDING";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized DRAINING status.
 * Firmware address: 0x200084c4. */
void pico_label_draining(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "DRENAR";
    } else if (g_language == '\x02') {
        label = "VIDANGE";
    } else {
        label = "DRAINING";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized Start Brewing text.
 * Firmware address: 0x20008664. */
void pico_label_start_brewing(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "Comienzo";
    } else if (g_language == '\x02') {
        label = "Commencer";
    } else {
        label = "Start Brewing";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies localized Cancel text.
 * Firmware address: 0x20008690. */
void pico_label_cancel(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "Cancele";
    } else if (g_language == '\x02') {
        label = "Annuler";
    } else {
        label = "Cancel";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies uppercase localized NEXT label.
 * Firmware address: 0x20008764. */
void pico_label_next_uppercase(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "SIGUIENTE";
    } else if (g_language == '\x02') {
        label = "PROCHAIN";
    } else {
        label = "NEXT";
    }
    libc_strcpy(destination, label);
    return;
}

/* Copies uppercase localized EXIT label.
 * Firmware address: 0x20008790. */
void pico_label_exit_uppercase(char *destination)
{
    char *label;

    if (g_language == '\x01') {
        label = "SALIDA";
    } else if (g_language == '\x02') {
        label = "QUITTER";
    } else {
        label = "EXIT";
    }
    libc_strcpy(destination, label);
    return;
}
