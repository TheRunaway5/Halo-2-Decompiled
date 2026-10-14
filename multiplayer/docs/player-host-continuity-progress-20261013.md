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
