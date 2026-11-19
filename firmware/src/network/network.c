#include "network_internal.h"
#include <math.h>
#include "picobrew/config.h"
#include "picobrew/local_control.h"

void pico_network_initialize(PicoNetwork *n, PicoAppState *app, PicoSettings *settings,
                             const PicoNetworkPlatform *platform, void *platform_context,
                             const PicoNetworkUi *ui, void *ui_context)
{
    memset(n, 0, sizeof(*n));
    n->app = app;
    n->settings = settings;
    n->platform = platform;
    n->platform_context = platform_context;
    n->ui = ui;
    n->ui_context = ui_context;
#if PICO_STANDALONE
    n->server_host = "";
#else
    n->server_host = "picobrew.com";
#endif
    n->server_port = 80;
}

void pico_network_log(PicoNetwork *n, const char *level, const char *component, const char *message)
{
    char prefix[128];
    net_now(n);
    snprintf(prefix, sizeof(prefix), "[%s][%s] ", level, component);
    net_serial(n, prefix);
    net_line(n, message);
}

void pico_network_assert(PicoNetwork *n, bool condition, const char *expression)
{
    if (!condition) {
        ++n->assertion_count;
        net_serial(n, "***** ASSERT [");
        net_serial(n, expression);
        net_line(n, "] *****");
    }
}

bool pico_network_ipv4_is_zero(PicoIpv4 a)
{
    return !(a.octet[0] | a.octet[1] | a.octet[2] | a.octet[3]);
}

bool pico_network_parse_ipv4(const char *text, PicoIpv4 *address)
{
    unsigned index = 0;
    uint16_t octet = 0;
    for (;; ++text) {
        uint8_t c = (uint8_t)*text;
        if (!c) {
            if (index != 3)
                return false;
            address->octet[3] = (uint8_t)octet;
            return true;
        }
        if ((uint8_t)(c - '0') < 10) {
            octet = (uint16_t)((c - '0') + octet * 10u);
            if (octet > 255)
                return false;
        } else if (c == '.' && index < 3) {
            address->octet[index++] = (uint8_t)octet;
            octet = 0;
        } else
            return false;
    }
}

char *pico_network_format_float(float value, char *output, unsigned decimals)
{
    char fraction[16];
    snprintf(output, 16, "%d", (int)net_truncate_signed((double)value));
    if (value < 0.0f)
        value = -value;
    if (!decimals)
        return output;
    if (decimals > 9) {
        output[0] = 0;
        return output;
    }
    strcat(output, ".");
    uint32_t scale = 1;
    for (unsigned i = 0; i < decimals; ++i)
        scale *= 10;
    float integral = (float)net_truncate_signed((double)value);
    float fractional = (float)(value - integral);
    fractional = (float)(fractional * (float)scale);
    uint32_t digits = net_truncate_unsigned(fractional);
    snprintf(fraction, sizeof(fraction), "%d", (int)(int32_t)digits);
    size_t size = strlen(fraction);
    for (unsigned i = 0; size + i < decimals; ++i)
        strcat(output, "0");
    strcat(output, fraction);
    return output;
}

size_t pico_network_url_encode(const char *input, char *output, size_t capacity)
{
    static const char reserved[] = " \"#$%&+,/:;<=>?@[\\]^`{}~";
    static const char hex[] = "0123456789ABCDEF";
    size_t used = 0;
    if (!capacity)
        return SIZE_MAX;
    for (; *input; ++input) {
        uint8_t value = (uint8_t)*input;
        bool escape = strchr(reserved, value) != NULL;
        size_t count = escape ? 3 : 1;
        if (used + count >= capacity) {
            output[used] = 0;
            return SIZE_MAX;
        }
        if (escape) {
            /* The original backslash mapping is %7C. */
            if (value == '\\')
                value = '|';
            output[used++] = '%';
            output[used++] = hex[value >> 4];
            output[used++] = hex[value & 15];
        } else
            output[used++] = (char)value;
    }
    output[used] = 0;
    return used;
}

