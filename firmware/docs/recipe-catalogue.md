# Local PicoPak recipes

## Manual Brew and Brew PicoPak

The reconstructed stock implementation keeps separate workflows:

- **Manual Brew** asks for temperature and hold time and lets the user choose
  the next step interactively. See `pico_recipe_manual_program` in
  `src/recipe_programs.c` and its original `pico_manual_brew_program` in
  `legacy/src/application/brewing.c`.
- **Brew PicoPak** selects a saved program, follows preparation prompts, and
  runs the ordered compartments, temperatures, holds and drains automatically.
  Stock recipe selection used `getAssociatedPaks`/`getRecipe`; standalone
  `src/network/api.c` reads the local recipe store for the same menu instead.

Neither menu is renamed. The starter set supplies the real local slots used
by the front panel, device browser and Hub installed-recipe editor.

## Starter set

| Stored ID | Native program |
| --- | --- |
| `pico-pak-64` | Pico Pale |
| `pico-pak-17` | ENIAC Mosaic IPA |
| `pico-pak-20` | Dead Guy Ale |
| `pico-pak-6` | Party Porter |
| `pico-pak-4` | Tweaties American Wheat Ale |
| `pico-pak-43` | Saison Blanc |
| `pico-pak-46` | Buffalo Craft Lager |
| `pico-pak-7049` | Shankill Stout |

These are the same IDs as the Hub catalogue's featured PicoPak programs.
Names, compartment numbers, temperatures, hold times and drain times are
unchanged from `picopak-library` commit
`93a9e104ba696a56f667050eae8e969e0b732e36`. Original PicoBrew programs and the
tmack8001 archive are attributed in `data/starter-recipes.json`, which records
each source path and original file hash. The source repository supplies no
repository license. `tools/generate_starter_recipes.py` creates the compact
compiled representation; its `--check` gate catches stale generated data.

The archive supplies machine programs, not ingredient bills. It does not
justify inventing grain quantities, water setup or a batch size. A matching
PicoPak or reviewed equivalent setup is needed to brew one of these programs.
Zero-minute heating steps wait for the target temperature then advance (or
drain), without adding a hold. They are preserved rather than converted to
one-minute steps.

## Migration and editing

There are 16 recipe slots. An empty store receives the eight starters on its
first 1.1.0 startup, leaving eight free slots. A nonempty older catalogue keeps
all of its recipes, with no insertion or replacement. A CRC-protected flag is
committed with the catalogue to mark this migration handled.

After that first migration, edits and deletions stay in place, including an
intentionally empty collection. **Restore starter recipes** explicitly adds
missing starter IDs. It never overwrites matching IDs or deletes other recipes.
All missing starters must fit before any write starts; insufficient space
returns 507. The local browser and Hub use the same GET/PUT/DELETE slots.

The seed installer verifies an empty rollback catalogue before writing a
pristine store's recipe records, then commits the entire visible set once.
Interrupted record writes and interrupted final catalogue commits can retry
from the committed prior catalogue. A corrupt sole initial bank is preserved
for recovery rather than treated as an empty device. Storage read errors also
stop migration without replacing files.

The new catalogue flag extends the old on-disk header. A downgrade to 1.0.x
cannot reliably read an upgraded catalogue; preserve a suitable backup when
planning a downgrade.

## Interrupted brewing

Standalone checkpoints use two CRC-checked records and bind to the complete
original program. At boot, a valid checkpoint offers continue or abort.
Displaying that choice leaves outputs off and does not start a session or
history recording. Continue requires a matching current recipe and restores
the remaining hold/drain time. A resumed drain does not heat the wort or add
the pass-through drain compensation a second time. Abort writes a tombstone;
missing, modified or corrupt programs and old `restart.txt` records do not
automatically start anything.

The stock interlocks and explicit preparation flow remain. Host and ARM tests
exercise software boundaries; physical brewing and power-loss behavior have
not been verified for this release.
