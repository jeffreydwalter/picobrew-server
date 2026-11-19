#!/usr/bin/env python3
"""Read-only CC3200 connection probe for an already-wired PicoBrew.

Hold BREAK before the user powers up the Pico, receive its initial ACK while
BREAK is still asserted, release BREAK, and immediately request version info.
Only GET_VERSION (0x2f) and the response acknowledgement are sent to the Pico.
No flash operation, RAM upload, firmware command, or automatic reset is used.

Sources:
  https://www.ti.com/lit/pdf/swru577 (section 5.3.1)
  https://github.com/mongoose-os/mos/blob/master/cli/flash/cc32xx/rom_client.go
  https://github.com/chiefwigms/picobrew_pico/issues/354
  https://github.com/torvalds/linux/blob/master/drivers/usb/serial/ch341.c
  https://github.com/libusb/libusb/blob/master/libusb/os/darwin_usb.c

Optional --transport usb uses PyUSB/libusb instead of a macOS serial driver.
Install its dependencies in the venv: python -m pip install pyusb libusb-package
USB capture requires sudo on macOS; it is limited to one 1a86:7523 adapter.
It releases the interface and permits ordinary driver reattachment on exit.
Keep Pico mains off while opening/configuring either transport.
"""

import argparse
import ctypes
import json
import os
import struct
import sys
import time


ACK = b"\x00\xcc"
NACK = b"\x00\x33"
GET_VERSION = b"\x00\x03\x2f\x2f"


class ProbeError(Exception):
    """A failed connection stage, without any flash operation."""


def hex_bytes(data):
    return data.hex(" ") if data else "NO BYTES"


class SerialTransport:
    def __init__(self, port, baud):
        self.port_name = port
        self.baud = baud
        self.serial = None
        self.description = f"serial port {port}; requested baud {baud}"

    def __enter__(self):
        try:
            import serial
            from serial.tools import list_ports
        except ImportError as exc:
            raise ProbeError("pySerial is missing: python -m pip install pyserial") from exc
        if not self.port_name:
            matches = [p.device for p in list_ports.comports()
                       if p.vid == 0x1A86 and p.pid == 0x7523]
            if len(matches) != 1:
                raise ProbeError(
                    f"Found {len(matches)} CH340 serial ports; specify --port explicitly."
                )
            self.port_name = matches[0]
        self.description = (
            f"serial port {self.port_name}; requested baud {self.baud}; "
            "8N1; flow control off; DTR/RTS deasserted"
        )
        # Set all properties before opening. Keep timeout and baud fixed once
        # connected: pySerial timeout setters reconfigure the macOS serial port.
        self.serial = serial.Serial(
            port=None, baudrate=self.baud, timeout=0.05, write_timeout=2,
            bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE, xonxoff=False,
            rtscts=False, dsrdtr=False,
        )
        self.serial.dtr = False
        self.serial.rts = False
        self.serial.port = self.port_name
        try:
            self.serial.open()
        except BaseException:
            self.serial.close()
            raise
        return self

    def __exit__(self, exc_type, exc, traceback):
        if self.serial is not None and self.serial.is_open:
            try:
                if self.serial.break_condition:
                    self.serial.break_condition = False
            except Exception as cleanup_error:
                if exc_type is None:
                    raise
                print(f"CLEANUP: could not release BREAK: {cleanup_error}; "
                      "disconnect/reconnect the USB cable after stopping.",
                      file=sys.stderr)
            finally:
                self.serial.close()

    def read(self, size=1):
        return self.serial.read(size)

    def write(self, data):
        return self.serial.write(data)

    def reset_input_buffer(self):
        self.serial.reset_input_buffer()

    @property
    def break_condition(self):
        return self.serial.break_condition

    @break_condition.setter
    def break_condition(self, enabled):
        # This must happen AFTER open(): pre-setting the flag alone does not
        # assert BREAK in pySerial's POSIX open implementation.
        self.serial.break_condition = enabled



class CH340USBError(RuntimeError):
    """Failure of the USB transport, distinct from a missing Pico response."""


