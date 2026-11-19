#!/usr/bin/env python3
"""Read the logical Pico filesystem through the proven queued CH340 USB path.

Keep this file beside pico_flash_backup_v2.py and pico_uart_probe.py. Use the
same Python environment as the verified backup. On macOS run with sudo and an
absolute --output path for a new JSON report. Leave USB connected and unplug
Pico mains before starting; restore mains only at the existing BREAK prompt.
Do not press RESET.

This tool never installs firmware or changes the boot selector. It permits
only the audited RAM helper, read-only filesystem operations on five named
files, and the backup transport's bounded storage queries/reads. No file
contents are saved except the eight-byte boot-selection record in the report.

The default qualification profile is the Pico backed up on 2026-11-17: a
1 MiB flash, the three exact firmware hashes below, no mcuimg3.bin, and normal
selector 1. Successful transport reads and matching that profile are reported
separately. This is a preflight check, not approval to write flash.
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

import pico_flash_backup_v2 as backup


PREFLIGHT_REVISION = 1
PROFILE_NAME = "pico-pro-backup-20261117-220719"
REFERENCE_BACKUP_SHA256 = "837a728400da02b9da4a7d563e1b67fb24d6a5af03c054963c4b1de60c72343f"
EXPECTED_FLASH = {"block_size": 4096, "block_count": 256, "size_bytes": 1048576}
EXPECTED_FILES = {
    "/sys/mcuimg.bin": {
        "size_bytes": 14956,
        "sha256": "67e22913fd7805a84d965928198f7d8cb795af125b81f1e0c29479a84980b975",
    },
    "/sys/mcuimg1.bin": {
        "size_bytes": 50504,
        "sha256": "26829ef97df46e6d3667afe8352fa86492f3ddf667371f2d19eafa8468862c63",
    },
    "/sys/mcuimg2.bin": {
        "size_bytes": 162792,
        "sha256": "e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702",
    },
}
BOOT_INFO_NAME = "/sys/mcubootinfo.bin"
OPTIONAL_IMAGE_NAME = "/sys/mcuimg3.bin"
ALLOWED_FILES = frozenset((*EXPECTED_FILES, OPTIONAL_IMAGE_NAME, BOOT_INFO_NAME))
EXPECTED_BOOT_SELECTOR = 1
EXPECTED_BOOT_MAGIC = 0xABCDDCBA
MAX_FILE_SIZE = 1048576
READ_CHUNK_SIZE = 256
FILE_CLOSE_COMMAND = b"\x22" + b"\0" * 63 + b"\x46" * 256 + b"\0"


class PreflightError(backup.BackupError):
    pass


def report(message):
    print(message, flush=True)


def _filename_bytes(name):
    if name not in ALLOWED_FILES:
        raise PreflightError("Blocked a filename outside the preflight allowlist")
    return name.encode("ascii")


def parse_file_info(payload):
    """Only decode fields established by the upstream loader protocol."""
    if len(payload) < 8:
        raise PreflightError(f"File-info response is too short: {len(payload)}/8 bytes")
    if payload[0] not in (0, 1):
        raise PreflightError("File-info response has an invalid existence flag")
    return {"exists": payload[0] == 1,
            "size_bytes": struct.unpack(">I", payload[4:8])[0]}


def build_connection(cc, probe_module, port, helper):
    """Extend the existing RAM-upload guard with a strict read-only file FSM.

    Non-filesystem commands are delegated to the unmodified backup guard.
    Only a validated filesystem command reaches the upstream packet sender
    directly. No write/open-create/delete opcode can use that route.
    """
    guarded = backup.guarded_connection(cc, probe_module, port, helper)

    class FileReadConnection(type(guarded)):
        def __init__(self):
            super().__init__()
            self.file_infos = {}
            self.open_file_name = None
            self.read_offset = 0
            self.file_phase = "idle"
            self.file_pending = None
            self.file_failed = False

        def _send_packet(self, data, timeout=None):
            if not data:
                raise PreflightError("Blocked an empty command")
            opcode = data[0]
            file_opcode = opcode in (0x21, 0x22, 0x23, 0x2A, 0x2B)
            if not file_opcode:
                if self.open_file_name is not None or self.file_pending is not None:
                    raise PreflightError("Blocked a command while a file operation is pending")
                return super()._send_packet(data, timeout)

            if (not self.helper_executed or self.flash_size is None
                    or self.file_failed or self.file_pending is not None):
                raise PreflightError("Filesystem command is not valid in the current state")

            pending = None
            opened = None
            next_phase = self.file_phase
            if opcode == 0x2A:
                if self.file_phase != "idle" or self.open_file_name is not None:
                    raise PreflightError("Cannot query file info while another file is open")
                if len(data) < 5:
                    raise PreflightError("Invalid file-info command")
                name_length = struct.unpack(">I", data[1:5])[0]
                name_blob = data[5:]
                if name_length != len(name_blob):
                    raise PreflightError("Invalid file-info filename length")
                names = {name.encode("ascii"): name for name in ALLOWED_FILES}
                if name_blob not in names:
                    raise PreflightError("Blocked a file-info filename outside the allowlist")
                pending = ("info", names[name_blob])
            elif opcode == 0x21:
                if (self.file_phase != "idle" or self.open_file_name is not None
                        or len(data) < 12 or not data.endswith(b"\0\0")):
                    raise PreflightError("Invalid file-open command/state")
                flags, token = struct.unpack(">II", data[1:9])
                if flags != 0 or token != 0:
                    raise PreflightError("Only READ file opens with token zero are allowed")
                names = {name.encode("ascii"): name for name in ALLOWED_FILES}
                name_blob = data[9:-2]
                if name_blob not in names:
                    raise PreflightError("Blocked a file-open filename outside the allowlist")
                opened = names[name_blob]
                info = self.file_infos.get(opened)
                if not info or not info["exists"]:
                    raise PreflightError("File must be confirmed present before opening")
                if not 0 <= info["size_bytes"] <= MAX_FILE_SIZE:
                    raise PreflightError("Reported file size exceeds the read bound")
                next_phase = "open_token"
            elif opcode == 0x2B:
                if (self.file_phase != "reading" or self.open_file_name is None
                        or len(data) != 9):
                    raise PreflightError("Invalid file-read command/state")
                offset, count = struct.unpack(">II", data[1:])
                size = self.file_infos[self.open_file_name]["size_bytes"]
                if (offset != self.read_offset or not 1 <= count <= READ_CHUNK_SIZE
                        or offset + count > size):
                    raise PreflightError("Blocked an out-of-order or out-of-bounds file read")
                pending = ("read", count)
            elif opcode == 0x22:
                if (data != FILE_CLOSE_COMMAND or self.file_phase != "reading"
                        or self.open_file_name is None
                        or self.read_offset != self.file_infos[self.open_file_name]["size_bytes"]):
                    raise PreflightError("Invalid file-close command/state")
                next_phase = "close_status"
            elif opcode == 0x23:
                if data != b"\x23" or self.file_phase not in ("open_status", "close_status"):
                    raise PreflightError("Status query is not valid in the current file state")
                pending = ("status", self.file_phase)

            try:
                # Bypass only the backup class's filesystem rejection, after
                # all parameters and the protocol state have been validated.
                cc.CC3200Connection._send_packet(self, data, timeout)
            except BaseException:
                self.file_failed = True
                raise
            self.file_pending = pending
            self.file_phase = next_phase
            if opened is not None:
                self.open_file_name = opened
                self.read_offset = 0

        def _read_packet(self, timeout=None):
            try:
                payload = super()._read_packet(timeout)
                pending = self.file_pending
                if pending is None:
                    return payload
                kind, value = pending
                if kind == "info":
                    self.file_infos[value] = parse_file_info(payload)
                elif kind == "read":
                    if len(payload) != value:
                        raise PreflightError(f"File read returned {len(payload)}/{value} bytes")
                    self.read_offset += value
                elif kind == "status":
                    if payload != b"\x40":
                        raise PreflightError("Filesystem operation did not return successful status 0x40")
                    if value == "open_status":
                        self.file_phase = "reading"
                    else:
                        self.open_file_name = None
                        self.read_offset = 0
                        self.file_phase = "idle"
                self.file_pending = None
                return payload
            except BaseException:
                self.file_failed = True
                raise

        def _open_file(self, filename, slfs_flags):
            try:
                super()._open_file(filename, slfs_flags)
                if self.file_phase != "open_token":
                    raise PreflightError("Unexpected state after the file-open token")
                self.file_phase = "open_status"
                if not self._get_last_status().is_ok:
                    raise PreflightError("Opening a file for read failed")
            except BaseException:
                self.file_failed = True
                raise

    return FileReadConnection()


def read_file_info(connection, name):
    name_blob = _filename_bytes(name)
    connection._send_packet(b"\x2a" + struct.pack(">I", len(name_blob)) + name_blob)
    payload = connection._read_packet()
    info = parse_file_info(payload)
    # build_connection records this independently while validating the reply.
    if connection.file_infos.get(name) != info:
        raise PreflightError("File-info guard and caller disagree")
    return dict(info)


def read_file_bytes(connection, name, expected_max=MAX_FILE_SIZE):
    _filename_bytes(name)
    info = connection.file_infos.get(name)
    if info is None:
        info = read_file_info(connection, name)
    if not info["exists"]:
        raise PreflightError(f"Cannot read missing file: {name}")
    if not 0 <= info["size_bytes"] <= min(expected_max, MAX_FILE_SIZE):
        raise PreflightError(f"Reported size for {name} exceeds the permitted read bound")
    connection._open_file_for_read(name)
    result = bytearray()
    while len(result) < info["size_bytes"]:
        count = min(READ_CHUNK_SIZE, info["size_bytes"] - len(result))
        connection._send_packet(b"\x2b" + struct.pack(">II", len(result), count))
        chunk = connection._read_packet()
        if len(chunk) != count:
            raise PreflightError(f"Short file read: {name}")
        result.extend(chunk)
    connection._close_file()
    return bytes(result)


def collect_preflight(connection, progress=report, details=None):
    """Collect live logical reads; unexpected contents are qualification issues."""
    if details is None:
        details = {}
    details["transport_read_success"] = False
    details["flash"] = backup.detect_flash(connection)
    if details["flash"] != EXPECTED_FLASH:
        raise PreflightError("Flash geometry differs from the 1 MiB device profile; file reads stopped")
    files = details.setdefault("files", {})
    for name in (*EXPECTED_FILES, OPTIONAL_IMAGE_NAME, BOOT_INFO_NAME):
        info = read_file_info(connection, name)
        files[name] = dict(info)
        if not info["exists"]:
            progress(f"FILE: {name}: absent")
            continue
        data = read_file_bytes(connection, name)
        digest = hashlib.sha256(data).hexdigest()
        files[name]["sha256"] = digest
        progress(f"FILE: {name}: {len(data):,} bytes; SHA256 {digest}")
        if name == BOOT_INFO_NAME:
            boot_info = {"size_bytes": len(data)}
            # Never dump unexpected-length content, even under this filename.
            if len(data) == 8:
                boot_info.update({
                    "hex": data.hex(" "),
                    "selector": data[0],
                    "reserved_hex": data[1:4].hex(" "),
                    "magic_le": struct.unpack("<I", data[4:8])[0],
                    "magic_hex": f"0x{struct.unpack('<I', data[4:8])[0]:08x}",
                })
                progress("LOGICAL BOOT INFO: " + boot_info["hex"])
                progress(f"BOOT SELECTOR: {boot_info['selector']}; magic {boot_info['magic_hex']}")
            details["boot_info"] = boot_info
    details["transport_read_success"] = True
    return details


def qualification(details):
    """Return every mismatch with the concrete profile, without authorizing writes."""
    issues = []
    if not details.get("transport_read_success"):
        issues.append("The live filesystem reads did not all complete successfully")
    if details.get("flash") != EXPECTED_FLASH:
        issues.append("Flash geometry does not match 256 blocks of 4096 bytes")
    files = details.get("files", {})
    for name, expected in EXPECTED_FILES.items():
        observed = files.get(name, {})
        if not observed.get("exists"):
            issues.append(f"Required file is missing or unread: {name}")
            continue
        if observed.get("size_bytes") != expected["size_bytes"]:
            issues.append(f"Unexpected file length: {name}")
        if observed.get("sha256") != expected["sha256"]:
            issues.append(f"File SHA256 does not match the verified backup: {name}")
    optional = files.get(OPTIONAL_IMAGE_NAME)
    if optional is None:
        issues.append(f"Presence of {OPTIONAL_IMAGE_NAME} was not checked")
    elif optional.get("exists"):
        issues.append(f"Unexpected additional image is present: {OPTIONAL_IMAGE_NAME}")
    boot = details.get("boot_info", {})
    if boot.get("size_bytes") != 8:
        issues.append("The logical boot record is missing, unread, or not eight bytes")
    else:
        if boot.get("selector") != EXPECTED_BOOT_SELECTOR:
            issues.append("The logical boot selector is not normal application slot 1 (mcuimg2.bin)")
        if boot.get("magic_le") != EXPECTED_BOOT_MAGIC:
            issues.append("The logical boot record does not contain normal magic 0xabcddcba")
    return issues


class PreflightReportFile:
    """Reserve a private report before USB access and publish without replacement."""

    def __init__(self, output, owner):
        self.output = Path(output)
        if not self.output.is_absolute():
            raise PreflightError("--output must be an absolute path")
        if "\n" in str(self.output) or "\r" in str(self.output):
            raise PreflightError("Output paths cannot contain newlines")
        if not self.output.parent.is_dir():
            raise PreflightError(f"Create the output directory first: {self.output.parent}")
        self.partial = Path(str(self.output) + ".partial")
        for path in (self.output, self.partial):
            if os.path.lexists(path):
                raise PreflightError(f"Refusing to overwrite an existing file: {path}")
        self.owner = owner
        self.handle = None
        self.published = False

    def __enter__(self):
        self.handle = backup.new_private_file(self.partial, self.owner)
        return self

    def publish(self, details):
        if self.handle is None or self.published:
            raise PreflightError("Report file is not available for publication")
        data = (json.dumps(details, indent=2, sort_keys=True) + "\n").encode("utf-8")
        if self.handle.write(data) != len(data):
            raise PreflightError("Short report-file write")
        self.handle.flush()
        os.fsync(self.handle.fileno())
        os.link(self.partial, self.output)
        self.published = True
        self.partial.unlink()

    def __exit__(self, exc_type, exc, traceback):
        if self.handle is not None:
            self.handle.close()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, help="Absolute path for a new private JSON report")
    parser.add_argument("--power-wait", type=float, default=60,
                        help="Seconds to wait for Pico power-on (default: 60)")
    args = parser.parse_args(argv)
    if not 1 <= args.power_wait <= 300:
        parser.error("--power-wait must be between 1 and 300 seconds")
    transport = None
    report_file = None
    phase = "local validation"
    details = {
        "preflight_revision": PREFLIGHT_REVISION,
        "qualification_profile": PROFILE_NAME,
        "reference_backup_sha256": REFERENCE_BACKUP_SHA256,
        "started_utc": datetime.now(timezone.utc).isoformat(),
        "flash_modified": False,
        "transport_read_success": False,
        "qualified": False,
        "warnings": [],
    }
    try:
        probe_module, cc, helper, version = backup.load_dependencies()
        report_file = PreflightReportFile(args.output, backup.sudo_owner())
        details["cc3200tool_version"] = version
        details["helper"] = {"name": backup.HELPER_NAME, "size_bytes": len(helper),
                             "sha256": backup.HELPER_SHA256, "destination": "SRAM only"}
        report("PREFLIGHT: queued direct CH340 USB; all flash/file writes blocked")
        report(f"SRAM HELPER: verified {backup.HELPER_NAME}, {len(helper):,} bytes")
        report("REPORT: " + str(report_file.output))
        result = 1
        with report_file:
            try:
                transport = probe_module.CH340USBTransport()
                try:
                    with backup.USBSession(transport) as raw:
                        report("TRANSPORT: " + raw.description)
                        phase = "initial connection"

                        def connection_report(line):
                            if line == "No Pico flash was read or written.":
                                return
                            report(line.replace("SUCCESS: complete GET_VERSION",
                                                "CONNECTION: complete GET_VERSION"))

                        payload = probe_module.probe(raw, args.power_wait, report=connection_report)
                        port = backup.USBSerialPort(raw)
                        connection = build_connection(cc, probe_module, port, helper)
                        connection.vinfo = cc.CC3x00VersionInfo.from_packet(payload)
                        if not connection.vinfo.is_cc3200 or connection.vinfo.bootloader[1] != 4:
                            raise PreflightError(f"Unexpected device/version: {connection.vinfo}")
                        details["bootloader_version"] = list(connection.vinfo.bootloader)
                        phase = "RAM loader setup"
                        report("LOADER: switching UART and loading the temporary helper into SRAM")
                        connection.switch_to_nwp_bootloader()
                        if not connection.helper_executed:
                            raise PreflightError("The expected SRAM helper was not executed")
                        details["apps_version"] = repr(connection.vinfo_apps)
                        phase = "queuing USB receive buffers"
                        with backup.QueuedUSBReceive(raw) as receiver:
                            port.transport = receiver
                            report("USB RX: 8 transfers of 4096 bytes queued before filesystem queries")
                            phase = "live filesystem reads"
                            collect_preflight(connection, details=details)
                        port.transport = raw
                except (probe_module.CH340USBError, backup.QueuedUSBReceiveError):
                    if (not details["transport_read_success"]
                            or not transport.details.get("cleanup_errors")):
                        raise
                    details["warnings"].append("USB cleanup needs a cable reconnect")
                issues = qualification(details)
                details["qualification_issues"] = issues
                details["qualified"] = not issues
                result = 0 if details["qualified"] else 2
            except KeyboardInterrupt:
                details["error"] = {"phase": phase, "type": "KeyboardInterrupt",
                                    "message": "Interrupted"}
                result = 130
            except Exception as exc:
                details["error"] = {"phase": phase, "type": type(exc).__name__,
                                    "message": str(exc)}
                result = 1
            finally:
                if "error" in details:
                    details["qualified"] = False
                    details["qualification_issues"] = qualification(details)
                    details["qualification_issues"].append("The session ended with an error")
                if transport is not None:
                    details["usb"] = transport.details
                details["completed_utc"] = datetime.now(timezone.utc).isoformat()
            phase = "report publication"
            report_file.publish(details)

        if "error" in details:
            error = details["error"]
            report(f"STOPPED during {error['phase']}: {error['type']}: {error['message']}")
        report("TRANSPORT READS: " + ("complete" if details["transport_read_success"] else "incomplete"))
        if details["qualified"]:
            report("QUALIFIED: live firmware hashes and logical boot record match the expected profile")
        else:
            report("NOT QUALIFIED: do not use this report as an installation preflight pass")
            for issue in details["qualification_issues"]:
                report("  " + issue)
        report("REPORT: " + str(report_file.output))
        report("No Pico flash was written or erased. The helper runs only in RAM.")
        if transport is not None and transport.details.get("cleanup_errors"):
            report("USB CLEANUP: disconnect/reconnect USB before further use")
            for message in transport.details["cleanup_errors"]:
                report("  " + message)
        return result
    except KeyboardInterrupt:
        report(f"STOPPED during {phase}: interrupted")
        return 130
    except Exception as exc:
        report(f"STOPPED during {phase}: {type(exc).__name__}: {exc}")
        if report_file is not None and report_file.published:
            report("The report was saved: " + str(report_file.output))
        else:
            report("No complete report was published. Any .partial file is retained.")
        return 1


if __name__ == "__main__":
    exit_code = main()
    # Keep the backup tool's native-transfer lifetime protection unchanged.
    if any(getattr(receiver.raw, "_queued_receive_cleanup_blocked", False)
           for receiver in backup._RETAINED_USB_RECEIVERS):
        sys.stdout.flush()
        sys.stderr.flush()
        os._exit(exit_code or 1)
    sys.exit(exit_code)
