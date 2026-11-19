/* Pico Pro 0.1.34 — checksum. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Reads SHA/MD5 interrupt flags, disables serviced flags and sets ready byte.
 * Firmware address: 0x200047b0. */
void pico_hash_interrupt_handler(void)
{
    uint32_t interrupt_status;

    interrupt_status = SHAMD5IntStatus(0x44035000, true);
    if ((int)(interrupt_status << 0x1c) < 0) {
        SHAMD5IntDisable(0x44035000, 8);
        g_hash_interrupt_ready_as_uint8_t = 1;
    }
    if ((int)(interrupt_status << 0x1d) < 0) {
        SHAMD5IntDisable(0x44035000, 4);
    }
    if ((int)(interrupt_status << 0x1e) < 0) {
        SHAMD5IntDisable(0x44035000, 2);
    }
    if ((int)(interrupt_status << 0x1f) < 0) {
        SHAMD5IntDisable(0x44035000, 1);
        return;
    }
    return;
}

/* MD5 of decimal signed nibble sum, formatted as32hex and compared with envelope field.
 * Firmware address: 0x20005de0. */
int pico_validate_ota_checksum(char *expected_hex, int nibble_sum)
{
    uint32_t started_ms;
    uint32_t now_ms;
    char *source;
    uint8_t *destination;
    uint32_t input_length_or_digest_byte;
    uint8_t *pointer_4;
    int digest_index;
    char hex_buffer[20];
    uint8_t checksum_text[31];
    uint8_t digest_storage[69];

    started_ms = millis();
    itoa(nibble_sum, (char *)checksum_text, 10);
    PRCMPeripheralClkEnable(0x12, 1);
    SHAMD5IntRegister(0x44035000, (PicoInterruptHandler *)&FUN_200047b0_1);
    SHAMD5IntEnable(0x44035000, 0xf);
    while (g_hash_interrupt_ready == '\0') {
        now_ms = millis();
        if (&g_identity_unknown_01 <= (uint8_t *)(now_ms - started_ms)) {
            return 0;
        }
    }
    g_hash_interrupt_ready = 0;
    SHAMD5ConfigSet(0x44035000, 0x18);
    for (input_length_or_digest_byte = 0; checksum_text[input_length_or_digest_byte] != '\0';
         input_length_or_digest_byte = input_length_or_digest_byte + 1) {
    }
    unresolved_ti_drivers_02(0x44035000, checksum_text, input_length_or_digest_byte,
                             digest_storage + 1);
    pointer_4 = checksum_text + 1;
    digest_index = 1;
    do {
        destination = pointer_4 + -1;
        *destination = '\0';
        input_length_or_digest_byte = (uint32_t)digest_storage[digest_index];
        if (input_length_or_digest_byte < 0x10) {
            libc_strcat((char *)destination, "0");
            source = itoa(input_length_or_digest_byte, hex_buffer, 0x10);
            destination = pointer_4;
        } else {
            source = itoa(input_length_or_digest_byte, hex_buffer, 0x10);
        }
        digest_index = digest_index + 1;
        libc_strcat((char *)destination, source);
        pointer_4 = pointer_4 + 2;
    } while (digest_index != 0x11);
    SHAMD5IntDisable(0x44035000, 0xf);
    input_length_or_digest_byte = libc_strcmp((char *)checksum_text, expected_hex);
    if (input_length_or_digest_byte < 2) {
        return 1 - input_length_or_digest_byte;
    }
    return 0;
}
