# Track `units` (unit, biped and vehicle type callbacks)

Native in `src/gameplay/objects/native_object_factory_units_tu15.cpp` (API in
`include/halo2/native_object_factory_units_tu15.hpp`); oracle cases in
`tools/native_object_factory_cases_units.py`, harness kernels in
`tests/native_object_factory_units_kernels.cpp`.

The track links the coordinator's `object_world_position` (core BBE00), the
base factory's `object_block_allocate` (BE3B0),
`native_tu15_wake_object_ancestors` (B9390) and track weapon's animation
graph natives (`weapon_animation::graph_animation` 1E3150,
`graph_require_resident` 1E5C80, `tag_block_search` 1E5810,
`manager_set_state` 1D5320, `choose_animation` 1D46A0,
`manager_start_channel` 1D36C0) and track deviceanim's
`device_animation::overlay_lookup` (1E3F30), and C6850's tail calls track
unitactions' `unit_actions::request` (E8E50) and `seat_drivers_update`
(CECF0). Vehicle new calls objtypes' deferred `object_wake` (BBBC0), and
biped physics mode changes call havok's `havok_component_state_flags`
(1CC310), preserving the live FP controls through biped initialization.
Build it with
`-DHALO2_NATIVE_OBJECT_FACTORY_TRACKS="units;unitactions;weapon;core;nodes;deviceanim;nodebuild;placement;objtypes;havok"`
(deviceanim links nodebuild's and placement's matrix helpers).

## Type callbacks (0x474B90 slots)

Coordinator shapes (`ObjectFactoryServices`) plus the caller's FP controls
(`UnitFp` = `NativeTu15PhysicsFrameFp`):

| Slot | Original | Native | Status |
|---|---|---|---|
| unit +2C new | C6440 | `unit_new(mem, unit, request, failed, scratch_slot, fp)` | native |
| unit +38 place | C6850 | `unit_place(mem, unit, fp)` | native; stops at B9B70 (weapon births) and at track unitactions' refusals past E8E50/CECF0 |
| unit +3C new failed | C68B0 | `unit_new_failed(mem, unit)` | native |
| unit +74 nodes | 1157D0 | `unit_nodes_changed(mem, unit, count, matrices, fp)` | native |
| biped +28 validate | DE9E0 | `biped_validate(mem, request, mxcsr)` | native |
| biped +2C new | DEA60 | `biped_new(mem, biped, request, failed, fp)` | native (modes 1 and 2) |
| biped +3C new failed | DEA70 | `biped_new_failed(mem, biped)` | native |
| vehicle +2C new | F10E0 | `vehicle_new(mem, vehicle, request, failed, fp)` | native, including BBBC0 with DL=1 for definition +2AC bit 0 |

`scratch_slot` is the BE3B0 relocation slot (see `object_block_allocate`);
any value works in production. `unit_new` and `vehicle_new` take an optional
`UnitAnimationStateService` that replaces their 111B10 call (empty: the
native `unit_animation_state_set`); the oracle's C6440/F10E0 kernels use it
to stop where their cases stop the original. 1157D0's count and matrices only reach the
refused IK solve.

## Ported owners

Cases are physical-original fixture cases (seed 0x5C41); coverage is over
each original's complete body.

