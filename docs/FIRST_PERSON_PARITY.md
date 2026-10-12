# First-person placement parity

The first-person target is the original Xbox build `02.09.27.09809`, using
the user's matching `default.xbe` and authored `mode` / `jmad` / `weap` / `bipd`
data. First-person arms and weapon placement follows its recovered transform
order, including authored SMG hand attachment. A retained Ivory Tower original
frame now supplies camera, projection and skin-matrix evidence. Full motion and
all-weapon visual parity remain incomplete.

## Original placement path

1. `16760C` fills one graph's bind channels from the weapon model, then the
   arms model, and samples the animation. Both render models use that graph.
2. It creates a translation root from `weap +0x308`, the authored first-person
   offset, and builds the graph's node transforms beneath it.
3. The primary weapon's `camera_control` node (StringID `0x0E0000E0`) becomes
   the adjustment. The authored weapon offset is subtracted from its position.
4. `165F16` builds the active camera basis with `1420F0`, inverts the adjustment
   with `141590`, and composes the two with `142A60`. The result is applied to
   the remapped arms and weapon nodes, before rendering.
5. The player camera reads its horizontal FOV from `bipd +0xCC` (`187130`).
   `171D90` converts that horizontal angle for the viewport aspect. The Cairo
   Chief tag contains `1.22173047` radians (70 degrees); its control multiplier
   at `0x47136C` is 1. The viewer's former 75-degree vertical default was wrong
   for this player camera.
6. `168644` admits the single-weapon hand attachment using the actual unit
   aiming counter. `1D90E0` composes the weapon marker with the inverse arms
   marker, solves the root/middle/end chain through `120220`, and propagates
   the hand delta to every descendant through `16D8D0`. Camera composition
   precedes this operation. SMG has one authored `left_hand` constraint; the
   battle rifle's `any:any` constraint list is empty.

`2F970` builds the final safe/outer viewport frustum; it does not use an
unmodified symmetric perspective. Original first-person GPU projection
constants are the world projection constants multiplied by `0.15000000596`.
The two draws retain identical observer basis/position constants. This proves
the first-person model uses the rotated observer camera, rather than an
unrotated unit aim basis. `1396C7` mode 4 places the captured reticle at
`(320,284)` in the original 640×480 frame. The source camera/frustum helpers
are documented in `docs/MULTIPLAYER_SHADING_PARITY.md`.

Without the adjustment, camera-control rotations/translations displace the
weapon incorrectly during action clips. Sampling two separate bind skeletons
can also give the arms and weapon different transforms for their shared nodes.
The native path now shares a sampled skeleton, preserves the original offset
order and uses the rendered camera's forward/up basis, including roll.

Local retail pseudocode is in `analysis/retail/functions/0016760c.c`,
`00165f16.c`, `00187130.c`, `00171d90.c`, and the three math helpers below.
The matching named reconstruction is
[unknown_165ce5.cpp](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_165ce5.cpp).
Its larger first-person routines are marked `todo`; the original instructions,
rather than guessed decompiler types, are the math comparison authority.

## Module boundaries

| Module | Responsibility |
| --- | --- |
| `src/model.cpp` | Bounded model, skin and animation resource decoding |
| `src/model_pose.cpp` | Graph sampling, shared bind priority, camera-control adjustment and skinning both first-person meshes |
| `src/first_person_ik.cpp` | Authored marker pairs, source two-bone solver, descendant propagation, unit aiming counter and eligibility |
| `src/reference_player.cpp` | Bounded original-frame camera, packed profile colors, retained object lighting and aiming fixture |
| `src/transform.cpp` | Original scaled-transform inverse, composition and camera-basis arithmetic |
| `src/content.cpp` | Authored player FOV, weapon offset and model/animation references |
| `src/main.cpp` | Active camera placement, original player projection, action selection and deterministic pose captures |
| `tests/first_person_tests.cpp` | Transform-order, shared-bind, roll/FOV, malformed-input and retail action fixtures |
| `tests/first_person_ik_tests.cpp` | Original GPU skin-bank fixture, marker admission and aiming counter checks |
| `tests/reference_player_tests.cpp` | Original player words, malformed fixtures and active first-person shader input comparisons |

## Validation

`tools/differential_first_person.py` executes the three original math helpers
only inside a Unicorn test harness. Each has 10,016 exact comparisons:

| Original helper | Result |
| --- | --- |
| `1420F0`: camera basis | 10,016 / 10,016 bit-exact |
| `141590`: scaled transform inverse | 10,016 / 10,016 bit-exact |
| `142A60`: scaled transform composition | 10,016 / 10,016 bit-exact |

