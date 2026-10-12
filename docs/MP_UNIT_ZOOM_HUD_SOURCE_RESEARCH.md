# Original stock SMG binocular HUD and model requests

The original status producer distinguishes the unit's CURRENT selector from
the camera's retained view selector. With an equipped stock SMG, CURRENT 0 sets
HUD level 0 and binocular status; imported masks select the real binocular
overlay and exclude the normal shield, radar, ammo and crosshair. The bounded
native helper now matches 309 executed original status/mask cases exactly.
The original CPU first-person request owner still requests arms and SMG during
the six measured scoped/unscoped frames. This evidence does not justify adding
an invented hide-weapon gate.

## Independent source evidence

The 1.1 executable and supplied authenticated 1.5 executable hashes are the same
as [the scope-camera contract](MP_UNIT_SCOPE_CAMERA_SOURCE_RESEARCH.md). No
original executable/cache bytes, earlier oracle tools or reports were changed.

New independent files are under
`analysis/multiplayer/integration/scope-presentation/`:

| Evidence | Tool | Report SHA-256 |
|---|---|---|
| `measured` / `repeat`, six composed source frames | `tools/oracle_mp_scope_presentation_1_1.py` | `bb9d7a8b1bf3b3dca9a188feedb132d0cf11605d0dd8ea8f290e4fb30a60abd5` |
| `controls-measured` / `controls-repeat`, 309 explicit original branch cases | `tools/oracle_mp_scope_hud_controls_1_1.py` | `2f0f30db211c10808131d8f2f22bdfb191f70782eb56332f6630025f622cb452` |
| `update-measured` / `update-repeat` | `tools/research_mp_scope_presentation_updates.py` | `a30280e371fe2a66b41257c1238e3d1fa78b2ae0d2b3e391ab835448265e4164` |
| Literal source/cache and prior-pin audit | `tools/audit_mp_scope_presentation.py` | `34549e7d6b7f30588efecd8cfe63a79769ff895b20ce3c7132deeac6e2cc3568` |

Each paired report and every original listing is independently byte-identical.
The audit validates 7291 literal original instruction rows, all authored HUD
mask bytes, six actual model-request outputs, all 11 primary complete 1.1→1.5
body mappings, 734 files from the prior scope manifest and its 211 earlier pins.
No HUD status, mask, active flag, model-count or render completion return is
supplied by a service in this experiment.

The source fixture reuses the frozen real registered first-person allocation,
SMG/arms graph resources, profile conversion, animation clock and node maps.
Its existing preallocated player/local-view linkage, raw host backing,
resource residency, clear muzzle and failed later-projectile boundary remain
explicit. Native selector/tick/neutral-button values are incoming stimuli.
The original HUD prefix stops before motion-sensor, shield and engine status;
the complete mask builder consumes that actually initialized partial buffer.
Its other unit/engine bits therefore are not promoted to complete gameplay HUD
conditions. The render-camera input bytes are captured inherited initial state;
this is CPU model request evidence, not a GPU frame or hardware matrix proof.

## Exact source status and native API

`13B09A` actually clears its 270h-byte output and default datums, resolves the
declared user/player/unit, then obtains `C9BC0` at 13B183. That leaf reads the
source signed byte unit+240. `13B18C` stores the extended selector into HUD
status signed-short +3C.

`13B1C8` initializes status+3E to 0. It becomes 1 at 13B224 only when CURRENT
is not -1, actual primary `CD1A0` is not NONE and `105120` returns AL=0. `105120`
qualifies an optic weapon when signed authored weap+1FE is nonzero and **no**
authored magazine has signed runtime mode 1..6. It does not test a weapon flag,
high-level native State, the camera selector, magnification or clipped FOV.

`source_zoom_hud_status(int8_t current, const ZoomWeaponView *primary)` now
returns only raw `ZoomHudStatus{int16_t current,uint8_t binocular}`. It requires
the actual primary presence/definition and all original magazine modes. It
does not perform scope admission or infer inventory ownership.

`13B7C0` builds level bits from status+3C and binocular from+3E:

| Exact source condition | Unit mask bit |
|---|---|
| CURRENT equals -1 | `0040` |
| CURRENT equals 0 | `0080` |
| CURRENT equals 1 | `0100` |
| binocular byte is nonzero | `0400` |

`source_zoom_hud_mask(const ZoomHudStatus&)` returns this subset only.
The caller replaces `(old_bits & ~0x05C0)` with this result and separately
owns every other HUD condition. All 256 signed CURRENT byte values execute in
the control suite: other values get none of the three level bits, without a
guessed clamp. Primary NONE, zero authored magazines, zoom counts 0/1/-1/32767/
-32768 and modes -1/0/1..7 are independently varied and explicitly labelled
branch stimuli. They are not claims that ordinary gameplay produces those
mutated fields.

The six ordinary native-input cases -1/0/0/0/-1/-1 produce source zoom subsets
0040/0480/0480/0480/0040/0040. Their partly constructed full unit masks are
0053/0493/0493/0493/0053/0053; the omitted status producer portions explain
why these are not a replacement for the live caller's complete other bits.

The existing unit_zoom implementation was extended only by these helpers.
Before the authorized append, its prior header/source bytes were copied into
`prior-native-unit-zoom/` with original SHA-256 pins. Older source fixtures,
manifests, docs and tools remain untouched.

## Authored widgets and first-person request gates

Freshly read original Lockout data supplies the real masks. Spartan
`binocular_mask` requires unit mask 0400 and has no exclusions. Shield meter/mask
and normal motion tracker exclude 0580. SMG backgrounds/ammo counters exclude
0400; its normal crosshair excludes 0580. The remaining imported fields, textures,
sequences, shader effects and bitmap geometry are consumed as authored. This
research does not invent a fullscreen replacement or claim verified GPU mask
coverage.

Actual `1678C1→168866` keeps primary hand flags 00000007 for all measured frames.
`167A0D` requires valid view/unit/character, at least one active hand and one
equipped hand. Per-hand model request gates are active flag 1, weapon datum
not NONE, arms-animated flag 4 for arms, weapon-animated flag 2 for weapon, valid
render definitions, matching view/unit and output capacity. Its full unchanged
body contains no CURRENT/retained-selector scope suppression branch. Full
source model construction returns count 2 in every frame: real arms and SMG
definitions using their original node maps. The upper 3D2B0 request owner also
requires its actual first-person camera mode and local view/player binding.

This establishes the measured request path under captured inputs. Other
render-camera, pose, depth or overlay behavior can alter visible pixels, so
do not generalize count 2 into a complete visible-weapon guarantee or an
unverified rule that all zoomed weapons render. Keep the existing arms/SMG
requests for this stock case and consume the actual binocular HUD assets.

The complete later-image status/mask/model gate bodies correspond in
`update-measured/report.json`; all 11 primary bodies and 39 retained pairs match.
That is bounded original instruction correspondence, not a new 1.5 complete
gameplay/render execution.

## Validation and caller handoff

`tests/unit_zoom_hud_tests.cpp` consumes 309 rows exported directly from original
control outputs by `tools/export_mp_scope_hud_fixtures.py`.
`controls-measured/fixtures.tsv` SHA-256 is
`187f67d54e361080782ef2654c3f2d2dab2f5dc9d67fe4efa0a4a46431bb18cf`.
Optimized and ASan/UBSan native tests pass all 309 cases. Existing zoom tests
still match 43 admissions, 51 selectors, 36 exact magnifications and 10 full unit
phases, retaining their two explicitly unavailable multilevel power cases.
No CMake, runtime, MultiplayerHud or unrelated header was edited here.

```sh
g++ -std=c++20 -O2 -fno-fast-math -ffp-contract=off -Iinclude src/unit_zoom.cpp tests/unit_zoom_hud_tests.cpp -o /tmp/halo2-zoom-hud-tests
/tmp/halo2-zoom-hud-tests analysis/multiplayer/integration/scope-presentation/controls-measured/fixtures.tsv
```

Live snapshot publication, per-viewport HUD consumption and renderer verification
are the parent task's integration step. Genuine local-session ownership, complete
other HUD bits, GPU overlay coverage and Xbox reciprocal/matrix parity remain
separate boundaries.
