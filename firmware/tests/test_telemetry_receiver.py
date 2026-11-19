#!/usr/bin/env python3
"""Exercise receiver HTTP framing and real SQLite commits without binding sockets."""
from contextlib import closing, redirect_stdout
from datetime import datetime, timezone
import importlib.util
import io
import json
import os
from pathlib import Path
import sqlite3
import stat
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("pico_telemetry_receiver", ROOT / "tools/pico_telemetry_receiver.py")
receiver = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(receiver)


def snapshot():
    return {"schema_version": 1, "firmware_version": "1.0.0", "status": {
        "uptime_ms": 123456, "ready": True, "device_uid": "test-pico-0123456789",
        "busy": True, "stopped": False, "stop_reason": "none", "command": "brew",
        "device_state": 3, "phase": 1, "controller_status": 0, "compartment": 2,
        "wort_f": 152.2, "thermoblock_f": 180.3, "target_f": 152.0,
        "step_seconds": 1200, "current_step": 1, "step_count": 5,
        "recipe_name": 'Test "Pale" Ale', "step_name": "Mash", "session_id": "local-1",
        "filter_inserted": True, "encoder": {"position": 0, "min": -100, "max": 100}}}


class FakeSocket:
    def __init__(self, wire, reader=io.BytesIO, on_write=None):
        self.reader = reader(wire)
        self.output = bytearray()
        self.timeouts = []
        self.on_write = on_write

    def makefile(self, mode, _buffering=None):
        assert mode == "rb"
        return self.reader

    def sendall(self, data):
        self.output.extend(data)
        if self.on_write:
            self.on_write(bytes(data))

    def settimeout(self, seconds):
        self.timeouts.append(seconds)


class ReceiverTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.database = Path(self.temp.name) / "private" / "pico.sqlite3"
        self.store = receiver.TelemetryStore(self.database)

    def tearDown(self):
        self.temp.cleanup()

    def rows(self):
        with closing(sqlite3.connect(self.database)) as connection:
            connection.row_factory = sqlite3.Row
            return [dict(row) for row in connection.execute("SELECT * FROM snapshots ORDER BY id")]

    def exchange(self, body=None, *, method="POST", path=receiver.ENDPOINT, headers=None,
                 reader=io.BytesIO, on_write=None, suffix=b""):
        if body is None:
            body = json.dumps(snapshot(), separators=(",", ":")).encode()
        if headers is None:
            headers = [("Content-Type", "application/json"), ("Content-Length", str(len(body)))]
        head = f"{method} {path} HTTP/1.1\r\nHost: fixture\r\n"
        head += "".join(f"{key}: {value}\r\n" for key, value in headers) + "\r\n"
        connection = FakeSocket(head.encode() + body + suffix, reader=reader, on_write=on_write)
        server = SimpleNamespace(store=self.store, server_name="fixture", server_port=8080)
        receiver.TelemetryHandler(connection, ("127.0.0.1", 12345), server)
        response = bytes(connection.output)
        code = int(response.split(b" ", 2)[1])
        self.assertIn(b"Connection: close\r\n", response)
        self.assertTrue(connection.timeouts)
        self.assertTrue(all(0 < value <= receiver.READ_TIMEOUT_SECONDS for value in connection.timeouts))
        return code, response

    def test_accepted_snapshot_is_durable_before_ack_and_preserves_raw_json(self):
        document = snapshot()
        document["status"]["device_uid"] = "device'; DROP TABLE snapshots; --"
        raw = json.dumps(document, indent=2).encode()
        before = datetime.now(timezone.utc)

        def acknowledge(data):
            if b" 204 " in data:
                self.assertEqual(len(self.rows()), 1, "204 was sent before the SQLite commit")

        code, response = self.exchange(raw, on_write=acknowledge)
        self.assertEqual(code, 204)
        self.assertTrue(response.endswith(b"\r\n\r\n"))
        receiver.TelemetryStore(self.database)  # Existing file remains readable after reopening.
        row, = self.rows()
        self.assertEqual(row["payload_json"], raw.decode())
        self.assertEqual(row["device_uid"], document["status"]["device_uid"])
        self.assertEqual((row["session_id"], row["uptime_ms"], row["firmware_version"]), ("local-1", 123456, "1.0.0"))
        received = datetime.fromisoformat(row["received_at"].replace("Z", "+00:00"))
        self.assertGreaterEqual(received.timestamp() + .001, before.timestamp())
        self.assertLessEqual(received, datetime.now(timezone.utc))

    def test_nullable_readings_and_maximum_uptime_are_preserved(self):
        document = snapshot()
        document["status"].update(ready=False, filter_inserted=None, wort_f=None,
                                  thermoblock_f=None, target_f=None, uptime_ms=0xffffffff)
        raw = json.dumps(document).encode()
        headers = [("Content-Type", 'application/json; charset="UTF-8"'), ("Content-Length", str(len(raw)))]
        self.assertEqual(self.exchange(raw, headers=headers)[0], 204)
        self.assertEqual(json.loads(self.rows()[0]["payload_json"]), document)

    def test_body_limit_counts_utf8_bytes_and_accepts_exactly_4096(self):
        document = snapshot()
        document["status"]["recipe_name"] = "Crème brûlée"
        raw = json.dumps(document, ensure_ascii=False).encode("utf-8")
        raw += b" " * (receiver.MAX_BODY_BYTES - len(raw))
        self.assertEqual(len(raw), 4096)
        self.assertEqual(self.exchange(raw)[0], 204)
        self.assertEqual(self.exchange(raw + b" ")[0], 413)
        row, = self.rows()
        self.assertEqual(row["payload_json"].encode("utf-8"), raw)

    def test_protocol_errors_reject_before_body_read_and_do_not_write(self):
        class NoBodyRead(io.BytesIO):
            def read1(self, _size=-1):
                raise AssertionError("Rejected headers must not cause body reads")

        valid = [("Content-Type", "application/json"), ("Content-Length", "2")]
        cases = [
            ([valid[0]], 411),
            ([valid[0], ("Content-Length", "-1")], 400),
            ([valid[0], ("Content-Length", "+2")], 400),
            ([valid[0], ("Content-Length", "two")], 400),
            ([valid[0], ("Content-Length", "2, 2")], 400),
            (valid + [("Content-Length", "2")], 400),
            ([valid[0], ("Content-Length", "0")], 400),
            ([valid[0], ("Content-Length", str(receiver.MAX_BODY_BYTES + 1))], 413),
            (valid + [("Transfer-Encoding", "chunked")], 400),
            (valid + [("Transfer-Encoding", "identity")], 400),
            ([valid[1]], 415),
            ([("Content-Type", "text/plain"), valid[1]], 415),
            ([("Content-Type", "application/json; charset=utf-16"), valid[1]], 415),
            (valid + [("Content-Type", "application/json")], 415),
            (valid + [("Content-Encoding", "gzip")], 415),
            (valid + [("Expect", "100-continue")], 417),
        ]
        for headers, expected in cases:
            with self.subTest(headers=headers):
                self.assertEqual(self.exchange(b"{}", headers=headers, reader=NoBodyRead)[0], expected)
        self.assertEqual(self.rows(), [])

    def test_invalid_payloads_are_rejected_without_storing_partial_records(self):
        raw = json.dumps(snapshot()).encode()
        cases = [b"null", b"[]", b"{}", b"{", raw + b"junk", b"\xff",
                 raw.replace(b'"schema_version": 1', b'"schema_version": 1, "schema_version": 1'),
                 raw.replace(b'"wort_f": 152.2', b'"wort_f": NaN'),
                 raw.replace(b'"wort_f": 152.2', b'"wort_f": 1e999'),
                 raw.replace(b'"wort_f": 152.2', b'"wort_f": ' + b'9' * 1000)]
        for location, value in (("schema_version", True), ("schema_version", 2), ("firmware_version", "invalid"),
                                ("status", None), ("uptime_ms", True), ("uptime_ms", -1),
                                ("uptime_ms", 0x100000000), ("ready", 1), ("encoder", {}),
                                ("filter_inserted", "yes"), ("stop_reason", "unknown"), ("session_id", 1)):
            document = snapshot()
            (document if location in document else document["status"])[location] = value
            cases.append(json.dumps(document).encode())
        document = snapshot()
        del document["status"]["step_seconds"]
        cases.append(json.dumps(document).encode())
        for body in cases:
            with self.subTest(body=body[:80]):
                self.assertEqual(self.exchange(body)[0], 400)
        self.assertEqual(self.rows(), [])

    def test_truncated_body_and_absolute_body_deadline_never_commit(self):
        raw = json.dumps(snapshot()).encode()
        headers = [("Content-Type", "application/json"), ("Content-Length", str(len(raw)))]
        self.assertEqual(self.exchange(raw[:40], headers=headers)[0], 400)

        class TimeoutBody(io.BytesIO):
            def read1(self, _size=-1):
                raise TimeoutError("simulated socket timeout")

        self.assertEqual(self.exchange(raw, reader=TimeoutBody)[0], 408)

        class SlowBody(io.BytesIO):
            def read1(self, _size=-1):
                return super().read1(1)

        with patch.object(receiver.time, "monotonic", side_effect=[0, 2, 4, 6]):
            self.assertEqual(self.exchange(raw, reader=SlowBody)[0], 408)
        self.assertEqual(self.rows(), [])

    def test_routes_methods_and_pipelined_second_request_cannot_control_or_duplicate(self):
        for method in ("GET", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS"):
            with self.subTest(method=method):
                self.assertEqual(self.exchange(method=method)[0], 405)
        for path in ("/", "/api/v1/commands", "/api/v1/status", receiver.ENDPOINT + "?x=1"):
            with self.subTest(path=path):
                self.assertEqual(self.exchange(path=path)[0], 404)
        self.assertEqual(self.rows(), [])
        document = json.dumps(snapshot()).encode()
        second = (f"POST {receiver.ENDPOINT} HTTP/1.1\r\nContent-Type: application/json\r\n"
                  f"Content-Length: {len(document)}\r\n\r\n").encode() + document
        code, response = self.exchange(document, suffix=second)
        self.assertEqual(code, 204)
        self.assertEqual(response.count(b"HTTP/1.0 204"), 1)
        self.assertEqual(len(self.rows()), 1)

    def test_failed_commit_is_not_acknowledged_and_exception_details_stay_private(self):
        with patch.object(self.store, "append", side_effect=sqlite3.OperationalError("private database detail")):
            code, response = self.exchange()
        self.assertEqual(code, 503)
        self.assertNotIn(b"private database detail", response)
        self.assertEqual(self.rows(), [])

    def test_cli_defaults_to_loopback_and_allows_explicit_lan_binding(self):
        for options, address in (([], ("127.0.0.1", 8080)), (["--bind", "0.0.0.0", "--port", "8123"], ("0.0.0.0", 8123))):
            with self.subTest(address=address), patch.object(receiver, "HTTPServer") as server, redirect_stdout(io.StringIO()):
                server.return_value.__enter__.return_value.serve_forever.side_effect = KeyboardInterrupt()
                self.assertEqual(receiver.main(["--database", str(self.database), *options]), 0)
                server.assert_called_once_with(address, receiver.TelemetryHandler)

    @unittest.skipUnless(os.name == "posix", "POSIX file permissions")
    def test_database_is_private_and_symlink_target_is_not_followed(self):
        self.assertEqual(stat.S_IMODE(self.database.stat().st_mode), 0o600)
        self.assertEqual(stat.S_IMODE(self.database.parent.stat().st_mode), 0o700)
        self.database.chmod(0o666)
        receiver.TelemetryStore(self.database)
        self.assertEqual(stat.S_IMODE(self.database.stat().st_mode), 0o600)
        if hasattr(os, "O_NOFOLLOW"):
            link = self.database.parent / "link.sqlite3"
            link.symlink_to(self.database)
            with self.assertRaises(OSError):
                receiver.TelemetryStore(link)

    def test_fixture_covers_the_current_firmware_status_schema(self):
        spec = json.loads((ROOT / "docs/openapi.json").read_text())
        self.assertEqual(set(snapshot()["status"]), set(spec["components"]["schemas"]["Status"]["required"]))


if __name__ == "__main__":
    unittest.main(verbosity=2)