int pico_network_connect_access_point(PicoNetwork *n)
{
    pico_network_log(n, "I", "WIFI", "connectToApInternal");
    PicoSettings *s = n->settings;
    if (s->wifi_security == 5) {
        pico_network_log(n, "I", "WIFI", s->wifi_ssid);
#if !PICO_STANDALONE
        pico_network_log(n, "I", "WIFI", s->wifi_password);
#endif
        int status = n->platform->wifi_begin(n->platform_context, s->wifi_ssid,
                                             (const uint8_t *)s->wifi_password, 5, false);
        if (status != 3) {
            size_t length = 0;
            while (length < 32 && s->wifi_password[length])
                ++length;
            if (length == 32)
                length = 0;
            uint8_t decoded[17] = {0};
            for (size_t i = 0; i < length; i += 2) {
                uint8_t hi = (uint8_t)s->wifi_password[i], lo = (uint8_t)s->wifi_password[i + 1];
                decoded[i / 2] =
                    (uint8_t)((((hi >> 6) * 9 + (hi & 15)) * 16) | ((lo >> 6) * 9 + (lo & 15)));
            }
            decoded[length / 2] = 0;
            status = n->platform->wifi_begin(n->platform_context, s->wifi_ssid, decoded, 5, true);
        }
        return status;
    }
    return n->platform->wifi_begin(n->platform_context, s->wifi_ssid,
                                   (const uint8_t *)s->wifi_password, s->wifi_security, false);
}

bool pico_network_connect_stored(PicoNetwork *n)
{
#if PICO_STANDALONE
    /* Association and DHCP are separate events. Neither a prior address
     * nor association without a fresh lease establishes a connection. */
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        if (pico_local_cancelled())
            return false;
        pico_network_connect_access_point(n);
        uint32_t started = net_now(n);
        bool associated = false;
        for (;;) {
            int status = n->platform->wifi_status(n->platform_context);
            if (status == 3) {
                if (!pico_network_ipv4_is_zero(n->platform->local_ip(n->platform_context)))
                    return true;
                if (!associated) {
                    associated = true;
                    started = net_now(n);
                }
                if ((uint32_t)(net_now(n) - started) >= 45000u)
                    break;
            } else if (associated || !n->platform->wifi_connecting(n->platform_context) ||
                       (uint32_t)(net_now(n) - started) >= 10000u) {
                break;
            }
            net_delay(n, 100);
            pico_local_poll();
            if (pico_local_cancelled()) {
                n->platform->wifi_disconnect(n->platform_context);
                return false;
            }
        }
        n->platform->wifi_disconnect(n->platform_context);
    }
    return false;
#else
    pico_network_log(n, "I", "WIFI", "connectToStoredWiFi");
    int status = pico_network_connect_access_point(n);
    if (status != 3) {
        unsigned attempts = 0;
        do {
            if (!n->platform->wifi_connecting(n->platform_context)) {
                pico_network_connect_access_point(n);
                ++attempts;
            } else {
                uint32_t start = net_now(n);
                do {
                    status = n->platform->wifi_status(n->platform_context);
                    if (status == 3)
                        break;
                } while ((uint32_t)(net_now(n) - start) < 10000u);
                if (n->platform->wifi_status(n->platform_context) == 3) {
                    net_delay(n, 500);
                    return true;
                }
                n->platform->wifi_disconnect(n->platform_context);
            }
        } while (attempts < 2);
    }
    if (n->platform->wifi_status(n->platform_context) == 3)
        return true;
    n->platform->wifi_disconnect(n->platform_context);
    return false;
#endif
}

bool pico_network_reconnect(PicoNetwork *n)
{
    n->platform->wifi_disconnect(n->platform_context);
    if (!n->platform->wifi_initialized(n->platform_context) &&
        !n->platform->wifi_initialize(n->platform_context))
        return false;
    return pico_network_connect_stored(n);
}

