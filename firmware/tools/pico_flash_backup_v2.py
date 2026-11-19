#!/usr/bin/env python3
"""Back up a PicoBrew CC3200 with queued direct-USB reception (revision 2).

Put this file beside pico_uart_probe.py. Use the same Python environment that
contains cc3200tool, pyusb and libusb-package. On macOS run with sudo and an
absolute --output path. Leave USB connected and unplug Pico mains BEFORE
starting; restore mains only when the script prints "BREAK asserted".

This uses the installed cc3200tool protocol implementation, but never its CLI
or serial-port connection. It permits only version/storage queries, the exact
upstream SRAM helper upload, executing that helper, and bounded flash reads.
It reads the detected flash capacity twice and compares every byte. No flash
write, erase, format, filesystem update, or application start is permitted.

The helper and protocol were audited against:
https://github.com/toniebox-reverse-engineering/cc3200tool/tree/c2332898a1ededc0bdd01ec279b23366d213f81e
The SRAM helper is rbtl3100s.dll, 49,588 bytes, SHA-256 pinned below.
"""

import argparse
from contextlib import contextmanager
import ctypes
from datetime import datetime, timezone
import hashlib
from importlib import metadata
import json
import math
import os
from pathlib import Path
import struct
import sys
import threading
import time


HELPER_NAME = "rbtl3100s.dll"
HELPER_SIZE = 49588
HELPER_SHA256 = "2c9a05c469389446e05871a88e84ca3994bf3d6a6785155135db21d33aa63cc9"
# Upstream documents 256-byte reads observed from TI UniFlash. Keep those
# frames while queued native USB transfers remove per-packet polling gaps.
CHUNK_SIZE = 256
BACKUP_REVISION = 2
MAX_FLASH_SIZE = 32 * 1024 * 1024


class BackupError(Exception):
    pass


class USBSession:
    """Never release a native handle that still owns receive transfers."""

    def __init__(self, raw):
        self.raw = raw

    def __enter__(self):
        return self.raw.__enter__()

    def __exit__(self, exc_type, exc, traceback):
        if getattr(self.raw, "_queued_receive_cleanup_blocked", False):
            message = ("Receive cancellation is incomplete; USB resources are "
                       "retained until this process exits. Reconnect USB afterward.")
            self.raw.details.setdefault("cleanup_errors", []).append(message)
            return False
        return self.raw.__exit__(exc_type, exc, traceback)


class QueuedUSBReceiveError(RuntimeError):
    pass


# If cancellation cannot finish, retain the native handle, transfers, buffers,
# and callbacks. The caller must also skip its ordinary USB cleanup in this
# case; closing/freeing objects still owned by libusb is unsafe.
_RETAINED_USB_RECEIVERS = []


class _NativeUSBSlot:
    def __init__(self, pointer, buffer):
        self.pointer = pointer
        self.buffer = buffer
        self.key = ctypes.cast(pointer, ctypes.c_void_p).value


