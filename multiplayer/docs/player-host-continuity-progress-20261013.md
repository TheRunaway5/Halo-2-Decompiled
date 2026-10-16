# Player-hosted continuation implementation progress

2026-10-13. The user selected player-hosted migration, with a migration pause,
instead of dedicated authority. **Gameplay host migration is not complete or
enabled in the installed ONLINE client.** The current default direct peer
transport is `halo2-native-queue-v12`; its installed/deployed evidence is in
[direct-player-hosted-online.md](direct-player-hosted-online.md). The v11
continuity coordinator below remains a gated fixture path. Neither that
coordinator nor the new resource checkpoints enables gameplay migration.

## Implemented and exercised

`master_server/match_continuity.py` coordinates physical connection owners,
completed-world seals and acknowledgements, authority generations, candidate
failure/timeouts, survivor rebinds and repeated handoffs. A couch's guest or
secondary seat cannot become a physical host. An unacknowledged or mismatched
world cannot be selected. When the latest acknowledgement races host loss,
election waits for that completed world instead of silently selecting an older
tick. The observed relay RTT tie-break is explicit native policy; the original
Xbox host-quality fields have not been reinterpreted as ping.

The real WebSocket relay has an opt-in `halo2-native-queue-v11` protocol:

- A nonhost departure retains the room and original admitted report roster.
- Host loss enters awaiting-world/migrating/rebinding phases. Resume requires
  acceptance of the selected tick/digest and rebinds from surviving PCs.
- Datagrams carry an authority generation. Queued old-generation traffic is
  discarded, and lost/timed-out physical connection capabilities are revoked.
- Stalled or invalid readers lose their own connection. Healthy survivors are
  preserved when a qualified complete world remains available.
- A departing four-seat couch removes all its active seats together.
- Required result confirmations follow surviving physical connections. The
  additive `match_result_consensus` table records the explicit
  `native-survivors-v1` policy, admitted/confirming connection counts, departed
  slot mask and gameplay authority epoch. Original admitted participants remain
  in results; guest personal publication remains excluded by admission policy.
  This transport receipt is **not** a quit-penalty event or anti-cheat proof.

The native `OnlineMatchSession` implements those control phases, complete-world
seal/acknowledgement APIs, validated accept/rebind requests, membership checks,
generation-framed routes and stale-queue invalidation. Existing constructor and
legacy framing behavior remain covered. The new protocol is intentionally not
advertised by the frontend until the complete game continuation is integrated.
No UI assets or layout have been changed by this stage.

The following actual native subowners now support continuation:

1. `SourceRuntimeMemory::checkpoint/restore` retains bytes, region owners,
   permissions, reservation extents and commitment holes. Import validates the
   whole bank before replacing it; a rejected import preserves existing views.
   Successful imports invalidate borrowed views and require caller rebinding.
   Original body-heap operations after import produce identical pointer cells,
   allocation links and payload bytes in the test.
2. `SourceDeviceWorld::authority_checkpoint/restore_authority` retains shared
   groups, per-generation motion/velocity/control flags and pending effects,
   validates authored identities and completed tick, stages exact cached collision,
   and resumes the actual native motion owner. The test compares sixty future
   ticks, one-use controls and effects against the uninterrupted owner. A display
   replica cannot export itself through the authority-checkpoint API. The version2
   follow-up below retains dirty-cache provenance and adds prepared publication.
3. `SourceRuntimeHostVirtualMemory` stages real reservations and committed
   pages against an incoming whole-bank checkpoint, preserving the allocation
   cursor, released-token gaps and commitment holes. Publication replaces its
   resource registry without deleting imported logical regions.
4. `SourceRuntimeHostPhysicalMemory` stages actual physical-page backing and
   read-only page protections, retaining its write-guard identity. Invalid
   imports and cancellation leave current resources untouched. The enclosing
   transaction must quiesce users and publish the admitted bank before committing
   prepared resources, on the same barrier thread.
5. `SourcePhysicsPoolHostStorage` stages its live allocation registry and the
   raw allocator cursor. Pool regions have a distinct native resource label so
   missing/orphaned entries can be rejected without claiming another raw owner's
   arena scratch. Observers remain local. Occupied original TU1.5 pools continue
   in a different bank through 3,528 subsequent source calls with identical ABI,
   ordered writes, allocation/release requests, registries and final bytes.
6. `NativeSynchronization` stages actual unlocked recursive mutexes and bounded
   semaphores with their aliases, permit counts, maxima and next-handle cursor.
   Previously captured local services resolve imported objects after publication.
   Active calls/waits and acquired mutexes reject export/import; thread IDs and
   waiters cannot be serialized. A service gate is a safeguard, not a substitute
   for joining/quiescing the enclosing world's workers.
7. `SourcePhysicsGameArenaNative` now stages the current live game's **retired
   startup arena**: actual physical pages/protections, scratch extent/cursor,
   native mutex/semaphore aliases and two retained file handles with their
   contents and positions. Export requires a real worker join at the original
   idle semaphore wait (`12236A` in the bound correspondence), an empty source
   queue and no pending permits. The replacement retains its own matching,
   actually retired worker; thread IDs, CPU contexts and entry closures never
   cross machines. Imported handles use independent private native files without
   overwriting the replacement's savegame-directory files. Pending/executing
   source jobs cannot be exported through this retired-arena API.

These are subowners; none alone claims a complete physics world or match save. The
first seven are now composed by `PhysicsLiveWorldTu15::AuthorityCheckpoint`,
`prepare_authority` and `commit_authority` (2026-10-14). That captures the current
implemented source physics owner; it does not capture the outer native
`Simulation` or `MultiplayerMatch`, or enable transport promotion.

## Composed source physics owner

The physics checkpoint stages the entire byte bank and every current mutable
`PhysicsLiveWorldTu15::Impl` sidecar: VM/pool/arena ownership, critical sections,
native events, lock-event bindings, allocation notifications, completed resource
reads/cursors, queued input mask, session state, model collectors/scratch/cursor,
pending projectile/damage/melee presentation, original exit callback addresses
and diagnostic instruction state. Biped definitions and BSP/cache bindings stay
local and must match the admitted immutable content. Native thread IDs, mutexes,
callback closures, observers and clock providers remain local. Retired equality
tokens do not get reused; the replacement's executor receives the retained main
token and next-token cursor. Publication swaps prevalidated backing and staged
owners without allocations; every borrowed view/native address must be rebound.

`PhysicsPlatformLocks` now records consumed critical sections and rejects export
or import with active calls, recursion, ownership or waiters. It retains the
original 28-byte transitions and native wait/wake behavior. A failed actual source
call prevents authority export; an admitted checkpoint can replace that faulted
state. Rejected read-only/precondition queries do not invalidate a healthy owner.

`SourceRuntimeResources` binds the actual private resident cache bytes (including
balance writes) and snapshots of the three original shared-resource namespaces.
SHA-256 identities and immutable executable-region comparison reject mismatched
bindings. Shared resource reads stay on their pinned snapshots after filesystem
mutation/unlink. File paths are local. Requiring matching resource banks and
rebasing native monotonic millisecond/cycle domains are explicit native
continuation policies, not recovered Xbox timing/host-selection coefficients.
UTC stays an absolute external clock input. Native 32-bit resource/model aliases
retain wider exhausted cursors rather than silently wrapping/reusing tokens.

The private evidence capsule is `.local/player-host-continuity-20261014/world/`:

- `continuation-qualified.log`: two independently initialized native owners;
  actual original biped/player/equipment creation, ten controller/physics ticks,
  an additional declared source physics call for the in-call guard and actual
  model/resource population. After import, sixty later source ticks match the
  uninterrupted complete bank byte for byte under controlled native clock inputs.
  A later model build/allocation and another source biped birth also match.
  Cancellation, damaged metadata/image/resource identities, repeat handoff and
  recovery from an interrupted source call are covered.
- `continuation-asan.log`: the same test with all changed physics, cache-resource,
  memory, VM/physical/pool/arena, synchronization, thread and lock modules freshly
  instrumented by AddressSanitizer/UndefinedBehaviorSanitizer.
- `native-world-regression.log`: the existing native world completes its 180
  controller/physics ticks using the default actual native clocks.
- `locks-ctest.log`, `locks-original.log`, `locks-asan.log`: the new continuation
  guard plus 1,595 pinned compatibility transitions and actual two-thread
  contention/recursive handoff. No Xbox kernel wait/status ABI is asserted.
- `build*.log`, `verification.json`, `asan/build-commands.json`: fresh native
  library/test builds, source/fixture/binary/archive hashes and focused commands.

This is one Linux process with two independent owners and controlled clock
inputs. Cross-process encoding, live peer checkpoint transfer, cross-CPU and
physical multi-PC behavior, active-projectile/queued-effect/moving-device
acceptance, the outer typed match/simulation and actual transport promotion stay
open. No new runtime/launcher installation, UI asset selection or backend
deployment is claimed by this physics-owner milestone.

## Outer source-pool and lifetime bindings

2026-10-14: `DatumPool::SourceBinding` and its staged view rebind now retain
logical pool address/name/capacity/stride without serializing a native pointer
or making a second allocator. Salt/bitmap occupancy, count/high-water, cursor,
next salt and all elements remain in the enclosing whole-bank checkpoint.
Incoming bindings require committed writable storage and coherent original
occupancy/frontiers. Preparation admits the exact replacement allocation;
foreign, premature and equal-but-separately-restored bank publication reject
without changing the current view. Successful publication rebinds native
addresses without allocation. Ordinary copying of a source view still throws.

`NativeObjectLifetimes::SourceAuthorityCheckpoint` now composes that binding
with its complete native owner map, ordered reserve/publish/retire history and
ordinal. Import replays and checks the history, then checks retained full salts,
types and published definitions against actual incoming bodies. This is native
integration bookkeeping, not an invented original Xbox save/network format.
Owned pools keep their existing value-copy path.

`PhysicsLiveWorldTu15::PreparedAuthority::replacement_memory()` exposes the
replacement read-only so these outer bindings stage against the exact bank the
physics transaction will publish. Current-pool inspection is read-only and does
not reacquire resource write/protection locks held by that transaction. The
native test initially exposed that lock conflict; the corrected path completes
while physical-resource staging holds its publication locks. The enclosing
match must still quiesce all callers/borrowed spans through commit/cancellation.

The evidence capsule is `.local/player-host-continuity-20261014/pools/`:

- `ctest-qualified.log`: six registered tests pass: new pool/lifetime staging,
  existing object lifetime coverage, original owned/source-bound pool fixtures,
  original pool fixtures with a staged bank handoff after every snapshot, and
  the actual TU1.5 composed physics continuation.
- `original-pool-continuation.log`: original expected allocation identities,
  snapshots and full-salt lookups continue after every imported pool snapshot:
  77 allocation outcomes, 96 snapshots and 28 full-datum lookups match.
- `physics-continuation-qualified.log`: the existing independent-source-owner
  test now stages and rebinds its outer player view and object lifetime owner;
  sixty later complete source ticks, later actual model/body allocation and
  actual original body retirement match. Repeated imports and recovery from a
  failed source call rebind these owners too.
- `asan-qualified-test.log`: malformed/cancelled imports, exact backing,
  allocator/lifetime continuation and subsequent mutations pass with all
  focused pool/lifetime/memory/raw-pool modules freshly instrumented.
- `physics-continuation-asan.log`: the same actual TU1.5 continuation test passes
  with the changed physics/resource/memory/platform/lock owners and the added
  pool/lifetime modules freshly instrumented by AddressSanitizer and
  UndefinedBehaviorSanitizer. Other generated runtime libraries use the current
  normal native build; this is not whole-application instrumentation.
- `focused-test.log`, build logs, `verification.json`,
  `asan-build-commands.json` and the build script record sources, fixtures,
  binaries, linked archives and commands for this focused integration.

This resolves the source-backed pool/lifetime seam needed by outer capture; it
does not yet add complete `Simulation`/`MultiplayerMatch` capture or live peer
promotion. No runtime/launcher installation, backend deployment or Vista asset
change is claimed by this step.

## Composed TU1.5 native object bridge

2026-10-14: `MultiplayerObjectRuntime::AuthorityCheckpoint` now composes the
TU1.5 physics checkpoint with its pending native fatal-damage reports. Each
report retains full salted unit identity, the original produced damage class,
direction and force operand for later ordinary cleanup. Import validates it
against the actual incoming unit and fatal transition, stages the map and
physics/resources, and publishes both without allocations. Resource observers,
clock providers and returned observer-world services remain local. Those local
services resolve the replacement bank after publication.

All 58 existing public bridge entry points now participate in a completed-call
barrier. Returned TU1.5 observer services also guard their calls, including
scratch writes before nested source execution. Same-thread reentry during
staging and capture during multi-call bridge operations reject. An interrupted
bridge operation that changed source bytes prevents export until an admitted
checkpoint replaces it. Read-only/precondition rejection does not invalidate
a healthy owner. External raw memory/spans still require the enclosing match's
caller barrier. The retained executor and native thread identities stay local.

Explicit original body retirement removes its pending report; completed native
world steps also discard reports whose full source identity actually retired.
New-map setup clears the superseded report map. This does not replace original
damage, fatal admission, death animation or physical cleanup with a native
copy; the existing original request/report/physical/cleanup graph still runs.

The private evidence capsule is `.local/player-host-continuity-20261014/bridge/`:

- `continuation-qualified.log`: two independently constructed TU1.5 owners,
  actual biped/player/equipment, outer pool/lifetime staging, cancellation,
  foreign/malformed import and executor/reentry guards. Thirty source ticks
  continue byte for byte after the first import. A defined-input fatal stimulus
  uses actual cache SMG/vitality definitions and native damage arithmetic, then
  the actual original reaction graph produces a nonempty pending report. Import
  retains it, explicit original retirement removes it, and restored ordinary
  cleanup consumes it once with identical complete source memory. Twenty more
  source ticks match, then known-good import recovers an interrupted bridge call.
  This is continuation of the admitted damage path, not weapon-hit admission
  or complete combat-parity proof.
- `continuation-asan.log`: the same test passes with the bridge and the changed
  physics/resource/memory/platform/synchronization/lock/pool/lifetime owners
  freshly instrumented by AddressSanitizer and UndefinedBehaviorSanitizer.
  Remaining generated libraries use the current normal native build.
- `ctest-qualified.log`: TU1.5 object bridge, TU1.5 physics continuation,
  source-pool continuation and default-clock TU1.5 death-camera tests all pass.
- `build*.log`, `asan-build-commands.json`, build scripts and `verification.json`
  record exact sources, fixture/binary/archive hashes and test scope.

The user confirmed **TU1.5 as the sole gameplay target** during this step.
TU1.1 construction tests are outside the ONLINE completion target; investigation
of their separate constructor failure was stopped, without substituting a
guessed heap owner. Earlier-image references still require established TU1.5
correspondence before they inform gameplay parity.

The departure audit also reconfirmed that current live `leave()` frees its
player allocation and current ONLINE PCR publication rejects disconnected
participants. Retained ordinary departure, its complete match/results owner,
outer `Simulation`/`MultiplayerMatch` capture, peer transfer and actual transport
promotion remain open. This source checkpoint does not enable migration in the
installed client or change Vista UI/assets/backend delivery.

## Scene authority continuation

2026-10-14: `SceneObjects::AuthorityCheckpoint` retains its whole ordered object
bank (including vacant slots), full-handle lookup/name tables, BSP, creation
ticks, visibility/script/inventory flags, attachment links, solved roots/WORLD
nodes and shader interpolation operands. Standalone owned lifetimes travel as
values; shared owned/source lifetimes remain a separate enclosing owner. Local
content references, construction/deletion callbacks and definition resolvers
stay bound to the replacement. The existing campaign copy/restore path remains
separate and its multiplayer guard has not been relaxed.

Admission fingerprints all scene-used effective placement/name/transition/
volume metadata and game/factory/resolver roles using explicit scalar encoding.
This is a native component binding, not an original Xbox save/network format or
a replacement for the enclosing full executable/cache/schema authentication.
Preparation validates live full salts/types/definitions, lookup tables, finite
poses/interpolation and owned pool shape. Source preparation checks the actual
incoming body/lifetime bank and requires the exact staged allocation. Publication
waits for the enclosing world and lifetime owner to publish/rebind, then swaps
preallocated scene state. Cancelled, foreign or premature commits preserve the
current scene. Display replicas cannot export or import authority.

Scene operations now guard completed-call capture and staged mutation. A failed
operation after a mutation invalidates capture until admitted restore. This is
an externally quiescent, single-executor seam; outstanding mutable `get()`
references, shared lifetime borrowers and full simulation calls still require
the enclosing owner barrier. It is not a new internal thread-safety claim.

Evidence is `.local/player-host-continuity-20261014/scene/`:

- Owned tests retain exact metadata/poses, interpolation and future full-salt
  allocation order between independently bound content/callback owners. They
  reject malformed bindings/lookups/poses/allocator shape, staging reentry,
  foreign commits, display replicas and premature shared-pool publication.
- The native test composes two actual TU1.5 worlds, native lifetime owners and
  scenes using explicit native caller placements backed by the real cache biped
  definition and actual source biped constructor/destructor. It verifies source
  backing publication followed by pool rebind followed by scene publication,
  ten future complete source ticks, subsequent deletion/recreation, independent
  local callbacks and recovery from an interrupted outer callback.
- `continuation-asan.log` repeats those checks with 22 selected scene/world/
  pool/platform/test modules instrumented by ASan/UBSan. Generated runtime
  libraries use the normal build; this is not whole-application instrumentation.
- Build/test logs, instrumented dependency/command records and `verification.json`
  record the bounded source/fixture/binary/archive evidence.

Original scenario factories remain a separate acceptance boundary. The attempted
authored fixture found no multiplayer-admitted scenery in Lockout. An admitted
Zanzibar scenery birth succeeded, but actual deletion reached the still-missing
TU1.5 `10F2F0` callback through `10DCEA` under `BA490`. The attempted source and
failure/admission logs are retained. No callback was substituted, and the biped
component test does not claim original scenery deletion or every factory type.
Complete projectile/presentation/outer simulation/match/results ownership,
ordinary retained departure and live peer transfer/promotion remain open. No
runtime/launcher install, UI asset selection or backend deployment changed.

## Prop and static collision component continuation

