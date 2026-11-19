# Offline community catalogue

The Hub bundles a pinned archive of recipes, machine programs, illustrations and
support references. Browsing and recipe copying work without picobrew.com or an
upstream server. The source archive is read-only; **Copy to library** creates an
independent editable recipe with its own revision history. Copying never sends a
recipe to a brewer or starts a program.

## Included content

| Source | Original records | Catalogue entries | Treatment |
| --- | ---: | ---: | --- |
| Public PicoBrew recipes | 1,320 | 1,320 | Ingredient recipes with source quantities, declared mash/boil plans and conversion warnings. |
| PicoPak/ZPak library | 10,161 | 3,925 | Native machine programs. Exact duplicate PicoPak variants are grouped; all originals remain available. |
| Chiefwigms server | 37 | 37 | Base programs and three Zymatic maintenance programs, with original notes. |

There are **5,282 browsable entries**: 1,320 ingredient recipes, 3,959 native
programs and three maintenance programs. Of these, 5,272 can be copied into the
library. There are 3,755 programs within Pico S/Pro wire limits; deployment also
requires compatible firmware, verified device identity and sufficient storage.

The PicoPak archive contains 247 malformed JSON records and 107 empty programs.
They are excluded from browsing and preserved byte-for-byte in the embedded
source archive and `excluded.json`. Entries with long or non-ASCII machine labels
remain copyable, but require explicit editing before deployment; labels and
timings are never silently rewritten during native import.

The bundle also contains 88 image/text assets, including beer colour/style
illustrations, machine drawings and support text. These illustrations are labelled
by kind; they are not presented as photographs of the named beers. An additional
27 original PDF assets are optional local references. The references API reports
which files are actually available.

## Recipe semantics

**Ingredient recipes** retain converted source measurements and declared plans.
The original public recipes target Z/Zymatic, and their separately editable
`MachineSteps` may disagree with those plans. They remain in original JSON.
Imported chamber locations are explicitly unmapped (`0`), so a plan requires
review and verified chamber assignments before a Pico deployment. Unknown yeast
doses, unsupported quantities, fractional/invalid timings and contradictory
source notes are recorded as review items; quantities or timings are not guessed.

**Native programs** retain their exact temperatures, timings, drain durations and
model-specific compartment routing. They do not acquire an inferred ingredient
bill, batch size or efficiency, and cannot be scaled by batch volume. A zero-minute
step retains zero: it waits for its temperature target and advances. The target
firmware must advertise `zero_minute_steps` support before accepting such a
transfer. Copying preserves the native program ID so the transfer preview can
recognize a program already present on the brewer.

The ten chiefwigms Pico base recipes use named locations and a separate `Prime`
step whose translation is not verified here. Their notes, illustrations and
original JSON are browsable, but automatic copying/deployment is disabled.
Z/Zymatic programs and cleaning programs retain their original routing and are
reference/export records while no live adapter for those models exists.

Every entry carries its repository, pinned commit, source path, SHA-256,
attribution/license notice and conversion warnings. Grouped PicoPak entries expose
each original record independently. Public recipe statistics come from the source;
unverified native `Abv`, `Ibu`, `MinTime` and `MaxTime` fields remain source
parameters rather than being misrepresented as measured statistics or controls.

## API

All endpoints require the normal owner session; copying also requires CSRF
protection.

| Method and path | Result |
| --- | --- |
| `GET /api/v1/catalog` | Search with `q`, `source`, `kind`, `model`, `style`, `offset` and `limit` (default 24, maximum 100). Returns entries, total and source/style metadata. |
| `GET /api/v1/catalog/{id}` | Recipe/program detail, original record list, provenance and source parameters. |
| `POST /api/v1/catalog/{id}/copy` | Empty JSON object; returns a new saved library recipe and initial revision. |
| `GET /api/v1/catalog/{id}/source` | Exact original JSON attachment; `record` selects another original listed on the entry. |
| `GET /api/v1/catalog/references` | Support references and asset metadata, optionally filtered by `model`. |
| `GET /api/v1/catalog/assets/*` | Manifest-listed local image/text/PDF, with pinned checksum verification. |

No catalogue endpoint fetches arbitrary URLs. Source paths are selected from the
entry manifest, assets are selected from the asset manifest, and optional files
are constrained to the reference directory and checked against their pinned hash.

## Rebuild the pinned bundle

Normal installation needs no source checkouts or catalogue-generation step. The
Go executable embeds `internal/catalog/data/catalog.zip`.

To regenerate it, check out the exact four source commits recorded in
`tools/build_catalog.py` into a single directory, using the configured folder
names (`picobrew-recipes`, `picopak-library`, `picobrew_pico` and
`picobrew-support`). From the community repository root:

```sh
python3 hub/tools/build_catalog.py \
  --sources /path/to/pinned-source-checkouts \
  --legacy legacy/zymatic-server \
  --references-output /tmp/picobrew-reference-bundle.zip
CATALOG_REQUIRE_FIRMWARE=1 python3 -m unittest discover -s hub/tools -p 'test_*.py'
cd hub
go test ./internal/catalog ./internal/server
```

The builder checks pinned commits and clean tracked files, preserves source bytes,
rejects active SVG content, and writes reproducible ZIP timestamps and ordering.
Python conversion tests use originals already included in the bundle. Go tests
validate every copyable recipe, every advertised deployment, all original source
hashes, exclusion preservation, and exact native temperatures/timings/routing.
The Python suite also compares every firmware starter's ID, complete schedule and
source checksum to its catalogue copy. `CATALOG_REQUIRE_FIRMWARE=1` makes a missing
firmware tree an error in the community checkout; an isolated Hub source archive
can run the remaining tests without the optional cross-component check.

To install optional manuals, extract the generated reference ZIP into the Hub data
directory. It contains the single top-level directory `catalog-references/`.
For Docker, this directory belongs in the existing `/data` volume. Source links
remain visible when a reference is absent; the Hub never downloads it on demand.
Existing recipes, revisions, calibration settings and deployed programs are not
rewritten when the bundled catalogue changes.

## Source rights

Source repositories without a repository-wide license are identified as such.
Including provenance does not grant a new license to original PicoBrew data,
manuals or artwork. Chiefwigms MIT notices and the legacy icon attribution are
retained alongside original source files; component license boundaries remain
documented in the repository's `LICENSES.md`.
