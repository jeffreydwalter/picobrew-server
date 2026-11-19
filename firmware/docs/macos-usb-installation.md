# Guarded Pico Pro writes from macOS

The owner has verified a complete 1 MiB backup and a live logical-file
preflight using the existing SparkFun CH340C and direct USB on macOS.
The live boot record is `01 3d 00 20 ba dc cd ab`, selecting
`/sys/mcuimg2.bin`. The [device review](device-firmware-review.md) records
the firmware identities and allocation constraints.

`tools/pico_flash_install.py` provides three separate operations. **Stage 1
passed on 2026-11-17, and Stage 2 passed on 2026-11-18: the standalone
image and matching API key were verified on the owner's Pico. Standalone
boot, an HTTP 200 health response and the connected browser console also passed
on 2026-11-18 for the previous candidate. The 1.0.0 application and all five
browser files subsequently passed installation/readback verification on that
date, and the owner subsequently reported it running 1.0.0. Versioned Overview
and Settings screenshots show an upright OLED preview (confirmed fixed by the
owner), arrows and Brew history/API navigation. A new health response and
functional feature checks remain pending.**
Stock restoration remains physically untested. Successful installation does
not demonstrate recovery from a power interruption during an application
write. The [sanitized installation result](../analysis/device-install-20261118.json)
records the earlier candidate's terminal evidence. The separate
[1.0.0 upgrade result](../analysis/device-upgrade-1.0.0-20261118.json) records
the application/browser upgrade and the verified tool's success conditions.

| Command | Files it may write | Purpose |
| --- | --- | --- |
| `provision-key` | `/local/api.key` | Create/read back the required key, then rewrite/read back the same key using existing-file mode |
| `install` | `/local/api.key`, the five `/local/web/*.gz` files, `/sys/mcuimg2.bin` | Verify the key, install/read back the pinned browser bundle, then install/read back the pinned application |
| `restore` | `/sys/mcuimg2.bin` | Restore and read back the exact stock application extracted from the verified original backup |

The writer never writes the secondary bootloader, recovery image, boot
record, service pack or Wi-Fi configuration. It does not format flash,
delete files, move allocations, perform raw flash writes, or automatically
start the application. A saved preflight report cannot bypass its current
session checks.

## Required local files

This tool is deliberately restricted to the owner's reviewed device
profile. The original backup must be exactly 1,048,576 bytes with SHA-256:

```text
837a728400da02b9da4a7d563e1b67fb24d6a5af03c054963c4b1de60c72343f
```

The existing backup path is:

```text
~/Code/pico-pro-source/firmware/backups/pico-pro-serialflash-20261117-220719.bin
```

Every mutation command verifies that entire local backup before opening
USB. It extracts and verifies the original image 2 locally; the backup
and its Wi-Fi credentials are never uploaded anywhere by this tool.
Keep the backup and its original checksum/metadata files unchanged.

Use the existing repository environment. If recreating it is necessary:

```sh
PICO_ROOT="$HOME/Code/pico-pro-source"
cd "$PICO_ROOT"
python3 -m venv .venv
"$PICO_ROOT/.venv/bin/python" -m pip install -r requirements-usb.txt
mkdir -p "$PICO_ROOT/firmware/backups"
```

The writer imports the repository's probe, queued-backup, preflight and
offline-inspection modules. Run it from the checkout; copying only the
writer into Downloads is insufficient. The proven transport sequence is retained; the initial prompt now describes
both power-on and reset-button entry.

## Enter a fresh programming session

The tool holds BREAK while waiting for the CC3200 to restart. It neither
resets the board automatically nor resumes the RAM helper left by a
previous command. TI's [SWRU577, section 5.3.1](https://www.ti.com/lit/ug/swru577/swru577.pdf)
explicitly permits powering on the device **or resetting it if already
running**, after BREAK is asserted.

Two entry procedures are available:

- **Power-on procedure:** with the Pico's mains disconnected and USB
  connected, start the command. On the initial `BREAK asserted` message,
  reconnect mains and turn the Pico on. This procedure produced the
  verified backup, preflight and key-provisioning results.
