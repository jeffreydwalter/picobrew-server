/* Original constant tables, decoded into ordinary C objects. */
#include <stdint.h>

static const char *const program_titles[7][3] = {
    {"", "", ""},
    {"BREWING", "BREWING", "BREWING"},
    {"DEEP CLEAN", "LIMPIEZA PROFUNDA", "NETTOYAGE"},
    {"SOUS VIDE", "SOUS VIDE", "SOUS VIDE"},
    {"COLD BREW", "COLD BREW", "COLD BREW"},
    {"PICOSTILL", "PICOSTILL", "PICOSTILL"},
    {"MANUAL BREW", "MANUAL BREW", "MANUAL BREW"},
};
static const char *const program_names[7][3] = {
    {"", "", ""},
    {"brew", "cerveza", "brasser"},
    {"clean", "limpieza", "nettoyer"},
    {"sous vide", "sous vide", "sous vide"},
    {"Cold Brew", "Cold Brew", "Cold Brew"},
    {"session", "session", "session"},
    {"Manual Brew", "Manual Brew", "Manual Brew"},
};
static const char *const program_states[7][3] = {
    {"", "", ""},
    {"Brewing", "preparar cerveza", "brassage"},
    {"Cleaning", "limpieza", "nettoyage"},
    {"Cooking", "coccion", "cuisson"},
    {"Brewing", "preparar", "brassage"},
    {"session", "session", "session"},
    {"Brewing", "preparar cerveza", "brassage"},
};
static const char *const program_logos[7][3] = {
    {"", "", ""},
    {"NOW BREWING", "CERVEZA", "EN BRASSAGE"},
    {"CLEANING", "LIMPIEZA", "NETTOYAGE"},
    {"COOKING", "COCINAR", "CUISSON"},
    {"NOW BREWING", "PREPARAR", "BRASSAGE"},
    {"DISTILLING", "DISTIL", "DISTILLATION"},
    {"MANUAL BREW", "MANUAL BREW", "MANUAL BREW"},
};
static const char *const program_interrupted[7][3] = {
    {"", "", ""},
    {"BREW INTERRUPTED", "CERVEZA INTERRUMPIDA", "BRASSAGE INTERROMPU"},
    {"CLEAN INTERRUPTED", "LIMPIEZA INTERRUMPIDA", "NETTOYAGE INTERROMPU"},
    {"COOKING INTERRUPTED", "COCINAR INTERRUMPIDA", "CUISSON INTERROMPUE"},
    {"BREW INTERRUPTED", "CERVEZA INTERRUMPIDA", "BRASSAGE INTERROMPU"},
    {"DISTIL INTERRUPTED", "DISTIL INTERRUMPIDA", "DISTILLATION INTERROMPUE"},
    {"BREW INTERRUPTED", "CERVEZA INTERRUMPIDA", "BRASSAGE INTERROMPU"},
};
static const char *const program_names_debug[] = {
    "None", "Brew", "Clean", "SousVideDefault", "CoffeeColdBrew", "Still", "ManualBrew"};
static const char *const program_session_names[] = {"Brewing", "Cleaning", "SousVide",
                                                    "Still",   "Coffee",   "ManualBrew"};
static const uint32_t actions_brewing[] = {1u, 3u, 6u, 5u, 9u, 11u};
static const uint32_t actions_cleaning[] = {1u, 3u, 5u, 9u, 11u};
static const uint32_t actions_sousvide[] = {1u, 3u, 5u, 10u, 11u};
static const uint32_t actions_still[] = {1u, 2u, 5u, 10u, 11u};
static const uint32_t actions_coffee[] = {1u, 3u, 5u, 10u, 11u};
static const uint32_t actions_manual[] = {1u, 5u, 4u};
