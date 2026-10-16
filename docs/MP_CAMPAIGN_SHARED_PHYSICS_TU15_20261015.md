# Shared campaign physics in the multiplayer world

The goal remains all player, vehicle and object physics at 1:1 English retail
TU1.5 behavior. Full parity is incomplete. OG Xbox TU1.5 is authoritative;
host i386 comparisons and native screenshots do not prove Xbox hardware or
framebuffer parity.

## Live integration and original ownership

`MultiplayerPreparedWorld` admits original kind1 campaign content and difficulty
alongside kind2 multiplayer. Campaign representation1, source membership/team1,
the actual Dervish body and unarmed current-weapon NONE survive original player
birth, first-person registration, UDP publication and client adoption. The
source campaign engine has index0 and a NONE callback; it cannot be ended by
the native multiplayer Slayer participant rule.

Actual original C93D0 constructs the unit input record and C92C0 delivers declared
NPC throttle/facing. CE440 remains the real controller admission owner. No
velocity, pose, contact, force or solver result is supplied. Original autonomous
AI, scripts and cinematic contexts remain separate unfinished owners.

The original constraint instance constructor, callbacks and release paths now
have nine whole TU1.5 bodies (813 instructions) wired into the live dispatcher.
All27 whole targets of the three original animation-composition dispatchers
(7153 original instructions) are now compiled and connected. Two branches
reached by actual SDL mouse-look were missing; both retain their genuine codec
callbacks and now pass complete original-call comparison. Other selector/mask
combinations still need execution coverage. The
reached full-call comparisons retain all accessible source memory and CPU/FP
state rather than comparing only selected output scalars.

Campaign UDP state carries all authored nonvehicle roots, completed WORLD pose
banks and the separate vehicle placement/transform/model-region bank. The
receive socket requests a bounded larger buffer for complete fragmented
snapshots; wire limits and state coverage are preserved. Device ownership binds
before source scene adoption. First adoption rebuilds derived prop/scenery
collision owners, and the first accepted host bank replaces the temporary
client world even at equal tick zero. Prepared machine/scenery resources stay
inactive until an actual source lifetime binds them. Client startup does not
bind temporary local handles into source presentation owners.

Unarmed source bodies use their actual WORLD nodes without a fabricated default
held weapon, lighting recipient, weapon attachment or IK/draw submission. Actual
non-NONE missing resources still fail explicitly.

## Campaign fog needed for the live world

`BspAtmosphere` loads literal 244-byte atmospheric palette rows from scnr+340.
TU1.5 131AC0 supplies direct primary/secondary, planar override, patchy and bloom
aggregation, including sky contributions to the planar denominator. 1329B0,
132CD0 and 1330E0 supply planar overrides, immersion flags, clamps, containing
planar behavior and eye/transition composition. The sampler is wired into each
ordinary rendered view. It is not a replacement tint or a bypass of fog.

`tools/differential_campaign_fog_tu15.py` runs the unchanged pinned TU1.5 image,
its real static initializer and full fog sample, using actual cache/tag/BSP
backing plus declared eyes, forward vector and forced cluster selectors. It
replaces no instruction, callee or aggregation result. All four BSPs of
06a_sentinelwalls pass 16,398 centroid/portal queries: 672,318 published float
fields match exact binary32, including signed zero, and every classification
matches. These forced-cluster math/metadata cases do not prove actual camera
leaf retention or Xbox pixels. Live mixer interpolation inputs and nonzero
patchy secondary-color rendering remain explicit unsupported owners; this map's
12 palette rows have zero mixers and zero secondary patchy colors.

## Current evidence

Paths are relative to `analysis/multiplayer/physics-live-tu15-20261012`.

| Evidence | Proven scope |
| --- | --- |
| `campaign-client-adoption-v497/report.json` | Complete actual three-player UDP snapshot; 684 nonvehicle roots, 741 WORLD pose banks and 54 vehicles; normal client device-first adoption and strict lifetimes. |
| `campaign-fog-all-bsps-v509.json` | Four actual campaign BSPs; 16,398 queries; 672,318 exact binary32 fields; no substituted fog callees. |
| `campaign-fog-mp-regression-v509.log` | Existing 21-map multiplayer atmosphere regression passes. |
| `campaign-class3-live-v516/lan-campaign/report.json` | Actual shared campaign authority, three source players, two rendered views, 156 frames, 434 exact UDP states, 42 ordinary SDL forward commands; source Enforcer rendered in each view on every frame. |
| `campaign-visible-controller-original-v517/report.json` | Complete captured actual controller delivery: 79 original instructions match. |
| `campaign-visible-object-original-v517/report.json` | Complete actual visible-session object update: 61,555 original instructions match. |
| `campaign-visible-world-original-v517/report.json` | Complete actual visible-session world solve: 148,444 original instructions match. |
| `vehicle-camera-animation-v527/lan-vehicle/report.json` | Actual Warthog driving, steering, enter/exit; 902 exact UDP observations, 501 original seated-camera frames, 0.6-radian camera look turn, 594 frames after return to the on-foot camera. |
| `normal-runtime-delivery-v528/execution.json` | Ordinary production entry point, authored spawns and no physics fixture or spawn override: 120 rendered frames, 349 source ticks, two local players. |

