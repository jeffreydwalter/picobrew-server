# Phase lifecycle recovery

This module reconstructs the phase lifecycle of Pico Pro 0.1.34 as portable,
typed C. It is integrated into the complete recovered application and native
firmware build. The evidence described here is function-level differential
validation; physical appliance operation remains unverified.

## Recovered entries

| Original entry | Portable API | Behavior |
|---|---|---|
| `0x20005a20` | `pico_phase_update_shuttle_calibration` | Two timed windows, heater-time feedback, float history, completion/save boundary |
| `0x2000cde0` | `pico_phase_set` | Phase entry, display state, labels, control start/stop, pumps and drain positioning |
| `0x2000cf68` | `pico_phase_clear_status` | Clear display status area, reset status, reenter current phase |
| `0x2000cf86` | `pico_phase_pause_until_resume` | Stop control/pumps, wait for UI resume, reread/reenter phase, conditional display delay |
| `0x2000d2c4` | `pico_phase_start_circulation` | Session-dependent steam-only or drain/steam circulation |
| `0x2000d314` | `pico_phase_finish_step` | Ending UI, pump ordering, optional compartment-3 sweep and delays, idle transition |

The original four status-label routines at `0x20008440`, `0x2000846c`,
`0x20008498`, and `0x200084c4` supply the English, Spanish, and French strings
used by the internal `copy_phase_label` helper. Original instructions and
literal references are retained in `analysis/phase-original-disassembly.txt`.

The controller, control signals, screen, session, and PID types are the shared
portable application types. `PicoShuttleCalibration` replaces the small original
global history/window group with an ordinary owned object. Production code
contains no original application code or RAM addresses.

## Preserved details

Phase entry writes `controller.phase` only after its other work. Pump interlocks
therefore observe the previous phase during entry. Missing-filter paths execute
the recovered actuator pause callback and can recursively reenter the phase
routine after the UI restores the filter. The module deliberately retains this
order.

The session-type test uses unsigned subtraction: session types 3, 4, and 5 select
steam-only circulation. The finish-step routine reads the clock even though it
does not use the result, waits 1 second, and additionally moves compartment 3 to
6 and waits 30 seconds. It finally clears status and drain subphase and enters
idle. It does not perform speculative extra output shutdowns.

Calibration begins a timing window after the thermoblock reaches 300 degrees F.
Both window transitions require unsigned elapsed time **strictly greater than
60,000 ms**. The first completed window sets a settling flag; another window
measures heater-on time. The accepted heater-time band is **22,700 through
26,700 ms inclusive**. Above that band the binary64 candidate decreases by
0.04; below it the candidate increases by 0.04. Updated candidates are rounded
to binary32 and placed in the three-entry history. Equality of entries 0 and 2
also completes calibration; the middle value is not part of that equality.
Otherwise the last two entries are shifted and retained.

Completion writes the thermoblock target to 330, sets the completion byte,
sets active scale to candidate plus 0.15, and invokes the save callback before
clearing the settling flag and history count. An already-complete call preserves
the original nonzero completion byte as its return. Heated-hold phase entry uses
candidate plus 0.1 instead. NaN comparison behavior is preserved rather than
replaced with an invented validity rule.

## Integration contracts

`PicoPhaseControl` references the same controller and actuator instances used by
the control timer code. The actuator `current_phase` callback must read that
controller's current phase. Its `pause_until_resume` callback invokes
`pico_phase_pause_until_resume` on the same phase instance. Control output
callbacks must likewise use the same actuator instance. This preserves the
existing filter interlock instead of bypassing it with raw GPIO writes.

The UI, stepper, logging, and settings callbacks in `PicoPhaseOps` are required
implementation boundaries, with no success stubs. `src/application.c` binds
them to the recovered UI, device and network implementations. The wait callback
may alter controller state; the phase code rereads it after return. The stepper
return value was ignored by these original phase routines, so the phase-facing
callback has a void return; the actual device implementation handles its
protocol and errors within that boundary.

The settings callback receives the candidate scale at the original save point.
The application binds it to `pico_storage_save_shuttle_scale`, whose recovered
implementation in `src/network/storage.c` performs the binary64-to-binary32
conversion, three-decimal formatting and six-byte `SS.txt` write. The phase
suite observes this call boundary; the network suite separately checks the
storage behavior. Neither suite establishes physical serial-flash operation.

Fresh calibration storage receives static/BSS zero initialization. Its history
count must be 0..2 on entry; arbitrary corrupt indices are outside the original
routine's valid-state contract. No hidden reset is added to phase entry.

## Differential validation

`test_phase.py` executes the original instructions in one Unicorn Cortex-M
instance and the newly compiled C from a separate ARM ELF in another. The
rebuilt instance contains no reference image. The C uses the real compiler
soft-float and C runtime libraries.

The test executes original and rebuilt phase, start/stop control, PID mode
changes, actuator interlocks, filter debounce, and label-copying code. It does
not replace those application functions with expected-return stubs. Controlled
boundaries are GPIO, clocks/interrupts, delays, UI, stepper, logging, and settings.
The original fixed `"%s %d"` phase-log formatter is emulated after verifying its
format and prefix; the rebuilt C uses its real `snprintf`. Resulting log strings
are compared through hashes.

Each scenario compares all 552 controller bytes after portable field mapping,
the seven control signals and relevant screen/session/language/calibration
state, meaningful return values, and ordered boundary events. Events include
snapshots of phase, status, compartment, completion flag, screen fields, control
enabled flag, calibration flags/count, phase-label hash, and heater-on time.
This checks intermediate ordering as well as final state.

The matrix covers all phase branches and integer truncation at phase storage;
language and session fallbacks; active/error controllers; automatic/manual PID
states; filter-present, contact-bounce, and missing-filter/resume paths;
compartment-dependent output decisions; resume-time phase changes; finish-step
timing; calibration thresholds, unsigned wrap, history shifts, infinities, and
NaNs; short multi-call sequences; and deterministic randomized scenarios.

The current function-level result and per-case evidence are recorded in
`analysis/phase-tests.json`; the generated
[validation summary](../analysis/validation-summary.json) supplies current
suite totals and image identities. Comparisons require exact ARM agreement,
including floating-point representations; no NaN exception is used. The phase
report records the tested source, original image and rebuilt ELF hashes.

This is bounded differential evidence, not a proof over all inputs. It does not
test interrupt concurrency, physical hardware, complete UI interactions,
stepper handshakes, or actual storage writes. Arbitrary memory corruption and
out-of-range calibration history indices are excluded.

## Reproduction in the integrated tree

After building the project's validation ELF with `tools/build.py`, run:

```sh
python3 tests/test_phase.py --elf build/application-validation.elf \
    --output analysis/phase-tests.json
```

The script discovers the packaged project root by default and also accepts
`--source-root` and `--image`. Its fixture belongs in
`tests/phase_fixture.c`; its exports have the unique `phase_fixture_` prefix.
The original firmware SHA-256 is
`e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702`.
