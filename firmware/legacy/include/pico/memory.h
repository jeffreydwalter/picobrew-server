#ifndef PICO_MEMORY_H
#define PICO_MEMORY_H
#include "pico/recovered_types.h"
#include "pico/core_layouts.h"

/* Absolute addresses describe the original CC3200 image and RAM, not host objects. */

/* Type inferred from use; semantic identity remains incomplete. Address 0x00000002. */
#define g_simplelink_protocol_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x00000002)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00000008. */
#define NMI (*((uint8_t *)(uintptr_t)UINT32_C(0x00000008)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00000063. */
#define g_simplelink_protocol_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x00000063)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00000402. */
#define g_simplelink_spi_transport_unknown_01 (*((uint16_t *)(uintptr_t)UINT32_C(0x00000402)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00001003. */
#define g_simplelink_protocol_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x00001003)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00001006. */
#define g_simplelink_protocol_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x00001006)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00001008. */
#define g_c_library_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x00001008)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000100b. */
#define g_simplelink_protocol_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000100b)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000100f. */
#define g_c_library_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000100f)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000120b. */
#define g_simplelink_protocol_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000120b)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00001387. */
#define g_brewing_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x00001387)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00001388. */
#define g_brewing_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x00001388)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00001822. */
#define g_simplelink_protocol_unknown_07 (*((uint8_t *)(uintptr_t)UINT32_C(0x00001822)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000182e. */
#define g_simplelink_protocol_unknown_08 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000182e)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00001830. */
#define g_simplelink_protocol_unknown_09 (*((uint8_t *)(uintptr_t)UINT32_C(0x00001830)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000270f. */
#define g_c_library_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000270f)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00002710. */
#define g_brewing_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x00002710)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00002711. */
#define g_identity_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x00002711)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00003a97. */
#define g_brewing_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x00003a97)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00003a98. */
#define g_brewing_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x00003a98)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00004e1f. */
#define g_libc_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x00004e1f)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x000058ac. */
#define g_temperature_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x000058ac)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x000061a8. */
#define g_brewing_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x000061a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x000061a9. */
#define g_hardware_control_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x000061a9)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000684c. */
#define g_temperature_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000684c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000752f. */
#define g_network_api_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000752f)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00007530. */
#define g_brewing_unknown_07 (*((uint8_t *)(uintptr_t)UINT32_C(0x00007530)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x00007531. */
#define g_network_api_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x00007531)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000afc8. */
#define g_session_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000afc8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000ea60. */
#define g_brewing_unknown_08 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000ea60)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000ea61. */
#define g_persistence_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000ea61)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000f830. */
#define g_simplelink_protocol_unknown_10 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000f830)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0000fff7. */
#define g_simplelink_protocol_unknown_11 (*((uint8_t *)(uintptr_t)UINT32_C(0x0000fff7)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0001c200. */
#define g_main_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x0001c200)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0001d4c0. */
#define g_brewing_unknown_09 (*((uint8_t *)(uintptr_t)UINT32_C(0x0001d4c0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x0001fbd0. */
#define g_brewing_unknown_10 (*((uint8_t *)(uintptr_t)UINT32_C(0x0001fbd0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20004715. */
#define LAB_20004714_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20004715)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20004741. */
#define LAB_20004740_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20004741)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20004759. */
#define g_hardware_control_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20004759)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2000477d. */
#define LAB_2000477c_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x2000477d)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20004789. */
#define LAB_20004788_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20004789)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200047b1. */
#define FUN_200047b0_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x200047b1)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20005439. */
#define g_ui_workflows_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20005439)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200074d9. */
#define LAB_200074d8_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x200074d9)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20007e79. */
#define LAB_20007e78_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20007e79)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20008809. */
#define LAB_20008808_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20008809)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20008969. */
#define LAB_20008968_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20008969)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20009a61. */
#define LAB_20009a60_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20009a61)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20009b75. */
#define LAB_20009b74_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20009b75)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20009bc1. */
#define LAB_20009bc0_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20009bc1)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2000a9dd. */
#define LAB_2000a9dc_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x2000a9dd)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20015e89. */
#define LAB_20015e88_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20015e89)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200173b9. */
#define LAB_200173b8_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x200173b9)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20017a81. */
#define LAB_20017a80_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20017a81)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20017a99. */
#define LAB_20017a98_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20017a99)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20018dbd. */
#define FUN_20018dbc_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x20018dbd)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20019ad3. */
#define g_cc3200_interrupts_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20019ad3)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20022ca7. */
#define g_menus_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20022ca7)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20022e7d. */
#define g_screens_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20022e7d)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20022f98. */
#define g_programs_unknown_01 (*((uint8_t **)(uintptr_t)UINT32_C(0x20022f98)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200230ec. */
#define g_programs_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x200230ec)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023108. */
#define g_ui_menus_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20023108)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023130. */
#define g_ui_screens_unknown_01 (*((uint8_t **)(uintptr_t)UINT32_C(0x20023130)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002319c. */
#define g_programs_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002319c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200231a8. */
#define g_display_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x200231a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200231c0. */
#define g_programs_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x200231c0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200231d4. */
#define g_ui_menus_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x200231d4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200231fc. */
#define g_programs_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x200231fc)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023234. */
#define g_programs_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x20023234)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023240. */
#define g_display_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20023240)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023248. */
#define g_programs_unknown_07 (*((uint8_t *)(uintptr_t)UINT32_C(0x20023248)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023254. */
#define g_display_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20023254)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023264. */
#define g_ui_workflows_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20023264)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002327c. */
#define g_display_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002327c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023284. */
#define g_programs_unknown_08 (*((uint8_t **)(uintptr_t)UINT32_C(0x20023284)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200232e0. */
#define g_programs_unknown_09 (*((uint8_t *)(uintptr_t)UINT32_C(0x200232e0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200232f4. */
#define g_programs_unknown_10 (*((uint8_t *)(uintptr_t)UINT32_C(0x200232f4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002330c. */
#define g_programs_unknown_11 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002330c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20023330. */
#define g_ui_workflows_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20023330)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20024347. */
#define g_screens_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20024347)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20024a19. */
#define g_screens_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20024a19)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20025773. */
#define g_ui_screens_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20025773)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20026495. */
#define g_ui_screens_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20026495)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20026596. */
#define g_ui_screens_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20026596)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20026f3c. */
#define g_ui_workflows_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20026f3c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20028318. */
#define g_programs_unknown_12 (*((uint8_t **)(uintptr_t)UINT32_C(0x20028318)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002836c. */
#define g_programs_unknown_13 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002836c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200285a0. */
#define g_programs_unknown_14 (*((uint8_t *)(uintptr_t)UINT32_C(0x200285a0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200285ac. */
#define g_main_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x200285ac)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200285c4. */
#define g_programs_unknown_15 (*((uint8_t *)(uintptr_t)UINT32_C(0x200285c4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200286d4. */
#define g_programs_unknown_16 (*((uint8_t **)(uintptr_t)UINT32_C(0x200286d4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20028728. */
#define g_ui_menus_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20028728)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20028910. */
#define g_programs_unknown_17 (*((uint8_t *)(uintptr_t)UINT32_C(0x20028910)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20028928. */
#define g_display_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x20028928)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20028c34. */
#define g_ui_screens_unknown_05 (*((uint8_t **)(uintptr_t)UINT32_C(0x20028c34)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20028c70. */
#define g_display_unknown_06 (*((void **)(uintptr_t)UINT32_C(0x20028c70)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20028c98. */
#define g_graphics_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20028c98)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200291a0. */
#define g_graphics_unknown_02 (*((void **)(uintptr_t)UINT32_C(0x200291a0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200291c8. */
#define g_spi_bus_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x200291c8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200291d0. */
#define g_spi_bus_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x200291d0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200291d8. */
#define g_spi_bus_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x200291d8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200291e0. */
#define g_spi_bus_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x200291e0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200291f0. */
#define g_wifi_client_unknown_01 (*((void **)(uintptr_t)UINT32_C(0x200291f0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029242. */
#define g_simplelink_device_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029242)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029246. */
#define g_simplelink_device_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029246)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029254. */
#define g_simplelink_protocol_unknown_12 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029254)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002925c. */
#define g_simplelink_protocol_unknown_13 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002925c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029262. */
#define g_simplelink_protocol_unknown_14 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029262)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029264. */
#define g_simplelink_protocol_unknown_15 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029264)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029298. */
#define g_simplelink_protocol_unknown_16 (*((uint16_t *)(uintptr_t)UINT32_C(0x20029298)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002929a. */
#define g_simplelink_protocol_unknown_17 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002929a)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200292d4. */
#define g_simplelink_protocol_unknown_18 (*((void **)(uintptr_t)UINT32_C(0x200292d4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029308. */
#define g_simplelink_fs_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029308)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002930c. */
#define g_simplelink_fs_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002930c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029310. */
#define g_simplelink_fs_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029310)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029314. */
#define g_simplelink_fs_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029314)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029318. */
#define g_simplelink_fs_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029318)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002931c. */
#define g_simplelink_fs_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002931c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029324. */
#define g_simplelink_netapp_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029324)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029328. */
#define g_simplelink_netapp_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029328)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002932c. */
#define g_simplelink_protocol_unknown_19 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002932c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029330. */
#define g_simplelink_netapp_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029330)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029334. */
#define g_simplelink_netcfg_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029334)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029338. */
#define g_simplelink_netcfg_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029338)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002933c. */
#define g_simplelink_socket_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002933c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029340. */
#define g_simplelink_socket_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029340)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029344. */
#define g_simplelink_socket_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029344)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029348. */
#define g_simplelink_protocol_unknown_20 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029348)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029358. */
#define g_simplelink_socket_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029358)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002935c. */
#define g_simplelink_socket_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002935c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029360. */
#define g_simplelink_socket_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029360)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029364. */
#define g_simplelink_wlan_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029364)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029368. */
#define g_simplelink_wlan_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029368)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002936c. */
#define g_simplelink_wlan_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002936c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029370. */
#define g_simplelink_wlan_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029370)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029380. */
#define g_filesystem_unknown_01 (*((void **)(uintptr_t)UINT32_C(0x20029380)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200293a0. */
#define g_serial_flash_unknown_01 (*((void **)(uintptr_t)UINT32_C(0x200293a0)))