The Enforcer session retains 30 identical neutral/control ticks before the NPC
throttle changes; causal separation reaches0.6157966 and observed travel reaches
0.6132634. Its immutable executable is
`tested-runtime-v516/campaign_multiplayer_acceptance_tu15`, SHA256
`75c5470342490dcc6e3c866c5e82f73d17ef54ffe483d3d7491846c6acb44bb0`.
The native RTX5080 Vulkan window was continuously mirrored to the desktop and
recorded for17.325843 seconds. The initial floor-level views crop the upper
Enforcer; source motion is visible, but full silhouette framing is not claimed
by this recording. `campaign-class3-look-v525/lan-campaign/report.json` also passes163 rendered
frames per view,436 exact UDP states and a0.6-radian camera turn from an actual
-240-pixel SDL mouse gesture. Its15.433333-second video shows the upper Enforcer
in the top view and its moving lower body in the independent bottom view;
architecture still occludes parts of the model. `visual-review.json` retains
that limit. The immutable v524 acceptance ELF is SHA256
`29474bc281b8e67cb28abe3c672c8330156a8bc4caad8cf8ae5c4945f05fa598`.

`campaign-look-animation-original-v526/report.json` and
`campaign-look-rotation-original-v526/report.json` match5549 and5019 unchanged
original instructions respectively, all60,084,310 accessible bytes per call,
and compared CPU/FP state. These are the real calls reached by the mouse input;
no decoder output, channel, mask or pose is supplied.

The three fresh visible-session original calls match210,078 instructions and
60,084,310 accessible bytes per replay, plus compared GP/SSE/x87/FP state under
the documented host interrupt-flag boundary. They are sampled whole calls, not
complete branch coverage or Xbox hardware trajectory proof.

The Warthog video was inspected seated at8seconds and after exit at25seconds;
its `visual-review.json` records the actual vehicle and camera behavior. The final `vehicle-camera-animation-v527/lan-vehicle/report.json` repeats the
actual Warthog test after all27 composition targets are connected:1121 rendered
frames,902 exact UDP states,501 actual original seated-camera frames,
0.5999999642-radian look turn and594 frames after exit. Its seated and exited
recording frames were inspected again. The production `Halo2-Physics-TU15`
launcher now selects tested v526, SHA256
`1e9189a9336f0043c3a8eb6cb0538d5f17b1268d1b3706ef10889b53771dca49`.
`normal-runtime-delivery-v528/execution.json` passes120 ordinary rendered frames,
349 source ticks and two players, again without physics fixture/spawn overrides.
Other launchers and unrelated root changes are outside this owned delivery.

The animation generator retains its exact lowering backend as
`tools/generate_animation_composition_backend_tu15.py`; the shared and frozen
general helpers have independent revisions. `animation-all-reproduction-v529b.json`
proves both source trees regenerate the byte-identical tested7153-instruction
module (SHA256 `a980ed539456d1caa84dc7dfc76095983cec8e9755af8e1baf91ef0531c94af0`).
The prior differing-backend attempt stays recorded as a failed reproducibility
check; no unrelated general generator was replaced.

## Required remaining gates

The full goal remains active. This three-player native stress roster and declared
NPC input are not proof of retail cooperative admission or autonomous campaign
AI/scripts. The other campaign-only Pelican definition, all47 authored
placements, intro Pelican cinematic context, successful cloth simulation,
remaining fourth Ivory Tower paddle and Terminal train rider contexts, fixed
human turret target damage, broader collision/controller/contact/lifetime and
corpse/ragdoll branches, quantitative Xbox TU1.5 trajectories and framebuffer
comparisons remain required. Existing Banshee host replay kernel-wait evidence
also stays unresolved. See the full physics goal and each subsystem's retained
matrix; this checkpoint does not shrink the objective.

## Source ownership and late water material integration

`campaign-phantom-live-v530/lan-campaign/report.json` passes the actual
06a_sentinelwalls BSP0 authored Phantom placement2:138 rendered frames in each
view,342 exact UDP observations,32 SDL forward commands and30 identical neutral
prefix ticks. Original controller stimulus separates the source and neutral
positions by16.7583904; observed travel is16.7563801. Its inspected recording
shows the moving silhouette and blue engines in both views, with a very dark
hull and restricted framing. `visual-review.json` explicitly leaves Xbox visual
parity unproven. `campaign-phantom-world-original-v532/report.json` matches the
complete captured original world call:60,551 instructions and all60,084,310
accessible bytes plus compared CPU/FP state. The exact authored definition and
map hashes are retained in `remaining-class3-original-targets-v538.json`.

