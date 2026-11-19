#!/usr/bin/env python3
"""Inspect a saved PicoBrew CC3200 flash backup without opening USB or serial.

Requires the already-installed cc3200tool package. Every recognized FAT is
parsed with its own filename metadata; no FAT or firmware slot is declared
active. Only firmware/boot filenames listed in TARGETS are extracted. The
review ZIP contains those files and the reports, never the raw flash image or
Wi-Fi configuration files. All outputs are private and created exclusively.

Sources:
https://github.com/toniebox-reverse-engineering/cc3200tool/blob/c2332898a1ededc0bdd01ec279b23366d213f81e/cc3200tool/cc.py
https://github.com/jeffreydwalter/picobrew-pico-pro-firmware/blob/feature/standalone-rest-api/docs/flash-layout.md
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import logging
import os
from pathlib import Path
import struct
import sys
import zipfile


BLOCK_SIZE = 4096
TARGETS = {
    "/sys/mcuimg.bin", "/sys/mcuimg1.bin", "/sys/mcuimg2.bin",
    "/sys/mcuimg3.bin", "/sys/mcubootinfo.bin",
}
REFERENCE_SIZE = 162792
REFERENCE_SHA256 = "e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702"


class InspectionError(Exception):
    pass


def canonical_name(name):
    return "/" + name.lstrip("/")


def validate_filename_metadata(fat_bytes, metadata):
    """Reject invalid declared extents instead of accepting truncated slices."""
    for index in range(128):
        allocation = fat_bytes[(index + 1) * 4:(index + 2) * 4]
        if allocation in (b"\xff\xff\xff\xff", bytes((0xFF, index, 0xFF, 0x7F))):
            continue
        descriptor = metadata[index * 4:index * 4 + 4]
        if len(descriptor) != 4:
            raise InspectionError(f"Missing filename descriptor for file ID {index}")
        name_offset, name_length = struct.unpack("<HH", descriptor)
        if name_length == 0:
            continue
        start = 0x200 + name_offset
        if start > len(metadata) or name_length > len(metadata) - start:
            raise InspectionError(f"Filename descriptor for file ID {index} lies outside filename metadata")
        name = metadata[start:start + name_length]
        if any(byte < 0x20 or byte >= 0x7F for byte in name):
            raise InspectionError(f"Filename for file ID {index} contains unsupported or control bytes")


def read_candidate(image, entry, fat_directory, mirror=False):
    name = canonical_name(entry.fname)
    block = entry.start_block + (entry.size_blocks if mirror else 0)
    offset = block * BLOCK_SIZE
    allocated = entry.size_blocks * BLOCK_SIZE
    result = {
        "kind": "mirror_candidate" if mirror else "fat_entry",
        "start_block": block, "offset_bytes": offset,
        "allocation_bytes": allocated, "status": "invalid",
    }
    if allocated < 8 or offset < 5 * BLOCK_SIZE or offset + allocated > len(image):
        result["error"] = "File allocation lies outside the permitted flash range"
        return result, None
    header = image[offset:offset + 8]
    result["header_signature_ls"] = header[6:8] == b"LS"
    if not result["header_signature_ls"]:
        result["error"] = "File header lacks the expected LS signature; candidate skipped"
        return result, None
    length = int.from_bytes(header[:3], "little")
    result["size_bytes"] = length
    if not 0 < length <= allocated - 8:
        result["error"] = "File length is empty or exceeds its allocated payload space"
        return result, None
    payload = image[offset + 8:offset + 8 + length]
    if len(payload) != length:
        result["error"] = "File payload is truncated"
        return result, None
    digest = hashlib.sha256(payload).hexdigest()
    result.update({
        "status": "extracted", "sha256": digest,
        "reference_match": length == REFERENCE_SIZE and digest == REFERENCE_SHA256,
    })
    prefix = fat_directory + ("/mirror_candidates" if mirror else "")
    result["relative_path"] = prefix + name
    result["review_include"] = False
    if name == "/sys/mcubootinfo.bin":
        if length == 8 and int.from_bytes(payload[4:8], "little") == 0xABCDDCBA:
            result["review_include"] = True
            result["boot_record"] = {
                "bytes_hex": payload.hex(" "),
                "first_byte": payload[0],
                "last_word_le_hex": f"0x{int.from_bytes(payload[4:8], 'little'):08x}",
                "interpretation": "Raw fields only; no boot-slot selection is inferred",
            }
        else:
            result["review_exclusion_reason"] = "Boot record lacks the expected eight-byte length and magic"
    elif len(payload) >= 8:
        stack, reset = struct.unpack("<II", payload[:8])
        result["plausible_cc3200_vectors"] = (
            0x20000000 < stack <= 0x20040000 and stack % 4 == 0
            and 0x20000000 <= (reset & ~1) < 0x20040000
            and bool(reset & 1)
        )
        if result["plausible_cc3200_vectors"]:
            result["review_include"] = True
            result["first_two_words_le"] = [f"0x{stack:08x}", f"0x{reset:08x}"]
        else:
            result["review_exclusion_reason"] = "Candidate does not have plausible CC3200 ARM vector words"
    else:
        result["review_exclusion_reason"] = "Firmware candidate is too short to contain vector words"
    if result["review_include"]:
        result["filesystem_header_hex"] = header.hex(" ")
    return result, payload


def inspect_image(image, image_name, cc):
    if not image or len(image) % BLOCK_SIZE or len(image) < 5 * BLOCK_SIZE:
        raise InspectionError("Backup must contain complete 4096-byte blocks and the filesystem area")
    storage = cc.CC3x00StorageInfo(BLOCK_SIZE, len(image) // BLOCK_SIZE)
    report = {
        "inspection_utc": datetime.now(timezone.utc).isoformat(),
        "source_image_name": image_name,
        "source_image_bytes": len(image),
        "source_image_sha256": hashlib.sha256(image).hexdigest(),
        "reference_application": {"size_bytes": REFERENCE_SIZE, "sha256": REFERENCE_SHA256},
        "selection_policy": "Both recognized FAT views are reported; active FAT and boot slot are not inferred",
        "privacy": "Archive contains only firmware candidates with plausible vectors, valid boot records, and metadata reports; raw flash and Wi-Fi files are excluded",
        "fat_views": [], "extracted_file_count": 0,
        "review_included_paths": [], "review_excluded": [],
    }
    outputs = {}
    for index in range(2):
        start = index * BLOCK_SIZE
        fat_bytes = image[start:start + BLOCK_SIZE]
        revision, signature = struct.unpack("<HH", fat_bytes[:4])
        view = {"fat_index": index, "revision": revision,
                "signature_hex": f"0x{signature:04x}", "recognized": False,
                "files": [], "missing_target_names": [], "errors": []}
        report["fat_views"].append(view)
        try:
            header = cc.CC3x00SffsHeader(index, fat_bytes, storage)
            if not header.is_valid:
                view["errors"].append("FAT is erased or its signature is unrecognized")
                continue
            view["recognized"] = True
            metadata_offset = 0x774 + index * BLOCK_SIZE
            view["filename_metadata_offset"] = metadata_offset
            # Deliberately use the metadata belonging to THIS FAT copy. The
            # upstream connection helper can accidentally use the other copy.
            metadata = image[metadata_offset:metadata_offset + BLOCK_SIZE]
            validate_filename_metadata(fat_bytes, metadata)
            fat = cc.CC3x00SffsInfo(header, storage, metadata, "cc3200")
            view["used_blocks"] = fat.used_blocks
            view["free_blocks"] = storage.block_count - fat.used_blocks
            names = [canonical_name(f.fname) for f in fat.files if canonical_name(f.fname) in TARGETS]
            duplicates = sorted(name for name in set(names) if names.count(name) > 1)
            if duplicates:
                raise InspectionError("Duplicate target filenames: " + ", ".join(duplicates))
            view["missing_target_names"] = sorted(TARGETS - set(names))
            directory = f"fat{index}-rev{revision}"
            for entry in fat.files:
                normalized = canonical_name(entry.fname)
                file_info = {
                    "file_id": entry.index, "filename": entry.fname,
                    "start_block": entry.start_block, "size_blocks": entry.size_blocks,
                    "total_blocks": entry.total_blocks, "mirrored": entry.mirrored,
                    "flags_hex": f"0x{entry.flags:x}", "target": normalized in TARGETS,
                }
                view["files"].append(file_info)
                if normalized not in TARGETS:
                    continue
                file_info["candidates"] = []
                for mirror in (False, True) if entry.mirrored else (False,):
                    candidate, payload = read_candidate(image, entry, directory, mirror)
                    file_info["candidates"].append(candidate)
                    if payload is not None:
                        relative = candidate["relative_path"]
                        if relative in outputs:
                            raise InspectionError("Duplicate output path: " + relative)
                        outputs[relative] = payload
                        if candidate["review_include"]:
                            report["review_included_paths"].append(relative)
                        else:
                            report["review_excluded"].append({
                                "relative_path": relative,
                                "reason": candidate["review_exclusion_reason"],
                            })
                    else:
                        view["errors"].append(f"{normalized}, {candidate['kind']}: {candidate['error']}")
        except Exception as exc:
            view["parse_error"] = f"{type(exc).__name__}: {exc}"
    report["extracted_file_count"] = len(outputs)
    return report, outputs


def text_report(report):
    lines = [
        "PicoBrew saved-flash inspection",
        f"Image: {report['source_image_name']}",
        f"Bytes: {report['source_image_bytes']}",
        f"SHA256: {report['source_image_sha256']}",
        report["selection_policy"], "",
    ]
    for view in report["fat_views"]:
        lines.append(f"FAT {view['fat_index']}: revision {view['revision']}; recognized={view['recognized']}")
        for entry in view["files"]:
            lines.append(
                f"  id={entry['file_id']:3} block={entry['start_block']:4} "
                f"blocks={entry['size_blocks']:3} mirrored={entry['mirrored']} "
                f"flags={entry['flags_hex']} {entry['filename']}"
            )
            for candidate in entry.get("candidates", []):
                lines.append(f"    {candidate['kind']}: {candidate['status']}")
                if candidate["status"] != "extracted":
                    lines.append("      " + candidate["error"])
                    continue
                lines.append(f"      file: {candidate['relative_path']}")
                lines.append(f"      bytes: {candidate['size_bytes']}")
                lines.append(f"      SHA256: {candidate['sha256']}")
                lines.append(f"      reference application match: {candidate['reference_match']}")
                lines.append(f"      included in review ZIP: {candidate['review_include']}")
                if not candidate["review_include"]:
                    lines.append("      exclusion reason: " + candidate["review_exclusion_reason"])
                if "first_two_words_le" in candidate:
                    lines.append("      first two words: " + ", ".join(candidate["first_two_words_le"]))
                    lines.append(f"      plausible CC3200 vectors: {candidate['plausible_cc3200_vectors']}")
                if "boot_record" in candidate:
                    lines.append("      boot record: " + candidate["boot_record"]["bytes_hex"])
                    lines.append("      final little-endian word: " + candidate["boot_record"]["last_word_le_hex"])
                if "warning" in candidate:
                    lines.append("      warning: " + candidate["warning"])
        if view.get("missing_target_names"):
            lines.append("  Missing target names: " + ", ".join(view["missing_target_names"]))
        if "parse_error" in view:
            lines.append("  PARSE ERROR: " + view["parse_error"])
        lines.append("")
    lines.append(f"Extracted firmware/boot candidates: {report['extracted_file_count']}")
    lines.append(f"Candidates included in review ZIP: {len(report['review_included_paths'])}")
    lines.append(report["privacy"])
    return "\n".join(lines) + "\n"


def private_file(path):
    fd = os.open(path, os.O_RDWR | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0), 0o600)
    return os.fdopen(fd, "w+b")


def save_outputs(directory, report, outputs):
    if not directory.is_absolute():
        raise InspectionError("--output-dir must be an absolute path")
    if os.path.lexists(directory):
        raise InspectionError(f"Output directory already exists; choose a new path: {directory}")
    if not directory.parent.is_dir():
        raise InspectionError(f"Parent directory does not exist: {directory.parent}")
    directory.mkdir(mode=0o700)
    rendered = text_report(report)
    files = dict(outputs)
    files["inspection.json"] = (json.dumps(report, indent=2, sort_keys=True) + "\n").encode()
    files["inspection.txt"] = rendered.encode()
    for relative, contents in files.items():
        path = directory / relative
        pending = []
        parent = path.parent
        while parent != directory and not parent.exists():
            pending.append(parent)
            parent = parent.parent
        for parent in reversed(pending):
            parent.mkdir(mode=0o700)
        with private_file(path) as file:
            file.write(contents)
    archive = directory / "firmware-review.zip"
    with private_file(archive) as file:
        with zipfile.ZipFile(file, mode="w", compression=zipfile.ZIP_DEFLATED) as zip_file:
            for relative in sorted(report["review_included_paths"] + ["inspection.json", "inspection.txt"]):
                zip_file.write(directory / relative, arcname=relative)
    return rendered, archive


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path, help="Verified raw serial-flash backup")
    parser.add_argument("--output-dir", required=True, type=Path, help="New absolute output directory")
    parser.add_argument("--expected-sha256", help="Require this complete-backup SHA256 before creating outputs")
    args = parser.parse_args(argv)
    try:
        image = args.image.read_bytes()
        digest = hashlib.sha256(image).hexdigest()
        if args.expected_sha256 is not None:
            expected = args.expected_sha256.lower()
            if len(expected) != 64 or any(c not in "0123456789abcdef" for c in expected):
                raise InspectionError("--expected-sha256 must contain 64 hexadecimal characters")
            if digest != expected:
                raise InspectionError(f"Backup checksum mismatch: got {digest}; expected {expected}")
        from cc3200tool import cc
        cc.log.setLevel(logging.WARNING)
        report, outputs = inspect_image(image, args.image.name, cc)
        rendered, archive = save_outputs(args.output_dir, report, outputs)
        print(rendered, end="")
        print("REPORT:", args.output_dir / "inspection.txt")
        print("REVIEW ZIP:", archive)
        if not report["review_included_paths"]:
            print("No firmware/boot candidates passed review-bundle checks; inspect the report before proceeding.")
            return 2
        return 0
    except Exception as exc:
        print(f"STOPPED: {type(exc).__name__}: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