/* Pin number lookup into GPIO bitmask table. Address 0x200293c5. */
#define digital_pin_bit_masks (*((uint8_t *)(uintptr_t)UINT32_C(0x200293c5)))

/* Port lookup yields GPIO MMIO bases. Address 0x200293f0. */
#define gpio_port_base_addresses (*((uint8_t *)(uintptr_t)UINT32_C(0x200293f0)))

/* 16-bit Energia pin to physical pin mapping used by pinMode. Address 0x20029404. */
#define digital_pin_hardware_numbers (*((uint8_t *)(uintptr_t)UINT32_C(0x20029404)))

/* Byte pin-to-port mapping used by digital I/O. Address 0x20029456. */
#define digital_pin_port_indices (*((uint8_t *)(uintptr_t)UINT32_C(0x20029456)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029480. */
#define g_hardware_serial_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029480)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029484. */
#define g_hardware_serial_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029484)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029490. */
#define g_hardware_serial_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029490)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029498. */
#define g_hardware_serial_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029498)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200294a0. */
#define g_hardware_serial_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x200294a0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200294b0. */
#define g_hardware_serial_unknown_06 (*((void **)(uintptr_t)UINT32_C(0x200294b0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200294c8. */
#define g_hardware_serial_unknown_07 (*((uint8_t *)(uintptr_t)UINT32_C(0x200294c8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200294d8. */
#define g_network_addresses_unknown_01 (*((void **)(uintptr_t)UINT32_C(0x200294d8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200294e8. */
#define g_network_addresses_unknown_02 (*((void **)(uintptr_t)UINT32_C(0x200294e8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200294f0. */
#define g_cc3200_interrupts_unknown_02 (*((uint8_t **)(uintptr_t)UINT32_C(0x200294f0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029508. */
#define g_cc3200_interrupts_unknown_03 (*((uint8_t **)(uintptr_t)UINT32_C(0x20029508)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029520. */
#define g_cc3200_interrupts_unknown_04 (*((uint8_t **)(uintptr_t)UINT32_C(0x20029520)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029538. */
#define g_cc3200_interrupts_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029538)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002960c. */
#define g_cc3200_gpio_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002960c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002970c. */
#define g_cc3200_clock_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002970c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029710. */
#define g_cc3200_clock_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029710)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200297b8. */
#define g_cc3200_spi_unknown_01 (*((uint32_t *)(uintptr_t)UINT32_C(0x200297b8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200297dc. */
#define g_cc3200_spi_unknown_02 (*((uint32_t *)(uintptr_t)UINT32_C(0x200297dc)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200297fc. */
#define g_libc_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x200297fc)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002980f. */
#define g_libc_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002980f)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029813. */
#define g_libc_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029813)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002981b. */
#define g_libc_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002981b)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029820. */
#define g_libc_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029820)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029838. */
#define g_libc_unknown_07 (*((uint32_t *)(uintptr_t)UINT32_C(0x20029838)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029874. */
#define g_c_library_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029874)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029878. */
#define g_c_library_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029878)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002987c. */
#define g_c_library_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002987c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200298b0. */
#define g_c_library_unknown_07 (*((uint8_t *)(uintptr_t)UINT32_C(0x200298b0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200298b1. */
#define g_c_library_unknown_08 (*((uint8_t *)(uintptr_t)UINT32_C(0x200298b1)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200299d4. */
#define g_c_library_unknown_09 (*((uint8_t *)(uintptr_t)UINT32_C(0x200299d4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200299d8. */
#define g_c_library_unknown_10 (*((uint8_t *)(uintptr_t)UINT32_C(0x200299d8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029aa0. */
#define g_libc_unknown_08 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029aa0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029ae0. */
#define g_c_library_unknown_11 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029ae0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029ae8. */
#define g_libc_unknown_09 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029ae8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029bb0. */
#define g_libc_unknown_10 (*((uint32_t *)(uintptr_t)UINT32_C(0x20029bb0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029bd8. */
#define g_c_library_unknown_12 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029bd8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029be4. */
#define __init_array_start (*((void **)(uintptr_t)UINT32_C(0x20029be4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029c18. */
#define _data (*((uint32_t *)(uintptr_t)UINT32_C(0x20029c18)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20029e60. */
#define g_ui_screens_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x20029e60)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002a5e0. */
#define g_ui_screens_unknown_07 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002a5e0)))

/* Pointer table indexed by numeric digit0..9. Address 0x2002a8c0. */
#define g_large_digit_bitmaps (*((uint8_t **)(uintptr_t)UINT32_C(0x2002a8c0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002a908. */
#define g_brewing_unknown_11 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002a908)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002a910. */
#define g_ui_screens_unknown_08 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002a910)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002aa10. */
#define g_brewing_unknown_12 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002aa10)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002aa20. */
#define g_screens_unknown_04 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002aa20)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002ac80. */
#define g_hardware_control_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002ac80)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002ac80. */
#define g_hardware_control_unknown_03_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002ac80)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002aca4. */
#define g_brewing_unknown_13 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002aca4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002aeb0. */
#define g_picostill_unknown_01 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002aeb0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002aed4. */
#define g_brewing_unknown_14 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002aed4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002af00. */
#define g_picostill_unknown_02 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002af00)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002af24. */
#define g_ui_screens_unknown_09 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002af24)))

/* 1024-byte buffer cleared by display_clear and read by OLED flush. Address 0x2002af60. */
#define g_display_framebuffer (*((uint8_t *)(uintptr_t)UINT32_C(0x2002af60)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002afe0. */
#define g_display_unknown_07 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002afe0)))

/* WiFi status enum storage used across begin/disconnect/events. Address 0x2002b360. */
#define wifi_connection_status (*((uint8_t *)(uintptr_t)UINT32_C(0x2002b360)))

/* ADC12-bit sample is shifted according to this configured bit count. Address 0x2002b364. */
#define analog_read_resolution (*((uint32_t *)(uintptr_t)UINT32_C(0x2002b364)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b368. */
#define g_software_float_unknown_01 (*((char *)(uintptr_t)UINT32_C(0x2002b368)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b4b8. */
#define g_c_library_unknown_13 (*((uint32_t **)(uintptr_t)UINT32_C(0x2002b4b8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b4bc. */
#define g_c_library_unknown_14 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002b4bc)))

/* Global pointer loaded by libc wrappers before _r routines. Address 0x2002b798. */
#define libc_reentrancy (*((int **)(uintptr_t)UINT32_C(0x2002b798)))

/* Global pointer loaded by libc wrappers before _r routines. Alternate typed view of the same
 * storage. Address 0x2002b798. */
#define libc_reentrancy_as_uint32_t_pointer (*((uint32_t **)(uintptr_t)UINT32_C(0x2002b798)))

/* Global pointer loaded by libc wrappers before _r routines. Alternate typed view of the same
 * storage. Address 0x2002b798. */
#define libc_reentrancy_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002b798)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b79c. */
#define g_c_library_unknown_15 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002b79c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b7a0. */
#define g_c_library_unknown_16 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002b7a0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b7a4. */
#define g_c_library_unknown_17 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002b7a4)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002b7a4. */
#define g_c_library_unknown_17_as_int (*((int *)(uintptr_t)UINT32_C(0x2002b7a4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b7a8. */
#define g_c_library_unknown_18 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002b7a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b7ac. */
#define g_c_library_unknown_19 (*((uint32_t **)(uintptr_t)UINT32_C(0x2002b7ac)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002b7ac. */
#define g_c_library_unknown_19_as_int (*((int *)(uintptr_t)UINT32_C(0x2002b7ac)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002b7b0. */
#define g_c_library_unknown_20 (*((uint32_t **)(uintptr_t)UINT32_C(0x2002b7b0)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002b7b0. */
#define g_c_library_unknown_20_as_int (*((int *)(uintptr_t)UINT32_C(0x2002b7b0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bba4. */
#define g_c_library_unknown_21 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bba4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bba8. */
#define g_c_library_unknown_22 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002bba8)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002bba8. */
#define g_c_library_unknown_22_as_int (*((int *)(uintptr_t)UINT32_C(0x2002bba8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bbac. */
#define g_libc_unknown_11 (*((int *)(uintptr_t)UINT32_C(0x2002bbac)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bbb0. */
#define g_c_library_unknown_23 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bbb0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bbe8. */
#define _edata (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bbe8)))

/* 128x64 graphics object constructed at image BSS start; API calls treat it as Print/GFX instance.
 * Address 0x2002bc00. */
#define g_display (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bc00)))

/* Multiplied by6/8 for text dimensions; graphics setters update this field. Address 0x2002bc18. */
#define g_display_text_scale (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc18)))

/* Display constructor assigns GPIO15 to field0x20; precise display signal not independently
 * established. Address 0x2002bc20. */
#define g_display_pin_20 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc20)))

/* Display constructor assigns GPIO7 to field0x21; precise display signal not independently
 * established. Address 0x2002bc21. */
#define g_display_pin_21 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc21)))

/* Display flush sets this pin before transferring pixels. Address 0x2002bc22. */
#define g_display_pin_command_data (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc22)))

/* Display flush sets this pin before transferring pixels. Alternate typed view of the same storage.
 * Address 0x2002bc22. */
#define g_display_pin_command_data_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc22)))

/* Display flush drives pin4 as select. Address 0x2002bc24. */
#define g_spi_chip_select_display (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc24)))

