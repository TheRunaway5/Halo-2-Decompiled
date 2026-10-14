# Death cycle implementation and remaining source owners

Normal TU1.5 multiplayer sessions now provide source death reactions, authored
animation, physical ragdolls, independently retained corpses, the obstructed
death camera, automatic scoreboard and restoration on respawn. Local and UDP
rendered acceptance passes. The dated sections below retain the implementation
history; the default-launch section records the completed multiplayer delivery.
Campaign co-op gameplay and its death-to-teammate camera remain unavailable.

## Implemented behavior

`MultiplayerScoreboard::step(view, held, state, slot)` now uses the original
`15D160 / 15E540` scoreboard tail during active play as well as after game over.
The automatic branch requires current unit NONE, signed player countdown at
most one, flag4000 clear, lives not exhausted, tick strictly greater than three
seconds, and player flag1. The existing original counter, cap, fade, roster,
font and normal-HUD suppression consumers remain in use. Current ownership
restoration closes the automatic branch and decrements its counter normally.
Manual display remains available during the countdown. Each local view receives
its own player record. The native session has no watched-player owner at
player+1B0; the special `1642F0 / 1656D0` exclusion therefore takes its original
NONE branch. No watched-player record or ten-second spectator state is invented.

Existing authoritative fatal cleanup, retained salted corpse identity,
previous-player identity, death tick, actual fatal momentum, signed timers and
source player-slot respawn scheduling are retained. Native UDP carries
the current unit, player flags, countdown and life count needed by the display
gate. Protocol52 additionally carries independent detached-corpse metadata and
source WORLD poses, including older bodies after respawn or player-slot reuse.

`tools/oracle_mp_death_scoreboard.py` executes 240 unchanged TU1.5 tail calls
with explicit player/clock/input/lifecycle API records. Cases cover live
ownership, countdown2/1/0/negative, tick90/91, player flags, finite-life exhaustion,
manual display during countdown, cap saturation and restoration of ownership.
Its direct original outputs are in
`tests/fixtures/death_scoreboard_original.tsv`. No death or respawn producer is
claimed by that isolated arithmetic experiment.

`multiplayer_death_cycle_tests` also runs actual stock-SMG fatal hits through
the native Match, automatic per-view roster/font submission, distinctly salted
respawns and HUD restoration, both locally and through actual UDP snapshots.
It uses a declared native debug arena to exclude unrelated falling deaths.
It does not supply a dead flag, health depletion, score or countdown.

`multiplayer_death_scoreboard_runtime_tests` verifies the rendered path on the
RTX 5080: an actual UDP-hosted SMG death releases the client's current unit,
the automatic source roster/font/cells reach GPU UI submission without F3,
the new salted unit arrives, and the scoreboard fades away as the HUD returns.
The probe passed with the selected TU1.5 executable. The eight focused CTest
checks for the new cycle, fatal/kill-volume momentum, lives, round/game endings,
respawn timing and original full scoreboard also passed. This does not validate
physical Xbox hardware, native two-computer LAN or the unfinished camera/corpses.

## Death-camera evidence

`tools/research_mp_death_camera_functions.py` verifies the complete normalized
instruction and internal branch correspondence for nine TU1.1/TU1.5 bodies:

| Owner | TU1.1 | TU1.5 |
| --- | --- | --- |
| Normal/death camera selection | 1573B0 | 158790 |
| Camera-mode transition | 157520 | 158900 |
| Death-camera construction | 23FB00 | 246CF0 |
| Death-camera packet | 23FF50 | 247140 |
| Watched-target replacement | 23FEA0 | 247090 |
| Watched-player selection | 23FD40 | 246F30 |
| Current observer getter | 170E20 | 172200 |
| Observer world/render output | 172A50 | 173E30 |
| Camera obstruction query | 1732B0 | 174690 |

The constructor copies the current observer position, consumes three values
from the local RNG, and retains the current unit or previous unit before
considering another participant. The packet reads the actual retained object's
current node-zero translation. It does not follow a guessed eye height or
choose a direction from the killer's position.

