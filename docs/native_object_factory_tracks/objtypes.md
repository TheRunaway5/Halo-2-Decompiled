# Track `objtypes` (object type new / new-failed callbacks)

Native in `src/gameplay/objects/native_object_factory_objtypes_tu15.cpp`
(API: `include/halo2/native_object_factory_objtypes_tu15.hpp`). Oracle cases
are in `tools/native_object_factory_cases_objtypes.py` and the harness kernels
in `tests/native_object_factory_objtypes_kernels.cpp`. Each owner replicates
its TU1.5 routine over the live world memory: every store keeps the
original's address, width, value and order. SSE steps run through
`native_campaign_script_fp` under the live MXCSR (DAZ compares through
`sse_compared`). x87 steps run the original instruction sequence under the
live x87 control word.

## Dispatch adapters

`objtypes_construct_callbacks(fp)` and `objtypes_construct_failed_callbacks()`
return address-keyed maps with the coordinator's `ObjectFactoryServices`
shapes (`construct`: `bool(mem, object, request, failed)`,
`construct_failed`: `void(mem, object)`):

- new (+2C): 10B5C0 device, 117870 machine, 20FE50 sound scenery, 11E320
  crate, 10F260 scenery, 11E270 garbage, FD200 projectile, and 11ABF0
  creature (a refusal);
- new failed (+3C): 10B830 device, 117900 machine, 10F2F0 scenery.

Garbage and projectile need the world's live x87 control word as well as
the MXCSR (`ObjectTypeFp`), so they are bound when the map is built.

The `ObjectTypeServices` overloads also carry two optional cross-track
hooks. An empty hook is the named refusal:

- `node_rebuild`: BF050, the last step of a successful 10F710;
- `phantom_wake`: 1CE340, which 10F300 runs for a scenery with a scenario
  slot while the Havok world [0x52BE74] is live. The services track binds
  it to the havok track's `havok_phantom_wake` (native through hkWorld
  add/remove phantom; see havok.md). This track's harness hooks stay empty,
  so 10F300 → 1CE340 composed is not proven here yet.

The fp-only and no-argument overloads leave both hooks empty.

## Animation natives this track calls

10F710's slice calls the weapon track's proven animation-manager owners
(`weapon_animation`) directly:

- `graph_require_resident` (1E5C80);
- `manager_set_state` (1D5320; 1D4D90 passes key 0xE0000C2, 0x7000101 three
  times, flags 4 and start flags 0x3F);
- `manager_start_channel` (1D36C0);
- `channel_set_frame` (1CE810).

A build needs `HALO2_NATIVE_OBJECT_FACTORY_TRACKS=objtypes;havok;weapon;core;nodes`
(or all tracks): BBBC0's 1D9F10 and 1CC3F0's 1CC310 are the havok track's
`havok_component_activate` and `havok_component_state_flags`, so `object_wake`,
`physics_component_activate`, `object_header_activate` and
`object_physics_rebuild_check` take the live controls (`ObjectTypeFp`). The oracle reuses, read only, the weapon track's graph and
manager `Fixture` and its 1D5320 jump tables and virtual owners.

