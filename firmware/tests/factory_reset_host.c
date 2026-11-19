/* Execute the actual service menus and storage helper in both profiles.
 * Only encoder input and electrical/filesystem boundaries are simulated. */
#include "picobrew/ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static PicoAppState state;
static PicoSettings settings;
static PicoNetwork network;
static PicoUi ui;
static struct {
    uint32_t now;
    unsigned clicks, choice_count, opens, closes, resets, removals;
    int choices[8];
    char removed[9][40];
} sim;
#if !PICO_STANDALONE
static const char *const stock_files[] = {
    "userPref.txt", "SS.txt", "/sys/mcuimg2.bin", "/sys/mcuimg3.bin",
    "sys/uid.txt", "/sys/mcubootinfo.bin"
};
#endif

static uint32_t clock_ms(void *context) { (void)context; return ++sim.now; }
static void delay_ms(void *context, uint32_t ms) { (void)context; sim.now += ms; }
static void serial_write(void *context, const char *text) { (void)context; (void)text; }
static void digital_write(void *context, unsigned pin, unsigned value)
{ (void)context; (void)pin; (void)value; }
static uint8_t spi_transfer(void *context, uint8_t value)
{ (void)context; (void)value; return 0; }
static bool file_open(void *context, const char *name, uint32_t mode)
{
    (void)context;
    assert(!strcmp(name, "/sys/mcuimg1.bin") && mode == 0);
    ++sim.opens;
    return true;
}
static bool file_close(void *context) { (void)context; ++sim.closes; return true; }
static int file_remove(void *context, const char *name)
{
    (void)context;
    assert(sim.removals < 9);
    snprintf(sim.removed[sim.removals++], sizeof sim.removed[0], "%s", name);
    return 0;
}
static void reset_mcu(void *context) { (void)context; ++sim.resets; }
static const PicoNetworkPlatform platform = {
    .millis = clock_ms, .delay_ms = delay_ms, .serial_write = serial_write,
    .file_open = file_open, .file_close = file_close, .file_remove = file_remove,
    .reset_mcu = reset_mcu
};
static const PicoUiHardware hardware = {
    .millis = clock_ms, .delay_ms = delay_ms, .digital_write = digital_write,
    .spi_transfer = spi_transfer
};

/* Supply a click after each menu has been drawn, including a stale selection
 * beyond the standalone menu. Selection/confirmation/dispatch code is real. */
int __wrap_pico_encoder_take_click(PicoEncoder *encoder)
{
    assert(sim.clicks < sim.choice_count);
    encoder->position = encoder->minimum + sim.choices[sim.clicks++];
    return 1;
}

static void initialize(unsigned language)
{
    memset(&sim, 0, sizeof sim);
    memset(&state, 0, sizeof state);
    memset(&settings, 0, sizeof settings);
    memset(&network, 0, sizeof network);
    memset(&ui, 0, sizeof ui);
    settings.language = (uint8_t)language;
    state.controller.phase = 4; /* No active program or filter prompt. */
    network.app = &state;
    network.settings = &settings;
    network.platform = &platform;
    ui.app = &state;
    ui.network = &network;
    ui.hardware = &hardware;
    ui.display.width = 128;
    ui.display.height = 64;
    ui.display.text_scale = 1;
    ui.display.foreground = 1;
    pico_ui_bind(&ui);
}

static void test_visible_menu(unsigned language)
{
    initialize(language);
    char exit_label[PICO_MENU_LABEL_LENGTH];
    pico_label_exit(exit_label);
    uint32_t count = pico_menu_populate_service();
#if PICO_STANDALONE
    assert(count == 6);
    for (unsigned i = 0; i < count; ++i) {
        assert(strcmp(ui.menu_labels[i], "Factory Reset"));
        assert(strcmp(ui.menu_labels[i], "Reinicio por defecto"));
    }
#else
    assert(count == 7);
    assert(!strcmp(ui.menu_labels[5], language == 1 ? "Reinicio por defecto" : "Factory Reset"));
#endif
    assert(!strcmp(ui.menu_labels[count - 1], exit_label));
}

static void test_direct_storage_call(void)
{
    initialize(0);
    pico_storage_delete_user_files(&network);
#if PICO_STANDALONE
    assert(sim.removals == 0);
#else
    assert(sim.removals == sizeof stock_files / sizeof stock_files[0]);
    for (unsigned i = 0; i < sim.removals; ++i)
        assert(!strcmp(sim.removed[i], stock_files[i]));
#endif
    assert(sim.resets == 0 && sim.opens == 0);
}

static void test_menu_dispatch(unsigned language)
{
#if PICO_STANDALONE
    /* Index 5 was Factory Reset, 6 was Exit in the legacy menu. Neither may
     * reach a confirmation, remove a file, or reset in the standalone profile. */
    for (int selection = 5; selection <= 6; ++selection) {
        initialize(language);
        sim.choices[0] = selection;
        sim.choice_count = 1;
        pico_ui_service_menu();
        assert(sim.clicks == 1 && sim.removals == 0 && sim.resets == 0);
        assert(sim.opens == 0 && sim.closes == 0);
        assert(network.assertion_count == 0);
    }
#else
    initialize(language);
    sim.choices[0] = 5; /* Factory Reset. */
    sim.choices[1] = 0; /* Decline. */
    sim.choices[2] = 6; /* Exit. */
    sim.choice_count = 3;
    pico_ui_service_menu();
    assert(sim.clicks == 3 && sim.removals == 0 && sim.resets == 0 && sim.opens == 0);
    for (int clear_wifi = 0; clear_wifi <= 1; ++clear_wifi) {
        initialize(language);
        sim.choices[0] = 5;
        sim.choices[1] = 1; /* Confirm Factory Reset. */
        sim.choices[2] = clear_wifi;
        sim.choices[3] = 6; /* Simulated reset returns to the menu. */
        sim.choice_count = 4;
        pico_ui_service_menu();
        assert(sim.clicks == 4 && sim.resets == 1 && sim.opens == 1 && sim.closes == 1);
        unsigned offset = clear_wifi ? 3 : 0;
        assert(sim.removals == offset + 6);
        if (clear_wifi) {
            assert(!strcmp(sim.removed[0], "ssid.txt"));
            assert(!strcmp(sim.removed[1], "wifiPass.txt"));
            assert(!strcmp(sim.removed[2], "wifiSecurity.txt"));
        }
        for (unsigned i = 0; i < 6; ++i)
            assert(!strcmp(sim.removed[offset + i], stock_files[i]));
        assert(network.assertion_count == 0);
    }
#endif
}

int main(void)
{
    for (unsigned language = 0; language < 3; ++language) {
        test_visible_menu(language);
        test_menu_dispatch(language);
    }
    test_direct_storage_call();
    puts(PICO_STANDALONE ? "PASS: standalone menu and direct reset preserve firmware and identity"
                        : "PASS: legacy factory reset labels, confirmations and deletion order preserved");
    return 0;
}
