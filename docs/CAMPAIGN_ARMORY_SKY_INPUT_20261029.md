# Armory sky input diagnosis

The V10 iteration1 Armory frame is blocked by the cohort's older V3 prepared
asset pack. It is not a machine material-index or renderer regression. The
existing V7 pack selected by `assets/mcc-source.tsv` passes the unchanged sky
authentication with the same frozen native resource decoder. Rendered Armory
and the machine packet lifetime controls remain separate pending gates.

`Cache::load_model` builds a model-local material bank at
`src/rendering/models/model.cpp:270`, then authenticates the sky at line594.
The failing check at `src/rendering/models/mcc_sky_geometry.cpp:235` precedes
global sky material insertion. Model `E17C0006`,
`scenarios\\skies\\solo\\spacestation\\spacestation`, has four canonical slots.
Only slot1, shader `E1860010` (`earth_matte`), differs.

| Input | Earth chain | Complete texture SHA256 |
| --- | --- | --- |
| V10 cohort V3 MCC pack | 1024x1024 RGBA8,11mips,5592404bytes | `481c96bdcbff17ef7abbc9915f26afac8e7e2a1fc40a164bd2a82768d380f05c` |
| Original TU1.5 offline authority and existing V7 prepared pack | 512x512 BC3,8mips,349520bytes | `b80a32d6384cbfa338297df337f8e66ebaf63c33c02e2f8283d0ff37aa5e80c8` |

Fresh Cache, BSP-only, machine-only and BSP-plus-machine histories all reproduce
the identical mismatch. None reports a bitmap-residency error. An independent
original-cache comparison, repeated exactly, finds differences only below
`/bound_textures/3`: dimensions, format, sizes, mip layout and pixel hashes.
All other material semantics agree. The current original semantic digest is
the catalog's expected `3473f64e8a46fcddfbc8d5627efd0fed30755a60615c2ea99400982e8726b385`;
the old MCC pack yields `e87e2f9e1173575bfb4c12167093b4e532251f2b9e525e9522658dc1b0fdf354`.

The exact raw Armory map and donor texture are already authenticated by
`tools/data/native-bitmap-corrections-v6.json`. This existing bundled correction
uses the MCC donor and a qualified delta; it does not require original assets
at runtime. Normal preparation includes it through
`tools/prepare_mcc_gameplay.py:450` and
`tools/mcc_graphics_corrections.py:149`. The prior independent V6 delivery and
source-projection receipts are linked in the diagnosis receipt below.

With `assets/imported/mcc-sky-water-tu15-20261025-v7`, the same decoder passes
fresh Cache, the actual BSP-plus-78-machine resource history and an exact fresh
repeat. All four slots match the original complete semantics and the unchanged
catalog. No source, asset, guard or profile was changed to obtain this result.

Evidence: `analysis/campaign-native-new-game-camera-20261028/armory-sky-diagnosis.json`.
The receipt pins runtime, borrowed archives, transitive headers, maps and
selected-pack resource/provenance inputs. Borrowed inputs stayed unchanged;
all owned compiler/probe/snapshot scratch was removed. These are resource
admission controls, not GPU, whole-campaign or physical Xbox framebuffer parity.