/* Display flush drives pin4 as select. Alternate typed view of the same storage. Address
 * 0x2002bc24. */
#define g_spi_chip_select_display_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc24)))

/* SPI/link helpers drive pin18; shared display flush toggles it. Address 0x2002bc25. */
#define g_spi_chip_select_auxiliary (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc25)))

/* SPI/link helpers drive pin18; shared display flush toggles it. Alternate typed view of the same
 * storage. Address 0x2002bc25. */
#define g_spi_chip_select_auxiliary_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc25)))

/* Encoder object base used by position/button APIs. Address 0x2002bc28. */
#define g_encoder (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc28)))

/* Encoder base+0x10 configured by set_range. Address 0x2002bc38. */
#define g_encoder_minimum (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bc38)))

/* Encoder base+0x14 configured by set_range. Address 0x2002bc3c. */
#define g_encoder_maximum (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bc3c)))

/* Encoder base+0x1c; reset writes1. Address 0x2002bc44. */
#define g_encoder_step (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bc44)))

/* Encoder base+0x20 incremented/decremented by quadrature handler. Address 0x2002bc48. */
#define g_encoder_position (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bc48)))

/* Encoder base+0x28 toggles clamp/wrap behavior. Address 0x2002bc50. */
#define g_encoder_wrap_enabled (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc50)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bc54. */
#define g_temperature_unknown_03 (*((char *)(uintptr_t)UINT32_C(0x2002bc54)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bc58. */
#define g_temperature_unknown_04 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bc58)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bc5c. */
#define g_temperature_unknown_05 (*((float *)(uintptr_t)UINT32_C(0x2002bc5c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bc60. */
#define g_temperature_unknown_06 (*((float *)(uintptr_t)UINT32_C(0x2002bc60)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bc64. */
#define g_temperature_unknown_07 (*((int *)(uintptr_t)UINT32_C(0x2002bc64)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bc68. */
#define g_temperature_unknown_08 (*((char *)(uintptr_t)UINT32_C(0x2002bc68)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bc6c. */
#define g_temperature_unknown_09 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bc6c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bc70. */
#define g_temperature_unknown_10 (*((char *)(uintptr_t)UINT32_C(0x2002bc70)))

/* Stepper capability check at 0x2000ccd8 sets flag; compartment movement at 0x2000cd4c chooses
 * legacy mapping when set. Address 0x2002bc71. */
#define g_legacy_compartment_mapping (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc71)))

/* Stepper capability check at 0x2000ccd8 sets flag; compartment movement at 0x2000cd4c chooses
 * legacy mapping when set. Alternate typed view of the same storage. Address 0x2002bc71. */
#define g_legacy_compartment_mapping_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc71)))

/* Stepper capability check at 0x2000ccd8 sets flag; compartment movement at 0x2000cd4c chooses
 * legacy mapping when set. Alternate typed view of the same storage. Address 0x2002bc71. */
#define g_legacy_compartment_mapping_as_char (*((char *)(uintptr_t)UINT32_C(0x2002bc71)))

/* 22action entries followed by count; menu APIs use this base. Address 0x2002bc74. */
#define g_active_menu (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bc74)))

/* Menu count at active menu+0x58. Address 0x2002bccc. */
#define g_active_menu_count (*((int *)(uintptr_t)UINT32_C(0x2002bccc)))

/* Menu count at active menu+0x58. Alternate typed view of the same storage. Address 0x2002bccc. */
#define g_active_menu_count_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bccc)))

/* millis timestamp used to throttle recovery-state writes. Address 0x2002bcd0. */
#define g_last_recovery_save_ms (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bcd0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bcd4. */
#define g_session_unknown_02 (*((char **)(uintptr_t)UINT32_C(0x2002bcd4)))

/* Pointer to program defaults/labels/action tables; initialized to2c108. Address 0x2002bcd8. */
#define g_active_program_context (*((uint8_t **)(uintptr_t)UINT32_C(0x2002bcd8)))

/* Pointer to program defaults/labels/action tables; initialized to2c108. Alternate typed view of
 * the same storage. Address 0x2002bcd8. */
#define g_active_program_context_as_int (*((int *)(uintptr_t)UINT32_C(0x2002bcd8)))

/* Pointer to program defaults/labels/action tables; initialized to2c108. Alternate typed view of
 * the same storage. Address 0x2002bcd8. */
#define g_active_program_context_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bcd8)))

/* Pointer to program defaults/labels/action tables; initialized to2c108. Alternate typed view of
 * the same storage. Address 0x2002bcd8. */
#define g_active_program_context_as_char_pointer (*((char **)(uintptr_t)UINT32_C(0x2002bcd8)))

/* Pointer to program defaults/labels/action tables; initialized to2c108. Alternate typed view of
 * the same storage. Address 0x2002bcd8. */
#define g_active_program_context_as_picoprogramcontext_pointer                                     \
    (*((PicoProgramContext **)(uintptr_t)UINT32_C(0x2002bcd8)))

/* Saved as first recipe-related text field in recovery CSV; source from recipe loading. Address
 * 0x2002bcdc. */
#define g_recipe_identifier (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bcdc)))

/* Base of32-byte recipe step array parsed from server response. Address 0x2002bd08. */
#define g_recipe_steps (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bd08)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bd1e. */
#define g_brewing_unknown_15 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bd1e)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bd1f. */
#define g_brewing_unknown_16 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bd1f)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bd20. */
#define g_brewing_unknown_17 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bd20)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bd24. */
#define g_brewing_unknown_18 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bd24)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bd28. */
#define g_initialization_unknown_01 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bd28)))

/* Parsed step count capped at15. Address 0x2002bee8. */
#define g_recipe_step_count (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bee8)))

/* Parsed step count capped at15. Alternate typed view of the same storage. Address 0x2002bee8. */
#define g_recipe_step_count_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bee8)))

/* Recipe header integer rendered as a large metric; likely IBU, retained neutral pending schema
 * confirmation. Address 0x2002beec. */
#define g_recipe_integer_metric (*((int *)(uintptr_t)UINT32_C(0x2002beec)))

/* Recipe header integer rendered as a large metric; likely IBU, retained neutral pending schema
 * confirmation. Alternate typed view of the same storage. Address 0x2002beec. */
#define g_recipe_integer_metric_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002beec)))

/* Low word of recipe header double rendered with one decimal; likely ABV. Address 0x2002bef0. */
#define g_recipe_decimal_metric_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bef0)))

/* High word paired with recipe decimal metric. Address 0x2002bef4. */
#define g_recipe_decimal_metric_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bef4)))

/* Recipe adjustment UI and URL assembly identify this selected ABV adjustment. Address 0x2002bef8.
 */
#define g_recipe_abv_adjustment (*((int *)(uintptr_t)UINT32_C(0x2002bef8)))

/* Recipe adjustment UI and URL assembly identify this selected ABV adjustment. Alternate typed view
 * of the same storage. Address 0x2002bef8. */
#define g_recipe_abv_adjustment_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bef8)))

/* Recipe adjustment UI and URL assembly identify this selected IBU adjustment. Address 0x2002befc.
 */
#define g_recipe_ibu_adjustment (*((int *)(uintptr_t)UINT32_C(0x2002befc)))

/* Recipe adjustment UI and URL assembly identify this selected IBU adjustment. Alternate typed view
 * of the same storage. Address 0x2002befc. */
#define g_recipe_ibu_adjustment_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002befc)))

/* ABV adjustment enabled when this reference is not-1; exact scaling interpretation remains
 * partial. Address 0x2002bf00. */
#define g_recipe_abv_adjustment_reference (*((int *)(uintptr_t)UINT32_C(0x2002bf00)))

/* Indexes32-byte recipe steps for current step title. Address 0x2002bf04. */
#define g_current_recipe_step_index (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bf04)))

/* Indexes32-byte recipe steps for current step title. Alternate typed view of the same storage.
 * Address 0x2002bf04. */
#define g_current_recipe_step_index_as_int (*((int *)(uintptr_t)UINT32_C(0x2002bf04)))

/* Bitterness adjustment enabled when this reference is not-1; exact scaling interpretation remains
 * partial. Address 0x2002bf08. */
#define g_recipe_ibu_adjustment_reference (*((int *)(uintptr_t)UINT32_C(0x2002bf08)))

/* Bitterness adjustment enabled when this reference is not-1; exact scaling interpretation remains
 * partial. Alternate typed view of the same storage. Address 0x2002bf08. */
#define g_recipe_ibu_adjustment_reference_as_uint32_t                                              \
    (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bf08)))

/* Animated progress state begins with row/start-column/end-column/glyph fields. Address 0x2002bf10.
 */
#define g_progress_indicator (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf10)))

/* Animated progress state begins with row/start-column/end-column/glyph fields. Alternate typed
 * view of the same storage. Address 0x2002bf10. */
#define g_progress_indicator_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf10)))

/* Second progress-state byte used as rectangle start column. Address 0x2002bf11. */
#define g_progress_start_column (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf11)))

/* Second progress-state byte used as rectangle start column. Alternate typed view of the same
 * storage. Address 0x2002bf11. */
#define g_progress_start_column_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf11)))

/* Third progress-state byte used as rectangle end column. Address 0x2002bf12. */
#define g_progress_end_column (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf12)))

/* Third progress-state byte used as rectangle end column. Alternate typed view of the same storage.
 * Address 0x2002bf12. */
#define g_progress_end_column_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf12)))

/* Fourth progress-state byte reset to0 whenever progress animation starts. Address 0x2002bf13. */
#define g_progress_glyph_index (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf13)))

/* Fourth progress-state byte reset to0 whenever progress animation starts. Alternate typed view of
 * the same storage. Address 0x2002bf13. */
#define g_progress_glyph_index_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf13)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bf14. */
#define g_initialization_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf14)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bf18. */
#define g_initialization_unknown_03 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bf18)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bf1c. */
#define g_initialization_unknown_04 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002bf1c)))

