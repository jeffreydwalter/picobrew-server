# Review of the owner's Pico Pro backup

This document preserves the original device review and first 161,720-byte
installation. For the new 1.0.0 candidate, separate browser files and revised
storage/RAM budgets, see [the release notes](release-1.0.0.md).

The owner obtained a verified full serial-flash backup on 2026-11-17 using
the [macOS direct-USB workflow](macos-usb-backup.md), then supplied
`firmware-review.zip` containing extracted firmware, both boot-record
candidates, and the offline inspector's reports. This review concerns that
actual appliance. The older [filesystem screenshot](flash-layout.md) came
from a different, unidentified Pico and has different allocations.

The complete backup is 1,048,576 bytes with SHA-256:

```text
837a728400da02b9da4a7d563e1b67fb24d6a5af03c054963c4b1de60c72343f
```

The owner's two complete reads matched byte-for-byte. Only the selected
firmware and boot metadata were uploaded for review; the full flash and
saved network credentials remain private. The owner subsequently verified
API-key creation and existing-file rewrite on the appliance, followed by
standalone application installation and full readback on 2026-11-18.
Standalone boot and an HTTP 200 health response were subsequently reported
on 2026-11-18. Stock restoration remains physically untested.

## Firmware identity in the original backup

| Logical filename | Content bytes | Finding |
| --- | ---: | --- |
| `/sys/mcuimg.bin` | 14,956 | Secondary bootloader, disassembled and exercised in an instruction emulator |
| `/sys/mcuimg1.bin` | 50,504 | Recovery/setup application v0.0.3; downloads replacement firmware from `picobrew.com` |
| `/sys/mcuimg2.bin` | 162,792 | Exact byte-for-byte match to this project's original Pico application v0.1.34 |
| `/sys/mcuimg3.bin` | — | Absent from both the supplied filesystem listing and live preflight |
| `/sys/mcubootinfo.bin` | 8 per copy | Live logical read confirms `01 3d 00 20 ba dc cd ab`, selector 1 and normal magic |

SHA-256 of the three images:

```text
mcuimg.bin   67e22913fd7805a84d965928198f7d8cb795af125b81f1e0c29479a84980b975
mcuimg1.bin  26829ef97df46e6d3667afe8352fa86492f3ddf667371f2d19eafa8468862c63
mcuimg2.bin  e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702
```

The last hash and complete contents match
[`reference/original-0.1.34.bin`](../reference/original-0.1.34.bin).
This identifies the application version; it does not by itself establish
the board revision or validate the reconstructed firmware on hardware.

## Actual bootloader behavior

The secondary loader copies its internal stage from file offset `0x100`
to `0x20000000`. That stage loads the selected application at `0x20004000`
and transfers control using its initial stack pointer and reset vector.
With the normal boot-record magic `0xABCDDCBA`, byte zero selects:

| Byte zero | Application |
| ---: | --- |
| 0 | `/sys/mcuimg1.bin` |
| 1 | `/sys/mcuimg2.bin` |
| 2 | `/sys/mcuimg3.bin` |
| Other values | `/sys/mcuimg1.bin` |

The selection instructions are at relocated addresses
`0x200009ae`–`0x200009d4`. Selector 2 does **not** select `mcuimg2.bin`.
Boot-record bytes 1–3 do not select an image or supply a load address;
preserve them in any future boot-record update.

The loader also implements a trial state. Magic `0x56788765` causes it to
persist `0x12344321` and try the alternate application: selector 1 tries
image 3; other selectors try image 2. On a following boot with magic
`0x12344321`, it restores normal magic and uses the original selector.
The current standalone application has no automatic trial confirmation.
This describes a capability of the loader, not an installation plan for
the present flash layout.

The selected-image load routine begins at `0x2000085c`. A failed open or
file-info operation leads to the loop at `0x20000a38`; it does not try a
different application. This path has no explicit cryptographic integrity,
vector, or complete-read check before jumping. Consequently, an installer
must verify every byte of the intended application before normal boot.

