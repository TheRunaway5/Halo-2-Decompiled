# Original frontend model scenes

The reported bind pose and misplaced preview have two source-backed causes:
the native frontend did not request the original model-widget animation, and
it treated the authored field of view as an ordinary full horizontal angle.
The original model widget has its own camera path and animation request.
No rotation or placement is selected by visual estimation.

## Authority

- Original executable `assets/disc/default.xbe`, SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- Original `assets/disc/maps/mainmenu.map`, SHA-256
  `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
- [model-scene-assets.json](../analysis/frontend/profile-regression/model-scene-assets.json)
  retains all 17 authored scene definitions and ten named bipeds. Its
  [audit script](../analysis/frontend/profile-regression/audit-model-scene-assets.py)
  reads the actual `wgit`, `scnr`, `bipd`, `hlmt` and `jmad` metadata.
- The original Xbox format labels are in
  [wgit.xml](../tools/reference/assembly/src/Assembly/Plugins/Halo2Xbox/wgit.xml)
  and [jmad.xml](../tools/reference/assembly/src/Assembly/Plugins/Halo2Xbox/jmad.xml).

The caller's current animation offset, controller state and active render
window are separate inputs. Numeric proofs below do not substitute for a
same-phase original/native rendered comparison.

## Camera and placement

Model-widget constructor `2B0B5E` installs vtable `45ADA8`. Its draw entry is
`2B0BAA`. The retained executable contains the complete draw instructions;
the decompiled file does not contain that complete body.

At `2B0D42..2B0D81`, the original computes:

```
camera.x = authored_camera.x - animation_scale.x * widget_offset.x
camera.y = authored_camera.y - animation_scale.y * widget_offset.z
camera.z = authored_camera.z - animation_scale.z * widget_offset.y
```

Each multiply and subtraction is a separate scalar SSE operation. The
authored `scale` triplet is the **Animation Scale Factor**, rather than an
object scale. Applying it to the mesh would change the model incorrectly.
Scenario placement and scenario object scale remain the model inputs.

At `2B0CBF..2B0CF7`, the field of view is clamped to `[1,359]`, then
multiplied by original float word `0x3C0EFA35` at `45E5AC`
(`0.008726646192371845`). Authored 70 therefore becomes a vertical render
angle of approximately 35 degrees. That value is passed directly to
`2C560 → 2F970 → 2FD90`, without the observer's `2F800` conversion or the
title world's safe-area factor. The original camera's forward/up are
`(0,1,0)` and `(0,0,1)`, normalized through unchanged `30BF0`.

`2C560` constructs identical outer and safe rectangles and uses the original
near/far constants `0.015625 / 1024`. With explicit caller frame bounds
`(top,left,bottom,right)=(0,0,480,640)`, authored widget viewport mapping is:

```
left   = 320 + authored.left
top    = 240 - authored.top
right  = 320 + authored.right
bottom = 240 - authored.bottom
```

The original draw gets its half-width/half-height from the supplied frame
rectangle and adds the separate `2363D4` window-padding outputs. The oracle
proves the actual 640×480 input above; it does not infer a new wide-window
caller rectangle. Native model previews retain the centered authored UI
bank described in [FRONTEND_TITLE_ASPECT_RESEARCH.md](FRONTEND_TITLE_ASPECT_RESEARCH.md).

[oracle-model-camera.py](../analysis/frontend/profile-regression/oracle-model-camera.py)
executes original `2B0BAA`, `30BF0` and `2363D4`. It intercepts object-name
resolution, visibility submission and rendering services, then captures the
actual camera and rectangle arguments at `2C560`. The comparison includes
all 17 authored scenes plus 1,000 finite offset/FOV-clamp cases. All 10,170
float words match the native pure helper bit for bit. The retained
[report](../analysis/frontend/profile-regression/model-camera-original.json)
and [fixture](../analysis/frontend/profile-regression/model-camera-original.bin)
provide the explicit inputs and original outputs.

Four-way screen 30 uses a 70-degree authored FOV and first-player camera
`(-56.9900017,56.2000008,-49.7000008)`. Its Spartan scenario position is
`(-58,58,-50.1884422)` with authored yaw `-0.785398185`.
Appearance screen 47 uses first-player Spartan camera
`(21.6000004,56.4000015,-49.8800011)`, authored FOV 72, and scenario position
`(22,58,-50.1884422)` with authored yaw `-2.26892805`. Elite scenes reference
their own object, camera, authored yaw and scenario scale 0.85. These values
are read from the scene inventory rather than substituted between screens.

## Original animation request

Original model-widget entry `2B0B96` requests mode StringID `0x020001B3`
(`ui`) through `2B0EEB`. For a biped, that entry calls
`10F5C0(object,0,2,ui,current)`. `10F430` forwards the state request as
`ui/current/current/current`. See
[unknown_2b116a.cpp](../tools/reference/halo2-decompiled/src/unknown_2b116a.cpp)
and [unknown_10f1e0.cpp](../tools/reference/halo2-decompiled/src/unknown_10f1e0.cpp).

`1CC2F0` resolves each `current` from the controller and applies the graph's
weapon-type-to-class alias. `1CC3F0 → 1DB170` looks up the action with the
original category fallbacks. Lookup flag 2 allows an idle fallback. Failed
lookup leaves the current channel intact in `1CD070`; it does not force a
bind pose. See
[unknown_1cafc0.cpp](../tools/reference/halo2-decompiled/src/unknown_1cafc0.cpp).

All supplied mainmenu bipeds have one authored starting weapon. Original
unit initialization calls `CCAB0`, which constructs and gives each referenced
weapon to the unit through `CD0C0`. See
[unit_object_type.cpp](../tools/reference/halo2-decompiled/src/unit_object_type.cpp).
The source references are:

| Model | Starting weapon | Resolved class | UI request | Retained clip |
| --- | --- | --- | --- | --- |
| Spartan | battle rifle `E2BE0120` | rifle | succeeds | graph `E25000DB`, index 132 |
| Elite | plasma rifle `E3BD01C1` | pistol alias | fails; no UI category | graph `E34F01A4`, combat idle index 73 |

[oracle-model-animation.py](../analysis/frontend/profile-regression/oracle-model-animation.py)
executes original `1CC2F0` and `1DB170` with the actual complete graph
metadata. The weapon-equipped controller categories remain explicit inputs.
All ten authored named bipeds select these source indices. The
[report](../analysis/frontend/profile-regression/model-animation-original.json)
records the UI request, resolved names, prior combat lookup and selected
index independently. Both clips have no parent or next-animation link.

The native selector now follows this request and decodes the source clip.
It rejects multiple initial weapons until the active-slot producer is
recovered. It does not derive animation names from filenames or reuse the
scenery-only named-idle policy for bipeds.

## Original primary clock and widget lifecycle

[oracle-model-clock.py](../analysis/frontend/profile-regression/oracle-model-clock.py)
executes unchanged `1C68C0 → 1C66A0 → 1C6560` and the actual UI request
`1CD070`, with complete graph metadata. The retained
[clock report](../analysis/frontend/profile-regression/model-clock-original.json)
contains 480 original ticks, 3,021 requests and three live zero-epoch
replays. The [raw-word fixture](../analysis/frontend/profile-regression/model-clock-original.tsv)
includes all 480 tick outputs and 2,014 successful phase-preserving requests,
including 1,007 repeated `132 → 132` requests. Native frame/rate/flags/delay
match each recorded output bit for bit. All 1,007 Elite requests fail and
retain their complete primary channel.

The end threshold is `frame_count − float(0x38D1B717)` (`0.0001`), rather
than an integer frame count. Each update rounds `seconds × rate`, then
`× 30`, then adds the retained frame. After wrap, the original retains the
small fractional remainder. Ordinary modulo of total elapsed seconds
does not reproduce that clock.

Read-only owned-Xemu snapshots at title, first profile entry and main-menu
entry show all ten bipeds ticking combat idle from game epoch zero,
including hidden objects. Replaying original update code for each actual
game tick matches all 30 complete primary-channel records bit for bit.
Creating screen30 requests UI for its four Spartan scene objects, while
Elite retains73. Leaving30 leaves those Spartan channels at132. Creating
appearance47 requests132 on its separate Spartan object. Successful
requests preserve the old frame fraction using separately rounded division
and multiplication; repeated requests clear/restart the same channel too.
The retained secondary channels1/2 are inactive.

Native clocks now start combat47/73 and advance at each frontend game tick.
New scene widgets request UI regardless of their signed-slot/species
visibility. Parent dialogs retain the object clocks, and returning from a
screen does not restore combat idle. Rendering consumes the retained source
frame directly; hidden vertex banks rebuild only when a model is drawn.
The screen-instance observation is adapted to the native menu stack. A
close/recreate of an identical screen/context between observations and the
source render-time interpolation remain separate lifecycle boundaries.

## Authored held weapons

[model-weapon-markers.json](../analysis/frontend/profile-regression/model-weapon-markers.json)
and its [audit script](../analysis/frontend/profile-regression/audit-model-weapon-markers.py)
retain the actual starting weapons and markers. Each parent has exactly one
unconditional hand marker: Spartan `right_hand`, node 20; Elite
`right_hand_elite`, node 38. Both unit preferred-child marker and weapon
child marker names are zero. No runtime region-permutation choice is needed
for these supplied markers.

The existing [source_shadow.cpp](../src/source_shadow.cpp) helpers implement
`D0870/B8D30/1D8F50` selection, `B8EE0/B92D0/B91D0` alignment, `B93B0`
parent-local storage, and `BD090` object composition. Their use requires the
actual posed parent node. Reusing this branch avoids an estimated hand
offset, weapon rotation or scenario-scale inheritance.

The battle rifle has no animation graph. The plasma rifle graph has one
single-frame clip. Both selected parent idles contain zero frame, sound or
effect events and no parent/next variant links. The held weapon is rebuilt
from the same current posed parent bank as the body; its authored marker
rotation and parent-local alignment are retained.

## Deferred node admission and captured bank stages

The original update bank is not automatically the final rendered bank.
`BD090` first copies all render-model bind transforms through `16D940`.
For an object with its own orientation block and a nonempty model mask at
`hlmt + 0xA4`, it samples the complement of that mask, limited to the model
node count. `BDB60` then limits the unit callback count to the highest active
mask bit plus one. This is an authored model optimization, rather than an
absent-animation-component fallback.

[oracle_frontend_model_node_admission.py](../tools/oracle_frontend_model_node_admission.py)
executes the unchanged `BD090` selector interval and `BDB60` count reduction
using the actual model definitions and captured object orientation ownership.
The [retained report](../analysis/frontend/profile-regression/frontend-model-node-admission-original.json)
records both 256-bit masks, all ten object headers and the exact outputs:

| Unit | Initially sampled nodes | Unit callback count | Deferred node mask words 0/1 |
| --- | --- | --- | --- |
| Spartan `E23400BF` | 0–11, 14–20 | 21 | `FFE03000 / 00000001` |
| Elite `E3110154` | 0–21, 34, 35, 38 | 39 | `FFC00000 / 007FFFB3` |

All remaining mask words are zero. The Elite's right jaw nodes36/37 are
deferred; the left upper jaw35 is sampled initially. In the retained hidden
Elite local bank, both right-jaw transforms equal the render-model bind
words exactly, even though primary clip73 contains their animation tracks.
This explains why an unrestricted primary pose differs from that hidden
bank. It does not establish a defect in the visible Elite rendering.

`BD610` can subsequently sample the deferred set and rebuild its matrices.
It tests the model's separate `hlmt + 0xC4` bit vector against each admitted
render **section** in region order. These are section indices, not skeleton
node indices. For Elite, the trigger sections are16,17,21,25,26,30. All
right-jaw palette references occur in sections17,21,26,30. The oracle executes
the unchanged admission branch for 285 discriminating section collectors
across all ten captured objects. Collector inputs here are explicit fixture
inputs; actual live geometry residency and the menu's admitted collector are
separate evidence requirements. Palette membership alone does not prove a
nonzero vertex weight.

The exact captured flags distinguish these stages. In the
[actual screen30 capture](../analysis/frontend/profile-regression/original-model-actual-fourway-ui-node-bank.json),
the displayed first Spartan has already cleared flag30, while every other
object retains deferred sampling. The keyboard/hidden-control capture has
flag30 set for all ten objects. The report's `captured_banks` retains these
per-object records and source-file hashes so a later comparator cannot
silently treat the hidden object banks as final rendered banks.

| Object address | Unit | Screen30 object flags | Deferred nodes retained |
| --- | --- | --- | --- |
| `82D8A050` | Spartan player1 | `01089B00` | No |
| `82D8BA20` | Spartan player2 | `41009B00` | Yes |
| `82D8D3F0` | Spartan player3 | `41009B00` | Yes |
| `82D8EDC0` | Spartan player4 | `41009B00` | Yes |
| `82D90790` | Elite player1 | `41009B00` | Yes |
| `82D92DB0` | Elite player2 | `41009B00` | Yes |
| `82D953D0` | Elite player3 | `41009B00` | Yes |
| `82D979F0` | Elite player4 | `41009B00` | Yes |
| `82D9A010` | Spartan appearance | `41009B00` | Yes |
| `82D9B9E0` | Elite appearance | `41009B00` | Yes |

The unrestricted comparator's approximately 0.21255 right-jaw matrix error
was on the hidden Elite player1 bank. No jaw-specific runtime adjustment is
justified by that comparison. The separate visible Elite capture below now
retains the render producer's actual inputs. Its pose comparison must use that
bank before claiming or changing visible jaw parity. `137800` also distinguishes
an already cached render bank from an object bank before calling `BD610`; that
input remains explicit rather than inferred from the screenshot.

## Model preview geometry audit

The current native frontend calls `load_model(unit,true,scenario_variant)`
without an explicit runtime section collector or geometry LOD. That decoder's
preview path picks the highest nonnegative authored LOD, then sorts and
deduplicates the selected sections. Its deterministic initial region choices
are resolved for all ten menu bipeds: Spartan permutations0,0,0,0 and Elite
permutations0,1,0,NONE,1. The
[read-only native audit](../analysis/frontend/profile-regression/native-model-geometry-audit.txt)
retains the selected sections, all six resident LOD candidates and every
nonzero native skin influence. Its
[provenance](../analysis/frontend/profile-regression/native-model-geometry-audit.provenance.json)
pins the actual native source/library hashes; material preparation was excluded
from the audit because it does not change this geometry selection.

| Unit | Native preview sections | Authored resident LOD4/5 in region order |
| --- | --- | --- |
| Spartan | 16,17,18,19 | 16,17,18,19 |
| Elite | 25,27,30,32 | 25,30,27,NONE,32 |

In the native Elite preview, right-jaw bones36/37 have230/147 nonzero expanded
vertex influences. The deferred-node decision therefore matters when those
detail sections are rendered; it cannot be discarded as an unused-bone issue.
These influence counts describe the native decoded mesh and are separate from
the source palette-membership inventory.

The original live visible appearance47 Elite draw is retained at
[137800 entry](../analysis/frontend/profile-regression/original-model-visible-elite-render-entry.json)
and [13784E after admission](../analysis/frontend/profile-regression/original-model-visible-elite-render-post-admission.json).
The actual collector is25,30,27,255,32, LOD5, and the cached-bank input is false.
It references Elite appearance object`82D9B9E0`. Flag30 is already clear in
both stops, so this particular draw uses its already sampled object bank and
does not initiate a new deferred sample. The native preview contains exactly
these four nonabsent geometry sections. The capture also retains the selected
world bank, local bank, animation channel/control and all admission metadata
together. No LOD was estimated from object distance or apparent size.

Original `4BCC0 → 4C450` chooses and caps a retained geometry LOD;
`4C190 → 4C2B0` selects sections using active region permutations and actual
geometry residency, preserving region order and absent slots. If the requested
LOD is unavailable, `4C190` can try its previous LOD. The native static preview
does not implement that changing collector policy. The observed visible Elite
appearance draw proves its current section set, rather than every menu's
camera, visibility, residency or LOD transition. Full geometry admission across
the frontend remains a separate implementation boundary.

## Verification boundary

[frontend_model_tests.cpp](../tests/frontend_model_tests.cpp) checks the
original camera fixture, all ten combat/UI selections, 480 original ticks,
2,014 exact phase transfers and that the selected clip replaces the reported
bind pose. Pose decoding and world hierarchy arithmetic also have separate
retained original fixture tests.

Actual original node banks are retained in
[reboot-node-bank](../analysis/frontend/profile-regression/original-model-reboot-node-bank.json)
and [screen30 UI bank](../analysis/frontend/profile-regression/original-model-actual-fourway-ui-node-bank.json).
Each debugger stop reads primary clip/frame and every model-order world
Transform together, with raw object bytes0..0x130. `BDF80` derives node count
from object+0x114 byte length divided by52 and uses object+0x116 relative
bank offset: Spartan33 and Elite55 nodes. The
[screen30 channel snapshot](../analysis/frontend/profile-regression/original-actual-fourway-ui-node-bank-channels.json)
and [original capture](../analysis/frontend/profile-regression/original-model-actual-fourway-ui-node-bank.png)
prove the active screen separately; the screenshot is not an exact clock
pairing with the debugger stop. The older `scenario_name_index` label in
earlier channel dumps was an unproved offset and is not used as evidence.

The visible render captures establish the finger/grip sampling stage. The
original local callback112070 selects combat grip43 for Spartan and99 for
Elite, including after a UI primary request. Corrected original codec probes
execute279860/28C96011 times for Spartan and15 for Elite; their outputs match
the actual visible post-admission rotation words exactly. Initial probes
that omitted the sampler's SSE weight were retracted and are not evidence.
Appearance material/color overrides and complete shading are not established
by the clock/camera or grip checks. The literal primary-codec and actual
producer checks below establish their separately measured animation inputs.
The scene's named `ui_basic` lights do not occur in the scenario object-name
table and there are zero scenario light instances; the source named resolver
must be proved before describing this unused name bank as missing lighting.
Full model or framebuffer parity is not claimed.

## Visible deferred draw and weapon world callback

The fresh original appearance draws are frozen at137800 entry and13784E,
with the controller frame, object header, locals, world bank, masks and
collector read at the same paused guest state. Logical hardware breakpoints
do not patch guest code/data or call guest functions. Screenshots are retained
separately and do not assert an identical framebuffer phase.

| Unit | Actual region collector | LOD | Cached | Bit30 | Callback count |
| --- | --- | --- | --- | --- | --- |
| Spartan | 16,17,18,19 | 5 | false | set→clear | 33 |
| Elite | 25,30,27,255,32 | 5 | false | set→clear | 55 |

The [Spartan entry](../analysis/frontend/profile-regression/original-model-visible-spartan-render-entry.json)
and [post-admission bank](../analysis/frontend/profile-regression/original-model-visible-spartan-render-post-admission.json)
retain the same primary132 frame words. The corresponding
[Elite fresh entry](../analysis/frontend/profile-regression/original-model-visible-elite-render-entry-fresh-tick.json)
and [post-admission bank](../analysis/frontend/profile-regression/original-model-visible-elite-render-post-admission-fresh-tick.json)
retain the same primary73 frame words. BD610 samples the literal deferred
mask, runs the full33/55-node callback, and returns the same object world-bank
pointer in both observed draws. The source rebuild changes50/110 local float
words and138/185 world float words respectively. The earlier Elite capture
with bit30 already clear remains a separate immutable fixture.

Original1130F0 admits world IK only when the unit definition's+BC bit11 is
clear and the primary manager is valid.113BA0 admits positive slot0 weight,
then113980 iterates graph class-pair markers through1DCFA0 and deduplicates
the actual body marker node. On these assets, Spartan resolves the unconditional
`left_hand→left_hand` fallback; the Elite pair references a missing plasma-rifle
marker and admits no constraint. No alternate left-hand pair is substituted.

[oracle-model-held-ik.py](../analysis/frontend/profile-regression/oracle-model-held-ik.py)
executes the unchanged original iterator, marker constructor,142A60 target
composition and1D90E0 with all ten retained body/weapon banks and six explicit
weights each. The [60-case fixture](../analysis/frontend/profile-regression/model-held-ik-original.json)
and [raw-word fixture](../analysis/frontend/profile-regression/model-held-ik-original.tsv)
match the native frontend world-IK helper in every float word. These explicit
input-bank tests do not infer the live initial blend or primary local sampling.

The [cold-boot lifecycle trace](../analysis/frontend/profile-regression/model-aim-initialization-original.json)
retains57 reads of original113870,113910 and1130F0 for first Spartan/Elite
from game epoch0 through tick8.113870 clears the slots and sets curve1.
The initial world callback selects slot0 to value1/count7/flags1, and slot1
to value0/count1/flags0. Subsequent actual tick advances bring slot0 to
value7/flags3 at tick5. Original idle action and actual animation+18 flags
feed `source_weapon_aim_eligible`; the runtime retains/advances the source
blend instead of supplying a constant weight1. The fixture preserves a
repeated before-tick read at tick2; it is not counted as an extra game tick.

[oracle-model-deferred-world.py](../analysis/frontend/profile-regression/oracle-model-deferred-world.py)
uses the actual entry world bank and post-admission local bank as explicit
inputs to unchanged1421F0/142A60. Ascending masked rebuild matches both actual
post-admission world banks in every float word. The
[retained outputs](../analysis/frontend/profile-regression/model-deferred-world-original.json)
isolate this arithmetic from primary sampling and reciprocal seed selection.

The shared [frontend_model_pose.cpp](../src/frontend_model_pose.cpp) helper
now performs initial complement-mask sample/grip, world weapon attachment and
IK, deferred primary/grip sample, then an ascending **masked** world rebuild.
It retains nondeferred parent matrices produced by IK. It does not assume that
a complete grip before IK has identical arithmetic. The supplied attachment
and IK chain nodes must belong to the initial bank; unsupported deferred
callback dependencies are rejected. Existing skinning consumes the returned
body and weapon world-node banks. Production rendering uses this helper
without advancing animation clocks. The independent visible-bank comparator
also calls this helper, supplies the captured fractional frame and root, and
derives IK weight from the captured source slot0 rather than a constant.
The final [Elite shared report](../analysis/frontend/profile-regression/frontend-pose-visible-elite-retained-shared-comparison.json)
matches715/715 final world-node words exactly, and the
[Spartan shared report](../analysis/frontend/profile-regression/frontend-pose-visible-spartan-retained-shared-comparison.json)
matches429/429 words exactly, covering all88 nodes together. Both local banks
are exact. The earlier
[Spartan report](../analysis/frontend/profile-regression/frontend-pose-visible-spartan-shared-comparison.json)
retains the88/429-word left-arm/hand discrepancy before attachment retention
was corrected. The initialization proof below isolates and fixes its cause;
the reports preserve all expected and actual words without offset or tolerance
tuning.

`frontend_model_ik_tests` checks all60 source IK cases, the retained aim-state
transitions, both original/live deferred hierarchy cases and the attachment
initialization bank/fields below. Model material/color/shader and framebuffer
parity remain separate requirements; these kernel proofs are not a100% visual-parity claim.

## Actual primary producer and authored placement

The initial, nondeferred sampling producer is retained separately from the
visible render admission. The paired
[Spartan sample](../analysis/frontend/profile-regression/original-model-primary-spartan-sampler.json)
and [Elite sample](../analysis/frontend/profile-regression/original-model-primary-elite-sampler.json)
stop at BDB60 with actual caller return BD358, then retain BDC34's completed
local bank. They read the channel, masks, all sampler arguments, SSE frame and
weight registers, codec descriptors and global decoder settings at the same
owned guest stop. No native frame or mask value is supplied to the original.

| Unit | Primary clip | Retained frame word | Sampler weight word | Interpolation byte | Initial node count |
| --- | --- | --- | --- | --- | --- |
| Spartan | 132 | 42304D98 | 3F800000 | 0 | 21 |
| Elite | 73 | 41111B28 | 3F800000 | 0 | 39 |

The fractional retained frame is unchanged before and after local sampling
and equals the actual 2798A0 frame register. The final grip calls retain clips
43/99 at frame0. The interpolation argument is a byte; the unused upper stack
bytes are preserved but are not interpreted as a boolean. Thus the discrete
sampling choice is an observed producer input, while the animation clock
continues to retain its fractional frame.

The [literal primary fixture](../analysis/frontend/profile-regression/frontend-model-primary-original.json)
executes unchanged 1C6F60, 2798A0 and the actual packed codecs. Native
`FrontendModelPrimary` retains the disc's packed integer keys and raw float
components directly. The focused regression matches54 original cases and
6,480 component words exactly; the actual visible Spartan and Elite local
banks match264/264 and440/440 words. Production loads these literal inputs for
both the combat and successful UI clip, and passes the retained clock directly
to both stages of the shared pose helper. It does not reconstruct packed
rotations from previously decoded floats.

Placement is tested independently of animation and the captured node bank.
[freeze_frontend_model_roots.py](../tools/freeze_frontend_model_roots.py)
executes unchanged BDC40 using all ten paused original object root inputs.
The [root fixture](../analysis/frontend/profile-regression/frontend-model-roots-original.json)
maps names through exact scenario position and definition identity. The
[native root test](../tests/frontend_model_root_tests.cpp) independently loads
production `LevelContent`, builds its authored Euler basis, and applies the
same `source_object_transform` used during frontend preparation. All130 root
float words match, covering position, facing and scale of all ten objects.
In particular the authored Z is already -50.18844223022461; no position
correction is added. Spartan authored scale0 resolves to1 and Elite retains
its authored0.85 scale through the source object scale rule.

The [focused run](../analysis/frontend/profile-regression/model-integrated-focused-test.log)
passes all eight model checks. Literal local values and authored roots do not
alone establish the final world callback result or rendered framebuffer.
The complete shared-path comparison above reports any differing matrix words.
Physical reciprocal/inverse-square-root seed limits, material colors, shaders
and skinning/raster output remain separately stated.

## One-time weapon attachment initialization

The initial shared Spartan comparison differed only in the world IK chain,
although primary/grip locals and the other24 world nodes were exact. A
[target isolation probe](../analysis/frontend/profile-regression/model-attachment-isolation.json)
uses each actual captured right-hand parent bank. Recalculating attachment
alignment at that later frame changes36–50 weapon-bank words. Feeding the
source child's stored local fields to the same native object transform and
weapon pose instead matches every captured weapon-bank word for all ten
objects. This establishes the differing input boundary independently of body
sampling or a visual offset.

The [cold-boot attachment trace](../analysis/frontend/profile-regression/model-attachment-initialization-original.json)
retains B94F2's actual parent banks and B97AF/B97B4's output/stored fields for
all ten weapons. It uses logical hardware breakpoints in the owned original
session without guest function calls or code/data patches. All90 stored local
float words are unchanged in the earlier live snapshot at animation frames
41–46, including objects whose primary subsequently changed to UI132.
Original B8EE0/B93B0 aligns, links and stores the child frame once. BD090 later
builds its world nodes from those retained parent-local fields.

The source bank at attachment predates the equipment request. B7B40 initializes
the default controller at B8100, runs BD020 at B8265, then dispatches the object
post-creation handlers at B82E6. The unit handler C4820 calls CCAB0 to create
the starting weapons. The independent
[executable ordering/selector audit](../analysis/frontend/profile-regression/model-startup-order-audit.json)
retains the literal call sites and actual type0 handler tables, and executes
unchanged1CC2F0/1DB170 for all ten graphs. It is a bounded ordering and selector
proof together with the live parent trace, rather than full constructor
emulation. At B94F2 the manager already names equipped combat
47/73 at frame0, but its bank retains the earlier default combat/unarmed/any
idle80/172 pose. Sampling equipped47/73 or using the render-model bind pose
fails12/13 hand words per object; neither is used as an initialization guess.

The existing source default selector `load_initialized_object_idle` resolves
80/172 from the original category/action tables. Its literal packed frame0
sample, before grip/world callbacks, reconstructs all ten original attachment
hand matrices exactly. Applying the original marker alignment and parent-local
conversion matches all90 original child fields. The
[constructor regression](../analysis/frontend/profile-regression/model-attachment-initialization-original.tsv)
checks130 hand words and90 local words using production-authored scenario
roots; the [focused run](../analysis/frontend/profile-regression/model-attachment-focused-test.log)
reports zero differences.

Production now computes that startup attachment frame once per prepared
model and retains it across all animation ticks and UI clip requests. The
shared final pose helper composes the retained local frame with the current
right-hand node before world IK. The independent comparator derives the same
initial frame from source assets and the supplied root, without importing
captured per-object child fields. This closes the measured final body-bank
discrepancy for both species, while the separate skinning/material/shader and
framebuffer comparison boundaries remain open.