- **Reset-button alternative:** leave mains and USB connected, start the
  command, and press and release the board's blue RESET button once when
  the initial `BREAK asserted` message appears. This follows TI's reset
  sequence; the successful installation transcript does not explicitly
  identify the owner's reset action, so it is not recorded as separate
  physical confirmation of this entry method.

At the **initial** `BREAK asserted` prompt, either power on the Pico or press
and release its blue RESET button once. Pressing the reset button does not
require unplugging mains. Do not press Return. Once the handshake begins,
do not reset again or disconnect power/USB until the command exits. The later
`APPS HANDSHAKE` is handled by the tool.

## Stage 1: provision and verify the API key

The owner completed this stage in the run beginning at 23:49:30 on
2026-11-17. The [sanitized physical result](../analysis/device-key-provisioning-20261117.json)
records the success marker and protected hashes. Keep the matching host
key; proceed to Stage 2 using the same environment and tested writer.
The procedure remains below for reference.

Update the existing feature-branch checkout and create the local key only
if it does not already exist:

```sh
PICO_ROOT="$HOME/Code/pico-pro-source"
cd "$PICO_ROOT"
git pull --ff-only origin feature/standalone-rest-api

if [ ! -e "$PICO_ROOT/private/api.key" ]; then
    "$PICO_ROOT/.venv/bin/python" \
        "$PICO_ROOT/tools/provision_api_key.py" \
        --output "$PICO_ROOT/private/api.key"
fi
```

The key generator writes exactly 64 lowercase hexadecimal characters plus
LF with mode `0600`. The writer requires that format in a private regular
file and never prints its contents, hash or file-access token. An existing
matching device key is reused and verified; a different key causes an
abort. Do not generate a new key to work around a mismatch.

Choose either entry procedure above and run:

```sh
PICO_ROOT="$HOME/Code/pico-pro-source"
PICO_BACKUP="$PICO_ROOT/firmware/backups/pico-pro-serialflash-20261117-220719.bin"
PICO_REPORT="$PICO_ROOT/firmware/backups/pico-pro-key-$(date +%Y%m%d-%H%M%S).json"

sudo "$PICO_ROOT/.venv/bin/python" \
    "$PICO_ROOT/tools/pico_flash_install.py" provision-key \
    --backup "$PICO_BACKUP" \
    --key-file "$PICO_ROOT/private/api.key" \
    --output "$PICO_REPORT"
```

On the initial **`BREAK asserted`**, perform the chosen power-on or
reset-button action. Do not wait for a Return prompt.

The tool checks the live protected images, original application, boot
record, flash geometry and allocation metadata. If the key is absent,
it also requires unambiguous absence from validated FAT/name metadata
before creating it. A logical “absent” flag by itself is insufficient.
It then verifies the key, reopens it for existing-file write, rewrites
the identical bytes and verifies them again. Before reporting success,
it rereads the original application and protected state.

Expected final marker:

```text
SUCCESS: API key provisioning and existing-file write verification completed
```

Return this run's terminal output for review before proceeding to Stage 2.
The original application remains selected throughout Stage 1. Each subsequent
command needs a fresh programming session using the procedure above.

## Stage 2: install the pinned standalone candidate

The previous candidate was written and read back in the run beginning at
00:14:02 on 2026-11-18. Its first boot, health API and connected browser were
subsequently observed. The owner then completed this **1.0.0** upgrade in the
run beginning at 09:36:07 on the same date (console timestamp as reported).
The [upgrade record](../analysis/device-upgrade-1.0.0-20261118.json) records
application/browser readback, key verification and protected-file postchecks.
The installer did not start the application; the owner subsequently reported
running 1.0.0. The commands below now install the **1.1.0 catalogue and recovery update**;
the installed 1.0.0 image is an approved upgrade source.

Proceed with the existing provisioned key. Do not rerun key provisioning after
installing standalone firmware; that separate stage intentionally requires the
original application. The exact new image size/hash and matching web manifest
are recorded in `firmware/standalone/firmware-report.json` and pinned in the
installer. The previously installed image's identity remains separately
allowlisted as an upgrade source.

