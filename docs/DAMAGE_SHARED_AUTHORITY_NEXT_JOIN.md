# Next join for one live damage authority

2026-10-12. Research only; no production or test source was changed. Current
production already connects TU1.5 object vitality to the completed damage
module. The remaining join is **real damage producers → retained source body →
native gameplay/presentation readback**. Adding readback alone would erase
current host damage. This note supersedes the missing-switch recommendation in
[the earlier coverage handoff](DAMAGE_COMPLETE_LIVE_COVERAGE_HANDOFF.md).

The [repeatable audit and pinned report](../analysis/damage-shields-goal-20261012/damage-authority-next-join/report.json)
verify all 90 selected paired bodies and all 8,976 instruction rows against
both unchanged executables, plus the current generated-module manifest.
TU1.1 bytes come from the explicitly retained backup assets; TU1.5 bytes come
from the owned imported English executable. No assets or launcher configuration
were restored. [Audit](../analysis/damage-shields-goal-20261012/damage-authority-next-join/audit.py).

## What is connected now

`PhysicsLiveWorldTu15::required` has `Module::Damage`, calling
`source_run_damage_delivery_runtime` with the retained bank, complete ABI,
`TitleUpdate15`, FP controls, observer and existing lower-owner service.
`step_objects` schedules actual `13B6F0, B8F40, 1CC8D0, B9090, B9180`, then
increments the source clock. `B8F40` calls `BE4A0`; at **BE554** it pushes the
full object and at **BE557** original bytes `E8 D4 9D 01 00` call **D8330**.
That entry selects canonical `D73E0` in `Module::Damage`. The complete vitality
update is therefore reachable through the actual TU1.5 object update, rather
than a host recharge substitute. Reaching and returning every branch still
requires its real lower services.
[World dispatcher/scheduler](../src/physics_live_world_tu15.cpp),
[entry table](../src/physics_live_entries_tu15.hpp),
[original B8F40](../analysis/damage-shields-goal-20261012/damage-authority-next-join/000b8f40-u5.asm),
[original BE4A0](../analysis/damage-shields-goal-20261012/damage-authority-next-join/000be4a0-u5.asm).

The general table contains 86 `Damage` rows. Four of the 90 actual entries
already belong to earlier modules: `B8250→B9390` (World), `B9C20→BAD60`,
`BBCB0→BCDF0`, and `177A10→178E30` (Phantom). Additionally, support ownership
precedes this table for 13 damage entries, including **DA0D0**. Its existing
support body compiles the complete 533-instruction dispatcher, and support
callers such as `DFEE0` call it directly. Consequently adding a Damage switch
does not make every damage call execute the shared module. This is an overlap
of complete authenticated owners; no missing return should be invented to
remove it. If consolidating ownership, remove the duplicate support roots and
regenerate their calls, or explicitly preserve and compare those complete
owners. Reordering only `required` does not intercept a static `run_da0d0`.
[Actual support graph](../src/physics_live_support_tu15_generated.cpp),
[original DA0D0](../analysis/damage-shields-goal-20261012/damage-authority-next-join/000da0d0-u5.asm),
[original kill-volume producer](../analysis/damage-shields-goal-20261012/damage-authority-next-join/000dfee0-u5.asm),
[all 90 route selections](../analysis/damage-shields-goal-20261012/damage-authority-next-join/report.json).

`MultiplayerObjectRuntime::call` already maps canonical IDs to actual TU1.5
entries and uses `run_actual`; it can call the existing damage/vitality roots.
`PhysicsLiveWorldTu15::run/call` instead enter the world module directly, so a
new raw damage API should use `run_actual`. The TU1.1 convenience composition
`source_run_damage_1_1` deliberately fixes `MapPack11` and cannot be reused for
TU1.5. The request constructor `D7C60→D8BB0` exists in support but has **no
canonical row** in the current general table: expose its authenticated binding
explicitly when adding a typed request owner. The table also retains actual
Phantom IDs for the three deduplicated aliases above; do not assume every
canonical damage root is exposed by the public lookup.
[Public call boundary](../src/multiplayer_object_runtime.cpp),
[TU1.1 composition](../src/source_damage_runtime.cpp),
[original request constructor](../analysis/damage-shields-goal-20261011/falling-producer/damage-request-owners/pairing/instructions/000d7c60-u5.asm).