/* Progress-state+0x10 active flag. Address 0x2002bf20. */
#define g_progress_active (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf20)))

/* Progress-state+0x10 active flag. Alternate typed view of the same storage. Address 0x2002bf20. */
#define g_progress_active_as_char (*((char *)(uintptr_t)UINT32_C(0x2002bf20)))

/* One22-byte row in display menu label storage. Address 0x2002bf24. */
#define g_menu_label_0 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf24)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002bf39. */
#define g_ui_menus_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf39)))

/* One22-byte row in display menu label storage. Address 0x2002bf3a. */
#define g_menu_label_1 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf3a)))

/* One22-byte row in display menu label storage. Address 0x2002bf50. */
#define g_menu_label_2 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf50)))

/* One22-byte row in display menu label storage. Address 0x2002bf66. */
#define g_menu_label_3 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf66)))

/* One22-byte row in display menu label storage. Address 0x2002bf7c. */
#define g_menu_label_4 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf7c)))

/* One22-byte row in display menu label storage. Address 0x2002bf92. */
#define g_menu_label_5 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bf92)))

/* One22-byte row in display menu label storage. Address 0x2002bfa8. */
#define g_menu_label_6 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bfa8)))

/* One22-byte row in display menu label storage. Address 0x2002bfbe. */
#define g_menu_label_7 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bfbe)))

/* One22-byte row in display menu label storage. Address 0x2002bfd4. */
#define g_menu_label_8 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bfd4)))

/* One22-byte row in display menu label storage. Address 0x2002bfea. */
#define g_menu_label_9 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002bfea)))

/* One22-byte row in display menu label storage. Address 0x2002c000. */
#define g_menu_label_10 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c000)))

/* One22-byte row in display menu label storage. Address 0x2002c016. */
#define g_menu_label_11 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c016)))

/* One22-byte row in display menu label storage. Address 0x2002c02c. */
#define g_menu_label_12 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c02c)))

/* One22-byte row in display menu label storage. Address 0x2002c042. */
#define g_menu_label_13 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c042)))

/* One22-byte row in display menu label storage. Address 0x2002c058. */
#define g_menu_label_14 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c058)))

/* One22-byte row in display menu label storage. Address 0x2002c06e. */
#define g_menu_label_15 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c06e)))

/* Session object begins with session-ID text; program, target temperature, duration, location, type
 * and title follow at offsets 0x20..0x34. Address 0x2002c108. */
#define g_session (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c108)))

/* Explicit session.programId assertion in pause instructions. Address 0x2002c128. */
#define g_session_program_id (*((int *)(uintptr_t)UINT32_C(0x2002c128)))

/* Stores return of getTemperatureInput. Address 0x2002c12c. */
#define g_session_target_fahrenheit (*((uint32_t *)(uintptr_t)UINT32_C(0x2002c12c)))

/* Stores return of getTemperatureInput. Alternate typed view of the same storage. Address
 * 0x2002c12c. */
#define g_session_target_fahrenheit_as_int (*((int *)(uintptr_t)UINT32_C(0x2002c12c)))

/* Stores return of getTemperatureInput. Alternate typed view of the same storage. Address
 * 0x2002c12c. */
#define g_session_target_fahrenheit_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c12c)))

/* Stores return of getTimeInput. Address 0x2002c130. */
#define g_session_hold_minutes (*((int *)(uintptr_t)UINT32_C(0x2002c130)))

/* Stores return of getTimeInput. Alternate typed view of the same storage. Address 0x2002c130. */
#define g_session_hold_minutes_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002c130)))

/* Manual brew initializes location 1 and passes it to location-to-compartment mapper before each
 * step. Address 0x2002c134. */
#define g_session_location (*((uint32_t *)(uintptr_t)UINT32_C(0x2002c134)))

/* Explicit SessionTypes::SousVide/Coffee assertions. Address 0x2002c138. */
#define g_session_type (*((int *)(uintptr_t)UINT32_C(0x2002c138)))

/* Explicit SessionTypes::SousVide/Coffee assertions. Alternate typed view of the same storage.
 * Address 0x2002c138. */
#define g_session_type_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002c138)))

/* Program title string passed to temperature/time/input instruction renderers and format%s. Address
 * 0x2002c13c. */
#define g_session_title (*((char **)(uintptr_t)UINT32_C(0x2002c13c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002c140. */
#define g_ui_screens_unknown_10 (*((char **)(uintptr_t)UINT32_C(0x2002c140)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002c14c. */
#define g_ui_screens_unknown_11 (*((char **)(uintptr_t)UINT32_C(0x2002c14c)))

/* run_brew_step asserts actions != NULL and indexes four-byte action IDs through this pointer.
 * Address 0x2002c158. */
#define g_step_action_ids (*((int *)(uintptr_t)UINT32_C(0x2002c158)))

/* run_brew_step asserts selected index < actionsCount before reading g_step_action_ids. Address
 * 0x2002c15c. */
#define g_step_action_count (*((int *)(uintptr_t)UINT32_C(0x2002c15c)))

/* Incremented by nonfatal assertion logger. Address 0x2002c160. */
#define g_assertion_count (*((int *)(uintptr_t)UINT32_C(0x2002c160)))

/* 0English,1Spanish,2French branches throughout localized labels. Address 0x2002c164. */
#define g_language (*((char *)(uintptr_t)UINT32_C(0x2002c164)))

/* 0English,1Spanish,2French branches throughout localized labels. Alternate typed view of the same
 * storage. Address 0x2002c164. */
#define g_language_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c164)))

/* 0English,1Spanish,2French branches throughout localized labels. Alternate typed view of the same
 * storage. Address 0x2002c164. */
#define g_language_as_uint8_t_2 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c164)))

/* WiFiClient object used for request Print operations. Address 0x2002c168. */
#define g_http_client (*((uint32_t *)(uintptr_t)UINT32_C(0x2002c168)))

/* Saved password file and connectToApInternal use this buffer. Address 0x2002c290. */
#define g_wifi_password (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c290)))

/* Alias one byte into WiFi password for hexadecimal WEP parsing. Address 0x2002c291. */
#define g_wifi_password_second_byte (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c291)))

/* Saved SSID file and network connect use this buffer. Address 0x2002c2d0. */
#define g_wifi_ssid (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c2d0)))

/* Network selector5=WEP,7=open, others secured; persisted as ASCII digit. Address 0x2002c2f1. */
#define g_wifi_security_type (*((char *)(uintptr_t)UINT32_C(0x2002c2f1)))

/* Network selector5=WEP,7=open, others secured; persisted as ASCII digit. Alternate typed view of
 * the same storage. Address 0x2002c2f1. */
#define g_wifi_security_type_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c2f1)))

/* Network selector5=WEP,7=open, others secured; persisted as ASCII digit. Alternate typed view of
 * the same storage. Address 0x2002c2f1. */
#define g_wifi_security_type_as_uint8_t_2 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c2f1)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002c2f2. */
#define g_network_api_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002c2f2)))

/* Controller base; prefix also contains PicoStill telemetry. Address 0x2002d2f8. */
#define g_controller (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d2f8)))

/* Received telemetry units compared with chosen display units. Address 0x2002d300. */
#define g_still_temperature_units (*((char *)(uintptr_t)UINT32_C(0x2002d300)))

/* Low word of PicoStill T1 displayed in still readings screen. Address 0x2002d308. */
#define g_still_temperature1_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d308)))

/* High word paired with T1. Address 0x2002d30c. */
#define g_still_temperature1_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d30c)))

/* Low word of PicoStill T2 displayed in still readings screen. Address 0x2002d310. */
#define g_still_temperature2_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d310)))

/* High word paired with T2. Address 0x2002d314. */
#define g_still_temperature2_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d314)))

/* Low word of PicoStill T3 displayed in still readings screen. Address 0x2002d318. */
#define g_still_temperature3_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d318)))

/* High word paired with T3. Address 0x2002d31c. */
#define g_still_temperature3_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d31c)))

/* Low word of PicoStill pressure formatted with psi suffix. Address 0x2002d328. */
#define g_still_pressure_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d328)))

/* High word paired with pressure. Address 0x2002d32c. */
#define g_still_pressure_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d32c)))

/* Controls Fan: ON/OFF label. Address 0x2002d331. */
#define g_still_fan_on (*((char *)(uintptr_t)UINT32_C(0x2002d331)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d338. */
#define g_initialization_unknown_05 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d338)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d33c. */
#define g_initialization_unknown_06 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d33c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d340. */
#define g_initialization_unknown_07 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d340)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d344. */
#define g_initialization_unknown_08 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d344)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d348. */
#define g_initialization_unknown_09 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d348)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d34c. */
#define g_initialization_unknown_10 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d34c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d350. */
#define g_initialization_unknown_11 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d350)))

/* Controller offset 0x60; set by compartment movement and run_brew_step compartment argument.
 * Address 0x2002d358. */
#define g_controller_compartment (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d358)))

/* Controller offset 0x61; zero permits heater/pump ISR, nonzero values trigger
 * sensor/heating/reprime/Still recovery paths. Address 0x2002d359. */
#define g_controller_status (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d359)))

/* Controller offset 0x61; zero permits heater/pump ISR, nonzero values trigger
 * sensor/heating/reprime/Still recovery paths. Alternate typed view of the same storage. Address
 * 0x2002d359. */
#define g_controller_status_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d359)))

/* Controller offset 0x61; zero permits heater/pump ISR, nonzero values trigger
 * sensor/heating/reprime/Still recovery paths. Alternate typed view of the same storage. Address
 * 0x2002d359. */
#define g_controller_status_as_char (*((char *)(uintptr_t)UINT32_C(0x2002d359)))

/* Controller offset 0x62; setBrewState switch writes phase 0..8 here; phase 4 is idle. Address
 * 0x2002d35a. */
