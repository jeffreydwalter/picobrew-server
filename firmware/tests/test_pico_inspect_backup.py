#!/usr/bin/env python3
"""Saved-flash parser bounds, mirrored candidates, and private-archive regressions.

Run from the repository root with the pinned USB-backup dependencies installed:
  .venv/bin/python tests/test_pico_inspect_backup.py
All device/flash input is synthetic; these tests never open real USB hardware.
"""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import stat
import struct
import tempfile
import sys
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests"))

from cc3200tool import cc
import pico_inspect_backup as inspect


BLOCK = 4096
# Synthetic disclosure sentinel; this is not a user credential.
SECRET = b"private-network-password-NEVER-SHARE"


def arm(tag=b"image", length=64):
    return (struct.pack("<II", 0x2003F000, 0x20004021) + tag).ljust(length, b"\xaa")


def write_file(image, block, payload, signature=b"LS", size_override=None):
    length = len(payload) if size_override is None else size_override
    offset = block * BLOCK
    image[offset:offset+8] = length.to_bytes(3, "little") + b"\x01\x00\x00" + signature
    image[offset+8:offset+8+len(payload)] = payload


def add_fat(image, index, revision, entries):
    base = index * BLOCK
    image[base:base+4] = struct.pack("<HH", revision, 0x534C)
    metadata = 0x774 + base
    name_offset = 0
    for file_id, name, block, blocks, mirrored in entries:
        encoded = name.encode("ascii")
        flags = 0x8 if mirrored else 0xC
        image[base+4*(file_id+1):base+4*(file_id+2)] = bytes([
            file_id, blocks, block & 255, (flags << 4) | (block >> 8),
        ])
        image[metadata+4*file_id:metadata+4*file_id+4] = struct.pack("<HH", name_offset, len(encoded))
        position = metadata + 0x200 + name_offset
        image[position:position+len(encoded)] = encoded
        name_offset += len(encoded)


def new_image():
    return bytearray(b"\xff" * (1024 * 1024))


class InspectorTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def parse(self, image):
        return inspect.inspect_image(bytes(image), "pico.bin", cc)

    def run_main(self, image, output="inspection", expected=None):
        path = self.root / "pico.bin"
        path.write_bytes(image)
        args = [str(path), "--output-dir", str(self.root/output)]
        if expected is not None:
            args.extend(["--expected-sha256", expected])
        stream = io.StringIO()
        with contextlib.redirect_stdout(stream), contextlib.redirect_stderr(stream):
            result = inspect.main(args)
        return result, stream.getvalue(), self.root/output

    def base_fixture(self):
        image = new_image()
        payloads = {
            "/sys/mcuimg.bin": arm(b"loader"),
            "/sys/mcuimg1.bin": arm(b"factory", 97),
            "/sys/mcuimg2.bin": arm(b"application", 177),
            "/sys/mcubootinfo.bin": b"\x02\x00\x00\x00\xba\xdc\xcd\xab",
        }
        entries = []
        for i, (name, payload) in enumerate(payloads.items()):
            entries.append((i, name, 5+i, 1, False))
            write_file(image, 5+i, payload)
        entries.append((8, "/wifipass.txt", 12, 1, False))
        write_file(image, 12, SECRET)
        add_fat(image, 0, 10, entries)
        return image, payloads

    def test_exact_lengths_and_private_archive(self):
        image, payloads = self.base_fixture()
        result, transcript, out = self.run_main(image, expected=hashlib.sha256(image).hexdigest())
        self.assertEqual(result, 0, transcript)
        self.assertEqual(stat.S_IMODE(out.stat().st_mode), 0o700)
        with zipfile.ZipFile(out/"firmware-review.zip") as archive:
            names = set(archive.namelist())
            expected = {"fat0-rev10" + name for name in payloads} | {"inspection.json", "inspection.txt"}
            self.assertEqual(names, expected)
            for name, payload in payloads.items():
                relative = "fat0-rev10" + name
                self.assertEqual(archive.read(relative), payload)
                self.assertEqual((out/relative).read_bytes(), payload)
                self.assertEqual(stat.S_IMODE((out/relative).stat().st_mode), 0o600)
            combined = b"".join(archive.read(name) for name in names)
            self.assertNotIn(SECRET, combined)
            self.assertNotIn(SECRET.hex().encode(), combined)
        self.assertNotIn(SECRET.decode(), transcript)

    def test_two_fat_views_use_their_own_names_and_contents(self):
        image = new_image()
        first, second = arm(b"first"), arm(b"second")
        write_file(image, 5, first)
        write_file(image, 6, second)
        add_fat(image, 0, 10, [(0, "/sys/mcuimg2.bin", 5, 1, False)])
        add_fat(image, 1, 9, [(0, "/sys/mcuimg1.bin", 6, 1, False)])
        report, outputs = self.parse(image)
        self.assertEqual(outputs["fat0-rev10/sys/mcuimg2.bin"], first)
        self.assertEqual(outputs["fat1-rev9/sys/mcuimg1.bin"], second)
        self.assertNotIn("fat0-rev10/sys/mcuimg1.bin", outputs)
        self.assertEqual([v["filename_metadata_offset"] for v in report["fat_views"]], [0x774, 0x1774])

    def test_revision_wrap_does_not_select_an_active_view(self):
        image = new_image()
        write_file(image, 5, arm())
        entries = [(0, "/sys/mcuimg2.bin", 5, 1, False)]
        add_fat(image, 0, 65534, entries)
        add_fat(image, 1, 0, entries)
        report, outputs = self.parse(image)
        self.assertEqual(len(outputs), 2)
        self.assertEqual([v["revision"] for v in report["fat_views"]], [65534, 0])
        self.assertNotIn("active_fat", report)
        self.assertIn("not inferred", report["selection_policy"])

    def test_wrong_hash_creates_no_output(self):
        image, _ = self.base_fixture()
        result, transcript, out = self.run_main(image, expected="0"*64)
        self.assertEqual(result, 1)
        self.assertIn("checksum mismatch", transcript)
        self.assertFalse(out.exists())

    def test_existing_output_is_never_overwritten(self):
        image, _ = self.base_fixture()
        out = self.root/"inspection"
        out.mkdir()
        sentinel = out/"inspection.json"
        sentinel.write_bytes(b"do not replace")
        result, transcript, _ = self.run_main(image)
        self.assertEqual(result, 1)
        self.assertIn("already exists", transcript)
        self.assertEqual(sentinel.read_bytes(), b"do not replace")

    def test_stale_invalid_header_does_not_hide_other_fat(self):
        image = new_image()
        add_fat(image, 0, 10, [(0, "/sys/mcuimg2.bin", 5, 1, False)])
        add_fat(image, 1, 11, [(0, "/sys/mcuimg2.bin", 6, 1, False)])
        write_file(image, 5, b"old", signature=b"??")
        valid = arm(b"new")
        write_file(image, 6, valid)
        report, outputs = self.parse(image)
        self.assertNotIn("fat0-rev10/sys/mcuimg2.bin", outputs)
        self.assertEqual(outputs["fat1-rev11/sys/mcuimg2.bin"], valid)
        self.assertTrue(report["fat_views"][0]["errors"])

    def test_duplicate_target_names_reject_that_view(self):
        image = new_image()
        write_file(image, 5, arm())
        write_file(image, 6, arm())
        add_fat(image, 0, 10, [(0, "/sys/mcuimg2.bin", 5, 1, False), (1, "sys/mcuimg2.bin", 6, 1, False)])
        report, outputs = self.parse(image)
        self.assertEqual(outputs, {})
        self.assertIn("Duplicate", report["fat_views"][0]["parse_error"])

    def test_malformed_name_length_cannot_truncate_to_target(self):
        image = new_image()
        write_file(image, 5, arm())
        name = b"/sys/mcuimg2.bin"
        add_fat(image, 0, 10, [(0, name.decode(), 5, 1, False)])
        meta = 0x774
        offset = BLOCK - 0x200 - len(name)
        image[meta:meta+4] = struct.pack("<HH", offset, len(name)+100)
        image[meta+BLOCK-len(name):meta+BLOCK] = name
        report, outputs = self.parse(image)
        self.assertEqual(outputs, {}, "Malformed descriptor must not be silently truncated into a target filename")
        self.assertTrue(report["fat_views"][0].get("parse_error"))

    def test_stale_target_pointing_to_secret_is_excluded_from_share_archive(self):
        image, _ = self.base_fixture()
        add_fat(image, 1, 9, [(0, "/sys/mcuimg2.bin", 12, 1, False)])
        result, transcript, out = self.run_main(image)
        self.assertEqual(result, 0, transcript)
        self.assertEqual((out/"fat1-rev9/sys/mcuimg2.bin").read_bytes(), SECRET)
        with zipfile.ZipFile(out/"firmware-review.zip") as archive:
            self.assertNotIn("fat1-rev9/sys/mcuimg2.bin", archive.namelist())
            all_data = b"".join(archive.read(name) for name in archive.namelist())
        self.assertNotIn(SECRET, all_data)
        self.assertNotIn(SECRET[:8].hex(" ").encode(), all_data)
        self.assertNotIn(SECRET[:8].hex().encode(), all_data)
        report = json.loads((out/"inspection.json").read_text())
        bad = report["fat_views"][1]["files"][0]["candidates"][0]
        self.assertFalse(bad["review_include"])
        self.assertNotIn("first_two_words_le", bad)
        self.assertNotIn("filesystem_header_hex", bad)

    def test_stale_target_inside_private_data_does_not_report_header_bytes(self):
        image = new_image()
        add_fat(image, 0, 10, [(0, "/sys/mcuimg2.bin", 5, 1, False)])
        secret_header = b"SECRET!!"
        image[5*BLOCK:5*BLOCK+8] = secret_header
        report, outputs = self.parse(image)
        candidate = report["fat_views"][0]["files"][0]["candidates"][0]
        self.assertEqual(outputs, {})
        self.assertNotIn("filesystem_header_hex", candidate)
        self.assertNotIn("size_bytes", candidate)
        rendered = json.dumps(report) + inspect.text_report(report)
        self.assertNotIn(secret_header.hex(" "), rendered)
        self.assertNotIn(secret_header.hex(), rendered)
        self.assertNotIn(secret_header.decode(), rendered)

    def test_unknown_boot_record_does_not_disclose_raw_words(self):
        image = new_image()
        add_fat(image, 0, 10, [(0, "/sys/mcubootinfo.bin", 5, 1, False)])
        write_file(image, 5, b"SECRETS!")
        report, outputs = self.parse(image)
        candidate = report["fat_views"][0]["files"][0]["candidates"][0]
        self.assertFalse(candidate["review_include"])
        self.assertNotIn("boot_record", candidate)
        self.assertNotIn(b"SECRETS!".hex(" "), json.dumps(report))

    def test_mirrored_firmware_is_labelled_as_candidate(self):
        image = new_image()
        add_fat(image, 0, 10, [(0, "/sys/mcuimg.bin", 5, 1, True)])
        write_file(image, 5, arm(b"first"))
        write_file(image, 6, arm(b"mirror"))
        report, outputs = self.parse(image)
        self.assertIn("fat0-rev10/mirror_candidates/sys/mcuimg.bin", outputs)
        candidates = report["fat_views"][0]["files"][0]["candidates"]
        self.assertEqual([c["kind"] for c in candidates], ["fat_entry", "mirror_candidate"])
        self.assertNotIn("active", json.dumps(candidates))


if __name__ == "__main__":
    unittest.main(verbosity=2)
