/* Pico Pro 0.1.34 — temperature. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Stores current double-word temperature into next of six history slots.
 * Firmware address: 0x20005004. */
void pico_heat_record_temperature_sample(int controller)
{
    int sample_index;
    uint32_t sample_high_word;

    if (*(int *)(controller + 0x1ac) < 6) {
        sample_index = *(int *)(controller + 0x1ac);
        *(int *)(controller + 0x1ac) = sample_index + 1;
        sample_high_word = *(uint32_t *)(controller + 500);
        sample_index = controller + sample_index * 8;
        *(uint32_t *)(sample_index + 0x78) = *(uint32_t *)(controller + 0x1f0);
        *(uint32_t *)(sample_index + 0x7c) = sample_high_word;
    }
    return;
}

/* If no fault, resets timers/history, enables two PID controllers and controller-enabled flag.
 * Firmware address: 0x20005028. */
void pico_heat_controller_start(int controller)
{
    uint32_t now_ms;

    if (*(char *)(controller + 0x61) == '\0') {
        *(uint32_t *)(controller + 0x1a4) = 0;
        *(uint32_t *)(controller + 0x1ac) = 0;
        now_ms = millis();
        *(uint32_t *)(controller + 0x200) = now_ms;
        *(uint32_t *)(controller + 0x208) = *(uint32_t *)(controller + 0x200);
        PID_set_mode((void *)(controller + 0xb0), 1);
        PID_set_mode((void *)(controller + 0x128), 1);
        *(uint32_t *)(controller + 0x1b8) = 0;
        *(uint32_t *)(controller + 0x1bc) = 0;
        g_heat_controller_enabled = 1;
    }
    return;
}

/* Uses six-sample trend or target-temperature bands depending on heating phase.
 * Firmware address: 0x20005120. */
int pico_heat_classify_temperature_progress(uint32_t argument_1, uint32_t argument_2,
                                            uint32_t temperature_low, uint32_t temperature_high)
{
    uint32_t value_1;
    uint32_t value_2;
    int __aeabi_d2iz_result;
    int sample_index_or_result;
    int __aeabi_dcmplt_result;
    int temperature_sum;
    int weighted_temperature_sum;
    double trend_or_threshold;

    value_2 = g_wort_target_fahrenheit_high;
    value_1 = g_wort_target_fahrenheit_low;
    if (g_controller_brew_phase == '\0') {
        if (g_temperature_unknown_12_as_int == 6) {
            sample_index_or_result = 0;
            __aeabi_dcmplt_result = 0;
            temperature_sum = 0;
            weighted_temperature_sum = 0;
            do {
                sample_index_or_result = sample_index_or_result + 1;
                __aeabi_d2iz_result = __aeabi_d2iz(
                    *(double *)(&g_temperature_unknown_11 + sample_index_or_result * 8));
                weighted_temperature_sum =
                    __aeabi_d2iz_result * sample_index_or_result + weighted_temperature_sum;
                temperature_sum = temperature_sum + __aeabi_d2iz_result;
                __aeabi_dcmplt_result =
                    sample_index_or_result * sample_index_or_result + __aeabi_dcmplt_result;
            } while (sample_index_or_result != 6);
            g_temperature_unknown_12_as_int = 0;
            trend_or_threshold =
                __aeabi_i2d((weighted_temperature_sum * 6 + temperature_sum * -0x15) /
                            ((__aeabi_dcmplt_result * 6 + -0x1b9) / 10));
            sample_index_or_result = __aeabi_dcmplt(trend_or_threshold, 1.0);
            if (sample_index_or_result == 0) {
                g_temperature_unknown_03 = '\0';
            } else {
                if (g_temperature_unknown_03 != '\0') {
                    g_temperature_unknown_03 = 0;
                    return 1;
                }
                g_temperature_unknown_03 = '\x01';
            }
        }
    } else if (g_controller_brew_phase == '\x01') {
        trend_or_threshold = __aeabi_dadd(
            (double)pico_join_u32_u32(g_wort_target_fahrenheit_high, g_wort_target_fahrenheit_low),
            30.0);
        sample_index_or_result = __aeabi_dcmpgt(
            (double)pico_join_u32_u32(temperature_high, temperature_low), trend_or_threshold);
        if (sample_index_or_result == 0) {
            sample_index_or_result = 0;
        } else {
            sample_index_or_result = 2;
        }
        trend_or_threshold = __aeabi_dsub((double)pico_join_u32_u32(value_2, value_1), 15.0);
        __aeabi_dcmplt_result = __aeabi_dcmplt(
            (double)pico_join_u32_u32(temperature_high, temperature_low), trend_or_threshold);
        if (__aeabi_dcmplt_result == 0) {
            return sample_index_or_result;
        }
        __aeabi_dcmplt_result = __aeabi_dcmpgt(
            (double)pico_join_u32_u32(g_temperature_unknown_14, g_temperature_unknown_13),
            (double)pico_join_u32_u32(temperature_high, temperature_low));
        if (__aeabi_dcmplt_result == 0) {
            return sample_index_or_result;
        }
        __aeabi_dcmplt_result = __aeabi_dcmpgt(
            (double)pico_join_u32_u32(g_temperature_unknown_14, g_temperature_unknown_13),
            (double)pico_join_u32_u32(temperature_high, temperature_low));
        if (__aeabi_dcmplt_result == 0) {
            g_temperature_unknown_13 = temperature_low;
            g_temperature_unknown_14 = temperature_high;
            return sample_index_or_result;
        }
        return sample_index_or_result + 1;
    }
    return 0;
}

