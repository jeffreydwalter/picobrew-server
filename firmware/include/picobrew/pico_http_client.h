#ifndef PICOBREW_PICO_HTTP_CLIENT_H
#define PICOBREW_PICO_HTTP_CLIENT_H
#include <WiFiClient.h>

/* Pico's two observed TCP connection paths with native SDK object lifetime,
 * layout and inherited I/O. No extra state or guessed vtable offsets. */
class PicoHttpClient : public WiFiClient
{
  public:
    virtual int connect(IPAddress address, uint16_t port);
    using WiFiClient::connect;
    int connectPico(IPAddress address, uint16_t port, bool background);
};
#endif
