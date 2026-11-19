# Recipe exchange and adaptation

Recipes are canonical brewing plans. A separate machine program contains the
small set of fields the standalone S/Pro firmware can execute. Importing a
recipe or making a planning preview does not operate a machine.

## Canonical units

Batch, boil and water volumes use liters. Temperatures use Celsius. Ingredient
addition times use minutes and retain fractional values. Machine-program step
durations use whole minutes; fractional BeerXML mash/boil durations are rejected
with an explanation instead of being rounded.

| Ingredient kind | Units |
| --- | --- |
| `fermentable` | `kg` |
| `hop` | `g` |
| `water` | `L` |
| `yeast`, `misc` | `g`, `mL`, or `count` |

`count` is preserved in JSON. BeerXML export requires a measured mass or volume
and does not guess the contents of a yeast packet. Fermentation durations are
days. Water ions are concentrations in mg/L; `ph` is dimensionless. Concentrations
and fermentation schedules do not change when the batch is scaled.

## Profiles and native programs

Profiles are configurable planning defaults. Pico Pro and S control support
requires the standalone REST firmware. C, Z and Zymatic profiles provide
planning/export only. Changing a profile flag cannot create a missing protocol
adapter. The Z profile represents a single brewing module.

C/S/Pro defaults use 5 L and approximately 50% efficiency from Small Brew
Companion's recipe-scaling guide. PicoBrew's C/Pro specifications also establish
the nominal 5 L batch. The Pico family's grain limit is unverified
(`MaxGrainKG=0`); a warning explicitly says that fit has not been proven.
Z/Zymatic retains its 2.5-US-gallon batch, 9 lb (4.08233133 kg) grain limit and
70% planning assumption. Efficiency defaults are not measurements of a machine.
Saved calibrations override the defaults. See the [profile sources and migration
notes](../../docs/equipment-profiles.md). The configured `BatchLiters` also bounds
the requested batch in the preview.

Grain and unspecified fermentable amounts scale by:

`target batch / source batch × source efficiency / target efficiency`

Known sugar/extract additions scale only by batch volume. Hops and other
ingredients also scale by volume. The preview warns that hop utilization/IBU,
grain absorption, dead space, boil-off, and yeast cell counts are not modeled.
Ingredient addition timings do not automatically create machine steps.

Native generation requires explicit `location` values 1–6 for every step. Zero
means **unmapped** and blocks deployment. Locations have no invented names such
as "grain chamber" in this package. Infusion, decoction, timed ramps, cooling,
and fermentation have no verified mapping to this firmware's step schema and
cannot silently become temperature holds. The caller must prepare a reviewed,
explicit machine program. Fermentation and carbonation remain separate planning
metadata; the hot-side program does not control them.

The native validator implements the actual firmware limits:

| Field | Limit |
| --- | --- |
| ID | 1–20 ASCII letters, digits, `_`, `-` |
| Recipe / step label | 1–31 / 1–21 printable ASCII bytes |
| Steps | 1–15 |
| Location | 1–6, explicitly provided |
| Temperature | Integer 0 (unheated), or 60–212°F |
| Active / drain duration | 1–599 / 0–120 minutes |
| Serialized JSON request | At most 4095 bytes |

Celsius values become integer Fahrenheit targets with explicit rounding warnings.
Zero Celsius in the step program is the unheated sentinel. Native labels replace
unsupported characters with `?` and truncate with explicit warnings; canonical
labels remain unchanged. Long/unsupported canonical IDs get a stable
`hub-<hash>` ID, also disclosed in the preview.

## Exchange scope

JSON imports/exports the full canonical model, as one object or an array of
objects. Imports reject unknown fields, duplicate JSON keys, trailing documents,
excessive nesting, empty recipes and files larger than 1 MiB.

BeerXML 1 import/export supports recipe type, batch/boil volume and duration,
efficiency, style name, notes, fermentables, hops, yeast, miscellaneous additions,
water chemistry, mash holds/infusion/ramp details, up to three fermentation
phases, and carbonation method/volume/temperature. It preserves the standardized
kilogram/liter/Celsius/minute units through explicit conversions. UTF-8 and
ISO-8859-1 input are supported. DTDs/directives and external entities are rejected.

This is a documented exchange subset, not full fidelity for every BeerXML field.
Equipment profiles, full BJCP style statistics, brewer identity, advanced yeast
properties, priming-sugar details and other optional fields are not represented.
Hub descriptions/tags/revisions and custom fermentation stage names are preserved
by JSON, not by standard BeerXML. Multiple distinct water-chemistry profiles are
rejected because the canonical recipe has one common water profile.

BeerXML export requires explicit recipe type (`All Grain`, `Partial Mash` or
`Extract`), positive `boil_liters`/`boil_minutes`, ingredient subtypes, and yeast
form when applicable. It does not substitute finished batch volume for pre-boil
volume. Case-insensitive recognized types emit standard BeerXML casing.

A `HUB_STEPS` extension stores the canonical step array as escaped JSON, retaining
explicit locations and drain times on a hub round-trip. Other BeerXML applications
may ignore/remove that extension. Standard BeerXML imported without it starts
with unmapped locations and therefore cannot be deployed immediately. Native
firmware JSON is a distinct schema; it is not silently treated as a complete
canonical recipe because it lacks batch size and efficiency.

## Source contracts

- [Standalone firmware API: recipe bounds and location semantics](https://github.com/jeffreydwalter/picobrew-pico-pro-firmware/blob/feature/standalone-rest-api/docs/standalone-api.md#recipes-sixteen-records-fifteen-steps-each)
- [Standalone request parser](https://github.com/jeffreydwalter/picobrew-pico-pro-firmware/blob/feature/standalone-rest-api/src/local_api.c)
- [BeerXML 1 specification](https://www.beerxml.com/beerxml.htm)
- [Pico Pro product specification](https://picobrew.com/store/products/pico)
- [Pico C product specification](https://picobrew.com/store/products/picoc)
- [Z product specification](https://picobrew.com/store/products/z)
- [Zymatic manual, brewing and ingredient capacities](https://picodevblob.blob.core.windows.net/content/zymatic/Zymatic_Manual.pdf)
