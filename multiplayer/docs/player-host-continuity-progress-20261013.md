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

## Validated recovery retention and compressed rendered stream, 2026-10-16

The preceding turn completed real Steam/native frontend login and browser-return
acceptance. This turn resumes player-hosted migration; Steam remains primary and
no gameplay or microphone data was moved to the master. The full ONLINE goal,
Xbox TU1.5 gameplay reference and selected Vista UI assets remain unchanged.

`SourceAuthorityReceiver::retain_validated_world` permanently stops accepting new
transfers. A cut already decoding or replaying must finish ordinary native
execution, full identity/result checks and validation before it can be exported.
There is no intermediate-bank export. The owning recovery value carries its exact
validated cut/context, independent full source-player admission, producer and
recipient view mappings, and coupled Match/source-world checkpoint. No native
world, view, callback, device or executor pointer transfers to the caller. A
quiescent missing/invalid world rejects. Context replacement still discards the
superseded candidate.

The background worker exposes a one-shot future for this retention. Checkpoint
exports and retention cannot overlap; repeated retention, missing admission and
cancelled workers reject. Cancellation revokes readiness immediately, and worker
destruction joins native work on its original executor. A successfully returned
value survives worker destruction and can be imported into a separate native
executor. This is recovery input, not an election/cut certificate or automatic
host promotion. The normal runtime does not yet call this API before its failure
teardown, so installed gameplay migration remains disabled.

Evidence is in `.local/player-host-continuity-20261016/retention-final/` and
`retention-overlap/`:

- Two actual authenticated native peer processes use the direct UDP stack and
  eight source actors/four local views per PC. They retain the earlier wrong
  identity/chain rejection, full repair and four canonical-world comparisons.
  An additional eight-phase authenticated batch brings accepted replay to fifty
  phases. Retention starts during partial replay; the host transport then closes.
  The final test explicitly requires session failure while replay is still active
  and the retention future is pending. The backup finishes that batch, compares a
  fifth complete world with the producer, retains its independent roster/views
  across worker destruction, imports/rebinds on a separate executor, departs the
  old host's four seats without added deaths, and advances ten surviving ticks.
  The native recovery run passes in 112.52 seconds; the stronger overlap-assertion
  run passes in 112.68 seconds. Fixture files supply comparisons, never backup
  replay input. External identity is still a fixture.
- Selected ASan/UBSan/leak instrumentation passes the same recovery path in
  119.78 seconds. The changed receiver and peer fixture plus retained Match,
  world, physics, execution, lens and contact owners are instrumented; original,
  generated and RTC libraries are not. This sanitizer fixture predates only the
  additional timing assertion; its production receiver sources are identical.
- Both touched C++ units pass current-checkout syntax checks with no observed
  source drift. The functional native cohort deliberately retains its matched
  frozen protocol86 layouts/archives rather than mixing concurrent engine class
  layouts. This is not a fresh complete current-runtime build or delivery.
- All 35 current backend continuity/transport tests pass, including nonhost
  departure, surviving result confirmations and stale-generation rejection.
  Those control fixtures do not establish decentralized election or gameplay
  takeover in a released client.

The pending level6 compression workload was also run to completion: ninety
seconds of real eight-actor/four-view rendering on NVIDIA Vulkan under Xvfb,
with the matched frozen cohort. It passes and acknowledges 224 phases through
tick671. The final sampled host has recorded287 phases, with31 queued plus32 in
flight; the backup has applied228. Measured replay-phase CPU median is214,055us
and p95 is334,719us. Compression shrinks the later32-phase payload to about1MB,
but it does not qualify sustained catch-up or a timely live takeover cut.
The result is recorded in `compression-live-retry/verification.json`; the earlier
interrupted attempt remains diagnostic and is not counted as acceptance.

The next integration still requires normal-runtime retention before teardown,
agreed cut/membership/epoch certification and survivor election, sufficient live
catch-up, actual authority/endpoint/controller/profile/voice promotion, full
current delivery, and physical multi-PC/WAN/headset acceptance. No launcher
installation, backend deployment or UI-art change occurred here. The modern
quorum/asymmetry/shadow-queue requirements and every other full-goal gate remain
open.

## Normal runtime recovery handoff and stalled-host disconnect, 2026-10-16

`run_multiplayer` now has an owning recovery-output overload. On an established
ONLINE client disconnect it requests validated replay retention before destroying
the worker. Already admitted in-flight native work drains on its original
executor; the caller pumps OS events, and a window quit cancels retention. The
retained checkpoint, exact cut, independent admissions and view mappings are
separate from the client's display snapshot and PCR outcome. The normal frontend
application retains this value across world teardown. Starting another runtime
clears the previous recovery output. No candidate is elected or promoted by
retention, and no recovery value grants authority to publish a result.

The existing client snapshot timeout previously threw `LAN host disconnected`
even while its authenticated direct/control session remained alive. That bypassed
ordinary outcome and recovery teardown. An established ONLINE client's error now
uses the same disconnect-retention path. A peer's rejection before native player
admission retains its prior error behavior. Local/LAN behavior is unchanged.

Evidence cuts are `.local/player-host-continuity-20261016/runtime-retention/`
and `runtime-stalled-host/`:

- A real two-process/eight-seat rendered native session loses its host after
  45 seconds of gameplay. The client retains validated tick287 while its last
  display snapshot is tick470. After actual runtime/world/renderer/worker teardown,
  the test constructs a separate native executor, imports the retained world,
  rebinds its four local views, departs the old host's four seats without adding
  deaths, and advances ten ticks. It passes in97.08 seconds. The first attempt
  already retained the world, but incorrectly passed the fixture's capture-only
  flag to the later constructor; that correctly rejected. The final fixture
  uses explicit gameplay-only configuration and retains that constructor guard.
- The stalled-host workload pauses only the actual host game loop for12 seconds,
  leaving its authenticated peer/control workers alive. Before the fix the real
  client threw `LAN host disconnected`, and the workload failed. After the fix it
  returns a validated recovery cut at tick95, with display tick245, while the
  session is explicitly required to remain Running. The same independent import,
  four-view rebind, departure/results and ten-tick continuation pass. The full
  workload passes in118.59 seconds. No timeout alone changes host or membership.
- Both rendered harnesses assert that no unfinished match result was persisted.
  These are actual NVIDIA Vulkan/Xvfb native loops with the Vista UI selection,
  not physical-display, controller/headset or WAN acceptance. External identity
  remains a fixture. They prove recovery handoff, not automatic live migration.
- The real local-runtime test passes stale-recovery clearing, unfinished world
  teardown and normal three-minute Slayer finalization/retained source result
  banks in21.05 seconds. Current-checkout syntax checks pass for the runtime,
  main, rendered fixture and local-outcome fixture, with no observed source drift.

The functional binaries deliberately use the matched frozen protocol86 native
layout/archive graph, with this task's production runtime integration applied as
explicit hunks. Concurrent campaign/renderer class layouts were not mixed into
that graph. A full current engine build/install, automatic cut/membership/epoch
certification, survivor election, transport/controller/profile/voice promotion,
sustained live replay catch-up and physical/WAN acceptance remain outstanding.
The displayed/retained tick differences are real lag and must not be described
as seamless continuation or silently rolled back. No backend deployment, UI art
replacement or master gameplay/media relay was added.


## Replay operation timing, 2026-10-16

A private diagnostic executable records actual root-operation durations while
replaying the unchanged complete source journal. The matched ninety-second
rendered workload completes successfully in134.67 seconds including setup,
with248 replay phases timed. Median phase CPU is218,076us; p95 is305,211us.
`MatchStep` accounts for48.69 seconds across743 actual source ticks, or85.49%
of summed root-operation time. `RunActualSourceCall` is5.30%, first-person
models2.55%, MatchState1.38% and player camera1.26%. Timings include ordinary
scheduling and this private instrumentation; they are not a controlled physical
hardware benchmark or whole-engine profile. No source operation, return check,
identity check or native mutation guard was removed.

The result is retained in
`.local/player-host-continuity-20261016/replay-call-profile/result.json` and
`call-timing-summary.json`, with private source, command, executable and logs.
Instrumentation is confined to that diagnostic translation unit, absent from
production sources. The evidence narrows the next catch-up work to native tick
cost and transfer/validation scheduling. A bounded overlap of authenticated
transfer and replay must preserve ordered native validation and every exact
cut/identity check; compression alone did not qualify sustained live catch-up.
The world lag, election/promotion, voice handoff, current delivery and physical/
WAN gates remain open.

## Bounded peer transfer and replay overlap, 2026-10-18

The authenticated direct-peer authority stream now negotiates a two-cut
pipeline. Once all predecessor bytes have been acknowledged, the host may send
one successor while the recipient validates the predecessor's native calls.
The recipient decodes that successor on its decoder thread, then waits for the
preceding native execution to complete before applying it. Full base cuts still
require native validation before replay begins. Existing byte, allocation,
rate, window, timeout and 32-phase batch limits remain in place; the pending
transfer count is bounded at two per recipient. Older endpoints retain the
single-cut path. No gameplay, world payload or media passes through the master.

Byte acknowledgements never produce world readiness. Earlier validation
receipts survive prefetch and are delivered in native order, including recovery
of a lost predecessor Ready from the same executor's validated successor.
Context replacement clears the pipeline. Invalid delivered input is rejected
without a receipt and releases its receive slot. A native replay failure also
rejects queued or decoded descendants; a decoder still working on an invalidated
generation rejects its own result. It cannot revive that failed chain.

The native regression first reproduced the missing descendant rejection in
the earlier receiver, then passed with this cleanup. Its first failed batch
contains 31 genuine native phases followed by a forged source-player identity;
an authenticated successor is already prefetched. Neither receives a validated
receipt. The host observes prompt successor rejection, repairs from a complete
cut, and the receiver again matches the complete canonical world. The cold
recipient case passes in 120.02 seconds. The background case with selected
native-owner ASan/UBSan and leak detection passes in 137.81 seconds; dependencies
outside that instrumentation cohort are not covered by that sanitizer claim.
CTest's cold and retention cases now exercise this regression.

The peer fixtures also compare four complete worlds, exercise 70-phase initial
catch-up and bounded overlap, reject a wrong chain, cancel encoders, and validate
82 acknowledged native match/effect/four-view/weather/audio phases. The
background retention case adds 40 phases in two prefetched batches. The real
host transport closes while the first batch is still executing; retention
drains both batches, compares the final complete world, survives worker
destruction, imports on a separate executor, rebinds four local views, departs
the old host's four seats without added deaths, and advances ten native ticks.
This is 122 accepted phases, plus the deliberately rejected identity batch.
No unfinished result or personal carnage report is published. These cases do
not elect a host or promote its gameplay/voice transport.

Transport owner tests pass normally, under ASan/UBSan/leak detection, and under
TSan, including simultaneous bidirectional polling, delivery, publication and
context replacement with loss, reordering and duplication. The changed C++
owners and test also pass syntax checks against current workspace headers.

Evidence is retained under
`.local/player-host-continuity-20261016/pipeline/`, including commands, source
manifests, owner results, initial cold/background peer results and
`prefetch-failure/` with the reproduced failure and both passing repair runs.
Native functional runs use the explicitly retained protocol-86 native/RTC/
layout cohort. Concurrent workspace work introduced a newer effect-frame API;
only the private fixture keeps that cohort's original effect-particle call.
The current source is preserved and syntax-checked separately. The exact
projection is recorded in `prefetch-failure/test-cohort-projection.json`.
These executables are not a current full engine build or installed delivery.

The extended rendered run is **not a terminal pass**. The OS killed its unit
for memory pressure after 187.45 seconds, with recorded peaks of 37.9 GiB memory
and 38.1 GiB swap. Its last paired sample, 117 seconds after the initial profile
sample, records 424 host phases, 408 acknowledged and 412 applied by the shadow.
After the first minute the host's unsent queue ranged from zero to eight phases.
This is useful partial scheduling evidence, not sustained catch-up, memory
qualification or a few-frame handoff guarantee. Logs, the OS journal and partial
analysis are retained in `long-run/`. A separate allocator/resource diagnostic
must also be reported independently; changing test resource limits does not
establish production readiness.

That diagnostic used `MALLOC_ARENA_MAX=2` and recorded process memory while
limiting the owned test unit. It was stopped after 185 seconds of observation
while the retained cohort remained in startup at its resource ceiling (24.25
GiB summed process RSS and 1.99 GiB swap). It did not reach the measured gameplay
window or pass. Samples, limit changes and stop status are in `allocator-run/`.
This does not diagnose memory behavior in the newer current engine. Both owned
rendered test processes were confirmed stopped; no unrelated game was stopped.
`pipeline/verification.json` ties the passing tiers, reproduced failure,
interrupted diagnostics and exact source/binary hashes together.

Automatic host promotion remains disabled. Agreed cut/membership/epoch
certification, native gameplay/endpoint/profile/controller/voice promotion,
broader mode/combat/PCR coverage, current full delivery and physical/WAN
acceptance remain required. Match and voice ownership stay on player PCs.

## Private camera continuation and native fatal readback, 2026-10-18

`NativeVehicleCamera` now exports an owning in-process checkpoint of its entire
four-slot observer/controller bank, player bindings, seated/transition flags,
saved clusters and private RNG. The checkpoint contains values, not runtime,
device or borrowed world pointers. Preparation requires the caller's exact
world-cut binding, matching native low32 game tick and full native player
allocations/identities. Those checks are not a host-election certificate. The
caller still has to restore the complete native world and its admitted asset/
service configuration first. There is no camera peer codec or automatic
promotion wired by this API.

Capture/preparation reject an active native input recording or replay. Commit
requires this target camera, its native executor, the prepared native mutation
stamps and the same current private camera state. Cancellation changes neither
world nor camera. An owning checkpoint survives destruction of the producing
camera; preparation remains bound to a live target and world. The existing
camera object's layout is unchanged.

The native vehicle fixture now checks real Ghost-driver, Warthog-passenger and
seated-death trajectories. Each captures twelve actual Match/state/camera
phases, restores the complete native world and camera, and replays the original
calls. Observer position/basis, lens, aim displacement and camera mode compare
bit-for-bit, and the complete canonical world archive must match. A further
recipient world is constructed and imported on a separate native executor with
different platform clocks. It imports only owning world/camera values and
reproduces the same trajectory and complete archive before destruction on its
own executor. This keeps the same two-player/view mapping; it does not prove a
new network role, four active couch views or hardware ownership handoff.

The cases reject empty/wrong bindings, stale ticks, changed native identities,
wrong executors/owners, changed native/private state, cancelled preparation and
publication inside an active source frame. Death additionally checks one
numerical death and retention of the exact old body identity. The death
trajectory includes original watcher cycle inputs; the native call journal
checks the private RNG argument and result on every camera packet. All three
native cases and selected native-owner ASan/UBSan/leak cases pass in the retained
matched cohort. Latest normal times are 7.03, 8.68 and 8.78 seconds; selected
sanitizer times are 10.68, 12.94 and 14.24 seconds. Generated/runtime dependencies
outside that sanitizer cohort are not covered by this claim.

The initial complete-world comparison exposed platform clock samples taken
between the final call and checkpoint. A named-field diff isolated exactly
`logical_milliseconds`, `logical_cycles` and `logical_utc_filetime`. The fixture
now controls these external providers; it still advances native game ticks,
executes and checks every real source input/return, and compares the entire
archive. No clock field or gameplay state was dropped from validation.

The death case also reproduced two actual native/CPP integration errors. A
native fatal transition had already cleared the first-person unit while the
CPP player still awaited completed world readback. The pre-world pickup pass
read that intermediate native binding, and subsequent command delivery tried
to send look/control to the CPP-cached old unit. A debugger traced the first
error through `step_weapon_pickups()` and showed FP unit NONE beside the old
CPP current unit. The pickup pass now refreshes only the CPP-owned inventory
path there; TU1.5 native views still refresh at the existing completed-world
boundary. Native command admission independently reads the actual salted
source-player row, supplies neutral controls while its current unit is NONE,
and continues to reject a different non-NONE owner. Existing world/death
readback retains and accounts for the real body. Final native/CPP consistency
checks remain intact. These fixes are in `multiplayer_inventory.cpp` and
`multiplayer.cpp`.

Evidence and commands are under
`.local/host-migration-native-current-20261018/camera-matched/`, including the
initial archive diffs, both guard failures, debugger trace, repairs, explicit
death-count assertions and independent-executor runs. The executable's native/
RTC/layout cohort is retained explicitly; it is not current runtime delivery.