class _NativeUSBReceiveDriver:
    def __init__(self, raw):
        if metadata.version("pyusb") != "1.3.1":
            raise QueuedUSBReceiveError("Queued reception requires the tested PyUSB 1.3.1")
        from usb.backend import libusb1
        if not isinstance(raw._backend, libusb1._LibUSB):
            raise QueuedUSBReceiveError("Queued reception requires the libusb1 backend")
        self._abi = libusb1
        self._lib = raw._backend.lib
        self._ctx = raw._backend.ctx
        self._handle = raw._device._ctx.handle.handle
        self._endpoint = raw._in_ep
        self._callback = None
        self._slots = {}
        transfer_pointer = ctypes.POINTER(libusb1._libusb_transfer)
        self._lib.libusb_alloc_transfer.argtypes = [ctypes.c_int]
        self._lib.libusb_alloc_transfer.restype = transfer_pointer
        self._lib.libusb_submit_transfer.argtypes = [transfer_pointer]
        self._lib.libusb_submit_transfer.restype = ctypes.c_int
        self._lib.libusb_cancel_transfer.argtypes = [transfer_pointer]
        self._lib.libusb_cancel_transfer.restype = ctypes.c_int
        self._lib.libusb_free_transfer.argtypes = [transfer_pointer]
        self._lib.libusb_free_transfer.restype = None

        class Timeval(ctypes.Structure):
            _fields_ = [("tv_sec", ctypes.c_long), ("tv_usec", ctypes.c_long)]

        self._Timeval = Timeval
        self._lib.libusb_handle_events_timeout_completed.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(Timeval), ctypes.POINTER(ctypes.c_int),
        ]
        self._lib.libusb_handle_events_timeout_completed.restype = ctypes.c_int

    def set_callback(self, callback):
        def deliver(pointer):
            key = ctypes.cast(pointer, ctypes.c_void_p).value
            try:
                fields = pointer.contents
                slot = self._slots[key]
                size = int(fields.actual_length)
                if not 0 <= size <= len(slot.buffer):
                    raise QueuedUSBReceiveError(f"Invalid USB completion length: {size}")
                data = bytes(slot.buffer[:size])
                callback(key, int(fields.status), data)
            except BaseException as exc:
                # Never let a Python exception disappear inside ctypes callback
                # handling. The owning queue turns this into a read failure.
                callback(key, -1, b"", error=exc)

        self._callback = self._abi._libusb_transfer_cb_fn_p(deliver)

    def allocate(self, size):
        pointer = self._lib.libusb_alloc_transfer(0)
        if not pointer:
            raise MemoryError("libusb_alloc_transfer failed")
        try:
            buffer = (ctypes.c_ubyte * size)()
            slot = _NativeUSBSlot(pointer, buffer)
            fields = pointer.contents
            # Assign only fixed fields before the flexible-array tail. The
            # PyUSB structure declares one iso descriptor, but alloc_transfer(0)
            # allocates none: never memset/copy sizeof(PyUSB's whole structure).
            fields.dev_handle = self._handle
            fields.flags = 0
            fields.endpoint = self._endpoint
            fields.type = 2  # LIBUSB_TRANSFER_TYPE_BULK
            fields.timeout = 0  # Cancellation occurs only during shutdown.
            fields.length = size
            fields.callback = self._callback
            fields.buffer = ctypes.cast(buffer, ctypes.c_void_p)
            fields.num_iso_packets = 0
            # Leave calloc-initialized user_data alone (PyUSB types it py_object).
            self._slots[slot.key] = slot
            return slot
        except BaseException:
            self._lib.libusb_free_transfer(pointer)
            raise

    def submit(self, slot):
        return self._lib.libusb_submit_transfer(slot.pointer)

    def cancel(self, slot):
        return self._lib.libusb_cancel_transfer(slot.pointer)

    def free(self, slot):
        self._lib.libusb_free_transfer(slot.pointer)
        self._slots.pop(slot.key, None)

    def handle_events(self, timeout_seconds):
        seconds = int(timeout_seconds)
        timeval = self._Timeval(seconds, int((timeout_seconds - seconds) * 1000000))
        return self._lib.libusb_handle_events_timeout_completed(
            self._ctx, ctypes.byref(timeval), None,
        )