2026-10-14: `PropSandbox` now exports/stages its complete implemented native
value state: full identities, vitality/recharge fields, captured damage owners,
section destroyed/pending/damage bytes, region choices and looping-effect
handles, actual roots/WORLD nodes, active flags, collision-dirty state and
pending effect/damage/removal requests. Import binds definition pointers to the
replacement's own content. All target allocations complete during preparation;
commit swaps the admitted banks. Prop region display replicas cannot export or
import authority.

`StaticSceneryCollision` now retains every full-generation placement/root/
active/node owner and its cached triangles. Import reconstructs the admitted
triangles from the target's definitions and saved WORLD nodes, and verifies
every scalar/source identity and the exact order before retaining the original
cached values. This concerns the existing native triangle backend; it is not
an implementation of missing original Havok or source scenery deletion.

Both components bind their effective immutable collision, hierarchy, region,
damage-response and material metadata with explicitly encoded scalar SHA-256
identities, plus the enclosing admitted scene binding. Render mesh/resources
remain separate owners. These are native component admission policies, not Xbox
save/network records or complete image/map/schema authentication. The parent
must stage the complete scene against its own content/identity owner and publish
all components under one quiescent root barrier before simulation resumes.

`NativeAuthorityBoundary` rejects capture during component calls/staging and
rejects mutation while a replacement is staged. A partially changed operation
or interrupted callback prevents capture until admitted restore. The root still
owns executor/quiescence across shared borrowers and callbacks; this helper
does not add independent internal thread safety or cross-owner atomicity.

Evidence is `.local/player-host-continuity-20261014/components/`:

- The component test uses two independently loaded real Lockout definition
  owners and the actual native prop damage/response adapter. It captures a
  damaged power module partway through its authored countdown. After composed
  scene/prop import, twelve subsequent response/collision steps agree, emit the
  same attributed response once, retire the old full salt and recreate the same
  replacement salt. An explicitly declared zero-delay definition fixture
  separately verifies pending request import, one-use delivery, in-callback
  capture rejection and recovery after callback interruption.
- Real Zanzibar fixed-scene definitions exercise moved roots/solved WORLD nodes,
  exact cached triangle order and source identities, cancelled/foreign/forged
  imports and later hide/delete/recreate transitions. Source definition banks
  are destroyed before the replacement uses its retained local pointers again.
- The existing public Match firing regression is run with the actual TU1.5 XBE:
  14 Lockout modules, 840 authored prop triangles, 552 posed casts, 333 blocked
  native moves and firing/blast/removal in 57 ticks pass. This uses the existing
  native triangle/gameplay adapter; it does not claim complete original physics
  scene construction or full combat parity.
- Five registered component/scene/TU1.5 scene/campaign/replica tests and ASan/
  UBSan over eleven selected component/scene/lifetime/test modules pass. Other
  generated libraries use the normal build. Commands/dependency records and
  `verification.json` retain sources, fixtures, binaries, archives and logs.

These components still need composition into the complete outer simulation/
match/result checkpoint, peer transfer and runtime promotion. Projectiles,
grenade attachments, unit/inventory state, presentation/event consumers and
retained ordinary departure remain required. The original `10F2F0` scenery
deletion callback remains open. No launcher/runtime installation, UI asset
selection or backend deployment changed in this component milestone.

## Script and camera/presentation continuation

2026-10-14: the native `ScriptVm` authority path retains every thread/expression
frame, absolute wake and polling clocks, child/result frontiers, random visited
mask, command actor/wait, globals, suspension and handled fault diagnostics.
Compiled program identity is a scalar SHA-256 binding, so independently bound
equivalent programs can import without copying the old program pointer. Host,
object resolver/list, external-global, primary-RNG and command-ready services
remain local; matching callback roles are required. Referenced list/object and
RNG banks remain the enclosing root's responsibility. Existing campaign
`Snapshot` restore still requires its original program owner and keeps its
separate diagnostic/suspension policy.

Script import validates compiled full node identities, call names/argument
lists, stack/thread frontiers and actor roles before allocating replacement
banks. Stopped faulted threads retain their partial stack and report rather
than hiding the fault. Completed-call guards reject dispatch/staging capture
and staged mutation. Transient dispatch context now unwinds on an outer thrown
callback, permitting an admitted known-good restore after interruption.

`Presentation` retains fade/letterbox/HUD/FOV envelopes, camera command clocks,
current/from/to cameras, observer velocity/acceleration, the actual selected
animation and anchor, and all cinematic/control flags. The target's content
reference and animation loader remain local. Point/flag metadata and selected
image fade-curve data are bound explicitly; camera/transition/key data are
validated before allocation-free publication. Direct public flags still
require the enclosing root's quiescence. Negative transition durations reject
before partial mutation. Campaign restore remains a separate API/policy.

The capsule is `.local/player-host-continuity-20261014/scripts-presentation/`.
Its script fixture captures a partly executed `begin_random`, a sleeping
expression, an actor waiting for completion, and a handled fault. Independent
program/service owners resume the same future callbacks/globals/frames and
primary RNG when the root restores that RNG separately. Malformed/foreign/
cancelled import, staged mutation and interrupted dispatch recovery are covered.

The presentation fixture uses the actual TU1.5 fade curve and Cairo's authored
`x02_01` animation/anchor. Explicitly declared native camera points exercise
observer derivative continuation; those points are not authored Cairo rows.
Sixty subsequent observer steps and twenty-eight animation steps agree,
including render-fraction samples. Import invokes no loader; subsequent commands
use the replacement's loader. An interrupted loader prevents capture until
admitted restore. Initial wrong-cache fixture attempts are retained as test
development evidence, not original behavior or acceptance proof.

Registered tests, selected-owner sanitizer checks and source/fixture/archive
hashes are recorded in the capsule. These are native component tests, not a
complete multiplayer root, original Xbox script-VM/camera parity, cross-machine
codec or host promotion. Unit/inventory, projectile/grenade attachment state,
result/departure ownership, full simulation/match composition and live peer
handoff remain required. No UI assets, launcher/runtime installation or backend
deployment changed. Current native world additions also require re-auditing
new first-person phase trace/CPU-input sidecars before a complete root seal;
earlier physics tests do not establish coverage of newly added owners.

## Grenade component continuation

2026-10-14: `NativeGrenadeSandbox` now retains its complete implemented mutable
projectile bank, native local attachment frames, pending events and bounded
detonation journal with its serial frontier. Import reconstructs decoded unit,
weapon-action and selected-animation caches from the replacement's own cache;
canonical scalar digests verify the effective hierarchy, markers, permissions,
throw speed and decoded animation data. The resident cache and selected-image
operands also bind the component. Complete image/shared-resource/schema admission
remains the outer root's responsibility.

Preparation validates full projectile identities, definition/type bindings,
finite values, attachment nodes and the incoming unit's held-grenade identity,
then allocates all replacement banks. Publication swaps them without allocation.
Cancelled/foreign tokens cannot publish, staged calls cannot mutate, and a
partially changed callback operation prevents capture until admitted recovery.
The root must quiesce shared users and restore Unit action/debit state and exact
object-lifetime ownership together; this helper is not an independent mutex or
a complete match transaction. Borrowed clip pointers expire on publication.

The capsule is `.local/player-host-continuity-20261014/grenades/`:

- Two independently loaded Lockout caches and actual TU1.5 operands exercise
  native frag and plasma preparation. The fixture explicitly constructs a
  native unit adapter and owned allocator; it is not original Havok/body
  construction. Capture occurs while the grenade is held and includes both
  decoded clip caches and pending requests. With the Unit and allocator restored
  separately by the fixture root, future release, triangle flight, fuse,
  attributed blast and one-use retirement agree for forty frag and forty-one
  plasma steps. Actual unit sticking/angular integration and full combat parity
  are outside this native backend's existing scope.
- A declared completed-source input fixture preserves source effect locations,
  colors/controls, the sixty-four-entry retention bound and frontier, and resumes
  at serial seventy-one after recovery from an interrupted adoption. This is
  presentation adoption, not source-world continuity. Default export/import
  rejects source-driven presentation without an explicit enclosing source-world
  qualification; the caller's flag does not itself prove that qualification.
- Eight registered grenade/component/scene/script/systems/campaign/replica tests
  pass. AddressSanitizer/UndefinedBehaviorSanitizer checks instrument five
  selected grenade/binding/lifetime/test units; other libraries use the normal
  build. `verification.json` records commands, dependencies and source/fixture/
  binary/archive/log hashes.

Complete Unit/inventory and weapon-projectile ownership, simulation/match/
results composition, retained ordinary departure, authenticated peer transfer
and live promotion still remain. Recent native-world first-person observer
additions still require their separate acceptance audit, and original `10F2F0`
scenery deletion remains open. No UI asset, runtime/launcher installation or
backend deployment changed. Xbox TU1.5 remains the sole gameplay target; Vista
remains the selected ONLINE/clan UI source.

## Exact device collision continuation

2026-10-14: the enclosing simulation audit found that version1 device restore
eagerly rebuilt posed collision and cleared its dirty flag. A completed device
call can legitimately leave the last-built triangles in place until the next
ordinary collision synchronization, including after a channel write or a full
generation's deletion. Rebuilding on import changes that frontier and can
invalidate the enclosing native body's current triangle contact.

Version2 now retains the exact cached triangles, their ordered full-generation
pose inputs and the dirty flag, in addition to groups, motion/control/door state
and pending requests. Import verifies effective target-local definition/animation/
hierarchy/marker/collision metadata, reconstructs the saved cache from its saved
pose inputs and compares every triangle/source identity/order. A clean cache must
also match the current owner poses. A dirty cache can retain a prior generation
until the normal rebuild. Existing display snapshot format and authority roles
remain separate from this native continuation record.

Preparation validates and allocates without changing the current device owner;
publication swaps the admitted banks without allocation. Completed-call guards
reject capture during callbacks, mutation during staging and capture after an
interrupted operation. Copying an active/staged `NativeAuthorityBoundary` also
rejects, so cloning a component cannot bypass its completed-call qualification.
Idle value copies and the existing campaign checkpoint path remain supported.
These guards still require the root executor's barrier over all borrowers;
they do not provide independent internal thread safety or whole-match atomicity.

The capsule is `.local/player-host-continuity-20261014/devices/`. Independently
loaded actual Zanzibar/TU1.5 definition owners exercise clean and dirty imports,
the real admitted control/gate motion, pending effect delivery, sixty future
motion/collision steps, cancellation/foreign/malformed inputs, deletion and
full-salt recreation. The replacement continues using its own definitions after
the source definition allocations retire. An explicitly declared native door
fixture covers query-callback capture/copy rejection, interrupted-call recovery,
and seventy-one future steps from a partial door timeout through its close
request; that fixture does not claim an authored Zanzibar automatic door.

The existing all-map device regression covers twenty-one maps and fifty-three
admitted machine/control owners. Door flags and open counters are now included
in the shared snapshot comparison. Registered regression results, the actual UDP
moving-gate/late-join check, selected-owner sanitizer scope, and source/fixture/
binary/archive hashes are recorded in `verification.json`. A concurrent attached-
effect readback implementation was already called by the object bridge but absent
from CMake; its missing source registration was added to permit the current build.

This fixes the native device prerequisite; complete simulation/match/results
composition, peer transfer and live host promotion remain required. It does not
implement missing original Havok or source `10F2F0` scenery deletion. No ONLINE/
clan UI asset, launcher/runtime installation or backend deployment changed.

## Native simulation transaction and collision traversal

2026-10-14: `Simulation::AuthorityCheckpoint` now joins its implemented native
value banks with staged scene, device, prop, static collision, script,
presentation, grenade and identity owners. It retains the full Unit records,
four-slot inventory and inactive weapon states, damage/history/action fields,
weapon projectiles with captured attribution and birth/barrel state, native raw
body storage, script lists/salts, primary RNG, absolute clocks, pending journals,
training/weather/audio values, vehicle rows/flips, objectives, debug game values
and the selected BSP. Unit/object animation clips travel by value; replacement
shared allocations and loaders belong to the target. Local callbacks remain
local and matching service roles are required. Melee action/RNG remain in Unit/
Simulation; its immutable decoded memo owner is reconstructed against local
resource/constant/representation bindings.

The native collision solver traverses its BVH in order, affecting tie hits and
iterative capsule response. Export now retains exact triangle order, permutation,
node bounds and topology for BSP/fixed/all views. Import validates the complete
permutation, leaf/internal ranges, partitions, bounds, cycles and orphan nodes,
then retains the admitted traversal rather than rerunning `nth_element`. BSP/
navigation rebinds against the target's actual geometry and fixed/all prefixes
preserve current contact indices. Inactive/hidden bodies can retain a contact
from a prior view; its native uint32 index is preserved until movement resets
it, rather than being rewritten or rejected against a newer shorter cache.
Dirty component caches retain their frontier.

Preparation performs all allocations and child admission. Publication uses
preallocated swaps, preserves the enclosing identity-owner address and rejects
premature source-view publication until the exact staged source bank is in place.
Calls/staging and interrupted root operations qualify capture; raw references
and the enclosing `MultiplayerMatch` still require the parent's executor barrier.
Campaign/debug saved-state owners explicitly reject this ONLINE path rather than
being omitted; their existing checkpoint APIs remain separate. The supplied
admitted-binding digest must come from complete map/image/resources/schema
admission. Comparing that caller-supplied value does not itself authenticate a
peer or establish a complete match/world seal.

Evidence is `.local/player-host-continuity-20261014/simulation/`:

- Independently loaded actual Zanzibar/TU1.5 metadata uses an explicitly declared
  native program, custom clip, inventory/raw-storage and in-flight projectile
  caller inputs. Ninety future unit/inventory/projectile/attribution/RNG/script/
  view/audio/collision steps agree after restore. A partly visited random/sleep
  expression, script-modified health/shield maxima, local animation allocation,
  raw-body append/retirement, cancelled/foreign/malformed inputs and recovery
  after an outer nonstandard loader exception are covered. This is not an
  authored Zanzibar script/custom-animation/projectile birth or full original
  combat/physics acceptance.
- Two actual TU1.5 world owners with independently loaded Lockout caches stage
  their source pool, scene and native simulation together. An explicit native
  biped placement uses the actual source constructor. Early/cancelled publication
  leaves the target bank unchanged; twenty subsequent source object ticks agree
  byte for byte after publication. This fixture has no complete player/match/
  recording roster and does not establish host promotion.
- The initial projectile fixture selected an absent barrel view. The retained
  debugger trace identifies `Simulation::step_impl`'s actual barrel consumer.
  The caller input now uses its real base view and admission rejects absent
  birth barrels. This was fixture development/admission evidence, not an original
  gameplay defect or a substituted source owner.
- Registered regressions, the native application build, selected-owner sanitizer
  scope and exact source/fixture/binary/archive hashes are recorded in the capsule.

Complete `MultiplayerMatch`/recording/results/departure composition, authenticated
cross-process encoding/transfer, local presentation/profile/voice rebinding and
actual peer promotion remain open. Newly modified source-world sidecars still
need their acceptance audit; original `10F2F0` scenery deletion remains open.
These APIs do not enable the installed v12 client's migration path. No UI asset,
launcher/runtime installation or backend deployment changed in this milestone.

## Resource continuation evidence

The additional private capsule is `.local/player-host-continuity-20261013/resources/`.

- `cmake-resource-build.log`, `cmake-focused-tests.log`: registered CMake
  resource, synchronization, VM and native-thread targets built and five tests
  passed. Existing retained-bank and physical byte-protection checks passed in
  `memory-current.log` and `physical-current.log`.
- `pool-original-continuation.log`: 3,552 original TU1.5 pool lifecycle calls,
  122,451 ordered writes, 27 native allocation/release requests and 258,048 final
  bytes matched the source corpus; 3,528 subsequent calls also matched after
  importing occupied pools into a different native owner.
- `arena-original-regression.log`: original game-arena startup and actual
  native-worker/file/synchronization regression with the changed resource and
  synchronization modules. This checks existing arena behavior, not arena import.
- `resources-asan.log`, `synchronization-asan.log`: focused continuation tests
  passed under AddressSanitizer and UndefinedBehaviorSanitizer.
- `verification.json`: exact source, fixture, binary, dependency and log hashes,
  reproduction commands and the bounds of this verification.

The source-pool and arena regressions compile changed modules and tests against
recorded existing generated runtime archives. The small registered continuation
targets compile all their sources freshly. This resource stage has not received
a full native application rebuild/install or a physical gameplay handoff, and
does not replace the installed direct peer runtime.

## Retired arena continuation evidence

The additional private capsule is `.local/player-host-continuity-20261013/arena/`.

- `continuation-final.log`: two independently initialized original TU1.5 arenas,
  with actual startup workers and files. Import preserves file contents/seek
  positions, page protections, scratch and synchronization ownership; native
  addresses rebind. Cancellation, malformed inputs and real file-staging failure
  preserve the current owner. Repeated handoffs retain descriptor counts, and
  destroying the source owner/files leaves the imported resources usable.
- The same test covers callback reentry, worker resource callbacks, rejection
  of nested restore/protect/stop/snapshot during preparation, and rejection of
  an idle-but-running worker, queued jobs and unretired startup owners. These
  guards do not substitute for the complete world's external caller barrier.
- `original-regression.log`: the existing 13 source arena calls still match
  1,049,656 ordered writes and 52,667,416 final bank bytes, including the actual
  source worker and two files, with the changed native arena owner.
- `continuation-asan.log`: the arena continuation test passes with its changed
  arena, memory, physical storage, raw storage, synchronization and native-thread
  modules instrumented by AddressSanitizer/UndefinedBehaviorSanitizer.
- `verification.json`, `build-commands.json`, `asan-build-commands.json` and
  `regression-link-command.json`: source/fixture/binary/archive hashes and exact
  focused build/link commands.

Both tests use the recorded existing generated source-runtime archives; this
does not establish a fresh full application build, gameplay migration, a running
worker/CPU-context transfer, immutable map/image admission or WAN/physical-PC
handoff. The registered `native_physics_game_arena_continuation_tu15` test will
also run with a normal full build. The installed launchers/runtime, UI asset
selection and backend are unchanged by this arena stage.

## Evidence

The private local capsule is `.local/player-host-continuity-20261013/`.

- `backend-final.log`: full master-server regression suite, including real
  HTTP/WebSocket nonhost survival, election/rebind, stale generation rejection,
  lost-capability rejection, reader isolation and retained result receipts.
- `native-transport-final.log`: three actual C++ `OnlineMatchSession` processes
  through the real relay, host departure, wrong-world rejection, handoff,
  survivor rebind and resumed authenticated datagrams.
