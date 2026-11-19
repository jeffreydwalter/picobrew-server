# Recipe, session, and program recovery

This component replaces **43 original Pico Pro 0.1.34 entry points** with ordinary C and owned application state. It covers recipe decoding and execution, session recovery and shutdown, brewing, manual brewing, sous vide, cold brew, the high-level PicoStill workflow, and the maintenance utilities. No entry in this domain remains an unimplemented service placeholder. Five shared phase functions are supplied by the previously recovered controller component.

The reference image is `reference/original-0.1.34.bin`, loaded at `0x20004000`, with SHA-256 `e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702`. Function names and local variable names in the replacement are descriptive reconstruction choices; the binary did not supply the original source identifiers.

## Ownership and files

| Source | Responsibility |
|---|---|
| `src/recipe.c` | Program context tables, recipe decoder, temperature preparation, complete brew-step loop, recipe runner, restart decoder, session setup/shutdown, and program wrappers |
| `src/recipe_programs.c` | Location translation, current-program instructions, manual brewing, sous vide/cold brew, and high-level PicoStill setup and execution |
| `src/recipe_utilities.c` | Filter removal, steam priming/repriming, vacuum, circulation, reservoir/filter draining, valve flushing, first rinse, routine rinse, beer racking, and descaling |
| `src/recipe_data.inc` | Owned string tables and action arrays transcribed from the reference image |
| `include/picobrew/recipe.h` | Typed runtime, request, service, screen, prompt, asset, and carousel contracts |
| `tests/test_recipe.py`, `tests/recipe_fixture.c`, `tests/recipe_reference.json` | Original-versus-rebuilt ARM execution tests and explicit controlled service boundaries |

The existing `PicoAppState`, `PicoControl`, `PicoPhaseControl`, and `PicoActuators` own shared state and low-level control. UI owns drawing, menus, instruction screens, and encoder interaction. Network owns HTTP, session identifiers, persistent restart records, settings, telemetry, and PicoStill protocol calls. Device/RFID components own physical movement, buzzer behavior, the thermoblock heat check, and card reading. `src/application.c` supplies the concrete bindings between them. Identifier copying uses the shared recovered `pico_text_copy` implementation, preserving its warnings and its rule that bytes after the copied NUL remain untouched.

## Verified field meanings

Recipe records are 32 bytes on the original ARM ABI. Execution at `0x20011590`, the loop at `0x20011e68`, and the decoder at `0x20008ccc` establish these meanings:

| Offset in a recipe record | Recovered meaning |
|---|---|
| `0x00..0x15` | Step name, with a 21-character maximum and terminating NUL |
| `0x16` | Compartment or special instruction location |
| `0x17` | Target temperature in Fahrenheit, stored as a byte |
| `0x18..0x1b` | Drain-stage duration in minutes |
| `0x1c..0x1f` | Active-stage duration in minutes |

The prior initialization-only field names `kind`, `duration`, and `temperature` remain layout-preserving aliases. New execution code uses `target_fahrenheit`, `drain_minutes`, and `active_minutes`.

Controller byte `0x1a1` records whether automatic reprime has already been attempted for status 5. It is exposed as `reprime_attempted`. The former neutral name remains an alias for compatibility. The first-rinse cooling helper also has a resolved ABI: its original `r3` argument points to the caller's signed next-compartment counter. It does not consume the apparent values in `r0`–`r2`.

All thermal arithmetic uses real `float`/`double` values and the Cortex-M4 soft-float ABI. In particular, drain overhead preserves the original signed conversion to single precision, two `ceil` operations, and subsequent signed conversion. Counter wrapping and strict versus inclusive time comparisons are retained.

## Validation evidence

The suite passes **1,694 differential cases across all 43 newly recovered original entry points**, plus **41 bounded-input checks** for deliberate parser repairs.

| Original module group | Recovered entries | Differential cases |
|---|---:|---:|
| Brewing and utilities | 25 | 1,069 |
| Parsing | 2 | 364 |
| Sessions | 5 | 138 |
| Program contexts/instructions | 7 | 91 |
| Hardware orchestration | 4 | 32 |
| **Total** | **43** | **1,694** |

The main step engine alone has 652 cases. They exercise controller phases 0–8 and the default branch; statuses 0–11; menu actions 0–12; all session kinds; drain subphases at their exact 2-, 15-, 30-, and 45-second boundaries; wrapped and signed duration arithmetic; reprime/recovery decisions; timed completion; indefinite holding; and PicoStill communication, temperature, and pressure decisions. Temperature preparation has another 143 cases around its thermal thresholds and filter-removal paths. Session tests cover cancellation, restoration, shutdown, holding/draining after completion, and every program family.

The original image and newly linked C execute in separate ARM emulator instances. Comparisons include returned results, controller bytes, recipe records and metadata, program contexts, screen state, carousel selection, progress flags, and ordered service calls. UI, networking, clocks, mechanics, and the previously verified control/phase entry points are explicit controlled boundaries. Higher-level tests can also isolate a recovered child workflow; that child's own entry point is tested separately. Unhooked fixture services execute a breakpoint rather than returning a fabricated success.