The original `tools/oracle_mp_death_camera.py` freeze stopped at17325A
because its selected startup lacked the cluster bank. Subsequent native work
now implements the observer registration/adoption, six-channel motion,
packet validation/repair, world/render delivery, saved death-view clusters,
obstruction distance and salted WORLD-node target lookup. The earlier stop
remains historical evidence; it is not the current native owner inventory.

`death_camera_lifecycle.cpp` composes complete156F60,157520,157070,1573B0,
23FB00 and multiplayer23FEA0 over retained camera/player/object storage.
The 97 original lifecycle cases include all four local views, four FP rounding
modes, participant replacement and death/hold/respawn selection. The separate
68 attached-camera cases execute the original startup initializers and
23FF50/11E7B0/173FA0. They cover zero-distance and orbital degeneracy,
node/type/attachment gates, counters and coordinate limits. The existing
400-frame packet/motion/world/obstruction comparison also passes.

These comparisons establish the compiled owners on their declared component
inputs. They do not supply missing live corpse nodes or prove rendered native
camera transitions. Actual world location, visibility,
fog and live geometry/pose/thread banks remain runtime providers. Complete
16B240 dispatch is now compiled and available to the camera, as detailed below.
A host terrain raycast is not silently treated as the complete original query.

`source_segment_collision.cpp` and its generated bodies own complete16B240,
terrain and breakable traversal, cluster/fog handling, instance arbitration,
posed-object and attachment dispatch, Havok tree traversal and the real instance
collector. Forty complete original bodies compile to native C++; reached
unbound virtual/platform owners still stop explicitly. The unchanged original
death world produces684 queries over75 authored instance centers and the corpse
position using the actual placement and two camera masks. Independent original
re-execution and native execution both match all469953 stores, including private
frames and explicitly retained thread exception-chain memory. There are no
replayed geometry/hit results or CPU estimate boundaries in these cases.
Measured results include471 terrain hits,120 instance hits and93 misses. The
posed-object branches are compiled but were not reached by this query matrix.

Defined-input segment and camera collision adapters additionally match38253
non-stack stores across all684 cases. The camera adapter returns the original
fraction bits and uses the original NONE exclusion handles. Ragdoll placement
can bind the same complete query at its actual E3270-derived call frame. Its
optional runtime configuration requires the current world and thread banks;
it does not construct them. This still does not establish a live Match corpse
or rendered camera, an Xbox exception ABI, or coverage of every virtual shape.

Complete16A9C0 point containment and its object-hierarchy/model owners are now
compiled separately and bound to the same ragdoll bank. Six original bodies
consume the actual current node transforms and collision model. Sixteen
original queries use the naturally posed corpse's bound center and root
position, four caller masks and NONE/corpse exclusions. The bound-center
queries reach posed-object containment; excluding the corpse changes the
actual result. All3012 original stores match independent original execution
and native execution. Defined-input containment additionally matches the
original results with no retained non-stack writes. Complete prephysics also
passes all13 captured phases with this native point owner connected. No pose,
bounds, contact or containment result is assigned by the runtime adapter.
The current focused native death suite passes58/58 tests, including these
query adapters and the complete prephysics containment binding.

## Following another player until respawn

The active multiplayer death camera initially retains the dead player's corpse.
Jump/A requests the original player cycle after its one-second input gate.
`23FD40 / 246F30` selects another occupied living player; team games restrict
selection to the dead player's team. The same source packet, observer motion
and world obstruction then follow the selected unit's actual current WORLD
node0. A newly owned unit clears the death camera on respawn.

The original watch timeout also requests replacement while the source clock
is unpaused. The native binding now reads the actual initialized/paused bytes
from that source clock instead of passing fixed match-state booleans. The
multiplayer timeout is the authored executable value (15 seconds); ordinary
short respawn waits finish before it. Loss of the target also enters replacement.
No living candidate preserves an otherwise valid corpse. This is separate
from the player+1B0 spectator/scoreboard owner discussed above.

An independent unchanged-executable run of the bounded `24040A..24042D`
clock gate covers all four initialized/paused combinations, calling complete
`23FEA0/23FD40` against an occupied two-player bank. An initialized, paused
clock preserves the corpse; an initialized, unpaused clock selects the live
player at timeout. Its diagnostic timer-zero/cycle-false inputs and report are
retained in `analysis/multiplayer/death-cycle-20261013/original-watch-clock-gate.json`.

