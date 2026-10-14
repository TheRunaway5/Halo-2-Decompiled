# Native authoritative continuation and disconnect audit

Implementation follow-up: [player-hosted continuation progress](player-host-continuity-progress-20261013.md).
The relay/native control path and memory/device subowner checkpoints now have
focused verification. Complete gameplay-world continuation and live retained
departure remain outstanding; this historical audit's owner inventory still
defines those requirements.

2026-10-13. Source inspection of the current native runtime and the recovered
retail migration/departure owners. This is an implementation inventory, not a
claim of implemented host migration. No product code, deployed service,
launcher, account or asset selection was changed by this investigation.

## Finding

The native client cannot safely become host from its current `MultiplayerState`.
That bank is a presentation/replication projection, not an authoritative save.
The live match contains additional typed simulation, allocator, inventory,
attribution, engine and recording state, and can also contain a retained
original TU1.5 object/physics world. None has a complete cross-process
export/import owner today. A new host field, copying positions into fresh
players, or restarting the map would lose source-owned continuity.
Sources: [snapshot projection](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer.cpp:1572),
[match owners](/home/eric/Developer/Halo-2-Decompiled/include/halo2/multiplayer.hpp:370),
[simulation owners](/home/eric/Developer/Halo-2-Decompiled/include/halo2/simulation.hpp:307),
[retained source world](/home/eric/Developer/Halo-2-Decompiled/include/halo2/physics_live_world_tu15.hpp:19).

Ordinary nonhost survival has a separate prerequisite. The live native
`leave()` retires its allocated player and clears the native player record.
The recovered ordinary departure instead retains the allocated player,
identity, profile and statistics, marks it left, and removes its machine/control
ownership. The existing standalone departure/retention kernels are reusable,
but are not wired into that live owner.
Sources: [native leave](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer.cpp:925),
[recovered departure adapter](/home/eric/Developer/Halo-2-Decompiled/src/source_player_departure.cpp:15),
[retained identity remap](/home/eric/Developer/Halo-2-Decompiled/src/source_result_retention.cpp:47).

## Original authority handoff that must remain the reference

`83220` compares session host ownership with simulation authority. During an
unusable session it calls `6B040`, rather than advancing an independently chosen
new authority. Once the session is usable, client-to-host promotion calls
`6AEE0`; the opposite transition calls `6AE20`. It then applies the new
membership through `83F00`. `83F00` requires the local machine in membership,
remaps machine indices, releases vanished remote views and marks vanished
machine players for removal on authoritative worlds.
Sources: [83220](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_083a40.cpp:824),
[83F00](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_083a40.cpp:739).

The distributed-client branch of `6AEE0` changes world state5 to4 and game
mode to5 over the existing world. It resets the substate/views, repairs
replication, admits missing scenario objects, calls engine promotion, and
refreshes retained participants/teams. It does not recreate every survivor
with a new salted identity. The separate state3 branch refuses promotion
while receiving join data and reconstructs its simulation-world owner.
Source: [6AEE0](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_067e10.cpp:1840).

Specific promotion owners expose requirements beyond visible transforms:

| Original owner | Established operation |
|---|---|
| `89A20` | Repairs tracked replication chains and advances their four-bit handle generation, including chain references; invokes the owner reset callback. |
| `162060` | Reestablishes netgame/engine owners and missing match/player replication entities. It derives round timing from retained engine/options fields and resets absent-unit respawn ticks through the original owner. |
| `196470` | Remaps retained properties, all player statistics, both PvP dimensions and event references by the original 12-byte identity. Allocated left players remain eligible; a freed salt does not. |
| `196780` | Marks newly active teams in the retained recording bank when recording permits it. |

Sources: [89A20](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/replication_entity_table.cpp:266),
[162060](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_162060.cpp:134),
[round/respawn continuation](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_162060.cpp:172),
[native faithful remap](/home/eric/Developer/Halo-2-Decompiled/src/source_result_retention.cpp:47),
[196780](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_1967d0.cpp:1206).