A direct current-worktree rebuild compiled the replay target but failed to
link a concurrently added camera-data mount. Its missing source registration
was added to `halo2_systems`. A second rebuild forced changed inputs, then hit
a scene-placement API changing across object compilation. Both failures and
source drift are recorded in `build-result.json` and `stage2/` under the same
evidence root. No foreign scene or rendering implementation was reverted.

Verification is now building `coherent-current/`: an immutable, checksum-listed
snapshot copied from the current worktree after a complete unchanged copy pass
(9,440 files, including the new native owners and fixes). Imported `assets`
and `analysis` reference directories stay shared. The official pinned peer
stack and enabled Discord SDK use their existing configured sources. This is
a fresh build of current captured code, without the earlier compatibility
projection. Its build and planned native tests have independent result files;
an active build is not a pass. Subsequent test-only independent-executor
coverage must be recorded separately from that snapshot's original tests.

The next integration boundary remains the complete host connection/input and
per-peer camera state at one native cut, followed by agreed survivor generation,
actual host/client/endpoint/controller promotion and voice handoff. Automatic
migration remains disabled. No master gameplay/media relay, backend deployment,
launcher replacement or claim of physical/WAN migration was added here.

## Owning host connection/input state, 2026-10-18

`LanHost::Continuation` now owns the logical connection table at a completed
TU1.5 cut. It retains every remote couch-seat mapping, selected team/profile,
nonce/cookie, accepted input and outgoing snapshot sequence, held controls,
received and consumed action serials, completed pickup/dual-wield durations,
completed vehicle intent, camera-jump serials, cached vehicle/death views and
each remote PC's complete private `NativeVehicleCamera` checkpoint. Peer idle
age is an owning duration; a paused import neither invents received traffic
nor counts paused time as execution of the resumed match. No socket, device,
pending transport packet, executor, callback or borrowed world is imported.

Capture requires the logical-endpoint transport, a nonzero admitted cut
binding, a quiescent native executor and a consistent completed native world.
Every remote endpoint and couch mapping must agree with the admitted roster.
`LanHost::resume` requires the same connection roster/configuration/fingerprint,
binding and native tick, plus the independently accepted full native salted
allocation/identity roster. It reconstructs private connections and cameras
without calling `join`, allocating a player or respawning a body. It verifies
that the native execution boundary did not change during preparation.

This API retains the existing same-host/local-view mapping. The caller still
has to restore the complete native world, revoke/quiesce the old authority and
isolate the transport generation. Preserved nonce/cookie values are connection
continuation values, not proof of election or protection against an unfenced
old host. There is no peer codec, changed membership, new host/local role,
client endpoint switch, controller/profile device acquisition or voice handoff
in this API. The runtime does not yet invoke it for automatic promotion.
The original native census's four local-player bindings are retained and
checked explicitly. A real native rebind to a different local player must be
rejected before connection reconstruction. TU1.5 `receive`, `broadcast` and
peer-count operations also enforce the native executor before touching the
peer table; the configuration I/O worker only exchanges immutable configuration
and queued packets. Off-executor calls to each operation are rejected.

`source_host_continuation_tu15` exercises two logical remote connections,
including four couch views, against six actual TU1.5 players. Three real native
protocol input messages coalesce before one native phase, leaving a pulse
backlog alongside already consumed interaction completions. The producer
captures eight actual receive/Match/state/camera/broadcast phases and is
destroyed. A separate native world imports the full cut on its own executor,
imports the owning host sidecar and replays the same input messages and native
calls. All complete emitted datagrams and the complete canonical world must
match, including snapshot sequences and camera results. The complete native
world is also equal immediately before/after host import, so this test rejects
rebirth or other native mutation by the connection reconstruction.

The test rejects empty/wrong bindings, wrong fingerprints, changed connection
names, missing/changed native admissions, wrong executors, raw UDP capture,
active source recording and different completed ticks. Failed preparations
must leave the complete imported world unchanged. Fresh keepalives keep the
input-continuation case independent of fixture execution speed and its separate
real-time idle/departure policy. The first fixture without those keepalives
failed a native argument comparison and is retained as an initial result.

The final owning-host case passes normally (3.90 seconds) and with the network
owner/fixture compiled under ASan/UBSan with leak checking (4.67 seconds).
Other native/generated/RTC dependencies in that latter executable are outside
the sanitizer claim. This is a logical transport test adapter, not encrypted
transport, actual consumer NATs, a host election or physical game/voice proof.
Production/fixture overlays, exact commands, source hashes and result files
are under `.local/host-migration-native-current-20261018/host-state/`.

The immutable captured-current-source build finished successfully in 1,599
seconds, with zero source-manifest drift. Its three original camera cases and
the cold direct-peer native replay case passed; the separately named retail
inventory CTest passed too. The original driver matched five tests because its
inventory pattern omitted the `retail_` prefix; the separate inventory result
must not be represented as part of that five-test run. Retention replay,
stronger independent-executor camera overlays and current-cohort real UDP
departure verification have their own result files; only completed results
establish acceptance. This captured build is not an installed runtime or a
claim of current live-worktree binary parity while other chats keep editing.

The stronger current-cohort camera overlay passes all three independent-
executor cases: Ghost driver (5.21 seconds), Warthog passenger (5.75 seconds)
and retained death observer (6.63 seconds). It compiles only the latest test
helper/call-site additions against the immutable source headers and its fresh
libraries. The original snapshot and source manifest remain unchanged. The
real UDP departure fixture also runs against this native cohort and the new
network owner; its exact graceful leave, five-second silent-peer timeout and
surviving native tick count are recorded in `host-state/departure-results.json`
and `departure-native-test.log`.

Two obsolete pre-fix diagnostic executables were losslessly gzip-compressed
after confirming they were not in use, then verified by decompressed SHA-256
before removing the redundant uncompressed copies. Their bytes and hashes
remain recoverable in `camera-matched/compressed-diagnostic-binaries.json`.
No source, original reference, installed launcher/runtime or other chat's
artifacts were removed.

Next required work is an explicit camera/host sidecar peer codec bound to the
same agreed complete-world cut, followed by survivor membership/generation
agreement and actual host/client/local-view/controller/voice rebinding. The
master continues to carry no mid-match gameplay or match voice here. Automatic
migration and the full ONLINE completion goal remain open.

## Explicit private replay scheduling and current recovery proof, 2026-10-18

The initial immutable-source CTest run passed its three camera cases and cold
peer replay, then failed the background-retention case before actual host loss.
Its native rejection was correct, but the test required a prefetched successor
that had not yet arrived. The Python harness then killed the other process;
that kill is not evidence of a gameplay host migration failure. Pre-recording
both native batches before publishing the first still reproduced the missing-
prefetch assertion. Both failing outputs remain under `coherent-current/` and
`replay-current/before-pause-*`.

`SourceAuthorityReceiver::prefetch()` now admits/decodes the bounded two-cut
stream while pausing native replay steps. Default `update()` still advances
replay as before. Full-cut loading/preparation can still update a private
candidate, so this is not a world freeze or permission to borrow its memory.
`SourceAuthorityReplayWorker::pause_replay(bool)` requests the same mode under
the existing mutex, applies it between completed native phases, wakes the
worker and acknowledges it through `state().replay_paused`. It exports no
executor, device, native pointer or host authority. A cancelled/stopped worker
rejects scheduling requests; paused retention drains after explicit resumption.
The added status bool occupies existing tail padding: the old/new status
structure remains 272 bytes with alignment 8 in the captured compiler cohort.

The tests now acknowledge the pause before publication, wait for both admitted
cuts, verify no native predecessor phase executed while paused, then resume.
The malformed predecessor and its actual prefetched successor are still both
rejected without a readiness receipt. For host-loss retention, the test first
admits both batches, resumes into partial native execution, pauses between
phases, requests owning retention and actually shuts down the old host. It
checks that the native world is still partial and not exportable, then resumes
and drains both batches after host loss. No input, clock, native result,
canonical world field or assertion of complete-world equality was removed.

The repaired current-cohort direct peer cases pass: cold replay validates 82
accepted native phases (111.01 seconds), and background retention validates
122 (118.41 seconds). Both retain independent eight-seat identities, four
recipient views, complete canonical worlds, malformed-chain/identity rejection,
full repair and ten actual surviving native ticks after original host-group
departure. No unfinished report is published. The receiver owner and fixture
also pass the complete 122-phase retention case under ASan/UBSan/leak checking
(118.02 seconds); native/generated/RTC libraries outside that instrumented
owner cohort are not covered by this sanitizer claim.

`replay-current/` records the exact source overlays, hashes, commands, normal/
sanitizer results, both initial failures and state-layout probes. These are
fresh components over the immutable captured-current-source libraries, not a
new installed runtime or renderer/WAN/hardware migration proof. Automatic
host/transport/controller/voice promotion remains disabled pending the actual
sidecar codec and agreed generation/role handoff. Gameplay and match voice
remain player-owned; no mid-match master simulation/media path was added.

Final combined integration also links the latest native network owner/executor
guards into those replay executables. Both actual two-process 122-phase
retention cases pass: normal (114.65 seconds) and network/receiver/fixture
ASan/UBSan/leak instrumentation (119.19 seconds). The latest actual UDP leave/
timeout regression runs 109 native ticks and passes in 6.06 seconds. All 9,440
original immutable-snapshot files still match their manifest after testing.
Current root syntax passes for the eight touched native/test units, followed
by a fresh network/host-fixture syntax check after the final executor guards.
`.local/host-migration-native-current-20261018/verification.json` records the
combined binaries, source/overlay hashes, initial failures, final results and
explicit acceptance limits. This completes the bounded recovery-state stage;
the agreed live host/client/controller/voice handoff remains unfinished.

## Paired world/host wire codecs and direct peer proof, 2026-10-18

The native authority codec library now carries explicit codecs for the owning
camera and host checkpoints. These are authored native-port formats, not an
Xbox System Link packet or a dump of C++ object layout. Camera records retain
all four 214-word observer banks, 80-word controller banks, full player salts/
identities, seated/transition flags, saved BSP/clusters and private RNG. The
host records all connection/view mappings, profile requests, teams, nonces/
cookies, transport sequences, held controls, received/consumed press counters,
interaction durations/intents, cached camera views and camera-jump counters.
All 27 `Controls` fields are explicit, including fields unused by the ordinary
remote-input packet. No native word or unknown observer/controller byte is
selected out of the checkpoint. Compile-time aggregate/array shape checks
reject unnoticed changes to the serialized owner structures.

The camera envelope is bounded to 8 KiB and the host envelope to 512 KiB.
Counts are bounded to sixteen admitted players/physical peers and four local
views. Strings, configuration blobs, profile arrays, optional markers, finite
input/presentation values and cached camera bases are checked. Checksums,
strict extents and trailing-byte rejection detect malformed encodings; they
do not authenticate a producer or prove honest gameplay. Independent source
identities and the accepted couch roster determine view ownership. Full-width
native words and wrapping counters remain exact values rather than receiving
invented gameplay interpretations.

`LanHost::ContinuationBinding` includes the complete room/epoch/membership/
host context and an exact world seal: cut ordinal, tick, byte extent and digest.
The host codec checks the native context binding, fingerprint, full native
allocations/identities, admitted endpoint mappings and original local view map.
`LanHost::resume` additionally checks every peer's original profile request and
join team against the restored native Match/admission before reconstructing
any connection. The original selected profile request is distinct from the
native sanitized profile/chosen name and from later live team membership.

`capture_source_host_continuation` obtains the actual host's own Match through
a private executor-checked accessor, captures both parts at one completed native
boundary and rejects changed execution stamps. It returns owning values that
can be encoded off the game executor. The whole container binds the host bytes
to the digest/extent of the actual complete-world encoding. Decode verifies
that association and cheap bounded host/admission metadata before decompressing
the large world. The producer's inner cut ordinal is separate from the outer
peer-transfer sequence. Native preparation and an independently agreed authority
decision remain necessary; no format/checksum is an election certificate.

Peer idle ages now have an explicit nanosecond representation. Reconstruction
anchors the recipient's real monotonic clock and retains the original observed
age as an offset, independently of local clock precision/epoch. Real traffic
resets that offset. Age addition saturates before signed overflow, so a valid
maximum received age follows the original timeout/departure path. The fixture
rejects an out-of-domain unsigned age and exercises maximum signed age through
actual native departure of four couch players, without numerical deaths.

The updated same-mapping native fixture decodes both world and host from the
wire container on another native executor, reconstructs connections/cameras
without birth, replays eight actual native phases and compares every complete
snapshot plus the full canonical world. It rejects every strict host/camera
prefix, invalid counts, duplicate local views, corrupt checksums, noncanonical
camera flags, wrong contexts/generations/membership/world digests/fingerprints/
identities, oversized envelopes and whole-container byte budgets. Substituting
world bytes and recomputing the outer checksum/world hash still fails against
the original host-side world binding. Ghost-driver, Warthog-passenger and death
observer cases now execute from decoded camera bytes and retain full canonical
world equality across independent executors.

`source_host_continuation_http` adds a separate test mode to the existing real
two-process queue/QoS/DTLS fixture. Both processes independently admit eight
source players with four couch seats per PC. The producer captures and sends
the whole world/host container over the actual authenticated peer UDP path.
The recipient performs native world preparation and host reconstruction before
acknowledging it. Four original native phases then travel over the same peer
transport. Re-execution compares the complete canonical world and exact emitted
native snapshot datagrams. The original host process actually shuts down;
the retained private world takes the original four-player departure path and
executes ten further ticks without ending the match or inventing deaths. The
fixture publishes no unfinished reports. Expected-world/packet files are
verification oracles, not inputs or a replacement transport.

The direct-peer case passes normally in 23.49 seconds and with selected native
network/camera-continuation/codec/fixture ASan/UBSan/leak instrumentation in
25.93 seconds. The final same-mapping host case passes normally in 5.35 seconds
and under that selected instrumentation in 6.31 seconds. All three camera
cases pass normally and under the selected sanitizers. The real UDP ordinary
leave/five-second silent-peer regression also passes: 106 native ticks in
6.19 seconds. Native/generated/RTC libraries outside the instrumented owner
cohort are not covered by the sanitizer claim.

Evidence, source manifests, exact commands, results and binaries are under
`.local/host-continuation-codec-20261018/verification.json`. New components are compiled against
the retained immutable Oct-18 current-source library cohort, with matching
headers and explicit overlays; the current root also passes syntax checks and
CMake configuration. This is not a new full live-worktree runtime installation.

The peer case intentionally retains the producer's view mapping and captures
outgoing resumed snapshots at a quiet local endpoint. The default runtime's
publisher/receiver still need adoption/negotiation of the paired format across
full cuts and replay successors, followed by agreed membership/cut/generation
selection and actual host/client/local-view/controller/voice rebinding. No live
authority switch, new master gameplay/media path, physical/WAN acceptance or
completion of the full ONLINE goal is claimed. Automatic migration stays off.


## Production paired host/world replay stream, 2026-10-18

The runtime now creates its selected shadow publisher with the actual native
`LanHost`. The publisher records an owning host/input/camera checkpoint at the
end of every native frame and retains the exact last checkpoint for each
bounded replay batch. Encoding uses owning values on the worker; it cannot
capture a later live host table beside an earlier replay. The existing
1024-phase/64-MiB logical-value queue budget also counts retained host/camera
values. These counters bound retained value storage, not allocator/RSS overhead.

A new capability bit negotiates paired host continuation. Runtime shadow
selection requires it alongside native replay and independent source-roster
confirmation. Native-only callers retain their previous stream format, with
`host_ready` false. Each paired full cut carries the complete world/host
container. Each paired replay cut carries its bounded native execution batch
and a sidecar bound to the exact replay bytes, accepted context, final tick
and phase sequence. The replay proof depends on actual re-execution from the
independently accepted base/previous cut; it is not a standalone world hash.
The receiver obtains the fingerprint from its own loaded game data.

The production receiver and background worker now decode both paired formats.
They validate the restored host against the actual native world/admissions
before acknowledging a full cut or completed replay batch. Temporary host
validation uses a transport which cannot send gameplay. Readiness is withheld
while native replay is incomplete; retention includes the exact validated
owning host state. Once paired, the stream refuses a native-only downgrade.
Preflight rejection preserves the prior validated pair; cancellation or a
failed native execution clears readiness. No socket, hardware device or native
executor borrower is exported by retention.

`source_host_continuation_stream_http` exercises the real production publisher,
receiver and background worker in two independently authenticated native
processes with eight admitted source players/four couch seats each. Seventy
actual native frames include two-cut transfer/decode prefetch while the
private replay executor is deliberately paused. The test withholds readiness
until replay completes, compares the complete canonical world and exact
historical host sidecar, rejects an otherwise valid full world with its host
owner stripped, verifies the previous pair remains unchanged, retains both
owners, destroys the worker and imports on a separate executor. The original
host process actually closes. The retained world takes the original four-seat
departure path and runs ten further native ticks without fabricated deaths,
match termination or unfinished report publication. Stale publisher exports
and cancelled publisher/worker readiness are also checked.

