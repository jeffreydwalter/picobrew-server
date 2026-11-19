/* Pico Pro 0.1.34 — rfid. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Initializes reader and tests registers0x54/0x22 for expected values0x80/0x3d.
 * Firmware address: 0x20006040. */
bool pico_rfid_probe(void)
{
    uint8_t register_value;

    MFRC522_initialize(0x2002d570);
    register_value = MFRC522_read_register(0x2002d570, 0x54);
    if (register_value == 0x80) {
        register_value = MFRC522_read_register(0x2002d570, 0x22);
        return register_value == '=';
    }
    return false;
}

/* Waits for tag/read success or encoder cancellation; returns UID as zero-padded hexadecimal.
 * Firmware address: 0x20006070. */
uint8_t pico_rfid_read_uid_until_click(char *uid_hex)
{
    bool tag_detected;
    int clicked_or_uid_index;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r2;
    char byte_text[20];

    pico_encoder_clear_click((PicoEncoder *)&g_encoder);
    while (true) {
        clicked_or_uid_index = pico_encoder_take_click((PicoEncoder *)&g_encoder);
        tag_detected =
            MFRC522_is_new_card_present(0x2002d570, unresolved_return_r1, unresolved_return_r2);
        if ((tag_detected) && (tag_detected = MFRC522_read_card_serial(0x2002d570), tag_detected))
            break;
        if (clicked_or_uid_index != 0) {
            return 0;
        }
    }
    if (clicked_or_uid_index != 0) {
        return 0;
    }
    MFRC522_halt_card(0x2002d570);
    for (clicked_or_uid_index = 0; clicked_or_uid_index < (int)(uint32_t)g_rfid_reader;
         clicked_or_uid_index = clicked_or_uid_index + 1) {
        if (*(uint8_t *)(clicked_or_uid_index + 0x2002d571) < 0x10) {
            libc_strcat(uid_hex, "0");
        }
        itoa((uint32_t) * (uint8_t *)(clicked_or_uid_index + 0x2002d571), byte_text, 0x10);
        libc_strcat(uid_hex, byte_text);
    }
    uid_hex[(uint32_t)g_rfid_reader * 2] = '\0';
    return 1;
}

/* Activates reader, waits for UID/cancel, then deactivates reader.
 * Firmware address: 0x20006104. */
uint8_t pico_rfid_read_uid(char *uid_hex)
{
    uint8_t read_succeeded;

    MFRC522_power_up(0x2002d570);
    read_succeeded = pico_rfid_read_uid_until_click(uid_hex);
    MFRC522_power_down(0x2002d570);
    return read_succeeded;
}

/* Explicit _initRFID log and initialization error path.
 * Firmware address: 0x2000c900. */
void pico_rfid_initialize(void)
{
    bool rfid_probe_result;

    pico_log_serial("I", "RFID", "_initRFID()");
    rfid_probe_result = pico_rfid_probe();
    MFRC522_power_down(0x2002d570);
    if (rfid_probe_result) {
        return;
    }
    pico_log_serial("E", "RFID", "_initRFID() failed");
    pico_report_error(1, 1, 0);
    return;
}

/* Waits for card and serial read, converts each UID byte to hexadecimal output.
 * Firmware address: 0x2000c94c. */
uint8_t pico_rfid_wait_for_uid(char *uid_text, int capacity, uint32_t argument_3,
                               uint32_t argument_4)
{
    bool waiting_for_filter;
    bool reader_succeeded;
    uint32_t scan_started_ms;
    int button_pressed;
    int uid_byte_index;
    uint32_t now_ms;
    uint32_t unresolved_return_r1;
    uint32_t unresolved_return_r1_00;
    uint32_t unresolved_return_r1_01;
    uint32_t value_7;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r2_00;
    uint32_t unresolved_return_r2_01;
    uint32_t value_8;
    uint32_t uid_length;
    char *text_30;
    int value;
    uint32_t value_28;
    uint32_t value_24;

    *uid_text = '\0';
    text_30 = uid_text;
    value = capacity;
    value_28 = argument_3;
    value_24 = argument_4;
    pico_ui_draw_insert_picopak();
    pico_display_flush();
    scan_started_ms = millis();
    waiting_for_filter = true;
    while (true) {
        button_pressed = pico_encoder_take_click((PicoEncoder *)&g_encoder);
        if (waiting_for_filter) {
            uid_byte_index = pico_step_filter_is_inserted();
            value_7 = unresolved_return_r1;
            value_8 = unresolved_return_r2;
            if (uid_byte_index != 0) {
                scan_started_ms = millis();
                waiting_for_filter = false;
                value_7 = unresolved_return_r1_00;
                value_8 = unresolved_return_r2_00;
            }
        } else {
            now_ms = millis();
            value_7 = unresolved_return_r1_01;
            value_8 = unresolved_return_r2_01;
            if (&g_brewing_unknown_05 < (uint8_t *)(now_ms - scan_started_ms)) {
                pico_report_error(0xc, 1, 1);
                return 0;
            }
        }
        reader_succeeded = MFRC522_is_new_card_present(0x2002d570, value_7, value_8);
        if ((reader_succeeded) &&
            (reader_succeeded = MFRC522_read_card_serial(0x2002d570), reader_succeeded))
            break;
        if (button_pressed != 0) {
            return 0;
        }
    }
    uid_byte_index = 0;
    if (button_pressed != 0) {
        return 0;
    }
    for (; (uid_length = (uint32_t)g_rfid_reader,
           uid_byte_index < (int)uid_length && (uid_byte_index * 2 < capacity));
         uid_byte_index = uid_byte_index + 1) {
        if (*(uint8_t *)(uid_byte_index + 0x2002d571) < 0x10) {
            libc_strcat(uid_text, "0");
        }
        itoa((uint32_t) * (uint8_t *)(uid_byte_index + 0x2002d571), (char *)&text_30, 0x10);
        libc_strcat(uid_text, (char *)&text_30);
    }
    if ((int)(uid_length * 2) < capacity) {
        uid_text[uid_length * 2] = '\0';
        return 1;
    }
    uid_text[capacity + -1] = '\0';
    return 1;
}

/* Enables RFID operation, calls UID reader, restores reader mode.
 * Firmware address: 0x2000ca34. */
uint8_t pico_rfid_read_picopak(char *uid_text, int capacity)
{
    uint8_t read_succeeded;
    uint32_t unresolved_return_r2;
    uint32_t unresolved_return_r3;

    MFRC522_power_up(0x2002d570);
    read_succeeded =
        pico_rfid_wait_for_uid(uid_text, capacity, unresolved_return_r2, unresolved_return_r3);
    MFRC522_power_down(0x2002d570);
    return read_succeeded;
}