#define g_controller_brew_phase (*((char *)(uintptr_t)UINT32_C(0x2002d35a)))

/* Controller offset 0x62; setBrewState switch writes phase 0..8 here; phase 4 is idle. Alternate
 * typed view of the same storage. Address 0x2002d35a. */
#define g_controller_brew_phase_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d35a)))

/* Controller offset 0x62; setBrewState switch writes phase 0..8 here; phase 4 is idle. Alternate
 * typed view of the same storage. Address 0x2002d35a. */
#define g_controller_brew_phase_as_uint8_t_2 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d35a)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d368. */
#define g_temperature_unknown_11 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d368)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d3a0. */
#define g_initialization_unknown_12 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d3a0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d3a8. */
#define g_initialization_unknown_13 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d3a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d420. */
#define g_initialization_unknown_14 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d420)))

/* Controller+0x1a0 is set by shuttle calibration convergence at 0x20005a20, persisted in restart
 * records and restored by parseWasBrewing; session end preserves saved scale only when set. Address
 * 0x2002d498. */
#define g_shuttle_calibration_complete (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d498)))

/* Controller+0x1a0 is set by shuttle calibration convergence at 0x20005a20, persisted in restart
 * records and restored by parseWasBrewing; session end preserves saved scale only when set.
 * Alternate typed view of the same storage. Address 0x2002d498. */
#define g_shuttle_calibration_complete_as_char (*((char *)(uintptr_t)UINT32_C(0x2002d498)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d499. */
#define g_initialization_unknown_15 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d499)))

/* Temperature ISR increments once per second and uses 30/60-second boundaries to update trend and
 * identify heating failures. Address 0x2002d49c. */
#define g_temperature_trend_seconds (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d49c)))

/* Temperature ISR increments once per second and uses 30/60-second boundaries to update trend and
 * identify heating failures. Alternate typed view of the same storage. Address 0x2002d49c. */
#define g_temperature_trend_seconds_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d49c)))

/* Controller offset 0x1a8; decrements in timed phases, increments in indefinite-hold phase 8,
 * rendered as hours/minutes/seconds. Address 0x2002d4a0. */
#define g_step_time_seconds (*((int *)(uintptr_t)UINT32_C(0x2002d4a0)))

/* Controller offset 0x1a8; decrements in timed phases, increments in indefinite-hold phase 8,
 * rendered as hours/minutes/seconds. Alternate typed view of the same storage. Address 0x2002d4a0.
 */
#define g_step_time_seconds_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4a0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4a4. */
#define g_temperature_unknown_12 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4a4)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002d4a4. */
#define g_temperature_unknown_12_as_int (*((int *)(uintptr_t)UINT32_C(0x2002d4a4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4a8. */
#define g_hardware_control_unknown_04 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4ac. */
#define g_hardware_control_unknown_05 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4ac)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4b0. */
#define g_temperature_unknown_13 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4b0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4b4. */
#define g_temperature_unknown_14 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4b4)))

/* Low word of target double displayed as Target/Goal. Address 0x2002d4c8. */
#define g_wort_target_fahrenheit_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4c8)))

/* High word paired with wort target. Address 0x2002d4cc. */
#define g_wort_target_fahrenheit_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4cc)))

/* Low word of thermoblock PID target double at controller+0x1d8; PID constructor receives its
 * address. Address 0x2002d4d0. */
#define g_thermoblock_target_fahrenheit_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4d0)))

/* High word paired with thermoblock target; initialized with high word 0x4072c000 (300F). Address
 * 0x2002d4d4. */
#define g_thermoblock_target_fahrenheit_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4d4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4d8. */
#define g_initialization_unknown_16 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4d8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4dc. */
#define g_hardware_control_unknown_06 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4dc)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4e0. */
#define g_initialization_unknown_17 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4e0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d4e4. */
#define g_hardware_control_unknown_07 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4e4)))

/* Low word printed under Wort / W: and logged as wort. Address 0x2002d4e8. */
#define g_wort_temperature_fahrenheit_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4e8)))

/* High word paired with wort temperature. Address 0x2002d4ec. */
#define g_wort_temperature_fahrenheit_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4ec)))

/* Low word printed under Steam / S: and logged as therm. Address 0x2002d4f0. */
#define g_steam_temperature_fahrenheit_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4f0)))

/* High word paired with steam temperature. Address 0x2002d4f4. */
#define g_steam_temperature_fahrenheit_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4f4)))

/* Heater/pump ISR advances this timestamp in 3000 ms control-cycle increments. Address 0x2002d4f8.
 */
#define g_heater_cycle_started_ms (*((int *)(uintptr_t)UINT32_C(0x2002d4f8)))

/* Shuttle output is enabled while elapsed time from shuttle pulse start is below this value;
 * computed in temperature ISR. Address 0x2002d4fc. */
#define g_shuttle_on_time_ms (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d4fc)))

/* Heater/pump ISR compares shuttle elapsed time to on-time and advances this epoch by 500 ms.
 * Address 0x2002d500. */
#define g_shuttle_pulse_started_ms (*((int *)(uintptr_t)UINT32_C(0x2002d500)))

/* Heater/pump timer ISR accumulates 20 whenever the heater is enabled; reset/read by temperature
 * control. Address 0x2002d504. */
#define g_heater_on_time_ms (*((int *)(uintptr_t)UINT32_C(0x2002d504)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d508. */
#define g_initialization_unknown_18 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d508)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d510. */
#define g_initialization_unknown_19 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d510)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d514. */
#define g_initialization_unknown_20 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d514)))

/* Low word formatted to3decimals for SS.txt and logged under shutScale. Address 0x2002d518. */
#define g_shuttle_scale_low (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d518)))

/* High word paired with saved shuttle scale. Address 0x2002d51c. */
#define g_shuttle_scale_high (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d51c)))

/* Set by SHA/MD5 interrupt bit8 and consumed before hash operations. Address 0x2002d520. */
#define g_hash_interrupt_ready (*((char *)(uintptr_t)UINT32_C(0x2002d520)))

/* Set by SHA/MD5 interrupt bit8 and consumed before hash operations. Alternate typed view of the
 * same storage. Address 0x2002d520. */
#define g_hash_interrupt_ready_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d520)))

/* 32-byte field set by screen_set_recipe_name. Address 0x2002d524. */
#define g_screen_recipe_name (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d524)))

/* 32-byte field set by screen_set_step. Address 0x2002d544. */
#define g_screen_step_name (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d544)))

/* Set by screen_set_step and selects progress bitmap0..7. Address 0x2002d564. */
#define g_screen_progress_component (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d564)))

/* Set by screen_set_step and selects progress bitmap0..7. Alternate typed view of the same storage.
 * Address 0x2002d564. */
#define g_screen_progress_component_as_int (*((int *)(uintptr_t)UINT32_C(0x2002d564)))

/* setBrewState assigns display states 0..3 independently of controller phases 0..8; localized
 * four-entry label lookup uses this state. Address 0x2002d568. */
#define g_screen_brew_state (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d568)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d56c. */
#define g_initialization_unknown_21 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d56c)))

/* Reader object used by MFRC-style register/UID APIs. Address 0x2002d570. */
#define g_rfid_reader (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d570)))

/* Selects basic/detailed action label and detailed temperature footer. Address 0x2002d57f. */
#define g_detailed_status (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d57f)))

/* Selects basic/detailed action label and detailed temperature footer. Alternate typed view of the
 * same storage. Address 0x2002d57f. */
#define g_detailed_status_as_char (*((char *)(uintptr_t)UINT32_C(0x2002d57f)))

/* PicoStill state object; holds addresses, LAN/session flags, failure counter, SSID and UID at
 * verified offsets. Address 0x2002d580. */
#define g_still_controller (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d580)))

/* Set from action 3 ConnectToLAN response being T. Address 0x2002d588. */
#define g_still_lan_connected (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d588)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d58c. */
#define g_initialization_unknown_22 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d58c)))

/* Copied into IPAddress scratch object and passed to WiFiClient connect for PicoStill requests on
 * port 80. Address 0x2002d590. */
#define g_still_active_ipv4 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d590)))

/* PicoStill status request increments this on failure, resets it after success and exposes it in
 * controller telemetry. Address 0x2002d595. */
#define g_still_consecutive_failures (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d595)))

/* PicoStill_ prefix plus stored device identifier. Address 0x2002d596. */
#define g_still_access_point_ssid (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d596)))

/* Identifier copied before building PicoStill SSID. Address 0x2002d5b6. */
#define g_still_identifier (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d5b6)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d5c8. */
#define g_network_api_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d5c8)))

/* Explicit offline-mode log at setup; setup/logging paths bypass API operations when nonzero.
 * Address 0x2002d9c8. */
#define g_offline_mode (*((char *)(uintptr_t)UINT32_C(0x2002d9c8)))

/* 0showsF, nonzero converts temperatures to Celsius. Address 0x2002d9c9. */
#define g_temperature_unit_celsius (*((char *)(uintptr_t)UINT32_C(0x2002d9c9)))

/* 0showsF, nonzero converts temperatures to Celsius. Alternate typed view of the same storage.
 * Address 0x2002d9c9. */
#define g_temperature_unit_celsius_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d9c9)))

/* 0showsF, nonzero converts temperatures to Celsius. Alternate typed view of the same storage.
 * Address 0x2002d9c9. */
#define g_temperature_unit_celsius_as_uint8_t_2 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002d9c9)))

/* wifi_log_event increments on failed API request, resets on success; five failures cause
 * connection reset. Address 0x2002d9cc. */
#define g_api_log_consecutive_failures (*((int *)(uintptr_t)UINT32_C(0x2002d9cc)))

/* Current page of UI carousel; callbacks indexed relative to the page-state object. Address
 * 0x2002d9d0. */
#define g_carousel_page_index (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d9d0)))