The comparisons include zero/negative scales, signed zero, translated matrices
and randomized finite inputs. The executable sums basis columns as `X+(Y+Z)`
but translated positions as `(X+Y)+Z`; retaining that distinction corrected
rounding differences exposed by the oracle. Native C++ runs these operations
without executing Xbox code.

Retail pose fixtures check both meshes across all 407 integer frames of eight
clips: battle-rifle and SMG `idle`, `fire_1`, `reload_full`, `reload_empty`.
`analysis/first-person/pose-frames.tsv` records their camera-local bounds.
These fixtures prove parsing and finite pose generation, alongside the synthetic
order tests; they do not independently prove original animation sampling or
GPU skinning results.

Fixed-pose Vulkan captures cover the start, middle and final frame of every
clip at 640x480 (4:3) and 1280x720 (16:9). The sweep records its immutable
tested binary and all logs/screenshots in `analysis/first-person/gpu/`.
All 48 captures pass synchronization validation without messages. All 14
normal and ASan/UBSan CTest groups also pass.
Those earlier captures are native render/validation evidence. They are
supplemented by the independent original-frame evidence below, rather than
establishing visual parity by themselves.

## Retained original frame and marker attachment

`analysis/multiplayer-shading/original-xemu/` contains an isolated original
xemu session, the paused `ivory-tower-shading-baseline` state, raw observer/unit
bytes, `original_capture_8.rdc`, original first-person GPU banks and matching
native captures. Its writable HDD overlay and EEPROM are separate from the
existing frontend reference session. The retained source camera and root
float bits are recorded in `ivory-paused-state.json`. The comparison image is
`frames/ivory-original-native-frustum.png`.

All four original SMG skin matrices match the authored idle frame 32 with a
largest coefficient error below `2.84e-7`. The visible front sight is within
approximately one pixel in this matched camera. The original hand skin bank
exposed a separate missing attachment: before applying source `1D90E0`, its
largest coefficient difference was `0.0691402`; afterwards all 34 palette
entries are within `1.44e-6`. This is the evidence behind adding marker IK,
rather than adjusting the weapon offset to compensate for a hand mismatch.

`tools/differential_first_person_ik.py` executes untouched original routines
and compares the production C++ module against them:

| Original operation | Bit-exact fixtures |
| --- | --- |
| `1421B0` / `141E10` marker construction | 1,024 |
| `120220` two-bone chain | 1,024 |
| `1D9240` / `1D9320` / `1D9370` aiming counter and curve | 2,048 |
| `1112F0` / `C7070` / `111590` / `1DAEA0` / `CD660` eligibility | 2,048 |
| `1D90E0` / `16D8D0` complete 37-node hand attachment | 64 |
| `2963F0` / `1DD560` authored unit action/overlay fallback | 1,024 |

The complete hierarchy cases cover eight SMG idle frames and eight weights,
including source near-zero admission and a weight above one. The original
normalizer accumulates and square-roots in x87 extended precision, then stores
float before scalar SSE normalization; preserving that order is necessary.
No Xbox code is executed by the native posing path.

The original palette fixture `tests/fixtures/retail_fp_skin.tsv` initially
exercised only camera/IK/inverse-bind transforms, using the same native local
inputs and weights on both sides. It could not independently prove decoding
or influence quantization. The production world-pose regression now consumes
independent original local inputs, bones and effective weights for all 10,344
expanded weapon/hand vertices; its largest position error is `1.90735e-6`.
It also checks 4,980 hand basis outputs against actual captured original GPU
normal/tangent/binormal values, with largest error `5.36442e-7`.

The separate `retail_fp_packed.tsv` fixture supplies that independent check:
3,295 original source vertices cover all 10,344 expanded native vertices.
Position, UV and all three tangent-basis vectors match bit for bit. UV values
are actual original post-VS outputs; position and basis values are derived from
original captured raw buffers and the observed GPU operations. Seven original
paint `DebugVertex` traces also confirm these derived position/basis values.
`tools/export_retail_fp_packed.py` records export and fixture hashes and checks
6,590 actual post-VS UV components against the decoded original inputs.

Untouched original `1C330`/`35790` selects position format `0x35`, UV `0x25`
and packed normals `0x16`. Original `1CDD0` separately rounds the half range
and midpoint and uploads c170..172. Executing that routine with the authored
SMG/hand bounds matches both captured GPU constant banks exactly. Positions
use MUL then ADD; the captured UV MAD helper also separates those operations.
Actual drawn signed-short values -32768 and -32767 both enter the shader as
-1. Packed 11/11/10-bit basis values instead divide by 1023/1023/511 without
an endpoint clamp: original paint vertices 4/8/17 produce -1.000977516 and
-1.001956940. The corrected loader preserves those distinct format rules.

