# Native TU1.5 object factory (B9B70 `object_new`)

Started 2026-11-11. User direction: port all remaining campaign blockers
natively ("All of the above"): object factory, AI frame owners, animation
pipeline, unit actions. The factory comes first. Its weapon path unblocks
`cinematic_clone_players_weapon`, which stops the main mission thread on
04b, 05b, 06b, 07b and 08b. It also leads to native `object_create`,
`ai_place` and item births.

## Current state

Every campaign object birth runs the original factory through the live
world:

- `call_actual(0xb9b70)` in `src/physics/physics_live_world_tu15.cpp`
  (player and starting-weapon births)
- the scene placement owner (scripted `object_create`)
- `place_ai`
- multiplayer weapon spawns

There is no native object construction.

## Measured scope

- Static callee closure of B9B70: 2,208 functions, 180,399 instructions.
- Executed slice (instruction census, 04b, 300 ticks of real births): 699
  functions, 36,525 instructions. B9B70 itself ran 549 of its 614
  instructions, 428 times.
- Heaviest executed owners: 1E3420 and 1E3F30 (about 690 each), 1D46A0,
  1CB8E0, 31A260, BF0C0, 1DE170, BB3E0, 1D5320, C2CB0, C0270.

Command:

```
HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1 HALO2_INSTRUCTION_CENSUS=<file> \
  SDL_VIDEODRIVER=dummy <runtime>/halo2_native --map <04b> --simulate 300 --campaign
```

Census lines are `<hex pc> <count>`. Intersect them with the static closure.

Existing native coverage is approximate: an address mention in a
non-generated native source. It names 287 of the 699 executed functions
(14,451 instructions). That overcounts, since B9B70 itself is mentioned only in
comments, so verify each owner before relying on it. Native 16E6A0
(`native_tu15_data_array_allocate`) is real and reusable. The largest
unnamed executed owners are 1E3420/1E3F30 (about 690 each), 1D46A0, 1CB8E0,
31A260, 1DE170, BB3E0, 1D5320, C2CB0, 1E6150 and 2EBD10.

## Structure of B9B70 (TU1.5 decompile `000b9b70.c`)

1. Request validation. `10DAF0` runs unless request `+18` bit 4 is set.
2. Datum allocation (`BE2B0`, the object data array at `[0x4ECBD0]`), the
   header row (`+2` flags, `+3` type) and body `+0` (the tag).
3. Placement fields into the body (`+0x64..+0xA0`): position, forward, up,
   velocities and scale. Header flags `+4` (bits 0x400, 0x200, 0x80000000),
   team and owner fields.
4. Havok physics setup when the model's physics (`+0x14`) exists:
   `1D3180`, `1D3380`, `1D3270`, `1D3350`.
5. Variable-length body blocks through `BE3B0`: node matrices `+0x114`,
   region and permutation state `+0x118`, the attachment, damage and
   function blocks `+0x11C`/`+0x120`/`+0x124`, Havok blocks `+0x10C`/`+0x110`,
   and `+0x128`. `1CC4D0` follows.
6. The type-specific constructor `10DB40`: the object type definition table
   at `0x474B90` indexed by tag type, with the parent chain `object → item →
   weapon` and so on.
7. `BC570` (variant), `BC5C0` (regions and permutations), `C0270` (initial
   change colors), `D80A0` (vitality, stored to `+E4..+F0`), `C0910`
   (function-scaled change colors), `BC400`,
   `BF050` (node matrices and animation rebuild).
8. World placement: `C1540`/`BA630` (collision and clusters) when the world
   is live, then `C0200`, `B9390` (update marking), `BA8C0`, `C06C0`.
9. The type-specific place `10DC30`, the creation effect `179610` (definition
   `+0x50`), `2E1C0`, `10E440`, and the deferred activation `BA570`.

## Port plan

The port goes owner by owner. Each owner is replaced by a native kernel
over the live world bank, in parity with the original.

1. **Weapon path first.** Record the executed slice for a weapon birth alone:
   the starting-weapon `call_actual(0xb9b70)` sites in the live world, or the
   `13ED30` clone. Port its leaf owners first: datum allocation, block
   allocation, header and placement fields, vitality, functions, regions.
