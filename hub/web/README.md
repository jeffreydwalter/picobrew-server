# PicoBrew Hub browser application

React, TypeScript, Vite and SCSS frontend for the Go hub. The application talks to
`/api/v1` on the same origin; it does not contact individual machines or AI
providers from the browser. Machine keys and provider keys are submitted to the
hub and never placed in browser storage. Only the local temperature-unit
preference is stored in `localStorage`.

## Build and develop

Use Node.js 22.12 or newer:

```sh
cd web
npm ci
npm test
npm run build
```

The production application is written to `dist/`. Point the Go hub's web directory
at that folder. For frontend development, run the Go server on port 8080 and use:

```sh
npm run dev
```

Vite proxies `/api` to `http://127.0.0.1:8080`. Set `HUB_API_URL` if your development
server uses another address. The frontend has no runtime Node server requirement.
Pages are loaded separately; charting code is shared by fleet, machine and
history views.

## Working features

- Live fleet updates over server-sent events; machine state and temperatures.
- OLED decoding with the physical Pico's 180-degree orientation, remote dial,
  discovered programs, immediate stop, preferences and optional forwarding.
- Shared recipe editing, ingredients and schedule, JSON/BeerXML import/export,
  shopping lists, revision history, adaptation preview and targeted deployment.
- Retained/live brew history, temperature and controller charts, full sample
  tables, timestamp annotations in notes, ratings, and CSV/JSON exports.
- Optional provider-backed recipe drafts, live advice and brew analysis. Periodic
  advice is shown in the assistant feed. All machine actions remain user-driven.
- Machine onboarding, connection checks, credential setup and data export.
- Authenticated interactive OpenAPI explorer with explicit opt-in for mutations.

The data contracts are defined in `src/types.ts` and mirror the Go domain types.
Additional loaded recipe, ingredient and step properties are retained when
editing; recipe-level metadata can also be edited in the advanced JSON section.

## Verification

```sh
npm test
npm run build
npx playwright install chromium
npm run test:browser
```

Playwright builds and starts an isolated Go demo server in a temporary data
directory, then cleans it up. Set `GO_BINARY` if Go is not on PATH. To test an
already running simulation server, set `HUB_TEST_URL`; every test verifies demo
mode before any mutation. Tests create simulation recipes and update journal
notes, and never require physical brewing hardware. Playwright uses its bundled Chromium by default; set `CHROMIUM_PATH` to use a
system Chromium executable (for example `/usr/bin/chromium`). The suite exercises desktop/mobile navigation,
front-panel rendering, recipe creation/import/revisions, notes and ratings,
downloads, settings, API exploration and horizontal-overflow checks.

Unit tests verify OLED bit order/rotation, sensor presentation and annotations,
CSRF handling, command identifiers and the absence of automatic command retries.
The explorer also handles CSV/XML/text responses, binary downloads, and a bounded
first-event preview for live event streams.

`npm run test:integration` runs DOM interaction tests against a separate, temporary
Go demo server without launching a browser. It covers live data/OLED decoding,
recipe persistence, brew notes/ratings, calibration, and API exploration. This
checks real API integration; visual layout and browser behavior are covered by
the Playwright suite above. Both runners accept `GO_BINARY`.
