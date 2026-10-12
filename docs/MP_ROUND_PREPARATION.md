# Original preparation replication and constructed-world phase transitions

The shared native phase owner now uses the original preparation replication
gate. Entering phase 3 increments the signed round index, then marks the match
entity dirty with flag 8 only when that entity is not NONE. A missing reached
replication owner still stops before writing the preparation countdown. The
previous unconditional, separately named notification callback incorrectly
required a provider even for NONE entities.

The actual original TU1.1 post-world controller also completes two inter-round
transitions on a source-joined player and constructed Lockout world. It invokes
the full scenario reset through the phase initializer, rather than through a
separate direct reset call. These checks do not bind the native live-world
reset or establish a full playable Elimination match.

## Original replication contract

| Owner | Retail | TU1.1 | TU1.5 |
|---|---|---|---|
| Preparation wrapper | A7810 | A85C0 | B1A10 |
| Dirty-entity owner | B58C0 | B6760 | B7E50 |

Each complete wrapper checks the registered selected engine, then the match
entity at engine globals+24. It returns immediately for an absent engine or
NONE entity. Otherwise it forwards the original entity and flag argument to
the dirty-entity owner. This is the same owner used with flag 2 after the phase
callback and phase-field store; flag 8 is a replication mask, not a separate
message constructor.

[The preparation oracle](../tools/oracle_round_preparation.py) executes 288
wrapper cases and 36 complete-or-stopped phase cases across all three original
executables. Inputs include actual registered engine selectors, NONE, zero and
salted entity values, arbitrary flag masks, signed index wrap and signed clock
rates. Two complete instruction comparisons establish wrapper correspondence.
Non-NONE cases stop at the actual dirty-entity entry without supplying a return;
NONE phase cases execute the original Slayer callback and return naturally.

The native comparison checks all 0xC18 engine bytes for the 36 phase cases,
including the partial state before a missing owner. It also removes the callback
entirely in the reached cases. The existing 1,746 phase/post-world comparisons
retain their complete bank/player/row checks. Their NONE match entities now
need no substitute notification callback. This changes the shared control
owner and its tests, without supplying session tracking or enabling a variant.

## Original player activation and repeated phases

[The constructed-world probe](../tools/oracle_owned_round_phase.py) reads the
previous reset tool, report and verification as immutable inputs. The complete
storage initialization, tag postprocessing, declared index-header delivery and
separate cold audio prefixes reproduce their previous records exactly. The
whole guest executable text remains unchanged, and the live compatibility
worker remains alive throughout execution.

Map reset initially leaves the phase inactive. The probe calls original
1614F0 with the explicit first-active status request, then complete 15DBC0.
The joined player initially lacks active flag 1, so complete 15F3A0 rejects
respawn admission. Complete 14E9E0, the activation scan called by 14EAE0,
executes the real membership query and registered player callback. The later
admission queries succeed using the source-produced flag and life state.
No player active bit, life, readiness result or membership return is assigned.

The retained simulation context has its initialization byte clear. Original
688F0 therefore takes its actual immediate-true branch. This is the recorded
non-network API context, not proof of active original System Link membership.
The direct 1509C0 spawn remains a separate API call after admission; the complete
ordinary player-update/input/session flow is not executed.

Each inter-round sequence begins with an explicit original status-2 request,
followed by the complete original post-world controller. Delivered clock
increments remain declared inputs. At 30 Hz, phase 2 waits 150 deliveries,
phase 3 increments the index and waits 30, then phase 1 runs the actual full
object/map reset and restores player life state. No round index, countdown or
phase result is assigned by the probe.

The first nested reset deletes the biped and attached SMG. After another
source spawn, the second deletes the 19 rebuilt scenario objects and both
player objects: 23 actual object deletions total. Both transitions reconstruct
the same 19 authored objects, 15 physical components/bodies and six looping
records. The surviving player's current/previous/watched references become
NONE, and its spawn count advances from 1 to 2. Every inherited supplied
gameplay/resource return is guarded during phase, admission and activation
calls; the reset's recorded RNG stores execute in original instructions.

## Validation and remaining work

```sh
.tools/python/bin/python tools/oracle_round_preparation.py \
  --fixture tests/fixtures/round_preparation_original.json
.tools/python/bin/python tools/oracle_round_preparation.py \
  --output analysis/multiplayer/game-variants/round-preparation/reproduced/report.json
.tools/python/bin/python tools/oracle_owned_round_phase.py
.tools/python/bin/python tools/oracle_owned_round_phase.py \
  --output analysis/multiplayer/game-variants/owned-round-phase/reproduced/report.json
cmake --build build --target round_lifecycle_tests game_end_native_tests -j2
ctest --test-dir build -R '^(original_round_lifecycle|native_game_end)$' --output-on-failure
.tools/python/bin/python tools/verify_round_preparation.py
```

The source reports and instruction listings must reproduce exactly. The
verification records retain the native build/test logs, scoped ASan/UBSan
results, source/binary pins and the exact original contracts. Sanitizer coverage
is limited to the changed owners and direct shared writers; remaining libraries
are ordinary builds.

Complete native world/scenario reset, ordinary player-update spawning, combat
produced round endings, original replication/session tracking, normal audio
startup and full edited-setting local/network matches remain required. The
recorded catalogue admission stays at 2 of 39 built-in variants. The goal
continues to require functional original tag/data and general weapon/equipment
paths, rather than named-variant shortcuts.
