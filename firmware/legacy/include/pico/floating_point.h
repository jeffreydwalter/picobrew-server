#ifndef PICO_FLOATING_POINT_H
#define PICO_FLOATING_POINT_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

uint64_t unresolved_floating_point_01(uint32_t argument_1, uint32_t argument_2);
uint64_t unresolved_floating_point_02(int argument_1, int *argument_2);
int *unresolved_floating_point_03(int *argument_1, uint32_t argument_2, uint32_t argument_3,
                                  uint32_t argument_4, int *argument_5, int *argument_6);
double unresolved_floating_point_04(int argument_1, int argument_2);
void unresolved_floating_point_05(uint32_t *argument_1, int argument_2, int argument_3);
uint32_t unresolved_floating_point_06(int argument_1, uint32_t argument_2);
uint32_t unresolved_floating_point_07(int *argument_1, int *argument_2, int *argument_3);
int *unresolved_floating_point_08(int *argument_1, int argument_2, int argument_3);

#endif /* PICO_FLOATING_POINT_H */
