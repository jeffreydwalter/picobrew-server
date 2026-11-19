#ifndef PICOBREW_ENERGIA_NETWORK_H
#define PICOBREW_ENERGIA_NETWORK_H
#include "picobrew/network.h"
#include "picobrew/pico_http_client.h"

struct PicoEnergiaNetwork {
    PicoHttpClient *client;
};

/* The object supplied here must already have its native C++ constructor run. */
const PicoNetworkPlatform *pico_energia_network_platform();
#endif
