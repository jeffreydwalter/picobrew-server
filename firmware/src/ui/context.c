#include "ui_internal.h"
PicoUi *pico_ui_active;
void pico_ui_bind(PicoUi *ui)
{
    pico_ui_active = ui;
}
PicoUi *pico_ui_context(void)
{
    return pico_ui_active;
}
void pico_ui_initialize(PicoUi *ui, PicoAppState *app, PicoNetwork *network,
                        PicoActuators *actuators, PicoControl *control,
                        const PicoUiHardware *hardware, void *hardware_context,
                        const PicoUiServices *services, void *service_context)
{
    memset(ui, 0, sizeof *ui);
    ui->app = app;
    ui->network = network;
    ui->actuators = actuators;
    ui->control = control;
    ui->hardware = hardware;
    ui->hardware_context = hardware_context;
    ui->services = services;
    ui->service_context = service_context;
    pico_ui_bind(ui);
    pico_display_construct();
    pico_encoder_reset_state();
}
char *ui_itoa(int value, char *dst, int base)
{
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char reversed[34];
    unsigned n = 0;
    bool negative = base == 10 && value < 0;
    uint32_t magnitude = negative ? 0u - (uint32_t)value : (uint32_t)value;
    if (base < 2 || base > 36) {
        dst[0] = 0;
        return dst;
    }
    do {
        reversed[n++] = digits[magnitude % (unsigned)base];
        magnitude /= (unsigned)base;
    } while (magnitude);
    unsigned out = 0;
    if (negative)
        dst[out++] = '-';
    while (n)
        dst[out++] = reversed[--n];
    dst[out] = 0;
    return dst;
}