Original compiled skinning classes for one/two/three/four influences derive
the first weight as 1, `1-y`, `(1-z)-y`, or `((1-w)-z)-y`. They ignore its
stored byte and leave the other UBYTE_NORM weights unnormalized. Captured
hand vertices with bytes128/128 therefore use `.498039186,.501960814`, not
`.5,.5`. Actual original DebugVertex887/311 and879/91 confirm every blended
matrix row independently. Source VPs blend matrix rows from the last
influence back to the first, then perform DPH/DP3. The native pose follows
that order and consumes negative complements without clamping them.

`retail_fp_skin_weights.tsv` records captured bone bytes, authored node maps
and effective source weights. `retail_fp_world_basis.tsv` records actual
original hand environment-pass basis outputs. The source class fixture also
covers 36 cases across all four compiled variants, including signed weights.
The supplied 21 maps contain no real four-influence mesh; that variant's
coverage is a literal compiled-instruction gate, not a live asset claim.
`tools/export_retail_fp_skin.py` regenerates these independent fixtures.

The 21-map authored census covers 22,118 model sections, including a 3,294
section player/weapon/scenery superset and all278 admitted Slayer scenery
placements. Mixed opaque/transparent counts occur in63 eligible sections:
Elite uses3/1 and the first-person beam rifle2/1. Source `1CAA0` chooses
section+14 for part types1/2/3 and default, section+15 for4/5. The packed
stream retains the maximum count's layout; active bone/weight admission now
follows each expanded part. Rigid classifications1/2 retain full weight.

A production capture after packed position/UV/NORMPACKED fixes independently
matches all paint/rubber UV outputs; its retained hand basis discrepancy
motivated the weight correction. The position bound of `.0625193` pixels
applies to vertices whose source and native projections both lie inside the
640x480 framebuffer, across all seven compared pass families. Raw off-screen
projections have larger differences, and this is not a complete clipping or
final-frame pixel gate. The fresh post-weight production GPU capture confirms
hand D1 within2.38419e-7 and environment basis within1.90735e-6, while active
UVs remain bit exact. At five retained material pixels, substituting only the
original base texture sample reproduces original shading within one float ULP;
other texture samples match exactly. The measured base-sample boundary is
xemu's expanded host texture versus native compressed GPU sampling, and does
not establish physical NV2A decode rounding. Original BC2 blocks remain
unchanged. Details and all comparison outputs are retained under
`analysis/multiplayer-shading/original-xemu/fp-output-audit/README.md`.

The aiming counter is initialized by `113870`, advanced by `113910` at the
start of `111650`, and selected by `1130F0` using `1112F0`, with dual wield
overriding eligibility through `CD660`. Active/fade durations are `.267` /
`.06675` seconds: at 30 Hz the source uses counters 7 / 1. The retained unit
contains bytes `07 07 01 03`, yielding weight one. Its unit graph is
`EA4E08C9`, with combat/rifle/SMG identifiers and base animation 185
(`combat:rifle:idle`, flags zero); its secondary channel is absent. Weapon
class/type identifiers are authored at `weap +0x288/+0x28C`. The production
unit path can drive this counter from real ticks and authored flags.
`load_unit_weapon_action` reproduces the eight ordered mode/class/type
fallbacks from `2963F0`, keeping base and overlay lists separate. For the
captured SMG this selects base idle 185, overlay fire 172 and reload 215; all
have authored flags zero. The strict asset-preview `load_action` remains
available separately.

Full unit locomotion, secondary overlay transition timing, FP sway/blending
and every weapon remain separate unverified boundaries. The source unit reload
overlay is `reload_1`, rather than the first-person `reload_full` name.
The original-frame comparison does not certify all-map, all-action visual parity.

## Original player rendering fixture

`tests/fixtures/retail_ivory_player.reference` preserves the retained original
camera, physics root, four packed change colors, 84-byte object lighting record
and aiming counter. Its SHA-256 is
`5c8da2d4697ced3cc736f9cbebb2896395f41156340b73e53fd9902a3caae13b`.
`ivory-player-provenance.json` records the source offsets and raw-file hashes.
The fixture loader rejects malformed, nonfinite, missing or inconsistent words;
the production capture also checks the map and unit/weapon definitions.
This is a diagnostic rendering fixture, not a simulation/controller restore.