class QueuedUSBReceive:
    """USB stream reader backed by multiple outstanding native IN transfers.

    An optional driver injection supports tests without a USB device. Its API
    is set_callback, allocate, submit, cancel, free, and handle_events; callbacks
    pass (slot_key, status, bytes, error=None). Native libusb return codes apply.
    """

    def __init__(self, raw, driver=None, clock=time.monotonic, queue_depth=8,
                 transfer_bytes=4096, max_buffer=1024 * 1024, cleanup_timeout=5.0):
        if queue_depth < 2 or transfer_bytes < raw._packet_size:
            raise ValueError("Receive queue needs multiple transfers of at least one USB packet")
        if transfer_bytes % raw._packet_size:
            raise ValueError("USB transfer size must be a multiple of endpoint packet size")
        if max_buffer < queue_depth * transfer_bytes or cleanup_timeout <= 0:
            raise ValueError("Invalid receive buffer or cleanup timeout")
        self.raw = raw
        self._driver = driver
        self._clock = clock
        self._depth = queue_depth
        self._transfer_bytes = transfer_bytes
        self._max_buffer = max_buffer
        self._cleanup_timeout = cleanup_timeout
        self._condition = threading.Condition(threading.RLock())
        self._buffer = bytearray()
        self._completed = {}
        self._buffered_total = 0
        self._active = {}
        self._slots = {}
        self._next_submit = 0
        self._next_deliver = 0
        self._error = None
        self._stopping = False
        self._closed = False
        self._entered = False
        self._thread = None
        self._event_stop = threading.Event()
        self.stats = {
            "engine": "libusb asynchronous bulk IN", "queue_depth": queue_depth,
            "transfer_bytes": transfer_bytes, "usb_packet_bytes": raw._packet_size,
            "transfer_timeout_ms": 0, "submitted": 0, "completed": 0,
            "cancelled": 0, "received_bytes": 0, "read_calls": 0,
            "active_transfers": 0, "peak_buffered_bytes": 0,
            "out_of_order_callbacks": 0, "error": None,
            "cleanup_errors": [], "cleanup_pending": False, "closed": False,
        }
        raw.details["receive_queue"] = self.stats

    def _fail_locked(self, error):
        if self._error is None:
            self._error = QueuedUSBReceiveError(str(error))
            self.stats["error"] = str(error)
        self._stopping = True
        self._condition.notify_all()

    def _submit_locked(self, slot):
        sequence = self._next_submit
        self._next_submit += 1
        self._active[slot.key] = sequence
        try:
            result = self._driver.submit(slot)
        except BaseException as exc:
            self._active.pop(slot.key, None)
            self.stats["active_transfers"] = len(self._active)
            self._fail_locked(f"USB receive submission failed: {exc}")
            return
        if result != 0:
            self._active.pop(slot.key, None)
            self._fail_locked(f"libusb_submit_transfer failed with code {result}")
        else:
            self.stats["submitted"] += 1
        self.stats["active_transfers"] = len(self._active)

    def _on_complete(self, key, status, data, error=None):
        with self._condition:
            sequence = self._active.pop(key, None)
            self.stats["active_transfers"] = len(self._active)
            if sequence is None:
                self._fail_locked("Unexpected or duplicate USB completion")
                return
            self.stats["completed"] += 1
            self.stats["received_bytes"] += len(data)
            if status == 3 and self._stopping:
                self.stats["cancelled"] += 1
                self._condition.notify_all()
                return
            if error is not None or status != 0:
                self._fail_locked(error or f"USB receive transfer status {status}")
                return
            if self._stopping:
                self._condition.notify_all()
                return
            if self._buffered_total + len(data) > self._max_buffer:
                self._fail_locked("USB receive buffer limit exceeded; data cannot be verified")
                return
            if sequence != self._next_deliver:
                self.stats["out_of_order_callbacks"] += 1
            self._completed[sequence] = data
            self._buffered_total += len(data)
            self.stats["peak_buffered_bytes"] = max(
                self.stats["peak_buffered_bytes"], self._buffered_total,
            )
            while self._next_deliver in self._completed:
                self._buffer.extend(self._completed.pop(self._next_deliver))
                self._next_deliver += 1
            self._submit_locked(self._slots[key])
            self._condition.notify_all()

    def _event_loop(self):
        while not self._event_stop.is_set():
            try:
                result = self._driver.handle_events(0.050)
                if result not in (0, -10):  # EINTR is safe to retry.
                    raise QueuedUSBReceiveError(f"libusb event handling failed with code {result}")
            except BaseException as exc:
                with self._condition:
                    self._fail_locked(exc)
                # A broken event source must not become a busy loop during
                # bounded shutdown. Retain resources if callbacks cannot drain.
                self._event_stop.wait(0.050)

    def __enter__(self):
        if self._entered:
            raise QueuedUSBReceiveError("Receive queue cannot be entered twice")
        self._entered = True
        try:
            self._driver = self._driver or _NativeUSBReceiveDriver(self.raw)
            self._driver.set_callback(self._on_complete)
            self._buffer.extend(self.raw._buffer)
            self.raw._buffer.clear()
            self._buffered_total = len(self._buffer)
            if self._buffered_total > self._max_buffer:
                raise QueuedUSBReceiveError("Existing USB buffer exceeds receive limit")
            for _ in range(self._depth):
                slot = self._driver.allocate(self._transfer_bytes)
                self._slots[slot.key] = slot
            with self._condition:
                for slot in self._slots.values():
                    self._submit_locked(slot)
                    if self._error is not None:
                        raise self._error
            # All native transfers are queued before the caller can send a
            # storage query. No guessed sleep/start-ready race is involved.
            self._thread = threading.Thread(
                target=self._event_loop, name="pico-usb-receive", daemon=True,
            )
            self._thread.start()
            return self
        except BaseException as exc:
            try:
                self.close()
            except BaseException as cleanup:
                raise QueuedUSBReceiveError(f"Receive setup failed: {exc}; {cleanup}") from exc
            raise

    def read(self, size=1):
        if not isinstance(size, int) or size < 0:
            raise ValueError("read size must be a non-negative integer")
        if size == 0:
            return b""
        with self._condition:
            self.stats["read_calls"] += 1
            if not self._buffer and self._error is None and not self._stopping:
                self._condition.wait(0.050)
            if self._error is not None:
                raise self._error
            if self._stopping:
                raise QueuedUSBReceiveError("USB receive queue is closed")
            data = bytes(self._buffer[:size])
            del self._buffer[:size]
            self._buffered_total -= len(data)
            return data

    def write(self, data):
        with self._condition:
            if self._error is not None:
                raise self._error
            if self._stopping:
                raise QueuedUSBReceiveError("USB receive queue is closed")
        # No receive lock is held across synchronous USB I/O. libusb may invoke
        # receive callbacks from this thread while servicing the write.
        return self.raw.write(data)

    def _retain_unsafe(self, reason):
        self.stats["cleanup_pending"] = True
        self.stats["cleanup_errors"].append(reason)
        self.raw._queued_receive_cleanup_blocked = True
        self.raw._queued_receive_owner = self
        if self not in _RETAINED_USB_RECEIVERS:
            _RETAINED_USB_RECEIVERS.append(self)
        raise QueuedUSBReceiveError(
            reason + "; native USB resources were retained to avoid freeing active transfers"
        )

    def close(self):
        if self._closed:
            return
        with self._condition:
            self._stopping = True
            pending = [self._slots[key] for key in self._active]
            self._condition.notify_all()
        for slot in pending:
            try:
                result = self._driver.cancel(slot)
                # Darwin cancels the whole endpoint on the first call; later
                # cancels can find an already completed/cancelled transfer.
                if result not in (0, -5):
                    self.stats["cleanup_errors"].append(f"cancel returned {result}")
            except BaseException as exc:
                self.stats["cleanup_errors"].append(f"cancel failed: {exc}")
        deadline = self._clock() + self._cleanup_timeout
        while True:
            with self._condition:
                if not self._active:
                    break
                remaining = deadline - self._clock()
                if remaining <= 0:
                    self._retain_unsafe("USB cancellation did not finish before cleanup deadline")
                if self._thread is not None and self._thread.is_alive():
                    self._condition.wait(min(0.050, remaining))
                    continue
            # Setup can fail after submission but before creating the worker.
            try:
                self._driver.handle_events(min(0.050, remaining))
            except BaseException as exc:
                self.stats["cleanup_errors"].append(f"cleanup event handling: {exc}")
                time.sleep(min(0.010, max(0, deadline - self._clock())))
        self._event_stop.set()
        if self._thread is not None and self._thread.ident is not None:
            self._thread.join(timeout=1.0)
            if self._thread.is_alive():
                self._retain_unsafe("USB event thread did not stop")
        for slot in self._slots.values():
            self._driver.free(slot)
        self._slots.clear()
        self._closed = True
        self.stats["closed"] = True
        self.stats["cleanup_pending"] = False
        self.raw._queued_receive_cleanup_blocked = False

    def __exit__(self, exc_type, exc, traceback):
        try:
            self.close()
        except BaseException as cleanup:
            if exc is None:
                raise
            # Keep the original protocol error while making the unsafe-cleanup
            # state explicit for the containing raw transport context.
            self.stats["cleanup_errors"].append(str(cleanup))
        return False


