#ifndef PICOBREW_CONFIG_H
#define PICOBREW_CONFIG_H
#include "picobrew/version.h"
#ifndef PICO_STANDALONE
#define PICO_STANDALONE 0
#endif
#define PICO_LOCAL_API_PORT 80
#define PICO_LOCAL_API_KEY_FILE "/local/api.key"

/* Original protocol destination. Change these before rebuilding to use a
 * compatible community server. No account credentials belong in this file. */
#ifndef PICO_SERVER_HOST
#define PICO_SERVER_HOST "picobrew.com"
#endif
#ifndef PICO_SERVER_PORT
#define PICO_SERVER_PORT 80
#endif
#endif
