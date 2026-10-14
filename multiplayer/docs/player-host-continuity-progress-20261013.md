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
   validates authored identities and completed tick, stages posed collision,
   and resumes the actual native motion owner. The test compares sixty future
   ticks, one-use controls and effects against the uninterrupted owner. A display
   replica cannot export itself through the authority-checkpoint API.
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

These are subowners. None claims a complete physics world or match save. The
retired arena still needs composition into the full live-world transaction.
Physics lock sidecars, live-world callbacks, typed simulation/match state and
transport promotion remain outstanding.

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
