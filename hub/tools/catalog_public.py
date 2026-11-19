"""Convert the pinned public PicoBrew recipe archive into editable Hub plans.

This module reads data only. It never evaluates source JavaScript, imports source
Python, fetches a URL, or writes to a brewer. The catalogue retains the complete
original JSON separately; notes and warnings explain fields that cannot be
represented honestly in the canonical recipe model.

The archive's declared mash/boil settings are a planning recipe, not necessarily
the same as its separately editable Z/Zymatic MachineSteps. All converted chamber
locations are deliberately unmapped. Native programmes require their own model-
specific import path.
"""

from __future__ import annotations

import argparse
from collections import Counter
import html
import json
import math
from pathlib import Path
import re
from typing import Any


US_GALLON_LITERS = 3.785411784
POUND_KILOGRAMS = 0.45359237
OUNCE_GRAMS = 28.349523125
US_TEASPOON_MILLILITERS = 4.92892159375

ROUTING_WARNING = (
    "This is an editable Z/Zymatic source plan, not a Pico machine programme. "
    "Every chamber is unmapped (location 0). Review ingredient quantities, "
    "timings and actual equipment before preparing a machine programme."
)
DECLARED_PLAN_WARNING = (
    "The plan uses the source's declared mash, boil and whirlpool settings. "
    "Its separately editable MachineSteps, including ramps, pauses, drains and "
    "chamber assignments, remain in the original JSON and are not imported as "
    "Pico routing. Those settings can disagree; review the original programme."
)
YEAST_WARNING = (
    "The source specifies yeast identity but no quantity. Yeast details are "
    "retained in notes; no yeast dose was invented or included in ingredient totals."
)