The paired production test passes normally (71.02 seconds) and with selected
camera/network/host-codec/publisher/receiver/session/fixture ASan/UBSan/leak
instrumentation (75.79 seconds). Native/generated/RTC dependencies outside that
instrumented owner cohort are not covered by this sanitizer claim. The host
codec fixture additionally covers paired-replay round trips, wrong contexts,
parents, sequences, fingerprints and identities, stale host ticks, content
substitution, malformed extents/checksums and whole-envelope byte budgets.
The standalone host/replay codec case passes normally in 21.32 seconds and
under the same selected sanitizer instrumentation in 23.65 seconds.

Both legacy native-only regressions also pass against the updated publisher,
receiver and network owners: cold 82-phase recovery in 147.19 seconds and
background 122-phase in-flight retention in 141.85 seconds. These retain the
wrong-chain/forged-identity rejection, full repair, four-view recipient rebind
and ten-tick continuation checks. The current network owner also passes real
localhost UDP graceful leave plus five-second silent-peer expiry: 108 native
ticks in 11.57 seconds, retained native identities/profiles/results/corpse rows,
no added numerical deaths and a surviving peer.

The first selected sanitizer run stopped when the workspace disk could not
provide an independent native source-memory snapshot; this was not a
sanitizer finding. Final runs use task-specific temporary snapshot storage on
`/mnt/SSD4TB`. Unused diagnostic binaries owned by this task were losslessly
compressed with decompression/SHA-256 verification; source, references,
installed runtimes and other chats' build outputs were preserved.

Current-root syntax, including the runtime integration, and CMake configuration
pass. Exact sources, overlay manifests, commands, results and acceptance
limits are recorded under `.local/host-stream-integration-20261018/`.
Components use the retained immutable Oct-18 current-source library cohort
plus explicit matching-header/source overlays. This is not a new full runtime
build or installation. The native peer fixture retains the producer view
mapping and a quiet resumed snapshot endpoint. Agreed cut/membership/generation
selection and actual host/client/controller/profile/voice role rebinding still
need implementation and live acceptance. Automatic migration stays disabled.
Gameplay and match/postgame voice remain on the existing player connections;
this patch adds no master gameplay or voice-media path.


## Host-loss role transition and disposable test storage, 2026-10-18

The native peer wrapper now has an explicit `resume_after_host_loss` path for
an independently certified predecessor cut and next authority context. It
requires the same room, the next epoch, an increasing membership revision,
a different admitted host, and removal of the failed predecessor host from
the active physical membership. The caller must first import the complete
world and bind its actual native local-view census to the promoted PC's couch
seats. This wrapper does not choose a host or manufacture that certificate.

The role transition removes the promoted PC from the remote connection table
and returns its owning local controls, pending/consumed press banks, interaction
completions, observer/controller checkpoints, cached views and camera-cycle
serials. The saved local input bank can drain queued pulses once before fresh
hardware takes over. Surviving remote peers retain their existing nonces,
cookies, send/receive sequences, complete input banks and camera state.
Owning camera values rebind only to the new context while retaining ordered
independently admitted full native player identities. No native player is
joined or respawned by this operation. Departure remains a separate original
native call; admitted PCR rows can outlive their physical peer's membership.

The client binds its original continuity context after actual native hello,
using the independently admitted physical/couch roster. A certified rebind
keeps its outgoing input/held/interaction serial banks and couch IDs, clears
incomplete snapshot fragments and the old endpoint, and admits the first
complete new-host snapshot at/after the chosen cut. That first snapshot can
roll presentation back from a later old-host snapshot to the accepted native
cut; subsequent snapshots follow the ordinary sequence/tick checks again.
The current runtime now records that initial client context. Live generation
certification, the DTLS epoch fence, controller/headset ownership and the full
runtime host/client switch remain separate work.

This is authored native-port protocol machinery. The retained TU1.5 promotion
evidence at `0x6AEE0` establishes preservation/promotion of the existing world
and result participants, rather than creating a new match. These APIs and
packet-state rules are not claimed as retail Xbox System Link wire parity.
All four native checks pass in the fresh matched-source rebuild: host-loss
role rebind (8.59 seconds), paired host/world codec continuation (10.19 seconds),
production two-process paired full/replay stream (54.83 seconds), and real UDP
graceful leave/five-second silent-peer departure (7.49 seconds). Selected
camera/network/paired-codec/publisher/receiver/session/fixture owners use
ASan/UBSan with leak checking; native/generated/RTC dependencies are outside
that instrumentation. The complete configure/build/test run took 903.61
seconds. Its source manifest and receipts are recorded under
`.local/host-role-rebind-20261018/verification.json`.

The role fixture independently imports the paired complete world/host on a
new native executor, rebinds actual native local views from the predecessor
host to four promoted couch players, and restores their entire private camera
and saved input banks. An existing three-seat `LanClient` keeps its connection
and follows the replacement wrapper for twelve native ticks without hello,
join or respawn. It rejects queued old-host snapshots, accepts rollback from
a later old-host snapshot to the accepted cut, preserves all native player
allocations/PCR rows and applies the original predecessor departure without
inventing a death. Queued grenade/melee pulses drain once, consumed interaction
completions do not repeat, and the new host can encode its next paired world
while retaining the departed host's original report row. Wrong room/epoch/
membership, view ownership, native identities/profile names and stale or
unbound client handoffs are rejected. The test supplies the generation decision
explicitly and uses a sequential logical adapter; live election/DTLS fencing,
actual runtime/controller/headset promotion and WAN acceptance remain open.

The goal checklist now explicitly requires removal of unused generated test
outputs after validation. CMake runs use `tools/run_tests.py`; one-off compiler
checks use `tools/run_test.py`. Both own disposable scratch/build directories.
The current-worktree authority-transfer test passes and its build directory
was verified removed. An initial one-off compile reached link, where previously
removed matched-library archives were unavailable; its scratch was removed.
The replacement is a complete disposable rebuild from the preserved Oct-18
source snapshot with explicit current source/header overlays. Its first
configuration lacked bundled release movies; movie packaging is explicitly
disabled for this native test build. Neither failure changes game behavior.

A separate cleanup checked live file use/mappings and both launcher-selected
runtime paths before removing 105 unused generated test binaries/objects and
two old private CMake configure trees owned by this work. It reclaimed
5,300,504,655 bytes. Small reports, commands, hashes, diagnostic logs, source,
original/reference assets, frozen source snapshots, launcher-selected runtimes
and unrelated active work remain. The exact deletion inventory is
`.local/host-role-rebind-20261018/cleanup.json`. No new large artifacts are
intentionally retained. After the successful run, the outer source copy,
inner CMake build tree, per-test scratch and native snapshot caches were all
verified removed. The failed one-off/configure scratch and successful current-
worktree transfer build are also absent. Current role sources still match the
rebuild manifest; current-root runtime syntax and whitespace checks pass.
This remains a native component/peer-wrapper validation, not a new full
current-worktree runtime installation or complete ONLINE acceptance.

### 2026-10-18: direct-peer agreement on replacement host and retained cut

`PeerHostAgreement` now supplies the independently testable generation decision
that the previous role-rebind fixture had injected. It uses a fixed physical
nonhost electorate, a strict greater-than-500-ms local silence boundary, and
bounded authenticated verification/readiness/prepare/promise/accept packets.
Two-phase majority agreement preserves the highest earlier accepted host/cut
when a proposer disappears before anyone learns its majority. Direct receipts
from distinct authenticated PCs establish the decision; claimed voter masks,
duplicate packets, couch seats and the predecessor host cannot add votes.
Promises/acceptances persist for the object's live-generation lifetime, and
local peer timeouts never shrink the denominator independently. This is authored
crash/partition machinery with honest nonhost participants; persistent rejoin,
Byzantine fault handling and cheating bans are outside its current guarantee.
See [protocol, source basis and integration contract](peer-host-agreement.md).

The component test covers a healthy majority retaining the original host,
strictly half the voters failing to promote, partition healing, all sixteen
physical peers, four predecessor IDs, an interrupted proposer carrying an
already chosen exact cut forward, all eight strict parsers, duplicate and
authenticated-sender rejection, 64 competing-proposer schedules, and concurrent
state/receive/poll/host observations. A fixture's initial periodic every-fifth
loss schedule permanently dropped the same retransmitted receipt; the trace
showed agreeing survivors and an isolated learner. Finite loss is now bounded
per route/type, with persistent partitions modeled separately. An initial
missing test include and a parser-dispatch rejected-count assertion were also
corrected. Small failure receipts remain available.

A separate test establishes four real ICE/DTLS/UDP endpoints, destroys the
original host endpoint, and lets the three survivors agree directly on the
same host, cut and successor generation while exchanging parallel 200-byte
player payloads. No STUN, TURN or master is involved in that test. Its declared
ready cut is fixture metadata, so it does not establish native world import,
rendered/controller handoff, audible voice, WAN/NAT or a few-frame transition.
Normal and ASan/UBSan/leak CTests pass for both new targets. The expanded parser
corpus also passes its final normal/sanitized CTest and the complete agreement
fixture passes TSan. Instrumentation covers the agreement and its fixture,
with the existing peer transport and RTC stack outside ASan/UBSan instrumentation.

The API requires a frozen owning paired native world before publishing ready;
it does not validate native state or promote the simulation itself. The
production session/runtime still need to observe accepted host traffic, retain
that exact cut, exchange agreement traffic, apply an agreed gameplay/voice
epoch fence, preserve controller/headset/profile ownership and run the existing
native host/client role transition together. Candidate loss after agreement,
additional membership changes, native/PCR identity continuity, sustained memory,
full runtime delivery and physical/WAN acceptance remain open. No installed
runtime or launcher was replaced during this component step.

All CMake validation uses `tools/run_tests.py`; compiler/race probes use
`tools/run_test.py` with owned scratch. The terminal build and recorded compiler
scratch paths are verified absent. Only small reports, logs and source hashes
are retained under `.local/peer-host-agreement-20261018/verification.json`;
there are no new retained large artifacts. The preceding cleanup's documented
5,300,504,655-byte recovery and preservation boundaries remain in force. Test
sources remain part of the regression suite. The full ONLINE goal stays active.

### 2026-10-18: peer-owned session epochs and native-completion media barrier

The actual `OnlineMatchSession` now has an internal peer-migration qualification
gate. It starts agreement only after initial running admission, observes accepted
host simulation packets rather than account/QoS/voice activity, and exchanges
generation-bound control packets on existing direct DTLS channels. An accepted
vote fences queued gameplay and encoded voice at application receive/send and
worker-drain boundaries. A final decision exposes the candidate/follower role
while retaining the predecessor endpoint until the native owner explicitly
finishes that exact independently learned decision. Invented or replayed
completion certificates cannot open media. The active host, epoch and physical/
couch membership publish together, with fresh transfer/admission/QoS/agreement
owners; old worker references stay bound to old objects. The full bounded chain
of at most fifteen predecessor agreements remains available to delayed learners,
sharing a four-packet budget after gameplay/media.

Source admissions now have a certified host-loss factory. It checks the previous
accepted roster digest and exact room/epoch/membership relationship, preserves
all original native/PCR allocations, and binds those immutable rows to the
successor context. Native identities therefore survive even if an intermediate
host disappears before its admission offer reaches a delayed peer. Actual
successor offers still control acknowledgment; changed replacement identities
cannot overwrite the retained baseline. No guest personal-stat publication or
admitted profile/name/slot is manufactured by this operation.

A four-session real socket/DTLS fixture with sixteen couch seats passes two
host session/socket/signaling teardowns, exact host/cut agreement, forged-
completion rejection, candidate and recipient native-completion barriers,
sixteen retained allocation rows, 16/12/8 connected couch seats and resumed
gameplay/encoded-voice packets. Its complete native cut and role completion
are declared metadata fixtures. It does not exercise native simulation import,
rendering, physical controllers/headsets, decoded audio-device buffer fencing
or WAN/NAT. Normal and selected owner ASan/UBSan/leak cases pass alongside the
ordinary measured-host session, source-admission and original variant-buffer
regressions. The shared unchanged variant encoder now has its own
`halo2_game_variant` library, so session validation does not build the entire
asset cache. The old native/legacy continuation fixture also links the session
library directly; its declared-world controls remain separately qualified.

The first session check found that the public continuity accessor omitted the
new peer-owned mode. It now exposes the peer-owned state; the fixture also
checks optional presence before reading it. A later review preserved postgame
voice retargeting before membership checks. Failure diagnostics record the
session stage, and the runner can repeat up to three independent server/session
processes with separate small receipts. Every account/native-ready value here
is a fixture; live identity and source-world acceptance remain separate gates.

The broader race checks intentionally retain their failures. The unsuppressed
C++ TSan run stops in system OpenSSL 3.6.5 certificate generation. A scoped run
excluding noninstrumented modules completes both host changes, while another
produces an intermittent ICE transport/candidate error. C/C++ instrumentation
then exposes SCTP's global `ipi_free_chunks` counter being read without atomic
access while another connection atomically updates it. A second pinned patch
changes both live reads to zero-add atomic reads using the dependency's existing
portable primitive. The first repeated process passes afterward; a subsequent
process exposes a distinct global tick race between `sctp_handle_tick` and
`sctp_timer_start`. Keep dependency-wide TSan, system crypto and intermittent
setup acceptance open. The patch applier preserves unrelated dependency edits.
Receipts are under `.local/peer-session-migration-20261018`.

Ordinary matchmaking still leaves the new gate disabled. Actual paired native
import/role change, survivor selection/retry policy, controller/profile/headset
and decoded-audio continuity, protocol negotiation, after-match authority and
admission repair, sustained memory, delivered runtime and physical/WAN acceptance
must be connected and verified before enabling it. The existing v12 coordinator
closes its room on explicit host leave; the gated mode keeps surviving peer
transport running, while after-match result admission is unfinished. No runtime,
launcher or backend deployment was replaced during this session step. Continue
the full goal and remove generated builds/scratch after validation.

The final seven-case selected-owner sanitizer run passes six cases: the new
two-handoff session, agreement, source identity carry, unchanged variant buffers,
and both old native/legacy continuation transport fixtures. The ordinary
measured-host fixture reaches child-process exit and LSan reports 4,848 bytes
in 18 OpenSSL allocations in each process; broader dependency cleanup remains
open. Earlier normal and sanitized five-case passes are retained as bounded
evidence, not substituted for this later failure. The full C/C++ race repetition
still fails on the SCTP tick after its first complete process succeeds.

Storage audit: all eleven private CMake build/scratch trees from this session
step, including failed checks, are absent. The compiler syntax scratch and
audit workspace are also removed. Small logs, hashes, per-attempt results and
failure diagnostics remain in the session receipt directory; no new large
artifacts are retained. `verification.json` records those paths and the exact
remaining acceptance boundary. Original assets/reference evidence, frozen
sources, launcher-selected runtimes and unrelated active work are preserved.

### 2026-10-18: callout flag synchronization and final RTC-owner cleanup

The reported `sctp_handle_tick`/`sctp_timer_start` conflict is a callout flag
race, rather than an unlocked global tick counter. The tick getter, update,
schedule and queue traversal already hold the timer queue mutex. Pending/active
macros and deactivation did not. A new small test links the actual pinned SCTP
library, initializes its nonthreaded runtime, and runs concurrent tick/callback
and pending/active/deactivate/start operations on a real callout. TSan catches
the flag read/write conflict before the fix in 0.13 seconds. The third reviewed
dependency patch routes those flag operations through a small helper protected
by the existing queue mutex; all other live flag accesses remain inside that
mutex. It retains layout and return masks, and executes callbacks with the
queue lock released. The same test passes TSan afterward in 0.20 seconds.
The helper uses the dependency's inherited private compile definitions and
separates its legacy atomic initialization symbol from C11 test atomics. An
initial mixed-header compile and a malformed patch hunk count were corrected;
their small diagnostics remain in the receipt directory.

The ordinary measured-host session's exit leak also reproduces independently
with full allocation unwinding: 4,848 bytes in eighteen allocations per process.
The stacks identify `EVP_RAND_CTX_new`/`RAND_bytes_ex` through `SSL_CTX_new_ex`
inside DTLS construction on the ICE connection thread. The pinned RTC global
initializer and thread pool are retained globals with an optional asynchronous
cleanup future. A shared runtime lease now counts wrapper owners and drains
that future after the last wrapper releases its channel/connection fields.
The first member is destroyed last during normal teardown and failed-constructor
unwinding. New acquisition waits for final cleanup under a separate wrapper
mutex; RTC weak-state callbacks do not acquire that mutex. Existing live rooms
prevent cleanup. This leaves shared crypto providers intact while RTC/ICE
workers stop, without resetting global OpenSSL state used by other services.

