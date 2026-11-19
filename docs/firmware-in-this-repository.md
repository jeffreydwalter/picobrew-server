# Using firmware from the community checkout

The standalone firmware project was initially imported unchanged at `firmware/` from the commit recorded in [the import provenance](import-provenance.json). Subsequent firmware changes and regenerated installation packages are maintained in that component. Its own build-output directory is also named `firmware`, so installation packages are at `firmware/firmware/standalone/` relative to the community root. This preserves the established tooling paths; package hashes identify each firmware build.

## Working directory

Run the original firmware instructions from the component directory:

```sh
cd firmware
```

From there, `Makefile`, `tools/`, `requirements-usb.txt`, `web/`, `docs/`, `firmware/standalone/`, and `firmware/backups/` have their original meanings. Existing scripts derive project paths from their own locations and do not require the old repository name.

The firmware documentation contains examples using `~/Code/pico-pro-source`. For this checkout, set the firmware project root to the component directory instead. From the community root:

```sh
PICO_ROOT="$PWD/firmware"
```

Follow the [macOS backup guide](../firmware/docs/macos-usb-backup.md), [installation guide](../firmware/docs/macos-usb-installation.md), and [key provisioning guide](../firmware/docs/standalone-provisioning.md) with that root. Keep the backup and API key paths for the actual machine being worked on. Moving source files does not rotate a brewer's key or change its installed firmware.

## Build and tests

Root shortcuts forward into the component:

```sh
make firmware-bootstrap
make firmware
make firmware-test
make firmware-usb-test
```

The pinned ARM compiler is a Linux x86-64 toolchain; see [SDK build instructions](../firmware/docs/sdk-build.md) for host requirements. Portable and USB simulation tests do not connect to a physical brewer. The separate installation commands retain their existing guards and are not invoked by the community Makefile or CI.

The imported firmware's nested GitHub workflow remains as provenance. The active root workflow, `.github/workflows/firmware.yml`, supplies the `firmware/` working directory and prefixes artifact-upload paths. It checks that the committed installation binary, source identities and browser manifest match a clean build. Original notices and historical evidence remain preserved alongside the current firmware source and package.
