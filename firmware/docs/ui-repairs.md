# UI reconstruction changes and historical quirks

This records changes in the UI domain relative to the original image and
corrections relative to the earlier decompiler output. Neither category should
be mistaken for recovered original source spelling.

## Explicit repairs of invalid or oversized input

| Change | Result |
| --- | --- |
| Reject action ID 50 and larger before indexing the 50-entry action tables | Removes the original off-by-one out-of-bounds action-name lookup; valid IDs remain 0–49 |
| Reject title ID 5 and larger before indexing the five localized title rows | The original inclusive enum-count check reads framebuffer bytes as string pointers for ID 5. All original call sites use IDs 1–4; valid table IDs 0–4 remain unchanged. The earlier extraction's fabricated sixth empty row has been removed. |
| Bound scanned-network rows to 20, reserving two of 22 rows for manual entry/rescan | Oversized or negative scan results cannot overwrite adjacent menu state |
| Bound remote SSIDs, recipe labels/identifiers and still identifiers to their owned destination capacity | Original plain `strcpy` paths are replaced with explicit NUL-terminated bounds handling |
| Check zero matching recipes and limits while constructing associated-recipe choices | Prevents invalid index arithmetic and menu writes on malformed/empty network results |
| Bound in-brew label population and downloaded-image callback assignment to owned arrays | Malformed counts cannot write outside application-owned menu/carousel storage |
| Use sufficiently sized local formatting/label buffers | Avoids stack overwrites from multilingual strings or formatted values whose inferred decompiler buffers were too small |

These guards affect invalid/oversized inputs. Normal-case display behavior is
checked against original ARM execution where a test is listed in the ledger.
No claim is made that every invalid-input combination has been exhaustively
tested.

## Corrections to decompilation artifacts

The display APIs now accept typed `double` values where the ARM soft-float ABI
passes a 64-bit value across registers or stack words. AAPCS alignment padding
is not exposed as a fictitious function parameter. Numeric conversions follow
the actual single/double precision operations and integer truncation. In
particular, the decimal formatter multiplies the single-precision remainder by
one integer-derived power of ten; repeated multiplication by ten is not
equivalent and was rejected by differential tests.

The instruction renderer has real variadic line handling and owned localized
tables. Native bitmaps that the decompiler printed as string literals have
actual const byte arrays and typed width/height descriptors. Encoder ISR
wrappers and previously unnamed carousel callbacks have concrete readable
functions. UI-owned error rendering includes the original clear, localized
code-entry prompt, headings, large error number, help URL and separator.

The menu loop's original unsigned `(status - 9) < 2` check is kept unsigned; a
literal translation using integer promotion of an unsigned byte would
incorrectly treat states below 9 as blocking still states. This is correction
of a reconstruction error, not a firmware behavior change.

Direct menu and diagnostic comparisons also corrected two mistakes in the
earlier reconstruction:

| Correction | Original behavior restored | Evidence |
| --- | --- | --- |
| Restore the utilities menu's recorded action order | Action IDs are `40, 41, 42, 43, 44, 45, 46, 48, 47, 39`: Circulate appears before PicoStill, followed by Back. Sorting the IDs numerically had swapped those two options. | `analysis/ui-direct-menus-tests.json`, `pico_utilities_menu` cases |
| Preserve the buzzer diagnostic's return value | `pico_diagnostic_buzzer()` returns the result of the final buzzer-off operation. The typed `PicoUiServices.buzzer` callback now carries that boolean result through the application binding. | `analysis/ui-direct-menus-tests.json`, `pico_diagnostic_buzzer` success/failure cases |

These changes restore observed software behavior. The buzzer comparison checks
the request sequence and returned status; it does not establish physical sound
or communication with a connected auxiliary controller.

Original CString call sites use the shared `pico_text_copy()` and
`pico_text_is_empty()` implementations. Their warning logs and copy counts are
preserved, with logging bound to the real network logger. The separate local
bounded-copy helper is used only for the plain-copy repairs described above.

## Preserved original behavior

| Behavior | Evidence and consequence |
| --- | --- |
| `pico_display_refresh_current_page()` calls `pages[0]` | Original entry loads the first callback; the historical inferred name is retained, with a source comment |
| Additional fetched image callbacks all draw the first extra-image buffer | Original jump-table branches select the same callback; UI preserves that callback, while networking preserves its double page-count increment |
| Rotary movement changes position by one even if `step` is different | The original stores `step` but does not use it in the quadrature position update |
| Encoder initialization samples previously configured phase pins before storing new pin arguments | Exact original GPIO call order is preserved and tested |
| Button debounce disables interrupts, waits 20 ms, samples active-low input, then enables interrupts | Both high and low input traces are compared |
| Timeout alarm uses an absolute unsigned deadline and 750 ms on / 4,000 ms off timing | Original deadline behavior and buzzer pulse order are preserved |
| An associated-recipe menu can treat its last returned choice as cancellation | Original selection/count behavior is retained and noted at the reconstructed call site |
| Stored localized text includes historical spelling and wording mistakes | Extracted strings are not editorially corrected as part of behavior reconstruction |

The scope here is software recovery and reproducible analysis. Physical
appliance commissioning, electrical verification and thermal validation remain
separate from these emulator results.
