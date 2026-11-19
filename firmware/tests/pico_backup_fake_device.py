"""Independent CC3200 ROM/APPS/read-helper emulator for backup regressions.

All flash bytes are deterministic synthetic data, not an appliance dump. The
helper supplied by tests comes from the separately installed pinned package.
"""
import struct


ACK = b"\x00\xcc"
NACK = b"\x00\x33"


class FakePicoUSB:
    """Return at most one 32-byte USB packet while preserving surplus bytes."""

    def __init__(self, helper, flash_size=1024 * 1024, fault=None):
        self.helper = helper
        self.flash = bytes((i * 17 + i // 256) & 255 for i in range(flash_size))
        self.fault = fault
        self.rx = bytearray()
        self.trace = []
        self.uploaded = bytearray()
        self.mode = "rom"
        self.helper_running = False
        self._break = False
        self.break_count = 0
        self.read_count = 0
        self.read_bytes = 0
        self.raw_reads = []
        self.host_acks = 0
        self.closed = False
        self.description = "simulated CH340, 32-byte endpoint packets"
        self.details = {"stage": "ready", "cleanup_errors": [], "simulation": True}

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        self.closed = True
        self._break = False
        return False

    @property
    def break_condition(self):
        return self._break

    @break_condition.setter
    def break_condition(self, value):
        self._break = bool(value)
        if value:
            self.break_count += 1
            if self.fault != "silent_initial":
                self.rx.extend(ACK)

    def reset_input_buffer(self):
        self.rx.clear()

    def read(self, count=1):
        self.read_count += 1
        length = min(count, 32, len(self.rx))
        result = bytes(self.rx[:length])
        del self.rx[:length]
        return result

    def _reply(self, payload, fault=None):
        checksum = sum(payload) & 255
        if fault == "checksum":
            checksum ^= 1
        header = struct.pack(">HB", len(payload) + 2, checksum)
        if fault == "short":
            payload = payload[:len(payload) // 2]
        self.rx.extend(header + payload)

    def write(self, packet):
        assert not self._break, "Host sent a command with BREAK asserted"
        packet = bytes(packet)
        if packet == ACK:
            self.host_acks += 1
            return len(packet)
        assert len(packet) >= 4, "Command too short"
        expected_length, checksum = struct.unpack(">HB", packet[:3])
        assert expected_length == len(packet) - 1, "Bad command length"
        payload = packet[3:]
        assert checksum == sum(payload) & 255, "Bad command checksum"
        opcode = payload[0]
        self.trace.append(payload)
        if self.fault == "nack" and opcode == 0x2F:
            self.rx.extend(NACK)
            return len(packet)
        self.rx.extend(ACK)

        if opcode == 0x2F:
            assert len(payload) == 1
            version = bytearray(28)
            version[:4] = bytes((0, 4, 1 if self.mode == "rom" else 0, 2))
            version[16] = 0x10
            self._reply(version)
        elif opcode == 0x33:
            assert payload == b"\x33" + struct.pack(">I", 26666667)
            assert self.mode == "rom"
            self.mode = "apps"
        elif opcode == 0x27:
            assert len(payload) == 1
            self.rx.append(0x84 if self.helper_running else 0x80)
        elif opcode == 0x2D:
            assert self.mode == "apps"
            assert not self.helper_running
            storage, offset, length = struct.unpack(">III", payload[1:13])
            data = payload[13:]
            assert storage == 0, "Persistent flash write reached device"
            assert offset == len(self.uploaded), "Nonsequential helper upload"
            assert length == len(data)
            assert self.helper[offset:offset + length] == data
            self.uploaded.extend(data)
        elif opcode == 0x32:
            assert len(payload) == 1
            assert self.uploaded == self.helper
            assert not self.helper_running
            self.helper_running = True
            self.rx.extend(ACK)  # Separate startup ACK after command ACK.
        elif opcode == 0x31:
            assert self.helper_running
            assert payload == b"\x31" + struct.pack(">I", 2)
            self._reply(struct.pack(">HH", 4096, len(self.flash) // 4096) + b"\x00" * 4)
        elif opcode == 0x2C:
            assert self.helper_running
            storage, offset, length = struct.unpack(">III", payload[1:13])
            assert storage == 2
            assert 0 < length <= 4096
            assert offset + length <= len(self.flash)
            self.raw_reads.append((offset, length))
            data = self.flash[offset:offset + length]
            second_pass = self.read_bytes >= len(self.flash)
            self.read_bytes += length
            fault = self.fault if len(self.raw_reads) == 1 else None
            if second_pass and self.fault == "mismatch" and offset == 0:
                data = bytes([data[0] ^ 1]) + data[1:]
            self._reply(data, fault=fault)
        else:
            raise AssertionError(f"Unexpected opcode reached device: 0x{opcode:02x}")
        return len(packet)
