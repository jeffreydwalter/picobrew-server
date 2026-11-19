/* Pico Pro 0.1.34 — simplelink spi transport. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Observed CC3200 network-processor SPI transport: base0x44022000, uDMA channels12/13, 32-bit word
 * transfers and descriptor1. Matches pinned WiFi/utility/cc_pal.c transport. Firmware address:
 * 0x20015704. */
void nwp_spi_transfer_interrupt(void)
{
    SPIIntClear(0x44022000, 0x20000);
    SPICSDisable(0x44022000);
    nwp_spi_transfer_complete = 1;
    return;
}

/* Observed CC3200 network-processor SPI transport: base0x44022000, uDMA channels12/13, 32-bit word
 * transfers and descriptor1. Matches pinned WiFi/utility/cc_pal.c transport. Firmware address:
 * 0x20015728. */
void nwp_spi_start_dma_read(void *destination, int byteCount)
{
    uint32_t wordCount;

    wordCount = byteCount / 4;
    uDMAChannelControlSet(0xd, 0xee000000);
    uDMAChannelAttributeEnable(0xd, 1);
    uDMAChannelTransferSet(0xd, 1, &g_simplelink_spi_transport_unknown_03,
                           &mmio_simplelink_spi_transport_unknown_11, wordCount);
    uDMAChannelEnable(0xd);
    uDMAChannelControlSet(0xc, 0xae000000);
    uDMAChannelAttributeEnable(0xc, 1);
    uDMAChannelTransferSet(0xc, 1, &mmio_simplelink_spi_transport_unknown_12, destination,
                           wordCount);
    uDMAChannelEnable(0xc);
    SPIWordCountSet(0x44022000, wordCount);
    return;
}

/* Observed CC3200 network-processor SPI transport: base0x44022000, uDMA channels12/13, 32-bit word
 * transfers and descriptor1. Matches pinned WiFi/utility/cc_pal.c transport. Firmware address:
 * 0x2001579c. */
uint32_t nwp_spi_open(void)
{
    int value_1;
    uint32_t spiClockHz;
    uint32_t spiBitRate;

    PRCMPeripheralClkEnable(4, 0x101);
    SPICSDisable(0x44022000);
    SPIDisable(0x44022000);
    SPIReset(0x44022000);
    if (g_simplelink_spi_transport_unknown_01 - 1 < 2) {
        spiBitRate = 13000000;
    } else {
        spiBitRate = 20000000;
        if (g_simplelink_spi_transport_unknown_01 < 3) {
            spiBitRate = 0;
        }
    }
    spiClockHz = PRCMPeripheralClockGet(4);
    SPIConfigSetExpClk(0x44022000, spiClockHz, spiBitRate, 0, 0, 0x1000f80);
    nwp_spi_dma_enabled = PRCMPeripheralStatusGet(5);
    if ((bool)nwp_spi_dma_enabled) {
        value_1 = mmio_cc3200_dma_unknown_02;
        nwp_spi_dma_enabled = value_1 != 0;
    }
    if ((bool)nwp_spi_dma_enabled != false) {
        libc_memset(&g_simplelink_spi_transport_unknown_02, 0xff, 0x50);
        uDMAChannelAssign(0xc);
        uDMAChannelAttributeDisable(0xc, 2);
        uDMAChannelAssign(0xd);
        uDMAChannelAttributeDisable(0xd, 2);
        unresolved_ti_drivers_04(0x44022000, 0x10000000);
        unresolved_ti_drivers_04(0x44022000, 0x8000000);
        unresolved_ti_drivers_03(0x44022000, 0x8000);
        unresolved_ti_drivers_03(0x44022000, 0x4000);
        SPIFIFOLevelSet(0x44022000, 1, 1);
        IntRegister(0xc1, nwp_spi_transfer_interrupt + 1);
        IntPrioritySet(0xc1, ' ');
        IntEnable(0xc1);
        SPIIntEnable(0x44022000, 0x20000);
        nwp_spi_transfer_complete = 0;
    }
    SPIEnable(0x44022000);
    nwp_spi_is_open = 1;
    return 1;
}

/* Observed CC3200 network-processor SPI transport: base0x44022000, uDMA channels12/13, 32-bit word
 * transfers and descriptor1. Matches pinned WiFi/utility/cc_pal.c transport. Firmware address:
 * 0x200158c4. */
