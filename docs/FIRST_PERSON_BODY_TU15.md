# First-person body placement in TU1.5

Current source (2026-10-28) replaces the solid Spartan waist cap with 2,892
unchanged authored lower-abdomen and leg vertices and transports the completed
ordinary eye to network recipients. It does not draw a full upper chest. The
[frozen 35-check cohort](../build/test-results/player-shadows-20261028/frozen-validation/cohort-v5-x11/report.json)
passed, including all eight visible body GPU cases and four native body CPU
cases. The [controlled joined Original capture](../analysis/first-person-body-tu15-20261028/joined-body-gpu-capture.json)
separately passed at an exact authority endpoint with prediction disabled.
This source has not been promoted to a launcher-selected runtime by this work.

Historical 2026-10-25 status: the earlier eye-only correction passed ten feature
checks, including visible Chief/Elite standing and crouching cases. At that
delivery both launchers selected the qualified runtime, and owned scratch/capture
cleanup was verified in the
[completion receipt](../analysis/first-person-body-tu15-20261025/completion.json).
These historical results do not establish the current torso extension, full-game
or Xbox framebuffer parity.

The target is the original English Xbox TU1.5 executable and its authored
player representations. The prepared MCC caches supply native runtime data.
The reported symptom is a body that appears to float or crowd the camera when
the player looks down. Seeing the player's body in that view is intentional;
its appearance alone does not establish correct placement.

## Original body and camera contracts

Original [168DED](../analysis/multiplayer/title-update-1.5/decompiled/functions/00168ded.c)
resolves the representation's separate `+0C` first-person body render model.
It obtains the current unit WORLD-node bank through `BAC30`, checks the body
and unit render models' node counts and checksums, and compares the authored
body-model `+70` threshold against camera forward Z. Its body submission uses
mode `0` (WORLD); arms and weapons use mode `1`.

The authenticated [168DED instructions](../analysis/animation-interpolation-20261015/collision-attachment-original-v2/00168ded.asm)
show `BAC30` at `168F07..168F12` and the body submission at
`1690D1..1690ED`. [BAC30](../analysis/multiplayer/title-update-1.5/decompiled/functions/000bac30.c)
returns `unit + signed16(unit+116)` with count
`signed16(unit+114)/34h`. For the body, the original
[168D76 instructions](../analysis/animation-interpolation-20261015/collision-attachment-original-v2/00168d76.asm)
receive no camera root or node remapping and copy the WORLD matrices verbatim
at `168DD1..168DE5`. A compensating body translation would contradict that
contract. The completed render-pose bank after later `BF640` processing is a
different boundary from this direct `168DED` output.

The normal camera follows `245400 -> 245060 -> CD4A0 -> E18D0`.
Authenticated [245060 instructions](../analysis/camera/vehicle-tu15-completion-20261015/director-death-generated/00245060.asm)
at `2450F1..2450F8` pass the actual unit and packet position to
[CD4A0](../analysis/multiplayer/title-update-1.5/decompiled/functions/000cd4a0.c).
That owner computes the authored standing/crouching eye height, invokes
[E18D0](../analysis/multiplayer/title-update-1.5/decompiled/functions/000e18d0.c)
for the downward-look head correction, and performs the two original
`16C620` obstruction queries. `E18D0` consumes the definition's pitch range
at `+224/+228`, correction scale at `+22C/+230/+234`, minimum forward
displacement at `+238`, persistent unit head state at `+3C8/+3CC`, and the
actual node-zero translation. It is already lowered in the native TU1.5
physics support owner.

The separate [unit-center research](SOURCE_UNIT_CENTER.md) documents the
base-disc equivalent `DF380`. `DF41D` is an instruction inside that base-disc
routine, not a TU1.5 function entry; those addresses must not be interchanged.

## Native correction boundary

[NativeVehicleCamera](../src/gameplay/vehicles/native_vehicle_camera.cpp)
runs the complete player camera packet, observer adoption, motion and world
composition for an ordinary view. Before correction, its public optional
result admitted vehicle/death/handoff presentation and discarded the ordinary
result. The multiplayer renderer then used `ordinary_unit_eye`, which supplies
root plus standing/crouching height and omits the head correction and its
obstruction queries.

