# Pico Pro backup with a SparkFun CH340C on macOS

## Hardware result reported by the owner

On 2026-11-17 the owner completed a full serial-flash backup using the existing
Mac and SparkFun USB-C Serial Basic CH340C adapter, VID:PID `1a86:7523`. The
adapter remained attached to the Pico's six-pin CC3200 serial header. The
successful path used direct USB through PyUSB/libusb instead of a macOS serial
device node.

- macOS 26.3.2 on Apple Silicon; Python 3.14; pySerial 3.5;
  PyUSB 1.3.1; libusb-package 1.0.30.1.
- CC3200 bootloader: `0.4.1.2`.
- Flash geometry: 256 blocks of 4,096 bytes, totaling **1,048,576 bytes**.
- Both complete flash reads matched byte-for-byte; both saved copies were
  checked before publishing the final backup.
- Backup SHA-256:
  `837a728400da02b9da4a7d563e1b67fb24d6a5af03c054963c4b1de60c72343f`.
- USB receive queue reported zero active transfers after cleanup, zero errors,
  and zero out-of-order callbacks. Eight shutdown cancellations were expected.
- No flash write or erase was performed. The standard `rbtl3100s.dll` loader
  was uploaded to SRAM only, with its exact contents checked before upload.

This establishes a working read path and a verified device backup. The
owner subsequently completed [standalone installation and readback](../analysis/device-install-20261118.json)
on 2026-11-18, followed by [standalone boot and a successful health request](../analysis/device-runtime-20261118.json).
Physical stock restoration remains untested.

## What the connection investigation established

The adapter enumerated as USB `1a86:7523`. With Apple's `AppleUSBCHCOM`
driver it appeared as `/dev/cu.usbserial-110`; after activation of WCH's
driver it appeared as `/dev/cu.wchusbserial110`. A missing result from
`ioreg -r -n AppleUSBCHCOM` therefore did not establish that the adapter
was disconnected. The serial-device enumeration showed which driver was
currently providing the port. Neither serial-driver path completed the
required protocol exchange reliably in this session.

The owner's loopback checks passed at requested rates of 115200 and 921600,
including before and after BREAK. A loopback uses the same adapter for
transmission and reception, so it does not independently measure the wire
baud or prove compatibility with the Pico's receiver. A repeatable initial
ACK at a requested 1024000 baud also did not establish a usable connection:
the following GET_VERSION command still received no response.

The successful direct-USB setup programs the CH340 baud register to `0xF387`,
line control to `0xC3` (8N1), flow control off, and DTR/RTS deasserted. It
requests 921600 baud; the register formula calculates approximately
923076.923 baud. That figure is a calculation, not an oscilloscope
measurement. It captures the USB interface through PyUSB/libusb and does
not depend on a `/dev/cu.*` node or require another driver change.

