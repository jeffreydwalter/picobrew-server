# License boundaries

This repository contains components with different provenance. Consolidation does not apply one new license to all of them.

| Scope | License or notice |
| --- | --- |
| Hub application code | [MIT license](hub/LICENSE), copyright 2026 PicoBrew Hub contributors. Dependency licenses remain their own. |
| `hub/internal/catalog/data/catalog.zip` and `firmware/data/starter-recipes.json` | Pinned community recipe/program data, illustrations and original notices. Per-source attribution and license metadata are retained in the catalogue manifest and recipe provenance. The Hub's MIT license does not relicense these records or assets. |
| `legacy/zymatic-server/` | [Original MIT license](legacy/zymatic-server/LICENSE), copyright 2015 Tom Herold. Included third-party assets retain their notices. |
| `firmware/` | Original [LGPL-2.1](firmware/LICENSES/LGPL-2.1.txt), [TI notice](firmware/LICENSES/TI-BSD-notice.txt), per-file notices and [dependency provenance](firmware/dependencies/lock.json) are retained. These are component notices, not a blanket license for every firmware file. |
| New repository-level documentation and integration files outside those components | MIT terms in [the Hub license](hub/LICENSE). This grant does not override the component boundaries above. |

The firmware project's [README](firmware/README.md) states that its recovery does not assign a new license to PicoBrew's original binary or embedded assets. That limitation remains in force here; this import does not resolve rights to those materials or relabel them as MIT.

Repository-wide content licenses for the support/manual mirror, PicoPak catalogue and recipe/site archive were not established during the core packaging work. The catalogue and image integration retains source attribution, original license files and applicable per-file notices without relabeling that material as MIT. See [Community projects](docs/community-projects.md) and the [catalogue guide](hub/docs/community-catalog.md) for source identities and content boundaries. Public availability alone does not establish a license grant.