1D3380 (animation manager setup for a graph other than the manager's)
belongs to modelstate and is a named refusal until it is native. Scenery
never reaches it, because 10F4A0 passes the manager's own graph.

## Ported owners

Case counts are physical-original cases at seed 0x5C41. Coverage is the
number of original instructions executed by those cases out of each
routine's complete body.

| Owner | Native | Cases | Coverage | Mutations |
|---|---|---|---|---|
| 10B5C0 device new | `device_new` | 300 | 56/56 | 2/2 rejected |
| 117870 machine new | `machine_new` | 300 | 34/34 | 1/1 rejected |
| 20FE50 sound scenery new | `sound_scenery_new` | 200 | 12/12 | 1/1 rejected |
| 11E320 crate new (+212C70) | `crate_new` | 500 | 73/73; 212C70 67/67, BC190 36/36, 144BF0 53/53, 145560 69/69 | 3/3 rejected |
| 10F260 scenery new | `scenery_new` | 600 | 39/39 | 2/2 rejected |
| 10F8D0 | `scenery_lightmap_object_index` | via scenery new | 43/43 | 1/1 rejected |
| 10F960 | `lightmap_scenery_object_row` | 400 (+ scenery new) | 70/70 | 2/2 rejected |
| BBBC0 | `object_wake` | 600 + 500 composed (+ scenery new) | 61/61 | 3/3 rejected |
| B7E50 | `network_entry_mark` | via `object_wake` | 22/22 | 1/1 rejected |
| 1D9F10 (+ track havok's 315CB0) | `physics_component_activate` (`havok_component_activate`) | 500 (`objtypes_object_wake_bodies`) | 23/23; 315CB0 18/18, 316D40 45/45, 322660 98/98, 3165D0 45/57 (havok's refused refcount-zero branch) | — |
| 10F4A0 | `scenery_start_animation` | via scenery new | 28/29 (10F4FA follows a successful start) | 1/1 rejected |
| 10F710 | `object_set_animation` | 600 (+ scenery new) | 48/134: every failure path (see below) | 1/2; the survivor needs a found animation |
| 1D3FC0 | `animation_manager_find` | 500 | 10/10 | — |
| 1E5360 | `animation_graph_find_name` | via 1D3FC0 / 10F710 | 93/93 | 2/2 rejected |
| 1D4D90 | `animation_manager_start` | 600 (weapon Fixture managers) | 31/31 | 3/5; the two survivors are start-flag mutants that state flags 4 never reads |
| 1D4F00 | `animation_manager_clear_overlays` | 100 (+ 1D4D90) | 33/33 | 1/1 rejected |
| 1D4E30 | `animation_manager_set_frame` | 500 | 9/9 | 2/2 rejected |
| B92C0 | `object_header_activate` | 400 | 33/33 | 2/2 rejected |
| 1CC3F0 | `object_physics_rebuild_check` | via B92C0 | 19/22 (the rest is the 1CC310 call: track havok's `havok_component_state_flags`, which refuses at 1D9510/1D73D0) | — |
| BA870 | `object_physics_deactivate` | 100 | 12/19 (the rest is the 1D9510 refusal) | 1/1 rejected |
| 11E270 garbage new | `garbage_new` | 1,000 | 44/45 | 2/2 rejected |
| FD200 projectile new | `projectile_new` | 1,200 | 312/314 | 11/11 rejected |
| 1028B0 | `projectile_damage_scale` | via projectile new | 21/21 | 1/1 rejected |
| 1F1950 | `difficulty_scale` | via projectile new | 26/26 | 1/1 rejected |
| 100D40 | `projectile_spin` | 800 (+ projectile new) | 71/71 | 2/2 rejected |
| 10B830 → 10B730 device new failed | `device_new_failed` | 500 | 49/49 (+16E8A0 62/62) | 1/1 rejected |
| 117900 machine new failed | `machine_new_failed` | 400 | 56/56 | 2/3 rejected; the survivor is equivalent |
| 10F2F0 → 10F300 scenery new failed (+2F30F0) | `scenery_new_failed` | 300 | 43/45 (the rest is the 1CE340 refusal); 2F30F0 3/3 | 3/3 rejected |

These 20 kernels (9,900 cases) pass together with the base factory owners:
`PASS 15380 physical original evaluator tails, 30760 native comparisons,
246209 ordered original stores` (`HALO2_FACTORY_TRACKS=objtypes`, build
tracks `objtypes;weapon;core;nodes`).

Notes:

- Reused natives: B9390 (`native_tu15_wake_object_ancestors`), BD980
  (`native_tu15_update_object_delete_schedule`) and 16E8A0
  (`native_tu15_data_array_delete`).
- 212C70 fills a 0x54-byte query in 11E320's own stack frame. That query
  holds the object, its +B4, the BC190 transform, the model and physics tags,
  and the node matrix pointers. The crate reads back only +48 (the physics
  model data), so the native crate replicates 212C70's tag walk. The
  frame-private transform stores are not reproduced.
- The uncovered FADD sites (11E2CF, FD335, FD3E1) add 2^32 to a negative
  FILD operand. They cannot execute, because the operand is a random draw
  shifted right by 16.
- Machine new failed: the `count <= 0` → `count == 0` mutant is equivalent.
  For a negative count, the original's second range test (start >= start +
  count) returns as well.
- Garbage: half its cases put the delay within a float ULP of a half
  integer. This makes the `FSTP dword` rounding before `FISTP` observable
  under round-to-nearest. The seed value is the word at `SEED`
  ([0x4F3CD8] points to it).
- Projectile: a min-normal position (0x00800000) against a plane offset of
  0x00800001 gives the water-plane distance -2^-149. This makes the
  DAZ-aware COMISS observable.
- Mutations: 56 distinct mutants, 52 rejected. The four survivors:
  - machine count sign: equivalent;
  - 10F710's found-animation test: a found animation ends in the BF050 core
    stop, so the oracle cannot run it;
  - two 1D5320 start-flag mutants (0x3F to 0x3E and to 0x3B): the
    start-flag word is not read for state flags 4 on any reachable path.

10F710 coverage. These paths are native and proven:

- the NONE object, NONE graph and name 0 exits;
- the missing animation manager (+12A);
- an animation absent from the graph and every import, including import
  chains and NONE imports.

Every step after a found animation is native: 1D4D90, 1D4E30, the +12C
clear, B92C0, the +138 store and BBBC0. Each is proven by its own kernel,
but not in sequence inside 10F710: a found animation ends in BF050, a core
stop the oracle cannot enter. So the "found" test mutant survives.

Scenery births are native end to end in two cases: the graph lacks
0x400000C, or the coordinator binds `node_rebuild`.

## Refused dependencies

Each refused dependency is a named refusal (`detail::refuse`). The fixtures
never reach them, and the oracle declares them as `closure_stops`.

- **1D3380** (modelstate): animation manager setup when 10F710 gets a graph
  other than the manager's. Scenery never reaches it.
- **BF050** (core): the node rebuild after a successful 10F710, which
  includes every animated scenery whose graph has 0x400000C. This is census
  executed. It runs through the `node_rebuild` hook when one is bound.
- **BBA80** (core): 10F710 with an animation reference (not -1).
  BA870, B95D0, B9390, BA630 and a second B92C0 follow it. Scenery passes
  -1, so this path is not reached from scenery births.
- **1CC310** (Havok component rebuild) from 1CC3F0: the havok track's
  `havok_component_state_flags`, which refuses at 1D9510 (an active row)
  or 1D73D0 (shape refresh). **1D9510** (Havok component deactivation)
  from BA870 stays a refusal here.
- **1CE340**: Havok phantom wake around a released scenery. It runs through
  the `phantom_wake` hook when one is bound. 10F300 reaches it when the
  scenery has a scenario slot and the Havok world [0x52BE74] is live. This
  is census executed. It is not ported; see the next section.
- **315CB0** is native in the havok track (bound 2026-11-12):
  `objtypes_object_wake_bodies` runs BBBC0 with DL 0 over that track's
  physics world (borrowed read only through
  `tools/native_object_factory_track_fixtures.py`; the component row is
  [0x52BE5C] row 0), proving BBBC0 → 1D9F10 → 315CB0 against the original:
  `PASS 500 physical original evaluator tails, 1000 native comparisons,
  4010 ordered original stores`. With the wake, header activation and
  scenery new-failed kernels: `PASS 1800 ..., 3600 native comparisons,
  5241 ordered original stores` (strict writes).
- **11ABF0** creature new is refused whole. Every successful birth runs
  1ED780's physics-mode switch (jump tables, then 1F86A0/1F4850 and the
  Havok 1CC310). 1D5320 (now native in `weapon_animation`) and BC380 (scale
  interpolation) follow. It is never executed in the six campaign
  censuses.

