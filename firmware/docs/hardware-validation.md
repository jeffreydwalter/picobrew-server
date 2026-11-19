# Physical firmware acceptance and backup status

The source, test executables and complete firmware image have been built and
checked offline. The owner completed a read-only serial-flash backup on the
actual Pico Pro on 2026-11-17: two complete 1 MiB reads matched byte-for-byte.
The [macOS backup record](macos-usb-backup.md) documents that result. The owner
installed the pinned standalone image and verified its complete readback on
2026-11-18. The tool left the application unstarted, and the owner subsequently
booted it and received HTTP 200 from the public health endpoint. The
[runtime record](../analysis/device-runtime-20261118.json) preserves the exact
response and its limits. The owner subsequently showed the connected browser
console with sensor readings and an inverted OLED preview; the
[browser record](../analysis/device-browser-20261118.json) preserves those
observations. Those boot and browser results concern the previous 161,720-byte
candidate. **The 1.0.0 firmware and separate browser bundle subsequently passed
installation and readback verification on 2026-11-18.** The
[upgrade record](../analysis/device-upgrade-1.0.0-20261118.json) preserves the
installer's success and limits: it did not start the application. The owner
subsequently reported the Pico running 1.0.0 and supplied Overview and Settings
screenshots showing that version. The owner confirmed the upright OLED preview;
arrow controls and Brew history/API navigation are visible. A new health
response and functional feature checks remain outstanding. A successful link, file
readback, health response or browser connection cannot verify physical outputs,
recovery or a heating cycle.

The subsequent [1.0.1 Settings layout patch](release-1.0.1.md) has not yet been
installed. Its package and software checks are separate from these 1.0.0
hardware observations.

The owner's extracted firmware and boot records were also reviewed. The
main application exactly matches the original v0.1.34 reference. The
[device review](device-firmware-review.md) records the actual bootloader's
selection behavior, the cloud-dependent recovery image, and the flash-space
constraints. The owner also completed live logical-file preflight on
2026-11-17: all three firmware hashes match, image 3 is absent, and
`01 3d 00 20 ba dc cd ab` confirms normal selector 1 for image 2.
API-key creation, readback, existing-file rewrite and final verification
also passed on that date. The [installation result](../analysis/device-install-20261118.json)
records the later 161,720-byte application rewrite, full readback, matching
key and unchanged protected files. Standalone boot and public HTTP health
access passed for that earlier build; stock restoration and interrupted-write recovery
remain outstanding. The earlier
[third-party screenshot](flash-layout.md) has a different layout and unknown
Pro/C identity.

## Device acceptance record

| Check | Evidence to record | Current result |
| --- | --- | --- |
| Identify device | Main MCU/board revision, auxiliary-controller version, current firmware and serial log | Saved main application is an exact v0.1.34 reference match; remaining hardware identification not run |
| Preserve recovery | Verified existing application/recovery image and boot-file backup; proven way to restore the device | Full 1 MiB backup and live logical-file preflight verified on 2026-11-17; guarded stock restore implemented but not physically validated |
| Validate file writes | API key create, readback, existing-file rewrite and second readback, followed by protected-image checks | Passed on 2026-11-17; original application and protected-file postchecks passed |
| Install standalone image | Rewrite the existing image 2 allocation, compare every application byte, verify the key and recheck protected files | Passed for the earlier 161,720-byte candidate and the 1.0.0 application/browser upgrade on 2026-11-18; installers did not start the application; interrupted-write recovery remains untested |
| Standalone boot and readiness | Application starts, front-panel menu is available, public health reports standalone mode and readiness | Earlier candidate passed menu/health checks on 2026-11-18 with HTTP 200 and `ready: true`; owner subsequently reported running 1.0.0, but its health/readiness response remains uncaptured |
| Initial output and reset behavior | Outputs isolated for bench observation; no unexpected pump/heater activation; reset and service-mode behavior | Not verified; successful boot does not establish these checks |
| Display/encoder | Native font, rotation, navigation, click debounce and all three language selections | Owner confirmed the corrected upright OLED preview in the 1.0.0 browser; up/down controls are visible; their operation and complete display/encoder acceptance remain untested |
| Sensors | Raw ADC counts and independently measured wort/thermoblock temperature at several known points | Not run |
| Outputs | Actual logical pins 5/8/24/27 correspond to heater/drain/shuttle/steam outputs and default off states | Not run |
| Filter interlock | Pin 11 active-low presence; pump pause/resume; missing-filter prompts and alarms | Not run |
| Rotary valve/stepper | Capability query, ready/error replies, every compartment, legacy mapping, torque and home position | Not run |
| RFID/shared SPI | Known 4/7/10-byte cards, card removal/cancel, shared chip selects and simultaneous display activity | Not run |
| WiFi/API | Compatible local server, loss/reconnection, exact requests and UI failure handling | Public health passed on the previous candidate; 1.0.0 browser screenshots show Overview readings and Settings. No new health response, API-explorer request, network-change or failure-handling test has been supplied |
| Version 1.0.0 features | Matching application/browser install, upright OLED, arrow controls, network change, retained brew/export, optional receiver | Application/browser/key readback and owner-reported boot passed; versioned Overview/Settings and owner-confirmed upright OLED captured; arrows and Brew history/API entries visible; functional checks outstanding |
| Persistent files | Credentials/preferences/UID, shuttle scaling, restart serialization and failed/short I/O | Not run |
| Update/recovery | Inactive-image write, full binary integrity, boot-file transition, reset and restore after interrupted update | Not run |
| Timers/concurrency | ISR frequency, actual clock rate, SPI/display/network interaction and nonreturning fatal path | Not run |
| Controlled thermal run | Independent temperature reference, cutoff/fault behavior, pump continuity and cooling/drain transitions | Not run |
| Complete program | Recipe, rinse, cancel/restart and maintenance cycles under observed operation | Not run |

