#ifndef PICO_ENERGIA_RUNTIME_H
#define PICO_ENERGIA_RUNTIME_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

char *ltoa(int value, char *string, int radix);
char *itoa(int value, char *string, int radix);
uint32_t millis(void);
void delay(uint32_t millis_2);
void delayMicroseconds(uint32_t us);
void energia_platform_init(void);
void energia_main(void);
void energia_empty_hook(void);

#endif /* PICO_ENERGIA_RUNTIME_H */