## Readback that can be implemented without unknown behavior

Add a read-only vitality view beside `MultiplayerObjectRuntime::biped_motion`.
Resolve `object_body(full_datum)` each time; its checked datum-pool lookup
rejects retired salts before dereferencing the body. Preserve float bits,
signed widths, the runtime maxima and the complete flag word. Reuse the
existing feedback structure for its four floats and two signed timers.
[Checked body/motion seam](../src/multiplayer_object_runtime.cpp),
[feedback storage](../include/halo2/damage_feedback.hpp).

| Body offset | Readback meaning | Original owner evidence |
| --- | --- | --- |
| `E4/E8` | Runtime maximum body/shield | `D8330` reads maxima; dispatcher/body/shield owners use them directly |
| `EC/F0` | Normalized body/shield, including overshield fractions above one | `D89CB/D89D5` body stores; `D8681/D868B/D88B2/D88BC` shield stores |
| `F4/F8` | Visible recent shield/body damage | `DBB90` stores these independently of vitality loss |
| `FC/100` | Accumulated recent shield/body damage | Same report owner; retain separately from visible damage |
| `104/106` | Signed 16-bit shield/body stun | `D890A/D89F0`; shield `7FFF` has its original special handling |
| `108/109` | Signed 8-bit shield/body recent-damage timer | `DBB90` resets; `DB0A0` fades; `D8330` has its separate shield branch |
| `10A` WORD | Complete source damage-state flags | Masks below are exact bit/width operations in the paired owners |

Compatibility projections are dead `0004`, shield damaged `0002`, shield
depleted `0008`, body recharging `0100`, shield recharging `0200`. Retain
overshield charge `0010`, shield-feedback suppression `0800`, and every other
bit verbatim in `Unit::source_unit_10a/source_unit_10b` or an equivalent raw
view. Do not infer `RechargeState::paused` from an opaque body flag: the
original update calls its pause owner. The runtime maxima belong in the view;
the authored biped definition is a separate input.
[Paired full TU1.5 update](../analysis/damage-shields-goal-20261011/vitality-runtime/pairing-full/instructions/000d73e0-u5.asm),
[death flag writer](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/instructions/000d7e00-u5.asm),
[report writer](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/instructions/000dac40-u5.asm),
[host Unit](../include/halo2/simulation.hpp).

Publish source readback after birth, after an admitted source damage/recovery
event, and after the ordinary source object update **before native death,
scoring, snapshot, HUD or material consumers**. The current motion/equipment
publication in `MultiplayerMatch::step` is the existing tick seam. Snapshots
already read `Unit::vitality` and `damage_feedback`; the material callback
already consumes those snapshot fields, so they need no second damage
calculation. Keep dead/previous full object identity available for corpse and
camera consumers after current-player detachment.
[Birth/tick/snapshot owners](../src/multiplayer.cpp),
[material consumer](../src/multiplayer_runtime.cpp),
[authored feedback evaluator](../src/damage_feedback.cpp).

## Activate authority together with its producers

1. **Move admission before host mutation.** `Simulation::damage` currently
   applies scalar damage, clears/accumulates host feedback, disrupts camouflage,
   resets zoom and changes death state before invoking `damage_hook_`. That
   hook cannot be converted into a source-authority adapter after the fact.
   Introduce the source event boundary before these writes and publish the
   actual produced body/report afterward. Source-owned units must not also run
   scalar `recharge` or `update_damage_feedback`; unbound native units retain
   their existing owner. Enable these changes only when that producer actually
   reaches the source graph. [Host damage/tick](../src/simulation.cpp).