The existing [host migration evidence](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/halo2-host-migration-evidence.md)
traces the separate session election, generation acknowledgements, failure
timeouts and reestablishment. Its peer map is not a world checkpoint. The
unresolved original host-quality field meanings must not be renamed to a
lowest-ping policy. Original automatic matchmaking backfill and the precise
removed-player reconnect policy remain unresolved by this evidence.

## Current replication versus authoritative continuation

`LanHost::respond()` encodes a fresh `match.state()`, customized for the
recipient's local slots/death cameras, and fragments it under a per-peer
nonce/cookie/sequence. `LanClient` owns only decoded states, assigned local
slots, fragment assemblies and transport/input counters. No promotion,
world-import, authority-epoch or host-rebind API exists in either class.
Sources: [host response](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_network.cpp:1038),
[client bank](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_network.cpp:1276),
[public transport interface](/home/eric/Developer/Halo-2-Decompiled/include/halo2/multiplayer_network.hpp:15).

The table lists concrete present owners, not hypothetical future gameplay.
An authoritative continuation must retain the parts that affect subsequent
admitted ticks; immutable definitions/resources can be rebound from the
authenticated map/image identity rather than serialized as host pointers.

| Owner | Already replicated | Missing or nonpromotable authority state |
|---|---|---|
| Player roster | Connected player names/configurations, source player/current/previous-unit/weapon handles, flags, team, lives, respawn countdown, selected statistics. | Full `MultiplayerPlayer`: attribution/death latches, nonenemy credit, complete respawn owner, watched unit, initial/spawn counters, strength/field194, powerup timers, request configuration; player pool occupancy, salts, next salt, cursor/high-water. Unconnected rows are omitted from encoding, so retained left participants cannot travel through this shape. |
| Unit and vitality | Position/velocity, aim/crouch, health/shield, damage feedback, a shield-recharging bit, zoom/camouflage, action presentation, grenade/melee presentation. | Full `Unit`/`Body`, recharge stun/paused/depletion/recharge flags, damage history and captured owners, garbage/owner counters, source action edges, requested grenade and inventory selectors, jump/movement/control state. Body copies do not create the original physics body. |
| Weapons/inventory | Current and offhand definitions/datums, loaded/reserve, last-shot/reload timestamps, reload fraction/empty, shader state and selected first-person channels. | All four inventory slots and inactive states, current/requested selectors, sequences/ready ticks; delay, burst count, trigger latch, complete SMG state, ready/heat/cooling/energy/firing-effect fields. New ammo clocks must not be inferred from the visible reload fraction. |
| Match/engine/round | Engine2/4, visible King hill/occupants, scores/team statistic shorts, winner, game-over/countdown, some variant fields and clock seconds. | Complete rules/variant identity, `SourceRoundLifecycle` flags/phase/index/start tick/ranking/countdown/status, `SourceGameEndStats` header/player fields/last-kill clocks, King engine timers/state and team mapping, exact finalization gates. |
| Simulation clock/random | Low32 completed native tick; original selected random seed contributes to initial setup. | Current64-bit native tick/counters, current gameplay RNG, retained source clock and source RNG streams. A seed is not the consumed RNG state. |
| Objects/world | Prop tombstones, device bank, vehicle transforms, weapon presentation rows, body poses, corpse appearance/vitality, projectile presentation. | Full object/player allocation topology, source raw bodies/components/heaps/physics state, scene ownership, pickup/world-row timers and state, retained units/corpse garbage links. Visible transforms cannot restore activation, constraints, contacts or stable object handles. |
| Projectiles | Grenade projectile fields/detonation journal and weapon projectile transform/travel presentation. | Complete grenade sandbox attachment state/event serials and other mutable sidecars; weapon projectile owner/captured attribution, source-birth state, weapon/BSP/native-owner fields. Recreating a projectile from position would alter attribution and lifetime. |
| Results | Selected scoreboard statistics; raw completed PCR travels only after final publication. | Running `SourceResultStatistics` (all437 words per player plus PvP), recording enabled/frozen, result event ring/cursors/start/time, retained participant/machine properties and event references. The final report cannot recover a running recording bank after host loss. |
| Presentation/delivery | Bounded journals for device/rumble/damage/weapon/grenade events; selected animation channels and remote death cameras. | Event cursor/serial baselines and local presentation bindings need explicit rebinding so migration neither replays old effects nor loses future serials. Microphones, render/audio owners, threads and callbacks remain process-local resources. |

