#!/usr/bin/env python3
"""Native USB receive queue ordering, buffering, and resource-lifetime regressions.

Run from the repository root with the pinned USB-backup dependencies installed:
  .venv/bin/python tests/test_pico_queued_receive.py
All device/flash input is synthetic; these tests never open real USB hardware.
"""
from pathlib import Path
import sys
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests"))

from pico_flash_backup_v2 import QueuedUSBReceive, QueuedUSBReceiveError
from pico_fake_async_usb import FakeAsyncLib, FakeRawUSB


def read_exact(rx, size, timeout=1):
    result = bytearray()
    end = time.monotonic() + timeout
    while len(result) < size and time.monotonic() < end:
        result.extend(rx.read(size - len(result)))
    return bytes(result)


class QueuedReceiveTests(unittest.TestCase):
    def assert_closed_cleanly(self, raw):
        self.assertFalse(raw.native.pending)
        self.assertFalse(raw.native.events)
        self.assertEqual(len(raw.native.freed), len(raw.native.allocations))
        self.assertEqual(raw.native.violations, [])
        self.assertFalse(getattr(raw, "_queued_receive_cleanup_blocked", False))

    def test_full_response_captured_before_consumer_reads(self):
        raw = FakeRawUSB()
        response = b"\x00\xcc\x01\x02\x80" + bytes(range(256))
        with QueuedUSBReceive(raw) as rx:
            self.assertEqual(len(raw.native.pending), 8)
            raw.native.feed(response)
            raw.native.wait_for(lambda: raw.native.callback_count >= 1)
            # Consumer is deliberately idle while the complete frame arrives.
            time.sleep(0.03)
            self.assertEqual(rx.stats["received_bytes"], 261)
            self.assertEqual(rx.read(2), response[:2])
            self.assertEqual(rx.read(3), response[2:5])
            self.assertEqual(read_exact(rx, 256), response[5:])
            self.assertEqual(raw.native.dropped, b"")
            self.assertEqual(len(raw.native.pending), 8)
        self.assert_closed_cleanly(raw)

    def test_entire_queue_captures_bursts_while_callbacks_and_consumer_pause(self):
        raw = FakeRawUSB()
        with QueuedUSBReceive(raw) as rx:
            raw.native.hold_callbacks = True
            bursts = [bytes([i]) * 261 for i in range(8)]
            for burst in bursts:
                raw.native.feed(burst)
            self.assertEqual(len(raw.native.events), 8)
            self.assertFalse(raw.native.pending)
            self.assertFalse(raw.native.dropped)
            raw.native.release_callbacks()
            self.assertEqual(read_exact(rx, 8 * 261), b"".join(bursts))
            self.assertFalse(raw.native.dropped)
        self.assert_closed_cleanly(raw)

    def test_callback_reordering_preserves_stream_order(self):
        raw = FakeRawUSB()
        with QueuedUSBReceive(raw) as rx:
            raw.native.hold_callbacks = True
            raw.native.feed(b"first")
            raw.native.feed(b"second")
            raw.native.reverse_callbacks()
            raw.native.release_callbacks()
            self.assertEqual(read_exact(rx, 11), b"firstsecond")
            self.assertGreater(rx.stats["out_of_order_callbacks"], 0)
        self.assert_closed_cleanly(raw)

    def test_existing_input_is_preserved_before_queued_data(self):
        raw = FakeRawUSB()
        raw._buffer.extend(b"prefix")
        with QueuedUSBReceive(raw) as rx:
            raw.native.feed(b"suffix")
            self.assertEqual(read_exact(rx, 12), b"prefixsuffix")
            self.assertFalse(raw._buffer)
        self.assert_closed_cleanly(raw)

    def test_read_timeout_and_write_do_not_require_native_read_lock(self):
        raw = FakeRawUSB()
        with QueuedUSBReceive(raw) as rx:
            started = time.monotonic()
            self.assertEqual(rx.read(10), b"")
            self.assertLess(time.monotonic() - started, 0.2)
            self.assertEqual(rx.write(b"command"), 7)
            self.assertEqual(raw.tx, [b"command"])
        self.assert_closed_cleanly(raw)

    def test_darwin_cancel_all_drains_callbacks_before_free(self):
        raw = FakeRawUSB()
        with QueuedUSBReceive(raw) as rx:
            self.assertEqual(len(raw.native.pending), 8)
        self.assertEqual(rx.stats["cancelled"], 8)
        callbacks = [i for i, event in enumerate(raw.native.trace) if event[0] == "callback"]
        frees = [i for i, event in enumerate(raw.native.trace) if event[0] == "free"]
        self.assertLess(max(callbacks), min(frees))
        self.assert_closed_cleanly(raw)

    def test_submit_failure_partway_through_startup_drains_and_frees(self):
        raw = FakeRawUSB(FakeAsyncLib(fail_submit_at=4))
        with self.assertRaisesRegex(QueuedUSBReceiveError, "submit"):
            with QueuedUSBReceive(raw):
                self.fail("Start should have failed")
        self.assert_closed_cleanly(raw)
        self.assertEqual(raw.native.callback_count, 3)

    def test_fatal_transfer_error_is_never_returned_as_valid_data(self):
        raw = FakeRawUSB()
        with QueuedUSBReceive(raw) as rx:
            raw.native.complete_error(6)
            with self.assertRaisesRegex(QueuedUSBReceiveError, "status 6"):
                rx.read(1)
        self.assert_closed_cleanly(raw)

    def test_buffer_overflow_fails_instead_of_dropping_bytes(self):
        raw = FakeRawUSB()
        with QueuedUSBReceive(raw, queue_depth=2, transfer_bytes=288, max_buffer=576) as rx:
            raw.native.feed(b"a" * 261)
            raw.native.wait_for(lambda: raw.native.callback_count >= 1)
            raw.native.feed(b"b" * 261)
            raw.native.wait_for(lambda: raw.native.callback_count >= 2)
            raw.native.feed(b"c" * 261)
            raw.native.wait_for(lambda: raw.native.callback_count >= 3)
            with self.assertRaisesRegex(QueuedUSBReceiveError, "buffer limit"):
                rx.read(1)
        self.assert_closed_cleanly(raw)

    def test_cancellation_timeout_retains_owned_memory(self):
        raw = FakeRawUSB(FakeAsyncLib(cancel_delivers=False))
        rx = QueuedUSBReceive(raw, cleanup_timeout=0.05)
        rx.__enter__()
        try:
            with self.assertRaisesRegex(QueuedUSBReceiveError, "cancellation"):
                rx.close()
            self.assertTrue(raw._queued_receive_cleanup_blocked)
            self.assertFalse(raw.native.freed)
            self.assertEqual(len(raw.native.pending), 8)
        finally:
            # Permit callbacks and finish teardown, so the test leaves no
            # event worker or intentionally retained active native resources.
            raw.native.cancel_delivers = True
            rx.close()
        self.assert_closed_cleanly(raw)


if __name__ == "__main__":
    unittest.main(verbosity=2)