2. **Preserve each producer's real inputs.** Current `trace` reduces a capsule
   hit to `Unit*` and a scalar damage call; it carries no source model entry,
   section or node. The area path already reduces nearest point/obstruction
   and amount before the same call. Melee likewise computes a scalar amount.
   None can be relabelled as original `FFA60`, radius or melee admission.
   The real dispatcher takes a writable `88`-byte request plus full target,
   three signed low16 indices and response pointer; it returns `RET18` with
   EAX commonly `FFFFFFFF`. `D8BB0` supplies construction, `D8C20` supplies
   attribution, and the genuine impact/radius/melee producer supplies the rest.
   Preserve recursive request ownership and normalized requested report
   `+44/+48`, including overflow; never derive feedback from measured loss.
   [Native producers](../src/simulation.cpp),
   [full dispatcher ABI](DAMAGE_DISPATCH_RUNTIME_HANDOFF.md),
   [raw direct-impact edges](../analysis/damage-shields-goal-20261012/damage-coverage-research/primary-v1/report.json).
3. **Retire parallel writers and duplicate aftermath per admitted family.**
   Current Match kill volumes directly set host body/dead; `finish_death`
   invokes source death/cleanup again as a confirmed-fatal native boundary.
   Once original postphysics/dispatcher death is admitted, consume its actual
   state/attribution and avoid repeating already executed source aftermath.
   Spawn initialization, debug healing and script vitality/shield setters are
   additional host writers requiring an explicit source boundary. Health
   restore `D8CE0` and overshield activation `D9040` are already paired callable
   owners (`EAX=full object`, plain RET); an actual pickup still needs its
   producer. [Match death](../src/multiplayer.cpp),
   [debug healing](../src/debug_game.cpp),
   [script setters](../src/simulation.cpp),
   [health/overshield source contracts](SOURCE_VITALITY_RUNTIME.md).

The earlier TU1.1 composed pending entries map through authenticated operands
to `17AFB0, B98B0, 32CCD8, E7D30, E9600, 1CE670`. Current TU1.5 support already
owns `17AFB0`; the other five have no current general support/table owner.
Their actual admission/order must be measured on the live body rather than
copied from old declared fixtures. `32CCD8` is the unresolved paired acos/CRT
boundary; preserve that stop and original prefix. [Current route audit](../analysis/damage-shields-goal-20261012/damage-authority-next-join/report.json),
[retained precision discrepancy](SOURCE_DAMAGE_DELIVERY_RUNTIME.md).

## Meaningful verification available now

The existing raw-module comparator was rerun successfully for both images:
4,462 delivery cases and 7,168 vitality cases in total. Every image retains
40 delivery continuation phases and the 2,801-phase overshield trajectory.
These compare complete declared banks/CPU state, ordered stores and required
owner stops. The report pins the existing binary, four fixtures and four
logs; this is raw-module evidence, not a rebuilt live TU1.5 composition test.
[Results](../analysis/damage-shields-goal-20261012/damage-authority-next-join/report.json).

The next integration check should construct each actual Spartan/Elite body,
export the retained bank/full ABI at BE557 or a genuine damage producer, and
replay that same entry in the unchanged TU1.5 executable. Compare ordered
stores, exit/pending ABI and all retained bytes. Existing raw tests already
perform these comparisons; existing live-world diagnostics export committed
banks and ordered stores. Add readback assertions against exact body bytes,
including nonzero signed timers, raw flags, overshield values, stale full
handles and repeated hit/death/rebirth. Verify that one tick executes D8330
once for each eligible object and that rendering additional views causes no
damage writes. A natural hit must produce the request, mutate the source body
and reach host snapshot/HUD/material in that order. Do not count a manually
edited body or diagnostic request as a natural producer test.
[Strict module comparator](../tests/source_direct_damage_runtime_tests.cpp),
[live-bank diagnostic export](../tests/physics_live_world_tu15_tests.cpp),
[unchanged-original live-bank replay pattern](../tools/differential_physics_live_world_tu15.py).