| Owner | Native | Kernel (cases) | Coverage |
|---|---|---|---|
| 1157D0 | `unit_nodes_changed` | `units_nodes_changed` (800) | full |
| 113B70, 1139D0, 113C70, C9550, CFB40 | `unit_look_enabled`, `unit_aim_enabled`, `unit_animation_allows`, `unit_animation_mode_listed`, `unit_dual_wielding` | through 1157D0, `units_look_enabled`, `units_aim_enabled` (500 each) | full |
| CE230 | `unit_weapon` | `units_weapon` (100) | full |
| 1E14F0 | `animation_fade_start` | `units_fade_start` (600) | full |
| 1E1620, 110FC0, 1E1920 | `animation_fade_weight` | `units_fade_weight` (600) | full |
| 116340, 116280, 116060, 1D4140 | `unit_look_ik`, `unit_aim_ik`, `unit_ik_points` | through 1157D0 | full except the refused marker solve (1161BF..116269) |
| 1E5250, 1E51D0 | `animation_ik_point_next` | `units_ik_next_weapon` (600), `units_ik_next_look` (400) | full |
| 1E5810 | `tag_block_search` (forwards to weapon's) | through the IK kernels | full |
| 1E5360 | `animation_graph_find` | `units_graph_find` (400) | full |
| 1E3150, 1E5C80, 1E3270, 1E5AF0 | track weapon's `weapon_animation` natives | through the above | full except the refused 126200 calls |
| DE9E0 | `biped_validate` | `units_biped_validate` (400) | full |
| DEA60, E0A10, E7E80, E7ED0, E7CA0 | `biped_new`, `biped_initialize`, `biped_physics_initialize`, `biped_physics_mode` | `units_biped_new` (600) | full except mode 3 (E7CE0, refused) |
| 1ED780, 1F86A0, 1F4850 | `biped_physics_mode_set` | `units_physics_mode` (400) | modes 1..3 including 1CC310's no-component return and reset prefix; modes 4/5 and Havok removal/refresh still refuse |
| DEA70 | `biped_new_failed` | `units_biped_new_failed` (200) | full |
| C68B0, CF400 | `unit_new_failed`, `unit_drop_weapons` | `units_new_failed` (200) | full except refused D0950/BA570 paths |
| C6440 | `unit_new` | `units_new` (600), original stops at 111B10; 111B10 and the marker tail by their own kernels | full up to 111B10 |
| 111940, 111720, 111000, 1102A0, 115F50 | `unit_animation_reset` and its steps | `units_animation_reset` (400) | full except 1119EB (null manager) |
| 1175A0 | `unit_variant_record` | `units_variant_record` (400) | full |
| 1708F0 | `model_marker_index` | `units_marker_index` (200) | full |
| F10E0, F0F10 | `vehicle_new`, `vehicle_state_initialize` | `units_vehicle_new` (600), original stops at 111B10; 111B10 and 1D6200 by their own kernels | includes deferred BBBC0 for vehicle headers, up to 111B10 |
| BBF20 | `object_root_origin` | `units_root_origin` (400) | full |
| 1D6200, 1CE7B0 | `vehicle_animation_settle`, `animation_channel_restart` | `units_vehicle_settle` (400) | full except 1D6281..628C (unreachable: +40 was just set to -1) |
| C6850, CEF90 (+ track unitactions' E8E50, EAF00, 1EFA70, CECF0) | `unit_place`, `unit_create_starting_weapons` | `units_place` (400), original stops at B9B70 or a unitactions refusal | C6850 25/25, CEF90 41/67 (the rest follows B9B70), E8E50 33/33, EAF00 103/132 (the rest follows its refusals), 1EFA70 14/14, B9390 26/26, CECF0 188/188 |
| 115E90 | `animation_blend_seconds` | `units_blend_seconds` (600) | full |
| 1E16E0 | `animation_fade_remaining` | `units_fade_remaining` (400) | full |
| 1D38A0 | `animation_interpolation_start` | `units_interpolation_start` (600) | full |
| BC380 | `object_interpolation_start` | `units_object_interpolation` (500) | full |
| 115E20, 1D34E0 (head) | `unit_overlays_restart`, `animation_overlay_restart` | `units_overlays_restart` (200) | full except 1D34E0's refused set-channel path |

| 111B10 | `unit_animation_state_set` | `units_animation_state` (600) | full |
| 115AF0, 1D3D80, 1D3C80, 1D3C20, 1D37D0, 116A10, 1D4450 | `unit_animation_channels_refresh`, `unit_aim_overlay_start` and private helpers | `units_channels_refresh` (400), through 111B10 | full except 1D4450's channel > 2 exit (callers pass 0..2) and 1D3C80's unattached-manager exit (the original faults first in 1D4450) |
| 1D4350 | `manager_has_aim_overlays` (private) | through 111B10 | full |

1D3C80's trial channel record lives on the original's stack; the native
keeps it in a private 0x20-byte overlay of the world memory (`FrameRecord`).
1D3C80 (`manager_overlay_trial`) and 1D4450 (`manager_aim_handle`) are
exported in the header since 2026-11-12; track nodebuild's F2CC0 uses them.
Since 2026-11-15 1D37D0 is exported as well (`manager_start_named_overlay`,
a wrapper of the unchanged private owner): track nodebuild's 16AD45 starts
the first-person +B8 overlay 0x12000068 with it and finds the +DC/+E0/+D8
handles through 1D3C80 (see nodebuild.md, first-person sampler).
The 111B10 kernels use track deviceanim's animation fixture (track weapon's
graphs and managers plus overlay tables) with unit keys added.

Earlier 29-kernel qualification (`HALO2_FACTORY_TRACKS=units`,
`HALO2_ORACLE_STRICT_WRITES=1`, each `--only`; built with
`units;weapon;core;nodes;deviceanim;nodebuild;placement`): PASS 13,000
physical original evaluator tails, 26,000 native comparisons, 324,908 ordered
original stores (8,082 executed original sites; comparison SHA-256
7ee7f8f4201fc955...).

C6850 (integration, 2026-11-12): `unit_place` calls track unitactions'
`unit_actions::request(m, unit, Request::of(8), fp)` (E8E50 performs the
B9390 wake itself) and then `unit_actions::seat_drivers_update` (CECF0). The
`units_place` cases run over track unitactions' world (its `ua_seat_drivers`
builder, borrowed read only through
`tools/native_object_factory_track_fixtures.py`) with starting weapon rows at
TRACK +A100 and B9960's vectors at +A000. They stop at B9B70 and at every
unitactions `REFUSED` entry and refused table callback, recomputed from that
module on each load; the harness kernel treats any named refusal as the
stop (C6850 is void). With every track loaded:
`PASS 1100 physical original evaluator tails, 2200 native comparisons,
32542 ordered original stores` (with track weapon's `weapon_new` and
`weapon_set_state`).

