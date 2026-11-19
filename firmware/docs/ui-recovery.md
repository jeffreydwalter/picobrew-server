# UI, display and input recovery

The UI domain has concrete C implementations for all **257 indexed application
entries** in its final scope, **17 additional recovered entries** that were
missing from the previous function inventory, and **26 graphics dependency
entries** replaced by the owned Pico display implementation. The per-entry
inventory is the [UI entry ledger](../analysis/recovery/ui.json). No entry in
this scope is left as an undefined declaration or a success-returning
placeholder.

These are reconstructed implementations with descriptive names. They are not
claimed to be the original source files or the original programmer's symbols.
The application does not call into the original firmware image or address its
original RAM globals. Original addresses occur only in analysis/test data and
the extraction tool.

## Ownership and integration

`include/picobrew/ui.h` defines `PicoUi`, `PicoDisplay`, `PicoEncoder`, the concrete
public screen/menu APIs, and two explicit service contracts. `PicoUi` owns the
1,024-byte monochrome framebuffer, encoder state and 22 menu labels. It refers
to the application's typed program/controller/screen/carousel state, the
network/settings context, and the actual actuator/control objects.

`PicoUiHardware` supplies GPIO, SPI, time and interrupt operations. These are
real hardware boundaries; OLED command generation, framebuffer expansion,
encoder debounce and quadrature decoding are in recovered C. `PicoUiServices`
supplies recipe/device operations such as running a rinse, moving the valve,
sounding the buzzer and waiting for the filter. Top-level application
composition binds these to the corresponding recovered modules. The UI does
not simulate successful brewing or network operations.

There is one explicitly bound `PicoUi` context because the original appliance
has one display and encoder and its interrupt callbacks have no user-data
argument. `pico_ui_initialize()` binds that context. This is not a multi-device
or reentrant UI API. Source filenames in the table below are relative to
`src/ui/`.

| Area | Source files | Responsibility |
| --- | --- | --- |
| Graphics and immutable data | `graphics.c`, `assets.c` | Native font, bitmaps, clipping/rotation, geometry, glyphs and OLED command protocol |
| Display and pages | `display.c`, `display_values.c`, `pages.c` | Rows/frames, temperatures, timers, progress, carousel pages and downloaded recipe images |
| Input and menus | `encoder.c`, `encoder_editor.c`, `menus.c`, `ui_menus.c` | Rotary/button handling, text entry, adjustment controls, selection/scroll/timeout loops |
| Application screens | `screens.c`, `ui_screens.c`, `instruction_screens.c`, `ui_workflows.c`, `workflow_adapters.c` | Localized screens, prompts, instruction actions, WiFi/settings/recipe choices |
| Labels and actions | `actions.c`, `action_tables.c`, `strings.c`, `translations.c` | Action IDs, labels, instruction names and program titles |
| Diagnostics and PicoStill | `diagnostics.c`, `still_ui.c` | Concrete service/diagnostic loops, alarms, error reporting, fatal shutdown and still UI |

The fatal heater handler enables interrupts, disables controller activity,
requests the real actuator shutdown function, activates the buzzer, disables
interrupts, draws the malfunction screen, reports error 24, and does not return.
Tests observe that order at the actuator/API boundaries. They do not establish
the physical behavior of an attached appliance.

## Verification

The local Cortex-M4/Thumb soft-float validation ELF links with newlib-nano,
`-Wall -Wextra -Werror`, and no undefined symbols. It links the recovered UI and
the real network/control dependencies. External fixture callbacks trap unless
the individual test explicitly supplies an observation hook.

| Suite | Passing cases | Evidence |
| --- | ---: | --- |
| Rendering, static screens and numeric output | 643 | `analysis/ui-tests.json` |
| Encoder, menus, OLED/GPIO ordering and callbacks | 668 | `analysis/ui-interactions-tests.json` |
| Error screens, instruction actions/sequences, text editor, alarms and fatal paths | 396 | `analysis/ui-workflows-tests.json` |
| Remaining display entries and owned graphics replacements | 1,055 comparisons + 9 bounds-repair checks | `analysis/ui-direct-display-tests.json` |
| Remaining menus, diagnostics, PicoStill and service dispatch | 306 | `analysis/ui-direct-menus-tests.json` |
| Remaining workflow and network/recipe UI adapters | 543 | `analysis/ui-direct-workflows-tests.json` |
| Immutable bytes, bitmap descriptors and pointer tables | 52 | `analysis/ui-assets-tests.json` |