class CH340USBTransport:
    VID = 0x1A86
    PID = 0x7523
    READ_TIMEOUT_MS = 50
    CONTROL_TIMEOUT_MS = 1000
    WRITE_TIMEOUT_MS = 1000
    REQUESTED_BAUD = 921600
    # 48 MHz / (4 * 13), matching Linux's best divisor for 921600 baud.
    BAUD_REGISTER = 0xF387
    EFFECTIVE_BAUD = 48000000 / (4 * 13)
    LINE_CONTROL = 0xC3  # RX enabled, TX enabled, 8 data bits, no parity, 1 stop.

    def __init__(self):
        self.details = {
            "stage": "not opened",
            "usb_id": "1a86:7523",
            "requested_baud": self.REQUESTED_BAUD,
            "calculated_baud": self.EFFECTIVE_BAUD,
            "baud_register_target": "f387",
            "line_control_target": "c3",
            "warnings": [],
            "cleanup_errors": [],
        }
        self.description = "CH340 direct USB (not opened)"
        self._core = None
        self._util = None
        self._backend = None
        self._device = None
        self._interface = None
        self._in_ep = None
        self._out_ep = None
        self._packet_size = None
        self._buffer = bytearray()
        self._claimed = False
        self._detached = False
        self._open = False
        self._break = False
        self._break_register_initial = None

    def __enter__(self):
        if self._device is not None or self._open:
            raise CH340USBError("This transport is already open.")
        if sys.platform not in ("darwin", "linux"):
            raise CH340USBError("This diagnostic supports macOS and Linux only.")
        if sys.platform == "darwin" and os.geteuid() != 0:
            raise CH340USBError(
                "Direct USB capture on macOS requires root. Run this reviewed "
                "diagnostic with sudo and the absolute path to the venv Python. "
                "Do not uninstall drivers or change SIP."
            )
        try:
            import usb.backend.libusb1
            import usb.core
            import usb.util
        except ImportError as exc:
            raise CH340USBError(
                "USB dependencies are missing: python -m pip install pyusb libusb-package"
            ) from exc

        self._core = usb.core
        self._util = usb.util
        # The bundled library works even when sudo strips dylib search paths.
        try:
            import libusb_package
        except ImportError:
            libusb_package = None
        if libusb_package is not None:
            self._backend = libusb_package.get_libusb1_backend()
            if self._backend is not None:
                self.details["libusb_source"] = "libusb-package"
        if self._backend is None:
            self._backend = usb.backend.libusb1.get_backend()
        if self._backend is None and sys.platform == "darwin":
            # sudo may give ctypes a reduced dylib search environment.  Use
            # existing standard Homebrew paths without changing the environment.
            for path in ("/opt/homebrew/lib/libusb-1.0.dylib",
                         "/usr/local/lib/libusb-1.0.dylib"):
                if os.path.isfile(path):
                    self._backend = usb.backend.libusb1.get_backend(
                        find_library=lambda _name, selected=path: selected,
                    )
                    if self._backend is not None:
                        self.details["libusb_path"] = path
                        break
        if self._backend is None:
            raise CH340USBError(
                "PyUSB could not load libusb-1.0. In the venv, run: "
                "python -m pip install --upgrade pyusb libusb-package"
            )
        self._check_libusb_version()

        try:
            self.details["stage"] = "enumerate adapter"
            devices = list(self._core.find(
                find_all=True, idVendor=self.VID, idProduct=self.PID,
                backend=self._backend,
            ))
            if not devices:
                raise CH340USBError("No USB adapter with VID:PID 1a86:7523 was found.")
            if len(devices) != 1:
                raise CH340USBError(
                    f"Found {len(devices)} adapters with VID:PID 1a86:7523; "
                    "leave only the intended SparkFun adapter connected."
                )
            self._device = devices[0]
            self.details.update({
                "bus": self._device.bus,
                "address": self._device.address,
            })

            # Reading the current configuration is non-destructive.  Do not call
            # set_configuration(): that could reset an already configured device.
            self.details["stage"] = "inspect active configuration"
            try:
                config = self._device.get_active_configuration()
            except self._core.USBError as exc:
                raise CH340USBError(
                    "Cannot read an active USB configuration. Reconnect the "
                    "adapter and try again; this probe does not set/reset USB "
                    "configurations automatically."
                ) from exc
            self.details["configuration"] = config.bConfigurationValue

            candidates = []
            for interface in config:
                if (interface.bInterfaceClass != 0xFF
                        or interface.bAlternateSetting != 0):
                    continue
                ins, outs = [], []
                for endpoint in interface:
                    if self._util.endpoint_type(endpoint.bmAttributes) != 2:
                        continue
                    if self._util.endpoint_direction(endpoint.bEndpointAddress) == 0x80:
                        ins.append(endpoint)
                    else:
                        outs.append(endpoint)
                if len(ins) == len(outs) == 1:
                    candidates.append((interface, ins[0], outs[0]))
            if len(candidates) != 1:
                raise CH340USBError(
                    "Expected exactly one vendor-class interface with one bulk "
                    "IN and one bulk OUT endpoint; refusing unfamiliar hardware."
                )
            interface, in_ep, out_ep = candidates[0]
            self._interface = interface.bInterfaceNumber
            self._in_ep = in_ep.bEndpointAddress
            self._out_ep = out_ep.bEndpointAddress
            self._packet_size = int(in_ep.wMaxPacketSize) & 0x7FF
            if not 1 <= self._packet_size <= 1024:
                raise CH340USBError("Invalid bulk IN endpoint packet size.")
            self.details.update({
                "interface": self._interface,
                "bulk_in": f"0x{self._in_ep:02x}",
                "bulk_out": f"0x{self._out_ep:02x}",
                "bulk_in_packet_size": self._packet_size,
            })

            try:
                self.details["stage"] = "inspect current USB driver"
                active = self._device.is_kernel_driver_active(self._interface)
            except (NotImplementedError, self._core.USBError) as exc:
                raise CH340USBError(
                    "Cannot determine whether the macOS/Linux driver owns the "
                    "USB interface; refusing to assume it is free."
                ) from exc
            self.details["kernel_driver_was_active"] = bool(active)
            if active:
                try:
                    self.details["stage"] = "capture USB device"
                    self._device.detach_kernel_driver(self._interface)
                    self._detached = True
                except (NotImplementedError, self._core.USBError) as exc:
                    raise CH340USBError(
                        "Exclusive USB capture failed before any Pico commands. "
                        "On macOS use current libusb and run as root. No driver "
                        "uninstallation or SIP change is required by this probe."
                    ) from exc
            self.details["stage"] = "claim USB interface"
            self._util.claim_interface(self._device, self._interface)
            self._claimed = True
            self._configure()
            self._open = True
            self.details["stage"] = "ready"
            self.description = (
                f"CH340 direct USB 1a86:7523, bus {self._device.bus}, "
                f"address {self._device.address}, interface {self._interface}; "
                f"921600 requested / {self.EFFECTIVE_BAUD:.3f} calculated baud"
            )
            return self
        except BaseException as exc:
            self._record_failure(exc)
            self._cleanup()
            raise

    def _check_libusb_version(self):
        """Best-effort version query; PyUSB does not expose a public wrapper."""
        class Version(ctypes.Structure):
            _fields_ = [
                ("major", ctypes.c_uint16),
                ("minor", ctypes.c_uint16),
                ("micro", ctypes.c_uint16),
                ("nano", ctypes.c_uint16),
                ("rc", ctypes.c_char_p),
                ("describe", ctypes.c_char_p),
            ]
        try:
            library = self._backend.lib
            # Create a separate correctly typed callable.  Calling an untyped
            # ctypes symbol would default to a 32-bit int return and could
            # truncate the pointer on a 64-bit Mac.  Do not change PyUSB's
            # shared function signature either.
            get_version = ctypes.CFUNCTYPE(ctypes.POINTER(Version))(
                ("libusb_get_version", library)
            )
            pointer = get_version()
            if not pointer:
                raise ValueError("libusb_get_version returned a null pointer")
            version = pointer.contents
            parts = (int(version.major), int(version.minor), int(version.micro))
            self.details["libusb_version"] = ".".join(map(str, parts))
        except (AttributeError, TypeError, ValueError, OSError) as exc:
            self.details["libusb_version"] = "unavailable"
            self.details["warnings"].append(
                f"Could not query libusb version: {type(exc).__name__}. "
                "macOS capture requires libusb 1.0.25 or newer."
            )
            return
        if sys.platform == "darwin" and parts < (1, 0, 25):
            raise CH340USBError(
                f"libusb {self.details['libusb_version']} is too old for macOS "
                "driver capture; version 1.0.25 or newer is required."
            )

    def _control_in(self, request, value, length):
        self.details["last_control_transfer"] = {
            "type": "in", "request": f"0x{request:02x}",
            "value": f"0x{value:04x}", "length": length,
        }
        result = bytes(self._device.ctrl_transfer(
            0xC0, request, value, 0, length, timeout=self.CONTROL_TIMEOUT_MS,
        ))
        if len(result) != length:
            raise CH340USBError(
                f"USB request 0x{request:02x}/0x{value:04x} returned "
                f"{len(result)} bytes; expected {length}."
            )
        return result

    def _control_out(self, request, value, index):
        self.details["last_control_transfer"] = {
            "type": "out", "request": f"0x{request:02x}",
            "value": f"0x{value:04x}", "index": f"0x{index & 0xFFFF:04x}",
        }
        result = self._device.ctrl_transfer(
            0x40, request, value, index & 0xFFFF, None,
            timeout=self.CONTROL_TIMEOUT_MS,
        )
        if result != 0:
            raise CH340USBError("Unexpected data count for a USB control write.")

    def _configure(self):
        self.details["stage"] = "read CH340 version"
        version = self._control_in(0x5F, 0, 2)
        self.details["chip_version_reply"] = version.hex(" ")
        self.details["chip_version"] = f"0x{version[0]:02x}"
        if version[0] < 0x30:
            raise CH340USBError(
                f"Chip version 0x{version[0]:02x} uses older register semantics; "
                "this CH340C diagnostic intentionally does not support it."
            )

        # Verify real BREAK support before changing any adapter configuration.
        self.details["stage"] = "verify real BREAK support"
        try:
            initial = self._control_in(0x95, 0x1805, 2)
        except self._core.USBError as exc:
            raise CH340USBError(
                "Reading the CH340 BREAK registers failed. This probe does not "
                "simulate BREAK by changing baud rate, because that can lose "
                "the CC3200's immediate acknowledgment."
            ) from exc
        self._break_register_initial = initial
        self.details["break_registers_before_init"] = initial.hex(" ")

        self.details["stage"] = "initialize CH340 UART"
        self._control_out(0xA1, 0, 0)
        self._control_out(0x9A, 0x1312, self.BAUD_REGISTER)
        self.details["baud_register_written"] = "f387"
        self._control_out(0x9A, 0x2518, self.LINE_CONTROL)
        self.details["line_control_written"] = "c3"
        self._control_out(0x9A, 0x2727, 0)
        # CH341 modem outputs are active-low.  Linux sends ~mcr; mcr=0
        # deasserts both DTR and RTS, therefore wValue=0xffff, wIndex=0.
        self._control_out(0xA4, 0xFFFF, 0)
        self.details["modem_control"] = "DTR and RTS deasserted (0xa4/0xffff/0)"

        # Establish TX idle explicitly, using the same registers as Linux.
        self.details["stage"] = "release BREAK and read back configuration"
        self._set_break(False)
        self.details["break_registers_ready"] = self._control_in(
            0x95, 0x1805, 2,
        ).hex(" ")
        for name, registers in (("baud_register_readback", 0x1312),
                                ("line_register_readback", 0x2518),
                                ("flow_register_readback", 0x2727)):
            try:
                self.details[name] = self._control_in(0x95, registers, 2).hex(" ")
            except self._core.USBError as exc:
                # Some revisions do not expose every writable register for
                # reads.  Record that limitation rather than invent a readback.
                self.details[name] = f"unavailable: {exc}"
                self.details["warnings"].append(f"{name} unavailable")

    @property
    def break_condition(self):
        return self._break

    @break_condition.setter
    def break_condition(self, enabled):
        self._require_open()
        self._set_break(bool(enabled))

    def _set_break(self, enabled):
        registers = bytearray(self._control_in(0x95, 0x1805, 2))
        if enabled:
            registers[0] &= ~0x01
            registers[1] &= ~0x40
        else:
            registers[0] |= 0x01
            registers[1] |= 0x40
        self._control_out(0x9A, 0x1805, registers[0] | registers[1] << 8)
        self._break = enabled

    def _require_open(self):
        if not self._open:
            raise CH340USBError("USB transport is not open.")

    def read(self, size=1):
        """Return up to size bytes; perform at most one 50 ms USB read.

        A caller asking for one byte must still provide enough USB receive
        space for a complete endpoint packet.  Surplus bytes are retained.
        Short results and b'' on timeout follow the serial-probe convention.
        """
        self._require_open()
        if not isinstance(size, int) or size < 0:
            raise ValueError("size must be a non-negative integer")
        if size == 0:
            return b""
        if len(self._buffer) < size:
            # One complete USB packet per read also avoids a larger libusb
            # transfer timing out after already receiving a full short batch.
            length = self._packet_size
            try:
                packet = self._device.read(
                    self._in_ep, length, timeout=self.READ_TIMEOUT_MS,
                )
                self._buffer.extend(packet)
            except self._core.USBTimeoutError:
                pass
        result = bytes(self._buffer[:size])
        del self._buffer[:size]
        return result

    def write(self, data):
        self._require_open()
        if self._break:
            raise CH340USBError("Refusing to transmit while BREAK is asserted.")
        if not isinstance(data, (bytes, bytearray, memoryview)):
            raise TypeError("data must be bytes-like")
        data = bytes(data)
        if not data:
            return 0
        count = self._device.write(
            self._out_ep, data, timeout=self.WRITE_TIMEOUT_MS,
        )
        if count != len(data):
            raise CH340USBError(f"Short USB write: {count} of {len(data)} bytes.")
        return count

    def reset_input_buffer(self):
        """Drain at most 250 ms / 64 packets, preventing an endless read loop."""
        self._require_open()
        self._buffer.clear()
        deadline = time.monotonic() + 0.250
        for _ in range(64):
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return
            try:
                self._device.read(
                    self._in_ep, self._packet_size,
                    timeout=max(1, min(20, int(remaining * 1000))),
                )
            except self._core.USBTimeoutError:
                return

    def _cleanup(self):
        errors = self.details["cleanup_errors"]
        if self._device is None:
            return
        # Always attempt to release BREAK before dropping ownership.  This is
        # also useful after an exception during a BREAK operation.
        if self._claimed and self._break_register_initial is not None:
            try:
                self._set_break(False)
            except Exception as exc:
                errors.append(f"release BREAK: {exc}")
        if self._claimed:
            try:
                self._util.release_interface(self._device, self._interface)
            except Exception as exc:
                errors.append(f"release USB interface: {exc}")
            finally:
                self._claimed = False
        if self._detached:
            try:
                self._device.attach_kernel_driver(self._interface)
            except Exception as exc:
                errors.append(f"reattach original USB driver: {exc}")
            finally:
                self._detached = False
        try:
            self._util.dispose_resources(self._device)
        except Exception as exc:
            errors.append(f"dispose USB resources: {exc}")
        self._open = False
        self._device = None
        self._buffer.clear()

    def _record_failure(self, exc):
        self.details["failure"] = {
            "stage": self.details.get("stage"),
            "error": f"{type(exc).__name__}: {exc}",
            "last_control_transfer": self.details.get("last_control_transfer"),
        }

    def __exit__(self, exc_type, exc_value, traceback):
        if exc_value is not None:
            self._record_failure(exc_value)
        self._cleanup()
        if exc_type is None and self.details["cleanup_errors"]:
            raise CH340USBError(
                "USB cleanup was incomplete: "
                + "; ".join(self.details["cleanup_errors"])
                + ". Disconnect and reconnect the adapter to restore its "
                  "ordinary macOS serial port."
            )
        return False