The source shader context contains change colors `00FB9BC9`, `00416C8F`,
`00BE2C2C`, `00FB9BC9`. These are actual captured words, rather than a selected
profile preset. `33980` / `131BB0` unpacks that bank with the original float
reciprocal of 255. Its floats can differ by one ULP from the GPU's normalized
byte expansion; the comparison therefore checks the packed shader values.
The hands consume the first two colors in original draws 830/838. The held
weapon has no runtime change-color block and keeps its own authored material
inputs. Applying the owner's profile bank to its material is unsupported by
this source frame.

Actual lighting draws 848/856/871 (gun) and 879/887 (hands) have c26/c27
directions bit-identical to the retained lighting record. Their packed primary,
secondary and ambient RGB values are `191B1D`, `191B1E`, `0F1112`. The base hand
texture draw retains different c26/c27 values but does not consume them.
Similarly, its inherited c20.xy feeds a disabled fragment texture stage and is
excluded from the active-input comparison. Authored per-material brightness
remains active: SMG paint/bare metal use specular 2, rubber and hands use 1.
The paused final HUD context's zero brightness values are recorded only for
provenance and never override those first-person materials.

The original hands also submit layer 8
`default_environment_map_tangent_space` in draws 895/903. Adding that authored
pass closes a missing normal/cubemap contribution. It consumes source camera
forward through pixel extern 0 (`F95C8B` in this frame) and authored pixel
constant `787387`. The fixture checks the actual per-view remap, rather than
comparing an unbound material template.

`retail_reference_player_shader_bindings` prepares and animates production
materials at the retained phase, applies their source object banks and checks
70 c18..33/pixel constant records across seven original pass
families: gun bare base/lighting, paint/rubber lighting, hand base/lighting and
hand environment mapping. Signed zero is equivalent for these vertex-input
comparisons; unused inherited components are outside this gate.
`tools/differential_reference_player.py` executes the original unpack/pack,
direction selector and aiming-weight routines: all 23 compared words are
bit-exact, and all 21 independent GPU binding checks pass.

Actual production GPU replay exposed a further binding error that a prepared
material check alone could not detect: reconstruction of per-view pixel
programs discarded their fragment offsets. Paint stage-5 ambient and stage-6
alpha remaps then wrote stage 0 instead. Retaining the active program offsets
restores all 18 captured paint pixel constants exactly. At pixel (438,380),
the original paint shader outputs RGB (.022372,.022827,.026512). The fresh
production `native-skin-weights_capture.rdc`, after the packed-input and skin
weight fixes, outputs (.023151,.023404,.027434), with identical alpha .067451.
Its actual uniforms match all 44 packed pixel constants and all consumed
vertex-constant components across the seven retained pass families. All seven
families meet the measured 0.0625193-pixel position bound for vertices inside
the viewport; raw offscreen vertices and triangles crossing the viewport from
outside remain separate rasterization boundaries. The initial hand-base pass
is included: its differing oD0/oT2 values do not contribute because stage 2 is
disabled and its D0 RGB contribution is multiplied by zero D1 RGB.

At five independently traced paint/hand pixels, the three non-base texture
samples match exactly. Substituting only the original base texture sample into
the actual native combiner inputs reproduces hand results exactly and paint
results within one float ULP. This isolates those sampled-output residuals to
the base texture sample. The captured xemu RGBA8 expansion and native BC2 GPU
path differ in decode rounding; physical NV2A decode/filter precision remains
unverified. The native path retains the original compressed blocks. Detailed
raw inputs, active-output classifications, texture evidence and hashes are in
`analysis/multiplayer-shading/original-xemu/fp-output-audit/README.md`; this gate
does not establish complete framebuffer identity.

The physics root in the fixture is unit +0x270, while lighting query position
is animated object +0x30. `unit-query-origin-reference.json` proves the latter
from the retained node-0 transform and authored bounding offset `(-.02,0,0)`:
source `BDEF0` produces `C0A3AE17 C0B05C37 3EBB4240` exactly. The retained body
channel uses idle 185 at frame position `6.0188446`, sampled as integer frame 6;
that is distinct from the proved first-person idle frame 32. The fixture does
not restore the full third-person pose/controller state, and its retained
lighting should not be presented as a proof that a fresh native body query
reproduces that state.

The Zanzibar capture20 fixture is `retail_zanzibar_player.reference`. Its
camera, root physics/velocity, profile colors, aiming counter and lighting
are independently corroborated by actual original draw inputs. Actual used
gun and hand palettes identify authored idle frame56; the later paused
controller frame7 is a different input. `retail_zanzibar_fp_bindings.tsv`
retains70 source GPU records across seven active shader families. The focused
binding regression checks both maps,140 records/14 families; the untouched
Zanzibar reference-player differential matches23/23 words and21/21 GPU checks.