def report(message):
    print(message, flush=True)


class USBSerialPort:
    """Accumulate USB packets like pySerial.read; timeout changes are local.

    Each underlying USB read waits at most 50 ms, so a deadline can overshoot
    by that amount. This backup uses only positive, finite read timeouts.
    It never reconfigures baud, DTR, RTS, or the USB device on timeout changes.
    """

    def __init__(self, transport, clock=time.monotonic):
        self.transport = transport
        self.clock = clock
        self.timeout = 5.0

    @property
    def timeout(self):
        return self._timeout

    @timeout.setter
    def timeout(self, value):
        if value is None or not math.isfinite(value) or value <= 0:
            raise ValueError("This USB wrapper requires a positive finite timeout")
        self._timeout = float(value)

    def read(self, size=1):
        if not isinstance(size, int) or size < 0:
            raise ValueError("read size must be a non-negative integer")
        deadline = self.clock() + self.timeout
        data = bytearray()
        while len(data) < size and self.clock() < deadline:
            data.extend(self.transport.read(size - len(data)))
        return bytes(data)

    def write(self, data):
        sent = self.transport.write(data)
        if sent != len(data):
            raise BackupError(f"Short USB write: {sent}/{len(data)} bytes")
        return sent


def validate_helper(helper):
    if not isinstance(helper, bytes):
        raise BackupError("cc3200tool returned an invalid SRAM helper")
    digest = hashlib.sha256(helper).hexdigest()
    if len(helper) != HELPER_SIZE or digest != HELPER_SHA256:
        raise BackupError(
            f"The installed {HELPER_NAME} differs from the audited helper: "
            f"{len(helper)} bytes, SHA256 {digest}. No USB connection was opened."
        )
    return helper