def _number(value: Any, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{label} must be a number")
    result = float(value)
    if not math.isfinite(result):
        raise ValueError(f"{label} must be finite")
    return result


def _text(value: Any, label: str, *, optional: bool = False) -> str:
    if value is None and optional:
        return ""
    if not isinstance(value, str):
        raise ValueError(f"{label} must be text")
    # Decode source HTML entities, but never treat the result as HTML. Consumers
    # must render these values as text; the original bytes remain in the archive.
    result = html.unescape(value).strip()
    if "\x00" in result or not result and not optional:
        raise ValueError(f"{label} must be nonempty text without NUL bytes")
    result.encode("utf-8")
    return result


def _objects(value: Any, label: str) -> list[dict[str, Any]]:
    if value is None:
        return []
    if not isinstance(value, list) or any(not isinstance(x, dict) for x in value):
        raise ValueError(f"{label} must be an array of objects")
    return value


def _fahrenheit(value: Any, label: str) -> float:
    return (_number(value, label) - 32.0) * 5.0 / 9.0


def _display(value: Any) -> str:
    if value is None:
        return "unspecified"
    if isinstance(value, float) and value.is_integer():
        return str(int(value))
    return str(value)


def _source_addition(item: dict[str, Any], collection: str) -> str:
    name = _text(item.get("Name"), f"{collection}.Name")
    line = (
        f"{collection}: {name}; source amount {_display(item.get('Amount'))}; "
        f"source units {_display(item.get('Units'))}"
    )
    if "Time" in item:
        line += f"; source time {_display(item['Time'])}"
    return line + "."


def _misc_amount(amount: Any, unit: Any) -> tuple[float, str] | None:
    value = _number(amount, "addition Amount")
    if value <= 0 or not isinstance(unit, str):
        return None
    units = {
        "g": (1.0, "g"),
        "kg": (1000.0, "g"),
        "mg": (0.001, "g"),
        "lb": (POUND_KILOGRAMS * 1000.0, "g"),
        "lbs": (POUND_KILOGRAMS * 1000.0, "g"),
        "oz": (OUNCE_GRAMS, "g"),
        "ml": (1.0, "mL"),
        "l": (1000.0, "mL"),
        "tsp": (US_TEASPOON_MILLILITERS, "mL"),
        "tbsp": (US_TEASPOON_MILLILITERS * 3.0, "mL"),
        "count": (1.0, "count"),
    }
    conversion = units.get(unit.strip().lower())
    if conversion is None:
        return None
    scale, target_unit = conversion
    return value * scale, target_unit


def convert_public(
    raw: dict[str, Any], list_metadata: dict[str, Any] | None = None
) -> tuple[dict[str, Any], list[str]]:
    """Return an editable canonical recipe and explicit conversion warnings.

    No source object is mutated. Unsupported units and unknown quantities stay
    in notes, not ingredient totals. Unexpected source schemas raise ValueError
    so a caller can exclude/report a record instead of silently mis-converting it.
    """
    try:
        source = raw["VM"]["Recipe"]
    except (KeyError, TypeError) as exc:
        raise ValueError("expected VM.Recipe public-archive record") from exc
    if not isinstance(source, dict) or source.get("IsMetric") is not False:
        raise ValueError("only the verified IsMetric=false archive schema is supported")
    metadata = list_metadata or {}
    warnings = [ROUTING_WARNING, DECLARED_PLAN_WARNING]
    notes: list[str] = []
    retained: list[str] = []

    def warn(message: str) -> None:
        if message not in warnings:
            warnings.append(message)

    name = _text(source.get("Name"), "Name")
    author = _text(source.get("Author"), "Author", optional=True)
    if author:
        notes.append(f"Source author: {author}.")
    original_author = _text(source.get("OriginalAuthor"), "OriginalAuthor", optional=True)
    original_name = _text(source.get("OriginalName"), "OriginalName", optional=True)
    if original_author and original_author != author:
        attribution = f"Original attribution: {original_author}"
        if original_name:
            attribution += f" — {original_name}"
        notes.append(attribution + ".")
    original_notes = _text(source.get("Notes"), "Notes", optional=True)
    if original_notes:
        notes.append("Source recipe notes:\n" + original_notes)
    tasting = _text(source.get("TastingNotes"), "TastingNotes", optional=True)
    if tasting:
        notes.append("Source tasting notes:\n" + tasting)

    beer_style = source.get("BeerStyle") or {}
    style = _text(beer_style.get("StyleNameCode") or metadata.get("Style") or "", "Style", optional=True)
    description = _text(metadata.get("Description") or "", "Description", optional=True)
    recipe: dict[str, Any] = {
        "name": name,
        "style": style,
        "description": description,
        "batch_liters": _number(source.get("BatchSize"), "BatchSize") * US_GALLON_LITERS,
        "efficiency": _number(source.get("Efficiency"), "Efficiency"),
        "ingredients": [],
        "steps": [],
        "tags": ["public-archive", "z-zymatic-source"],
        "notes": "",
        "boil_minutes": _number(source.get("BoilTime"), "BoilTime"),
        "fermentation": [],
    }
    if recipe["batch_liters"] <= 0 or not 0 < recipe["efficiency"] <= 100:
        raise ValueError("source batch size or efficiency is invalid")

    # Starting water is a real source quantity. It is not a pre-boil volume and
    # does not encode mash water chemistry, absorption, dead space or evaporation.
    start_water = source.get("H2O")
    if start_water is not None:
        liters = _number(start_water, "H2O") * US_GALLON_LITERS
        if liters > 0:
            recipe["ingredients"].append({
                "name": "Source starting water", "kind": "water", "amount": liters,
                "unit": "L", "use": "mash", "minutes": 0,
            })
            retained.append(
                f"Source starting water: {_display(start_water)} US gal ({liters:g} L). "
                "This is the archived Z/Zymatic fill amount, not a verified Pico fill or pre-boil volume."
            )
    boil_size = source.get("BoilSize")
    if boil_size is not None and _number(boil_size, "BoilSize") > 0:
        recipe["boil_liters"] = float(boil_size) * US_GALLON_LITERS
    else:
        warn("Pre-boil volume is unspecified. Starting water was not substituted for boil_liters; review this before BeerXML export.")

    fermentable_types = {
        "Grain": "Grain", "OtherGrain": "Grain", "Sugar": "Sugar",
        "Extract": "Extract", "DryExtract": "Dry Extract",
    }
    found_types: set[str] = set()
    for item in _objects(source.get("Fermentables"), "Fermentables"):
        item_name = _text(item.get("Name"), "Fermentables.Name")
        amount = _number(item.get("Amount"), "Fermentables.Amount")
        kind = fermentable_types.get(item.get("FermentableType"))
        if amount <= 0 or kind is None:
            retained.append(_source_addition(item, "Fermentables"))
            warn("A fermentable has an unknown type or nonpositive quantity and is retained in notes, not ingredient totals.")
            continue
        found_types.add(kind)
        recipe["ingredients"].append({
            "name": item_name, "kind": "fermentable", "amount": amount * POUND_KILOGRAMS,
            "unit": "kg", "use": "mash", "minutes": 0, "type": kind,
            "yield": _number(item.get("Yield"), "Fermentables.Yield") * 100.0 / 46.0,
            "color": _number(item.get("Color"), "Fermentables.Color"),
        })
    if found_types:
        has_extract = bool(found_types & {"Extract", "Dry Extract"})
        has_grain = "Grain" in found_types
        recipe["type"] = "Partial Mash" if has_extract and has_grain else "Extract" if has_extract else "All Grain"

    for collection, use in (("Hops", "boil"), ("DryHops", "dry-hop"), ("WhirlpoolHops", "aroma")):
        for item in _objects(source.get(collection), collection):
            amount = _number(item.get("Amount"), collection + ".Amount")
            duration = _number(item.get("Time"), collection + ".Time")
            if amount <= 0 or duration < 0:
                retained.append(_source_addition(item, collection))
                warn("A hop has a nonpositive quantity or invalid timing and is retained in notes, not ingredient totals.")
                continue
            recipe["ingredients"].append({
                "name": _text(item.get("Name"), collection + ".Name"), "kind": "hop",
                "amount": amount * OUNCE_GRAMS, "unit": "g", "use": use,
                "minutes": duration, "alpha": _number(item.get("Alpha"), collection + ".Alpha"),
            })
            if collection == "DryHops" and duration == 0:
                warn("Dry-hop duration is unspecified (source Time=0); the amount is retained but no contact duration was invented.")
            if collection == "Hops" and item.get("BoilUse") not in (0, None):
                retained.append(f"Hops: {item['Name']}; source BoilUse={item['BoilUse']}, Time={_display(item.get('Time'))}. Review the addition's use against the original programme.")
                warn("Some main-hop records have non-default source BoilUse codes. Their placement remains a review item; it is not inferred from a numeric code.")
    if source.get("IsFirstWort"):
        warn("The source marks first-wort hopping but does not identify individual first-wort additions here. Review the original loading instructions and hop use.")
        retained.append("Source IsFirstWort=true; no particular hop was reassigned automatically.")

    for collection, use in (
        ("Adjuncts", "boil"), ("WhirlpoolAdjuncts", "aroma"),
        ("DryAdjuncts", "fermentation"), ("Amendments", "mash"),
    ):
        for item in _objects(source.get(collection), collection):
            converted = _misc_amount(item.get("Amount"), item.get("Units"))
            if converted is None:
                retained.append(_source_addition(item, collection))
                warn("Some adjunct or water-amendment quantities have missing/unsupported units or nonpositive amounts. They are retained in notes, not ingredient totals; no units were guessed.")
                continue
            amount, unit = converted
            duration = _number(item.get("Time", 0), collection + ".Time")
            item_type = "Water Agent" if collection == "Amendments" else _text(item.get("AdjunctType") or "Other", collection + ".AdjunctType")
            recipe["ingredients"].append({
                "name": _text(item.get("Name"), collection + ".Name"), "kind": "misc",
                "amount": amount, "unit": unit, "use": use, "minutes": duration, "type": item_type,
            })
            if str(item.get("Units", "")).strip().lower() in ("tsp", "tbsp"):
                warn("Source teaspoon/tablespoon quantities use US customary volume measures (IsMetric=false); they were converted to mL, not to an assumed mass.")
            if collection in ("WhirlpoolAdjuncts", "DryAdjuncts"):
                warn("Whirlpool/fermentation adjunct use is retained explicitly; standard BeerXML misc-use categories may require review or an extension.")

    yeast = source.get("Yeast")
    if yeast:
        if not isinstance(yeast, dict):
            raise ValueError("Yeast must be an object")
        identity = " / ".join(
            _text(yeast.get(key), "Yeast." + key, optional=True)
            for key in ("Laboratory", "ProductID", "Name") if yeast.get(key)
        )
        parts = [f"Yeast: {identity}; quantity unspecified."]
        for low, high, label, unit in (
            ("MinAtten", "MaxAtten", "source attenuation range", "%"),
            ("MinTemp", "MaxTemp", "source temperature range", "°F"),
        ):
            if yeast.get(low) is not None and yeast.get(high) is not None:
                parts.append(f"{label}: {_display(yeast[low])}–{_display(yeast[high])}{unit}.")
        if yeast.get("ExpectedTemp") is not None:
            parts.append(f"Source expected temperature: {_display(yeast['ExpectedTemp'])}°F.")
        if yeast.get("ExpectedAtten") is not None:
            parts.append(f"Source expected attenuation: {_display(yeast['ExpectedAtten'])}%.")
        retained.append(" ".join(parts))
        warn(YEAST_WARNING)

    def plan_step(item: dict[str, Any], *, kind: str, duration_key: str, context: str) -> None:
        item_name = _text(item.get("Name"), context + ".Name")
        duration = _number(item.get(duration_key), context + "." + duration_key)
        target = _fahrenheit(item.get("Temp"), context + ".Temp")
        if not 0 <= target <= 100 or duration <= 0 or duration != int(duration):
            retained.append(
                f"Unmapped {context}: {item_name}; target {_display(item.get('Temp'))}°F; "
                f"{duration_key}={_display(item.get(duration_key))}."
            )
            warn(f"A {context} setting has an unsupported temperature, zero/invalid duration or fractional minutes; it remains in notes and the original rather than being rounded or assigned a new duration.")
            return
        step: dict[str, Any] = {
            "name": item_name, "kind": kind, "target_c": target,
            "minutes": int(duration), "drain_minutes": 0, "location": 0,
        }
        if context == "mash":
            source_type = item.get("Style")
            if source_type in ("Infusion", "Temperature", "Decoction"):
                step["type"] = source_type
            elif source_type:
                retained.append(f"Mash style for {item_name}: {source_type}.")
                warn("An unsupported mash style remains in notes for review.")
        recipe["steps"].append(step)

    for item in _objects(source.get("MashSteps"), "MashSteps"):
        kind = "mash-out" if re.fullmatch(r"mash[ -]*out", str(item.get("Name", "")), flags=re.I) else "mash"
        plan_step(item, kind=kind, duration_key="Time", context="mash")

    if recipe["boil_minutes"] > 0:
        target = _fahrenheit(source.get("BoilTemp"), "BoilTemp")
        duration = recipe["boil_minutes"]
        if 0 <= target <= 100 and duration == int(duration):
            recipe["steps"].append({
                "name": "Boil", "kind": "boil", "target_c": target,
                "minutes": int(duration), "drain_minutes": 0, "location": 0,
            })
            if target < 85:
                warn("The declared boil target is unusually low; it was preserved as Fahrenheit, not silently reinterpreted as Celsius. Review the source.")
        else:
            retained.append(f"Unmapped declared boil: {_display(source.get('BoilTemp'))}°F for {_display(duration)} min.")
            warn("The declared boil has an unsupported target or fractional duration. It remains in notes and the original; no replacement target or duration was invented.")
    elif recipe["boil_minutes"] == 0:
        warn("The source declares zero boil time; no timed boil step was invented. Review the original machine programme and recipe notes.")
    else:
        raise ValueError("BoilTime must not be negative")

    # Time is a remaining-duration countdown, while BoilTime is the actual
    # segment length. Using Time for every segment would double-count a stand.
    for item in _objects(source.get("WhirlpoolSteps"), "WhirlpoolSteps"):
        plan_step(item, kind="hold", duration_key="BoilTime", context="whirlpool")

    for item in _objects(source.get("FermentationSteps"), "FermentationSteps"):
        item_name = _text(item.get("Name"), "FermentationSteps.Name")
        days = _number(item.get("Days", 0), "FermentationSteps.Days")
        hours = _number(item.get("Hours", 0), "FermentationSteps.Hours")
        minutes = _number(item.get("Minutes", 0), "FermentationSteps.Minutes")
        duration = days + hours / 24.0 + minutes / 1440.0
        target = _fahrenheit(item.get("Temp"), "FermentationSteps.Temp")
        if duration > 0 and days >= 0 and hours >= 0 and minutes >= 0 and -10 <= target <= 60:
            recipe["fermentation"].append({"name": item_name, "target_c": target, "days": duration})
        else:
            retained.append(
                f"Unscheduled fermentation phase: {item_name}; source target {_display(item.get('Temp'))}°F; "
                f"duration {_display(days)} d, {_display(hours)} h, {_display(minutes)} min."
            )
            warn("A fermentation/cold-crash phase has no usable duration or target. Its original values are retained in notes; no schedule was invented.")

    # Existing archive validation messages are data, not instructions. Keep them
    # visible instead of making a clean import imply that the source is verified.
    for field in ("Errors", "Warnings"):
        messages = source.get(field) or []
        if messages:
            readable = []
            for entry in messages:
                if isinstance(entry, dict):
                    message = entry.get("Error") or entry.get("Message") or json.dumps(entry, ensure_ascii=False, sort_keys=True)
                else:
                    message = str(entry)
                readable.append(html.unescape(str(message)))
            retained.append(f"Source {field.lower()}: " + " | ".join(readable))
            warn(f"The original source contains {field.lower()}; these are retained in notes and require review.")

    if retained:
        notes.append("Source details and items requiring review:\n" + "\n".join(retained))
    notes.append("Machine mapping: " + DECLARED_PLAN_WARNING)
    recipe["notes"] = "\n\n".join(notes)
    if not recipe["steps"]:
        raise ValueError("source has no representable declared plan steps")
    return recipe, warnings


def _main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="local picobrew-recipes checkout")
    parser.add_argument("--output-jsonl", type=Path, help="optional converted records for local validation")
    args = parser.parse_args()
    metadata = json.loads((args.archive / "data/recipe-list-community.json").read_text())["PublicRecipesList"]
    by_guid = {item["GUID"]: item for item in metadata}
    counts: Counter[str] = Counter()
    failures = []
    output = []
    for path in sorted((args.archive / "data/recipes").glob("*.json")):
        try:
            recipe, warnings = convert_public(json.loads(path.read_text()), by_guid.get(path.stem))
        except (ValueError, TypeError, KeyError) as exc:
            failures.append({"path": path.name, "error": str(exc)})
            continue
        output.append({"source_path": str(path.relative_to(args.archive)), "recipe": recipe, "warnings": warnings})
        counts.update(warnings)
    if args.output_jsonl:
        with args.output_jsonl.open("w") as file:
            for item in output:
                file.write(json.dumps(item, ensure_ascii=False, sort_keys=True, allow_nan=False) + "\n")
    print(json.dumps({"converted": len(output), "failed": failures, "warnings": dict(counts)}, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    _main()
