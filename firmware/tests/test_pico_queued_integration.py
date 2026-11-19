#!/usr/bin/env python3
"""Full backup integration using the upstream protocol and actual ctypes RX queue.

Run from the repository root with the pinned USB-backup dependencies installed:
  .venv/bin/python tests/test_pico_queued_integration.py
All device/flash input is synthetic; these tests never open real USB hardware.
"""
import contextlib
import ctypes
import hashlib
import io
import json
from pathlib import Path
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
from pico_fake_async_usb import FakeRawUSB


HELPER = cc.dll_data("rbtl3100s.dll")


class NativeQueuedPico(FakePicoUSB):
    def __init__(self, flash_size=1024 * 1024, fault=None):
        super().__init__(HELPER, flash_size=flash_size, fault=fault)
        self.native_raw = FakeRawUSB()
        for name in ("native", "_backend", "_device", "_in_ep", "_packet_size", "_buffer", "_open"):
            setattr(self, name, getattr(self.native_raw, name))
        self.description = "simulated Pico + native libusb transfer queue"
        self.raw_reads_after_queue = 0

    def write(self, packet):
        count = super().write(packet)
        if self.native.allocations and len(self.native.freed) != len(self.native.allocations):
            if self.rx:
                data = bytes(self.rx)
                self.rx.clear()
                # Deliver an entire response on the USB producer side before
                # Python's acknowledgment/header consumer is allowed to run.
                self.native.feed(data)
        return count

    def read(self, count=1):
        if self.native.allocations and len(self.native.freed) != len(self.native.allocations):
            self.raw_reads_after_queue += 1
            raise AssertionError("Backup returned to one-packet synchronous receive")
        return super().read(count)

    def __exit__(self, *args):
        assert not self.native.pending, "Raw handle closed while native reads still own buffers"
        assert not self.native.events, "Raw handle closed before completion callbacks drained"
        return super().__exit__(*args)


class IntegrationTests(unittest.TestCase):
    def run_main(self, fake):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "pico.bin"
            transcript = io.StringIO()
            with mock.patch.object(probe, "CH340USBTransport", return_value=fake), \
                 mock.patch.object(cc.time, "sleep"), \
                 mock.patch.object(cc.log, "disabled", True), \
                 mock.patch.object(cc.CC3200Connection, "TIMEOUT", 0.2), \
                 contextlib.redirect_stdout(transcript), \
                 contextlib.redirect_stderr(transcript):
                result = backup.main(["--output", str(output)])
            binary = output.read_bytes() if output.exists() else None
            manifest_path = Path(str(output) + ".json")
            manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else None
            self.assertFalse(fake.native.pending)
            self.assertFalse(fake.native.events)
            self.assertEqual(len(fake.native.freed), len(fake.native.allocations))
            self.assertEqual(fake.native.violations, [])
            self.assertEqual(fake.native.dropped, b"")
            self.assertEqual(fake.raw_reads_after_queue, 0)
            return result, transcript.getvalue(), binary, manifest

    def test_two_full_megabyte_reads_through_native_queued_engine(self):
        fake = NativeQueuedPico()
        result, transcript, binary, manifest = self.run_main(fake)
        self.assertEqual(result, 0, transcript)
        self.assertEqual(binary, fake.flash)
        self.assertEqual(fake.read_bytes, 2 * 1024 * 1024)
        self.assertEqual(len(fake.raw_reads), 8192)
        self.assertEqual(len(fake.uploaded), 49588)
        self.assertEqual(fake.break_count, 2)
        self.assertEqual(manifest["reads"][0]["sha256"], hashlib.sha256(fake.flash).hexdigest())
        self.assertEqual(manifest["reads"][1]["sha256"], hashlib.sha256(fake.flash).hexdigest())
        self.assertGreater(manifest["usb"]["receive_queue"]["received_bytes"], 2 * 1024 * 1024)
        self.assertEqual(manifest["usb"]["receive_queue"]["queue_depth"], 8)
        self.assertTrue(manifest["usb"]["receive_queue"]["closed"])
        self.assertIn("VERIFIED", transcript)

    def test_bad_checksum_still_fails_and_cleans_up_queue(self):
        fake = NativeQueuedPico(flash_size=8192, fault="checksum")
        result, transcript, binary, manifest = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertIn("checksum mismatch", transcript)
        self.assertIsNone(binary)
        self.assertIsNone(manifest)

    def test_second_read_mismatch_still_fails_and_cleans_up_queue(self):
        fake = NativeQueuedPico(flash_size=8192, fault="mismatch")
        result, transcript, binary, manifest = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertIn("two reads differ", transcript)
        self.assertIsNone(binary)
        self.assertIsNone(manifest)

    def test_truncated_packet_still_fails_and_cleans_up_queue(self):
        fake = NativeQueuedPico(flash_size=8192, fault="short")
        result, transcript, binary, manifest = self.run_main(fake)
        self.assertEqual(result, 1, transcript)
        self.assertIn("Response incomplete: 128/256", transcript)
        self.assertIsNone(binary)
        self.assertIsNone(manifest)


if __name__ == "__main__":
    unittest.main(verbosity=2)
