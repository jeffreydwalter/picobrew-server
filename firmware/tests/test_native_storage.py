#!/usr/bin/env python3
"""Exercise file-open recovery through the production ARM bridge and SDK.

The actual linked SLFS and SimpleLink filesystem functions execute. Only the
SimpleLink command transport is replaced with deterministic filesystem replies;
no appliance, flash programming, or physical power-loss behavior is tested.
"""
import argparse
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from rebuilt_arm import RebuiltArm


class FileBoundary:
    def __init__(self, elf):
        self.machine = RebuiltArm(elf)
        self.open_error = 0
        self.info_error = 0
        self.events = []
        self.content = bytearray()
        self.active = False
        self.machine.hook_symbol("_SlDrvCmdOp", self.command)
        # The filehandle, length, offset, and is_write fields have their
        # constructor-defined zero values in the fresh ELF's BSS. The tested
        # nonvirtual methods do not require other appliance constructors.
        self.name = self.machine.SCRATCH
        self.data = self.name + 128
        self.machine.write(self.name, b"/local/settings.a\0")

    def command(self, m):
        # Pinned SDK driver.h/protocol.h structures under the target's ARM ABI:
        # _SlCmdCtrl_t.Opcode is uint16; _SlCmdExt_t payload pointers are at 8/12.
        opcode = struct.unpack("<H", m.read(m.reg(0), 2))[0]
        message, extension = m.reg(1), m.reg(2)
        if opcode == 0xA43C:  # FILEOPEN
            mode, token = struct.unpack("<II", m.read(message, 8))
            filename = m.cstring(m.read_u32(extension + 8))
            assert token == 0 and not self.active
            error, self.open_error = self.open_error, 0
            self.events.append(("open", filename, mode, error))
            self.active = error == 0
            m.write(message, struct.pack("<iI", error or 47, 0))
        elif opcode == 0xA442:  # FILEGETINFO
            error, self.info_error = self.info_error, 0
            self.events.append(("info", error))
            m.write(message, struct.pack("<hH6I", error, 0,
                                         len(self.content), 512, 0, 0, 0, 0))
        elif opcode == 0xA43D:  # FILECLOSE
            handle = struct.unpack("<i", m.read(message, 4))[0]
            self.events.append(("close", handle))
            if handle == 47:
                assert self.active
                self.active = False
            else:
                assert handle < 0 and not self.active
            m.write(message, struct.pack("<hH", 0 if handle == 47 else -5, 0))
        elif opcode == 0xA441:  # FILEWRITE
            handle, offset, length, _ = struct.unpack("<IIHH", m.read(message, 12))
            assert handle == 47 and self.active
            data = m.read(m.read_u32(extension + 8), length)
            self.events.append(("write", offset, data))
            self.content[offset:offset + length] = data
            m.write(message, struct.pack("<hH", length, 0))
        elif opcode == 0xA440:  # FILEREAD
            handle, offset, length, _ = struct.unpack("<IIHH", m.read(message, 12))
            assert handle == 47 and self.active
            data = bytes(self.content[offset:offset + length])
            self.events.append(("read", offset, data))
            m.write(m.read_u32(extension + 12), data)
            m.write(message, struct.pack("<hH", len(data), 0))
        else:
            raise AssertionError(f"Unexpected SimpleLink command: {opcode:#x}")
        return 0

    def open(self, mode=0x3001):
        return self.machine.call_symbol("_ZL9file_openPvPKcm", [0, self.name, mode])["r0"]

    def write(self, data):
        self.machine.write(self.data, data)
        return self.machine.call_symbol("_ZL10file_writePvPKhj",
                                        [0, self.data, len(data)])["r0"]

    def close(self):
        return self.machine.call_symbol("_ZL10file_closePv", [0])["r0"]

    def read(self):
        return self.machine.call_symbol("_ZL9file_readPv", [0])["r0"]

    def write_and_readback(self):
        assert self.open() == 1, "A failed open must not wedge later file operations"
        data = b"storage retry succeeds"
        assert self.write(data) == len(data)
        assert self.close() == 1 and not self.active
        assert self.open(0) == 1
        assert bytes(self.read() for _ in data) == data
        assert self.read() == 0xFFFFFFFF
        assert self.close() == 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path,
                        default=ROOT / "build/standalone/firmware/pico-s-pro-standalone.elf")
    args = parser.parse_args()
    cases = 0
    for error in (-11, -30, -34):  # Missing file, exhausted blocks, exhausted file entries.
        b = FileBoundary(args.elf)
        b.open_error = error
        assert b.open() == 0
        b.write_and_readback()
        cases += 1
    b = FileBoundary(args.elf)
    b.info_error = -5
    assert b.open() == 0 and not b.active
    b.write_and_readback()
    cases += 1
    b = FileBoundary(args.elf)
    assert b.open() == 1
    before = list(b.events)
    assert b.open() == 0
    assert b.events == before and b.active, "A rejected second open must preserve the first file"
    assert b.write(b"kept open") == 9
    assert b.close() == 1
    b.write_and_readback()
    cases += 1
    print(f"PASS: {cases} native storage scenarios; failed opens recover, existing opens survive; "
          "actual ARM bridge/SLFS/SimpleLink execute with simulated command transport.")


if __name__ == "__main__":
    main()