The originally failing two-process session now passes ASan/UBSan/leak checking
with full allocation unwinding. A new actual direct-UDP lifecycle test covers
independent live/departing rooms, failed TURN construction, preserved bidirectional
maximum-size payloads, final-owner shutdown/restart, and concurrent room cycles.
Those checks and the ordinary direct transport case pass (three of three).
Compiled C/C++ TSan also passes the real callout, concurrent runtime lifecycle,
and three independent four-session/two-handoff processes (six host losses total).
It explicitly uses `ignore_noninstrumented_modules=1`; the earlier system
OpenSSL-internal report remains outside this guarantee. The migration fixtures
still declare native readiness/role completion, so renderer, native world,
controller and audible headset acceptance are not implied. Full default-mode
migration, native/device integration and runtime delivery remain open. Evidence
and source hashes are under `.local/peer-teardown-20261018`.

The final broader C/C++ ASan/UBSan/leak run passes all nine focused CTests:
real callout regression, majority agreement, source identity carry, original
variant buffers, both old continuation transport modes, ordinary measured-host
session, runtime lifecycle and the two-handoff direct session. The migration
fixture keeps declared native-world/role inputs; no rendered or hardware
continuity is inferred. This supersedes the preceding six-of-seven sanitizer
boundary for the reproduced exit leak. System crypto-internal race qualification,
the historical intermittent setup diagnostic, full native/device integration,
memory/WAN acceptance and runtime delivery remain open. The full goal is active.

The teardown storage audit verifies all eight private CMake build/scratch trees
are removed, including the compile/configure and regression failures. The final
audit workspace is also absent. Small logs, commands, hashes and failure traces
remain under the teardown receipt directory; no new large artifacts are retained.
The tested owner hashes are unchanged. Original assets/reference evidence,
frozen source snapshots, launcher-selected runtimes and unrelated work remain.

## Native peer takeover owner, 2026-10-18

`SourcePeerHostTakeover` now joins actual native continuation with the session's
own learned peer decision. A receiver can consume its prepared native pair on
the same executor; a destroyed background replay worker can instead leave an
owning retained value for import/rebinding through a new pair on the receiving
executor. Both paths verify independent native admissions and local profiles,
restore couch views/input/cameras and apply original old-host departures without
new numeric deaths. Session media stays fenced until explicit one-shot
activation, which also verifies that no native execution occurred after staging.
The native host accessor is unavailable before activation.

Receiver exports and replay successors now compare the full execution boundary
saved at validation. A same-byte write at the same tick cannot retain a source
receipt; the new direct case rejects that export and repairs from a fresh
authenticated full cut. Twelve-player FFA also exposed a production codec bug:
admission placement groups0..15 were incorrectly limited to7. The admission
check now accepts the existing matchmaking range; actual native selected-team
requests remain independently limited to-1/0..7.

Four focused CTests pass with selected native network/camera/codec/receiver/
takeover/session owners instrumented by ASan/UBSan and with leak checking.
The native/generated/RTC/system code outside those selected owners is not
access-instrumented in this run. The new CTest runs two fresh native processes,
each containing three real direct sessions and twelve actual native players.
Each destroys the original native world and host session, learns the two-survivor
decision for its actual validated cut, promotes four local seats, rebinds the
existing four-seat client, then delivers twelve connected native ticks and twelve
encoded voice packets. The second case also destroys the background native
executor before importing its owning retained value on the receiving executor.
The other checks cover paired replay, prior native input/camera role rebinding
and the two-host-loss session barriers. No unfinished PCR is published.

Evidence: `.local/native-peer-takeover-20261018/verification.json`,
`effective-variant/report.json`, `last-tests.log`, source manifests and the
rebuild command/script. Initial sanitizer-link setup, the FFA range reproduction
and the fixture's duplicate effective-weapon override diagnosis remain recorded.
The source-manifest reconciliation records three controlled pre-compilation
overlays whose hash entries were overwritten by stale retry bookkeeping; those
source files did not change after their final copy into the tested cohort.

The final 2,662,993,859-byte supervised workspace, all private builds and test
scratch are removed. Bounded reports/hashes/diagnostics remain; no new large
artifacts are retained. Original reference assets, frozen source and installed
runtimes are preserved. This is internal native/transport qualification with
explicit candidate selection/activation, not ordinary rendered migration.
Renderer borrower adoption, physical controllers/headsets, decoded audio
fencing, automatic candidate/compatibility policy, after-match recovery,
sustained memory, WAN acceptance and full runtime delivery remain open. The
complete ONLINE goal stays active.

## Retained audio and runtime quiescence, 2026-10-18

`GameVoice` now owns asynchronous delivery-generation retirement. A fence closes
input/output delivery immediately without SDL calls or a worker join on the
network/native executor. The audio worker completes any in-flight device call,
clears queued capture/playback, destroys old talker decoders/sequence state and
recreates the microphone encoder before acknowledging. Local stamps survive
owning microphone/receive batches but are not placed on the wire. Old held
batches and old voice frames already drained by the transport worker cannot
cross into a successor. Enabled/microphone mute, manual mutes, blocks and the
same device handles survive. `finish_peer_migration` waits for retirement;
native takeover activation can retry while its prepared world stays unchanged.
Postgame voice-leader/membership retargets also retire the prior local route.

Both runtime loops now stop source frame publication/recording, simulation and
presentation calls during `AwaitingWorld`/`Migrating`/`Rebinding`. The windowed
caller services quit/resize/hotplug and input releases, stops rumble and drains
relative mouse motion. It resets time/voice baselines after a separately
completed role resumes. This is current-source syntax-checked scaffolding for
renderer adoption; a delivered rendered takeover is still unproven.

Final checks: five compiled C/C++ ASan/UBSan/leak CTests pass, and three compiled
C/C++ TSan CTests pass. Both session tests run three independent processes in
TSan. `ignore_noninstrumented_modules=1` leaves system crypto/codec internals
outside race qualification. The actual audio test holds decoded PCM in a queued
synthetic device while real peer agreement completes, verifies that native
completion waits, then requires fresh gameplay/voice packets/decoded stereo
after **each** of two socket/signaling host losses. It rejects held old microphone
and receive batches and keeps the same device ownership/preferences. The unit
case also holds capture in flight, verifies opt-out and closed/foreign stamps,
and restarts a lower-sequence speaker after decoder retirement. All audio is
synthetic; no physical microphone or headset is opened. Native world readiness
and role completion in the session case are declared fixtures, separate from
the previous actual twelve-player native-world evidence.

Receipts and source hashes: `.local/voice-handoff-20261018/verification.json`.
All five private CMake build/scratch trees and compiler probe scratch are removed;
bounded reports remain, with no new retained large artifacts. Default migration,
renderer borrower adoption, physical controllers/headsets, sustained memory/WAN,
after-match authority repair and full runtime delivery remain open. The full
ONLINE goal remains active.

## Foreground runtime ownership, 2026-10-18

`MultiplayerRuntimeAuthority` now retains the initial map/cache resources while
owning the current Match/host/client, source publication and recovery stream.
Both runtime loops resolve their live owners through it and hold a frame scope
through simulation/presentation work. Service, recovery, staging and activation
cannot replace that owner during a live frame; early exits and unwinding release
the recording before the frame guard. A fenced session cannot begin a new frame.

The same owner is exercised by the native takeover fixture. Its two fresh cases
reject cross-executor access, nested frames and staging during an outstanding
frame without consuming the caller's unique candidate. Private staging keeps
the initial replica and client visible until exact native activation, then
adopts the new Match/host together while keeping the resident cache valid.
Four local seats and an existing four-seat client continue for twelve actual
native ticks with twelve encoded direct voice packets after old-host teardown.
The background case destroys its replay worker before foreground import.

Five selected-owner ASan/UBSan/leak CTests pass: native takeover, prior native
role rebind, paired full/replay stream, direct two-handoff synthetic decoded
audio, and queued audio retirement. Generated/native/RTC/system internals remain
outside this instrumentation. The current-root runtime also passes warning-clean
syntax checks. Native camera/input/renderer cache adoption in the windowed loop,
automatic survivor policy/compatibility, default migration and full delivery,
hardware/WAN and sustained memory acceptance remain open.

The first two harness builds exposed missing sanitizer-runtime propagation from
the selected-source voice library to its standalone test. The disposable CMake
harness now propagates those runtimes; bounded diagnostics and the exact harness
hash change remain in `.local/runtime-authority-20261018`. The final source
manifest matches the tested runtime-owner and fixture files. The outer test
supervisor removed all 2,711,637,063 bytes of generated workspace files, including
the private source copy/build/cache, and the recorded compiler scratch is gone.
Original/frozen inputs and unrelated live work remain intact. No new large
artifacts are retained. See `verification.json` in that report directory.

## Measured survivor preference and executor retirement, 2026-10-18

The session can now propose a replacement without a caller-specified PC. It
filters the independently ready native cuts through actual reciprocal survivor
measurements and preserves the existing authored loss/upload/RTT/peer-ID order.
Only the predecessor host is removed from scoring; another local timeout cannot
shrink the agreed membership. One remaining physical PC needs its own native
cut and one-voter quorum, without a fabricated bandwidth or NAT measurement.
Gated QoS now shares the native authority/voting revision, fixing the mismatch
exposed by strict generation comparison. Legacy setup keeps its prior scheme.

Foreground runtime recovery now polls worker retirement after receiving the
owning export. `SourceAuthorityReplayWorker::stopped()` acknowledges that its
receiver, decoder worker and native worlds have actually been destroyed on the
worker executor. The native fixture uses this runtime recovery path and then
selects the replacement from measurements before its exact native takeover.
Both fresh twelve-player native cases retain local input/camera/allocation state,
four promoted seats and an existing four-seat client for twelve further ticks
and encoded voice packets. This does not qualify rendered/device adoption or
the elapsed time of subsequent foreground world reconstruction.

Final checks: seven native-cohort selected-owner ASan/UBSan/leak CTests, four root
C/C++ ASan/UBSan/leak CTests and two root C/C++ TSan CTests pass. The measured
session runs three independent TSan processes, each covering three host losses
and 16/12/8/4 seats; its native readiness/role remains declared fixture metadata.
System-library internals are excluded from TSan, and native/generated/RTC/system
internals are outside the selected native-owner instrumentation. Actual native
world checks remain separate from decoded synthetic audio and physical devices.

Evidence: `.local/survivor-selection-20261018/verification.json`, final source
manifests and bounded diagnostics. The 2,731,636,383-byte supervised native
workspace, other private CMake trees and compiler scratch are gone. No new large
artifacts remain. Windowed adoption, automatic scheduling/retry, false-alarm
readiness lifecycle, additional agreed membership removal, default compatibility,
after-match recovery, sustained memory, hardware/WAN and delivery remain open.

## Retained local input and resident scene lifetimes, 2026-10-18

The foreground authority now exposes its promoted native camera bank and owns
retained local input delivery. An active native frame consumes the saved serials
before new hardware admission. A repeated poll at the same simulation tick
returns the same controls without consuming another serial; ownership clears
only after the native step. A neutral retained bank still owns its first tick.
The per-view renderer selects the promoted camera when present. The windowed
input suspension/drain and full rendered adoption handler are not wired yet.

The native fixture sends three real authenticated DTLS input packets containing
switch-grenade pulses for four couch seats, consumes one normally before the
paired cut, then retains two pending input ticks. Both same-executor and
background-retention cases consume those ticks after exact activation, reject
out-of-frame delivery and preserve serials on repeated same-tick polls. They
continue twelve actual native ticks with an existing four-seat client and twelve
encoded direct voice packets. Fresh hardware/controller and audible headset
acceptance remain separate.

Machines, props and scenery now support completed-frame presentation lifetime
retirement. The operation invalidates handle, clock, pose, light, view and LOD
history without reallocating resident material/geometry resources. Rebinding
uses the actual current source object and region selection. The machine case
rolls tick131 back to tick0 with the original salted handles and preserves
authored permutations and material indices. Prop checks compare complete vertex
streams across the existing map corpus. The animated scenery case rolls tick100
back to tick0 and matches an independently prepared scene. These are CPU scene
checks with original assets, not GPU image or full rendered takeover proof.

Ten selected-owner ASan/UBSan/leak CTests pass, including those three scene cases,
both native takeover paths, paired stream, role rebind, measured migration,
encoded/decoded synthetic voice handoff and mesh quality. Native/generated,
RTC and system interiors remain outside this instrumentation. The current-root
runtime also passes syntax checks; the native/renderer tests use the coherent
frozen ABI plus explicitly hashed current owner overlays. Critical current
owner/test hashes and resident geometry/renderer dependencies match that cohort.

The first configuration omitted a required peer-runtime test file. The next
scene run exposed three original reference fixtures missing only from the
private source copy. The final retry copied those unchanged inputs and passed
all comparisons; bounded failure diagnostics remain alongside the final reports.
Evidence: `.local/rendered-handoff-20261018/verification.json`.
The outer supervisor removed 27,574 generated regular files totaling
2,961,309,972 bytes, including its source copy/build/cache; all six recorded
compiler scratch trees are absent. Original fixtures/assets, frozen source,
selected runtimes and unrelated work remain intact. No new large artifacts
are retained. Full windowed adoption, effects/GPU retirement, automatic
scheduling/readiness lifecycle, memory/hardware/WAN, after-match recovery,
default compatibility and delivery remain open. The full ONLINE goal is active.

## Automatic session elections and paced retries, 2026-10-18

The direct session's network worker now calls the agreement driver using its
current measured, native-ready survivor preference. Frequent service calls do
not restart an in-flight ballot. Fixed-electorate proposer order staggers initial
attempts; retries wait for a bounded interval and genuine protocol progress.
Repeated old prepare/promise/accepted messages do not keep a dead proposer alive.
The existing two-phase agreement still preserves its highest accepted value,
fixed physical majority, healthy-host veto and strict silence beyond 500 ms.
The new retry timers are client policy, not claimed original Xbox constants.

The session fixtures no longer make a positive proposal call. Ordinary and
decoded-audio cases retain the healthy ready-host veto. The measured case holds
readiness until predecessor telemetry expires, confirms that no native cut has
been invented, then requires the worker to elect and later reach the sole-PC
survivor. Native role completion remains declared fixture metadata in those
transport cases. Each measured and audio case runs in three fresh TSan processes.

Both actual twelve-player native cases now use that automatic election after
the original host's real socket/session teardown. Their retained native cut is
published only after teardown so long fixture-only validation pauses cannot
elect prematurely during setup. Exact native staging/activation and client
rebinding remain explicit completed-frame operations. Four promoted couch seats,
an existing four-seat client, original allocation/result identities, retained
input serials and camera state survive twelve further native ticks and twelve
encoded direct voice packets. Readiness, foreground import, renderer/input/
device adoption and activation are not yet an automatic runtime coordinator.

Final checks: six broader compiled C/C++ ASan/UBSan/leak CTests, three broader
compiled C/C++ TSan CTests, expanded agreement ASan/UBSan/leak and unsuppressed
owner TSan checks, and seven selected-owner native CTests pass. Automatic unit
coverage includes sixteen peers, 64 varying-preference schedules, four-packet
polling budgets, delayed replies, failed proposers with replayed old prepares,
accepted-value continuity and concurrent scheduling. Broader TSan excludes
system-library internals; generated/native/RTC/system interiors remain outside
the selected native instrumentation. The native cohort retains its coherent
frozen ABI plus explicitly hashed current owners. The expanded agreement unit
fixture is built separately from current root source.

Evidence: `.local/automatic-election-20261018/verification.json`. The outer
supervisor removed 27,199 generated regular files totaling 2,732,562,234 bytes,
including its private source/build/cache. All six root build/scratch trees are
also absent. Approximately 409 KB of bounded receipts, commands, source hashes
and diagnostics remain, with no new large artifacts. Original assets, reference
fixtures, frozen inputs, selected runtimes and unrelated work are preserved.
Automatic native retention and false-alarm readiness lifecycle, full rendered
adoption, further agreed membership changes, compatibility/default delivery,
after-match repair, sustained memory, hardware/WAN and other ONLINE acceptance
requirements remain open. The full goal stays active.

## Native preparation and host/client publication, 2026-10-18

