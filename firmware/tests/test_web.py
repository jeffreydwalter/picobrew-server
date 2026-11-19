#!/usr/bin/env python3
"""Exercise the browser against the real portable C API/store with simulated I/O.

The local HTTP harness supplies authentication and static assets; production
HTTP framing/auth and SimpleLink bindings have their own sanitizer suites.
No physical brewing, Wi-Fi or flash behavior is exercised here.
"""
import gzip
import importlib.util
import json
import os
from pathlib import Path
import shutil
import threading
import unittest
from http.server import BaseHTTPRequestHandler, HTTPServer
from playwright.sync_api import sync_playwright, expect

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("local_api_tests", ROOT / "tests/test_local_api.py")
backend = importlib.util.module_from_spec(spec)
spec.loader.exec_module(backend)
# Deliberately public test-only credential. Never written to a firmware artifact.
TEST_KEY = "a" * 64
LOCK = threading.Lock()


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def handle_request(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        encoding = None
        if self.path.startswith("/api/"):
            if self.headers.get("Authorization") != "Bearer " + TEST_KEY:
                code, data = 401, {"error": "unauthorized", "message": "Invalid device key"}
            else:
                with LOCK:
                    code, data = backend.request(self.command, self.path, raw=body)
            payload = json.dumps(data).encode() if data is not None else b""
            mime = "application/json"
        else:
            assets = {"/": ("index.html", "text/html"), "/app.js": ("app.js", "text/javascript"),
                      "/style.css": ("style.css", "text/css"),
                      "/api-explorer.js": ("api-explorer.js", "text/javascript"),
                      "/history.js": ("history.js", "text/javascript")}
            if self.path not in assets:
                code, payload, mime = 404, b"", "text/plain"
            else:
                name, mime = assets[self.path]
                payload = gzip.compress((ROOT / "web" / name).read_bytes(), mtime=0)
                encoding, code = "gzip", 200
        self.send_response(code)
        self.send_header("Content-Type", mime)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Security-Policy", "default-src 'self'; object-src 'none'; frame-ancestors 'none'; base-uri 'none'")
        if encoding:
            self.send_header("Content-Encoding", encoding)
        self.end_headers()
        self.wfile.write(payload)

    do_GET = do_POST = do_PUT = do_PATCH = do_DELETE = handle_request


class BrowserWorkflow(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = HTTPServer(("127.0.0.1", 0), Handler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.base = "http://127.0.0.1:" + str(cls.server.server_port)
        cls.playwright = sync_playwright().start()
        executable = os.environ.get("PICOBREW_CHROMIUM") or shutil.which("chromium")
        cls.browser = cls.playwright.chromium.launch(executable_path=executable, headless=True,
                                                     args=["--no-sandbox"])
        cls.output = ROOT / "build/web-preview"
        cls.output.mkdir(parents=True, exist_ok=True)

    @classmethod
    def tearDownClass(cls):
        cls.browser.close()
        cls.playwright.stop()
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join()

    def setUp(self):
        with LOCK:
            backend.LIB.test_reset(1)
            backend.request("PUT", "/api/v1/recipes/house-ale", backend.recipe())
        self.context = self.browser.new_context(viewport={"width": 1440, "height": 1000})
        self.page = self.context.new_page()
        self.errors = []
        self.page.on("pageerror", lambda error: self.errors.append(str(error)))
        self.page.goto(self.base)

    def tearDown(self):
        self.context.close()
        self.assertEqual(self.errors, [])

    def connect(self):
        self.page.locator("#apiKey").fill(TEST_KEY)
        self.page.locator("#submitConnect").click()
        expect(self.page.locator("#connectionText")).to_have_text("Connected")
        expect(self.page.locator("#connectDialog")).not_to_be_visible()

    def test_connection_live_frame_and_desktop_layout(self):
        expect(self.page.locator("#startRecipe")).to_be_disabled()
        self.connect()
        expect(self.page.locator("#wortTemperature")).to_have_text("72.4°F")
        expect(self.page.locator("#displayHint")).to_have_text("LIVE OLED · 128 × 64")
        pixels = self.page.locator("#display").evaluate("c => Array.from(c.getContext('2d').getImageData(0,63,2,1).data)")
        self.assertEqual(pixels, [199, 225, 180, 255, 18, 38, 32, 255])
        self.assertEqual(self.page.evaluate("sessionStorage.getItem('pico-api-key')"), TEST_KEY)
        self.assertFalse(self.page.evaluate("document.documentElement.scrollWidth > innerWidth"))
        self.page.screenshot(path=str(self.output / "overview-desktop.png"), full_page=True)

    def test_oled_panel_bytes_render_in_upright_screen_coordinates(self):
        # Independent panel-byte landmarks: corners, page boundary and
        # nonzero bit offsets distinguish a half-turn from either reflection.
        raw = bytearray(1024)
        raw[1023], raw[1021], raw[772], raw[110] = 0x80, 0x10, 0x40, 0x20
        frame = {"width": 128, "height": 64, "format": "page-lsb-hex", "data": raw.hex()}
        self.page.route("**/api/v1/display", lambda route: route.fulfill(json=frame))
        self.connect()
        expect(self.page.locator("#displayHint")).to_have_text("LIVE OLED · 128 × 64")
        lit = self.page.locator("#display").evaluate("""canvas => {
          const data = canvas.getContext('2d').getImageData(0,0,128,64).data;
          const result = [];
          for (let y=0;y<64;y++) for (let x=0;x<128;x++) {
            const at=(y*128+x)*4;
            if (data[at]===199 && data[at+1]===225 && data[at+2]===180)
              result.push([x,y]);
          }
          return result;
        }""")
        self.assertEqual(lit, [[0, 0], [2, 3], [123, 9], [17, 58]])

    def test_recipe_save_load_export_and_empty_delete_response(self):
        self.connect()
        self.page.locator('[data-tab="recipes"]').click()
        self.page.locator("#newRecipe").click()
        self.page.locator("#recipeName").fill("Browser test ale")
        self.page.locator("#saveRecipe").click()
        expect(self.page.locator("#notice")).to_have_text("Recipe saved on the Pico.")
        expect(self.page.locator("#recipeCapacity")).to_have_text("2 / 16")
        self.page.locator(".recipe-item").filter(has_text="Browser test ale").click()
        expect(self.page.locator("#recipeName")).to_have_value("Browser test ale")
        with self.page.expect_download() as download:
            self.page.locator("#exportRecipe").click()
        document = json.loads(Path(download.value.path()).read_text())
        self.assertEqual(document["id"], "browser-test-ale")
        self.assertEqual(document["steps"][0]["target_f"], 152)
        self.page.screenshot(path=str(self.output / "recipes-desktop.png"), full_page=True)
        self.page.once("dialog", lambda dialog: dialog.accept())
        self.page.locator("#deleteRecipe").click()
        expect(self.page.locator("#notice")).to_have_text("Recipe deleted.")
        expect(self.page.locator("#recipeCapacity")).to_have_text("1 / 16")

    def test_program_encoder_stop_and_idle_only_clear(self):
        self.connect()
        for name, arrow, delta in [("Move up", "↑", -1), ("Move down", "↓", 1)]:
            button = self.page.get_by_role("button", name=name, exact=True)
            expect(button).to_have_text(arrow)
            with self.page.expect_response(lambda response: response.url.endswith("/api/v1/commands")):
                button.click()
            expect(self.page.locator("#notice")).to_have_text("Command accepted.")
            with LOCK:
                self.assertEqual(backend.LIB.test_last_delta(), delta)
        self.page.locator("#startRecipe").click()
        expect(self.page.locator("#startRecipe")).to_be_disabled()
        self.page.locator("#stopButton").click()
        expect(self.page.locator("#modeBadge")).to_have_text("STOPPED")
        expect(self.page.locator("#clearStop")).to_be_disabled()
        with LOCK:
            backend.LIB.test_busy(0)  # Simulated workflow has unwound.
        expect(self.page.locator("#clearStop")).to_be_enabled(timeout=8000)
        self.page.locator("#clearStop").click()
        expect(self.page.locator("#modeBadge")).to_have_text("READY")
        self.page.locator("#program").select_option("rinse")
        self.page.locator("#runProgram").click()
        expect(self.page.locator("#modeBadge")).to_have_text("IN PROGRESS")

    def test_restore_starters_edit_zero_minute_step_and_preserve_changes(self):
        self.connect()
        self.page.locator('[data-tab="recipes"]').click()
        self.page.once("dialog", lambda dialog: dialog.accept())
        with self.page.expect_request("**/api/v1/recipes/restore-defaults") as request:
            self.page.locator("#restoreRecipes").click()
        self.assertEqual(request.value.method, "POST")
        self.assertEqual(request.value.headers["content-type"], "application/json")
        expect(self.page.locator("#recipeCapacity")).to_have_text("9 / 16")
        self.page.locator(".recipe-item").filter(has_text="Pico Pale").click()
        expect(self.page.locator('#steps input[data-field="minutes"]').first).to_have_value("0")
        self.page.locator("#recipeName").fill("My Pico Pale")
        self.page.locator("#saveRecipe").click()
        expect(self.page.locator("#notice")).to_have_text("Recipe saved on the Pico.")
        self.page.once("dialog", lambda dialog: dialog.accept())
        self.page.locator("#restoreRecipes").click()
        expect(self.page.locator("#notice")).to_have_text("All starter recipes are already on the Pico.")
        expect(self.page.locator("#recipeName")).to_have_value("My Pico Pale")
        for width in (1440, 390):
            self.page.set_viewport_size({"width": width, "height": 900})
            self.assertFalse(self.page.evaluate("document.documentElement.scrollWidth > innerWidth"))
        self.page.screenshot(path=str(self.output / "starter-recipes-mobile.png"), full_page=True)

    def test_settings_and_disconnect(self):
        self.connect()
        self.page.locator('[data-tab="settings"]').click()
        self.page.locator("#temperatureUnit").select_option("C")
        self.page.locator("#language").select_option("fr")
        self.page.locator("#detailedStatus").check()
        self.page.locator("#saveSettings").click()
        expect(self.page.locator("#notice")).to_have_text("Preferences saved on the Pico.")
        self.assertEqual(self.page.locator('[data-tab="activity"]').count(), 0)
        self.assertEqual(self.page.locator('#exportEvents').count(), 0)
        self.page.locator('[data-tab="overview"]').click()
        expect(self.page.locator("#wortTemperature")).to_have_text("22.4°C")
        self.page.locator('[data-tab="settings"]').click()
        self.page.locator("#disconnectButton").click()
        expect(self.page.locator("#connectionText")).to_have_text("Disconnected")
        self.assertIsNone(self.page.evaluate("sessionStorage.getItem('pico-api-key')"))

    def test_auth_failure_and_untrusted_recipe_label(self):
        self.page.locator("#apiKey").fill("b" * 64)
        self.page.locator("#submitConnect").click()
        expect(self.page.locator("#connectError")).to_have_text("Invalid device key")
        expect(self.page.locator("#startRecipe")).to_be_disabled()
        with LOCK:
            backend.request("PUT", "/api/v1/recipes/markup", backend.recipe("<img src=x onerror=alert(1)>"))
        self.connect()
        self.page.locator('[data-tab="recipes"]').click()
        expect(self.page.locator("#recipeList")).to_contain_text("<img src=x onerror=alert(1)>")
        self.assertEqual(self.page.locator("#recipeList img").count(), 0)

    def test_mobile_layout_and_import(self):
        self.page.set_viewport_size({"width": 390, "height": 844})
        self.connect()
        self.assertFalse(self.page.evaluate("document.documentElement.scrollWidth > innerWidth"))
        self.page.screenshot(path=str(self.output / "overview-mobile.png"), full_page=True)
        self.page.locator('[data-tab="recipes"]').click()
        document = dict(id="imported", **backend.recipe("Imported recipe"))
        self.page.locator("#importFile").set_input_files({"name": "recipe.json", "mimeType": "application/json",
                                                        "buffer": json.dumps(document).encode()})
        expect(self.page.locator("#recipeName")).to_have_value("Imported recipe")
        self.page.locator("#saveRecipe").click()
        expect(self.page.locator("#notice")).to_have_text("Recipe saved on the Pico.")
        self.assertFalse(self.page.evaluate("document.documentElement.scrollWidth > innerWidth"))
        self.page.screenshot(path=str(self.output / "recipes-mobile.png"), full_page=True)

        # Legal labels need not contain spaces: neither a maximum-length name
        # nor its ID should force the two-column mobile recipe list wider.
        long_name, long_id = "W" * 31, "w" * 20
        document = dict(id=long_id, **backend.recipe(long_name))
        self.page.locator("#importFile").set_input_files({"name": "long-recipe.json", "mimeType": "application/json",
                                                        "buffer": json.dumps(document).encode()})
        expect(self.page.locator("#recipeName")).to_have_value(long_name)
        self.page.locator("#saveRecipe").click()
        expect(self.page.locator("#notice")).to_have_text("Recipe saved on the Pico.")
        item = self.page.locator(".recipe-item").filter(has_text=long_name)
        expect(item.locator("strong")).to_have_text(long_name)
        expect(item.locator("small")).to_contain_text(long_id)
        self.assertTrue(item.evaluate("el => el.scrollWidth <= el.clientWidth"))
        self.assertFalse(self.page.evaluate("document.documentElement.scrollWidth > innerWidth"))
        self.page.screenshot(path=str(self.output / "recipes-mobile-long-label.png"), full_page=True)


if __name__ == "__main__":
    unittest.main(verbosity=2)
