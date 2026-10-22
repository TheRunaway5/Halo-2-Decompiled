# PlayStation companion prompts

The editable artwork in `resources/presentation/prompts-playstation` matches
the revised Xbox bank in `resources/presentation/prompts-xbox`. The earlier
masters in `resources/presentation/prompts` remain a preserved reference.
The face buttons use charcoal bodies with blue cross, red circle, pink square
and green triangle symbols. Both revised banks retain the circular outlines,
bevels and white top highlights. Only the colored left-side reflection on
PlayStation face buttons is removed; Xbox lighting stays as authored. Xbox RB now uses a
1-unit interletter gap, matching LB; R is wider than L, so B moves from
7 to 8.7 units. Shoulders retain the original
blue material with dark L1/R1 lettering; the retained trigger silhouettes
carry L2/R2 labels enlarged from 72% to 110% of the shoulder lettering size.

The PlayStation bank contains 331 selector/control bindings, 88 bound SVG variants,
one supplementary touchpad SVG and six lossless RGBA DDS atlases. Every
binding retains the Xbox bank's selector, codepoint, logical canvas and
fourfold raster dimensions. `prompts.tsv` uses the existing raster-table
format. `reconstruction.json` records the control mapping and source hashes;
`manifest.json` records every output file's size and SHA-256.
The revised Xbox bank has its own 331 bindings, 94 editable SVG variants and
six atlases. Aliases keep their existing selector/codepoint identities.

| Existing slot | PlayStation artwork |
| --- | --- |
| A / B / X / Y | Cross / Circle / Square / Triangle |
| LB / RB | L1 / R1 |
| LT / RT | L2 / R2 |
| Start / Back | Options / Share-Create pictograms |
| Left / right stick press, E10E / E10F | L3 / R3 |
| Movement / look, E110 / E111 | L / R stick silhouettes |
| Directional arrows | Existing directional artwork |
| Keyboard frames, lock, four-pane special glyph | Existing artwork |
| Supplementary touchpad | Standalone master; no invented codepoint binding |

The movement/press distinction follows `src/training_controls.cpp` and
`src/action_prompt_bindings.cpp`. Other stick aliases retain the original
poses and side designation. No replacement font, embedded bitmap, filter or
external resource is used in the individual prompt masters. The preview
compositions use ordinary text for their captions.

Open `buttons.svg` or `buttons.png` for the set, and `comparison.svg` or
`comparison.png` for Xbox and PlayStation artwork side by side. These are
custom companion drawings, not recovered Sony artwork or framebuffer parity
references.

Regenerate and verify from the preserved SVGs without a game installation:

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/recreate_playstation_prompts.py --platform xbox \
  --output resources/presentation/prompts-xbox

HALO2_XBOX_PROMPT_PREVIEW="$PWD/resources/presentation/prompts-xbox" \
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/recreate_playstation_prompts.py \
  --output resources/presentation/prompts-playstation

python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/recreate_playstation_prompts.py \
  --verify resources/presentation/prompts-playstation
```

Regeneration stages and validates the bank in an owned temporary directory.
It replaces only an authenticated, unmodified output from this generator;
hand edits or unrelated destination files cause a failure instead. Validation
checks all binding keys and extents against the Xbox bank, replays all 88
bound saved SVGs through librsvg, decodes the six DDS atlases independently
with Pillow, and compares every bound rectangle byte for byte. The touchpad
is rendered separately. The previews have also been visually inspected.

The intentionally retained requested artwork, SVG/PNG previews, metadata and
atlases total 40,077,401 bytes (38.22 MiB) across both revised banks. Intermediate
renders and generated test builds use the disposable storage workflow.

`PlayerPromptDevices` follows actual SDL instance ownership per local view.
SDL's PlayStation controller types and Sony vendor identity select PlayStation;
other normalized controllers select Xbox. Meaningful input changes only its
owner's selection. Keyboard/mouse events are restricted to the game window and
the actual KBM view. Idle motion, dead-zone noise and unrelated controllers do
not steal prompts. Rebinding unchanged devices preserves last-used input;
disconnects affect only their owners. Runtime controller assignments retain
instance IDs when SDL compacts its device list, filling only vacant controller
views on replacement.

Fresh vanilla onboarding and existing-cache reuse both install the two banks
under `presentation/prompts-xbox` and `presentation/prompts-playstation`.
Each receives six atlases, its control table, relative binding and a bounded
runtime manifest: 18 installed files in total, with hashes recorded in
`provenance.json` under `controller_prompts`. The SVG masters and preview sheets
stay in the port resources. Missing defaults are repaired; authenticated old
defaults can be upgraded. Modified or extra user files cause an error before
either bank is changed. No copied MCC map or font is replaced.

The renderer prefers the selected vanilla cache's banks. The portable setup
resources remain a fallback for source/reference contexts. The C++ regression
also selects a freshly installed vanilla bank with setup resources explicitly
unavailable, so onboarding's copies have a tested runtime consumer.

`ControllerPromptArtwork` authenticates the control table and DDS hashes and
prepares immutable texture/material records before GPU upload. HUD drawing
chooses the family per view without changing ordinary font pixels, logical
metrics, bindings or source gameplay. Action prompts and campaign training use
the same owner. Menus retain the focused local player's family; keyboard
captions retain their original pointer actions. Pause menus follow the player
who opened them. Linux and macOS packagers include both portable banks.

Validation status is kept separate from physical-controller and full-game
acceptance:

| Requested behavior | Evidence |
| --- | --- |
| Remove only PlayStation colored side glare; keep white top highlights | Direct SVG checks; viewed comparison |
| Equal Xbox RB/LB spacing | Direct interletter extent regression |
| Larger PlayStation L2/R2 labels | Direct label-scale regression and viewed render |
| Per-player device choice | Four-player routing, isolation, drift, reconnect and SDL virtual-controller checks |
| Matching actual HUD quads | Four mixed local views at 640x480, 1280x720 and 3840x2160; 448 additional Xbox/PlayStation control draws; installed-cache selection passes |
| Native build | Clean native build and all four focused CTests pass |
| Packaged assets | Actual copy, hash verification and tamper rejection pass |
| Vanilla onboarding assets | Fresh/reuse integration, missing-file repair, upgrade and custom-file preservation pass |

The final native build/CTest report is
`build/test-results/controller-prompt-vanilla-final/report.json`. The Python
onboarding, fresh-preparation and presentation-dispatch suites pass 39, 24
and 22 cases respectively. The direct artwork and portable/cache installer
checks pass as well.

The currently selected vanilla cache
`assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b` has also received its
18 files (38,711,095 bytes, 36.92 MiB). Their hashes match, all 40 existing
MCC font files remain byte-identical, and the selected-cache registry stays
unchanged. The bounded installation receipt and source hashes are under
`analysis/controller-prompts-20261021`. No installed launcher executable was
replaced. Generated test builds and temporary renders were removed.

Physical Xbox/PlayStation controllers and a full playable mixed-controller
session have not been exercised by these component checks.