The constructor's no-current-unit/no-previous-unit fallback now receives the
same team restriction as later cycling. The extra unchanged-executable
`construct_participant_team_only` experiment confirms this path; the previous
96 lifecycle cases regenerate byte-identically. The typed constructor test
also distinguishes the eligible teammate from later living enemies.

`native_death_spectate_local_tu15` and `native_death_spectate_remote_tu15`
exercise an authored-spawn SMG death, a short cycle press, actual movement of
the surviving player, completed camera alignment with that live WORLD target,
and restoration on respawn. The remote case uses the real UDP Jump press
serial and authoritative observer snapshots. These are headless checks;
existing local/remote GPU checks cover completed observer submission separately.
The source-camera component additionally expires the actual multiplayer watch
timer and checks automatic replacement on its initialized, unpaused clock.
Its elapsed-time input is diagnostic. Separate `native_death_spectate_auto_local_tu15`
and `native_death_spectate_auto_remote_tu15` checks use a real admitted20-second
respawn setting and continuous source-world ticks. They check automatic live-player
following after the original15-second watch timeout and three-second transition,
while retaining the corpse, then verify restoration on actual respawn.

Campaign co-op is not implemented in the native gameplay runtime. Campaign
owns one `Simulation::player_`; `game_is_cooperative` explicitly reports its
actual solo mode. The multiplayer camera binding is not called by campaign.
The original non-multiplayer target restriction (`23FCA0`), fallback target
(`23FE30`), campaign watch timing and cooperative respawn scheduler are now
compiled and component-tested. They still require a real cooperative session,
player bank, birth/placement, checkpoint and transport owners before claiming
that flow. See [campaign co-op death implementation](CAMPAIGN_COOP_DEATH_IMPLEMENTATION.md).

## Animation and physical corpse owners

Ordinary `CB410 -> CBDC0 -> E6050` passes a zero forced-animation flag.
`landing_dead` remains conditional on the original animation/physical branch;
its authored42-frame clip is not selected merely because a native unit is dead.
The physical transition through `E6DC0 -> 1E8E60 -> 1C70D0` is compiled,
including source ground/dead packet constructors and the cold biped fields.

The source constructor, body grouping, nine ragdoll/hinge constraints, body
placement/velocity, controller construction, world insertion/removal and
contact retirement now have compiled implementations. The original natural
experiment advances its actual clock and admits the ten-body/nine-constraint
Spartan at tick8, without supplying a mode, contact, pose or admission result.
Native admission/constraint updates and contact-impulse distribution are
compared with that execution. The contact suite additionally covers55 cases,
500 actual per-body velocity requests and four FP rounding modes.

`biped_prephysics.cpp` now owns the fullDE960 caller, including parent routing,
heading normalization, source counters, physical/action packets, naturalE2520
scheduling, recovery, dead action selection, action-release timing and the
sixty-slot action dispatch. Both isolated and native-admission compositions
match13 original post-death phases. The finite-input1F0940 orientation owner
matches192 original cases and1728 retained stores in all four FP rounding
modes. It delivers forward/up vectors; it is not a linear/angular velocity
substitute. Nonfinite CRT angle-error handling remains an explicit dependency.

`biped_movement.cpp` owns completeE3B20 movement scheduling and its source
activation, orientation-fraction and action helpers. `biped_physical_step.cpp`
owns complete1E8F60 physical output, all six1E94E0 state dispatch branches and
complete1F0080 dead stepping. Original comparisons cover768 dead-step cases,
1248 physical-output cases and1324 helper cases in four FP rounding modes.
The completeDDBC0 physical-input/root-motion producer now composes natively;
13 original calls match2332 stores and13 actual cache-byte boundaries.
E1800 action input,E4F40 recovery,DF7C0 object update, movement basis and
root-motion owners match68 original calls and600 stores. BC910 marking uses
the existing complete vehicle/object-update owner. Contact/world-query
producers remain required when reached.

