#ifndef PICOBREW_NETWORK_H
#define PICOBREW_NETWORK_H

#include "picobrew/config.h"
#include "picobrew/state.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PicoIpv4 {
    uint8_t octet[4];
} PicoIpv4;

typedef struct PicoSettings {
    char wifi_ssid[34];
    char wifi_password[65];
    uint8_t wifi_security;
    uint8_t language;
    uint8_t temperature_unit_celsius;
    uint8_t offline_mode;
    uint8_t detailed_status;
    uint8_t legacy_compartment_mapping;
    uint8_t home_compartment;
} PicoSettings;

typedef enum PicoNetworkUiEvent {
    PICO_NET_UI_CLEAR,
    PICO_NET_UI_DEVICE,
    PICO_NET_UI_DEVICE_CONNECTED,
    PICO_NET_UI_DEVICE_FAILED,
    PICO_NET_UI_FLUSH,
    PICO_NET_UI_CONNECTING,
    PICO_NET_UI_STOP_PROGRESS,
    PICO_NET_UI_REGISTRATION_CODE,
    PICO_NET_UI_REFRESH_PAGE,
    PICO_NET_UI_UPDATING,
    PICO_NET_UI_DOWNLOADING,
    PICO_NET_UI_UPDATE_FAILED,
    PICO_NET_UI_STILL_CONNECTING,
    PICO_NET_UI_STILL_EXITING,
    PICO_NET_UI_RESET_SHUTTLE
} PicoNetworkUiEvent;

/* All used callbacks are real integration boundaries. No optional successful
 * defaults are supplied. File open/close results are normalized to bool by the
 * SDK bridge because Pico's SLFS fork and upstream use different conventions. */
typedef struct PicoNetworkPlatform {
    uint32_t (*millis)(void *context);
    void (*delay_ms)(void *context, uint32_t milliseconds);
    void (*serial_write)(void *context, const char *text);
    bool (*wifi_initialized)(void *context);
    bool (*wifi_initialize)(void *context);
    bool (*wifi_connecting)(void *context);
    int (*wifi_status)(void *context);
    int (*wifi_begin)(void *context, const char *ssid, const uint8_t *password, uint8_t security,
                      bool binary_wep);
    void (*wifi_disconnect)(void *context);
    PicoIpv4 (*local_ip)(void *context);
    int (*resolve_host)(void *context, const char *host, PicoIpv4 *address);
    void (*set_dns)(void *context, PicoIpv4 address);
    int (*scan_networks)(void *context);
    const char *(*scan_ssid)(void *context, unsigned index);
    uint8_t (*scan_security)(void *context, unsigned index);
    int32_t (*scan_rssi)(void *context, unsigned index);
    void (*mac_address)(void *context, uint8_t mac[6]);
    bool (*md5)(void *context, const uint8_t *data, size_t length, uint8_t digest[16],
                uint32_t started_ms);
    int (*client_connect)(void *context, PicoIpv4 address, uint16_t port, bool background);
    int (*client_connected)(void *context);
    int (*client_available)(void *context);
    int (*client_read)(void *context);
    size_t (*client_write)(void *context, const uint8_t *data, size_t length);
    void (*client_flush)(void *context);
    void (*client_stop)(void *context);
    void (*file_begin)(void *context);
    bool (*file_open)(void *context, const char *name, uint32_t mode);
    int (*file_read)(void *context);
    int32_t (*file_write)(void *context, const uint8_t *data, size_t length);
    bool (*file_close)(void *context);
    int (*file_remove)(void *context, const char *name);
    int (*boot_info_read)(void *context, uint8_t data[8]);
    int (*boot_info_write)(void *context, const uint8_t data[8]);
    void (*reset_mcu)(void *context);
    uint8_t (*ping_public_dns)(void *context);
    void (*ip_configuration)(void *context, PicoIpv4 *local, PicoIpv4 *dns, PicoIpv4 *gateway,
                             PicoIpv4 *mask);
    /* Serial's native single-precision formatter. The value is widened at
     * this C boundary and converted back to float by the SDK adapter. */
    void (*serial_write_float)(void *context, double value, unsigned digits);
} PicoNetworkPlatform;