def receive_ack(port, stage, seconds, clock=time.monotonic):
    deadline = clock() + seconds
    received = bytearray()
    while clock() < deadline:
        data = port.read(1)
        if not data:
            continue
        received.extend(data)
        if received[-2:] == ACK:
            return bytes(received), clock()
        if received[-2:] == NACK:
            raise ProbeError(f"{stage}: NACK received. RX: {hex_bytes(received)}")
        if len(received) >= 1024:
            raise ProbeError(
                f"{stage}: received 1024 bytes without an ACK. "
                f"First 64: {hex_bytes(received[:64])}"
            )
    raise ProbeError(f"{stage}: timeout after {seconds:g}s. RX: {hex_bytes(received)}")


def receive_exact(port, size, stage, seconds=3, clock=time.monotonic):
    deadline = clock() + seconds
    received = bytearray()
    while len(received) < size and clock() < deadline:
        received.extend(port.read(size - len(received)))
    if len(received) != size:
        raise ProbeError(
            f"{stage}: received {len(received)}/{size} bytes. "
            f"RX: {hex_bytes(received)}"
        )
    return bytes(received)


def probe(port, power_wait=60, report=print, clock=time.monotonic):
    port.reset_input_buffer()
    try:
        port.break_condition = True
        report("BREAK asserted. Power on the Pico or press and release its blue RESET button NOW.")
        report(f"Listening for {power_wait:g} seconds. Do not press Return; do not reset again after the handshake starts.")
        boot_rx, ack_time = receive_ack(port, "INITIAL ACK", power_wait, clock)
        # No input(), long read, baud/timeout setter, or print before this write.
        port.break_condition = False
        sent = port.write(GET_VERSION)
        command_time = clock()
        if sent != len(GET_VERSION):
            raise ProbeError(f"GET_VERSION: short write ({sent}/4 bytes).")
        report(f"INITIAL RX: {hex_bytes(boot_rx)}")
        report(f"GET_VERSION TX: {hex_bytes(GET_VERSION)}")
        report(f"ACK-to-command-write: {(command_time - ack_time) * 1000:.1f} ms")
        if command_time - ack_time >= 5:
            raise ProbeError("Command write exceeded the bootloader's 5-second window.")
        command_rx, _ = receive_ack(port, "COMMAND ACK", 3, clock)
        report(f"COMMAND RX: {hex_bytes(command_rx)}")
        header = receive_exact(port, 3, "VERSION HEADER", clock=clock)
        length = struct.unpack(">H", header[:2])[0]
        if length != 30:
            raise ProbeError(
                f"VERSION HEADER: expected length 30, got {length}. "
                f"RX: {hex_bytes(header)}"
            )
        payload = receive_exact(port, 28, "VERSION PAYLOAD", clock=clock)
        if sum(payload) & 0xFF != header[2]:
            raise ProbeError(f"VERSION CHECKSUM: invalid. RX: {hex_bytes(header + payload)}")
        if port.write(ACK) != len(ACK):
            raise ProbeError("HOST ACK: short write.")
        report(f"VERSION FRAME: {hex_bytes(header + payload)}")
        report("Bootloader version: " + ".".join(str(b) for b in payload[:4]))
        report("Chip-type bytes: " + hex_bytes(payload[16:20]))
        report("SUCCESS: complete GET_VERSION response, correct length and checksum.")
        report("No Pico flash was read or written.")
        return payload
    finally:
        pending_error = sys.exc_info()[0] is not None
        try:
            if port.break_condition:
                port.break_condition = False
        except Exception as cleanup_error:
            if not pending_error:
                raise
            report(f"CLEANUP: could not release BREAK: {cleanup_error}; "
                   "disconnect/reconnect the USB cable after stopping.")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--transport", choices=("serial", "usb"), default="serial",
                        help="Use macOS serial driver or direct CH340 USB (default: serial)")
    parser.add_argument("--port", help="Serial port; otherwise locate the sole CH340 adapter")
    parser.add_argument("--baud", type=int, default=921600,
                        help="Host-requested baud (default: 921600)")
    parser.add_argument("--power-wait", type=float, default=60,
                        help="Seconds to wait for power-up and ACK (default: 60)")
    args = parser.parse_args(argv)
    if not (1 <= args.power_wait <= 300) or not (1 <= args.baud <= 3000000):
        parser.error("power-wait must be 1..300 seconds; baud must be 1..3000000")
    if args.transport == "usb" and (args.port is not None or args.baud != 921600):
        parser.error("direct USB uses VID:PID 1a86:7523 and fixed 921600 settings; omit --port/--baud")
    transport = (CH340USBTransport() if args.transport == "usb"
                 else SerialTransport(args.port, args.baud))
    try:
        with transport as port:
            print("TRANSPORT:", port.description, flush=True)
            if args.transport == "usb":
                print("USB SETUP:", json.dumps(port.details, sort_keys=True), flush=True)
            probe(port, args.power_wait, report=lambda s: print(s, flush=True))
    except KeyboardInterrupt:
        print("\nStopped; releasing the adapter.", file=sys.stderr)
        return 130
    except Exception as exc:
        print(f"STOPPED: {type(exc).__name__}: {exc}", file=sys.stderr)
        return 1
    finally:
        if args.transport == "usb":
            print("USB DETAILS:", json.dumps(transport.details, sort_keys=True), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