- `native-legacy-final.log`: the existing native constructor, legacy admission
  and routing, and unchanged unsupported-continuation behavior.
- `memory-tests.log`: whole memory-bank import/atomic rejection and identical
  subsequent real source body-heap allocation.
- `device-tests.log`: authority continuation plus existing device checks over
  21 owned Xbox maps and 53 admitted machine/control owners.
- `verification.json`: source, focused binary, dependency and log hashes.

The native relay test supplies declared world-seal fixtures. It does not capture
or import a real game world. The changed C++ modules were compiled and linked
against the recorded existing native baseline; this stage has not received a
full application rebuild/install or a gameplay/physical-PC migration test.
Discord verification in the protocol harness is an external fixture.

Registered native transport checks:

```sh
cmake --build build-multiplayer --target online_continuity_transport_tests
python3 tests/online_continuity_transport_http_test.py build-multiplayer/online_continuity_transport_tests
python3 tests/online_continuity_transport_http_test.py build-multiplayer/online_continuity_transport_tests legacy
python3 -m unittest discover -s multiplayer/master_server/tests -t .
```

## Required before enabling live migration

The [authoritative-state audit](native-migration-state-audit-20261013.md) remains
the complete owner inventory. The outstanding integration includes:

1. Complete portable match/simulation continuation: all typed player/unit/
   weapon/inventory, RNG, timer, allocation, projectile, prop, scene, engine and
   running result/recording owners. The current `MultiplayerState` projection
   cannot replace these banks.
2. Quiescence and export/import/rebinding of TU1.5 physics/game-arena workers,
   actual host page/resource registries, synchronization, source call contexts,
   resource/file jobs and runtime sidecars. A byte-bank import alone does not
   restore those owners. Immutable image/map bindings must be authenticated.
3. Transfer or synchronized execution of that complete world before abrupt
   host loss, followed by validation of its exact completed tick and digest.
   No game path calls the new seal API merely because positions look equal.
4. Live retained departure through the recovered player departure owner and
   actual deferred unit damage/cleanup, with lethal statistics suppressed.
   Current `MultiplayerMatch::leave` still performs slot deletion; it must not
   be used as ordinary retained departure. Native PCR publication/validation
   must retain those allocated left-player rows.
5. Runtime tick freeze and actual host/client transport promotion/rebinding over
   the validated world, preserving salted identities, local controllers and
   selected profiles. Rebase presentation cursors and restore voice routing.
   The current runtime constructs transport roles once.
6. Gameplay acceptance for graceful and abrupt host loss with at least two
   surviving physical PCs, ordinary and four-seat departures, repeated migration,
   candidate failure, outstanding projectiles/reloads, moving devices, results,
   guest privacy and voice. Then rebuild/install both launchers and deploy the
   backend with retained-state backup/restore verification.

Until those requirements pass, the live client and service continue their
existing behavior. This stage must not be described as completed gameplay
disconnect survival or enabled host migration.

## 2026-10-14: enclosing native Match authority transaction

The new `MultiplayerMatch::AuthorityCheckpoint` joins the previous Simulation
transaction to the enclosing native match values. It preserves the salted
player pool and roster/profile requests, current/previous unit bindings,
retained units and deep animation clips, corpses, pickup/ammunition and world
item rows, garbage ownership, native engine/round/game-over values, all raw
match/result statistics and PvP/team banks, recording gates and the complete
1,000-entry event ring. Pending effect/rumble journals, native controls and
first-person/replicated view clocks also survive publication.

Preparation binds the complete effective native rules/spawns, King markers,
world weapon definitions, local and replicated first-person definitions,
selected local roster, result descriptors and game-over operands. Callbacks
and immutable definitions remain target-local. Player/full object generations,
current unit links, retained corpse/pickup owners, channel bounds and journal
frontiers are checked before publishing. Source-backed player and Simulation
views stage against the same exact external replacement bank; the external
world must publish first. The new source-backed Match path is not accepted by
an actual whole-Match source fixture in this milestone.

Public Match mutations and the registered external source effect observer now
hold a completed-call guard. Reentrant capture, mutation during staging,
foreign/reused tokens and partially interrupted callback export are rejected.
Preparation may recover a faulted owner from a previously admitted checkpoint.
The external session executor still must quiesce all borrowed references, raw
source-world users and callbacks. The supplied digest is an admission contract,
not authentication or an encoded peer message. Successful publication expires
borrowed references and performs no allocations.

Native continuation fixtures use two independently decoded Lockout/TU1.5
metadata/animation/result owners, with an explicitly declared debug arena
(collision/kill volumes removed and noclip/teleport inputs). Four local views,
actual stock SMG fatal hits, pending respawn, salt/inventory ownership,
retained corpse/pickups and source score/stat/lifecycle writers continue for
180 subsequent steps through game-over. King uses actual authored hill data,
its original native scorer and real retained event production with declared
debug-position inputs; 90 subsequent occupancy/recording steps agree. The
fixtures cover cancellation, malformed admission/late child rejection, wrong
allocation generations, local callback retention and nonstandard callback
interruption/recovery. Initially unallocated Simulation actor slots now retain
their absent definition instead of requiring an invented biped on import.

Qualification is recorded in
`.local/player-host-continuity-20261014/match/verification.json` after fresh
registered checks, selected sanitizer instrumentation and native application
build. No gameplay/UI assets, installed launcher/runtime or deployed backend
are changed by this milestone.

Remaining gates include original PCR header/machine/identity completion,
ordinary retained departure and guest publication through disconnect, actual
source-backed Match continuation with every newer source-world sidecar, and
portable authenticated encoding/transfer. Different local presentation rosters
must rebind before promotion; the current matching-roster checkpoint is not a
complete multi-PC handoff. Continuous shadow execution/capture performance,
voice/controllers/headsets and graceful/abrupt physical-PC acceptance remain
open. The ONLINE goal and live migration enablement remain unfinished.

## 2026-10-14: coupled TU1.5 source world and actual four-player Match

`SourceMatchAuthority` now captures and stages the real `MultiplayerObjectRuntime`
and its native `MultiplayerMatch` together. Publication moves the exact source
backing first, followed by the native player/object/scene views and all Match
values. Cancelling destroys the borrowed-view tokens before their replacement
bank. Foreign, reused and active-stage transactions cannot publish.

The enclosing validator checks the actual TU1.5 source clock/player-pool globals,
full allocated player identities, current-unit bindings, selected configurations,
departure flags and membership. It also binds the actual game-options mode,
30 Hz rate, seed and complete game variant to local admission. Different cuts,
source clocks, profiles, units, seeds, rates and variants are rejected before
publication. Original data addresses are used for these checks; native allocation
pointers are not saved. This remains an in-process continuation API with an
external session executor/quiescence and admission-digest contract, rather than
an authenticated peer encoding or promotion path.

Two independently decoded Lockout/TU1.5 caches, private file/resource owners and
separate controlled native clocks construct the actual authored world, four
real source players, their selected profiles, equipment and local source views.
The first fixture restores the initial Match. The active fixture delivers real
source firing/reload/grenade inputs before capture at tick20: the stock SMG
magazine changes from60 to54, and an original source projectile is attached to
the throwing unit (grenade phase2/counter3). These are actual constructor/action
outputs, not supplied projectile, body, ammunition or animation snapshots.
Both fixtures compare the full raw bank, source lifetime history and native
player/unit/inventory/vitality/grenade/recording values for forty later real
input/world ticks. Source and target use their own retained callbacks/resources.
The native debug arena from the previous Match milestone is not used here;
normal authored spawn/scene/physics paths are retained. The fixture disables
native score/time limits and supplies deterministic clocks/inputs for bounded
continuation, not an original network recording or complete gameplay-parity run.

Fresh registered checks and selected ASan/UBSan qualification, sources, linked
archives, fixture and binary hashes are recorded in
`.local/player-host-continuity-20261014/source-match/verification.json`. The
native application is rebuilt; installed runtimes/launchers, UI assets and
backend deployment are unchanged at this milestone.

This closes the earlier lack of any actual source-backed whole-Match
continuation fixture. It does not establish source combat scoring/final PCR
publication, ordinary retained departure, every newer source-world sidecar or
sound-resource lifecycle, actual physical-PC transfer, continuous shadow-capture
performance, different local-view roster rebinding or voice/controller/device
survival. Original scenery deletion10F2F0 and all other completion-checklist
requirements remain open. No live migration switch is enabled by this API.

## 2026-10-14: actual TU1.5 source player birth and ordinary departure

`PhysicsLiveWorldTu15::depart_player()` and the enclosing
`MultiplayerObjectRuntime` now deliver the actual allocated player's low slot
and retained twelve-byte identity to TU1.5's original88C50 verification gate.
The complete155920 leave and15A860 engine-leave bodies execute against the
owned world. They retain the allocated salted player, identity and profile;
remove machine/local control and first-person ownership; mark flag2 and the
source leave tick; decrement the separate active census; and invoke original
engine/event/retained-result remapping. The current unit receives the original
deferred death request and ownership unlink. Departure does not substitute an
immediate native kill, weapon drop or allocation deletion.

The generated module includes the complete19AE70 retained remapper, actual
machine-query owners, original Slayer callback and original stack probe. Its
single stack-probe `xchg esp,eax` lowering reads both registers before either
write and preserves flags. All726 instructions in eleven entry bodies are
bound to the authenticated English TU1.5 image; regeneration reproduces the
same C++. Generated entry coverage is distinct from branches reached by the
bounded native fixture. Existing downstream original owners remain mandatory.

The stronger fixture exposed a real birth bug: the source pool contained four
allocated players and four local users while its separate active-player census
was zero. The old direct14EBE0 record-update call skipped the new-player owner.
Construction now runs the already-allocated14EAD0 continuation, which invokes
14EBE0 in new-record mode, increments the active census and reaches the original
15A950 engine/results join. No count is supplied or repaired by the test. The
coupled Match validator also binds this actual source census to its allocated
nondeparted roster and rejects an otherwise matching cut with a forged count.

The registered source departure fixture constructs four real player records,
stock SMG equipment and local views on actual authored Lockout/TU1.5 scene and
physics owners. It checks retained full salts/identities/configurations, real
local control removal, source leave tick, repeat/stale rejection and a deferred
unit death consumed one later original world tick after the tested departure.
It also rejects mutation while replacement backing is staged and removes the
remaining source participants: active census reaches zero, while all four
allocated identities remain retained. The raw original recording gate is zero
in this fixture. Unchanged retained and engine numeric banks therefore do not
prove active source stat-writer suppression or final PCR completion.

Qualification, executable/cache/source/archive hashes, deterministic generation,
registered continuation/birth/death-camera checks, selected ASan/UBSan modules
and the native application build are recorded in
`.local/player-host-continuity-20261014/departures/verification.json`.
Normal binaries and linked archives are preserved at the passing build cut;
selected sanitizer sources/headers are also preserved before compilation.
Concurrent later workspace edits are outside this bounded qualification.
The earlier coupled Match initial/active fixtures still qualify forty future
source ticks, including real changed ammunition and a held grenade, after the
birth-owner correction.

This is a source-world foundation. Native `MultiplayerMatch::leave()` still
deletes its allocation and remains shared with failed-admission rollback.
Actual runtime departure must be separated from cancellation, preserve the
native roster/results, consume source death without duplicate scoring or
cleanup, and survive transport/room/PCR policies. Those integrations remain
open. New concurrent source-world sidecars, sound-resource lifecycles and the
native unit-interaction asset owner also need current continuation audit.
No live host migration, peer quorum, installed runtime/launcher delivery,
Vista UI asset change or backend deployment is claimed by this milestone.

## 2026-10-14: retained native TU1.5 departure, real UDP survival and direct-room review

`MultiplayerMatch::depart()` now invokes the actual TU1.5 source departure and
adopts the original retained player flags/current/previous bindings. It keeps
the native allocated roster, selected profile request, chosen name and numeric
result owners. The source body stays alive until its original deferred world
request runs. The later native projection adopts that completed corpse and
the source inventory/drop result without calling another kill, weapon drop,
native garbage admission or death/score accountant. Departed inputs are
neutralized, retained rows travel in the existing native snapshot, and voice
routing excludes flag2 participants. `connected` means retained roster presence
here; it is not alone an active-gameplay or voice-membership test.

The source-backed `leave()` path uses this owner. Transport admission rollback
now explicitly calls `cancel_join()`, retaining the older rollback implementation
as a distinct operation. Complete original source cancellation is still open:
TU1.5's separate deletion14F090 reaches pool release16E8A0, retained remapping
and object player-reference callbacks throughBD920/10E010. Partial source
constructor failure also needs a qualified transactional recovery boundary.
Native-owned compatibility/custom worlds still use the older deleting leave
behavior; this stage does not claim Fiesta/Infection disconnect/PCR completion.

The four-source-player fixture departs the actual stock-SMG/held-grenade owner
after twenty original ticks. It captures before deferred death, imports that
pending state into the second source authority and compares the complete raw
bank/native owners before and after the first real world tick. Both copies
consume the original request, keep three remaining players and add no numeric
death or score. A subsequent settled cut continues for forty more source ticks.
Existing initial/active cases, staging/malformed checks and native Match checks
remain qualified. This is in-process composition, not portable peer promotion.

The new real native UDP fixture uses two local source players and three actual
localhost UDP clients. One client sends its authenticated native leave packet;
another stops sending and reaches the real five-second transport timeout while
the remaining client continues. Native source ticks, allocation identities,
chosen profiles/names, retained result rows and original corpse presentation
continue at that remaining peer without added numerical deaths or a room-ending
native exception. The fingerprint and endpoint/inputs are declared test inputs;
this is native protocol/source-world localhost evidence, not retail wire,
Discord authorization, physical consumer-NAT or live master integration.

The direct master now distinguishes an explicitly authenticated nonhost leave
from temporary master TCP loss. It retires only that connection's capability
and future confirmation requirement, keeps every admitted result row and leaves
the running host unchanged. Result review can continue when a participant,
including the already-finished world's host, explicitly leaves. All required
survivors must confirm; the ledger records original/confirming connection counts,
departed seat mask and fixed-host epoch0 in the existing survivor metadata table.
Guest personal-history exclusion remains in the shared commit path. This mask
describes departures before confirmation; it is not a new inferred quit penalty.
The native direct client accepts completed-world voice membership during review
and validates physical-owner/guest seat consistency. Live-world routing yields
the microphone/packet queues to the existing passive session pump during review
and postgame. An in-flight master reconnect rechecks its bearer after the
handshake, and a replaced reader cannot revoke the new connection; late ICE to
retired endpoints is ignored. No gameplay/media relay or
active-world host election is introduced.

Unannounced UDP loss still needs authenticated after-match membership/result
arbitration: a missing master TCP reader cannot itself retire a still-running
UDP player. Live host departure also remains fail-closed until a validated
surviving simulation can take authority. Neither the passing source transaction
nor a changed server host field supplies that missing peer handoff.

Qualification is preserved in
`.local/player-host-continuity-20261014/native-departures/verification.json`,
including native check logs, master HTTP/DB regressions, selected sanitizer
inputs, binaries and linked archives at their tested cuts. Concurrent later
workspace edits are outside that qualification. Installed launchers/runtimes,
production master deployment, UI assets and physical voice/controller/WAN
acceptance are separate delivery gates. The full ONLINE goal remains active.

## 2026-10-14: vehicle interaction definition admission

Xbox TU1.5 remains the sole gameplay target; Vista remains the ONLINE/clan UI
asset requirement. Simulation authority checkpoint version2 now binds locally
loaded unit interaction assets by value: seat flags and admission limits,
boarding relationships, messages, posed markers, model hierarchy/permutations,
all entry/idle/board/exit clip data and initial transforms, flip operands and
netgame placement indices. The binding also includes netgame equipment rows
and weapon inventory flags/capacities used by these consumers. Allocation
addresses are excluded; another independently loaded owner can agree.

Preparation rejects a different interaction binding before staging child
owners. Validation also rejects nonfinite seat-entry positions, future active
seat-action clocks and invalid flip identity/direction/tick/velocity values.
Missing scene parents/flip targets are not newly forbidden: the native update
already clears or retires these transient states on its next tick.

The native vehicle fixture declares approach locations, a native-owned player
generation and an overturned scene pose against actual Zanzibar/TU1.5 assets.
Entry, exit and flip still require the production prompt/admission path; restored
owners must continue their seat/animation/respawn/flip clocks with independent
clip allocations. These declared native inputs do not prove original Xbox
spawns, the complete original animation controller or Havok vehicle dynamics.
The coupled source Match fixture also rejects a changed interaction binding
and an obsolete Simulation checkpoint without publishing its staged source bank.

Verification artifacts are kept in
`.local/player-host-continuity-20261014/unit-interactions/`. This remains a local
continuation admission milestone. Portable authenticated state transfer,
different local controller/profile/view ownership, live promotion and physical
host-loss survival remain outstanding. No launcher installation, production
deployment, UI asset change or full ONLINE completion is claimed here.

`verification.json` records four passing preserved-binary invocations: the
native Simulation/vehicle/composed-source fixture and the initial, active and
pending-departure four-player source Match fixtures. Native and source objects
continue after restoration, including forty subsequent source Match ticks.
Two test binaries, selected input snapshots, fixture hashes and logs are
preserved. This does not seal every concurrently edited compiler input.

## 2026-10-14: staged source local-view rebinding

`SourceMatchAuthority::prepare_with_local_views` can prepare an incoming cut
for this executor's configured local roster through a separate disposable
source executor. The workspace first restores the complete incoming source
world and Match, then runs the existing original TU1.5 `151C90` local-view
owner, including its `18A0C0` input and `16B16A` first-person transitions.
The rebound complete cut is captured there and staged on the live target
through the normal world/Match transaction. Cancelling that preparation must
leave the live target's world unchanged. The workspace is consumed and may need
reconfiguration or rebuilding before reuse; it must have private files and
local-only observers. This is an explicit owning executor, not borrowed live
source memory or a replacement source-function return.

The caller must supply each local slot's already admitted full source player
generation and twelve-byte identity, in its configured local-view order. The
operation validates those against the incoming source records, rejects stale,
missing or departed selections, and cannot derive admission from a name or the
incoming cut itself. Newly constructed offline identities on a replacement PC
are not a session admission: the current constructor derives them using the
native machine address. Authenticated peer identity distribution and physical
machine/user grouping remain part of the pending live session integration.

Normal coupled capture/preparation now validates original player local indices,
the original local-view map, and original/native first-person current units
against the native configured roster. These checks account for a retained left
player releasing its source view without deleting its native result row.

