# First-person body placement in TU1.5

Status: the native eye correction passes all ten current-cohort feature checks,
including visible Chief/Elite standing and crouching cases. Both launchers now
select the qualified runtime, and owned scratch/capture cleanup is verified in
the [completion receipt](../analysis/first-person-body-tu15-20261025/completion.json).
These results do not establish full-game or Xbox framebuffer parity.

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

## Delivered runtime and storage closeout

The [qualified delivery](../analysis/first-person-body-tu15-20261025/delivery-qualified/report.json)
passes 13 map/view smoke cases and eight launcher tests, including all 45
promotion-helper cases. Both launchers select
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