Missing boot metadata is a separate case: the initialization path at
`0x20000926`–`0x20000996` chooses the first available image in the order
image 1, image 2, image 3. That does not provide fallback for a failed
explicitly selected image.

Nineteen cases were checked by executing the actual relocated Thumb
instructions in Unicorn, with filesystem and hardware services stubbed.
These covered normal selection, trial and rollback, missing metadata,
invalid magic, and failed selected-image operations. They are software
evidence, not a physical boot or interrupted-write test.

The public [emulation trace](../analysis/device-bootloader-emulation-results.json)
records the addressed evidence and all nineteen cases. The reproducible
audit requires an externally supplied bootloader with the exact hash above;
the private uploaded bootloader is not included in the repository:

```sh
python -m pip install -r requirements-test.txt
python tools/audit_device_bootloader.py \
  --image /path/to/extracted/mcuimg.bin \
  --output /path/to/new-bootloader-audit.json
```

The audit executes that loader's actual instructions. Its application-file
fixtures are synthetic buffers containing only the observed lengths and
initial vectors; recovery/main firmware bodies are unnecessary and are not
included in the results.

## Why the boot record must be read through the filesystem

The raw backup contains:

| Physical copy | Eight-byte file header | Eight-byte payload |
| --- | --- | --- |
| First allocation, block 79 | `08 00 00 20 05 00 4c 53` | `00 3d 00 20 ba dc cd ab` |
| Mirror, block 80 | `08 00 00 20 06 00 4c 53` | `01 3d 00 20 ba dc cd ab` |

The second header's counter is higher. That supported it being the newer
copy but was not a substitute for observing which copy the network
processor actually serves. A raw extractor that always returns the first
allocation would report factory selection even though the mirror contains
the main-application selection.

The owner ran `tools/pico_flash_preflight.py` on the physical Pico on
2026-11-17 at 23:04:38 (owner terminal timestamp). The live filesystem served
`01 3d 00 20 ba dc cd ab`, SHA-256
`59112b9c5cd615ee3427b916c2a35a0a89a40a80250197169a545fb93b8e9435`.
All three image lengths and hashes matched the reviewed backup, image 3
was absent, and storage geometry was 256 blocks of 4,096 bytes. The tool
reported both `TRANSPORT READS: complete` and `QUALIFIED`.

This resolves the ambiguity: **normal selector 1 selects image 2**, and an
installation into that existing slot requires no boot-record change. The
[sanitized physical preflight record](../analysis/device-preflight-20261117.json)
is transcribed from the owner's output; it is not an independently captured
USB trace or a replacement for the private full report. No flash or file
write was performed by that preflight.

## Recovery image limitations and the standalone reset fix

Recovery v0.0.3 is confirmed by the About-screen code at `0x20004fe8`
in the original recovery image loaded at `0x20004000`. Its normal setup
path registers with `picobrew.com`, then requests
`/API/pico/getFirmware?uid=...` over HTTP. Its destination-opening routine
at `0x20005b14` deletes both image 2 and image 3 before creating the
download destination. It is not an offline copy of the full application.

Holding the front dial during recovery startup opens Wi-Fi diagnostics,
About, and Exit. This is not an independent bootloader override or an
offline firmware-restore mechanism.

The reconstructed legacy Factory Reset action likewise deletes both
application slots and the boot record before resetting. The standalone
profile now omits it from the service menu, excludes its dispatch code,
and disables the underlying deletion function. Regression tests exercise
the real menu and deletion code in all three languages, including stale
selections and direct helper calls. The legacy comparison profile retains
its original behavior. The exact saved application and the verified backup
are the recovery sources for future USB restoration.

The rebuilt candidate containing this fix is **161,720 bytes**, with the
original 32 KiB stack and 32 KiB heap reservations intact. Its SHA-256 is
`a8fc16afce43c4b91b82374a339b86631daefe1a40f08df5f8bdd7a0d0a8d904`.
The firmware build, standalone host/API tests, native ARM storage tests,
and ARM interrupt/control checks pass. The browser sources did not change
in this fix. The owner subsequently booted this candidate and obtained
an HTTP 200 health response identifying standalone mode with readiness
and key configuration both true; see the [runtime evidence](../analysis/device-runtime-20261118.json).
This does not establish physical actuator behavior or a successful brew.

