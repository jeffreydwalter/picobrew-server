#include "ui_internal.h"

void pico_screen_get_instruction_lines(PicoTextScreen *screen, uint32_t screen_id,
                                       uint32_t language)
{
    if (screen_id >= 25) {
        ui_log("E", "SCREEN", "getScreenIdLines() invalid ScreenId arg");
        return;
    }
    if (language >= 3) {
        ui_log("E", "SCREEN", "getScreenIdLines() invalid language arg");
        language = 0;
    }
    const PicoInstructionText *text = &pico_ui_instructions[screen_id][language];
    for (unsigned i = 0; i < text->count; i++)
        pico_screen_add_lines(screen, 1, text->lines[i]);
}
void pico_screen_thermal_recovery_help(void)
{
    unsigned language = pico_ui_active->network->settings->language;
    if (language >= 3)
        language = 0;
    for (unsigned row = 1; row <= 6; row++)
        pico_display_center_text(row, pico_ui_thermal_help[language][row - 1]);
}
const char *pico_title_label(uint32_t title, uint32_t language)
{
    /* Five rows precede the original framebuffer. The old inclusive enum-count
     * check admitted ID 5 and read framebuffer bytes as string pointers. */
    ui_assert(title < 5, "0 <= titleId && titleId <= TitleId::_EnumCount");
    ui_assert(language < 3, "0 <= language && language <= 2");
    return title >= 5 ? "<titleId>" : pico_ui_titles[title][language < 3 ? language : 0];
}
const char *pico_brew_display_state_label(uint32_t state, uint32_t language)
{
    ui_assert(state < 4, "isValid(brewStateId)");
    ui_assert(language < 3, "0 <= language && language <= 2");
    return state >= 4 ? "<brewState>" : pico_ui_brew_labels[state][language < 3 ? language : 0];
}
void pico_ui_draw_instruction_screen(uint32_t screen_id, uint32_t language, const char *title,
                                     int use_context_title)
{
    ui_assert(screen_id < 25, "0 <= screenId && screenId < ScreenId::_EnumCount");
    char log[128];
    snprintf(log, sizeof log, "drawScreen %d %20s %s", (int)language, title,
             screen_id < 25 ? pico_ui_instruction_names[screen_id] : "<screenId>");
    ui_log("I", "LANG", log);
    if (use_context_title)
        pico_display_draw_titled_frame(title);
    else
        pico_ui_draw_header_at_row(title, 0);
    PicoTextScreen screen = {0};
    pico_screen_get_instruction_lines(&screen, screen_id, language);
    for (unsigned line = 0; line < screen.line_count; line++)
        pico_display_center_text(line + 3, screen.lines[line]);
    if (screen_id == 11)
        pico_graphics_bitmap(&pico_ui_active->display, 0, 23, pico_ui_bitmap_instruction_11, 128,
                             16, 1);
    else if (screen_id == 17)
        pico_graphics_bitmap(&pico_ui_active->display, 0, 23, pico_ui_bitmap_instruction_17, 128,
                             16, 1);
    else if (screen_id == 18)
        pico_graphics_bitmap(&pico_ui_active->display, 0, 16, pico_ui_bitmap_instruction_18, 128,
                             30, 1);
    else if (screen_id == 19) {
        char line[32];
        PicoProgramContext *program = &pico_ui_active->app->session;
        snprintf(line, sizeof line, "%-10s %3d%c",
                 "Target:", ui_int(ui_display_temperature(program->temperature)),
                 pico_ui_active->network->settings->temperature_unit_celsius ? 'C' : 'F');
        pico_display_center_text(3, line);
        snprintf(line, sizeof line, "%-10s %02d:%02d", "Hold Time:", (int)(program->duration / 60),
                 (int)(program->duration % 60));
        pico_display_center_text(4, line);
    }
}
void pico_screen_device_information(void)
{
    uint8_t mac[6];
    char line[152], hex[12];
    pico_ui_active->network->platform->mac_address(pico_ui_active->network->platform_context, mac);
    pico_display_clear();
    pico_display_logo();
    pico_copy_firmware_version(line);
    pico_display_center_text(3, line);
    snprintf(line, sizeof line, "ID:%s", pico_ui_active->app->device_uid);
    pico_display_text_at_cell(5, 0, line);
    line[0] = 0;
    for (unsigned i = 0; i < 6; i++) {
        if (mac[i] < 16)
            strcat(line, "0");
        strcat(line, ui_itoa(mac[i], hex, 16));
        if (i != 5)
            strcat(line, ":");
    }
    pico_display_center_text(7, line);
    pico_display_flush();
    pico_display_wait_for_click();
}

void pico_ui_draw_deep_clean_confirmation(void)
{
    pico_ui_draw_subheader("CLEAN");
    pico_display_center_text(3, "Pico Needs to be");
    pico_display_center_text(4, "Deep Cleaned.");
    pico_display_center_text(5, "Run Deep Clean Now?");
}
int pico_ui_choose_still(void *context, const char identifiers[][22], size_t count)
{
    PicoUi *previous = pico_ui_active;
    PicoUi *ui = context;
    pico_ui_bind(ui);
    if (count > PICO_MENU_CAPACITY)
        count = PICO_MENU_CAPACITY;
    for (size_t i = 0; i < count; i++)
        ui_copy(ui->menu_labels[i], 22, identifiers[i]);
    pico_display_clear();
    pico_ui_draw_subheader("CHOOSE A STILL");
    int selected = (int)pico_menu_choose(3, (uint32_t)count & 255, 0);
    pico_ui_bind(previous);
    return selected;
}
