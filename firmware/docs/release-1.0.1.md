# Standalone firmware 1.0.1

This patch replaces Settings' three-card grid with one continuous page.
Display preferences use compact label/control rows, the receiver host and port
share a row on desktop, and browser access is a small section at the bottom.
Horizontal dividers separate the sections; controls stack on narrow screens.
The existing fields, save actions, authentication, and forwarding behavior are
preserved. Firmware identity is now 1.0.1; the REST API remains `/api/v1`.

## Package and storage

The application remains **164,600 bytes**, with its original **32 KiB stack**
and **32 KiB heap** reservations and **952 bytes** of unallocated SRAM.
The five gzip browser files total **21,764 bytes** and still use **eight 4 KiB
blocks** including filesystem metadata. `style.css.gz` is **3,558 bytes**,
within its existing 3,584-byte capacity; `index.html.gz` is **4,123 bytes**.
No browser-file expansion, deletion, or relocation is required.

Application SHA256:

```text
7f1ce21158b6f0f0297087e37c71cda23926e1a5c4902d938f4f0162929d36ce
```

Browser-manifest SHA256:

```text
a65d853c56f7a9751364523234c7c3aa41d04ff126c263dd9bdb69cb2a33eb0f
```

Browser lengths are compiled into the application descriptors, so this is a
matched application/browser update. Use the [macOS installation procedure](macos-usb-installation.md)
with the existing API key and original backup. Installer revision 5 accepts
stock, the earlier 161,720-byte standalone image, released 1.0.0, and the exact
current target as known live application identities. Only the pinned 1.0.1
payload is accepted for installation. It writes and verifies the browser
files before the existing image2 allocation, retaining the other firmware
images, boot metadata, recipes, history, and settings.

## Validation status

The owner's 1.0.0 upgrade passed complete application/browser/key verification,
and the owner then reported running 1.0.0. Supplied Overview and Settings
screenshots show the version, upright OLED, arrow controls and new navigation;
the owner explicitly confirmed the OLED fix. Those observations are recorded
in the [sanitized upgrade record](../analysis/device-upgrade-1.0.0-20261118.json).

**1.0.1 has not yet been installed on the physical Pico.** Its build checks,
installer upgrade-source tests, and browser validation are recorded separately
in [the patch validation record](../analysis/standalone-1.0.1-validation.json)
and the CI run for the published commit. Browser previews include desktop,
wide desktop, and 390-pixel mobile Settings layouts; the existing workflows
continue to exercise preference saves, forwarding edits and disconnection.

Physical DHCP switching, control-button operation, retained history/export,
forwarding, interlocks, complete brewing, and interrupted-write restoration
remain outstanding from the 1.0.0 acceptance work.
