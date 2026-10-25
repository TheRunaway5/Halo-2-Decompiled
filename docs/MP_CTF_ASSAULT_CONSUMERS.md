# CTF/Assault native presentation and session return

Xbox TU1.5 remains the gameplay authority, pinned to executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The nine authored presets are Multi Flag CTF, CTF Classic, 1 Flag CTF,
1 Flag CTF Fast, Multi Bomb, Single Bomb, Single Bomb Fast, Neutral Bomb,
and Blast Resort. Their original 304-byte rules and existing native objective
owners remain intact. The earlier [native rule conversion](MP_CTF_ASSAULT_NATIVE_CONVERSION.md)
retains the complete-match native/literal and 54 edited-rule comparisons.

## Native consumers

`source_announcer_events_tu15.cpp` implements the original recipient selectors,
weighted/alternate response selection, delay conversion, recent-event suppression,
and random progression. The unchanged original executable supplies 483 reference
cases. The native event adapter observes real dispatch-time player membership,
flags, event data, original tick and pre-dispatch random state. It retains no
borrowed world memory and does not write a second gameplay RNG. Original
local-player audio gating and its authoritative side effects still execute.

Prepared MCC `matg`/`mulg` definitions and sounds feed the existing native PCM,
announcer queue and completion owners. The ordinary runtime drains unseen events
at source tick boundaries, including its terminal headless tick. A newly admitted
peer skips historical broadcasts while retaining its own targeted admission.
Caption keys are preserved; this work does not replace the existing source
HUD/text-message owner. A 64-event owned journal is included in authenticated
native snapshots. Protocol 102 introduced the typed journal and per-player
waypoint/goal packets. Protocol 106 also retains exact completed source coverage
bounds alongside the compressed visual WORLD matrices; its strict bounds and
validation remain active. The delta stream uses kind 14, outside the ONLINE
control namespace. Older native peers reject the changed snapshot schema.
This is the native protocol, not original Xbox wire compatibility.

The registered objective presentation callbacks run against a read-only world
with private caller/view storage. Their real descriptor, goal and UI submission
boundaries collect owned values. The existing compiled C++ projection, icon,
background and arming-meter arithmetic feeds native resident HUD materials.
Actual source collision `16CEA0` remains responsible for a requested occlusion
query. All eight canonical waypoint styles use identical visible/occluded
sequences across the 28 pinned original maps. Original `25C484` therefore skips
that query for these styles. A remote view uses an authenticated metadata and
arithmetic resource bank without allocating a local gameplay/physics authority
or supplying a visibility result. A genuinely requested query still requires
its actual collision owner and rejects when absent.
Emblems use the retained genuine player appearance, palette and original
resource choices. Private bitmap views project authenticated original logical
width/height/registration into the compiled consumer's source layout; they do
not overwrite the canonical MCC tag bank. All nine required bitmap layouts
agree across 28 original maps, and actual MCC artwork mappings are validated.

Friendly-player emblem clearance uses source-authenticated font metadata at the
native font resource boundary. The private caller binds native logical font selectors; it does not construct
a legacy disk cache or publish legacy ready bits. Original selector 6 supplies a single-view line height and
selector 5 supplies a split-screen line height. Each actual local view rebinds
that clearance before projection, including packets received from another peer.

The indirect goal callback `24D760`, outside the earlier 57 direct mode entries,
is a native circle/wall kernel: original 32-point circle, x87 sine/cosine spills,
SSE rotation, exact UV/fade/color and 0.4 wall height. Its existing shared King
wall owner retains the original 0.8 height. Ninety-six unchanged original CPU
cases compare all 6,336 emitted vertices. `24D900` is the renderer's restoration
wrapper around the same callback; native command/material bindings own that
restoration. The existing source transparent queue submits the actual MCC goal
shader and original designator/context. No game executable payload is copied
into runtime storage, and no runtime XBE instruction decoder is added.

A rendered flag also reached original cloth shader getter `1191A0`. Its typed
native owner follows the real cloth definition and canonical tag directory.
Thirty-two unchanged original cases qualify return registers, defined flags,
argument cleanup and read-only behavior. Its generator retains the native entry
and the literal diagnostic control without regenerating unrelated cloth code.

## Local results and lobby lifecycle

The native local session admits original engines 1 and 9 with the actual source
selected-team mask. Player birth/activation/spawn retain their original Xbox
owners. Real map/session readiness and signed-in profile/team input flow through
the existing menu countdown and local session services. The native PCR projects
valid original objective rules, complete retained statistics and genuine player,
team, rank and identity data into its existing resource/factory owner.

The complete-flow test uses two real signed-in local players and selected teams,
four final-stage beeps during the real ten-second countdown, natural objective
match completion, both source local views bound, native audio and waypoint CPU
consumers, original retained results, world destruction, real PCR Done, a fresh
lobby, and the next Start/cancel. Approach and goal positions remain explicit
source position inputs; objective/timer/score/winner results are not injected.
This closes the local split-screen return lifecycle for these nine presets.
`NativeVistaOfflineFlow` continues to require its genuine local session; LAN and
ONLINE session-return/migration owners are outside this local lifecycle proof.

## Remote resources and ONLINE return

`objective_waypoint_memory_tu15.cpp` binds canonical `matg`, `hudg` and `bitm`
metadata, full-salt cache rows and individually authenticated numeric operands
for the compiled waypoint consumer. It does not construct world startup or
sound SDK banks. The source callbacks, original waypoint descriptors, camera
projection, bitmap geometry and native renderer remain the ordinary consumers.
The exact source object spheres travel losslessly in the snapshot body; the
existing dynamic-object/WORLD consistency guard remains intact. Rounded visual
pose bounds cannot replace the object's completed source coverage sphere.