/* Adjusts shuttle scale coefficient in timed windows and persists SS.txt when stable; network field
 * is shutScale. Firmware address: 0x20005a20. */
char pico_shuttle_update_calibration(int controller)
{
    char calibration_complete;
    int __aeabi_dcmpge_result;
    uint32_t now_ms;
    float coefficient_sample_bits;
    double *active_coefficient;
    double *candidate_coefficient;
    double updated_coefficient_bits;

    calibration_complete = *(char *)(controller + 0x1a0);
    candidate_coefficient = (double *)(controller + 0x220);
    active_coefficient = (double *)(controller + 0x218);
    if (calibration_complete != '\0') {
        updated_coefficient_bits = __aeabi_dadd(*candidate_coefficient, 0.15);
        *active_coefficient = updated_coefficient_bits;
        return calibration_complete;
    }
    if (g_temperature_unknown_10 == '\0') {
        __aeabi_dcmpge_result = __aeabi_dcmpge(*(double *)(controller + 0x1f8), 300.0);
        g_temperature_unknown_10 = __aeabi_dcmpge_result != 0;
        *(uint32_t *)(controller + 0x20c) = 0;
        g_temperature_unknown_09 = millis();
    } else if (g_temperature_unknown_08 == '\0') {
        now_ms = millis();
        g_temperature_unknown_08 =
            &g_persistence_unknown_01 <= (uint8_t *)(now_ms - g_temperature_unknown_09);
        g_temperature_unknown_10 = !(bool)g_temperature_unknown_08;
    } else {
        now_ms = millis();
        if (&g_persistence_unknown_01 <= (uint8_t *)(now_ms - g_temperature_unknown_09)) {
            g_temperature_unknown_10 = calibration_complete;
            if (&g_temperature_unknown_02 < *(uint8_t **)(controller + 0x20c)) {
                updated_coefficient_bits = __aeabi_dsub(*candidate_coefficient, 0.04);
            } else {
                if (&g_temperature_unknown_01 <= *(uint8_t **)(controller + 0x20c)) {
                    *(uint32_t *)(controller + 0x1d8) = 0;
                    *(uint32_t *)(controller + 0x1dc) = 0x4074a000;
                    *(uint8_t *)(controller + 0x1a0) = 1;
                    updated_coefficient_bits = __aeabi_dadd(*candidate_coefficient, 0.15);
                    *active_coefficient = updated_coefficient_bits;
                    pico_settings_save_shuttle_scale();
                    g_temperature_unknown_07 = 0;
                    g_temperature_unknown_08 = '\0';
                    goto branch_1;
                }
                updated_coefficient_bits = __aeabi_dadd(*candidate_coefficient, 0.04);
            }
            __aeabi_dcmpge_result = g_temperature_unknown_07;
            *candidate_coefficient = updated_coefficient_bits;
            g_temperature_unknown_07 = g_temperature_unknown_07 + 1;
            coefficient_sample_bits = __aeabi_d2f(*candidate_coefficient);
            (&g_temperature_unknown_04)[__aeabi_dcmpge_result] = coefficient_sample_bits;
            coefficient_sample_bits = g_temperature_unknown_06;
            if (g_temperature_unknown_07 == 3) {
                __aeabi_dcmpge_result =
                    __aeabi_fcmpeq(g_temperature_unknown_04, g_temperature_unknown_06);
                if (__aeabi_dcmpge_result != 0) {
                    *(uint32_t *)(controller + 0x1d8) = 0;
                    *(uint32_t *)(controller + 0x1dc) = 0x4074a000;
                    *(uint8_t *)(controller + 0x1a0) = 1;
                    updated_coefficient_bits = __aeabi_dadd(*candidate_coefficient, 0.15);
                    *active_coefficient = updated_coefficient_bits;
                    pico_settings_save_shuttle_scale();
                    g_temperature_unknown_08 = '\0';
                    g_temperature_unknown_07 = 0;
                    goto branch_1;
                }
                g_temperature_unknown_04 = g_temperature_unknown_05;
                g_temperature_unknown_05 = coefficient_sample_bits;
                g_temperature_unknown_07 = 2;
            }
        }
    }
    *(uint32_t *)active_coefficient = *(uint32_t *)candidate_coefficient;
    *(uint32_t *)(controller + 0x21c) = *(uint32_t *)(controller + 0x224);
branch_1:
    return *(char *)(controller + 0x1a0);
}

/* Critical section disables controller, heater/shuttle and both automatic PID loops.
 * Firmware address: 0x20005f50. */
bool pico_heat_controller_stop(int controller)
{
    bool int_master_enable_result;

    IntMasterDisable();
    g_heat_controller_enabled = 0;
    pico_heater_off();
    pico_shuttle_pump_off();
    PID_set_mode((void *)(controller + 0xb0), 0);
    PID_set_mode((void *)(controller + 0x128), 0);
    int_master_enable_result = IntMasterEnable();
    return int_master_enable_result;
}