def guarded_connection(cc, probe_module, port, helper):
    """Build a connection with an enforced outbound-command allowlist."""
    validate_helper(helper)

    class ReadOnlyConnection(cc.CC3200Connection):
        def __init__(self):
            super().__init__(
                port, reset=cc.Pincfg(False, "none"),
                sop2=cc.Pincfg(False, "none"), device="cc3200",
            )
            self.helper_sent = 0
            self.helper_executed = False
            self.flash_size = None

        @contextmanager
        def _serial_timeout(self, timeout=None):
            previous = self.port.timeout
            try:
                if timeout is not None:
                    self.port.timeout = timeout
                yield self.port
            finally:
                self.port.timeout = previous

        def _read_ack(self, timeout=None):
            seconds = self.port.timeout if timeout is None else timeout
            try:
                probe_module.receive_ack(
                    self.port.transport, "COMMAND ACK", seconds,
                )
            except probe_module.ProbeError as exc:
                raise cc.CC3200Error(str(exc)) from exc
            return True

        def _read_packet(self, timeout=None):
            with self._serial_timeout(timeout):
                header = self.port.read(3)
                if len(header) != 3:
                    raise cc.CC3200Error(
                        f"Response header incomplete: {len(header)}/3 bytes"
                    )
                length = struct.unpack(">H", header[:2])[0]
                if not 3 <= length <= CHUNK_SIZE + 2:
                    raise cc.CC3200Error(f"Invalid response length: {length}")
                data = self.port.read(length - 2)
                if len(data) != length - 2:
                    raise cc.CC3200Error(
                        f"Response incomplete: {len(data)}/{length - 2} bytes"
                    )
                if sum(data) & 0xFF != header[2]:
                    raise cc.CC3200Error("Response checksum mismatch")
            self._send_ack()
            return data

        def _try_breaking(self, tries=5, timeout=1):
            # The second handshake happens automatically after the UART switch.
            # Keep BREAK asserted while receiving ACK, as in the proven probe.
            raw = self.port.transport
            report("APPS HANDSHAKE: holding BREAK while waiting for ACK...")
            try:
                raw.break_condition = True
                probe_module.receive_ack(
                    raw, "APPS INITIAL ACK", max(5.0, tries * timeout),
                )
            finally:
                raw.break_condition = False
            # Upstream immediately sends GET_VERSION after this return.

        def _send_packet(self, data, timeout=None):
            if not data:
                raise BackupError("Blocked an empty command")
            opcode = data[0]
            helper_advance = 0
            if opcode in (0x27, 0x2F):
                allowed = len(data) == 1
            elif opcode == 0x33:
                allowed = (data == b"\x33" + struct.pack(">I", 26666667)
                           and self.helper_sent == 0
                           and not self.helper_executed)
            elif opcode == 0x2D:
                allowed = False
                if len(data) >= 13 and not self.helper_executed:
                    storage, offset, size = struct.unpack(">III", data[1:13])
                    allowed = (
                        storage == 0 and offset == self.helper_sent
                        and 1 <= size <= 4080
                        and offset + size <= len(helper)
                        and len(data) == 13 + size
                        and data[13:] == helper[offset:offset + size]
                    )
                    if allowed:
                        helper_advance = size
            elif opcode == 0x32:
                allowed = (len(data) == 1 and not self.helper_executed
                           and self.helper_sent == len(helper))
            elif opcode == 0x31:
                allowed = (data == b"\x31" + struct.pack(">I", 2)
                           and self.helper_executed)
            elif opcode == 0x2C:
                allowed = False
                if len(data) == 13 and self.helper_executed and self.flash_size:
                    storage, offset, size = struct.unpack(">III", data[1:])
                    allowed = (storage == 2 and 1 <= size <= CHUNK_SIZE
                               and offset + size <= self.flash_size)
            else:
                allowed = False
            if not allowed:
                raise BackupError(f"Blocked unexpected command 0x{opcode:02x}")
            super()._send_packet(data, timeout)
            self.helper_sent += helper_advance
            if opcode == 0x32:
                self.helper_executed = True

        def connect(self):
            raise BackupError("Native serial connect is disabled; use the USB probe")

        def reboot_to_app(self):
            raise BackupError("Starting the Pico application is disabled during backup")

    return ReadOnlyConnection()


