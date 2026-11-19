/* Pico Pro 0.1.34 — wifi. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014a24. */
void destroy_cached_wifi_clients(void)
{
    uint32_t *client;
    uint32_t *pointer_1;

    client = (uint32_t *)&g_wifi_unknown_04;
    do {
        pointer_1 = client + -0x4a;
        WiFiClient_destroy(client);
        client = pointer_1;
    } while (pointer_1 != &g_carousel_last_update_ms);
    return;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014a70. */
uint32_t WiFi_initialize(void)
{
    int sl_start_result;
    uint32_t value_2;

    if (wifi_initialized != '\0') {
        return 1;
    }
    unresolved_simplelink_protocol_48();
    sl_start_result = sl_Start(0, 0, 0);
    value_2 = 0;
    if ((1 < sl_start_result + 2U) && (sl_start_result != -3)) {
        if (sl_start_result != 0) {
            sl_WlanSetMode('\0');
            sl_Stop(0);
            sl_Start(0, 0, 0);
        }
        sl_WlanDisconnect();
        value_2 = 1;
        unresolved_simplelink_protocol_31((char *)0x0, '\0');
        wifi_initialized = '\x01';
        sl_NetAppStop(1);
        sl_WlanRxStatStart();
    }
    return value_2;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014ad4. */
uint32_t WiFi_find_unused_socket(void)
{
    uint32_t value_1;

    value_1 = 0;
    do {
        if ((&wifi_socket_handles)[value_1] == -1) {
            return value_1 & 0xff;
        }
        value_1 = value_1 + 1;
    } while (value_1 != 8);
    return 0xff;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014af0. */
void WiFi_enable_dhcp_if_unconfigured(uint32_t argument_1, uint32_t argument_2, uint32_t argument_3)
{
    uint8_t byte_value_9;
    uint32_t value_8;

    if (g_wifi_unknown_07 == 0) {
        value_8 = argument_3;
        sl_NetCfgSet('\x04', '\x01', '\x01', &byte_value_9);
    }
    return;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014b14. */
void WiFi_set_dns(uint32_t argument_1, int dnsAddress)
{
    uint8_t byte_value_21;
    uint8_t buffer_20[12];
    uint32_t value_14;

    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    libc_memset(buffer_20, 0, 0x10);
    byte_value_21 = '\x10';
    sl_NetCfgGet('\x03', (uint8_t *)0x0, &byte_value_21, buffer_20);
    value_14 = (uint32_t) * (uint8_t *)(dnsAddress + 6) << 8 |
               (uint32_t) * (uint8_t *)(dnsAddress + 4) << 0x18 |
               (uint32_t) * (uint8_t *)(dnsAddress + 7) |
               (uint32_t) * (uint8_t *)(dnsAddress + 5) << 0x10;
    sl_NetCfgSet('\x05', '\x01', '\x10', buffer_20);
    return;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014b6c. */
uint32_t WiFi_disconnect(void)
{
    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    sl_WlanDisconnect();
    wifi_begin_called_as_uint8_t = 0;
    wifi_initialized = 0;
    wifi_connection_status = 6;
    return 6;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014b9c. */
uint32_t *WiFi_get_mac_address(uint32_t argument_1, uint32_t *macAddress, uint32_t argument_3,
                               uint32_t argument_4)
{
    uint32_t value_14;
    uint32_t value_10;
    uint32_t value;

    value_14 = macAddress;
    value_10 = argument_3;
    value = argument_4;
    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    value_14 = (uint32_t *)pico_join_u8_u24(6, (uint32_t)value_14);
    sl_NetCfgGet('\x02', (uint8_t *)0x0, (uint8_t *)((int)&value_14 + 3), (uint8_t *)&value_10);
    *macAddress = value_10;
    *(uint16_t *)(macAddress + 1) = (uint16_t)value;
    return macAddress;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014bd0. */
uint32_t *WiFi_get_local_ip(uint32_t *resultAddress)
{
    uint32_t sl_htonl_result;
    uint8_t byte_value_21;
    uint32_t buffer_20[5];

    thunk_FUN_20017574();
    libc_memset(buffer_20, 0, 0x10);
    byte_value_21 = '\x10';
    sl_NetCfgGet('\x03', (uint8_t *)0x0, &byte_value_21, (uint8_t *)buffer_20);
    IPAddress_construct_octets(resultAddress, 0, 0, 0, 0);
    sl_htonl_result = sl_Htonl(buffer_20[0]);
    IPAddress_assign_word((int)resultAddress, sl_htonl_result);
    return resultAddress;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014c1c. */
void WiFi_get_ip_configuration(uint32_t argument_1, int localAddress, int dnsAddress,
                               int gatewayAddress, int subnetMask)
{
    uint32_t sl_htonl_result;
    uint8_t byte_value_29;
    uint32_t value_28;
    uint32_t value_24;
    uint32_t value_20;
    uint32_t value;

    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    libc_memset(&value_28, 0, 0x10);
    byte_value_29 = '\x10';
    sl_NetCfgGet('\x03', (uint8_t *)0x0, &byte_value_29, (uint8_t *)&value_28);
    sl_htonl_result = sl_Htonl(value_28);
    IPAddress_assign_word(localAddress, sl_htonl_result);
    sl_htonl_result = sl_Htonl(value);
    IPAddress_assign_word(dnsAddress, sl_htonl_result);
    sl_htonl_result = sl_Htonl(value_20);
    IPAddress_assign_word(gatewayAddress, sl_htonl_result);
    sl_htonl_result = sl_Htonl(value_24);
    IPAddress_assign_word(subnetMask, sl_htonl_result);
    return;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014c90. */
int WiFi_scan_networks(void)
{
    char byte_value_1;
    short sl_wlan_policy_set_result;
    uint8_t buffer_384[888];

    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    sl_wlan_policy_set_result = sl_WlanPolicySet('\x10', '\0', (uint8_t *)0x0, '\0');
    if (sl_wlan_policy_set_result == 0) {
        buffer_384[0] = '\n';
        buffer_384[1] = '\0';
        buffer_384[2] = '\0';
        buffer_384[3] = '\0';
        sl_wlan_policy_set_result = sl_WlanPolicySet(' ', '\x01', buffer_384, '\x04');
        if (sl_wlan_policy_set_result == 0) {
            delay(300);
            sl_wlan_policy_set_result = sl_WlanGetNetworkList('\0', '\x14', buffer_384 + 4);
            wifi_scan_result_count = (int)sl_wlan_policy_set_result;
            sl_WlanPolicySet(' ', '\0', (uint8_t *)0x0, '\0');
            byte_value_1 = (char)wifi_scan_result_count;
            goto branch_1;
        }
    }
    byte_value_1 = '\0';
branch_1:
    return (int)byte_value_1;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014d00. */
uint8_t *WiFi_scan_result_ssid(uint32_t argument_1, int networkIndex, uint32_t argument_3,
                               uint32_t argument_4)
{
    int value_1;
    uint8_t *pointer_2;
    uint32_t value_18;

    value_18 = argument_4;
    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    if (networkIndex < wifi_scan_result_count) {
        value_1 = -(wifi_scan_result_count * 0x2c + 7U & 0xfffffff8);
        libc_memset((void *)((int)&value_18 + value_1), 0, wifi_scan_result_count * 0x2c);
        sl_WlanGetNetworkList('\0', (uint8_t)wifi_scan_result_count,
                              (void *)((int)&value_18 + value_1));
        libc_strcpy(&wifi_scan_ssid_buffer,
                    (char *)((int)&value_18 + networkIndex * 0x2c + value_1));
        pointer_2 = &wifi_scan_ssid_buffer;
    } else {
        pointer_2 = (uint8_t *)0x0;
    }
    return pointer_2;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014d60. */
uint32_t WiFi_scan_result_security(uint32_t argument_1, int networkIndex, uint32_t argument_3,
                                   uint32_t argument_4)
{
    int value_1;
    uint32_t value_2;
    uint8_t u_stack00000009;
    uint32_t value_18;

    value_18 = argument_4;
    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    if (networkIndex < wifi_scan_result_count) {
        value_1 = -(wifi_scan_result_count * 0x2c + 7U & 0xfffffff8);
        libc_memset((void *)((int)&value_18 + value_1), 0, wifi_scan_result_count * 0x2c);
        sl_WlanGetNetworkList('\0', (uint8_t)wifi_scan_result_count,
                              (void *)((int)&value_18 + value_1));
        value_2 = (uint32_t)(uint8_t)(&u_stack00000009)[networkIndex * 0x2c + value_1];
        if (value_2 != 2) {
            if (value_2 == 1) {
                value_2 = 5;
            } else if (value_2 == 0) {
                value_2 = 7;
            } else if (value_2 - 3 < 2) {
                value_2 = 2;
            } else {
                value_2 = 8;
            }
        }
    } else {
        value_2 = 0;
    }
    return value_2;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014dd4. */
uint8_t WiFi_status(void)
{
    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    if (g_wifi_unknown_06 == '\x02') {
        return 7;
    }
    thunk_FUN_20017574();
    return wifi_connection_status;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014e94. */
uint8_t WiFi_begin_with_passphrase(uint32_t argument_1, char *ssid, uint32_t passphrase,
                                   char *argument_4)
{
    uint8_t wi_fi_status_result;
    int wi_fi_initialize_result;
    uint32_t text_length;
    uint32_t text_length_2;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    char buffer[4];
    char *text_28;
    uint8_t byte_value_24;

    if (wifi_begin_called == '\0') {
        wi_fi_initialize_result = WiFi_initialize();
        if (wi_fi_initialize_result != 0) {
            sl_WlanProfileDel(0xff);
            WiFi_enable_dhcp_if_unconfigured(argument_1, unresolved_return_r1,
                                             unresolved_return_r2);
            sl_WlanPolicySet('\x10', '\x02', (uint8_t *)0x0, '\0');
            text_length = libc_strlen(ssid);
            libc_memset(buffer, 0, 0xc);
            buffer[0] = '\x01';
            text_28 = argument_4;
            text_length_2 = libc_strlen(argument_4);
            byte_value_24 = (uint8_t)text_length_2;
            wi_fi_initialize_result = unresolved_simplelink_protocol_49(
                ssid, (int)(short)text_length, (uint32_t *)0x0, buffer, (uint32_t *)0x0);
            if (wi_fi_initialize_result == 0) {
                unresolved_simplelink_protocol_50(ssid, (int)(short)text_length, (uint32_t *)0x0,
                                                  buffer, (uint32_t *)0x0, 6);
                wifi_begin_called = '\x01';
                goto branch_1;
            }
        }
        wi_fi_status_result = 4;
    } else {
        delay(500);
    branch_1:
        wi_fi_status_result = WiFi_status();
    }
    return wi_fi_status_result;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20014fdc. */
int WiFi_host_by_name(uint32_t argument_1, char *hostname, int resultAddress)
{
    short sl_net_app_dns_get_host_by_name_result;
    uint32_t text_length;
    uint32_t sl_htonl_result;
    int value_4;
    char *text_14;
    int value_10;

    text_14 = hostname;
    value_10 = resultAddress;
    if (wifi_initialized == '\0') {
        WiFi_initialize();
    }
    text_length = libc_strlen(hostname);
    sl_net_app_dns_get_host_by_name_result =
        sl_NetAppDnsGetHostByName(hostname, (uint16_t)text_length, (uint32_t *)&text_14, '\x02');
    value_4 = (int)sl_net_app_dns_get_host_by_name_result;
    sl_htonl_result = sl_Htonl((uint32_t)text_14);
    IPAddress_assign_word(resultAddress, sl_htonl_result);
    if (-1 < value_4) {
        value_4 = 1;
    }
    return value_4;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20015020. */
void WiFi_register_connected_device(uint32_t *ipAddress, uint32_t *macAddress)
{
    int value_1;
    int libc_memcmp_result;
    int value_3;

    value_3 = 0;
    while ((value_1 = value_3 * 0xb,
            (&connected_device_table)[value_1] == '\0' ||
                (libc_memcmp_result =
                     libc_memcmp((void *)((int)&g_wifi_unknown_02 + value_1), macAddress, 6),
                 libc_memcmp_result != 0))) {
        value_3 = value_3 + 1;
        if (value_3 == 4) {
            value_3 = 0;
            do {
                value_1 = value_3 * 0xb;
                if ((&connected_device_table)[value_1] == '\0') {
                    (&connected_device_table)[value_1] = 1;
                    *(uint32_t *)((int)&g_wifi_unknown_01 + value_1) = *ipAddress;
                    *(uint32_t *)((int)&g_wifi_unknown_02 + value_1) = *macAddress;
                    *(uint16_t *)((int)&g_wifi_unknown_03 + value_1) =
                        *(uint16_t *)(macAddress + 1);
                    return;
                }
                value_3 = value_3 + 1;
            } while (value_3 != 4);
            return;
        }
    }
    *(uint32_t *)((int)&g_wifi_unknown_01 + value_1) = *ipAddress;
    wifi_last_connected_device = value_3;
    return;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x20015098. */
void WiFi_unregister_connected_device(void *macAddress)
{
    int value_1;
    int libc_memcmp_result;
    int value_3;

    value_3 = 0;
    while (true) {
        value_1 = value_3 * 0xb;
        if (((&connected_device_table)[value_1] != '\0') &&
            (libc_memcmp_result =
                 libc_memcmp((void *)((int)&g_wifi_unknown_02 + value_1), macAddress, 6),
             libc_memcmp_result == 0))
            break;
        value_3 = value_3 + 1;
        if (value_3 == 4) {
            return;
        }
    }
    (&connected_device_table)[value_1] = 0;
    return;
}

/* Control flow and SimpleLink operations match the named WiFiClass method in pinned Energia
 * libraries/WiFi/WiFi.cpp; original methods have an implicit C++ self argument when used. Firmware
 * address: 0x200150cc. */
uint8_t *initialize_global_wifi(void)
{
    uint8_t *client;
    int value_1;

    value_1 = 0;
    do {
        client = &cached_wifi_clients + value_1;
        value_1 = value_1 + 0x128;
        WiFiClient_construct(client);
    } while (value_1 != 0x940);
    unresolved_c_library_01(0, 0x20014a25, &g_initialization_unknown_25);
    IPAddress_construct_zero((uint32_t *)&g_wifi_unknown_05);
    value_1 = 0;
    do {
        *(uint16_t *)((int)&g_wifi_unknown_08 + value_1) = 0xffff;
        *(uint16_t *)((int)&wifi_socket_types + value_1) = 0xffff;
        *(uint16_t *)((int)&wifi_socket_ports + value_1) = 0xffff;
        *(uint16_t *)((int)&wifi_socket_handles + value_1) = 0xffff;
        value_1 = value_1 + 2;
    } while (value_1 != 0x10);
    return &global_wifi;
}
