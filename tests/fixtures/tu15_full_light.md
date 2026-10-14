# Combined OG Xbox TU1.5 light fixtures

These local fixtures are generated from the owned unchanged TU1.5 executable,
Lockout Xbox cache, and authenticated `xbox-tu15-frag-capture-v7` RAM snapshot.
They are not Vista references. Expected outputs come from original execution.

| File | Original owner |
| --- | --- |
| `tu15_full_light_commands.original.json` | Whole secondary 44AB0 after original C35D0/31C70 and the declared 40E80 reset. All command storage, six actual model palette inputs, special bank and 263 ordered calls. |
| `tu15_full_light_selection.original.json` | Original 3BAA0 layer 12 sort, retaining models and map geometry in the same sequence. |
| `tu15_full_light_ranges.original.bin` | Original 4D5D2..4D616 / whole 4DA90 indexed range selection, intercepting only final device flush and submission. |

Generation tools: `tools/oracle_tu15_light_model_commands.py` and
`tools/oracle_tu15_effect_light_draw_ranges.py --include-models`. Use the local
`.tools/python/bin/python` environment containing Unicorn. Pass the owned TU1.5
XBE, authenticated snapshot, and `assets/disc/maps/lockout.map` explicitly.
Never accept an empty command trace as an oracle result.

Registered check: `original_tu15_model_effect_light_ranges`. It verifies native
command outputs/call order, 12 sorted records, 28 indexed submissions and every
one of 1,303 selected indices. Current palette inputs are retained only through
the center node consumed by 45CD0. Complete collector, pose/palette production,
live receiver admission, material binding and pixel equality remain separate
requirements. No-op model draws do not prove visible illumination.

See `docs/TU15_MODEL_LIGHT_RECEIVERS_20261013.md` and the adjacent local
`tu15_full_light.provenance.json` for identities, artifact paths and limits.