Version2 adds two explicit diagnostic device inputs: source bigwheel
definition EBE20A6D placement0 and blades EC080A93 placement5. Their complete
source object/tag/render-model identities and root float words are retained.
The authored channel at solved image phase3F5CB371 reproduces actual20
palettes within5.96046448e-8 and consumed PRT banks within8.94069672e-8.
`SourceMachineScene::set_reference_devices` admits only matching unique source
placements and preserves those image inputs through synchronize/tick updates.
Version1 behavior remains unchanged. Runtime binds this bank only inside the
existing isolated finite `--mp-reference-player` screenshot mode; it does not
replace the production device timer or invent a phase for the uncaptured
flywheels. Original finer wheel/blade draw-admission ranges remain separate
from this frame-input fixture. Evidence is retained in
`analysis/multiplayer-shading/original-xemu/prt-output-audit/zanzibar20-device-image-proof.json`.

The fresh production `native-crane-source-pose_capture.rdc` (SHA-256
`87daf6928c198daf50b25cf83fb31c507a0c0f360a1c2f1a97543c23b3feecf2`)
now verifies these fixture inputs at the actual GPU binding. All44 packed pixel
constants and every consumed component of the70 source records agree.
Three raw inherited c20 components differ: arm base x and bare base x/w;
they feed only the captured disabled texture stages and remain reported.
The actual FP c6..17 words are exact. After removing the source common16384
projection scale, c0..3 differ by at most5.96046448e-7; c4/c5 by at most
1.78813934e-7 /5.96046448e-8. All seven pass families retain the
inside-viewport position bound0.0625183px. Paint/rubber/arm active bitmap UVs
remain bit exact; hand lighting D1 differs by at most3.57627869e-7.
Scene-projected T0 includes the source homogeneous factor: after xy/w,
the inside-viewport hand-lighting maximum is0.0119517 source pixels.
Raw outside-viewport and disabled-output differences remain separately recorded
in `fp-output-audit/native-crane-postvs-comparison.json`.

At three actual paired paint fragments, detail/bump/cube samples are exact;
base BC2 RGB differs by one8-bit step with identical alpha. Substituting only
the original base sample in the native scalar combiner reproduces the source
within5.96046448e-8. Three paired hand-lighting fragments have identical
non-scene samples; substituting the original640x480 scene sample reproduces
source within1.63912743e-7. These are actual debugger shader-input/output
checks, not a claim that every selected fragment passes depth or that final
framebuffers are equal. Scene-target admission and host compressed-texture
decoding remain explicit boundaries. Original BC blocks remain unchanged.

Focused validation:

```bash
ctest --test-dir build -R 'original_player_reference|retail_reference_player_shader_bindings' --output-on-failure
.tools/python/bin/python tools/differential_reference_player.py --probe build/reference_player_tests
```

```bash
./build/first_person_tests assets/disc/maps analysis/first-person/pose-frames.tsv
.tools/python/bin/python tools/differential_first_person.py
.tools/python/bin/python tools/differential_first_person_ik.py --probe build/first_person_ik_tests
./build/first_person_ik_tests assets/disc/maps/cyclotron.map
VK_LAYER_VALIDATE_SYNC=1 python3 tools/smoke_first_person.py --validation
```

One reproducible pose:

```bash
./build/halo2_native --map assets/disc/maps/01b_spacestation.map \
  --weapon battle_rifle --first-person-pose reload_full 29 \
  --viewport 640 480 --hidden --mute --frames 3 \
  --screenshot analysis/first-person/br-reload-29.ppm
```

`--first-person-pose` freezes only presentation sampling; ordinary firing and
reload state continue using the existing simulation. The fixture frame must
belong to the authored clip. Missing actions, models or invalid frames fail
explicitly. It is a capture aid, not a new gameplay animation state.

## Remaining acceptance work

- Recover and compare the complete sampling/normalization, blend, inheritance
  and linked action-variant paths; the three exact affine kernels do not cover
  the whole animation engine.
- Recover movement/aiming sway springs, overlays, weapon/ammunition overlays,
  transition blends, aiming/IK adjustments and dual-wield mirroring.
- Implement player-representation switching and the original first-person
  body visibility/node mapping. Current live fixtures use Chief arms.
- Match first-person shader/lighting, projection/depth precision and any
  original pass-specific clipping behavior.
- Capture original and native images with the same camera, viewport, FOV,
  weapon, action frame, stance and lighting. Cover idle, fire/reload, walking,
  crouching, looking up/down, rolling cameras and weapon switches. Only this
  completed gate supports an unqualified position/visual parity claim.
