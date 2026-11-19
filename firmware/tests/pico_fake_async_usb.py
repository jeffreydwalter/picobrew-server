"""Independent native-libusb emulator using actual PyUSB 1.3.1 ctypes ABI.

No device access. A USB producer can fill already-submitted transfers while
the Python consumer and callback dispatcher are paused. Completed transfers
remain live until their callback has run. Darwin-style cancellation cancels
every pending transfer on the endpoint.
"""
import ctypes
from collections import deque
import threading
import time
from types import SimpleNamespace

import usb.backend.libusb1 as abi


class Function:
    def __init__(self, call):
        self.call = call
        self.argtypes = None
        self.restype = None

    def __call__(self, *args):
        return self.call(*args)


class FakeAsyncLib:
    def __init__(self, fail_submit_at=None, cancel_delivers=True):
        self.lock = threading.Condition(threading.RLock())
        self.allocations = {}
        self.pending = {}
        self.events = deque()
        self.freed = set()
        self.trace = []
        self.submit_count = 0
        self.callback_count = 0
        self.fail_submit_at = fail_submit_at
        self.cancel_delivers = cancel_delivers
        self.hold_callbacks = False
        self.dropped = bytearray()
        self.violations = []
        self.libusb_alloc_transfer = Function(self.allocate)
        self.libusb_free_transfer = Function(self.free)
        self.libusb_submit_transfer = Function(self.submit)
        self.libusb_cancel_transfer = Function(self.cancel)
        self.libusb_handle_events_timeout_completed = Function(self.events_once)
        self.libusb_handle_events_timeout = Function(self.events_once)
        self.libusb_interrupt_event_handler = Function(self.interrupt)
        self.libusb_error_name = Function(lambda error: f"fake error {error}".encode())

    @staticmethod
    def address(ptr):
        return ctypes.addressof(ptr.contents)

    def allocate(self, count):
        assert count == 0
        value = abi._libusb_transfer()
        ptr = ctypes.pointer(value)
        with self.lock:
            addr = self.address(ptr)
            self.allocations[addr] = (value, ptr)
            self.trace.append(("allocate", addr))
        return ptr

    def free(self, ptr):
        addr = self.address(ptr)
        with self.lock:
            if addr in self.pending or any(self.address(p) == addr for p, _, _ in self.events):
                self.violations.append("freed pending or undelivered transfer")
                raise AssertionError(self.violations[-1])
            if addr in self.freed:
                self.violations.append("double free")
                raise AssertionError(self.violations[-1])
            assert addr in self.allocations
            self.freed.add(addr)
            self.trace.append(("free", addr))

    def submit(self, ptr):
        addr = self.address(ptr)
        with self.lock:
            self.submit_count += 1
            if self.submit_count == self.fail_submit_at:
                self.trace.append(("submit-failed", addr))
                return -1
            assert addr not in self.freed
            assert addr not in self.pending
            assert not any(self.address(p) == addr for p, _, _ in self.events)
            value = ptr.contents
            assert value.endpoint == 0x82
            assert value.type == 2
            assert value.timeout == 0, "Queued readers must not cancel one another via timeout"
            assert value.length >= 261 and value.length % 32 == 0
            assert value.buffer
            assert bool(value.callback)
            self.pending[addr] = ptr
            self.trace.append(("submit", addr))
            self.lock.notify_all()
            return 0

    def feed(self, data, status=0):
        """Hardware producer: fill posted reads without running Python callback."""
        data = bytes(data)
        with self.lock:
            while data:
                if not self.pending:
                    self.dropped.extend(data)
                    return
                addr = next(iter(self.pending))
                ptr = self.pending.pop(addr)
                take = min(len(data), ptr.contents.length)
                part, data = data[:take], data[take:]
                ctypes.memmove(ptr.contents.buffer, part, len(part))
                self.events.append((ptr, status, len(part)))
                self.trace.append(("hardware-complete", addr, len(part)))
            self.lock.notify_all()

    def complete_error(self, status=1):
        with self.lock:
            addr = next(iter(self.pending))
            ptr = self.pending.pop(addr)
            self.events.append((ptr, status, 0))
            self.lock.notify_all()

    def cancel(self, ptr):
        addr = self.address(ptr)
        with self.lock:
            self.trace.append(("cancel", addr))
            if addr not in self.pending:
                return -5
            if not self.cancel_delivers:
                return 0
            # macOS AbortPipe behavior: cancellation covers the whole endpoint.
            for pending in list(self.pending.values()):
                self.events.append((pending, 3, 0))
            self.pending.clear()
            self.lock.notify_all()
            return 0

    def events_once(self, context, timeval, completed=None):
        with self.lock:
            until = time.monotonic() + 0.01
            while not self.events or self.hold_callbacks:
                remaining = until - time.monotonic()
                if remaining <= 0:
                    return 0
                self.lock.wait(remaining)
            ptr, status, count = self.events.popleft()
            addr = self.address(ptr)
            assert addr not in self.freed, "Callback received freed transfer"
            ptr.contents.status = status
            ptr.contents.actual_length = count
            self.trace.append(("callback", addr, status, count))
        # Invoke genuine ctypes callback outside native-state lock; this may
        # immediately resubmit the transfer, just like libusb callbacks do.
        ptr.contents.callback(ptr)
        with self.lock:
            self.callback_count += 1
            self.lock.notify_all()
        return 0

    def reverse_callbacks(self):
        with self.lock:
            self.events.reverse()

    def release_callbacks(self):
        with self.lock:
            self.hold_callbacks = False
            self.lock.notify_all()

    def interrupt(self, _context):
        with self.lock:
            self.lock.notify_all()

    def wait_for(self, predicate, seconds=1.0):
        until = time.monotonic() + seconds
        with self.lock:
            while not predicate():
                remaining = until - time.monotonic()
                if remaining <= 0:
                    raise AssertionError("Native emulator wait timed out")
                self.lock.wait(remaining)


class FakeBackend(abi._LibUSB):
    def __init__(self, lib):
        self.lib = lib
        self.ctx = ctypes.c_void_p(11)

    def _finalize_object(self):
        pass


class FakeRawUSB:
    def __init__(self, lib=None):
        self.native = lib or FakeAsyncLib()
        self._backend = FakeBackend(self.native)
        handle = SimpleNamespace(handle=ctypes.c_void_p(22))
        self._device = SimpleNamespace(_ctx=SimpleNamespace(handle=handle, backend=self._backend))
        self._in_ep = 0x82
        self._packet_size = 32
        self._buffer = bytearray()
        self._open = True
        self.details = {"cleanup_errors": []}
        self.tx = []

    def _require_open(self):
        assert self._open

    def write(self, data):
        self.tx.append(bytes(data))
        return len(data)
