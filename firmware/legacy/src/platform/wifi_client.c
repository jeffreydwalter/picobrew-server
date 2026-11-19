/* Pico Pro 0.1.34 — wifi client. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015110. */
uint32_t WiFiClient_write_byte(void *client, uint8_t value)
{
    uint32_t value_1;
    uint32_t unresolved_input_register_00000025;
    uint8_t buffer_6[6];

    if (*(int *)((int)client + 0x14) == 0xff) {
        value_1 = 0;
    } else {
        value_1 = (**(PicoOpaqueFunction **)(*(int *)client + 4))(
            client, buffer_6 + -3, 1, *(PicoOpaqueFunction **)(*(int *)client + 4), client,
            pico_join_u8_u24(value,
                             (int32_t)pico_join_u24_u8(unresolved_input_register_00000025, value)));
    }
    return value_1;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015132. */
int WiFiClient_read_byte(void *client)
{
    int value_1;

    value_1 = (**(PicoOpaqueFunction **)(*(int *)client + 8))();
    if (value_1 == 0) {
        return -1;
    }
    value_1 = *(int *)((int)client + 0x11c);
    *(int *)((int)client + 0x11c) = value_1 + 1;
    return (uint32_t) * (uint8_t *)((int)client + value_1 + 0x18);
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015154. */
int WiFiClient_peek(void *client)
{
    uint32_t value_1;

    if (*(int *)((int)client + 0x11c) < *(int *)((int)client + 0x118)) {
        value_1 = (uint32_t) * (uint8_t *)((int)client + *(int *)((int)client + 0x11c) + 0x18);
    } else {
        value_1 = 0xffffffff;
    }
    return value_1;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x2001516a. */
bool WiFiClient_is_valid(void *client)
{
    return *(int *)((int)client + 0x14) != 0xff;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015174. */
int WiFiClient_connect_hostname(void *client, char *hostname, uint16_t port)
{
    int wi_fi_host_by_name_result;
    int value_2;
    uint16_t unresolved_input_register_0000002a;
    uint32_t value_20;
    uint32_t value;
    uint8_t **pointer_18;
    uint32_t value_14;

    IPAddress_construct_octets(&value_20, 0, 0, 0, 0);
    wi_fi_host_by_name_result = WiFi_host_by_name(&global_wifi, hostname, (int)&value_20);
    value_2 = 0;
    if (wi_fi_host_by_name_result != 0) {
        pointer_18 = &g_network_addresses_unknown_01;
        value_14 = value;
        value_2 = (**(PicoOpaqueFunction **)(*(int *)client + 0x18))(
            client, &pointer_18, pico_join_u16_u16(unresolved_input_register_0000002a, port));
    }
    return value_2;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x200151b8. */
int WiFiClient_ssl_connect_hostname(void *client, char *hostname, uint16_t port)
{
    int wi_fi_host_by_name_result;
    int value_2;
    uint16_t unresolved_input_register_0000002a;
    uint32_t value_20;
    uint32_t value;
    uint8_t **pointer_18;
    uint32_t value_14;

    IPAddress_construct_octets(&value_20, 0, 0, 0, 0);
    wi_fi_host_by_name_result = WiFi_host_by_name(&global_wifi, hostname, (int)&value_20);
    value_2 = 0;
    if (wi_fi_host_by_name_result != 0) {
        pointer_18 = &g_network_addresses_unknown_01;
        value_14 = value;
        value_2 = (**(PicoOpaqueFunction **)(*(int *)client + 0x24))(
            client, &pointer_18, pico_join_u16_u16(unresolved_input_register_0000002a, port));
    }
    return value_2;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x200151fc. */
void WiFiClient_flush(void *client)
{
    libc_memset((void *)((int)client + 0x18), 0, 0xff);
    *(uint32_t *)((int)client + 0x118) = 0;
    *(uint32_t *)((int)client + 0x11c) = 0;
    return;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015218. */
int WiFiClient_available(void *client)
{
    short sl_recv_result;
    int value_2;
    uint32_t value_28;
    uint32_t buffer_24[4];

    if (*(int *)((int)client + 0x14) != 0xff) {
        value_2 = *(int *)((int)client + 0x118) - *(int *)((int)client + 0x11c);
        if (0 < value_2) {
            return value_2;
        }
        buffer_24[2] = 500;
        buffer_24[1] = 0;
        unresolved_simplelink_protocol_47(&value_28);
        unresolved_simplelink_protocol_47(buffer_24);
        unresolved_simplelink_protocol_45(
            (int)(short)(&wifi_socket_handles)[*(int *)((int)client + 0x14)], &value_28);
        unresolved_simplelink_protocol_45(
            (int)(short)(&wifi_socket_handles)[*(int *)((int)client + 0x14)], buffer_24);
        sl_recv_result = sl_Recv((&wifi_socket_handles)[*(int *)((int)client + 0x14)],
                                 (void *)((int)client + 0x18), 0xff, 0);
        value_2 = (int)sl_recv_result;
        if ((value_2 < 1) && (value_2 != -0xb)) {
            sl_Close((&wifi_socket_handles)[*(int *)((int)client + 0x14)]);
            value_2 = *(int *)((int)client + 0x14);
            (&wifi_socket_ports)[value_2] = 0xffff;
            (&wifi_socket_handles)[value_2] = 0xffff;
            (&wifi_socket_types)[value_2] = 0xffff;
            *(uint32_t *)((int)client + 0x14) = 0xff;
            libc_memset((void *)((int)client + 0x18), 0, 0xff);
        } else {
            if (value_2 == -0xb) {
                value_2 = 0;
            }
            *(uint32_t *)((int)client + 0x11c) = 0;
            *(int *)((int)client + 0x118) = value_2;
            if (-1 < value_2) {
                return value_2;
            }
        }
    }
    return 0;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x200152dc. */
int WiFiClient_read_buffer(void *client, uint8_t *buffer, uint32_t count)
{
    int value_1;
    uint32_t count_00;

    value_1 = (**(PicoOpaqueFunction **)(*(int *)client + 8))();
    count_00 = 0;
    if (value_1 != 0) {
        count_00 = *(int *)((int)client + 0x118) - *(int *)((int)client + 0x11c);
        if (count <= count_00) {
            count_00 = count;
        }
        libc_memcpy(buffer, (void *)(*(int *)((int)client + 0x11c) + 0x18 + (int)client), count_00);
        *(uint32_t *)((int)client + 0x11c) = *(int *)((int)client + 0x11c) + count_00;
    }
    return count_00;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015318. */
void WiFiClient_stop(void *client)
{
    short sl_close_result;
    int value_2;

    if ((*(int *)((int)client + 0x14) != 0xff) &&
        (sl_close_result = sl_Close((&wifi_socket_handles)[*(int *)((int)client + 0x14)]),
         -1 < sl_close_result)) {
        value_2 = *(int *)((int)client + 0x14);
        (&wifi_socket_ports)[value_2] = 0xffff;
        (&wifi_socket_handles)[value_2] = 0xffff;
        (&wifi_socket_types)[value_2] = 0xffff;
        *(uint32_t *)((int)client + 0x14) = 0xff;
    }
    return;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015358. */
uint32_t WiFiClient_write_buffer(void *client, uint8_t *buffer, uint32_t count)
{
    short sl_send_result;
    uint32_t value_2;

    if (*(int *)((int)client + 0x14) != 0xff) {
        sl_send_result = (&wifi_socket_handles)[*(int *)((int)client + 0x14)];
        while (true) {
            sl_send_result = sl_Send(sl_send_result, buffer, (short)count, 0);
            value_2 = (uint32_t)sl_send_result;
            if (value_2 != 0xfffffff5)
                break;
            delay(10);
            thunk_FUN_20017574();
            sl_send_result = (&wifi_socket_handles)[*(int *)((int)client + 0x14)];
        }
        if ((-1 < (int)value_2) && (value_2 == count)) {
            return value_2;
        }
        (**(PicoOpaqueFunction **)(*(int *)client + 0x2c))(client);
    }
    return 0;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x200153ac. */
int WiFiClient_connect_ipv4_blocking(void *client, void *ipAddress, uint16_t port)
{
    short socket_descriptor;
    short sl_connect_result;
    uint32_t wi_fi_find_unused_socket_result;
    uint32_t value_38;
    uint32_t value_34;
    uint16_t short_value_30;
    uint16_t sl_htons_result;
    uint32_t value;

    if (((*(int *)((int)client + 0x14) == 0xff) &&
         (wi_fi_find_unused_socket_result = WiFi_find_unused_socket(),
          wi_fi_find_unused_socket_result != 0xff)) &&
        (socket_descriptor = sl_Socket(2, 1, 6), -1 < socket_descriptor)) {
        libc_memset(&short_value_30, 0, 0x10);
        short_value_30 = 2;
        sl_htons_result = sl_Htons(port);
        value = *(uint32_t *)((int)ipAddress + 4);
        sl_connect_result = sl_Connect(socket_descriptor, &short_value_30, 0x10);
        if (-1 < sl_connect_result) {
            value_38 = 2;
            value_34 = 0;
            sl_SetSockOpt(socket_descriptor, 1, 0x18, &value_38, 8);
            sl_SetSockOpt(socket_descriptor, 1, 9, &value_38, 8);
            *(uint32_t *)((int)client + 0x14) = wi_fi_find_unused_socket_result;
            (&wifi_socket_handles)[wi_fi_find_unused_socket_result] = socket_descriptor;
            (&wifi_socket_types)[wi_fi_find_unused_socket_result] = 1;
            (&wifi_socket_ports)[wi_fi_find_unused_socket_result] = port;
            return 1;
        }
        sl_Close(socket_descriptor);
    }
    return 0;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015470. */
int WiFiClient_connect_ipv4(void *client, void *ipAddress, uint16_t port)
{
    bool condition_1;
    short sd;
    short sl_connect_result;
    uint32_t wi_fi_find_unused_socket_result;
    uint32_t value_4;
    uint32_t value_40;
    uint32_t buffer[3];
    uint16_t short_value_30;
    uint16_t sl_htons_result;
    uint32_t value;

    if ((*(int *)((int)client + 0x14) == 0xff) &&
        (wi_fi_find_unused_socket_result = WiFi_find_unused_socket(),
         wi_fi_find_unused_socket_result != 0xff)) {
        sd = sl_Socket(2, 1, 6);
        value_4 = (uint32_t)sd;
        if (-1 < (int)value_4) {
            value_40 = 1;
            sl_SetSockOpt(sd, 1, 0x18, &value_40, 4);
            libc_memset(&short_value_30, 0, 0x10);
            short_value_30 = 2;
            sl_htons_result = sl_Htons(port);
            value = *(uint32_t *)((int)ipAddress + 4);
            sl_connect_result = sl_Connect(sd, &short_value_30, 0x10);
            if (-1 < sl_connect_result) {
            branch_1:
                buffer[1] = 1;
                sl_SetSockOpt(sd, 1, 0x18, buffer + 1, 4);
                sl_SetSockOpt(sd, 1, 9, buffer + 1, 4);
                *(uint32_t *)((int)client + 0x14) = wi_fi_find_unused_socket_result;
                (&wifi_socket_handles)[wi_fi_find_unused_socket_result] = sd;
                (&wifi_socket_types)[wi_fi_find_unused_socket_result] = 1;
                (&wifi_socket_ports)[wi_fi_find_unused_socket_result] = port;
                return 1;
            }
            if (sl_connect_result == -0x72) {
                buffer[1] = 10;
                buffer[2] = 500;
                unresolved_simplelink_protocol_47(buffer);
                unresolved_simplelink_protocol_45(value_4, buffer);
                sl_connect_result = sl_Select(sd + 1, (void *)0x0, buffer, (void *)0x0, buffer + 1);
                if ((0 < sl_connect_result) &&
                    ((condition_1 = unresolved_simplelink_protocol_46(value_4, buffer),
                      !condition_1 || (sl_connect_result = sl_Connect(sd, &short_value_30, 0x10),
                                       -1 < sl_connect_result))))
                    goto branch_1;
            }
            sl_Close(sd);
        }
    }
    return 0;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015588. */
int WiFiClient_ssl_connect_ipv4(void *client, void *ipAddress, uint16_t port)
{
    short sd;
    uint16_t sl_connect_result;
    uint32_t wi_fi_find_unused_socket_result;
    int value_3;
    uint32_t value_34;
    uint16_t short_value_30;
    uint16_t sl_htons_result;
    uint32_t value;

    if (((*(int *)((int)client + 0x14) == 0xff) &&
         (wi_fi_find_unused_socket_result = WiFi_find_unused_socket(),
          wi_fi_find_unused_socket_result != 0xff)) &&
        (sd = sl_Socket(2, 1, 100), -1 < sd)) {
        if (*(char *)((int)client + 0x121) != '\0') {
            sl_SetSockOpt(sd, 1, 0x20, "/cert/rootCA.pem", 0x10);
        }
        *(uint8_t *)((int)client + 0x10) = 1;
        libc_memset(&short_value_30, 0, 0x10);
        short_value_30 = 2;
        sl_htons_result = sl_Htons(port);
        value = *(uint32_t *)((int)ipAddress + 4);
        sl_connect_result = sl_Connect(sd, &short_value_30, 0x10);
        value_3 = (int)(short)sl_connect_result;
        sl_connect_result = sl_connect_result & 0xfff7;
        if ((-1 < value_3) || (sl_connect_result == 0xfe33)) {
            if (*(char *)((int)client + 0x120) == '\0') {
                if (sl_connect_result == 0xfe33) {
                    *(int *)((int)client + 0x124) = value_3;
                    *(uint8_t *)((int)client + 0x10) = 0;
                }
            branch_1:
                value_34 = 1;
                sl_SetSockOpt(sd, 1, 0x18, &value_34, 4);
                sl_SetSockOpt(sd, 1, 9, &value_34, 4);
                *(uint32_t *)((int)client + 0x14) = wi_fi_find_unused_socket_result;
                (&wifi_socket_handles)[wi_fi_find_unused_socket_result] = sd;
                (&wifi_socket_types)[wi_fi_find_unused_socket_result] = 1;
                (&wifi_socket_ports)[wi_fi_find_unused_socket_result] = port;
                return 1;
            }
            if (sl_connect_result != 0xfe33)
                goto branch_1;
        }
        *(int *)((int)client + 0x124) = value_3;
        sl_Close(sd);
    }
    return 0;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x20015688. */
void WiFiClient_construct(void *client)
{
    *(uint32_t *)((int)client + 8) = 1000;
    *(uint8_t ***)client = &g_wifi_client_unknown_01;
    *(uint32_t *)((int)client + 4) = 0;
    *(uint32_t *)((int)client + 0x11c) = 0;
    *(uint32_t *)((int)client + 0x118) = 0;
    *(uint32_t *)((int)client + 0x14) = 0xff;
    *(uint8_t *)((int)client + 0x121) = 0;
    *(uint8_t *)((int)client + 0x120) = 0;
    *(uint32_t *)((int)client + 0x124) = 0;
    return;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x200156b4. */
void *WiFiClient_destroy(void *client)
{
    *(uint8_t ***)client = &g_wifi_client_unknown_01;
    if (*(int *)((int)client + 0x14) != 0xff) {
        libc_memcpy(&cached_wifi_clients + *(int *)((int)client + 0x14) * 0x128, client, 0x128);
    }
    return client;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x200156e0. */
uint8_t WiFiClient_connected(void *client)
{
    (**(PicoOpaqueFunction **)(*(int *)client + 8))();
    return *(int *)((int)client + 0x14) != 0xff;
}

/* Source/control-flow match to pinned libraries/WiFi/WiFiClient.cpp, including socket-index,
 * receive-buffer and SimpleLink call behavior. Firmware address: 0x200156f4. */
uint8_t WiFiClient_status(void *client)
{
    uint8_t connected;

    connected = WiFiClient_connected(client);
    return connected != '\0';
}
