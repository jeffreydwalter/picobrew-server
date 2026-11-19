#include "picobrew/rfid.h"
#include <string.h>

/* Pico's MFRC522 variant uses doubled register addresses and status numbers
 * beginning with success=1. Shared-select handling and collision behavior
 * differ from generic upstream implementations and are recovered here. */
static uint8_t transfer(PicoRfid *r, uint8_t value)
{
    return r->ops->spi_transfer(r->context, value);
}
static void select_reader(PicoRfid *r)
{
    r->ops->digital_write(r->context, r->select_pin, 0);
    r->ops->digital_write(r->context, r->shared_select_pin, 1);
}
static void release_reader(PicoRfid *r)
{
    r->ops->digital_write(r->context, r->select_pin, 1);
    r->ops->digital_write(r->context, r->shared_select_pin, 1);
}
void pico_rfid_construct(PicoRfid *r, uint8_t select_pin, uint8_t shared_select_pin,
                         uint8_t auxiliary_pin)
{
    r->select_pin = select_pin;
    r->shared_select_pin = shared_select_pin;
    r->auxiliary_pin = auxiliary_pin;
}
void pico_rfid_configure_spi(PicoRfid *r)
{
    r->ops->spi_bit_order(r->context, 1);
    r->ops->spi_mode(r->context, 0);
}
void pico_rfid_write_register(PicoRfid *r, uint8_t reg, uint8_t value)
{
    select_reader(r);
    transfer(r, reg & 0x7e);
    transfer(r, value);
    release_reader(r);
}
void pico_rfid_write_register_buffer(PicoRfid *r, uint8_t reg, uint32_t count,
                                     const uint8_t *values)
{
    select_reader(r);
    transfer(r, reg & 0x7e);
    /* The MFRC522 FIFO is 64 bytes. Valid callers remain within this range. */
    for (uint32_t index = 0; (index & 0xffu) < count; ++index)
        transfer(r, values[index]);
    release_reader(r);
}
uint8_t pico_rfid_read_register(PicoRfid *r, uint8_t reg)
{
    select_reader(r);
    transfer(r, (reg & 0x7e) | 0x80);
    uint8_t result = transfer(r, 0);
    release_reader(r);
    return result;
}
void pico_rfid_read_register_buffer(PicoRfid *r, uint8_t reg, uint8_t count, uint8_t *values,
                                    uint8_t alignment)
{
    if (count == 0)
        return;
    uint8_t address = (reg & 0x7e) | 0x80;
    select_reader(r);
    transfer(r, address);
    for (unsigned index = 0; index < (unsigned)count - 1u; ++index) {
        uint8_t value = transfer(r, address);
        if (index == 0 && alignment != 0) {
            uint8_t mask = alignment < 8 ? (uint8_t)(0xffu << alignment) : 0;
            values[0] = (values[0] & (uint8_t)~mask) | (value & mask);
        } else {
            values[index] = value;
        }
    }
    /* Original one-byte reads replace the final byte even with alignment. */
    values[count - 1] = transfer(r, 0);
    release_reader(r);
}
void pico_rfid_set_register_bits(PicoRfid *r, uint8_t reg, uint8_t mask)
{
    pico_rfid_write_register(r, reg, pico_rfid_read_register(r, reg) | mask);
}
void pico_rfid_clear_register_bits(PicoRfid *r, uint8_t reg, uint8_t mask)
{
    pico_rfid_write_register(r, reg, pico_rfid_read_register(r, reg) & (uint8_t)~mask);
}
int pico_rfid_calculate_crc(PicoRfid *r, const uint8_t *data, uint32_t count, uint8_t result[2])
{
    pico_rfid_write_register(r, 2, 0);
    pico_rfid_write_register(r, 10, 4);
    pico_rfid_set_register_bits(r, 0x14, 0x80);
    pico_rfid_write_register_buffer(r, 0x12, count, data);
    pico_rfid_write_register(r, 2, 3);
    for (unsigned remaining = 5000; remaining; --remaining) {
        if (pico_rfid_read_register(r, 10) & 4) {
            pico_rfid_write_register(r, 2, 0);
            result[0] = pico_rfid_read_register(r, 0x44);
            result[1] = pico_rfid_read_register(r, 0x42);
            return PICO_RFID_OK;
        }
    }
    return PICO_RFID_TIMEOUT;
}
void pico_rfid_reset(PicoRfid *r)
{
    pico_rfid_write_register(r, 2, 15);
    r->ops->delay_ms(r->context, 100);
}
void pico_rfid_initialize_reader(PicoRfid *r)
{
    r->ops->pin_mode(r->context, r->select_pin, 1);
    release_reader(r);
    pico_rfid_configure_spi(r);
    pico_rfid_reset(r);
    static const uint8_t configuration[][2] = {{0x54, 0x80}, {0x56, 0xa9}, {0x58, 3},
                                               {0x5a, 0xe8}, {0x2a, 0x40}, {0x22, 0x3d},
                                               {0x4c, 0x60}, {0x4e, 0x28}, {0x50, 4}};
    for (unsigned index = 0; index < sizeof configuration / sizeof configuration[0]; ++index)
        pico_rfid_write_register(r, configuration[index][0], configuration[index][1]);
    uint8_t antenna = pico_rfid_read_register(r, 0x28);
    if ((antenna & 3) != 3)
        pico_rfid_write_register(r, 0x28, antenna | 3);
}
void pico_rfid_antenna_off(PicoRfid *r)
{
    pico_rfid_clear_register_bits(r, 0x28, 3);
}

