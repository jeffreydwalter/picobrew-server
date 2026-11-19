/* Pico Pro 0.1.34 — network addresses. */
#include "pico/firmware.h"
#include <stdarg.h>

/* Pinned Energia IPAddress.cpp/MACAddress.cpp API; four-octet IP or six-octet MAC storage, Print
 * dispatch and constructor/assignment behavior. Firmware address: 0x20019494. */
int IPAddress_printTo(int address, void *output)
{
    uint32_t print_print_unsigned_alias_a_result;
    uint32_t print_print_char_result;
    int value_3;
    int value_4;

    value_3 = 0;
    value_4 = 0;
    do {
        print_print_unsigned_alias_a_result = Print_print_unsigned_alias_a(
            output, (uint32_t) * (uint8_t *)(address + value_3 + 4), 10);
        print_print_char_result = Print_print_char(output, '.');
        value_3 = value_3 + 1;
        value_4 = value_4 + print_print_char_result + print_print_unsigned_alias_a_result;
    } while (value_3 != 3);
    print_print_unsigned_alias_a_result =
        Print_print_unsigned_alias_a(output, (uint32_t) * (uint8_t *)(address + 7), 10);
    return print_print_unsigned_alias_a_result + value_4;
}

/* Pinned Energia IPAddress.cpp/MACAddress.cpp API; four-octet IP or six-octet MAC storage, Print
 * dispatch and constructor/assignment behavior. Firmware address: 0x200194d0. */
uint32_t *IPAddress_construct_zero(uint32_t *address)
{
    *address = &g_network_addresses_unknown_01;
    libc_memset(address + 1, 0, 4);
    return address;
}

/* Pinned Energia IPAddress.cpp/MACAddress.cpp API; four-octet IP or six-octet MAC storage, Print
 * dispatch and constructor/assignment behavior. Firmware address: 0x200194ec. */
void IPAddress_construct_octets(uint32_t *address, uint8_t octet0, uint8_t octet1, uint8_t octet2,
                                uint8_t octet3)
{
    *(uint8_t *)((int)address + 6) = octet2;
    *address = &g_network_addresses_unknown_01;
    *(uint8_t *)(address + 1) = octet0;
    *(uint8_t *)((int)address + 5) = octet1;
    *(uint8_t *)((int)address + 7) = octet3;
    return;
}

/* Pinned Energia IPAddress.cpp/MACAddress.cpp API; four-octet IP or six-octet MAC storage, Print
 * dispatch and constructor/assignment behavior. Firmware address: 0x20019504. */
int IPAddress_assign_word(int address, uint32_t value)
{
    *(uint32_t *)(address + 4) = value;
    return address;
}

/* Pinned Energia IPAddress.cpp/MACAddress.cpp API; four-octet IP or six-octet MAC storage, Print
 * dispatch and constructor/assignment behavior. Firmware address: 0x20019518. */
void initialize_global_any_ip_address(void)
{
    global_any_ip_address = &g_network_addresses_unknown_01;
    PICO_PART_U8(any_ip_address_value_as_uint8_t, 0) = 0;
    PICO_PART_U8(any_ip_address_value_as_uint8_t, 1) = 0;
    PICO_PART_U8(any_ip_address_value_as_uint8_t, 2) = 0;
    PICO_PART_U8(any_ip_address_value_as_uint8_t, 3) = 0;
    return;
}

/* Pinned Energia IPAddress.cpp/MACAddress.cpp API; four-octet IP or six-octet MAC storage, Print
 * dispatch and constructor/assignment behavior. Firmware address: 0x20019534. */
int MACAddress_printTo(int address, void *output)
{
    uint32_t print_print_char_result;
    uint32_t print_print_char_result_2;
    int value_3;
    int value_4;

    value_4 = 0;
    value_3 = 0;
    do {
        if (*(uint8_t *)(address + value_4 + 4) < 0x10) {
            print_print_char_result = Print_print_char(output, '0');
            value_3 = value_3 + print_print_char_result;
        }
        print_print_char_result = Print_print_unsigned_alias_a(
            output, (uint32_t) * (uint8_t *)(address + value_4 + 4), 0x10);
        print_print_char_result_2 = Print_print_char(output, ':');
        value_4 = value_4 + 1;
        value_3 = value_3 + print_print_char_result + print_print_char_result_2;
    } while (value_4 != 5);
    if (*(uint8_t *)(address + 9) < 0x10) {
        print_print_char_result = Print_print_char(output, '0');
        value_3 = value_3 + print_print_char_result;
    }
    print_print_char_result =
        Print_print_unsigned_alias_a(output, (uint32_t) * (uint8_t *)(address + 9), 0x10);
    return print_print_char_result + value_3;
}

/* Pinned Energia IPAddress.cpp/MACAddress.cpp API; four-octet IP or six-octet MAC storage, Print
 * dispatch and constructor/assignment behavior. Firmware address: 0x20019594. */
void initialize_global_any_mac_address(void)
{
    global_any_mac_address = &g_network_addresses_unknown_02;
    any_mac_address_bytes = 0;
    g_network_addresses_unknown_03 = 0;
    g_network_addresses_unknown_04 = 0;
    g_network_addresses_unknown_05 = 0;
    g_network_addresses_unknown_06 = 0;
    g_network_addresses_unknown_07 = 0;
    return;
}
