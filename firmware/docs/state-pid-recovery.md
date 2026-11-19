# Application state and Pico PID recovery

This module reconstructs the application global constructor, its application
state, the telemetry reset it calls, and the eight recovered PID entry points.
It is ordinary C with application-owned objects and real pointer fields. It
does not require the original firmware to be present when the rebuilt module
runs. The module is now integrated into the complete application through
`src/application.c` and `platform/firmware_main.cpp`; this document describes
its state and PID responsibilities.

## Reference

The reference is the Pico Pro 0.1.34 binary, loaded at `0x20004000` for analysis:

```text
SHA-256 e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702
```

Addresses below are evidence locations in that image. Production source uses
them only in explanatory comments; no original RAM or function addresses are
used by the implementation.

| Recovered entry | Original address | Source entry |
| --- | --- | --- |
| Global application constructor | `0x200090f8` | `pico_application_initialize` |
| Clear still telemetry | `0x2000489a` | `pico_still_clear_telemetry` |
| PID disable | `0x200146cc` | `pico_pid_disable` |
| PID calculate | `0x200146d4` | `pico_pid_compute` |
| PID set tunings | `0x200147c0` | `pico_pid_set_tunings` |
| PID set output limits | `0x20014890` | `pico_pid_set_output_limits` |
| PID initialize integral | `0x2001491c` | `pico_pid_initialize_integral` |
| PID set mode | `0x20014962` | `pico_pid_set_mode` |
| PID set direction | `0x20014982` | `pico_pid_set_direction` |
| PID constructor | `0x200149c8` | `pico_pid_construct`, `pico_pid_construct_clocked` |

## Constructor signature and actual Pico tunings

The original PID constructor takes `this`, input, output, and setpoint pointers
in r0-r3. Its remaining arguments are:

| Entry stack offset | Argument |
| --- | --- |
| `+0x00` | Kp, double |
| `+0x08` | Ki, double |
| `+0x10` | Kd, double |
| `+0x18` | direction, integer |
| `+0x1c` | unused double-alignment padding |
| `+0x20` | maximum output, double |

The apparent extra argument in the earlier raw decompilation was alignment
padding. The constructor does not read it. Differential tests set that word
to `0xdeadbeef` while executing the original instructions.

The global constructor supplies these values:

| PID | Kp | Ki | Kd | Direction | Minimum | Maximum | Sample interval |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Wort | 130 | 50 | 30 | 0 | 0 | 3000 | 100 ms |
| Thermoblock | 150 | 50 | 80 | 0 | 0 | 3000 | 100 ms |

These are the displayed tunings. The internal integral gain is Ki multiplied
by the sample interval in seconds; the internal derivative gain is Kd divided
by that interval. The reconstructed constructor preserves the order of limit
setting, interval setting, direction, tunings, clock read, and pointer/timestamp
binding. The clocked form is used by the application initializer so that the
state visible at each clock call also agrees with the original.

Two established behaviors differ from assumptions commonly made about an
Arduino PID library: this calculation routine has no elapsed-time gate, and
this mode setter initializes the integral on either mode transition. Both are
retained. The caller controls when calculation runs. Calling the constructor
on fresh storage requires static/BSS zero initialization; reconstructing a
previously used object requires it to be disabled with otherwise valid state.
The application constructor explicitly disables both objects before their
constructors run.

## Application initialization

`PicoAppState` owns the controller, recipe, menu, screen state, session metadata,
carousel, and application flags. Pointer fields refer to those objects, or to
callbacks supplied by integration. SDK objects remain native SDK objects and
are passed through `PicoPlatformObjects`. Their internal layouts are not
invented here.

The application constructor performs these operations in the recovered order:

1. Construct two zero IP address objects for still communication.
2. Clear the first byte of both screen names and reset three screen fields.
3. Construct the HTTP client, register its destruction, and construct the RFID
   object with arguments 4, 18, and 0.
4. Empty the device UID, set device state 2, and construct the server address
   with four zero octets.
5. Clear all 15 carousel callback slots, select a 10,000 ms interval, and install
   the five supplied callbacks. Reset the progress fields written by the image.