CompleteDEFC0 biped postphysics now runs its original packet constructors,
contact refresh, support updates, position correction and animation follow-up.
Its six contact owners include1E93F0/1F0E90/1D5600/3121A0. Thirteen unchanged
original contact refreshes match483 stores; the enclosing thirteen postphysics
phases match2816 stores, including private call-frame memory. Defined-input
adapters also pass, retaining319 non-stack stores for the full phase. These
comparisons supply no contact, ground correction, pose or lower gameplay result.
The46 focused death-runtime CTest checks pass after this addition. This remains
component evidence until the active Match owns and schedules the same banks.

`unit_animation_runtime.cpp` and `unit_animation_channels.cpp` own the full
112E10 phase,110BF0 requests,1D0A60 selection/installation, third-channel
mapping, packed transition clocks, retained weights, channel/event clocks and
resource demand. Both captured death requests now run without replayed lower
animation dependencies. The four reached gameplay event calls atC04F0/E8050
now execute through native original bodies, includingE6670/E63C0 dispatch;
their complete isolated comparisons match34 original stores. Separate original matrices cover20736 transition,
3072 weight and2304 channel cases, including all four FP rounding modes.

`unit_animation_selection_runtime.cpp` compiles the authenticated original
selection, variant, overlay, local-pose and packed codec paths to native C++.
There is no production instruction decoder or fixture replay. The original
codec table entries1..8 and node kernels are compiled. Current comparisons
cover the actual codecs reached by the captured death run; this does not
claim measured coverage of every codec. Twenty-two selection calls match
17566 source stores without captured dependencies. Fifty-six complete local
pose/callback calls match22064 stores,2136 independent original CPU estimate
pairs and56 explicitly retained real-cache-byte delivery boundaries. Defined
pose adapters additionally match9426 retained stores.

The combined native prephysics/input/action/animation/velocity/ragdoll-world
path matches9731 gameplay writes and20483 final gameplay bytes from13 unchanged
original post-death phases, with no replayed gameplay calls. E2520's BAA80
activation and1D4A00 velocity deltas use their native owners. E2CE0/E30B0 now
compose native component removal/disposal/reconstruction, placement and world
insertion over the same bank. Recursive placement frames and point-query
temporaries derive from the actual caller ESP. Original RSQRTSS instructions
retain46 explicitly measured CPU estimate pairs. The unchanged frozen trace
also contains466 x86 FS exception-chain writes; the exporter separately names
and excludes those platform stores because native C++ uses the host exception
ABI. The earlier velocity-only composition retains10197 writes and20487 bytes
with two explicit world boundaries. Neither isolated composition constructs the live Match's banks. The later
five-phase comparison below establishes continuous native scheduling over its
explicit initial bank. Segment collision and object containment now bind their
complete native queries; unknown shape owners and live bank construction remain
mandatory on reached branches.

`object_current_nodes.cpp` owns completeBDF80 physical-first orchestration,
E2CA0/1F0030 height correction and local defaults/masks. Its1D7310/1D7710
physical hierarchy, body matrices, matrix-to-quaternion conversion and local
pose callbacks now execute as complete native original bodies. Physical node
masks protect admitted nodes from animation/root overwrites. Strict comparison
of13 original controller/ragdoll phases matches every14618 retained store,
including its original instruction/address/before/written bytes, and all34225
final output bytes. CPU estimate instructions and actual animation resource
byte delivery remain explicit services. Host estimates do not establish Xbox
hardware equality. These components still need the live retained banks.

`SourceRuntimeMemory` provides owned32-bit regions, permissions, alias-safe
reads/writes and stable span adapters. The authenticated TU1.1 image loader
imports33 real sections without constructing pools. Native raw datum startup
matches300 original cases and12111 stores. Complete16CFF0 pool creation and
125E50/1656A0 game-arena allocation/checksum ownership additionally match108
original cases and42606 retained stores. These preserve raw payload and use
authenticated read-only pool names and allocator method tables. Complete
1D2620 component-pool construction additionally matches three original stages
and1287 retained stores through11E150/33785C with actual Linux mmap backing.
The host adapter owns opaque32-bit address allocation and actual page rounding;
it does not supply pool fields, validity or salts. Source map reset and first
allocation produce those fields. Kernel failure/error branches are explicit
services and are outside that successful-allocation experiment. This backing
and pool construction are available to allocation owners; they are not yet
the active Match's world.

