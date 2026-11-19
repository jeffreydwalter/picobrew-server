#!/usr/bin/env python3
"""Host integration tests for the actual standalone API and persistent store.

This exercises JSON with the same shared request/response arena as the native
HTTP server. Hardware and flash transport are simulated; no device is flashed.
"""
import ctypes
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
TEMP = tempfile.TemporaryDirectory(prefix="picobrew-local-api-")
LIBRARY = Path(TEMP.name) / "api.so"
SOURCES = [ROOT / "tests/standalone/local_api_host.c", ROOT / "src/local_api.c", ROOT / "src/local_store.c", ROOT / "src/local_history.c", ROOT / "src/local_telemetry.c"]
subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-fPIC", "-shared", "-O1", "-g",
                "-I" + str(ROOT / "include"), *map(str, SOURCES), "-o", str(LIBRARY)], check=True)
LIB = ctypes.CDLL(str(LIBRARY))
LIB.test_request.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t,
                             ctypes.c_void_p, ctypes.c_size_t]
LIB.test_session.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
LIB.test_corrupt.argtypes = [ctypes.c_char_p, ctypes.c_uint]
LIB.test_load_recipe.argtypes = [ctypes.c_char_p]
LIB.test_event.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
LIB.test_history_start.argtypes = [ctypes.c_char_p]
LIB.test_history_advance.argtypes = [ctypes.c_uint, ctypes.c_uint]


def request(method, path, body=None, raw=None):
    payload = raw if raw is not None else (json.dumps(body, separators=(",", ":")).encode() if body is not None else b"")
    output = ctypes.create_string_buffer(4096)
    code = LIB.test_request(method.encode(), path.encode(), payload, len(payload), output, len(output))
    parsed = json.loads(output.value) if output.value else None
    return code, parsed


def recipe(name="House ale", count=1):
    return {"name": name, "steps": [{"name": f"Mash {i}", "location": i % 6 + 1,
            "target_f": 152, "minutes": 60, "drain_minutes": 2} for i in range(count)]}


