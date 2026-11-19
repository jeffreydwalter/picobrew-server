# PicoBrew Hub

A shared brewing workspace: a Go server and React/TypeScript console for a fleet of PicoBrew machines, a versioned recipe library, persistent brew history, and optional AI assistance. Each brewer keeps its own local interface and continues to own its control loop.

**Version 0.1.0.** This is a new server application, separate from the Pico firmware and from GoMVC. Hardware support is explicit: the live adapter targets the standalone REST firmware for Pico S/Pro. Z, Zymatic and Pico C have recipe planning profiles, but their stock cloud protocols and live control adapters are not implemented. A simulator exercises the full hub workflow without connecting to a brewer. See [support and feature coverage](docs/coverage.md).

## Run it on your Mac

Install Go **1.26 or newer** and Node.js **22.12 or newer**. Go's SQLite driver uses CGO; on macOS install the Xcode Command Line Tools if the C compiler is unavailable (`xcode-select --install`).

In the repository, this independent application is in `hub/`. From that directory:

```sh
npm --prefix web ci
npm --prefix web run build
mkdir -p bin
go build -trimpath -o bin/picobrew-hub ./cmd/picobrew-hub
./bin/picobrew-hub --data ./data
```

Open **http://127.0.0.1:8080**. On first launch the server prints a one-time setup token. Enter it in the browser and choose an owner password of at least 12 characters. Subsequent visits use that password. Sessions expire after 24 hours or a server restart.

On **Overview**, click **Add machine**, choose **Pico standalone firmware**, select S or Pro, and enter the brewer's current URL and the API key provisioned with its firmware. The server must be able to reach that URL. For the existing Pico project, the provisioned key is in `~/Code/pico-pro-source/private/api.key`; enter it in the local browser form, not in a chat or a recipe. The hub encrypts it at rest and does not send it back to the browser.

The address `http://picobrew.s.local` can be used when your Mac resolves it. If a container or remote host cannot resolve that name, use the current IP shown on the brewer. Adding a machine starts capability discovery, status polling and history reads. It does not start a brew.

Edit or remove a connection from its Overview card or its individual machine page, including while it is offline. **Edit connection** manages the Hub registration; **Brewer preferences** changes settings on an online brewer. Removing a registration retains its archived Hub brew history. Active-operation restrictions still apply. See [machine connections and API keys](docs/device-connections.md).

Use a unique provisioned API key for each physical brewer. The Hub shows **Shared API key** when the same hexadecimal credential is recorded for more than one physical-machine registration, including disabled records. This is an advisory, so existing connections remain usable. Entering a replacement in the Hub updates its saved credential; it does not provision or rotate the key on the brewer.

### Explore the simulator first

```sh
./bin/picobrew-hub --demo --data ./demo-data
```

This seeds three simulated brewers, two example recipes and three historical brew traces. Demo mode is restricted to loopback and rejects hardware adapters, including hardware records found in an existing data directory. Use a different directory from your real workspace. Seed recipes and traces are examples, not measurements from your machines. No provider credentials are included; AI requests are disabled until you explicitly configure them.

## What you can do

- **Manage a fleet.** See each machine's connection, state, temperature and active recipe. Open several machines in separate tabs while the fleet continues streaming updates.
- **Operate an S/Pro.** Mirror its OLED, turn or press its dial, select an installed recipe, open its advertised maintenance programs, stop outputs, and update its display/forwarding preferences. The console exposes the capabilities reported by that device.
- **Curate recipes.** Maintain ingredients, mash/boil steps, water chemistry, fermentation and carbonation details, tags and notes. Every save creates a revision; simultaneous edits are detected. Import/export JSON and BeerXML, and aggregate selected recipes into a shopping list.
- **Adapt and deploy.** Save measured model defaults in Settings, preview batch and efficiency scaling for the target model, review warnings and compartment routing, then deploy the compiled recipe to one idle machine. Deployment and starting are separate actions. The original library recipe stays intact.
- **Keep the brew record.** Import retained sessions automatically, preserve their available sensor samples beyond the device's ring buffer, chart temperatures and phase/controller state, add notes and ratings, and export JSON or CSV. Disconnections, truncated data and unknown start times remain visible.
- **Use optional AI.** Supply an OpenAI or Anthropic API key and a model available to your account. Generate a structured recipe draft, ask about an active brew, or analyze recorded history. An opt-in monitor reviews online active brews at most once every five minutes per machine, with advice saved in the console. Drafts require review and saving. The AI has no machine command tools.
- **Build other clients.** Browse the authenticated API explorer or download `/api/v1/openapi.json`. The same API serves the UI, with session authentication, CSRF protection and idempotent command receipts.

### Connect the brew assistant

In **Settings → Brew assistant**, choose OpenAI or Anthropic, enter a provider API key and click **Connect and load models**. Select a model from the returned account catalogue, enable the assistant and save. Connection discovery works before a model is selected and does not save the supplied key until you save settings. Returning to Settings with a saved key loads its models automatically; **Refresh models** reloads the catalogue.

The model list comes from the provider's API, including all Anthropic result pages. Choose a model that supports the provider's chat API; a listing alone does not establish that compatibility or inference permission. Manual model entry remains available for private models or restricted keys that cannot list models. Changing providers clears the previous catalogue and uses only the selected provider's key.

### Equipment starting points

