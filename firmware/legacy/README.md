# Pico Pro 0.1.34 — readable C reconstruction

This folder is a source-oriented reconstruction of the Pico S/Pro firmware.
Open `src/application/main.c` to start following the program. Headers are in
`include/pico`; no Ghidra installation or browser is needed to read the code.

The project contains all **948 functions recognized in the analyzed image**,
organized into **79 named modules**, plus the recovered action tables.
**848 functions have identified or inferred names.** The other 100 retain
explicit `unresolved_…` names. The naming records include 1,312 reviewed
variable/parameter annotations; other names come from signatures, identified
calls, or neutral descriptions where the purpose remains uncertain.

This is not the original PicoBrew source tree. Most bodies remain annotated
decompilation, and some contain unresolved dataflow or inaccurate C types.
**28 routines received a deeper, manually reviewed C reconstruction.** Those
include the main loop, manual brewing, the API reader, the firmware receiver,
menu/text/encoder helpers, and seven PID methods. Each such routine is marked
`reviewed C reconstruction` in `analysis/function_map.json`.

## Start here

| File | What it explains |
| --- | --- |
| `src/application/main.c` | Setup, main menu, and dispatch into the application. |
| `src/application/brewing.c` | Manual brewing, brewing steps, recipe iteration, draining, and recovery. Start with `pico_manual_brew_program`. |
| `src/application/menus.c` | Menu construction and action selection. |
| `src/application/network_api.c` | Recipe requests, response framing, and session telemetry. Start with `pico_read_api_response`. |
| `src/application/firmware_update.c` | Update requests, firmware envelope decoding, chunk writes, and checksums. |
| `src/platform/pid_controller.c` | The reconstructed PID algorithm and gain/limit handling. |
| `include/pico/core_layouts.h` | Named menu, text screen, program, encoder, and page-carousel structures. |
| `include/pico/pid_controller_layout.h` | The recovered PID object layout. |
| `include/pico/action_ids.h` | Names for the actual action identifiers in the image. |

## Folder layout

- `src/application/`: brewing, controls, menus, screens, settings, network API,
  and firmware update behavior.
- `src/platform/`: Energia/Arduino support, TI drivers, SimpleLink, filesystem,
  RFID, display, startup, and compiler/library routines.
- `include/pico/`: module declarations, recovered structures, action IDs,
  address aliases, and explicitly identified analysis helpers.
- `analysis/`: address/name mappings, naming evidence, remaining ABI gaps,
  provenance, and compiler diagnostics. These files let a reviewer trace a
  readable name back to the original routine.
- `tools/check_sources.py`: a repeatable syntax and layout check.

All modules currently use `pico/firmware.h` as an umbrella header. This makes
cross-module dependencies explicit without requiring the unpublished PicoBrew
SDK. The reconstructed platform declarations describe calls seen in the image;
they do not constitute a recovered vendor development kit.

## What the names mean

SDK names were retained when identified. Application names were inferred from
strings, callers, callees, state changes, and data accesses. They describe the
observed behavior, rather than claiming to be original developer names.

The original address is kept near each function. The complete naming evidence
and confidence labels live in `analysis/function_map.json` and
`analysis/global_map.json`. The variable map records annotations and automatic
naming decisions from the decompiler stage; the 28 manually rewritten bodies
may use further improved local names.

Unknowns remain visible: `unresolved_…` functions, neutral `value_…` locals,
unknown structure fields, and unresolved register/flag inputs. A label alone
does not establish a function signature or behavioral equivalence.

## Reconstruction status

**This project is for reading and further reverse engineering. It cannot yet
be linked into a trustworthy firmware image.**

- Sixteen expressions use `PICO_UNRESOLVED_RETURN(...)` because a caller appears
  to consume registers after a logically void call. The marker has no
  implementation. Replacing it with a success value would fabricate behavior.
- Other register and condition-flag inputs remain unresolved. For example,
  `pico_receive_firmware_image` still has two unresolved destination-open inputs.
  The sixteen markers are not a count of all remaining analysis gaps.
- The low-level floating-point and numeric-conversion code contains known
  differences between C numeric casts and the bit operations represented by
  the original instructions. See `analysis/runtime_float_audit.md`. It is not
  a usable replacement math library.
- Some pointers still have incomplete types. Absolute memory aliases in
  `include/pico/memory.h` describe the original device memory, and are not host
  variables. Multiple views of the same address preserve byte versus word
  indexing while the exact object layout is being recovered.
- Processor-operation declarations in `recovered_types.h` still require
  appropriate ARM implementations for any future target build. There is no
  complete linker script, startup integration, or firmware build target here.

Reviewed rewrites preserve the observed firmware behavior, including quirks.
The API reader retains its original check after writing; OTA retains its
delimiter order, ignored open result, and some duplicate close paths. The PID
compute routine has no elapsed-period guard, and mode changes initialize the
integral state in either direction. These details were not silently “fixed.”

## Check the source structure

On a system with Python 3 and GCC supporting the 32-bit compilation model:

```sh
make check
```

Or run `python3 tools/check_sources.py`. The check parses every C translation
unit, rejects missing function declarations, verifies selected recovered
structure sizes/offsets, and checks the 948-entry function index. It does not
link or execute the code. `-m32` is used to check 32-bit pointer layouts on the
host; this is not an ARM cross-compilation.

The check deliberately retains compiler diagnostics and allows incomplete
pointer/integer conversions with `-Wno-error=int-conversion` and
`-Wno-error=incompatible-pointer-types`. Therefore a passing check is **syntax
and structure validation**, not a warning-free build or semantic verification.
The exact flags and all diagnostics are in `analysis/syntax_check.json`.

## Image and provenance

- Firmware: Pico S/Pro 0.1.34, CC3200, little-endian ARM Thumb, software floating
  point calling convention.
- Payload: 162,792 bytes; load address `0x20004000`.
- SHA-256: `e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702`.
- Recorded build: December 16, 2019, 12:59:40; branch `dev`; commit
  `d0da766714f2012e98f8e43ecd6519895caf565e`.
- Upstream Energia reference commit:
  `c5507401fe46454d38c3ce3af0a140df8fc088a3`.

The previous analysis archive remains the reference for the original payload
and Ghidra project. This folder focuses on readable source. Provenance and SDK
match records are copied into `analysis/provenance/`. Reconstructed proprietary
code is not assigned a new upstream license; matching third-party notices are
retained under `LICENSES/`.