The physics world step, iterative322ED0 solver and verified31BBE0/31BD00/
322380 Jacobian producers are also compiled. Unknown virtual/schema owners
and their live backing remain required when reached. The presence of these
modules does not establish a connected live solver world.

Complete1C7660 world scheduling now runs its original floating-point controls,
world step, deferred body collection, dirty-object propagation and heap pressure
owners. Sixty-one additional complete physics/contact/solver bodies include
five real method/tail targets absent from the function index. The unchanged
original13-tick death timeline naturally admits ten bodies and nine constraints
at tick8. Independent original re-execution matches all161181 stores. Raw ABI
and defined-input native world execution each match all52267 retained non-stack
stores, with181 original CPU estimate boundaries, no cache-byte boundaries and
no replayed lower gameplay calls. Actual original call and virtual-tail target
coverage is audited separately against the complete compiled entry identities.

Private world FP scratch is outside the native comparison: Unicorn omits a
host SSE precision sticky bit in STMXCSR. Production still executes the actual
STMXCSR/LDMXCSR and original control-word owners; no FP result is normalized to
make the fixture pass. The complete reached world component also participates in the continuous
five-phase comparison below. Live Match construction and rendering remain
separate work.

Complete13A380 -> B7E00 ->1C7660 -> B7F50 -> B8040 now runs through a single
production native API, sharing one retained bank across13 cycles and65 phases.
The original139863..13986B clock fragment advances time between cycles.
Two real register callbacks missing from the function index are authenticated
against their actual original method loads and compiled in full. The new
object-phase family contains237 complete bodies and26002 source instructions.
All42861 actually executed original instructions are covered by complete native
bodies; this static coverage is audited separately from behavior.

Independent original reexecution checks all338646 stores. Independent native
phases, continuous native phases, defined-input continuous phases and the
production five-phase API each match89592 retained non-stack stores,1256 actual
CPU estimate boundaries and27 real cache-byte delivery boundaries. The persistent
44418-byte experiment bank is mounted once and is never reset between phases
or cycles. Fresh register inputs suffice for the production API; no captured
GP/SIMD state or lower gameplay result is supplied. Natural admission at tick8
still produces ten bodies and nine constraints. This establishes continuous
native object/world/pose orchestration over the explicit captured initial bank.
It does not construct that bank from startup or establish live corpse/camera
rendering. Details and source pins are in
`analysis/multiplayer/integration/death-native-20261011/object-phases-validation.json`.

## Current live integration (2026-10-13)

`MultiplayerObjectRuntime` constructs a fresh retained source bank from the
owned executable and cache. The TU1.5 path constructs actual players, bipeds,
equipment, animation resources and the physical world; object/world phases
produce the current WORLD matrices. Native command delivery and the source
player scheduler are being developed concurrently with the physics work.
Missing reached callbacks still fail explicitly.

`death_cycle_runtime_tu15_generated.cpp` lowers complete authenticated TU1.5
owners reached by fatal cleanup, animation cancellation/selection, effects,
observer collision and continuous dead-object updates. Its generator validates
the executable hash, instruction bytes, complete control flow and jump tables.
The SDK resource-busy predicate is recovered from the actual D3D section. No
runtime instruction decoder, replayed pose or successful missing-call fallback
is installed. `oracle_death_cycle_helpers_tu15.py` independently executes96
unchanged original helper cases; native execution matches351 ordered stores,
retained bytes and the defined GP/SSE ABI.

A fresh-map corruption bug was also fixed in physical-world startup. Native map
publication previously omitted the physical reservation corresponding to actual
125354..1253A2. Texture I/O consequently reused80061000 and overwrote the resident
tag table. Startup now consumes the literal Xbox header+1C size, reserves the
original physical lower-bound range and protects those pages before publication.
This change belongs to the physical-world adapter; tag-system files are separate.

