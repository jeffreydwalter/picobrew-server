# Recorded core integration audit

This document preserves the earlier core integration review of the state,
PID, control, actuator, sensor and Energia adapter sources, together with that
milestone's build/test entry points. The review compared the recovered source,
original function evidence and pinned Energia/TI declarations. It made no
production-source edits.

The project now also includes the complete setup/main loop, UI, device/RFID,
recipe, network and storage implementations and a native firmware build.
Current scope and results are recorded in the
[completion audit](../analysis/completion-audit.json) and generated
[validation summary](../analysis/validation-summary.json). The findings below
are retained as core integration evidence, not a claim that this earlier
review exercised the complete appliance.

## Result

No actionable integration defect was established in the sources inspected by
that review.

The following potentially error-prone joins were checked:

- `PicoControl` points to the owned controller and the same seven control signals
  used by the initialization and serialized control tests.
- Both PID instances bind their own input/output/setpoint fields, and ordinary
  native pointer widths are handled by test serialization rather than forced
  original memory addresses.
- The adapter reads logical pin 25 for wort and 26 for thermoblock, passes the
  session type from the original session context field at offset `0x30`, and
  restores `INPUT` after each ADC read. The pinned SDK initializes ADC resolution
  to 12 bits, matching the calibration domain.
- Original PID calculation reads the clock before reading its input; the
  control adapter supplies the clock at that same stage. The constructor's
  separate clock boundary preserves its more specific state-update ordering.
- The SDK adapter uses actual `IPAddress`, `WiFiClient`, GPIO, interrupt, timer,
  and clock declarations. Its C++ translation unit shares the same Cortex-M4
  soft-float AAPCS settings as the C application archive.
- Placement storage has native `sizeof(T)` and `alignas(T)`. HTTP destruction
  registration uses a native destructor callback and the address of the actual
  runtime DSO handle.
- The core differential suites were integrated into the common build, and the
  SDK check retained every exported application/adapter symbol when linking.
  The current Makefile and generated validation summary define the complete
  suite set; the native linker still verifies that all rooted exports remain.
- Test scripts locate the project by their own path; they do not depend on the
  original scratch-work paths after integration. Python test dependencies are
  listed in `requirements-test.txt`.

## Caller contracts and known boundaries

The core caller contracts remain applicable to the complete application:
initialize application state before starting control; keep bound
state/runtime/platform storage alive; supply every required UI/RFID callback;
perform SDK placement construction once for a given object's lifetime; and
preserve the 12-bit ADC configuration. The state constructor's reentry comparison
tests logical partial writes, not safe repeated construction of live SDK
objects.

`src/application.c` and `platform/firmware_main.cpp` now supply the complete
setup sequence and concrete callback bindings. Display/input and RFID logic
are implemented in `src/ui/` and `src/rfid.c`, with their own function-level
comparison suites. Interrupt concurrency on an actual appliance remains
outside the offline evidence.

This recorded review alone did not establish a complete firmware image. The
current project separately builds and inspects that image as described in
[SDK build](sdk-build.md). Neither that build nor the core function-level
comparisons establish physical electrical behavior, device installation or a
successful brew; [hardware validation](hardware-validation.md) records those
unperformed checks.
