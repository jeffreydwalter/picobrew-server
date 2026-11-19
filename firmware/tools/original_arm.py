"""Execute bounded routines from the original Pico Pro image under Unicorn.

This is a function-level analysis harness, not a simulation of the appliance.
RAM and caller registers are explicit. Hardware callees may be hooked by tests.
"""
from pathlib import Path
import hashlib
import struct
import sys

LOCAL_DEPS = Path(__file__).resolve().parent / "python"
if LOCAL_DEPS.is_dir():
    sys.path.insert(0, str(LOCAL_DEPS))

from unicorn import Uc, UcError, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import (
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
    UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11,
    UC_ARM_REG_R12, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC,
    UC_ARM_REG_XPSR,
)

IMAGE_BASE = 0x20004000
IMAGE_SHA256 = "e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702"
DEFAULT_IMAGE = (Path(__file__).resolve().parents[1] /
    "reference/original-0.1.34.bin")
REGISTERS = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
             UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
             UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11,
             UC_ARM_REG_R12, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC]
REGISTER_NAMES = ["r" + str(i) for i in range(13)] + ["sp", "lr", "pc"]


class ArmFirmware:
    """Little-endian Cortex-M Thumb execution with original soft-float ABI."""

    SCRATCH = 0x20080000
    STACK = 0x200F0000
    RETURN = 0x200FF000

    def __init__(self, image=DEFAULT_IMAGE, verify=True):
        image = Path(image).read_bytes()
        if verify and hashlib.sha256(image).hexdigest() != IMAGE_SHA256:
            raise ValueError("The firmware image does not match the analyzed payload")
        self.uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        self.uc.mem_map(0x20000000, 0x100000)
        self.uc.mem_write(IMAGE_BASE, image)
        self.uc.mem_write(self.RETURN, b"\x00\xbe")
        self.hooks = {}
        self.trace = []
        self.instructions = 0
        self.returned = False
        self.uc.hook_add(UC_HOOK_CODE, self._instruction)

    def _instruction(self, uc, address, size, user_data):
        self.instructions += 1
        if address == self.RETURN:
            self.returned = True
            uc.emu_stop()
            return
        callback = self.hooks.get(address)
        if callback is not None:
            self.trace.append({"address": address, "args": [self.reg(i) for i in range(4)]})
            result = callback(self)
            if isinstance(result, int):
                self.set_reg(0, result)
            elif isinstance(result, (tuple, list)):
                for index, value in enumerate(result):
                    self.set_reg(index, value)
            self.set_reg("pc", self.reg("lr"))

    def hook(self, address, callback):
        self.hooks[address & ~1] = callback

    def unhook(self, address):
        self.hooks.pop(address & ~1, None)

    def reg(self, index):
        if isinstance(index, str):
            if index.lower() in ("xpsr", "apsr", "cpsr"):
                return self.uc.reg_read(UC_ARM_REG_XPSR)
            index = REGISTER_NAMES.index(index.lower())
        return self.uc.reg_read(REGISTERS[index])

    def set_reg(self, index, value):
        if isinstance(index, str):
            if index.lower() in ("xpsr", "apsr", "cpsr"):
                self.uc.reg_write(UC_ARM_REG_XPSR, value & 0xFFFFFFFF)
                return
            index = REGISTER_NAMES.index(index.lower())
        self.uc.reg_write(REGISTERS[index], value & 0xFFFFFFFF)

    def read(self, address, count):
        return bytes(self.uc.mem_read(address, count))

    def write(self, address, data):
        self.uc.mem_write(address, bytes(data))

    def read_u32(self, address):
        return struct.unpack("<I", self.read(address, 4))[0]

    def write_u32(self, address, value):
        self.write(address, struct.pack("<I", value & 0xFFFFFFFF))

    def cstring(self, address, limit=4096):
        result = bytearray()
        for offset in range(limit):
            byte = self.read(address + offset, 1)
            if byte == b"\0":
                return bytes(result)
            result.extend(byte)
        raise ValueError("No terminator in bounded string read")

    def call(self, address, args=(), stack_args=(), registers=None, max_instructions=1000000):
        self.trace = []
        self.instructions = 0
        self.returned = False
        for index in range(13):
            self.set_reg(index, 0)
        self.set_reg("sp", self.STACK)
        self.set_reg("lr", self.RETURN | 1)
        self.set_reg("xpsr", 1 << 24)
        if len(args) > 4:
            raise ValueError("Use stack_args for arguments beyond r0-r3")
        for index, value in enumerate(args):
            self.set_reg(index, value)
        for index, value in enumerate(stack_args):
            self.write_u32(self.STACK + 4 * index, value)
        for index, value in (registers or {}).items():
            self.set_reg(index, value)
        try:
            self.uc.emu_start(address | 1, self.RETURN, count=max_instructions)
        except UcError as error:
            raise RuntimeError(f"ARM execution failed at 0x{self.reg('pc'):08x}: {error}") from error
        self.returned = self.returned or (self.reg("pc") & ~1) == self.RETURN
        if not self.returned:
            raise RuntimeError(f"Routine did not return after {self.instructions} instructions; "
                               f"PC=0x{self.reg('pc'):08x}")
        result = {name: self.reg(name) for name in REGISTER_NAMES}
        result["xpsr"] = self.reg("xpsr")
        result["instructions"] = self.instructions
        return result


def words_from_double(value):
    return struct.unpack("<II", struct.pack("<d", value))


def double_from_words(low, high):
    return struct.unpack("<d", struct.pack("<II", low, high))[0]


if __name__ == "__main__":
    firmware = ArmFirmware()
    # __aeabi_i2d: original soft-float ABI returns the two double words in r0/r1.
    result = firmware.call(0x20020DF0, [123])
    value = double_from_words(result["r0"], result["r1"])
    assert value == 123.0, value
    print(f"ARM harness smoke check: signed 123 -> {value}; "
          f"{result['instructions']} original instructions")