uint32_t nwp_spi_close(void)
{
    nwp_spi_is_open = 0;
    if (nwp_spi_dma_enabled != '\0') {
        SPIIntUnregister(0x44022000);
        nwp_spi_transfer_complete = 0;
        SPIFIFODisable(0x44022000, 0x10000000);
        SPIFIFODisable(0x44022000, 0x8000000);
        SPIDmaDisable(0x44022000, 0x8000);
        SPIDmaDisable(0x44022000, 0x4000);
    }
    SPICSDisable(0x44022000);
    SPIDisable(0x44022000);
    SPIReset(0x44022000);
    PRCMPeripheralClkDisable(4, 0x101);
    return 0;
}

/* Observed CC3200 network-processor SPI transport: base0x44022000, uDMA channels12/13, 32-bit word
 * transfers and descriptor1. Matches pinned WiFi/utility/cc_pal.c transport. Firmware address:
 * 0x20015934. */
int nwp_spi_read(int descriptor, uint32_t *destination, int byteCount)
{
    int value_1;
    uint32_t value_2;
    int transferredBytes;
    bool condition_4;

    if ((descriptor != 1) || (nwp_spi_is_open_as_int != 1)) {
        return -1;
    }
    if (((byteCount < 0x65) || (nwp_spi_dma_enabled == '\0')) ||
        (transferredBytes = 0, ((uint32_t)destination & 3) != 0)) {
        SPIWordCountSet(0x44022000, 0);
        SPICSEnable(0x44022000);
        transferredBytes = byteCount + 3 >> 2;
        while (condition_4 = transferredBytes != 0, transferredBytes = transferredBytes + -1,
               condition_4) {
            do {
                value_1 = mmio_simplelink_spi_transport_unknown_10;
            } while (-1 < value_1 << 0x1e);
            mmio_simplelink_spi_transport_unknown_11 = 0xffffffff;
            do {
                value_1 = mmio_simplelink_spi_transport_unknown_10;
            } while (-1 < value_1 << 0x1f);
            value_2 = mmio_simplelink_spi_transport_unknown_12;
            *destination = value_2;
            destination = destination + 1;
        }
        SPICSDisable(0x44022000);
        return byteCount;
    }
    do {
        if (byteCount < 4000) {
            nwp_spi_start_dma_read((void *)((int)destination + transferredBytes), byteCount);
            SPICSEnable(0x44022000);
            do {
            } while (nwp_spi_transfer_complete_as_char != '\x01');
            nwp_spi_transfer_complete_as_char = 0;
            return transferredBytes + byteCount;
        }
        nwp_spi_start_dma_read((void *)((int)destination + transferredBytes), 4000);
        SPICSEnable(0x44022000);
        do {
        } while (nwp_spi_transfer_complete_as_char != '\x01');
        byteCount = byteCount + -4000;
        nwp_spi_transfer_complete_as_char = '\0';
        transferredBytes = transferredBytes + 4000;
    } while (byteCount != 0);
    return transferredBytes;
}

/* Observed CC3200 network-processor SPI transport: base0x44022000, uDMA channels12/13, 32-bit word
 * transfers and descriptor1. Matches pinned WiFi/utility/cc_pal.c transport. Firmware address:
 * 0x20015a0c. */
int nwp_spi_write(int descriptor, void *source, int byteCount)
{
    int value_1;
    uint32_t value_2;
    int value_3;
    uint32_t ul_transfer_size;

    if ((descriptor == 1) && (nwp_spi_is_open_as_int == 1)) {
        if ((byteCount < 0x65) ||
            ((nwp_spi_dma_enabled == '\0' || (((uint32_t)source & 3) != 0)))) {
            SPIWordCountSet(0x44022000, 0);
            SPICSEnable(0x44022000);
            value_3 = 0;
            while (value_3 != byteCount + 3 >> 2) {
                do {
                    value_1 = mmio_simplelink_spi_transport_unknown_10;
                } while (-1 < value_1 << 0x1e);
                mmio_simplelink_spi_transport_unknown_11 = *(uint32_t *)((int)source + value_3 * 4);
                do {
                    value_1 = mmio_simplelink_spi_transport_unknown_10;
                } while (-1 < value_1 << 0x1f);
                value_3 = value_3 + 1;
                value_2 = mmio_simplelink_spi_transport_unknown_12;
            }
            SPICSDisable(0x44022000);
        } else {
            ul_transfer_size = byteCount >> 2;
            uDMAChannelControlSet(0xd, 0xea000000);
            uDMAChannelAttributeEnable(0xd, 1);
            uDMAChannelTransferSet(0xd, 1, source, &mmio_simplelink_spi_transport_unknown_11,
                                   ul_transfer_size);
            uDMAChannelEnable(0xd);
            uDMAChannelControlSet(0xc, 0xee000000);
            uDMAChannelAttributeEnable(0xc, 1);
            uDMAChannelTransferSet(0xc, 1, &mmio_simplelink_spi_transport_unknown_12,
                                   &g_simplelink_spi_transport_unknown_04, ul_transfer_size);
            uDMAChannelEnable(0xc);
            SPIWordCountSet(0x44022000, ul_transfer_size);
            SPICSEnable(0x44022000);
            do {
            } while (nwp_spi_transfer_complete_as_char != '\x01');
            nwp_spi_transfer_complete_as_char = '\0';
        }
    } else {
        byteCount = -1;
    }
    return byteCount;
}