`MultiplayerMatch::finish_death` delivers the completed native damage owner's
normalized fractions and recent-damage state before original source cleanup.
It preserves the authored maxima and leaves transition flags to their original
owners. This prevents a physical source corpse from retaining its spawn vitality
when gameplay damage was computed by the native source-derived damage module.

Detached bodies are enumerated by full salted object identity, independently of
current player ownership. Their definition and player configuration are retained
at death. Repeated deaths, respawn, disconnect and player-slot reuse preserve
older bodies until their actual object lifetime ends. Authoritative WORLD poses
include every retained corpse. The pose wire supports the actual2048-object pool;
fragmented native snapshots use16-bit fragment counts and32-bit total lengths,
with an8MiB transport bound. Local and UDP repeated-death tests pass.

The renderer skins each retained corpse's completed source WORLD nodes and gives
it independent lighting, object colors, emblem and damage-function inputs. It
also publishes the corpse's current effect markers after respawn. A missing
hierarchy is an error. The draw code does not substitute an idle animation for a
physical corpse. The later local and remote GPU checks below now cover this path.

`NativeDeathCamera` consumes the same full player/corpse pools and actual node-zero
translation, then runs the original packet, observer, world and obstruction owners.
The fresh-bank camera probe passes60 frames, distinct replacement-body creation,
ordinary-camera restoration and preservation of the old corpse. This component
check does not run continuous physics or establish remote-client camera behavior.
Remote clients now receive the completed host observer view rather than constructing
a replica source observer bank; see the native transport policy below.

The explicit forwarded death-animation flag installs graphE9180793,
packed66FFFF and activity0C000043 in the Lockout Spartan's actual manager. That
is authored `landing_dead`, clip102 with42 frames. Ordinary cleanup forwards zero
and follows its original physical branch. The test supplies the explicit flag;
its natural gameplay producer remains unresolved and is not inferred from a
falling death or from a clip name.

The continuous fresh-bank probe warms up a real source player using delivered
commands, kills it through D8D50/CCF40 and advances actual source frames. It
requires the authored ten-body/nine-constraint admission, finite moving WORLD
nodes and90 completed post-death ticks. This probe now passes: the first completed post-death tick has one body,
the second has ten bodies and nine constraints, and all90 ticks complete with
finite moving WORLD matrices. The source player remains detached afterward,
retaining the actual previous corpse and countdown zero. Its complete diagnostic
output is in`analysis/multiplayer/death-cycle-20261013/isolated-ragdoll-progress.json`.
The isolated diagnostic pins dependency archives while the shared physics source
changes; it is not installed-runtime or GPU acceptance evidence. A separate
`native_death_match_tu15_tests` checks actual authored-spawn SMG firing,
Match timers, source physics, observer, scoreboard and replacement together.
With delivered forward input0.1 for the victim during firing, this complete
headless match check passes. The original source admits ten bodies/nine constraints
on the next observed frame and later returns the settled corpse to a one-body
sleeping controller. All five camera/scoreboard/respawn gates pass.

The stationary failure is now closed by delivering the original direct-hit
reaction before death cleanup. `Simulation::damage` retains the actual incoming
projectile direction/contact and the recent-damage bank before the new report.
The source bridge preserves the original order: fatal marking, complete
`CB1D0` response selection, completed recent-damage stores, the authenticated
`DBD64..DBFCC` physical block, then complete `CAC00` reaction delivery.
Attribution/history and ordinary `CCF40` cleanup remain subsequent owners.
The body is marked fatal once; cleanup does not repeat its fatal transition.

The physical block is explicitly bounded before attribution and `CB410`, rather
than described as a complete function or a successful lower-service return.
It uses the real biped mass, damage acceleration, source constants and complete
physical callees. The actual SMG acceleration0.125 and biped scale1.2 produce
an impulse below the source biped admission threshold; forcing that impulse
would not reproduce the original. The missing producer was the authored reaction.
Complete `CB1D0/CAC00` now installs the63-frame `s_kill` naturally for the
fixed stationary case. This changes the real animation/controller state, produces the contact
request, and admits ten bodies/nine constraints without assigning a wake flag,
velocity, contact or ragdoll result. Eight complete reached animation owners
were added from authenticated TU1.5 bodies.

