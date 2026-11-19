# Verification record

## Machine management and device-key advisory — local checks

- Full Go race tests, vet and production build: passed.
- Frontend typecheck, production build and all 22 unit tests: passed.
- All 5 DOM integration tests against an isolated Go simulator: passed, including Overview add/edit, offline management, removal cancellation, and retained brew samples after removing a registration.
- Device-key tests cover reuse warnings across active and disabled registrations, exact stored-key preservation, simulator exclusion, changing/removing a duplicate, restart, secret read failures and concurrent fleet snapshots.
- The browser suite now contains 6 tests, including mobile card/modal management. Its result must be taken from the published commit's CI, not inferred from these local checks.

## Model discovery and Settings update — local checks

- All Go package tests with the race detector, `go vet ./...` and a production Go build: passed.
- Frontend typecheck and production build: passed.
- Frontend unit tests: 22 passed, including automatic model loading, connecting an unsaved key, provider changes, stale-response cancellation, manual entry and refresh.
- DOM interaction tests against an isolated real Go server: 4 passed.
- Provider model-list protocol fixtures cover OpenAI credentials, Anthropic pagination, bounded responses, cancellation, redirects and redacted errors. Server tests cover authentication, CSRF, provider-key isolation and discovery without changing saved credentials or configuration.
- Profile regression tests preserve older saved calibration values and existing recipes while applying sourced defaults only to unconfigured profiles or an explicit reset.
- The duplicate Settings API section is removed; browser tests navigate to the explorer from the sidebar.

The later [branch run at b8e2542](https://github.com/jeffreydwalter/picobrew-server/actions/runs/44440433270)
and [PR run](https://github.com/jeffreydwalter/picobrew-server/actions/runs/44440440070)
both passed all four Hub jobs: Linux/macOS Go checks, frontend build with 22 unit,
4 DOM and 5 browser tests, and container build/startup/restart. The simulator build
now has its own deadline before DOM server readiness is timed; a deliberately
31-second delayed build also passed all four DOM tests locally.

## Local checks — 18 November 2026

- Go package tests with the race detector: passed.
- `go vet ./...`: passed.
- React/TypeScript typecheck and production build: passed.
- Frontend unit tests: 15 passed.
- Production dependency audit (`npm audit --omit=dev`): no reported vulnerabilities at the time of the check.
- DOM interaction tests against an isolated real Go HTTP server: 4 passed.
- Local Chromium launch: blocked before any page loads because this workspace denies Chromium's local sockets. The explicit escalation request was rejected by workspace approval policy. This is not recorded as a browser pass.

The checked-in GitHub Actions workflow runs Go tests/builds on Linux and macOS, frontend unit and real-server DOM tests, Chromium integration with desktop/mobile screenshots, and a Docker build/startup/restart check. Consult the PR's latest commit checks for actual CI results; the presence of the workflow alone is not a successful run.

## Covered behavior

- SQLite restart persistence, transactional schema setup, optimistic concurrent saves, atomic encrypted credential/config updates and telemetry deduplication.
- Owner setup/login/logout, session cookies, CSRF/Origin checks, host-based throttling and encrypted key redaction.
- Two independent machines receiving commands concurrently, stable command receipts, repeated-key payload conflicts and unknown delivery without automatic retry.
- Physical REST protocol fixtures for exact payloads, authentication, Content-Length, serialized requests, bounded reads, redirects, cancellation, no mutation retries, recipe readback and history framing.
- Simulator isolation, preparation prompts, stop latch, capacity limits and history lifecycle.
- Recipe units, adaptation/efficiency scaling, hard native bounds, unknown routing, recipe revision conflicts, BeerXML/JSON exchange and deployment without starting.
- History pagination, retained samples after interruptions, concurrent edits preserved across imports, partial data, terminal-session caching and simulation labeling.
- Provider request/authentication contracts, no redirect credential forwarding, bounded errors/context, opt-in monitor cadence, saved advice and invalid AI drafts kept out of the recipe library.
- Frontend framebuffer orientation, chart/sample calculations, request transport, binary/text/SSE explorer handling, forms, recipe/history persistence and equipment calibration.

## Not physically verified by this build

No real PicoBrew was contacted, no firmware was flashed, and no actual heating/pumping command was issued during this implementation. The standalone firmware API is tested using contract fixtures; the next hardware check should begin with connecting the already-installed S/Pro firmware and inspecting discovery/status/OLED/history before operating a prepared machine.

No live paid AI API request was made. Provider request formats and errors were exercised with local HTTP test servers. Model availability and account billing depend on the user's configured provider.

Z, Zymatic and Pico C planning profiles and simulators are not evidence of stock-hardware protocol compatibility. The feature coverage document states this boundary explicitly.
