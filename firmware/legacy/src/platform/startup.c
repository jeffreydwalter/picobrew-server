/* Pico Pro 0.1.34 — startup. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Existing verified vector/startup seed retained, with subsystem classification.
 * Firmware address: 0x20018cc8. */
void NmiSR(void)
{
    do {

    } while (true);
}

/* Existing verified vector/startup seed retained, with subsystem classification.
 * Firmware address: 0x20018cca. */
void FaultISR(void)
{
    do {

    } while (true);
}

/* Existing verified vector/startup seed retained, with subsystem classification.
 * Firmware address: 0x20018ccc. */
void BusFaultHandler(void)
{
    do {

    } while (true);
}

/* Existing verified vector/startup seed retained, with subsystem classification.
 * Firmware address: 0x20018cce. */
void IntDefaultHandler(void)
{
    do {

    } while (true);
}

/* Existing verified vector/startup seed retained, with subsystem classification.
 * Firmware address: 0x20018cd0. */
void ResetISR(void)
{
    uint32_t *index;
    int index_2;

    for (index_2 = 0; (uint8_t *)((int)&_data + index_2) < &_edata; index_2 = index_2 + 4) {
        *(uint32_t *)((int)&_data + index_2) = *(uint32_t *)((int)&_data + index_2);
    }
    for (index = &g_display; (int)index < 0x20037804; index = index + 1) {
        *index = 0;
    }
    energia_platform_init();
    for (index_2 = 0; index_2 != 0; index_2 = index_2 + 1) {
        (*(PicoOpaqueFunction *)(&__init_array_start)[index_2])();
    }
    for (index_2 = 0; index_2 != 10; index_2 = index_2 + 1) {
        (*(PicoOpaqueFunction *)(&__init_array_start)[index_2])();
    }
    energia_main();
    return;
}

/* Existing verified vector/startup seed retained, with subsystem classification.
 * Firmware address: 0x20018d60. */
void *_sbrk(intptr_t __delta)
{
    uint8_t *pointer_1;

    if (heap_break == (uint8_t *)0x0) {
        heap_break = &_ebss_heap_start;
    }
    if (heap_break + __delta < (uint8_t *)((int)&g_startup_unknown_01 + 1)) {
        pointer_1 = heap_break;
        heap_break = heap_break + __delta;
    } else {
        pointer_1 = (uint8_t *)0x0;
    }
    return pointer_1;
}