The source fixture uses independent caches, private files and three executors
with different clocks. It exercises four local views per PC across eight actual
source player records, live stock SMG ammunition and an attached grenade,
repeated rebinding, cancellation, a declared different prototype identity and
pending local departure. It compares complete live source object bodies and
complete source player rows except their deliberately rebound local-index
word. It does not compare every other source arena byte after different local
presentation calls. The explicit alternate prototype identity is a fixture
input, not proof on a second physical PC.

The artifacts live in
`.local/player-host-continuity-20261014/local-view-rebind/`. No peer codec,
authenticated live admission, actual authority promotion, Discord/hardware
profile/controller/voice rebinding, consumer-NAT match, live source-audio
lifecycle transfer or few-frame migration timing is established by this stage.
The source audio quiescence guards remain in force. Installed launchers,
production services and Vista UI assets are unchanged; the complete goal stays
active.

The eight-player rebind invocation passes. Original exact static CFG traces
for `151C90` (52 instructions), `18A0C0` (92) and `16B16A` (30) are preserved in
`tu15-local-view-calls.json` against the owned authenticated TU1.5 image. They
identify the source routines; runtime proof comes from the separately preserved
native binaries and logs. The final source regressions also reject an altered
original view map or original/native first-person unit before publication.

## 2026-10-14: owning source authority codec and separate process continuation

`encode_source_authority`/`decode_source_authority` now transfer the owning
`SourceMatchAuthority::Checkpoint` through a named-field binary format. Scalars
use explicit little-endian values; float/double bit patterns and complete raw
source banks are retained. Every aggregate field is visited by name. Private
body-storage, owned datum/list banks, sound envelopes and nonaggregate grenade
action state have explicit visitors; the SMG's built-in hand-timer array also
has an explicit visitor. An unknown state owner requires a compile-time visitor
decision. Native process addresses, borrowed owners, present process pointers
and native-owned lifetime banks are rejected; source32 addresses stay data.

The versioned envelope has exact encoded/decoded extents, a SHA-256 corruption
checksum and bounded zlib decompression. Decoding checks schema names/types,
integer narrowing, fixed extents, map/set duplicate keys, dynamic storage and
element budgets. Unscoped state enums are validated before constructing an
enum value. Character and enum encoding does not depend on a compiler's plain
character or enum signedness choice. Fixed/default owner and decompression
storage are separate from the logical dynamic-value budget; no total RSS cap
or network authentication is asserted by these limits.

The four-player active Lockout/TU1.5 fixture retains the changed SMG magazine
and held grenade through encode/decode and whole source-world preparation.
The decoded owning archive re-encodes identically, and forty future source
world banks match. A separately executed receiver also continues forty ticks
and matches the producer's complete owning checkpoint, including all native
values and source sidecars. GCC and Clang producer/receiver combinations pass
in both directions; the Clang codec/test uses unsigned plain `char`. These are
Linux processes with independent clocks/files/cache owners, not Windows,
macOS, different consumer NATs or a live Discord match.

Corrupt/truncated/trailing/version/size/allocation/element payloads, a validly
checksummed invalid state enum and a present animation pointer are rejected.
A decoded incompatible biped definition is still rejected by normal authority
preparation without publishing the replacement source bank. The active fixture
compresses to approximately8.5MB; it is a correctness fixture, not an acceptable
per-tick bandwidth or few-frame promotion measurement. Immutable/delta reuse,
authenticated generation-bound UDP delivery, acknowledged cut selection and
actual host promotion remain to integrate. The master receives no mid-match
state or media through this codec.

Boost.PFR1.92 is vendored as a private header-only dependency with its original
notices and Boost1.0 license in `third_party/boost-pfr`. Public codec headers do
not expose Boost. Validation inputs, exact linked native archives and separate
compiler binaries are preserved in
`.local/player-host-continuity-20261014/authority-codec/`. This stage changes no
installed launcher, production service or Vista UI asset. The full ONLINE goal
remains active; live source-audio/resource lifecycle and custom native-owned
mode continuation gates are not relaxed by the new format.

The final preserved GCC-to-Clang and Clang-to-GCC runs pass, including the
checksummed malformed-enum and incompatible-definition cases. ASan/UBSan with
leak detection also passes the archive round trip and separate exec receiver.
Only the codec and test translation units are instrumented; their54 linked
native archives are preserved uninstrumented inputs. `verification.json`
records exact frozen compiler inputs, compiler/link commands, binaries,
fixture hashes and log scope. Later concurrent workspace edits are outside
that qualification.

## 2026-10-14: authenticated direct peer authority archive delivery

`PeerAuthorityTransfer` now delivers bounded owning archives over the room's
existing authenticated encrypted UDP channels. Every packet binds the room,
authority epoch, agreed physical-owner membership, host, recipient and cut ID.
The sender uses 1024-byte selective-repeat fragments, a bounded window and a
shared rate budget. Receivers reject wrong endpoints/contexts, stale or timed
out cut IDs, malformed extents and corrupt complete data. A local timeout
does not remove a peer from agreed membership or elect another host.

Byte receipt and complete world validation have separate acknowledgements.
The receiver exposes a complete archive only after its checksum matches; the
owning executor must decode and prepare the world before acknowledging that
exact cut. A capability handshake or checksum alone never certifies a world.
Context replacement requires an externally committed authority decision.

`OnlineMatchSession` exposes this owner and pumps it directly after queued
gameplay/voice datagrams, with its own rate/window limits. Archive bytes never
enter master signaling or its media queue. Capability checks also require the
physical DTLS connection to remain ready. An ordinary nonhost DTLS failure no
longer ends the healthy host session; a send racing endpoint closure is handled
as datagram loss. Host failure still ends a client session until actual world
promotion is integrated.

Qualified evidence is retained in
`.local/player-host-continuity-20261014/authority-transfer/verification.json`:

- Deterministic loss, reordering, duplication, corruption, stale context,
  wrong endpoint, timeout replay and couch-only room checks pass. Concurrent
  pumping, publication, executor acknowledgement, observation and context
  replacement pass ordinary, ASan/UBSan with leak detection, and ThreadSanitizer
  runs. Sanitizer instrumentation covers the transfer module and its test;
  it does not cover the full native game or third-party crypto/transport stack.
- The actual eight-player TU1.5 source fixture delivers an 8,664,614-byte cut
  across encrypted UDP with deliberate packet loss. Thirty source ticks run
  during delivery alongside 798 media-sized and 798 control-sized datagrams.
  A separate executor restores four local views from 0/1/2/3 to 4/5/6/7,
  acknowledges the prepared cut, consumes an explicit test input log and
  matches forty further ticks of complete live source object bodies. Its
  admission identities are separately trusted fixture inputs. The datagram
  probes are not an Opus/headset test; the input log is not a network replay
  protocol or authority promotion.
- Independent native HTTP/session workers deliver a 512KiB byte fixture
  beside movement, firing and synthetic-device Opus. During a temporary
  master-socket outage, both advance ninety simulation ticks with 89/87 audible
  frames. Postgame voice, actual Vista PCR construction, unanimous persistent
  results, Done, retained sign-in and requeue pass. The byte fixture deliberately
  receives no prepared-world acknowledgement. Only external Discord identity
  is simulated; this is localhost, without physical headsets or consumer NATs.
- A real nonhost UDP/signaling shutdown retains the host's running session and
  does not store an unfinished report. The requeue test now disconnects the
  actual QoS-selected host, independently of fixture account order. The shared
  eight-player local-view fixture and endpoint-closure regression also pass.

Full archives remain too large for per-tick delivery; immutable/delta reuse and
bounded live input/event catch-up remain outstanding. Automatic runtime capture,
authenticated source identity admission, retained shadow executors, an agreed
cut/epoch certificate and actual gameplay role promotion are still required.
Complete live source-audio/resource and native-owned custom-mode continuation,
physical controllers/headsets, WAN acceptance and few-frame handoff timing are
not established. This stage installs no launcher/runtime, deploys no service
and changes no Vista UI assets. Xbox TU1.5 remains the gameplay target and the
full ONLINE goal remains active.

## 2026-10-14: complete canonical world deltas over authenticated UDP

The actual runtime does not mutate its source world only inside `Match::step`.
It also runs per-frame effects, native model/view and source presentation calls;
remote observer/camera publication can run during network broadcast. A controls
log alone is consequently not a complete catch-up protocol for that runtime.
The next wire owner retains complete named-field archive images and reconstructs
their exact bytes using base-bound copy/literal deltas. It does not select a
smaller gameplay field subset or replace the original source frame consumers.

`AuthorityArchiveImage` owns shared immutable canonical bytes. The delta format
checks its source envelope, exact base extent/hash, target extent/hash, patch
checksum, compression extent, instruction count and every copy/literal range.
Base matching is checked before expanding a patch; raw/encoded/patch/instruction
budgets bound the individual inputs and output, not total process RSS. The copy
index uses bounded candidate lists and verifies every copied byte; its rolling
hash is only a search aid. Packet authentication and epoch/membership binding
still come from `PeerAuthorityTransfer`, and every reconstructed image still
needs the full codec schema checks and `SourceMatchAuthority` preparation.

The receiver must retain the sender's canonical base image. Its own locally
rebound world has different view bytes and cannot supply that base. A successor
base is retained after the owning executor validates the exact received cut;
byte receipt alone never establishes a ready shadow world. Runtime cache and
agreed cut/epoch selection are still caller responsibilities.

The qualification capsule is
`.local/player-host-continuity-20261014/authority-delta/verification.json`:

- A deterministic synthetic archive with inserts, removals, moved ranges,
  repeated/zero regions and changed bytes reconstructs identically. Its
  1,116,032-byte full archive becomes a 2,856-byte delta. Wrong bases, corrupt
  or trailing payloads, malformed checksummed instructions, invalid copy/output
  ranges, tiny cuts and exceeded budgets are rejected. Concurrent immutable
  reconstruction/encoding passes ordinary, ASan/UBSan with leak detection and
  ThreadSanitizer checks. Instrumentation covers this module and its test,
  not the full native game or third-party compression/crypto libraries.
- The actual eight-player TU1.5 Lockout fixture first restores its complete
  8,664,614-byte archive over encrypted UDP with deliberate loss and four new
  local views. Two later cuts include the actual source per-frame particle
  call outside `Match::step`; 8,685,505/8,686,707-byte full archives become
  63,327/34,715-byte encrypted delta payloads. Each resolved canonical archive
  equals the complete producer archive before local rebinding. Both prepared
  worlds acknowledge their exact transfer seal and match ten further source
  object-body/player/statistics ticks. Source identities remain separately
  trusted fixture admissions; input catch-up remains an explicit test log.
- Measured capture/encode totals were 718/732ms, encrypted delivery 115/122ms,
  and resolve/prepare/commit 1,830/1,831ms. These busy-host fixture measurements
  establish no real-time cadence or few-frame migration. Preparation must be
  completed on a retained shadow executor before promotion; bandwidth reduction
  does not by itself put that work outside the game's critical path.

Current runtime integration also needs a real authority owner: ordinary clients
currently construct no source physics world, the running loop retains its world
and host/client role at entry, and source mutations/preparation require their
retained executor thread. Pure archive work can use immutable captured values
on a worker, while source capture, workspace preparation, role switching and
view/resource consumers need explicit ownership. Do not move a live source
world to another thread or reuse a display replica as its replacement.

Automatic runtime capture/publication, an acknowledged base cache, authenticated
source admissions, warm shadow execution, cut/epoch certification and actual
promotion remain to integrate. Live source-audio/resource and native-owned
custom-mode continuation guards remain in force. No runtime/launcher was
installed, no service deployed and no Vista UI asset changed in this stage.
Xbox TU1.5 remains the gameplay target; the full ONLINE goal remains active.

## 2026-10-14: bounded background publisher through actual ONLINE sessions

`SourceAuthorityPublisher` connects a coupled TU1.5 source executor to the
existing direct `OnlineMatchSession` authority carrier. Capture remains on its
constructing executor at a quiescent boundary. One owning checkpoint and an
immutable canonical base enter a worker; encoding, compression and delta search
run there while source ticks can continue. Overlapping captures are refused.
Only an exact prepared-world receipt from the selected physical recipient
advances the delta base. Context changes and cancellation discard stale encoder
output and local readiness, without electing a host or changing membership.
Already queued peer datagrams retain the carrier's lifetime/context rules;
publisher cancellation is not revocation of an accepted authority generation.

The admission digest binds the room, host, authority epoch, agreed physical
membership and membership revision. The selected recipient is supplied by the
caller; this owner implements neither QoS selection nor a quorum certificate.
Its default pending deadline matches the carrier's five-minute maximum lifetime
so a maximum-size baseline is not rejected by a shorter default timeout. The
qualified fixture uses an explicit sixty-second deadline.

Two independent native processes use real queue admission, measured host choice
and encrypted UDP, with two Discord fixture owners and six admitted guests.
Both source worlds retain the chosen Halo names. The receiving executor uses
separately trusted private fixture admissions, prepares four local views and
checks all live source object bodies and original player rows against an
independent capture-time producer hash, except the deliberate local-index word.
It acknowledges only after that preparation. The sender continues actual source
steps during encoding, sends a full baseline followed by a smaller validated-base
delta, then cancels a third encoder job without publishing it. No unfinished
match or carnage report is stored. This exercises the real session API, not a
production source-admission protocol or automatic gameplay role transition.

Native and selected ASan/UBSan/leak checks pass. The final patched native fixture
delivers an 8,396,550-byte full cut as a 52,778-byte later delta and advances
fourteen source ticks during encoding. Its measured capture is 472,435us and
worker encoding 256,500us. Explicit process-end RTC cleanup waits for deferred
transport/crypto teardown before leak qualification. The instrumented fixture
records encoding status before a tick, so work finishing during that tick does
not falsely report no progress. These are localhost/source fixtures, not a
rendered frame-rate, real Discord, physical controller/headset or WAN proof.

Capture is still too slow for a few-frame handoff. A retained link-wrap profile
measures 393,610us in Simulation capture and 82,945us in source-world capture,
with 476,850us total; snapshot disk writes account for 71,365us. A smaller sparse
write experiment did not improve native capture time and was removed, preserving
the prior storage implementation. Complete checkpoint optimization must retain
all owners and validation rather than omit state or weaken the source barrier.

Expanding ThreadSanitizer to the wrappers and pinned RTC/ICE/SCTP stack exposed
additional findings. Its lock-order detector reports the existing native
synchronization operation gate versus a retained mutex. The gate uses try-only
export/import admission and rejects held mutexes; the diagnostic is retained
without a blanket clean-lock claim. A separate data-race run found concurrent
receive-window advisory-counter accesses in usrsctp. The repository now applies
`cmake/patches/usrsctp-freed-window-atomic.patch` idempotently during peer-stack
configuration. It atomically reads/clears and stores that counter without changing
association layout or transport settings. Patch application with unrelated edits,
idempotence and conflict rejection are checked; the encrypted transport bounds
and publisher native/ASan checks pass after the patch. Further instrumented
execution reports a socket buffer counter access through `usrsctp_get_events`;
the full transport race audit is therefore still open, with no suppression or
clean ThreadSanitizer claim.

The initial broad native target build failed on concurrent renderer-palette
work requiring an explicit `NativeRuntimeAddress` construction. That construction
has since been corrected; the current-worktree publisher and peer transport
targets now build successfully. The earlier focused publisher builds retain
the recorded native archives and rebuild the changed publisher/session/transport
units and patched peer stack. Neither qualification establishes an installed
runtime or complete rendered gameplay acceptance.

The subsequent current-worktree `source_authority_publisher_http` and
`online_peer_direct_udp` tests both pass. This fresh TU1.5 publisher run transfers
two prepared cuts, with an 8,398,478-byte full cut reduced to a 51,186-byte later
delta, thirteen source ticks during encoding, 472,071us capture and 243,192us
worker encoding. Its fixture explicitly selects the sixty-second deadline;
the public five-minute default remains distinct from this timing qualification.

Evidence, frozen inputs/archives, binaries, commands, passing checks and retained
failed diagnostics live in
`.local/player-host-continuity-20261014/authority-publisher/verification.json`.
Default runtime scheduling, authenticated source admission distribution, a warm
shadow owner for ordinary clients, complete rendered/audio/resource continuation,
live catch-up, agreed promotion and transport findings remain open. This stage
installs no runtime/launcher or backend release and changes no Vista UI assets.
Xbox TU1.5 and the full ONLINE acceptance checklist remain the active goal.


## 2026-10-14: native source admissions over encrypted peers

`PeerSourceAdmissions` replaces the publisher fixture's private identity file
with a production native-session exchange. The current TU1.5 host exports the
actual allocated player handles and original twelve-byte identities through
`SourceMatchAuthority::player_admissions`, independently of an incoming cut.
The exchange binds every ordered slot to its accepted physical owner, guest
flag and private per-room presentation key, plus the room, host epoch and
membership revision. The existing DTLS path authenticates the sender. A roster
checksum detects corruption; it is not a signature or quorum certificate.

The first accepted roster stays immutable within its context. Conflicting
replacement offers, nonhost/nonmember endpoints, stale contexts, mismatched
private keys/owners/guest flags, invalid full-handle slots, duplicate identities,
truncated and trailing data are rejected. A committed context replacement is
explicit; a local timeout never shrinks membership. Offers retry every 250ms
until their physical recipients acknowledge them. A maximum sixteen-player
roster fits in 522 bytes; acknowledgements are 89 bytes, and queues/state remain
bounded and mutex-protected. Original source identities are private peer data;
no new UI, log or master-stat field publishes them.

`OnlineMatchSession` multiplexes this exchange beside direct gameplay, voice and
authority archives. The normal native runtime exports once all accepted native
hello/profile joins have completed, both in its headless loop and its rendered
loading/game loops. Ordinary clients retain the authenticated roster without
creating a source simulation. `SourceAuthorityPublisher` refuses capture until
its selected recipient has acknowledged this metadata. That acknowledgement
never certifies a world: exact decoded/prepared-cut receipts are still required
to advance the publisher's base or readiness.

The actual two-process/eight-player TU1.5 fixture now uses that exchange. It
still uses private capture-time body hashes as an independent test oracle, but
no identity file. Since both processes share this machine's MAC, the receiver's
disposable prototype identities are deliberately changed in the fixture; the
accepted identities must come from the authenticated host. Two complete cuts
then prepare four local views, preserve complete object/player bytes, and
continue ten source ticks. The native run records 8,396,050 full bytes reduced to
a 46,856-byte later delta, 480,152us capture, 264,549us worker encoding and twelve
source ticks during encoding. Overlap, cancellation and unadmitted publication
checks also pass, with no unfinished results stored.

