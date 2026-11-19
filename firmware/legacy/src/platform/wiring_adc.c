/* Pico Pro 0.1.34 — wiring adc. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Configures selected ADC channel, samples12-bit FIFO and shifts to configured resolution; matches
 * Energia wiring_analog.c. Firmware address: 0x20018f98. */
uint32_t analogRead(int pin)
{
    uint16_t short_value_1;
    uint32_t adcfifolvl_get_result;
    uint32_t value_3;
    uint32_t channel;

    short_value_1 = *(uint16_t *)(&digital_pin_hardware_numbers + pin * 2);
    switch (short_value_1) {
    case 0x38:
        channel = 0;
        break;
    case 0x39:
        channel = 8;
        break;
    case 0x3a:
        channel = 0x10;
        break;
    case 0x3b:
        channel = 0x18;
        break;
    default:
        return 0;
    }
    while (adcfifolvl_get_result = ADCFIFOLvlGet(0x4402e800, channel), adcfifolvl_get_result != 0) {
        ADCFIFORead(0x4402e800, channel);
    }
    PinTypeADC((uint32_t)short_value_1, 0xff);
    ADCChannelEnable(0x4402e800, channel);
    ADCTimerConfig(0x4402e800, 0x1ffff);
    ADCTimerEnable(0x4402e800);
    ADCEnable(0x4402e800);
    do {
        adcfifolvl_get_result = ADCFIFOLvlGet(0x4402e800, channel);
    } while (adcfifolvl_get_result == 0);
    adcfifolvl_get_result = ADCFIFORead(0x4402e800, channel);
    ADCDisable(0x4402e800);
    ADCChannelDisable(0x4402e800, channel);
    ADCTimerDisable(0x4402e800);
    value_3 = (adcfifolvl_get_result & 0x3fff) >> 2;
    if (analog_read_resolution != 0xc) {
        if (analog_read_resolution < 0xc) {
            value_3 = value_3 >> (0xc - analog_read_resolution & 0xff);
        } else {
            value_3 = value_3 << (analog_read_resolution - 0xc & 0xff);
        }
    }
    return value_3 & 0xffff;
}
