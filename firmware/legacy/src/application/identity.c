/* Pico Pro 0.1.34 — identity. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Hashes6-byte WiFi MAC with MD5 accelerator and formats16digest bytes as32hex characters.
 * Firmware address: 0x20005c00. */
uint32_t pico_device_compute_uid(void)
{
    uint32_t started_ms;
    uint32_t now_ms;
    char *source;
    uint32_t unresolved_return_r0;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;
    int digest_index;
    uint32_t digest_byte;
    uint8_t mac_address[8];
    char hex_buffer[20];
    uint8_t digest[64];

    started_ms = millis();
    WiFi_get_mac_address(&global_wifi, (uint32_t *)mac_address, unresolved_return_r2,
                         unresolved_return_r3);
    PRCMPeripheralClkEnable(0x12, 1);
    SHAMD5IntRegister(0x44035000, (PicoInterruptHandler *)&FUN_200047b0_1);
    SHAMD5IntEnable(0x44035000, 0xf);
    do {
        if (g_hash_interrupt_ready != '\0') {
            digest_index = 0;
            g_hash_interrupt_ready = 0;
            SHAMD5ConfigSet(0x44035000, 0x18);
            unresolved_ti_drivers_02(0x44035000, mac_address, 6, digest);
            do {
                digest_byte = (uint32_t)digest[digest_index];
                if (digest_byte < 0x10) {
                    libc_strcat(&g_device_uid, "0");
                }
                source = itoa(digest_byte, hex_buffer, 0x10);
                digest_index = digest_index + 1;
                libc_strcat(&g_device_uid, source);
            } while (digest_index != 0x10);
            g_device_uid_terminator = 0;
            SHAMD5IntDisable(0x44035000, 0xf);
            return unresolved_return_r0;
        }
        now_ms = millis();
    } while ((uint8_t *)(now_ms - started_ms) < &g_identity_unknown_01);
    return 0;
}
