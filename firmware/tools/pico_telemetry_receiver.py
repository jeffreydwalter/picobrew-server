#!/usr/bin/env python3
"""Optional Pico telemetry sink: receive live HTTP snapshots into SQLite.

This is a trusted-LAN reference receiver, not a required part of Pico Local.
It implements only POST /api/v1/telemetry, has no appliance control routes,
and does not use device API keys. HTTP is unencrypted. Bind to a LAN interface
only when you want other devices to reach it; the default is loopback only.

Run from the repository root, for example:
    python3 tools/pico_telemetry_receiver.py --bind 0.0.0.0 --port 8080

Then enable forwarding in the Pico's Settings using this computer's LAN host
name/IP and port 8080. Received snapshots accumulate in private/pico-telemetry.sqlite3.
Stopping this process does not affect brewing or on-device history. This sink
records delivered live snapshots; it does not request or backfill brew history.
"""
import argparse
from contextlib import closing
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import math
import os
from pathlib import Path
import re
import sqlite3
import stat
import sys
import time

ENDPOINT = "/api/v1/telemetry"
MAX_BODY_BYTES = 4096
READ_TIMEOUT_SECONDS = 5.0
INTEGER_FIELDS = {
    "uptime_ms": (0, 0xffffffff),
    "device_state": (0, 255), "phase": (0, 255),
    "controller_status": (0, 255), "compartment": (0, 255),
    "step_seconds": (-0x80000000, 0x7fffffff),
    "current_step": (-0x80000000, 0x7fffffff), "step_count": (0, 15),
}
STRING_FIELDS = {"device_uid", "command", "recipe_name", "step_name", "session_id", "stop_reason"}
BOOLEAN_FIELDS = {"ready", "busy", "stopped"}
TEMPERATURE_FIELDS = {"wort_f", "thermoblock_f", "target_f"}


def _unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate JSON key")
        result[key] = value
    return result


def _invalid_constant(_value):
    raise ValueError("nonfinite JSON number")


def validate_payload(raw):
    """Validate schema version 1 and preserve the exact UTF-8 JSON received."""
    text = raw.decode("utf-8")
    payload = json.loads(text, object_pairs_hook=_unique_object, parse_constant=_invalid_constant)
    if not isinstance(payload, dict) or set(payload) != {"schema_version", "firmware_version", "status"}:
        raise ValueError("invalid envelope")
    if type(payload["schema_version"]) is not int or payload["schema_version"] != 1:
        raise ValueError("unsupported schema version")
    version = payload["firmware_version"]
    if not isinstance(version, str) or not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
        raise ValueError("invalid firmware version")
    status = payload["status"]
    required = set(INTEGER_FIELDS) | STRING_FIELDS | BOOLEAN_FIELDS | TEMPERATURE_FIELDS | {"filter_inserted", "encoder"}
    if not isinstance(status, dict) or set(status) != required:
        raise ValueError("invalid status fields")
    for field, (minimum, maximum) in INTEGER_FIELDS.items():
        value = status[field]
        if type(value) is not int or not minimum <= value <= maximum:
            raise ValueError("invalid integer")
    if any(not isinstance(status[field], str) for field in STRING_FIELDS):
        raise ValueError("invalid string")
    if any(type(status[field]) is not bool for field in BOOLEAN_FIELDS):
        raise ValueError("invalid boolean")
    if status["stop_reason"] not in {"none", "requested", "filter-removed"}:
        raise ValueError("invalid stop reason")
    if status["filter_inserted"] is not None and type(status["filter_inserted"]) is not bool:
        raise ValueError("invalid filter state")
    for field in TEMPERATURE_FIELDS:
        value = status[field]
        if value is not None and (type(value) not in (int, float) or not math.isfinite(value)):
            raise ValueError("invalid temperature")
    encoder = status["encoder"]
    if not isinstance(encoder, dict) or set(encoder) != {"position", "min", "max"} or any(
            type(value) is not int or not -0x80000000 <= value <= 0x7fffffff for value in encoder.values()):
        raise ValueError("invalid encoder")
    return payload, text


