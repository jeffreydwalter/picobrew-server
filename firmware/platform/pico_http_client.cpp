#include "picobrew/pico_http_client.h"
#include <WiFi.h>

int PicoHttpClient::connect(IPAddress address, uint16_t port)
{
    return connectPico(address, port, false);
}

int PicoHttpClient::connectPico(IPAddress address, uint16_t port, bool background)
{
    if (_socketIndex != NO_SOCKET_AVAIL)
        return 0;
    const uint8_t index = WiFiClass::getSocket();
    if (index == NO_SOCKET_AVAIL)
        return 0;
    const int16_t socket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_IPPROTO_TCP);
    if (socket < 0)
        return 0;
    SlSockAddrIn_t server = {};
    server.sin_family = SL_AF_INET;
    server.sin_port = sl_Htons(port);
    server.sin_addr.s_addr = address;
    int32_t enabled = 1;
    if (background)
        sl_SetSockOpt(socket, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &enabled, sizeof(enabled));
    int result = sl_Connect(socket, reinterpret_cast<SlSockAddr_t *>(&server), sizeof(server));
    bool connected = result >= 0;
    if (background && result == SL_EALREADY) {
        SlFdSet_t writable;
        SL_FD_ZERO(&writable);
        SL_FD_SET(socket, &writable);
        SlTimeval_t timeout = {10, 500};
        result = sl_Select(socket + 1, NULL, &writable, NULL, &timeout);
        /* Original 0x20015514..0x2001553a accepts a positive select result
         * when its socket bit is clear, or a successful second connect. */
        connected = result > 0 && (!SL_FD_ISSET(socket, &writable) ||
                                   sl_Connect(socket, reinterpret_cast<SlSockAddr_t *>(&server),
                                              sizeof(server)) >= 0);
    }
    if (!connected) {
        sl_Close(socket);
        return 0;
    }
    if (background) {
        sl_SetSockOpt(socket, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &enabled, sizeof(enabled));
        sl_SetSockOpt(socket, SL_SOL_SOCKET, SL_SO_KEEPALIVE, &enabled, sizeof(enabled));
    } else {
        /* Pico passes both 32-bit words (2,0), length 8, for these two options
         * at 0x20015420 and 0x20015430. Upstream passes (1), length 4. */
        const int32_t option[2] = {2, 0};
        sl_SetSockOpt(socket, SL_SOL_SOCKET, SL_SO_NONBLOCKING, option, sizeof(option));
        sl_SetSockOpt(socket, SL_SOL_SOCKET, SL_SO_KEEPALIVE, option, sizeof(option));
    }
    _socketIndex = index;
    WiFiClass::_handleArray[index] = socket;
    WiFiClass::_typeArray[index] = TYPE_TCP_CLIENT;
    WiFiClass::_portArray[index] = port;
    return 1;
}