2. **Type dispatch.** Port `10DB40`/`10DC30`/`10DCA0`/`10DAF0` as typed
   native dispatch: object → item → weapon first. Any other type is a named
   refusal, never a call into original code.
3. **Shared heavy owners.** `BF050` (node rebuild) is shared with the
   animation pipeline. The Havok setup is shared with the physics
   replacement work in `src/physics/native_tu15_*`. World placement
   (`BA630`/`BA8C0`) is shared with `objects_detach`.
4. **Production switch.** Once the whole weapon path is native, `13ED30`
   (clone and attach through `10FFD0`) and the starting-weapon births switch
   to it. Other types keep refusing until they are ported.

## Type dispatch (0x474B90)

Each object type's definition at `0x474B90[type]` lists its parent chain at
`+0x84`, null-terminated. Every chain level is a 0x84-byte slot table. The
factory calls these slots:

- `+0x28` validate (10DAF0)
- `+0x2C` new (10DB40)
- `+0x38` place (10DC30)
- `+0x3C` new-failed (10DCA0)
- `+0x74` nodes (10E1F0, from C0200)

The type definition's `+0x08` word is the body size.

| Type | Chain | Slots the factory uses |
|---|---|---|
| weapon | object → item → weapon | new: item FA820, weapon 102AD0; place: object C0790 |
| equipment, garbage | object → item → (type) | new: item FA820 (garbage also 11E270) |
| biped, vehicle | object → unit → (type) | unit new C6440; nodes 1157D0; biped validate DE9E0, new DEA60 |
| scenery, crate, creature, sound scenery, projectile | object → (type) | per type |
| machine, control, light fixture | object → device → (type) | device new 10B5C0 |

Native dispatch keeps the chain table as literal TU1.5 data. Each callback is
either a native function or a named refusal.

## Ported owners

All of these are native in `src/gameplay/objects/native_object_factory_tu15.cpp`.
Each is proven by physical-original fixture cases in
`tools/validate_native_campaign_script_builtins_tu15.py`: every original
instruction is covered (except two branches of 1912E0 that its only caller
cannot reach), and every listed mutation is rejected. The combined run is
5,480 cases with 126,003 identical ordered stores. The whole builtins oracle,
factory owners included, passes 26,940 physical cases (168,215 ordered stores)
with the empty-REP filter enabled.

| Owner | Native | What it does |
|---|---|---|
| 1910A0 (+191480/1913F0/191350/1912E0) | `memory_pool_allocate` | object memory pool allocation |
| 191130 (+191220) | `memory_pool_resize` | grow in place or relocate |
| BE2B0 | `object_allocate` | datum (native 16E6A0) plus a zeroed body |
| BE3B0 | `object_block_allocate` | variable body blocks |
| BE330 | `object_free` | failed-birth release (native 16E8A0) |
| C19D0 | `object_model_tag_pair` | model tags at +4/+C |
| D80A0 | `object_damage_info` | model damage-information row |
| BBD50 | `object_root_header_bit6` | root parent header flag |
| BC400 | `object_clear_block_12a` | clears the +12A block bytes, then B9390 |
| 10E440 | `object_queue_definition_flagged` | 32-entry queue at [51DAF0] |
| 1CC4D0 | `object_definition_physics_capacity` | physics capacity gate |
| BA850 | `object_memory_locked` | [4EABC4] lock byte |
| 1D3180 / 1D3270 / 1D3350 | `havok_record_*` | Havok record defaults |
| 10D950 | `definition_object_type` | tag group → type |
| 16FFE0 / BC570 | `model_variant_index`, `object_set_variant` | body +B1 |
| BAC30 | `object_node_matrices` | node matrix block and count |
| 2E1C0 | `object_retire_lens_queries` | native 20E30 per owned slot |

Notes:

- An empty `REP` string instruction (ECX 0) performs no memory access. The
  oracle now marks REP sites with `repeat_ecx`, so such instructions are not
  logged as stores.