Boundary kernels: the case rows declare `stop_before_entries` (the original
stops at the refused callee's entry); the native refuses that callee by name
and the harness kernel reports the original's register at that entry.
`closure_stops` keeps refused subtrees out of the authenticated closure.
The resumed binding proof also compares the exact stop entry, distinguishing
a normal return from every named refusal even when the return register and
stores happen to match.

Mutations: see [Mutations](#mutations).

## Resumed qualification (2026-11-12)

Two caller refusals now enter existing native owners. F10E0 calls BBBC0
with the original `DL=1` deferred-wake argument. 1ED780 calls 1CC310 with
the object's full datum and flags zero when changing into or out of mode 3;
the biped initialization chain carries both live FP controls. This preserves
1CC310's removal-before-reset ordering and its later named refusals. No
callee result, instruction patch or fallback is supplied.

A fresh all-track build and self-test passed, followed by **13,300 physical
original cases, 26,600 native comparisons and 329,321 identical ordered
stores**: all 29 units kernels plus 300 generic `units_set_maximum_vitality`
cases. The new physics fixtures include unchanged modes, entry/exit of mode
3, absent components, active components stopping before removal, and
inactive components running the native reset before refresh. Vehicle cases
exercise definition +2AC bit 0 with live vehicle headers and both component
presence states.

The focused binding proof compares **2,000 original cases and 4,000 native
results**, including exact normal-return/refusal identity for
`units_physics_mode`, `units_vehicle_new`, `units_place` and `units_new`.
It checks 212,221 ordered stores and reaches original call sites F1166 and
1ED83E, the 1CC369 flags store, and the 1D8470 reset owner. All four source-copy
mutants are rejected: omitted wake, immediate instead of deferred wake,
omitted component reset, and flags one instead of zero. Production source
is never mutated by this screen.

Receipts: [full units cohort](../../build/test-results/resume-factory-integration-20261112/units-bindings/units.json),
[binding comparisons and mutations](../../build/test-results/resume-factory-integration-20261112/units-bindings/mutations.json),
[official harness refresh](../../build/test-results/resume-factory-integration-20261112/units-bindings/stop-identity-refresh.json),
and [commands/source pins](../../build/test-results/resume-factory-integration-20261112/units-bindings/session.json).
The full cohort began before the test-only stop-entry observation was added;
the focused proof covers that stronger contract on all four boundary kernels.
The refreshed harness SHA-256 starts `3e6d91fbe18eb9ee`.
The [closure receipt](../../build/test-results/resume-factory-integration-20261112/units-bindings/closure.json)
records each cohort's source scope, the later comment-only header correction,
and verified removal of the owned builds/probes and retired helpers. No large
artifacts were retained.

This qualification covers the declared native paths and refusal prefixes.
It does not establish complete live object birth, flying/ragdoll physics,
or the Havok removal/refresh/activation owners listed below.

## Census slice status

Executed instructions are the six-map census union over each callback's
executed closure (other tracks' roots excluded; the union includes paths
other callers take, such as biped mode changes into Havok).

| Callback | Slice | Native | Remaining |
|---|---|---|---|
| 1157D0 | 20 fns / 835 | all | none |
| DE9E0, DEA70, C68B0 | 9 / 0 / 0 | all | none |
| DEA60 | 172 / 8,951 | the birth path; 1CC310's no-component return and native reset prefix are now linked | Havok removal/refresh/activation past 1CC310; flying/ragdoll mode owners |
| C6440 | 60 / 5,159 | all (with tracks weapon and deviceanim) | refused rare branches only (126200, weapon's 1E4C39) |
| F10E0 | 55 / 4,693 | admitted constructor paths, including BBBC0's deferred wake (absent from the earlier census) | the same rare animation branches |
| C6850 | 245 / 14,383 | C6850, CEF90 and track unitactions' E8E50/EAF00/1EFA70/CECF0 | B9B70 recursion; EAF00's remaining refusals (see unitactions.md) |

111B10 (49 functions / 4,328 executed instructions) is native: this track's
body, 115AF0 subtree, 1D4350 and leaves, track weapon's 1D5320 subtree and
track deviceanim's 1E3F30. CECF0 (C6850's tail after the action, 95 of 188
executed) is a seated-children driver/gunner walk.

## Refused (not native) dependencies

| Callee | Reached from | Why |
|---|---|---|
| B9B70 | C6850 (CEF90 starting weapons) | coordinator object_new |
| 1E1390 (+BAD60) | 116060 when an IK point has both markers (never executed) | IK solve |
| 126200 | 1E5C80/1E5AF0 with a resident resource (never executed) | animation resource admission |
| E65A0, 204610, 2045B0 | biped physics modes 3..5 (never at birth) | biped flying/ragdoll physics |
| 1D9510, 1D73D0, 1D97F0 | biped mode change through track havok's 1CC310 | component removal/refresh/activation; the intervening reset is native |
| D0950, BA570 | C68B0 with weapons or a held object (C6440 clears both first) | weapon release, object delete |
| 1D34E0 tail | 115E20 with a set overlay channel (never at birth: 111720 clears them) | overlay animation restart |

## Shared code

- 1E3150/1E5C80/1E5810 come from track weapon. Track nodebuild still has
  its own private 1E3150 family.
- The C9550 mode table check also exists privately in
  `native_campaign_script_presentation_tu15.cpp`.

## Mutations

101 source mutations, each rebuilt and run against its kernels: 95 rejected,
four equivalent survivors, two unobserved (details below).

- 1157D0 slice (27): fade store order, x87 rounding, the 0x7F clamp, the
  active-bit compare, the divisor guard, the fresh position, smoothstep
  operand order, the smootherstep term, swapped curves, the upper clamp, the
  binary search direction, the IK stage keys, the kept count after an
  exhausted row, the look cursor's store order and its "any" stage, the
  marker gate, dual wielding, the fade speeds, the final flag bit, the
  113C70 bit, one member each of the 113B70 and 1139D0 mode sets and the
  1139D0 second set, the overlay gate, the aim weapon slot, the inheritance
  index of 1E5360 and the CE230 index sign. The two set-member mutations
  first survived on 1157D0 alone (too rare along that path) and were
  rejected by the focused `units_look_enabled`/`units_aim_enabled` kernels.
- Biped, new-failed and C6440/F10E0/C6850 owners (48): store orders, the
  REP MOVSD length, the mode 2 vector, mode selection, the E7CA0 tail, the
  BBE00 target, the +1F0 bit, the reread order, the DE9E0 gate and SSE
  operand order (distinct NaN payloads), the DEA70 test, the 1708F0 index,
  the 111720 order and tail, the +6C marker, the 1102A0 names, the 115F50
  curves, the manager slots, three 1175A0 rules, C6440's clear order,
  manager gate, block size, failed byte, extension order, copies, seat bound,
  team and game-mode tests, the 1CE7B0 frame, clamp and store order, the
  1D6200 overlay and flag, BBF20's root walk, index bound and fallback
  object, F10E0's model gate, store order, word range and manager gate,
  CEF90's NONE skip and E8E50's wake, the 1F4850 order and source vector,
  and 1ED780's mode store.
- 111B10 leaves (10): 115E90's slow states, urgent byte, vehicle exit and
  minimum; 1E16E0's direction; 1D38A0's hold bit, clear order and copy
  length; BC380's wake and block order.

Equivalent survivor: 115E90's `state == 0x400000C` term in the vehicle-exit
rule. That state is not one of the three slow states, so the blend time is
already 0.267 s whenever the term applies.

111B10 batch (16 mutations, strict writes): 11 rejected (the 0x20 flag, the
1D5320 skip, the urgent change, the forced blend, manager +6C bit 4, a 115AF0
mode class, the first two 115AF0 names, the aim overlay test, 116A10's 0x803F
flags, 1D3D80's fallback and the 1D4350 key order). Three survivors are equivalent: dropping the urgent byte
from the restate test (an urgent change already takes the changed branch),
and the trial record's rate word and start byte (1D3C80's record is private
and neither changes the trial's result). Two survive unobserved: the 0x40
(exchangeable state) and 0x200 ([51DAF8]) flags only matter deep inside
track weapon's 1D5320, which these fixtures rarely drive there.

