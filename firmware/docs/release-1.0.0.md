# Standalone firmware 1.0.0

This page records the 1.0.0 release and its hardware observations. The original
package is preserved in [commit 0a7d499](https://github.com/jeffreydwalter/picobrew-pico-pro-firmware/tree/0a7d4991bcdb29871534e08fd73a784637bae79b/firmware).
The current Settings layout patch is described in [1.0.1](release-1.0.1.md).

This is the first major standalone release after the recovered stock 0.1.34
firmware and the initial unversioned standalone candidate. REST stays at
`/api/v1`; `api_version` remains 1. The firmware version is defined in
`include/picobrew/version.h` and appears in the OLED startup label, web footer,
health/discovery responses, and build report.

The previous 161,720-byte candidate was installed and booted on the owner's
Pico Pro. Its connected browser and sensor/display readings were observed.
**The 1.0.0 application and its five browser files passed installation and
readback verification on the owner's Pico on 2026-11-18.** The
[sanitized upgrade result](../analysis/device-upgrade-1.0.0-20261118.json)
records the verified key and unchanged protected files. The installer did not
start the application; the owner subsequently reported running 1.0.0 and
supplied Overview and Settings screenshots showing that version. The owner
confirmed the corrected upright OLED preview; arrows and Brew history/API
navigation are visible. A new health response and functional feature checks
remain pending.
Software tests cover the specific contracts described below; a complete brew,
physical interlocks, RF timing, and interrupted application restoration remain
hardware work.

## Changes

- Correct the web OLED preview's fixed 180-degree panel mapping. The physical
  OLED and raw framebuffer API retain their existing behavior.
- Replace minus/plus dial controls with up/down arrows, preserving encoder
  deltas -1/+1. Wrap long device identifiers inside the sidebar.
- Disconnect and clear the old Wi-Fi association/address before joining a
  changed network. Report success only after a fresh IPv4 DHCP event. Close
  old-network sockets and invalidate the telemetry DNS cache.
- Add an interactive API explorer covering all 16 documented operations,
  their parameters, request examples, and response status/body.
- Record brew histories on the Pico, with multiple retained sessions,
  temperature graphs, phase/step changes, sample inspection and JSON/CSV export.
- Add optional external telemetry forwarding by IPv4 address or hostname.
  It defaults off; local recording and brewing operate independently of it.
- Remove the Recent activity screen and its automatic button/event polling.
  The diagnostic event API remains available in the API explorer.
- Serve the browser's compressed files directly from serial flash, freeing
  application SRAM. The guarded USB installer verifies the complete matching
  browser bundle before opening the application file for write.

AI integration and deletion of the old recovery image are deferred. All
existing firmware images, boot metadata, service pack and identity files remain
in place. There is no new OTA firmware route or AI actuation path.

## Retained brew history

The Pico records recipe and manual brewing independently of the browser and
Wi-Fi association. Settings, rinse and other maintenance actions do not create
new brew histories. Each retained recording contains elapsed times, wort,
thermoblock and target temperatures, step time/index, phase, controller status,
compartment, device state, and readiness/stop/filter/gap flags.

A pool of 24 data chunks stores **4,224 20-byte samples**. The regular interval
is 15 seconds; meaningful state changes also produce samples. The pool permits
about **17.6 hours of periodic samples** before extra transitions and partly
filled final chunks. Practical estimates are three or four 3–4-hour brews, or
two 6-hour brews. These are estimates, not guaranteed brew counts. The catalog
holds at most eight entries and all entries share the same sample pool.

The oldest completed or interrupted brew is removed only as another entry or
chunk is needed. Export never deletes the local recording. A long active brew
can eventually consume the whole pool; its `truncated` flag is then set and
its beginning is retained. The UI exposes incomplete recordings and gaps.

Closed chunks carry CRCs and session/ordinal identities. The live tail is
checkpointed about once a minute with alternating tail and catalog records.
After restart, the last committed prefix of an unfinished brew is retained as
`interrupted`; readings that were only in RAM can be lost. No wall-clock start
time is invented. JSON/CSV exports retain Fahrenheit values; the graph follows
the selected display unit.

## Storage budget

This uses the **owner's actual 1 MiB backup**, whose initial free allocation
was 83 blocks, not the earlier unidentified Pico board screenshot. Every block
is 4 KiB. Existing firmware allocations are retained.

| Reserved use of the original free area | Blocks | KiB |
| --- | ---: | ---: |
| All 16 recipes, catalog, preferences, session counter and API key | 39 | 156 |
| Restart record | 1 | 4 |
| Five compressed browser files, including graphing and API explorer | 8 | 32 |
| History data, two catalogs and two tail checkpoints | 28 | 112 |
| Optional forwarding configuration, two banks | 2 | 8 |
| **Total additional maximum** | **78** | **312** |
| **Remaining headroom against the original layout** | **5** | **20** |

This budgets every bank, including records that may not exist yet. Additional
system/network files may consume headroom, so it is not a claim of currently
measured live free space. The resulting known file count is 92, within the
filesystem's 128-file limit. Small separate history chunks avoid requiring a
single large contiguous free extent. The installer checks the live holes and
file slots for new browser files before any installation mutation.

`firmware/web/manifest.json` gives exact compressed sizes, hashes, creation
maxima and metadata-inclusive allocations. Browser files are unmirrored; the
installer verifies them before changing the application. History/settings use
their own alternating commit records rather than filesystem mirroring.

## RAM budget

Executable code and constants share the CC3200's SRAM with working data. Free
serial flash does not increase this RAM. Browser payloads have therefore moved
out of the application image and are streamed through a 512-byte portion of
the existing HTTP buffer.

The original **32 KiB stack and 32 KiB heap reservations are retained**. History
uses one lazy 3,552-byte heap buffer; optional forwarding uses one lazy
1,152-byte heap buffer. Their combined payload is **4,704 bytes**, plus allocator
overhead, within that heap. They are reused without allocation per sample or
upload. Failed allocation is reported without changing brewing controls or
replacing previously committed history. Other SDK/application allocations
still use the same heap; the reservation is not all free memory.

See the packaged [firmware report](../firmware/standalone/firmware-report.json)
for the exact linked image size, static RAM, remaining unallocated SRAM,
source hashes and matching browser manifest. Build fit does not establish
physical peak stack/heap usage or timing.

## Optional external logging

Set an HTTP receiver's IP/hostname and port in Settings. The Pico sends a
versioned JSON status snapshot to `POST /api/v1/telemetry` on that receiver,
normally every 15 seconds. The default port is 8080. No device API key or Wi-Fi
password is forwarded. Delivery is best effort, with bounded nonblocking socket
progress, a five-second deadline and failure backoff. DNS lookup is synchronous
in the SDK and runs only while the appliance is idle. A network change
invalidates cached DNS; forwarding can wait until idle to resolve again.

This does not upload a historical backlog or stop a brew on receiver failure.
The Settings page exposes delivery counters/status. An optional standard-library
SQLite receiver is included; see [telemetry setup](telemetry.md). It is not
required for brewing, the browser console or retained local histories.

## Installation and verification

Use the [guarded macOS installation procedure](macos-usb-installation.md) from
the updated checkout. An existing matching API key is reused. The installer
accepts only the known original image, the previously installed standalone
candidate, or this exact target. It pins the original backup, protected files,
application allocation and complete browser bundle.

The installer writes/verifies browser files first, then the application; a
failure in browser provisioning leaves the previous application's embedded UI
available. Application rewrite itself is not atomic. No reset, boot selector
change, format, delete, or automatic recovery write is performed.

Software gates include bounded HTTP/file streaming, native Wi-Fi/DHCP callbacks,
API parsing and persistent stores, history eviction and torn-write recovery,
allocation failures, telemetry deadlines/offline behavior, the optional receiver,
USB protocol/installer guards, ARM interrupt exclusion and native filesystem
failure recovery. Browser workflows run in GitHub CI because the local restricted
environment cannot launch Chromium. Check the CI run for the exact published
commit; source changes alone do not imply a successful run.