/* Initialized5 for five page callbacks. Address 0x2002d9d4. */
#define g_carousel_page_count (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d9d4)))

/* Initialized5 for five page callbacks. Alternate typed view of the same storage. Address
 * 0x2002d9d4. */
#define g_carousel_page_count_as_int (*((int *)(uintptr_t)UINT32_C(0x2002d9d4)))

/* Carousel timestamp field reset during global initialization. Address 0x2002d9d8. */
#define g_carousel_last_update_ms (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d9d8)))

/* Initialized10000ms for page rotation. Address 0x2002d9dc. */
#define g_carousel_interval_ms (*((uint8_t **)(uintptr_t)UINT32_C(0x2002d9dc)))

/* Base of rotating UI page function pointers. Address 0x2002d9e0. */
#define g_carousel_page_callbacks (*((uint8_t **)(uintptr_t)UINT32_C(0x2002d9e0)))

/* Base of rotating UI page function pointers. Alternate typed view of the same storage. Address
 * 0x2002d9e0. */
#define g_carousel_page_callbacks_as_picointerrupthandler_pointer                                  \
    (*((PicoInterruptHandler **)(uintptr_t)UINT32_C(0x2002d9e0)))

/* Base of rotating UI page function pointers. Alternate typed view of the same storage. Address
 * 0x2002d9e0. */
#define g_carousel_page_callbacks_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d9e0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d9e4. */
#define g_brewing_unknown_19 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002d9e4)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002d9e4. */
#define g_brewing_unknown_19_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d9e4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d9e8. */
#define g_brewing_unknown_20 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002d9e8)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002d9e8. */
#define g_brewing_unknown_20_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d9e8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d9ec. */
#define g_initialization_unknown_23 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002d9ec)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002d9f0. */
#define g_initialization_unknown_24 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002d9f0)))

/* 128-byte scratch buffer formatted as[%s][%s]. Address 0x2002da1c. */
#define g_log_prefix_buffer (*((uint8_t *)(uintptr_t)UINT32_C(0x2002da1c)))

/* Serialized as state in API query; 2 idle, 3 brew, 5 rack, 6 rinse, 8 Still, 9 descale in
 * application workflows. Address 0x2002da9c. */
#define g_device_state (*((uint8_t *)(uintptr_t)UINT32_C(0x2002da9c)))

/* Serialized as state in API query; 2 idle, 3 brew, 5 rack, 6 rinse, 8 Still, 9 descale in
 * application workflows. Alternate typed view of the same storage. Address 0x2002da9c. */
#define g_device_state_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002da9c)))

/* 32-character lowercase MD5 hex of6-byte WiFi MAC. Address 0x2002da9d. */
#define g_device_uid (*((uint8_t *)(uintptr_t)UINT32_C(0x2002da9d)))

/* 32-character lowercase MD5 hex of6-byte WiFi MAC. Alternate typed view of the same storage.
 * Address 0x2002da9d. */
#define g_device_uid_as_char (*((char *)(uintptr_t)UINT32_C(0x2002da9d)))

/* FinalNUL byte of device UID buffer. Address 0x2002dabd. */
#define g_device_uid_terminator (*((uint8_t *)(uintptr_t)UINT32_C(0x2002dabd)))

/* Set on controller start, cleared on stop. Address 0x2002dabe. */
#define g_heat_controller_enabled (*((uint8_t *)(uintptr_t)UINT32_C(0x2002dabe)))

/* Set on controller start, cleared on stop. Alternate typed view of the same storage. Address
 * 0x2002dabe. */
#define g_heat_controller_enabled_as_char (*((char *)(uintptr_t)UINT32_C(0x2002dabe)))

/* Buzzer on/off command wrappers skip repeated commands using this state byte. Address 0x2002dac0.
 */
#define g_buzzer_enabled (*((uint8_t *)(uintptr_t)UINT32_C(0x2002dac0)))

/* Buzzer on/off command wrappers skip repeated commands using this state byte. Alternate typed view
 * of the same storage. Address 0x2002dac0. */
#define g_buzzer_enabled_as_char (*((char *)(uintptr_t)UINT32_C(0x2002dac0)))

/* Stores stepper-board capability query response at 0x2000ccd8; response affects legacy compartment
 * mapping. Address 0x2002dac4. */
#define g_stepper_capabilities (*((uint32_t *)(uintptr_t)UINT32_C(0x2002dac4)))

/* Stores stepper-board capability query response at 0x2000ccd8; response affects legacy compartment
 * mapping. Alternate typed view of the same storage. Address 0x2002dac4. */
#define g_stepper_capabilities_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002dac4)))

/* IPAddress object populated by picobrew.com DNS lookup. Address 0x2002dac8. */
#define g_server_ip_address (*((uint8_t *)(uintptr_t)UINT32_C(0x2002dac8)))

/* Packed value of server IPAddress; compared against0.0.0.0. Address 0x2002dacc. */
#define g_server_ipv4_value (*((int *)(uintptr_t)UINT32_C(0x2002dacc)))

/* Packed value of server IPAddress; compared against0.0.0.0. Alternate typed view of the same
 * storage. Address 0x2002dacc. */
#define g_server_ipv4_value_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002dacc)))

/* SPI object begins with module byte0 and bit-order byte1; static initializer146bc. Address
 * 0x2002dad0. */
#define global_spi (*((uint8_t *)(uintptr_t)UINT32_C(0x2002dad0)))

/* SPI object second byte set1 for MSB first and used by transfer routine. Address 0x2002dad1. */
#define spi_bit_order (*((uint8_t *)(uintptr_t)UINT32_C(0x2002dad1)))

/* Four11-byte entries: presence flag,4-byte IP and6-byte MAC; WiFi register/unregister device
 * methods. Address 0x2002dad2. */
#define connected_device_table (*((uint8_t *)(uintptr_t)UINT32_C(0x2002dad2)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002dad3. */
#define g_wifi_unknown_01 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002dad3)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002dad7. */
#define g_wifi_unknown_02 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002dad7)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002dadb. */
#define g_wifi_unknown_03 (*((uint16_t *)(uintptr_t)UINT32_C(0x2002dadb)))

/* Flag set by WiFi begin and consumed by status method. Address 0x2002dafe. */
#define wifi_begin_called (*((char *)(uintptr_t)UINT32_C(0x2002dafe)))

/* Flag set by WiFi begin and consumed by status method. Alternate typed view of the same storage.
 * Address 0x2002dafe. */
#define wifi_begin_called_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002dafe)))

/* Flag checked/set by WiFi_initialize. Address 0x2002daff. */
#define wifi_initialized (*((char *)(uintptr_t)UINT32_C(0x2002daff)))

/* Array of8 WiFiClient objects of0x128 bytes, destruction/initialization loops. Address 0x2002db00.
 */
#define cached_wifi_clients (*((uint8_t *)(uintptr_t)UINT32_C(0x2002db00)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e318. */
#define g_wifi_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e318)))

/* 8-entry16-bit socket type table used by connect and send. Address 0x2002e440. */
#define wifi_socket_types (*((uint16_t *)(uintptr_t)UINT32_C(0x2002e440)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e450. */
#define g_wifi_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e450)))

/* 8-entry16-bit socket-associated port table; WiFiClient connect stores its destination port.
 * Address 0x2002e458. */
#define wifi_socket_ports (*((uint16_t *)(uintptr_t)UINT32_C(0x2002e458)))

/* WiFi global object initialized near150cc. Address 0x2002e468. */
#define global_wifi (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e468)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e469. */
#define g_wifi_unknown_06 (*((char *)(uintptr_t)UINT32_C(0x2002e469)))