typedef struct PicoNetworkUi {
    void (*event)(void *context, PicoNetworkUiEvent event, const char *text);
    void (*report_error)(void *context, int code, bool wait, bool report_to_server);
    bool (*choose_wifi_and_connect)(void *context);
    void (*diagnose_wifi)(void *context);
    void (*choose_language)(void *context);
    void (*choose_units)(void *context);
    void (*wait_for_continue)(void *context);
    int (*choose_still)(void *context, const char identifiers[][22], size_t count);
    bool (*decode_recipe)(void *context, const char *text, size_t length);
    void (*recipe_logo_ready)(void *context, const uint8_t image[1024]);
    void (*recipe_image_ready)(void *context, unsigned index, const uint8_t image[1024]);
} PicoNetworkUi;

typedef struct PicoStillConnection {
    PicoIpv4 home_ip;
    uint8_t lan_connected;
    PicoIpv4 active_ip;
    uint8_t session_started;
    char ssid[32];
    char identifier[16];
    uint32_t consecutive_failures;
} PicoStillConnection;

typedef struct PicoRecipeListing {
    int total_items;
    int page_offset;
    int loaded_items;
    int menu_item_count;
    char identifiers[19][22];
    char titles[19][22];
    char menu_labels[19][22];
} PicoRecipeListing;

typedef struct PicoNetwork {
    PicoAppState *app;
    PicoSettings *settings;
    const PicoNetworkPlatform *platform;
    void *platform_context;
    const PicoNetworkUi *ui;
    void *ui_context;
    const char *server_host; /* NULL in standalone; original host in legacy. */
    uint16_t server_port;
    PicoIpv4 server_ip;
    PicoStillConnection still;
    uint32_t last_recovery_save_ms;
    uint32_t assertion_count;
    uint32_t log_consecutive_failures;
    uint8_t boot_info[8];
#if !PICO_STANDALONE
    uint8_t recipe_logo[1024];
    uint8_t recipe_images[4][1024];
#endif
} PicoNetwork;

void pico_network_initialize(PicoNetwork *network, PicoAppState *app, PicoSettings *settings,
                             const PicoNetworkPlatform *platform, void *platform_context,
                             const PicoNetworkUi *ui, void *ui_context);
void pico_network_log(PicoNetwork *network, const char *level, const char *component,
                      const char *message);
void pico_network_assert(PicoNetwork *network, bool condition, const char *expression);
bool pico_network_parse_ipv4(const char *text, PicoIpv4 *address);
bool pico_network_ipv4_is_zero(PicoIpv4 address);
int pico_network_connect_access_point(PicoNetwork *network);
bool pico_network_connect_stored(PicoNetwork *network);
bool pico_network_reconnect(PicoNetwork *network);
bool pico_network_resolve_address(PicoNetwork *network);
bool pico_network_wait_local_ip(PicoNetwork *network, uint32_t timeout_ms, bool show_error);
bool pico_network_resolve_server(PicoNetwork *network);
int pico_network_check_status(PicoNetwork *network, bool background, bool resolve_server);
bool pico_network_connect_with_screen(PicoNetwork *network);
bool pico_network_ensure_connection(PicoNetwork *network);
void pico_network_close(PicoNetwork *network);
bool pico_network_open_request(PicoNetwork *network, const char *request, bool keep_open,
                               bool background);
bool pico_network_call(PicoNetwork *network, const char *request, bool wait_response,
                       bool background);
bool pico_network_wait_hash(PicoNetwork *network);
bool pico_network_read_response(PicoNetwork *network, char *buffer, size_t capacity, size_t *length,
                                bool close_after, uint8_t terminator);
bool pico_network_read_hex_image(PicoNetwork *network, uint8_t image[1024], uint8_t terminator);
size_t pico_network_url_encode(const char *input, char *output, size_t capacity);
char *pico_network_format_float(float value, char *output, unsigned decimals);

bool pico_storage_read_text(PicoNetwork *network, const char *name, char *buffer, size_t capacity);
bool pico_storage_write_text(PicoNetwork *network, const char *name, const char *text,
                             uint32_t file_capacity);