The six behavioral suites record **3,611 comparisons with zero failures**, plus
**9 separately identified bounds-repair checks**. Direct cases execute the
pinned original image and the newly linked ELF in separate Unicorn instances.
Depending on the routine, comparisons cover
all framebuffer bytes, cursor/color state, sequences of flushed frames, menu
labels, encoder state, return values, call arguments and GPIO/SPI/time/interrupt
event order. They include all 256 input character values, all 25 instruction
screens in three languages and two title modes, all stored error headings,
128 quadrature-state combinations, menu timeout/filter/still branches,
rotary-editor control characters, non-returning fatal paths, every diagnostic
and utilities-menu dispatch, WiFi scan/retry/cancellation paths, recipe catalog
pagination, associated PicoPak selection, recipe adjustments and drain toggling.

**All 257 indexed application entries have direct original-versus-rebuilt ARM
cases.** The three additional suites close the previous 119-entry gap. Each
ledger entry identifies its report, case function and case count; no indexed
entry is classified only as source-reviewed and compiled. Entry coverage does
not imply that every branch, possible input or asynchronous interaction has
been exercised.

The **17 additional entries and 26 graphics dependency entries** also have
behavioral evidence. Their evidence methods remain explicit. Some former SDK
helpers are replaced by composed rasterizer operations, an actual inlined
caller body, or public fields in the owned display object. Those are recorded
as such, rather than being called standalone replacement function tests. The
unused graphics-base inversion method is an observed original no-op whose
replacement is deliberately elided; no replacement machine body is claimed.
Invalid action/title ID checks are recorded as repairs, separately from
equivalent behavior on valid inputs.

The tests are function-level analyses with explicit boundary hooks. They are
not a full appliance simulation and do not verify physical display timing,
encoder bounce on an actual knob, WiFi infrastructure, pumps, mains power or
thermal performance. The validation ELF is a test fixture and is not an image
to install on a machine.

From the standalone project, the suites accept the unified validation ELF and
an explicit output location:

```sh
python tests/test_ui.py --elf build/application-validation.elf --output analysis/ui-tests.json
python tests/test_ui_interactions.py --elf build/application-validation.elf --output analysis/ui-interactions-tests.json
python tests/test_ui_workflows.py --elf build/application-validation.elf --output analysis/ui-workflows-tests.json
python tests/test_ui_direct_display.py --elf build/application-validation.elf --output analysis/ui-direct-display-tests.json
python tests/test_ui_direct_menus.py --elf build/application-validation.elf --output analysis/ui-direct-menus-tests.json
python tests/test_ui_direct_workflows.py --elf build/application-validation.elf --output analysis/ui-direct-workflows-tests.json
python tests/test_ui_assets.py --elf build/application-validation.elf --output analysis/ui-assets-tests.json
```

They use the project's `tools/original_arm.py` and `tools/rebuilt_arm.py`, plus
Unicorn and pyelftools. `tests/fast_arm.py` restricts emulator hooks to observed
entry points, and the direct suites use compiler DWARF where needed to locate
owned fields and callback tables without guessing their layouts. The original inventory files are
`tests/ui_original_entries.json` and `tests/ui_original_inventory.json`.

## Native data and reproducibility

The image is `reference/original-0.1.34.bin`, SHA-256:

```text
e18c240fa1cf35222674bf3b4fa992a306c6e63b0b99be8793604f199085d702
```

All **44 font/bitmap arrays, totaling 8,836 bytes**, are native extracted bytes.
The asset test checks every compiled byte against the pinned image, all six
bitmap descriptors against their owned arrays/dimensions, and the digit/setup
pointer tables. `analysis/ui-assets.json` records source locations, dimensions,
sizes, SHA-256 hashes and the extraction traces. This is not an approximation
of the Pico graphics with a replacement desktop font.

`tools/ui_assets/extract_assets.py` regenerates `src/ui/assets.c`,
`src/ui/ui_assets.h` and `analysis/ui-assets.json` from the pinned image.
Besides byte arrays, it recovers 75 instruction-text variants, thermal-recovery
text, error headings and localized title/state tables. Native non-ASCII bytes
are emitted with octal escapes so the C compiler cannot silently reinterpret
them as different UTF-8 sequences. The extractor is idempotent and changes no
reviewed function implementations.

## Intentional changes and preserved behavior

[UI repairs](ui-repairs.md) lists the explicit bounded-input repairs separately from
corrections to the decompilation. A few surprising original behaviors are
preserved in this baseline: refresh redraws carousel page zero; encoder step
storage does not scale rotation; additional recipe images share the first-image
callback and the network side increments page count twice; alarm timeout uses
the original absolute unsigned deadline. Those behaviors are suitable for
separate community changes with their own tests, rather than undocumented
differences in the reconstruction.