The isolated protocol passes normal, ASan/UBSan/leak and ThreadSanitizer checks,
including loss/duplicate/replay and concurrent publication/context changes and
a full sixteen-player/four-PC roster. The expanded selected native fixture
also passes ASan/UBSan/leaks with the admission, source exporter, session,
publisher and snapshot wrappers instrumented. Its underlying native archives
and pinned peer C stack remain release code; this does not close the separate
full transport race audit. That run records 8,387,034 full bytes reduced to 34,644,
582,867us capture, 419,916us encoding and four source ticks during encoding.

A fresh default-direct protocol regression also passes: native gameplay and
synthetic-device Opus voice continue through a temporary master-socket outage,
advancing 89 ticks with 89/87 audible frames on the two peers. Archive bytes coexist
with that traffic without an unvalidated world acknowledgement; source results
are confirmed/persisted after the match, Vista PCR Done retains sign-in, and
requeue succeeds. This lightweight native Match/media fixture is separate from
the actual TU1.5 complete-source-world test and from physical headset/WAN proof.

The current native rendered couch target builds. In the actual GPU run, the
receiving client confirms all eight identities exported by the live host,
without the fixture's explicit export orchestration. The overall run fails:
the host reports `source shield vertex lost its authored part`; the receiver
then fails its finite-capture snapshot check after the host disconnects. This
is retained as failed rendered acceptance, not a full four-view gameplay pass.
No renderer guard was removed and no Vista assets were changed.

Evidence, frozen compiler inputs/archives, binaries, commands and failed
rendered diagnostics are recorded under
`.local/player-host-continuity-20261014/source-admissions/verification.json`.
Initial source admission distribution is now integrated. Warm source owners
on ordinary clients, automatic full-world capture/catch-up, identity continuity
across certified membership/host changes, agreed cut/epoch promotion, complete
rendered/audio/resource continuation, native-owned custom modes, transport race
findings and physical/WAN validation remain open. No runtime/launcher was
installed or backend release deployed. Xbox TU1.5, high-resolution Vista UI and
the full ONLINE goal remain the required final state.


## 2026-10-14: client-owned complete source replacement world

`MultiplayerPreparedWorld::source_shadow` now uses the actual native world
initializer to construct a private TU1.5 executor for an accepted direct ONLINE
roster. It owns separate cache/files, original physics, source player allocations,
selected local profiles and the complete first-person definition bank. Candidate
views belong to the receiving PC; disposable staging views belong to the incoming
source host. A shadow prototype fails ordinary `matches()` admission, so it cannot
silently become the visible gameplay world. Construction alone grants neither
world validation nor hosting authority.

`SourceAuthorityReceiver` owns this candidate. One bounded worker job decodes an
owning complete archive or exact-base delta; native construction, preparation
and publication stay on the constructing executor. The sender/context/tick,
complete accepted slot/name roster and local selected configurations are checked.
The extended `SourceMatchAuthority::prepare_with_local_views` then validates every
allocated source identity against the independently accepted full roster, including
remote players, before changing the candidate. Complete native world/Match
preparation and commit precede the exact executor receipt. The canonical producer
image remains the delta base; the locally rebound world is never used as that base.

A context change or cancellation discards queued/stale worker output. A cancelled
receiver stays stopped. Host connection loss alone preserves an already prepared
candidate. `take_world` transfers only the exact held context/seal to an external
continuation owner, clearing local readiness; it does not elect a host, change
membership, grant an authority epoch, rebind transport, catch up inputs or attach
controllers/audio/renderers. Commit/receipt failure clears candidate readiness,
and a possibly changed candidate is discarded rather than labeled as its prior cut.

The native client loop now pumps this receiver for source-backed TU1.5 sessions.
It remains lightweight until a cut arrives. Automatic host capture/publication and
the controller that retains/takes this world before runtime teardown are not yet
integrated. The current runtime still leaves on host connection failure; only the
component test explicitly keeps/takes its candidate across that failure.

The actual two-process test uses normal native prepared-world initialization,
real grouped `LanHost`/`LanClient` profile admission, measured host selection,
independent source admissions and encrypted peer transfer. The recipient's
prototype identities deliberately differ. A well-formed/checksummed cut with a
forged NONLOCAL identity is rejected before candidate mutation or a world receipt.
A full cut and subsequent delta then preserve all live source object bodies and
original player rows, except the deliberate local-index word, with four rebound
views. An additional owning decoder job is cancelled in flight; its stale result
never creates a world or acknowledgement. After the host process closes its real
transport, the exact prepared candidate remains available. The test transfers it,
applies the original departure of the host's four-player group without invented
numeric deaths or released player identities, and continues ten source ticks
with four surviving players and no match end. It does not perform a certified
network role switch or prove communication between two remote survivors.

The qualified native run reduces an 8,395,013-byte full cut to a 34,277-byte later
delta. Decode takes 250,389/283,924us; cold candidate construction takes 742,183us;
preparation takes 2,273,950/2,385,948us including the disposable executor. This is
not a few-frame handoff. Cold construction and native preparation still block
this executor and need loading-time scheduling or further proven optimization
before regular live publication can be enabled. The unchanged original local-view
rebind test also passes. Frozen qualification and additional sanitizer results
are recorded in
`.local/player-host-continuity-20261014/source-receiver/verification.json`.

A subsequent fresh protocol-81 build and native run pass the same complete
receiver, cancellation, actual host-loss, host-group departure and continuation
checks. That run records 8,394,193 full bytes reduced to 31,510 delta bytes,
380,727/272,203us decode, 994,721us construction and 2,648,061/3,338,934us
preparation. The actual gameplay-datagram wrapper now rejects mixed library/
executor protocol versions immediately, before a misleading admission timeout.

The expanded ASan/UBSan/leak attempt is NOT a complete pass. It instruments the
receiver, factory, binding, codec, delta, source authority, publisher, admission,
session, transport, snapshot storage and test units. Concurrent work changed
packet protocol/state layouts from80 to81 while its preserved native archives
remained80. The diagnostic wrapper proves the mismatch. Private frozen headers
were then restored to their verified protocol80 layouts without reverting the
shared checkout. The aligned run rejects the forged identity and prepares its
first actual cut, but instrumented preparation takes6,979,818us, beyond the
native peer activity window. Host validation then times out. This records a
real loading/scheduling/performance limit; production timeouts were not raised
and routine mid-match publication remains unenabled. Full sanitizer completion
for this receiver stage therefore remains open.

The current rendered client target builds with the receive owner. No Vista UI
asset or renderer guard was changed. The earlier source-shield authored-part
failure remains an open rendered gate; this headless native test does not close
it. Full transport ThreadSanitizer findings, live source-audio/resource roles,
native-owned custom modes, input/call catch-up, certified authority/membership
changes, actual gameplay/voice/profile rebinding, independent NATs/controllers/
headsets and delivery remain open. No runtime/launcher was installed and no backend
release was deployed. Xbox TU1.5 and the full ONLINE goal remain the target.


### TU1.5 melee-resource lifetime and preparation profile (2026-10-15)

The private `binding-performance/verification.json` capsule under the
2026-10-14 continuity directory records a reproducible failing resource-owner
regression, its passing fix, frozen source/native archives, exact compiler
commands and the subsequent native checks. No original game data was packaged.
The root core remains C++20 and Xbox TU1.5 remains the sole parity target.

Repeated melee bindings reconstructed and copied the same original shared-file
snapshots on every capture/preparation. NativeMeleeAnimation now retains its
immutable SourceRuntimeResources owner. Replacement melee owners share those
exact bytes; first-person and body action decoding also read through that owner.
The canonical binding field order is unchanged. Actual cache regression checks
compare the digest to the independent fresh-resource binding, retain authored
strike events/timing/RNG after fixture file replacement/unlink, reject changed
movement/digests, and ensure a new resource admission sees different bytes.
Animation graph selection uses the existing object+38 -> hlmt+14 operand,
compared against full model import; it no longer loads unrelated mesh streams.
Existing cache entry points retain their ordinary resource-reading overloads.

The eight-player focused handoff fixture falls from1.71--2.03s to0.375--0.421s
preparation, preserving complete object/player bodies. Loading the complete
local/replicated weapon definition bank in the same fixture gives0.573s; this
still exceeds the100ms diagnostic budget and those profiling runs return2.
The actual production receiver test passes full/delta reception, full-roster
forgery rejection, decoder cancellation, real host transport loss, exact-cut
ownership transfer, the original four-player departure and ten surviving source
ticks. It still takes2.365/2.671s to prepare, including disposable workspace
construction. The native local-rebind, first-person-registration and actual
melee/sword gameplay checks also pass. These are bounded CPU/localhost checks.

The expanded selected sanitizer run retains the aligned protocol80 native
archives and instruments the six changed gameplay/cache components in addition
to the previous thirteen receiver components. It prepares one real cut but
still takes6.514s and exceeds native peer liveness. It does not qualify a clean
ASan/UBSan/leak run. Production timeouts and original validation guards were
not weakened. Profiling is retained privately; no debug logs remain in product
source. No runtime/launcher was installed or backend deployed. Per-cut workspace
construction, scheduling, catch-up, certified promotion and all earlier hardware,
rendered/voice/WAN and delivery gates remain open; the full goal remains active.


### Reusable TU1.5 source workspace and cold-load boundaries (2026-10-15)

The private `.local/player-host-continuity-20261015/workspace-reuse` capsule
records the native regression that failed because the receiver rebuilt its
private executor between a rejected cut and the following valid cut. The
receiver now retains one workspace per authority context. The reusable
SourceMatchAuthority overload selects the producer's independently admitted
local views before the ordinary full-world preparation. It does not run source
calls against the previous recipient-mapped bank or remove any identity, schema,
configuration, world, lifetime or view check. Changed workspace metadata stays
unqualified for export until a complete import commits. A quiescent executor
guard checks thread affinity before changing that selector.

The native local-rebind check covers wrong-thread rejection without bank or
configuration changes, alias/duplicate-view rejection, an invalid-version import
that cannot be exported, subsequent full repair, repeated cuts through the same
workspace, forty equal future source ticks and repeated pending host-player
departure. Complete bodies/player bytes and the original local-index exception
remain the existing comparison oracle.

Warm construction creates at most one executor per public call. The two calls
have a native input/network pump between them. Ordinary identity metadata does
not construct either executor; automatic construction starts only after a
successfully decoded cut. A second regression first failed on metadata-only cold
construction and now passes. Cold initialization remains1.3--4.2s under this
machine's varying load, so this is not a few-frame/live-scheduling qualification.

The native protocol82 final run passes `source_match_local_rebind_tu15`,
`source_authority_receiver_http` and `source_authority_receiver_cold_http`.
The two receiver paths each use one workspace for a forged nonlocal-identity
cut and the following full/delta cuts. Full admitted identities, real native
four-seat profile admission, complete body comparisons, owning decoder
cancellation, actual host DTLS/session loss, exact-cut owner transfer, original
four-player departure and ten surviving source ticks remain covered. Latest
warm preparation is461,572/466,053us; cold-from-cut preparation is509,030/636,605us.
These are separate preparation times, not cold-load or rendered frame times.

The selected sanitizer capsules retain the matched protocol80 layouts and
native archives, with exact patches/compiler/link commands. Receiver/rebind,
codec/delta, native-session/admission and changed cache/melee components are
instrumented; the large native object wrapper, physics/RTC and other retained
libraries remain at their stated native tier. ASan/UBSan with leak checking now
complete both explicitly warmed and initially cold paths without a diagnostic.
Latest instrumented warm preparation is1.116/1.091s; cold preparation is1.847/2.178s.
This closes the selected receiver timeout repro, not the full transport/native
sanitizer audit or its earlier ThreadSanitizer findings.

The current `halo2_native` target builds. A missing concrete unit-interaction
header in the concurrent runtime preload code was added; no guard or gameplay
operand was changed for that compile fix. No runtime/launcher was installed,
backend deployed or UI art changed. The earlier rendered source-shield guard
remains open. The runtime still needs automatic authoritative publication,
complete source input/call catch-up, agreed cut/epoch certification, role/media/
hardware rebinding and retention before disconnect teardown. These CPU/localhost
checks do not prove multi-survivor election, WAN/headset/controller behavior or
live gameplay migration. Xbox TU1.5 and the full ONLINE goal remain active.


### Catch-up seam audit after workspace qualification (2026-10-15)

The next stage cannot infer live continuation from replaying Controls alone.
`physics_live_world_tu15.cpp` consumes frame milliseconds, timestamp-counter and
reciprocal-square-root inputs in its services, plus native UTC at its admitted
system-time import. Its FPU factory configuration is constant, not an omitted
mutable between-call register bank. `MultiplayerMatch::result_wall_clock_` is a
separate result/event input, defaulting to std::time. The authoritative runtime
also advances effect particles, observer positions, visibility, weather and
source model builders outside Match::step. The existing entry diagnostic wraps
run_actual, while run dispatches directly, so it is not a complete call journal.
These source operands/order and typed event boundaries must be covered by actual
replay evidence before an old cut is promoted as a caught-up world.

Moving an existing private world to a worker is also not a pointer-only change:
the wrapper and TU1.5 world retain construction-thread owners, the native sound
SDK separately requires its owned passive thread, and phantom diagnostics have
thread-local registrations. Those owners need an explicit quiescent transfer
proof or a different fully qualified scheduling boundary; guards remain intact.
The older OnlineMatchSession migration acceptance/rebind methods queue master
world controls. They do not implement decentralized certification or prove a
runtime endpoint/voice switch. The private continuation-input-audit capsule pins
the eleven source files behind these findings. No input-only replay, worker
handoff or host promotion was implemented or inferred by this audit.


### Ordered external inputs for TU1.5 source replay (2026-10-15)

The private `.local/player-host-continuity-20261015/execution-inputs` capsule
retains an actual eight-player failing baseline: Controls-only continuation
sampled the replacement PC's result wall clock and produced result start 1 versus
906. This is an external execution input, not a missing native memory region.
The source audit separately identifies milliseconds, effective timestamp
counter, UTC FILETIME and CPU reciprocal-square-root estimates. Original
source function returns and the native Newton steps remain untouched.

`SourceExecutionInputs` now captures those five kinds in exact consumption
order. Estimate operands bind their raw input float bits. The replay supplies
the host's recorded external values without sampling the replacement PC's
providers. A tape belongs to one executor and one exact native memory owner;
its owning replay copy validates kinds, operands, widths and entry bounds.
Reordered, missing, differently bound or leftover reads reject the phase.
The closed values are not yet a peer wire format, a complete frame journal or
a promotion certificate.

Capture overflow/allocation failure invalidates publication while continuing
to return the host's actual external inputs. A separate failing exception repro
showed that an original clock-provider error could leave a publishable tape;
that error now invalidates the tape and propagates unchanged. The ordinary
unbound provider path remains the original source/CPU/clock policy. No mutable
FPU register state, CPU estimate parity with physical Xbox, or replacement
source gameplay return is inferred by this work.

The native fixture imports an actual complete TU1.5 Lockout cut, then executes
eight control/effect phases on the same admitted producer views with deliberately
different PC clocks and result clocks. Its 10,053 ordered inputs preserve every
native bank region/topology and the existing body/player/inventory/statistics
oracle, plus complete result-event start/last/index/events. It also exercises
actual host continuation after capture overflow, a mismatched native read that
faults the world, export rejection, and repair through a complete import.
The focused checks cover all five value types, provider exceptions, ownership,
width/order/operand rejection and complete consumption.

A frozen selected ASan/UBSan run with leak checking passes the focused and
actual native fixtures after the provider-exception repair. The tape, native
world wrapper, Match and both fixtures are instrumented; physics/generated
source, transport and other retained libraries are not fully instrumented.
The capsule retains exact sources/headers, archives, commands, diagnostics and
the earlier missing-def build failure. This is selected sanitizer evidence,
not closure of the full native/RTC sanitizer audit.

Clock anchors at the end of a replayed frame still require explicit validation:
checkpoint logical time and later untaped result time must continue in the
producer's domain. Render/observer/visibility/weather/model-builder source calls
also need complete typed order and operands. The test retains producer views;
it does not prove replay on already recipient-rebound views. No automatic
publisher, catch-up delivery, certified host promotion, endpoint/media switch,
worker transfer, runtime retention before disconnect teardown, hardware/WAN
acceptance, installed runtime/launcher or backend deployment is claimed. Xbox
TU1.5 and high-resolution Vista ONLINE/clan UI requirements remain unchanged;
the full ONLINE goal stays active.

The final native build passes. All five selected CTests pass: execution-input
validation, native divergent-clock replay, native local-view rebinding, explicitly
warmed reception and cold-from-cut reception. Warm preparation is 400,938/403,825us;
cold-from-cut preparation is 446,854/483,901us. These retain the existing actual
host-transport-loss/source-departure continuation check; they do not perform
certified runtime promotion. The private diagnostic serializes each captured
entry to 13 bytes without C++ padding: 130,689 bytes over eight phases compress
to 4,994 bytes with independent zlib6 frames. This is a compressibility observation,
not an implemented/authenticated peer codec, a rendered workload, or a measured
network/frame-time budget. Instrumented actual capture/replay phases still cost
about 0.23--0.49s, so scheduling/performance acceptance remains open.

The next seam has a retained actual failing native repro, not only an audit:
after the eight exactly replayed banks, the exported host logical clock is 1,264ms
while the replacement exports 49,264ms. `anchor-repro-result.json` records its
expected failure. This must be repaired with explicit producer frame-end clock
anchors before untaped continuation or certified promotion; bank equality alone
does not satisfy it. The repro retains the complete same-producer-view context
and selected instrumentation described above. No host promotion is enabled by
these results.


### Explicit replay clock domains and active-session departure owner (2026-10-15)

The `.local/player-host-continuity-20261015/execution-clock-anchors` capsule
retains the failing actual native loop: the producer exported 1,264ms after the
eight replayed phases, while the replacement exported 49,264ms despite equal
native banks. `SourceMatchAuthority` now captures an owning execution phase
with its ordered external reads and explicit end-of-phase milliseconds, cycles,
UTC FILETIME and result seconds. Replay consumes every read before anchoring
those external domains. It does not infer an anchor from the last read, which
may be absent for a particular kind. Subsequent unrecorded calls use local
providers with the anchored offsets. Native clock/RNG bytes and source gameplay
returns are unchanged.