bool pico_network_resolve_address(PicoNetwork *n)
{
#if PICO_STANDALONE
    (void)n;
    return false; /* No cloud endpoint exists in this profile. */
#else
    if (!n->platform->wifi_initialized(n->platform_context) ||
        n->platform->wifi_status(n->platform_context) != 3)
        return false;
    uint32_t start = net_now(n);
    while (pico_network_ipv4_is_zero(n->server_ip) && (uint32_t)(net_now(n) - start) < 30000u) {
        int result = n->platform->resolve_host(n->platform_context, n->server_host, &n->server_ip);
        if (result < 0) {
            memset(&n->server_ip, 0, sizeof(n->server_ip));
            return false;
        }
        net_delay(n, 500);
    }
    return (uint32_t)(net_now(n) - start) < 30000u && !pico_network_ipv4_is_zero(n->server_ip);
#endif
}

bool pico_network_wait_local_ip(PicoNetwork *n, uint32_t timeout, bool show_error)
{
    uint32_t start = net_now(n);
    while (pico_network_ipv4_is_zero(n->platform->local_ip(n->platform_context)) &&
           (uint32_t)(net_now(n) - start) < timeout) {
        net_delay(n, 300);
        pico_local_poll();
        if (pico_local_cancelled())
            return false;
    }
    if (pico_network_ipv4_is_zero(n->platform->local_ip(n->platform_context)) && show_error) {
        net_error(n, 11, true, false);
        return false;
    }
    /* Observed firmware returns true on timeout when show_error is false. */
    return true;
}

bool pico_network_resolve_server(PicoNetwork *n)
{
#if PICO_STANDALONE
    return pico_network_wait_local_ip(n, 45000, true);
#else
    pico_network_wait_local_ip(n, 45000, true);
    if (pico_network_resolve_address(n))
        return true;
    PicoIpv4 dns = {{8, 8, 8, 8}};
    n->platform->set_dns(n->platform_context, dns);
    return pico_network_resolve_address(n);
#endif
}

int pico_network_check_status(PicoNetwork *n, bool background, bool resolve)
{
#if PICO_STANDALONE
    (void)resolve;
#endif
    bool resolved = false;
    if (!n->platform->wifi_initialized(n->platform_context) &&
        !n->platform->wifi_initialize(n->platform_context))
        return 9;
    if (n->platform->wifi_status(n->platform_context) != 3) {
        if (!pico_network_reconnect(n))
            return 10;
        if (background)
            return 0;
    }
    if (pico_network_ipv4_is_zero(n->platform->local_ip(n->platform_context)))
        return 11;
#if !PICO_STANDALONE
    if (resolve && pico_network_ipv4_is_zero(n->server_ip)) {
        if (!pico_network_resolve_address(n))
            return 3;
        resolved = true;
    }
#endif
    if (n->platform->client_connected(n->platform_context)) {
        net_line(n, "WARNING: wClient disconnected!");
        n->platform->client_stop(n->platform_context);
    }
    return resolved && background ? 2 : 1;
}

bool pico_network_connect_with_screen(PicoNetwork *n)
{
    net_ui(n, PICO_NET_UI_CONNECTING, n->settings->wifi_ssid);
    PicoControlSignals *p = &n->app->control_signals;
    p->progress_row = 3;
    p->progress_start_column = 8;
    p->progress_end_column = 21;
    p->progress_glyph_index = 0;
    p->progress_active = 1;
    return pico_network_connect_stored(n);
}

