/* Pico Pro 0.1.34 — rfid reader. */
#include "pico/firmware.h"
#include <stdarg.h>

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018494. */
void MFRC522_construct(int reader, uint8_t selectPin, uint8_t sharedSelectPin, uint8_t auxiliaryPin)
{
    *(uint8_t *)(reader + 0xc) = selectPin;
    *(uint8_t *)(reader + 0xd) = sharedSelectPin;
    *(uint8_t *)(reader + 0xe) = auxiliaryPin;
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x200184a0. */
void MFRC522_configure_spi(void)
{
    SPI_set_bit_order(&global_spi, '\x01');
    SPI_set_data_mode(&global_spi, '\0');
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x200184bc. */
void MFRC522_write_register(int reader, uint8_t registerAddress, uint8_t value)
{
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 0);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    SPI_transfer_byte(&global_spi, registerAddress & 0x7e);
    SPI_transfer_byte(&global_spi, value);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 1);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018500. */
void MFRC522_write_register_buffer(int reader, uint8_t registerAddress, uint32_t count, int values)
{
    uint32_t index;

    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 0);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    SPI_transfer_byte(&global_spi, registerAddress & 0x7e);
    for (index = 0; (index & 0xff) < count; index = index + 1) {
        SPI_transfer_byte(&global_spi, *(uint8_t *)(values + index));
    }
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 1);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018550. */
uint8_t MFRC522_read_register(int reader, uint8_t registerAddress)
{
    uint8_t registerValue;

    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 0);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    SPI_transfer_byte(&global_spi, registerAddress & 0x7e | 0x80);
    registerValue = SPI_transfer_byte(&global_spi, '\0');
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 1);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    return registerValue;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018598. */
void MFRC522_read_register_buffer(int reader, uint8_t registerAddress, int count, uint8_t *values,
                                  uint8_t receiveAlignment)
{
    uint8_t spi_transfer_byte_result_2;
    uint8_t spi_transfer_byte_result;
    uint32_t index;
    uint32_t index_2;
    uint32_t value_5;
    uint32_t value_6;

    if (count == 0) {
        return;
    }
    spi_transfer_byte_result = registerAddress & 0x7e | 0x80;
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 0);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    SPI_transfer_byte(&global_spi, spi_transfer_byte_result);
    value_6 = count - 1U & 0xff;
    for (index_2 = 0; (index_2 & 0xff) < value_6; index_2 = index_2 + 1) {
        if (((index_2 & 0xff) == 0) && (receiveAlignment != 0)) {
            value_5 = 0;
            for (index = (uint32_t)receiveAlignment; index = index & 0xff, index < 8;
                 index = index + 1) {
                value_5 = value_5 | 1 << index & 0xffU;
            }
            spi_transfer_byte_result_2 = SPI_transfer_byte(&global_spi, spi_transfer_byte_result);
            *values = *values & ~(uint8_t)value_5 | spi_transfer_byte_result_2 & (uint8_t)value_5;
        } else {
            spi_transfer_byte_result_2 = SPI_transfer_byte(&global_spi, spi_transfer_byte_result);
            values[index_2] = spi_transfer_byte_result_2;
        }
    }
    spi_transfer_byte_result = SPI_transfer_byte(&global_spi, '\0');
    values[value_6] = spi_transfer_byte_result;
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 1);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x200185ac. */
void MFRC522_set_register_bits(int reader, uint8_t registerAddress, uint8_t mask)
{
    uint8_t mfrc522_read_register_result;

    mfrc522_read_register_result = MFRC522_read_register(reader, registerAddress);
    MFRC522_write_register(reader, registerAddress, mfrc522_read_register_result | mask);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x200185ca. */
void MFRC522_clear_register_bits(int reader, uint8_t registerAddress, uint8_t mask)
{
    uint8_t mfrc522_read_register_result;

    mfrc522_read_register_result = MFRC522_read_register(reader, registerAddress);
    MFRC522_write_register(reader, registerAddress, mfrc522_read_register_result & ~mask);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x200185e8. */
uint32_t MFRC522_calculate_crc(int reader, int data, uint32_t count, uint8_t *result)
{
    uint8_t mfrc522_read_register_result;
    uint8_t mfrc522_read_register_result_2;
    uint8_t *pointer_3;

    MFRC522_write_register(reader, 2, '\0');
    MFRC522_write_register(reader, 10, '\x04');
    MFRC522_set_register_bits(reader, 0x14, 0x80);
    MFRC522_write_register_buffer(reader, 0x12, count, data);
    MFRC522_write_register(reader, 2, '\x03');
    pointer_3 = &g_brewing_unknown_02;
    do {
        mfrc522_read_register_result = MFRC522_read_register(reader, 10);
        if ((int)((uint32_t)mfrc522_read_register_result << 0x1d) < 0) {
            MFRC522_write_register(reader, 2, '\0');
            mfrc522_read_register_result_2 = MFRC522_read_register(reader, 0x44);
            *result = mfrc522_read_register_result_2;
            mfrc522_read_register_result_2 = MFRC522_read_register(reader, 0x42);
            result[1] = mfrc522_read_register_result_2;
            return 1;
        }
        pointer_3 = pointer_3 + -1;
    } while (pointer_3 != (uint8_t *)0x0);
    return 4;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x2001865e. */
void MFRC522_reset(int reader)
{
    MFRC522_write_register(reader, 2, '\x0f');
    delay(100);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018698. */
void MFRC522_initialize(int reader)
{
    uint8_t antennaControl;

    pinMode((uint32_t) * (uint8_t *)(reader + 0xc), 1);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xc), 1);
    digitalWrite((uint32_t) * (uint8_t *)(reader + 0xd), 1);
    MFRC522_configure_spi();
    MFRC522_reset(reader);
    MFRC522_write_register(reader, 0x54, 0x80);
    MFRC522_write_register(reader, 0x56, 0xa9);
    MFRC522_write_register(reader, 0x58, '\x03');
    MFRC522_write_register(reader, 0x5a, 0xe8);
    MFRC522_write_register(reader, 0x2a, '@');
    MFRC522_write_register(reader, 0x22, '=');
    MFRC522_write_register(reader, 0x4c, '`');
    MFRC522_write_register(reader, 0x4e, '(');
    MFRC522_write_register(reader, 0x50, '\x04');
    antennaControl = MFRC522_read_register(reader, 0x28);
    if ((antennaControl & 3) == 3) {
        return;
    }
    MFRC522_write_register(reader, 0x28, antennaControl | 3);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018724. */
void MFRC522_antenna_off(int reader)
{
    MFRC522_clear_register_bits(reader, 0x28, 3);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x2001872c. */
int MFRC522_communicate_with_card(int reader, int command, uint32_t waitInterruptMask, int sendData,
                                  uint8_t sendCount, uint8_t *receiveData, uint8_t *receiveCount,
                                  char *validBits, uint8_t receiveAlignment, char checkCrc)
{
    uint8_t mfrc522_read_register_result;
    uint8_t mfrc522_read_register_result_2;
    uint32_t unresolved_saved_r8;
    int mfrc522_calculate_crc_result;
    char byte_value_4;
    uint8_t byte_value;
    uint8_t byte_value_2;

    if (validBits == (char *)0x0) {
        byte_value_4 = '\0';
    } else {
        byte_value_4 = *validBits;
    }
    MFRC522_write_register(reader, 2, '\0');
    MFRC522_write_register(reader, 8, '\x7f');
    MFRC522_set_register_bits(reader, 0x14, 0x80);
    MFRC522_write_register_buffer(reader, 0x12, (uint32_t)sendCount, sendData);
    MFRC522_write_register(reader, 0x1a, byte_value_4 + receiveAlignment * '\x10');
    MFRC522_write_register(reader, 2, (uint8_t)command);
    if (command == 0xc) {
        MFRC522_set_register_bits(reader, 0x1a, 0x80);
    }
    mfrc522_calculate_crc_result = 2000;
    while (true) {
        mfrc522_read_register_result = MFRC522_read_register(reader, 8);
        if ((mfrc522_read_register_result & waitInterruptMask) != 0)
            break;
        if (((int)((uint32_t)mfrc522_read_register_result << 0x1f) < 0) ||
            (mfrc522_calculate_crc_result = mfrc522_calculate_crc_result + -1,
             mfrc522_calculate_crc_result == 0)) {
            return 4;
        }
    }
    mfrc522_read_register_result = MFRC522_read_register(reader, 0xc);
    if ((mfrc522_read_register_result & 0x13) != 0) {
        return 2;
    }
    if ((receiveData != (uint8_t *)0x0) && (receiveCount != (uint8_t *)0x0)) {
        mfrc522_read_register_result_2 = MFRC522_read_register(reader, 0x14);
        if ((uint32_t)*receiveCount < (uint32_t)mfrc522_read_register_result_2) {
            return 5;
        }
        *receiveCount = mfrc522_read_register_result_2;
        MFRC522_read_register_buffer(reader, 0x12, (uint32_t)mfrc522_read_register_result_2,
                                     receiveData, receiveAlignment);
        mfrc522_read_register_result_2 = MFRC522_read_register(reader, 0x18);
        unresolved_saved_r8 = mfrc522_read_register_result_2 & 7;
        if (validBits != (char *)0x0) {
            *validBits = (char)unresolved_saved_r8;
        }
    }
    if ((mfrc522_read_register_result & 8) != 0) {
        return 3;
    }
    if (((receiveData == (uint8_t *)0x0) || (receiveCount == (uint8_t *)0x0)) ||
        (checkCrc == '\0')) {
        return 1;
    }
    mfrc522_read_register_result = *receiveCount;
    if (mfrc522_read_register_result == 1) {
        if (unresolved_saved_r8 == 4) {
            return 9;
        }
        return 8;
    }
    if ((1 < mfrc522_read_register_result) && (unresolved_saved_r8 == 0)) {
        mfrc522_calculate_crc_result = MFRC522_calculate_crc(
            reader, (int)receiveData, (uint32_t)(uint8_t)(mfrc522_read_register_result - 2),
            &byte_value);
        if (mfrc522_calculate_crc_result != 1) {
            return mfrc522_calculate_crc_result;
        }
        if ((receiveData[*receiveCount - 2] == byte_value) &&
            (receiveData[*receiveCount - 1] == byte_value_2)) {
            return 1;
        }
    }
    return 8;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018888. */
int MFRC522_transceive_data(int reader, int sendData, uint8_t sendCount, uint8_t *receiveData,
                            uint8_t *receiveCount, char *validBits, uint8_t receiveAlignment,
                            char checkCrc)
{
    int mfrc522_communicate_with_card_result;

    mfrc522_communicate_with_card_result =
        MFRC522_communicate_with_card(reader, 0xc, 0x30, sendData, sendCount, receiveData,
                                      receiveCount, validBits, receiveAlignment, checkCrc);
    return mfrc522_communicate_with_card_result;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018916. */
int MFRC522_request_card(int reader, uint8_t *answerToRequest, uint8_t *answerLength)
{
    int status;
    uint8_t requestCommand[8];
    char validBits[5];

    requestCommand[0] = 0x26;
    if ((answerToRequest == (uint8_t *)0x0) || (*answerLength < 2)) {
        status = 5;
    } else {
        MFRC522_clear_register_bits(reader, 0x1c, 0x80);
        validBits[0] = '\a';
        status = MFRC522_transceive_data(reader, (int)requestCommand, '\x01', answerToRequest,
                                         answerLength, validBits, '\0', '\0');
        if (status == 1) {
            if (*answerLength == 2) {
                if (validBits[0] == '\0') {
                    status = 1;
                } else {
                    status = 2;
                }
            } else {
                status = 2;
            }
        }
    }
    return status;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018928. */
int MFRC522_select_card(int reader, uint8_t *uid, uint32_t knownUidBits)
{
    int value_1;
    uint8_t receive_alignment;
    uint8_t mfrc522_read_register_result;
    uint32_t value_3;
    uint8_t send_count;
    uint32_t value_4;
    int mfrc522_calculate_crc_result;
    char byte_value_6;
    uint8_t *receive_data;
    uint32_t value_7;
    uint32_t value_8;
    uint32_t value_9;
    int mfrc522_calculate_crc_result_2;
    uint8_t byte_value_36;
    uint8_t byte_value_35;
    uint8_t buffer_34[2];
    uint8_t byte_value_32;
    uint8_t byte_value_31;
    uint8_t byte_value_30;
    uint8_t byte_value;
    uint8_t byte_value_2;
    uint8_t buffer[9];

    if (0x50 < knownUidBits) {
        return 7;
    }
    MFRC522_clear_register_bits(reader, 0x1c, 0x80);
    byte_value_6 = '\x01';
    do {
        if (byte_value_6 == '\x02') {
            buffer_34[0] = 0x95;
            if (knownUidBits == 0) {
                mfrc522_calculate_crc_result = 3;
            branch_1:
                value_9 = knownUidBits;
            } else {
                value_9 = (uint32_t)(7 < *uid);
                mfrc522_calculate_crc_result = 3;
            }
        } else if (byte_value_6 == '\x03') {
            buffer_34[0] = 0x97;
            mfrc522_calculate_crc_result = 6;
            value_9 = 0;
        } else {
            if (byte_value_6 != '\x01') {
                return 6;
            }
            buffer_34[0] = 0x93;
            if (knownUidBits == 0) {
                mfrc522_calculate_crc_result = 0;
                goto branch_1;
            }
            mfrc522_calculate_crc_result = 0;
            value_9 = (uint32_t)(4 < *uid);
        }
        mfrc522_read_register_result =
            (char)knownUidBits - (char)(mfrc522_calculate_crc_result << 3);
        mfrc522_read_register_result =
            mfrc522_read_register_result & ~((char)mfrc522_read_register_result >> 0x1f);
        value_4 = (uint32_t)mfrc522_read_register_result;
        if (value_9 == 0) {
            mfrc522_calculate_crc_result_2 = 2;
        } else {
            byte_value_32 = 0x88;
            mfrc522_calculate_crc_result_2 = 3;
        }
        value_3 = mfrc522_read_register_result & 7;
        if ((mfrc522_read_register_result & 7) != 0) {
            value_3 = 1;
        }
        value_3 = value_3 + ((int)(value_4 << 0x18) >> 0x1b) & 0xff;
        if (value_3 != 0) {
            if (value_9 == 0) {
                value_7 = 4;
            } else {
                value_7 = 3;
            }
            if (value_3 <= value_7) {
                value_7 = value_3;
            }
            value_3 = 0;
            do {
                value_8 = mfrc522_calculate_crc_result_2 + value_3;
                value_1 = value_3 + mfrc522_calculate_crc_result;
                value_3 = value_3 + 1;
                buffer_34[value_8 & 0xff] = uid[value_1 + 1];
            } while ((value_3 & 0xff) < value_7);
        }
        if (value_9 != 0) {
            value_4 = mfrc522_read_register_result + 8 & 0xff;
        }
        while (true) {
            while (true) {
                value_9 = (uint32_t)(char)value_4;
                if ((int)value_9 < 0x20) {
                    value_3 = (value_9 & 0x7ff) >> 3;
                    value_7 = value_3 + 2;
                    value_8 = value_7 & 0xff;
                    byte_value_36 = (uint8_t)(value_4 & 7);
                    buffer_34[1] = byte_value_36 + (char)(value_8 << 4);
                    send_count = (uint8_t)value_7;
                    if ((value_4 & 7) != 0) {
                        send_count = send_count + '\x01';
                    }
                    byte_value_35 = '\a' - (char)value_3;
                    receive_data = buffer_34 + value_8;
                } else {
                    buffer_34[1] = 'p';
                    byte_value_2 = byte_value_32 ^ byte_value_31 ^ byte_value_30 ^ byte_value;
                    mfrc522_calculate_crc_result_2 =
                        MFRC522_calculate_crc(reader, (int)buffer_34, 7, buffer);
                    if (mfrc522_calculate_crc_result_2 != 1) {
                        return mfrc522_calculate_crc_result_2;
                    }
                    byte_value_36 = '\0';
                    byte_value_35 = '\x03';
                    receive_data = &byte_value_2;
                    send_count = '\t';
                }
                receive_alignment = byte_value_36;
                MFRC522_write_register(reader, 0x1a, byte_value_36 * '\x11');
                mfrc522_calculate_crc_result_2 = MFRC522_transceive_data(
                    reader, (int)buffer_34, send_count, receive_data, &byte_value_35,
                    (char *)&byte_value_36, receive_alignment, '\0');
                if (mfrc522_calculate_crc_result_2 != 3)
                    break;
                mfrc522_read_register_result = MFRC522_read_register(reader, 0x1c);
                if ((int)((uint32_t)mfrc522_read_register_result << 0x1a) < 0) {
                    return 3;
                }
                if (2 < (int)value_9) {
                    return 6;
                }
                byte_value_32 = byte_value_32 | 4;
                value_4 = 3;
            }
            if (mfrc522_calculate_crc_result_2 != 1) {
                return mfrc522_calculate_crc_result_2;
            }
            if (0x1f < (int)value_9)
                break;
            value_4 = 0x20;
        }
        if (byte_value_32 == 0x88) {
            mfrc522_calculate_crc_result_2 = 3;
            value_9 = 3;
        } else {
            mfrc522_calculate_crc_result_2 = 2;
            value_9 = 4;
        }
        value_4 = 0;
        do {
            uid[value_4 + mfrc522_calculate_crc_result + 1] =
                buffer_34[mfrc522_calculate_crc_result_2 + value_4 & 0xff];
            value_4 = value_4 + 1;
        } while ((value_4 & 0xff) < value_9);
        if ((byte_value_35 != '\x03') || (byte_value_36 != '\0')) {
            return 2;
        }
        mfrc522_calculate_crc_result =
            MFRC522_calculate_crc(reader, (int)receive_data, 1, &byte_value_32);
        if (mfrc522_calculate_crc_result != 1) {
            return mfrc522_calculate_crc_result;
        }
        if ((byte_value_32 != receive_data[1]) || (byte_value_31 != receive_data[2])) {
            return 8;
        }
        if (-1 < (int)((uint32_t)*receive_data << 0x1d)) {
            uid[0xb] = *receive_data;
            *uid = byte_value_6 * '\x03' + 1;
            return 1;
        }
        byte_value_6 = byte_value_6 + '\x01';
    } while (true);
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018b9a. */
void MFRC522_power_down(int reader)
{
    MFRC522_antenna_off(reader);
    delay(5);
    MFRC522_write_register(reader, 2, '\x17');
    delay(5);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018bbc. */
void MFRC522_power_up(int reader)
{
    MFRC522_write_register(reader, 2, '\a');
    delay(5);
    MFRC522_initialize(reader);
    return;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018bd8. */
int MFRC522_halt_card(int reader)
{
    int mfrc522_calculate_crc_result;
    int mfrc522_transceive_data_result;
    uint8_t byte_value_14;
    uint8_t byte_value_13;
    uint8_t buffer_12[2];

    byte_value_14 = 0x50;
    byte_value_13 = 0;
    mfrc522_calculate_crc_result = MFRC522_calculate_crc(reader, (int)&byte_value_14, 2, buffer_12);
    if ((mfrc522_calculate_crc_result == 1) &&
        (mfrc522_transceive_data_result =
             MFRC522_transceive_data(reader, (int)&byte_value_14, '\x04', (uint8_t *)0x0,
                                     (uint8_t *)0x0, (char *)0x0, '\0', '\0'),
         mfrc522_transceive_data_result != 4)) {
        if (mfrc522_transceive_data_result == 1) {
            mfrc522_transceive_data_result = 2;
        }
    } else {
        mfrc522_transceive_data_result = mfrc522_calculate_crc_result;
    }
    return mfrc522_transceive_data_result;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018c22. */
bool MFRC522_is_new_card_present(int reader, uint32_t argument_2, uint32_t argument_3)
{
    int mfrc522_request_card_result;
    uint8_t byte_value;
    uint32_t value;
    uint32_t value_8;

    value = argument_2;
    value_8 = argument_3;
    mfrc522_request_card_result = MFRC522_request_card(reader, (uint8_t *)&value, &byte_value);
    return mfrc522_request_card_result == 1 || mfrc522_request_card_result == 3;
}

/* MFRC522 register/protocol identification: SPI register addressing, CommandReg2, FIFODataReg0x12,
 * CRC engine, TxControlReg0x28, ISO14443 cascade commands0x93/0x95/0x97, REQA0x26 and HALT0x50.
 * Names reconstructed from the observed operation; status values differ from some upstream
 * libraries. Firmware address: 0x20018c44. */
bool MFRC522_read_card_serial(int reader)
{
    int mfrc522_select_card_result;

    mfrc522_select_card_result = MFRC522_select_card(reader, (uint8_t *)reader, 0);
    return mfrc522_select_card_result == 1;
}
