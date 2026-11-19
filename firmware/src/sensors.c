#include "picobrew/sensors.h"

#include <float.h>
#include <math.h>

_Static_assert(sizeof(float) == 4 && FLT_MANT_DIG == 24, "Pico calibration requires IEEE binary32");
_Static_assert(sizeof(double) == 8 && DBL_MANT_DIG == 53,
               "Pico calibration requires IEEE binary64");

/* Reconstructed from the original routine at 0x20006680. The deliberate
 * integer truncation and intermediate float roundings are firmware behavior.
 * Compile without fast-math or floating-point contraction.
 */
float pico_temperature_pin26_from_adc(uint32_t adc_count)
{
    double adjusted_count = (double)(int32_t)(adc_count + 1u);
    double resistance_ratio = (adjusted_count * 20000.0) / (5049.863 - adjusted_count);
    int32_t resistance_ohms = (int32_t)resistance_ratio;
    float rounded_log_resistance = (float)log((double)resistance_ohms);
    double log_resistance = (double)rounded_log_resistance;

    double linear_term = log_resistance * 0.0002276660423 + 0.0006232719375;
    double cubic_term = ((log_resistance * 7.184906863e-08) * log_resistance) * log_resistance;
    float kelvin = (float)(1.0 / (linear_term + cubic_term));
    float celsius = (float)((double)kelvin - 273.15);
    return (float)((double)celsius * 1.8 + 32.0);
}

/* Reconstructed from the original routine at 0x200067a0. Session types 2
 * and 4 apply the original two-degree Fahrenheit correction.
 */
float pico_temperature_pin25_from_adc(uint32_t adc_count, int session_type)
{
    double adjusted_count = (double)(int32_t)(adc_count + 1u);
    double resistance_ratio = (adjusted_count * 10000.0) / (5049.863 - adjusted_count);
    int32_t resistance_ohms = (int32_t)resistance_ratio;
    float rounded_log_resistance = (float)log((double)resistance_ohms);
    double log_resistance = (double)rounded_log_resistance;

    double linear_term = log_resistance * 0.0002351430103 + 0.001127286052;
    double cubic_term = ((log_resistance * 7.753973551e-08) * log_resistance) * log_resistance;
    float kelvin = (float)(1.0 / (linear_term + cubic_term));
    float celsius = (float)((double)kelvin - 273.15);
    float fahrenheit = (float)((double)celsius * 1.8 + 32.0);
    if (session_type == 2 || session_type == 4)
        fahrenheit -= 2.0f;
    return fahrenheit;
}
