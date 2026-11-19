#ifndef PICO_SIMPLELINK_SOCKET_H
#define PICO_SIMPLELINK_SOCKET_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

short sl_Socket(short domain, short type, short protocol);
short sl_Close(short socket_descriptor);
short sl_Connect(short socket_descriptor, void *address, short address_length);
short sl_Send(short socket_descriptor, void *buffer, short length, short flags);
uint32_t sl_Htonl(uint32_t value);
uint16_t sl_Htons(uint16_t value);
short sl_Recv(short socket_descriptor, void *buffer, short length, short flags);
short sl_SetSockOpt(short sd, short level, short optname, void *optval, uint16_t optlen);
short sl_Select(short nfds, void *readsds, void *writesds, void *exceptsds, void *timeout);

#endif /* PICO_SIMPLELINK_SOCKET_H */
