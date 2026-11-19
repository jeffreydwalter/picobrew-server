#include "network_internal.h"
#include "picobrew/config.h"
#if PICO_STANDALONE
#include "picobrew/local_store.h"
#endif

uint32_t pico_storage_create_mode(uint32_t capacity, uint32_t flags)
{
    static const uint32_t block_sizes[] = {256, 1024, 4096, 16384, 65536};
    if (capacity > 0xff0000u)
        return 0;
    unsigned index = 0;
    while (capacity > block_sizes[index] * 255u)
        ++index;
    uint32_t blocks = capacity / block_sizes[index] + (capacity % block_sizes[index] != 0);
    return (blocks & 255u) | ((flags & 255u) << 16) | 0x3000u | (index << 8);
}

/* capacity is the actual C destination capacity, including its terminator.
 * Original valid records are unchanged; oversized records are rejected before
 * the write that overflowed the original caller's storage. */
bool pico_storage_read_text(PicoNetwork *n, const char *name, char *buffer, size_t capacity)
{
    if (!buffer || !capacity)
        return false;
    buffer[0] = 0;
    if (!n->platform->file_open(n->platform_context, name, 0)) {
        n->platform->file_close(n->platform_context);
        return false;
    }
    size_t count = 0;
    int value;
    while ((value = n->platform->file_read(n->platform_context)) != -1) {
        if (count + 1 >= capacity) {
            buffer[count] = 0;
            n->platform->file_close(n->platform_context);
            return false;
        }
        buffer[count++] = (char)value;
    }
    buffer[count] = 0;
    n->platform->file_close(n->platform_context);
    return count != 0;
}

bool pico_storage_write_text(PicoNetwork *n, const char *name, const char *text, uint32_t capacity)
{
    size_t length = strlen(text);
    if (length > capacity)
        return false;
    if (!n->platform->file_open(n->platform_context, name, 0x1000)) {
        n->platform->file_close(n->platform_context);
        uint32_t mode = pico_storage_create_mode(capacity, 0x1000);
        if (!mode || !n->platform->file_open(n->platform_context, name, mode)) {
            n->platform->file_close(n->platform_context);
            return false;
        }
    }
    int32_t written = n->platform->file_write(n->platform_context, (const uint8_t *)text, length);
    bool closed = n->platform->file_close(n->platform_context);
    return closed && written >= 0 && (size_t)written == length;
}

bool pico_storage_load_wifi(PicoNetwork *n)
{
    char security[3];
    if (!pico_storage_read_text(n, "ssid.txt", n->settings->wifi_ssid,
                                sizeof(n->settings->wifi_ssid)))
        return false;
    net_delay(n, 50);
    if (!pico_storage_read_text(n, "wifiSecurity.txt", security, sizeof(security)))
        return false;
    n->settings->wifi_security = (uint8_t)(security[0] - '0');
    net_delay(n, 50);
    if (n->settings->wifi_security != 7) {
        if (!pico_storage_read_text(n, "wifiPass.txt", n->settings->wifi_password,
                                    sizeof(n->settings->wifi_password)))
            return false;
        net_delay(n, 50);
    }
    return true;
}

bool pico_storage_save_wifi(PicoNetwork *n)
{
    char security[2] = {(char)(n->settings->wifi_security + '0'), 0};
    if (!pico_storage_write_text(n, "ssid.txt", n->settings->wifi_ssid, 33))
        return false;
    net_delay(n, 50);
    if (!pico_storage_write_text(n, "wifiSecurity.txt", security, 2))
        return false;
    net_delay(n, 50);
    if (n->settings->wifi_security != 7) {
        if (!pico_storage_write_text(n, "wifiPass.txt", n->settings->wifi_password, 64))
            return false;
        net_delay(n, 50);
    }
    return true;
}

void pico_storage_delete_wifi(PicoNetwork *n)
{
    const char *files[] = {"ssid.txt", "wifiPass.txt", "wifiSecurity.txt"};
    for (unsigned i = 0; i < 3; ++i)
        n->platform->file_remove(n->platform_context, files[i]);
}

void pico_storage_delete_user_files(PicoNetwork *n)
{
#if PICO_STANDALONE
    /* Preserve the installed application and identity even if a stale caller
     * reaches the stock factory-reset helper. Its recovery requires the cloud. */
    (void)n;
#else
    const char *files[] = {"userPref.txt",     "SS.txt",      "/sys/mcuimg2.bin",
                           "/sys/mcuimg3.bin", "sys/uid.txt", "/sys/mcubootinfo.bin"};
    for (unsigned i = 0; i < 6; ++i)
        n->platform->file_remove(n->platform_context, files[i]);
#endif
}

bool pico_storage_save_display(PicoNetwork *n)
{
#if PICO_STANDALONE
    return pico_local_settings_save(n);
#else
    char text[6];
    snprintf(text, sizeof(text), "%c%u", '0' + n->settings->temperature_unit_celsius,
             n->settings->language);
    return pico_storage_write_text(n, "userPref.txt", text, 5);
#endif
}

bool pico_storage_load_shuttle_scale(PicoNetwork *n)
{
    char text[7];
    if (!pico_storage_read_text(n, "SS.txt", text, sizeof(text)))
        return false;
    n->app->controller.candidate_shuttle_scale = strtod(text, NULL);
    return true;
}