def detect_flash(connection):
    storage = connection._get_storage_list()
    if not storage.sflash:
        raise BackupError("The loader did not report serial flash")
    info = connection._get_storage_info(2)
    size = info.block_size * info.block_count
    if info.block_size != 4096 or not 0 < size <= MAX_FLASH_SIZE:
        raise BackupError(
            f"Unexpected flash geometry: {info.block_count} blocks of "
            f"{info.block_size} bytes. No dump will be marked verified."
        )
    connection.flash_size = size
    report(f"FLASH: {size:,} bytes ({info.block_count} x {info.block_size}-byte blocks)")
    return {"block_size": info.block_size, "block_count": info.block_count,
            "size_bytes": size}


def read_pass(connection, destination, total, pass_number, reference=None,
              progress=report):
    """Stream every byte, validating each packet and comparing pass two."""
    digest = hashlib.sha256()
    started = time.monotonic()
    last_report = started
    if reference is not None:
        reference.seek(0)
    offset = 0
    progress(f"READ {pass_number}/2: starting {total:,}-byte flash read")
    while offset < total:
        count = min(CHUNK_SIZE, total - offset)
        try:
            chunk = connection._read_chunk(offset, count, 2)
        except Exception as exc:
            raise BackupError(
                f"Read {pass_number}, flash offset 0x{offset:06x} "
                f"({offset:,} bytes completed): {exc}"
            ) from exc
        if len(chunk) != count:
            raise BackupError(f"Short flash chunk at offset 0x{offset:x}")
        if destination.write(chunk) != len(chunk):
            raise BackupError("Short backup-file write")
        digest.update(chunk)
        if reference is not None:
            previous = reference.read(count)
            if previous != chunk:
                raise BackupError(
                    f"The two reads differ in the chunk at offset 0x{offset:x}. "
                    "Partial files are retained; no verified backup was published."
                )
        offset += count
        now = time.monotonic()
        if now - last_report >= 2 or offset == total:
            progress(f"READ {pass_number}/2: {offset:,}/{total:,} bytes "
                     f"({offset * 100 / total:.0f}%)")
            last_report = now
    if reference is not None and reference.read(1):
        raise BackupError("First-pass file has unexpected trailing data")
    destination.flush()
    os.fsync(destination.fileno())
    return {"sha256": digest.hexdigest(), "size_bytes": offset,
            "seconds": round(time.monotonic() - started, 3)}


def disk_digest(file):
    file.flush()
    file.seek(0)
    digest = hashlib.sha256()
    size = 0
    while True:
        chunk = file.read(1024 * 1024)
        if not chunk:
            break
        digest.update(chunk)
        size += len(chunk)
    return digest.hexdigest(), size


def sudo_owner():
    if not hasattr(os, "geteuid") or os.geteuid() != 0:
        return None
    uid, gid = os.environ.get("SUDO_UID"), os.environ.get("SUDO_GID")
    if uid is None and gid is None:
        return None
    if not uid or not gid or not uid.isdecimal() or not gid.isdecimal():
        raise BackupError("Invalid SUDO_UID/SUDO_GID; cannot set backup ownership")
    import pwd
    owner = pwd.getpwuid(int(uid))
    if os.environ.get("SUDO_USER") not in (None, owner.pw_name):
        raise BackupError("SUDO_USER and SUDO_UID do not identify the same user")
    return int(uid), int(gid)


