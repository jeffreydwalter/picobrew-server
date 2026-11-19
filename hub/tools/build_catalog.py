#!/usr/bin/env python3
"""Build the offline catalogue from pinned, already checked-out source repositories.

This program does not fetch the network or run source-repository code. It retains
exact original JSON (including excluded records), attributed images and support
text in a deterministic ZIP consumed through Go's embed package. PDFs are an
optional, separately generated reference bundle, never a firmware dependency.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import urllib.parse
import xml.etree.ElementTree as ET
import zipfile

from catalog_public import convert_public


SOURCES = {
    "public-recipes": {
        "folder": "picobrew-recipes",
        "name": "PicoBrew public recipes",
        "repository": "https://github.com/jeffreydwalter/picobrew-recipes",
        "upstream": "https://github.com/Justin-Credible/picobrew-recipes",
        "commit": "8e0289a5b9aaef8b87d94bbe2eb4b7b4f1430e6e",
        "license": "No repository license supplied",
        "attribution": "Original recipe authors, PicoBrew Inc., and the Justin-Credible public archive",
        "notes": "Mirror of public recipe records. Original authors and source JSON are retained. Colour glasses illustrate source SRM; they are not photographs of individual brews.",
    },
    "picopak-library": {
        "folder": "picopak-library",
        "name": "PicoPak and ZPak program archive",
        "repository": "https://github.com/jeffreydwalter/picopak-library",
        "upstream": "https://github.com/tmack8001/picopak-library",
        "commit": "93a9e104ba696a56f667050eae8e969e0b732e36",
        "license": "No repository license supplied",
        "attribution": "Original PicoBrew programs and the tmack8001 archive",
        "notes": "Native machine programs, not ingredient bills. Exact duplicate PicoPak variants are grouped; every original record remains available. Invalid and empty records are excluded from browsing and retained in the source archive.",
    },
    "chiefwigms": {
        "folder": "picobrew_pico",
        "name": "Chiefwigms community server",
        "repository": "https://github.com/jeffreydwalter/picobrew_pico",
        "upstream": "https://github.com/chiefwigms/picobrew_pico",
        "commit": "176b9fa2e7245a8504120b50e327b51d96d1cf2b",
        "license": "MIT repository license; original content may retain separate rights",
        "attribution": "Chiefwigms and picobrew_pico contributors; original PicoBrew base recipes",
        "notes": "Base programs, cleaning programs, style illustrations and device drawings. Recipe notes are preserved verbatim; contradictory quantities are not promoted into an invented ingredient bill.",
    },
    "support": {
        "folder": "picobrew-support",
        "name": "PicoBrew support archive",
        "repository": "https://github.com/jeffreydwalter/picobrew-support",
        "upstream": "https://github.com/Intecpsp/picobrew-support",
        "commit": "02aed3600ae5f3f8ea5c005f828c74542f382c88",
        "license": "No repository license supplied; original PicoBrew documentation",
        "attribution": "PicoBrew documentation, archived by Intecpsp on 2020-08-06",
        "notes": "Historical documentation, not current Hub installation instructions. Small reference text and images are bundled. The original PDF manuals are available as an optional verified local reference bundle.",
    },
    "legacy-server": {
        "folder": "legacy/zymatic-server",
        "name": "Original Zymatic server",
        "repository": "https://github.com/jeffreydwalter/picobrew-server",
        "upstream": "https://github.com/hotzenklotz/picobrew-server",
        "commit": "aa7531965c32d124643797937177431cf9dd1c8d",
        "license": "MIT code; separate Creative Commons notice for selected icons",
        "attribution": "Tom Herold; ingredient and glass icons by The Crew at Fusionary, Noun Project",
        "notes": "The original server includes no recipe corpus. Its ingredient/glass drawings are retained with their separate attribution notice; old application HTML and JavaScript are not used by the Hub.",
    },
}

STARTERS = (64, 17, 20, 6, 4, 43, 46, 7049)
ASSET_PREFIX = "/api/v1/catalog/assets/"
NATIVE_WARNING = "This is an archived machine program. It contains no ingredient bill or measured batch volume; use the matching PicoPak or a reviewed ingredient plan."
PUBLIC_MODELS = ["z", "zymatic"]
PICO_MODELS = ["pico-s", "pico-pro", "pico-c"]


def dump(value) -> bytes:
    return (json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"), allow_nan=False) + "\n").encode()


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def source_url(source_id: str, path: str, raw=False) -> str:
    s = SOURCES[source_id]
    prefix = s["repository"].replace("https://github.com/", "https://raw.githubusercontent.com/") if raw else s["repository"] + "/blob"
    return prefix + "/" + s["commit"] + "/" + urllib.parse.quote(path, safe="/")


def check_source(path: Path, expected: str):
    head = subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip()
    if head != expected:
        raise ValueError(f"{path}: expected commit {expected}, found {head}")
    if subprocess.check_output(["git", "-C", str(path), "status", "--porcelain", "--untracked-files=no"], text=True).strip():
        raise ValueError(f"{path}: tracked source files have changes")


def tracked(path: Path):
    raw = subprocess.check_output(["git", "-C", str(path), "ls-files", "-z"])
    return sorted(p.decode() for p in raw.split(b"\0") if p)


def static_svg(data: bytes):
    # Copy only static drawings. Do not rely on a browser CSP as a sanitizer.
    if b"<!ENTITY" in data.upper() or b"<!DOCTYPE" in data.upper():
        raise ValueError("SVG external entity/doctype is not permitted")
    root = ET.fromstring(data)
    if root.tag.rsplit("}", 1)[-1].lower() != "svg":
        raise ValueError("expected an SVG drawing")
    for node in root.iter():
        tag = node.tag.rsplit("}", 1)[-1].lower()
        if tag in {"script", "foreignobject", "iframe", "object", "embed", "a", "set"} or tag.startswith("animate"):
            raise ValueError(f"active SVG element {tag}")
        # The bundled drawings use basic class declarations. Reject CSS imports,
        # resource references and escapes rather than pretending to parse CSS.
        if tag == "style" and re.search(r"[@\\\\]|url\s*\(|expression\s*\(", "".join(node.itertext()), re.I):
            raise ValueError("active SVG stylesheet")
        for key, value in node.attrib.items():
            local = key.rsplit("}", 1)[-1].lower()
            if local.startswith("on") or "javascript:" in value.lower():
                raise ValueError("active SVG attribute")
            if local == "href" and not value.startswith("#"):
                raise ValueError("external SVG reference")
            if local == "style" and re.search(r"[@\\\\]|expression\s*\(", value, re.I):
                raise ValueError("active SVG style attribute")
            for target in re.findall(r"url\((.*?)\)", value, re.I):
                if not target.strip(" '\"").startswith("#"):
                    raise ValueError("external SVG CSS reference")


def native_steps(raw, names=("Name", "Location", "Temp", "Time", "Drain")):
    fields = ("name", "location", "target_f", "minutes", "drain_minutes")
    steps = []
    for step in raw:
        mapped = dict(zip(fields, (step[k] for k in names)))
        if not isinstance(mapped["name"], str) or not mapped["name"].strip():
            raise ValueError("missing step name")
        for key in fields[1:]:
            value = mapped[key]
            if isinstance(value, str) and re.fullmatch(r"[0-9]+", value):
                value = int(value)
            if isinstance(value, bool) or not isinstance(value, (float, int)) or not math.isfinite(value) or value != int(value):
                raise ValueError(f"noninteger native {key}")
            mapped[key] = int(value)
        steps.append(mapped)
    return steps


def pico_deployable(program):
    def label(value, limit):
        return isinstance(value, str) and 0 < len(value) <= limit and all(32 <= ord(c) <= 126 for c in value)

    if not re.fullmatch(r"[A-Za-z0-9_-]{1,20}", program["id"]) or not label(program["name"], 31) or not 0 < len(program["steps"]) <= 15:
        return False
    for step in program["steps"]:
        if not label(step["name"], 21) or not 1 <= step["location"] <= 6:
            return False
        if step["target_f"] != 0 and not 60 <= step["target_f"] <= 212:
            return False
        if not 0 <= step["minutes"] <= 599 or not 0 <= step["drain_minutes"] <= 120:
            return False
    # Go's JSON encoder escapes HTML-sensitive ASCII characters on the wire.
    wire = json.dumps(program, ensure_ascii=True, separators=(",", ":"))
    for char in "<>&":
        wire = wire.replace(char, "\\u" + format(ord(char), "04x"))
    return len(wire.encode()) <= 4095


def native_recipe(entry, program, notes=""):
    return {
        "id": "", "name": entry["name"], "kind": entry["kind"],
        "source_model": entry["source_model"], "native_program": program,
        "style": entry["style"], "description": entry["description"],
        "batch_liters": 0, "efficiency": 0, "ingredients": [], "steps": [],
        "tags": ["community-catalog", entry["source_id"]], "notes": notes,
        "revision": 0, "provenance": entry["provenance"],
    }


class Builder:
    def __init__(self, sources: Path, legacy: Path):
        self.roots = {key: sources / value["folder"] for key, value in SOURCES.items() if key != "legacy-server"}
        self.roots["legacy-server"] = legacy
        self.files = {}
        self.optional = {}
        self.entries = []
        self.assets = []
        self.references = []
        self.excluded = []
        self.sources = []
        for key, spec in SOURCES.items():
            if key != "legacy-server":
                check_source(self.roots[key], spec["commit"])
            self.sources.append({k: v for k, v in spec.items() if k != "folder"} | {
                "id": key, "record_count": 0, "valid_count": 0, "entry_count": 0,
                "excluded_count": 0, "excluded_reasons": {}, "asset_count": 0, "reference_count": 0,
            })
        self.counts = {s["id"]: s for s in self.sources}

    def raw(self, source_id, path):
        file = self.roots[source_id] / path
        if file.is_symlink() or not file.is_file():
            raise ValueError(f"not a regular source file: {file}")
        data = file.read_bytes()
        self.files[f"originals/{source_id}/{path}"] = data
        return data

    def record(self, source_id, path, record_id):
        data = self.raw(source_id, path)
        return {"id": str(record_id), "path": path, "sha256": sha(data), "bytes": len(data)}

    def provenance(self, source_id, record, catalog_id, warnings):
        spec = SOURCES[source_id]
        return {
            "catalog_id": catalog_id, "source_id": source_id,
            "repository": spec["repository"], "commit": spec["commit"],
            "path": record["path"], "sha256": record["sha256"],
            "source_url": source_url(source_id, record["path"]),
            "license": spec["license"], "warnings": warnings,
        }

    def entry(self, source_id, record, catalog_id, name, kind, source_model, models, warnings):
        return {
            "id": catalog_id, "name": name, "kind": kind, "source_id": source_id,
            "source_model": source_model, "models": models, "deploy_models": [],
            "style": "", "author": "", "description": "", "notes": "",
            "source_record_count": 1, "can_copy": False, "can_deploy": False,
            "featured": False, "official": False, "warnings": warnings,
            "source_records": [record], "source_url": source_url(source_id, record["path"]),
            "provenance": self.provenance(source_id, record, catalog_id, warnings),
        }

    def add_asset(self, source_id, path, title, kind, models=None, license=None, attribution=None, bundled=True):
        data = (self.roots[source_id] / path).read_bytes()
        suffix = Path(path).suffix.lower()
        content_type = {".png": "image/png", ".jpg": "image/jpeg", ".svg": "image/svg+xml", ".pdf": "application/pdf", ".md": "text/plain; charset=utf-8", ".txt": "text/plain; charset=utf-8"}[suffix]
        if suffix == ".svg":
            static_svg(data)
        key = f"{source_id}/{path}"
        target = self.files if bundled else self.optional
        target[f"assets/{key}"] = data
        asset = {
            "path": key, "url": ASSET_PREFIX + urllib.parse.quote(key, safe="/"),
            "title": title, "kind": kind, "models": models or [], "source_id": source_id,
            "source_path": path, "sha256": sha(data), "bytes": len(data), "content_type": content_type,
            "license": license or SOURCES[source_id]["license"],
            "attribution": attribution or SOURCES[source_id]["attribution"], "bundled": bundled,
            "download_url": source_url(source_id, path, raw=True),
        }
        self.assets.append(asset)
        self.counts[source_id]["asset_count"] += 1
        return asset

    def reference(self, asset, description):
        self.references.append({
            "id": "ref-" + sha(asset["path"].encode())[:20], "title": asset["title"],
            "description": description, "models": asset["models"], "source_id": asset["source_id"],
            "asset_path": asset["path"], "url": asset["url"], "content_type": asset["content_type"],
            "source_url": source_url(asset["source_id"], asset["source_path"]),
            "bundled": asset["bundled"], "available": asset["bundled"],
            "download_url": asset["download_url"], "sha256": asset["sha256"],
        })
        self.counts[asset["source_id"]]["reference_count"] += 1

    def public_recipes(self):
        source = "public-recipes"
        official = json.loads(self.raw(source, "data/recipe-list-official.json"))["PublicRecipesList"]
        listing = json.loads(self.raw(source, "data/recipe-list-community.json"))["PublicRecipesList"]
        lookup = {r["GUID"]: r for r in listing}
        official_ids = {r["GUID"] for r in official}
        srm_assets = {}
        for path in tracked(self.roots[source]):
            m = re.fullmatch(r"assets/images/srm/200px/srm_(\d+)\.png", path)
            if m:
                level = int(m[1])
                srm_assets[level] = self.add_asset(source, path, f"SRM {level} colour illustration", "srm-illustration")
        for path in tracked(self.roots[source]):
            if not path.startswith("data/recipes/") or not path.endswith(".json"):
                continue
            record = self.record(source, path, Path(path).stem)
            data = json.loads(self.files[f"originals/{source}/{path}"])
            raw = data["VM"]["Recipe"]
            converted, warnings = convert_public(data, lookup.get(raw["GUID"]))
            catalog_id = "public-" + raw["GUID"]
            entry = self.entry(source, record, catalog_id, raw["Name"], "recipe", "z/zymatic", PUBLIC_MODELS, warnings)
            entry.update({
                "style": converted["style"], "author": raw.get("Author", "") or "",
                "description": converted["description"], "can_copy": True,
                "official": raw["GUID"] in official_ids, "recipe": converted,
                "notes": raw.get("Notes", "") or "",
                "source_parameters": {k: raw.get(k) for k in ["RecipeID", "GUID", "CrafterVersion", "H2O", "BoilSize", "Yeast", "MashSteps", "BoilSteps", "WhirlpoolSteps", "MachineSteps", "FermentationSteps", "Errors", "Warnings"]},
            })
            for key in ("ABV", "IBU", "SRM"):
                value = raw.get(key)
                if isinstance(value, (float, int)) and math.isfinite(value) and value >= 0:
                    entry[key.lower()] = value
            if "srm" in entry:
                level = min(srm_assets, key=lambda level: (abs(level - entry["srm"]), level))
                entry.update(image_url=srm_assets[level]["url"], image_kind="srm-illustration")
            converted["provenance"] = entry["provenance"]
            converted["kind"] = "recipe"
            converted["source_model"] = "z/zymatic"
            self.entries.append(entry)
            self.counts[source]["record_count"] += 1
            self.counts[source]["valid_count"] += 1

    def native_archive(self):
        source = "picopak-library"
        groups = {}
        for path in tracked(self.roots[source]):
            if not path.startswith("recipes/picopaks/") or not path.endswith(".json"):
                continue
            record = self.record(source, path, Path(path).stem)
            self.counts[source]["record_count"] += 1
            try:
                raw = json.loads(self.files[f"originals/{source}/{path}"])
            except (json.JSONDecodeError, UnicodeDecodeError):
                self.exclude(source, record, "invalid_json")
                continue
            if not isinstance(raw, dict) or not raw.get("Name") or not raw.get("Steps"):
                self.exclude(source, record, "empty_program")
                continue
            try:
                native_steps(raw["Steps"])
                if any(s.get("Action") != 0 for s in raw["Steps"]):
                    raise ValueError("unsupported Action")
            except (KeyError, ValueError, TypeError):
                self.exclude(source, record, "unsupported_program")
                continue
            fingerprint = sha(dump({k: v for k, v in raw.items() if k != "PicoRecipeID"}))
            groups.setdefault(fingerprint, []).append((record, raw))
            self.counts[source]["valid_count"] += 1
        for records in groups.values():
            records.sort(key=lambda pair: (int(pair[0]["id"]) not in STARTERS, int(pair[0]["id"])))
            record, raw = records[0]
            warnings = [NATIVE_WARNING, "Original Abv/Ibu and MinTime/MaxTime fields are preserved as source parameters; no unverified tweak ranges or beer statistics are applied."]
            program = {"id": "pico-pak-" + record["id"], "name": raw["Name"], "steps": native_steps(raw["Steps"])}
            deployable = pico_deployable(program)
            if any(s["minutes"] == 0 for s in program["steps"]):
                warnings.append("Zero-minute steps wait for the target temperature and then advance. They require firmware advertising zero_minute_steps support.")
            if not deployable:
                warnings.append("The original program exceeds a standalone device label or numeric limit. Copy it and explicitly edit the unsupported values before sending it to a brewer.")
            entry = self.entry(source, record, "picopak-" + record["id"], raw["Name"], "native-program", "pico", PICO_MODELS, warnings)
            entry.update({
                "description": "Original PicoPak brewing program with its complete mash and hop schedule.",
                "source_records": [record for record, _ in records], "source_record_count": len(records),
                "native_program": program, "can_copy": True, "can_deploy": deployable,
                "deploy_models": ["pico-s", "pico-pro"] if deployable else [],
                "featured": int(record["id"]) in STARTERS,
                "source_parameters": {k: v for k, v in raw.items() if k != "Steps"} | {"step_limits": [{k: s.get(k) for k in ["Name", "Action", "MinTime", "MaxTime"]} for s in raw["Steps"]]},
            })
            entry["recipe"] = native_recipe(entry, program)
            self.entries.append(entry)
        for path in tracked(self.roots[source]):
            if not path.startswith("recipes/zpaks/") or not path.endswith(".json"):
                continue
            record = self.record(source, path, Path(path).stem)
            raw = json.loads(self.files[f"originals/{source}/{path}"])
            program = {"id": "z-pak-" + record["id"], "name": raw["Name"], "steps": native_steps(raw["Steps"])}
            warnings = ["This is a native Z program. Its compartment routing and instruction steps are not interchangeable with Pico S/Pro programs; the Hub has no Z control adapter.", "The original record contains no ingredient bill. StartWater is preserved as source data, not invented batch volume."]
            entry = self.entry(source, record, "zpak-" + record["id"], raw["Name"], "native-program", "z", ["z"], warnings)
            entry.update(native_program=program, can_copy=True, description="Original ZPak machine program.", source_parameters={k: v for k, v in raw.items() if k != "Steps"})
            entry["recipe"] = native_recipe(entry, program)
            self.entries.append(entry)
            self.counts[source]["record_count"] += 1
            self.counts[source]["valid_count"] += 1

    def exclude(self, source, record, reason):
        self.excluded.append({"source_id": source, "reason": reason, **record})
        self.counts[source]["excluded_count"] += 1
        reasons = self.counts[source]["excluded_reasons"]
        reasons[reason] = reasons.get(reason, 0) + 1

    def chief(self):
        source = "chiefwigms"
        style_files = {
            "American Amber Ale": "amber-ale", "American Brown Ale": "brown-ale",
            "American IPA": "ipa", "Imperial IPA": "ipa", "American Lager": "lager",
            "American Pale Ale": "pale-ale", "American Porter": "porter",
            "California Common": "california-common", "Hazy": "hazy-ipa",
            "Imperial Stout": "imperial-stout", "Irish Red": "irish-red",
            "Saison": "saison", "Weissbier": "weissbier",
        }
        style_assets = {}
        for path in tracked(self.roots[source]):
            if path.startswith("app/static/img/beer-styles/") and path.endswith(".png"):
                style_assets[Path(path).stem] = self.add_asset(source, path, Path(path).stem.replace("-", " ").title() + " style illustration", "style-illustration")
        for filename, models, label in [
            ("picobrew.svg", PICO_MODELS, "Pico brewer"), ("zseries.svg", ["z"], "Z brewer"),
            ("zymatic.svg", ["zymatic"], "Zymatic brewer"), ("picoferm.svg", ["picoferm"], "PicoFerm"),
            ("picostill.svg", ["picostill"], "PicoStill"), ("tilt.svg", ["tilt"], "Tilt hydrometer"),
            ("iSpindel.svg", ["ispindel"], "iSpindel hydrometer"),
        ]:
            self.add_asset(source, "app/static/img/" + filename, label, "device-illustration", models)
        for path in tracked(self.roots[source]):
            base = path.startswith("app/static/support/base_recipes/") and path.endswith(".json")
            cleaning = path.startswith("app/recipes/zymatic/") and path.endswith(".json")
            if not base and not cleaning:
                continue
            raw = json.loads(self.raw(source, path))
            record = self.record(source, path, Path(path).stem)
            family = "zymatic" if cleaning else Path(path).parent.name
            family = "z" if family == "z-series" else family
            models = PICO_MODELS if family == "pico" else [family]
            kind = "maintenance" if cleaning else "native-program"
            warnings = ["Original recipe notes are retained, including any conflicting or missing quantities. They are not converted into an inferred ingredient bill."]
            catalog_id = "chief-" + family + "-" + sha(path.encode())[:16]
            entry = self.entry(source, record, catalog_id, raw["name"], kind, family, models, warnings)
            entry["notes"] = raw.get("notes", "") or ""
            style = entry["name"].removeprefix("Base Recipe - ") if base else ""
            entry["style"] = style
            if style in style_files and style_files[style] in style_assets:
                entry.update(image_url=style_assets[style_files[style]]["url"], image_kind="style-illustration")
            for key in ["abv", "ibu"]:
                if isinstance(raw.get(key), (int, float)) and raw[key] >= 0:
                    entry[key] = raw[key]
            if family != "pico":
                locations = {"PassThru": 0, "PassThrough": 0, "Mash": 1, "Adjunct1": 2, "Adjunct2": 3, "Adjunct3": 4, "Adjunct4": 5, "Pause": 6}
                converted = []
                for s in raw["steps"]:
                    converted.append({"Name": s["name"], "Location": locations[s["location"]], "Temp": s["temperature"], "Time": s["step_time"], "Drain": s["drain_time"]})
                program = {"id": "chief-" + sha(path.encode())[:14], "name": raw["name"], "steps": native_steps(converted)}
                entry["native_program"] = program
                entry["can_copy"] = True
                warnings.append(f"This program retains {family} native locations. The Hub has no live control adapter for that model; do not send it to a Pico S/Pro.")
                entry["recipe"] = native_recipe(entry, program, entry["notes"])
            else:
                warnings.append("This source uses named Pico locations and a separate Prime step. Its exact mapping to standalone firmware is unverified, so it is retained for reference and cannot be automatically copied or deployed.")
            entry["source_parameters"] = {k: v for k, v in raw.items() if k not in {"image", "notes"}}
            entry["description"] = "Original cleaning/maintenance program." if cleaning else "Community base recipe with its original notes and machine schedule."
            self.entries.append(entry)
            self.counts[source]["record_count"] += 1
            self.counts[source]["valid_count"] += 1
        for path in tracked(self.roots[source]):
            if path.endswith("/Base-Recipes.md"):
                asset = self.add_asset(source, path, Path(path).parent.name + " base recipe reference", "reference-text")
                self.reference(asset, "Original source links and style-image associations for the community base recipes.")

    def support(self):
        source = "support"
        labels = {"pico-pro": "Pico Pro", "pico-s": "Pico S", "pico-c": "Pico C", "zymatic": "Zymatic", "z-series": "Z", "picoferm": "PicoFerm", "picostill": "PicoStill", "kegsmarts": "KegSmarts", "accessories": "Accessories", "common": "Legacy firmware tools"}
        for path in tracked(self.roots[source]):
            suffix = Path(path).suffix.lower()
            if suffix not in {".md", ".png", ".jpg", ".pdf"} or path == "README.md":
                continue
            family = Path(path).parent.name
            model = "z" if family == "z-series" else family
            models = [model] if model not in {"common", "accessories"} else []
            title = labels.get(family, family) + ": " + Path(path).stem.replace("_", " ").replace("-", " ")
            kind = "reference-manual" if suffix == ".pdf" else "reference-image" if suffix in {".png", ".jpg"} else "reference-text"
            asset = self.add_asset(source, path, title, kind, models, bundled=suffix != ".pdf")
            self.reference(asset, "Original archived support reference. Historical cloud and flashing instructions may not apply to standalone Hub firmware.")

    def notices_and_legacy(self):
        for source in SOURCES:
            if source == "legacy-server":
                continue
            for path in tracked(self.roots[source]):
                if path == "README.md" or re.fullmatch(r"(LICENSE|NOTICE|COPYING)(\..*)?", path, re.I):
                    self.raw(source, path)
        source = "legacy-server"
        notice = "picobrew_server/static/img/LICENSE.txt"
        self.raw(source, notice)
        if (self.roots[source] / "LICENSE").is_file():
            self.raw(source, "LICENSE")
        for name in ["glass", "grain", "hop"]:
            path = f"picobrew_server/static/img/{name}.svg"
            # Validate that the copied legacy file is the original pinned blob.
            original = subprocess.check_output(["git", "-C", str(self.roots[source]), "show", SOURCES[source]["commit"] + ":" + path])
            if original != (self.roots[source] / path).read_bytes():
                raise ValueError(f"legacy asset differs from pinned commit: {path}")
            self.add_asset(source, path, name.capitalize(), "ingredient-illustration", license="Creative Commons, version unspecified in original notice", attribution="The Crew at Fusionary, Noun Project; see original static/img/LICENSE.txt")

    def finish(self):
        for source in self.counts:
            self.counts[source]["entry_count"] = sum(e["source_id"] == source for e in self.entries)
        details = {"recipe", "native_program", "warnings", "provenance", "source_records", "source_url", "notes", "source_parameters"}
        index = []
        for entry in sorted(self.entries, key=lambda e: (not e["featured"], e["name"].casefold(), e["id"])):
            if entry.get("recipe"):
                entry["recipe"]["description"] = entry["description"]
            self.files[f"entries/{entry['id']}.json"] = dump(entry)
            summary = {k: v for k, v in entry.items() if k not in details}
            ingredients = [i["name"] for i in entry.get("recipe", {}).get("ingredients", [])]
            summary["search"] = " ".join([entry["name"], entry["style"], entry["author"], entry["description"], *ingredients]).casefold()
            index.append(summary)
        self.files["index.json"] = dump(index)
        self.files["manifest.json"] = dump({"version": 1, "sources": self.sources, "assets": sorted(self.assets, key=lambda a: a["path"]), "references": sorted(self.references, key=lambda r: (r["title"], r["id"])), "entry_count": len(index)})
        self.files["excluded.json"] = dump(sorted(self.excluded, key=lambda e: (e["source_id"], e["path"])))
        return {"entries": len(index), "sources": self.sources, "core_files": len(self.files), "core_uncompressed_bytes": sum(map(len, self.files.values())), "optional_files": len(self.optional), "optional_uncompressed_bytes": sum(map(len, self.optional.values()))}


def write_zip(path, files, prefix=""):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".partial")
    with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(prefix + name, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    temporary.replace(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sources", required=True, type=Path, help="Directory containing the four pinned source checkouts")
    parser.add_argument("--legacy", required=True, type=Path, help="Canonical legacy/zymatic-server subtree")
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "internal/catalog/data/catalog.zip")
    parser.add_argument("--references-output", type=Path, help="Optional original PDF bundle; extract into the Hub data directory")
    args = parser.parse_args()
    builder = Builder(args.sources, args.legacy)
    builder.public_recipes()
    builder.native_archive()
    builder.chief()
    builder.support()
    builder.notices_and_legacy()
    report = builder.finish()
    write_zip(args.output, builder.files)
    if args.references_output:
        # Files are rooted in one explicit directory, not arbitrary source paths.
        write_zip(args.references_output, {name.removeprefix("assets/"): data for name, data in builder.optional.items()}, "catalog-references/")
    report.update(output=str(args.output), bytes=args.output.stat().st_size, sha256=sha(args.output.read_bytes()))
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
