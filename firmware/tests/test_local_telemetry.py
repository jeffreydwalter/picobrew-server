#!/usr/bin/env python3
"""Test portable optional telemetry, persistent config and bounded transport."""
import ctypes as C
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Config(C.Structure):
    _fields_ = [("host", C.c_char * 128), ("port", C.c_uint16), ("enabled", C.c_bool)]


class Status(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in ("sent", "failed", "dropped", "last_success_ms")]
    _fields_ += [("last_http_status", C.c_uint16), ("state", C.c_int), ("last_error", C.c_int),
                ("resolved", C.c_bool), ("storage_error", C.c_bool)]


class TelemetryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        library = Path(cls.temp.name) / "telemetry.so"
        compiler = os.environ.get("CC", "cc")
        flags = ["-fPIC", "-std=c11", "-O1", "-Wall", "-Wextra", "-Werror",
                 "-DPICO_STANDALONE=1", "-I" + str(ROOT / "include")]
        core = Path(cls.temp.name) / "telemetry.o"
        subprocess.run([compiler, *flags, "-Dmalloc=tf_test_malloc", "-Dfree=tf_test_free",
                        "-c", str(ROOT / "src/local_telemetry.c"), "-o", str(core)], check=True)
        subprocess.run([compiler, "-shared", *flags, str(core), str(ROOT / "tests/local_telemetry_host.c"),
                        "-o", str(library)], check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.tf_config.argtypes = [C.c_char_p, C.c_int, C.c_int]
        cls.lib.tf_tick.argtypes = cls.lib.tf_clock.argtypes = [C.c_uint32]
        cls.lib.tf_response.argtypes = cls.lib.tf_snapshot.argtypes = [C.c_char_p]
        cls.lib.tf_transmitted.restype = C.c_char_p
        cls.lib.tf_address.restype = C.c_uint32

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.lib.tf_reset()

    def status(self):
        value = Status()
        self.lib.pico_telemetry_get_status(C.byref(value))
        return value

    def config(self):
        value = Config()
        self.lib.pico_telemetry_get_config(C.byref(value))
        return value

    def polls(self, count=30, delta=5):
        for _ in range(count):
            self.lib.tf_tick(delta)

    def enable(self, host=b"10.0.1.9", port=8080):
        self.assertEqual(self.lib.tf_config(host, port, 1), 0)

    def test_disabled_is_quiet_and_config_is_strict(self):
        self.polls()
        self.assertFalse(self.config().enabled)
        self.assertEqual(self.config().port, 8080)
        self.assertEqual(self.status().state, 0)
        self.assertEqual([self.lib.tf_counter(i) for i in range(7)], [0] * 7)
        for host in (b"", b"http://nas.local", b"nas/path", b"user@host", b"a\r\nX:Y", b"a b",
                     b"-host", b"host-", b"a..b", b"host.", b"a" * 64 + b".local", b"x" * 128,
                     b"999.1.2.3", b"0.0.0.0", b"255.255.255.255", b"::1", b"host:8080"):
            with self.subTest(host=host):
                self.assertEqual(self.lib.tf_config(host, 8080, 1), 1)
        self.assertEqual(self.lib.tf_config(b"nas.local", 0, 1), 1)
        self.assertEqual(self.lib.tf_counter(6), 0)

    def test_config_persists_and_failed_rewrite_keeps_previous_bank(self):
        self.enable(b"nas.local", 8081)
        writes = self.lib.tf_counter(6)
        self.enable(b"nas.local", 8081)
        self.assertEqual(self.lib.tf_counter(6), writes)
        self.lib.tf_reboot()
        self.assertEqual((self.config().host, self.config().port, self.config().enabled), (b"nas.local", 8081, True))
        for corrupt in (False, True):
            (self.lib.tf_corrupt_write if corrupt else self.lib.tf_fail_write)(1)
            self.assertEqual(self.lib.tf_config(b"new.local", 8090, 1), 3)
            self.assertTrue(self.status().storage_error)
            self.lib.tf_reboot()
            self.assertEqual(self.config().host, b"nas.local")

    def test_literal_ip_partial_io_and_canonical_frame_without_key(self):
        self.enable()
        self.lib.tf_behavior(3, 7)
        self.lib.tf_behavior(5, 2)
        self.lib.tf_busy(1)
        self.polls(200)
        state = self.status()
        self.assertEqual((state.sent, state.failed, state.last_http_status), (1, 0, 204))
        self.assertEqual(self.lib.tf_counter(0), 0)
        self.assertEqual(self.lib.tf_counter(5), 1)
        request = self.lib.tf_transmitted().decode()
        header, body = request.split("\r\n\r\n", 1)
        self.assertTrue(header.startswith("POST /api/v1/telemetry HTTP/1.0\r\nHost: 10.0.1.9:8080"))
        self.assertNotIn("Authorization", header)
        self.assertNotIn("api.key", request)
        self.assertIn(f"Content-Length: {len(body)}", header)
        payload = json.loads(body)
        self.assertEqual(payload["firmware_version"], "1.1.0")
        self.assertEqual(payload["schema_version"], 1)
        self.assertEqual(payload["status"]["wort_f"], 153.2)
        self.assertEqual(self.lib.tf_address(), 0x0a000109)
        self.assertEqual(self.lib.tf_counter(7), 0)

    def test_dns_only_while_idle_cached_during_brew_and_cleared_offline(self):
        self.enable(b"nas.local")
        self.lib.tf_busy(1)
        self.polls()
        self.assertEqual((self.lib.tf_counter(0), self.lib.tf_counter(1)), (0, 0))
        self.assertEqual(self.status().state, 2)
        self.lib.tf_busy(0)
        self.lib.tf_tick(15000)
        self.polls()
        first = self.lib.tf_address()
        self.assertEqual(self.lib.tf_counter(0), 1)
        self.lib.tf_busy(1)
        self.lib.tf_tick(300000)
        self.polls()
        self.assertEqual(self.lib.tf_counter(0), 1)
        self.assertEqual(self.lib.tf_address(), first)
        self.lib.tf_network(0)
        self.lib.tf_tick(1)
        self.assertFalse(self.status().resolved)
        self.lib.tf_network(1)
        self.lib.tf_tick(15000)
        self.polls()
        self.assertEqual(self.status().state, 2)
        self.assertEqual(self.lib.tf_counter(0), 1)
        self.lib.tf_busy(0)
        self.lib.tf_tick(15000)
        self.polls()
        self.assertEqual(self.lib.tf_counter(0), 2)
        self.assertNotEqual(self.lib.tf_address(), first)
        self.assertEqual(self.lib.tf_counter(7), 0)

    def test_connect_timeout_is_nonblocking_closes_and_backs_off(self):
        self.enable()
        self.lib.tf_behavior(2, 1)
        self.lib.tf_tick(1)
        for _ in range(5):
            old = self.lib.tf_counter(2)
            self.lib.tf_tick(500)
            self.assertEqual(self.lib.tf_counter(2) - old, 1)
        self.lib.tf_tick(2500)
        status = self.status()
        self.assertEqual((status.failed, status.last_error, self.lib.tf_counter(5)), (1, 7, 1))
        opens = self.lib.tf_counter(1)
        self.lib.tf_tick(15000)
        self.assertEqual(self.lib.tf_counter(1), opens)
        self.assertGreater(self.status().dropped, 0)
        self.lib.tf_behavior(2, 0)
        self.lib.tf_tick(30000)
        self.polls()
        self.assertEqual(self.status().sent, 1)

    def test_dns_socket_send_receive_http_failures_do_not_mutate_configuration(self):
        for field, result, error in ((0, -1, 1), (1, -1, 2), (2, -1, 3), (3, -1, 4), (4, -1, 5)):
            with self.subTest(field=field):
                self.lib.tf_reset()
                self.enable(b"nas.local" if field == 0 else b"10.0.1.9")
                self.lib.tf_behavior(field, result)
                self.polls()
                self.assertEqual((self.status().failed, self.status().last_error), (1, error))
                self.assertTrue(self.config().enabled)
                self.assertEqual(self.lib.tf_counter(6), 1)
        for response in (b"HTTP/1.1 503 Unavailable\r\n", b"garbage header bytes", b"HTTP/1.1 2000 invalid\r\n"):
            self.lib.tf_reset()
            self.enable()
            self.lib.tf_response(response)
            self.polls()
            self.assertEqual(self.status().sent, 0)
            self.assertEqual(self.status().last_error, 6)

    def test_configuration_busy_no_isr_io_and_disable_closes_inflight_socket(self):
        self.lib.tf_busy(1)
        self.assertEqual(self.lib.tf_config(b"nas.local", 8080, 1), 2)
        self.assertEqual(self.lib.tf_counter(6), 0)
        self.lib.tf_busy(0)
        self.enable()
        self.lib.tf_tick(1)
        self.lib.tf_foreground(0)
        self.polls()
        self.assertEqual(self.lib.tf_counter(2), 0)
        self.lib.tf_foreground(1)
        self.assertEqual(self.lib.tf_config(b"10.0.1.9", 8080, 0), 0)
        self.assertEqual(self.lib.tf_counter(5), 1)
        self.polls()
        self.assertEqual(self.status().state, 0)
        self.assertEqual(self.lib.tf_counter(7), 0)

    def test_maximum_host_snapshot_and_capacity_failure_are_bounded(self):
        self.enable(b"a" * 63 + b"." + b"b" * 63)
        payload = {"device_uid": '"' * 32, "session_id": "s" * 31, "recipe_name": "r" * 31,
                   "step_name": "t" * 31, "phase": 8, "wort_f": None, "thermoblock_f": 400.5,
                   "padding": "x" * 250}
        self.lib.tf_snapshot(json.dumps(payload, separators=(",", ":")).encode())
        self.polls()
        self.assertEqual(self.status().sent, 1)
        self.assertLessEqual(len(self.lib.tf_transmitted()), 1152)
        body = self.lib.tf_transmitted().decode().split("\r\n\r\n", 1)[1]
        self.assertEqual(json.loads(body)["status"], payload)
        self.lib.tf_snapshot(json.dumps({"padding": "x" * 950}).encode())
        self.lib.tf_tick(15000)
        self.assertEqual(self.status().last_error, 9)
        self.assertEqual(self.lib.tf_counter(1), 1)

    def test_explicit_network_change_forgets_socket_and_dns_without_poll_gap(self):
        self.enable(b"nas.local")
        self.lib.tf_tick(1)
        self.assertTrue(self.status().resolved)
        self.lib.pico_telemetry_network_changed()
        self.assertEqual(self.lib.tf_counter(5), 1)
        self.assertFalse(self.status().resolved)
        self.lib.tf_busy(1)
        self.lib.tf_tick(15000)
        self.assertEqual(self.lib.tf_counter(0), 1)
        self.assertEqual(self.status().state, 2)
        self.lib.tf_busy(0)
        self.lib.tf_tick(15000)
        self.polls()
        self.assertEqual(self.lib.tf_counter(0), 2)

    def test_one_lazy_frame_reused_and_freed_when_disabled_or_rebound(self):
        self.enable()
        self.assertEqual(self.lib.tf_heap(0), 0)
        self.polls()
        self.lib.tf_tick(15000)
        self.polls()
        self.assertEqual([self.lib.tf_heap(i) for i in (0, 1, 3, 4, 5)], [1, 1, 1, 1152, 0])
        self.assertEqual(self.lib.tf_config(b"10.0.1.9", 8080, 0), 0)
        self.assertEqual([self.lib.tf_heap(i) for i in (2, 3, 4)], [1, 0, 0])
        self.enable()
        self.lib.tf_tick(1)
        self.assertEqual(self.lib.tf_heap(3), 1)
        closes = self.lib.tf_counter(5)
        self.lib.tf_reboot()
        self.assertEqual(self.lib.tf_counter(5), closes + 1)
        self.assertEqual([self.lib.tf_heap(i) for i in (2, 3, 5)], [2, 0, 0])

    def test_allocation_failure_drops_before_dns_socket_or_snapshot(self):
        self.enable(b"nas.local")
        self.lib.tf_allocation_failure(1)
        self.polls()
        self.assertEqual((self.status().failed, self.status().dropped, self.status().last_error), (1, 1, 9))
        self.assertEqual([self.lib.tf_counter(i) for i in (0, 1, 2, 3, 4, 5, 8)], [0] * 7)
        self.assertEqual(self.lib.tf_heap(3), 0)
        self.lib.tf_allocation_failure(0)
        self.lib.tf_tick(45000)
        self.polls()
        self.assertEqual(self.status().sent, 1)
        self.assertEqual(self.lib.tf_heap(0), 1)
        self.assertEqual(self.lib.tf_heap(5), 0)


class NativeTelemetryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        folder = Path(cls.temp.name)
        (folder / "utility").mkdir()
        (folder / "WiFi.h").write_text("#define WL_CONNECTED 3\n#define WL_AP_MODE 8\n"
            "class WiFiClass { public: static bool _initialized; int status(); }; extern WiFiClass WiFi;\n")
        (folder / "utility/simplelink.h").write_text("#pragma once\n#include <stdint.h>\n"
            "typedef uint8_t _u8; typedef int8_t _i8; typedef uint16_t _u16; typedef int16_t _i16; "
            "typedef uint32_t _u32; typedef int32_t _i32;\n")
        (folder / "utility/fs.h").write_text('''#pragma once
#include "simplelink.h"
#define SL_FS_ERR_FILE_NOT_EXISTS (-11)
#define FS_MODE_OPEN_READ 0
#define FS_MODE_OPEN_WRITE 0x1000
#define FS_MODE_OPEN_CREATE(size, flags) (0x2000u | ((_u32)(size) << 16) | (flags))
typedef struct { _u32 FileLen, AllocatedLen; } SlFsFileInfo_t;
_i32 sl_FsGetInfo(const _u8 *, _u32, SlFsFileInfo_t *);
_i32 sl_FsOpen(const _u8 *, _u32, _u32 *, _i32 *);
_i32 sl_FsRead(_i32, _u32, _u8 *, _u32);
_i32 sl_FsWrite(_i32, _u32, _u8 *, _u32);
_i32 sl_FsClose(_i32, const _u8 *, const _u8 *, _u32);
''')
        (folder / "utility/netapp.h").write_text('#pragma once\n#include "simplelink.h"\n'
            '_i16 sl_NetAppDnsGetHostByName(_i8 *, _u16, _u32 *, _u8);\n')
        (folder / "utility/socket.h").write_text('''#pragma once
#include "simplelink.h"
#define SL_AF_INET 2
#define SL_SOCK_STREAM 1
#define SL_IPPROTO_TCP 6
#define SL_SOL_SOCKET 1
#define SL_SO_NONBLOCKING 2
#define SL_EAGAIN (-11)
#define SL_EALREADY (-114)
#define SL_EISCONN (-106)
typedef struct { _u32 NonblockingEnabled; } SlSockNonblocking_t;
typedef struct { _u16 sin_family, sin_port; struct { _u32 s_addr; } sin_addr; } SlSockAddrIn_t;
typedef struct { _u16 family; _u8 data[14]; } SlSockAddr_t;
_i16 sl_Socket(_i16, _i16, _i16);
_i16 sl_SetSockOpt(_i16, _i16, _i16, const void *, _i16);
_u16 sl_Htons(_u16); _u32 sl_Htonl(_u32);
_i16 sl_Connect(_i16, const SlSockAddr_t *, _i16);
_i16 sl_Send(_i16, const void *, _i16, _i16);
_i16 sl_Recv(_i16, void *, _i16, _i16);
_i16 sl_Close(_i16);
''')
        library = folder / "native-telemetry.so"
        subprocess.run([os.environ.get("CXX", "c++"), "-shared", "-fPIC", "-std=c++11", "-O1",
                        "-Wall", "-Wextra", "-Werror", "-I" + str(folder), "-I" + str(ROOT / "include"),
                        str(ROOT / "platform/energia_telemetry.cpp"), str(ROOT / "tests/energia_telemetry_fixture.cpp"),
                        "-o", str(library)], check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.nt_write.argtypes = [C.c_char_p, C.c_size_t]
        for name in ("nt_mode", "nt_port", "nt_address"):
            getattr(cls.lib, name).restype = C.c_uint

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.lib.nt_reset()

    def trace(self):
        return [self.lib.nt_trace(i) for i in range(self.lib.nt_calls())]

    def test_socket_nonblocking_before_use_and_option_failure_closes(self):
        self.assertEqual(self.lib.nt_open(), 7)
        self.assertEqual(self.trace(), [5, 6])
        self.lib.nt_reset()
        self.lib.nt_result(6, -1)
        self.assertEqual(self.lib.nt_open(), -1)
        self.assertEqual(self.trace(), [5, 6, 10])
        self.assertEqual(self.lib.nt_bad(), 0)

    def test_native_connect_pending_success_error_and_host_byte_order(self):
        for code, expected in ((0, 0), (-106, 0), (-114, 1), (-11, 1), (-55, -1)):
            self.lib.nt_result(7, code)
            self.assertEqual(self.lib.nt_connect(), expected)
        self.assertEqual(self.lib.nt_address(), 0x0a000109)
        self.assertEqual(self.lib.nt_port(), 8080)
        self.assertEqual(self.lib.nt_bad(), 0)

    def test_native_partial_io_and_would_block_map_without_retry(self):
        for call, field in ((self.lib.nt_send, 8), (self.lib.nt_receive, 9)):
            for code, expected in ((9999, 32), (7, 7), (-11, 0), (0, -1), (-42, -1)):
                self.lib.nt_reset()
                self.lib.nt_result(field, code)
                self.assertEqual(call(), expected)
                self.assertEqual(self.lib.nt_calls(), 1)

    def test_native_dns_and_nwp_close_lifecycle(self):
        self.assertEqual(self.lib.nt_resolve(), 0)
        self.assertEqual(self.trace(), [11])
        self.lib.nt_reset()
        self.lib.nt_result(11, -159)
        self.assertEqual(self.lib.nt_resolve(), -1)
        self.lib.nt_network(0, 3)
        self.lib.nt_close()
        self.assertEqual(self.trace(), [11])
        self.assertEqual(self.lib.nt_ready(), 0)
        self.lib.nt_network(1, 0)
        self.assertEqual(self.lib.nt_ready(), 0)
        self.lib.nt_network(1, 3)
        self.lib.nt_close()
        self.assertEqual(self.trace(), [11, 10])

    def test_native_config_scope_existing_write_and_error_close(self):
        self.assertEqual(self.lib.nt_write(b"/local/telemetry.a", 160), 0)
        self.assertEqual(self.lib.nt_mode(), 0x2000 | (160 << 16))
        self.lib.nt_reset()
        self.lib.nt_result(0, 0)
        self.assertEqual(self.lib.nt_write(b"/local/telemetry.b", 160), 0)
        self.assertEqual(self.lib.nt_mode(), 0x1000)
        self.lib.nt_reset()
        self.lib.nt_result(3, 7)
        self.assertEqual(self.lib.nt_write(b"/local/telemetry.a", 160), -1)
        self.assertEqual(self.trace(), [0, 1, 3, 4])
        self.lib.nt_reset()
        for path in (b"/sys/mcuimg2.bin", b"/local/api.key", b"/local/history.meta.a", b"/local/telemetry.a/../api.key"):
            self.assertEqual(self.lib.nt_write(path, 160), -1)
        self.assertEqual(self.lib.nt_calls(), 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