The foreground runtime owns its replay/takeover gameplay selections without
retaining presentation callbacks, desktop or prepared worlds. Its preparation
method polls replay-worker retirement, reconstructs the owning value and stages
the exact locally learned host decision once. Foreground reconstruction now
imports the producer view bank into one private native world, validates full
session admissions and selected recipient profiles, then binds recipient views.
This is the existing qualified native role-rebind sequence; a second temporary
import world is no longer needed. Construction is still synchronous and has no
qualified frame-time or sustained-memory bound. Replay workers still use their
separate candidate/workspace pair.

Session completion now checks the certificate, admissions and audio retirement
and allocates all successor owners before an optional native commit callback.
Refusal leaves the transport context unchanged. Host activation moves the new
world, authority and host role before publishing that context, while old client
destruction occurs outside the session lock. The surviving client prepares an
opaque endpoint change without changing its native role; it commits that change
before transport publication and retains the same client, local slots, input
serials and resident world. Old receive assemblies are cleared at commit.
Stale/foreign prepared commits are rejected without consuming the preparation.
Each runtime uses its own learned certificate; consensus requires the same
world/cut/generation, not identical local ballot/voter receipt sets.

Both runtime pause loops now stage these native roles. They still wait for
rendered/input/device borrower adoption before activation. Actual native tests
exercise pending idempotence, later menu selections, callback-borrower expiry,
wrong profiles/views, refused adoption without epoch publication, stale/foreign
endpoint preparation and restart of a survivor's failed old shadow. They retain
four promoted local seats, an existing four-seat client, native allocation/PCR
identities and saved input/camera state for twelve additional native ticks and
twelve direct encoded voice packets. These are still explicit adapter-free
activation tests, not a delivered windowed migration.

Seven final selected-owner native ASan/UBSan/leak CTests pass, following two
earlier passing incremental native cohorts. Three root C/C++ ASan/UBSan/leak
session CTests pass. Two root C/C++ TSan CTests also pass, each measured/audio
case repeated in three fresh processes with system-library internals excluded.
Native/generated/RTC/system interiors remain outside the selected native-owner
instrumentation. Three disposable current-root syntax checks pass, including
the runtime loop integration. Selected current source hashes match the native
cohort's pre-build manifest; the native ABI remains the coherent frozen base.

Evidence: `.local/native-prepare-coordinator-20261018/verification.json`.
After every final CMake/CTest step passed, the outer recipe terminated with143
while awaiting finish. The supervisor removed its owned native workspace. Both
root private build/scratch trees and all three compiler scratch trees are also
absent. The optional final trace copy and byte-count measurement were not made;
passing step reports, bounded test summaries and source hashes remain. No new
large artifacts are retained. Original assets, references, frozen source,
installed runtimes and unrelated work are preserved. Automatic readiness and
false-alarm handling, complete rendered/input/HUD/effects/GPU adoption, runtime
activation, broader modes/generations/PCR, timing/memory, hardware/WAN, default
compatibility and delivery remain open. The full ONLINE goal stays active.

The preceding input audit made this gap explicit: the promoted bank comes from the sealed
host cut, while the old `LanClient` can own newer outgoing press counters.
`LanClient::send` retains those counters, but current host activation retires
the client without importing them. Existing native checks send their queued
pulses before capture, so they do not establish post-cut local-command survival.
Carry and validate the latest local input ownership before retirement, including
held controls and completed interactions, and add a native delivery case before
enabling ordinary migration.

## Promoted input after the sealed cut, 2026-10-18

The old native client now retains one latest owning encoded input packet. Before
retirement, runtime activation merges it into the independently validated host
bank after checking session/generation, endpoint, fingerprint, nonce/cookie,
physical couch owner, selected profiles/teams and monotonic action serials.
Parsing and every guard precede mutation. Delivered counters, native world and
camera state are unchanged. The pending takeover retains its immutable sidecar
until successful activation; repeated refresh and refused adoption do not consume
or duplicate input. No native pointer, hardware owner or world crosses threads.

Both actual twelve-player native tests now produce three pre-cut input pulses,
consume one, seal the cut, then produce two more pulses plus changed movement,
look/zoom and a completed interaction without another predecessor receive.
Promotion delivers all four pending action ticks exactly once for every local
seat, including one saved interaction duration/neutral target. Repeated polling
within a native tick is idempotent; fresh input ownership returns only after the
last carried tick is stepped. The existing four-seat client follows twelve more
native ticks while twelve direct encoded voice packets arrive. The lower native
role fixture rejects foreign contexts/owners, wrong slots, premature consumption
and malformed owning packets without changing delivery state.

Seven selected-owner native ASan/UBSan/leak CTests pass in 83.94 seconds. The
cohort uses the frozen coherent native ABI with hashed current owners; original
generated/native/RTC/system interiors remain uninstrumented. A separate disposable
current-root syntax check passes, including the runtime loop. No new TSan or
hardware/WAN claim is made for this step. Evidence:
`.local/promoted-input-continuity-20261018/verification.json`.

The successful supervisor disposed of 27,199 generated files and 2,738,244,560
logical bytes of copied source/build/cache. Compiler and cleanup scratch are also
absent. An audited 44 unused old test executables in `build-multiplayer` reclaimed
5,703,429,208 more logical bytes. Both launchers select the preserved native
runtime; no sources, libraries, assets/reference inputs, frozen evidence or
concurrent work were removed. Only bounded reports remain, with no new retained
large artifacts. The full goal stays active: actual hardware input pause/drain/
release, full renderer/HUD/effects/interpolation/GPU/device adoption, readiness
and false-alarm lifecycle, runtime activation, broader modes/generations/results,
timing/memory, delivery and physical/WAN checks remain open. Default migration
remains gated.

## Windowed input ownership during migration, 2026-10-18

The runtime now calls `continue_local_input()` before consuming fresh controls
on every command tick. During retained delivery it uses that bank's movement,
look, zoom and interaction target, excludes scripted/prompt replacement, and
does not consume device-edge/hold queues. After the final carried native step,
the inner catch-up loop stops; hardware ownership resumes on the next frame.
The transport fence and input drain share one pause clock. SDL focus, device
changes and exit events still run; relative mouse motion is discarded. Pads
retain their old edge baseline and active hold elapsed time, while a changed
controller instance clears only its own old hold. KBM tracks physical state
without queuing paused taps, freezes existing durations, and delivers an old
completion before a newly held request, including events between resume and
the next command. These are native-adapter policies, not newly claimed original
Xbox migration constants.

The actual SDL event-queue suite passes 287 checks with the adapter and settings
compiled directly under ASan/UBSan. This focused target no longer rebuilds the
entire native world just to validate the device adapter. Current-root syntax
passes separately. A full current-source freeze, followed by the latest adapter
and fixture updates, compiled all dependencies and the windowed runtime. Its
TU1.5 windowed normal-input case passes with 154 native command observations,
an actual attached neutral virtual controller, movement/crouch/jump/fire, reload
transfer, mouse look, focus and overlay checks. It is not a rendered migration
test and does not qualify four physical controllers or headsets.

One initial build ended with143 before linking; its owned tree was removed,
without an established termination cause. The next full build linked; requested
Khronos GPU validation could not run because the layer is unavailable. An
explicit private CTest environment override then used normal Vulkan. That run
exposed an old reload assertion against the separate C++ presentation timer.
The preserved trace shows exactly one R press, magazine52 ->60 and reserve120
->112 while that scalar stays0. The test now requires unchanged weapon identity,
conserved total ammunition, exact reserve-to-magazine transfer and one R press.
No gameplay reload state, timing or native owner was changed. Trace writing now
precedes assertions so failed runtime checks retain bounded diagnostics.

Final evidence: `.local/runtime-input-handoff-20261018/verification.json`.
Selected device/fixture hashes match the successful windowed cohort. Current
runtime input wiring matches its frozen translation unit; the only subsequent
runtime difference is an unrelated concurrent material include, checked by the
separate current-root syntax pass. The successful supervisor removed 34,345
generated files totaling 1,063,983,811 logical bytes. Its source/build/cache,
earlier interrupted workspace and adapter/compiler scratch are absent; only
bounded traces, source hashes, recipes and diagnostics remain. Original/reference
inputs, installed runtimes and unrelated work remain intact.

The next full integration must still retire/rebind presentation, interpolation,
HUD/effects/GPU query rings and device borrowers before activation. Automatic
readiness retention/false-alarm lifetime, actual rendered migration, current-ABI
native migration coverage, physical couch/headset/WAN and sustained timing/memory,
default compatibility/delivery and other ONLINE requirements remain open. The
full goal stays active and default migration remains gated.

## Original query retirement and replacement GPU ownership, 2026-10-18

Original20700's retirement branch loops over the request high-water count and
calls original20E30. That complete integer body clears the selected request's
three ring bits and32-byte record, then publishes its originalFF80 free tag.
The incoming-host implementation now invokes that unchanged body through the
real TU1.5 executor after local camera/profile conversion and predecessor
departure, before sealing the new native adoption boundary. It checks the
native quiescent executor and complete bounded spans first. A malformed count
cannot partially change the bank; unused bits and the high-water count retain
their original semantics. The enclosing object-runtime method does not begin
an unrelated model view. This is an explicit native handoff policy using a
recovered original primitive, not a claim to have recovered an Xbox migration
entry point.

SourceLensQueryFrameTu15 can now bind a separately prepared retired CPU bank at
a completed renderer frame. All replacement private ABI storage allocates
before the old GPU owner retires. Only a read-only incoming identity is retained
until begin acquires the actual native borrower; the old source pointer,
generations and results do not survive. Invalid bank, foreign executor and
duplicate identity checks run before device retirement. A failed allocation
does not destroy the old private owner. The device lifetime remains guarded by
the existing renderer lease and actual GPU wait/retirement.

The new oracle executes unchanged pinned English TU1.5 20700 ->20E30 over16
declared input formulas: counts0/1/2/31/32/33/510/511, each with two seeds. Every
instruction's bytes are authenticated, writes are confined to the declared bank
and caller stack, and complete16KB output hashes are retained. The5,148-byte
fixture contains formulas/hashes, not raw banks or executable data. The native
test recreates those exact inputs in a real configured one-player world,
compares each entire bank with the original hash, checks all other committed
source bytes except its declared original-ABI caller stack, repeats retirement
idempotently and rejects malformed/foreign execution.

Eight current-source native/session selected-owner ASan/UBSan/leak CTests pass.
The cohort is a complete current-source freeze plus recorded owner updates,
without the previous historical native ABI overlay. Both actual twelve-player
promotion paths additionally carry33 controlled CPU-only pending requests through
the authenticated paired cut, verify retirement before role adoption and retain
the existing four-seat input, allocation/results, twelve native ticks and direct
voice checks. These request rows are declared CPU controls, not fabricated GPU
completion observations.

The actual RTX5080 GPU assertions and Khronos validation pass:45 authored
original quads, unchanged color/depth/stencil, visible/occluded/empty samples,
rejection preserving the live GPU, old-result retirement, foreign/duplicate
rejection and fresh generation1 reuse after old generations4/5. An initial
handoff fixture had inherited the preceding area test's removed world occluder;
restoring that declared occluder fixed its coverage comparison without changing
production rasterization or the expected samples. The temporary validation
package came from the configured repository, matched its SHA-256 and the
installed loader version, and lived only in owned scratch.

The GPU process CTest remains failing on shutdown LeakSanitizer reports. The
isolated NVIDIA run reports27 allocations/3,692 bytes after all functional
assertions pass. A minimal independent Vulkan create/enumerate/wait/destroy
control with no Halo, SDL or query pools completes and reproduces27 allocations
(byte totals vary with DBus messages). A separate live-device control finds24
allocations/1,098 bytes in NVIDIA glcore/glsi before shutdown, followed by the
same shutdown families. SDL's documented isolated-process DBus cleanup hint
does not remove this baseline. No leak suppression or leak-check disabling is
used, and no leak-clean GPU-process claim follows from these results. Generated
native/RTC/system interiors remain outside the selected owner instrumentation.

Evidence: `.local/lens-query-handoff-20261018/verification.json`. The final
supervisor returns8 from the recorded GPU leak-check failure after all native
tests pass; its cleanup completes and removes34,502 generated files totaling
1,512,947,527 logical bytes. Source/build/cache, raw captures, the downloaded
layer package/extraction and all recorded compiler/control scratch are absent.
The small original hash fixture is required reference evidence and remains;
original assets, frozen inputs, installed runtimes and concurrent work are
preserved. Full windowed scene/HUD/effects/interpolation/device adoption, actual
rendered migration, readiness/false-alarm lifetime, sustained memory/timing,
physical couch/headset/WAN and the other ONLINE gates remain open. Default
migration stays gated and the complete goal stays active.

## Dormant lens residency and final-draw ownership, 2026-10-18

The normal client path previously prepared no native flare bitmap materials.
A later promotion could not add those images after renderer construction had
fixed residency. Clients admitted to the gated peer-migration path now prepare
the same authored map lens resources before upload. Their native world/query/
draw producers remain dormant, guarded independently of resource residency.
The ordinary ungated client path and ONLINE/clan Vista asset selection retain
their existing behavior.

SourceLensDrawFrameTu15 now supports an explicit completed-frame rebind to a
separately prepared retired query bank. The replacement copies only the resident
resource catalog, bitmap aliases and material identities. Its private SDK bank,
bitmap headers, FIFO, source borrower, camera/projection, draw data and counters
start fresh. Allocation and retired-bank admission precede publication, so a
rejected preparation leaves the old owner intact. The read-only incoming
identity survives an inspection scope; the next actual native view acquires
the borrower and runs the existing original header initialization. No source
bank is mutated by rebind itself, and the native adoption boundary remains
valid. This is native presentation policy using original routines, not a
recovered Xbox migration entry point.

The new GPU-backed owner case uses two real configured TU1.5 worlds. Thirteen
native views and six rebindings cover a cold owner, repeated source switches,
unchanged 39 resident material slots, preserved sealed native boundaries and
rejection of pending banks, active frames, foreign executors, duplicate worlds,
wrong borrowers and an expired renderer. Its cold owner represents uploaded
client resources; it does not execute the normal ONLINE client window loop.
The retired rings are empty, so this qualifies original bitmap/header lifetime,
not active visible flare scheduling during migration. The separate original
GPU regression passes 45 complete callbacks, 25 emitted quads and 46 actual
bitmap images, with repeated visible pixels, alpha/depth gates and unchanged
depth/stencil. Both Khronos validation logs are empty on the RTX5080.

The private halo2_native executable builds from a complete source freeze with
recorded current dependency updates. Later concurrent main/campaign and test
updates are listed separately in the receipt; this qualifies the recorded
cohort, not the subsequently changing whole worktree or an installed build.
Six CPU/reference CTests pass: original
draw geometry/callback/setup/parent gate, native query retirement and campaign
camera compatibility. Selected final-lens/frame/execution owners use ASan/UBSan
with leak checks; generated/native, RTC and system interiors are uninstrumented.
Both GPU functional reports pass, but their process CTests still fail LSan:
27 allocations / 3,740 bytes for the owner case and 27 / 3,676 for the original
GPU regression. The preceding standalone Vulkan controls reproduce those
driver families without Halo, SDL or query pools, including live glcore/glsi
allocations. No leak suppression or leak-check disabling is used; these are
not leak-clean GPU processes. The overall CTest/supervisor status remains 8.

The first CPU run placed its independent snapshot cache on RAM-backed storage
and was rejected. An owned disk cache resolves that check without changing
production snapshot behavior. A concurrent source refresh required newly
referenced test files in the private CMake cohort; copying those files resolves
configuration. Earlier diagnostics are retained alongside the final reports.
Evidence: `.local/renderer-lens-adoption-20261018/verification.json`.

The supervisor removes 34,761 generated files totaling 1,519,555,707 logical
bytes (1,616,343,040 allocated bytes), including the 99,214,320-byte executable,
source copy, private build, shader objects, captures and temporary validation
package/libraries. Separate disk-cache/compiler workspaces are also absent.
Only bounded source hashes, commands, functional reports and failure diagnostics
remain; no new large artifacts are retained or installed. Original assets,
reference evidence, frozen sources, selected runtimes and concurrent work remain.

Before activation, the enclosing adopter still needs coordinated allocation of
all replacement owners before irreversible GPU retirement. It must rebase the
muzzle/world-effect clocks and event cursors, HUD shield/winning/sensor histories,
scene/model/first-person interpolation, and source model/particle borrowers.
Existing muzzle/world-effect clocks explicitly reject backward ticks; switching
the host pointer alone cannot survive an earlier accepted cut. Full rendered
host/client activation, automatic retained readiness and false-alarm lifetime,
sustained timing/memory, hardware/WAN and the remaining ONLINE checklist stay
open. Default migration remains gated and gameplay/match voice remain direct
and player-hosted.