def new_private_file(path, owner):
    flags = os.O_CREAT | os.O_EXCL | os.O_RDWR | getattr(os, "O_NOFOLLOW", 0)
    fd = os.open(path, flags, 0o600)
    try:
        if owner is not None:
            os.fchown(fd, *owner)
        return os.fdopen(fd, "w+b")
    except BaseException:
        os.close(fd)
        raise


class BackupFiles:
    def __init__(self, output, owner):
        self.output = Path(output)
        if not self.output.is_absolute():
            raise BackupError("--output must be an absolute path")
        if "\n" in str(self.output) or "\r" in str(self.output):
            raise BackupError("Output paths cannot contain newlines")
        if not self.output.parent.is_dir():
            raise BackupError(f"Create the output directory first: {self.output.parent}")
        self.partial = Path(str(self.output) + ".partial")
        self.verify_partial = Path(str(self.output) + ".verify.partial")
        self.manifest = Path(str(self.output) + ".json")
        self.checksum = Path(str(self.output) + ".sha256")
        self.owner = owner
        self.first = None
        self.second = None
        self.published = False
        for path in (self.output, self.partial, self.verify_partial,
                     self.manifest, self.checksum):
            if os.path.lexists(path):
                raise BackupError(f"Refusing to overwrite an existing file: {path}")

    def __enter__(self):
        self.first = new_private_file(self.partial, self.owner)
        try:
            self.second = new_private_file(self.verify_partial, self.owner)
        except BaseException:
            self.first.close()
            raise
        return self

    def __exit__(self, exc_type, exc, traceback):
        for file in (self.first, self.second):
            if file is not None:
                file.close()

    def publish(self, details):
        expected = details["reads"][0]["sha256"]
        total = details["flash"]["size_bytes"]
        for file in (self.first, self.second):
            if disk_digest(file) != (expected, total):
                raise BackupError("An on-disk backup differs from the verified reads")
            os.fsync(file.fileno())
        # Hard-link creation is atomic and fails if the final path exists.
        # Keep the original inode, permissions and invoking-user ownership.
        os.link(self.partial, self.output)
        self.published = True
        self.partial.unlink()
        self.verify_partial.unlink()
        for path, data in (
            (self.checksum, f"{expected}  {self.output.name}\n".encode()),
            (self.manifest, (json.dumps(details, indent=2, sort_keys=True) + "\n").encode()),
        ):
            with new_private_file(path, self.owner) as file:
                file.write(data)
                file.flush()
                os.fsync(file.fileno())


