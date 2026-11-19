# Community projects and consolidation scope

The community repository contains the active Hub and standalone firmware together. The older projects provide protocol references, a manual mirror, and the pinned recipe/image data now included in the Hub's offline catalogue. Source snapshots remain traceable through per-entry provenance and checksums.

| Existing repository | Role | Treatment here |
| --- | --- | --- |
| [picobrew-server](https://github.com/jeffreydwalter/picobrew-server) | Originally a fork of Tom Herold's Python Zymatic server; now also contains the new Hub | Existing Hub remains in `hub/`; original Python files and MIT license move to `legacy/zymatic-server/`. This repository hosts the consolidation branch. |
| [picobrew-pico-pro-firmware](https://github.com/jeffreydwalter/picobrew-pico-pro-firmware) | Firmware recovery, standalone REST/local UI, hardware evidence and USB tools | Its pinned tracked tree is imported intact at `firmware/`. The source repository remains unchanged. |
| [picobrew_pico](https://github.com/jeffreydwalter/picobrew_pico) | Fork of the chiefwigms community server | Protocol, recipe and image source for shared catalogue and feature integration. Its MIT notice must remain with reused code/assets where applicable. |
| [picobrew-support](https://github.com/jeffreydwalter/picobrew-support) | Support/manual mirror | Bundled support text and images plus optional local PDF references. Repository-wide content-license scope is not established here. |
| [picopak-library](https://github.com/jeffreydwalter/picopak-library) | PicoPak/ZPak JSON catalogue and synchronization tooling | Native programs imported without fabricated ingredients or batch sizes; duplicate variants are grouped while every original record is retained. |
| [picobrew-recipes](https://github.com/jeffreydwalter/picobrew-recipes) | Public recipe JSON and site archive | 1,320 ingredient recipes and source artwork included with conversion warnings and exact original records. JSON/BeerXML exchange remains available for complete recipes. |

These links identify the source archive locations. None is a Git submodule or a required second checkout for starting the Hub. Catalogue records and bundled assets are served locally with per-source provenance. Ordinary dependency installation and the firmware's pinned toolchain download remain separate build steps.

## Integration scope

Recipe/image ingestion, JSON/BeerXML exchange, capacity-limited bulk transfer, native cleaning records and durable brew history are covered by the Hub implementation and tests. Z/Zymatic/Pico C live control and migration of older server databases remain separate work; importing their archives does not supply those adapters or migrations. See the [catalogue guide](../hub/docs/community-catalog.md) for exact conversion boundaries.

See the [Hub coverage matrix](../hub/docs/coverage.md), [firmware guide](../firmware/README.md), and [license boundaries](../LICENSES.md) before treating any reference project as an installed capability.