## HUD histories, device event boundary and pending sound cues, 2026-10-18

MultiplayerHud::reset_lifetimes is a completed-frame native presentation policy.
It clears the private shield/winning clocks, motion-sensor trails and sampled
tick/unit keys, clock and speaker text. It preserves uploaded bitmap/font/
material identities, all local keyboard/controller prompt bindings and authored
donor sampling offsets. Those offsets describe static Vista shield fill/outline
alignment, not simulation history. The enclosing adopter must observe/animate
every local view and invalidate its outer radar upload tick before drawing,
including a same-tick replacement retaining the same salted unit identity.
No authoritative vitality, score or input is changed. No original Xbox migration
HUD reset entry point has been recovered or asserted here.

NativeDeviceEventCursor::rebase validates the incoming journal at its accepted
cut before changing the delivered serial. Accepted requests are baselined without
replay, while a later event may reuse a discarded-future serial. Repeated/empty
adoption is supported, and invalid/future journals reject without changing the
old cursor. Authentication/generation agreement remains the enclosing owner's
responsibility. NativeEffectSounds::retire_pending clears only unsubmitted
discarded-world cues, preserving authored definitions and subsequent requests.
This does not reconstruct accepted active effects or stop already playing audio.

Eleven focused CTests pass in the full source freeze plus explicit HUD/test
updates. Selected HUD/device/cue owners and their new tests use ASan/UBSan and
enabled leak checks without suppressions; native/generated and system interiors
remain uninstrumented. Both authored artwork paths pass8 replacement cases,
8 stale-history controls and160 complete local-view vertex streams each:
Spartan/Elite, same/earlier accepted ticks, team/FFA, one through four local views
and640x480/1280x720. Actual shader inputs/radar pixels and complete typed vertex
fields match independent fresh HUD owners, while reset alone preserves resident
image backing pointers and material slots. The state banks are declared UI
inputs, not a source gameplay trajectory or a rendered migration. Existing
motion-sensor/viewport, source device world/network, original score, authored HUD,
Vista shield/crosshair and muzzle/sound checks also pass. Current-root syntax and
diff-whitespace checks pass for the changed files.

Initial failures are preserved. The new negative control originally compared
only shader state, missing stale geometry/speaker output. Comparing complete
draw streams fixes that fixture and exposes a real implementation mistake:
clearing the static donor sampling offsets moves Vista shield pixels. Keeping
those offsets resolves the Vista reset regression. The broader HUD fixture
also expected a profile appearance change to reskin a fixed biped's authored
HUD; it now checks the actual Spartan and Elite owners separately, including
their original split sprites/colors. Production species selection is unchanged.
Evidence: `.local/hud-event-handoff-20261018/verification.json`. Later unrelated
root changes are recorded separately; they are not part of this qualified cut.

The supervisor removed34,163 generated files totaling1,653,612,981 logical bytes
(1,748,848,640 filesystem-reported allocated bytes), including the private source
copy, all nine test executables, object/archive files and test scratch. Compiler
and source-check scratch are absent too. The shared filesystem's observed free
space changed by60,432,384 bytes during cleanup; concurrent builds and shared
extents prevent attributing that delta exclusively to these removed files.
Only bounded reports, source hashes and failure diagnostics remain. No new
large binary/capture or installed runtime is retained. Original assets,
reference evidence, frozen inputs, selected runtimes and concurrent work remain.

These primitives are not wired into partial normal-loop activation. The full
coordinator must prepare every replacement before irreversible GPU retirement,
preserve/reconstruct accepted active muzzle/world effects and audio, rebase all
event/interpolation/source borrowers and device owners, invalidate GPU upload
cursors, then publish the native role. Existing effect clocks reject backward
ticks; merely clearing them would lose accepted live effects. Automatic retained
readiness, false-alarm lifetime, actual rendered activation, timing/memory,
hardware/WAN and the remaining ONLINE checklist stay open. Default migration
remains gated; gameplay and match/postgame voice remain direct and player-hosted.

## Owning active-effect presentation cuts, 2026-10-18

NativeMuzzleEffects and NativeWorldEffects now support opaque local checkpoints,
private prepared replacements and allocation-free publication on the same local
owner. The muzzle snapshot retains its five datum pools/salts/occupancy, systems,
emitters, particles and their previous states, event clocks, pending slices,
completed marker/view poses, attachments and unsubmitted sound requests. The
world snapshot retains clocks/identities, marker/projectile records, active
effects/systems/emitters and particles, decals, light records/spatial lists,
delivery metadata, query epochs and all four local selection caches. Authored
definition graphs are copied; every world effect/event/system pointer is rebound
into the new graph. Resident material indices and shared immutable particle
materials remain stable. The discarded predecessor graph can be destroyed before
the restored state is observed.

Checkpoint random values are owned copies. Collision callbacks are deliberately
excluded so the checkpoint cannot retain a predecessor world through a closure.
Preparation takes explicit accepted-world callbacks and long-lived RNG bindings;
the original alias relation between the two RNG channels must match. It changes
neither the current owner nor shared RNG. Publication swaps the prepared private
owner without allocating, rejects foreign/consumed tokens and leaves shared RNG
to the enclosing coordinator. All owners must stay externally quiescent between
preparation and publication. The caller must pair these local presentation cuts
with the exact validated/agreed native cut and restore shared application/visual
RNG once with all its consumers. This is native presentation continuation policy,
not a recovered Xbox migration routine or a network/world checkpoint format.

Two TU1.5 CTests pass with ASan/UBSan and enabled leak checks on both modified
owners and tests; native/generated/system/dependency interiors remain
uninstrumented. No suppression or disabled leak checking is used. The world
case restores at tick210 with24 particles,11 decals and1 grenade light, after
executing a declared discarded-future presentation request. It preserves the
same live resources and emitted particle inputs, rejects ordinary backward
advance, and continues40 further match ticks against the independent batched
control. The separate occupied-light case restores spatial/query state after
all four local selections and resumes the original query epoch/identity/fades.
The muzzle case performs two restores of an active flash, disposes each old
graph, retains the accepted pending sound exactly once, then resumes12 ticks
including new allocation generations and matching draw/sound requests. Foreign
checkpoints/tokens, token reuse and changed RNG aliasing reject. Weak-reference
controls prove predecessor collision closures are not retained. World resident
material/image backing and muzzle material pointers remain unchanged.

The enclosing existing regressions also pass600 SMG birth/expiry cases,347 Sword
particle draws with the delayed ready sound,60 actual contacts,one grenade,
80 manual SDL audio packets, original spark texture inputs, resolution/localhost
LAN checks and final particle/decal/light retirement. Current-root compiler
syntax and diff-whitespace checks pass. The first recorded run passed; the final
run adds RNG-alias guards, next-allocation continuation and resource-identity
assertions. Later unrelated root edits are listed separately. Evidence:
`.local/effect-presentation-handoff-20261018/verification.json`.

The supervisor removed34,152 generated files totaling1,078,856,808 logical bytes
(1,174,110,208 filesystem-reported allocated bytes), including the private source,
both test executables, object/archive files and per-test scratch. Compiler and
source-check scratch are absent too. The shared filesystem's observed available
space changed by345,104,384 bytes during cleanup; concurrent jobs and shared
extents prevent attributing this delta exclusively to this removal. Only bounded
reports/source hashes/commands remain. No large binary/capture or installed
runtime is retained; original assets, reference evidence, frozen source inputs,
launcher runtimes and unrelated active work remain untouched.

Normal runtime activation remains gated. The next integration must capture all
local presentation owners with the exact accepted native cut, including map/
combat attachment caches and shared RNG. Scene/model/FP interpolation, device/
source borrowers and playing audio must be staged with these owners before GPU
retirement and role publication. Automatic readiness/false-alarm lifetime, full
rendered host/client migration, timing/memory, delivery, physical/WAN and all
remaining ONLINE requirements stay open. Gameplay and match/postgame voice
remain direct and player-hosted.

## Grouped effect caches and delayed cut binding, 2026-10-19

SourceMapEffects and SourceCombatAttachments now retain owning local checkpoints
of placement/marker/seed/lifetime/instance caches, counters and diagnostics. A
successful synchronization records its tick. Incomplete synchronization blocks
capture, while an older completed snapshot can repair the same local owner.
Preparation copies privately and publication swaps the private owner without
allocating. Both retain their immutable Cache and stable NativeWorldEffects
references; these are local presentation values, not a peer wire format.

EffectPresentationContinuity groups those caches with NativeMuzzleEffects,
NativeWorldEffects and shared visual RNG. Capture requires matching completed
ticks for active owners and matching RNG values. A never-synchronized cache is
explicitly cold, not proof of original history. The owned snapshot can wait for
asynchronous native validation, then bind the exact peer/context/seal without
recapturing the now-later world. Preparation rejects foreign owners or changed
bound fields and allocates all four replacements. Publication preflights every
closed token on the executor, swaps the four owners and restores visual RNG
once, without intervening allocation/callback. Enclosing native/GPU/device
readiness and the completed-frame barrier remain caller responsibilities.
Context/tick agreement is necessary but does not independently establish exact
native phase equality; the caller must prove that association before binding.

Six selected-owner ASan/UBSan/leak CTests pass. The new combined case uses actual
English TU1.5 data, resolves the authored Plasma Pistol option, uses normal
source/Match local-view binding and hashes a real encoded native checkpoint.
On Turf it captures tick45 with4 map births,4 combat instances and88 particles,
advances ten ticks, explicitly discards presentation owners and faults both
caches, then binds the saved state. Native authority import and grouped repair
restore the accepted state; repeated synchronization does not duplicate births,
and twenty further native match ticks complete. Changed room/generation/
membership/host/cut fields, foreign owners/executors and token reuse reject.
The first-person presenter is empty in this combined case; active muzzle/world
cases and the named containment/Turf/dune source-function regressions pass
separately. This is an in-process native/local-presentation case with fixture
room metadata, not a quorum, transport, runtime, GPU or device migration.
Native/generated/codec/system interiors remain uninstrumented; only the changed
presentation/cache owners and selected tests use sanitizers. No leak suppression
or disabled leak checks is used.

The initial build found a missing explicit authority-codec link on the new
test. A recorded minimal private CMake patch fixes it. The first executed run
passed five cases and rejected the combined fixture because it assigned a
source local player without binding Match local views. Using the same
bind_source_local_views path as normal loading resolves that mismatch; the
validator was not weakened. Current-root compiler syntax and whitespace checks
pass. The full source freeze, explicit updates/patch and later unrelated root
drift are recorded in `.local/effect-cache-cut-20261019/verification.json`.

Cleanup is verified:34,272 generated files totaling1,263,200,521 logical bytes
(1,358,753,792 filesystem-reported allocated bytes) are removed, including the
private source/build, four test executables, object/archive files and test
scratch. All recorded compiler/check scratch is absent. Observed shared free
space changed by610,185,216 bytes during cleanup; concurrent work and shared
extents prevent attributing that delta exclusively to this removal. Only bounded
commands, source hashes, results and failure diagnostics remain. No large binary,
capture, installed runtime or original/reference input was removed or retained
as a new deliverable; unrelated active work remains intact.

The next runtime proof at that checkpoint was concrete. LanHost::checkpoint retains per-peer
snapshot_sequence (`src/multiplayer_network.cpp`, host continuation capture).
LanClient accepts newer complete snapshots with the same world tick because
roster/ownership can change within that tick, but received_states then
omitted the sequence association. Host broadcast occurs before the native frame
closes. A local presentation must therefore be correlated with the admitted
snapshot sequence/cookie/epoch and the paired native host continuation; tick
matching alone cannot establish exact cut identity. No automatic readiness or
normal migration is enabled by this grouped API. Runtime capture/retention,
scene/model/FP interpolation, device/source borrowers, playing audio and
coordinated GPU/native publication, timing/memory, delivery and rendered/
physical/WAN/full-ONLINE acceptance remain open. Matches and match/postgame
voice remain direct and player-hosted.

## Exact accepted snapshot origins and sidecar proof, 2026-10-19

LanClient now exposes one opaque owning MultiplayerSnapshotOrigin per accepted
state, in matching order. Empty receives clear both banks; rejected/stale/
duplicate fragments never create a receipt. Receipt identity includes the
complete reassembled body's SHA-256 digest and byte count, snapshot sequence,
fingerprint, nonce/cookie, admitted context, physical peer, local connection
lease and rebind revision. Pre-admission/LAN receipts have no ONLINE context.
The host records the last sent body stamp beside its snapshot counter and
preserves both in checkpoint/resume. Role promotion clears the old sent stamps
while preserving counters so the new generation must publish fresh state.
Both accepted banks reserve capacity before publication with geometric growth.

LanClient::prove_snapshot matches an owning local receipt to the exact saved
per-peer body/sequence in the host sidecar from an independently authenticated
and natively validated paired cut. Connection/roster/context/native tick and
sidecar binding must agree. The method does not independently establish the
full world/sidecar pairing; that remains the native receiver's responsibility.
It permits an older owning receipt to bind after later receives, and refuses
new proofs after local host rebind retires that generation. The resulting
opaque MultiplayerSnapshotProof retains the exact validated cut and receipt.
EffectPresentationContinuity can capture with that receipt and bind later only
with its matching proof. The raw ValidatedAuthorityCut overload rejects these
receipt-based captures. Its earlier explicit caller-proof/local API remains.
The caller still must consume the exact associated state before capture; this
does not attest arbitrary presentation state merely supplied with a receipt.

The explicit host-sidecar wire format advances from H2HOST01 to H2HOST02;
the old format rejects. Native gameplay protocol89 and gameplay packet layout
remain unchanged. Feature negotiation and coherent delivery of the new sidecar
version remain gates before default activation.

Four focused CTests pass with selected network/origin, sidecar-codec, effect-
binder and test TUs under ASan/UBSan and enabled leak checks. No suppressions
are used; native/generated and most system/dependency interiors are not
instrumented. The new test uses an actual English TU1.5 native world/cut with
normal local-view binding, four couch seats and a late same-tick join. An older
valid snapshot body relabeled with a genuinely newer sequence reaches the
ordinary decoder but fails exact sidecar proof. Genuine later state succeeds;
duplicate/foreign/unbound/stale receipts, wrong capture/proof associations,
H2HOST01 and retired context reject. A saved earlier receipt remains provable
against its own earlier cut after later receives. Existing native host
continuation, role-rebind and grouped-effect regressions pass too. These use
sequential logical endpoint adapters and locally validated cuts, not live
DTLS/WAN/quorum. The new receipt test's effect presenters are empty; the
separate grouped-effect regression retains active world effects.

The full current source cohort was frozen without a historical ABI overlay.
After the initial four passes, an explicit geometric-allocation fix was copied
into that cohort and all four checks passed again. Final current-root GCC C++20
syntax and whitespace checks pass. All changed owner/test/CMake hashes match
the tested cohort; the only later root drift is in unrelated gameplay helper/
test files, preserved intact. Evidence, manifests and instrumentation options
are in `.local/snapshot-origin-20261019/verification.json`.

Cleanup is verified:34,314 generated files totaling1,271,417,932 logical bytes
(1,367,085,056 filesystem-reported allocated bytes) were removed with the owned
source/build/cache and all per-test scratch. Recorded compiler scratch is
absent too. The observed shared filesystem available-space delta is431,300,608
bytes; concurrent work and shared extents prevent exclusive attribution.
Only bounded reports/commands/hashes remain, with no large binary/capture or
installed runtime retained. Original assets, reference evidence, frozen
reference inputs, launcher runtimes and unrelated active work are preserved.

This closes local snapshot-body/sidecar association, not automatic runtime
readiness. Normal runtime still skips ownership updates on newer same-tick
states; it needs an explicit ownership refresh without double advancement of
native gait/per-tick presentation. Bounded receipt/presentation retention,
distribution of authenticated cut metadata to ordinary survivors, full scene/
model/FP/device/source/audio adoption and coordinated GPU/native publication
remain open. Timing/memory, delivery and rendered/physical/WAN/full-ONLINE
acceptance are outstanding. Default migration stays gated and the full goal
stays active. Gameplay and match/postgame voice remain direct and player-hosted.

## Native same-tick receipt adoption, 2026-10-19

The normal client no longer discards same-tick state after copying devices.
Each accepted state is consumed beside its opaque receipt; bank count and tick
must agree. The latest body supplies roster/owner/scene/effect state while the
previous completed-tick interpolation endpoint remains unchanged. Explicit
SourceWorldInterpolation/SceneInterpolation refresh operations require the
current completed tick, retain surviving endpoints and retire missing or
changed identities. A removed owner cannot regain that old history if it
returns in the same tick. The next real step starts at the refreshed endpoint.
Existing capture/reset semantics are preserved.