def load_dependencies():
    try:
        import pico_uart_probe as probe_module
    except ImportError as exc:
        raise BackupError(
            "Place pico_flash_backup_v2.py beside the working pico_uart_probe.py "
            "in your Downloads folder."
        ) from exc
    try:
        from cc3200tool import cc
    except ImportError as exc:
        raise BackupError(
            "cc3200tool could not be imported. Use the absolute path to Python "
            "in the same venv where cc3200tool already works."
        ) from exc
    helper = validate_helper(cc.dll_data(HELPER_NAME))
    try:
        version = metadata.version("cc3200tool")
    except metadata.PackageNotFoundError:
        version = "source checkout"
    return probe_module, cc, helper, version


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, help="Absolute path for verified .bin backup")
    parser.add_argument("--power-wait", type=float, default=60,
                        help="Seconds to wait for Pico power-on (default: 60)")
    args = parser.parse_args(argv)
    if not 1 <= args.power_wait <= 300:
        parser.error("--power-wait must be between 1 and 300 seconds")
    transport = None
    files = None
    phase = "preflight"
    try:
        probe_module, cc, helper, version = load_dependencies()
        files = BackupFiles(args.output, sudo_owner())
        report("BACKUP v2: queued direct CH340 USB; flash writes and erases blocked")
        report(f"SRAM HELPER: verified {HELPER_NAME}, {len(helper):,} bytes")
        report(f"OUTPUT: {files.output}")
        details = {
            "backup_revision": BACKUP_REVISION,
            "started_utc": datetime.now(timezone.utc).isoformat(),
            "cc3200tool_version": version,
            "helper": {"name": HELPER_NAME, "size_bytes": len(helper),
                       "sha256": HELPER_SHA256, "destination": "SRAM only"},
            "flash_modified": False,
            "verification": "two full flash reads, byte-for-byte comparison and disk SHA-256",
            "warnings": [],
        }
        with files:
            transport = probe_module.CH340USBTransport()
            reads_verified = False
            try:
                with USBSession(transport) as raw:
                    report("TRANSPORT: " + raw.description)
                    phase = "initial connection"

                    def connection_report(line):
                        if line == "No Pico flash was read or written.":
                            return
                        report(line.replace("SUCCESS: complete GET_VERSION", "CONNECTION: complete GET_VERSION"))

                    payload = probe_module.probe(raw, args.power_wait, report=connection_report)
                    port = USBSerialPort(raw)
                    connection = guarded_connection(cc, probe_module, port, helper)
                    connection.vinfo = cc.CC3x00VersionInfo.from_packet(payload)
                    if not connection.vinfo.is_cc3200 or connection.vinfo.bootloader[1] != 4:
                        raise BackupError(f"Unexpected device/version: {connection.vinfo}")
                    details["bootloader_version"] = list(connection.vinfo.bootloader)
                    details["initial_version_payload"] = payload.hex()
                    phase = "RAM loader setup"
                    report("LOADER: switching UART and loading the temporary helper into SRAM")
                    connection.switch_to_nwp_bootloader()
                    if not connection.helper_executed:
                        raise BackupError("The expected SRAM helper was not executed")
                    details["apps_version"] = repr(connection.vinfo_apps)
                    phase = "queuing USB receive buffers"
                    with QueuedUSBReceive(raw) as receiver:
                        port.transport = receiver
                        report("USB RX: 8 transfers of 4096 bytes queued before flash queries")
                        phase = "flash capacity detection"
                        details["flash"] = detect_flash(connection)
                        total = details["flash"]["size_bytes"]
                        phase = "first flash read"
                        first = read_pass(connection, files.first, total, 1)
                        phase = "second flash read and comparison"
                        second = read_pass(connection, files.second, total, 2, files.first)
                        if first["sha256"] != second["sha256"]:
                            raise BackupError("The complete flash-read hashes differ")
                        details["reads"] = [first, second]
                        reads_verified = True
                    port.transport = raw
            except (probe_module.CH340USBError, QueuedUSBReceiveError):
                if not reads_verified or not transport.details.get("cleanup_errors"):
                    raise
                # Successful reads remain valid if ordinary-driver reattachment
                # fails during context-manager exit. Record that separately.
                details["warnings"].append("USB cleanup needs a cable reconnect")
            details["usb"] = transport.details
            phase = "on-disk verification and publication"
            details["completed_utc"] = datetime.now(timezone.utc).isoformat()
            files.publish(details)
        report("VERIFIED: both complete flash reads match byte-for-byte")
        report("BACKUP: " + str(files.output))
        report("BYTES: " + str(details["flash"]["size_bytes"]))
        report("SHA256: " + details["reads"][0]["sha256"])
        report("DETAILS: " + str(files.manifest))
        report("No Pico flash was written or erased. The helper runs only in RAM.")
        if transport.details.get("cleanup_errors"):
            report("USB CLEANUP: backup is verified; disconnect/reconnect USB before further use")
            for message in transport.details["cleanup_errors"]:
                report("  " + message)
        return 0
    except KeyboardInterrupt:
        report(f"STOPPED during {phase}: interrupted; any partial files are retained")
        if files is not None and files.published:
            report("The verified binary was saved: " + str(files.output))
        return 130
    except Exception as exc:
        report(f"STOPPED during {phase}: {type(exc).__name__}: {exc}")
        if files is not None and files.published:
            report("The verified binary was saved, but a later output step failed: " + str(files.output))
        elif files is not None:
            report("No verified backup was published. Any .partial files are retained.")
        return 1
    finally:
        if transport is not None:
            report("USB DETAILS: " + json.dumps(transport.details, sort_keys=True))


if __name__ == "__main__":
    result = main()
    if any(getattr(receiver.raw, "_queued_receive_cleanup_blocked", False)
           for receiver in _RETAINED_USB_RECEIVERS):
        # This exceptional path occurs only if libusb cannot complete transfer
        # cancellation. Let the OS reclaim this process's USB handles instead
        # of running Python finalizers on buffers still owned by native I/O.
        sys.stdout.flush()
        sys.stderr.flush()
        os._exit(result or 1)
    sys.exit(result)