The actual05b_deltatowers starting profile points to the beam rifle. Source
physics authorities execute original weapon owners; UDP replicas consume their
completed state. `MultiplayerMatch` now declares that replica role explicitly,
keeps native-only trigger/energy admission checks, and rejects any replica step
without an actual source physics owner before advancing its tick. The actual
Throne world regression `campaign-throne-ownership-v536/report.json` passes
327 adopted SCEN roots,331 WORLD poses,three UDP players, the native weapon
rejection and the replica no-step invariant. This does not supply a native
beam-rifle energy result or prove source starting-equipment activation.

The Throne visible runv537 reproduced an admitted cinematic material rejection.
The actual authored shader has constant-color source factor0x8001, inverse
source-alpha destination0x303 and constant ARGB0x00FFFFFF. The existing mode5
preserves white RGB and zero source alpha. `cinematic-material-green-v540.json`
passes the actual original layer15/program/constants/samplers and RGBA/depth
state, and `cinematic-material-mp-regression-v540.log` passes the existing21-map
material suite. No ordinary premultiplied-alpha substitute was used.

The same actual Throne runv541 then rejected a later pool material. Parent
GDBv544 identifies resident root6867 as
`scenarios\shaders\solo\deltatemple\tunnel_pool_water`, forward vertex7 and
water definition-1. The authenticated English TU1.5 full listings and source
instructions are retained in
`campaign-water-source-v542/authenticated-state-binding.json`:4D630/4D6EE loads
BSP+218 directly when BSP+214 is positive, then calls full3C640 and3CFF0 using
the first water record. The runtime now binds pool states after all material
roots are registered. `campaign-water-material-green-v545.json` passes that
actual late pool without changing its original program/constants; absent BSP
water stays unbound. `campaign-water-mp-regression-v545.log` passes the existing
21-map water suite. Continued visible controller/body/world proof remains the
next gate; these material fixes alone do not establish Throne physics parity.

## Actual Gravity Throne controller, body and world proof

`campaign-throne-live-v546/lan-campaign/report.json` passes the actual
05b_deltatowers BSP0 placement0, definition4131919057:1243 frames per view,
1133 exact UDP states,30 identical neutral ticks and2.2999048 source controller
travel/causal separation. Its lower view visibly shows the Throne moving; the
upper view is obstructed, explicitly recorded in `visual-review.json`.

The observer input helper now requires an actual cached-BSP ray to the authored
bounding center, along with the existing floor, location and capsule-clearance
checks. A32-angle/four-distance survey chooses only declared prebirth inputs;
all subsequent physical state remains original-source output. The first
near-ring-only visibility attemptv550 failed explicitly and stays retained.
The successful repeat `campaign-throne-framing-v553/lan-campaign/report.json`
passes1278 frames per view,1243 exact UDP states,30 neutral ticks and2.2999048
travel. Its inspected1s and20s frames show the moving Throne in both views and
the SDL-controlled player's body in the independent watcher. The source unit
is still unarmed while the fallback weapon HUD remains; framebuffer and HUD
parity are not claimed. The immutable v552 campaign runner SHA256 is
`1722e1052a5f42ec76eebfa46061c980170195430c055761e215fa881dd4277d`.

Four complete actualv546 calls are replayed through unchanged English TU1.5
instructions. `campaign-throne-original-whole-calls-v548.json` retains79
controller,1,927,868 object-update,215,840 fullB9090 body-update and386,076
world-solve instructions:2,529,863 total. Every call matches all60,084,310
accessible bytes and compared GP/SSE/x87/FP state under the documented host IF
mask. The object phase also executes its authenticated original D3D callee;
no callee, pose, contact or solver result is replaced. These sampled whole-call
comparisons do not establish all branches or physical Xbox trajectories.
The no-flare Throne/Pelican definitions, other authored placements and every
wider required gate above remain open.

## Current ordinary runtime and actual vehicle camera