## Space available on this appliance

The recognized allocation table is FAT 1, revision 225. Including the
five reserved/filesystem blocks, 173 of 256 blocks are allocated and
83 blocks (339,968 bytes) are free. Each block is 4,096 bytes.
The [complete allocation record](../analysis/device-flash-layout.json)
contains all 17 filenames, block locations, per-copy allocations, mirror
flags and free extents, without any configuration-file contents.

| Free block range | Blocks | Bytes |
| --- | ---: | ---: |
| 81–101 | 21 | 86,016 |
| 178–213 | 36 | 147,456 |
| 230–255 | 26 | 106,496 |

TI's [SWRA657, sections 3–4](https://www.ti.com/lit/an/swra657/swra657.pdf)
describes fixed contiguous file allocations and choosing a free hole large
enough for a new file. The 161,720-byte candidate needs at least 40 blocks
after allocation rounding and metadata. The largest current hole is
36 blocks, so a simple new-file allocation for image 3 does not fit.
Total free space must not be used to promise alternate-slot installation.

The existing image 2 allocation is 48 blocks (196,608 bytes). Rewriting
that existing file can retain the allocation. The pinned UART helper does
not expose the full filesystem maximum-size/attribute record; the guarded
installer instead pins the original backup, verifies its allocation and
live application content, and accepts only the smaller 161,720-byte
candidate. TI's
[SWRU368C, chapter 13](https://www.ti.com/lit/ug/swru368c/swru368c.pdf)
states that opening an existing file for write erases its contents.
It is therefore not an atomic update. The separate guarded writer includes
an existing-file restore command using the exact stock application from
the verified backup. Normal file creation and existing-file rewrite have
now been verified on the API-key file, and the pinned application was
written and read back successfully on 2026-11-18. Stock restoration and
interrupted-write recovery remain untested. If an interrupted file cannot
be reopened, the tool stops;
it does not delete or recreate the application based on an ambiguous
file-info result. See [the guarded USB workflow](macos-usb-installation.md).

Keeping the existing allocations leaves room for the standalone local
store's full 39-block budget, including the API key: 44 blocks remain,
or 43 after allowing one block for `restart.txt`. Additional system files
may use some of that headroom. The embedded web interface consumes no
separate serial-flash files. See [the storage budget](flash-layout.md#standalone-storage-budget-for-this-layout)
for individual record sizes; that document's larger free-space figures
apply only to the earlier third-party screenshot.

## Immediate next step

Both read-only preflight and key provisioning have passed. The owner's
2026-11-17 23:49:30 run created `/local/api.key`, verified it, rewrote the
same key through existing-file mode, and verified it again. The success
marker also requires complete original-application readback and unchanged
protected files. The [sanitized result](../analysis/device-key-provisioning-20261117.json)
records the terminal evidence without any key contents or digest.

The owner subsequently completed the guarded `install` command on
2026-11-18 at 00:14:02 (owner terminal timestamp). The existing image 2
allocation was rewritten with the pinned 161,720-byte candidate. The
success marker requires complete application readback and key verification;
the printed final protected hashes match the originals. The
[sanitized installation result](../analysis/device-install-20261118.json)
records the evidence. The boot record remains selected and unchanged.

The owner then booted the application, used the network menu, and obtained
an HTTP 200 response from `/api/v1/health`. Its response identified
`"mode":"standalone"`, `"ready":true`, and `"auth_configured":true`.
The [runtime record](../analysis/device-runtime-20261118.json) separates
these observations from the remaining checks.

The next step is [browser authentication and observation](macos-usb-installation.md#browser-authentication-and-observation).
The public health response establishes an accessible running application
and a configured key; it does not verify a bearer-authenticated request,
sensor accuracy, actuator behavior or interrupted-write restoration.
