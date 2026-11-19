"""Focused source-to-Hub invariants; run without executing upstream code."""

import copy
import json
from pathlib import Path
import unittest
import zipfile

from catalog_public import convert_public


BUNDLE = Path(__file__).resolve().parents[1] / "internal/catalog/data/catalog.zip"


def load(guid):
    with zipfile.ZipFile(BUNDLE) as bundle:
        return json.loads(bundle.read(f"originals/public-recipes/data/recipes/{guid}.json"))


class PublicConversionTests(unittest.TestCase):
    def test_real_quantities_without_invented_yeast_or_routing(self):
        raw = load("0069af6cd5754c46b70b7f97723a5b4b")
        original = copy.deepcopy(raw)
        recipe, warnings = convert_public(raw)
        self.assertEqual(raw, original)
        self.assertAlmostEqual(recipe["batch_liters"], 9.46352946)
        grain = next(i for i in recipe["ingredients"] if i["kind"] == "fermentable")
        hop = next(i for i in recipe["ingredients"] if i["kind"] == "hop")
        self.assertAlmostEqual(grain["amount"], 7.5 * 0.45359237)
        self.assertAlmostEqual(grain["yield"], 80.0)
        self.assertAlmostEqual(hop["amount"], 0.5 * 28.349523125)
        self.assertFalse(any(i["kind"] == "yeast" for i in recipe["ingredients"]))
        self.assertIn("Fermentis / US-05 / SafAle", recipe["notes"])
        self.assertNotIn("boil_liters", recipe)
        self.assertTrue(all(s["location"] == 0 for s in recipe["steps"]))
        self.assertTrue(any("chamber is unmapped" in w for w in warnings))

    def test_whirlpool_countdown_is_not_double_counted(self):
        recipe, _ = convert_public(load("676a62af977c4efbb6fce3dcdb953e69"))
        whirlpool = [s for s in recipe["steps"] if s["kind"] == "hold"]
        self.assertEqual([s["minutes"] for s in whirlpool], [5, 30])
        self.assertEqual(sum(s["minutes"] for s in whirlpool), 35)

    def test_invalid_declared_boil_is_not_replaced_from_machine_program(self):
        recipe, warnings = convert_public(load("d46d2263af82491ab22e6d02d83d8570"))
        self.assertFalse(any(s["kind"] == "boil" for s in recipe["steps"]))
        self.assertIn("Unmapped declared boil: 0°F for 90 min.", recipe["notes"])
        self.assertTrue(any("no replacement target" in w for w in warnings))

    def test_explicit_misc_units_and_unknown_units(self):
        raw = load("0069af6cd5754c46b70b7f97723a5b4b")
        raw["VM"]["Recipe"]["Adjuncts"] = [
            {"Name": "Salt", "Units": "mg", "Amount": 50, "Time": 5, "AdjunctType": "Other"},
            {"Name": "Spice", "Units": "TBSP", "Amount": 2, "Time": 5, "AdjunctType": "Spice"},
            {"Name": "Mystery", "Units": None, "Amount": 20, "Time": 5, "AdjunctType": "Other"},
        ]
        recipe, warnings = convert_public(raw)
        by_name = {i["name"]: i for i in recipe["ingredients"]}
        self.assertEqual(by_name["Salt"]["amount"], 0.05)
        self.assertEqual(by_name["Salt"]["unit"], "g")
        self.assertAlmostEqual(by_name["Spice"]["amount"], 29.5735295625)
        self.assertEqual(by_name["Spice"]["unit"], "mL")
        self.assertNotIn("Mystery", by_name)
        self.assertIn("Mystery; source amount 20; source units unspecified", recipe["notes"])
        self.assertTrue(any("no units were guessed" in w for w in warnings))

    def test_unverified_metric_schema_is_rejected(self):
        raw = load("0069af6cd5754c46b70b7f97723a5b4b")
        raw["VM"]["Recipe"]["IsMetric"] = True
        with self.assertRaisesRegex(ValueError, "IsMetric=false"):
            convert_public(raw)


if __name__ == "__main__":
    unittest.main()
