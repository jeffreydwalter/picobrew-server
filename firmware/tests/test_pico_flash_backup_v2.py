#!/usr/bin/env python3
"""Read-only backup protocol, publication, and command-guard regressions.

Run from the repository root with the pinned USB-backup dependencies installed:
  .venv/bin/python tests/test_pico_flash_backup_v2.py
All device/flash input is synthetic; these tests never open real USB hardware.
"""
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import stat
import struct
import tempfile
import sys
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests"))

from cc3200tool import cc
import pico_uart_probe as probe
import pico_flash_backup_v2 as backup
from pico_backup_fake_device import FakePicoUSB


HELPER = cc.dll_data("rbtl3100s.dll")


class CleanupFailureUSB(FakePicoUSB):
    def __exit__(self, exc_type, exc, tb):
        super().__exit__(exc_type, exc, tb)
        self.details["cleanup_errors"].append("simulated driver reattachment failure")
        if exc_type is None:
            raise probe.CH340USBError("simulated driver reattachment failure")
        return False


class PassthroughReceive:
    def __init__(self, raw):
        self.raw = raw
    def __enter__(self):
        return self.raw
    def __exit__(self, *args):
        return False


class BackupTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.output = Path(self.temp.name) / "pico.bin"

    def run_main(self, fake, extra_args=()):
        transcript = io.StringIO()
        with mock.patch.object(probe, "CH340USBTransport", return_value=fake), \
             mock.patch.object(backup, "QueuedUSBReceive", PassthroughReceive), \
             mock.patch.object(cc.time, "sleep"), \
             mock.patch.object(cc.log, "disabled", True), \
             mock.patch.object(cc.CC3200Connection, "TIMEOUT", 0.05), \
             contextlib.redirect_stdout(transcript), \
             contextlib.redirect_stderr(transcript):
            result = backup.main(["--output", str(self.output), *extra_args])
        return result, transcript.getvalue()

    def new_connection(self, fake=None):
        fake = fake or FakePicoUSB(HELPER, flash_size=8192)
        connection = backup.guarded_connection(cc, probe, backup.USBSerialPort(fake), HELPER)
        return connection, fake

    def test_full_megabyte_both_passes_fragmented(self):
        fake = FakePicoUSB(HELPER)
        result, transcript = self.run_main(fake)
        self.assertEqual(result, 0, transcript)
        self.assertEqual(self.output.read_bytes(), fake.flash)
        self.assertEqual(len(fake.raw_reads), 2 * len(fake.flash) // backup.CHUNK_SIZE)
        self.assertEqual(fake.read_bytes, 2 * 1024 * 1024)
        self.assertEqual(len(fake.uploaded), 49588)
        self.assertEqual(fake.break_count, 2)
        self.assertGreater(fake.read_count, 65000)
        self.assertTrue(fake.closed)
        self.assertFalse(fake.break_condition)
        self.assertEqual(stat.S_IMODE(self.output.stat().st_mode), 0o600)
        self.assertFalse(Path(str(self.output) + ".partial").exists())
        self.assertFalse(Path(str(self.output) + ".verify.partial").exists())
        expected = hashlib.sha256(fake.flash).hexdigest()
        manifest = json.loads(Path(str(self.output) + ".json").read_text())
        self.assertFalse(manifest["flash_modified"])
        self.assertEqual(manifest["flash"]["size_bytes"], len(fake.flash))
        self.assertEqual([r["sha256"] for r in manifest["reads"]], [expected, expected])
        self.assertEqual(Path(str(self.output) + ".sha256").read_text(), expected + "  pico.bin\n")
        self.assertIn("VERIFIED: both complete flash reads match byte-for-byte", transcript)
        self.assertNotIn("No Pico flash was read or written.", transcript)

    def test_valid_checksum_second_read_mismatch_not_published(self):
        fake = FakePicoUSB(HELPER, flash_size=8192, fault="mismatch")
        result, transcript = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertIn("two reads differ", transcript)
        self.assertFalse(self.output.exists())
        self.assertEqual(Path(str(self.output) + ".partial").read_bytes(), fake.flash)
        self.assertEqual(Path(str(self.output) + ".verify.partial").stat().st_size, backup.CHUNK_SIZE)
        self.assertTrue(fake.closed)

    def test_packet_checksum_failure_not_published(self):
        fake = FakePicoUSB(HELPER, flash_size=8192, fault="checksum")
        result, transcript = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertIn("checksum mismatch", transcript)
        self.assertFalse(self.output.exists())
        self.assertEqual(Path(str(self.output) + ".partial").stat().st_size, 0)
        self.assertTrue(fake.closed)

    def test_truncated_packet_not_published(self):
        fake = FakePicoUSB(HELPER, flash_size=8192, fault="short")
        result, transcript = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertIn(f"Response incomplete: {backup.CHUNK_SIZE // 2}/{backup.CHUNK_SIZE} bytes", transcript)
        self.assertFalse(self.output.exists())
        self.assertTrue(fake.closed)

    def test_nack_stops_without_loader_or_flash(self):
        fake = FakePicoUSB(HELPER, flash_size=8192, fault="nack")
        result, transcript = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertIn("NACK", transcript)
        self.assertFalse(self.output.exists())
        self.assertEqual(fake.uploaded, b"")
        self.assertEqual(fake.raw_reads, [])
        self.assertTrue(fake.closed)

    def test_cleanup_failure_retains_verified_backup(self):
        fake = CleanupFailureUSB(HELPER, flash_size=8192)
        result, transcript = self.run_main(fake)
        self.assertEqual(result, 0, transcript)
        self.assertEqual(self.output.read_bytes(), fake.flash)
        self.assertIn("USB CLEANUP: backup is verified", transcript)
        manifest = json.loads(Path(str(self.output) + ".json").read_text())
        self.assertTrue(manifest["warnings"])

    def test_existing_output_is_never_overwritten(self):
        self.output.write_bytes(b"existing backup")
        fake = FakePicoUSB(HELPER, flash_size=8192)
        result, transcript = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertEqual(self.output.read_bytes(), b"existing backup")
        self.assertIn("Refusing to overwrite", transcript)
        self.assertEqual(fake.trace, [])

    def test_dangling_output_symlink_is_never_followed(self):
        target = self.output.parent / "missing"
        self.output.symlink_to(target)
        fake = FakePicoUSB(HELPER, flash_size=8192)
        result, transcript = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertTrue(self.output.is_symlink())
        self.assertFalse(target.exists())
        self.assertEqual(fake.trace, [])

    def test_existing_partial_is_never_overwritten(self):
        partial = Path(str(self.output) + ".partial")
        partial.write_bytes(b"partial backup")
        result, transcript = self.run_main(FakePicoUSB(HELPER, flash_size=8192))
        self.assertEqual(result, 1, transcript)
        self.assertEqual(partial.read_bytes(), b"partial backup")
        self.assertFalse(self.output.exists())

    def test_disk_corruption_blocks_publication(self):
        with backup.BackupFiles(self.output, None) as files:
            files.first.write(b"abcd")
            files.second.write(b"abce")
            details = {"reads": [{"sha256": hashlib.sha256(b"abcd").hexdigest()}],
                       "flash": {"size_bytes": 4}}
            with self.assertRaisesRegex(backup.BackupError, "on-disk backup"):
                files.publish(details)
        self.assertFalse(self.output.exists())
        self.assertTrue(Path(str(self.output) + ".partial").exists())

    def test_publication_race_does_not_overwrite(self):
        with backup.BackupFiles(self.output, None) as files:
            files.first.write(b"abcd")
            files.second.write(b"abcd")
            self.output.write_bytes(b"another process")
            details = {"reads": [{"sha256": hashlib.sha256(b"abcd").hexdigest()}],
                       "flash": {"size_bytes": 4}}
            with self.assertRaises(FileExistsError):
                files.publish(details)
        self.assertEqual(self.output.read_bytes(), b"another process")

    def test_guard_rejects_persistent_mutations_and_unexpected_helper(self):
        connection, fake = self.new_connection()
        commands = [
            b"\x28", b"\x30" + struct.pack(">III", 2, 0, 1),
            b"\x2e" + b"/sys/mcuimg2.bin", b"\x21", b"\x22", b"\x24",
            b"\x2d" + struct.pack(">III", 2, 0, 1) + HELPER[:1],
            b"\x2d" + struct.pack(">III", 0, 1, 1) + HELPER[1:2],
            b"\x2d" + struct.pack(">III", 0, 0, 1) + bytes([HELPER[0] ^ 1]),
            b"\x2d" + struct.pack(">III", 0, 0, 2) + HELPER[:1],
            b"\x32", b"\x2c" + struct.pack(">III", 2, 0, 4096),
            b"\x33" + struct.pack(">I", 1),
        ]
        for command in commands:
            with self.subTest(command=command.hex()):
                with self.assertRaises(backup.BackupError):
                    connection._send_packet(command)
        self.assertEqual(fake.trace, [])
        self.assertEqual(connection.helper_sent, 0)

    def test_guard_rejects_out_of_bounds_flash_reads(self):
        connection, fake = self.new_connection()
        connection.helper_executed = True
        connection.flash_size = 8192
        for storage, offset, count in [(0, 0, 1), (2, 0, 0), (2, 0, backup.CHUNK_SIZE + 1), (2, 0, 4097), (2, 8192, 1), (2, 8191, 2)]:
            with self.assertRaises(backup.BackupError):
                connection._send_packet(b"\x2c" + struct.pack(">III", storage, offset, count))
        self.assertEqual(fake.trace, [])

    def test_invalid_helper_fails_before_usb(self):
        with self.assertRaisesRegex(backup.BackupError, "differs from the audited helper"):
            backup.validate_helper(HELPER[:-1] + bytes([HELPER[-1] ^ 1]))

    def test_timeout_restored_after_exception(self):
        connection, _ = self.new_connection()
        initial = connection.port.timeout
        with self.assertRaises(RuntimeError):
            with connection._serial_timeout(0.5):
                self.assertEqual(connection.port.timeout, 0.5)
                raise RuntimeError("test")
        self.assertEqual(connection.port.timeout, initial)

    def test_private_file_ownership_is_applied(self):
        with mock.patch.object(backup.os, "fchown") as fchown:
            with backup.new_private_file(self.output, (123, 456)) as file:
                fd = file.fileno()
                fchown.assert_called_once_with(fd, 123, 456)
                file.write(b"private")
        self.assertEqual(stat.S_IMODE(self.output.stat().st_mode), 0o600)

    def test_malformed_header_is_rejected_before_payload_read(self):
        connection, fake = self.new_connection()
        for length in (0, 1, 2, backup.CHUNK_SIZE + 3, 4099, 65535):
            fake.rx[:] = struct.pack(">HB", length, 0)
            with self.assertRaisesRegex(cc.CC3200Error, "Invalid response length"):
                connection._read_packet()


if __name__ == "__main__":
    unittest.main(verbosity=2)
