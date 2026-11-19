#!/usr/bin/env python3
"""Independent wire-level tests for the read-only Pico logical-file preflight.

Run with the pinned USB-backup dependencies installed:
  .venv/bin/python tests/test_pico_flash_preflight.py
No real USB device is opened. The independent ROM/helper wire model was adapted
from the existing backup regression fixture; filesystem behavior is modeled here.
"""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
try:
    from cc3200tool import cc
except ImportError as exc:
    raise unittest.SkipTest("Audited cc3200tool package is required for wire tests") from exc
import pico_uart_probe as probe
import pico_flash_backup_v2 as backup
import pico_flash_preflight as preflight

HELPER = cc.dll_data("rbtl3100s.dll")

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


FILE_NAMES = frozenset((
    "/sys/mcuimg.bin", "/sys/mcuimg1.bin", "/sys/mcuimg2.bin",
    "/sys/mcuimg3.bin", "/sys/mcubootinfo.bin",
))
CLOSE_PACKET = b"\x22" + b"\x00" * 63 + b"\x46" * 256 + b"\x00"
NORMAL_BOOTINFO = bytes.fromhex("013d0020badccdab")


def fixture_images():
    """Deterministic images with authentic sizes and vector shape, no secrets."""
    def image(size, stack, reset, seed):
        return struct.pack("<II", stack, reset) + bytes(
            (i * seed + i // 257) & 255 for i in range(size - 8)
        )
    return {
        "/sys/mcuimg.bin": image(14956, 0x200040d8, 0x20004069, 11),
        "/sys/mcuimg1.bin": image(50504, 0x20019688, 0x2000af29, 13),
        "/sys/mcuimg2.bin": image(162792, 0x200365b8, 0x20018cd1, 17),
        "/sys/mcubootinfo.bin": NORMAL_BOOTINFO,
    }


class FakePreflightUSB(FakePicoUSB):
    """ROM + SRAM helper + read-only logical FS, independently decoded packets.

    Each read returns at most one 32-byte endpoint packet. Token returns are raw
    bytes rather than framed packets. The fake rejects write/create opens before
    generating any response, independently of the production connection guard.
    """
    def __init__(self, files=None, fault=None, flash_size=1024 * 1024):
        super().__init__(HELPER, flash_size=flash_size)
        self.files = dict(fixture_images() if files is None else files)
        self.original_files = dict(self.files)
        self.fs_fault = fault
        self.active_file = None
        self.active_offset = 0
        self.file_reads = []
        self.file_opens = []
        self.file_closes = []
        self.info_queries = []
        self.status_queries = 0
        self.status_context = None
        self.fault_used = False
        self.token = bytes.fromhex("12345678")

    def _frame_fault(self, stage):
        if self.fs_fault and self.fs_fault.startswith(stage + ":") and not self.fault_used:
            self.fault_used = True
            return self.fs_fault.split(":", 1)[1]
        return None

    def write(self, packet):
        packet = bytes(packet)
        if packet == ACK or len(packet) < 4 or packet[3] not in (0x21, 0x22, 0x23, 0x2a, 0x2b):
            return super().write(packet)
        assert not self._break, "Command sent with BREAK asserted"
        length, checksum = struct.unpack(">HB", packet[:3])
        payload = packet[3:]
        assert length == len(packet) - 1
        assert checksum == sum(payload) & 255
        assert self.helper_running, "Filesystem command before helper start"
        opcode = payload[0]
        self.trace.append(payload)
        self.rx.extend(ACK)
        if opcode == 0x2a:
            size = struct.unpack(">I", payload[1:5])[0]
            name = payload[5:].decode("ascii")
            assert len(payload) == 5 + size
            assert name in FILE_NAMES, "Unexpected logical filename"
            assert self.active_file is None, "Info query while another file is open"
            self.info_queries.append(name)
            data = self.files.get(name)
            response = bytes((int(data is not None), 0, 0, 0)) + struct.pack(">I", len(data) if data is not None else 0)
            fault = self._frame_fault("info")
            if fault == "length":
                response = response[:-1]
            elif fault == "exists":
                response = b"\x02" + response[1:]
            self._reply(response, fault=fault)
        elif opcode == 0x21:
            flags, token = struct.unpack(">II", payload[1:9])
            assert flags == 0, "Persistent write/create open reached device"
            assert token == 0, "Unexpected input file token"
            assert payload[-2:] == b"\x00\x00"
            name = payload[9:-2].decode("ascii")
            assert name in FILE_NAMES
            assert name in self.files, "Open attempted for absent file"
            assert self.active_file is None, "Second open without close"
            self.active_file = name
            self.active_offset = 0
            self.file_opens.append(name)
            self.status_context = "open"
            self.rx.extend(self.token[:3] if self._frame_fault("open") == "short" else self.token)
        elif opcode == 0x2b:
            assert len(payload) == 9
            offset, count = struct.unpack(">II", payload[1:])
            assert self.active_file is not None, "Read without open"
            assert offset == self.active_offset, "Nonsequential file read"
            assert 1 <= count <= 256, "Unbounded file read"
            content = self.files[self.active_file]
            assert offset + count <= len(content)
            self.file_reads.append((self.active_file, offset, count))
            self.active_offset += count
            response = content[offset:offset + count]
            self._reply(response, fault=self._frame_fault("read"))
        elif opcode == 0x22:
            assert payload == CLOSE_PACKET, "Unexpected close/signature packet"
            assert self.active_file is not None, "Close without read-only open"
            self.file_closes.append(self.active_file)
            self.status_context = "close"
            self.active_file = None
        elif opcode == 0x23:
            assert payload == b"\x23"
            self.status_queries += 1
            self._reply(b"\x41" if self.fs_fault == self.status_context + ":status" else b"\x40")
            self.status_context = None
        return len(packet)


class PassthroughReceive:
    def __init__(self, raw):
        self.raw = raw
    def __enter__(self):
        return self.raw
    def __exit__(self, *args):
        return False


class PreflightWireTests(unittest.TestCase):
    def setUp(self):
        self.files = fixture_images()
        self.profile = {
            name: {"size_bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
            for name, data in self.files.items() if name != "/sys/mcubootinfo.bin"
        }
        self.addCleanup(mock.patch.stopall)
        mock.patch.dict(preflight.EXPECTED_FILES, self.profile, clear=True).start()
        mock.patch.object(cc.log, "disabled", True).start()
        mock.patch.object(cc.time, "sleep").start()
        mock.patch.object(cc.CC3200Connection, "TIMEOUT", 0.02).start()

    def ready_connection(self, fake=None):
        fake = fake or FakePreflightUSB(self.files)
        fake.mode = "apps"
        fake.helper_running = True
        connection = preflight.build_connection(cc, probe, backup.USBSerialPort(fake), HELPER)
        connection.helper_sent = len(HELPER)
        connection.helper_executed = True
        connection.flash_size = len(fake.flash)
        connection.port.timeout = 0.02
        return connection, fake

    def collect(self, fake=None):
        connection, fake = self.ready_connection(fake)
        report = preflight.collect_preflight(connection, progress=lambda line: None)
        return report, connection, fake

    def assert_read_only(self, fake):
        self.assertEqual(fake.files, fake.original_files)
        for command in fake.trace:
            opcode = command[0]
            self.assertNotIn(opcode, (0x24, 0x28, 0x2e, 0x30), command.hex())
            if opcode == 0x21:
                self.assertEqual(struct.unpack(">II", command[1:9]), (0, 0))
            elif opcode == 0x2d:
                self.assertEqual(struct.unpack(">I", command[1:5])[0], 0)
            elif opcode == 0x2b:
                offset, size = struct.unpack(">II", command[1:9])
                self.assertLessEqual(size, 256)
                self.assertGreater(size, 0)

    def test_complete_logical_preflight_resolves_mirror_and_hashes_all_files(self):
        # The raw storage fixture contains unrelated bytes; only the logical
        # file API provides the selected mirror's actual boot-info record.
        report, connection, fake = self.collect()
        self.assertEqual(preflight.qualification(report), [])
        self.assertTrue(report["transport_read_success"])
        self.assertEqual(report["flash"], {"block_size": 4096, "block_count": 256, "size_bytes": 1048576})
        self.assertEqual(bytes.fromhex(report["boot_info"]["hex"]), NORMAL_BOOTINFO)
        self.assertEqual(report["boot_info"]["selector"], 1)
        self.assertEqual(report["boot_info"]["magic_le"], 0xABCDDCBA)
        self.assertEqual(bytes.fromhex(report["boot_info"]["reserved_hex"]), bytes.fromhex("3d0020"))
        self.assertFalse(report["files"]["/sys/mcuimg3.bin"]["exists"])
        for name, expected in self.profile.items():
            self.assertEqual(report["files"][name]["size_bytes"], expected["size_bytes"])
            self.assertEqual(report["files"][name]["sha256"], expected["sha256"])
        self.assertEqual(set(fake.file_opens), set(self.files))
        self.assertEqual(fake.file_closes, fake.file_opens)
        self.assertEqual(fake.status_queries, 2 * len(fake.file_opens))
        self.assertEqual(fake.raw_reads, [])
        self.assertIsNone(connection.open_file_name)
        self.assert_read_only(fake)

    def test_read_file_uses_exact_256_byte_chunks_and_raw_open_token(self):
        connection, fake = self.ready_connection()
        data = preflight.read_file_bytes(connection, "/sys/mcuimg2.bin")
        self.assertEqual(data, self.files["/sys/mcuimg2.bin"])
        expected = [("/sys/mcuimg2.bin", offset, min(256, len(data) - offset))
                    for offset in range(0, len(data), 256)]
        self.assertEqual(fake.file_reads, expected)
        self.assertEqual(fake.status_queries, 2)
        self.assertEqual(fake.file_closes, ["/sys/mcuimg2.bin"])
        self.assertEqual(fake.rx, b"")
        self.assert_read_only(fake)

    def test_missing_file_is_reported_without_open(self):
        connection, fake = self.ready_connection()
        info = preflight.read_file_info(connection, "/sys/mcuimg3.bin")
        self.assertEqual(info, {"exists": False, "size_bytes": 0})
        self.assertEqual(fake.file_opens, [])
        self.assert_read_only(fake)

    def test_excessive_file_size_stops_before_open(self):
        connection, fake = self.ready_connection()
        with self.assertRaises(Exception):
            preflight.read_file_bytes(connection, "/sys/mcuimg2.bin", expected_max=162791)
        self.assertEqual(fake.file_opens, [])
        self.assert_read_only(fake)

    def test_malformed_or_corrupt_info_stops_before_open(self):
        for fault in ("info:length", "info:exists", "info:checksum", "info:short"):
            with self.subTest(fault=fault):
                connection, fake = self.ready_connection(FakePreflightUSB(self.files, fault=fault))
                with self.assertRaises(Exception):
                    preflight.read_file_info(connection, "/sys/mcuimg2.bin")
                self.assertEqual(fake.file_opens, [])
                self.assert_read_only(fake)

    def test_open_token_and_status_failures_stop_before_file_reads(self):
        for fault in ("open:short", "open:status"):
            with self.subTest(fault=fault):
                connection, fake = self.ready_connection(FakePreflightUSB(self.files, fault=fault))
                with self.assertRaises(Exception):
                    preflight.read_file_bytes(connection, "/sys/mcubootinfo.bin")
                self.assertEqual(fake.file_reads, [])
                self.assert_read_only(fake)

    def test_truncated_or_bad_checksum_file_data_is_rejected(self):
        for fault, message in (("read:short", "incomplete"), ("read:checksum", "checksum")):
            with self.subTest(fault=fault):
                connection, fake = self.ready_connection(FakePreflightUSB(self.files, fault=fault))
                with self.assertRaisesRegex(Exception, message):
                    preflight.read_file_bytes(connection, "/sys/mcuimg2.bin")
                self.assertEqual(len(fake.file_reads), 1)
                self.assert_read_only(fake)

    def test_failed_close_status_cannot_complete_read(self):
        connection, fake = self.ready_connection(FakePreflightUSB(self.files, fault="close:status"))
        with self.assertRaises(Exception):
            preflight.read_file_bytes(connection, "/sys/mcubootinfo.bin")
        self.assertEqual(fake.file_closes, ["/sys/mcubootinfo.bin"])
        self.assert_read_only(fake)

    def test_wrong_firmware_digest_does_not_qualify(self):
        files = dict(self.files)
        files["/sys/mcuimg2.bin"] = files["/sys/mcuimg2.bin"][:-1] + bytes([files["/sys/mcuimg2.bin"][-1] ^ 1])
        report, _, fake = self.collect(FakePreflightUSB(files))
        self.assertTrue(report["transport_read_success"])
        issues = preflight.qualification(report)
        self.assertTrue(any("mcuimg2.bin" in issue for issue in issues), issues)
        self.assert_read_only(fake)

    def test_wrong_geometry_does_not_qualify(self):
        connection, fake = self.ready_connection(FakePreflightUSB(self.files, flash_size=512 * 1024))
        report = {}
        with self.assertRaisesRegex(preflight.PreflightError, "geometry"):
            preflight.collect_preflight(connection, details=report, progress=lambda line: None)
        self.assertTrue(preflight.qualification(report))
        self.assertFalse(report["transport_read_success"])
        self.assertEqual(fake.info_queries, [])
        self.assertEqual(fake.file_opens, [])
        self.assert_read_only(fake)

    def test_wrong_slot_or_magic_does_not_qualify(self):
        for record in (bytes.fromhex("003d0020badccdab"), bytes.fromhex("023d0020badccdab"),
                       bytes.fromhex("013d002065877856"), bytes.fromhex("013d002021433412")):
            with self.subTest(record=record.hex()):
                files = dict(self.files)
                files["/sys/mcubootinfo.bin"] = record
                report, _, fake = self.collect(FakePreflightUSB(files))
                self.assertTrue(preflight.qualification(report))
                self.assertEqual(bytes.fromhex(report["boot_info"]["hex"]), record)
                self.assert_read_only(fake)

    def test_unexpected_image3_does_not_qualify(self):
        files = dict(self.files)
        files["/sys/mcuimg3.bin"] = b"unexpected installed image"
        report, _, fake = self.collect(FakePreflightUSB(files))
        issues = preflight.qualification(report)
        self.assertTrue(any("mcuimg3.bin" in issue for issue in issues), issues)
        self.assert_read_only(fake)

    def test_guard_blocks_every_persistent_mutation_before_transport(self):
        connection, fake = self.ready_connection()
        preflight.read_file_info(connection, "/sys/mcuimg2.bin")
        name = b"/sys/mcuimg2.bin"
        commands = [b"\x24" + b"\x00" * 8, b"\x28", b"\x2e" + struct.pack(">I", 0) + name + b"\x00",
                    b"\x30" + struct.pack(">III", 2, 0, 1),
                    b"\x2d" + struct.pack(">III", 2, 0, 1) + b"\x00",
                    b"\x2d" + struct.pack(">III", 0, 0, 1) + HELPER[:1],
                    b"\x32", b"\x33" + struct.pack(">I", 26666667)]
        # Opcode 0x21 is also used for reads, so check every non-read access
        # mode and unexpected flag bit rather than blacklisting the opcode.
        for flags in (1, 0x1000, 0x2000, 0x3000, 0x10000, 0xffffffff):
            commands.append(b"\x21" + struct.pack(">II", flags, 0) + name + b"\x00\x00")
        for command in commands:
            with self.subTest(command=command.hex()):
                before = list(fake.trace)
                with self.assertRaises(Exception):
                    connection._send_packet(command)
                self.assertEqual(fake.trace, before, "Unsafe command reached the transport")
        self.assert_read_only(fake)

    def test_guard_blocks_unknown_file_names_and_unapproved_tokens(self):
        connection, fake = self.ready_connection()
        preflight.read_file_info(connection, "/sys/mcuimg2.bin")
        names = (b"wifipass.txt", b"/sys/uid.txt", b"/sys/mcuimg2.bin\x00junk", b"../sys/mcuimg2.bin")
        commands = []
        for name in names:
            connection.file_infos[name.decode("ascii")] = {"exists": True, "size_bytes": 8}
            commands.append(b"\x2a" + struct.pack(">I", len(name)) + name)
            commands.append(b"\x21" + struct.pack(">II", 0, 0) + name + b"\x00\x00")
        commands.append(b"\x21" + struct.pack(">II", 0, 123) + b"/sys/mcuimg2.bin\x00\x00")
        for command in commands:
            with self.subTest(command=command.hex()):
                before = list(fake.trace)
                with self.assertRaises(Exception):
                    connection._send_packet(command)
                self.assertEqual(fake.trace, before)
        self.assert_read_only(fake)

    def test_guard_blocks_read_without_open_and_close_without_open(self):
        connection, fake = self.ready_connection()
        for command in (b"\x2b" + struct.pack(">II", 0, 1), CLOSE_PACKET):
            with self.assertRaises(Exception):
                connection._send_packet(command)
        self.assertEqual(fake.trace, [])

    def test_guard_blocks_filesystem_commands_before_helper(self):
        fake = FakePreflightUSB(self.files)
        connection = preflight.build_connection(cc, probe, backup.USBSerialPort(fake), HELPER)
        name = b"/sys/mcubootinfo.bin"
        for command in (b"\x2a" + struct.pack(">I", len(name)) + name,
                        b"\x21" + struct.pack(">II", 0, 0) + name + b"\x00\x00", b"\x23"):
            with self.assertRaises(Exception):
                connection._send_packet(command)
        self.assertEqual(fake.trace, [])

    def test_qualification_requires_complete_transport_read(self):
        report, _, _ = self.collect()
        report["transport_read_success"] = False
        self.assertTrue(preflight.qualification(report))

    def test_guard_blocks_out_of_order_and_oversize_reads_after_open(self):
        connection, fake = self.ready_connection()
        preflight.read_file_info(connection, "/sys/mcubootinfo.bin")
        connection._open_file_for_read("/sys/mcubootinfo.bin")
        for offset, count in ((1, 1), (0, 0), (0, 9), (0, 257), (8, 1), (0xffffffff, 1)):
            with self.subTest(offset=offset, count=count):
                before = list(fake.trace)
                with self.assertRaises(preflight.PreflightError):
                    connection._send_packet(b"\x2b" + struct.pack(">II", offset, count))
                self.assertEqual(fake.trace, before)
        with self.assertRaises(preflight.PreflightError):
            connection._send_packet(CLOSE_PACKET)
        self.assertEqual(fake.file_reads, [])
        # A rejected local command does not damage a valid read-only session.
        connection._send_packet(b"\x2b" + struct.pack(">II", 0, 8))
        self.assertEqual(connection._read_packet(), NORMAL_BOOTINFO)
        for command in (CLOSE_PACKET[:-1], CLOSE_PACKET + b"\0", CLOSE_PACKET[:64] + b"\0" * 257):
            before = list(fake.trace)
            with self.assertRaises(preflight.PreflightError):
                connection._send_packet(command)
            self.assertEqual(fake.trace, before)
        connection._close_file()
        self.assert_read_only(fake)

    def test_guard_blocks_commands_until_pending_response_consumed(self):
        connection, fake = self.ready_connection()
        name = b"/sys/mcubootinfo.bin"
        query = b"\x2a" + struct.pack(">I", len(name)) + name
        connection._send_packet(query)
        for command in (query, b"\x2f", b"\x21" + struct.pack(">II", 0, 0) + name + b"\0\0"):
            before = list(fake.trace)
            with self.assertRaises(preflight.PreflightError):
                connection._send_packet(command)
            self.assertEqual(fake.trace, before)
        self.assertEqual(len(connection._read_packet()), 8)
        self.assertEqual(preflight.read_file_bytes(connection, name.decode()), NORMAL_BOOTINFO)
        self.assert_read_only(fake)

    def test_guard_preserves_exact_pinned_ram_helper_allowlist(self):
        fake = FakePreflightUSB(self.files)
        connection = preflight.build_connection(cc, probe, backup.USBSerialPort(fake), HELPER)
        bad_chunks = [
            b"\x2d" + struct.pack(">III", 2, 0, 1) + HELPER[:1],
            b"\x2d" + struct.pack(">III", 0, 1, 1) + HELPER[1:2],
            b"\x2d" + struct.pack(">III", 0, 0, 1) + bytes([HELPER[0] ^ 1]),
            b"\x2d" + struct.pack(">III", 0, 0, 2) + HELPER[:1],
            b"\x32",
        ]
        for command in bad_chunks:
            with self.assertRaises(backup.BackupError):
                connection._send_packet(command)
        self.assertEqual(fake.trace, [])
        with self.assertRaises(backup.BackupError):
            preflight.build_connection(cc, probe, backup.USBSerialPort(fake), HELPER[:-1])

    def test_short_or_missing_boot_info_does_not_qualify_or_expose_contents(self):
        for content in (None, b"unexpected secret content", b"", b"1234567"):
            with self.subTest(length=None if content is None else len(content)):
                files = dict(self.files)
                if content is None:
                    del files["/sys/mcubootinfo.bin"]
                else:
                    files["/sys/mcubootinfo.bin"] = content
                report, _, fake = self.collect(FakePreflightUSB(files))
                self.assertTrue(preflight.qualification(report))
                self.assertNotIn("hex", report.get("boot_info", {}))
                self.assertNotIn("unexpected secret content", json.dumps(report))
                self.assert_read_only(fake)

    def run_main(self, fake, output):
        transcript = io.StringIO()
        with mock.patch.object(probe, "CH340USBTransport", return_value=fake), \
             mock.patch.object(backup, "QueuedUSBReceive", PassthroughReceive), \
             contextlib.redirect_stdout(transcript), contextlib.redirect_stderr(transcript):
            result = preflight.main(["--output", str(output)])
        manifest = json.loads(output.read_text()) if output.exists() else None
        self.assert_read_only(fake)
        return result, manifest, transcript.getvalue()

    def test_main_complete_rom_helper_and_filesystem_flow_publishes_private_report(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "preflight.json"
            fake = FakePreflightUSB(self.files)
            result, manifest, transcript = self.run_main(fake, output)
            self.assertEqual(result, 0, transcript)
            self.assertTrue(manifest["qualified"])
            self.assertTrue(manifest["transport_read_success"])
            self.assertFalse(manifest["flash_modified"])
            self.assertEqual(manifest["qualification_issues"], [])
            self.assertEqual(len(fake.uploaded), 49588)
            self.assertEqual(fake.break_count, 2)
            self.assertTrue(fake.closed)
            self.assertEqual(output.stat().st_mode & 0o777, 0o600)
            self.assertFalse(Path(str(output) + ".partial").exists())
            self.assertIn("QUALIFIED: live firmware hashes", transcript)

    def test_main_transport_error_saves_unqualified_report(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "preflight.json"
            fake = FakePreflightUSB(self.files, fault="read:checksum")
            result, manifest, transcript = self.run_main(fake, output)
            self.assertEqual(result, 1, transcript)
            self.assertFalse(manifest["qualified"])
            self.assertFalse(manifest["transport_read_success"])
            self.assertIn("checksum", manifest["error"]["message"])
            self.assertTrue(fake.closed)
            self.assertNotIn("QUALIFIED: live firmware hashes", transcript)

    def test_main_profile_mismatch_is_distinct_from_transport_failure(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "preflight.json"
            files = dict(self.files)
            files["/sys/mcubootinfo.bin"] = bytes.fromhex("003d0020badccdab")
            fake = FakePreflightUSB(files)
            result, manifest, transcript = self.run_main(fake, output)
            self.assertEqual(result, 2, transcript)
            self.assertFalse(manifest["qualified"])
            self.assertTrue(manifest["transport_read_success"])
            self.assertNotIn("error", manifest)
            self.assertTrue(fake.closed)

    def test_existing_report_blocks_usb_and_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "preflight.json"
            output.write_bytes(b"existing saved report")
            fake = FakePreflightUSB(self.files)
            with mock.patch.object(probe, "CH340USBTransport") as transport_factory, \
                 contextlib.redirect_stdout(io.StringIO()):
                result = preflight.main(["--output", str(output)])
            self.assertEqual(result, 1)
            transport_factory.assert_not_called()
            self.assertEqual(output.read_bytes(), b"existing saved report")
            self.assertEqual(fake.trace, [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