int pico_rfid_communicate(PicoRfid *r, uint8_t command, uint8_t wait_irq, const uint8_t *send,
                          uint8_t send_count, uint8_t *receive, uint8_t *receive_count,
                          uint8_t *valid_bits, uint8_t alignment, bool check_crc)
{
    uint8_t tx_bits = valid_bits ? *valid_bits : 0;
    pico_rfid_write_register(r, 2, 0);
    pico_rfid_write_register(r, 8, 0x7f);
    pico_rfid_set_register_bits(r, 0x14, 0x80);
    pico_rfid_write_register_buffer(r, 0x12, send_count, send);
    pico_rfid_write_register(r, 0x1a, (uint8_t)(tx_bits + alignment * 16u));
    pico_rfid_write_register(r, 2, command);
    if (command == 12)
        pico_rfid_set_register_bits(r, 0x1a, 0x80);
    for (unsigned remaining = 2000;;) {
        uint8_t irq = pico_rfid_read_register(r, 8);
        if (irq & wait_irq)
            break;
        if ((irq & 1) || --remaining == 0)
            return PICO_RFID_TIMEOUT;
    }
    uint8_t error = pico_rfid_read_register(r, 0x0c);
    if (error & 0x13)
        return PICO_RFID_ERROR;
    uint8_t rx_bits = 0;
    if (receive && receive_count) {
        uint8_t count = pico_rfid_read_register(r, 0x14);
        if (*receive_count < count)
            return PICO_RFID_NO_ROOM;
        *receive_count = count;
        pico_rfid_read_register_buffer(r, 0x12, count, receive, alignment);
        rx_bits = pico_rfid_read_register(r, 0x18) & 7;
        if (valid_bits)
            *valid_bits = rx_bits;
    }
    if (error & 8)
        return PICO_RFID_COLLISION;
    if (!receive || !receive_count || !check_crc)
        return PICO_RFID_OK;
    if (*receive_count == 1)
        return rx_bits == 4 ? PICO_RFID_MIFARE_NACK : PICO_RFID_CRC_WRONG;
    if (*receive_count > 1 && rx_bits == 0) {
        uint8_t crc[2];
        int status = pico_rfid_calculate_crc(r, receive, *receive_count - 2u, crc);
        if (status != PICO_RFID_OK)
            return status;
        if (receive[*receive_count - 2] == crc[0] && receive[*receive_count - 1] == crc[1])
            return PICO_RFID_OK;
    }
    return PICO_RFID_CRC_WRONG;
}
int pico_rfid_transceive(PicoRfid *r, const uint8_t *send, uint8_t send_count, uint8_t *receive,
                         uint8_t *receive_count, uint8_t *valid_bits, uint8_t alignment,
                         bool check_crc)
{
    return pico_rfid_communicate(r, 12, 0x30, send, send_count, receive, receive_count, valid_bits,
                                 alignment, check_crc);
}
int pico_rfid_request(PicoRfid *r, uint8_t *answer, uint8_t *answer_length)
{
    if (!answer || *answer_length < 2)
        return PICO_RFID_NO_ROOM;
    pico_rfid_clear_register_bits(r, 0x1c, 0x80);
    uint8_t command = 0x26, valid_bits = 7;
    int status = pico_rfid_transceive(r, &command, 1, answer, answer_length, &valid_bits, 0, false);
    if (status == PICO_RFID_OK && (*answer_length != 2 || valid_bits != 0))
        return PICO_RFID_ERROR;
    return status;
}