Gait creates or retires its actual lifetime without another source-channel
tick on refresh; aim advances zero ticks and a repeated FP pair does not
dispatch its audio events again. Snapshot receipt does not restart the render
interpolation interval. Completed shot stamps baseline both at admission and
when the viewport starts already connected. Read-only diagnostics expose the
accepted receipt and actual retained locomotion endpoint, without supplying
state, clocks, poses or input. Full presentation history/proof binding is not
enabled by these observations.

The rendered regression uses an actual English TU1.5 world on its own worker
executor, normal native local-view binding, localhost UDP and four local
viewport views. It holds the host at tick2 after actual local weapon cues, then
joins/departs another peer. The native leave flag changes while the connected
PCR row remains retained. It passes with39 accepted receipts,36 same-tick
refreshes,28 rendered frames,192 unchanged completed gait observations and four
actual local weapon cues without replay. The GPU is RTX5080 with Khronos
validation enabled and no validation diagnostics recorded. The inherited
Wayland/Xwayland session reports zero monitors, so an owned private Xvfb display
is used; this does not establish a physical display/controller/headset check.
The test is raw localhost UDP, not authenticated ONLINE/DTLS/quorum/WAN or a
host migration. Native Match exports FP clock pairs only for producer-local
views; remote clock pairs and FP channel audio are both zero here. Real local
weapon-effect cues supply the repeat control; remote FP channel delivery stays
open rather than being replaced by an invented clock.

Five CPU/native regressions pass normally and with selected interpolation/
presentation/test TUs under ASan/UBSan and enabled leak detection. No
suppression is used; native/generated, most system/dependency interiors and
rendered TUs remain uninstrumented. Current-root GCC C++20 syntax and whitespace
checks pass. The initial fixture lacked a required LanClient count. Later runs
identified the zero-monitor display, the existing finite-capture interpolation
guard and absent system validation layer; each diagnostic is retained. A
matching configured-repository validation package was checked against the
current repository SHA-256, extracted under owned scratch and removed.

The older standalone late-audio fixture used a software-only world without
completed source model regions and an obsolete generic sound notification
queue. It now owns a real TU1.5 world on its worker and observes the completed
source effect journal through a separate authored FP muzzle owner. This
additional regression remains red: one prejoin cue produces zero client
replays, but10 later FP-adapter cues produce20 WORLD-client submissions. Both
submissions have the same full weapon owner. The actual source effect
3865444353/definition3852338217 has two location rows with name251658459.
Original TU1.5 WORLD/FP location/part/sound admission must be established before
changing the WORLD count or the FP-adapter expectation. The older registered-
local-effect original comparison is TU1.1 supporting evidence, not proof for
this TU1.5 owner. No arbitrary deduplication or expected-count multiplier is
applied. This red check is retained explicitly, not included in the five passing
sanitizer cases and not hidden by the successful four-view result.

Evidence: `.local/replica-refresh-20261019/verification.json`. The full source
freeze and explicit owner/fixture/header updates match the final hashes. Seven
later unrelated root changes are recorded and preserved; no historical ABI
overlay is used. Cleanup removes34,710 files/1,528,119,727 logical bytes
(1,624,621,056 reported allocated bytes) from the owned source/build/cache,
plus all recorded compiler/render/display/validation scratch. Observed shared
available space changes by-558,243,840 bytes during cleanup; concurrent work
and shared extents prevent attributing this delta to the removal. No new large
artifact or runtime is retained. Original assets, reference/frozen inputs,
launcher runtimes and unrelated work remain untouched.

Next are authentic remote FP ownership, the red WORLD/FP sound-admission case,
bounded completed presentation/receipt history and actual readiness binding.
Complete scene/model/FP/device/source/playing-audio adoption, coordinated
GPU/native publication, timing/memory, coherent delivery and rendered-migration/
physical/WAN/full-ONLINE acceptance remain open. Default migration stays gated;
the full goal stays active. Gameplay and match/postgame voice remain direct
and player-hosted.

## Original TU1.5 WORLD/FP location admission, 2026-10-19

The preceding late-audio failure is resolved at its source. A fresh full-source
baseline reproduced10 authoritative FP-adapter firing cues versus20 WORLD
viewport submissions, with zero stale prejoin replay. The live effect retained
two equal-name location records. The WORLD exporter walked every raw chain,
resolved both records and presented both as WORLD locations. Original17D210
instead pushes mode0 into17DCE0 at17D2C4/17D2D0 and17D6DC/17D6E4 before part
dispatch. The iterator excludes encoded FP nodes while retaining WORLD and
unbound FFFF nodes; distinct WORLD nodes sharing a name are still separate.

`tools/oracle_effect_location_admission_tu15.py` executes unchanged English
TU1.5 SHA21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd
instructions. It verifies both caller push0/call bytes, every one of65,536 node
words in modes0/1/3, mixed linked chains and a cursor/stack-only write boundary.
There are no hooked callees or supplied instruction outcomes. WORLD admits
32,769 words, FP32,767 and combined65,536; the WORLD/FP mixed chain yields one
record in each specific mode and both in combined mode. This is original
location admission, not geometry, audio backend or complete sound-dispatch
parity. The65,536-byte WORLD fixture is retained with its full image/result
hashes in `tests/fixtures/effect_location_world_tu15.md`; the public helper's
independent repeat reproduces it exactly. No TU1.1 mapping is assumed.

The production WORLD exporter now applies that exact node predicate before
complete original transform resolution. It does not mask off an FP bit and
reinterpret a different pose, nor collapse marker names or halve sound counts.
Existing full-generation/cycle/bounds checks and read-only source behavior
remain. No packet shape, identity, UI or gameplay/media routing changes.

The native late-audio regression now reads the actual allocated effect/location
pools and requires both WORLD and FP records. It observes WORLD1/FP1 and
published_WORLD1. Ten postjoin source cues produce exactly ten actual SDL
submissions; the prejoin cue produces zero replays, with no unobserved tail.
All eight normal focused checks pass, including native host continuation,
role rebind, snapshot receipts, grouped effects and the four-view native
same-tick renderer. That RTX5080/private-Xvfb/Khronos-validation case observes
47 receipts/44 refreshes at tick2,19 frames,176 unchanged gait observations
and four local weapon cues without replay. Authentic remote FP clock pairs
and FP channel audio remain zero; their original ownership/delivery is still
an integration gate. Raw localhost UDP and SDL submission do not establish
authenticated ONLINE/DTLS/WAN, migration or physical headset acceptance.

Six CPU/native cases also pass with selected WORLD-exporter/admission-test
ASan/UBSan and enabled leak detection. No suppression is used; native/generated,
most system/dependency interiors and rendered TUs remain uninstrumented.
Current-root GCC C++20 syntax and whitespace checks pass. The coherent full
source freeze plus explicit owner/fixture/oracle updates match final hashes;
later concurrent multiplayer/header/runtime-test drift is recorded and
preserved. Evidence: `.local/world-location-admission-20261019/verification.json`.

Cleanup is verified:34,792 generated files/1,553,200,174 logical bytes
(1,649,913,856 reported allocated bytes) from the owned source/build/cache,
all recorded compiler/oracle/render/display/validation scratch and one redundant
65,536-byte reference copy are removed. The small canonical fixture remains.
Observed shared available space increases640,991,232 bytes; concurrent work and
shared extents prevent exclusive attribution. No large artifact or installed
runtime is retained; original assets, reference/frozen inputs, launcher-selected
runtimes and unrelated active work are untouched.

The next migration owners remain authentic remote FP views/clocks, bounded
completed presentation/receipt history and actual readiness binding. Complete
scene/model/FP/device/source/playing-audio adoption, coordinated GPU/native
publication, timing/memory, coherent delivery and rendered-migration/physical/
WAN/full-ONLINE acceptance stay open. Default migration stays gated and the
full goal stays active. Gameplay and match/postgame voice remain player-hosted.

## Owning validated host proof for foreground receipts, 2026-10-19

`SourceAuthorityHostProof` now carries immutable evidence of one independently
admitted, natively validated complete world/host pair through the receiver's
existing worker-status mutex. Only the receiver constructs it, after native
world preparation/replay and sidecar validation and before acknowledging the
cut. It retains the owning host table and exact validated cut, without native
executor/world borrowers. The foreground can verify a retained client receipt
against the exact connection, authority context, rebind revision, sequence and
body through `LanClient`. The validation recipient is not confused with the
original packet host; one paired table can verify each independently admitted
survivor's own receipt.

Current status clears proof with readiness during incomplete replay,
cancellation, native-world consumption and executor failure. Native-only input
cannot manufacture paired evidence, and a rejected stripped-host downgrade
preserves the previous validated pair. Owning historical copies survive later
validation and worker retirement. They remain evidence about that past cut,
not permission to elect/activate or proof that a mutable candidate is still at
that boundary. Client queries remain on their executor; current checkpoint
validation and the full adoption transaction remain mandatory.

Five focused normal cases pass across the initial four passing cases and the
corrected stream rerun. The initial stream fixture left the ordinary gameplay
pump idle throughout70 fragmented snapshots and failed to retain its final
receipt, despite validating all native replay phases. Pumping while replay
runs, with an owning receipt retained across empty receives, passes with70
actual accepted snapshots and the same strict proof. No transport queue,
source simulation, wire payload or proof condition was relaxed. The exact
number of individual dropped datagrams was not measured.

Three native cases also pass with selected receiver/fixture ASan/UBSan and
enabled leak detection, alongside two uninstrumented receipt/effect controls.
Generated/native/system/dependency interiors remain uninstrumented; there are
no suppressions. Real local DTLS takeover exercises three PCs' session owners,
twelve admitted seats/four per PC, both receiver and worker-retention paths,
native world repair, two-survivor agreement, twelve resumed native ticks and
twelve encoded direct voice packets. Both surviving clients prove their exact
receipt; foreign/pre-admission origins fail, historical proofs survive worker
destruction, and the old proof fails after the surviving client rebinds. This
is in-process native/transport evidence, not a rendered migration, physical
headset/controller or WAN acceptance. Current-root GCC C++20 syntax passes.

Evidence: `.local/validated-host-proof-20261019/verification.json`. The frozen
source cohort plus explicit stream-fixture update matches all changed owner
hashes. Shared backend source remained stable. Later unrelated network
diagnostic and native weapon-test edits are recorded and preserved. Cleanup
removes34,386 files/1,358,022,460 logical bytes (1,453,916,160 reported allocated
bytes) from the owned workspace; all recorded build/compiler/sanitizer scratch
is absent. The initial system-temporary run was cancelled, its removal checked,
and validation restarted with explicit project scratch. Observed shared free
space increases544,645,120 bytes, without exclusive attribution. No large
artifact or installed runtime remains; assets, reference inputs, launchers and
concurrent work are preserved.

Bounded completed presentation/receipt history and actual readiness binding
are next. Authentic remote FP ownership, complete scene/model/FP/device/source/
playing-audio adoption, coordinated GPU/native publication, timing/memory,
delivery and rendered-migration/physical/WAN/full-ONLINE acceptance remain
open. Default migration stays gated and the full goal stays active. Gameplay
and match/postgame voice stay direct and player-hosted.


## Bounded completed effect history and rendered ONLINE binding, 2026-10-19

`EffectPresentationHistory` now retains completed CPU effect cuts with their
opaque accepted receipts on the game executor. Independent session state
admits/retires the context; a receipt cannot reopen a retired generation.
Retained duplicates are idempotent. Modulo32 ordering matches the native
transport, so an evicted receipt cannot be recaptured later. An exact old
receipt can be found behind newer same-tick refreshes, but another body or
sequence at the same tick cannot replace a missing/evicted one. Failed capture
preserves earlier completed entries. Binding requires the receiver-only
validated paired host proof and a matching live client connection.

The runtime finishes muzzle poses/clocks and drains pending muzzle cues for
each accepted replica before capture. Same-tick poses refresh without advancing
time. Under the existing internal migration gate it retains128 history entries
and one separately owned matched checkpoint. This is an explicitly native
count policy, not an Xbox constant or byte budget. Repeated identical binding
attempts are cached. A read-only observer exposes the CPU barrier/history to
validation; no identity, wire payload, product UI or media routing changes.
Historical binding does not advertise readiness or activate a native/GPU/audio
role. Existing local visual RNG channels are retained; original remote FP and
complete event provenance are still separate gates.

Nine focused normal cases pass across the expanded run's eight passing cases
and corrected ONLINE rerun. The new rendered ONLINE case uses two real local
DTLS session owners/eight seats, four rendered client views, a native authority
paced at30Hz and10Hz snapshots, and the actual shadow worker. It captures all
257 accepted receipts, reaches but does not exceed128 history entries, binds
two independently validated native world/host cuts and submits463 frames.
Khronos validation is enabled. The private backend confirms no gameplay/media
relay or unfinished PCR publication. Identity is a fixture, not live Steam or
Discord authorization. No host is disconnected/promoted in this rendered case.

That case initially failed strict native configuration validation. The fixture
had advertised its pre-override ordered variant writes, while world creation
applies starting_weapon as a later setter. The final304 bytes were equivalent,
but the exact advertised lists differed. The fixture now uses the resolved
world configuration, matching the normal runtime. No host-sidecar or receipt
condition was relaxed. The red run and correction are retained in diagnosis.

The normal raw-LAN four-view regression also passes43 accepted/40 same-tick
receipts,43 completed CPU effect barriers,25 rendered frames,224 unchanged
gait observations and four local weapon cues. It creates no admitted ONLINE
history. Authentic remote FP pairs/channel audio remain zero. The source
WORLD/FP location audit remains1/1/published_WORLD1; ten postjoin cues produce
ten SDL submissions, with zero prejoin replay or unobserved tail. Three native
history/snapshot/group cases pass selected history/group/fixture ASan/UBSan
and enabled leak detection, alongside two uninstrumented world/role controls.
Generated/native/system/dependency interiors and renderer TUs remain
uninstrumented; no suppressions are used. Current-root GCC C++20 syntax passes.

Whole ONLINE fixture process peak RSS is7,408,412 KiB (about7.1 GiB), including
collocated host/client/replay worlds and renderer. This is not isolated history
memory or a qualified client budget. Immutable checkpoint sharing, a byte
budget, sustained catch-up/latency and all-map/load coverage remain necessary.

Evidence: `.local/effect-history-20261019/verification.json`. The coherent full
source freeze and explicit updates match all changed owner hashes; shared
backend source remains stable. Nineteen later unrelated root changes are
recorded and preserved. All recorded source/build/compiler/sanitizer/display/
validation scratch is absent. Cleanup removes34,857 files/1,742,441,803 logical
bytes (1,839,280,128 reported allocated bytes); observed shared available space
increases660,180,992 bytes without exclusive attribution. No large artifact or
installed runtime remains; reference assets, launcher runtimes and unrelated
work are untouched.

Completed CPU effect receipt history is now present and exercised in the
rendered ONLINE path. Full readiness for every survivor, authentic remote FP,
complete scene/model/FP/device/source/playing-audio adoption, coordinated
GPU/native publication, timing/memory, coherent delivery and rendered-migration/
physical/WAN/full-ONLINE acceptance remain open. Default migration stays gated
and the full goal remains active; gameplay and match/postgame voice remain
direct and player-hosted.


## Immutable active-light map residency, 2026-10-19

The light checkpoint now separates owning, immutable BSP atmosphere/object-light
geometry/raw terrain from mutable light pools, spatial links and deliveries.
Snapshots and prepared restores share only the authored resource owner. Light
identities, brightness/fades, query/selection caches and RNG remain independent.
Collision geometry and published texture bytes already shared residency; those
owners were inspected and left unchanged. No source algorithm, packet, identity,
UI, variant or gameplay/media route changes.

A matched normal glibc allocation probe holds128 actual active-light checkpoints
at tick210 (24 particles,11 decals,1 light). Additional live heap
(uordblks+hblkhd) falls from95,980,784 to40,763,552 bytes, saving55,217,232 bytes
(57.53%). Only the world-effect owner changes between these probe runs; the
frozen cohort and probe fixture are the same. Observed capture time is20,154
versus8,734 microseconds for the128 copies; these are single-run observations,
not a qualified latency guarantee. Mutable-state restore and independent
continuation remain exact. The grouped Turf case also retains88 particles,
four combat instances/four map births and20 continued ticks.

