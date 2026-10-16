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
The original additive animation composition owner is also connected. The
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
| `vehicle-camera-integration-v517/lan-vehicle/report.json` | Actual Warthog driving, steering, enter/exit; 902 exact UDP observations, 505 original seated-camera frames, 0.6-radian camera look turn, 465 frames after return to the on-foot camera. |
| `normal-runtime-delivery-v518/execution.json` | Ordinary production entry point, authored spawns and no physics fixture or spawn override: 120 rendered frames, 340 source ticks, two local players. |

The Enforcer session retains 30 identical neutral/control ticks before the NPC
throttle changes; causal separation reaches0.6157966 and observed travel reaches
0.6132634. Its immutable executable is
`tested-runtime-v516/campaign_multiplayer_acceptance_tu15`, SHA256
`75c5470342490dcc6e3c866c5e82f73d17ef54ffe483d3d7491846c6acb44bb0`.
The native RTX5080 Vulkan window was continuously mirrored to the desktop and
recorded for17.325843 seconds. The initial floor-level views crop the upper
Enforcer; source motion is visible, but full silhouette framing is not claimed
by this recording. A separate actual mouse-look demonstration is recorded next.

The three fresh visible-session original calls match210,078 instructions and
60,084,310 accessible bytes per replay, plus compared GP/SSE/x87/FP state under
the documented host interrupt-flag boundary. They are sampled whole calls, not
complete branch coverage or Xbox hardware trajectory proof.

The Warthog video was inspected seated at8seconds and after exit at25seconds;
its `visual-review.json` records the actual vehicle and camera behavior. The
production `Halo2-Physics-TU15` launcher now selects tested v517, SHA256
`85f9767e2a0f35116df02f83f3d6a236ecb0f55350a6cc091f729d87ebc39142`.
Other launchers and unrelated root changes are outside this owned delivery.

## Required remaining gates

The full goal remains active. This three-player native stress roster and declared
NPC input are not proof of retail cooperative admission or autonomous campaign
AI/scripts. The other four campaign-only class3 definitions, all47 authored
placements, intro Pelican cinematic context, successful cloth simulation,
remaining fourth Ivory Tower paddle and Terminal train rider contexts, fixed
human turret target damage, broader collision/controller/contact/lifetime and
corpse/ragdoll branches, quantitative Xbox TU1.5 trajectories and framebuffer
comparisons remain required. Existing Banshee host replay kernel-wait evidence
also stays unresolved. See the full physics goal and each subsystem's retained
matrix; this checkpoint does not shrink the objective.