The implemented correction exposes the already completed ordinary result
through an optional output while preserving the existing return admission.
The renderer retains that eye separately as `source_player_camera` and reuses
its position for the main view and first-person effect/model preparation.
Vehicle, death, seat-exit and respawn handoffs keep their existing semantics;
locally admitted camera owners expose the same completed result. The body
WORLD matrices and authored geometry are unchanged.

The existing live PC FOV path remains a separate presentation contract.
`source_observer_world` has already applied the original FOV-dependent
screen-offset rotation to its completed forward/up basis and HUD aim
displacement. Live multiplayer independently applies its current user FOV,
modern angle scale and scope projection. Replacing that orientation or HUD
displacement with the completed original values would bypass this behavior;
applying `source_observer_aim` again to the already offset basis would rotate
twice. The narrow eye correction therefore preserves the existing live
orientation, FOV, scope and HUD aim calculation. Any later adoption of complete
original orientation must explicitly reconcile those inputs. The implementation
therefore copies only the source eye position into `player_camera`.

The focused target also exposed the existing static archive dependency from
`halo2_physics_world_runtime` to `halo2_systems`. The
[CMake declaration](../cmake/PhysicsLiveWorldTu15.cmake) now records that private
dependency so the linker can resolve the archive cycle. It does not add a
duplicate `SourceRuntimeMemory` implementation to the test executable.

## Authored body metadata

The bounded [metadata audit](../analysis/first-person-body-tu15-20261025/body-metadata-audit.json)
reads original Lockout and the selected prepared MCC `shared.map`. The
requested Chief/Elite models match by authored name, regions, permutations,
material names, node count and threshold; cache-local datums differ.

| Representation | Nodes | Regions and sections | Forward-Z threshold |
| --- | ---: | --- | ---: |
| Chief | 33 | `body`: section 0 | -0.5 |
| Elite | 55 | `legs`: 0; `lights`: 1; `rightarm`: 2; `torso`: 3 | -0.30000001192092896 |

Each region has exactly one permutation and selects the same section at all
six authored LODs. The original first-person collector initializes its temporary
permutation bank to zero even when one of its records has WORLD mode `0`.
Consequently, the current default-permutation loader and original collector
choose the same inspected sections; there is no evidence here for a region or
LOD selection cause. Elite's authored FP body includes torso and right-arm
geometry as well as legs.

The audit records input/source hashes and its read method. All recorded probe
scratch directories were verified removed. Only the small JSON report is
retained; no generated binaries, raw frames or other large outputs were kept
by this audit. This metadata comparison is not GPU draw or framebuffer proof.

## Measured regression and feature validation

The [current-cohort baseline](../analysis/first-person-body-tu15-20261025/baseline-native/report.json)
fails all eight intended camera checks: four CPU cases and four GPU-submitted
cases across both representations and stances. The CPU cases retain 105
original scheduler/observer calls each but publish zero ordinary native views.
The submitted camera differs from the independently completed original eye
by the following distances in world units:

| Representation | Standing | Crouching |
| --- | ---: | ---: |
| Chief | 0.12558420 | 0.07918494 |
| Elite | 0.16374357 | 0.17325024 |

Those submitted baseline frames retain exact body nodes, posed vertices and
the existing user-FOV aim policy. They occur behind startup fade, so they prove
the camera handoff defect at an actual draw boundary, not a visible final
image comparison.

The [final feature run](../analysis/first-person-body-tu15-20261025/fixed-visible-final/report.json)
passes all ten selected checks. Four CPU cases retain 420 exact original
camera outputs, compare 2,640 body WORLD nodes exactly, and check 145,890
skinned vertices. Four GPU cases submit 32 actual frames each, for 128 total;
each includes 22 frames after the original fade clears. Every final camera-eye
error is `0.0`, the existing user-FOV aim policy remains exact, and the actual
final RGB captures are nonblack and nonuniform. The presentation regression
passes 4,772 samples, and admitted numeric-camera replay also passes.

The renderer observes the original fade owner until its activation and opacity
clear naturally. Merely advancing time before any rendering does not finish
the original startup fade. No opacity or successful fade result is supplied.
The inspected Elite capture places the legs below the viewpoint. These captures
were transient validation outputs and have been removed with the owned build.

