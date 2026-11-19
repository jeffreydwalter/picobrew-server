#!/usr/bin/env python3
"""Provision, install, or restore this verified Pico through direct CH340 USB.

Keep this script beside pico_flash_backup_v2.py, pico_uart_probe.py,
pico_flash_preflight.py, and pico_inspect_backup.py. Each operation requires
the exact original full-flash backup and a new absolute --output JSON path.
The backup is verified locally before USB is opened. The original application
is extracted in memory; raw flash and credentials are never copied to reports.

provision-key writes only /local/api.key, then proves an existing-file rewrite
of that same key. install repeats that proof, installs and reads back the exact
pinned browser bundle, then updates the existing /sys/mcuimg2.bin allocation.
restore rewrites only that existing application allocation
with the exact original image. None changes boot information, erases/deletes
files, formats flash, relocates allocations, or starts the application.

Opening an existing file for write erases that file's contents. An interrupted
application write can require restore. Restore attempts mode 0x1000 only;
it stops if the filesystem cannot reopen the existing allocation. It does not
claim recovery from every interrupted or damaged filesystem state.

Leave USB connected and unplug Pico mains before starting. Restore mains only
when the existing BREAK prompt appears. Do not press RESET.
"""

import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import hashlib
import gzip
import hmac
import json
import os
from pathlib import Path
import re
import stat
import struct
import sys
import time
from types import MappingProxyType

import pico_flash_backup_v2 as backup
import pico_flash_preflight as preflight
import pico_inspect_backup as inspection
import pico_uart_probe as probe


INSTALL_REVISION = 5
BACKUP_SIZE = 1048576
BACKUP_SHA256 = "837a728400da02b9da4a7d563e1b67fb24d6a5af03c054963c4b1de60c72343f"
ORIGINAL_OFFSET = 130 * 4096 + 8
ORIGINAL_SIZE = 162792
ORIGINAL_SHA256 = "e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702"
CANDIDATE_SIZE = 165656
CANDIDATE_SHA256 = 'a5e7fc51841ad0444e6b3a731af48dcdac214cce65c04b54b373001b543f6df4'
# Previously installed standalone versions are approved upgrade sources,
# never alternate payloads accepted by --firmware. Keep these identities fixed
# when the target CANDIDATE_* values change.
PREVIOUS_CANDIDATE_SIZE = 161720
PREVIOUS_CANDIDATE_SHA256 = "a8fc16afce43c4b91b82374a339b86631daefe1a40f08df5f8bdd7a0d0a8d904"
RELEASED_1_0_1_SIZE = 164600
RELEASED_1_0_1_SHA256 = "7f1ce21158b6f0f0297087e37c71cda23926e1a5c4902d938f4f0162929d36ce"
RELEASED_1_0_0_SIZE = 164600
RELEASED_1_0_0_SHA256 = "652f94f941a4551ebb4a2df17e818dec3b3aebd2fc1cf43ec6bcb2e5ec684558"
BOOT_INFO = bytes.fromhex("01 3d 00 20 ba dc cd ab")
BOOT_INFO_SHA256 = "59112b9c5cd615ee3427b916c2a35a0a89a40a80250197169a545fb93b8e9435"
APP_NAME = "/sys/mcuimg2.bin"
# The pinned nonmirrored allocation is 48 x 4096 bytes. The CC3200 SDK's
# _sl_GetCreateFsMode rounds this size range to 1024-byte pages, then TI's
# filesystem adds metadata and rounds to 4096-byte blocks (SWRA657 section 4;
# SWRU368C conservatively budgets 512 metadata bytes). A 48-block allocation
# requires a rounded creation maximum of at least 47 blocks. Reserve another
# complete block below that boundary, also covering up to 4096-byte creation
# granularity. This 184 KiB limit is derived conservatively, not a live
# AllocatedLen measurement: the pinned UART helper exposes only FileLen.
APPLICATION_ALLOCATION_BLOCKS = 48
APPLICATION_PAYLOAD_LIMIT = (APPLICATION_ALLOCATION_BLOCKS - 2) * 4096
KEY_NAME = "/local/api.key"
KEY_SIZE = 65
# Updated together with the final firmware candidate, after asset generation.
# A placeholder deliberately prevents installation of an unreviewed bundle.
WEB_MANIFEST_SHA256 = '29cccfb3fbf2c466e0c123b214c145272a00f91fbb1f867b3908eb32ad2a6bfa'
WEB_DIRECTORY = Path(__file__).resolve().parents[1] / "firmware/web"
WEB_FILES = (("index.html", "/local/web/i.gz"), ("app.js", "/local/web/a.gz"),
             ("api-explorer.js", "/local/web/e.gz"), ("history.js", "/local/web/h.gz"),
             ("style.css", "/local/web/s.gz"))
WEB_NAMES = frozenset(path for _, path in WEB_FILES)
READ_NAMES = frozenset((*preflight.ALLOWED_FILES, KEY_NAME, *WEB_NAMES))
PROTECTED_NAMES = ("/sys/mcuimg.bin", "/sys/mcuimg1.bin", preflight.BOOT_INFO_NAME)
READ_CHUNK_SIZE = 256
WRITE_CHUNK_SIZE = 4096
LAYOUT_BYTES = 5 * 4096
OPEN_READ = 0
OPEN_EXISTING_WRITE = 0x1000
OPEN_KEY_CREATE = 0x3001
FILE_CLOSE_COMMAND = preflight.FILE_CLOSE_COMMAND
SUCCESS_MARKERS = {
    "provision-key": "SUCCESS: API key provisioning and existing-file write verification completed",
    "install": "SUCCESS: standalone firmware, browser files and API key verified",
    "restore": "SUCCESS: original firmware restored and read back exactly",
}


class InstallError(backup.BackupError):
    """Messages must never include key contents, tokens, or raw receive bytes."""


class HandshakeError(InstallError):
    """An actionable ACK failure containing only fixed labels and numbers."""

    def __init__(self, diagnostic):
        self.diagnostic = dict(diagnostic)
        reason = {"timeout": "timed out", "nack": "NACK received",
                  "byte_limit": "receive limit reached without ACK"}[diagnostic["reason"]]
        message = (f"{diagnostic['stage']}: {reason}; "
                   f"received {diagnostic['received_bytes']} bytes")
        if "timeout_seconds" in diagnostic:
            message += f"; wait limit {diagnostic['timeout_seconds']:g}s"
        if "elapsed_seconds" in diagnostic:
            message += f"; elapsed {diagnostic['elapsed_seconds']:.3f}s"
        super().__init__(message)


