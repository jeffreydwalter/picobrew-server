#include "network_internal.h"
#include "picobrew/config.h"

int pico_boot_read(PicoNetwork *n)
{
    return n->platform->boot_info_read(n->platform_context, n->boot_info);
}
int pico_boot_write(PicoNetwork *n)
{
    return n->platform->boot_info_write(n->platform_context, n->boot_info);
}

static void boot_magic(PicoNetwork *n)
{
    n->boot_info[4] = 0xba;
    n->boot_info[5] = 0xdc;
    n->boot_info[6] = 0xcd;
    n->boot_info[7] = 0xab;
}

bool pico_boot_select_factory(PicoNetwork *n)
{
    if (pico_boot_read(n) < 0)
        return false;
    boot_magic(n);
    memset(n->boot_info, 0, 4);
    return pico_boot_write(n) == 0;
}

bool pico_boot_toggle_application(PicoNetwork *n, bool activate)
{
    if (pico_boot_read(n) < 0)
        return false;
    if (!activate)
        return true;
    boot_magic(n);
    n->boot_info[0] = n->boot_info[0] == 1 ? 2 : 1;
    return pico_boot_write(n) == 0;
}

bool pico_update_checksum(PicoNetwork *n, const char *expected, int32_t nibble_sum)
{
    static const char hex[] = "0123456789abcdef";
    char number[16], digest_text[33];
    uint8_t digest[16];
    uint32_t started = net_now(n);
    snprintf(number, sizeof(number), "%d", (int)nibble_sum);
    if (!n->platform->md5(n->platform_context, (const uint8_t *)number, strlen(number), digest,
                          started))
        return false;
    for (unsigned i = 0; i < 16; ++i) {
        digest_text[2 * i] = hex[digest[i] >> 4];
        digest_text[2 * i + 1] = hex[digest[i] & 15];
    }
    digest_text[32] = 0;
    return !strcmp(digest_text, expected);
}

bool pico_update_open_destination(PicoNetwork *n, bool recovery_image)
{
    const char *filename;
    uint32_t capacity;
    if (!recovery_image) {
        if (!pico_boot_select_factory(n))
            return false;
        n->platform->file_remove(n->platform_context, "/sys/mcuimg3.bin");
        filename = "/sys/mcuimg2.bin";
        capacity = 0x2f800;
    } else {
        filename = "/sys/mcuimg1.bin";
        capacity = 0xf000;
    }
    n->platform->file_remove(n->platform_context, filename);
    if (n->platform->file_open(n->platform_context, filename, 0x1000))
        return true;
    n->platform->file_close(n->platform_context);
    if (n->platform->file_open(n->platform_context, filename,
                               pico_storage_create_mode(capacity, 0x1000)))
        return true;
    n->platform->file_close(n->platform_context);
    return false;
}