int pico_rfid_select(PicoRfid *r, PicoRfidUid *uid, uint32_t known_bits)
{
    if (known_bits > 80)
        return PICO_RFID_INVALID;
    pico_rfid_clear_register_bits(r, 0x1c, 0x80);
    for (unsigned cascade = 1; cascade <= 3; ++cascade) {
        uint8_t buffer[9] = {0};
        unsigned uid_index = (cascade - 1) * 3;
        buffer[0] = cascade == 1 ? 0x93 : cascade == 2 ? 0x95 : 0x97;
        bool cascade_tag =
            known_bits && (cascade == 1 ? uid->size > 4 : cascade == 2 && uid->size > 7);
        int known = (int)known_bits - (int)uid_index * 8;
        if (known < 0)
            known = 0;
        unsigned offset = cascade_tag ? 3 : 2;
        if (cascade_tag)
            buffer[2] = 0x88;
        unsigned bytes = ((unsigned)known + 7) / 8;
        unsigned maximum = cascade_tag ? 3 : 4;
        if (bytes > maximum)
            bytes = maximum;
        for (unsigned index = 0; index < bytes; ++index)
            buffer[offset + index] = uid->bytes[uid_index + index];
        if (cascade_tag)
            known += 8;
        uint8_t valid_bits = 0, response_length = 0;
        uint8_t *response = NULL;
        for (;;) {
            uint8_t send_count;
            if (known >= 32) {
                buffer[1] = 0x70;
                buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
                int status = pico_rfid_calculate_crc(r, buffer, 7, buffer + 7);
                if (status != PICO_RFID_OK)
                    return status;
                valid_bits = 0;
                response_length = 3;
                response = buffer + 6;
                send_count = 9;
            } else {
                unsigned whole_bytes = (unsigned)known / 8;
                unsigned receive_offset = whole_bytes + 2;
                valid_bits = (uint8_t)(known & 7);
                buffer[1] = (uint8_t)(valid_bits + receive_offset * 16);
                send_count = (uint8_t)(receive_offset + (valid_bits != 0));
                response_length = (uint8_t)(7 - whole_bytes);
                response = buffer + receive_offset;
            }
            uint8_t alignment = valid_bits;
            pico_rfid_write_register(r, 0x1a, (uint8_t)(valid_bits * 17));
            int status = pico_rfid_transceive(r, buffer, send_count, response, &response_length,
                                              &valid_bits, alignment, false);
            if (status == PICO_RFID_COLLISION) {
                uint8_t collision = pico_rfid_read_register(r, 0x1c);
                if (collision & 0x20)
                    return PICO_RFID_COLLISION;
                /* Original instructions select bit 3, rather than using the
                 * reported collision-position field. Preserve this Pico fork. */
                if (known > 2)
                    return PICO_RFID_INTERNAL_ERROR;
                buffer[2] |= 4;
                known = 3;
                continue;
            }
            if (status != PICO_RFID_OK)
                return status;
            if (known >= 32)
                break;
            known = 32;
        }
        offset = buffer[2] == 0x88 ? 3 : 2;
        unsigned count = buffer[2] == 0x88 ? 3 : 4;
        for (unsigned index = 0; index < count; ++index)
            uid->bytes[uid_index + index] = buffer[offset + index];
        if (response_length != 3 || valid_bits != 0)
            return PICO_RFID_ERROR;
        uint8_t crc[2];
        int status = pico_rfid_calculate_crc(r, response, 1, crc);
        if (status != PICO_RFID_OK)
            return status;
        if (crc[0] != response[1] || crc[1] != response[2])
            return PICO_RFID_CRC_WRONG;
        if (!(response[0] & 4)) {
            uid->sak = response[0];
            uid->size = (uint8_t)(cascade * 3 + 1);
            return PICO_RFID_OK;
        }
    }
    return PICO_RFID_INTERNAL_ERROR;
}