The checked-out `firmware/web/manifest.json` and all five gzip payloads are
required alongside the application. They are validated before USB is opened.
The default bundle path is resolved from the repository, so the install command
below does not need an extra browser-files argument. The installer verifies
live FAT/free-hole constraints, key identity, all browser file readbacks, then
the application and protected-file postchecks. It leaves local recipes,
histories, preferences and forwarding configuration in place.

Unknown or partial application content requires the explicit restore path;
normal installation does not overwrite an unknown image. A different build,
even from this branch, requires a reviewed tool/profile update.

Update this branch first:

```sh
PICO_ROOT="$HOME/Code/pico-pro-source"
cd "$PICO_ROOT"
git pull --ff-only origin feature/standalone-rest-api
"$PICO_ROOT/.venv/bin/python" -m pip install -r requirements-usb.txt
```

With either programming-entry procedure above:

```sh
PICO_ROOT="$HOME/Code/pico-pro-source"
PICO_REPORT="$PICO_ROOT/firmware/backups/pico-pro-install-$(date +%Y%m%d-%H%M%S).json"

sudo "$PICO_ROOT/.venv/bin/python" \
    "$PICO_ROOT/tools/pico_flash_install.py" install \
    --backup "$PICO_ROOT/firmware/backups/pico-pro-serialflash-20261117-220719.bin" \
    --key-file "$PICO_ROOT/private/api.key" \
    --firmware "$PICO_ROOT/firmware/standalone/pico-s-pro-standalone.bin" \
    --output "$PICO_REPORT"
```

The writer first proves existing-file writing on the matching key in this
session, then installs and verifies every browser file. Only then does it open
the existing image 2 with mode `0x1000`, retaining its allocation and attributes,
and write sequentially. Opening for write
**erases the file's contents immediately**. It is not an atomic update.

The existing application reserves 48 nonmirrored 4 KiB blocks. The writer
additionally caps application payloads at 188,416 bytes, conservatively below
the capacity derived from that allocation and the SDK's size-rounding rules.
This is a derived guard, not a measured `AllocatedLen` value. The 165,656-byte
1.1.0 image passes it; no application-file recreation or relocation is used.
Every write requires an acknowledged command and successful loader status;
the tool does not retry an ambiguous write chunk.

After closing, it rereads every application byte, compares with the local
candidate, verifies the key, and rechecks the protected state. If the
candidate was already installed, it skips the application rewrite while
still verifying the key and final state. Expected marker:

```text
SUCCESS: standalone firmware, browser files and API key verified
```

The tool leaves the Pico in the programming session. First standalone
startup, display/controls, network access and output behavior are separate
physical validation steps described in [hardware validation](hardware-validation.md).
Keep USB connected and review the installation result before attempting
that first startup. During the application write, keep mains and USB
connected until the command has completed.

## First standalone startup

The owner reported successful startup of the earlier 161,720-byte standalone
candidate on 2026-11-18 and supplied an HTTP 200 health response with standalone
mode, readiness and valid key configuration. The
[runtime evidence](../analysis/device-runtime-20261118.json) records that result.
The [1.0.0 candidate](release-1.0.0.md) subsequently passed installation and
readback verification, and the owner reported it running 1.0.0. Overview and
Settings screenshots now show that version, an upright OLED preview and visible
arrows/navigation. Its health response and functional feature checks remain pending.
The exact reset/cable sequence used for the earlier startup was not identified
in the report; the recommended procedure follows.

After `SUCCESS: standalone firmware, browser files and API key verified` and return to
the shell, the RAM helper is still the last program the installer started.
USB cleanup releases BREAK and returns the adapter to the macOS driver;
it does not start the installed application or verify the header's boot
signals. A normal boot with USB still attached has not been established
for this six-pin arrangement.

For the first startup, leave the Pico's mains connected, disconnect only
the USB-C cable from the SparkFun adapter, leave the six-pin header in
place, and press and release the board's blue RESET button once. This
uses the owner's earlier observation that normal operation returned with
USB disconnected. Do not infer that the later successful boot validated
every possible attached-adapter or reset configuration.

Observe the display before attempting any brewing or maintenance program.
Record a blank screen, reset loop or error exactly as shown. Firmware
startup, network access and physical output behavior remain separate
checks in [hardware validation](hardware-validation.md).