- 191130's relocation briefly records the caller's stack slot as the moved
  block's owner, then overwrites it. The native API takes that slot as
  `scratch_slot`; the value never survives.

## Remaining weapon path

These are B9B70's remaining owners, sized from the 04b census (executed
instructions; the closure stops at core owners):

- **BF050**: node rebuild, about 16.8k executed. It is also reached through
  item new FA820 → FD100 → B96B0. This is the critical heavy owner.
- **102AD0**: weapon new, 65 functions / 4,911.
- **1D3380**: animation manager setup, 60 / 4,753. It runs when the model's
  +14 is set.
- **C0910**: function-scaled change colors, 28 / 1,257.
- **C0270**: initial change colors, 20 / 1,071.
- **BC5C0**: regions. It calls BC6C0, 1700E0, 17E060 and 1CE0E0.
- **C06C0**: D6E40, then C0CD0 (which reaches the core).
- **C1540/BA630**: world placement.
- **179610**: the creation effect.
- **BABA0**: the spatial refresh. Native BA910/16E290/1D30A0/29FFF0 already
  exist in `native_tu15_object_spatial_packet`.
- **C0200**: BAC30 plus the 10E1F0 nodes dispatch (units only).
- **C0790**: object place. It creates variant child objects, which recurse
  into B9B70 and BAF10.
- **The B9B70 body itself**: header and placement fields.

See [NATIVE_CAMPAIGN_SUBSYSTEM_SCOPE.md](NATIVE_CAMPAIGN_SUBSYSTEM_SCOPE.md)
for the shared object core and the other three subsystems.

## Parallel tracks

Births run through four original call sites:

- `C11A9` in C1120, scenario object placement: 3,043 births across six
  campaign censuses;
- `C0877` in C0790, variant children: 93;
- `CEFF2` in CEF90, unit weapons: 28;
- `1E8DA4`: 1 (01a only).

Three more are direct `call_actual(0xb9b70)` sites in
`physics_live_world_tu15.cpp`: player birth, equip weapon and create weapon.

What B9B70 executes is too large to port serially, so the work is split into
tracks that never touch each other's files. A track `<name>` owns exactly:

| File | Role |
|---|---|
| `src/gameplay/objects/native_object_factory_<name>_tu15.cpp` | production owners (globbed into `halo2_systems` and the oracle harness) |
| `include/halo2/native_object_factory_<name>_tu15.hpp` | its API, namespace `halo2::native_object_factory_tu15` |
| `tests/native_object_factory_<name>_kernels.cpp` | harness kernels (`native_object_factory_test_registry.hpp`) |
| `tools/native_object_factory_cases_<name>.py` | oracle cases: `register(v)` adds KERNELS, ROUTINES, TRACK_ARGUMENTS, TRACK_COUNTS |
| `docs/native_object_factory_tracks/<name>.md` | ported owners, proofs, dependencies |

Shared world access is in
`src/gameplay/objects/native_object_factory_tu15_internal.hpp` (`detail`).
Unported callees go through `detail::refuse()`: a named hard error, never a
call into original code.

Build and oracle isolation:

- `-DHALO2_NATIVE_OBJECT_FACTORY_TRACKS=<name>` builds the base owners plus
  that track only.
- `HALO2_FACTORY_TRACKS=<name>` loads only that track's oracle cases.
- The `world.track` fixture bank (0x1500000, 0x10000 bytes) is available to
  any track fixture.

The template track is `nodes` (C15D0, BAC00).