The coupled owner, Match and physics checkpoint components are now version 2;
new checkpoints include result-time and UTC anchors. The native peer protocol
is 83 so an older participant does not silently exchange this schema. These
are native transport/checkpoint versions, not Xbox title-update versions;
Xbox TU1.5 remains the only gameplay/network reference. The archive envelope
format remains its existing named-record container; component/schema admission
rejects the old values. A fresh schema 2 cross-compiler exchange remains a
separate delivery gate from the earlier schema 1 evidence.

Capture/replay requires the actual quiescent executor, complete source and
bridge owners, and no outstanding sound/DSP borrower. An active phase cannot
capture or prepare a coupled checkpoint. Capture overflow/allocation failure
rejects the recording without poisoning the actual host; failed binding
allocation executes its real phase once without a recording. A bad, missing or
leftover replay read, failed native call or unavailable end-clock leaves the
candidate non-exportable until a complete import repairs it. Malformed tape
structure is rejected before changing that candidate. The original result-clock
provider is sampled before either bank publishes during a coupled restoration;
a provider failure retains the live bank. The commit checks executor identity
before invoking that provider and keeps the existing staged/public-call guards.

The native fixture retains eight admitted players and the producer's four local
views. It executes eight control/effect phases with 10,053 ordered inputs, different
physical millisecond/cycle/UTC/result clocks, then forty ordinary untaped future
ticks. It compares complete native bank/topology, body/player/inventory/statistics
state, full result-event headers/ring and all four exported clock domains. It
also covers wraparound, recording overflow survival, wrong-thread rejection,
active-phase export/import rejection, native mismatch/leftover qualification,
provider failure before publication and complete repair. The independent scripted
wall-clock fixtures replace callbacks derived from restored simulation state;
restoring a Match must not move an external provider. Future provider deltas are
controlled test inputs, not a physical-clock rate/calibration qualification.
A read-sensitive fake counter also requires symmetric observations when a
transaction deliberately fails without rebasing either live world.

The wider departure checks exposed the actual 1559F8 call to 6A9A0 when the
original active-session branch is taken. The retained earlier local-rebind
binary still passes; its bounded evidence did not qualify this missing owner in
the current runtime. The authenticated TU1.5 executable establishes the complete
50-instruction session-player row cleanup and 29-instruction 6A890 iterator.
Both are now lowered through the existing original-instruction generator. No
session flag, original identity check or missing-owner guard was disabled, and
no service return is substituted.

A new oracle executes those untouched routines in Unicorn with declared session,
typed-row and caller-stack inputs. Four cases cover player slots 0/4/15, an empty
row table, admitted/excluded types, disconnected/inactive rows, and set/clear
membership bits. Generated C++ matches the exact instruction traces, register
outputs, defined flags and complete retained session/row/stack memory. The fixture
first fails against the retained unowned library, then passes with the original
owner. The two routines and fixture also pass selected ASan/UBSan/leak checking.
The clock phase/World/Match/coupled owner and native fixture have a separate frozen
selected sanitizer pass; other physics/generated/RTC/native libraries retain
their stated uninstrumented tier. This does not close the full transport/TSan audit.

This phase API still receives its call order and operands from the caller. A
complete typed rendered/source call journal, authenticated catch-up delivery,
post-replay agreed world/cut certification, automatic runtime publication,
retention before disconnect teardown, role/endpoint/hardware/profile/voice
rebinding, scheduling/performance, actual multi-survivor promotion and physical/
WAN acceptance remain open. No runtime/launcher installation, UI-art change or
backend deployment is claimed. High-resolution Vista ONLINE/clan UI and the full
ONLINE completion goal remain active.

The final native build and all twelve selected CTests pass, including the active
original departure branch, local-view restoration, full/delta codec, explicitly
warmed/cold reception, actual host transport loss and ten surviving source ticks.
Latest warm preparation is 445,647/407,299us; cold-from-cut preparation
is 422,059/420,900us. These remain CPU preparation times, not cold construction,
rendered frame times or certified migration latency. The repaired eight-phase
loop exports 1,264ms on both owners and passes the selected sanitizers; its retained
pre-fix loop exports 1,264/49,264ms and fails. Initial mixed-build linker failures,
read-sensitive counter and state-derived fixture failures, the four unowned
session-departure failures, and the final passes remain in the capsule. No guard
or timeout was weakened to obtain these results.


### Owning native root-call journal and closure-free replay (2026-10-15)

The `.local/player-host-continuity-20261015/execution-call-journal` capsule
retains a failing actual native replay: the replacement omitted the producer's
observer-position publication, but the external-input tape still consumed every
read and accepted the phase. The observer's native bank writes were not described
by Controls or the clock/CPU transcript. This was a real qualification gap, not
an inferred complete-frame proof.

Execution phases now own a bounded ordered root-call journal. Fourteen operations
cover Match stepping, effect particles, model frame/view starts, observer position,
complete visibility banks, weather, world/first-person models, audio advancement,
sound/prop/grenade event drains and actual original player departure. Canonical
arguments preserve float bits, optional input targets and explicit native padding
fields, without host pointers or callbacks. Portal sequence limits match the
existing native owner. Decoding rejects malformed/trailing data and oversized
sequences before executing an owning journal. Each root also binds the exact
begin/end of its external input consumption, preventing reads from sliding into
the next operation. Nested source calls execute under their actual root, rather
than being executed twice.

The new replay overload dispatches the owning journal through the actual native
APIs without importing a caller closure or replacing an original function return.
The retained closure overload now verifies the same kind/order/arguments and
input-consumption boundaries. Omitted, changed, reordered or incomplete calls
leave a replay candidate unqualified until a complete import repairs it. Static
malformed arguments are rejected before mutating that candidate. Frame-end clock
anchoring remains after complete call and operand consumption.

Registered write/topology observers detect native work outside a supported root.
Topology observation is separate from platform access/write policies; no existing
resource guard is replaced. Unknown native public mutations/entries and unaccounted
Match/Simulation mutations reject publication. Capture failure still lets the
real host finish; replay guards reject before native writes where the boundary
permits, and failed phases cannot export. The existing no-throw backing swap
retains its publication ABI while latching an invalid journal. These guards cover
normal native API, memory and mutation-counter paths, not arbitrary uninstrumented
C++ writes through references retained in violation of the quiescent-owner contract.

The new native test starts from a complete eight-player TU1.5 cut on the producer's
four admitted views. It rejects the actual omitted-observer repro, changed position,
and reordered view work, then replays two native four-view model/first-person,
visibility, weather, effects, audio and event-drain phases from their owning calls.
Complete canonical coupled archives agree afterward. It covers unsupported direct
source work and C++ mutations, complete repair, malformed journal rejection and
original departure followed by a source tick. Focused tests cover bit-preserving
arguments, nested-root suppression, raw writes/topology rejection, foreign-executor
writes, allocator failure isolation and external-input call boundaries.

The final native build and nine selected CTests pass, retaining local-view rebind,
original session departure, full/delta codec and warm/cold receiver/host-loss
continuation. Eleven frozen translation units are instrumented for selected
ASan/UBSan/leak checking; the focused, native clock and native multiview journal
fixtures all pass. Other retained bridge/physics/RTC libraries remain at their
stated native tier; this does not close the full transport/TSan audit. Exact
sources, headers, compiler/link commands, archives, original failing loop and
native binaries are retained privately. No product debug logs were added.

This is not yet complete rendered-frame capture. Current live shadow selection
and root construction use prepare_source_call/run_actual with raw caller scratch
and full ABI inputs; those boundaries still invalidate a recording. Their source
files and runtime callers are pinned in the capsule for the next stage. Other
uncovered mutations must gain actual owning operations rather than bypassing the
coverage guards. Automatic full-frame recording/publication in the real runtime,
authenticated journal transport, producer-view catch-up in the receiver, agreed
post-replay world/cut certification, role/endpoint/hardware/profile/voice switching,
scheduling/performance and physical/WAN validation remain open. No installation,
backend deployment or UI-art change is claimed. The full ONLINE goal, sole Xbox
TU1.5 gameplay/network target and Vista ONLINE/clan UI requirements remain active.

## Explicit native caller input and ABI/result replay (2026-10-15)

Capsule: `.local/player-host-continuity-20261015/execution-abi-inputs/`.
This supersedes the preceding caller-scratch/full-ABI limitation, while retaining
the open complete-rendered-frame and live migration acceptance requirements.

The retained original failing loop executes a real `prepare_source_call` and
TU1.5 `335B0` entry. The previous journal rejects capture as unrecorded work.
The same source loop now passes under selected ASan/UBSan/leak checks.

Six additional owning operations bring the native root journal to twenty:
explicit caller input writes, native argument-frame preparation, canonical and
actual TU1.5 full-ABI runs, and canonical/actual argument calls. Correspondence
IDs still select the authenticated TU1.5 executor; no alternate title version
is introduced. The shadow-selection and shadow-root callers now publish their
existing inputs through the explicit write operation, including camera/queue
inputs and stack scratch. Original caller addresses, ordering and uncleared
scratch bytes remain intact. Arbitrary writes/topology changes still invalidate
a recording; existing resource access guards are retained.

Full ABI arguments/results retain GP registers, XMM bit patterns, flags, source
FS token, live x87 depth/status/80-bit lanes, optional FP controls, and known
physical MMX aliases. Dead lanes, unknown MMX bytes and host long-double padding
are omitted. Unsupported live extended representations, invalid x87/MMX states,
unmasked FP controls and reserved MXCSR high bits reject before replay reaches
the native executor. Original argument frames retain their 4096-word bound.
The existing four-MiB phase budget now includes argument and result bytes.

A result-bearing root must complete its actual native work and seal its return.
Replay compares that real ABI/value with the journal, never imports a stored
function return. Missing, changed or incomplete returns invalidate qualification.
Malformed results/arguments are validated before faulting/mutating a candidate.
Recording/allocation failure still permits the real host to finish; a failed
replay remains unexportable until repaired by a complete coupled import.

The native ABI fixture records two actual source-tick/four-view phases with
real argument/scratch inputs, native original entries, live high-precision x87,
negative zero, XMM NaN/subnormal bits and active known MMX aliases. It exercises
the shadow-selection caller with declared empty main queues and the eligibility
caller on all four actual player units, following the original four-local-player
disabled-shadow route. It replays from owning values with no caller closure and
compares complete canonical coupled archives. Changed ABI and caller bytes are
rejected; the changed caller bytes never reach the bank. A corrupted expected
return still executes the original entry and exposes its actual return before
rejection. Unrecorded writes continue on the real host, fail qualification, and
repair through a complete cut before a subsequent recorded source tick.

The final native runtime build passes. Ten selected native/original CTests pass,
including original session departure, local-view rebind, ordinary departure,
warm/cold receiver continuation after actual transport loss, codec, clock,
multiview-call and new ABI cases. Selected instrumentation covers ten production
translation units and four test translation units; focused, native clock,
multiview and ABI fixtures plus the original failing loop all pass with
ASan/UBSan/leak checks. Other retained native/physics/RTC libraries remain
uninstrumented at this tier. Full transport TSan, hardware and WAN acceptance
are not implied. Compiler commands, source/header cuts, archives, native
binaries, red/green loops and results are retained; no product debug logs added.
A shared renderer compile error occurred during final qualification and was
already corrected in the shared checkout before the successful rebuild; this
milestone did not edit that renderer file.

Complete rendered-frame recording still needs the actual live model caller's
transient collector allocations, build/presentation/resource operations and any
other uncovered mutation owners. Automatic real-runtime recording/publication,
authenticated bounded journal transport, replay on producer views before local
recipient rebinding, agreed post-replay cut/epoch certification, endpoint/role
promotion, hardware/profile/voice switching, scheduling/performance and physical
WAN/gameplay verification remain open. Journals are not yet transmitted, so
native protocol83 and checkpoint component schema2 are unchanged here. No
installation, backend deployment, UI-art change or seamless live migration is
claimed. The full ONLINE goal and Vista ONLINE/clan UI requirements remain active.

## Owning live model collector and observer isolation (2026-10-15)

Capsule: `.local/player-host-continuity-20261015/execution-live-model-calls/`.
This supersedes the preceding transient live-model collector limitation. The
full ONLINE objective and complete rendered-frame/live migration gates remain.

The retained red loop calls the actual live-model collector on the real TU1.5
world. The earlier journal rejects its original temporary 1 MiB mount/release at
64000000 as unrecorded native topology. That exact loop now passes with selected
ASan/UBSan/leak checks; no original call/return or allocation is substituted.

A twenty-first owning operation executes `source_live_model_draws_tu15` from
recorded admission, complete retained draw banks and actual view inputs. The
native caller still owns its temporary collector memory, queue ordering, model
builder, presentation/child recursion, resource requests and context writes.
Nested native work stays within that actual root and is not executed twice.
The callback-free input type explicitly contains the seven original caller
value fields; a field-count check requires a new Context field to receive
serialization review. Arrays retain tail/unused words, float bits and source
address tokens; no C++ callbacks, borrowed spans, host pointers or padding travel.
Native object/admission, association, draw, override, section and arena extents
are bounded before dispatch. Structurally invalid/trailing payloads reject before mutating
or faulting a candidate. The existing phase/call byte bounds remain enforced.

Actual post-call admission/draw-bank values are canonically serialized and
sealed with SHA-256, including retained unused bank words. Replay recomputes
that 32-byte result after the original collector completes and compares it with
the producer; it does not restore a recorded callee output. Result digests avoid
sending a second complete copy of the derived draw banks per root. Input banks
still retain complete owning values: compression, scheduling, bandwidth and
realtime full-frame performance are not qualified by this component test.

The `built` callback stays local to presentation. A scoped observer guard allows
reads/copies, but invalidates capture on source writes/topology, new native calls,
external operands, or Match/Simulation mutation-counter changes. Replay rejects
source mutation before the offending API/write where its boundary permits;
unknown C++ counter work invalidates at observer completion. A capture failure
continues the actual host collection and cannot publish that transcript. Existing
memory policies and executor guards remain. This covers declared APIs/guards,
not unsignalled C++ writes through references retained against the owner contract.
Other callback ownership and remaining frame paths still need their own evidence.

The new native test uses real eight-player source objects and native admission
with declared all-cluster cameras. Two source-tick/four-local-view frames each
run actual main and secondary collection, preserving the main prefix and
retained banks. Sixteen roots produce 262 collectors and 434 draw units; fixed
journal replay without the producer callback preserves complete canonical
coupled archives. Read-only observer access works. Changed output digest,
malformed payload and a mutating observer are rejected; a failed candidate
repairs from a complete cut. A mutating capture observer still finishes the
real host's collection, rejects publication, and permits a later recorded
source tick after complete import. These declared camera/CPU cases do not
prove original portal traversal, GPU pixels, native Windows or physical couches.

The native runtime build and eleven selected CTests pass, including clock,
multiview, full-ABI, original session departure, local rebind, ordinary departure,
warm/cold receiver/transport-loss continuation and codec coverage. Selected
instrumentation covers eleven production and five test translation units;
focused, native clock/multiview/ABI/live-model and red-to-green cases all pass
ASan/UBSan/leak checking. Other retained native/physics/RTC code remains at the
uninstrumented tier; full transport TSan is open. Exact source/header cuts,
commands, archives, native binaries, failing loop and results are retained
privately. No product debug logs, installation, deployment or UI-art changes.

Remaining frame work includes attachment command generation and render resource
requests outside this root, particle callers' raw scratch publication, and other
uncovered native mutations. Automatic real-runtime recording/publication,
authenticated bounded/compressed catch-up transport, producer-view replay before
recipient rebinding, agreed post-replay certification, endpoint/role promotion,
profile/hardware/voice handoff and physical/WAN validation remain open. Internal
journal operations are not yet transmitted; native protocol83 and checkpoint
component schema2 are unchanged by this milestone. Seamless live migration and
the full ONLINE goal are not complete; the sole Xbox TU1.5 reference and Vista
ONLINE/clan UI requirements remain in force.

## Native command/resource and real-world particle replay (2026-10-15)

Capsule: `.local/player-host-continuity-20261015/execution-render-commands/`.
The full ONLINE goal remains active; these are native replay-owner boundaries,
not endpoint promotion, complete renderer acceptance or a deployed feature.

The retained red loop imports the actual native command capacities/storage and
calls original attachment traversal. Its temporary 64100000 caller mount fails
qualification under the preceding journal. The exact loop is now green. Two
additional roots bring the journal to 23 operations: attachment commands and
render-geometry requests. Typed attachment input owns all three command banks,
retained command bytes/header words, special units/count, main counts, diagnostic
entry trace, admission and mode. Its trace has a separate 65536-entry bound from
the portal collector's 128-element index vector. Storage/capacity/count/admission
and mode bounds are checked before dispatch; actual native capacity/identity
checks remain in the original owner. Full caller storage, including the last
two source-retained command bytes, is preserved. SHA-256 seals the actual
post-call values. Geometry calls retain the actual flags/header and verify the
original boolean return, never replace a source result.

The original particle caller now sends its existing count/camera/view/reset
scratch values through `write_source_call_input`. Existing addresses, ordering,
ABI controls, native queue producers and simulation remain unchanged. Material
preload/presentation resources and GPU output are not imported as host pointers.
A source clock may now be explicitly supplied to ordinary prepared-world
construction, using the existing native clock owner. In this new constructor
path its FILETIME UTC also supplies Match's separate 32-bit Unix/PCR input.
Default ordinary and existing shadow construction retain their platform result
clock behavior. Clock anchoring remains explicit at each owning replay phase.

A real prepared-world replay initially failed before any particle work. Named
archive comparison found one differing field: `match.logical_result_seconds`,
advanced by two real seconds during import; all native bank regions agreed.
The new injected constructor had pinned native clocks but left the result
provider on real wall time. Completing its UTC-to-Unix input ownership fixes the
actual boundary without normalizing comparison fields or weakening the complete
archive check. The fixture advances injected UTC and asserts the final result
time comes from that source. Private red captures and diagnostic comparison
remain in the capsule; no product debug logs were added.

The native command test uses eight actual players/four local views over two
source frames. Sixteen original light/shadow attachment traversals and eight
resident geometry requests preserve retained command bytes and complete
canonical coupled worlds; changed return digest/boolean and malformed payloads
reject qualification and repair through a complete cut. The particle test uses
the actual normal prepared-world constructor on owned Backwash data, original
portal traversal and two distinct scripted native clocks. Across 240 real
match/render frames in ten replay phases it records 11,041 calls and emits 14 peak
and final particles. Original observer, visibility, queue, RNG, timer and PCR
state replay into equal complete canonical coupled archives. These are CPU/
native cases, not GPU/pixel, native Windows, WAN or physical headset/controller
acceptance.

