#ifndef PICO_RFID_READER_H
#define PICO_RFID_READER_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

void MFRC522_construct(int reader, uint8_t selectPin, uint8_t sharedSelectPin,
                       uint8_t auxiliaryPin);
void MFRC522_configure_spi(void);
void MFRC522_write_register(int reader, uint8_t registerAddress, uint8_t value);
void MFRC522_write_register_buffer(int reader, uint8_t registerAddress, uint32_t count, int values);
uint8_t MFRC522_read_register(int reader, uint8_t registerAddress);
void MFRC522_read_register_buffer(int reader, uint8_t registerAddress, int count, uint8_t *values,
                                  uint8_t receiveAlignment);
void MFRC522_set_register_bits(int reader, uint8_t registerAddress, uint8_t mask);
void MFRC522_clear_register_bits(int reader, uint8_t registerAddress, uint8_t mask);
uint32_t MFRC522_calculate_crc(int reader, int data, uint32_t count, uint8_t *result);
void MFRC522_reset(int reader);
void MFRC522_initialize(int reader);
void MFRC522_antenna_off(int reader);
int MFRC522_communicate_with_card(int reader, int command, uint32_t waitInterruptMask, int sendData,
                                  uint8_t sendCount, uint8_t *receiveData, uint8_t *receiveCount,
                                  char *validBits, uint8_t receiveAlignment, char checkCrc);
int MFRC522_transceive_data(int reader, int sendData, uint8_t sendCount, uint8_t *receiveData,
                            uint8_t *receiveCount, char *validBits, uint8_t receiveAlignment,
                            char checkCrc);
int MFRC522_request_card(int reader, uint8_t *answerToRequest, uint8_t *answerLength);
int MFRC522_select_card(int reader, uint8_t *uid, uint32_t knownUidBits);
void MFRC522_power_down(int reader);
void MFRC522_power_up(int reader);
int MFRC522_halt_card(int reader);
bool MFRC522_is_new_card_present(int reader, uint32_t argument_2, uint32_t argument_3);
bool MFRC522_read_card_serial(int reader);

#endif /* PICO_RFID_READER_H */
