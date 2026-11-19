# Provisioning a standalone API key

The standalone firmware reads its per-device bearer key from the CC3200 serial
flash file **`/local/api.key`**. The key is separate from the firmware binary.
There is no shared password and no unauthenticated API for installing one.

For the owner's verified Pico Pro, the [guarded macOS USB workflow](macos-usb-installation.md)
provides key provisioning using the existing SparkFun CH340C.
Its `provision-key` command verifies the original backup and live firmware,
creates and verifies the key, then rewrites and verifies the same key using
the existing-file write mode. The owner completed this operation on
2026-11-17 at 23:49:30; creation, both readbacks, the existing-file rewrite,
and final original-application/protected-file checks passed. The
[sanitized physical result](../analysis/device-key-provisioning-20261117.json)
records that evidence. A separate `install` run on 2026-11-18 subsequently
wrote and verified the standalone image and matching key; see the
[installation result](../analysis/device-install-20261118.json).
The owner then booted the application and obtained an HTTP 200 health
response with `"mode":"standalone"`, `"ready":true`, and
`"auth_configured":true`. The [runtime result](../analysis/device-runtime-20261118.json)
confirms key loading; an authenticated request and stock restoration
remain unverified on the physical appliance.

This document also provides a TI-documented UniFlash user-file workflow,
which has **not been exercised on a physical Pico Pro or Pico S**. It assumes that the
appliance's production firmware and serial flash have already been backed up,
and that its UART connection, bootloader entry, and normal boot restoration are
verified. It does not identify Pico connector pins or claim a validated firmware
flashing procedure.

## 1. Generate a private key

Run from the repository root:

```sh
python tools/provision_api_key.py --output private/api.key
```

This writes 64 lowercase hexadecimal characters generated from 32 random bytes,
followed by a newline. On POSIX systems it creates the file with mode `0600`.
The command refuses to overwrite an existing file and does not print its key,
connect to an appliance, or flash anything. `private/` is ignored by Git.

Generate a different key for each appliance. Store the file and any programmer
session containing it privately; do not add them to a commit, firmware package,
issue, screenshot, or public build log. If choosing another output directory,
keep it outside tracked repository files.

## 2. Transfer exactly one user file

### Guarded direct USB on the owner's Mac

Follow the [macOS installation guide](macos-usb-installation.md#stage-1-provision-and-verify-the-api-key).
It uses the repository's pinned environment and the documented
[bootloader-entry sequence](macos-usb-installation.md#enter-a-fresh-programming-session).
A different existing device key is
never silently replaced; reuse the original matching host key when retrying.
The script accepts exactly the generator's 65-byte lowercase-hex-plus-LF
format and refuses a group/world-readable key file.

### TI UniFlash alternative

TI directs CC3100/CC3200 users to **UniFlash v3.4.1**. Download it through TI's
[UniFlash release page](https://software-dl.ti.com/ccs/esd/uniflash/docs/v3_4/uniflash-v3.4.1_release_notes.html),
as linked from the [current UniFlash documentation](https://software-dl.ti.com/ccs/esd/uniflash/docs/uniflash.html).
The procedure below follows sections 7.5 and 7.7 of TI's
[CC3100/CC3200 UniFlash User's Guide, SWRU558](https://www.ti.com/lit/ug/swru558/swru558.pdf).

1. Close any serial terminal using the appliance's UART. Open a CC3200 UniFlash
   session using the verified serial port and bootloader connection.
2. Choose **Add File** to create a **User Files** entry.
3. Configure that entry:

   | Field | Value |
   | --- | --- |
   | Name | `/local/api.key`, including the leading slash |
   | Url | Full host path to the generated `private/api.key` |
   | MaxSize | `0` (use the source file size) |
   | Mode | Ordinary non-secured user file |
   | Update | Checked |
   | Verify | Checked |
   | Erase | Unchecked for first provisioning |

4. Clear **Update** and **Erase** on every unrelated file and configuration
   group. **Program** applies the selected operations across the session.
5. Choose **Program**, reset when requested, and inspect the verification result.
6. Restore the verified normal boot configuration and reboot the appliance.

Use this user-file operation without **Format**, **Image Programming**, service
pack replacement, or MCU-image changes. `/local/api.key` is not a web asset and
must not be placed under `www/`. Its non-secured file mode is required because
this firmware opens it without a SimpleLink secure-file token; the bearer key
itself is the file's content, not a UniFlash file-access token.

The repository's [probe, backup and preflight tools](macos-usb-backup.md)
remain read-only. The separate guarded writer is the tool that transfers
the key. There is no runtime serial-console command for setting it.
`provision_api_key.py` only creates the host file; copying or typing its
content into a serial terminal does not provision the appliance.

## 3. Verify the running appliance

After normal startup and Wi-Fi connection, open the appliance's LAN IP address
in a browser. The root page and the minimal health endpoint are public:

```text
http://<pico-lan-ip>/
http://<pico-lan-ip>/api/v1/health
```

The health response should contain `"mode":"standalone"` and
`"auth_configured":true`. It never returns the key. Open the private host file
locally and enter its 64-character key in the browser connection form. A valid
key permits authenticated status, display, recipe, settings, and command
requests. This local HTTP implementation does not provide TLS; use it on a
trusted LAN rather than exposing its port to the Internet.

If `auth_configured` is false, protected endpoints return HTTP 503 with
`api_key_not_provisioned`. Check the exact target filename, the programmer's
verification result, and the key's contents. The accepted formats are exactly
64 lowercase hex characters, optionally followed by LF or CRLF; a byte-order
mark, extra spaces, other characters, or additional lines are invalid.

The firmware attempts to read the key once after Wi-Fi becomes available.
Missing files, malformed contents, short reads, and flash errors leave
authentication unavailable. **Reboot after provisioning or correcting the
file**; reconnecting Wi-Fi alone does not reload it. A wrong bearer key when a
valid device key exists returns HTTP 401 instead.

## Rotation and recovery

The guarded macOS writer intentionally refuses to replace a different
existing key. Its current scope is initial provisioning and verification
of the matching key. The UniFlash rotation procedure below is a separate,
physically unvalidated alternative.

Create a replacement without overwriting the old host file:

```sh
python tools/provision_api_key.py --output private/api-next.key
```

Use the same device destination `/local/api.key` and select the replacement
host file in UniFlash. If erasing the old file for replacement, select **Erase**
only on that key entry while retaining **Update** and **Verify**. Keep every
unrelated entry deselected. Reboot and authenticate with the replacement key;
the previous key then stops working.

A lost key can be replaced through the same verified physical programming
connection. There is no default key, cloud recovery service, or remote key-reset
endpoint. Replacing this file does not intentionally modify the separate local
recipe catalog or settings records.