| Case | Result |
| --- | --- |
| Chief standing, looking down | Passed CPU and visible GPU checks; exact source eye, WORLD nodes and posed vertices |
| Chief crouching, looking down | Passed the same checks with completed source crouch/head state |
| Elite standing, looking down | Passed against Elite's authored model and threshold |
| Elite crouching, looking down | Passed with completed source crouch/head state |
| Horizontal/upward look | Original admission retained; no unconditional body draw or manual offset |
| Consecutive render samples | Passed presentation sampling; camera/body endpoints remain paired |
| Discontinuities | Passed presentation cuts across unit/player replacement, respawn, teleport, seat and parent changes |
| FOV and scope | Existing calculation and classification preserved; measured live user-FOV aim outputs exact |
| Native visible acceptance | Passed four stance/representation cases after natural fade completion |
| Launcher delivery | Passed qualification, both launcher selections and the installed source-hidden down-look probe |
| Storage closeout | Metadata probes and the complete owned build/source/save/cache/capture tree removed and verified |

## Historical delivered runtime and storage closeout

The [qualified delivery](../analysis/first-person-body-tu15-20261025/delivery-qualified/report.json)
passes 13 map/view smoke cases and eight launcher tests, including all 45
promotion-helper cases. Both launchers selected
`bin/engine-builds/1b0fce4631494e81b0c841861b8de638/halo2_native`, SHA-256
`a877f0cc28493c65094ab4ca23aacf2f3caebf5a5512b553c351beaab703e9fe`.
The [installed down-look probe](../analysis/first-person-body-tu15-20261025/installed-look-down.json)
runs that selected binary for 89 submitted frames and 90 ticks with the entire
private source tree hidden. It passes, its visible capture was inspected, and
the source tree was restored afterward.

Packaging preparation materialized the 17 MiB shader-policy inputs needed by
the owned source snapshot and refreshed its packaging helper and fixture.
The helper audits source-program and bitmap payloads as two separate
components and rejects unknown neighboring paths; all 45 regression cases
pass. These packaging changes do not establish loading functionality.

The complete root-owned `halo2-test-7h5oo6vc` tree was removed, including its
build, source, saves, caches and captures. The completion receipt verifies both
scratch and capture removal. The only newly retained large artifact is the
selected runtime/resources directory
`bin/engine-builds/1b0fce4631494e81b0c841861b8de638`, totaling 3,140,258,917
bytes (3.14 GB). Earlier selected runtimes, original assets and existing
reference evidence are preserved.

A deliberately forced constructor fallback is a setup failure, not evidence
for this defect. Original Xbox framebuffer/hardware parity remains separate
from these native CPU/GPU and presentation results.

## Connected Spartan torso and peer eye, 2026-10-28

The clarified complaint is the **solid waist cap**, rather than missing faces.
A finite down-look replay of installed runtime `9d531caa3b6a4b0c9284b954c3874139`
reproduces that silhouette in Original, pStencil and PathTracing. Its actual
Spartan FP body is a closed manifold: 976 unique triangles, 2,928 expanded
vertices and 1,464 edges, each shared by two triangles. Reversing native GPU
culling exposes the inside faces and removes world faces, rejecting that
counterfactual as a correction. Authored Chief/Elite eye parameters and bind
matrices agree between the original Lockout and the prepared MCC shared cache.

The original lower-body model deliberately ends at a solid waist cap. This PC
presentation extension keeps `168DED`'s authored down-look admission and selects
the full Chief model's native `body` and `legs` resource bank through the native
permutation/section selector. The original FP geometry policy uses LOD4; the
actual Chief mapping selects sections 17/19 and omits arms 16 and helmet 18. The
primary self-view then omits complete authored triangles with any active skin
influence from the named `spine1` subtree. It retains 516 lower-abdomen vertices
and all 2,376 leg vertices, for 2,892 vertices from the 4,152-vertex bank. The
authored cap model is not drawn, and no vertices, triangles, cut planes or eye
offsets are invented. The separate first-person arms and weapon path supplies
the foreground. Full-body shadows and PathTracing secondary geometry keep the
complete native body. Representations without four independent body/legs/arms/
helmet regions retain their original FP-body model; Elite's body region also
includes its arms. This is an explicit PC lower-torso visibility policy, not a
claim of the original Xbox first-person silhouette or a visible full chest.