The reaction bridge now also receives admitted melee and area damage. Complete
`D7C60` constructs each request and complete `DAA90` translates it into the
original report and calls `CB1D0`. Direct hits preserve the actual projectile's
reporting type and direct flag. Melee reads attacker bounds/aim and the selected
weapon's reporting type from the retained source bank, matching `D0830/D10F0`.
The authenticated `D9A30` radius-geometry block calls the real closest-point
owner and derives direction from current physical bodies before report delivery.
Its complete 418-instruction body was paired to TU1.1 `D8AE0`; no address delta
was assumed. Native melee casts and area candidate/falloff/occlusion admission
remain their existing host geometry adapters, rather than a claim of full
original `D0830` or radius broadphase parity.

Fatal cleanup retains the actual report's class, direction and force bit. Source
falling/kill-volume deaths produced inside a world phase are published into
native death accounting, camera and respawn ownership. An already detached
physical corpse is not cleaned a second time. Native retained-unit copies are
pruned when their full source datum actually retires. The actual walk-off case
keeps its ordinary airborne channel at death and later admits ten bodies; no
`landing_dead` flag is invented for falling. The separate forced-animation
component test still covers that explicitly requested branch.

## Remote observer delivery

Native protocol54 carries a completed observer position, forward/up basis and
horizontal FOV for each peer's own dead local players. The host runs the same
`NativeDeathCamera` packet/adoption/motion/world collision owners against its
retained source player/body/WORLD bank. Each peer retains four independent
view states, cached by authoritative tick. An explicit native presentation RNG
is separate from gameplay RNG. The client submits this completed view directly;
it does not invent a replica physics world or a clear collision result.

Camera views are omitted for other peers and living players. Snapshot validation
checks presence, local ownership, dead/current-unit binding, finite floats, FOV
and basis. Respawn clears the observer view with the same snapshot that publishes
the distinct replacement unit. This is a native host-authority presentation and
transport policy at simulation cadence, not the Xbox System Link protocol or
proof of original network/render interpolation under packet loss.

## Validation boundary

All13 selected death checks pass in the normal optimized build, including
stationary, moving and UDP source-Match
acceptance: actual authored-spawn SMG firing, natural authored reaction,
ten-body/nine-constraint ragdoll, original observer, automatic scoreboard,
replacement, HUD restoration and retained corpse. The helper comparison now
contains160 unchanged-executable cases, including64 bounded physical-report
cases, with490 ordered stores and exact retained bytes/GP/SSE state. The physical
block comparison stops before its real impulse callee; continuous Match checks
exercise the live owner composition separately.

`native_death_render_tu15_tests` passes on the RTX5080 with a stationary victim,
real authored starts and a deterministic input script. Read-only observations
confirm actual source corpse WORLD skin, local post-death camera, automatic
scoreboard GPU geometry, distinct respawn, HUD restoration and continued corpse
submission after respawn. Its screenshot is a native PPM in
`build/native-death-render-tu15.ppm`. The earlier ordinary-path UDP scoreboard
probe remains separate evidence. `native_death_remote_render_tu15_tests` also passes with a real UDP host and
client renderer. It checks the received observer view against the actual camera
submitted in the same render frame, plus replicated corpse skin, automatic
scoreboard, distinct respawn, HUD restoration and retained corpse submission.
The broader multiplayer rules/combat/UDP and source-pose checks also pass.
The fresh ordinary-path UDP scoreboard GPU check passes separately, and the
285-entry death module regenerates byte-identically. Logs and source/runtime
hashes are retained under`analysis/multiplayer/death-cycle-20261013/` in
`continued-validation.json`.

## Default launch and completed native acceptance

Normal TU1.5 local, LAN-host and dedicated sessions now create the retained
source world automatically. Each session owns a private temporary directory for
its two source scratch files, closed and removed after the world. An explicit
`--mp-source-object-world DIRECTORY` retains caller-selected storage. Clients
consume the host's actual WORLD nodes and completed observer without creating
a competing local physics world. Older executable paths, native custom Slayer
rulesets and explicit frozen/reference diagnostics retain their existing owners.