class TelemetryStore:
    """Append-only received snapshots. Each acknowledgement follows a commit."""

    def __init__(self, database):
        self.database = Path(database).expanduser().absolute()
        self.database.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        flags = os.O_RDWR | os.O_CREAT | getattr(os, "O_NOFOLLOW", 0)
        descriptor = os.open(self.database, flags, 0o600)
        try:
            if not stat.S_ISREG(os.fstat(descriptor).st_mode):
                raise OSError("Database must be a regular file")
            if os.name == "posix":
                os.fchmod(descriptor, 0o600)
        finally:
            os.close(descriptor)
        with closing(self._connection()) as connection:
            connection.execute("PRAGMA journal_mode=DELETE")
            with connection:
                connection.execute("""CREATE TABLE IF NOT EXISTS snapshots (
                    id INTEGER PRIMARY KEY,
                    received_at TEXT NOT NULL,
                    device_uid TEXT NOT NULL,
                    session_id TEXT NOT NULL,
                    uptime_ms INTEGER NOT NULL,
                    firmware_version TEXT NOT NULL,
                    payload_json TEXT NOT NULL
                )""")
                connection.execute("""CREATE INDEX IF NOT EXISTS snapshots_device_time
                    ON snapshots(device_uid, received_at, id)""")

    def _connection(self):
        connection = sqlite3.connect(self.database, timeout=1.0)
        connection.execute("PRAGMA synchronous=FULL")
        return connection

    def append(self, payload, text):
        status = payload["status"]
        received_at = datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace("+00:00", "Z")
        with closing(self._connection()) as connection:
            with connection:
                connection.execute("""INSERT INTO snapshots
                    (received_at, device_uid, session_id, uptime_ms, firmware_version, payload_json)
                    VALUES (?, ?, ?, ?, ?, ?)""", (
                        received_at, status["device_uid"], status["session_id"], status["uptime_ms"],
                        payload["firmware_version"], text))


class TelemetryHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"
    timeout = READ_TIMEOUT_SECONDS

    def log_message(self, _format, *_args):
        # Never record request headers, paths, credentials, or telemetry in logs.
        pass

    def reply(self, code, error=None):
        self.close_connection = True
        body = (json.dumps({"error": error}) + "\n").encode() if error else b""
        self.send_response(code)
        self.send_header("Connection", "close")
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        if code == 405:
            self.send_header("Allow", "POST")
        self.end_headers()
        if self.command != "HEAD" and body:
            self.wfile.write(body)

    def _read_body(self, length):
        deadline = time.monotonic() + READ_TIMEOUT_SECONDS
        chunks = []
        while length:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError("body deadline exceeded")
            self.connection.settimeout(remaining)
            chunk = self.rfile.read1(length)
            if not chunk:
                raise ValueError("incomplete body")
            chunks.append(chunk)
            length -= len(chunk)
        return b"".join(chunks)

    def do_POST(self):
        if self.path != ENDPOINT:
            return self.reply(404, "not_found")
        if self.headers.get_all("Transfer-Encoding") is not None:
            return self.reply(400, "transfer_encoding_not_supported")
        if self.headers.get_all("Expect") is not None:
            return self.reply(417, "expectation_not_supported")
        if self.headers.get_all("Content-Encoding") is not None:
            return self.reply(415, "content_encoding_not_supported")
        types = self.headers.get_all("Content-Type", [])
        if len(types) != 1 or not re.fullmatch(
                r'application/json(?:\s*;\s*charset\s*=\s*(?:utf-8|"utf-8"))?\s*',
                types[0], flags=re.IGNORECASE):
            return self.reply(415, "application_json_required")
        lengths = self.headers.get_all("Content-Length", [])
        if not lengths:
            return self.reply(411, "content_length_required")
        if len(lengths) != 1 or not re.fullmatch(r"[0-9]{1,10}", lengths[0]):
            return self.reply(400, "invalid_content_length")
        length = int(lengths[0])
        if length > MAX_BODY_BYTES:
            return self.reply(413, "body_too_large")
        if length == 0:
            return self.reply(400, "empty_body")
        try:
            payload, text = validate_payload(self._read_body(length))
        except TimeoutError:
            return self.reply(408, "request_timeout")
        except (OSError, ValueError, OverflowError, RecursionError):
            return self.reply(400, "invalid_payload")
        try:
            self.server.store.append(payload, text)
        except sqlite3.Error:
            return self.reply(503, "storage_unavailable")
        self.reply(204)

    def reject_method(self):
        self.reply(405 if self.path == ENDPOINT else 404, "method_not_allowed" if self.path == ENDPOINT else "not_found")

    do_GET = do_HEAD = do_PUT = do_PATCH = do_DELETE = do_OPTIONS = reject_method


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--bind", default="127.0.0.1", help="Listen address (default: 127.0.0.1; use 0.0.0.0 for LAN access)")
    parser.add_argument("--port", type=int, default=8080, help="HTTP port (default: 8080)")
    parser.add_argument("--database", type=Path, default=Path("private/pico-telemetry.sqlite3"), help="SQLite file (default: private/pico-telemetry.sqlite3)")
    args = parser.parse_args(argv)
    if not 1 <= args.port <= 65535:
        parser.error("port must be between 1 and 65535")
    try:
        store = TelemetryStore(args.database)
        with HTTPServer((args.bind, args.port), TelemetryHandler) as server:
            server.store = store
            print(f"Receiving POST http://{args.bind}:{args.port}{ENDPOINT}", flush=True)
            print(f"SQLite: {store.database}", flush=True)
            try:
                server.serve_forever()
            except KeyboardInterrupt:
                pass
    except (OSError, sqlite3.Error) as error:
        print(f"Receiver stopped: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