The expected code path shows the Pico splash, attempts the saved Wi-Fi
connection, and reaches the regular menu. Standalone 1.1.0 shows `v1.1.0`,
using the shared version in `include/picobrew/version.h`. The earlier
standalone candidate retained the stock `v0.1.34` label. If an old-session
resume prompt appears, choose Abort for this initial observation.

On the front panel, open **Settings > Network > Status** and read the IP
address. If a Wi-Fi connection is needed, use **Settings > Network >
Connect to WiFi**. The firmware does not automatically configure a new
fallback access point. From a browser on the same network, open:

```text
http://<pico-ip>/api/v1/health
```

After initialization, the 1.1.0 read-only public endpoint should include
`"firmware_version":"1.1.0"`, `"api_version":1`, `"mode":"standalone"`,
`"ready":true`, and `"auth_configured":true`. The response's `uptime_ms`
varies. This is the running-firmware identity check; it requires no API key.
The browser console is at `http://<pico-ip>/`. Record the actual response
before starting a program.

### Network changes since 1.0.0

With the earlier candidate, the owner changed the saved SSID and observed
the new SSID label with an address outside the expected subnet. A reboot
resolved the address mismatch, followed by the successful health request.
That report does not establish the exact internal state that caused the
mismatch.

Standalone 1.0.0 includes the reconnect fix in
`platform/energia_network.cpp`: it disconnects the old association, clears
the cached connection and address state, enables DHCP explicitly, and
requests the selected network. Connection success requires a fresh IPv4
DHCP event, and old-network HTTP sockets are closed before reconnecting.
The intended behavior is to obtain the new network's address without a
manual reboot. The native Wi-Fi tests cover this flow; its behavior on the
owner's Pico still needs physical verification with 1.0.0.

After changing networks, wait for the connection attempt to complete,
check **Settings > Network > Status**, and request `/api/v1/health` at the
new address. Confirm `"firmware_version":"1.1.0"` after this upgrade and compare the address
with the router's client information. The displayed saved SSID alone does
not establish that the Pico joined that network or acquired a valid lease.

## Browser authentication and observation

For reference, the owner's earlier 161,720-byte candidate returned this
health response before firmware-version reporting was added:

```json
{"api_version":1,"mode":"standalone","ready":true,"auth_configured":true,"uptime_ms":132543}
```

The public health endpoint alone does not verify a bearer-authenticated
request. The owner's earlier connected-browser screenshot showed protected
status and OLED readings on the unversioned candidate. The subsequent 1.0.0
Overview and Settings screenshots show the new version, sensor readouts, upright
OLED and visible arrow/navigation controls. The owner confirmed the OLED fix;
control and feature operation remain untested. To repeat the browser check,
open `http://<pico-ip>/` in a browser. The initial
**Connect to your brewer.** dialog has a **Device API key** field and
**Connect** button. On the owner's Mac, copy the existing key directly
to the clipboard without printing it:

```sh
tr -d '\r\n' < "$HOME/Code/pico-pro-source/private/api.key" | pbcopy
```

Paste into **Device API key** and click **Connect**. Successful protected
reads should change the indicator to **Connected** and populate Overview
with temperatures, target, step time, filter state and the live OLED.
Compare the OLED image with the physical screen and record the actual
readings before starting a program. The initial login and polling use
GET requests; they do not start a brew or maintenance cycle. The key is
stored in that browser tab's session; Disconnect removes it from the tab.

The earlier candidate's authenticated reporting and the 1.0.0 browser
screenshots are separate observations. The latter verify displayed version,
readouts and the owner's confirmed OLED orientation fix; they do not establish
arrow-button operation, history/API requests, forwarding, network changes or
physical output acceptance. Sensor calibration, interlocks and controlled
thermal operation also remain unverified; a returned temperature alone does
not establish its calibration.

## Restore the original application

The stock restore source is the verified full backup, not a freshly
downloaded image or the cloud-dependent recovery application. The writer
extracts exactly 162,792 original bytes with SHA-256
`e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702`.

Start a fresh programming session with either entry procedure above:

```sh
PICO_ROOT="$HOME/Code/pico-pro-source"
PICO_REPORT="$PICO_ROOT/firmware/backups/pico-pro-restore-$(date +%Y%m%d-%H%M%S).json"

sudo "$PICO_ROOT/.venv/bin/python" \
    "$PICO_ROOT/tools/pico_flash_install.py" restore \
    --backup "$PICO_ROOT/firmware/backups/pico-pro-serialflash-20261117-220719.bin" \
    --output "$PICO_REPORT"
```

Restore checks the protected images, logical boot record and named image 2
allocation. It does not require the damaged application's contents to be
readable or hash-valid. It attempts only an existing-file write, then
requires complete stock readback and protected-state checks. It leaves
the API key and all other files alone. Expected marker:

```text
SUCCESS: original firmware restored and read back exactly
```

If the filesystem cannot reopen the existing image 2, restore stops.
The loader's coarse file-info “absent” result cannot distinguish every
corrupt/interrupted state; it does not authorize deletion, recreation,
formatting or raw writes. **This bounded restore command has not yet been
tested on the physical Pico, and cannot guarantee recovery from every
interrupted write.** Do not use Factory Reset as an offline restore method.

## Reports and failures

Each command requires a new absolute `--output` path. It reserves a
private report and an append-only journal before opening USB, records
intent before destructive opens, and records completed operations after
acknowledgment/status checks. The report and adjacent `.journal.jsonl`
contain no key contents, key digest or file-access token. Failed runs
retain their evidence; files are never silently overwritten.

On any transport failure, unexpected response, different protected hash
or readback mismatch, the tool stops without automatically booting or
resending an uncertain write. Keep the terminal output and report. After
an application-write failure, remain in the programming workflow until
the application has been successfully verified or restored.

The original backup, probe and preflight remain usable independently.
`make usb-test` runs synthetic device/protocol tests without physical USB;
those tests include immediate erase on existing-write open and failures
after a command may already have taken effect.

### APPS handshake diagnostics

The owner's first `provision-key` attempt on 2026-11-17 at 23:33:53
verified the local backup and initial ROM version, then stopped at the
second `APPS INITIAL ACK` handshake. It did not reach helper upload or
any filesystem operation. The [sanitized attempt record](../analysis/device-key-attempt-20261117.json)
preserves that distinction. A subsequent run at 23:49:30 passed both
handshakes, key provisioning and all final checks. That success does not
establish the earlier handshake failure's physical cause.

Revision 1 replaced the underlying error with `ProbeError: operation
failed; device payload omitted`, which concealed whether the device was
silent, returned NACK, or reached the unexpected-byte limit. That output
does not establish a baud, wiring or timing cause.

Revision 2 keeps the same inherited BREAK sequence and adds safe failure
diagnostics. A timeout with no bytes, for example, is reported as:

```text
STOPPED during RAM loader setup: APPS INITIAL ACK: timed out; received 0 bytes; wait limit 5s; elapsed ...s
No Pico flash or file write was attempted
```

This is an example of the new reporting, not a recovered explanation of
the earlier failure. The report's `error.handshake` field distinguishes
timeout, NACK and the 1,024-byte bound without storing received payload.
It also records `helper_uploaded_bytes`, `helper_executed` and
`persistent_write_attempted`. No automatic retry or connection-setting
change was added. Use a new report path and the same host key for a fresh
attempt; the prior report and journal remain evidence.

## Protocol references

The wire framing and helper come from the pinned
[`cc3200tool` revision](https://github.com/toniebox-reverse-engineering/cc3200tool/blob/c2332898a1ededc0bdd01ec279b23366d213f81e/cc3200tool/cc.py).
The writer does not call its generic `write_file()` helper, which deletes
and recreates existing files.
TI's [SWRU368C, chapter 13](https://www.ti.com/lit/ug/swru368c/swru368c.pdf)
documents whole-file erase on open-for-write and fixed allocation.
The [CC3100/CC3200 filesystem API](https://software-dl.ti.com/ecs/cc31xx/APIs/public/cc31xx_simplelink/latest/html/group___file_system.html)
defines distinct read, existing-write and creation modes. See
[SWRA657](https://www.ti.com/lit/an/swra657/swra657.pdf) for allocation and
serial-flash power-interruption limitations.