This evidence establishes behavior at the documented boundaries. It does not substitute for running the composed firmware on the physical appliance. The final complete ELF should run the same suite:

```sh
python tests/test_recipe.py \
  --elf build/application-validation.elf \
  --output build/recipe-validation.json
```

The JSON report records the reference-image hash, tested ELF hash, source hashes, fixture hash, per-case result, and original address. The per-entry recovery manifest gives each function's replacement and case count.

## Deliberate malformed-input repairs

The original delimiter helper has no destination-capacity argument and does not treat a NUL as the end of a field. Reproducing that behavior for externally supplied text would allow oversized recipe fields to overwrite a 100-byte local buffer, a 21-character restart identifier to overwrite the next field, or a truncated restart record to be parsed beyond its terminator.

The historical helper remains available with an explicit caller-sized-output contract for analysis. Production recipe and restart parsing use a checked equivalent. It verifies the destination capacity, the actual input extent, and the required separator before copying a field. Recipe responses reject null input, negative lengths, embedded NULs, missing termination at the declared endpoint, and fields of 100 or more bytes. Restart decoding bounds the 101-byte receive buffer, rejects an unterminated record, and allows at most 20 identifier characters plus NUL.

The 41 additional checks cover oversized header/numeric/step-name fields, invalid text extents, null input, every truncation of a representative restart record, an oversized restart identifier, an oversized numeric field, and a completely unterminated restart buffer. These tests do not execute the original undefined memory behavior as an oracle. Valid-record behavior and observable failure behavior within the original defined bounds continue to pass the differential suite.

## Original behavior intentionally retained

- A custom-instruction step at location 8 can return directly from the recipe runner without its common finalizer. The reconstruction preserves that control flow.
- The original `run_circulation_program` entry at `0x20012658` actually builds and runs the heated six-compartment cleaning recipe. The separate circulation utility remains a separate function.
- PicoStill builds one counted `Heating` record with active duration `0x7fffffff`. It also writes `Maintain` into the following record without incrementing the count to include it. The reconstruction does not invent a second active step.
- First-rinse retries preserve the original distinction between the first heat-test result and the later cooling result. A retry's heat-test return does not replace the first result.
- Successful reprime returns an existing nonzero calibration byte literally. It is not normalized before the wrapper returns.
- The drain subphase-5 path falls through into the subphase-6 timing check. Timing and output order retain that behavior.

## Integration contracts

The runtime, settings bytes, state, page callbacks, and bitmap descriptors must outlive every workflow using them. Required callbacks must point to actual services. Application initialization constructs native SDK storage, installs owned state, binds the UI/network/device/recipe services, and only then enters the setup sequence that enables timers.

Three filter-related operations are deliberately distinct: the plain device wait, the alarm-backed actuator insertion prompt, and the UI instruction that asks for a filter. Merging them changes visible behavior. Temperature-column and temperature-summary callbacks take typed doubles; the old apparent extra integer register was AAPCS alignment padding.

The five normal carousel pages must be ordered as brew name, current step, step progress, temperature, and recipe metrics. Manual pages, cooking pages, the holding page, and the still readings page have separate callbacks. `application.c` supplies these mappings and the actual owned bitmap assets.

A final read-only audit of `application.c` and `platform/firmware_main.cpp` checked screen/prompt mappings, callbacks, pressure-limit polarity, recovery parameter ordering, object lifetimes, setup/restart dispatch, and the main menu. The normal-page ordering discrepancy found during that audit was corrected. No additional concrete mapping defect was identified in that review.

## Native production constructor validation

`tests/test_native_startup.py` executes `_GLOBAL__sub_I_setup` from the actual production ELF, after loading its data and zeroing BSS as the reset sequence requires. The ELF contains no added test fixtures. The script locates the application constructor in the native initialization array and reads C/C++ layouts from compiler DWARF, including SDK base classes. It controls only `millis` and `__aeabi_atexit`; native IPAddress and WiFiClient construction, application initialization, PID initialization, and all service bindings execute as compiled. No hardware, network, UI, or application-constructor function is intercepted.

Four independent startup cases pass **1,632 assertions**, with **238 callback slots across 14 service tables** checked per case. The cases use different PID clock readings, include a wrapping clock, and verify that a failed destructor-registration return is recorded. Checks cover native vtables and initialized client fields, owned controller/PID/recipe state, every shared context pointer, display and encoder defaults, the normal carousel order, and owned bitmap data. Two bound callbacks are executed to verify their real context translation. The native HTTP destructor supplied to registration also executes with the client's original no-socket state.

The test initially rejected the stale production ELF's wrong carousel order and passed after the source correction was rebuilt. Its JSON report records the exact production ELF hash, script hash, per-case instruction counts and assertions, and controlled boundaries:

```sh
python tests/test_native_startup.py \
  --elf build/firmware/pico-pro-recovered.elf \
  --output analysis/native-startup-tests.json
```

This closes a specific lifecycle gap beyond successful linking: the production application constructor creates native objects and connects the recovered modules correctly under bounded execution. It does not execute `ResetISR`, SDK peripheral initialization, `setup()`, a live network session, or a complete appliance cycle. Those boundaries remain distinct from this constructor test.