The native mode0 distinction remains significant. Original `3D4F0` returns each
cached entry's mode, and the TU1.5 ordinary draw owner tests flag `0x800` at
`4D694` before enabling `12D30`'s foreground projection scaling. A mode0 body
keeps WORLD projection despite belonging to the cached FP group. The source
node-bank owner also suppresses `BD610/BF640` for that cached group, so its
direct `168DED` matrices remain the owner of the authored lower-body model.
The Chief draw uses the native completed full-unit WORLD publication, normal
body interpolation/prediction and the full model's own inverse palette. Initial
full-torso captures were rejected: a giant dark mass and a left plate obscured
the view. An actual tick 90 GPU probe then compared all 4,152 posed vertices with
the native full WORLD collector's current `1D690` palette and found zero
position differences. Changing pose ownership did not resolve the obstruction.
The exact offending head/neck and spine1 triangles were in front of the native
near plane, so near clipping alone also could not resolve it. Body region 17
itself contains head/neck tissue and upper chest in one authored part; omitting
helmet region 18 does not hide that tissue. A neck-only visibility candidate
removed the giant mass but retained the left plate. The spine1-subtree candidate
removed both while retaining the uncapped lower abdomen and complete legs.
Actual GPU images at steep standing, steep crouching and shallower standing
angles were inspected before accepting this presentation policy.
The bounded [candidate receipt](../analysis/first-person-body-tu15-20261028/anatomical-body-gpu-candidates.json)
keeps camera words, unchanged native-position comparisons and subset counts;
its temporary binaries, vertex banks and captures were released afterward.

A separate real-UDP joining-client Original-mode capture reached a steep,
grounded Spartan endpoint without a gameplay WORLD on that client. It submitted
2,892 lower-body vertices; all 33 WORLD nodes and every selected position matched
the authority's independently materialized native `1D690` palette across eight
endpoint frames. The submitted eye also matched the recipient's ordinary native
camera exactly. Inspection of the actual frame found no solid cap, head mass or
shoulder wedge, with the natural hip and leg below the crosshair. This bounded
check used the existing `H2_NET_PROFILE=unlimited`
and `H2_NET_PREDICTION=0` diagnostic settings so the independent oracle observes
the same authority endpoint. It does not qualify predicted-root displacement,
all online graphics modes or native online fixed shadows. The
[joined capture receipt](../analysis/first-person-body-tu15-20261028/joined-body-gpu-capture.json)
records those settings, owner/model bytes, camera values and temporary image
hashes. Its private executable, object copy, raw geometry, logs and captures
were removed after inspection; the [storage closeout](../analysis/first-person-body-tu15-20261028/joined-body-storage-closeout.json)
checks every probe owner and retains no large artifacts.

The investigation separately found a peer-camera defect. The host completed
the ordinary native observer for local views but discarded it when responding
to a peer; the state codec transported only vehicle/death cameras. It now keeps
the genuine completed ordinary result for each peer and transports that view
only to its recipient. Remote clients continue to use the same native eye/head
correction. Movement prediction translates this eye and the mode0 FP body by
the same displayed-root displacement while preserving native relative node
poses, bases and lens words. Missing snapshot intervals retain the completed
ordinary basis/lens and carry the eye with the interpolated root; teleports and
large position cuts reset FP pose history.

The native GPU check compares submitted Chief vertices with the anatomically
selected subset of full-model sections 17/19, records the source FP model
separately, and checks the native full-unit WORLD nodes and camera eye. After
the captured runtime returns, it starts a fresh original render frame at the
unchanged simulation endpoint, independently materializes the actual native full
WORLD collector's LOD4 palette and checks every retained submitted position. This
avoids reusing pStencil's legitimate same-frame main-model LOD5 cache; it does not
claim a draw-time LOD4 selection by that main-model collector.
An independent lower-anatomy name predicate checks the selected triangles;
asset checks require every leg triangle and nonempty lower abdomen to remain.
The nonhost presentation regression covers real movement prediction with independent pose
and animation fractions, dropped snapshots, corrections and a teleport. Its
pre-fix command fails with `Native looking-down eye detached from the predicted
body root`. Bounded reports are under
`analysis/first-person-body-tu15-20261028/`. Current GPU acceptance is now recorded
below; launcher qualification remains separate before replacing a selected runtime.