bool pico_network_ensure_connection(PicoNetwork *n)
{
#if PICO_STANDALONE
    /* A failed access point must never prevent local brewing or force an
     * endless WiFi setup loop. New credentials remain available in Settings. */
    if (!pico_storage_load_wifi(n))
        return false;
    bool connected = pico_network_connect_with_screen(n);
    net_ui(n, PICO_NET_UI_STOP_PROGRESS, NULL);
    return connected;
#else
    net_ui(n, PICO_NET_UI_CLEAR, NULL);
    net_ui(n, PICO_NET_UI_DEVICE, NULL);
    net_ui(n, PICO_NET_UI_FLUSH, NULL);
    bool connected;
    if (!pico_storage_load_wifi(n))
        connected = n->ui->choose_wifi_and_connect(n->ui_context);
    else {
        connected = pico_network_connect_with_screen(n);
        if (connected && !pico_network_resolve_server(n)) {
            net_ui(n, PICO_NET_UI_STOP_PROGRESS, NULL);
            net_ui(n, PICO_NET_UI_DEVICE_FAILED, NULL);
            net_ui(n, PICO_NET_UI_FLUSH, NULL);
            net_delay(n, 1000);
            net_ui(n, PICO_NET_UI_CLEAR, NULL);
            n->ui->diagnose_wifi(n->ui_context);
        }
        net_ui(n, PICO_NET_UI_STOP_PROGRESS, NULL);
    }
    if (!connected) {
        do {
            net_ui(n, PICO_NET_UI_DEVICE_FAILED, NULL);
            net_ui(n, PICO_NET_UI_FLUSH, NULL);
            net_delay(n, 1000);
            connected = n->ui->choose_wifi_and_connect(n->ui_context);
        } while (!connected);
    } else {
        net_ui(n, PICO_NET_UI_DEVICE_CONNECTED, NULL);
        net_ui(n, PICO_NET_UI_FLUSH, NULL);
    }
    pico_storage_save_wifi(n);
    return true;
#endif
}

void pico_network_close(PicoNetwork *n)
{
    if (n->platform->client_connected(n->platform_context)) {
        n->platform->client_flush(n->platform_context);
        n->platform->client_stop(n->platform_context);
    }
}

#if !PICO_STANDALONE
static bool rejected_status(int status)
{
    return status == 3 || status == 9 || status == 10 || status == 11;
}

static bool write_request(PicoNetwork *n, const char *request)
{
    if (!net_put(n, "GET "))
        return false;
    net_delay(n, 5);
    if (!net_put(n, request))
        return false;
    net_delay(n, 5);
    if (!net_put(n, " HTTP/1.0\r\nHost: "))
        return false;
    net_delay(n, 5);
    if (!net_put(n, n->server_host))
        return false;
    net_delay(n, 5);
    if (!net_put(n, "\nConnection: close\n\n"))
        return false;
    net_delay(n, 5);
    return true;
}
#endif

bool pico_network_open_request(PicoNetwork *n, const char *request, bool keep_open, bool background)
{
#if PICO_STANDALONE
    (void)n; (void)request; (void)keep_open; (void)background;
    return false;
#else
    net_line(n, "in connectToApi");
    int status = pico_network_check_status(n, background, true);
    if (!background && rejected_status(status)) {
        net_error(n, status, true, false);
        return false;
    }
    if (background && status != 1)
        return false;
    net_line(n, "sending: ");
    net_line(n, request);
    if (!n->platform->client_connect(n->platform_context, n->server_ip, n->server_port,
                                     background)) {
        pico_network_close(n);
        return false;
    }
    if (!write_request(n, request)) {
        pico_network_close(n);
        return false;
    }
    if (!keep_open)
        pico_network_close(n);
    return true;
#endif
}