class LocalApi(unittest.TestCase):
    def setUp(self):
        LIB.test_reset(1)

    def test_discovery_health_status_and_screen(self):
        code, data = request("GET", "/api/v1")
        self.assertEqual(code, 200)
        self.assertEqual(data["api_version"], 1)
        self.assertTrue(data["zero_minute_steps"])
        self.assertTrue(data["restore_default_recipes"])
        self.assertEqual(data["starter_recipe_count"], 8)
        self.assertEqual(data["firmware_version"], "1.1.0")
        self.assertIn("encoder", data["commands"])
        self.assertFalse(data["dedicated_pause_resume"])
        health = request("GET", "/api/v1/health")[1]
        self.assertEqual(health["api_version"], 1)
        self.assertEqual(health["firmware_version"], data["firmware_version"])
        self.assertEqual(health["uptime_ms"], 123456)
        status = request("GET", "/api/v1/status")[1]
        self.assertEqual(status["wort_f"], 72.4)
        self.assertTrue(status["filter_inserted"])
        self.assertEqual(status["encoder"]["max"], 5)
        self.assertEqual(status["stop_reason"], "none")
        screen = request("GET", "/api/v1/display")[1]
        self.assertEqual(bytes.fromhex(screen["data"]), bytes(range(256)) * 4)
        self.assertEqual((screen["width"], screen["height"]), (128, 64))
        LIB.test_ready(0)
        self.assertIsNone(request("GET", "/api/v1/status")[1]["wort_f"])

    def test_recipe_crud_reboot_and_existing_ui_bridge(self):
        self.assertEqual(request("GET", "/api/v1/recipes"), (200, {"recipes": [], "capacity": 16}))
        original = recipe(count=15)
        code, saved = request("PUT", "/api/v1/recipes/house-ale", original)
        self.assertEqual(code, 201)
        self.assertEqual(saved, dict(id="house-ale", **original))
        self.assertEqual(LIB.test_recipe_list(), 1)
        self.assertEqual(LIB.test_load_recipe(b"house-ale"), 1)
        self.assertEqual(request("GET", "/api/v1/status")[1]["recipe_name"], "House ale")
        LIB.test_reset(0)
        self.assertEqual(request("GET", "/api/v1/recipes/house-ale"), (200, saved))
        modified = recipe('Quoted "ale" \\ test')
        self.assertEqual(request("PUT", "/api/v1/recipes/house-ale", modified)[0], 200)
        self.assertEqual(request("GET", "/api/v1/recipes")[1]["recipes"][0]["name"], modified["name"])
        self.assertEqual(request("DELETE", "/api/v1/recipes/house-ale"), (204, None))
        LIB.test_reset(0)
        self.assertEqual(request("GET", "/api/v1/recipes/house-ale")[0], 404)

    def test_recipe_capacity_and_slot_reuse(self):
        for index in range(16):
            self.assertEqual(request("PUT", f"/api/v1/recipes/r{index}", recipe())[0], 201)
        self.assertEqual(len(request("GET", "/api/v1/recipes")[1]["recipes"]), 16)
        self.assertEqual(request("PUT", "/api/v1/recipes/overflow", recipe())[0], 507)
        self.assertEqual(request("DELETE", "/api/v1/recipes/r7")[0], 204)
        self.assertEqual(request("PUT", "/api/v1/recipes/reused", recipe())[0], 201)
        LIB.test_reset(0)
        self.assertEqual(request("GET", "/api/v1/recipes/r7")[0], 404)
        self.assertEqual(request("GET", "/api/v1/recipes/reused")[0], 200)

    def test_invalid_recipe_and_json_never_write(self):
        invalid = [b"", b"{}", b"null", b"[]", b"{\"name\":\"x\",\"steps\":[]}",
            b'{"name":"x","name":"y","steps":[]}', b'{"name":"x","steps":[]}trailing',
            b'{"name":"x\\u0000","steps":[]}', b'{"name":"x\\ud800","steps":[]}',
            b'{"name":"x\\n","steps":[]}', b'{"name":"x","steps":[],}', b'{"x":true}',
            b'{"name":"x","steps":[],"__proto__":{}}']
        for payload in invalid:
            with self.subTest(raw=payload):
                self.assertEqual(request("PUT", "/api/v1/recipes/x", raw=payload)[0], 400)
        for path in ["../x", "a/b", "%2e%2e", "x?x", "x" * 21, ""]:
            self.assertEqual(request("PUT", "/api/v1/recipes/" + path, recipe())[0], 400)
        for field, value in [("location", 0), ("location", 7), ("target_f", 59), ("target_f", 213),
                              ("minutes", -1), ("minutes", 600), ("minutes", 1.5), ("minutes", True),
                              ("drain_minutes", -1), ("drain_minutes", 121), ("name", "x" * 22)]:
            document = recipe(); document["steps"][0][field] = value
            with self.subTest(field=field, value=value):
                self.assertEqual(request("PUT", "/api/v1/recipes/x", document)[0], 400)
        for document in [recipe(count=0), recipe(count=16), recipe("x" * 32),
                         dict(recipe(), id="another-id"), dict(recipe(), unknown=1)]:
            self.assertEqual(request("PUT", "/api/v1/recipes/x", document)[0], 400)
        self.assertEqual(LIB.test_file_count(), 0)

    def test_native_picopak_zero_minute_heating_survives_reload(self):
        document = recipe("Archived PicoPak", count=2)
        document["steps"][0] = {"name": "Heating", "location": 1,
                                "target_f": 110, "minutes": 0, "drain_minutes": 0}
        self.assertEqual(request("PUT", "/api/v1/recipes/pak-8215", document)[0], 201)
        LIB.test_reset(0)
        self.assertEqual(request("GET", "/api/v1/recipes/pak-8215")[1]["steps"], document["steps"])
        self.assertEqual(LIB.test_load_recipe(b"pak-8215"), 1)

    def test_starters_seed_once_and_are_real_shared_slots(self):
        self.assertEqual(LIB.test_initialize_recipes(), 0)
        expected = json.loads((ROOT / "data/starter-recipes.json").read_text())["recipes"]
        self.assertEqual(LIB.test_recipe_list(), 8)
        for entry in expected:
            stored = request("GET", "/api/v1/recipes/" + entry["id"])[1]
            self.assertEqual(stored, {key: entry[key] for key in ("id", "name", "steps")})
            self.assertEqual(LIB.test_load_recipe(entry["id"].encode()), 1)
        self.assertEqual(request("PUT", "/api/v1/recipes/pico-pak-64", recipe("My Pico Pale"))[0], 200)
        self.assertEqual(request("DELETE", "/api/v1/recipes/pico-pak-17")[0], 204)
        LIB.test_reset(0)
        self.assertEqual(LIB.test_initialize_recipes(), 0)
        self.assertEqual(LIB.test_write_count(), 0)
        self.assertEqual(request("GET", "/api/v1/recipes/pico-pak-64")[1]["name"], "My Pico Pale")
        self.assertEqual(request("GET", "/api/v1/recipes/pico-pak-17")[0], 404)
        for item in request("GET", "/api/v1/recipes")[1]["recipes"]:
            self.assertEqual(request("DELETE", "/api/v1/recipes/" + item["id"])[0], 204)
        LIB.test_reset(0)
        self.assertEqual(LIB.test_initialize_recipes(), 0)
        self.assertEqual(LIB.test_recipe_list(), 0)
        self.assertEqual(LIB.test_write_count(), 0)

    def test_upgrade_migrates_empty_catalogue_but_preserves_occupied_catalogue(self):
        self.assertEqual(request("PUT", "/api/v1/recipes/custom", recipe())[0], 201)
        self.assertEqual(request("DELETE", "/api/v1/recipes/custom")[0], 204)
        LIB.test_reset(0)
        self.assertEqual(LIB.test_initialize_recipes(), 0)
        self.assertEqual(LIB.test_recipe_list(), 8)
        LIB.test_reset(1)
        self.assertEqual(request("PUT", "/api/v1/recipes/custom", recipe())[0], 201)
        self.assertEqual(LIB.test_initialize_recipes(), 0)
        self.assertEqual(LIB.test_recipe_list(), 1)
        self.assertEqual(request("DELETE", "/api/v1/recipes/custom")[0], 204)
        LIB.test_reset(0)
        self.assertEqual(LIB.test_initialize_recipes(), 0)
        self.assertEqual(LIB.test_recipe_list(), 0)

    def test_restore_adds_missing_only_and_preflights_capacity(self):
        self.assertEqual(LIB.test_initialize_recipes(), 0)
        self.assertEqual(request("PUT", "/api/v1/recipes/pico-pak-64", recipe("Edited starter"))[0], 200)
        self.assertEqual(request("DELETE", "/api/v1/recipes/pico-pak-17")[0], 204)
        self.assertEqual(request("POST", "/api/v1/recipes/restore-defaults"),
                         (200, {"added": 1, "preserved": 7, "capacity": 16}))
        self.assertEqual(request("GET", "/api/v1/recipes/pico-pak-64")[1]["name"], "Edited starter")
        writes = LIB.test_write_count()
        self.assertEqual(request("POST", "/api/v1/recipes/restore-defaults")[1]["added"], 0)
        self.assertEqual(LIB.test_write_count(), writes)
        LIB.test_busy(1)
        self.assertEqual(request("POST", "/api/v1/recipes/restore-defaults")[0], 409)
        LIB.test_busy(0)
        self.assertEqual(request("POST", "/api/v1/recipes/restore-defaults", {})[0], 400)
        LIB.test_reset(1)
        for index in range(9): request("PUT", f"/api/v1/recipes/custom-{index}", recipe())
        before = request("GET", "/api/v1/recipes")
        writes = LIB.test_write_count()
        self.assertEqual(request("POST", "/api/v1/recipes/restore-defaults")[0], 507)
        self.assertEqual(request("GET", "/api/v1/recipes"), before)
        self.assertEqual(LIB.test_write_count(), writes)

    def test_restore_interrupted_records_never_publish_partial_starter_set(self):
        failures = [(write, cut) for write in range(8) for cut in (0, 1, 15, 16, 64, 488)]
        failures += [(8, cut) for cut in range(432)]
        for write, cut in failures:
            LIB.test_reset(1)
            self.assertEqual(request("PUT", "/api/v1/recipes/custom", recipe())[0], 201)
            LIB.test_write_failure(write, cut)
            self.assertEqual(request("POST", "/api/v1/recipes/restore-defaults")[0], 503)
            LIB.test_reset(0)
            saved = request("GET", "/api/v1/recipes")[1]["recipes"]
            self.assertEqual([item["id"] for item in saved], ["custom"], (write, cut))
            self.assertEqual(request("POST", "/api/v1/recipes/restore-defaults")[0], 200)
            self.assertEqual(LIB.test_recipe_list(), 9)

    def test_seed_read_errors_and_corruption_never_replace_catalogue(self):
        request("PUT", "/api/v1/recipes/custom", recipe())
        writes = LIB.test_write_count()
        LIB.test_stat_failure(0)
        self.assertEqual(LIB.test_initialize_recipes(), 4)
        self.assertEqual(LIB.test_write_count(), writes)
        LIB.test_stat_failure(-1)
        self.assertEqual(LIB.test_corrupt(b"/local/index.a", 20), 1)
        self.assertEqual(LIB.test_initialize_recipes(), 5)
        self.assertEqual(LIB.test_write_count(), writes)

    def test_pristine_seed_interruption_retries_after_verified_empty_baseline(self):
        failures = [(write, cut) for write in range(1, 9) for cut in (0, 16, 488)]
        failures += [(9, cut) for cut in range(432)]
        for write, cut in failures:
            LIB.test_reset(1)
            LIB.test_write_failure(write, cut)
            self.assertEqual(LIB.test_initialize_recipes(), 4)
            LIB.test_reset(0)
            self.assertEqual(LIB.test_recipe_list(), 0, (write, cut))
            self.assertEqual(LIB.test_initialize_recipes(), 0, (write, cut))
            self.assertEqual(LIB.test_recipe_list(), 8)
        # The first-ever catalogue commit retains the pre-existing fail-closed
        # boundary: a corrupt sole bank cannot be assumed to contain no user data.
        for cut in (0, 16, 431):
            LIB.test_reset(1)
            LIB.test_write_failure(0, cut)
            self.assertEqual(LIB.test_initialize_recipes(), 4)
            LIB.test_reset(0)
            self.assertEqual(LIB.test_initialize_recipes(), 5)
            self.assertEqual(LIB.test_write_count(), 0)

    def test_recovery_records_torn_writes_reboot_and_tombstone(self):
        def checkpoint():
            active, drain, phase = (ctypes.c_uint() for _ in range(3))
            code = LIB.test_read_recovery(ctypes.byref(active), ctypes.byref(drain), ctypes.byref(phase))
            return code, active.value, drain.value, phase.value
        for cut in range(52):
            LIB.test_reset(1)
            request("PUT", "/api/v1/recipes/house", recipe())
            LIB.test_load_recipe(b"house")
            self.assertEqual(LIB.test_save_recovery(13, 2, 1), 1)
            LIB.test_write_failure(0, cut)
            self.assertEqual(LIB.test_save_recovery(0, 1, 3), 0)
            LIB.test_reset(0)
            self.assertEqual(checkpoint(), (0, 13, 2, 1))
        self.assertEqual(LIB.test_clear_recovery(), 1)
        LIB.test_reset(0)
        self.assertEqual(checkpoint()[0], 1)
        self.assertEqual(LIB.test_clear_recovery(), 1)
        self.assertEqual(LIB.test_write_count(), 0)

    def test_every_truncated_write_preserves_committed_recipe(self):
        # Every truncation point in a recipe record and catalog commit, followed
        # by a simulated reboot. The previous committed image must remain usable.
        for write_index, size in [(0, 489), (1, 432)]:
            for truncated in range(size):
                LIB.test_reset(1)
                self.assertEqual(request("PUT", "/api/v1/recipes/test", recipe("Before"))[0], 201)
                LIB.test_write_failure(write_index, truncated)
                self.assertEqual(request("PUT", "/api/v1/recipes/test", recipe("After"))[0], 503)
                LIB.test_reset(0)
                self.assertEqual(request("GET", "/api/v1/recipes/test")[1]["name"], "Before",
                                 (write_index, truncated))

    def test_storage_read_failures_do_not_initialize_empty_catalog(self):
        self.assertEqual(request("PUT", "/api/v1/recipes/test", recipe())[0], 201)
        count = LIB.test_file_count()
        LIB.test_stat_failure(0)
        self.assertEqual(request("PUT", "/api/v1/recipes/another", recipe())[0], 503)
        self.assertEqual(LIB.test_file_count(), count)
        LIB.test_stat_failure(-1); LIB.test_read_failure(0)
        self.assertEqual(request("PUT", "/api/v1/recipes/another", recipe())[0], 503)
        self.assertEqual(LIB.test_file_count(), count)
        LIB.test_read_failure(-1)
        self.assertEqual(request("GET", "/api/v1/recipes/test")[0], 200)
        self.assertEqual(LIB.test_corrupt(b"/local/index.a", 20), 1)
        self.assertEqual(request("PUT", "/api/v1/recipes/another", recipe())[1]["error"], "storage_corrupt")
        self.assertEqual(LIB.test_file_count(), count)

    def test_partial_overwrite_with_retained_flash_tail_is_commit_uncertain(self):
        # Existing SLFS files may retain their tail after a short write. A
        # partial catalog prefix can then be a valid full new commit. A 503
        # must permit either the old or the fully verified new recipe, never
        # malformed/mixed records. This is why clients must GET before retry.
        for write_index, size in [(0, 489), (1, 432)]:
            for truncated in range(size):
                LIB.test_reset(1)
                self.assertEqual(request("PUT", "/api/v1/recipes/test", recipe("Initial"))[0], 201)
                self.assertEqual(request("PUT", "/api/v1/recipes/test", recipe("Before"))[0], 200)
                LIB.test_retain_tail(1)
                LIB.test_write_failure(write_index, truncated)
                self.assertEqual(request("PUT", "/api/v1/recipes/test", recipe("After"))[0], 503)
                LIB.test_reset(0)
                code, saved = request("GET", "/api/v1/recipes/test")
                self.assertEqual(code, 200, (write_index, truncated))
                allowed = [dict(id="test", **recipe("Before"))]
                if write_index == 1: allowed.append(dict(id="test", **recipe("After")))
                self.assertIn(saved, allowed, (write_index, truncated))

    def test_busy_mutations_and_validated_command_dispatch(self):
        self.assertEqual(request("POST", "/api/v1/commands", {"command": "brew", "recipe_id": "missing"})[0], 404)
        self.assertEqual(LIB.test_control_calls(), 0)
        self.assertEqual(request("PUT", "/api/v1/recipes/test", recipe())[0], 201)
        self.assertEqual(request("POST", "/api/v1/commands", {"command": "brew", "recipe_id": "test"})[0], 202)
        self.assertEqual(LIB.test_control_calls(), 1)
        for method, path, body in [("PUT", "/api/v1/recipes/test", recipe()),
                                    ("DELETE", "/api/v1/recipes/test", None),
                                    ("PATCH", "/api/v1/settings", {"language": "es"})]:
            self.assertEqual(request(method, path, body)[0], 409)
        self.assertEqual(request("POST", "/api/v1/commands", {"command": "stop"})[0], 202)
        self.assertTrue(request("GET", "/api/v1/status")[1]["stopped"])
        self.assertEqual(request("GET", "/api/v1/status")[1]["stop_reason"], "requested")
        self.assertEqual(request("POST", "/api/v1/commands", {"command": "clear-stop"})[0], 409)
        LIB.test_busy(0)
        self.assertEqual(request("POST", "/api/v1/commands", {"command": "rinse"})[0], 409)
        self.assertEqual(request("POST", "/api/v1/commands", {"command": "clear-stop"})[0], 202)
        self.assertEqual(request("POST", "/api/v1/commands", {"command": "encoder", "delta": -3, "click": True})[0], 202)
        self.assertEqual((LIB.test_encoder_calls(), LIB.test_last_delta(), LIB.test_last_click()), (1, -3, 1))
        for command in [{"command": "heater", "on": True}, {"command": "encoder", "delta": 101},
                        {"command": "encoder", "delta": 0}, {"command": "rinse", "delta": 1},
                        {"command": "brew"}, {"command": "pause"}, {"command": "encoder", "click": 1}]:
            self.assertEqual(request("POST", "/api/v1/commands", command)[0], 400)

    def test_settings_and_session_sequences_survive_reboot(self):
        expected = {"temperature_unit": "C", "language": "fr", "detailed_status": True}
        self.assertEqual(request("PATCH", "/api/v1/settings", expected), (200, expected))
        one = ctypes.create_string_buffer(32); two = ctypes.create_string_buffer(32)
        self.assertEqual(LIB.test_session(one, len(one)), 1)
        LIB.test_reset(0)
        self.assertEqual(LIB.test_load_settings(), 1)
        self.assertEqual(request("GET", "/api/v1/settings"), (200, expected))
        self.assertEqual(LIB.test_session(two, len(two)), 1)
        self.assertEqual(one.value, b"local-01234567-00000001")
        self.assertEqual(two.value, b"local-01234567-00000002")
        for settings in [{}, {"wifi_password": "secret"}, {"language": "de"},
                         {"temperature_unit": "c"}, {"detailed_status": 1}]:
            self.assertEqual(request("PATCH", "/api/v1/settings", settings)[0], 400)
        LIB.test_write_failure(0, 8)
        self.assertEqual(request("PATCH", "/api/v1/settings", {"language": "en"})[0], 503)
        LIB.test_reset(0); self.assertEqual(LIB.test_load_settings(), 1)
        self.assertEqual(request("GET", "/api/v1/settings"), (200, expected))

    def test_event_ring_is_bounded_and_honest_about_persistence(self):
        for i in range(20): LIB.test_event(b"sample", str(i).encode())
        data = request("GET", "/api/v1/events")[1]
        self.assertTrue(data["volatile"])
        self.assertEqual([event["detail"] for event in data["events"]], list(map(str, range(12, 20))))
        self.assertEqual(data["next_sequence"], 21)
        LIB.test_reset(0)
        self.assertEqual(request("GET", "/api/v1/events")[1]["events"], [])

    def test_history_paging_persistence_and_session_identity(self):
        empty = request("GET", "/api/v1/history")[1]
        self.assertFalse(empty["available"])
        self.assertEqual(empty["sessions"], [])
        self.assertEqual((empty["capacity_records"], empty["capacity_sessions"]), (4224, 8))
        self.assertEqual(LIB.test_history_start(b'First "brew"'), 1)
        for i in range(40): LIB.test_history_advance(15000, i // 20)
        LIB.test_history_finish(0)
        meta = request("GET", "/api/v1/history")[1]
        self.assertEqual(meta["state"], "completed")
        self.assertEqual(meta["recipe_name"], 'First "brew"')
        first = meta["id"]
        collected, offset = [], 0
        while offset is not None:
            code, page = request("GET", f"/api/v1/history/samples/{offset}?id={first}")
            self.assertEqual(code, 200)
            self.assertEqual(page["id"], first)
            self.assertEqual(page["offset"], offset)
            self.assertLessEqual(len(page["samples"]), 32)
            collected.extend(page["samples"])
            offset = page["next_offset"]
        self.assertEqual(len(collected), meta["record_count"])
        self.assertEqual(collected[0][meta["fields"].index("wort_f")], 72.4)
        self.assertEqual(LIB.test_history_start(b"Second brew"), 1)
        LIB.test_history_advance(15000, 3); LIB.test_history_finish(0)
        LIB.test_reset(0)
        restored = request("GET", "/api/v1/history")[1]
        self.assertEqual([x["recipe_name"] for x in restored["sessions"]], ["Second brew", 'First "brew"'])
        self.assertEqual(request("GET", f"/api/v1/history/samples/0?id={first}")[1]["samples"], collected[:32])
        self.assertEqual(request("GET", "/api/v1/history/samples/0?id=4294967295")[0], 409)
        self.assertEqual(request("GET", f"/api/v1/history/samples/4224?id={first}")[0], 400)
        for bad in ["-1?id=1", "0", "0?id=0", "0?id=01", "00?id=1", "0?id=1&id=2",
                    "0?id=4294967296", "4294967296?id=1", "0?id=1&token=secret"]:
            self.assertEqual(request("GET", "/api/v1/history/samples/" + bad)[0], 400, bad)
        self.assertEqual(request("POST", "/api/v1/history", {})[0], 405)
        self.assertEqual(request("DELETE", f"/api/v1/history/samples/0?id={first}")[0], 405)

    def test_full_history_inventory_fits_shared_http_arena(self):
        for i in range(8):
            self.assertEqual(LIB.test_history_start(('"' * 30 + str(i)).encode()), 1)
            LIB.test_history_advance(15000, 1); LIB.test_history_finish(i % 2)
        code, result = request("GET", "/api/v1/history")
        self.assertEqual(code, 200)
        self.assertEqual(len(result["sessions"]), 8)
        self.assertEqual(result["sessions"][0]["id"], result["id"])
        self.assertLess(len(json.dumps(result, separators=(",", ":")).encode()), 4096)

    def test_telemetry_is_opt_in_validated_and_persistent(self):
        code, result = request("GET", "/api/v1/telemetry")
        self.assertEqual(code, 200)
        self.assertEqual((result["enabled"], result["host"], result["port"]), (False, "", 8080))
        self.assertEqual(result["status"]["sent"], 0)
        configured = {"enabled": True, "host": "brews.example.net", "port": 8080}
        self.assertEqual(request("PUT", "/api/v1/telemetry", configured)[0], 200)
        LIB.test_reset(0)
        restored = request("GET", "/api/v1/telemetry")[1]
        for field, value in configured.items(): self.assertEqual(restored[field], value)
        for host in ["https://brews.example.net", "evil/route", "evil\\r\\nX: y", "x" * 128,
                     "256.0.0.1", "-bad.example", "bad..example", ""]:
            self.assertEqual(request("PUT", "/api/v1/telemetry", dict(configured, host=host))[0], 400)
        self.assertEqual(request("PUT", "/api/v1/telemetry", {"enabled": True})[0], 400)
        self.assertEqual(request("PUT", "/api/v1/telemetry", dict(configured, token="secret"))[0], 400)
        LIB.test_busy(1)
        self.assertEqual(request("PUT", "/api/v1/telemetry", dict(configured, enabled=False))[0], 409)
        LIB.test_busy(0)
        self.assertEqual(request("PUT", "/api/v1/telemetry", dict(configured, enabled=False))[0], 200)
        self.assertEqual(request("POST", "/api/v1/telemetry", configured)[0], 405)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(LocalApi)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful(): raise SystemExit(1)
    executable = Path(TEMP.name) / "api-sanitized"
    subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-DLOCAL_API_SELFTEST_MAIN",
                    "-I" + str(ROOT / "include"), *map(str, SOURCES), "-o", str(executable)], check=True)
    # This target does not allocate. LeakSanitizer cannot inspect processes in
    # ptrace-based execution environments; ASan and UBSan remain fully enabled.
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1")
    subprocess.run([str(executable)], check=True, env=environment)