The successful power sequence holds BREAK before Pico power-on, receives
the initial `00 cc` ACK while BREAK is held, releases it, and immediately
sends GET_VERSION (`00 03 2f 2f`). Two owner runs received the command ACK
and a complete, checksum-valid version response identifying `0.4.1.2`.
Manual blue RESET-button attempts were not part of the successful method.
TI also documents resetting a running target while BREAK is held; the
[installation guide](macos-usb-installation.md#enter-a-fresh-programming-session)
describes that alternative when mains cannot be disconnected. The later
successful installation transcript does not identify which entry action
the owner used.

The existing six-pin attachment remained in place. With USB unplugged the
owner reported normal appliance operation; a blank display with USB
connected was observed during programming attempts. These observations
apply to this setup and are not a universal pinout or a diagnosis that
every blank Pico display is in programming mode. The earlier
[community hardware discussion](https://github.com/chiefwigms/picobrew_pico/issues/354)
is background evidence; the successful direct-USB protocol and full backup
are the stronger device-specific result.

## Tools

`tools/pico_uart_probe.py` provides the proven adapter setup and initial
CC3200 handshake. `tools/pico_flash_backup_v2.py` imports that module, loads
the audited SRAM helper, detects flash geometry, and reads the complete flash
twice. Keep the two files in the same directory.

The first backup implementation used synchronous 32-byte USB reads. It read
hundreds of kilobytes successfully but then received only 59 of 256 payload
bytes. Version 2 posts eight asynchronous 4 KiB receive transfers before any
flash query. It keeps the initial handshake, UART settings and RAM-loader
sequence unchanged. The complete backup above was obtained with version 2.

The native receive queue handles macOS endpoint-wide cancellation and frees
native buffers only after every transfer callback has completed. Packet
lengths, checksums, command restrictions and double-read verification remain
mandatory. There is no automatic protocol retransmission after a partial or
corrupt frame.

## Invocation from this checkout

Create or activate the project virtual environment and install the pinned
dependencies used for the successful backup:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip setuptools wheel
python -m pip install -r requirements-usb.txt
mkdir -p firmware/backups
```

Run these setup commands from `~/Code/pico-pro-source` when using the
owner's checkout location. This creates/reuses `~/Code/pico-pro-source/.venv`
and keeps the outputs in `~/Code/pico-pro-source/firmware/backups`.
An existing environment that completed the backup already has these
dependencies. The requirement file pins `cc3200tool` to commit
`c2332898a1ededc0bdd01ec279b23366d213f81e`. The script additionally verifies
the helper's exact 49,588-byte length and SHA-256 before opening USB.

Turn the Pico off and disconnect its mains power, leaving USB attached. From
the repository root:

```sh
PICO_DUMP="$PWD/firmware/backups/pico-pro-serialflash-$(date +%Y%m%d-%H%M%S).bin"
sudo "$PWD/.venv/bin/python" "$PWD/tools/pico_flash_backup_v2.py" --output "$PICO_DUMP"
```

Reconnect mains and turn the Pico on when the script prints `BREAK asserted`.
No reset-button action is used. The script prints progress for both complete
reads and publishes the final `.bin`, `.sha256` and `.json` files only after
verification. An incomplete read leaves `.partial` files. Existing outputs
are never overwritten.

The backup directory is ignored by Git because raw flash includes saved
network and device settings. File-level inspection should use the saved
image and disclose only the firmware/boot files needed for comparison.

## Inspect the saved backup offline

The inspector reads only a local file and requires no USB or elevated
permissions. It checks both FAT views independently, validates content
lengths and file bounds, and labels physical mirrored copies as candidates
instead of guessing which one is active. For the owner's verified backup:

```sh
"$PWD/.venv/bin/python" "$PWD/tools/pico_inspect_backup.py" \
  "$PWD/firmware/backups/pico-pro-serialflash-20261117-220719.bin" \
  --expected-sha256 837a728400da02b9da4a7d563e1b67fb24d6a5af03c054963c4b1de60c72343f \
  --output-dir "$PWD/firmware/backups/review-20261117"
```

Choose a new output directory for each run; existing results are preserved.
The output includes `inspection.txt`, `inspection.json`, and
`firmware-review.zip`. The archive includes only plausible firmware images,
valid boot records and metadata reports. Network/password/preferences file
contents and the raw flash are excluded. Inspect the archive before sharing;
this is a narrow evidence bundle, not a redacted replacement for the backup.

## Read the logical boot record before installation

The [owner's extracted firmware review](device-firmware-review.md) found
different selectors in the two raw physical boot-record copies. This
read-only check asks the live filesystem for the logical file and verifies
the three firmware hashes against this specific backup.

The owner completed this check on 2026-11-17. The live logical record was
`01 3d 00 20 ba dc cd ab`: normal magic and selector 1, which selects
`/sys/mcuimg2.bin`. All three firmware hashes matched, image 3 was absent,
and the tool reported `QUALIFIED`. The [sanitized result](../analysis/device-preflight-20261117.json)
records the supplied terminal output. The command below remains useful for
repeating the read-only check before any installation.

Keep `pico_flash_preflight.py`, `pico_flash_backup_v2.py`, and
`pico_uart_probe.py` together. Use the same existing environment and power-on
sequence as the successful backup. From this checkout:

```sh
PICO_REPORT="$PWD/firmware/backups/pico-pro-preflight-$(date +%Y%m%d-%H%M%S).json"
sudo "$PWD/.venv/bin/python" "$PWD/tools/pico_flash_preflight.py" --output "$PICO_REPORT"
```

The preflight prints `LOGICAL BOOT INFO` and `BOOT SELECTOR`, hashes the
three original images, checks image 3, and saves a private JSON report.
It permits only read-mode file opens, reads of at most 256 bytes, read-handle
closure, storage queries and the pinned SRAM-helper upload. Flash writes,
deletion, formatting, and file-write/create opens are blocked.

`TRANSPORT READS: complete` means the requested logical-file operations
completed. `QUALIFIED` additionally means the geometry, three firmware
hashes, absence of image 3, selector 1, and normal magic match the expected
device profile. A profile mismatch returns exit code 2; a transport failure
returns 1. Both save an unqualified report when possible. A successful
preflight does not install firmware or establish physical write/restore
validation.

The separate [guarded installation workflow](macos-usb-installation.md)
starts with API-key provisioning and readback, which the owner completed
successfully on 2026-11-17. Application installation and complete readback
passed on 2026-11-18, followed by standalone boot and public health access.
Authenticated browser access and status/display observation are the next
physical steps.
The probe, backup and
preflight scripts remain read-only; they do not acquire file-write support
when the writer is installed beside them.

The USB-tool regression tests cover backup guards and publication,
receive-queue ordering/cancellation, complete simulated transfers, logical
preflight, guarded file writes and offline-inspector privacy/bounds. They
use the real pinned `cc3200tool` package and synthetic USB/flash fixtures,
without opening physical USB:

```sh
make usb-test PYTHON=.venv/bin/python
```

## Primary implementation references

- [Pinned cc3200tool protocol and loader flow](https://github.com/toniebox-reverse-engineering/cc3200tool/blob/c2332898a1ededc0bdd01ec279b23366d213f81e/cc3200tool/cc.py)
- [PyUSB 1.3.1 libusb backend](https://github.com/pyusb/pyusb/blob/v1.3.1/usb/backend/libusb1.py)
- [libusb asynchronous transfers and cancellation](https://libusb.sourceforge.io/api-1.0/group__libusb__asyncio.html)
- [Pico serial-header discussion](https://github.com/chiefwigms/picobrew_pico/issues/354)