The final native runtime rebuild passes. Native qualification covers thirteen selected CTest cases plus the original
standalone 240-frame swarm/repeated-view/RNG test. The CTest wrapper for that
legacy case skipped because its configured XBE variable was unavailable; direct
execution with the owned explicit XBE/map/shared-map paths passes. The initial
ABI case failed after concurrent shadow work added a main-viewport owner; it
now executes the actual main-input 2C470/1BE90 caller before selection and passes,
retaining those guards. A missing header include was corrected in the shared
BSP effect-light caller; reconfiguration admitted a concurrently added ordered
shadow source before the repaired ABI link. The combined qualification target
builds runtime/tests together, avoiding per-target recursive dependency churn.

Selected ASan/UBSan/leak qualification covers thirteen production and seven
test translation units, including the new prepared-world/particle paths. Eight
selected fixture/red-to-green runs pass, and the final explicit UTC-result-time
assertion also passes. That frozen ASan cut precedes the concurrent main-shadow
caller requirement; the updated caller fixture has separate current native
qualification. Other native/physics/RTC code remains uninstrumented at this
tier; transport TSan is open. Exact cuts, commands, dependency archives, native
binaries, failing loops and results are retained privately. No installation,
backend deployment, UI-art or live migration claim is made.

Remaining work includes complete frame coverage of new shared shadow storage,
ordered-draw/main-context and any other raw caller mutations; automatic actual
runtime recording/publication; bounded/compressed authenticated catch-up;
producer-view replay before recipient rebinding; agreed post-replay world/epoch
certification; endpoint/role promotion; hardware/profile/voice handoff;
scheduling/bandwidth and physical/WAN checks. Complete typed command input can
be sizable; the existing four-MiB phase bound and realtime bandwidth/performance
remain acceptance constraints, not proof of full-frame coverage. Internal root
operations are not yet peer-transmitted; protocol 83 and component schema 2 remain
unchanged. Sole Xbox TU1.5 authority and Vista ONLINE/clan UI selection persist.


## Write admission and compressed native phase format (2026-10-15)

Capsule: `.local/player-host-continuity-20261015/execution-write-validation/`.
The retained red case rejects a validation-only native writable-storage query
as untracked mutation. Resource access policies and mutation auditing now have
separate registrations under the existing shared lock. `validate_mutable_view`
still checks regions, commitment, access and platform/backing write policies,
but returns no mutable span and does not report a write. Actual writes and
mutable borrows still notify the recorder and world mutation observer; topology
changes retain their existing guards. The physical resource permission owner
remains a write guard. Unknown replay mutations still reject before publication.

The real eight-player/four-local-view native fixture captures original shadow
storage construction, BSP-plane insertion, warm reset and reinsertion. Replaying
owning calls reproduces the complete coupled canonical archive. Rebinding keeps
the original source allocation identities while using independent native64
backing. Permission-only frame admission produces no calls or external inputs;
its full before/after archive is compared using an explicitly frozen injected
profiling clock. The ordinary test clock advances whenever it is sampled,
including checkpointing, so that assertion requires a fixed clock rather than
normalizing archive fields. Mutable borrow/raw-write rejection and complete-cut
repair remain covered.

`source_execution_codec` adds a versioned little-endian owning phase format:
ordered input kind/operand/value, root call kind/arguments/input ranges/actual
result seals, independent millisecond/cycle/UTC clocks and Unix/PCR seconds.
Zlib compression and SHA-256 detect corruption; they do not authenticate a
sender. Fixed phase bounds remain 262144 inputs, 65536 calls and 4 MiB of total
call arguments/results. Decode admits envelope/decompression extents, minimum
wire counts and destination allocations before growing their containers, and
runs the same native input and typed call validators before returning a phase.
Truncation, unknown headers/kinds, impossible counts, changed consumption ranges,
wrong scalar widths, oversized payloads, trailing compressed/raw state and
configured byte/allocation budgets are covered, along with maximum admitted
input, call and payload sizes. Native model, ABI, command/resource and real-world
particle fixtures now cross compressed encode/decode before actual replay;
complete archive equality is retained.

This format is not yet accepted by `SourceAuthorityReceiver` or sent by the
runtime. Before replay, the session still needs to authenticate the physical
host and bind an ordered stream to its authority generation, membership and
validated base cut. Automatic whole-frame capture/publication, producer-view
catch-up before recipient rebinding, certification, gameplay endpoint/role and
voice promotion, custom native-owned mode coverage, performance and physical/WAN
verification remain open. Native protocol 83 and checkpoint component schema 2
remain unchanged. No launcher installation, backend deployment or UI-art change
is part of this milestone. The full ONLINE goal remains active.


Verification for this milestone: the final full native aggregate build passes,
and 17 selected native checks pass without skips, including warm and cold actual
peer reception, source clock/call/model/ABI/command/particle replay, original
session departure, local-view rebind, network departure, complete archive codec
and retained/shared memory. All tested native binary hashes remain unchanged
through the run. Nine selected ASan/UBSan/leak fixtures pass with 16 production
translation units instrumented. Existing native/physics/RTC archives and the
paired original shadow dispatcher remain uninstrumented; the optional attempt
to expand generated-dispatcher sanitizer coverage was stopped. This does not
close full transport TSan or platform/WAN acceptance. Initial shared-checkout
link drift was repaired by reconfiguration; no stub or resource-guard bypass
was introduced. Exact source/header cuts, commands, logs, assets hashes and
native binaries are retained with `verification.json` and `SHA256SUMS`.

The current native cut includes a concurrent unit-camera mode API extension
absent from the selected sanitizer cut. The write-observer edits match across
both cuts. That unrelated camera extension is not newly sanitizer-qualified;
the retained source comparison makes this boundary explicit.


## Ordered direct-peer native replay and delayed local handoff (2026-10-15)

Capsule: `.local/player-host-continuity-20261015/ordered-peer-replay/`.
The full ONLINE goal remains active. This connects the phase format to the
actual source publisher/receiver over authenticated peer bulk transfer; it
does not elect a new host or switch the visible game's transport/voice role.

An explicit replay mode retains the producer's view mapping. Each batch binds
its session, authority epoch, physical membership revision/mask and host to the
exact validated base cut, previous validated transfer seal and next phase
sequence. Each phase has a contiguous before/after simulation-tick interval.
The receiver checks the expected binding before allocating or decompressing
phases, then validates phase payloads under aggregate budgets. Defaults permit
32 phases, 32 MiB encoded/decoded and 64 MiB destination values per batch.
The publisher queues at most 1024 phases/64 MiB of owning values, selects at most
32 phases/~16 MiB of values for one worker job and awaits an exact executor
receipt before advancing the previous seal/sequence. Queue/encoding/recording
gaps invalidate this stream; they never replace missing source work with a
control-only approximation or stop/repeat successful real host work. Actual
exceptions from the host callback still propagate.

Snapshot and batch encoding/decoding run on workers over owning values only.
Native construction, transaction preparation and actual replay remain on the
retained source executor. Replay advances at most one phase per update, leaving
network/input pump boundaries between phases. Readiness is cleared before a
batch starts; a receipt is sent only after every phase succeeds. Cheap live
coupled checks retain the original variant, source/Match ticks, player pool and
active census, current-unit, local-index and first-person bindings. Every full
import and completed replay batch also checks all actual allocated player salts
and original identity bytes against the independent accepted source roster.
An execution-boundary stamp combines Match/Simulation mutations with actual
native writes/topology and mutating bridge calls, rejecting work omitted
between captures. Permission-only checks do not advance that stamp.

Legacy snapshot-mode constructors remain available. Replay candidates keep the
host's views until a separately chosen exact validated cut is handed off.
`take_world_for_local_views` stages the existing original local-view/input/
first-person transaction on private executors; active/identity checks remain
strict. Neither taking the producer world nor rebinding it certifies an epoch,
changes network endpoints, installs hardware listeners or promotes voice.
Explicitly supplied shadow clocks now also own their Unix/PCR provider through
UTC conversion, matching explicit ordinary-world injection. Default platform
clock behavior is unchanged; native milliseconds/cycles/UTC and Unix/PCR
anchoring remain independent phase values.

The native peer fixture uses two real native processes, the production queue,
measured host choice, actual grouped profiles, independent source admission and
DTLS/UDP transfer. Discord is a fixture. Forty-two accepted phases include
Match, particles, four ordinary views, visibility, weather, first-person models,
audio and event drains. Four complete producer-view canonical archives agree.
Filesystem files supply expected archives and coordination only; they do not
feed replay inputs to the recipient. A wrong sequence leaves the old qualified
world intact without acknowledging the bad transfer. A validly encoded actual
caller write that changes a source player's identity is executed, then rejected
before acknowledgement; its candidate is discarded and a new complete cut
repairs it. Inter-phase writes and unsupported in-phase work drop the sender
stream while its real host continues once. Worker cancellation cannot revive
readiness. Actual host transport shutdown retains the last validated world;
recipient couch views rebind, original host-group departure preserves retained
rows without numeric deaths, and ten subsequent native ticks survive.

Two test-boundary failures were reproduced and retained privately. A newer
attachment adapter preserves live ordinary bank0 instead of projecting a typed
callback replacement. The command fixture now seeds its retained bytes through
an explicit recorded input and tests that a conflicting typed replacement does
not overwrite them; banks1/2 retain their secondary-input checks. A sanitizer
handoff failure contained genuinely departed recipient rows: during slow
private-world loading, the fixture polled LanHost's five-second gameplay-input
timeout without advancing the game. That preparation interval is now quiescent
while the independent DTLS worker stays live. The departed-player handoff guard
was not weakened. No product diagnostic logging or source-owner stub was added.

Automatic full-runtime capture, remaining frame/camera caller coverage,
measured shadow-target policy, authenticated agreed cut/epoch certification,
retention before runtime teardown, actual gameplay role/endpoint promotion,
profile/hardware/voice handoff, custom native-mode coverage, realtime budgets
and physical/WAN checks remain open. The replay mode is currently explicit;
`multiplayer_runtime` does not yet instantiate its publisher or promote its
receiver. Protocol 83/checkpoint component 2 are unchanged at this opt-in stage;
production enablement still needs capability/version admission. No launcher
installation, backend deployment or UI-art change is part of this milestone.
The Xbox TU1.5 reference and Vista ONLINE/clan asset requirements remain in force.


Verification: the complete native runtime aggregate builds successfully. Twenty
initial native checks pass; the corrected command fixture and both warm/cold
replay and legacy-receiver fixtures pass on the retained matching native graph,
for 21 unique passing native checks without skips. Eight selected ASan/UBSan/leak
checks pass with 20 production translation units instrumented, including actual
warm/cold native peer replay, legacy receiver/publisher, mutation/input/batch
admission, shadow storage and corrected command traversal. Existing native
original/generated and RTC libraries remain uninstrumented; full transport TSan
and platform/hardware/WAN acceptance are not implied. Five later cinematic/
effect-light/options header edits in the shared checkout are outside this
retained cut; current native archives and matching frozen headers were kept
instead of mixing ABI variants or reverting concurrent work. Exact source,
headers, archives, native binaries, diagnostics and commands are retained with
`verification.json` and `SHA256SUMS`.

## Direct peer quality and measured physical host setup, 2026-10-15

The previous ordered-replay turn made concrete progress. This continuation
connects a new C++20 `PeerMeshQuality` owner to the production native session
worker and initial coordinator host choice. It does not promote a world or
close the full ONLINE goal. The retained evidence is
`.local/player-host-continuity-20261015/peer-quality/verification.json`, with
1,236 frozen source/header/backend/RTC inputs, matching network binaries,
commands in the tool history, build/test logs, `input-manifest.json` and
`SHA256SUMS`. The current selected source files match that frozen cut.

Every physical connection owner exchanges application echo probes, 64 bounded
1,200-byte upload chunks per directed path and complete 16-column quality rows
over its existing authenticated direct DTLS/UDP connections. The coordinator
receives initial setup summaries only. Gameplay and microphone queues drain
before measurement work; probe/reply/report queues and per-poll output remain
bounded. Mutable measurement state uses one short mutex; copies and ordering
are safe beside poll/receive updates. No Discord identifier is carried in the
measurement protocol or candidate presentation.

Probes bind the exact room digest, pre-host or accepted host context, epoch,
membership value and physical peer mask. The transport supplies authenticated
sender identity. Reports have strictly advancing sequence numbers; duplicate
or reordered reports cannot refresh their five-second freshness window.
Unknown/truncated/oversized/invalid context or column values never qualify a
peer. Upload receipts bind the actual outstanding round and chunk. Duplicate
receipts cannot extend duration or increase the acknowledged chunk count.

The first deterministic test exposed acknowledgement-queue congestion biasing
the sender's wall-clock estimate. Upload measurement now uses the receiver's
elapsed arrival span over distinct acknowledged chunks, without synchronizing
PC clocks. This is observed application delivery under a bounded offered
load; it is not an ISP line-speed estimate, an independently attested bandwidth
claim or a recovered Xbox coefficient. Receipt loss makes the estimate
conservative. NAT type stays unknown. No endpoint timeout changes membership
or supplies a cheating ban.

The native worker runs measurement before `peer_ready`, then resets telemetry
to the accepted host context and continues direct row exchange. Host eligibility
requires fresh measured reciprocal paths to every physical participant.
Ordering uses worst reciprocal loss, minimum outgoing observed delivery,
mean worst-direction RTT and stable room peer slot. The coordinator applies
the same setup policy only to physical owners, preserving playlist connection
thresholds. All-legacy rooms retain their existing RTT policy. Once upload
columns are present, missing reciprocal measurements cannot silently downgrade
the room to unmeasured selection. This policy is explicitly native, not an
assertion that the retail Microsoft formula has been recovered.

`OnlineMatchSession::host_candidates()` exposes fresh measurement candidates.
It neither creates a source publisher nor produces a prepared-world receipt.
Independent original allocation admission and complete replay validation remain
mandatory. The normal runtime still lacks automatic complete-frame recording,
agreed shadow retention and cut/epoch certification, actual host/endpoint role
promotion and profile/hardware/voice handoff. Local candidate snapshots are not
an agreed decentralized election. Survivor membership, asymmetric-loss policy,
500 ms suspicion/quorum, NAT qualification and the final asynchronous shadow
queue remain open. The new setup fields also need coordinated client/backend
capability admission and delivery before enabling the tested source in releases.

Verification at this cut:

- Five deterministic cases cover all directed four-PC probes, measured ordering,
  exact 12.5% directional loss, duplicate receipts, stale/disconnected paths,
  every context field, malformed/duplicate reports and concurrent readers.
- Four real socket peers complete 768 upload probes while 1,152 ordinary payload
  frames pass alongside telemetry. This exercises the actual ICE/DTLS/SCTP UDP
  stack. It is not a new audible Opus/device validation.
- Two separate native session processes use the actual authenticated queue,
  admitted four-seat groups, direct measurement workers and coordinator.
  No RTT, loss, upload score or host choice is injected. The final run chooses
  physical slot 4 from the initial measured summaries, receives 198 direct
  game datagrams on each PC and retains two fresh candidates. QoS alone leaves
  source admission and prepared-world receipts empty. Discord authorization
  remains a fixture; these are session/transport payloads, not a new complete
  source-world gameplay or migration acceptance run.
- 44 backend tests pass across direct admission/selection, gathering sessions,
  couch/guest policy and explicit continuity controls. The original direct
  transport test still passes its packet bounds and TCP/TURN denial.
- ASan/UBSan/leak and ThreadSanitizer each pass all five cases for the new
  production measurement owner. This is not full RTC/transport TSan proof.

The isolated aggregate compiles eight fresh production network/session/variant/
voice translation units against matching frozen headers and retained pinned
RTC libraries. Its session cut uses native protocol 86 because concurrent
gameplay work advanced that header after the previous protocol 83 replay cut.
No unrelated native archive was mixed into this build. The complete current
native renderer/runtime was not rebuilt or installed here, and no backend
deployment or Vista UI-art change was made. Original TU1.5 gameplay fidelity,
all remaining ONLINE requirements and physical/WAN gates stay active.

## Scoped host frames and automatic publication wiring, 2026-10-15

The preceding peer-quality turn was concrete progress. This continuation adds
an executor-local `SourceMatchAuthority::ExecutionRecording` scope and
`SourceAuthorityPublisher::Frame`. The scope owns the existing input/call tape,
write/topology observers and active native boundary over the real frame.
Finishing seals actual calls, returns, external inputs and independent end
clocks. Cancelling releases guards without repeating or stopping completed
real host work. Active checkpoint/nested scope attempts and wrong-thread or
duplicate finish attempts reject. An input-limit failure still runs the actual
source tick once and leaves the host available for a complete resynchronization.

The publisher's optional frame scope closes on normal destruction, including
an early game-loop continue/break. Unwinding cancels the stream and preserves
the original native exception. Transcript, allocation or queue failure rejects
the backup qualification; it does not substitute for or repeat the frame.
Its callback `execute` API now uses the same scoped owner. Publisher calls
that need a quiescent boundary reject while a frame is active. The authority
and publisher must outlive their respective executor-local scopes.

The normal game-loop source now selects a measured eligible physical backup,
requires independent source-admission confirmation and explicit replay wire
support, captures a complete base and records the whole headless/rendered host
iteration. This includes native work after Match.step, camera/effect/render
callers and the renderer tail; early exits release the scope. An eligible warm
backup is retained until it or the accepted context becomes unavailable.
This is backup retention, not an election of a replacement gameplay host.
The client receiver now requests replay mode. Unknown frame callers or raw
mutations still invalidate the transcript; the capture boundary does not
magically implement an uncovered original caller.

Authority hello/ack packets can advertise an optional replay feature separately
from their maximum byte size. Maximum-only legacy hellos advertise no replay
support. Context replacement clears the capability. `authority_replay_supported`
requires an admitted direct endpoint and that feature; neither a hello nor QoS
metadata certifies a native world. Actual full-cut/replay validation and receipts
remain mandatory. Existing legacy snapshot constructors remain available.

Evidence is retained in
`.local/player-host-continuity-20261015/frame-recording/verification.json`,
`input-manifest.json`, commands, logs, binaries and `SHA256SUMS`:

