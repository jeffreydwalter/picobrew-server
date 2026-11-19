# Standalone firmware 1.1.0

`pico-s-pro-standalone.bin` is the raw linked application image for the Pico
S/Pro reconstruction, starting at `0x20004000`. It contains the REST API,
persistent brew history, optional telemetry forwarding and browser-file
streaming. The matching five compressed browser files are packaged separately
in [`../web/`](../web/) and are required for the full browser console.

The image is **165,656 bytes**. Its SHA-256 is:

```text
a5e7fc51841ad0444e6b3a731af48dcdac214cce65c04b54b373001b543f6df4
```

`firmware-report.json` records the source hashes, vectors, native link and RAM
checks, as well as the matching browser manifest. The original **32 KiB stack
and 32 KiB heap reservations remain intact**, with 656 bytes of unallocated
SRAM after the linked image and reservations. History and enabled telemetry
use reusable heap payloads totalling 4,704 bytes, plus allocator overhead,
within that heap. This is a link budget, not measured peak runtime usage.

The previous 161,720-byte standalone candidate was installed and booted on the
owner's Pico Pro, and its connected browser was observed. **The 1.0.0 application
and its browser bundle passed installation and readback verification on
2026-11-18.** The [upgrade result](../../analysis/device-upgrade-1.0.0-20261118.json)
records that the installer did not start the application. The owner subsequently
reported running 1.0.0 and supplied versioned Overview/Settings screenshots,
confirming the upright OLED preview. Arrows and Brew history/API navigation
are visible; their operation and a new health response remain unverified.
The packaged **1.1.0** release installs starter PicoPak programs into an empty
catalogue, preserves native heating-only steps, adds explicit starter restoration,
and binds interrupted-brew recovery to the unchanged program. It has not yet
been installed on the owner's machine.
This image is not a complete serial-flash
backup. Physical brew, interlock and restore validation remain outstanding;
see [hardware acceptance](../../docs/hardware-validation.md).

Use the [guarded USB installer](../../docs/macos-usb-installation.md). It accepts
the known unversioned candidate and releases 1.0.0/1.0.1 as upgrade sources, verifies the existing key,
installs and reads back the matching browser bundle, then rewrites and verifies
the existing image2 allocation. It retains all other firmware images and boot
metadata. No API key is included in these artifacts.

Rebuild with `make standalone`. The matching debug ELF, linker map and legacy
update-response envelope are generated in `build/standalone/firmware/`; the
CI workflow also uploads the raw image, ELF, map and report. The standalone
firmware does not implement an OTA receiver or an HTTP flashing endpoint.

See the [1.1.0 release notes](../../docs/release-1.1.0.md),
[API guide](../../docs/standalone-api.md), and
[optional telemetry guide](../../docs/telemetry.md).
