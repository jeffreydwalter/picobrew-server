# Relocatable brewing-control implementation

Seven original routines now have ordinary application-owned state and typed
interfaces. `control.c` contains no original application RAM/code addresses.
The only fixed addresses are genuine CC3200 timer peripheral bases.

| Original entry | Rebuilt implementation |
| --- | --- |
| `0x20005004` | `pico_control_record_temperature_sample` |
| `0x20005028` | `pico_control_start` |
| `0x20005120` | `pico_control_classify_temperature_progress` |
| `0x20005f50` | `pico_control_stop` |
| `0x2000d1c0` | `pico_control_outputs_tick`, reached through the typed output ISR |
| `0x2000e088` | `pico_control_temperature_tick`, reached through the typed temperature ISR |
| `0x20005080` | `pico_control_timers_initialize` |

`PicoControl` refers to a normal `PicoBrewController`, its application-owned
`PicoControlSignals`, and explicit platform operations. PID state has native
typed pointers. The original saved r0/r1 words in the temperature interrupt
are scratch storage, not meaningful application parameters. Both vector
callbacks now have the real `void(void)` interrupt signature. Registering a
function pointer uses the compiler's normal Thumb representation; there is
no extra `+1` adjustment.

## Established behavior

- Timer setup preserves the original order, peripheral IDs 14 and 15,
  periodic configuration `0x22`, and load values 80,000,000 and 1,600,000.
  Output timing starts before temperature timing. The routine disables and
  then unconditionally enables interrupts, as the image does.
- Both interrupt routines obtain their timestamp before disabling interrupts
  and clear the status returned by the corresponding timer interrupt query.
- The temperature interrupt always samples pin 25 into wort temperature and
  pin 26 into thermoblock temperature. It promotes each returned float to a
  double. The original split-word representation casts are gone.
- Step time advances in phase 8 and otherwise decreases when nonzero,
  including the original 32-bit wrap behavior. This happens even while heat
  control is disabled.
- With control enabled and status zero, a reading below 32°F stops control,
  disables steam/drain outputs, and stores status 3. Thermoblock temperature
  strictly above 385°F enters the non-returning heater-fault handler.
- Wort temperature strictly above 215°F combined with a target strictly above
  195°F stops control and stores status 1. Equality does not trigger it.
- A PID compute has a clock read only when that PID is automatic. Both real
  recovered PID implementations run; they are not replaced by output stubs.
- Heater timing advances one 3000 ms window when elapsed time is strictly
  greater than 3000; it does not catch up multiple missed windows. Shuttle
  timing similarly advances a single 500 ms window after the output decision.
- Shuttle on-time uses `(wort_pid_output / 3000) * scale * 500`, preserving
  operation order. Negative scale becomes zero. Conversion saturates like
  the original compiler helper, and results below 22 become 21 ms.
- The history/trend state increments each enabled, status-zero tick. A
  30-second boundary records a sample and resets the short-window peak. At
  60 seconds the routine records, updates peaks, and classifies temperature
  progress. Return statuses 1, 2, and 3 become controller statuses 5, 6, and 7.
- Heating classification retains six integer-truncated samples, wrapping
  32-bit sums, the integer denominator 10, and the two-slow-window latch.
  Heated-hold classification retains the separate volatile peak reads.
- The final overtemperature check runs when heat control is disabled. The
  original branch order, including the enabled/nonzero-status path, remains.

## Non-returning fault boundary

The original handler at `0x2000e054` enables interrupts, clears the control
enable flag, shuts down its designated outputs, sounds the alarm, disables
interrupts, draws/reports the fault, and enters an endless `millis()` loop.
The caller's apparent dependencies on incoming register values occur only
after that non-returning call and are not legitimate ISR arguments.

The `halt_heater_malfunction` platform callback must implement that fault
handling contract and not return. The C controller also has an infinite
fallback after the callback. The control suite stops at the common
fault-handler entry and establishes detection and ordering up to that boundary.
The complete application binds it to `pico_halt_heater_malfunction` in
`src/ui/diagnostics.c`. The [UI suite](ui-recovery.md) separately checks that
handler's shutdown, display and reporting order and its non-returning behavior.

## Output and UI integration contract

`shuttle_pump_on` is the original application boundary, which checks the
filter and may pause/resume before writing the GPIO. It must not be replaced
by an unconditional GPIO-high callback. The remaining output operations in
this module are the simple heater on/off, shuttle off, steam off, and drain
off boundaries. The progress display operations preserve row/column values,
glyph wrap, clearing, and flush order.

All operations used on a selected path must be supplied. They are explicit
integration requirements, not default success stubs. Bind the single control
instance before enabling its timers, and keep its objects alive.

## Validation

`analysis/control-tests.json` records function-level comparisons between the
original firmware and a newly linked Cortex-M4 Thumb/soft-float ELF compiled
with GNU Arm Embedded 10.3.1. The generated
[validation summary](../analysis/validation-summary.json) supplies current
suite totals and image identities. The rebuilt ELF uses the real compiler
runtime, libc and recovered PID C implementation. It executes in a separate
emulator instance and cannot call into the original image.

The tests compare all 552 bytes of the original controller image after
mapping the owned structure back to the original layout, seven signal bytes,
meaningful return values, and ordered external events. Pointer fields are
mapped to their equivalent owned objects. Unknown and padding bytes retain
their input image values. The original PID, sample recorder, classifier,
start, and stop instructions execute beneath the ISR without mocks.

The cases include threshold neighbors, enabled/status gates, phase and
step-counter wrap, 3000/500 ms timer boundaries, delayed ticks, both PID
modes, history states, deterministic random states, infinities, and NaNs.
The newly built ARM results match exact state bytes, including tested NaN
representations. The report retains the executed instruction counts.

A historical native-host run also passed this matrix under an explicitly
narrower contract: two PID arithmetic NaNs had different signs from the
original soft-float result, and the host mode reported those separately while
comparing NaN classification. The rebuilt-ARM comparison permits no such
differences. The packaged final verification uses the ARM mode.

Hardware registers, clock values, sensor measurements, outputs, and UI calls
are controlled fixtures. These tests do not simulate interrupt concurrency,
physical hardware, filter timing, or the complete appliance. Negative
corrupted history indices are outside the defined C test domain. The test
ELF is a function-validation image and is not a bootable brewer firmware.

## Reproduction and integration

Production files are `src/control.c` and `include/picobrew/control.h`, using
the shared recovered state and PID interfaces.

The integrated test files are `tests/control_fixture.c` and
`tests/test_control.py`. The fixture exports only `control_fixture_*` public
names and is linked into the common application-validation ELF. It is absent
from the production firmware.

From the integrated project:

```sh
python3 tests/test_control.py --source-root . \
    --elf build/application-validation.elf \
    --output analysis/control-tests.json
```

The project must supply `tools/original_arm.py` and `tools/rebuilt_arm.py`.
`--image` can select the exact original reference image; its hash is checked.
Omitting `--elf` performs native-host comparison instead.
