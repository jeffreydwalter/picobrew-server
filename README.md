# PicoBrew Community

One checkout for the **PicoBrew Hub** fleet console and the **Pico S/Pro standalone firmware**, with the older Zymatic server preserved separately. The Hub manages shared recipes, machine connections and brew history; each compatible brewer keeps its local browser interface and its own control loop.

The Hub includes an offline community catalogue of 5,282 recipes and machine programs, attributed beer illustrations and machine artwork. Pico S/Pro firmware includes eight real starter programs for **Brew PicoPak**, and both browser interfaces edit the same local recipe slots. See the [catalogue guide](hub/docs/community-catalog.md) and [community sources](docs/community-projects.md) for provenance, model support and import details.

## Start the Hub

For this consolidation branch:

```sh
git clone --branch feature/community-monorepo https://github.com/jeffreydwalter/picobrew-server.git picobrew-community
cd picobrew-community
docker compose -f hub/compose.yaml up --build -d
docker compose -f hub/compose.yaml logs hub
```

Open **http://127.0.0.1:8080**. Use the one-time setup token from the logs to create the owner account. Add a reachable Pico S/Pro running the standalone REST firmware and enter the API key already provisioned on that brewer.

The Compose file and its build context remain in `hub/`. Running it from there still works, and the commands above use the same Compose project directory and named data volume. Existing installations should keep their current data directory, Compose project name, and encryption key. See [Hub operations](hub/docs/operations.md) for backups and deployment.

### Run without Docker

With Go 1.26 or newer and Node.js 22.12 or newer:

```sh
cd hub
npm --prefix web ci
npm --prefix web run build
mkdir -p bin
go build -trimpath -o bin/picobrew-hub ./cmd/picobrew-hub
./bin/picobrew-hub --data ./data
```

For an isolated simulator workspace, use `./bin/picobrew-hub --demo --data ./demo-data`. The [Hub guide](hub/README.md) describes the fleet, recipes, history, API explorer, optional AI assistance, and network requirements.

## Components

| Path | Purpose | Entry point |
| --- | --- | --- |
| `hub/` | Go/SQLite service and React/TypeScript fleet console | [Run and develop the Hub](hub/README.md) |
| `firmware/` | Standalone Pico S/Pro firmware, on-device web UI, USB backup/install tools, build evidence and original references | [Firmware guide](firmware/README.md) · [Using it from this checkout](docs/firmware-in-this-repository.md) |
| `legacy/zymatic-server/` | Preserved Python Zymatic cloud-protocol server and its original documentation | [Original project README](legacy/zymatic-server/README.md) |
| `docs/` | Community entry points, component boundaries and import provenance | [Community projects](docs/community-projects.md) · [Consolidation record](docs/consolidation.md) |

No Git submodule or second project checkout is required to build the Hub. Normal package and compiler downloads still apply when setting up dependencies.

## Machine support

| Machine | Current Hub support | Firmware / other component |
| --- | --- | --- |
| Pico S / Pro with standalone REST firmware | Live fleet monitoring, front-panel controls, supported programs, recipe deployment and retained-history import | Standalone firmware and local browser UI included here. Hardware evidence and outstanding validation are recorded in the firmware documentation. |
| Pico C | Recipe planning profile; no stock-protocol live adapter | The S/Pro firmware is not a Pico C installation image. |
| Pico Z | Recipe planning profile; no stock-protocol live adapter | Separate community implementations remain references. |
| Zymatic | Recipe planning profile; no live adapter in the Hub | The historical Python Zymatic server is preserved in `legacy/zymatic-server/`; it runs separately and is not integrated into the fleet console. |

The Hub simulator exercises workflows without a physical brewer. Simulation and CI results do not establish physical brewing performance. Use the [coverage matrix](hub/docs/coverage.md) and [firmware hardware record](firmware/docs/hardware-validation.md) for the current limits.

## Browse, edit and transfer recipes

Open **Community catalogue** in the Hub to search the bundled records. **Copy to library** creates an editable copy with source provenance. Ingredient recipes retain their source quantities and require reviewed physical compartment assignments. Archived PicoPak programs retain their exact machine steps and do not acquire an inferred ingredient bill or batch size.

Select library entries and preview a transfer to an idle brewer. The preview shows existing recipes, available slots, updates and skipped entries. Existing matching IDs are preserved unless **Update matching recipes** is selected. The Hub rechecks the preview, writes each accepted program and verifies readback. Transfer does not start a brew. You can also edit an installed program from the machine page or from the brewer's own web interface.

**Manual Brew** remains the interactive temperature/hold workflow. **Brew PicoPak** runs an installed automatic sequence. On first upgrade, an empty legacy catalogue is seeded once; existing recipes and later intentional edits/deletions remain intact. The firmware's **Restore starter recipes** action adds missing starters without overwriting matching IDs.

## Build and verification

Run Hub commands from `hub/` and firmware commands from `firmware/`; their existing internal paths are preserved. Root Make targets provide shortcuts:

```sh
make hub                 # Build browser assets and the Go executable
make hub-test            # Go race tests and frontend unit tests
make firmware-test       # Portable standalone firmware tests
make firmware-usb-test   # USB/protocol simulations; no physical device access
```

The root GitHub workflows run Hub verification and standalone firmware verification. The older Python workflows are retained with the legacy server as historical files rather than active release automation. Firmware ARM builds use the pinned Linux x86-64 toolchain described in [SDK build instructions](firmware/docs/sdk-build.md).

## Source and license boundaries

The original firmware import preserves the tracked source tree from commit [`63c49d0`](https://github.com/jeffreydwalter/picobrew-pico-pro-firmware/commit/63c49d01784fb502c56fa85b709472e02f923d78). Subsequent firmware changes and their regenerated installation package are tracked in this repository, with historical evidence and notices retained. Private device backups, API keys and local dependency caches are excluded.

The Hub and historical Python server have their own MIT licenses. Firmware dependencies retain LGPL and TI notices, and the firmware project does not grant a new license to PicoBrew's original binaries or embedded assets. See [License boundaries](LICENSES.md). Community catalogue and image integration must retain each source's attribution and license metadata; the root documentation's license does not relicense upstream data.