The local GPU acceptance uses ordinary launch options without the development
switch. Both local and actual UDP-client render checks pass for corpse skin,
observer submission, automatic scoreboard, distinct replacement, restored HUD
and corpse retention after respawn. Authored-start headless checks additionally
pass ordinary melee impact timing, an admitted actual grenade explosion, a real
input-driven walk/jump off a BSP ledge, and extended corpse lifetime. The grenade
naturally selects the authored37-frame `airborne_dead` before the real ragdoll;
the ordinary stationary direct hit naturally selects the63-frame `s_kill`.
The falling case reaches original environment death and later ten bodies/nine
constraints without supplying a collision, fatal result or physical transition.

Original automatic scoreboard admission starts when the signed respawn counter
is at most1. Its fade can remain partial during a short slot-stagger interval
before respawn; full HUD suppression has a separate opacity threshold. Tests
accept actual visible popup geometry and then verify its disappearance and HUD
restoration. They do not force a longer death interval or change original gates
to obtain a fully opaque board in every timing case.

The death module contains301 authenticated entries:298 complete bodies and
three explicit block entries, totaling25,426 unique instructions. The helper
differential now contains224 unchanged-executable cases and1914
ordered stores, with exact retained bytes, GP/SSE state and defined flags. Its64
additional radius-direction cases start after declared closest-point inputs;
they do not supply a successful query result to live gameplay. Live area checks
separately execute the real closest-point owner. Original source byte verification
and generator reproducibility remain required.

A normal menu-to-match integration run with the selected TU1.5 executable
completes two actual countdown/world/report/Done/lobby rounds and subsequent
title/session restart. This is CPU lifecycle evidence; the separate GPU checks
establish the rendered death cycle. Original Xbox framebuffer equivalence,
arbitrary damage/collision/vehicle admission, the separate special forced-death
packet producer and presentation under packet loss remain outside this proof.
The native multiplayer death cycle is now delivered through normal TU1.5
session options; these limits are not substituted with guessed behavior.

Final delivery validation: all18 selected death-cycle CTests pass, including
ordinary and kill-trigger momentum, respawn timing, local/UDP GPU cycles, six
source-Match modes, cleanup/animation/continuous-physics branches and original
helper comparison. Broader multiplayer rules/combat/UDP/map and source-pose
checks pass. Missing Ninja dependency records left by concurrent builds were
repaired for eight reachable objects; the final dependency audit is empty.
Both installed launchers now select `build/halo2_native`, confirmed by their
`--launcher-info` output, with existing owned assets and selected TU1.5 image
preserved. Previous launchers/shortcuts are backed up under the ignored death
validation directory. Exact source/runtime hashes and logs are recorded in
`analysis/multiplayer/death-cycle-20261013/final-validation.json`.

## Spectating delivery and concurrent-work boundary

The follow-up spectating delivery passes10 camera/spectating/render checks,
including local and real UDP manual and automatic switching, plus the current
Fiesta/Infection preservation check. An isolated negative control using the old
fixed clock flags fails the new automatic-switch regression at its intended
assertion. The unchanged original four-case clock gate also passes.

Concurrent builds were replacing shared archives and objects. Final validation
therefore uses private copied/recompiled objects and private archives, including
the matching newly compiled physics-support owner. Every reachable dependent of
the current multiplayer match header was refreshed to retain the custom Slayer
layout and lifecycle. The private delivery preserves the already selected custom
Slayer snapshot's grenade and map-effect attachment boundaries; newer unfinished
bridges remain in the shared source checkout. The camera changes themselves are
in the shared source. The isolation manifests identify these precise boundaries.

Both installed launchers select `.local/death-spectate-20261013/delivery/halo2_native`.
Its loaded ELF sections are byte-identical to the validated executable after
removing debug/symbol data. Fiesta/Infection definitions and the existing Discord
SDK are bundled. The previous custom Slayer delivery is retained. Launcher and
shortcut backups, test logs, source/runtime hashes and the selected-launcher
four-player smoke result are recorded under
`analysis/multiplayer/death-cycle-20261013/spectate-final-validation.json`.
Campaign co-op gameplay and spectating remain unavailable.