The final coherent GPU matrix passed Chief standing/crouching in Original,
pStencil and Path Tracing, plus Elite standing/crouching in Original. Each case
submitted 32 actual frames, retained exact native eye/pose/vertex comparisons,
finished the original startup fade naturally and produced a nonuniform visible
capture. Chief retained all 2,892 selected vertices; Elite retained its authored
1,935-vertex FP-body policy. All eight cases recorded actual SDL driver `x11` and
passed prepared-World observer reuse, normal/exception capture release and
exclusive-owner rejection controls. Detailed small reports are in the
[final cohort evidence](../build/test-results/player-shadows-20261028/frozen-validation/cohort-v5-x11/report.json).

Earlier crouch/input failures remain retained failed evidence. An owned SDL
probe confirmed that inherited Wayland remained selected despite xvfb's DISPLAY.
Only xvfb-wrapped tests now explicitly select X11 and assert the actual driver;
direct callers retain their normal backend. The original real SDL input
assertions remain unchanged, with bounded event/focus diagnostics and no event
filtering, reinjection or expected-control substitution.

The asset-only [geometry receipt](../build/test-results/first-person-body-geometry-20261028/receipt.json)
passes against both the original Lockout and prepared MCC Lockout. Each prepares
the 4,152-vertex body/leg resource bank and retains the exact authored vertex,
skin-binding and provenance records. The updated anatomy check selects 2,892
unchanged lower-torso/leg vertices from that bank and retains all leg triangles.
The full model's own inverse palette is required: its authored root
bind differs from the capped FP model, while the other 32 node records match.
The native peer-eye transport, predicted-body/eye presentation and host-sidecar
schema have separate bounded reports; these CPU/asset checks do not replace
the required visible GPU checks in all three graphics modes.

For joined PathTracing clients, the visible torso/legs representation is also
kept separate from secondary-ray geometry. Protocol 122 appends the authority's
actual object definition and current native `16D660` permutation bytes beside
each living biped/held-weapon WORLD pose. This lossless metadata sits outside
the Rust bandwidth codec's unchanged `H2WP1` matrix range and is rebound by
the complete salted owner within the same accepted snapshot. Earlier body
versions through 121 preserve their exact prior bytes and carry no such bank.

The joined renderer resolves those current choices through native `4C2B0` at
the explicit secondary-ray LOD4 policy and skins the full native model with
its own inverse palette and the same interpolated/predicted WORLD nodes used
by other WORLD presentation. Authoritatively hidden regions stay absent.
Its full local-body bank replaces the visible partial-body geometry for
secondary rays; ordinary first-person arms and the connected torso remain
the primary representation. Offline secondary geometry retains its completed
native collector selection and copies its current resident palette before
later native queries reuse the frame arena. Geometry cache reuse does not run
`137800` again or publish another WORLD-node observation; secondary rays consume
the actual current collector token's base/compound and region-map matrices in
that case. A missing fresh node observation must not cause a SIM fallback.
The chosen online LOD4 policy is not a claim that
a camera-dependent native collector selected that LOD or completed its
resource-residency branches remotely.

The production C++ snapshot and acknowledged Rust stream regression preserves
these exact model bytes, rejects missing/foreign region ownership and verifies
unchanged 110..121 layouts. The original/MCC asset regression checks full
Chief sections 16/17/18/19 and an authority-hidden helmet. A real native UDP
warmup witness additionally checks the living unit and held weapons against
their source definition and current region banks. Coherent offline native
execution is now recorded by the frozen cohort. A joined Path Tracing GPU shadow
capture remains required before claiming delivered online shadow coverage; the
declared transport and asset checks alone do not prove it.

The [current storage closeout](../build/test-results/player-shadows-20261028/frozen-validation/cleanup.json) verifies removal of the complete private source/build and raw outputs after the final 35 tests and both 960×720 fixed-shadow checks. No new large artifacts were retained. The new torso/eye source has not been promoted to a launcher-selected runtime by this work.