Sources for the table: [player/state layouts](/home/eric/Developer/Halo-2-Decompiled/include/halo2/multiplayer.hpp:77),
[typed match banks](/home/eric/Developer/Halo-2-Decompiled/include/halo2/multiplayer.hpp:370),
[Unit/inventory/weapon layouts](/home/eric/Developer/Halo-2-Decompiled/include/halo2/simulation.hpp:50),
[simulation private owners](/home/eric/Developer/Halo-2-Decompiled/include/halo2/simulation.hpp:307),
[recharge owner](/home/eric/Developer/Halo-2-Decompiled/include/halo2/kernels.hpp:82),
[damage history](/home/eric/Developer/Halo-2-Decompiled/include/halo2/damage_history.hpp:5),
[round lifecycle](/home/eric/Developer/Halo-2-Decompiled/include/halo2/source_round_lifecycle.hpp:8),
[full engine statistics](/home/eric/Developer/Halo-2-Decompiled/include/halo2/source_game_end.hpp:11),
[result ring](/home/eric/Developer/Halo-2-Decompiled/include/halo2/source_result_event.hpp:11),
[grenade sidecars](/home/eric/Developer/Halo-2-Decompiled/include/halo2/native_grenade.hpp:81),
[wire encoder and disconnected-row omission](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_network.cpp:319),
[snapshot producer](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer.cpp:1572).

## Existing seams and their exact limits

1. **Campaign in-process checkpoint.** `Simulation::SavedState` copies typed
   scene/device/prop/unit/projectile/script/allocator/RNG/clock state and
   restores it with collision rebuilding and transient presentation cleanup.
   It excludes the enclosing match/recording/round banks, grenade sandbox and
   retained original object/physics runtime. `debug_save_snapshot` and
   `debug_load_snapshot` explicitly reject multiplayer. Moreover, copying a
   `DatumPool` that views source memory throws “A source pool view requires a
   whole-bank checkpoint owner.” Extending this checkpoint with only player
   positions is therefore insufficient even for an in-process source world.
   Sources: [SavedState and capture](/home/eric/Developer/Halo-2-Decompiled/src/simulation.cpp:107),
   [restore](/home/eric/Developer/Halo-2-Decompiled/src/simulation.cpp:170),
   [multiplayer guard](/home/eric/Developer/Halo-2-Decompiled/src/simulation.cpp:244),
   [source pool copy guard](/home/eric/Developer/Halo-2-Decompiled/src/datum_pool.cpp:28).

2. **Retained original source bank.** `SourceRuntimeMemory::visit_committed`
   exports borrowed address/byte spans for diagnostics. It omits uncommitted
   pages and does not export region access, reservation/commit topology or
   owner metadata. The bank is noncopyable and has no checkpoint/import API.
   The owning physics runtime additionally holds virtual-memory/pool backing,
   synchronization/lock/thread registrations, allocation/resource-read maps,
   process callbacks, arena and cache bindings outside the byte bank. A raw
   memory dump alone is not a proven clone of these owners.
   Sources: [memory contract](/home/eric/Developer/Halo-2-Decompiled/include/halo2/source_runtime_memory.hpp:11),
   [diagnostic traversal](/home/eric/Developer/Halo-2-Decompiled/src/source_runtime_memory.cpp:77),
   [TU1.5 owner sidecars](/home/eric/Developer/Halo-2-Decompiled/src/physics_live_world_tu15.cpp:50),
   [other source-runtime sidecars](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_object_runtime.cpp:80).

