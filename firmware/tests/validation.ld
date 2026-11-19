/* Function-test address space only. This is NOT a recovered Pico boot linker
 * configuration and this ELF is not intended for device installation.
 */
ENTRY(pico_validation_entry)
/* Fixture storage is emulator-only; production has a separate 240 KiB limit. */
MEMORY { RAM (rwx) : ORIGIN = 0x20004000, LENGTH = 0x70000 }
SECTIONS
{
    .text : { *(.text .text.*) *(.rodata .rodata.*) } > RAM
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > RAM
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > RAM
    .data : { *(.data .data.*) } > RAM
    .bss (NOLOAD) : {
        . = ALIGN(8);
        __bss_start__ = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(8);
        __bss_end__ = .;
    } > RAM
    . = ALIGN(8);
    end = .;
    _end = .;
    /DISCARD/ : { *(.comment) *(.note*) }
}
