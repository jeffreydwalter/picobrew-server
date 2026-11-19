# Observed Pico serial-flash layout

The standalone budget below describes the earlier embedded-UI design. The
current 1.0.0 budget uses the owner's actual backup and includes separate web
files, retained history and optional forwarding; see [release 1.0.0](release-1.0.0.md#storage-budget).

## Evidence and limits

The owner's actual verified backup has since been reviewed. See
[device-firmware-review.md](device-firmware-review.md) for its confirmed
v0.1.34 application, actual bootloader behavior, mirrored boot-record
ambiguity, and 83 free blocks split into three extents. The figures below
remain a transcription of the earlier third-party screenshot; they must
not be used as the owner's current installation layout.

On 2026-11-17 the owner supplied a screenshot of a third party's Pico
filesystem listing. The sender did not know whether the appliance was a
Pico Pro or Pico C. The screenshot is a directory/allocation listing, not a
flash dump, and it is not a measurement of the owner's Pro.

The table below transcribes the screenshot. Its output matches
[ALLTERCO's cc3200tool](https://github.com/ALLTERCO/cc3200tool/blob/master/cc3200tool/cc.py).
The layout is consistent with the CC3200 storage and update paths recovered
in this repository, but filenames and allocation sizes alone do not establish
the appliance model, installed application version, or selected boot image.

The community server also distinguishes these targets: its
[firmware mapping](https://github.com/chiefwigms/picobrew_pico/blob/36acae3003845b72568ef5f7d6cf168acb6f9b69/app/main/firmware.py)
selects separate `pico` and `pico_c` firmware, and its
[update route](https://github.com/chiefwigms/picobrew_pico/blob/36acae3003845b72568ef5f7d6cf168acb6f9b69/app/main/routes_pico_api.py)
explicitly distinguishes C firmware from S/Pro firmware. A shared filesystem
layout is not evidence that the Pro build can run on a C.

## Observed capacity

| Item | Blocks | Bytes | Binary units |
| --- | ---: | ---: | ---: |
| Serial flash | 256 | 1,048,576 | 1 MiB |
| Used, including filesystem reservation | 110 | 450,560 | 440 KiB |
| Free | 146 | 598,016 | 584 KiB |
| Free extent, block 17 | 1 | 4,096 | 4 KiB |
| Free extent, blocks 111–255 | 145 | 593,920 | 580 KiB |

Every block is 4,096 bytes. These are external serial-flash figures, not
available MCU RAM. The firmware's RAM, stack and heap budget is checked
separately by the native build.

### Directory transcription

Paths retain the screenshot's spelling, including the missing leading slash
on `sys/uid.txt`. The `size` column is the allocation per copy; the `total`
column includes a second copy for files marked fail-safe. Neither column is
an exact content length.

| Index | Start block | Size, blocks | Fail-safe | Flags | Total, blocks | Name |
| --- | ---: | ---: | --- | --- | ---: | --- |
| N/A | 0 | 5 | N/A | N/A | 5 | FATFS |
| 0 | 5 | 4 | yes | 0x0 | 8 | `/sys/mcuimg.bin` |
| 4 | 47 | 1 | yes | 0x8 | 2 | `/sys/mcubootinfo.bin` |
| 6 | 18 | 16 | no | 0xc | 16 | `/sys/mcuimg1.bin` |
| 7 | 34 | 5 | yes | 0x8 | 10 | `/tmp/phy.cal` |
| 8 | 53 | 1 | no | 0xc | 1 | `sys/uid.txt` |
| 9 | 49 | 1 | yes | 0x8 | 2 | `/sys/mdns.cfg` |
| 10 | 51 | 1 | yes | 0x8 | 2 | `/sys/mode.cfg` |
| 11 | 54 | 1 | no | 0xc | 1 | `ssid.txt` |
| 12 | 55 | 1 | yes | 0x8 | 2 | `/sys/stacfg.ini` |
| 13 | 57 | 2 | yes | 0x8 | 4 | `/sys/pref.net` |
| 14 | 61 | 1 | yes | 0x8 | 2 | `/sys/ipcfg.ini` |
| 15 | 44 | 1 | no | 0xc | 1 | `wifisecurity.txt` |
| 16 | 45 | 1 | no | 0xc | 1 | `wifipass.txt` |
| 17 | 63 | 48 | no | 0xc | 48 | `/sys/mcuimg2.bin` |
| 18 | 46 | 1 | no | 0xc | 1 | `userpref.txt` |
| 19 | 13 | 1 | no | 0xc | 1 | `ss.txt` |
| 20 | 14 | 1 | yes | 0x8 | 2 | `/tmp/table.arp` |
| 21 | 16 | 1 | no | 0xc | 1 | `rinse.txt` |

There are 18 live named files. Gaps in the file indices are not additional
live files. The five-block FATFS row accounts for filesystem/reserved blocks.

TI documents allocation in whole flash blocks, fixed maximum sizes chosen
at file creation, and doubled allocation for fail-safe mirroring. See
[SWRU368C, chapter 13](https://www.ti.com/lit/ug/swru368c/swru368c.pdf) and
[SWRA657, sections 3–5](https://www.ti.com/lit/an/swra657/swra657.pdf).
The screenshot's raw flags are preserved as evidence; they are not a complete
decoding of every SimpleLink attribute.

## Boot images: supported interpretation

TI's [CC3200 application bootloader example](https://software-dl.ti.com/ecs/CC3200SDK/1_5_0/exports/cc3200-sdk/example/application_bootloader/README.html)
uses `/sys/mcuimg.bin` as a secondary bootloader and `mcuimg1.bin` as the
factory/default application. `mcuimg2.bin` and `mcuimg3.bin` are updateable
application slots. The [TI OTA application note](https://software-dl.ti.com/ecs/CC3200SDK/1_5_0/exports/cc3200-sdk/docs/CC3200_OTA_Update_Application_Note.pdf)
describes the boot-selection metadata.

The recovered Pico code in [update.c](../src/network/update.c) independently
supports the following interpretation:

| File | Role supported by the code and TI convention | What remains unknown |
| --- | --- | --- |
| `/sys/mcuimg.bin` | Likely the secondary bootloader; 16 KiB per copy, 32 KiB total | Its exact code and selection/fallback behavior |
| `/sys/mcuimg1.bin` | Factory/recovery image; the recovered updater allocates 0xf000 bytes for this file | Its contents, version and recovery capabilities |
| `/sys/mcuimg2.bin` | Main downloaded application; the recovered updater allocates 0x2f800 bytes | Its actual length, version/hash and whether it is selected |
| `/sys/mcubootinfo.bin` | Boot-selection metadata read/written as an eight-byte record by the recovered platform adapter | The record from this appliance and interpretation by its actual bootloader |

The 64 KiB allocation for `mcuimg1.bin` is too small to contain this
repository's 162,792-byte original full application. It must not be described
as another identical copy of that application. A smaller recovery/updater
image is consistent with the recovered code, but its behavior needs its dump.

`mcuimg3.bin` is absent from the listing. The recovered updater explicitly
removes that file when replacing `mcuimg2.bin`; its absence alone does not
indicate a fault. The standalone profile disables that cloud update path.

Two diagnostic fields must not be treated as application identity:

- `selected FAT revision: 640 (active)` identifies the allocation-table copy
  selected by cc3200tool. It does not select or identify an MCU image.
- `APPS version: CC3x00VersionInfo((0, 4, 0, 2), ...)` comes from the tool's
  serial-loader version exchange. It is not evidence of Pico firmware
  version 0.4.0.2 or of a particular appliance model.

## Standalone storage budget for this layout

The local store's fixed record sizes and filenames are defined in
[local_store.c](../src/local_store.c). Recipes, catalog, preferences and
session sequence use alternating records with CRCs. They are created with
SimpleLink fail-safe flags set to zero; the paired records are the
application's own commit mechanism, not a second layer of filesystem
mirroring.

The maximum footprint after every bank has been used is:

| Data | Paths | Files | Content bytes per file | Flash blocks |
| --- | --- | ---: | ---: | ---: |
| 16 recipes, two banks each | `/local/recipe00.a` through `/local/recipe15.b` | 32 | 489 | 32 |
| Catalog, two banks | `/local/index.a`, `/local/index.b` | 2 | 432 | 2 |
| Preferences, two banks | `/local/settings.a`, `/local/settings.b` | 2 | 24 | 2 |
| Session sequence, two banks | `/local/sessions.a`, `/local/sessions.b` | 2 | 20 | 2 |
| Device key, ordinary user file | `/local/api.key` | 1 | 64–66 | 1 |
| **Total local data** | | **39** | | **39** |

Each small file fits one 4 KiB block, including the filename/header and
allocation rounding. Thus the complete local data budget is **156 KiB**,
leaving **107 blocks / 428 KiB** from the pictured free space while preserving
the existing firmware allocations. With the 18 existing files, the total is
57, below TI's 128-file filesystem limit. This estimate assumes the key is
provisioned as an ordinary, unmirrored file with its source size, as described
in [standalone-provisioning.md](standalone-provisioning.md).

The recovered runtime can also create `restart.txt` (100-byte maximum), which
is absent from the screenshot. Budgeting one additional block leaves
**106 blocks / 424 KiB**. System/network files may be created later as well,
so that is an estimate, not a guaranteed future free-space reading. Keep
headroom for those files. The 145-block contiguous free extent means the
pictured free space is not scattered into small unusable holes.

The web interface is embedded, compressed, in the application image; it
does not need separate HTML/CSS/JavaScript files on serial flash. Recent
events occupy RAM only. No durable brew-history database is implemented.
The full local footprint is reached over use; a newly provisioned device
does not allocate all recipe banks immediately.

The pictured `mcuimg2.bin` allocation is 192 KiB. Compare the exact candidate
length from a fresh `build/standalone/firmware/firmware-report.json` against
the actual file's available content capacity before installation. A layout
budget cannot establish boot-slot selection, binary compatibility, or a
successful install. Do not use `mcuimg.bin` as a replacement target merely
because a generic CC3200 flashing example uses that filename.

## Original evidence request, now partly completed

The owner has since supplied a verified full backup and the four files below.
Their findings are in the [actual device review](device-firmware-review.md).
The remaining immediate check is a live logical read of the mirrored boot
record; the hardware identification and physical acceptance items remain
separate. The original evidence request is retained here for other appliances.

Preserve a complete verified flash backup and obtain file-level, read-only
copies of these four files with their exact lengths and SHA-256 hashes:

1. `/sys/mcuimg.bin`
2. `/sys/mcuimg1.bin`
3. `/sys/mcuimg2.bin`
4. `/sys/mcubootinfo.bin`

Also record the machine's model/board identification and normal startup
log. The four files let us distinguish the bootloader, recovery application
and main application, inspect vectors/load addresses, and interpret the boot
record against the bootloader actually installed.

The known reference in [reference/provenance.json](../reference/provenance.json)
is 162,792 bytes with SHA-256:

```text
e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702
```

An exact `mcuimg2.bin` match identifies that application image; it does not by
itself prove a model label or verify the remaining peripherals. Compare
file contents, not a block-padded raw storage slice. Keep the complete backup
private because credentials and device-specific settings are present. The
four selected firmware/boot files are sufficient for the next software
comparison; Wi-Fi password files are not needed for that comparison.

This evidence supplements [hardware-validation.md](hardware-validation.md).
No physical acceptance item is marked passed by this directory listing.
