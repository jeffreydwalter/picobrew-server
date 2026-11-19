# Equipment profiles and sources

Equipment profiles supply starting values for recipe planning. They are bundled
with the Hub and remain available without an internet connection. Opening a
source link is optional; the Hub does not scrape websites or call a cloud service
when selecting a profile or adapting a recipe.

## Pico C, S and Pro defaults

| Field | Default | Basis |
|---|---|---|
| Batch size | 5 L | Nominal batch size, supported by the manufacturer specifications for C/Pro and the Small Brew Companion C/S/Pro recipe guide |
| Planning efficiency | 50% | Small Brew Companion's approximate community estimate, not a measurement of the connected brewer |
| Grain capacity | Unknown (`0`) | No verified maximum for the actual grain container has been established |

The [Small Brew Companion scaling guide](https://www.smallbrewcompanion.com/docs/scale-recipe-to-pico-c-s-pro-manual-steps/)
uses a 5 L batch and approximately 50% efficiency for C/S/Pro recipes. Its
[recipe-details guide](https://www.smallbrewcompanion.com/docs/recipe-details-overview/)
also identifies this as an estimated efficiency. These are community planning
references, not PicoBrew calibration specifications.

The manufacturer's [Pico Pro specifications](https://picobrew.com/store/products/pico)
and [Pico C specifications](https://picobrew.com/store/products/picoc) separately
specify a nominal 5 L batch. A brew keg's stated volume is not the recipe's batch
volume. Neither specification establishes the efficiency or maximum grain weight
used by a particular reusable basket or brew bag.

The word "Manual" in the SBC scaling guide's title refers to the conversion
steps in its interface. It does not limit that estimate to the appliance's
bag-based Manual Brew program. Its associated brewing-profile guide describes
PicoPak-style compartment routing. The actual setup, ingredients and process
still require individual calibration.

These sources were checked on 2026-11-18. Manufacturer web pages may be
intermittently unavailable; retaining their references does not make them a
runtime dependency. Z and Zymatic defaults have not changed in this update.

## Saved calibration takes priority

Saving a calibration replaces the three planning numbers for that model. It
does not change the native command capabilities or manufacture measurements that
the Pico does not report. The current calibration applies to all registered
machines using that model profile.

`GET /api/v1/profiles` includes:

- `sources`: labeled links and descriptions identifying the published starting
  points. These remain visible when a saved calibration supplies different
  numbers.
- `user_configured`: a server-computed flag that is true when an override has
  been saved. It is not proof that the numbers were measured.

The profile write endpoint accepts only `batch_liters`, `efficiency` and
`max_grain_kg`. Clients cannot write or override the provenance fields.

Existing saved calibrations retain their values, including an efficiency of
70% saved with an earlier release. Existing recipes, revisions and deployed
machine programs are not rewritten. Resetting a C/S/Pro profile explicitly
restores the new 5 L / 50% / unknown-grain defaults and clears `user_configured`.

For an unchanged source recipe and batch volume, adapting at 50% instead of 70%
increases the calculated grain requirement by 40%. Review the adapted quantities
and the actual container capacity before deployment. `0` grain capacity means
unknown; it does not claim that an arbitrarily large grain bill will fit.

## What the machine supplies

The current standalone API reports its firmware/API version, UID, supported
commands, recipe/step capacities, history support and live controller readings.
It does not supply batch-volume, gravity, grain-mass or extraction-efficiency
measurements, or an S-versus-Pro model identifier. The Hub uses the model chosen
at machine registration.

Measured efficiency needs brewing observations beyond the current device
telemetry. User-supplied measurements should replace the planning estimate when
available; a published profile is a starting point for that process.