3. **Partial world replica imports.** Prop-removal import validates actual
   placement/definition/generation and rebuilds collision. Device import
   validates and atomically replaces complete groups/owners. Both are useful
   subowners; neither imports a match. Device import sets `replica_tick_`, and
   `step`, control activation and script writes then reject client authority.
   There is no device promotion transition to resume its motion clock.
   Sources: [prop import](/home/eric/Developer/Halo-2-Decompiled/src/simulation.cpp:44),
   [device import](/home/eric/Developer/Halo-2-Decompiled/src/source_device_world.cpp:180),
   [replica step/write guards](/home/eric/Developer/Halo-2-Decompiled/src/source_device_world.cpp:118).

4. **Existing prepared world.** `MultiplayerPreparedWorld` keeps the actual
   cache/content/source runtime/match alive across load-to-render handoff;
   network transports remain runtime-owned. Its constructor configures source
   objects and attaches source pool views; it is not a migration reconstruction
   API. Preserve this lifetime model when adding continuation rather than
   creating a second authoritative match with independent salts.
   Sources: [prepared owner](/home/eric/Developer/Halo-2-Decompiled/include/halo2/multiplayer_world.hpp:8),
   [construction/binding](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_world.cpp:27),
   [source pool views](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer.cpp:191).

5. **Source departure/result kernels.** `source_simulation_player_depart`
   implements the whole `84F30` gate followed by `152DF0/157DE0`, retaining
   unknown record bytes and requiring reached external services. The result
   remapper preserves the complete `DE78` retained context and the standalone
   full-scoreboard comparator already handles original left-player/team
   ordering. These owners should be integrated, not replaced by deleting the
   roster or manufacturing suicide/death/quit XP events.
   Sources: [service requirements](/home/eric/Developer/Halo-2-Decompiled/include/halo2/source_player_departure.hpp:45),
   [retained context](/home/eric/Developer/Halo-2-Decompiled/include/halo2/source_result_retention.hpp:31),
   [unchanged-fixture comparison](/home/eric/Developer/Halo-2-Decompiled/tests/player_departure_tests.cpp:135),
   [left-player standings](/home/eric/Developer/Halo-2-Decompiled/tests/player_departure_tests.cpp:83).

The current live leave immediately drops carried weapons, retires the player
salt and clears the record. The original ordinary departure instead requests
deferred force depletion with lethal statistics suppressed, unlinks current
player ownership and retains the left player. Its unit damage/death/drop
callbacks still need real source owners. Do not invoke an ordinary stock-SMG
lethal hit or the unchanged native `finish_death` as a substitute. Detailed
required event/flags/callback limits are in the existing
[departure-unit-damage evidence](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/halo2-departure-unit-damage-evidence.md).

## Integration inventory

The items below are engineering requirements inferred from the owner graph;
their proposed native format/transport is not recovered Xbox wire behavior.

