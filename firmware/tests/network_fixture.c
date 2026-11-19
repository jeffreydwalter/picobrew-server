#include "picobrew/network.h"
#include <stddef.h>
#include <string.h>
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* These test boundaries deliberately trap if a scenario does not bind them.
 * They are never included in the device application. */
#define TRAP() __asm__ volatile("bkpt 0")
#define U(name, ...)                                                                               \
    uint32_t name(__VA_ARGS__)                                                                     \
    {                                                                                              \
        TRAP();                                                                                    \
        return 0;                                                                                  \
    }
#define I(name, ...)                                                                               \
    int name(__VA_ARGS__)                                                                          \
    {                                                                                              \
        TRAP();                                                                                    \
        return 0;                                                                                  \
    }
#define B(name, ...)                                                                               \
    bool name(__VA_ARGS__)                                                                         \
    {                                                                                              \
        TRAP();                                                                                    \
        return false;                                                                              \
    }
#define V(name, ...)                                                                               \
    void name(__VA_ARGS__)                                                                         \
    {                                                                                              \
        TRAP();                                                                                    \
    }
U(fixture_millis, void *c)
V(fixture_delay, void *c, uint32_t ms)
V(fixture_serial, void *c, const char *s)
V(fixture_serial_float, void *c, double value, unsigned digits)
B(fixture_initialized, void *c)
B(fixture_initialize, void *c)
B(fixture_connecting, void *c)
I(fixture_status, void *c)
I(fixture_begin, void *c, const char *s, const uint8_t *p, uint8_t security, bool binary)
V(fixture_disconnect, void *c)
PicoIpv4 fixture_local(void *c)
{
    TRAP();
    return (PicoIpv4){{0, 0, 0, 0}};
}
I(fixture_resolve, void *c, const char *s, PicoIpv4 *a)
V(fixture_dns, void *c, PicoIpv4 a)
I(fixture_scan, void *c)
const char *fixture_ssid(void *c, unsigned i)
{
    TRAP();
    return NULL;
}
uint8_t fixture_security(void *c, unsigned i)
{
    TRAP();
    return 0;
}
int32_t fixture_rssi(void *c, unsigned i)
{
    TRAP();
    return 0;
}
V(fixture_mac, void *c, uint8_t mac[6])
B(fixture_md5, void *c, const uint8_t *data, size_t length, uint8_t digest[16], uint32_t started)
I(fixture_connect, void *c, PicoIpv4 a, uint16_t port, bool background)
I(fixture_connected, void *c)
I(fixture_available, void *c)
I(fixture_read, void *c)
size_t fixture_write(void *c, const uint8_t *data, size_t length)
{
    TRAP();
    return 0;
}
V(fixture_flush, void *c)
V(fixture_stop, void *c)
V(fixture_file_begin, void *c)
B(fixture_file_open, void *c, const char *name, uint32_t mode)
I(fixture_file_read, void *c)
int32_t fixture_file_write(void *c, const uint8_t *data, size_t length)
{
    TRAP();
    return 0;
}
B(fixture_file_close, void *c)
I(fixture_file_remove, void *c, const char *name)
I(fixture_boot_read, void *c, uint8_t data[8])
I(fixture_boot_write, void *c, const uint8_t data[8])
V(fixture_reset, void *c)
uint8_t fixture_ping(void *c)
{
    TRAP();
    return 0;
}
V(fixture_ip_config, void *c, PicoIpv4 *l, PicoIpv4 *d, PicoIpv4 *g, PicoIpv4 *m)
V(fixture_event, void *c, PicoNetworkUiEvent event, const char *text)
V(fixture_error, void *c, int code, bool wait, bool report)
B(fixture_choose_wifi, void *c)
V(fixture_diagnose, void *c)
V(fixture_language, void *c)
V(fixture_units, void *c)
V(fixture_continue, void *c)
I(fixture_choose_still, void *c, const char ids[][22], size_t count)
B(fixture_decode, void *c, const char *text, size_t length)
V(fixture_logo, void *c, const uint8_t image[1024])
V(fixture_image, void *c, unsigned index, const uint8_t image[1024])
V(fixture_text_log, void *c, const char *level, const char *component, const char *message)

PicoNetwork fixture_network;
PicoAppState fixture_app;
PicoSettings fixture_settings;
PicoRecipeListing fixture_listing;
const PicoNetworkPlatform fixture_platform = {
    fixture_millis,     fixture_delay,      fixture_serial,      fixture_initialized,
    fixture_initialize, fixture_connecting, fixture_status,      fixture_begin,
    fixture_disconnect, fixture_local,      fixture_resolve,     fixture_dns,
    fixture_scan,       fixture_ssid,       fixture_security,    fixture_rssi,
    fixture_mac,        fixture_md5,        fixture_connect,     fixture_connected,
    fixture_available,  fixture_read,       fixture_write,       fixture_flush,
    fixture_stop,       fixture_file_begin, fixture_file_open,   fixture_file_read,
    fixture_file_write, fixture_file_close, fixture_file_remove, fixture_boot_read,
    fixture_boot_write, fixture_reset,      fixture_ping,        fixture_ip_config,
    fixture_serial_float};
const PicoNetworkUi fixture_ui = {fixture_event,    fixture_error,        fixture_choose_wifi,
                                  fixture_diagnose, fixture_language,     fixture_units,
                                  fixture_continue, fixture_choose_still, fixture_decode,
                                  fixture_logo,     fixture_image};
const uint32_t fixture_offsets[] = {offsetof(PicoNetwork, server_ip),
                                    offsetof(PicoNetwork, server_host),
                                    offsetof(PicoNetwork, last_recovery_save_ms),
                                    offsetof(PicoNetwork, log_consecutive_failures),
                                    offsetof(PicoNetwork, boot_info),
                                    offsetof(PicoNetwork, still),
                                    offsetof(PicoAppState, device_uid),
                                    offsetof(PicoAppState, session),
                                    offsetof(PicoAppState, recipe),
                                    offsetof(PicoAppState, device_state),
                                    offsetof(PicoAppState, carousel),
                                    offsetof(PicoAppState, control_signals),
                                    offsetof(PicoSettings, wifi_security),
                                    offsetof(PicoSettings, offline_mode),
                                    sizeof(PicoNetwork),
                                    sizeof(PicoAppState),
                                    sizeof(PicoSettings)};
void fixture_bind(void)
{
    memset(&fixture_app, 0, sizeof(fixture_app));
    memset(&fixture_settings, 0, sizeof(fixture_settings));
    memset(&fixture_listing, 0, sizeof(fixture_listing));
    pico_network_initialize(&fixture_network, &fixture_app, &fixture_settings, &fixture_platform,
                            NULL, &fixture_ui, NULL);
    strcpy(fixture_app.device_uid, "0123456789abcdef0123456789abcdef");
    strcpy(fixture_app.session.identifier, "session42");
    strcpy(fixture_app.recipe.identifier, "recipe42");
    strcpy(fixture_settings.wifi_ssid, "Network");
    strcpy(fixture_settings.wifi_password, "password");
    fixture_settings.wifi_security = 4;
    fixture_network.server_ip = (PicoIpv4){{1, 2, 3, 4}};
    fixture_app.controller.candidate_shuttle_scale = 0.2;
    fixture_app.controller.wort_fahrenheit = 152.9;
    fixture_app.controller.thermoblock_fahrenheit = 180.3;
    fixture_app.controller.step_seconds = 60;
}