6. Set still limits to 95, 5, and 10, and the communication failure limit to 5;
   clear the telemetry fields.
7. Disable both PIDs; select compartment 2 and idle phase 4; initialize wort
   temperature 70 °F, thermoblock temperature 83 °F, thermoblock target 300 °F,
   and both shuttle scales to 0.2; clear the observed phase/calibration counters.
8. Clear telemetry again, then construct the two PIDs with the parameters above.
9. Empty the 22-action menu; link the recipe to the owned session; initialize
   15 recipe steps with an empty name, location 2, and zero kind/duration/temperature.
10. Clear both 21-byte recipe identifiers; set ABV and IBU adjustments to -1;
    reset the current step and two metric values, buzzer flag, and stepper flags.

This is a partial constructor, not a general reset. In particular, it does not
clear the complete recipe name buffers, step count, carousel index, wort
setpoint, history arrays, PID integral/history, or every control flag. It relies
on startup zero initialization for fresh globals. The implementation preserves
those fields on reentry because the original does. Unknown fields are retained
under explicit reserved/unknown names rather than assigned speculative roles.
The clear-telemetry routine also preserves the limit settings and padding.

## Supplied carousel callbacks

| Slot | Original Thumb pointer | Recovered role |
| --- | --- | --- |
| 0 | `0x20009ac9` | Brew name page |
| 1 | `0x20009bc1` | Current recipe-step name page |
| 2 | `0x20009c25` | Recipe-step progress page |
| 3 | `0x20008969` | Controller phase and wort-temperature page |
| 4 | `0x20009be5` | Recipe metrics page |

The two entries at `0x20009bc0` and `0x20008968` were not independently indexed
in the earlier function export; following the constructor's callback table
identified them as callable entry points. The supplied function pointer array
keeps this order without embedding those original pointers. This state module
does not itself reconstruct the display implementations.

## Runtime construction boundary

The original constructor calls `0x2001ae90` with the HTTP client object, its
destructor `0x200156b5`, and DSO handle `0x200365b8`. The callee writes a runtime
destructor-registration table. The initialization interface exposes that
registration explicitly so integration can match the actual C++ object
lifetime. It is not silently discarded as a meaningless decompiler call.
The platform adapter is responsible for native object storage, construction,
and correct lifetime registration; those effects are controlled boundaries in
these tests.

## Validation and its limits

`test_state_pid.py` compiles the production C and a test-only serialization
fixture for a native comparison. With `--elf`, it repeats every case against
the linked ARM ELF in a separate Unicorn instance. The rebuilt ELF does not
contain or call the original image. The ARM comparison used GCC 10.3, Cortex-M4
Thumb, the soft-float ABI, actual newlib/libgcc arithmetic, and no fast-math.

There are 524 distinct cases: 12 application-initializer scenarios and 512 PID
cases across the eight recovered entry points. Each case passed both native
and rebuilt ARM execution, with zero mismatches. One pass executes 141,379
original instructions. The rebuilt instruction count includes its serialization
fixture and can vary with code placement; see the final run in
`analysis/state-pid-tests.json`. The initializer scenarios
cover genuinely zeroed startup state and patterned preexisting state, plus
ordinary, near-zero, and wrapping millisecond values.

For initialization, the tests compare the complete 8 KiB reference RAM window
through explicit field/pointer serialization and retain untouched bytes from
the initial snapshot. They also compare the order and arguments of all eight
platform/clock calls and the application state at each boundary. The original
initializer, PID helpers, telemetry clear, menu clear, and numeric instructions
execute; SDK construction and destructor registration are intercepted.

PID fixtures use selected finite values, exercise both modes and directions,
invalid negative tuning requests, limit clamping, mode transitions, and clock
wraparound. They compare the original 120-byte PID representation, including
preserved bytes, and the input/output/setpoint values. They do not establish
behavior for every IEEE-754 NaN payload, every possible invalid state, hardware
timing, interrupt races, network/RFID execution, or full application boot.

The native and target layouts of pointer-bearing state need not be identical.
The serialization is test-only. The production implementation uses normal
owned C objects, so pointer width changes do not turn an original RAM address
into an application dependency.