Use an isolated output/bench setup before powered thermal operation. The
firmware contains the recovered filter and thermal interlocks, but their
physical execution has not been demonstrated by this project. Do not treat a
software assertion or an emulator's simulated sensor value as a physical
safety test.

## First runtime results and Wi-Fi correction

The 1.0.0 Overview screenshot displays wort temperature 76.0°F, thermoblock
83.3°F, an unheated target, step time 0:00, READY, and filter removed. The main
menu OLED is upright, matching the owner's “OLED fixed” confirmation. The
Settings screenshot shows the existing three sections and firmware 1.0.0.
These are rendering and displayed-reading observations; they do not verify
button operation, sensor calibration, filter interlock behavior, history/API
requests, forwarding or DHCP changes. They are recorded with the
[1.0.0 upgrade evidence](../analysis/device-upgrade-1.0.0-20261118.json).

The owner supplied an HTTP 200 response from `GET /api/v1/health` on
2026-11-18 for the earlier unversioned build. It reported API version 1, standalone mode, `ready: true`,
`auth_configured: true` and uptime 132,543 ms. This proves the application
served that public request and reported a configured key; it does not
prove a bearer-authenticated request or physical sensor/output behavior.
The later connected-browser screenshot separately demonstrates browser
rendering and the application's authenticated reporting path, but no complete
command or interlock acceptance sequence was supplied.

Before that response, changing the selected SSID showed the new network
name alongside an address outside the expected subnet. The owner reported
that a reboot corrected DHCP. Source inspection found that the status
screen displays the configured SSID, and that the connection adapter can
return early while the SDK's connecting flag remains set. These findings
are consistent with the observation, but the exact runtime cause was not
captured. Version 1.0.0 now explicitly disconnects, clears cached addresses,
requests DHCP and waits for a fresh IPv4 callback before reporting success.
Tests exercise delayed old callbacks, immediate reconnect, missing leases,
errors and the real foreground socket-service throttle. The fix still needs
confirmation on the owner's network without a reboot.

For 1.0.0, also check that a recorded brew remains available after closing the
browser and restarting the appliance, that exports match the displayed
readings, and that an unavailable optional receiver does not interrupt local
recording or operation. These are acceptance checks, not completed results.

## What the offline image checks establish

`tools/build_firmware.py` checks the vector-table load address, Thumb reset
entry, eight-byte initial stack alignment, native entry symbols, physical RAM
extent, 32 KiB heap reservation and binary/ELF correspondence. It rejects test
fixture symbols and undefined functions in the production link.

`tests/test_update_package.py` feeds the complete generated server response to
the actual rebuilt ARM receiver and compares the resulting in-memory file with
the compiled image. This verifies the software encoding and receiver together;
it does not establish serial-flash endurance, atomic writes, real bootloader
behavior or a working recovery route.