| Track | Status | Doc |
|---|---|---|
| core | Done: type table, B9960, BBE00, BABA0, C0200, 1496F0 | [core](native_object_factory_tracks/core.md) |
| birth | `object_new` and C0790; first half proven, success path waits on the tracks | [birth](native_object_factory_tracks/birth.md) |
| objtypes | Done: device, machine, sound scenery, crate, scenery, garbage and projectile new; device, machine and scenery new-failed; 10F710 scenery animation start on `weapon_animation` (9,900 cases). BBBC0's 1D9F10 and 1CC3F0's 1CC310 run on track havok's owners (BBBC0 → 1D9F10 → 315CB0 proven over havok's physics world) | [objtypes](native_object_factory_tracks/objtypes.md) |
| havok | Native allocator/growth and body activation; bounded broadphase add/remove 2EC4B0/2EC9B0, phantom 318550/318680 and 1CE340; standalone 1CD6A0; component world removal/insertion 1D9510/1D97F0 with hkWorld removeEntity/addEntity 3198A0/319370 and the 1D73D0 dispatch (2026-11-14). Bound into objtypes, modelstate, placement (BA8C0's 1D97F0, 282C10's 1D9510) and units. Remaining: 1D73D0's five shape builders (the dispatch is native standalone; 1CC310 still refuses at 1D9510/1D73D0 for units' stop list), entity contact agents, 319750 and the track doc's other refusals | [havok](native_object_factory_tracks/havok.md) |
| modelstate | Done: C0270, BC5C0, 1D3380 (with weapon's 1D5320 subtree), the animation and color leaves, C0910 on attachments' BCB70/134730, C06C0/C0CD0 on attachments' light, looping sound and contrail creators (composed paths proven against the original). Still refuses: effects (179540 → 178E30), D6E40's widget creators, 17B1F0, and 1CE0E0's Havok blockers through havok's 1CC310 | [modelstate](native_object_factory_tracks/modelstate.md) |
| weapon | Done: 102AD0 weapon new, the animation-manager subtree (`weapon_animation`: 1D5320, 1D46A0, 1E3420, 2A0100, physical queries, channels), FA820/FD100. Since 2026-11-16 the subtree also searches the MCC cache's first-person state names and dual class, as the translated query scopes do (`QueryNamespace`/`QueryScope`). 8,320 cases | [weapon](native_object_factory_tracks/weapon.md) |
| nodebuild | BF050/BF0C0/BFF20/BFB90, the codec 283A90 and the unit/vehicle/device orientation hooks; since 2026-11-14 also 1DBBD0 (physics nodes), 1DC7A0 (crate bounds), the 125FB0 cache miss (13FE50 through the native page allocator, the read through a bound reader), BF9A0 function overlays, the masked second-frame loops 28A7D0/28E0B0, 28C8F0/28BF00 and decoder 296700. BE850's BE892/BE941 calls, every other BF050 caller and BF640's BFB90 are required boundaries in production, native by default since 2026-11-14 (also decoders 296540/2962F0/296620), with translated reference switches and a live differential (see the track doc); since 2026-11-15 also 283E10 event frames, the kind-4 loops, the 284090 mask intersection and the forced decoders 296890/2969E0, proven by a scripted-input differential sweep of every MP map (1p/4p) and campaign 01a..08b (1,594,436 calls, 0 differences, 0 refusals) and native by default again; since the 2026-11-15 continuation BF640 itself (with 110240, 1114B0, 1112E0, 110C10, 2130C0, 1159D0, 115500, 1447E0) is native at the model builder and body publication, and 16A4DD's 1D5840, BF9A0, 1CF3B0 and 1CF210 calls (the first-person sampler) are bound sites; since the second 2026-11-15 continuation 16A4DD itself (with 16AD45, 169863, 1E5470/1E5540 and the rounds owners) is native at 169C46's 16A479 site, by default, with three residual originals outside the factory (17A570 effects, 171440 resource prediction, 128E70 sound stop) that run only when they would store; since the third 2026-11-15 continuation 169C46, the first-person update (with 1D39C0's animation events, 121890 and 16961F), is native and proven, and 16AD45's translated callers (168D40, 16B1AD, 16B78B, 16940B) are native by default. It was opt-in until the weapon track's 1D5320 divergence on Elongation was fixed: that fix is the MCC first-person query names, 2026-11-16. 169C46 at 168D55 and 169863 at 16B7F5 are now native by default, with `HALO2_REFERENCE_FIRST_PERSON_UPDATE_TU15=1` as the reference switch. Still refuses: codec kind 1, the unexecuted loops/decoders, the 1DBBD0 child shape transform | [nodebuild](native_object_factory_tracks/nodebuild.md) |
| placement | Done: C1540, BA630, BA7E0, BA8C0, B9460, B95D0, C4D30 light reconnection with sphere and frustum lights, and helpers (14,050 strict cases); since 2026-11-15 also C3750 owned lights, 1692E0/16918F first-person light markers (1E1200 node maps) and the 145070 parent-node light frame; since 2026-11-16 16D740, C4D30's collision sphere fit, with its whole structure-BSP closure (16D2A0, 1E60C0..1E6570, 120FA0, 24EDB0..24E580, 24FCC0..24F9C0), and C3AB0's degenerate-target fallback; since its continuation 1E6150's plane stack past 128 planes (the walk record's overlapped projection state) and C3AB0's orthogonal frustum; since the second continuation 16D2A0's instanced geometry (16CF00 with 16B810, 144090, 186E30 and 24E710/24E850/24EB60's transform paths) and its cluster/instance/object visit loops (16D0E0, objects, refuses). 1CC310 calls track havok's owner; BA8C0's 1D97F0 and 1CC190's creation still refuse | [placement](native_object_factory_tracks/placement.md) |
| units | 1157D0, biped callbacks, unit new-failed, BC380, 111B10, and the admitted C6440/F10E0 constructor paths are native. F10E0 uses objtypes' deferred BBBC0; biped mode changes use havok's 1CC310 with live FP controls. Havok removal/refresh, flying/ragdoll mode owners, and the listed rare animation paths still refuse. C6850 calls unitactions' E8E50/CECF0 and retains its starting-weapon B9B70 refusal | [units](native_object_factory_tracks/units.md) |
| unitactions | Native action dispatch, EAF00/CECF0 and script vehicle-exit cores; EB190 ready-interrupt and EB360 put-away prefixes are now wired, and D0520/116920 reset helpers are qualified. Armed action tails still refuse; ED060 retains its D0520 gate until the complete seat-release continuation is qualified | [unitactions](native_object_factory_tracks/unitactions.md) |
| attachments | Done: C0CD0's creators 18BD80 looping sound, 17E940 contrail, C2260 light (+C2CB0), the BCB70 function value subtree and 134730, all wired into modelstate's C0CD0/C0910; since 2026-11-14 also 10DE40's unit (C9070), biped (E0900), vehicle (F9270) and weapon (1046C0) callbacks; since 2026-11-15 they compare live names through the MCC StringID join (`ObjectFunctionNamespace`); since 2026-11-16 every BEC50 callee (C1F90, BEAA0, 15B2D0, BE9A0, the console clock's BEBE0/BEB00/BEB40/BEB90). 179540 effects refuse at 178E30 (the effect system) | [attachments](native_object_factory_tracks/attachments.md) |
| lights | C3430 (the per-tick light update) and C5290 (light deletion), over attachments' C2CB0 and placement's C4D30; bound at the live world's 13AB4E with reference and differential switches (see the track doc). Since the 2026-11-16 continuation C4D30 (with 16D740) is also bound at C0F60's C100F, C3750's C3810 and C20D0's C2135, C2CB0 at C2260's C2353 and BCB70 (with BC8F0, 10DE40) at C0700's C0756 and (second continuation) C0910's C09EC/C0A57 (`light_caller`, `HALO2_REFERENCE_LIGHT_CALLERS_TU15` / `HALO2_LIGHT_CALLERS_DIFFERENTIAL_TU15`) | [lights](native_object_factory_tracks/lights.md) |
| deviceanim | Done: `device_set_overlay_track` (10D5E0) fully native; `device_set_position_track` (10CF80) native up to the 283A90 codec (nodebuild). Both are registered in the native script host (4,660 cases + 2,300 composite) | [deviceanim](native_object_factory_tracks/deviceanim.md) |
| services | `native_object_factory_services()` binds `ObjectFactoryServices` to every track (C0910/C06C0 with the live FP controls, `phantom_wake` to havok); the script host's vehicle exit evaluators (`script_vehicle_unload`, `script_unit_exit_vehicle`, `script_ai_vehicle_exit`) are proven at the HS evaluator tails | see [Integration](#integration-2026-11-12-wiring-and-per-track-proofs) |

Census-executed refusals that still block some births:

- 1CE340: Havok phantom wake, from scenery new-failed: bound to havok's
  `havok_phantom_wake`, now native through its 318550/318680 (it refuses
  only non-default phantom, broadphase and listener owners). The composed
  10F300 → 1CE340 path is not yet proven through objtypes' kernel, whose
  harness hooks stay empty.
- The nodebuild refusals (see its doc): the remaining unported
  animation/codec paths (no census or live differential executes them).
  1DBBD0, 1DC7A0, the 125FB0 cache miss and BF9A0 are native since
  2026-11-14.
- Unit place (C6850): starting weapon births (B9B70, coordinator) and the
  unitactions refusals past EAF00 and CECF0 (see its doc).
- 179540: effect attachments from C0CD0 (attachments: effect_new 178E30).

Census caveat: a function the runtime already intercepts natively never
appears in an instruction census (for example 3DD00, served by the light-cache
native). Slices derived from censuses miss such functions; port them anyway.

## Integration (2026-11-12): wiring and per-track proofs

Finished owners that other tracks refused by name are now wired:

| Caller (track) | Was refused | Now calls (track) |
|---|---|---|
| C0CD0 (modelstate) | C2260, 18BD80, 17E940 | `object_light_attachment_create` (body +4 \|= 0x10), `object_looping_sound_attachment_create` (\|= 0x20), `object_contrail_attachment_create` (attachments); 179540 still refuses at 178E30 |
| C0910 (modelstate) | BCB70, 134730 | `object_function_value`, `color_interpolate` (attachments) |
| C06C0, C0910 (services binding) | — | take the live x87 control word and MXCSR |
| 1CE0E0 (modelstate), 1CC310 sites (placement), 1CC3F0 (objtypes) | 1CC310 | `havok_component_state_flags` (havok) |
| BBBC0's 1D9F10 (objtypes) | 315CB0 | `havok_component_activate` (havok) |
| `ObjectTypeServices::phantom_wake` (services) | 1CE340 | `havok_phantom_wake` (havok; native through 318550/318680 since havok's later update) |
| C6850 (units) | EAF00 | `unit_actions::request(Request::of(8))` (its own B9390 wake), then `seat_drivers_update` (unitactions) |
| Script host | `vehicle_unload`, `unit_exit_vehicle`, `ai_vehicle_exit` | services' `script_vehicle_unload`/`script_unit_exit_vehicle`/`script_ai_vehicle_exit` over unitactions' 11D990/11D670/27EAB0; `ai_vehicle_exit` walks with `native_campaign_script_tu15::ai_reference_actors` (27CAA0/27CB30) |

The harness lists `src/campaign/ai/native_campaign_unit_action_support_tu15.cpp`
directly (halo2_systems already builds it), and the unitactions kernels no
longer include it. Since 2026-11-12, units also binds the biped mode
switch's 1CC310 and F10E0's BBBC0 to havok and objtypes.

**Oracle.** Composed kernels borrow the callee track's fixture builders read
only through `tools/native_object_factory_track_fixtures.py` (`borrow(v,
track)`: a private copy of that track's cases module registered against a
capturing view, so nothing of it is registered or wrapped, while its global
closure declarations are shared). Modelstate's stops no longer list the
wired owners; its composed routines take the attachments track's stops.
`weapon_set_state`/`weapon_new` declare per-routine stops for the refused
126200/CC1E0/CFB40: with every track loaded, unitactions' declared E8E50
dispatch would otherwise pull every action callback into their closures.
New kernels: `objtypes_object_wake_bodies` (BBBC0 over havok's physics
world) and the services track's HS-level `vehicle_unload`,
`unit_exit_vehicle` and `ai_vehicle_exit` (descriptor-authenticated
evaluator tails 2ADE60/2ADC30/2AFE00, stopping at unitactions' refusals).

Per-track runs (seed 0x5C41, `HALO2_ORACLE_STRICT_WRITES=1`,
`HALO2_FACTORY_TRACKS=<track>`, each track's own kernels; units,
unitactions and services rerun on the 19:45 tree):

| Track | Result | Composed coverage |
|---|---|---|
| modelstate | PASS 9700 tails, 19400 comparisons, 162835 stores | C0910 183/183, BCB70 138/138, C0CD0 111/124, C06C0 19/19, C2260 77/77, 17E940 72/82, 18BD80 27/32 |
| attachments | PASS 12000 tails, 24000 comparisons, 117601 stores | — |
| units | PASS 13000 tails, 26000 comparisons, 329595 stores | C6850 25/25, E8E50 33/33, CECF0 188/188, EAF00 103/132 |
| unitactions | PASS 13400 tails, 26800 comparisons, 16045 stores | — |
| objtypes | PASS 10400 tails, 20800 comparisons, 123320 stores | BBBC0 61/61, 1D9F10 23/23, 315CB0 18/18 |
| havok | PASS 7960 tails (16 kernels, strict, current tree); 5 seeds × 6360 cases; combined base+havok PASS 33300 tails, 708877 stores | — |
| placement | PASS 14050 tails, 28100 comparisons, 652105 stores | — |
| weapon | PASS 7320 tails, 14640 comparisons, 79891 stores | — |
| services | PASS 1200 tails, 2400 comparisons, 963 stores (1209 before unitactions' 19:21 fixture update) | 11D990 53/54, 11D670 44/50, 27EAB0 43/63 (all of it with EBX NONE), 27CB30 50/50 |

Nodebuild's composite runner (BF050/BF0C0/BFF20 reach placement's BA630,
whose 1CC310 now goes through havok) on the same tree: `PASS 10248 physical
composite cases, 725873 ordered original stores`; after F2CC0 switched to
units' exported 1D3C80/1D4450: `PASS 10248 physical composite cases,
727342 ordered original stores` (units then: unchanged `PASS 13000 ...,
329595 ordered original stores`).

**Whole builtins oracle, every track loaded** (no `--only`, strict writes,
seed 0x5C41; harness built 20:38 from the tree of that time, before the
1D3C80/1D4450 export): `PASS 136162 physical original evaluator tails,
272324 native comparisons, 2414697 ordered original stores` (385 kernels).
With every track, the validator holds all ~136k cases' banks and original
outcomes in one process and was OOM-killed (over 20 GB). The run therefore
used a streaming driver over the validator's own steps (same `cases()`
order, the tracks' `cases` wrappers, `physical`, `expected`, `compare`) in
2000-case batches, sharded six ways by batch index, with the counts summed.
The validator itself now implements bounded streaming, so CTest no longer
needs that external driver. See the current
[streaming proof and fresh full-run receipts](CAMPAIGN_SCRIPT_BUILTINS_NATIVE.md#proof);
the historical six-shard totals above remain scoped to their original tree.

## Validation

Fixture-only oracles cannot cover whole `object_new` with real tag data. The
plan is a live differential harness, used only as a check:

- At each real birth, snapshot the world bank.
- Run the native factory on a private copy (the `NativeCampaignScriptScratch`
  overlay pattern).
- Compare every store and the resulting datum against the original's run on
  the live world.

Leaf owners also get physical-original fixture cases through
`tools/validate_native_campaign_script_builtins_tu15.py`.

## Integration (2026-11-12)

The resumed integration connects C0910/C0CD0/C06C0 to the native object
function, light, looping-sound and contrail owners; C6850 to the native
unit action dispatcher and seat-driver update; and objtypes/modelstate/
placement to the shared Havok activation/reset owners. The script host's
`vehicle_unload`, `unit_exit_vehicle` and `ai_vehicle_exit` evaluators call
the services track's native evaluators with their original argument order.
The AI evaluator retains the live 27CAA0/27CB30 actor walk.

A fresh all-track harness and its self-test passed. The focused integration
oracle passed **5,000 physical-original cases, 10,000 native comparisons
and 76,275 identical ordered stores**, with all track registrations loaded.
That includes the three evaluator tails, composed modelstate attachments
and colors, C6850, weapon new/state and BBBC0's body activation. The former
units/weapon closure failures are resolved by per-routine refusal scopes;
one track no longer supplies another track's missing stops.

Receipt: [integration oracle](../build/test-results/resume-factory-integration-20261112/integration-oracle.json).
The [session record](../build/test-results/resume-factory-integration-20261112/session.json)
pins the built sources and documents the separate nodebuild fixture refresh
during that focused run. The later units wake/shape bindings have their own
expanded qualification in the [units track](native_object_factory_tracks/units.md).

After those bindings and the stop-entry harness refresh, the final composition
cohort passed **4,600 original cases, 9,200 native comparisons and 73,114
ordered stores**, with unchanged source hashes throughout the run. It covers
the same integration kernels except C6850, which is covered by the expanded
units cohort and its exact-boundary binding screen. Receipt:
[final integration oracle](../build/test-results/resume-factory-integration-20261112/units-bindings/integration.json).
The same harness passed all 32 unitactions kernels: **13,400 original cases,
26,800 comparisons and 15,901 ordered stores**, including exact refusal
identity where declared. Its fixture pins precede the separately proven
final C9590 key-list control; see the [unitactions track](native_object_factory_tracks/unitactions.md)
for that focused follow-up and mutation results.

These are native-owner and composed-prefix checks. They do not qualify a
complete live B9B70 birth or remove the remaining Havok, effect, streaming
and unit-action refusals.

## Rule: StringID literals under MCC caches (2026-11-15)

Native owners that compare StringIDs against TU1.5 XBE literals are wrong on
MCC caches, because the IDs differ. For example, `barrel_spin` is
`0x0B0005A9` in TU1.5 and `0x0B0005B5` in MCC. The translated code receives
MCC operands through `source_object_function_compiled_symbol_tu15` and the
`source_object_function_symbols_tu15.inc` join.

The attachments track has `ObjectFunctionNamespace`, a thread scope, and
`object_function_tu15_name`. Together they map MCC names to TU1.5 names at
BCB70's literal checks and at the entry of the 10DE40 callbacks (C9070,
E0900, F9270, 1046C0, BEC50), including the folded SUB chains. Outside a
scope the mapping is the identity, so oracle results are unchanged.

What this means for other code:

- Every live caller of these natives must open the scope. Today that is only
  the node-rebuild binding.
- Native code that compares or looks up animation state keys from code
  literals (1D5320's callers: 169863's keys, 169493's table) is not covered
  by this join. On Elongation the native 1D46A0 path found no animation for
  0xC0005AE/0x140005B0/0xB0005B1 where the translated one found animation
  11 (see nodebuild.md, "Production: 169C46 and the callers").
  - The cause, found 2026-11-16, is a second join: the translated 1D5320's
    weapon animation query scopes
    (`src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp`,
    emitted by `tools/generate_physics_live_support_tu15.py`). They map
    the copied search keys at 1E34A3 (staged-reload and melee states) and
    2A019A (the dual class).
  - The weapon track's `weapon_animation::QueryNamespace` and `QueryScope`
    now replicate that join (see
    [weapon.md](native_object_factory_tracks/weapon.md#mcc-first-person-query-names-2026-11-16)).
  - When you look for a missing MCC join, search for every `source_*_symbol_tu15(`
    call the generators emit, not only the object-function `.inc`.
- Any new native that compares StringID literals needs the same mapping.
- The bug was found as the Turf 4p native-vs-original difference: a BF9A0
  overlay on `barrel_spin` returned 0 instead of 1/255.
- Compiled SUB chains (2026-11-15): the translated code joins only the
  instructions the .inc lists, and a SUB after a joined SUB keeps its TU1.5
  distance. The translated route then matches the MCC operand plus that
  distance, and the native join must map that name too. An audit of all
  145 comparison sites of the 10DE40 callbacks found eleven such names
  (BEC50, F9270, 1046C0, 1DB800) plus 100E70's projectile join; see
  [attachments.md](native_object_factory_tracks/attachments.md#mcc-names-of-the-chained-comparisons-2026-11-15).
  New natives that compare names after a SUB chain need the same audit.
- Literals in low string indices can be identical in both namespaces:
  1159D0's 0x4000095 ("head", index 0x95) is 0x04000095 on MCC caches too.