/* Observed CC3200 network-processor SPI transport: base0x44022000, uDMA channels12/13, 32-bit word
 * transfers and descriptor1. Matches pinned WiFi/utility/cc_pal.c transport. Firmware address:
 * 0x20015b00. */
uint32_t nwp_register_interrupt_handler(PicoInterruptHandler *handler)
{
    if (handler == (PicoInterruptHandler *)0x0) {
        IntDisable(0xbb);
        IntUnregister(0xbb);
        IntPendClear(0xbb);
    } else {
        IntRegister(0xbb, handler);
        IntPrioritySet(0xbb, ' ');
        IntPendClear(0xbb);
        IntEnable(0xbb);
    }
    return 0;
}

/* Pinned WiFi/utility/cc_pal.c matches register addresses400f7088/400f708c and network-processor
 * reset/power sequencing. Firmware address: 0x20015b38. */
void NwpMaskInterrupt(void)
{
    mmio_simplelink_spi_transport_unknown_05 = 1;
    return;
}

/* Pinned WiFi/utility/cc_pal.c matches register addresses400f7088/400f708c and network-processor
 * reset/power sequencing. Firmware address: 0x20015b44. */
void NwpUnMaskInterrupt(void)
{
    mmio_simplelink_spi_transport_unknown_06 = 1;
    return;
}

/* Pinned WiFi/utility/cc_pal.c matches register addresses400f7088/400f708c and network-processor
 * reset/power sequencing. Firmware address: 0x20015b50. */
void NwpPowerOn(void)
{
    uint32_t value_1;

    value_1 = mmio_simplelink_spi_transport_unknown_15;
    mmio_simplelink_spi_transport_unknown_15 = value_1 & 0xfffffffd;
    mmio_simplelink_spi_transport_unknown_13 = 1;
    unresolved_ti_drivers_06(8000000);
    mmio_simplelink_spi_transport_unknown_06 = 1;
    return;
}

/* Pinned WiFi/utility/cc_pal.c matches register addresses400f7088/400f708c and network-processor
 * reset/power sequencing. Firmware address: 0x20015b80. */
void NwpPowerOff(void)
{
    uint32_t value_1;

    unresolved_ti_drivers_06(8000);
    mmio_simplelink_spi_transport_unknown_05 = 1;
    value_1 = mmio_simplelink_spi_transport_unknown_16;
    mmio_simplelink_spi_transport_unknown_16 = value_1 & 0xf7ffffff;
    value_1 = mmio_simplelink_spi_transport_unknown_15;
    mmio_simplelink_spi_transport_unknown_15 = value_1 | 2;
    unresolved_ti_drivers_06(800000);
    return;
}

/* Pinned WiFi/utility/cc_pal.c matches register addresses400f7088/400f708c and network-processor
 * reset/power sequencing. Firmware address: 0x20015bbc. */
void NwpPowerOnPreamble(void)
{
    int value_1;
    int value_2;

    mmio_simplelink_spi_transport_unknown_08 = 1;
    unresolved_ti_drivers_06(160000);
    value_2 = mmio_simplelink_spi_transport_unknown_14;
    value_1 = mmio_simplelink_spi_transport_unknown_15;
    if ((value_2 != 0x20) && (-1 < value_1 << 0x1e)) {
        value_2 = 1000;
        do {
            value_1 = mmio_simplelink_spi_transport_unknown_09;
            if (-1 < value_1 << 0x1f)
                break;
            unresolved_ti_drivers_06(160000);
            value_2 = value_2 + -1;
        } while (value_2 != 0);
    }
    mmio_simplelink_spi_transport_unknown_07 = 1;
    unresolved_ti_drivers_06(160000);
    NwpPowerOff();
    return;
}
