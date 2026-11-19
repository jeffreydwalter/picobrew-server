#ifndef PICOBREW_RFID_H
#define PICOBREW_RFID_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PicoRfidUid {
    uint8_t size;
    uint8_t bytes[10];
    uint8_t sak;
} PicoRfidUid;
typedef struct PicoRfidOps {
    void (*pin_mode)(void *context, uint8_t pin, uint8_t mode);
    void (*digital_write)(void *context, uint8_t pin, uint8_t value);
    uint8_t (*spi_transfer)(void *context, uint8_t value);
    void (*spi_bit_order)(void *context, uint8_t order);
    void (*spi_mode)(void *context, uint8_t mode);
    void (*delay_ms)(void *context, uint32_t milliseconds);
} PicoRfidOps;
typedef struct PicoRfid {
    PicoRfidUid uid;
    uint8_t select_pin, shared_select_pin, auxiliary_pin;
    const PicoRfidOps *ops;
    void *context;
} PicoRfid;

enum PicoRfidStatus {
    PICO_RFID_OK = 1,
    PICO_RFID_ERROR = 2,
    PICO_RFID_COLLISION = 3,
    PICO_RFID_TIMEOUT = 4,
    PICO_RFID_NO_ROOM = 5,
    PICO_RFID_INTERNAL_ERROR = 6,
    PICO_RFID_INVALID = 7,
    PICO_RFID_CRC_WRONG = 8,
    PICO_RFID_MIFARE_NACK = 9
};

void pico_rfid_construct(PicoRfid *reader, uint8_t select_pin, uint8_t shared_select_pin,
                         uint8_t auxiliary_pin);
void pico_rfid_configure_spi(PicoRfid *reader);
void pico_rfid_write_register(PicoRfid *reader, uint8_t reg, uint8_t value);
void pico_rfid_write_register_buffer(PicoRfid *reader, uint8_t reg, uint32_t count,
                                     const uint8_t *values);
uint8_t pico_rfid_read_register(PicoRfid *reader, uint8_t reg);
void pico_rfid_read_register_buffer(PicoRfid *reader, uint8_t reg, uint8_t count, uint8_t *values,
                                    uint8_t alignment);
void pico_rfid_set_register_bits(PicoRfid *reader, uint8_t reg, uint8_t mask);
void pico_rfid_clear_register_bits(PicoRfid *reader, uint8_t reg, uint8_t mask);
int pico_rfid_calculate_crc(PicoRfid *reader, const uint8_t *data, uint32_t count,
                            uint8_t result[2]);
void pico_rfid_reset(PicoRfid *reader);
void pico_rfid_initialize_reader(PicoRfid *reader);
void pico_rfid_antenna_off(PicoRfid *reader);
int pico_rfid_communicate(PicoRfid *reader, uint8_t command, uint8_t wait_irq, const uint8_t *send,
                          uint8_t send_count, uint8_t *receive, uint8_t *receive_count,
                          uint8_t *valid_bits, uint8_t alignment, bool check_crc);
int pico_rfid_transceive(PicoRfid *reader, const uint8_t *send, uint8_t send_count,
                         uint8_t *receive, uint8_t *receive_count, uint8_t *valid_bits,
                         uint8_t alignment, bool check_crc);
int pico_rfid_request(PicoRfid *reader, uint8_t *answer, uint8_t *answer_length);
int pico_rfid_select(PicoRfid *reader, PicoRfidUid *uid, uint32_t known_bits);
void pico_rfid_power_down(PicoRfid *reader);
void pico_rfid_power_up(PicoRfid *reader);
int pico_rfid_halt(PicoRfid *reader);
bool pico_rfid_is_new_card_present(PicoRfid *reader);
bool pico_rfid_read_card_serial(PicoRfid *reader);

typedef struct PicoRfidApplicationOps {
    uint32_t (*millis)(void *context);
    void (*clear_click)(void *context);
    bool (*take_click)(void *context);
    bool (*filter_inserted)(void *context);
    void (*insert_picopak_screen)(void *context);
    void (*display_flush)(void *context);
    void (*log)(void *context, const char *level, const char *module, const char *message);
    void (*report_error)(void *context, unsigned code, bool acknowledge, bool report);
} PicoRfidApplicationOps;
typedef struct PicoRfidApplication {
    PicoRfid *reader;
    const PicoRfidApplicationOps *ops;
    void *context;
} PicoRfidApplication;

bool pico_rfid_probe(PicoRfid *reader);
void pico_rfid_application_initialize(PicoRfidApplication *application);
/* read_uid callers supply an initially empty text buffer. A ten-byte UID needs
 * 21 bytes including its terminator. Capacity failures return false without
 * overflowing the buffer; these guards intentionally repair original bounds. */
bool pico_rfid_read_uid_until_click(PicoRfidApplication *application, char *uid_text,
                                    size_t capacity);
bool pico_rfid_read_uid(PicoRfidApplication *application, char *uid_text, size_t capacity);
bool pico_rfid_wait_for_picopak(PicoRfidApplication *application, char *uid_text, size_t capacity);
bool pico_rfid_read_picopak(PicoRfidApplication *application, char *uid_text, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
