#!/usr/bin/env python3
"""Exercise actual C history storage with destructive file rewrites and resets."""
import ctypes as C
import math
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Metadata(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in (
        "id", "duration_ms", "record_count", "persisted_record_count", "gap_count", "first_elapsed_ms")]
    _fields_ += [("state", C.c_int), ("available", C.c_bool), ("truncated", C.c_bool),
                ("storage_error", C.c_bool), ("recovered_after_restart", C.c_bool),
                ("command", C.c_char * 16), ("recipe_name", C.c_char * 32)]


class Sample(C.Structure):
    _fields_ = [("elapsed_ms", C.c_uint32), ("step_seconds", C.c_int32)]
    _fields_ += [(name, C.c_int16) for name in ("wort", "thermoblock", "target")]
    _fields_ += [(name, C.c_uint8) for name in ("phase", "controller_status", "compartment", "current_step", "flags", "device_state")]


class HistoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        library = Path(cls.temp.name) / "history.so"
        compiler = os.environ.get("CC", "cc")
        flags = ["-fPIC", "-std=c11", "-O1", "-Wall", "-Wextra", "-Werror",
                 "-DPICO_STANDALONE=1", "-I" + str(ROOT / "include")]
        core = Path(cls.temp.name) / "history.o"
        # Rename allocation calls only inside the module under test; fixture
        # wrappers still call the real allocator, never affecting Python.
        subprocess.run([compiler, *flags, "-Dmalloc=hf_test_malloc", "-Dfree=hf_test_free",
                        "-c", str(ROOT / "src/local_history.c"), "-o", str(core)], check=True)
        subprocess.run([compiler, "-shared", *flags, str(core), str(ROOT / "tests/local_history_host.c"),
                        "-o", str(library)], check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.hf_start.argtypes = [C.c_char_p]
        cls.lib.hf_tick.argtypes = [C.c_uint32]
        cls.lib.hf_clock.argtypes = [C.c_uint32]
        cls.lib.hf_values.argtypes = [C.c_double, C.c_double, C.c_double]
        cls.lib.hf_name.argtypes = [C.c_char_p]
        cls.lib.pico_history_count.restype = C.c_size_t
        cls.lib.pico_history_at.argtypes = [C.c_size_t, C.POINTER(Metadata)]
        cls.lib.pico_history_at.restype = C.c_bool
        cls.lib.pico_history_find.argtypes = [C.c_uint32, C.POINTER(Metadata)]
        cls.lib.pico_history_find.restype = C.c_bool
        cls.lib.pico_history_read.argtypes = [C.c_uint32, C.c_uint32, C.POINTER(Sample), C.c_size_t, C.POINTER(C.c_size_t)]

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.lib.hf_reset()

    def metadata(self, ordinal=0):
        value = Metadata()
        self.lib.pico_history_at(ordinal, C.byref(value))
        return value

    def page(self, identity, offset=0, capacity=32):
        values = (Sample * capacity)()
        count = C.c_size_t()
        result = self.lib.pico_history_read(identity, offset, values, capacity, C.byref(count))
        return result, list(values[:count.value])

    def all_samples(self, identity):
        values = []
        while True:
            result, page = self.page(identity, len(values))
            self.assertEqual(result, 0)
            if not page:
                return values
            values.extend(page)

    def brew(self, ticks, finish=True):
        self.lib.hf_busy(1)
        self.assertEqual(self.lib.hf_start(b"brew"), 1)
        identity = self.metadata().id
        for _ in range(ticks):
            self.lib.hf_tick(15000)
        if finish:
            self.lib.hf_finish(0)
        return identity

    def test_start_is_explicit_and_maintenance_cannot_evict_history(self):
        self.assertFalse(self.metadata().available)
        for command in (b"rinse", b"settings", b"utilities", b"drain-filter", b"front-panel", b"idle"):
            self.assertEqual(self.lib.hf_start(command), 0)
        self.assertEqual(self.lib.hf_writes(), 0)
        identity = self.brew(2)
        writes = self.lib.hf_writes()
        self.assertEqual(self.lib.hf_start(b"rinse"), 0)
        self.assertEqual(self.metadata().id, identity)
        self.assertEqual(self.lib.hf_writes(), writes)

    def test_live_tail_phase_changes_temperatures_and_checkpoint_recovery(self):
        self.brew(2, finish=False)
        identity = self.metadata().id
        self.lib.hf_values(153.2, 180.1, 154.0)
        self.lib.hf_phase(1, 2)
        self.lib.hf_tick(10)
        meta = self.metadata()
        self.assertEqual(meta.record_count, 4)
        self.assertEqual(meta.persisted_record_count, 1)
        samples = self.all_samples(identity)
        self.assertEqual([v.elapsed_ms for v in samples], [0, 15000, 30000, 30010])
        self.assertEqual((samples[-1].wort, samples[-1].thermoblock, samples[-1].target), (1532, 1801, 1540))
        self.assertEqual((samples[-1].phase, samples[-1].current_step), (1, 2))
        self.lib.hf_tick(30000)
        committed = self.metadata().record_count
        self.assertEqual(self.metadata().persisted_record_count, committed)
        self.lib.hf_tick(15000)
        self.lib.hf_reboot()
        meta = self.metadata()
        self.assertEqual(meta.state, 3)
        self.assertTrue(meta.recovered_after_restart)
        self.assertEqual(meta.record_count, committed)
        self.assertEqual(len(self.all_samples(identity)), committed)

    def test_completed_history_survives_reboot_and_page_crosses_chunk_boundary(self):
        identity = self.brew(200)
        before = self.metadata()
        self.assertEqual(before.state, 2)
        self.assertEqual(before.persisted_record_count, before.record_count)
        result, values = self.page(identity, 170)
        self.assertEqual(result, 0)
        self.assertEqual(len(values), before.record_count - 170)
        self.assertEqual(values[0].elapsed_ms, 170 * 15000)
        self.lib.hf_reboot()
        self.assertEqual(self.metadata().record_count, before.record_count)
        self.assertEqual(self.metadata().state, 2)
        self.assertFalse(self.metadata().recovered_after_restart)
        self.assertEqual(len(self.all_samples(identity)), before.record_count)

    def test_inventory_bound_retains_newest_eight_and_ids_guard_reads(self):
        identities = [self.brew(1) for _ in range(10)]
        self.assertEqual(self.lib.pico_history_count(), 8)
        self.assertEqual([self.metadata(i).id for i in range(8)], identities[-1:1:-1])
        self.assertEqual(self.page(identities[0])[0], 1)
        self.assertEqual(self.page(identities[-1], 100)[0], 2)
        self.lib.hf_reboot()
        self.assertEqual([self.metadata(i).id for i in range(8)], identities[-1:1:-1])

    def test_chunk_retention_evicts_only_whole_old_histories(self):
        identities = [self.brew(720) for _ in range(5)]
        self.assertEqual(self.lib.pico_history_count(), 4)
        self.assertEqual([self.metadata(i).id for i in range(4)], identities[-1:0:-1])
        self.assertEqual(self.page(identities[0])[0], 1)
        for identity in identities[1:]:
            values = self.all_samples(identity)
            self.assertEqual(values[0].elapsed_ms, 0)
            self.assertEqual(values[-1].elapsed_ms, 720 * 15000)
        self.assertLessEqual(self.lib.hf_file_count(), 28)
        self.lib.hf_reboot()
        self.assertEqual([self.metadata(i).id for i in range(4)], identities[-1:0:-1])

    def test_active_capacity_is_explicit_and_preserves_beginning(self):
        identity = self.brew(4250, finish=False)
        meta = self.metadata()
        self.assertEqual(meta.record_count, 4224)
        self.assertTrue(meta.truncated)
        self.assertEqual(meta.state, 1)
        self.assertEqual(self.page(identity)[1][0].elapsed_ms, 0)
        self.lib.hf_finish(0)
        self.lib.hf_reboot()
        self.assertTrue(self.metadata().truncated)
        self.assertEqual(self.metadata().record_count, 4224)

    def test_sampling_gap_missing_temperatures_and_timer_wrap(self):
        self.lib.hf_clock(0xfffff000)
        self.brew(0, finish=False)
        identity = self.metadata().id
        self.lib.hf_values(math.nan, math.inf, 0)
        self.lib.hf_tick(45000)
        value = self.all_samples(identity)[-1]
        self.assertEqual(value.elapsed_ms, 45000)
        self.assertEqual((value.wort, value.thermoblock, value.target), (-32768, -32768, 0))
        self.assertTrue(value.flags & 16)
        self.assertEqual(self.metadata().gap_count, 1)

    def test_no_io_before_nwp_or_from_interrupt_context(self):
        self.lib.hf_storage(0)
        self.assertEqual(self.lib.hf_start(b"brew"), 0)
        self.lib.hf_tick(15000)
        self.assertEqual(self.lib.hf_writes(), 0)
        self.lib.hf_storage(1)
        self.brew(0, finish=False)
        writes = self.lib.hf_writes()
        self.lib.hf_foreground(0)
        self.lib.hf_tick(60000)
        self.assertEqual(self.lib.hf_writes(), writes)
        self.lib.hf_foreground(1)
        self.lib.hf_tick(1)
        self.assertEqual(self.lib.hf_bad_io(), 0)

    def test_meaningful_control_changes_are_recorded_without_temperature_noise(self):
        identity = self.brew(0, finish=False)
        self.lib.hf_values(72, 84, 152)
        self.lib.hf_tick(10)
        self.assertEqual(self.metadata().record_count, 1)
        self.lib.hf_values(73, 85, 155)
        self.lib.hf_tick(10)
        self.lib.hf_controller(5, 2, 1)
        self.lib.hf_tick(10)
        self.lib.hf_controller(5, 2, 0)
        self.lib.hf_tick(10)
        self.lib.hf_stop()
        self.lib.hf_tick(10)
        values = self.all_samples(identity)
        self.assertEqual(len(values), 5)
        self.assertEqual(values[1].target, 1550)
        self.assertEqual((values[2].controller_status, values[2].compartment), (5, 2))
        self.assertFalse(values[3].flags & 8)
        self.assertTrue(values[4].flags & 4)

    def test_failed_tail_or_manifest_write_recovers_last_complete_checkpoint(self):
        for fault in (1, 2):
            for corrupt in (False, True):
                with self.subTest(fault=fault, corrupt=corrupt):
                    self.lib.hf_reset()
                    self.brew(3, finish=False)
                    identity = self.metadata().id
                    old_count = self.metadata().persisted_record_count
                    (self.lib.hf_corrupt_write if corrupt else self.lib.hf_fail_write)(fault)
                    self.lib.hf_tick(15000)
                    self.assertTrue(self.metadata().storage_error)
                    self.lib.hf_reboot()
                    self.assertEqual(self.metadata().id, identity)
                    self.assertEqual(self.metadata().record_count, old_count)
                    self.assertEqual(self.metadata().state, 3)
                    self.assertEqual(len(self.all_samples(identity)), old_count)

    def test_bad_chunk_crc_is_rejected_without_partial_page(self):
        identity = self.brew(200)
        self.lib.hf_corrupt_file(0, 50)
        result, page = self.page(identity)
        self.assertEqual(result, -1)
        self.assertEqual(page, [])
        self.assertTrue(self.metadata().storage_error)

    def test_reclaim_catalog_commits_before_old_file_is_reused(self):
        for fault in (1, 2):
            with self.subTest(fault=fault):
                self.lib.hf_reset()
                old = [self.brew(960) for _ in range(4)]
                self.assertEqual(self.lib.pico_history_count(), 4)
                first_write = self.lib.hf_writes()
                current = self.brew(174, finish=False)
                # Starting the fifth brew first commits a catalog eviction;
                # no data file is reused until its new active chunk is full.
                self.assertIn(self.lib.hf_trace(first_write), (24, 25))
                self.assertEqual(self.page(old[0])[0], 1)
                for i in range(first_write, self.lib.hf_writes()):
                    self.assertGreaterEqual(self.lib.hf_trace(i), 24)
                durable = self.metadata().persisted_record_count
                self.lib.hf_fail_write(fault)  # data erase/write, then manifest
                self.lib.hf_tick(15000)
                self.assertTrue(self.metadata().storage_error)
                self.lib.hf_reboot()
                self.assertEqual(self.page(old[0])[0], 1)
                self.assertEqual(self.metadata().id, current)
                self.assertEqual(self.metadata().record_count, durable)
                self.assertEqual(self.all_samples(current)[0].elapsed_ms, 0)
                for identity in old[1:]:
                    self.assertEqual(self.all_samples(identity)[-1].elapsed_ms, 960 * 15000)

    def test_failed_new_catalog_preserves_prior_histories(self):
        for fault in (1, 2, 3, 4):
            with self.subTest(fault=fault):
                self.lib.hf_reset()
                old = [self.brew(1) for _ in range(8)]
                self.lib.hf_fail_write(fault)
                self.assertEqual(self.lib.hf_start(b"brew"), 0)
                self.lib.hf_reboot()
                for identity in old[1:]:
                    self.assertEqual(self.all_samples(identity)[0].elapsed_ms, 0)
                # Failure before eviction commits retains all eight; after
                # that commit it may retain seven plus an interrupted start.
                if fault == 1:
                    self.assertEqual(self.page(old[0])[0], 0)
                else:
                    self.assertEqual(self.page(old[0])[0], 1)

    def test_one_lazy_buffer_reused_and_freed_on_rebind(self):
        self.assertEqual(self.lib.hf_heap(0), 0)
        self.brew(200)
        self.brew(200)
        self.assertEqual([self.lib.hf_heap(i) for i in (0, 1, 3, 4, 5)], [1, 1, 1, 3552, 0])
        self.lib.hf_reboot()  # only completed records: no active tail allocation
        self.assertEqual([self.lib.hf_heap(i) for i in (2, 3, 4, 5)], [1, 0, 0, 0])
        self.brew(0, finish=False)
        self.assertEqual([self.lib.hf_heap(i) for i in (0, 3, 4, 5)], [2, 1, 3552, 0])

    def test_allocation_failure_precedes_new_history_eviction_or_writes(self):
        identities = [self.brew(1) for _ in range(8)]
        self.lib.hf_reboot()
        writes = self.lib.hf_writes()
        self.lib.hf_allocation_failure(1)
        self.assertEqual(self.lib.hf_start(b"brew"), 0)
        self.assertEqual(self.lib.hf_writes(), writes)
        self.assertEqual(self.lib.pico_history_count(), 8)
        self.assertTrue(self.metadata().storage_error)
        self.assertEqual(self.page(identities[0])[0], 0)
        self.assertEqual(self.lib.hf_heap(3), 0)

    def test_recovery_allocation_failure_preserves_committed_tail(self):
        identity = self.brew(5, finish=False)
        committed = self.metadata().persisted_record_count
        writes = self.lib.hf_writes()
        self.lib.hf_allocation_failure(1)
        self.lib.hf_reboot()
        self.assertEqual(self.lib.hf_writes(), writes)
        self.assertTrue(self.metadata().storage_error)
        self.assertEqual(self.lib.hf_heap(3), 0)
        self.lib.hf_allocation_failure(0)
        self.lib.hf_reboot()
        self.assertEqual(self.metadata().id, identity)
        self.assertEqual(len(self.all_samples(identity)), committed)
        self.assertEqual(self.lib.hf_heap(3), 1)
        self.assertEqual(self.lib.hf_heap(5), 0)


class NativeHistoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        folder = Path(cls.temp.name)
        (folder / "utility").mkdir()
        (folder / "WiFi.h").write_text("class WiFiClass { public: static bool _initialized; };\n")
        (folder / "utility/simplelink.h").write_text(
            "#pragma once\n#include <stdint.h>\ntypedef uint8_t _u8; typedef uint32_t _u32; typedef int32_t _i32;\n")
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
        library = folder / "native-history.so"
        subprocess.run([os.environ.get("CXX", "c++"), "-shared", "-fPIC", "-std=c++11", "-O1",
                        "-Wall", "-Wextra", "-Werror", "-I" + str(folder), "-I" + str(ROOT / "include"),
                        str(ROOT / "platform/energia_history.cpp"), str(ROOT / "tests/energia_history_fixture.cpp"),
                        "-o", str(library)], check=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.hb_write.argtypes = cls.lib.hb_read.argtypes = [C.c_char_p, C.c_size_t]
        cls.lib.hb_mode.restype = C.c_uint

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.lib.hb_reset()

    def trace(self):
        return [self.lib.hb_trace(i) for i in range(self.lib.hb_calls())]

    def test_native_creation_and_existing_rewrite_have_independent_closed_handles(self):
        self.assertEqual(self.lib.hb_write(b"/local/history00", 3552), 0)
        self.assertEqual(self.trace(), [0, 1, 3, 4])
        self.assertEqual(self.lib.hb_mode(), 0x2000 | (3584 << 16))
        self.lib.hb_reset()
        self.assertEqual(self.lib.hb_write(b"/local/history.meta.a", 1024), 0)
        self.assertEqual(self.lib.hb_mode(), 0x2000 | (1024 << 16))
        self.lib.hb_reset()
        self.lib.hb_result(0, 0)
        self.assertEqual(self.lib.hb_write(b"/local/history23", 52), 0)
        self.assertEqual(self.lib.hb_mode(), 0x1000)
        self.assertEqual(self.lib.hb_bad_handle(), 0)

    def test_native_errors_never_fall_back_to_create_or_reopen(self):
        for field, value, expected_trace in (
            (0, -7, [0]), (1, -9, [0, 1]), (3, 7, [0, 1, 3, 4]), (4, -7, [0, 1, 3, 4])
        ):
            with self.subTest(field=field):
                self.lib.hb_reset()
                self.lib.hb_result(field, value)
                self.assertEqual(self.lib.hb_write(b"/local/history00", 52), -1)
                self.assertEqual(self.trace(), expected_trace)
        self.lib.hb_reset()
        self.lib.hb_result(0, 0)
        self.lib.hb_capacity(32)
        self.assertEqual(self.lib.hb_write(b"/local/history00", 52), -1)
        self.assertEqual(self.trace(), [0])

    def test_native_read_closes_and_reports_partial_or_close_failure(self):
        self.assertEqual(self.lib.hb_read(b"/local/history.tail.a", 52), 52)
        self.assertEqual(self.trace(), [1, 2, 4])
        self.lib.hb_reset()
        self.lib.hb_result(2, 7)
        self.assertEqual(self.lib.hb_read(b"/local/history.tail.a", 52), 7)
        self.assertEqual(self.trace(), [1, 2, 4])
        self.lib.hb_reset()
        self.lib.hb_result(4, -7)
        self.assertEqual(self.lib.hb_read(b"/local/history.tail.a", 52), -1)

    def test_native_scope_and_uninitialized_nwp_are_rejected_without_io(self):
        for path in (b"/sys/mcuimg2.bin", b"/local/api.key", b"/local/history24", b"/local/history00/../api.key"):
            self.assertEqual(self.lib.hb_write(path, 52), -1)
        self.lib.hb_ready(0)
        self.assertEqual(self.lib.hb_write(b"/local/history00", 52), -1)
        self.assertEqual(self.lib.hb_read(b"/local/history00", 52), -1)
        self.assertEqual(self.lib.hb_calls(), 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
