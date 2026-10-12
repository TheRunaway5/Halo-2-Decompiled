# Death cycle implementation and remaining source owners

The active native match now opens the original full scoreboard automatically
at the source respawn-countdown gate, then fades it away after the new unit is
allocated. This is a partial implementation of the requested full death cycle.
Death animations, physical corpse rendering and the death camera are unfinished.

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
source player-slot respawn scheduling are retained. Native UDP already carries
the current unit, player flags, countdown and life count needed by the display
gate; no protocol change is necessary.

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
The 96 original lifecycle cases include all four local views, four FP rounding
modes, participant replacement and death/hold/respawn selection. The separate
68 attached-camera cases execute the original startup initializers and
23FF50/11E7B0/173FA0. They cover zero-distance and orbital degeneracy,
node/type/attachment gates, counters and coordinate limits. The existing
400-frame packet/motion/world/obstruction comparison also passes.

These comparisons establish the compiled owners on their declared component
inputs. They do not supply missing live corpse nodes or prove rendered native
camera transitions. Non-multiplayer23FCA0 and actual world location, visibility,
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

## Remaining live integration

The new `MultiplayerObjectRuntime` now constructs an empty original physical
world in a fresh retained bank from the authenticated TU1.1 executable and
actual Lockout cache. Complete object/header/body-heap and physics storage
constructors, actual BSP byte delivery, original BSP publication, native process
heap backing, CRT registration and the complete new-map world constructor run
without a captured gameplay bank. A native probe verifies that the resulting
object pool is genuinely empty and that the world, manager and Havok owners
exist. This proves empty-world construction, not a playable match or corpse.

The previous `oracle_mp_death_world_startup_1_1.py` map-reset claim is withdrawn:
that experiment mapped address zero, so an absent scenario pointer could read
zero bytes rather than fail. The frozen experiment is retained as historical
evidence. Its replacement, `oracle_mp_death_cache_startup_1_1.py`, leaves NULL
unmapped, delivers the actual cache and compares nine original roots and all
1,073,676 stores. The original storage-only prefix remains valid.

The new native object-world path also owns real animation resource byte I/O.
The original animation sampling and codec owners consume the cache's authored
descriptor and bytes; no pose or resource-completion state is supplied. Actual
biped creation now executes its authored effect attachments and reaches the
post-construction type registration graph. Missing reached owners still stop
explicitly before this birth can be called complete.
Game-options and AI/PVS storage use explicitly bounded original allocation
prefixes, and native session inputs supply the actual variant, clock rate and
seed. Neither complete Xbox process boot nor title/lobby startup is claimed.

The active `MultiplayerMatch` still owns typed native `Unit` records and object
headers, rather than the complete retained source biped, animation manager,
component/body/constraint banks and physics world. Its existing fatal-hit,
corpse datum/lifetime, source timers, respawn and scoreboard path is live;
the newly compiled physical/camera owners are not yet scheduled there.

The development `--mp-source-object-world DIRECTORY` path now constructs and
retains `MultiplayerObjectRuntime` before Match/Simulation placements. Simulation
receives a view of its actual source header pool. Player spawn calls complete
`B8820/B8A30`, observes the resulting source identity without a second allocation,
and the body renderer skins that object's current WORLD matrices. This path
requires the authenticated TU1.1 executable and an owned savegame directory.
Its Match-level probe is currently being brought through the remaining birth
callbacks; it has not yet passed a rendered validation. Ordinary launchers still
use the existing native path. Native category-only scenery/weapon storage,
source input/tick delivery, fatal transitions, remote pose delivery and the
death-camera view remain required before this development path can become the
default complete cycle.

The remaining runtime work is to construct and retain those banks from their
actual allocation/constructor owners, connect animation/request and physical
input/output producers, bind actual collision/solver services, and schedule
prephysics/world/current-node phases in the original order. The same current
WORLD nodes must then feed corpse skinning and camera target lookup, with
retained corpse state delivered to remote clients. The render loop currently
excludes dead players and selects the normal player camera. It must receive
the completed physical pose and observer output before those branches change.
No idle corpse, root-position camera proxy, chosen contact or successful
missing-owner default is installed.

Validation so far consists of original-executable component comparisons,
native tests and the existing live UDP/GPU scoreboard probe. A complete live
fatal-hit -> animated/physical corpse -> obstructed death camera -> respawn ->
HUD restoration run remains unverified and the requested full cycle remains
unfinished.