| Area | Required implementation and acceptance boundary |
|---|---|
| Complete continuation owner | Add a versioned capture/validate/import path over all admitted mutable match, simulation, object/physics and recording owners above. Quiesce at a completed authoritative tick, retain immutable map/image identity, verify all handles/links/cursors, and rebind process-local resources. Both typed and raw source banks must agree after import. A validated in-process save/restore is useful first evidence but is not cross-machine migration. |
| Replicated promotion candidate | Deliver sufficient authoritative continuation before loss, or retain an equally complete running distributed world. Abrupt loss cannot request an unreplicated private bank from the departed machine. A bounded last acknowledged checkpoint may imply rollback; any such native rollback policy must be explicit and tested, not attributed to Xbox. |
| Native session/relay epoch | Add migration/reestablishment state, an authority generation, survivor membership, candidate acknowledgement of a particular complete checkpoint/tick/digest, and old-generation rejection. A server-selected host must be a surviving physical connection owner, not a guest/local child slot. Bound retries/failure without fabricating completion. |
| Runtime role transition | Freeze admitted game ticks during transition, replace/rebind `LanClient`/`LanHost` with a validated import/promote operation, reestablish cookies/baselines/input serials and surviving local slots, then resume the preserved clock. Current runtime constructs roles once and exits on Failed. |
| Ordinary disconnect | Run the retained departure owner once for every logical local seat owned by the lost connection. Preserve salted participant/configuration/statistics rows through final PCR; remove only their active control/machine membership. Recompute playing/retained teams through original owners. True slot deletion is a separate operation. |
| Result ownership | Preserve original admitted participants and per-room guest classification across migration. Current publication requires the host and every original physical connection, and confirmations require all token owners. Reconciliation after departure needs an explicit native policy and complete retained PCR state; survivor-only confirmation is not established original arbitration policy. Guests remain excluded from persistent personal stats even when their shared PC becomes host. |
| Voice/presentation | Reattach shared-PC voice routing and retained block/mute membership to the new physical authority. Reset/rebase transient effect/event consumers against the imported bank; keep chosen names, private account binding and local controller/view bindings intact. Postgame voice host replacement is not gameplay-world migration. |

Sources: [fixed runtime roles](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_runtime.cpp:242),
[runtime failure/result path](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_runtime.cpp:1587),
[session phase contract](/home/eric/Developer/Halo-2-Decompiled/include/halo2/online_match_session.hpp:13),
[session control parsing](/home/eric/Developer/Halo-2-Decompiled/src/online_match_session.cpp:131),
[physical couch owners](/home/eric/Developer/Halo-2-Decompiled/multiplayer/master_server/match_transport.py:22),
[postgame/result acknowledgements](/home/eric/Developer/Halo-2-Decompiled/multiplayer/master_server/match_transport.py:260),
[active disconnect closes room](/home/eric/Developer/Halo-2-Decompiled/multiplayer/master_server/match_transport.py:342).

## Validation performed and still required

Fresh isolated binaries were compiled directly from the current departure,
retention and scoreboard source and run against the existing frozen original
fixtures. All 18 original departure/gate controls and 1,224 whole retained
remap/machine calls passed (1,111 mutations). Commands,
source/fixture/binary SHA-256 values and exit codes are in
[kernel-verification.json](/home/eric/Developer/Halo-2-Decompiled/.local/native-migration-state-audit-20261013/kernel-verification.json).
Logs: [departure](/home/eric/Developer/Halo-2-Decompiled/.local/native-migration-state-audit-20261013/departure-test.log),
[retention](/home/eric/Developer/Halo-2-Decompiled/.local/native-migration-state-audit-20261013/retention-test.log).
The source-reference hashes and successful link/line check are recorded in
[inspected-reference-manifest.json](/home/eric/Developer/Halo-2-Decompiled/.local/native-migration-state-audit-20261013/inspected-reference-manifest.json).
These controls establish only the declared kernel branches/whole-record
comparisons. They do not execute native live-unit departure, original network
election, source-world restoration or a WAN match.

Required continuation proof includes checkpoint/import equality followed by
identical admitted future tick inputs, preserving all allocation handles/RNG,
damage attribution, reloads/inactive inventory, pending grenades/projectiles,
moving devices, destroyed props, source physics and recording rings. Exercise
actual graceful host leave and abrupt loss with at least two surviving physical
PCs, ordinary nonhost loss, a departing four-seat couch, repeated migration,
candidate loss during election, stale old-host/input/snapshot rejection, and
event/voice rebinding. Each successful survival case must finish with a valid
retained PCR, correct left-player/team placements and guest-stat exclusion.
An election fixture or a visible player-position match cannot substitute for
those proofs.