/* WiFi scan_ssid copies network name into this buffer. Address 0x2002e46a. */
#define wifi_scan_ssid_buffer (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e46a)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e48c. */
#define g_simplelink_protocol_unknown_21 (*((int *)(uintptr_t)UINT32_C(0x2002e48c)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002e48c. */
#define g_simplelink_protocol_unknown_21_as_uint32_t                                               \
    (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e48c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e490. */
#define g_simplelink_protocol_unknown_22 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e490)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e491. */
#define g_simplelink_protocol_unknown_23 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e491)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e492. */
#define g_simplelink_protocol_unknown_24 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e492)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e493. */
#define g_simplelink_protocol_unknown_25 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e493)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e494. */
#define g_simplelink_protocol_unknown_26 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e494)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e495. */
#define g_simplelink_protocol_unknown_27 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e495)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e496. */
#define g_simplelink_protocol_unknown_28 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e496)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e4b8. */
#define g_wifi_unknown_07 (*((int *)(uintptr_t)UINT32_C(0x2002e4b8)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002e4b8. */
#define g_wifi_unknown_07_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e4b8)))

/* Index assigned by connected-device registration. Address 0x2002e4bc. */
#define wifi_last_connected_device (*((int *)(uintptr_t)UINT32_C(0x2002e4bc)))

/* 8-entry signed16-bit socket table; free entries are-1. Address 0x2002e4c0. */
#define wifi_socket_handles (*((uint16_t *)(uintptr_t)UINT32_C(0x2002e4c0)))

/* scanNetworks result count used for bounds checks. Address 0x2002e4d0. */
#define wifi_scan_result_count (*((int *)(uintptr_t)UINT32_C(0x2002e4d0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e4d4. */
#define g_wifi_unknown_08 (*((uint16_t *)(uintptr_t)UINT32_C(0x2002e4d4)))

/* SPI transport descriptor valid when flag1. Address 0x2002e4e4. */
#define nwp_spi_is_open (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e4e4)))

/* SPI transport descriptor valid when flag1. Alternate typed view of the same storage. Address
 * 0x2002e4e4. */
#define nwp_spi_is_open_as_int (*((int *)(uintptr_t)UINT32_C(0x2002e4e4)))

/* Transport selects DMA only when uDMA clock/status ready. Address 0x2002e4e8. */
#define nwp_spi_dma_enabled (*((char *)(uintptr_t)UINT32_C(0x2002e4e8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e4e9. */
#define g_simplelink_protocol_unknown_29 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e4e9)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002e4e9. */
#define g_simplelink_protocol_unknown_29_as_char (*((char *)(uintptr_t)UINT32_C(0x2002e4e9)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e4ec. */
#define g_simplelink_protocol_unknown_30 (*((uint8_t **)(uintptr_t)UINT32_C(0x2002e4ec)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002e4ec. */
#define g_simplelink_protocol_unknown_30_as_int (*((int *)(uintptr_t)UINT32_C(0x2002e4ec)))

/* Upstream g_pCB, points to the 188-byte driver control block at 0x20036b68. Aliases preserve the
 * original byte/word addressing views. Address 0x2002e4f0. */
#define g_simplelink_control_block (*((int **)(uintptr_t)UINT32_C(0x2002e4f0)))

/* Upstream g_pCB, points to the 188-byte driver control block at 0x20036b68. Aliases preserve the
 * original byte/word addressing views. Alternate typed view of the same storage. Address
 * 0x2002e4f0. */
#define g_simplelink_control_block_as_int (*((int *)(uintptr_t)UINT32_C(0x2002e4f0)))

/* Upstream g_pCB, points to the 188-byte driver control block at 0x20036b68. Aliases preserve the
 * original byte/word addressing views. Alternate typed view of the same storage. Address
 * 0x2002e4f0. */
#define g_simplelink_control_block_as_uint8_t_pointer                                              \
    (*((uint8_t **)(uintptr_t)UINT32_C(0x2002e4f0)))

/* Upstream g_pCB, points to the 188-byte driver control block at 0x20036b68. Aliases preserve the
 * original byte/word addressing views. Alternate typed view of the same storage. Address
 * 0x2002e4f0. */
#define g_simplelink_control_block_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e4f0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e4f4. */
#define g_filesystem_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e4f4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e4f5. */
#define g_filesystem_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e4f5)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e535. */
#define g_filesystem_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e535)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e578. */
#define g_filesystem_unknown_05 (*((uint8_t ***)(uintptr_t)UINT32_C(0x2002e578)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e57c. */
#define g_filesystem_unknown_06 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e57c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e580. */
#define g_filesystem_unknown_07 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e580)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e588. */
#define g_filesystem_unknown_08 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e588)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002e588. */
#define g_filesystem_unknown_08_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e588)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e589. */
#define g_filesystem_unknown_09 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e589)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002e589. */
#define g_filesystem_unknown_09_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e589)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e58a. */
#define g_filesystem_unknown_10 (*((char *)(uintptr_t)UINT32_C(0x2002e58a)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e58b. */
#define g_filesystem_unknown_11 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e58b)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x2002e58b. */
#define g_filesystem_unknown_11_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e58b)))

/* Global SLFS object used by file operations and static initializer182fc. Address 0x2002e58c. */
#define global_serial_flash (*((uint8_t ***)(uintptr_t)UINT32_C(0x2002e58c)))

/* Global SLFS object used by file operations and static initializer182fc. Alternate typed view of
 * the same storage. Address 0x2002e58c. */
#define global_serial_flash_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e58c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e590. */
#define g_serial_flash_unknown_02 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e590)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e594. */
#define g_serial_flash_unknown_03 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e594)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e59c. */
#define g_serial_flash_unknown_04 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e59c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e5a0. */
#define g_serial_flash_unknown_05 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e5a0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e5a4. */
#define g_serial_flash_unknown_06 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e5a4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e5a8. */
#define g_serial_flash_unknown_07 (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e5a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2002e5ac. */
#define g_serial_flash_unknown_08 (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e5ac)))

/* First byte of8-byte boot record; toggles1/2 for application images or0 for factory. Address
 * 0x2002e5b0. */
#define boot_info_selected_image (*((uint8_t *)(uintptr_t)UINT32_C(0x2002e5b0)))

/* First byte of8-byte boot record; toggles1/2 for application images or0 for factory. Alternate
 * typed view of the same storage. Address 0x2002e5b0. */
#define boot_info_selected_image_as_char (*((char *)(uintptr_t)UINT32_C(0x2002e5b0)))

/* Written0xabcddcba before updating boot-info file. Address 0x2002e5b4. */
#define boot_info_magic (*((uint32_t *)(uintptr_t)UINT32_C(0x2002e5b4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200365b8. */
#define g_initialization_unknown_25 (*((uint8_t *)(uintptr_t)UINT32_C(0x200365b8)))

/* Current heap-end pointer updated by _sbrk. Address 0x200365bc. */
#define heap_break (*((uint8_t **)(uintptr_t)UINT32_C(0x200365bc)))

/* GPIO port0 per-pin interrupt callbacks. Address 0x200365c0. */
#define gpio_port_a0_callbacks (*((uint32_t *)(uintptr_t)UINT32_C(0x200365c0)))

/* 8 callback pointers dispatched by SysTick. Address 0x20036640. */
#define systick_callbacks (*((uint32_t *)(uintptr_t)UINT32_C(0x20036640)))

/* Counter incremented by SysTick and read by millis. Address 0x20036660. */
#define elapsed_milliseconds (*((uint32_t *)(uintptr_t)UINT32_C(0x20036660)))

/* Counter incremented by SysTick and read by millis. Alternate typed view of the same storage.
 * Address 0x20036660. */
#define elapsed_milliseconds_as_int (*((int *)(uintptr_t)UINT32_C(0x20036660)))

/* First HardwareSerial object, wired to UARTIntHandler. Address 0x20036664. */
#define global_serial0 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036664)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036778. */
#define g_hardware_serial_unknown_08 (*((int *)(uintptr_t)UINT32_C(0x20036778)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2003677c. */
#define g_hardware_serial_unknown_09 (*((int *)(uintptr_t)UINT32_C(0x2003677c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036780. */
#define g_hardware_serial_unknown_10 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036780)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036880. */
#define g_hardware_serial_unknown_11 (*((uint32_t *)(uintptr_t)UINT32_C(0x20036880)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036884. */
#define g_hardware_serial_unknown_12 (*((int *)(uintptr_t)UINT32_C(0x20036884)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036888. */
#define g_hardware_serial_unknown_13 (*((int *)(uintptr_t)UINT32_C(0x20036888)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2003688c. */
#define g_hardware_serial_unknown_14 (*((int *)(uintptr_t)UINT32_C(0x2003688c)))

/* Second HardwareSerial object, wired to UARTIntHandler1. Address 0x20036894. */
#define global_serial1 (*((uint8_t ***)(uintptr_t)UINT32_C(0x20036894)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036898. */
#define g_hardware_serial_unknown_15 (*((uint32_t *)(uintptr_t)UINT32_C(0x20036898)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2003689c. */
#define g_hardware_serial_unknown_16 (*((uint32_t *)(uintptr_t)UINT32_C(0x2003689c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200369a4. */
#define g_hardware_serial_unknown_17 (*((uint32_t *)(uintptr_t)UINT32_C(0x200369a4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200369a8. */
#define g_hardware_serial_unknown_18 (*((int *)(uintptr_t)UINT32_C(0x200369a8)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x200369a8. */
#define g_hardware_serial_unknown_18_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x200369a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x200369ac. */
#define g_hardware_serial_unknown_19 (*((int *)(uintptr_t)UINT32_C(0x200369ac)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x200369ac. */
#define g_hardware_serial_unknown_19_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x200369ac)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ab0. */
#define g_hardware_serial_unknown_20 (*((uint32_t *)(uintptr_t)UINT32_C(0x20036ab0)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x20036ab0. */
#define g_hardware_serial_unknown_20_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x20036ab0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ab4. */
#define g_hardware_serial_unknown_21 (*((int *)(uintptr_t)UINT32_C(0x20036ab4)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x20036ab4. */
#define g_hardware_serial_unknown_21_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x20036ab4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ab8. */
#define g_hardware_serial_unknown_22 (*((int *)(uintptr_t)UINT32_C(0x20036ab8)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x20036ab8. */
#define g_hardware_serial_unknown_22_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x20036ab8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036abc. */
#define g_hardware_serial_unknown_23 (*((int *)(uintptr_t)UINT32_C(0x20036abc)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x20036abc. */
#define g_hardware_serial_unknown_23_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x20036abc)))

/* IPAddress object constructed as zero address. Address 0x20036ac4. */
#define global_any_ip_address (*((uint8_t ***)(uintptr_t)UINT32_C(0x20036ac4)))

/* Four zero address bytes after IPAddress vtable. Address 0x20036ac8. */
#define any_ip_address_value (*((int *)(uintptr_t)UINT32_C(0x20036ac8)))

/* Four zero address bytes after IPAddress vtable. Alternate typed view of the same storage. Address
 * 0x20036ac8. */
#define any_ip_address_value_as_uint32_t (*((uint32_t *)(uintptr_t)UINT32_C(0x20036ac8)))

/* Four zero address bytes after IPAddress vtable. Alternate typed view of the same storage. Address
 * 0x20036ac8. */
#define any_ip_address_value_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x20036ac8)))

/* MACAddress object constructed as six zero octets. Address 0x20036acc. */
#define global_any_mac_address (*((uint8_t ***)(uintptr_t)UINT32_C(0x20036acc)))

/* Six zero MAC address bytes after MACAddress vtable. Address 0x20036ad0. */
#define any_mac_address_bytes (*((uint8_t *)(uintptr_t)UINT32_C(0x20036ad0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ad1. */
#define g_network_addresses_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036ad1)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ad2. */
#define g_network_addresses_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036ad2)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ad3. */
#define g_network_addresses_unknown_05 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036ad3)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ad4. */
#define g_network_addresses_unknown_06 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036ad4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ad5. */
#define g_network_addresses_unknown_07 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036ad5)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ad8. */
#define g_c_library_unknown_24 (*((uint8_t **)(uintptr_t)UINT32_C(0x20036ad8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036adc. */
#define g_c_library_unknown_25 (*((uint8_t **)(uintptr_t)UINT32_C(0x20036adc)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ae0. */
#define g_c_library_unknown_26 (*((int *)(uintptr_t)UINT32_C(0x20036ae0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ae4. */
#define g_c_library_unknown_27 (*((uint8_t **)(uintptr_t)UINT32_C(0x20036ae4)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x20036ae4. */
#define g_c_library_unknown_27_as_int (*((int *)(uintptr_t)UINT32_C(0x20036ae4)))

/* Volatile DMA completion flag set by SPI ISR and polled by read/write. Address 0x20036b0c. */
#define nwp_spi_transfer_complete (*((uint8_t *)(uintptr_t)UINT32_C(0x20036b0c)))

/* Volatile DMA completion flag set by SPI ISR and polled by read/write. Alternate typed view of the
 * same storage. Address 0x20036b0c. */
#define nwp_spi_transfer_complete_as_char (*((char *)(uintptr_t)UINT32_C(0x20036b0c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036b10. */
#define g_simplelink_spi_transport_unknown_02 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036b10)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036b24. */
#define g_simplelink_spi_transport_unknown_03 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036b24)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036b4c. */
#define g_simplelink_spi_transport_unknown_04 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036b4c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036b60. */
#define g_simplelink_protocol_unknown_31 (*((char *)(uintptr_t)UINT32_C(0x20036b60)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x20036b60. */
#define g_simplelink_protocol_unknown_31_as_uint8_t (*((uint8_t *)(uintptr_t)UINT32_C(0x20036b60)))

/* Type inferred from use; semantic identity remains incomplete. Alternate typed view of the same
 * storage. Address 0x20036b60. */
#define g_simplelink_protocol_unknown_31_as_uint8_t_2                                              \
    (*((uint8_t *)(uintptr_t)UINT32_C(0x20036b60)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036b68. */
#define g_simplelink_protocol_unknown_32 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036b68)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036c24. */
#define g_simplelink_protocol_unknown_33 (*((uint32_t *)(uintptr_t)UINT32_C(0x20036c24)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036c28. */
#define g_simplelink_protocol_unknown_34 (*((uint32_t *)(uintptr_t)UINT32_C(0x20036c28)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036c2c. */
#define g_simplelink_protocol_unknown_35 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036c2c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036c2e. */
#define g_simplelink_protocol_unknown_36 (*((uint8_t *)(uintptr_t)UINT32_C(0x20036c2e)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036ccc. */
#define g_simplelink_protocol_unknown_37 (*((uint32_t *)(uintptr_t)UINT32_C(0x20036ccc)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20036cd0. */
#define g_simplelink_protocol_unknown_38 (*((uint32_t *)(uintptr_t)UINT32_C(0x20036cd0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20037000. */
#define g_simplelink_protocol_unknown_39 (*((uint32_t *)(uintptr_t)UINT32_C(0x20037000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20037400. */
#define g_simplelink_protocol_unknown_40 (*((uint8_t *)(uintptr_t)UINT32_C(0x20037400)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20037800. */
#define g_c_library_unknown_28 (*((int *)(uintptr_t)UINT32_C(0x20037800)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x20037804. */
#define _ebss_heap_start (*((uint8_t *)(uintptr_t)UINT32_C(0x20037804)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x2003f808. */
#define g_startup_unknown_01 (*((uint32_t *)(uintptr_t)UINT32_C(0x2003f808)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x40004000. */
#define mmio_wiring_gpio_unknown_01 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x40004000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x40005000. */
#define mmio_wiring_gpio_unknown_02 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x40005000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x40006000. */
#define mmio_wiring_gpio_unknown_03 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x40006000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x40007000. */
#define mmio_wiring_gpio_unknown_04 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x40007000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x40030000. */
#define mmio_cc3200_timer_unknown_01 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x40030000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x40031000. */
#define mmio_cc3200_timer_unknown_02 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x40031000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x40032000. */
#define mmio_cc3200_timer_unknown_03 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x40032000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400f7088. */
#define mmio_simplelink_spi_transport_unknown_05                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400f7088)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400f708c. */
#define mmio_simplelink_spi_transport_unknown_06                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400f708c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400f70b0. */
#define mmio_simplelink_spi_transport_unknown_07                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400f70b0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400f70b8. */
#define mmio_simplelink_spi_transport_unknown_08                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400f70b8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400f70c0. */
#define mmio_simplelink_spi_transport_unknown_09                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400f70c0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff004. */
#define mmio_cc3200_dma_unknown_01 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff004)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff008. */
#define mmio_cc3200_dma_unknown_02 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff008)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff018. */
#define mmio_cc3200_dma_unknown_03 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff018)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff01c. */
#define mmio_cc3200_dma_unknown_04 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff01c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff020. */
#define mmio_cc3200_dma_unknown_05 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff020)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff024. */
#define mmio_cc3200_dma_unknown_06 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff024)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff028. */
#define mmio_cc3200_dma_unknown_07 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff028)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff030. */
#define mmio_cc3200_dma_unknown_08 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff030)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff034. */
#define mmio_cc3200_dma_unknown_09 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff034)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff038. */
#define mmio_cc3200_dma_unknown_10 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff038)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff03c. */
#define mmio_cc3200_dma_unknown_11 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff03c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x400ff504. */
#define mmio_cc3200_dma_unknown_12 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x400ff504)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44020000. */
#define mmio_cc3200_spi_unknown_03 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x44020000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44021000. */
#define mmio_cc3200_spi_unknown_04 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x44021000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402112c. */
#define mmio_spi_bus_unknown_05 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402112c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44022000. */
#define mmio_cc3200_spi_unknown_05 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x44022000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44022130. */
#define mmio_simplelink_spi_transport_unknown_10                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44022130)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44022138. */
#define mmio_simplelink_spi_transport_unknown_11                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44022138)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402213c. */
#define mmio_simplelink_spi_transport_unknown_12                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402213c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44025000. */
#define mmio_cc3200_clock_unknown_03 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44025000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44025020. */
#define mmio_cc3200_clock_unknown_04 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44025020)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44025118. */
#define mmio_simplelink_spi_transport_unknown_13                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44025118)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44026094. */
#define mmio_cc3200_spi_unknown_06 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44026094)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402609c. */
#define mmio_cc3200_spi_unknown_07 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402609c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x440260b0. */
#define mmio_cc3200_timer_unknown_04 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x440260b0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402d000. */
#define mmio_cc3200_clock_unknown_05 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402d000)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402d00c. */
#define mmio_cc3200_clock_unknown_06 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402d00c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402d404. */
#define mmio_simplelink_spi_transport_unknown_14                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402d404)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402dc78. */
#define mmio_cc3200_clock_unknown_07 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402dc78)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402e0a0. */
#define mmio_cc3200_gpio_unknown_02 (*((volatile uint8_t *)(uintptr_t)UINT32_C(0x4402e0a0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402e0a4. */
#define mmio_cc3200_clock_unknown_08 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402e0a4)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402e0a8. */
#define mmio_cc3200_clock_unknown_09 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402e0a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402e110. */
#define mmio_cc3200_clock_unknown_10 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402e110)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402e114. */
#define mmio_cc3200_clock_unknown_11 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402e114)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402e144. */
#define mmio_cc3200_gpio_unknown_03 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402e144)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402e16c. */
#define mmio_simplelink_spi_transport_unknown_15                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402e16c)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402e184. */
#define mmio_cc3200_clock_unknown_12 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402e184)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402f024. */
#define mmio_simplelink_spi_transport_unknown_16                                                   \
    (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402f024)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402f064. */
#define mmio_cc3200_clock_unknown_13 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402f064)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402f0a8. */
#define mmio_cc3200_clock_unknown_14 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402f0a8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402f0b0. */
#define mmio_cc3200_clock_unknown_15 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402f0b0)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402f0c8. */
#define mmio_cc3200_clock_unknown_16 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402f0c8)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402f804. */
#define mmio_cc3200_clock_unknown_17 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402f804)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402fc18. */
#define mmio_cc3200_clock_unknown_18 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402fc18)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402fc68. */
#define mmio_cc3200_clock_unknown_19 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402fc68)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x4402fc74. */
#define mmio_cc3200_clock_unknown_20 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x4402fc74)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44030810. */
#define mmio_cc3200_hash_unknown_01 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44030810)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44030814. */
#define mmio_cc3200_hash_unknown_02 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44030814)))

/* Type inferred from use; semantic identity remains incomplete. Address 0x44030818. */
#define mmio_cc3200_hash_unknown_03 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0x44030818)))

/* Type inferred from use; semantic identity remains incomplete. Address 0xe000e010. */
#define mmio_cc3200_interrupts_unknown_06 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0xe000e010)))

/* Type inferred from use; semantic identity remains incomplete. Address 0xe000e014. */
#define mmio_cc3200_timer_unknown_05 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0xe000e014)))

/* Type inferred from use; semantic identity remains incomplete. Address 0xe000e018. */
#define mmio_energia_runtime_unknown_01 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0xe000e018)))

/* Type inferred from use; semantic identity remains incomplete. Address 0xe000ed04. */
#define mmio_cc3200_interrupts_unknown_07 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0xe000ed04)))

/* Type inferred from use; semantic identity remains incomplete. Address 0xe000ed08. */
#define mmio_cc3200_interrupts_unknown_08 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0xe000ed08)))

/* Type inferred from use; semantic identity remains incomplete. Address 0xe000ed24. */
#define mmio_cc3200_interrupts_unknown_09 (*((volatile uint32_t *)(uintptr_t)UINT32_C(0xe000ed24)))

#endif
