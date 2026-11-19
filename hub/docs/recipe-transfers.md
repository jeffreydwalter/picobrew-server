# Recipe and program transfers

The community catalogue is immutable. **Copy to library** creates a separate
editable record and retains source provenance. The library is independent of
each brewer's installed recipe slots. The machine page and the brewer's own
browser interface both read and edit those same installed slots.

## Ingredient recipes and native programs

Ingredient recipes have a batch volume, efficiency, ingredient quantities and a
reviewed plan. Adaptation applies the selected machine profile and validates
explicit compartment assignments. PicoPak machine programs retain exact source
temperatures, hold times, drains and routing. They have no inferred batch size
or ingredient quantities, and transfer with `batch_liters: 0`.

Native IDs are preserved. For example, the Pico Pale starter has the same
`pico-pak-64` ID in firmware and catalogue copies. Copying it into the Hub library
does not create another appliance slot until a transfer is explicitly confirmed;
an identical installed program is skipped.

Zero-minute native steps are kept as zero. The Hub checks the firmware's
`zero_minute_steps` capability and will not silently change such a step to a
one-minute hold for an older firmware. Source labels that exceed the brewer's
ASCII/length limits require an explicit edit.

## Preview and confirm

Select library entries and choose a connected idle brewer. A transfer preview
reports its advertised capacity, occupied and available slots, new entries,
proposed updates, identical programs and skipped entries. It never deletes an
installed recipe. Matching IDs are preserved unless the update option is enabled.

Pico S/Pro standalone firmware provides 16 recipe slots with up to 15 steps per
program. The Hub reads the device's advertised limits rather than assuming other
models use these values. Maintenance programs use the same source-model and
capacity rules. The archived Zymatic cleaning programs cannot be sent to a Pico;
they remain reference/export records until a compatible live adapter exists.

Before execution, the Hub verifies device identity, idle state, current profile,
recipe revisions, current inventory and firmware capabilities against the
preview. Previews expire after 15 minutes. If those inputs change, prepare a new
preview. Each accepted write is read back before continuing to the next entry.
No transfer starts a brewing or cleaning program.

## Interrupted transfers

Receipts are stored before sending and after each verified write. A transport or
verification failure stops the batch. An unconfirmed write may have reached the
machine; it is not automatically retried or rolled back. Refresh the installed
inventory and prepare a new preview to resolve the remaining work.

After a Hub restart, an in-progress receipt becomes `interrupted`. Already
verified entries stay verified, an in-flight entry becomes `unconfirmed`, and
remaining entries are `not_sent`. The browser remembers the last receipt ID per
machine and can reopen it. A completed transfer can be read again without
repeating its writes.

## Editing an installed slot

The installed-program editor submits the expected previous program along with
the requested update. The Hub reads the machine again and rejects an edit if the
previous program has changed or disappeared. It verifies the written program by
readback. The firmware has no conditional-write API, so an independent client
that writes after this comparison can still race with the final write. Avoid
editing the same slot in both browser interfaces simultaneously.

## API

All actions use the existing owner session and CSRF rules.

| Method and path | Purpose |
| --- | --- |
| `POST /api/v1/devices/{id}/deployments` | Create a persisted preview from `profile_id`, `overwrite_existing`, and `items` containing library `recipe_id`, `source_revision`, and `batch_liters`. |
| `GET /api/v1/devices/{id}/deployments/{deployment_id}` | Read the preview or current receipt. |
| `POST /api/v1/devices/{id}/deployments/{deployment_id}/execute` | Execute the accepted preview once, using an empty JSON object. |
| `PUT /api/v1/devices/{id}/recipes/{recipe_id}` | Edit an existing slot using `recipe` and `expected` machine-program objects; the ID must stay unchanged. |

The authenticated OpenAPI document includes the request and response schemas.
