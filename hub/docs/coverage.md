# Feature and hardware coverage

This table records implemented behavior. It is deliberately not a claim of feature parity with every release of every PicoBrew project.

| Area | Implemented | Boundary |
|---|---|---|
| Fleet | Persistent machine registry, simultaneous independent polling, connection status, live browser stream | One owner; no team roles or remote-site agent |
| Pico S/Pro | Adapter for the standalone API v1: OLED, dial, programs, stop latch, recipes, preferences, history and forwarding | Requires that firmware; protocol-tested with local HTTP fixtures, not newly tested on a physical brewer in this project |
| Z / Zymatic / Pico C | Recipe planning profiles, quantity adaptation and export; simulated operation for demos | Stock cloud protocols and real control adapters are not implemented |
| Recipe library | CRUD, revision conflict protection, archived revisions, tags, notes, ingredients and program editor | No bundled proprietary recipe database or comprehensive ingredient/style catalog |
| Brewing details | Water chemistry, fermentation/carbonation metadata, mash and boil attributes | Metadata does not imply a connected fermenter or automatic addition hardware |
| Adaptation | Volume/efficiency scaling, saved profile overrides, sourced C/S/Pro starting defaults, native compilation and review warnings | Efficiency defaults are estimates; S/Pro grain capacity is unknown. Hop utilization is not fully modeled; plumbing must be explicitly supplied |
| Recipe deployment | Per-machine idle checks, source-revision check, native write/readback, previous-program record | Does not start automatically; no automatic rollback after an uncertain write |
| Installed recipes | Read, deploy/overwrite, choose for brewing, remove while idle | Limited by the appliance's own slots and protocol |
| History | Retained-session imports, durable samples, temperature/state graphs, notes/rating, CSV/JSON | Cannot recover overwritten source samples; old start times may be unknown |
| AI recipe creation | OpenAI/Anthropic API integration, validated structured drafts, explicit save | Provider key/model and external API access required |
| AI model discovery | Account model catalogue, automatic lookup with a saved key, pre-save connection, refresh and manual entry | Model-list permission and chat compatibility are distinct; discovery does not invoke inference |
| AI monitoring | On-demand live advice and optional five-minute active-brew checks with saved results | Advisory; no low-level control or live recipe/temperature overrides |
| AI analysis | Analysis of selected history, downsampled bounded context | No invented measurements; raw archive/export remains available |
| Shopping list | Aggregate selected recipes by ingredient and unit | Inventory purchasing, supplier integrations and stock deduction are absent |
| Telemetry forwarding | Configure machine receiver; ingest known-UID unsigned observations | Unsigned forwarding is not authoritative status or a control channel |
| Data | SQLite persistence, encrypted credentials, portable nonsecret ZIP, full-directory recovery instructions | No browser upload/restore of a full encrypted data directory |
| API | Authenticated REST endpoints, OpenAPI schemas/catalog, in-app explorer, SSE | Hub authentication differs from the appliance's bearer-key API |
| Packaging | Source, pinned dependencies, Dockerfile, Compose, Go/frontend/browser CI | No managed hosting or app-store package |

## Prior projects considered

The product scope was informed by these primary project descriptions and documentation:

- [chiefwigms/picobrew_pico](https://github.com/chiefwigms/picobrew_pico): local cloud replacement, appliance-specific protocol handling and session/recipe workflows.
- [hotzenklotz/picobrew-server](https://github.com/hotzenklotz/picobrew-server): Zymatic protocol and local recipe/session server.
- [PicoClaw](https://github.com/cetezadi/picobrew_picoclaw): another community PicoBrew server implementation.
- [Small Brew Companion documentation](https://www.smallbrewcompanion.com/docs/): recipe creation, equipment/brewing profiles, brewing logs, ingredients/water/fermentation/carbonation and shopping-list workflows.
- [Standalone Pico S/Pro API contract](https://github.com/jeffreydwalter/picobrew-pico-pro-firmware/blob/feature/standalone-rest-api/docs/standalone-api.md) and [OpenAPI](https://github.com/jeffreydwalter/picobrew-pico-pro-firmware/blob/feature/standalone-rest-api/docs/openapi.json): the implemented hardware contract, inspected at firmware commit `63c49d0`.

No application code from those servers or from GoMVC is imported. The UI and server are an independent implementation. Device-specific compatibility should grow through separately tested adapters, without making unsupported operations look available on a real machine.