The rendered ONLINE fixture now requires at least256 captures/two full history
turnovers,128 peak entries and two native cut associations. Baseline/shared runs
both pass:257/256 accepted-and-captured receipts,128 entries and two cuts, with
455/504 rendered frames. Whole-process peak RSS is6,842,848/7,408,516 KiB. It
does not demonstrate a whole-client reduction; collocated native worlds,
renderer allocations and scheduling/allocator variation remain part of this
measurement. The active-light allocation result is not a client byte budget.

All four normal cases pass before and after the change. The final three CPU
cases pass selected world-effect/active-world/group ASan/UBSan with enabled leak
detection; generated/native/system/dependency/renderer interiors and the muzzle
fixture TU remain uninstrumented, with no suppressions. The allocator probe is
explicitly enabled only in normal builds. Current-root GCC C++20 syntax and
changed-owner/frozen-source/backend-stability audits pass. Eleven later
unrelated source changes are recorded and preserved.

Evidence: `.local/effect-light-residency-20261019/verification.json`. Small
baseline owner/fixture source and hashes remain for reproducibility; all
recorded build/compiler/probe/sanitizer/display scratch is absent. Cleanup
removes34,871 files/1,358,228,683 logical bytes (1,455,144,960 reported allocated
bytes). Shared available space changes by-651,776,000 bytes during cleanup;
concurrent work/shared extents prevent attributing that delta to these removals.
No large artifact or installed runtime remains. Assets, references, launchers
and unrelated work are untouched.

History byte budget/other snapshot caches, whole-client/native-world memory,
sustained replay/catch-up and all-map/load coverage remain open. Authentic
remote FP/event ownership, full survivor readiness, complete owner/audio/GPU/
native adoption, delivery and rendered-migration/physical/WAN/full-ONLINE
acceptance remain open. Default migration stays gated and the full goal stays
active. Gameplay and match/postgame voice remain direct and player-hosted.

## Staged lens presentation transaction, 2026-10-19

The previous immediate lens rebinds could not participate in an all-owner
preparation barrier: a query rebind retired old GPU work before a later draw
preparation could reject. Both lens owners now have move-only prepared tokens,
readiness checks and allocation-free publication. Preparation builds private
SDK/header/query state without changing old source bindings or GPU work.
Owner identity and completed-frame revisions reject stale tokens, including
unsubmitted frames and a return to the same world after another generation.

`SourceLensPresentationHandoffTu15` stages both, verifies their common live
renderer, rechecks the incoming retired query ring and both tokens, then retires
old query work and publishes both. Destruction/cancellation and any failed
preparation/readiness check preserve both old owners. No incoming native call
or result is invented. The caller must retain/quiesce the exact incoming world
and validate its full native seal separately. The existing GPU device wait is
unchanged; this is not a bounded-time or full-runtime migration claim.

The current frozen cohort builds the native executable and passes 11 normal
focused CTests. The expanded actual two-world case has 20 final-draw views,
preserves real pending hardware results across five declined/cancelled cases,
retires them on paired commit, preserves resident material identities and the
incoming native adoption boundary, and rejects stale/foreign/replayed/retired
renderer tokens. Original GPU query coverage (45 quads) and final-draw callback
coverage (45 cases, 25 quads) remain green with empty Khronos validation logs.
Current-root GCC syntax checks pass. A duplicate fixture variable was corrected
after the initial compile rejection; the complete rerun passes. No new sanitizer
or visible-flare migration acceptance is claimed.

Evidence: `.local/lens-transaction-20261019/verification.json`. All recorded
scratch/build/source/dependency/capture paths are absent. The supervisor removes
35,086 files / 1,529,265,786 logical bytes (1,626,501,120 allocated file bytes).
Observed shared free-space increase is 510,263,296 bytes; shared extents and
concurrent activity prevent attributing that measurement solely to this cleanup.
Only bounded reports/commands/hashes remain; no executable is retained or
installed. Two unrelated later transparent-group source changes are recorded
and preserved. Original assets, reference evidence and launchers remain intact.

This closes the lens subtransaction preparation seam. Full scene/model/device/
playing-audio and coordinated native adoption, exact effect/FP ownership,
readiness for all survivors, timing/memory, coherent delivery and actual
rendered/physical/WAN/full-ONLINE acceptance remain open. Default migration
stays gated and the goal stays active. Gameplay and match/postgame voice remain
direct and player-hosted; Vista ONLINE/clan assets and widescreen policy remain.

## Scene lifetime retirement seams, 2026-10-19

Dynamic, weapon-projectile and released-grenade scene owners previously lacked
an explicit way to retire same-handle presentation caches during a world rewind.
They now clear full-owner pose/light/LOD state, ordered owner lists, all four
model draw lists and projectile glow commands. Shared geometry view histories
are invalidated; authored definitions, material IDs and bitmap/model residency
remain. Existing scenery/machine/prop reset methods and shared geometry reset
are now `noexcept`, permitting all six to participate in a no-fail commit after
the enclosing caller finishes preparation and quiesces borrowers. These methods
do not validate/adopt a native world or supply exact retained lighting history.

Five normal focused CTests pass. The new authored-resource case compares four
views after rewinding declared same-handle inputs from frame300 to frame0 with
independently prepared fresh scenes: 2,544 dynamic vertices, 648 projectile
vertices, 498 grenade vertices and four glow commands match, including lighting,
LOD/material bindings and cleared borrowed draw lists. This is declared
presentation evidence, not original birth/physics, a retained scene checkpoint,
GPU rendering or live ONLINE migration. The initial run passed this case and
three companions but exposed an existing prop bug: its retained source-view
array was indexed before the invalid fifth view reached the geometry guard.
The prop entry now rejects view>=4 first; the complete five-case rerun passes.

The same five cases pass unsuppressed ASan/UBSan/leak checks. Seven edited scene/
geometry owners and five fixture TUs are instrumented; shared cache/systems/
native-generated/dependency interiors remain uninstrumented. Normal and
sanitized new-case reports match exactly. The current frozen native executable
builds; current-root GCC syntax checks and source audits pass. Four unrelated
later MCC Python/tool/test changes are recorded and preserved.

Evidence: `.local/scene-retirement-20261019/verification.json`. All recorded
scratch/source/build/dependency paths are absent. The supervisor removes 34,926
files / 1,320,113,892 logical bytes (1,417,134,080 allocated file bytes).
Observed shared free-space increase is 359,342,080 bytes; shared extents and
concurrent work prevent attributing that delta solely to these removals. No
executable or capture is retained or installed. Original assets, references,
launchers and unrelated active work remain intact.

The runtime still must invoke all six retirements and rebind source pointers,
interpolation, event and scene sidecars within complete GPU/native/device/audio
adoption. Exact scene/light/FP history, all-survivor readiness, false-alarm
lifetime, sustained memory/catch-up/timing, coherent delivery and actual
rendered/physical/WAN/full-ONLINE acceptance remain open. Default migration
stays gated and the full goal stays active. Gameplay and match/postgame voice
remain direct and player-hosted; Vista ONLINE/clan UI selection is unchanged.

## Native runtime presentation commit, 2026-10-19

Host promotion and surviving-client activation now accept a prepared presentation
commit at the existing native/transport transaction boundary. Session validation
and successor allocations precede it. The host wrapper publishes presentation
before moving native ownership; the client wrapper preflights its staged
endpoint, publishes presentation and then commits the endpoint. Old native
client destruction remains outside the session lock. Failed hooks preserve
pending native/transport ownership for retry and must preserve their own owners.
Successful hooks are noexcept and cannot allocate, block, perform device I/O,
mutate native/transport state or call the runtime/session. An executor-local
commit flag rejects runtime reentrancy before mutex acquisition or partial-state
observation. Frame and foreign-executor rejection remain in force.

The native executable builds. Three normal CTests pass, including both actual
twelve-player direct-session/paired-cut variants (same-executor receiver and
retired background executor). The new assertions cover host refusal/retry,
once-only publication of a receipt-bound four-owner cold CPU effect checkpoint,
an unchanged native seal, forbidden reentrancy, foreign executor/frame rejection,
client callback observation of its still-committable old endpoint, preservation
of receive assemblies/pending role/world on refusal, and once-only client retry.
Existing full native allocation/camera/input/departure/result checks continue;
each case runs twelve native ticks and twelve encoded direct voice packets after
old-host shutdown, with no master gameplay/media relay or unfinished PCR upload.
This does not qualify active effects, rendering, physical headsets/controllers
or WAN migration.

The same three cases pass unsuppressed selected-owner ASan/UBSan/leak checks.
Runtime authority, peer takeover and grouped-effect TUs plus three fixture TUs
are instrumented; other systems/native-generated/network/codec/dependency
interiors remain uninstrumented. Current-root GCC syntax and source audits pass.
The cohort includes a copied/frozen Python backend, avoiding concurrent backend
mutation. Six unrelated later MCC/cache/test/menu-source changes are recorded
and preserved. Evidence:
`.local/runtime-presentation-commit-20261019/verification.json`.

All recorded private scratch is absent. The supervisor removes 34,956 files /
1,345,909,397 logical bytes (1,443,012,608 allocated file bytes). Shared available
space increases by 442,916,864 bytes during cleanup; shared extents/concurrent
work prevent attributing that delta solely to these removals. No binary, capture
or runtime is retained/installed; original assets, references, launchers and
unrelated active work are preserved.

The windowed loop still needs complete prepared owner adoption. The current
lens transaction performs GPU retirement/device wait internally and cannot be
used under this callback's no-device-I/O contract until retirement is deferred
outside the locked commit. Active scene/FP/effect/lighting-history continuity,
all device/playing-audio/source-pointer/interpolation/HUD ownership, survivor
readiness, false-alarm lifetime, timing/memory/catch-up, coherent delivery and
rendered/physical/WAN/full-ONLINE acceptance remain open. Default migration
stays gated and the full goal stays active. Gameplay and match/postgame voice
remain direct and player-hosted; Vista ONLINE/clan UI selection is unchanged.

## Deferred GPU retirement and checklist M01, 2026-10-19

The preceding callback restriction is now resolved for the lens transaction.
`Renderer::PreparedLensQueryRetirement` stages an actual device use and revision;
publication swaps old query GPU ownership/cached results into the caller's token.
Query/draw/group commit no longer moves that token into a local destructor.
Retired private CPU owners and GPU resources remain retained until the caller
destroys the consumed token outside the enclosing session lock, on the renderer
executor with the SDL window alive. The GPU destructor retains its existing
device wait; this change does not qualify migration latency or responsiveness.

Readiness rejects draw/loading, resize, harvest, immediate retirement, expired
renderer/lease, foreign executor, altered incoming ring and consumed/old owner
generations. Actual device lifetime remains pinned even after logical renderer
retirement. Review also found and fixed device-gate locking in readiness and
an unconditional reference to a particle-only executor field: lease `valid()`
is now an atomic eligibility snapshot, while `acquire()`/retirement retain their
serialization; the renderer executor exists without optional particle graphics.

A coherent private native executable builds. All 12 normal focused CTests pass,
including the renderer device lease regression. Two real native worlds exercise
21 final-draw views and six cancelled/rejected preparations retaining old GPU
results. Both actual Vulkan observers record zero wait/destruction/query-read
calls during publication. Explicit token release afterward records one device
wait, two query-pool destructions and one each buffer/free/pipeline/layout call.
The new GPU owner reproduces original query coverage while the old owner remains
retained. The 45 original query cases and 45 original final-draw callbacks/25
quads preserve depth/stencil and repeated output. Three Khronos logs are empty.
Nine GCC C++20 syntax checks pass, including both renderer particle configurations.
This is normal component/real-GPU evidence; no sanitizer, visible active-flare
migration, physical controller/headset or WAN acceptance is claimed.

The 17 selected owners match the qualified 15,903-input frozen cohort. Five later
unrelated root changes remain intact and are recorded. The 96,942,736-byte native
executable (SHA-256 `d76560c985341779438d9f85bf70a564c5e1992de5b0e3bf60e505f536973719`)
is hashed and removed, not installed. Cleanup verifies every recorded private
path/process absent and removes 35,248 generated files / 1,552,133,164 logical
bytes (1,649,754,112 allocated). No large artifact is retained. Free-space delta
is separately recorded because this is a shared filesystem. Evidence:
`.local/lens-deferred-retirement-20261019/verification.json`.

M01 is checked in the [working checklist](online-completion-goal.md#working-checklist-and-goal-completion-contract);
M02 is next. The [bounded adoption inventory](../../.local/lens-deferred-retirement-20261019/next-adoption-inventory.md)
identifies missing exact runtime/event state, startup-role resource admission,
rollback-sensitive portal/scene/model/FP/HUD histories, four-seat input and
pending/playing audio. Inspection alone closes none of those gates. Complete
prepared adoption, all-survivor readiness, memory/catch-up/timing, coherent
delivery and rendered/physical/WAN/full-ONLINE acceptance remain open. Default
migration stays gated and the full goal stays active. Gameplay and match/postgame
voice remain direct and player-hosted; Vista ONLINE/clan UI is preserved.

## Owning receipt/runtime frontier and deferred cues, 2026-10-19

M02 now has one immutable history entry for the exact accepted receipt/decoded
state/local slots, grouped completed CPU effects, typed event/binding frontiers,
undelivered sound queues and a freshly captured pending-effect sound checkpoint.
The history owns and checks its completed-tick predecessor rather than trusting
caller-supplied same-tick state. Exact native proof binds the whole entry; foreign,
incomplete, stale, evicted and failed-capture controls preserve the prior entries.
Pending-cue restoration prepares copies and publishes with callback-free swaps,
retaining old storage in the caller's token. Already-playing voices remain M04.

The private native build and six normal tests pass: source peer takeover, exact
snapshot origin, real rendered ONLINE effect history, authored weapon/effect sound
metadata, grouped effect continuity and late-join audio. Rendered four-view history
accepts 259 receipts/captures258/initial-unproven1, peak128, two bound native cuts
and 381 frames. The whole fixture peaks at 7,098,512 KiB RSS; this includes native
worlds/renderer and does not qualify isolated history memory or sustained catch-up.
Late-join audio observes real SDL submissions, without a physical-speaker claim.

The first selected sanitizer run passed three tests but lost the retained-worker
case's LAN host on resumption. Isolated instrumentation measured 4,174 ms warm-up;
a controlled 6,002 ms idle reproduced host peers1-to0 on first resumed receive,
followed by the exact disconnect. The fixture now pumps actual neutral direct
traffic while the restarted shadow warms, without stepping the world or consuming
carried input. Both variants pass a minimum six-second regression and the original
twelve native ticks/direct encoded voice assertions. Normal warm-up/publication
measurements are 6006/80 and 6003/121 ms; selected sanitizer measurements are
6011/120 and 6005/111 ms. These are fixture observations, not migration latency
acceptance. Production timeouts and transport are unchanged.

All four selected ASan/UBSan/leak tests pass with no suppressions: network,
effect-sound, receipt-history and grouped-continuity owners plus four fixture TUs
are instrumented. Other systems, generated/native, codec/dependency interiors and
the rendered runtime are uninstrumented. Nine current-root GCC syntax checks pass.
The 17 selected hashes match a 15,910-input complete frozen source cohort; 30 later
root changes are recorded and preserved rather than overlaid into the test build.
This is not combined-current-release acceptance. Evidence:
[verification.json](../../.local/runtime-receipt-frontier-20261019/verification.json).

The 96,985,440-byte native executable (SHA-256
`25e2df20c22df9ea918c8becec43a0bcca2e89042f8788afbff744df34d0eea2`)
is hashed and removed, not installed. Cleanup verifies every recorded private
path/process absent and removes 35,095 generated files / 1,602,889,260 logical
bytes (1,700,376,576 allocated). Nine one-off helpers (54,316 bytes) are also retired
after preserving small reports/commands/hashes/failure diagnostics. No large
artifact is retained. Shared available space decreases by 1,340,067,840 bytes
during cleanup; concurrent filesystem activity/shared extents prevent attributing
that delta to this run. It is separate from the removed-file measurement.

M01 remains checked, M02 remains unchecked and default migration remains disabled.
The [next production dependency](../../.local/runtime-receipt-frontier-20261019/next-adoption.md)
is complete windowed world-borrower/resource adoption, with exact scene/HUD/input/
playing-audio, survivor readiness, memory/catch-up, rendered gameplay, physical
devices, WAN and the rest of the ONLINE checklist still open. Gameplay and
match/postgame voice remain direct and player-hosted. U01/D01 now explicitly
require MCC-derived Vista ONLINE/clan metadata/resources and selection; the
[bounded UI wiring audit](../../.local/runtime-receipt-frontier-20261019/mcc-ui-wiring.md)
found current catalogue/admission/selection gaps and does not close visual gates.