- The current `multiplayer_runtime.cpp` passes a syntax check against its frozen
  current headers, including both automatic frame scopes and publisher wiring.
- Actual TU1.5 owner checks preserve complete canonical worlds through a scoped
  tick and later observer/model work. Active/nested/wrong-thread/duplicate use,
  cancellation and exhausted transcript cases pass.
- Warm and cold two-process native peer fixtures each replay 42 accepted phases
  and compare four complete worlds. Their actual frames use the scoped API;
  alternate frames close by early return. Wrong chains and changed identities
  remain unacknowledged, full cuts repair the stream, and retained source worlds
  still rebind after actual host transport loss and continue ten surviving ticks.
- Selected ASan/UBSan/leak runs pass the scope owner and warm native peer fixture.
  Twenty-three production translation units are instrumented, with matching
  retained original/generated/RTC libraries uninstrumented. Pure transfer-owner
  ThreadSanitizer passes capability, concurrent pumping/publication/acknowledgement
  and context-replacement checks; this does not close full RTC transport TSan.

The native tests retain the previously completed native layout/archive graph,
with updated public methods and freshly compiled phase/publisher/session/transfer/
quality owners. Their native layout headers and archives are frozen;
new concurrent gameplay layouts are not mixed into them. Both frozen cuts
actually contain protocol 86, confirmed from their headers and the native
fixture's check of emitted gameplay packets. Earlier protocol 83 notes do not
describe this retained cut. There are 2,322 frozen inputs. A complete
current renderer/runtime build and actual rendered publication/qualification
have not been established by these checks.

Complete frame/camera/hardware-input ownership, actual game-loop qualification,
base-capture latency, bounded replay catch-up at real frame rates, retention
before runtime teardown and agreed cut/membership/epoch certification remain
open. The runtime still exits on host connection failure; it does not promote
the prepared world, rebind gameplay endpoints, profiles, controllers or voice.
Custom native modes, survivor election, the modern quorum/asymmetry/shadow queue,
full current runtime delivery, deployment and physical/WAN acceptance stay open.
No launcher installation, backend deployment or Vista asset change was made.
The full ONLINE goal and sole Xbox TU1.5 gameplay reference remain active.

## Full native/rendered cohort and state-query ownership, 2026-10-15

The scoped-recorder turn was progress. This continuation freezes 17,014 code
and pinned RTC inputs in `.local/player-host-continuity-20261015/rendered-auto`,
builds the full native executable and actual rendered ONLINE fixture, and
exercises two real game processes on the RTX 5080 with four local views each.
The fixture pins the verified English Xbox TU1.5 executable, uses the selected
Vista UI bank, admits eight actors through the production queue and direct
peer transport, and runs the real input/simulation/render loop. Discord remains
a fixture. This is local GPU evidence, not physical controllers or WAN proof.

The initial workload exposed a test error: its legacy `replay` control option
disabled automatic native-world construction. The corrected workload explicitly
sets `source_object_world` and witnesses construction on the authoritative PC.
The fallback pass is retained as a diagnostic and does not count as source
replication evidence. Source-owner runs advance all eight players and submit
hundreds of real four-view frames. The earlier shield-part error did not occur
in this particular frozen TU1.5 workload; this is not a general visual-parity
claim.

Automatic recording starts, but its transcript rejects unrecorded native work.
A private first-failure stack trace identifies `LanHost::receive` calling
`MultiplayerMatch::state`, whose native HUD queries execute mutating source
operations despite the public snapshot method being const. Those calls must
remain in their actual order; simply classifying them as read-only would hide
real scratch/native mutations.

The new 24th owning operation, `MatchState`, executes the actual state method
and validates a SHA-256 digest of its complete returned value. Its canonical
encoding retains exact scalar/float bits, bounded strings and explicit grenade
action fields, without C++ padding or normalized fields. The result is compared
with the real return; it is never supplied as a replacement. Empty arguments
and exact digest size are preflighted. The native regression rejects a changed
digest, repairs through a complete cut and then reproduces equal full canonical
worlds. ASan/UBSan/leak checks pass with the changed Match/input/call owners
instrumented against the matching frozen native graph.

Replay now requires both the original replay capability and the state-journal
capability. An older replay decoder cannot qualify by advertising the original
bit alone. Pure transfer-owner tests retain concurrent pumping, validation,
context-reset and legacy capability checks. A real two-process peer fixture
includes state queries before and after every frame: all 42 accepted phases
and four complete worlds still agree, including wrong-chain/identity rejection,
full repair, delayed view rebinding and ten surviving ticks after transport loss.

The rendered trace then advances to a different missing caller:
`PhysicsLiveWorldTu15::player_camera_packet`, reached through
`NativeVehicleCamera::update_player` during host broadcast. That camera caller,
its observer/controller continuation state and subsequent frame work remain
to own. Rendered replication does not qualify yet. Native capture, mutation,
identity and replay guards were not weakened to obtain the workload pass.
The private stack instrumentation is removed from the final clean full build;
the exact diagnostic binary hash and symbolized stacks are retained separately.

`verification.json`, manifests, clean/diagnostic build logs, native/peer/sanitizer
results and `SHA256SUMS` record the tier boundaries. Assets and original analysis
references remain external read-only inputs. Later concurrent campaign changes
to `multiplayer.cpp` remain untouched in the workspace; only this task's two
state-journal insertions were composed into the frozen matching native layout.
This avoids linking newly changed class layouts with older native objects.

Full rendered qualification, camera and remaining caller ownership, real-time
capture/catch-up budgets, retention before teardown, agreed generation/cut
certification and actual gameplay/profile/hardware/voice promotion remain open.
No launcher was installed, backend deployed or Vista art modified. All other
ONLINE acceptance requirements and the full active goal remain in force.


## Camera and lens-query caller ownership, 2026-10-16

The previous full-build/state-query milestone was progress. This continuation
keeps the full ONLINE completion goal active and preserves the sole Xbox TU1.5
gameplay reference. Evidence is retained in
`.local/player-host-continuity-20261015/camera-caller` and its `lens-query`
successor; neither cut is a launcher installation or backend deployment.

`PlayerCamera` is the 25th owning root operation. It retains the real player
identity, seconds, all 80 controller words, 214 observer words, random state,
cycle input and optional mode-output presence. Replay executes the actual
private-view scheduler and checks a canonical digest of the resulting packet,
controller, observer, random and mode values. Native observer locate/collision/
visibility callbacks now use the existing explicit source-argument-frame,
caller-write and actual-entry boundaries. No native query was relabeled as
read-only and no original camera result is substituted.

The camera native fixture updates four actual private views, rejects a changed
camera return, repairs through a full cut and compares complete coupled worlds.
The peer fixture also runs the four actual cameras. A private trace of the
rendered eight-seat workload then moved the first unrecorded write to
`SourceLensQueryFrameTu15::Impl::run`, through the original `20B90` query body.

`LensQuery` and `LensQueryFinish` bring the root-operation count to 27. The query
bridge records ordered, address/width-bound reads of the renderer's private
camera/projection/device/query-metadata storage in the existing bounded input
tape. Replay executes the actual TU1.5 query bodies with those owning external
operands. It checks the actual full ABI plus the ordered private-device write
digest; reads of bytes written earlier in the same kernel must agree with those
actual writes. The bridge preserves the original adapter's distinct read/write
overlays. It records operands rather than copying the entire GPU FIFO into
each phase. Unbound calls continue directly through the actual device adapter.
Recorder allocation/hashing failure revokes capture and lets the real host
continue; replay fails immediately. Final digest construction is inside the
existing failure-isolated journal completion boundary.

Actual query-ring rollback and the original 64-bit presentation-count
publication now execute through an owning operation. View retirement uses the
existing explicit constant caller-write operation. The real renderer continues
to supply hardware query results and actual submission/decline events. Vulkan
leases, pending/staged query identities, fences and private camera continuation
are not transferred by this tape and still require a certified hardware/view
handoff; recorded device inputs do not qualify that handoff.

The native extension fixture executes original `402650` allocation and `402710`
pending polling using declared device inputs, plus both retirement paths. It
compares complete native/Match checkpoints, rejects a changed device-write
seal and repairs. The independent-process peer fixture now carries the same
query operands alongside four camera views in 42 phases and four complete
world comparisons. Its device inputs are declared fixtures, not a physical GPU
sample test. The rendered workload uses the actual RTX 5080 query owner.

Native replay capability now requires mask 15 (replay, native-state, camera and
lens-query revisions); older decoders remain ineligible as source replay
recipients. Narrow-width/wrapped-address operands, instrumentation failure
isolation and older capability handling have focused coverage. Selected native
ASan/UBSan/leak checks retain their explicitly uninstrumented original/generated
and transport dependencies; they do not establish full transport sanitizer or
physical WAN acceptance. Final statuses and hashes belong to each evidence
cut's `verification.json`, rather than this narrative.

The native build overlays retain the full prior frozen graph and its exact
class-layout headers. Only the two camera wrapper/callback blocks were composed
into frozen physics/object-runtime sources; concurrent campaign class layouts
were preserved in the working tree. Current source syntax is checked separately
and is not a current-layout executable claim. The final lens serializer uses
the frozen vendored PFR headers. Original assets/reference analysis remain
external read-only inputs.

The next rendered first unrecorded writer is now the final flare drawing
adapter: `SourceLensDrawFrameTu15::Impl::initialize -> execute ->` actual
`1301E0`. Its private residency/device input, complete original parent/world-pass
handoff and draw emission still need an owning boundary. Full rendered frame
qualification/catch-up, source-world retention before runtime teardown, agreed
cut/membership/epoch election, actual role/gameplay/profile/controller/hardware/
voice promotion and physical/WAN acceptance remain open. The runtime still
exits on host failure. The modern quorum/asymmetry/shadow queue and every other
unfinished requirement of the full ONLINE goal remain active; neither an
endpoint switch nor these native replay fixtures are seamless host migration.


## Rendered replay, drawing/collision boundaries and shared contact journal, 2026-10-16

The camera/query turn was progress. This continuation retains the full ONLINE
goal, the sole Xbox TU1.5 gameplay reference and the existing Vista UI selection.
Evidence cuts are `.local/player-host-continuity-20261015/lens-draw`,
`particle-query` and `projectile-journal`. No launcher installation, backend
deployment or UI-art replacement is included in this milestone.

`LensDraw` is the 28th owning operation. The actual drawing adapter now supplies
its private device-region descriptors and uses the existing ordered, address/
width-bound device-read tape. Actual private writes and the complete returned
ABI are checked. The original draw/query/live-support/math bodies still execute.
The original13660 parent stops at its actual2BCB0 world-pass call; all six real
arguments and the partial ABI/write result are sealed. Replay checks that real
handoff rather than inventing a return from the child. Native fixture inputs
include an actual owned-cache 2D bitmap metadata row. Header construction and
its resident alias, parent handoff, changed result/argument rejection and
complete coupled-world equality pass. Private region descriptors reject empty,
unsorted, overlapping and wrapped extents.

The next actual rendered trace found the native particle collision callback.
`ParticleCollision` is the 29th root operation: it owns the exact flags, origin
and displacement and checks every actual optional hit field, with no host
padding or normalized floats. The real16C620 owner executes. Native eight-actor
rays exercise actual hits and misses, altered results reject and full repair
preserves the complete world. Peer fixtures now carry private-camera, query,
header/world-pass and particle-query work in42 phases/four complete worlds.

With those native callers covered, the strict C++ counters found exactly one
extra Match mutation and one Simulation mutation per rendered network frame.
Both came from the repeated default workbench-settings setter. Network settings
now reset once during session setup, before any source cut; network debug
requests remain rejected. The mutation guard still rejects actual unrecorded
changes. The frame budget was not the problem: the diagnosed frame contained
1,124 calls,288,397 call bytes and5,188 external reads.

The actual backup next rejected service roles97 versus65 with equal admission
and configuration bindings. The only extra host bit was the renderer-local
software weapon-effect callback. Original source movement/weapon stepping
bypasses that software path and publishes completed effects through its native
observer/journal. ONLINE source worlds now retain that path and do not install
the unused fallback. Other local/LAN paths retain their registration. A native
30-tick firing probe installs a fallback that throws if invoked; real ammunition
is consumed, original scheduled effects appear and full replay equality holds.
No compatibility bit was cleared to bypass the rejection.

The next first actual replay mismatch was MatchState. Private canonical field
comparison found identical initial states and then one differing top-level
field: `grenade_detonations` had1,388 bytes on the host and an empty4-byte vector
on the backup. The live runtime had registered the real projectile-contact
observer outside the prepared world. That binding now belongs to every source
Match through the object runtime's private continuation, with the same actual
projectile/effect listener and `publish_source_projectile_contact` callback.
Match destruction retires it before the journal target; transient projectile
update scopes remain quiescent at completed authority boundaries. Binding
metadata stays readable while a native import is staged; this getter performs
no original execution or bank access. The Match service contract requires the
real binding, and mismatched roles reject before native storage staging.

The capture tool consumes the common Match journal rather than installing a
second observer. Native tests cover the real binding, a forged missing role,
callback retirement and re-registration after Match destruction. The isolated
metadata-rejection fixture freezes its declared probe-on-read cycle clock;
an extra checkpoint must not itself advance the fixture's comparison clock.
New Core replay requires capability mask127. Public class data layouts are
retained; only method declarations and a private continuation owner were added.
Frozen Match/physics/object-runtime bodies receive only the relevant insertions,
without importing concurrent campaign layouts. World, receiver, runtime and
owning fixtures are rebuilt against the explicit Match destructor.

The rendered test now demonstrates an actual validated batch and peer receipt:
the backup completed32 phases through tick95, and the live host acknowledged
that batch. It subsequently reached48 applied phases. This runs two real
four-view game processes/eight source actors on the RTX5080. The desktop's
Xwayland display became0x0, so the later workload uses an isolated Xvfb X11
surface with the actual NVIDIA Vulkan device; this is not physical-monitor,
headset, widescreen-pixel or WAN acceptance. Private runtime/state probes are
kept only in diagnostic executables; production guards and actual return
validation remain intact.

Sustained catch-up is not qualified: the host still had58 queued phases at the
recorded acknowledgement/shutdown. The native core remains slower than the
render-only client in this workload. Full current-layout delivery, broad native
mode/weapon coverage, private camera/GPU continuation, exact-cut/membership/
epoch certification, world retention before normal runtime failure teardown,
actual gameplay/voice role promotion and physical/WAN acceptance remain open.
The normal runtime still exits on host failure. The modern quorum/asymmetry/
shadow queue and all other unfinished full-goal requirements remain active.
Each evidence cut's `verification.json` owns final statuses/hashes; selected
sanitizers retain their stated uninstrumented dependencies.

## Background native replay executor and measured frame backlog, 2026-10-16

The first private timing run uses the matched final contact-journal cut. Its
8,207,203-byte base captures in 496,316us and encodes in 934,047us; the existing
512 KiB/s application transfer budget takes approximately 18 seconds on the
localhost peer path. Candidate/workspace construction takes 2.82/3.20 seconds,
preparation 184,872us. The first 32 replay phases take 9.25 seconds including the
client's intervening frames; actual phase CPU median is 204,574us and p95
280,198us. This is a measured replay/input-thread scheduling and initial-transfer
cost, not evidence of a new original gameplay or network timeout policy.
Detailed events are retained in `.local/player-host-continuity-20261015/replay-profile/`.

`SourceAuthorityReplayWorker` now constructs, warms, replays and destroys its
private native worlds on one background executor. Existing source thread,
callback, borrow, identity, service-role, input/result and receipt guards remain
in force. Status is an owning value copied under a short mutex. At most one
checkpoint export may be outstanding; the executor checks the exact validated
peer/context/seal and fulfills a future with an owning coupled checkpoint.
Cancellation immediately revokes exported readiness, wakes the worker and
rejects further requests. Destruction joins after the current native call and
destroys the world on its constructing thread. No native world/view/hardware
pointer crosses to the game thread. Initial warm overlaps transport only after
independent source-player admission; a failed replay still needs a valid full
cut for repair. The normal runtime reads status without executing backup phases
on its render/input thread. It still exits on host failure: this worker does not
elect, certify or promote a replacement host.

The background eight-actor/two-process peer fixture passes 42 accepted phases,
four complete canonical comparisons, forged full/replayed identities, wrong
sequence, full repair, stale export and cancellation rejection. After the host
transport fails, it exports the retained exact cut, retires the worker, imports
into a separate executor with the independent full roster, runs original local
view rebinding, departs the original host's four seats without fabricating
deaths and advances ten more source ticks. No filesystem fixture supplies
replay input to the backup. `HALO2_RECEIVER_ASYNC=1` selects this additional
CTest case; the existing direct-executor and cold cases retain their checks.

A matching 45-second rendered workload passes with eight actors/four views per
PC on the actual RTX5080 through Xvfb. The host acknowledges 64 phases through
tick 191; the backup applies 96 phases. The last sampled queue is 59 phases.
The first 32-phase replay batch takes 6.42 seconds; sampled game-thread status
reads take 14–28 us during active replay, rather than executing the roughly
0.2-second native phase there. The rendered client submits 878 frames versus 492
in the preceding observed run; these sequential observations are not a physical
display, frame-rate guarantee or a controlled hardware benchmark. No rate/window
budget, original simulation call, input/result check or compatibility gate was
weakened. Sustained catch-up, hardware/camera handoff and automatic certified
host/gameplay/voice takeover remain outstanding. The later 90-second workload
and selected sanitizer lifecycle results are recorded separately when terminal.

The 90-second rendered workload also passes. The last host sample records 302
phases, acknowledges 224 through tick 671 and retains 46 queued phases plus a
32-phase batch in flight; the receiver validates 224 phases. The host finishes
at tick 923. This still leaves a substantial world lag, so sustained catch-up
and a usable takeover cut are not qualified. No automatic host-failure recovery
is claimed. Selected ASan/UBSan/leak instrumentation of the new receiver/worker,
peer fixture and retained Match/world/contact/execution owners passes the same
complete peer flow in 119.90 seconds; retained original/generated/RTC code is
uninstrumented, and this is not a whole-program race proof. The three touched
C++ units also pass current-checkout syntax checks without observed source drift.
`.local/player-host-continuity-20261015/background-replay/verification.json`
records the matched sparse-overlay source/link hashes, sanitizer tier and both
rendered timings. No launcher installation, backend deployment or UI art change
occurred in this cut. Every remaining full ONLINE goal gate remains active.