uint32_t pico_storage_create_mode(uint32_t maximum_size, uint32_t flags);
bool pico_storage_load_wifi(PicoNetwork *network);
bool pico_storage_save_wifi(PicoNetwork *network);
void pico_storage_delete_wifi(PicoNetwork *network);
void pico_storage_delete_user_files(PicoNetwork *network);
bool pico_storage_save_display(PicoNetwork *network);
void pico_storage_load_preferences(PicoNetwork *network);
bool pico_storage_compute_uid(PicoNetwork *network);
bool pico_storage_load_uid(PicoNetwork *network);
bool pico_storage_load_shuttle_scale(PicoNetwork *network);
bool pico_storage_save_shuttle_scale(PicoNetwork *network);
bool pico_storage_reset_shuttle_scale(PicoNetwork *network);
bool pico_storage_recovery_supported(PicoNetwork *network, uint32_t session_type);
bool pico_storage_read_restart(PicoNetwork *network, char buffer[101]);
void pico_storage_delete_restart(PicoNetwork *network);
bool pico_storage_save_recovery(PicoNetwork *network, int active_minutes, int drain_minutes,
                                int step_index, int resume_state, bool force,
                                uint8_t calibration_complete);
bool pico_storage_recovery_image_exists(PicoNetwork *network);

bool pico_api_register(PicoNetwork *network);
bool pico_api_get_session(PicoNetwork *network, char *session_id, size_t capacity);
bool pico_api_create_picopak(PicoNetwork *network, int recipe_id);
int pico_api_required_actions(PicoNetwork *network, uint32_t *actions, size_t capacity);
int pico_api_fetch_recipe(PicoNetwork *network, const char *identifier, int ibu, int abv);
void pico_api_report_state(PicoNetwork *network);
bool pico_api_error(PicoNetwork *network, int code);
bool pico_api_log_event(PicoNetwork *network, const char *step, const char *event);
bool pico_api_log_step(PicoNetwork *network, const char *step);
bool pico_api_log_progress(PicoNetwork *network, const char *step);
bool pico_api_recipe_list(PicoNetwork *network, PicoRecipeListing *listing, bool associated);
bool pico_api_still_permission(PicoNetwork *network, const char *method, const char *still_uid);

void pico_still_connection_initialize(PicoStillConnection *connection);
bool pico_still_set_identifier(PicoNetwork *network, const char *identifier);
void pico_still_select_network(PicoNetwork *network, bool home);
bool pico_still_parse_response(PicoNetwork *network, PicoStillTelemetry *telemetry,
                               const char *response);
bool pico_still_pressure_ok(const PicoStillTelemetry *telemetry);
bool pico_still_call(PicoNetwork *network, unsigned action, char *response, size_t capacity);
bool pico_still_send_wifi(PicoNetwork *network);
bool pico_still_get_ip(PicoNetwork *network);
bool pico_still_connect_lan(PicoNetwork *network);
bool pico_still_read_status(PicoNetwork *network);
bool pico_still_blocked(PicoNetwork *network);
bool pico_still_choose_access_point(PicoNetwork *network);
bool pico_still_connect_wifi(PicoNetwork *network, bool home);
bool pico_still_wait_response(PicoNetwork *network, bool home, uint32_t delay_seconds,
                              uint32_t timeout_seconds);
bool pico_still_configure(PicoNetwork *network);
bool pico_still_start(PicoNetwork *network);
bool pico_still_start_once(PicoNetwork *network);
bool pico_still_stop(PicoNetwork *network);
void pico_still_set_fan(PicoNetwork *network, bool enabled);

bool pico_update_checksum(PicoNetwork *network, const char *expected, int32_t nibble_sum);
bool pico_update_open_destination(PicoNetwork *network, bool recovery_image);
bool pico_update_receive(PicoNetwork *network, bool recovery_image);
bool pico_update_available(PicoNetwork *network);
bool pico_update_download(PicoNetwork *network);
void pico_update_run(PicoNetwork *network);
int pico_boot_read(PicoNetwork *network);
int pico_boot_write(PicoNetwork *network);
bool pico_boot_select_factory(PicoNetwork *network);
bool pico_boot_toggle_application(PicoNetwork *network, bool activate);

#ifdef __cplusplus
}
#endif
#endif