bool pico_network_call(PicoNetwork *n, const char *request, bool wait, bool background)
{
#if PICO_STANDALONE
    (void)n; (void)request; (void)wait; (void)background;
    return false;
#else
    int status = pico_network_check_status(n, background, true);
    if ((background && status != 1) || (!background && rejected_status(status))) {
        char message[128];
        snprintf(message, sizeof(message), "checkWifiStatus %d", status);
        pico_network_log(n, "E", "WIFI", message);
        return false;
    }
    if (!n->platform->client_connect(n->platform_context, n->server_ip, n->server_port,
                                     background)) {
        pico_network_log(n, "E", "WIFI", "connect failed");
        return false;
    }
    if (!write_request(n, request)) {
        pico_network_close(n);
        return false;
    }
    if (wait)
        return pico_network_wait_hash(n);
    pico_network_close(n);
    return true;
#endif
}

bool pico_network_wait_hash(PicoNetwork *n)
{
    uint32_t start = net_now(n);
    do {
        for (;;) {
            if (!n->platform->client_connected(n->platform_context))
                goto failed;
            if (!n->platform->client_available(n->platform_context))
                break;
            if ((uint8_t)n->platform->client_read(n->platform_context) == '#')
                return true;
        }
    } while ((uint32_t)(net_now(n) - start) < 30001u);
failed:
    pico_network_close(n);
    return false;
}

bool pico_network_read_response(PicoNetwork *n, char *buffer, size_t capacity, size_t *length,
                                bool close_after, uint8_t terminator)
{
    bool started = false;
    const char *error = NULL;
    if (!buffer || !capacity || !length || *length >= capacity)
        return false;
    net_serial(n, "readApiResponse() ");
    net_serial(n, " [closeConnection = ");
    net_serial(n, close_after ? "true" : "false");
    net_line(n, "]");
    uint32_t start = net_now(n);
    for (;;) {
        while (!n->platform->client_available(n->platform_context)) {
            if (!n->platform->client_connected(n->platform_context)) {
                error = "ERROR wClient disconnected!";
                goto failed;
            }
            if ((uint32_t)(start + 30000u) < net_now(n)) {
                error = "ERROR timeout";
                goto failed;
            }
        }
        int value = n->platform->client_read(n->platform_context);
        if ((uint32_t)value > 255u) {
            error = " wClient.read() ERROR";
            goto failed;
        }
        char character[2] = {(char)value, 0};
        net_serial(n, character);
        if (!started) {
            started = value == '#';
            continue;
        }
        if (value == terminator) {
            buffer[*length] = 0;
            net_serial(n, "RECV [");
            net_serial(n, buffer);
            net_serial(n, "] ");
            char count[24];
            snprintf(count, sizeof(count), "%u Bytes", (unsigned)*length);
            net_line(n, count);
            if (close_after)
                pico_network_close(n);
            return true;
        }
        if (*length + 1 >= capacity) {
            error = "ERROR data exceeds buffer size";
            goto failed;
        }
        buffer[(*length)++] = (char)value;
    }
failed:
    buffer[*length] = 0;
    net_line(n, error);
    pico_network_close(n);
    return false;
}

bool pico_network_read_hex_image(PicoNetwork *n, uint8_t image[1024], uint8_t terminator)
{
    uint32_t last = net_now(n);
    unsigned pair_length = 0, bytes = 0, characters = 0;
    char pair[3] = {0};
    do {
        for (;;) {
            if (!n->platform->client_connected(n->platform_context))
                goto failed;
            if (!n->platform->client_available(n->platform_context))
                break;
            last = net_now(n);
            uint8_t value = (uint8_t)n->platform->client_read(n->platform_context);
            if (value == '#')
                goto failed;
            if (value == terminator) {
                if ((characters >> 1) == 1024 || bytes == 1024)
                    return true;
                goto failed;
            }
            pair[pair_length++] = (char)value;
            pair[pair_length] = 0;
            ++characters;
            if (pair_length == 2) {
                if (bytes >= 1024)
                    goto failed;
                image[bytes++] = (uint8_t)strtoul(pair, NULL, 16);
                pair_length = 0;
            }
        }
    } while ((uint32_t)(net_now(n) - last) < 30001u);
failed:
    pico_network_close(n);
    return false;
}
