# Original map scripts, primary randomness and machine lifetimes

The authority is the supplied original multiplayer cache syntax and unchanged retail executable (`default.xbe` SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`). This records bounded HS initialization/dispatch and machine recreation evidence. It does not establish a complete game scheduler, a live launch seed, all script host services, networked device replication or final framebuffer parity.

## Initialization and scheduling

`18F000` calls module initialization `137CA0`, then automatic persistent creation `D5560(0)`, before BSP/player initialization `11C1B0`. The 68-row module table at `440DD4` has ordinary initialization callback `11BE20` in row 24 and HS initialization callback `20A750` in row 54. HS initialization therefore occurs after the former callback and before the first automatic object-creation phase.

The initialization oracle executes full `20A750`, including `2A09C0` syntax-string relocation/object-list pool reset and `2090A0` global initialization/thread activation, over all 21 authored caches. Each original initialization produces zero primary random draws and no supplied object-factory/destruction event. Global initializers run synchronously. Startup/continuous threads begin with wake 0; dormant threads begin with wake -2 and preserve their continuation.

| Maps | Initially runnable roots | Initially dormant roots |
| --- | --- | --- |
| Colossus | `recycling_can_spawner_r`, `recycling_can_spawner_l`, `hanger_can_killer` | None |
| Elongation | `crate_spawner_right`, `crate_spawner_left`, `crate_eraser` | None |
| Triplicate | `train_of_death` | `_stealth_toggle_monitor`, `_stealth_timer_monitor` |
| Backwash | `backwash_weather` | Same two monitors |
| Turf | `mission_main` | Same two monitors |
| Waterworks, Zanzibar, Containment, Dune | None | Same two monitors |
| Other 12 maps | None | None |

`137FE0` calls `209310` during an ordinary simulation tick before object updates. The dispatcher visits actual thread-pool ordinals and executes a nonnegative wake when it is at or before the current clock. A completed continuous root restarts on the next dispatcher tick. A rendering frame can contain zero ordinary simulation ticks; initialization does not itself dispatch startup roots. This evidence does not justify forcing an extra pre-render script tick or claiming that destruction always precedes the first possible world draw.

## Primary random stream

`include/halo2/source_script.hpp` and `src/source_script.cpp` expose arithmetic without owning or initializing a stream:

- `source_hs_random_range(primary, lower, upper)` implements original `2A29D0`: one wrapping LCG advance, wrapping integer product, then the zero-extended SHORT result. Valid bounds select from `[lower, upper)`. Equal or reversed bounds preserve the original arithmetic and still advance the stream.
- `source_hs_begin_random(primary, count, visited)` implements original `207CF0`: advance once on every dispatch, including completion; derive a starting child from the high random word; search cyclically for an unvisited child. The admitted frame has at most 32 children. The supplied Backwash expression has four.

`ScriptVm::set_random_choice` accepts an explicit `(count, visited) -> optional child` callback. The caller must pass the same primary state reference used by host `random_range`. The VM marks the selected child before entering a queued expression, retains the visited word across nested calls and sleeps, and asks again after that child returns. An up-front shuffle changes the original random-call order. Completion returns the last child's raw result slot, separately from the visited bitmap: instruction `207EDD` reads that result slot, and builtin result type 3 makes `209AE0` preserve its bits.

The VM callback does not infer a launch seed or recreate object-constructor random draws. Full initialization/constructor event ordering and the final pre-dispatch primary state remain caller services. Restoring a simulation checkpoint must restore both VM continuation and caller RNG state.

## Actual Triplicate lifetime behavior

At the first original `209310` dispatch, `train_of_death` destroys named objects in this order:

1. Name 4 `eastbound_train`, MACH placement 0.
2. Name 6 `eastbound_death`, MACH placement 5.
3. Name 5 `westbound_train`, MACH placement 1.
4. Name 7 `westbound_death`, MACH placement 6.

It then evaluates `random_range(60, 360)` and sleeps. Later `random_range(0, 2)` selects a train/death pair, creates that pair through the actual name metadata, and sleeps 240 ticks. Completion and next-tick continuous restart produce the subsequent destruction. All four scripted machine placements have private power and position groups.

The authored syntax inventory contains no `device_set_position` or `device_set_power` call in any of the 21 caches. It does not imply that door/platform interaction or network state is absent; those producers are independent of these startup scripts.

## SourceMachineScene lifetime contract

Call `SourceMachineScene::synchronize(SceneObjects)` before `update`. The mapping uses global `LevelContent::objects` indices, full generation handles, actual visibility and actual `created_tick`. Destruction stops vertex and lighting-binding submission. A new generation resets private device position/power, controller-derived runtime flags and the lighting cache, and advances a gear only from its actual creation tick. Repeated updates at the same tick do not advance it again.

Original `106680` destruction followed by `106510`, `115190`, `107A30` and `1076E0` creation is independently executed for all 49 initial machine records. Private group handles are released and fresh generations reset to authored initial state. Named groups retain their actual full handles and current shared values. The simulation-owned device path supplies current shared groups on recreation via `synchronize(scene, &devices)`; the unbound diagnostic overload still rejects named-group recreation. A newly live placement without resident prepared geometry also throws. The supported Triplicate recreated objects use private groups. Native LAN snapshots now carry retained machine handles, creation ticks, transforms and device state. Clients observe this bank without local HS/random execution. This does not emulate the original network object-construction/effect services; see [current map scope](MAP_WORLD_FEATURES.md).

The 49 original machine controller paths all select initialized idle NONE and perform zero `1DD630` calls or primary/secondary RNG writes through the retained controller/channel construction. That proof excludes the rest of `B7B40`, model selection, attachment/effect creation and material object initialization.

## Retained comparisons

| Fixture | Scope and result |
| --- | --- |
| `machine-hs-startup-original.py` / `.json` | Full original HS initialization for all 21 caches; explicit-state Zanzibar/Triplicate thread, argument, sleep, branch and random dispatch |
| `machine-hs-native-differential.json` | 10 Zanzibar/Triplicate map/seed cases, 11,000 tick dispatches, 113 exact event/RNG records |
| `hs-random-range-original.py` / `hs-random-range-differential.json` | 4,822 original range cases, 9,644 bit-exact state/result words, including signed-boundary/equal/reversed inputs |
| `hs-begin-random-original.py` / `hs-begin-random-differential.json` | 5,482 original frame/choice cases, 10,964 bit-exact state/choice words and 16,446 frame checks; child execution/return propagation supplied |
| `backwash-hs-original.py` / `backwash-hs-native-differential.json` | Actual compiled Backwash roots: five explicit seeds, 100,000 tick dispatches, 307 exact random-choice/range/weather-command records; weather state producer excluded |
| `device-controller-original.py` / `.json` | 49 original constructor/controller fixtures plus 49 destruction/recreation fixtures, exact private/named group semantics |
| `source_script_tests` | Resumable random choice with sleeps and inner random draws, final completion draw, raw result, empty/completed frames and unsupported-frame rejection |
| `source_machine_scene_tests` | Actual Triplicate metadata, destruction, fresh full handle/creation tick, age-zero and age-one pose reset, same-tick stability and hidden-object submission |

The original HS fixtures supply object factory/protection/deletion and object-list garbage collection at explicit boundaries. Original name resolution, syntax evaluation, scalar conversions, thread allocation/dispatch and random arithmetic still run. The Backwash comparison records original weather-call arguments after conversion and leaves the separate weather state/render update to its own oracle. The current native multiplayer owner now supplies object-list pools and a bounded static-volume adapter, and starts authored roots as described below. Complete ordinary object-constructor RNG chronology, original spatial candidate registration order, attached volume transforms and complete object-construction/effect consumers remain unresolved. A replicated typed device bank does not supply those missing producers.

## Earlier diagnostic survey on 2026-10-12

`analysis/map-world-features/script-runtime-survey/` explicitly activates the
existing native VM only in a diagnostic probe. At that earlier checkpoint ordinary multiplayer did
not call `Simulation::start_level()`. The probe steps 1,200 ordinary
native ticks per admitted map and records current scene generations and VM
faults. It covered all 23 supplied maps. Colossus faults on its named-object
list conversion and `volume_return_objects`; Elongation faults on
`volume_return_objects`. The two Vista caches fail earlier with
`stale/poison script datum`. Triplicate, Backwash and Turf reach 1,200 ticks
without VM faults after explicit diagnostic activation. A missing fault does
not prove original initialization, spatial physics, fade output or RNG parity.

`source_hs_object_list` and `source_hs_named_object_list` now reproduce the
bounded 20A170/20A130 conversion. A live object creates a fresh list and then
appends its full handle; a missing object or name outside signed-short
0..639 returns NONE without allocation. Only the low signed short of a name
is used. The allocator and append service are explicit caller inputs. Even
an allocator returning NONE still reaches append, matching the original
call sequence; the service must handle or reject that boundary. These conversion wrappers delegate list/reference pools and garbage
collection to their caller; the later bank implementation is described below.

The VM has an explicit list-service constructor overload and now converts
compiled object types 50..55 and name types 56..61 to object-list type 31
through those services. Existing hosts provide no implicit list pool. The
caller must restore its list/reference bank alongside VM snapshots, just as
it must restore the primary RNG bank used by callbacks.

`script-list-kernel-snapshot-20261012/compare_original.py` executes the
unchanged Xbox conversion helpers with separately supplied allocation and
append services. All 1,116 cases and 5,580 output words match, including name
bounds, poisoned upper name bits, absent handles, full salted handles and
allocation failure. VM operand fixtures cover all twelve object/name types.
Focused and ASAN/UBSAN checks pass. This removes a scalar-conversion gap and
did not by itself enable production spawners, trains or startup weather roots.


## Production roots and list pools on 2026-10-12

Ordinary `MultiplayerMatch` now owns a `SourceHsListBank` and calls
`Simulation::start_level()` after installing its host services. Only authored
startup/continuous roots start; dormant roots retain their continuations. The
ordinary simulation tick dispatches them before device/object updates. Clients
observing snapshots do not independently run the authoritative scripts.

`SourceHsListBank` retains the original 48 twelve-byte headers and 128
twelve-byte references. Allocation, prepend, signed-short indexing, reference
exhaustion, root counts, collection, and full salted handles follow
`1DED60`, `1DEDB0`, `2A0280` and `1DEED0`. VM roots are refreshed before an
allocation can trigger collection and after dispatch. Checkpoints copy the
complete bank alongside VM state. The native player-list sentinel is kept
outside that pool. Campaign's existing native list owner is preserved.

`script-list-bank-snapshot-20261012/verification.json` retains 213 operations
and 115,659 output words against unchanged original routines with real datum
pools, with zero mismatches. The separate `11C5F0` volume collector comparison
passes 84 cases and 45,612 output words with explicit transform/location,
candidate and point-test services. The new bank and fixtures also pass
ASAN/UBSAN. These comparisons do not establish the spatial candidate producer.

The production volume adapter uses authored static bounds and query radius,
current scene/player/actor centers and strict-open point containment. It
retains native scene placement order; it does not reproduce `BB050` cluster
registration order. Attached volumes and candidate banks over 128 fail
explicitly. This boundary matters for allocation and object iteration parity.

Vista syntax audits now ignore zero-salt free records, whose retained payload
can contain obsolete links. References to those records still fail. The
unchanged original datum routines and both raw Vista syntax banks establish
the occupied/free distinction; each supplied Vista bank has 517 free records
and nine occupied records.

The ordinary match probe now runs all 21 Xbox maps and both Vista-only maps
for 1,200 ticks without VM faults. Colossus creates its cans, Elongation creates
its crates, and Triplicate destroys/recreates its trains. Named NeverPlaced
props have resident geometry before their scripts create them; submission
follows actual full-handle scene lifetimes. The 21-cache prop fixture preserves
the automatic initial submitted geometry and verifies script-born generations.

See [normal launcher delivery](SOURCE_WORLD_LAUNCHER_DELIVERY.md) for the
installed runtime and live checks. Fault-free roots and visible new geometry
do not prove falling/conveyor physics, crushing, complete prop lifetime
replication/reset, original constructor RNG chronology or all attached effects.
The full all-map goal remains open.