void pico_rfid_power_down(PicoRfid *r)
{
    pico_rfid_antenna_off(r);
    r->ops->delay_ms(r->context, 5);
    pico_rfid_write_register(r, 2, 0x17);
    r->ops->delay_ms(r->context, 5);
}
void pico_rfid_power_up(PicoRfid *r)
{
    pico_rfid_write_register(r, 2, 7);
    r->ops->delay_ms(r->context, 5);
    pico_rfid_initialize_reader(r);
}
int pico_rfid_halt(PicoRfid *r)
{
    uint8_t buffer[4] = {0x50, 0, 0, 0};
    int status = pico_rfid_calculate_crc(r, buffer, 2, buffer + 2);
    if (status != PICO_RFID_OK)
        return status;
    status = pico_rfid_transceive(r, buffer, 4, NULL, NULL, NULL, 0, false);
    return status == PICO_RFID_TIMEOUT ? PICO_RFID_OK
           : status == PICO_RFID_OK    ? PICO_RFID_ERROR
                                       : status;
}
bool pico_rfid_is_new_card_present(PicoRfid *r)
{
    /* The original writes answer_length=2 at sp+3; old decompilation missed it. */
    uint8_t answer[2], answer_length = 2;
    int status = pico_rfid_request(r, answer, &answer_length);
    return status == PICO_RFID_OK || status == PICO_RFID_COLLISION;
}
bool pico_rfid_read_card_serial(PicoRfid *r)
{
    return pico_rfid_select(r, &r->uid, 0) == PICO_RFID_OK;
}

bool pico_rfid_probe(PicoRfid *r)
{
    pico_rfid_initialize_reader(r);
    return pico_rfid_read_register(r, 0x54) == 0x80 && pico_rfid_read_register(r, 0x22) == 0x3d;
}
void pico_rfid_application_initialize(PicoRfidApplication *a)
{
    a->ops->log(a->context, "I", "RFID", "_initRFID()");
    bool found = pico_rfid_probe(a->reader);
    pico_rfid_power_down(a->reader);
    if (!found) {
        a->ops->log(a->context, "E", "RFID", "_initRFID() failed");
        a->ops->report_error(a->context, 1, true, false);
    }
}
static bool append_uid(PicoRfid *r, char *text, size_t capacity)
{
    static const char hex[] = "0123456789abcdef";
    size_t length = 0;
    while (length < capacity && text[length])
        ++length;
    if (length == capacity || r->uid.size > sizeof r->uid.bytes)
        return false;
    if ((size_t)r->uid.size > (capacity - length - 1u) / 2u)
        return false;
    for (unsigned index = 0; index < r->uid.size; ++index) {
        text[length++] = hex[r->uid.bytes[index] >> 4];
        text[length++] = hex[r->uid.bytes[index] & 15];
    }
    text[length] = 0;
    return true;
}
bool pico_rfid_read_uid_until_click(PicoRfidApplication *a, char *text, size_t capacity)
{
    a->ops->clear_click(a->context);
    for (;;) {
        bool clicked = a->ops->take_click(a->context);
        bool found =
            pico_rfid_is_new_card_present(a->reader) && pico_rfid_read_card_serial(a->reader);
        if (clicked)
            return false;
        if (found) {
            pico_rfid_halt(a->reader);
            return append_uid(a->reader, text, capacity);
        }
    }
}
bool pico_rfid_read_uid(PicoRfidApplication *a, char *text, size_t capacity)
{
    pico_rfid_power_up(a->reader);
    bool found = pico_rfid_read_uid_until_click(a, text, capacity);
    pico_rfid_power_down(a->reader);
    return found;
}
bool pico_rfid_wait_for_picopak(PicoRfidApplication *a, char *text, size_t capacity)
{
    if (!capacity)
        return false;
    text[0] = 0;
    a->ops->insert_picopak_screen(a->context);
    a->ops->display_flush(a->context);
    uint32_t started = a->ops->millis(a->context);
    bool awaiting_filter = true;
    for (;;) {
        bool clicked = a->ops->take_click(a->context);
        if (awaiting_filter) {
            if (a->ops->filter_inserted(a->context)) {
                started = a->ops->millis(a->context);
                awaiting_filter = false;
            }
        } else if ((uint32_t)(a->ops->millis(a->context) - started) > 15000u) {
            a->ops->report_error(a->context, 12, true, true);
            return false;
        }
        bool found =
            pico_rfid_is_new_card_present(a->reader) && pico_rfid_read_card_serial(a->reader);
        if (clicked)
            return false;
        if (found)
            return append_uid(a->reader, text, capacity);
    }
}
bool pico_rfid_read_picopak(PicoRfidApplication *a, char *text, size_t capacity)
{
    pico_rfid_power_up(a->reader);
    bool found = pico_rfid_wait_for_picopak(a, text, capacity);
    pico_rfid_power_down(a->reader);
    return found;
}