The C/S/Pro profiles use **5 L and approximately 50% efficiency** from Small Brew Companion's documented recipe-scaling guidance, with source links visible in Settings. The Pro's nominal 5 L batch is also supported by PicoBrew's published specifications. Efficiency is a planning estimate, and S/Pro grain capacity remains unknown until supplied for the actual grain container. Saved calibration values override these starting points. See [equipment profiles and sources](docs/equipment-profiles.md).

## Brewing workflow

1. Create or import a recipe in the shared library.
2. Select a target machine and batch volume. Review the adapted ingredients and generated native program.
3. Assign the actual physical compartment for every machine step. An imported recipe does not establish plumbing; unknown mappings block deployment. Infusions, decoction and timed ramps that the native protocol cannot represent also block deployment.
4. Deploy while the machine is ready at its main menu. The server verifies the stored program; it does not start it.
5. Open that machine's page, select the installed recipe and confirm Start. Follow the machine's preparation prompts through its physical or mirrored front panel.
6. Watch progress on the fleet or machine page. The journal imports available retained samples. Finish with notes, a rating, exports and optional analysis.

A successful HTTP command response means the firmware queued the request. It is not evidence that heating or pumping has begun. An uncertain command delivery is recorded and never automatically retried. Current standalone firmware has no dedicated pause/resume or live recipe/temperature override; the hub and AI do not fabricate those operations.

## Run with Docker

```sh
docker compose up --build -d
docker compose logs hub
```

The default Compose file publishes only `127.0.0.1:8080` and persists the SQLite database and encryption key in the `hub-data` volume. Use the setup token from the logs to create the owner account. The image runs as a nonroot user with a read-only application filesystem.

For access from other computers on a trusted LAN, change the published host address to the LAN address or `0.0.0.0`. For a public HTTPS site, terminate TLS at a reverse proxy and add `--secure-cookies` to the server command. Preserve the request Host header; do not cache `/api/` responses or buffer `/api/v1/events`. Example Caddy configuration:

```caddyfile
brew.example.com {
    reverse_proxy 127.0.0.1:8080
}
```

A cloud host needs a private route to the machines, such as a site VPN. This release does not contain an outbound site agent or NAT traversal service. Keep the brewer's HTTP API on that private route; only the authenticated hub needs to be browser-accessible. The on-device UI works independently when the hub is unavailable.

## Optional firmware telemetry forwarding

The hub accepts the standalone firmware's envelope at `POST /api/v1/telemetry`. Configure the receiver host and port through a machine's settings. Register and connect that machine first so its UID is known.

That firmware envelope is unsigned. The hub stores it as an explicitly untrusted observation and does not use it to authorize commands or replace authenticated polled status. Full archived brew history comes from the authenticated history API. Forwarding does not make an otherwise unreachable machine remotely controllable.

## Data and recovery

The data directory contains `hub.sqlite` (with its WAL/SHM files while running) and `master.key`. The key encrypts device and AI credentials in the database. Do not commit this directory.

The browser's **Export workspace** action downloads portable recipe, machine and brew records without passwords or API keys. It is a data export, not a full disaster-recovery image.

For a complete backup, stop the server, copy the **entire data directory**, and restart. Restore that directory to a stopped server using the same application version or a version with a compatible migration. Preserve `master.key` with the database; a new key cannot decrypt previous credentials. Docker users should stop the Compose service while archiving/restoring the `hub-data` volume. See [operations](docs/operations.md).

## Development and tests

```sh
# Go tests, including SQLite, HTTP adapters, concurrency and command outcomes
go test -race ./...
go vet ./...

# Frontend typecheck / optimized build and focused unit tests
npm --prefix web ci
npm --prefix web run build
npm --prefix web test

# Browser integration suite starts and removes its own isolated simulator
cd web
npx playwright install chromium
npm run test:browser
```

If Chromium is installed separately, set `CHROMIUM_PATH` to its executable. `HUB_TEST_URL` targets an existing **demo** instance instead of starting the suite's isolated server on port 18080. The suite checks demo mode before making any changes.

For interactive development run the Go server and `npm --prefix web run dev`. Vite proxies `/api` to the Go server. The server serves production assets from `web/dist` by default; `--web` changes this path. `HUB_PASSWORD` can provision a password noninteractively only when no owner account exists. Existing owner credentials are never overwritten by that variable.

## Project map

| Directory | Responsibility |
|---|---|
| `cmd/picobrew-hub` | Process configuration and graceful shutdown |
| `internal/server` | HTTP API, owner sessions, encrypted credentials, fleet orchestration, history import, optional AI monitor |
| `internal/device` | Exact S/Pro REST adapter and deterministic, stateful simulator |
| `internal/recipe` | Canonical recipe validation, adaptation, native compilation, JSON/BeerXML exchange |
| `internal/store` | SQLite migrations, record versions, encrypted blobs, deduplicated telemetry |
| `internal/ai` | Bounded OpenAI/Anthropic clients with no machine tools |
| `internal/domain` | Shared server models |
| `web` | React/TypeScript UI, SCSS, unit and browser tests |
| `docs` | Architecture, operations, support coverage and verification record |

The implementation is independent of GoMVC and of the existing Python PicoBrew servers. Their public capabilities informed the product scope; protocol compatibility is based on the standalone firmware's published contract. See [architecture](docs/architecture.md) and [coverage](docs/coverage.md).