The production `Halo2-Physics-TU15` launcher now selects immutable v552,
SHA256 `06082429453d863de211777f3f8b5988ea260e862b3aee212432cd31511ea176`.
`normal-runtime-delivery-v554/execution.json` passes ordinary authored-spawn
startup:120 rendered frames,303 source ticks,two local players and no physics
fixture/spawn override. `vehicle-camera-integration-v555/lan-vehicle/report.json`
passes actual Warthog entry,drive,steering,look and exit with901 exact UDP
states,500 actual original seated-camera frames,a0.5999999642-radian look turn,
39.00917 horizontal travel and572 frames after return to the on-foot camera.
Its8s seated and25s exited recording frames were inspected; source camera and
visible motion pass while whole-frame Xbox parity remains unproven.
The acceptance runner SHA256 is
`0aa6a20e48c906db71c27301c3e68037473e5860b2a3d3f1c3b6852e92b4a58b`.
Root/frozen launcher receipts `launcher-v552-verified-*.json` verify this exact
production pin and the original English TU1.5 XBE. Other launchers and the full
canonical production integration remain outside this dedicated delivery.

## Authored NONE equipment and visible no-flare Throne

`00a_introduction.map` has zero SCNR+F8 starting-profile rows, retained in
`campaign-introduction-unarmed-source-v558.json`. Authenticated TU1.5 campaign
source authorities and replicas preserve that NONE value; native-only matches
still reject unsupported missing equipment. Source births use full original
B1A70/B1C30/1613B0 equipment calls and their completed inventory, rather than
initializing an intermediate native gun/magazine/inventory. The actual source
world regressionv559 passes two unarmed births,source tick advance,54 adopted
SCEN roots,58 WORLD banks and three UDP players. The equipped-profilev562
regression retains its native trigger/energy admission and source ownership.

Player body resources now have a representation/NONE entry independent of
weapon/hand graphs. Actual source WORLD nodes own unarmed body posing. Unit
HUDs load the real biped interfaces with weaponNONE; absent guns have no muzzle,
camouflage or material-animation bank. The first visiblev561 run exposed an
empty first-person bank access; parent GDBv563 identified the exact animation
loop and the repaired lookup skips the absent bank. The failed execution and
stack remain retained, not reclassified as passing.

`campaign-no-flare-throne-live-v565/lan-campaign/report.json` now passes actual
00a_introduction BSP0 authored placement10,definition3915515885:382 frames per
view,242 exact UDP observations,30 identical neutral ticks,0.6774012 observed
travel and0.699815 causal separation. Its inspected1s/6s frames show the moving
no-flare Throne in both council-room views and the independent unarmed player.
There is no substituted gun or fallback weapon HUD. Original console pixels
and autonomous cinematic/AI/HS context remain unproven. Immutable acceptance
v564 is SHA256
`b586f6576e8f6b6864a01586881c0678fc67daa3434349a56d5555ee22065a74`.
`campaign-no-flare-body-original-v567/report.json` matches fullB9090:94,449
original instructions,all60,084,310 accessible bytes and compared CPU/FP state.

The Pelican03b_newmombasa BSP0 placement3 visiblev568 attempt now reaches the
actual world but rejects SCEN preparation that lacks actual RNG-selected region
bytes. `campaign-pelican-world-v571/report.json` separately passes source birth,
706 adopted SCEN roots,711 WORLD banks,three UDP players and two world vehicles.
Those are world/replication facts, not visible Pelican proof. The original
scenery choices must reach the renderer without rerolling or dropping owners.
The older03a intro Pelican BSP3 inactive cinematic context remains separate.

The v566 ordinary production delivery includes the unarmed paths. Normal
startupv570 passes120 frames,322 source ticks and two authored-spawn players.
Fresh Warthogv569 passes902 exact UDP states,480 original seated-camera frames,
a0.6000000238-radian look turn and447 on-foot camera frames after exit;8s/25s
recording frames were inspected. The production pin is
`a36dc424f5a464508dd0088e393e19f54ced99b033ba22d7ef99f70dc842dbf0`;
root/frozen launcher receipts are `launcher-v566-verified-*.json`.
All remaining placements,lift/train riders,collision/lifetime/controller,
cloth/corpse/ragdoll,canonical launcher and Xbox reference gates remain open.

`campaign-no-flare-world-original-v575/report.json` additionally matches the
complete actual1CC8D0 solve:176,810 original instructions and all60,084,310
accessible bytes plus compared CPU/FP state. Together with the body call,
271,259 unchanged original instructions match for this actual visible session.

The read-only source auditv573 proves that actual RNG choices already exist in
the authority:seven bridge-light placements187,196,200,204,208,216,224,junction
box238 and toll booth312. The report retains full source handles,definitions,
variant indices and actual permutation bytes. No reroll or supplied seed was
used. `SourceSceneRoot` currently carries transforms/poses but lacks these model
region banks,and `SourceSceneryScene` prepares geometry before initial client
state adoption. The next integration must carry the actual source regions,
bind the initial authority state before resource selection,and preserve exact
full-handle lifetimes. Later changed selections also need the full resident
collector catalog; the current fixed-selection lifetime guard must stay strict.