The ONLINE frontend now uses the same source rule selector as the objective
lobby. The original objective option buffer passes unchanged through the PCR
bridge. Native network PCR construction admits engines 1 and 9 with their
actual team flag, roster, statistics and placements. The service accepts the
matching original preset family and preserves unanimous confirmation, identity,
team and retained-row checks. Legacy relay routing admits host delta packets
while continuing to reject client-authored snapshots. These are native room
presentation identities, not manufactured Xbox Live XUIDs.

The two-process ONLINE regression drives real menu/profile selection, native
queue admission, encrypted peer gameplay, natural original match completion,
remote native audio/PCM, unanimous stored results, world destruction, PCR Done
and fresh admission. Its declared position/controller inputs are explicit test
inputs; remote press/release samples pass through the actual client and host
ingress. No objective, score, timer, winner, completion or readiness result is
inserted. The source matrix and bounded receipts are under
`build/ctf-assault-network-consumers-20261024`; `acceptance.json` records the
qualified cohorts and cleanup rather than treating an earlier failed probe as
acceptance.

The final `final-cohort-20` builds the full native application and passes all
eight selected CTests. All nine remote cases render 120 actual Vulkan frames
each, totaling 1,080 frames and 14,838 waypoint vertices. All nine ONLINE cases
complete the encrypted two-process flow, produce nonzero native remote PCM,
store unanimously confirmed results, release the world and return through PCR
Done to fresh admission. Their remote objective bank produces 185 events,
115 selected cues and 46 actual queue requests. The same cohort repeats all nine
complete local audio/PCR/lobby flows. Sixteen service transport/result tests
also pass, including family, identity, team and client-snapshot forgery rejection.

System Link session/PCR return remains a separate missing owner. The application
still lacks a retained native LAN lobby lifetime across world teardown. The
offline session guard is preserved; an admitted remote player is not recast as
an offline PC session member. Network migration, physical played routes,
controller/speaker/Xbox framebuffer comparisons and launcher installation keep
their own acceptance gates.

## Evidence and limits

Evidence is under `build/ctf-assault-consumers-20261024`. The final
`acceptance.json` names exact frozen-source hashes, report hashes, declared
cohorts, concurrent workspace differences and cleanup receipts. Earlier failed
probes remain bounded diagnostics, not acceptance. The launcher-selected runtime
and unrelated active work are preserved.

- `kernels-session-final`: five ASan/UBSan suites, including the 483 audio cases,
  original objective/PCR projection, 96 goal/shared-wall cases and native session
  services.
- `cloth-sanitizers-final`: all 32 original cloth getter cases under ASan/UBSan.
- `final-cohort-23`: complete native application build, all nine headless menu
  starts, all nine two-player rendered starts, complete local audio/PCR return
  flows, snapshot checks, PCR queries, native memory ownership and goal GPU proof.
  Later strengthened local-view and font-path qualification is listed in the
  final receipt separately.
- `final-cohort-29`: final complete native application build and 14 CTests:
  13 pass and the optional legacy announcer fixture skips. All nine complete
  local flows bind the original local-view/audio gate. All nine actual two-player
  rendered starts pass. Authenticated complete loopback matches agree across
  13,362 replicated frames, 22 remote goals, late admission and departure. Actual
  original font headers independently match the new native metric boundary.
- `friendly-render-30`: all nine presets also pass three-player native Vulkan
  rendering, with actual third-profile admission and nonempty original friendly
  waypoint packets. Production files are unchanged from the final cohort;
  only this rendered regression's third-player coverage is added.
- `bound-views-25`: complete local Multi Flag flow with the actual original
  local-view/audio gate active.
- `network-font-27`: complete authenticated remote Multi Flag match after the
  friendly-waypoint font dependency correction, including snapshot equality,
  late admission and departure.

Rendered startup uses a real Vulkan device under X11/Xvfb, two local players
and three submitted frames per preset. Actual startup cameras correctly cull
distant enemy goal rings; the separate visible-goal GPU test compares real
shader visibility, zero alpha, opacity and animation controls. Temporary frame
captures are removed. The Khronos validation layer is unavailable on this host;
these GPU runs explicitly disable it and are not clean-validation-layer proof.
The optional preexisting Vista announcer fixture is absent and its legacy test
skips. The older optional 22-case King boundary fixture is also absent; the new
48 original King-height cases verify the shared wall code for their declared
inputs.

The older authenticated loopback cohort proves event/waypoint packet delivery;
the later remote-consumer cohort separately exercises the actual native remote
renderer with declared native mouse input. A metadata-only display replica
does not gain physics authority. Physical controller routes, speaker waveforms,
original Xbox framebuffer comparisons, all option combinations, cross-machine
play, migration and launcher installation are not inferred from these tests.

## Storage

New retained fixtures are required original CPU reference data, not executable
payloads: `objective_audio_original_tu15.json`,
`objective_goal_geometry_original_tu15.json`,
`objective_pc_variant_original.json`, and `cloth_shader_original_tu15.json`.
The four required reference fixtures total 1,254,312 bytes. Small
source/command/hash and failure reports are retained. The enclosing
`tools/run_test.py` owner removes source snapshots, object/archive files,
compiled executables, shader products and raw frame/capture scratch after the
investigation. `cleanup.json` verifies the owned paths were removed. No new
large runtime/build/capture artifact is retained.
