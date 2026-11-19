"""Preserve device limits and reject active content while building the bundle."""

import json
import os
from pathlib import Path
import unittest
import zipfile

from build_catalog import pico_deployable, static_svg


class CatalogBuilderTests(unittest.TestCase):
    def test_firmware_starters_match_catalogue_identity_and_schedule(self):
        community = Path(__file__).resolve().parents[2]
        starters = community / "firmware/data/starter-recipes.json"
        if not starters.is_file():
            if os.environ.get("CATALOG_REQUIRE_FIRMWARE") == "1" or (community / "firmware").exists():
                self.fail("the community checkout is missing its firmware starter recipe data")
            self.skipTest("isolated Hub source archive: firmware tree is not included")
        source = json.loads(starters.read_text())
        bundle = community / "hub/internal/catalog/data/catalog.zip"
        with zipfile.ZipFile(bundle) as archive:
            index = json.loads(archive.read("index.json"))
            featured = {entry["id"] for entry in index if entry["featured"]}
            self.assertEqual(len(source["recipes"]), len(featured))
            for item in source["recipes"]:
                archive_id = Path(item["source_path"]).stem
                entry_id = "picopak-" + archive_id
                self.assertIn(entry_id, featured)
                detail = json.loads(archive.read("entries/" + entry_id + ".json"))
                self.assertEqual(source["source_commit"], detail["provenance"]["commit"])
                self.assertEqual(item["source_sha256"], detail["provenance"]["sha256"])
                self.assertEqual({key: item[key] for key in ("id", "name", "steps")}, detail["native_program"])
                self.assertEqual(detail["recipe"]["native_program"], detail["native_program"])

    def test_native_zero_minute_steps_and_exact_labels(self):
        program = {
            "id": "pico-pak-64", "name": "Pico Pale",
            "steps": [{"name": "Heating", "location": 1, "target_f": 110,
                       "minutes": 0, "drain_minutes": 0}],
        }
        self.assertTrue(pico_deployable(program))
        for name in ("Bender Braü", "x" * 32, "Name\n"):
            self.assertFalse(pico_deployable(dict(program, name=name)))
        self.assertEqual(program["steps"][0]["minutes"], 0)

    def test_static_drawings_cannot_fetch_or_execute(self):
        static_svg(b'<svg><style>.cls-1{fill:#fff}</style><path fill="url(#fill)"/></svg>')
        for fragment in (
            '<script>alert(1)</script>',
            '<style>@import "https://example.com/style";</style>',
            '<style>.x{fill:url(https://example.com/image)}</style>',
            '<style>.x{fill:u\\72l(https://example.com/image)}</style>',
            '<image href="https://example.com/image"/>',
            '<a href="#local"><path/></a>',
            '<animateTransform attributeName="transform"/>',
            '<path onclick="alert(1)"/>',
        ):
            with self.subTest(fragment=fragment), self.assertRaises(ValueError):
                static_svg(("<svg>" + fragment + "</svg>").encode())


if __name__ == "__main__":
    unittest.main()
