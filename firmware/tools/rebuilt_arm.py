"""Execute linked application functions from the newly built ARM ELF.

The ELF and original firmware run in separate emulator instances. A rebuilt
function cannot accidentally call into the reference image. No peripherals
are simulated: tests supply explicit hooks at the documented boundaries.
"""
from pathlib import Path

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE

from original_arm import ArmFirmware


class RebuiltArm(ArmFirmware):
    def __init__(self, elf_path):
        self.uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        self.uc.mem_map(0x20000000, 0x100000)
        self.uc.mem_write(self.RETURN, b"\x00\xbe")
        self.hooks = {}
        self.trace = []
        self.instructions = 0
        self.returned = False
        self.symbols = {}
        self.elf_path = Path(elf_path)
        with self.elf_path.open('rb') as handle:
            elf = ELFFile(handle)
            if elf['e_machine'] != 'EM_ARM' or not elf.little_endian:
                raise ValueError('Expected a little-endian ARM ELF')
            for segment in elf.iter_segments():
                if segment['p_type'] != 'PT_LOAD':
                    continue
                address, length = segment['p_vaddr'], segment['p_memsz']
                if not 0x20000000 <= address <= address + length <= 0x20100000:
                    raise ValueError('ELF segment outside the test RAM mapping')
                self.uc.mem_write(address, segment.data())
            table = elf.get_section_by_name('.symtab')
            if table is None:
                raise ValueError('The test ELF must retain its symbol table')
            for symbol in table.iter_symbols():
                if symbol.name and symbol['st_shndx'] != 'SHN_UNDEF':
                    self.symbols[symbol.name] = symbol['st_value']
        self.uc.hook_add(UC_HOOK_CODE, self._instruction)

    def address(self, symbol):
        try:
            return self.symbols[symbol]
        except KeyError as error:
            raise ValueError('Missing ELF symbol: ' + symbol) from error

    def call_symbol(self, symbol, args=(), **options):
        return self.call(self.address(symbol) & ~1, args=args, **options)

    def hook_symbol(self, symbol, callback):
        self.hook(self.address(symbol) & ~1, callback)
