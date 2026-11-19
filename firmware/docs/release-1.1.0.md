# Firmware 1.1.0

This release fills an empty **Brew PicoPak** catalogue with eight real archived
programs, supports zero-minute heating steps, and lets users restore missing
starters from the device browser or REST API. Firmware and Hub use identical
stable IDs. Existing recipe collections and later edits/deletions are preserved.
The REST namespace remains `/api/v1`; discovery advertises the new capabilities.

Interrupted-brew checkpoints now carry a checksummed program signature and
phase. Boot offers an explicit continue/abort choice without starting hardware
or recording a new brew. Missing or changed recipes cannot silently resume;
drain restoration preserves remaining time and avoids repeated compensation.

The ARM image is 165,656 bytes. The original 32 KiB stack and 32 KiB heap remain
reserved, with 656 bytes of unallocated SRAM. Compact starter tables, shared
identical OLED images and DMA-table alignment placement keep this within the
device's memory budget. The browser bundle occupies eight serial-flash blocks.
This is a link budget, not a measurement of runtime stack/heap peaks.

The packaged image and five browser files must be installed together. The
guarded installer pins their hashes and accepts the known earlier candidate,
1.0.0 and 1.0.1 as upgrade sources. No installation or hardware operation was
performed while developing this release.

Validation covers migration, repeated boot, edited/deleted starters, capacity
failures, interrupted batch/record writes, native ARM filesystem retry behavior,
interrupt/filter-trip guards, and checkpoint identity/tombstone semantics.
Browser tests are included for restoring and editing the starter set. Physical
brewing, real power interruption and this image's installation remain untested.

See [catalogue behavior](recipe-catalogue.md) and the [API guide](standalone-api.md).
