/* Pico Pro's observed CC3200 application starts above the ROM bootloader's
 * 16 KiB reserved area. The reference image uses the 256 KiB RAM variant.
 * Original heap reservation is 32 KiB; Energia startup provides a 32 KiB stack.
 * This layout keeps the native SDK reset/constructor/heap symbol contract. */
HEAP_SIZE = 0x8000;
MEMORY { SRAM (rwx) : ORIGIN = 0x20004000, LENGTH = 0x3c000 }
/* Keep ELF metadata outside the loadable appliance image. */
PHDRS { firmware PT_LOAD FLAGS(7); }
SECTIONS
{
    .text : {
        _text = .;
        KEEP(*(.intvecs))
        *(.text*) *(.rodata*)
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4); KEEP(*(.init))
        . = ALIGN(4); __preinit_array_start = .;
        KEEP(*(SORT(.preinit_array*))) KEEP(*(.preinit_array))
        __preinit_array_end = .;
        . = ALIGN(4); __init_array_start = .;
        KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array))
        __init_array_end = .;
        . = ALIGN(4); KEEP(*(.fini))
        . = ALIGN(4); __fini_array_start = .;
        KEEP(*(.fini_array)) KEEP(*(SORT(.fini_array.*)))
        __fini_array_end = .;
        . = ALIGN(8); _etext = .;
    } > SRAM :firmware
    .ARM : {
        __exidx_start = .; *(.ARM.exidx*) __exidx_end = .;
    } > SRAM
    __init_data = .;
    .data : AT(__init_data) {
        _data = .; *(.data*) . = ALIGN(8); _edata = .;
    } > SRAM
    .bss (NOLOAD) : {
        . = ALIGN(8); _bss = .;
        /* The SDK DMA table requires 1024-byte alignment. Put it at the
           already-aligned section start instead of padding after application
           BSS; its size and address alignment remain unchanged. */
        *(.bss.gpCtlTbl)
        /* SDK declares uint32_t[8192], whose C alignment is only four. AAPCS
           requires eight-byte alignment at the reset call boundary. */
        *(.bss.pui32Stack)
        *(.bss*) *(COMMON) . = ALIGN(8); _ebss = .;
    } > SRAM
    .heap (NOLOAD) : {
        _heap = .; . += HEAP_SIZE; . = ALIGN(8); _eheap = .;
    } > SRAM
    ASSERT(_text == 0x20004000, "Pico vector table must begin at 0x20004000")
    ASSERT((_heap & 7) == 0, "Pico heap must be 8-byte aligned")
    ASSERT(_eheap <= 0x20040000, "Pico firmware exceeds 256 KiB device RAM")
}