## 1CE340 is not ported

1CE340 does not overlap the placement or nodebuild owners: neither track
ports 1DE170, 1DEFB0, 1DF6B0, 185C00 or 318DE0, and those are not in its
closure. It is a Havok world operation:

1. 31C3B0 builds the AABB of the object's bounding sphere, padded by
   2 * [0x48B874].
2. The Havok allocator virtual ([0x48C938]+10) allocates a phantom, and
   3154E0 constructs it.
3. 318550 adds it to the world [0x52BE48]: a broadphase insert through the
   3177E0 pairs and the 37CBF0/37C960 typed pair create virtuals, with
   phantom-added listeners (322280).
4. 315CB0 activates every rigid body in its overlap list (+80/+84).
5. 318680 removes it (broadphase remove, 3222F0 listeners), then 283450 and
   the destructor virtual run.

Of these, only 283450, 315CB0/3165D0/316D40/322660, 37CB20/37CBF0/37C960,
3177E0/317870 and 2E3250/2E2E30 have physics natives (`src/physics`). Every
one of them is bound to physics services: growth, listeners, typed pair
create/delete, and destruction. 3154E0, 318550, 318680, 31C3B0, 322280 and
3222F0 have none.

A port means owning the Havok phantom lifetime and the broadphase
inside the factory. This belongs to the physics replacement. The factory
exposes `ObjectTypeServices::phantom_wake`; the services track binds it to
the havok track's `havok_phantom_wake`, which the havok track has since made
native (this section describes the state before that).

Executed census slice: every executed instruction of this track's own
routines is native, including 10F710's own body and its executed callees
1D3FC0, 1E5360, 1D4D90, 1D4F00, 1D4E30, B92C0, 1CC3F0 and BA870. The
exceptions are the refusal call sites above:

- 1D3380 (a graph change, never from scenery);
- BF050, BBA80, B95D0 and BA630;
- 1CC310 and 1D9510;
- 10F300 → 1CE340;
- the 1D9F10 loop, which is reached only from DL-0 callers.