bool pico_storage_save_shuttle_scale(PicoNetwork *n)
{
    char text[32];
    pico_network_format_float((float)n->app->controller.candidate_shuttle_scale, text, 3);
    return pico_storage_write_text(n, "SS.txt", text, 6);
}

bool pico_storage_reset_shuttle_scale(PicoNetwork *n)
{
    net_ui(n, PICO_NET_UI_RESET_SHUTTLE, NULL);
    net_delay(n, 1000);
    n->app->controller.candidate_shuttle_scale = 0.2;
    return pico_storage_save_shuttle_scale(n);
}

bool pico_storage_compute_uid(PicoNetwork *n)
{
    static const char hex[] = "0123456789abcdef";
    uint8_t mac[6], digest[16];
    uint32_t started = net_now(n);
    n->platform->mac_address(n->platform_context, mac);
    if (!n->platform->md5(n->platform_context, mac, 6, digest, started))
        return false;
    for (unsigned i = 0; i < 16; ++i) {
        n->app->device_uid[i * 2] = hex[digest[i] >> 4];
        n->app->device_uid[i * 2 + 1] = hex[digest[i] & 15];
    }
    n->app->device_uid[32] = 0;
    return true;
}

bool pico_storage_load_uid(PicoNetwork *n)
{
    if (pico_storage_read_text(n, "sys/uid.txt", n->app->device_uid, sizeof(n->app->device_uid)))
        return true;
    if (!pico_storage_compute_uid(n))
        return false;
    pico_storage_write_text(n, "sys/uid.txt", n->app->device_uid, 33);
    return true;
}

void pico_storage_load_preferences(PicoNetwork *n)
{
    char text[6] = {0};
    n->platform->file_begin(n->platform_context);
#if PICO_STANDALONE
    if (pico_local_settings_load(n))
        goto settings_loaded;
#endif
    if (!pico_storage_read_text(n, "userPref.txt", text, sizeof(text))) {
#if !PICO_STANDALONE
        n->ui->choose_language(n->ui_context);
        n->ui->choose_units(n->ui_context);
        pico_storage_save_display(n);
        net_ui(n, PICO_NET_UI_CLEAR, NULL);
#endif
    } else {
        n->settings->language = (uint8_t)atoi(text + 1);
        n->settings->temperature_unit_celsius = text[0] != '0';
    }
#if PICO_STANDALONE
    /* Existing userPref.txt is a migration source, not a second live store. */
    pico_local_settings_save(n);
settings_loaded:
#endif
    if (!pico_storage_load_shuttle_scale(n))
        pico_storage_save_shuttle_scale(n);
    pico_storage_load_uid(n);
}

bool pico_storage_recovery_supported(PicoNetwork *n, uint32_t type)
{
    pico_network_assert(n, type < 6, "isSessionTypeValid(sessionType)");
    if (type < 6)
        return type < 2;
    net_line(n, "Invalid session type argument");
    return false;
}

bool pico_storage_read_restart(PicoNetwork *n, char buffer[101])
{
    return pico_storage_read_text(n, "restart.txt", buffer, 101);
}

void pico_storage_delete_restart(PicoNetwork *n)
{
#if PICO_STANDALONE
    /* A committed tombstone prevents an older bank from reappearing after
     * an interrupted clear. Already-clear/absent records do not wear flash. */
    (void)pico_local_recovery_clear(n);
#endif
    n->platform->file_remove(n->platform_context, "restart.txt");
}

bool pico_storage_save_recovery(PicoNetwork *n, int active, int drain, int step, int resume,
                                bool force, uint8_t calibrated)
{
    uint32_t type = n->app->session.session_type;
    if (!pico_storage_recovery_supported(n, type))
        return false;
    if ((uint32_t)(net_now(n) - n->last_recovery_save_ms) < 60001u && !force)
        return false;
#if PICO_STANDALONE
    if (active < 0 || active > 599 || drain < 0 || drain > 200 || step < 0 ||
        step >= n->app->recipe.step_count || resume < 0 || resume > 5 || resume == 4 ||
        calibrated > 1 || !pico_local_recipe_id_valid(n->app->recipe.identifier))
        return false;
    PicoLocalRecovery record = {.session_type = (uint8_t)type, .step = (uint8_t)step,
        .phase = (uint8_t)resume, .calibrated = calibrated,
        .active_minutes = (uint16_t)active, .drain_minutes = (uint16_t)drain};
    memcpy(record.recipe_id, n->app->recipe.identifier, sizeof record.recipe_id);
    if (!pico_local_recovery_save(n, &record)) return false;
    n->last_recovery_save_ms = net_now(n);
    return true;
#else
    n->last_recovery_save_ms = net_now(n);
    char text[101];
    int size =
        snprintf(text, sizeof(text), "%u:%s:%d:%d:%d:%d:%d:%d:%u:", (unsigned)type,
                 n->app->recipe.identifier, (int)n->app->recipe.abv_adjustment,
                 (int)n->app->recipe.ibu_adjustment, step, active, drain, resume, calibrated);
    return net_request_fits(size, sizeof(text)) &&
           pico_storage_write_text(n, "restart.txt", text, 100);
#endif
}

bool pico_storage_recovery_image_exists(PicoNetwork *n)
{
    bool opened = n->platform->file_open(n->platform_context, "/sys/mcuimg1.bin", 0);
    n->platform->file_close(n->platform_context);
    return opened;
}