bool pico_update_receive(PicoNetwork *n, bool recovery_image)
{
    char header[52], expected[52] = {0};
    uint8_t chunk[256];
    size_t used = 0, pair_length = 0;
    int declared = 0;
    uint32_t characters = 0, nibble_sum = 0;
    bool payload = false;
    uint32_t last = net_now(n);
    if (!pico_update_open_destination(n, recovery_image)) {
        pico_network_close(n);
        return false;
    }
    for (;;) {
        if (!n->platform->client_connected(n->platform_context))
            goto failed;
        if (!n->platform->client_available(n->platform_context)) {
            if ((uint32_t)(net_now(n) - last) > 30000u) {
                pico_network_close(n);
                goto failed;
            }
            continue;
        }
        last = net_now(n);
        uint8_t value = (uint8_t)n->platform->client_read(n->platform_context);
        if (value == ',') {
            if (used >= sizeof(header))
                goto failed;
            header[used] = 0;
            used = 0;
            declared = (int)strtol(header, NULL, 10);
            continue;
        }
        if (value == '|') {
            if (used >= sizeof(header))
                goto failed;
            header[used] = 0;
            net_copy(expected, sizeof(expected), header);
            payload = true;
            used = 0;
            continue;
        }
        if (value == '#') {
            pico_network_close(n);
            if (!payload)
                goto failed;
            if (used && n->platform->file_write(n->platform_context, chunk, used) != (int32_t)used)
                goto failed;
            if (declared != (int32_t)(characters >> 1) || !declared)
                goto failed;
            if (!pico_update_checksum(n, expected, (int32_t)nibble_sum))
                goto failed;
            return n->platform->file_close(n->platform_context);
        }
        if (!payload) {
            if (used >= 50)
                goto failed;
            header[used++] = (char)value;
            continue;
        }
        header[pair_length++] = (char)value;
        header[pair_length] = 0;
        uint32_t nibble = (value & 15) + (value >> 6) * 9;
        if (characters % 15 == 0)
            nibble_sum -= nibble;
        else
            nibble_sum += nibble;
        ++characters;
        if (pair_length > 1) {
            chunk[used++] = (uint8_t)strtoul(header, NULL, 16);
            pair_length = 0;
            if (used >= 248) {
                if (n->platform->file_write(n->platform_context, chunk, used) != (int32_t)used)
                    goto failed;
                used = 0;
            }
        }
    }
failed:
    pico_network_close(n);
    n->platform->file_close(n->platform_context);
    return false;
}

bool pico_update_available(PicoNetwork *n)
{
#if PICO_STANDALONE
    (void)n;
    return false;
#else
    char request[501];
    net_line(n, "wifi_checkFirmware");
    snprintf(request, sizeof(request), "/API/pico/checkFirmware?uid=%s&version=0.1.34",
             n->app->device_uid);
    if (!pico_network_call(n, request, true, false))
        return false;
    uint32_t started = net_now(n);
    bool available = false;
    do {
        if (!n->platform->client_connected(n->platform_context))
            goto finished;
        if (n->platform->client_available(n->platform_context)) {
            int value = n->platform->client_read(n->platform_context);
            if ((uint32_t)value > 255u) {
                net_line(n, " wClient.read() ERROR");
                pico_network_close(n);
                return false;
            }
            available = value == 'T';
            goto finished;
        }
    } while (net_now(n) <= (uint32_t)(started + 30000u));
    net_line(n, "TIMEOUT");
finished:
    pico_network_close(n);
    net_serial(n, "IsUpdatedRequired = ");
    net_line(n, available ? "1" : "0");
    return available;
#endif
}

bool pico_update_download(PicoNetwork *n)
{
#if PICO_STANDALONE
    (void)n;
    return false;
#else
    if (!*n->app->device_uid && !pico_storage_load_uid(n))
        return false;
    char request[501];
    net_prefix(n, request, sizeof(request), "getFirmware");
    net_ui(n, PICO_NET_UI_CLEAR, NULL);
    return pico_network_call(n, request, true, false) && pico_update_receive(n, false);
#endif
}

void pico_update_run(PicoNetwork *n)
{
#if PICO_STANDALONE
    (void)n;
#else
    if (!pico_update_available(n))
        return;
    net_ui(n, PICO_NET_UI_CLEAR, NULL);
    net_ui(n, PICO_NET_UI_UPDATING, NULL);
    net_ui(n, PICO_NET_UI_FLUSH, NULL);
    if (pico_update_download(n)) {
        net_ui(n, PICO_NET_UI_DOWNLOADING, NULL);
        net_ui(n, PICO_NET_UI_FLUSH, NULL);
        n->platform->wifi_disconnect(n->platform_context);
        net_delay(n, 1000);
        if (pico_boot_toggle_application(n, true))
            n->platform->reset_mcu(n->platform_context);
    }
    net_ui(n, PICO_NET_UI_CLEAR, NULL);
    net_ui(n, PICO_NET_UI_UPDATE_FAILED, NULL);
    net_ui(n, PICO_NET_UI_FLUSH, NULL);
    pico_api_error(n, 23);
    n->ui->wait_for_continue(n->ui_context);
#endif
}