def _handshake_diagnostic(exc):
    """Decode only the pinned probe's ACK-error templates, never echo RX.

    Keeping this conversion outside the probe preserves the connection code
    that succeeded on the owner's hardware. Unrecognized exceptions remain
    redacted. Hex tokens are counted, not retained in reports or messages.
    """
    original = exc
    for _ in range(3):
        if isinstance(original, probe.ProbeError):
            break
        original = original.__cause__
        if original is None:
            return None
    if not isinstance(original, probe.ProbeError):
        return None
    message = str(original)
    stage_pattern = r"(?P<stage>INITIAL ACK|COMMAND ACK|APPS INITIAL ACK)"
    tokens = r"(?:NO BYTES|[0-9a-f]{2}(?: [0-9a-f]{2}){0,1023})"
    match = re.fullmatch(stage_pattern + r": timeout after "
                         r"(?P<seconds>[0-9]+(?:\.[0-9]+)?(?:e[+-]?[0-9]+)?)s\. RX: "
                         + r"(?P<rx>" + tokens + r")", message)
    if match:
        received = 0 if match["rx"] == "NO BYTES" else (len(match["rx"]) + 1) // 3
        seconds = float(match["seconds"])
        if not 0 < seconds <= 300:
            return None
        return {"stage": match["stage"], "reason": "timeout",
                "received_bytes": received, "timeout_seconds": seconds}
    match = re.fullmatch(stage_pattern + r": NACK received\. RX: (?P<rx>" + tokens + r")", message)
    if match and match["rx"].endswith("00 33"):
        return {"stage": match["stage"], "reason": "nack",
                "received_bytes": (len(match["rx"]) + 1) // 3}
    match = re.fullmatch(stage_pattern + r": received 1024 bytes without an ACK\. "
                         r"First 64: [0-9a-f]{2}(?: [0-9a-f]{2}){63}", message)
    if match:
        return {"stage": match["stage"], "reason": "byte_limit", "received_bytes": 1024}
    return None


def _safe_exception(exc, elapsed_seconds=None):
    if isinstance(exc, InstallError):
        return exc
    diagnostic = _handshake_diagnostic(exc)
    if diagnostic is not None:
        if elapsed_seconds is not None:
            diagnostic["elapsed_seconds"] = round(max(0.0, elapsed_seconds), 3)
        return HandshakeError(diagnostic)
    return InstallError(_safe_error(exc))


def report(message):
    print(message, flush=True)


def _safe_error(exc):
    if isinstance(exc, InstallError):
        return str(exc)
    if isinstance(exc, KeyboardInterrupt):
        return "Interrupted"
    diagnostic = _handshake_diagnostic(exc)
    if diagnostic is not None:
        return str(HandshakeError(diagnostic))
    return f"{type(exc).__name__}: operation failed; device payload omitted"


def _read_regular(path, limit, private=False, owner=None):
    """Read a bounded regular file through one descriptor; reject symlinks."""
    flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    try:
        fd = os.open(path, flags)
        with os.fdopen(fd, "rb") as handle:
            info = os.fstat(handle.fileno())
            if not stat.S_ISREG(info.st_mode) or info.st_size > limit:
                raise InstallError("Local input is not a regular file of the permitted size")
            if private:
                expected_uid = owner[0] if owner is not None else os.geteuid()
                if stat.S_IMODE(info.st_mode) & 0o077 or info.st_uid != expected_uid:
                    raise InstallError("The host key must be owned by the invoking user with mode 0600")
            data = handle.read(limit + 1)
            if len(data) > limit:
                raise InstallError("Local input grew beyond its permitted size")
            return data
    except InstallError:
        raise
    except Exception:
        raise InstallError("Could not safely read the required local input file") from None


def verify_backup(path):
    image = _read_regular(path, BACKUP_SIZE)
    digest = hashlib.sha256(image).hexdigest()
    if len(image) != BACKUP_SIZE or digest != BACKUP_SHA256:
        raise InstallError("The full backup does not match the pinned original 1 MiB backup")
    header = image[ORIGINAL_OFFSET - 8:ORIGINAL_OFFSET]
    if header[6:8] != b"LS" or int.from_bytes(header[:3], "little") != ORIGINAL_SIZE:
        raise InstallError("The pinned backup has an unexpected original-image header")
    original = image[ORIGINAL_OFFSET:ORIGINAL_OFFSET + ORIGINAL_SIZE]
    if len(original) != ORIGINAL_SIZE or hashlib.sha256(original).hexdigest() != ORIGINAL_SHA256:
        raise InstallError("The original image extracted from the backup did not verify")
    return original, {"size_bytes": len(image), "sha256": digest,
                      "original_image_size_bytes": len(original),
                      "original_image_sha256": ORIGINAL_SHA256}


def validate_key(data):
    if not isinstance(data, bytes) or re.fullmatch(rb"[0-9a-f]{64}\n", data) is None:
        raise InstallError("The host key must contain exactly 64 lowercase hexadecimal characters and LF")
    return data


def load_key(path, owner=None):
    return validate_key(_read_regular(path, KEY_SIZE, private=True, owner=owner))


def _validate_application_size(size):
    if not 8 <= size <= APPLICATION_PAYLOAD_LIMIT:
        raise InstallError("Application payload exceeds the conservative existing-allocation capacity limit")


def load_candidate(path):
    _validate_application_size(CANDIDATE_SIZE)
    data = _read_regular(path, CANDIDATE_SIZE)
    if len(data) != CANDIDATE_SIZE or hashlib.sha256(data).hexdigest() != CANDIDATE_SHA256:
        raise InstallError("The firmware does not match the pinned standalone image")
    return data


def parse_web_manifest(data):
    """Return immutable, exact-path allocation metadata for the pinned bundle."""
    if (not isinstance(data, bytes) or len(data) > 16384
            or hashlib.sha256(data).hexdigest() != WEB_MANIFEST_SHA256):
        raise InstallError("The browser manifest does not match the pinned firmware bundle")
    try:
        manifest = json.loads(data)
        if (set(manifest) != {"schema_version", "block_size", "metadata_per_file", "fail_safe", "assets"}
                or manifest["schema_version"] != 1 or manifest["block_size"] != 4096
                or manifest["metadata_per_file"] != 512 or manifest["fail_safe"] is not False
                or not isinstance(manifest["assets"], list)
                or len(manifest["assets"]) != len(WEB_FILES)):
            raise ValueError()
        entries = {}
        fields = {"source", "file_path", "payload", "content_type", "content_encoding", "length",
                  "max_size", "allocation_blocks", "sha256", "source_length", "source_sha256"}
        for item, (source, destination) in zip(manifest["assets"], WEB_FILES):
            if (set(item) != fields or item["source"] != source or item["file_path"] != destination
                    or item["payload"] != source + ".gz" or item["content_encoding"] != "gzip"
                    or any(type(item[field]) is not int for field in
                           ("length", "max_size", "allocation_blocks", "source_length"))
                    or not 1 <= item["length"] <= 65024 or not 1 <= item["source_length"] <= 524288
                    or re.fullmatch(r"[0-9a-f]{64}", item["sha256"]) is None
                    or re.fullmatch(r"[0-9a-f]{64}", item["source_sha256"]) is None):
                raise ValueError()
            blocks = (item["length"] + 512 + 4095) // 4096
            if item["allocation_blocks"] != blocks or item["max_size"] != blocks * 4096 - 512:
                raise ValueError()
            # _sl_GetCreateFsMode in the pinned CC3200 SDK: 256-byte units,
            # CREATE_IF_ABSENT access, no COMMIT/mirroring/security flags.
            entry = dict(item, create_mode=0x3000 | (item["max_size"] // 256))
            entries[destination] = MappingProxyType(entry)
        return MappingProxyType(entries)
    except Exception:
        raise InstallError("The pinned browser manifest has an invalid schema or allocation") from None


def load_web_bundle(directory):
    directory = Path(directory)
    manifest = _read_regular(directory / "manifest.json", 16384)
    entries = parse_web_manifest(manifest)
    payloads = {}
    for name, entry in entries.items():
        data = _read_regular(directory / entry["payload"], entry["length"])
        if (len(data) != entry["length"] or hashlib.sha256(data).hexdigest() != entry["sha256"]):
            raise InstallError("A browser file differs from the pinned bundle")
        try:
            raw = gzip.decompress(data)  # Includes gzip footer/CRC validation.
        except Exception:
            raise InstallError("A pinned browser file is not valid gzip") from None
        if (len(raw) != entry["source_length"] or hashlib.sha256(raw).hexdigest() != entry["source_sha256"]):
            raise InstallError("A browser file's expanded contents differ from the pinned bundle")
        payloads[name] = data
    return manifest, payloads


def _image_metadata(data):
    return {"size_bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def _approved_current_applications(mode):
    """Exact live images allowed before mutation; install payload stays pinned."""
    identities = [(ORIGINAL_SIZE, ORIGINAL_SHA256)]
    if mode == "install":
        identities.extend(((PREVIOUS_CANDIDATE_SIZE, PREVIOUS_CANDIDATE_SHA256),
                           (RELEASED_1_0_0_SIZE, RELEASED_1_0_0_SHA256),
                           (RELEASED_1_0_1_SIZE, RELEASED_1_0_1_SHA256),
                           (CANDIDATE_SIZE, CANDIDATE_SHA256)))
    return tuple({"size_bytes": size, "sha256": digest} for size, digest in identities)


def _expected_public(name):
    if name == preflight.BOOT_INFO_NAME:
        return {"size_bytes": 8, "sha256": BOOT_INFO_SHA256}
    return preflight.EXPECTED_FILES[name]


def build_connection(cc, probe_module, port, helper, mode, payloads, journal, web_manifest=None):
    """Create a session whose complete write bytes and destinations are pinned.

    The inherited guard still owns the exact SRAM-helper upload. Only this
    class's fully checked filesystem commands bypass its read-only rejection.
    A protocol failure poisons the session; no command/chunk retry is possible.
    """
    if mode not in SUCCESS_MARKERS:
        raise InstallError("Unknown operation")
    immutable = MappingProxyType(dict(payloads))
    web_assets = parse_web_manifest(web_manifest) if mode == "install" else MappingProxyType({})
    expected_names = {APP_NAME} if mode == "restore" else {KEY_NAME}
    if mode == "install":
        expected_names.add(APP_NAME)
        expected_names.update(web_assets)
    if set(immutable) != expected_names or any(not isinstance(v, bytes) for v in immutable.values()):
        raise InstallError("Write payload registration does not match the selected operation")
    if KEY_NAME in immutable:
        validate_key(immutable[KEY_NAME])
    if APP_NAME in immutable:
        _validate_application_size(len(immutable[APP_NAME]))
        expected = ((ORIGINAL_SIZE, ORIGINAL_SHA256) if mode == "restore"
                    else (CANDIDATE_SIZE, CANDIDATE_SHA256))
        if (len(immutable[APP_NAME]), hashlib.sha256(immutable[APP_NAME]).hexdigest()) != expected:
            raise InstallError("Registered application payload is not the pinned image")
    for name, entry in web_assets.items():
        if (len(immutable[name]) != entry["length"]
                or hashlib.sha256(immutable[name]).hexdigest() != entry["sha256"]):
            raise InstallError("Registered browser payload differs from the pinned bundle")
    guarded = backup.guarded_connection(cc, probe_module, port, helper)

    class FileConnection(type(guarded)):
        def __init__(self):
            super().__init__()
            self.operation = mode
            self.protocol_module = cc
            self.journal = journal
            self.file_infos = {}
            self.observed = {}
            self.layout = None
            self.open_file_name = None
            self.open_access = None
            self.offset = 0
            self.phase = "idle"
            self.pending = None
            self.failed = False
            self.armed = None
            self.key_matches = False
            self.key_created = False
            self.key_rewrite_closed = False
            self.key_existing_write_verified = False
            self.application_write_attempted = False
            self.application_verified = False
            self.web_verified = set()
            self.web_write_attempted = set()
            self.web_new_blocks_claimed = 0
            self.web_new_files_claimed = 0
            self.web_space_checked = False
            self.mutation_attempts = []
            self._read_digest = None
            self._key_read = bytearray()

        @property
        def payloads(self):
            return immutable

        @property
        def web_assets(self):
            return web_assets

        def _record(self, event, **fields):
            try:
                self.journal.record(event, **fields)
            except BaseException as exc:
                self.failed = True
                if isinstance(exc, KeyboardInterrupt):
                    raise
                raise InstallError("The durable journal failed; the session is stopped") from None

        def _read_ack(self, timeout=None):
            try:
                seconds = self.port.timeout if timeout is None else timeout
                deadline = time.monotonic() + seconds
                previous = None
                received = 0
                while time.monotonic() < deadline:
                    data = self.port.transport.read(1)
                    if not data:
                        continue
                    value = data[0]
                    received += 1
                    if previous == 0 and value == 0xCC:
                        return True
                    if previous == 0 and value == 0x33:
                        raise InstallError("The device rejected a command acknowledgement")
                    if received >= 1024:
                        raise InstallError("No acknowledgement in the bounded response stream")
                    previous = value
                raise InstallError("Command acknowledgement timed out")
            except BaseException as exc:
                self.failed = True
                if isinstance(exc, KeyboardInterrupt):
                    raise
                # Match the proven scanner's ACK/NACK/deadline behavior, but
                # retain only the previous byte, never a diagnostic RX buffer.
                raise InstallError(_safe_error(exc)) from None

        def _try_breaking(self, tries=5, timeout=1):
            # Delegate the entire BREAK/ACK/release sequence to the physically
            # proven backup implementation. Only failure reporting changes.
            started = time.monotonic()
            try:
                return super()._try_breaking(tries, timeout)
            except BaseException as exc:
                self.failed = True
                if isinstance(exc, KeyboardInterrupt):
                    raise
                raise _safe_exception(exc, time.monotonic() - started) from None

        def _live_state_ok(self):
            if self.flash_size != BACKUP_SIZE or not self.layout:
                raise InstallError("Live geometry and allocation checks have not passed")
            if (not self.layout["application_allocation_verified"]
                    or not self.layout["image3_absent_in_all_fats"]):
                raise InstallError("The live application layout differs from the approved layout")
            for name in PROTECTED_NAMES:
                if self.observed.get(name) != _expected_public(name):
                    raise InstallError("Protected firmware or logical boot information has not verified")
            image3 = self.file_infos.get(preflight.OPTIONAL_IMAGE_NAME)
            if image3 is None or image3["exists"]:
                raise InstallError("The additional image slot is not confirmed absent")
            if self.operation != "restore":
                app = self.observed.get(APP_NAME)
                if app not in _approved_current_applications(self.operation):
                    raise InstallError("Current application is not an approved stock/standalone image; use restore")

        def arm_write(self, name, flags):
            if self.failed or self.phase != "idle" or self.pending is not None or self.armed is not None:
                raise InstallError("Cannot authorize a write in the current protocol state")
            self._live_state_ok()
            info = self.file_infos.get(name)
            if name == KEY_NAME and self.operation in ("provision-key", "install"):
                if flags == OPEN_KEY_CREATE:
                    if (self.operation != "provision-key" or info is None or info["exists"]
                            or not self.layout["key_absent_in_all_fats"]
                            or self.key_created or self.key_rewrite_closed):
                        raise InstallError("Key creation is not justified by the live logical/FAT absence checks")
                    label = "key-create"
                elif flags == OPEN_EXISTING_WRITE:
                    if (info is None or not info["exists"] or not self.key_matches
                            or self.key_rewrite_closed):
                        raise InstallError("Existing key must exactly match the host key before its rewrite")
                    label = "key-existing-write-proof"
                else:
                    raise InstallError("Unapproved key open mode")
            elif name == APP_NAME and self.operation in ("install", "restore"):
                if flags != OPEN_EXISTING_WRITE or self.application_write_attempted:
                    raise InstallError("Only one existing-application write is permitted per session")
                if self.operation == "install" and not self.key_existing_write_verified:
                    raise InstallError("The matching key's existing-file rewrite must verify before installation")
                if self.operation == "install" and self.web_verified != set(self.web_assets):
                    raise InstallError("Every pinned browser file must verify before the application write")
                # Restore need not trust logical existence/length of a damaged
                # file. The raw named allocation is checked independently.
                if info is None or (not info["exists"] and self.operation != "restore"):
                    raise InstallError("Application presence has not been checked")
                label = "application-restore" if self.operation == "restore" else "application-install"
            elif name in self.web_assets and self.operation == "install":
                if not self.key_existing_write_verified or not self.key_matches:
                    raise InstallError("The matching key's existing-file rewrite must verify before browser files")
                if not self.web_space_checked:
                    raise InstallError("The complete browser bundle's free-space plan has not verified")
                if self.application_write_attempted or name in self.web_write_attempted:
                    raise InstallError("Only one browser write per file is allowed, before the application write")
                allocation = self.layout.get("web_assets", {}).get(name)
                entry = self.web_assets[name]
                if info is None or allocation is None:
                    raise InstallError("Browser file presence and allocation have not been checked")
                if flags == entry["create_mode"]:
                    if info["exists"] or not allocation["absent_in_all_fats"]:
                        raise InstallError("Browser creation requires confirmed logical and FAT absence")
                    if (self.web_new_blocks_claimed + entry["allocation_blocks"] > self.layout["free_blocks_min"]
                            or self.web_new_files_claimed + 1 > self.layout["free_file_slots_min"]):
                        raise InstallError("The verified free space cannot hold the browser bundle")
                    label = "web-create"
                elif flags == OPEN_EXISTING_WRITE:
                    if not info["exists"] or not allocation["existing_write_compatible"]:
                        raise InstallError("Existing browser allocation cannot hold the pinned file; no delete/create fallback")
                    label = "web-existing-write"
                else:
                    raise InstallError("Unapproved browser file open mode")
            else:
                raise InstallError("Blocked a write outside the operation's destination allowlist")
            if name not in self.payloads:
                raise InstallError("No immutable payload is registered for this destination")
            self.armed = (name, flags, label)

        def _send_packet(self, data, timeout=None):
            if self.failed:
                raise InstallError("The session failed; reconnect instead of retrying commands")
            if not data:
                raise InstallError("Blocked an empty command")
            opcode = data[0]
            if opcode not in (0x21, 0x22, 0x23, 0x24, 0x2A, 0x2B):
                if self.phase != "idle" or self.pending is not None or self.armed is not None:
                    raise InstallError("Blocked a command during a filesystem transaction")
                try:
                    return super()._send_packet(data, timeout)
                except BaseException:
                    self.failed = True
                    raise
            if not self.helper_executed or self.flash_size != BACKUP_SIZE or self.pending is not None:
                raise InstallError("Filesystem command is not valid in the current state")
            names = {name.encode("ascii"): name for name in READ_NAMES}
            pending = None
            next_phase = self.phase
            opened = None
            access = None
            write_event = None
            if opcode == 0x2A:
                if self.phase != "idle" or self.armed is not None or len(data) < 5:
                    raise InstallError("Invalid file-info state")
                length = struct.unpack(">I", data[1:5])[0]
                if length != len(data[5:]) or data[5:] not in names:
                    raise InstallError("Blocked an invalid or unapproved file-info name")
                pending = ("info", names[data[5:]])
            elif opcode == 0x21:
                if self.phase != "idle" or len(data) < 12 or not data.endswith(b"\0\0"):
                    raise InstallError("Invalid file-open state or format")
                flags, token = struct.unpack(">II", data[1:9])
                if token != 0 or data[9:-2] not in names:
                    raise InstallError("Blocked a file-open token or filename")
                opened = names[data[9:-2]]
                info = self.file_infos.get(opened)
                if flags == OPEN_READ:
                    if self.armed is not None or not info or not info["exists"]:
                        raise InstallError("File must be confirmed present before reading")
                    bound = (KEY_SIZE if opened == KEY_NAME else
                             self.web_assets[opened]["max_size"] if opened in self.web_assets else BACKUP_SIZE)
                    if not 0 <= info["size_bytes"] <= bound:
                        raise InstallError("Reported file length exceeds its read bound")
                    access = "read"
                else:
                    if self.armed is None or self.armed[:2] != (opened, flags):
                        raise InstallError("Unapproved write/create open was blocked")
                    self._live_state_ok()
                    access = "write"
                    write_event = {"target": opened, "mode": f"0x{flags:04x}",
                                   "size_bytes": len(self.payloads[opened]), "stage": self.armed[2]}
                    # This must be durable BEFORE the command that erases or
                    # creates file content is sent. A journal error blocks it.
                    self._record("before-write-open", **write_event)
                    self.mutation_attempts.append(dict(write_event))
                    if opened == APP_NAME:
                        self.application_write_attempted = True
                    elif opened == KEY_NAME:
                        self.key_matches = False
                    elif opened in self.web_assets:
                        self.web_write_attempted.add(opened)
                        self.web_verified.discard(opened)
                        if flags != OPEN_EXISTING_WRITE:
                            self.web_new_blocks_claimed += self.web_assets[opened]["allocation_blocks"]
                            self.web_new_files_claimed += 1
                    self.observed.pop(opened, None)
                next_phase = "open_token"
            elif opcode == 0x2B:
                if self.phase != "reading" or self.open_access != "read" or len(data) != 9:
                    raise InstallError("Invalid file-read state")
                offset, count = struct.unpack(">II", data[1:])
                size = self.file_infos[self.open_file_name]["size_bytes"]
                if offset != self.offset or not 1 <= count <= READ_CHUNK_SIZE or offset + count > size:
                    raise InstallError("Blocked an out-of-order or out-of-bounds file read")
                pending = ("read", count)
            elif opcode == 0x24:
                if self.phase != "writing" or self.open_access != "write" or len(data) < 6:
                    raise InstallError("File chunk is not authorized in the current state")
                offset = struct.unpack(">I", data[1:5])[0]
                chunk = data[5:]
                approved = self.payloads[self.open_file_name]
                if (offset != self.offset or not 1 <= len(chunk) <= WRITE_CHUNK_SIZE
                        or offset + len(chunk) > len(approved)
                        or not hmac.compare_digest(chunk, approved[offset:offset + len(chunk)])):
                    raise InstallError("Write bytes or offset differ from the immutable approved payload")
                pending = None
                next_phase = "chunk_status"
                self._pending_write_count = len(chunk)
            elif opcode == 0x22:
                if data != FILE_CLOSE_COMMAND or self.phase not in ("reading", "writing"):
                    raise InstallError("Invalid file-close command/state")
                size = (len(self.payloads[self.open_file_name]) if self.open_access == "write"
                        else self.file_infos[self.open_file_name]["size_bytes"])
                if self.offset != size:
                    raise InstallError("Closing an incomplete file transaction is blocked")
                next_phase = "close_status"
            elif opcode == 0x23:
                if data != b"\x23" or self.phase not in ("open_status", "chunk_status", "close_status"):
                    raise InstallError("Status query is not valid in the current state")
                pending = ("status", self.phase)
            try:
                cc.CC3200Connection._send_packet(self, data, timeout)
            except BaseException as exc:
                self.failed = True
                if isinstance(exc, KeyboardInterrupt):
                    raise
                raise InstallError(_safe_error(exc)) from None
            self.pending = pending
            self.phase = next_phase
            if opened is not None:
                self.open_file_name = opened
                self.open_access = access
                self.offset = 0
                self._read_digest = hashlib.sha256() if opened != KEY_NAME else None
                self._key_read.clear()
                self._web_read_matches = True
                if access == "write":
                    self._write_flags = self.armed[1]
                    self.armed = None
                    self._record("write-open-acknowledged", target=opened)

        def _read_packet(self, timeout=None):
            try:
                payload = super()._read_packet(timeout)
                if self.pending is None:
                    return payload
                kind, value = self.pending
                if kind == "info":
                    self.file_infos[value] = preflight.parse_file_info(payload)
                elif kind == "read":
                    if len(payload) != value:
                        raise InstallError("File response length differs from the requested length")
                    if self.open_file_name in self.web_assets:
                        approved = self.payloads[self.open_file_name]
                        self._web_read_matches = (self._web_read_matches and
                            hmac.compare_digest(payload, approved[self.offset:self.offset + value]))
                    self.offset += value
                    if self.open_file_name == KEY_NAME:
                        self._key_read.extend(payload)
                    else:
                        self._read_digest.update(payload)
                elif kind == "status":
                    if payload != b"\x40":
                        if (value == "open_status" and self.open_file_name == APP_NAME
                                and self.operation == "restore" and self.open_access == "write"):
                            raise InstallError(
                                "The filesystem could not reopen the allocated image for restore; "
                                "this state is unsupported, and no create/delete fallback was attempted")
                        raise InstallError("Filesystem operation did not return successful status 0x40")
                    if value == "open_status":
                        self.phase = "writing" if self.open_access == "write" else "reading"
                        if self.open_access == "write":
                            self._record("write-open-status-ok", target=self.open_file_name)
                    elif value == "chunk_status":
                        self.offset += self._pending_write_count
                        self.phase = "writing"
                        self._record("write-chunk-status-ok", target=self.open_file_name,
                                     completed_bytes=self.offset)
                    else:
                        name = self.open_file_name
                        if self.open_access == "write":
                            self._record("write-close-status-ok", target=name, size_bytes=self.offset)
                            self.file_infos.pop(name, None)
                            if name == KEY_NAME:
                                if self._write_flags == OPEN_KEY_CREATE:
                                    self.key_created = True
                                else:
                                    self.key_rewrite_closed = True
                        elif name == KEY_NAME:
                            self.key_matches = (name in self.payloads and hmac.compare_digest(
                                bytes(self._key_read), self.payloads[name]))
                            self._key_read.clear()
                            if self.key_rewrite_closed and self.key_matches:
                                self.key_existing_write_verified = True
                        else:
                            self.observed[name] = {"size_bytes": self.offset,
                                                   "sha256": self._read_digest.hexdigest()}
                            if name in self.web_assets:
                                self.web_verified.discard(name)
                                if self._web_read_matches and self.offset == len(self.payloads[name]):
                                    self.web_verified.add(name)
                        self.open_file_name = None
                        self.open_access = None
                        self.offset = 0
                        self.phase = "idle"
                self.pending = None
                return payload
            except BaseException as exc:
                self.failed = True
                if isinstance(exc, KeyboardInterrupt):
                    raise
                raise InstallError(_safe_error(exc)) from None

        def _open_file(self, filename, slfs_flags):
            try:
                super()._open_file(filename, slfs_flags)
                if self.phase != "open_token":
                    raise InstallError("Unexpected state after consuming the file-open token")
                self.phase = "open_status"
                if not self._get_last_status().is_ok:
                    raise InstallError("File-open status failed")
            except BaseException as exc:
                self.failed = True
                if isinstance(exc, KeyboardInterrupt):
                    raise
                raise InstallError(_safe_error(exc)) from None

    return FileConnection()


def read_file_info(connection, name):
    if name not in READ_NAMES:
        raise InstallError("Blocked a file-info name outside the allowlist")
    encoded = name.encode("ascii")
    connection._send_packet(b"\x2a" + struct.pack(">I", len(encoded)) + encoded)
    parsed = preflight.parse_file_info(connection._read_packet())
    if connection.file_infos.get(name) != parsed:
        raise InstallError("File-info guard and caller disagree")
    return dict(parsed)


def read_file_bytes(connection, name, expected_max=BACKUP_SIZE):
    info = read_file_info(connection, name)
    if not info["exists"]:
        raise InstallError("A required logical file is absent or unreadable")
    bound = min(expected_max, KEY_SIZE if name == KEY_NAME else BACKUP_SIZE)
    if not 0 <= info["size_bytes"] <= bound:
        raise InstallError("A logical file exceeds the permitted read length")
    connection._open_file_for_read(name)
    result = bytearray()
    while len(result) < info["size_bytes"]:
        count = min(READ_CHUNK_SIZE, info["size_bytes"] - len(result))
        connection._send_packet(b"\x2b" + struct.pack(">II", len(result), count))
        payload = connection._read_packet()
        if len(payload) != count:
            raise InstallError("Short logical-file read")
        result.extend(payload)
    connection._close_file()
    return bytes(result)


@contextmanager
def _quiet_protocol_logs(cc):
    # The upstream FAT parser logs arbitrary filenames on malformed overlaps.
    # Those bytes are untrusted metadata and must not reach credential logs.
    previous = cc.log.disabled
    cc.log.disabled = True
    try:
        yield
    finally:
        cc.log.disabled = previous


def read_live_layout(connection, cc):
    """Prove relevant names/allocations without selecting an active FAT copy."""
    data = bytearray()
    while len(data) < LAYOUT_BYTES:
        count = min(READ_CHUNK_SIZE, LAYOUT_BYTES - len(data))
        data.extend(connection._read_chunk(len(data), count, 2))
    storage = cc.CC3x00StorageInfo(4096, 256)
    views = []
    key_absent = True
    image3_absent = True
    web_layout = {name: {"absent_in_all_fats": True, "allocations": []}
                  for name in connection.web_assets}
    free_blocks_min = 256
    free_file_slots_min = 128
    try:
        for index in range(2):
            start = index * 4096
            fat_bytes = bytes(data[start:start + 4096])
            revision, signature = struct.unpack("<HH", fat_bytes[:4])
            if revision == 0xFFFF and signature == 0xFFFF:
                continue
            if revision == 0xFFFF or signature != 0x534C:
                raise InstallError("A FAT copy is neither validated nor unequivocally erased")
            metadata_start = 0x774 + start
            metadata = bytes(data[metadata_start:metadata_start + 4096])
            inspection.validate_filename_metadata(fat_bytes, metadata)
            with _quiet_protocol_logs(cc):
                header = cc.CC3x00SffsHeader(index, fat_bytes, storage)
                if not header.is_valid:
                    raise InstallError("A recognized FAT copy failed validation")
                fat = cc.CC3x00SffsInfo(header, storage, metadata, "cc3200")
            names = {}
            occupied = set(range(5))
            for entry in fat.files:
                if (not entry.fname or any(ord(c) < 0x20 or ord(c) >= 0x7F for c in entry.fname)
                        or entry.size_blocks <= 0 or entry.start_block < 5
                        or entry.start_block + entry.total_blocks > 256):
                    raise InstallError("A live FAT allocation/name is malformed")
                name = "/" + entry.fname.lstrip("/").lower()
                blocks = set(range(entry.start_block, entry.start_block + entry.total_blocks))
                if name in names or occupied.intersection(blocks):
                    raise InstallError("Live FAT names or allocations overlap")
                names[name] = entry
                occupied.update(blocks)
            app = names.get(APP_NAME)
            if (app is None or app.start_block != 130 or app.size_blocks != APPLICATION_ALLOCATION_BLOCKS
                    or app.mirrored or app.flags != 0xC):
                raise InstallError("The existing image2 allocation differs from the approved 48-block allocation")
            key_absent = key_absent and KEY_NAME not in names
            image3_absent = image3_absent and preflight.OPTIONAL_IMAGE_NAME not in names
            free_blocks_min = min(free_blocks_min, 256 - len(occupied))
            free_file_slots_min = min(free_file_slots_min, 128 - len(names))
            for name, allocation in web_layout.items():
                entry = names.get(name)
                if entry is not None:
                    allocation["absent_in_all_fats"] = False
                    allocation["allocations"].append({"fat_index": index,
                        "start_block": entry.start_block, "size_blocks": entry.size_blocks,
                        "mirrored": bool(entry.mirrored), "flags": entry.flags})
            extents = []
            extent_start = None
            for block in range(257):
                if block < 256 and block not in occupied:
                    if extent_start is None:
                        extent_start = block
                elif extent_start is not None:
                    extents.append({"start_block": extent_start, "blocks": block - extent_start})
                    extent_start = None
            known_local = []
            for name, entry in names.items():
                if (name in WEB_NAMES or name in (KEY_NAME, "/restart.txt") or
                        re.fullmatch(r"/local/(?:recipe[0-9]{2}\.[ab]|(?:index|settings|sessions)\.[ab]|"
                                     r"history[0-9]{2}|history\.(?:meta|tail)\.[ab])", name)):
                    known_local.append({"file_path": name, "start_block": entry.start_block,
                        "size_blocks": entry.size_blocks, "total_blocks": entry.total_blocks,
                        "mirrored": bool(entry.mirrored)})
            views.append({"index": index, "revision": revision,
                "allocated_blocks": len(occupied), "free_blocks": 256 - len(occupied),
                "file_count": len(names), "free_extents": extents,
                "known_local_allocations": sorted(known_local, key=lambda item: item["file_path"])})
    except InstallError:
        raise
    except Exception:
        raise InstallError("Live FAT/name metadata could not be validated") from None
    if not views:
        raise InstallError("No validated FAT view is available")
    for name, allocation in web_layout.items():
        allocation["existing_write_compatible"] = bool(allocation["allocations"]) and all(
            not item["mirrored"] and item["flags"] == 0xC
            and item["size_blocks"] >= connection.web_assets[name]["allocation_blocks"]
            for item in allocation["allocations"])
    result = {"fat_views": views, "application_allocation_verified": True,
              "application_payload_limit_bytes": APPLICATION_PAYLOAD_LIMIT,
              "key_absent_in_all_fats": key_absent,
              "image3_absent_in_all_fats": image3_absent,
              "free_blocks_min": free_blocks_min, "free_file_slots_min": free_file_slots_min,
              "web_assets": web_layout}
    connection.layout = result
    return result


def _check_protected(connection, progress):
    result = {}
    for name in PROTECTED_NAMES:
        data = read_file_bytes(connection, name)
        observed = _image_metadata(data)
        if observed != _expected_public(name):
            raise InstallError("Protected firmware or logical boot information differs from the verified device")
        result[name] = observed
        progress(f"VERIFIED: {name}: {observed['size_bytes']:,} bytes; SHA256 {observed['sha256']}")
    return result


def _verify_key(connection, key_bytes):
    data = read_file_bytes(connection, KEY_NAME, expected_max=KEY_SIZE)
    if not hmac.compare_digest(data, key_bytes) or not connection.key_matches:
        raise InstallError("The device key differs from the supplied host key; no replacement is authorized")


def _write_registered(connection, name, flags):
    connection.arm_write(name, flags)
    connection._open_file(name, flags)
    payload = connection.payloads[name]
    offset = 0
    while offset < len(payload):
        chunk = payload[offset:offset + WRITE_CHUNK_SIZE]
        connection._send_packet(b"\x24" + struct.pack(">I", offset) + chunk)
        if not connection._get_last_status().is_ok:
            raise InstallError("A file-write chunk failed")
        offset += len(chunk)
    connection._close_file()


def _plan_web_bundle(connection, details):
    """Prove destinations and whole-file holes before any install mutation."""
    plans = []
    new_blocks = new_files = 0
    for name, entry in connection.web_assets.items():
        info = read_file_info(connection, name)
        allocation = connection.layout["web_assets"][name]
        if info["exists"]:
            if not allocation["existing_write_compatible"]:
                raise InstallError("An existing browser allocation is incompatible; no erase/delete fallback is allowed")
            flags = OPEN_EXISTING_WRITE
        else:
            if not allocation["absent_in_all_fats"]:
                raise InstallError("An unreadable browser file still has a FAT allocation; creation is blocked")
            flags = entry["create_mode"]
            new_blocks += entry["allocation_blocks"]
            new_files += 1
        plans.append((name, flags, info))
    if (new_blocks > connection.layout["free_blocks_min"]
            or new_files > connection.layout["free_file_slots_min"]):
        raise InstallError("The verified filesystem lacks room for the complete browser bundle")
    for view in connection.layout["fat_views"]:
        holes = [extent["blocks"] for extent in view["free_extents"]]
        for name, flags, _ in plans:
            if flags == OPEN_EXISTING_WRITE:
                continue
            needed = connection.web_assets[name]["allocation_blocks"]
            for index, available in enumerate(holes):
                if available >= needed:
                    holes[index] -= needed
                    break
            else:
                raise InstallError("Free flash is fragmented; the complete browser bundle does not fit the verified holes")
    details["web_bundle"] = {"manifest_sha256": WEB_MANIFEST_SHA256,
        "files": [], "new_allocation_blocks": new_blocks,
        "maximum_allocation_blocks": sum(entry["allocation_blocks"] for entry in connection.web_assets.values()),
        "verified": False}
    connection.web_space_checked = True
    connection._record("web-plan-verified", new_allocation_blocks=new_blocks,
                       new_files=new_files, manifest_sha256=WEB_MANIFEST_SHA256)
    return tuple(plans)


def _install_web_bundle(connection, details, progress, plans):
    """Install the prepared destinations; image2 stays intact until all verify."""
    for name, flags, info in plans:
        entry = connection.web_assets[name]
        already_matches = False
        if info["exists"] and info["size_bytes"] == len(connection.payloads[name]):
            current = read_file_bytes(connection, name, expected_max=entry["max_size"])
            already_matches = hmac.compare_digest(current, connection.payloads[name])
        if not already_matches:
            progress("BROWSER: " + ("creating " if flags != OPEN_EXISTING_WRITE else "rewriting ") + name)
            _write_registered(connection, name, flags)
            current = read_file_bytes(connection, name, expected_max=entry["max_size"])
            if not hmac.compare_digest(current, connection.payloads[name]):
                raise InstallError("Browser readback differs from the complete pinned file")
        if name not in connection.web_verified:
            raise InstallError("Browser file byte-for-byte readback did not verify")
        metadata = {"file_path": name, "size_bytes": entry["length"], "sha256": entry["sha256"],
                    "already_matches": already_matches, "allocation_blocks": entry["allocation_blocks"]}
        details["web_bundle"]["files"].append(metadata)
        connection._record("web-readback-verified", **metadata)
    if connection.web_verified != set(connection.web_assets):
        raise InstallError("The complete browser bundle has not verified")
    details["web_bundle"]["verified"] = True
    connection._record("web-bundle-verified", manifest_sha256=WEB_MANIFEST_SHA256)


def run_operation(connection, mode, key_bytes, candidate_bytes, original_bytes, journal,
                  progress=report, details=None):
    if details is None:
        details = {}
    if mode != connection.operation:
        raise InstallError("Operation and guarded connection disagree")
    if mode != "restore" and key_bytes != connection.payloads.get(KEY_NAME):
        raise InstallError("The registered key payload changed")
    expected_app = original_bytes if mode == "restore" else candidate_bytes
    if mode != "provision-key" and expected_app != connection.payloads.get(APP_NAME):
        raise InstallError("The registered application payload changed")
    details["live_preflight"] = before = {}
    before["flash"] = backup.detect_flash(connection)
    if before["flash"] != preflight.EXPECTED_FLASH:
        raise InstallError("Flash geometry differs from the verified 1 MiB device")
    # The exact installed protocol module is retained by the caller, so this
    # read-only parser uses the same version as the guarded connection.
    cc = connection.protocol_module
    before["layout"] = read_live_layout(connection, cc)
    before["protected"] = _check_protected(connection, progress)
    optional = read_file_info(connection, preflight.OPTIONAL_IMAGE_NAME)
    if optional["exists"] or not before["layout"]["image3_absent_in_all_fats"]:
        raise InstallError("An unexpected image3 is present")
    app_info = read_file_info(connection, APP_NAME)
    if mode == "restore":
        before["application"] = {"logical_info": app_info, "content_read_required": False}
    else:
        data = read_file_bytes(connection, APP_NAME)
        observed = _image_metadata(data)
        before["application"] = observed
        if observed not in _approved_current_applications(mode):
            raise InstallError("Current application is not an approved stock/standalone image; use restore")
        progress(f"VERIFIED: {APP_NAME}: {len(data):,} bytes; SHA256 {observed['sha256']}")
    connection._live_state_ok()
    connection._record("live-preflight-passed", mode=mode)

    if mode != "restore":
        key_info = read_file_info(connection, KEY_NAME)
        if key_info["exists"]:
            _verify_key(connection, key_bytes)
        else:
            if mode != "provision-key":
                raise InstallError("Provision and verify the host key before installing firmware")
            if not before["layout"]["key_absent_in_all_fats"]:
                raise InstallError("The key has a FAT entry but is logically unreadable; creation is blocked")
            progress("KEY: creating the confirmed-absent private key file")
            _write_registered(connection, KEY_NAME, OPEN_KEY_CREATE)
            _verify_key(connection, key_bytes)
            connection._record("key-create-readback-verified")
        if mode == "install":
            web_plans = _plan_web_bundle(connection, details)
        progress("KEY: proving an existing-file rewrite of the matching private key")
        _write_registered(connection, KEY_NAME, OPEN_EXISTING_WRITE)
        _verify_key(connection, key_bytes)
        if not connection.key_existing_write_verified:
            raise InstallError("The key's existing-file rewrite did not verify")
        connection._record("key-existing-write-readback-verified")
        details["key"] = {"created": connection.key_created, "matches_host": True,
                          "existing_write_verified": True, "size_bytes": KEY_SIZE}

    if mode == "install":
        _install_web_bundle(connection, details, progress, web_plans)
        already = before["application"] == {"size_bytes": CANDIDATE_SIZE, "sha256": CANDIDATE_SHA256}
        details["application_already_installed"] = already
        if not already:
            progress("APPLICATION: rewriting the existing image2 allocation with the pinned standalone image")
            _write_registered(connection, APP_NAME, OPEN_EXISTING_WRITE)
    elif mode == "restore":
        progress("APPLICATION: attempting an existing-allocation restore of the exact original image")
        _write_registered(connection, APP_NAME, OPEN_EXISTING_WRITE)

    details["postcheck"] = after = {}
    expected = original_bytes if mode in ("restore", "provision-key") else candidate_bytes
    observed_app = read_file_bytes(connection, APP_NAME)
    if not hmac.compare_digest(observed_app, expected):
        raise InstallError("Application readback differs from the intended complete image")
    after["application"] = _image_metadata(observed_app)
    connection.application_verified = True
    connection._record("application-readback-verified", **after["application"])
    after["protected"] = _check_protected(connection, progress)
    if mode != "restore":
        _verify_key(connection, key_bytes)
        after["key_matches_host"] = True
    after["layout"] = read_live_layout(connection, cc)
    if mode == "install" and not all(item["existing_write_compatible"]
                                    for item in after["layout"]["web_assets"].values()):
        raise InstallError("Installed browser allocations failed the final filesystem check")
    if not after["layout"]["image3_absent_in_all_fats"]:
        raise InstallError("An unexpected image3 appeared during the operation")
    if read_file_info(connection, preflight.OPTIONAL_IMAGE_NAME)["exists"]:
        raise InstallError("An unexpected logical image3 appeared during the operation")
    details["operation_verified"] = True
    connection._record("operation-verified", mode=mode)
    return details


class InstallFiles:
    """Private append-only journal plus a no-replacement final JSON report."""

    def __init__(self, output, owner):
        self.output = Path(output)
        if not self.output.is_absolute() or any(c in str(self.output) for c in "\r\n"):
            raise InstallError("--output must be an absolute path without newlines")
        if not self.output.parent.is_dir():
            raise InstallError("Create the report's output directory before starting")
        self.partial = Path(str(self.output) + ".partial")
        self.journal_path = Path(str(self.output) + ".journal.jsonl")
        for path in (self.output, self.partial, self.journal_path):
            if os.path.lexists(path):
                raise InstallError("An output/report/journal already exists; choose a new --output path")
        self.owner = owner
        self.handle = None
        self.journal_handle = None
        self.published = False

    def __enter__(self):
        self.handle = backup.new_private_file(self.partial, self.owner)
        try:
            self.journal_handle = backup.new_private_file(self.journal_path, self.owner)
        except BaseException:
            self.handle.close()
            raise
        return self

    def record(self, event, **fields):
        if self.journal_handle is None:
            raise InstallError("The durable journal is not open")
        item = {"event": event, "utc": datetime.now(timezone.utc).isoformat(), **fields}
        data = (json.dumps(item, sort_keys=True) + "\n").encode()
        try:
            if self.journal_handle.write(data) != len(data):
                raise OSError("short write")
            self.journal_handle.flush()
            os.fsync(self.journal_handle.fileno())
        except Exception:
            raise InstallError("The durable journal could not be updated; further device commands are blocked") from None

    def publish(self, details):
        if self.handle is None or self.published:
            raise InstallError("The report is unavailable for publication")
        data = (json.dumps(details, indent=2, sort_keys=True) + "\n").encode()
        if self.handle.write(data) != len(data):
            raise InstallError("Short report write")
        self.handle.flush()
        os.fsync(self.handle.fileno())
        os.link(self.partial, self.output)
        self.published = True
        self.partial.unlink()

    def __exit__(self, exc_type, exc, traceback):
        for handle in (self.handle, self.journal_handle):
            if handle is not None:
                handle.close()


def _usb_summary(details):
    """Exclude raw failure/control buffers from credential-capable reports."""
    result = {name: details[name] for name in (
        "usb_id", "bus", "address", "chip_version", "requested_baud", "calculated_baud",
    ) if name in details}
    result["cleanup_error_count"] = len(details.get("cleanup_errors", []))
    if "receive_queue" in details:
        queue = details["receive_queue"]
        result["receive_queue"] = {
            key: value for key, value in queue.items()
            if isinstance(value, (int, float, bool)) or value is None
        }
        result["receive_queue"]["error_present"] = bool(queue.get("error"))
        result["receive_queue"]["cleanup_error_count"] = len(queue.get("cleanup_errors", []))
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="mode", required=True)
    for name in SUCCESS_MARKERS:
        command = commands.add_parser(name)
        command.add_argument("--backup", required=True, type=Path, help="Exact original full-flash backup")
        command.add_argument("--output", required=True, type=Path, help="New absolute private JSON report path")
        command.add_argument("--power-wait", type=float, default=60)
        if name != "restore":
            command.add_argument("--key-file", required=True, type=Path, help="Existing private host key file")
        if name == "install":
            command.add_argument("--firmware", required=True, type=Path, help="Pinned standalone binary")
            command.add_argument("--web-directory", type=Path, default=WEB_DIRECTORY,
                                 help="Matching pinned browser bundle (defaults to repository firmware/web)")
    args = parser.parse_args(argv)
    if not 1 <= args.power_wait <= 300:
        parser.error("--power-wait must be between 1 and 300 seconds")
    files = None
    transport = None
    connection = None
    phase = "local validation"
    details = {"install_revision": INSTALL_REVISION, "mode": args.mode,
               "started_utc": datetime.now(timezone.utc).isoformat(),
               "success": False, "operation_verified": False, "warnings": [],
               "mutation_attempts": [], "application_write_attempted": False,
               "application_readback_verified": False, "persistent_write_attempted": False,
               "helper_uploaded_bytes": 0, "helper_executed": False}
    try:
        owner = backup.sudo_owner()
        original, details["backup"] = verify_backup(args.backup)
        key = load_key(args.key_file, owner) if args.mode != "restore" else None
        candidate = load_candidate(args.firmware) if args.mode == "install" else None
        web_manifest, web_payloads = (load_web_bundle(args.web_directory)
                                      if args.mode == "install" else (None, {}))
        if candidate is not None:
            details["candidate"] = _image_metadata(candidate)
        probe_module, cc, helper, version = backup.load_dependencies()
        files = InstallFiles(args.output, owner)
        details["cc3200tool_version"] = version
        details["helper"] = {"name": backup.HELPER_NAME, "size_bytes": len(helper),
                             "sha256": backup.HELPER_SHA256, "destination": "SRAM only"}
        payloads = {APP_NAME: original} if args.mode == "restore" else {KEY_NAME: key}
        if args.mode == "install":
            payloads[APP_NAME] = candidate
            payloads.update(web_payloads)
        report("LOCAL BACKUP: exact original full-flash backup and stock image verified")
        report("OPERATION: " + args.mode)
        report("REPORT: " + str(files.output))
        report("JOURNAL: " + str(files.journal_path))
        result = 1
        with files:
            files.record("session-start", mode=args.mode, backup_sha256=BACKUP_SHA256)
            try:
                transport = probe_module.CH340USBTransport()
                try:
                    with backup.USBSession(transport) as raw:
                        try:
                            report("TRANSPORT: " + raw.description)
                            phase = "initial connection"

                            def connection_report(line):
                                if line.startswith(("BREAK asserted.", "Listening for ",
                                                    "Bootloader version:")):
                                    report(line)
                                elif line.startswith("SUCCESS: complete GET_VERSION"):
                                    report("CONNECTION: ROM version response verified")

                            payload = probe_module.probe(raw, args.power_wait, report=connection_report)
                            port = backup.USBSerialPort(raw)
                            connection = build_connection(cc, probe_module, port, helper,
                                                          args.mode, payloads, files,
                                                          web_manifest=web_manifest)
                            connection.vinfo = cc.CC3x00VersionInfo.from_packet(payload)
                            if (not connection.vinfo.is_cc3200
                                    or connection.vinfo.bootloader[1] != 4):
                                raise InstallError("Unexpected device or ROM-loader version")
                            details["bootloader_version"] = list(connection.vinfo.bootloader)
                            phase = "RAM loader setup"
                            report("LOADER: loading only the pinned temporary SRAM helper")
                            connection.switch_to_nwp_bootloader()
                            if not connection.helper_executed:
                                raise InstallError("The pinned RAM helper did not execute")
                            phase = "queued filesystem operation"
                            with backup.QueuedUSBReceive(raw) as receiver:
                                port.transport = receiver
                                report("USB RX: 8 transfers queued before filesystem operations")
                                run_operation(connection, args.mode, key, candidate, original, files,
                                              details=details)
                            port.transport = raw
                        except BaseException as exc:
                            if isinstance(exc, KeyboardInterrupt):
                                raise
                            # USBSession records the escaping exception. Only
                            # sanitized messages may reach that error buffer.
                            raise _safe_exception(exc) from None
                except probe_module.CH340USBError:
                    if not details["operation_verified"] or not transport.details.get("cleanup_errors"):
                        raise
                    details["warnings"].append("USB cleanup needs a cable reconnect")
                details["success"] = True
                files.record("session-success", mode=args.mode)
                result = 0
            except KeyboardInterrupt:
                details["error"] = {"phase": phase, "type": "KeyboardInterrupt", "message": "Interrupted"}
                result = 130
            except Exception as exc:
                details["error"] = {"phase": phase, "type": type(exc).__name__, "message": _safe_error(exc)}
                if isinstance(exc, HandshakeError):
                    details["error"]["handshake"] = dict(exc.diagnostic)
                result = 1
            finally:
                if "error" in details:
                    details["success"] = False
                if connection is not None:
                    details["mutation_attempts"] = connection.mutation_attempts
                    details["application_write_attempted"] = connection.application_write_attempted
                    details["application_readback_verified"] = connection.application_verified
                    details["persistent_write_attempted"] = bool(connection.mutation_attempts)
                    details["helper_uploaded_bytes"] = connection.helper_sent
                    details["helper_executed"] = connection.helper_executed
                if transport is not None:
                    details["usb"] = _usb_summary(transport.details)
                details["completed_utc"] = datetime.now(timezone.utc).isoformat()
            files.publish(details)
        if details["success"]:
            report(SUCCESS_MARKERS[args.mode])
        else:
            error = details.get("error", {})
            report("STOPPED during " + error.get("phase", phase) + ": "
                   + error.get("message", "The operation did not verify"))
            if not details["persistent_write_attempted"]:
                report("No Pico flash or file write was attempted")
            if details.get("application_write_attempted") and not details.get("application_readback_verified"):
                report("APPLICATION: image2 may be incomplete; use a fresh restore session with the original backup")
            report("No retries or automatic recovery writes were attempted")
        report("REPORT: " + str(files.output))
        report("JOURNAL: " + str(files.journal_path))
        report("The application was not started; no boot-information, erase, delete, or format command was sent")
        if transport is not None and transport.details.get("cleanup_errors"):
            report("USB CLEANUP: reconnect USB before further use")
        return result
    except KeyboardInterrupt:
        report("STOPPED: interrupted")
        return 130
    except Exception as exc:
        report("STOPPED: " + _safe_error(exc))
        if files is not None:
            report("Keep the private journal and any partial report for recovery")
        return 1


if __name__ == "__main__":
    exit_code = main()
    if any(getattr(receiver.raw, "_queued_receive_cleanup_blocked", False)
           for receiver in backup._RETAINED_USB_RECEIVERS):
        sys.stdout.flush()
        sys.stderr.flush()
        os._exit(exit_code or 1)
    sys.exit(exit_code)
