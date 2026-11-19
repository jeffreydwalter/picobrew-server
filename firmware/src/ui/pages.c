#include "ui_internal.h"

static void temperature_footer(void)
{
    PicoBrewController *c = &pico_ui_active->app->controller;
    pico_display_temperature_summary(c->wort_fahrenheit, c->thermoblock_fahrenheit,
                                     c->wort_target_fahrenheit, 7);
}
void pico_screen_brew_name_page(void)
{
    pico_display_draw_content_frame();
    pico_display_center_text(3, pico_ui_active->app->screen.recipe_name);
    if (pico_ui_active->network->settings->detailed_status)
        temperature_footer();
    else
        pico_display_center_text(7, "NOW BREWING");
}
void pico_screen_recipe_metrics_page(void)
{
    pico_display_recipe_decimal_metric();
    pico_display_recipe_integer_metric();
    pico_display_draw_content_frame();
    if (pico_ui_active->network->settings->detailed_status)
        temperature_footer();
}
void pico_screen_step_progress_page(void)
{
    PicoAppState *app = pico_ui_active->app;
    if (app->screen.progress_component < 8)
        pico_graphics_bitmap(&pico_ui_active->display, 0, 16,
                             pico_ui_setup_images[app->screen.progress_component], 128, 30, 1);
    int step = app->recipe.current_step;
    if (step >= 0 && step < PICO_RECIPE_STEP_CAPACITY)
        pico_display_draw_titled_frame(app->recipe.steps[step].name);
    if (pico_ui_active->network->settings->detailed_status)
        temperature_footer();
}
static void current_timer(void)
{
    int seconds = pico_ui_active->app->controller.step_seconds;
    pico_display_large_timer_with_seconds(seconds / 60, seconds % 60);
    if (pico_ui_active->network->settings->detailed_status)
        temperature_footer();
}
void pico_screen_current_step_page(void)
{
    PicoAppState *app = pico_ui_active->app;
    int step = app->recipe.current_step;
    if (step >= 0 && step < PICO_RECIPE_STEP_CAPACITY)
        pico_display_center_text(0, app->recipe.steps[step].name);
    pico_display_draw_content_frame();
    current_timer();
}
void pico_screen_temperature_page(void)
{
    PicoBrewController *controller = &pico_ui_active->app->controller;
    pico_display_center_text(0, controller->phase_label);
    pico_display_large_temperature_with_decimal(controller->wort_fahrenheit);
    pico_display_target_temperature(7);
    pico_display_draw_content_frame();
}
void pico_screen_program_status_page(void)
{
    PicoAppState *app = pico_ui_active->app;
    pico_display_center_text(0, app->session.title);
    pico_display_draw_content_frame();
    if (app->screen.heated_hold)
        current_timer();
    else {
        pico_display_center_text(3, app->screen.step_name);
        pico_display_center_text(
            4, pico_brew_display_state_label(app->screen.brew_state,
                                             pico_ui_active->network->settings->language));
    }
}
static void target_line(char *buffer, size_t capacity)
{
    PicoBrewController *c = &pico_ui_active->app->controller;
    PicoSettings *settings = pico_ui_active->network->settings;
    static const char *arrow[] = {" --> TARGET:", " ->OBJETIVO:", " ---> CIBLE:"};
    double target = c->wort_target_fahrenheit;
    if (target != 0.0)
        target = ui_display_temperature(target);
    char unit = settings->temperature_unit_celsius ? 'C' : 'F';
    unsigned language = settings->language;
    snprintf(buffer, capacity, "%d%c%s%d%c",
             (int)ui_int(ui_display_temperature(c->wort_fahrenheit)), unit,
             arrow[language < 3 ? language : 0], (int)ui_int(target), unit);
}
void pico_screen_basic_brew_status(void)
{
    PicoAppState *app = pico_ui_active->app;
    const PicoProgramContext *program = app->recipe.program;
    int step = app->recipe.current_step;
    if (step == 0) {
        if (app->controller.phase == PICO_BREW_PHASE_COOLING) {
            static const char *warning[3][2] = {{"KEG WATER TEMP", "TOO HIGH"},
                                                {"TEMP DEL AGUA DEL", "BARRIL MUY ALTA"},
                                                {"TEMP DE L'EAU DANS", "LE FUT TROP HAUTE"}};
            unsigned language = pico_ui_active->network->settings->language;
            if (language >= 3)
                language = 0;
            pico_display_center_text(3, warning[language][0]);
            pico_display_center_text(4, warning[language][1]);
        } else if (program->bitmap_primary) {
            const PicoBitmap *bitmap = program->bitmap_primary;
            pico_graphics_bitmap(&pico_ui_active->display, 0, 24, bitmap->pixels, bitmap->width,
                                 bitmap->height, 1);
        }
    } else if (step > 0 && step <= 2 && program->bitmap_secondary) {
        const PicoBitmap *bitmap = program->bitmap_secondary;
        pico_graphics_bitmap(&pico_ui_active->display, 0, 23, bitmap->pixels, bitmap->width,
                             bitmap->height, 1);
    }
    char line[64];
    target_line(line, sizeof line);
    pico_display_center_text(7, line);
    pico_display_draw_titled_frame(app->screen.recipe_name);
}
void pico_screen_detailed_brew_status(void)
{
    PicoAppState *app = pico_ui_active->app;
    const PicoProgramContext *program = app->recipe.program;
    ui_assert(program->session_type == 2 || program->session_type == 4,
              "sess.type == SessionTypes::SousVide || sess.type == "
              "SessionTypes::Coffee");
    int duration = (int)program->duration;
    if (program->session_type != 2 && program->session_type != 4) {
        int step = app->recipe.current_step;
        duration = step >= 0 && step < PICO_RECIPE_STEP_CAPACITY
                       ? (int)app->recipe.steps[step].active_minutes
                       : 0;
    }
    char line[64];
    snprintf(line, sizeof line, "TEMP:%d%c  TIME:%d:%02d",
             (int)ui_int(ui_display_temperature(app->controller.wort_target_fahrenheit)),
             pico_ui_active->network->settings->temperature_unit_celsius ? 'C' : 'F', duration / 60,
             duration % 60);
    int seconds = app->controller.step_seconds;
    pico_display_large_timer_with_seconds(seconds / 60, seconds % 60);
    pico_display_center_text(7, line);
    pico_display_draw_titled_frame(app->screen.recipe_name);
}
void pico_ui_draw_hold_status(void)
{
    PicoAppState *app = pico_ui_active->app;
    char line[64];
    snprintf(line, sizeof line, "HOLDING TEMP:%d%c",
             (int)ui_int(ui_display_temperature(app->controller.wort_target_fahrenheit)),
             pico_ui_active->network->settings->temperature_unit_celsius ? 'C' : 'F');
    pico_display_center_text(3, line);
    int seconds = app->controller.step_seconds, hours = seconds / 3600,
        minutes = seconds / 60 - hours * 60;
    if (minutes < 1)
        minutes = 0;
    snprintf(line, sizeof line, "TOTAL TIME:%d:%02d:%02d", hours, minutes, seconds % 60);
    pico_display_center_text(4, line);
    pico_display_draw_titled_frame(app->screen.recipe_name);
    pico_ui_draw_footer_option("Drain", 1);
}
void pico_ui_draw_circulation_status(double wort, uint32_t seconds)
{
    char line[32];
    pico_screen_draw_program_notice();
    snprintf(line, sizeof line, "%-10s  %3d%c",
             "Wort Temp:", (int)ui_int(ui_display_temperature(wort)),
             pico_ui_active->network->settings->temperature_unit_celsius ? 'C' : 'F');
    pico_display_text_at_cell(3, 3, line);
    unsigned first = seconds / 3600, second = (seconds % 3600) / 60;
    if (!first) {
        first = second;
        second = seconds % 60;
    }
    snprintf(line, sizeof line, "%-10s %02d:%02d", "Ellapsed:", (int)first, (int)second);
    pico_display_text_at_cell(4, 3, line);
    pico_ui_draw_stop_footer();
    pico_display_flush();
}
void pico_screen_still_readings(void)
{
    PicoStillTelemetry *still = &pico_ui_active->app->controller.still;
    PicoSettings *settings = pico_ui_active->network->settings;
    double t1 = still->temperature1, t2 = still->temperature2, t3 = still->temperature3;
    if (still->temperature_units != settings->temperature_unit_celsius) {
        if (settings->temperature_unit_celsius) {
            t1 = (t1 - 32) / 1.8;
            t2 = (t2 - 32) / 1.8;
            t3 = (t3 - 32) / 1.8;
        } else {
            t1 = t1 * 1.8 + 32;
            t2 = t2 * 1.8 + 32;
            t3 = t3 * 1.8 + 32;
        }
    }
    char a[32], b[32], line[80];
    char unit = settings->temperature_unit_celsius ? 'C' : 'F';
    pico_display_center_text(2, "Still Readings");
    pico_format_float_decimal((float)t1, a, 1);
    pico_format_float_decimal((float)t2, b, 1);
    snprintf(line, sizeof line, "T1:%s%c  T2:%s%c", a, unit, b, unit);
    pico_display_center_text(3, line);
    pico_format_float_decimal((float)t3, a, 1);
    pico_format_float_decimal((float)still->pressure, b, 2);
    snprintf(line, sizeof line, "T3:%s%c %spsi", a, unit, b);
    pico_display_center_text(4, line);
    pico_display_center_text(5, still->fan_on ? "Fan: ON" : "Fan: OFF");
    target_line(line, sizeof line);
    pico_display_center_text(7, line);
    pico_display_draw_titled_frame(pico_ui_active->app->screen.recipe_name);
}
#if !PICO_STANDALONE
static void recipe_logo_page(void)
{
    pico_graphics_bitmap(&pico_ui_active->display, 0, 0, pico_ui_active->network->recipe_logo, 128,
                         64, 1);
    if (pico_ui_active->network->settings->detailed_status)
        temperature_footer();
    else
        pico_display_center_text(7, pico_ui_active->app->recipe.program->logo_text);
}
static void recipe_image_page(void)
{
    pico_graphics_bitmap(&pico_ui_active->display, 0, 0, pico_ui_active->network->recipe_images[0],
                         128, 64, 1);
}
void pico_ui_recipe_logo_ready(void *context, const uint8_t image[1024])
{
    PicoUi *ui = context;
    if (image != ui->network->recipe_logo)
        memcpy(ui->network->recipe_logo, image, 1024);
    ui->app->carousel.pages[0] = recipe_logo_page;
}
void pico_ui_recipe_image_ready(void *context, unsigned index, const uint8_t image[1024])
{
    PicoUi *ui = context;
    if (index < 4 && image != ui->network->recipe_images[index])
        memcpy(ui->network->recipe_images[index], image, 1024);
    if (ui->app->carousel.page_count < PICO_CAROUSEL_CAPACITY)
        ui->app->carousel.pages[ui->app->carousel.page_count] = recipe_image_page;
}
#else
/* Local recipes use the built-in status pages. They have no downloaded art,
 * so the standalone profile does not reserve 5 KiB for cloud image buffers. */
void pico_ui_recipe_logo_ready(void *context, const uint8_t image[1024])
{
    (void)context;
    (void)image;
}
void pico_ui_recipe_image_ready(void *context, unsigned index, const uint8_t image[1024])
{
    (void)context;
    (void)index;
    (void)image;
}
#endif

void pico_screen_manual_overview_page(void)
{
    pico_display_center_bitmap((short)((64u - (uint32_t)pico_ui_asset_manual_overview.height) >> 1),
                               &pico_ui_asset_manual_overview);
}
void pico_screen_current_target_page(void)
{
    PicoAppState *app = pico_ui_active->app;
    pico_display_center_text(0, app->session.title);
    pico_display_draw_content_frame();
    char line[64];
    char unit = pico_ui_active->network->settings->temperature_unit_celsius ? 'C' : 'F';
    snprintf(line, sizeof line, "%-10s %3d%c", "Current",
             ui_int(ui_display_temperature(app->controller.wort_fahrenheit)), unit);
    pico_display_text_at_cell(3, 3, line);
    snprintf(line, sizeof line, "%-10s %3d%c", "Target",
             ui_int(ui_display_temperature(app->controller.wort_target_fahrenheit)), unit);
    pico_display_text_at_cell(4, 3, line);
}
