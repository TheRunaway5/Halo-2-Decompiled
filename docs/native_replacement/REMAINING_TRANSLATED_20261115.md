# Remaining translated execution and claims (2026-11-15)

The user's direction: "replace all translated instructions with native
equivalents. our PC port can't be emulated."

## Census

The census is the Armory first room: 01a, Original mode,
`--campaign-reference-capture .05 0 305419896`, 300 frames, run with
`HALO2_CENSUS_NATIVE_BRIDGES=1` on the 2026-11-15 shared build.

- Translated work is 316,778 instructions per frame across 1,892 routines.
- About 51.7k per frame of that is the 136EA0 render bridges
  (136EA0/135970/166C00/184690/167EF0/171070/136E10). They are native in
  production and appear only because their `Frame` bridges ignore the census
  switch.
- Raw files are in `build/test-results/remaining-translated-20261115/`:
  `armory-census-now.txt` and `remaining-by-unit.json`, the per-unit top 25.
- Tooling: `tools/perf_4k60/census_map.py`.

## Claims

Each agent owns one group. Do not port another group's owners. If you need
one, call it through its current route.

| Group | Units and owners (per-frame in Armory) | Owner |
|---|---|---|
| H. Havok physics world | `physics_world_runtime_generated` (69k, 321 routines: 304350, 1F5FE0, 30E930, 2EAFB0, 2EC4B0, 30CF40, ...), its copies (2F36F0, 2E2E30, 2E3250, 30D3F0, 312620, 2F8300), `physics_phantom_world` / `physics_phantom_force` (1D9EC0, 22D460, 282870, 315C90, 1D9080, ...), `physics_vehicle_update` (E8D80, B9770, 1CD6A0), `physics_game_arena` (125280, 165700, 1221B0, ...) | agent H |
| L. Live-support remainder | `physics_live_support` and its multi-copy owners (1D6FB0, 1F8200, 110350, BCB70, 2EC9B0, 120FA0, 13FE50, D80A0, 179CD0, C9550, 1E5810, 1E5C80, 1E3150, 1EC970, 243F20, 32CC0C, 16E450/16E4B0, 16E600, 22A790, 1211C0, 171010, C1E90, 138D90, 145560/1E1200/BAD60/144910 copies). Bind existing object-factory natives where proven, and port the rest. | agent L |
| X. 169C46 / C2CB0 / C3430 light update / Havok shape builders / codec loops | the first-person caller and the light update chain | running agent (nodebuild) |
| E. Effects | `death_cycle_runtime` and its copies (BA9E0, 176A40, 17C140, 252100, 178070, C3A20, 17C370, 17C5C0, 178110, 22B790, 251C80, 250780, 17BEB0, 30BD0, ...), `source_effect_particle_frame` / `_draw`, `source_effect_light_view` (C2A80, C3300, C35D0), `source_lens_flare` (20680, 17C710, 20700), `native_light_shape` C3AB0 | agent E |
| F. Frame and gameplay misc | `source_multiplayer_engines` (1EEE80, 1EFD60, 1F0670, 1503D0, 153360, 154D00), `source_campaign_environment` (131AC0, 133A70, ...), `source_combat_action` (153BA0, 1540D0, E79C0, ...), `source_geometry_cache_frame` (1306B0, ...), `source_damage_delivery_runtime` (D8330, DB0A0), `source_model_render` (41970, D7230), `source_texture_camera_producer`, `source_map_world_support` | agent F |
| Paused (safeguard stops) | `source_shadow_frame` remainder (1362A0, 134D30, 4BAE0, 1365C0, 136D50, 166C90), `source_scene_device_constructor` (1179C0, 10B840, ...), the 16C620 MP verify | waiting on the user |
| Other session | `source_ai_frame` (1B0020, 1B01E0, 209F40, 1B08B0, ...), `source_ai_actor`, `source_campaign_runtime` AI owners | other session |

## Rules for every group

- **Measure first.** Confirm genuine translated execution with a census
  (`HALO2_CENSUS_NATIVE_BRIDGES=1`) or a sampled profile before porting.
- **Prove, then bind.** Use the established native-replacement method:
  - oracle proof;
  - complete reachable coverage;
  - an out-of-tree mutation screen;
  - ASan/UBSan;
  - a `HALO2_REFERENCE_*` switch;
  - a live verify or differential in the Armory and in an MP sweep with
    scripted input (fire, grenade, jump, use, move);
  - frame parity, Armory frame `5627a51badf47762` / trace `8a9b58b38bef`.
- **Shared tree builds** go only through `scratchpad/build_perf.sh`.
- **Don't collide.** Before editing a file, check `git status` and its mtime.
  If another session changed it in the last hours and it is not yours, keep
  your edit surgical, or coordinate through this doc.
- **StringIDs.** Any native that compares StringID literals must use the MCC
  mapping (`ObjectFunctionNamespace`; see `docs/NATIVE_OBJECT_FACTORY.md`).
- **Track progress here.** Append your progress to this doc (group, owners
  done, census delta).

## Code standard (user, 2026-11-15)

"i need native stuff only, mainly because this will be open source and will
eventually be ported and studied. Too much shortcuts are bad for the
legibility and cleanliness of the codebase."

Every native replacement is hand-written systems code:

- **Readable C++.** Use named structs and fields for game data, such as
  object, unit, light and Havok entity layouts. Accessors replace raw offsets.
  Control flow is clear, and comments say what the system does and why. No
  simulated register file or flags in the logic, and no address-shaped
  helpers.
- **Thin boundary adapters.** Where a native owner is still called from
  translated code or calls into it, the exact ABI lives in a small, separate
  adapter: registers, flags, stack slots, cursor, partial-register writes. The
  logic itself stays clean. Delete the adapter when the neighbour goes native.
- **Exactness stays.** FP operation order, NaN payloads, and memory-write
  order are kept where they are observable. Comments mark where the original
  order matters.
- **Proofs live in tests.** Oracle harnesses, mutation screens, sanitizer
  runs, and live verify or differential runs live in `tests/` and `tools/`,
  not in production logic.
- **No generator or recompiler output** counts as "native".

## Legibility refactor backlog

These are native and proven but machine-shaped: they replicate registers,
flags, and stack inside the logic. Each needs a rewrite to this standard,
re-proven with the existing harnesses.

- ~~B8F40/B9090/B9180/BE4A0/10DD10 object tick and BE850 subtree closures
  (`src/physics/runtime/native_tu15_object_tick.hpp`,
  `native_tu15_object_subtree.hpp`)~~ done 2026-11-16: rewritten (logic
  `native_tu15_object_tick_systems.hpp` and
  `native_tu15_object_subtree_systems.hpp`, adapters in the two original
  headers), re-proven and live-verified; see "Object update owners: rewrite
  to the code standard, and C0F60 restored"
- ~~131470 cluster distance (`native_tu15_cluster_distance_flood.hpp`)~~ done
  2026-11-16: logic `native_tu15_cluster_distance_flood_systems.hpp`, adapter
  `native_tu15_cluster_distance_flood.hpp`; same section
- BD080 sphere query (`native_tu15_object_sphere_query.hpp`)
- 1532A0 PVS flood (`native_tu15_pvs_flood.hpp`)
- 14DD70 and the other matrix leaves (`native_tu15_matrix_leaves.hpp`)
- the 135B60/136A80 renderer closures
- ~~group H physics world leaves 1F5FE0/2E4550/303BA0/2E47B0~~ done
  2026-11-15: rewritten (`native_tu15_world_leaves_systems.hpp` with the
  adapter `native_tu15_world_leaves.hpp`), re-proven and bound; see "Group H
  progress: rewrite, bind and 303EE0"

Audit others as they come up.

## Progress: group F (agent F)

2026-11-15 19:50. Five owners have proven closures (record/replay harness,
census and verify modes, ASan/UBSan, mutation screens):
1306B0 geometry cache count, D7230 attachment update, 1EEE80 engine player
update, 41970 attachment schedule, 133A70 environment palette
(`tools/validate_native_frame_misc_tu15.py --owner X`, binder
`tools/native_frame_misc_replacements.py`, registered in
`spatial_owner_bindings`). They are machine-shaped, so they are **not bound**
by default (`BOUND = ()` in the binder) until rewritten to the code standard.
The shared build started about 19:45 was specialized before that change and
does carry them. Their reference switches restore the translated bodies:
`HALO2_REFERENCE_GEOMETRY_CACHE_COUNT`, `HALO2_REFERENCE_ATTACHMENT_UPDATE`,
`HALO2_REFERENCE_ENGINE_PLAYER_UPDATE`, `HALO2_REFERENCE_ATTACHMENT_SCHEDULE`,
`HALO2_REFERENCE_ENVIRONMENT_PALETTE`. The next build unbinds them.

Census-accuracy admissions (no logic change): 1E6E80 now honours
`HALO2_CENSUS_NATIVE_BRIDGES`; the 10BE20 source frame skips its PC reports
through `native_census_detail::skip_report`. Both existing oracles pass.

## Group H progress (physics world), 2026-11-15

**Measured.** The census (`armory-census-now.txt`) for group H units:

| Unit | Instr/frame |
|---|---|
| `physics_world_runtime_generated` | 69.0k (321 routines) |
| its +1-copy owners | 4.2k |
| `physics_game_arena` | 3.5k (125280 is one call in 300 frames: about 1M instructions at load, not per frame) |
| `physics_vehicle_update` | 2.8k (E8D80 2.2k, B9770 0.5k, 1CD6A0 0.1k) |
| `physics_phantom_world` | 1.7k (1D9EC0 1.6k) |
| `physics_phantom_force` | 1.0k (315C90 0.7k) |

About 12.8k/frame of the world unit is already bound native but refuses under
the census switch, so it is counted as translated. These are 2EAFB0 3.9k,
2EC4B0 3.4k, 2E2E30 1.3k, 1F6610 1.1k, the allocator, pool and heap ABIs
(about 2.0k together), 2E3250 0.4k, 30D3F0 0.4k, 312620 0.3k and 30D830 0.3k
(build receipt `native-operands/.../physics_world_runtime_generated.operands.json`).

**Census accuracy (done, census-only).** 2EC4B0 (`native_tu15_broadphase_add_object.hpp`)
and 2EAFB0 (`native_tu15_broadphase_add.hpp`) now admit through
`native_partition_temporary_detail::admitted_census`, the helper another
session added at 19:13. Each is a one-line, surgical change; production
admission is unchanged. Not yet measured: no census was run after it.
Converting the shared array, pool and heap ABI headers to `admitted_census` is
left to their owner, because group L is already converting these headers.

**Ported, proven, not bound** (superseded: rewritten and bound, see "Group H
progress: rewrite, bind and 303EE0" below). `native_tu15_world_leaves.hpp` holds native
kernels with record/replay verify (`HALO2_VERIFY_PHYSICS_WORLD_LEAVES`) and
`HALO2_REFERENCE_PHYSICS_WORLD_LEAVES` for these owners:

| Owner | Canonical body | Census before (instr/frame) |
|---|---|---|
| 1F5FE0 | `run_1f16f0`, collision surface polygon | 8.2k |
| 2E4550 | `run_2e0a60`, relative transform | 2.6k |
| 303BA0 | `run_300100`, contact record copy | 0.9k |
| 2E47B0 | `run_2e0cc0`, transposed rotation | 0.6k |

Together that is 12.4k/frame. The proof is
`tools/validate_native_world_leaves_tu15.py` with
`tests/native_tu15_world_leaves_tests.cpp`:

- 40,000 comparisons with 805,844 ordered stores and 3,918 fault cases;
- census mode 10k and verify mode 10k;
- ASan/UBSan 8k plus 1k verify;
- 11/11 branches;
- 30/30 mutants rejected;
- source hashes verified.

The report is `build/test-results/physics-world-leaves-20261115/native-tu15-world-leaves.json`;
the details are in `docs/native_replacement/1F5FE0_world_leaves.md`.

They were bound and built once (`build-perf-4k60/halo2_native` 19:45, receipt
`native_spatial_owners` 1f5fe0/2e4550/303ba0/2e47b0). The new code standard
arrived before any live run. The kernels are machine-shaped, so the
registration in `specialize_native_operands.py` and
`NativeOperandSpecialization.cmake` was removed. The binder
(`tools/native_world_leaves_replacements.py`) is kept, and the next shared
build drops the bindings. No Armory or multiplayer verify, parity or census
has been run with them.

**Continuation (group H).**

1. Rewrite the four leaves to the standard, then re-run the validator
   unchanged. The harness compares against the translated bodies, so it
   stays valid. The shape of the rewrite:
   - `struct BspSurface` (+2 first edge, +4 flags byte, +6 word), `BspEdge`
     (12 bytes: vertex[2], next[2], +0xA right surface) and `CollisionBsp`
     (+0x2C surfaces, +0x34 edges, +0x3C vertices);
   - `struct PolygonShape` (+0 vtable 4620D0, +6, +8, +C radius 48B880, +10,
     +14 vertex count, +18/+1A/+1B/+1C, vertices at +0x20);
   - `struct HavokTransform` (scale and a 3x4 matrix, or four rows);
   - a `walk_surface_edges()` loop in plain C++;
   - a separate adapter that reads the stack arguments and EDI and publishes
     EAX/EDX/ESP/flags/XMM/cursor, keeping the stack-slot reuse writes
     (argument 1 and 2 slots, the three vertex spills) and the store
     sites, because observers see store PCs.
   Then re-register the binder in `spatial_owner_bindings` (marker
   `static void run_1f16f0(ObservedWorldState`) and the cmake helper list.
   After that: the live Armory verify, a census on one binary (reference
   switch on and off), frame parity (`5627a51badf47762` / `8a9b58b38bef`),
   and the multiplayer verify sweep. A private copy of `nb_sweep.py` that
   keeps the `Physics world leaves verify` lines is in the session
   scratchpad as `h_sweep.py`. Run on vehicle maps: coagulation, headlong,
   zanzibar, containment and burial_mounds.
2. Next genuine owners, largest first:
   - 304350 (10.4k, 534 instructions; children 30CF40, 304130, 2F36F0,
     303EE0, 30CD00, 2E47B0);
   - 30E930 (4.75k; a typed controller exists,
     `30E930_convex_simplex.md`);
   - 30CF40 (3.0k, 192 instructions; child 2E31E0 is rare);
   - 2E4FE0 and 2F8630 (2.6k and 2.4k; semantic natives exist in
     `native_tu15_contact_query.cpp`, unbound);
   - 304130 (2.3k);
   - 1F6D10, 1F76A0 (1F76A0 has a jump table and a 185AA0 dispatch) and
     1F7C10, about 4.3k together;
   - 2F33B0 (1.5k, recursive);
   - 1D7260 (1.5k);
   - 303EE0 (1.0k).
   Vehicle: E8D80 (2.2k). Phantom: 1D9EC0 (1.6k) and 315C90 (0.7k). The
   havok track's semantic natives (1CD6A0, 315CB0, 1D9080/1D9100) are not
   bound at the live sites.

## Progress: group L (agent L), 2026-11-15

Details: [LIVE_SUPPORT_LEAVES.md](LIVE_SUPPORT_LEAVES.md).

**Most of group L was already native; the census overcounted it.** Two
kinds of native owner inflated the census:

- cursor-gated bridges that refuse census callbacks: 13FE50 with its
  32A660/13FE00/140310/12FCB0/4057F0 children, 2EC9B0, 120FA0, 1E5810,
  1E5C80, 1E3150 and 1E5AF0;
- complete natives that report every original PC: D80A0, BCB70's scalar
  frame, and the weapon-marker owners BAD60/1E1200/144910/145560/BACD0/
  FC510/1E11B0, plus the 16EA60/16E6A0/145560 effect protocol.

They now honour `HALO2_CENSUS_NATIVE_BRIDGES`. The mechanisms are
`native_census_detail::uncursored`, the new
`native_partition_temporary_detail::admitted_census`, and
`native_census_detail::skip_report` (`HALO2_CENSUS_COUNT_NATIVE_REPORTS=1`
restores the reports). Production admission is unchanged.

Census on one binary (`halo2_native.L1`): 308,533 → 292,715 per frame
(−15,818); the group's tracked owners 28,826 → 13,117.

The effect-protocol leaves (16EA60/16E6A0/145560 fast path) were reverted:
they write the cursor unguarded, so they crashed the census. They stay
counted.

Census on one binary after the ports (`halo2_native.L3`), group L reverted
against native: the tracked owners 28,848 → 9,811 per frame (−19,037). Of
that, 16,222 is census honesty and 2,815 the ports.

**Ported to the code standard: five translated leaves.** They total 2,815
per frame:

- 110350: 1,009;
- 1EC970: 746;
- 1F7F60: 553;
- C9550: 453;
- C1E90: 54.

The systems logic is in `native_tu15_live_support_systems.hpp` and the ABI
adapters are in `native_tu15_live_support_leaves.hpp`. They are bound by
`tools/native_live_support_leaves_replacements.py`.

Proof: 36,000 comparisons, verify 5,606, census 6,000, ASan/UBSan
9,000 + 1,126, every reachable branch direction, and 41/41 mutants
rejected.

**Live checks.** Armory parity: native, reference and verify all give
`5627a51badf47762` / `8a9b58b38bef`. Coagulation MP parity
(4:90/4:150/1:150): reference = native = verify.

MP sweep, all 25 maps in 1p and 4p, native and verify roles:

- no verify difference;
- 43 cases with identical frames;
- 3 cases with nondeterministic 4p frames;
- needle and street_sweeper stop on a pre-existing active-camouflage shader
  resource error, also on the reference route and on the pre-change binary.

**Remaining in group L** (honest census): 32CC0C 2.1k, 1D6FB0 1.6k, 1F8200
1.0k, 16E600 0.9k, 144090 0.6k, 10DE40 0.6k (native, not census-admitted),
179CD0 0.5k, 16E6A0/16EA60 0.8k (native, unsafe census admission), 22A790
0.5k, 243F20 0.5k, 1211C0 0.45k, 171010 65, and small ones. See the table in
LIVE_SUPPORT_LEAVES.md.

**Continuation.**

1. Port 243F20 (vertex decompression: named descriptor and bounds structs,
   and a jump-table adapter).
2. Port 32CC0C (x87 `_ftol2`: logic as the int64 truncation with explicit
   FISTP/FSUBP order, and an adapter owning the x87 stack and spill slots).
3. Port 16E600 and 1F8200.
4. Guard the effect leaves' cursor writes so that they can be
   census-admitted.
5. Rewrite 171010 to the standard. The first draft's kernel and fixture
   were removed; the harness takes one more `Leaf` entry and author.

## Group H progress: rewrite, bind and 303EE0 (2026-11-15, evening)

**Rewritten to the code standard and bound: 1F5FE0, 2E4550, 303BA0, 2E47B0.**

- Logic: `src/physics/runtime/native_tu15_world_leaves_systems.hpp`, readable
  systems code:
  - named layouts: `CollisionBsp`, `BspSurface`, `BspEdge`, `BspVertex`,
    `RealMatrix4x3`, `HavokTransform`, `PolygonShapeHeader`, `ContactRecord`;
  - plain loops, with every access naming its role;
  - explicit-destination SSE where FP order matters, marked in comments.
- Thin boundary adapter: `native_tu15_world_leaves.hpp`. Per leaf it holds the
  role -> instruction table, the stack arguments, register saves and
  restores, the exit registers and flags, the scratch-XMM residue and the RET.
- The equivalence harness is unchanged. It gives the same figures as the
  machine-shaped kernels: 40,000 comparisons, 805,844 ordered stores, 3,918
  faults, census 10k, verify 10k, ASan/UBSan 8k + 1k, 11/11 branches.
- The mutation screen was retargeted at the new source (its anchors are
  text): 34/34 rejected.
- CTest `native_tu15_world_leaves` (`cmake/NativeTu15WorldLeaves.cmake`).
- Binder re-registered (`spatial_owner_bindings` and the cmake helper list).
- Details: `1F5FE0_world_leaves.md`.

**Ported, proven and bound: 303EE0** (contact record append, 1.0k/frame, a
child of 304350). This is the first group H parent with children:

- 2E3250 is entered as a genuine child through the unit's route, with exact
  registers at the call. Verify mode records its boundary, and the binder's
  reference copy routes the translated call to `replay_child`.
- 303BA0 runs its native kernel inline.
- Proof `tools/validate_native_contact_array_tu15.py`: 40,000 comparisons,
  1,075,898 ordered stores, 1,238 faults, 6,250 growth calls, census 10k,
  verify 10k, ASan/UBSan 8k + 1k, 2/2 branches, 13/13 mutants.
- CTest `native_tu15_contact_array`; binder
  `tools/native_contact_array_replacements.py`.

**Live (all five bound, 21:24 build).**

- Armory verify: 1F5FE0 9,992, 2E4550 5,768, 2E47B0 5,141 and 303EE0 9,571
  calls identical; coverage `1fff`.
- Frame parity: native, reference and verify all give `5627a51badf47762` /
  `8a9b58b38bef`.
- Census on one binary (reference switch on, then off): 290,558 -> 277,131
  per frame (-13,427); the five owners go to 0.
- MP verify sweep (5 vehicle maps at 1p/4p, 4 more maps, 2 campaign maps):
  713,089 calls, 0 differences, 0 refusals. The campaign 03a/05a exits are a
  pre-existing renderer-thread error that reproduces with the reference switch.
- MP frame parity on coagulation with motion: PASSED.
- Details: `1F5FE0_world_leaves.md`, "Live verification".

**Findings on the "bind existing natives" items.** None meets the code
standard as it stands, so binding it as-is is not an option:

- 30E930: the in-tree typed solver (`native_tu15_convex_simplex_solver.hpp`)
  covers only the vertex/vertex and first-edge/vertex paths ("wider
  simplex/projection branches refuse explicitly"). The complete controller
  in `30E930_convex_simplex.md` exists only in private corpora.
- 2E4FE0/2F8630 (`native_tu15_contact_query.cpp`): `std::function` services,
  raw offsets.
- 1D9EC0/315C90 (phantom world/force units): 1D9EC0 loops over a phantom's
  entries (count +74, 0x60-byte rows at +70) asking 315C90 (`*out = obj+44 ?
  (obj+44)->byte 29 : 0`) per entry; the pair is self-contained and native
  together would remove about 2.4k/frame. But the phantom units have no
  `selected()` resolver and no bridge mechanism, so binding needs new
  build-time specialization for those units first (a shared-build change).
- E8D80 (vehicle update unit, canonical `run_e7e60`): not a leaf. It loops
  over 60 bits of an object mask and calls through the function table at
  46EC68 (an indirect dispatch child). It also needs the vehicle unit's
  canonical-PC observation remap.

**Continuation (group H), largest first.** The 303EE0 adapter is the pattern
for every owner below:

- `Port::call_child` for translated children, with exact registers at the call;
- the binder's routed reference copy plus `replay_child` for verify mode;
- children that are already native run inline at their own boundary.

1. 304350 (10.4k, canonical `run_300870`, 536 instructions, 34 x87
   operations). Children:
   - native now: 2E47B0 ×2, 303EE0;
   - trivial leaf, to inline: 30BAD0 (`MOV EAX,sentinel; RET`);
   - small dispatchers: 303C70 and 303D00 (about 55 instructions, one
     dispatch each);
   - large x87 owners: 2F36F0 (577 lines), 304130 (144), 30CD00 (189) and
     30CF40 (194);
   - plus 7 virtual dispatch sites.
   Port 303C70/303D00 first, then 304350 with the large x87 owners as child
   boundaries. The x87 parts need the raw x87 scope precedent
   (`native_tu15_raw_x87_scope.hpp`).
2. 30CF40 (3.0k, `run_309450`): `FLD/FMUL/FSTP` (80-bit intermediates,
   rounded on spill under the live control word) plus packed SSE, and a rare
   2E31E0 growth child.
3. 1F76A0 (1.55k, `run_1f2db0`): a shape-key predicate. It runs a 4-entry
   jump table at 1F77AC over the key type (bits 29..31):
   - type 1: an instanced surface, which asks 185AA0 through the dispatcher;
   - type 2: a collision BSP surface, tested in the breakable-surface bit
     vector at [4F9B50]+1;
   - type 3: an object's byte at +11A, compared with the key's secondary
     index;
   - type 4: always true.
   Each exit leaves a different EAX upper part, ECX, EDX and deciding flag
   operation, so the logic needs a per-path Residue. The jump-table read
   belongs in the adapter; an unknown target must throw the translated
   route's messages ("Required original solver dispatch target unavailable"
   for 0, "Unbound TU1.5 contact dispatch target" otherwise).
4. 2E4FE0 and 2F8630, then 304130, 1F6D10 and 1F7C10 (1F7C10 calls the
   native 1F5FE0 and 1F76A0), 2F33B0 (recursive) and 1D7260.
5. Phantom 1D9EC0+315C90 and the havok-track sites (1CD6A0, 315CB0,
   1D9080/1D9100), once the phantom units have a binding mechanism. Then the
   vehicle E8D80, with a dispatch-child boundary.

## Group H continuation: paused (2026-11-15)

A safety classifier stopped the next group H agent (304350 family) before
anything was ported. The tree is clean: its unfinished header was deleted and
no tracked file changed. Its finding, for whoever resumes:

- In 304350 the adapter is the hard part, not the logic.
- Its children (304130, 30CD00, 30CF40) push callee-saved registers, and
  verify mode compares the full register file at every child boundary.
- So the adapter must reproduce, at each child call (including the virtual
  dispatch sites and the calls inside 303C70/303D00):
  - registers, flags, XMM, and x87 status;
  - the original's frame-slot stores, in order.

Remaining group H work, in instructions per frame:

| Owner | Instr/frame |
|---|---|
| 304350 | 10,361 |
| 30CF40 | 2,958 |
| 2E4FE0 | 2,648 |
| 2F8630 | 2,367 |
| 304130 | 2,291 |
| 1F6D10 | 1,552 |
| 1F76A0 | 1,551 |
| 2F33B0 | 1,526 |
| 1D7260 | 1,494 |
| 1F7C10 | 1,162 |

## Progress: group L, second batch (2026-11-15, evening)

Details: section 4 of [LIVE_SUPPORT_LEAVES.md](LIVE_SUPPORT_LEAVES.md).

**Ported to the code standard and bound** (logic in
`native_tu15_live_support_systems.hpp`, adapters in
`native_tu15_live_support_leaves.hpp`, binder
`tools/native_live_support_leaves_replacements.py`):

- 32CC0C `_ftol2` (2,079/frame), all six copies, in both caller x87 models;
- 16E600 `data_delete_all` with its strncpy call 32A4E0 (937 + 34), five copies;
- 243F20 vertex position (495), three copies;
- 1211C0 point on triangle, calling the 123210 leaf (450), four copies;
- 171010 interval table (65), four copies (the withdrawn draft rewritten).

**Census honesty** (no production change): 16EA60/16E6A0/145560 (the
unguarded cursor store fixed), 10DE40 and 144090.

The leaf framework's `Machine` now carries the x87 stack
(`native_tu15_matrix_leaves.hpp`, additive).

**Proof** (`tools/validate_native_live_support_leaves_tu15.py`, CTest
`native_tu15_live_support_leaves`): 79,200 comparisons, 1,103,667 ordered
stores, 4,483 faults, verify 12,448, census 13,200, ASan/UBSan 19,800 + 1,136,
every reachable branch direction (strncpy 32A4E0: 22/48, the rest
unreachable with count 2), 104/104 mutants rejected, source hashes rechecked.

**Live** (`halo2_native.L5`, sha256 `653d5c4f…`): Armory parity native =
reference = verify = `5627a51badf47762` / `8a9b58b38bef`.

**Census on one binary (L5)**: all translated 351,580 → 271,096 per frame;
group L's 42 tracked routines 28,848 → 3,778. Against the previous honest
census (L3) this batch's routines total 6,082 → 43 per frame (−6,040).

**Multiplayer sweep** (L5; every MP map in 1p and 4p, 50 cases, native and
verify roles, the node-rebuild scripted input; receipt
`build/test-results/live-support-leaves-20261115/mp-sweep-L5.json`): no verify
difference in any case; 43 cases with identical native and verify frames; the
3 known nondeterministic 4p cases (ascension, elongation, lockout) differ; the
4 needle and street_sweeper cases stop in both roles on the known
`Source dynamic model resource unavailable: ... active_camouflage` error. This
is the same outcome as the L2 sweep.

Live runs need `tools/run_test.py --scratch-root .local/test-scratch`: on the
default tmpfs scratch the MCC sound DAT refuses its snapshot in every role.

**Continuation (group L).**

1. 1F7E90 (232/frame): a pure leaf, the channel iterator's *begin* (owner,
   position -1, the 16 selection bytes zeroed, copied, or remapped through a
   0x10-byte-record table with its count clamped to 16). It belongs beside
   `animation_channel_iterator` and fits the leaf framework as is.
2. 1F8200 (1,017), 22A790 (503) and 179CD0 (549) call children that are
   natives of other frameworks or another group's owners (144090 and 1E6980
   bridges, 4057F0 resource busy, group E's 17C140/17C370/17C5C0). A leaf
   kernel cannot enter a state-based child, and the verify overlay cannot run
   one. They need a child-call boundary in the leaf framework (publish the
   Machine to the state, call the child's route, reload it; in verify mode
   record the child's boundary as group H's 303EE0 does), or group L ports
   them as owners in that style.
3. 16E450 → 16E4B0 → 32A4E0 (23/frame + strncpy): 16E4B0 copies the array
   name with count 32, so it needs the general strncpy (the dword-at-a-time
   copy with the 7EFEFEFF terminator test, and the dword padding loop), not
   16E600's two-character case.
4. 138D90 (24): calls 138510/138C20 and two dispatches.
5. 1D6FB0 stays with group H.

## Progress: group L, third batch (2026-11-16)

Details: section 5 of [LIVE_SUPPORT_LEAVES.md](LIVE_SUPPORT_LEAVES.md).

**Ported to the code standard and bound** (logic in
`native_tu15_live_support_systems.hpp`, adapters in
`native_tu15_live_support_leaves.hpp`, binder
`tools/native_live_support_leaves_replacements.py`):

- 1F7E90, the channel iteration's start (232/frame);
- 32A4E0, the C runtime's strncpy in general (the dword loop with its
  zero-byte test), bound by itself in nine units; 16E600 now uses it;
- 16E4B0 and 16E450, the data array header and `data_initialize` (23);
- 22A790, the referenced-resource busy scan (503), the first leaf with a
  genuine child (4057F0).

**Child-call boundary** in the leaf framework
(`native_tu15_leaf_children.hpp`, new): genuine children with the exact
registers at the call; verify mode records the native's accesses and child
boundaries and replays the binder's reference copy (its child calls routed to
`replay_child`). `native_tu15_matrix_leaves.hpp`: the admission is extracted
into `admitted()` (surgical, no behaviour change).

**16E600 residue fixed**: the three calls per 300 frames came from the
physics world unit's canonical copy `run_16d220`, never matched by the
binder. Its TU1.5 form is byte-identical to the proven body; it is now bound
(guarded by `selected_observation_pcs`).

**Proof** (`tools/validate_native_live_support_leaves_tu15.py`, CTest
`native_tu15_live_support_leaves`): 115,200 comparisons (2,415,772 ordered
stores, 8,926 faults), verify 17,737 (22A790 through the record/replay child
boundary), census 19,200, ASan/UBSan 28,800 + 1,123, every branch direction
of the new leaves (32A4E0 now 48/48), 157/157 mutants rejected, source hashes
rechecked. Receipt `build/test-results/live-support-leaves-20261115/validator-L6.json`.

**Live** (`halo2_native.L6`, sha256 `8d973b94…`): Armory native = reference =
verify = `5627a51badf47762` / `8a9b58b38bef`; the verify run checked 798
22A790 calls (51,204 ordered accesses, 2,969 child boundaries identical).
MP sweep: no verify difference in any case; 43 cases with identical native and verify frames, and all 43 native frames identical to the L5 sweep; the verify role checked 387,384 22A790 calls through the child boundary (23,992,972 ordered accesses, 1,382,749 child boundaries identical); the 3 known nondeterministic 4p cases (ascension, elongation, lockout) differ; the 4 needle and street_sweeper cases stop in both roles on the known `Source dynamic model resource unavailable: ... active_camouflage` error. This is the L2/L5 outcome.

**Census on one binary (L6)**: all translated 350,910 → 269,622 per frame;
group L's 42 tracked routines 28,848 → 3,238 (L5 3,778). This batch's
routines 1,728 → 3.1 per frame (22A790 503, 16E600 937, 1F7E90 232, 32A4E0 34,
16E450 12, 16E4B0 11 → 0, 0, 0, 2.2, 0.4, 0.5; the residue is the physics pool
startup unit's canonical copies, which report canonical store PCs).

**Continuation (group L).** Details and the analysis of each owner: the
continuation in section 5 of LIVE_SUPPORT_LEAVES.md.

1. 1F8200 (1,017/frame), the channel ray cast: children 144090 (dispatch
   1F8284) and 1E6980 (local 1F84A4) through the child boundary, 1F7E90 and
   1F7F60 inline; every XMM register is caller-visible at exit.
2. 179CD0 (549), the effect update loop: five children (16EA60, 17A380
   dispatches; group E's 17C370, 17C5C0, 17C140 as genuine children).
3. 138D90 (24): x87 FISTP spills, two IDIVs, four children.
4. The pool startup unit's canonical 16D070/16D0D0/16D220/326970 copies
   (about 3/frame) would need their canonical store PCs mapped; the physics
   world unit's run_326970 can bind as a canonical 32A4E0 copy now.

## Progress: group X, the light update (2026-11-16)

The tick's `call_actual(0xc3430u)` (13AB4E) is native and the default:
new track `lights`
([lights.md](../native_object_factory_tracks/lights.md)), switches
`HALO2_REFERENCE_LIGHT_UPDATE_TU15=1` (translated) and
`HALO2_LIGHT_UPDATE_DIFFERENTIAL_TU15=1`.

**Ported to the code standard:**

- C3430 `lights_update` and C5290 `light_delete` (`DataArray`, `GameTime`
  and `Light` views; the live world's `light_tick` adapter holds the FP
  controls, the MCC object-function namespace and the switches).
- 16D740 `collision_sphere_fit`, C4D30's collision sphere fit, with its
  structure-BSP closure (16D2A0, 1E60C0..1E6570, 120FA0, 24EDB0..24E580,
  24FCC0..24F9C0, 30BD0) in placement: host structs for the original's
  stack records, a `CollisionBsp` view, small x87 helpers. Instanced
  geometry and objects (16CF00, 16D0E0) stay named refusals; C4D30 never
  asks for them.
- Found by the live differential and ported: C3910 (BEC50 0x130006BB, the
  lights' transition) and 13E6E0 (C2B30's light colour function, its MMX
  colour blends as 16-bit lane steps) in attachments; C3AB0's
  degenerate-target fallback in placement.

**Proof** (strict writes, seed 0x5C41, all-tracks harness): lights 1,800
cases (290,642 stores; C3430 132/134, C5290 79/79); placement light and fit
kernels 4,100 (148,173; 16D740 627/627 and every reachable closure
instruction); attachments 8,300 (114,765; C3910 20/20) and a direct 13E6E0
kernel 1,600 (13E6E0 183/183). ASan/UBSan 7,400 cases, no report. Mutation
screen out of tree, in-tree hashes checked: 74 mutants, 57 rejected; the 17
survivors are argued equivalent in the track docs (operand orders that only
move NaN payloads, x87/SSE steps that only part on denormals under DAZ/FTZ,
boundaries no reachable input meets).

**Live:** differential sweep (every MP map 1p/4p with scripted input,
campaign 01a..08b as far as they run): 56 runs, 28,573 C3430 calls
compared, 0 differed, 0 refused (the first sweep's 99 refusals were
13E6E0, now native). Armory parity (translated, native, default):
`5627a51badf47762` / `8a9b58b38bef`; MP parity Coagulation (native and the
default binary) and Highplains (motion) PASSED.

**Census:** one binary (`ff04359c...`), `HALO2_CENSUS_NATIVE_BRIDGES=1`, the
translated route (`HALO2_REFERENCE_LIGHT_UPDATE_TU15=1`) against the
default. Coagulation 4p, 450 ticks of the sweep's scripted input:
279,434,088 translated instructions -> 264,596,131 (-14.84M, -5.3%); C3430
450 entries / 1.21M instructions -> 0; C2CB0 21,770 / 5.28M -> 64 / 15k
(light attachment births, C2260); C2B30 21,770 -> 64; 134870 43,108 -> 112;
BEC50 44,112 / 2.02M -> 16,927 / 0.79M; C9070 20,803 -> 11,200; BC8F0 8,367
-> 4,903; C3910 3,107 -> 532; C4D30 25,430 -> 25,173 and 16D740 8 -> 8
(their other callers, BA630 and C0F60). Armory 01a, 300 frames: 80,887,608
-> 79,701,781 (-1.19M, -1.5%); C3430 450 / 165k -> 0, C2CB0 2,706 / 528k ->
6 / 1.2k. BCB70 and 10DE40 have no translated entries in either role on this
binary.

**Remaining:** 16D740's instanced-geometry and object queries (the effects'
17D625 passes bit 2); BCB70/10DE40/BC8F0 entries of other callers (the
effects' 17C370 and 17EAD0, C0700); 13E6E0's copies outside C2B30.

## Progress: group X, light update continuation (2026-11-16)

**Refusals removed from the default light natives** (no sweep reached
them; each would have stopped the game):

- 16D740 and C1540 past 128 stacked planes: 1E6150's stack runs over the
  walk record's projection state; the native keeps the overlap
  (`WalkRecord`: the stop byte, the projected point, the axis word).
  Slot 131 (1E60C0's return address) and a ninth polygon vertex (24EB60's
  return address) stay refusals: the original faults there.
- C3AB0's orthogonal frustum (type 1): its slopes from [0x474CD4].
- BEC50's callees: C1F90 (the heading), BEAA0, 15B2D0 (the player scale),
  BE9A0 (the object's bytes; `active` forced) and the console clock
  (BEBE0, BEB00, BEB40, BEB90 over 13BB70/13BB10, GetLocalTime: the native
  reads the host's local time).

**Bound at the other translated callers** (track docs: lights.md, "Other
translated callers"): C4D30 (with 16D740) at C0F60's C100F, C3750's C3810
and C20D0's C2135; C2CB0 at C2260's C2353 (required boundaries re-emitted
by `tools/regenerate_light_caller_boundaries_tu15.py`, also in the live
support generator's list); BCB70 (with BC8F0 and 10DE40) at C0700's C0756.
Only the object-function join applies to these closures
(`ObjectFunctionNamespace`). Switches `HALO2_REFERENCE_LIGHT_CALLERS_TU15`,
`HALO2_LIGHT_CALLERS_DIFFERENTIAL_TU15`; native by default.

**Proof** (strict writes, seed 0x5C41, the all-tracks harness, final
sources): lights 1,800 cases (297,687 stores), attachments' 35 kernels
31,800 (159,444; BEC50 and 10DE40 again on the tightened fixture, 2,200),
placement's 38 kernels 15,550 (653,295). Coverage: 1E6150 260/260, 16D740
627/627, C1540 43/43, C3AB0 182/194 (the sphere branch), C1F90 13/13,
15B2D0 54/54, BE9A0 75/75, BEAA0 22/24 (dead), BEC50 239/252 (the clock
calls, proven by five clock kernels entering after 13BB10, and BEE69).
ASan/UBSan: 10,800 cases (479,531 stores), no report. Mutation screen out
of tree, in-tree hashes checked: 38 mutants, 30 rejected; the 8 survivors
are argued equivalent in placement.md and attachments.md.

**Live:** differential sweep (every MP map 1p/4p with scripted input,
campaign 01a..08b as far as they run), light callers and light update
differentials together: 56 runs, 1,209,362 caller calls compared (C100F
1,094,354, C0756 112,342, C2353 2,387, C3810 266, C2135 13), 0 differed, 0
refused; light update 28,573 compared, 0 differed (sweep 1's 14,832 C0756
differences were BCB70's own popped argument, where 10FB70 stores its
slot: the differential now treats the stack below the return ESP as dead,
as the node rebuild's does). Armory parity (translated, native, default):
`5627a51badf47762` / `8a9b58b38bef`; MP parity Coagulation (default cases)
and Backwash (motion) PASSED.

**Census** (one binary `e465b2...`, `HALO2_CENSUS_NATIVE_BRIDGES=1`,
translated callers against the default): Coagulation 4p 450 ticks
268,421,408 -> 266,529,845 (-1.89M); C4D30 25,173 entries -> 8, C48E0
1,595 -> 8, C2CB0 64 -> 8, BEC50 16,927 -> 13,153, C9070 11,200 -> 8,650,
BC8F0 4,903 -> 4,369. Armory 300 frames 79,890,873 -> 79,751,799 (-139k);
C4D30 948 -> 3.

**Remaining (group X):** 16D2A0's instanced geometry (16CF00, which the
effects' 17D625 needs) and objects (16D0E0) stay refusals; the scoped plan
is in placement.md ("Continuation: 16D2A0's instanced geometry and
objects"). BCB70's other translated sites (C0910's C09EC/C0A57; the
effects' 17C47C/17C497/17EA30, paused with group E) and 13E6E0's copies
outside C2B30.

## Progress: group X, instanced geometry and the remaining light callers (2026-11-16, continuation 2)

**Ported to the code standard** (placement; [placement.md](../native_object_factory_tracks/placement.md#instanced-geometry-16cf00-2026-11-16-continuation)):
16D2A0's cluster, instance and object visit loops (`query_clusters`, the
.bss visit blocks as `VisitStamps`), 16CF00 (`add_instance_features`),
16B810 (`instance_collides`, the pathfinding data's instance references),
144090 (`inverse`), 186E30 (`breakable_bits`) and the matrix paths of
24E710, 24E850 and 24EB60 (one small function per site, each keeping its
summation order). 16D740's instanced-geometry query (bit 2, the effects'
17D625) is native; nothing live reaches it while group E is paused.
16D0E0 (objects, bit 3) stays a refusal at the first unvisited object: no
16D740 caller sets bit 3.

**Bound, native by default:** BCB70 at C0910's C09EC and C0A57 (the node
lights' weights and scales) through the light callers' adapter, which now
returns BCB70's active byte in AL ([lights.md](../native_object_factory_tracks/lights.md#c0910s-bcb70-sites-2026-11-16-continuation)).
The native C0910 bridge's BCB70 child leaves through the required owner at
its site (`tools/native_object_subtree_replacements.py`,
`REQUIRED_CHILDREN`). C09EC ran 1,819 times in the sweep (Cyclotron and
Colossus 4p); C0A57 never ran.

**13E6E0's other copies:** not bound. The census (every MP map at 4
players, campaign 01a/03a/05a) enters 13E6E0 only at C2C03 (C2B30 through
the effects' C2CB0); the lens draw unit's copy (13E971) runs outside the
main-thread census, in the lens flare area of group E
([attachments.md](../native_object_factory_tracks/attachments.md#13e6e0s-other-copies-2026-11-16-continuation)).

**Proof** (strict writes, seed 0x5C41, the all-tracks harness, final
sources): the new kernel `collision_sphere_fit_instances` (1,500 cases,
entry 16D740, flags with bits 2 and 3) with every placement kernel: `PASS
17050 physical original evaluator tails, 34100 native comparisons, 661688
ordered original stores`; lights `PASS 1800 ..., 297687 ordered original
stores`. Coverage: 16CF00 120/120, 16B810 25/25, 144090 82/82, 24E710
83/83, 24E850 210/210, 24EB60 148/148, 16D740 627/627; 16D2A0 214/234 (a
dead NONE-leaf branch and the refused 16D0E0 call), 186E30 8/13 (its NONE
path, unreachable from 16CF00). ASan/UBSan: 6,000 cases over seven
kernels (367,490 stores) and the final instanced fixture (1,500 cases), no
report. Mutation screen out of tree (in-tree SHA-256 unchanged after each
round): 51 mutants, 45 rejected; four survivors argued equivalent, two
(an ulp of the instance-space centre) not distinguished.

**Live:** the light callers' and light update differential sweep (every MP
map 1p/4p with scripted input, campaign 01a..08b as far as they run): 56
runs, 1,211,181 light-caller calls compared (C09EC 1,819), 0 differed, 0
refused; light update 28,573 compared, 0 differed. Armory parity (default
and translated callers, binary `02dba867...`): `5627a51badf47762` /
`8a9b58b38bef`; MP parity Cyclotron (motion, 4:600 and 4:300) and
Coagulation (default cases) PASSED.

**Census:** 13E6E0 as above. The effects' 17D625 calls the translated
16D740 8..10 times per 600 ticks on Colossus and Cyclotron (4p) and never
enters 16D2A0 (the radius test returns first), so 16CF00 has no live
caller even through the translated effects. All light callers translated
against the default (one binary): Cyclotron 4p -5.41M translated
instructions, Colossus 4p -1.88M (every light caller; C0910's share is not
separable).

**Remaining (group X):** 16D0E0 (objects; only the AI's 1A676B reaches
16D2A0 with bit 3); the lens draw unit's 13E6E0 copy (13E971, lens flares,
group E's area, unmeasured by the census); BCB70 at the effects'
17C47C/17C497/17EA30 (paused with group E). The object subtree validator
(`tools/validate_native_object_subtree_tu15.py`) stops before its own
checks because C0F60's canonical body no longer matches its pin since the
light callers' C100F boundary was re-emitted (C0F60 runs translated); it
needs a new pin and route for C100F. (Fixed 2026-11-16: see "Object update owners:
rewrite to the code standard, and C0F60 restored".)

## Object update owners: rewrite to the code standard, and C0F60 restored (2026-11-16)

**C0F60 restored.** The light callers' re-emission of C0F60's C100F call as a
required boundary changed C0F60's canonical body, so the object subtree
binder's pin stopped matching and C0F60 ran translated. The binder
(`tools/native_object_subtree_replacements.py`) now pins the new body,
requires the C100F required-boundary route (`ROUTES['c0f60']`), and sends the
native bridge's C4D30 child through the World's required owner at C100F
(`REQUIRED_CHILDREN`), that is, through the light callers' binding (placement's
native `light_reconnect` by default). The harness's required-owner stand-in
accepts C4D30 at C100F; the validator also refuses C0F60 with a direct C4D30
call. Proof on the machine-shaped closures first (unchanged figures: 14,000
comparisons, 278,399 ordered stores, 38,590 children, 2,198 faults, census
3,500, verify 3,500, ASan/UBSan 2,800 + 500, 94/94 branches, 63/63 mutants).
Details: `BE850_object_subtree.md`, "C0F60's C100F boundary".

**Rewritten to the code standard** (the backlog's first two items): the 14
owners B8F40, B9090, B9180, BE4A0, 10DD10, BE850, BE610, C0F60, 10DD80,
C0200, 1D5790, 1E15D0, C0910 and 131470.

- Logic (game systems, no registers, flags, stack slots or coverage marks):
  `native_tu15_object_tick_systems.hpp` (`native_object_update`: the named
  layouts `DataArray`, `ObjectHeader`, `ObjectData`, `ObjectGlobals`,
  `PlayerRoots`, `GameGlobals`, `ObjectTypeDefinition`, `ClusterRecord`,
  `TimerBlock`, the header and update flags, the data_array search
  `next_in_use`, and the five passes), `native_tu15_object_subtree_systems.hpp`
  (`native_object_subtree`) and `native_tu15_cluster_distance_flood_systems.hpp`
  (`native_cluster_distance`: `StructureBsp`, `ClusterPortal`, `Cluster`,
  `ClusterDistance`, the frame's locals as addresses). Values the original
  keeps in callee-saved registers across a child call are named "kept"
  structs, handed back after each call, so non-conforming callees are followed
  exactly.
- Thin adapters in the original headers: role -> instruction tables, register
  effects, frames, child calls with the deciding instruction's flags,
  epilogues, and coverage as instruction-order edges. The shared boundary
  framework (`boundary::Port`, `boundary::Memory`) is in
  `native_tu15_object_tick.hpp`, next to the unchanged verification machinery.
  `Machine` stays only for the not-yet-rewritten group E/F closures that use it.
- The subtree's BE610 adapter recomputed the cluster record address, which left
  the logic's computation dead. The new mutation screen found this, and the
  adapters now take every derived value from the logic.

**Proof** (the unchanged harnesses; the mutation screens now name the file of
each defect and were ported to the new code, plus new defects for the
hand-back, the register mirror and the logic's access order; reports in
`build/test-results/object-update-legibility-20261116/`):

| Validator | Comparisons | Stores | Children | Faults | Census / verify | ASan/UBSan | Branches | Mutants |
|---|---|---|---|---|---|---|---|---|
| object tick | 10,000 | 808,046 | 175,202 | 1,088 | 2,500 / 2,500 | 2,000 + 400 | 55/59 (4 unreachable) | 39/39 |
| object subtree | 14,000 | 278,399 | 38,590 | 2,198 | 3,500 / 3,500 | 2,800 + 500 | 94/94 | 78/78 |
| cluster distance | 8,000 (+8,000 inline) | 845,366 | 75,875 | 2,108 | 2,000 / 2,000 (+inline) | 1,600 + 400 (both modes) | 17/17 | 30/30 |

The equivalence figures equal the machine-shaped closures': the reference
bodies decide them.

**Live** (shared build 08:18, binary pinned as `build-perf-4k60/halo2_native.ou`,
sha256 `88138e4f…`; the specialized live-support unit carries the 5 tick and
8 subtree bindings, C0F60 with the C100F required route; the environment unit
carries 131470):

- Armory 01a, 300 frames: default, reference
  (`HALO2_REFERENCE_OBJECT_TICK/_SUBTREE/_CLUSTER_DISTANCE=1`) and verify all
  give `5627a51badf47762` / `8a9b58b38bef`, with 0 validation messages.
- Armory verify (every call identical):
  - `Object tick verify: B8F40 450, B9090 450, B9180 450, BE4A0 11282, 10DD10
    11282 calls; 3059873 ordered accesses (358246 stores) and 90593 child
    boundaries identical`. These are the 2026-11-15 machine-shaped figures
    exactly.
  - `Object subtree verify: BE850 3874, BE610 3398, C0F60 4078, 10DD80 3874,
    C0200 4042, 1D5790 7934, 1E15D0 20318, C0910 11450 calls; 791337 ordered
    accesses (251355 stores) and 64752 child boundaries identical`. C0F60 is
    native again.
  - `Cluster distance verify: 300 calls; 978600 ordered accesses (235500
    stores) and 51300 child boundaries identical`.
- **Not run: the MP verify sweep and the census.** Claude Code stopped the
  census and both sweep roles (verify, native; 3 workers each, started with
  the census) at about 08:25 because the system ran critically low on memory,
  before any case finished. They were not restarted. The orphaned game
  instances were stopped and their scratch removed.

**Continuation (object update owners).**

1. Census on one binary (reference switches on, then off). Run it with
   nothing else on the machine:
   `python3 tools/run_test.py --scratch-root .local/test-scratch -- zsh build/test-results/object-update-legibility-20261116/scripts/census_ou.sh $PWD/build-perf-4k60/halo2_native.ou $PWD/build/test-results/object-update-legibility-20261116/census`.
   It keeps `compare.txt` (totals and the 14 owners plus C4D30) and `runs.txt`.
   Expect the 14 owners at 0 in the native role. C0F60's translated cost
   (639/frame on the 2026-11-15 ranking) returns to 0; it was counted
   translated on the light-callers binaries.
2. MP verify sweep, every MP map at 1p/4p with the scripted input, plus the
   campaign levels. Run one role at a time:
   `zsh build/test-results/object-update-legibility-20261116/scripts/ou_sweep.sh $PWD/build-perf-4k60/halo2_native.ou $PWD/build/test-results/object-update-legibility-20261116/sweep-verify --env HALO2_VERIFY_OBJECT_TICK=1 --env HALO2_VERIFY_OBJECT_SUBTREE=1 --env HALO2_VERIFY_CLUSTER_DISTANCE=1`,
   then the native role (no `--env`, into `sweep-native`), to compare the
   frames. Each case keeps its JSON (exit, verify lines, differences, frame
   hash). Known stops (not this work): needle and street_sweeper
   (active_camouflage), 03a/03b/05a (scene-copy renderer thread), 01b (1CD130
   trim), 08b (B9180 backing at 0x48C33C); ascension, elongation and lockout at
   4p are nondeterministic.
3. MP frame parity: `tools/perf_4k60/parity.py` on Coagulation (the reference
   switches against the default), as in group L's runs.
4. Delete `build-perf-4k60/halo2_native.ou` (345,856,432 bytes) after these
   runs.

## Object-update legibility: live results (coordinator, 2026-11-16)

These runs used binary `halo2_native.ou`, which has since been deleted. The
receipts are in `build/test-results/object-update-legibility-20261116/`.

**Census.** Armory, 300 frames, run once with all three reference switches and
once native:

- Total translated instructions: 94,019,909 → 79,562,984.
- All 14 owners go to 0, including C0F60 (4,078 entries / 190k instructions on
  the translated route). C0F60's native route is restored.

**MP verify sweep.** Every MP map at 1p and 4p, plus campaign:

- The verify role and the native role each ran all 56 cases: 0 refusals,
  0 differences.
- 47 cases exited 0. The other 9 are the known stops:
  - needle and street_sweeper, 1p and 4p;
  - campaign 01b, 03a, 03b, 05a and 08b.
- Native and verify frames are identical in 44 of 47 cases. The three that
  differ are the known nondeterministic ascension, elongation and lockout at
  4p.

This completes the object-update legibility item.

## Progress: group L, fourth batch (2026-11-16)

Details: section 6 of [LIVE_SUPPORT_LEAVES.md](LIVE_SUPPORT_LEAVES.md).

**Ported to the code standard and bound** (logic in
`native_tu15_live_support_systems.hpp`, adapters in
`native_tu15_live_support_leaves.hpp`, binder
`tools/native_live_support_leaves_replacements.py`), all through the
child-call boundary with genuine children:

- 1F8200, the ray against a posed collision model's BSPs (1,017/frame):
  children 144090 and 1E6980; the region iteration (1F7E90, 1F7F60) inline;
- 138D90, a texel's address in a bitmap's mip level (24): children 138510,
  1385D0, 1386E0 and 138C20; x87 FISTP pairs in both caller status models;
  bound in four units;
- 179CD0, the effect update loop (549): children 16EA60, 17A380 and group
  E's 17C370, 17C5C0 and 17C140, kept genuine (none of group E's owners was
  ported); bound in two units.

**Pool startup copies** (the last of the 16E450/16E4B0/32A4E0 residue): the
physics pool startup unit's 16D070/16D0D0/16D220/326970 report canonical
store PCs. The binder maps their store sites too, and binds them through
`run_with_store_sites`, which reports each store at the unit's canonical
PC. The physics world unit's `run_326970` binds as a plain canonical 32A4E0
copy.

Supporting changes (group L's files): `kernel_1f7f60` now has the
original's exact access order (the replay checks reads too);
`data_array::first_in_use` is shared by 1EC970 and 179CD0; the binder has a
per-unit `('call', site, name)` child route; the mutation screen also runs
verify mode.

**Proof** (`tools/validate_native_live_support_leaves_tu15.py`, CTest
`native_tu15_live_support_leaves`; 24 harness forms): 172,800 comparisons
(5,236,484 ordered stores, 14,996 faults; this batch's eight forms 57,600),
verify 26,525 (the leaves with children replayed in exact access order),
census 28,800, ASan/UBSan 43,200 + 1,110 verify, every reachable branch
direction (1F8200 16/16, 138D90 22/22, 179CD0 26/28 with 2 declared
unreachable), 215/215 mutants rejected (58 new, access-order mutants
included), source hashes rechecked. Receipt
`build/test-results/live-support-leaves-20261115/validator-L7.json`.

**Live** (`halo2_native.L7`, sha256 `68e3322f…`): Armory native = reference
= verify = `5627a51badf47762` / `8a9b58b38bef` (verify: 2,449 calls with
children, 362,161 ordered accesses, 15,701 child boundaries identical).
Coagulation MP parity reference = native = verify (`9c14a330…`,
`13fa114e…`, `05f31264…`). MP sweep: no verify difference; 43 cases with
identical native and verify frames, all identical to the L6 sweep; the 3
known nondeterministic 4p cases and the 4 known needle/street_sweeper
active-camouflage stops (receipt `mp-sweep-L7.json`).

**Census on one binary (L7)**: all translated 345,367 → 263,608 per frame;
group L's 42 tracked routines 28,259 → 1,646 (L6 native 3,238). This batch:
1F8200 1,016.8, 179CD0 549.0, 138D90 24.3 and the pool residue (3.1 in L6)
→ 0.

**Continuation (group L).** Remaining: 1D6FB0 (group H's), the BAD60/1E1200
report residues (44/frame), and 138D90's translated children 138C20/138510
(21/frame). Details: the continuation in section 6 of LIVE_SUPPORT_LEAVES.md.

## Progress: group E, rewrite to the code standard and bind (2026-11-16)

Details: [EFFECT_OWNERS_20680_C35D0_176A40.md](EFFECT_OWNERS_20680_C35D0_176A40.md).

**Rewritten and bound: 20680, C35D0 (with C3300 and C2A80 inline) and
176A40 (with 252100 inline).** About 6.5k translated instructions per frame
in the 2026-11-15 ranking.

- Logic: `src/rendering/effects/native_tu15_effect_owners_systems.hpp`
  (`native_effects`):
  - `lens_query_reset` (20680);
  - `light_fades`: `distance_fade` (C2A80), `fade_light` (C3300), `run`
    (C35D0);
  - `particle_inputs`: `input_value` (252100), `run` (176A40);
  - named layouts `LensQueryRecord`, `LightDatum`, `LightDefinition`,
    `TagInstance`, `Particle`, `Emitter`, `Location`, `EffectDatum`,
    `GameTime` and `Camera`;
  - the data_array search reused from the object update systems.
- Adapters: `native_tu15_effect_owners.hpp`, on `boundary::Port` and
  `boundary::Memory`. Only register effects that a store, a child or the
  exit can observe are mirrored. 176A40's kept registers use the object
  tick's keep/take pattern around 4EFA0. Group E no longer uses `Machine`;
  only group F's closures still do. The comment in
  `native_tu15_object_tick.hpp` that names groups E and F is left for the
  group F rewrite.
- Proof: the unchanged harness, with the mutation screen pointed at the two
  files (126 mutants: 73 logic, 53 adapter). Receipt
  `build/test-results/effect-owners-legibility-20261116/validator.json`.
  - 7,240 comparisons, 637,587 ordered stores, 4,670 children and 650
    faults; census 1,805, verify 1,785, reference switch 501.
  - ASan/UBSan 1,459 + 354 verify; 59/59 reachable branches; 126/126
    mutants rejected.
  - Every figure and coverage bitmap equals the machine-shaped closures'.
- Binding:
  - `BOUND = ('20680', 'c35d0', '176a40')`;
  - the binder is now registered in `spatial_owner_bindings` (it was not
    before);
  - CTest `native_tu15_effect_owners` is included from `CMakeLists.txt`;
  - shared build 13:22: one bridge in each of the five units.
- **Live checks blocked.** The launcher-selected MCC cache
  `assets/imported/mcc-82114d7d2e034d48ba762109935f9ca3` is gone (`assets/`
  changed at 10:15, trash emptied at 11:46), so the Armory parity and the
  MP sweep cannot run. No other cache matches the selection's pinned maps,
  and the newest one is refused by the build. Commands for the rerun are in
  the owner doc.
- **Shared tree reconfigure broken.** Since 09:56 `tests/texture_camera/`,
  with `tests/macos`, `tests/windows`, `tests/security`,
  `tests/online_protocol` and two `tests/shaders` probes, is deleted in the
  working tree. `cmake/TextureCameraNative.cmake` still adds
  `native_texture_camera_active_gpu_tests`, so every reconfigure of
  `build-perf-4k60` fails. For this build only, the three texture-camera
  files were restored from HEAD for the configure step, then deleted again
  (working tree as found). The next reconfigure fails the same way until the
  cmake file or the tests are fixed.

## Progress: group F, rewrite to the code standard (2026-11-16)

**Rewritten to the code standard and re-proven: 1306B0, D7230, 1EEE80,
41970, 133A70. Not bound** (`BOUND = ()` in
`tools/native_frame_misc_replacements.py`), on the coordinator's
instruction: the live checks cannot run because `assets/imported` (the
launcher-selected MCC map cache) is gone. Ready to bind, recorded in the
binder's comment:

    BOUND = ('1306b0', 'd7230', '1eee80', '41970', '133a70')

- Logic, as game systems with named layouts (no registers, flags, stack
  slots or coverage marks), in a new `*_systems.hpp` beside each bridge:
  - 1306B0, `native_tu15_geometry_cache_count_systems.hpp`
    (`native_geometry_cache::pending_reads`): the geometry reads still in
    flight. [4F2D5C] is the "xbox geometry cache" array (one datum per
    issued read; byte +2 is `ready`, set by the asynchronous read). The
    count runs only while the "xbox predicted geometry" array [4F2D60] is
    valid (DataArray +0x29). It uses the shared `next_in_use`/`datum_of`.
  - D7230, `native_tu15_attachment_update_systems.hpp`
    (`native_object_widgets::widget_render`): this is the object's
    **widgets**, not its attachments. They are the [4ECBF0] datums chained
    from object +0xDC; their types are rows of 4739F8. The type's +0x30 is
    a one-argument routine returning the shader in EAX. The render queue
    41480 then takes (mode, -1, 640.0, D7100, the type's +0x28/+0x2C
    callbacks, the widget data, object +0x30), which matches the objective
    presentation's 41480 contract. The type's +0x34 takes (mode, data).
  - 1EEE80, `native_tu15_engine_player_update_systems.hpp`
    (`native_multiplayer_engine::player_slots`): a local player's engine
    record (32 slots of state and timer, the current slot, seven countdown
    bytes). It runs the scan, the current slot's state machine (idle,
    starting, active, ending, cooling) and the award pass over slots 15..30.
    The two compiler case tables (1EF260; 1EF27C/1EF274) are read by the
    adapter.
  - 41970, `native_tu15_attachment_schedule_systems.hpp`
    (`native_object_attachments::marker_attachments`): lens flares, light
    volumes ('MGS2') and 'tdtl' attachments, placed at the object's markers
    (BAD60). It takes the scale from C0700, the attachment's primary scale
    function, and the color from BCD80, the change color. The 0x1CA0-byte
    stack frame is the logic's `Locals`.
  - 133A70, `native_tu15_environment_palette_systems.hpp`
    (`native_campaign_environment::cluster_sky`): a structure BSP cluster's
    sky. It takes the cluster's own sky (+0x6C) if that is in the scenario's
    sky list (+8). Otherwise it takes the visible sky (+0x6E), unless that
    sky's flag bit 4 is set. Otherwise it takes the current BSP's default
    sky (+0x210 list, +0x3C bit 0, +0x40). This is the rule that
    `bsp_atmosphere.cpp` already follows.
- Thin adapters stay in the original headers, so the binder's includes,
  bridges and switches are unchanged. They hold: role -> instruction tables;
  register mirrors and partial writes; the frames, pushes and kept walks;
  child calls with each deciding instruction's flags; exits. Coverage comes
  from instruction-order edges or the deciding compares. All of it is built
  on `boundary::Port`/`Memory`; group F no longer uses `Machine`.

**Proof.** The equivalence part of `tools/validate_native_frame_misc_tu15.py`
is unchanged. The mutation screen now names each defect's file: `header`,
`systems`, or the shared `native_tu15_object_tick_systems.hpp` (for the
data_array search). Every mutant was ported to the new code, so the counts
are unchanged. Every figure equals the machine-shaped baseline, run first on
the unchanged code:

| Owner | Comparisons | Stores | Children | Faults | Census / verify | ASan/UBSan | Branches | Mutants |
|---|---|---|---|---|---|---|---|---|
| 1306B0 | 8,000 | 29,440 | 0 | 724 | 2,000 / 2,000 | 1,600 + 400 | 6/8 (2 unreachable) | 22/22 |
| D7230 | 8,000 | 259,836 | 42,105 | 735 | 2,000 / 2,000 | 1,600 + 400 | 12/12 | 25/25 |
| 1EEE80 | 8,000 | 446,804 | 45,645 | 1,127 | 2,000 / 2,000 | 1,600 + 400 | 52/53 (1 unreachable) | 46/46 |
| 41970 | 8,000 | 12,364,516 | 91,758 | 1,169 | 2,000 / 2,000 | 1,600 + 400 | 24/24 | 39/41 |
| 133A70 | 8,000 | 28,803 | 0 | 287 | 2,000 / 2,000 | 1,600 + 400 | 15/15 | 25/26 |

Three mutants survive, and they already survived on the unchanged
machine-shaped code: 41970's `offset-partial` and `lens-byte`, and 133A70's
`primary-partial`. Each is an equivalent mutant, a partial-register write
whose upper bits are always overwritten or already zero. Because of them the
CTests `native_tu15_frame_misc_41970` and `_133a70` fail, as they did
before. Replace them with observable defects in a later pass.

Receipts: `build/test-results/frame-misc-legibility-20261116/`
(`baseline-*.json`, `final-*.json`, `baseline-sources.sha256`).

**Shared tree note.** From about 13:00 to 13:22, `BOUND` was briefly
filled, and builds of `build-perf-4k60` ran in that window. Their
specialized units (13:10 to 13:18) carry the five group F bridges, and so
does the `halo2_native` linked at 13:22 (sha256 `4bb86eb1…`; its verify
summary strings are present). My own queued build was interrupted before
linking. The binder is a `DEPENDS` of the specialization step, so the next
successful `build_perf.sh` re-specializes from `BOUND = ()` and unbinds
them. The bridges are equivalence-proven, but they are not live-checked.

**Continuation (group F).**

1. When the map cache is back, bind (the tuple above) and build.
2. Armory parity: native, reference (`HALO2_REFERENCE_GEOMETRY_CACHE_COUNT`,
   `_ATTACHMENT_UPDATE`, `_ENGINE_PLAYER_UPDATE`, `_ATTACHMENT_SCHEDULE`,
   `_ENVIRONMENT_PALETTE`) and verify (`HALO2_VERIFY_*` of the same names)
   should all give `5627a51badf47762` / `8a9b58b38bef`.
3. MP sweep with group L's `l_sweep.py` (every map at 1p and 4p): a verify
   role and a reference role, comparing frames.

## Groups E and F: live checks and binding (2026-11-16, evening)

The launcher selection's cache is back as
`assets/imported/mcc-7fde0eefc11d4530beed3cb109781361`. All the live checks
have passed, so both groups are **bound**:

- group E: `BOUND = ('20680', 'c35d0', '176a40')` in
  `tools/native_effect_owner_replacements.py`;
- group F: `BOUND = ('1306b0', 'd7230', '1eee80', '41970', '133a70')` in
  `tools/native_frame_misc_replacements.py`.

Group E's details are in
[EFFECT_OWNERS_20680_C35D0_176A40.md](EFFECT_OWNERS_20680_C35D0_176A40.md),
under "Live checks". This section is the record for group F (continuation
steps 1 to 3 above). Receipts and scripts are in
`build/test-results/effect-frame-owners-live-20261116/`:

- `armory.log`;
- `sweep/`, with one JSON per case and role;
- `sweep-summary.json`;
- `census/compare.txt`;
- `scripts/`.

**Build.** One `scratchpad/build_perf.sh` run (18:18 to 18:30) reconfigured
cleanly, so the texture-camera reconfigure failure is gone. It re-specialized
every unit. The bridges are installed as follows:

- group E: 20680 (lens flare), C35D0 (effect light view), and 176A40 (particle
  frame, combat action, death cycle);
- group F: 1306B0 (geometry cache frame), D7230 and 41970 (model render),
  1EEE80 (multiplayer engines) and 133A70 (campaign environment).

`halo2_native` sha256 is `4bb86eb18e4dbb45…`, byte-identical to the 13:22
build, which already carried both groups. Every run below used the pinned
copy `halo2_native.EF`. It was deleted afterwards.

**Armory parity** (01a, 300 frames, 1280x720, deterministic capture). Every
run gives frame `5627a51badf47762` and trace `8a9b58b38bef`:

- default;
- reference E (`HALO2_REFERENCE_EFFECT_OWNERS=1`);
- reference F (`HALO2_REFERENCE_GEOMETRY_CACHE_COUNT`, `_ATTACHMENT_UPDATE`,
  `_ENGINE_PLAYER_UPDATE`, `_ATTACHMENT_SCHEDULE`, `_ENVIRONMENT_PALETTE`);
- reference E and F;
- verify E and F (all six `HALO2_VERIFY_*`);
- `:validation`, which reports 0 messages.

The verify run reports:

| Verify summary | Calls | Ordered accesses (stores) | Child boundaries |
|---|---|---|---|
| Geometry cache count (1306B0) | 300 | 161,580 (1,200) | |
| Attachment update (D7230) | 18,890 | 132,230 (18,890) | 0 |
| Engine player update (1EEE80) | 450 | 167,407 (33,306) | 9,000 |
| Attachment schedule (41970) | 18,890 | 211,708 (78,565) | 18,890 |
| Environment palette (133A70) | 9,900 | 180,900 (48,300) | |
| Effect owners (20680 / C35D0 / 176A40) | 300 / 600 / 1,312 | 866,540 (307,866) | 870 |

**MP and campaign sweep.** This is group L's `l_sweep.py` with scripted fire,
grenade, jump, use, crouch, move and look over 600 ticks at 960x540. It
covers every MP map at 1p and 4p, plus 01a, 01b, 03a, 03b, 05a and 08b at
600 frames: 56 cases, 3 workers, one role at a time. The roles are verify
(both groups' six verify switches) and then native.

- **0 verify differences**, and no refusals or exceptions.
- 47 cases exit 0. The other 9 are the known stops, with the same errors as
  the object-update sweep:
  - needle and street_sweeper at 1p and 4p (active_camouflage);
  - 01b (1CD130 trim owner);
  - 03a, 03b and 05a (scene-copy renderer thread);
  - 08b (B9180 backing at 0x48C33C).
- Native and verify frames are identical in 44 of the 47 cases. The three
  that differ are the known nondeterministic ascension, elongation and
  lockout at 4p.
- The same 44 native frames also equal the object-update sweep's native
  frames, from the `.ou` binary.

Verify totals (verify role):

| Owner | Cases | Calls | Ordered accesses (stores) | Child boundaries |
|---|---|---|---|---|
| 1306B0 | 54 | 28,217 | 15,079,985 (112,868) | 0 |
| D7230 | 53 | 1,198,196 | 8,503,902 (1,236,065) | 6,677 |
| 1EEE80 | 54 | 70,066 | 26,256,177 (5,374,480) | 1,449,105 |
| 41970 | 53 | 1,198,196 | 43,876,129 (16,451,162) | 1,850,086 |
| 133A70 | 4 (campaign) | 19,815 | 362,043 (96,668) | 0 |
| 20680 / C35D0 / 176A40 | 54 | 28,217 / 70,230 / 3,638,726 | 249,305,234 (79,902,193) | 563,284 |

**Census on one binary** (Armory, 300 frames,
`HALO2_CENSUS_NATIVE_BRIDGES=1`). One run has every reference switch of both
groups on, the other has them off. Both runs give the parity hashes.

- All translated instructions: 79,082,471 → 74,881,346, which is **263,608 →
  249,604 per frame (−14,004)**. The translated figure equals group L's L7
  native census.
- Every bound routine goes to 0 entries and 0 instructions:

| Routine | Group | Translated entries | Instructions/frame |
|---|---|---|---|
| 20680 | E | 300 | 2,065 |
| C35D0 | E | 600 | 500 |
| C3300 (inline) | E | 2,400 | 536 |
| C2A80 (inline) | E | 12,000 | 1,628 |
| 176A40 | E | 1,312 | 1,059 |
| 252100 (inline) | E | 17,586 | 699 |
| 1306B0 | F | 300 | 2,124 |
| D7230 | F | 18,890 | 693 |
| 1EEE80 | F | 450 | 2,384 |
| 41970 | F | 18,890 | 1,182 |
| 133A70 | F | 9,900 | 1,136 |

- By group: E 6,486/frame and F 7,518/frame, 14,004/frame together. This
  equals the measured total drop within 0.4 instructions per frame.

**Not done here:**

- a reference-role MP sweep (the coordinator asked for verify then native);
- the Coagulation `parity.py` MP frame parity.

The validators and CTests were not rerun. Per the group F section above,
`native_tu15_frame_misc_41970` and `_133a70` fail on three equivalent
mutants, which still need observable replacements.

**Cleanup:**

- `halo2_native.EF` is deleted;
- the job left no new directory under `.local/test-scratch`;
- no processes remain;
- `gameplay-runtime-selection.json` is untouched.

The binder comments were updated with the binding, so the next
`build_perf.sh` re-specializes. The output is unchanged.

## Group H progress: 303C70/303D00 and 304350 (2026-11-16)

**Written, proven and bound: 303C70 and 303D00** (the local shape buffers:
construct and destroy an `hkLocalArray` of 256-byte shape buffers on the
thread memory's stack; their virtual calls are genuine dispatch children).

- Logic: `local_buffers::construct`/`destroy` in
  `native_tu15_world_leaves_systems.hpp`; adapters `kernel_303c70`/
  `kernel_303d00` in `native_tu15_world_leaves.hpp`.
- Proof `tools/validate_native_local_buffers_tu15.py` (CTest
  `native_tu15_local_buffers`): 40,000 comparisons, 1,186,399 ordered stores,
  3,732 faults, 27,046 dispatch children, census 10k, verify 10k, ASan/UBSan
  8k + 1k, 8/8 branches, 23/23 mutants.
- Binder `tools/native_local_buffers_replacements.py`.

**Written, proven and bound: 304350** (the welded collision agent of a shape
collection; 10.4k instructions/frame). 30BAD0 is inline (CALL, MOV EAX, RET),
as are 303C70, 303D00, 303EE0 and 2E47B0 (two sites). 2F36F0, 304130 (two
sites), 30CD00 and 30CF40 are genuine children through the unit's route, and
the seven virtual/function-pointer sites go through the unit's dispatch with
their canonical sites.

- Logic: `collection_agent::process_collision` and its phases; adapter
  `kernel_304350`/`CollectionAgentAbi`. Built one child call at a time
  against the harness, as the paused note asked: the register file at each
  call comes from per-role register mirroring (the loads and the register
  arithmetic between accesses), with EBX/ESI/EDI mirrored at every write.
- Proof `tools/validate_native_collection_agent_tu15.py` (CTest
  `native_tu15_collection_agent`): 20,000 comparisons, 24,991,033 ordered
  stores, 1,546 faults, 667,038 child calls, 54,659 gathered records, census
  5k, verify 5k, ASan/UBSan 4k + 1k, 19/19 branches, 35/35 mutants (screened
  in release and verify mode).
- Binder `tools/native_collection_agent_replacements.py`, registered after
  its children's binders.
- Shared verify changes (all group H owners): the register file carries the
  host's x87 exception flags, and the RDTSC service's results are recorded
  and served to the replay. The world leaves and contact array proofs were
  re-run with unchanged figures.
- Details: `1F5FE0_world_leaves.md`, "303C70 / 303D00 local shape buffers" and
  "304350 welded collection agent".

**Live (all eight bound, 20:16 build, run from a private copy).** Results in
`build/test-results/physics-world-leaves-20261115/live-304350/`.

- Armory verify: 304350 727 calls (every call), 1F5FE0 9,992, 2E4550 5,768,
  2E47B0 1,048; 4,241,333 ordered accesses (1,755,560 stores) and 65,188 child
  boundaries identical. 303C70, 303D00 and 303EE0 are only called from 304350
  in the Armory, so they are verified inline.
- Frame parity: verify, native and reference all give `5627a51badf47762` /
  `8a9b58b38bef`.
- Census on one binary (reference switch on, then off): 263,027 -> 238,971
  per frame (-24,056, the earlier leaves included). 304350 10,361, 303C70
  114, 303D00 94 and 30BAD0 64 go to 0 (-10,633 for this family);
  `physics_world_runtime_generated` 69,533 -> 45,477. The translated children
  are unchanged (2F36F0 1,592, 304130 2,291).
- MP verify sweep (the same 16 cases): 480,387 calls (304350 42,097),
  84.3M ordered accesses (36.9M stores) and 1,289,492 child boundaries
  identical, 0 refusals, 0 differences. The campaign 03a/05a exits are the
  pre-existing renderer-thread error, after identical verify lines.
- MP frame parity on coagulation with motion: PASSED.

**Next group H owners** (instructions/frame in the native census above):
30E930 4,750; 30CF40 2,958; 2E4FE0 2,648; 2F8630 2,367; 304130 2,291; E8D80
2,204; 1D9EC0 1,645; 2F36F0 1,592; 1F6D10 1,552; 1F76A0 1,551; 2F33B0 1,526;
1D7260 1,494; 1F7C10 1,162. 304130 and 30CF40 are now called from a native
parent: when they go native, `CollectionAgentAbi::weld_sorted/weld_point/
report_contact` can run them inline as the other children are, and the
reference's `replay_child` routes for them become their routed references.

## Group H progress: 30CF40, 304130, 2F33B0 and 1F76A0 (2026-11-16, night)

**Written, proven and bound: 30CF40 and 304130** (two of 304350's translated
children; bound at their own bodies, so 304350's native `call_child` reaches
them as any caller does).

- 30CF40 (2,958/frame): the manifold report. A convex child agent's cached
  contact points are checked against the bodies' current transforms
  (distance along the normal against the tolerance and the separation, drift
  across it); kept points go to the output, the others are removed through
  the contact manager. Children: 2E31E0 (array growth) and the contact
  manager's virtual removal.
- 304130 (2,291/frame): the contact weld. Records with state 1 are tested by
  303800 (through the dispatch); a rejected record is swapped with the last
  of the n (field by field, the floats through the x87 stack) and n shrinks.
- Logic `manifold_report::verify_and_report` and `contact_weld::weld` in
  `native_tu15_world_leaves_systems.hpp`; adapters `kernel_30cf40`/`ReportAbi`
  and `kernel_304130`/`WeldAbi` in `native_tu15_world_leaves.hpp`.
- The binder (`tools/native_manifold_replacements.py`) gives both translated
  routes and their references explicit-destination SSE (`ordered`, as for
  the world leaves): the first -O2 harness build showed the compiler commuting
  an `addps` on the reference side (a default NaN instead of the original's
  payload).
- Proof `tools/validate_native_manifold_tu15.py` (CTest
  `native_tu15_manifold`): 40,000 comparisons (20,000 per owner), 3,597,289
  ordered stores, 6,656 faults, 90,237 child calls; census 10,000, verify
  10,000; ASan/UBSan 8,000 + 2,000 verify; branches 8/8 and 5/5; 54/54
  mutants rejected.

**Live (both bound, 21:16 build, run from a private copy).** Results in
`build/test-results/physics-world-leaves-20261115/live-manifold/`.

- Armory verify: 30CF40 10,203 and 304130 3,772 calls identical, with 304350
  727, 1F5FE0 9,992, 2E4550 5,768, 2E47B0 1,048; 5,175,936 ordered accesses
  (2,014,576 stores) and 71,649 child boundaries identical. Live coverage:
  30CF40 room, no points, kept, far, separated; 304130 none, untested, kept,
  swapped.
- Frame parity: verify, native and reference all give `5627a51badf47762` /
  `8a9b58b38bef`.
- Census on one binary (reference switch on, then off): 263,035 -> 233,732
  per frame (-29,303 for all group H owners bound so far);
  `physics_world_runtime_generated` 69,533 -> 40,228. 30CF40 2,958 and 304130
  2,291 go to 0 (-5,249 for this pair; the previous native census was
  238,971).
- MP verify sweep (the same 16 cases, one worker): 750,511 calls (30CF40
  184,102, 304130 85,803), 95.8M ordered accesses (40.5M stores) and
  1,342,225 child boundaries identical, 0 refusals, 0 differences. mp
  ascension 2p exited 1 once after its verify line (`xvfb-run: kill: No such
  process`); alone, on the same binary, it exits 0 with the same counts, and
  so does its reference run. The campaign 03a/05a exits are the pre-existing
  renderer-thread error.
- MP frame parity on coagulation with motion: PASSED.
- Details: `1F5FE0_world_leaves.md`, "30CF40 manifold report and 304130
  contact weld".

**Written and proven: 2F33B0** (the key sort, 1,526/frame: Havok's
quickSortRecursive over unsigned keys, 2F36F0's child). Logic
`key_sort::sort`; adapter `kernel_2f33b0`/`SortAbi`, which runs the recursion
inline at its own boundary. Binder `tools/native_key_sort_replacements.py`
(its reference recurses into the bound body with the native route bypassed).
Proof `tools/validate_native_key_sort_tu15.py` (CTest `native_tu15_key_sort`):
20,000 comparisons, 587,125 ordered stores, 5,809 faults, census 5,000,
verify 5,000, ASan/UBSan 4,000 + 1,000, 5/5 branches, 17/17 mutants.
Registered in `spatial_owner_bindings` and the cmake helper list, and bound
in the 21:49 build.

**Live, 2F33B0 (21:49 build, private copy;
`build/test-results/physics-world-leaves-20261115/live-key-sort/`).**
Armory verify: 2F33B0 771 calls identical (all five branches), with the other
group H owners; frame parity `5627a51badf47762` / `8a9b58b38bef` for verify,
native and reference. Census on that binary: 257,338 -> 226,521 per frame,
`physics_world_runtime_generated` 69,499 -> 38,669, 2F33B0 1,526 -> 0. (The
reference total moved between the 21:16 and 21:49 binaries because another
session's natives landed in between.) MP verify sweep: 779,777 calls (2F33B0
29,528), 98.7M ordered accesses (41.5M stores), 1,342,225 child boundaries,
0 refusals, 0 differences, every MP case exit 0. MP frame parity: PASSED.

**Written, proven and bound: 1F76A0** (the shape key filter, 1,551/frame:
whether a key's child shape may still collide; four kinds through the
original's jump table, a 185AA0 child for instanced breakable surfaces). Logic
`shape_key_filter::may_collide`; adapter `kernel_1f76a0`/`FilterAbi` (the jump
table and the unit's `canonical_branch()` messages belong to the adapter).
Binder `tools/native_shape_key_filter_replacements.py`. Proof
`tools/validate_native_shape_key_filter_tu15.py` (CTest
`native_tu15_shape_key_filter`): 20,000 comparisons, 85,947 ordered stores,
5,306 faults, census 5,000, verify 5,000, ASan/UBSan 4,000 + 1,000, 6/6
branches, 20/20 mutants. The coverage word is now full (bit 63 is 1F76A0's
last branch): the next group H owner needs a second coverage word or its own
counter.


**Live, 1F76A0 (22:12 binary, private copy; `build/test-results/physics-world-leaves-20261115/live-shape-key-filter/`).** Armory verify:
1F76A0 21,371 calls identical (and every other group H owner); frame parity
`5627a51badf47762` / `8a9b58b38bef` for verify, native and reference. Census:
257,337 -> 224,958 per frame, `physics_world_runtime_generated` 69,499 ->
37,118, 1F76A0 1,551 -> 0. MP verify sweep: 1,217,136 calls (1F76A0
437,237), 102,786,007 ordered accesses, 1,342,225 child
boundaries, 0 refusals, 0 differences; non-zero exits: sp:03a_oldmombasa:600 1, sp:05a_deltaapproach:600 1. MP
frame parity: PASSED.

**Not done in this pass, with reasons.**

- 30E930 (4,750/frame) was not started. Its translated body is 1,023 lines
  (148 KB) with five fixed children (30D3F0, 30D5D0, 30D830, 312620 already
  native; 30DE60 not), two virtual feature and three virtual support sites,
  and the RSQRTSS service; the in-tree pieces in
  `include/halo2/native_tu15_simplex_*_kernel.hpp` are private raw seams, not
  code-standard logic. At the 304350 rate this is more than one session on
  its own; it needs its own pass.
- 2E4FE0 and 2F8630 (2,648 and 2,367): read, not started. 2F8630 (203 lines)
  has two profiler timer events (RDTSC), 2E4550 (native, can run inline),
  30E930, 30D350 and 2E4760 as children, and an x87 tolerance test; 2E4FE0
  calls it through four dispatch sites.
- 304350 still enters 30CF40 and 304130 through `call_child`; they could now
  run inline in `CollectionAgentAbi::weld_sorted/weld_point/report_contact`
  (and the reference's `replay_child` routes would become their routed
  references). Not needed for the census: the bound bodies already admit.

**Census of this pass** (Armory, native, from the live runs): 30CF40 2,958,
304130 2,291, 2F33B0 1,526 and 1F76A0 1,551 go to 0 (-8,326 per frame);
`physics_world_runtime_generated` is 37,118 per frame on the 22:12 binary
(69,499 with the reference switch; 40,228 on the 21:16 binary before this
pass's last two owners, 45,477 before this pass).

**Group H continuation.**

1. Next owners in the native census: 30E930 4,750 (its own pass; see above),
   2E4FE0 2,648 and 2F8630 2,367 (2F8630 first: its children are 2E4550
   (native, inline), 30E930, 30D350, 2E4760 and two profiler timer events),
   E8D80 2,204, 1D9EC0 1,645, 2F36F0 1,592, 1F6D10 1,552 and 1F7C10 1,162
   (both call 1F76A0, which is now native), 1D7260 1,494, 315C90 722.
2. Coverage: the shared 64-bit coverage word is full; widen it (a second
   word, with the validators' offsets) before the next owner.
3. Whenever 304350's adapter is next touched: run 30CF40 and 304130 inline
   in `CollectionAgentAbi` (they are bound at their own bodies now).

## Collision ray 16C620: native default (2026-11-16, night)

16C620 is now native by default. The MP verify sweep reports 0 differences
over 437,635 calls (47 of 56 cases complete; the 9 known stops are
unrelated).

The x87 C1 mismatch was a bug in the translated **reference** route, not the
native code. The live-support copy of 16C620 and the death-cycle copy of
30AA70 are older emissions that lose C1 after `FSTP ST(0)` and after some
`FSTP m32` stores. The AI-frame copy and the hardware agree with the native.

The fix patches two generated files directly:

- `death_cycle_runtime_tu15_generated.cpp`: the four 30AA70 `FSTP ST(0)` sites;
- `physics_live_support_tu15_generated.cpp`: five m32 stores.

**Caveat:** regenerating those units reverts the fix. When that happens,
`tools/native_collision_ray_replacements.py` refuses its pin, so 16C620 falls
back to translated. That is safe, but the native route is lost until the fix
is re-applied. The generators should be fixed to emit the C1-publishing
store and pop. Details are in `16C620_collision_ray.md`.

## Group H progress: coverage word widened (2026-11-16, late)

**Done: the shared branch-coverage word is now two 64-bit words.** The
single word was full (1F76A0's last branch was bit 63).

- `native_tu15_world_leaves.hpp`:
  - `coverage_words = 2` and `coverage_bits = 128`;
  - `coverage(word = 0)` returns word 0 or word 1;
  - `Port::hit` is a word array, and `reached(b)` sets bit `b%64` of word
    `b/64`;
  - `execute()` merges both words, on a fault too;
  - the branch-enum `static_assert`s check against `coverage_bits`.
- Every existing owner keeps its bit in word 0. Their tests (which print
  `coverage().load()`) and validators (`coverage >> (offset + index)`) are
  unchanged.
- The live `Physics world leaves verify` line prints coverage as one hex
  number, word 1 first, without leading zeros. It reads as before while word 1
  is 0.
- The next owner's enum starts at `filter_branch_end` (64), which is word 1.
  Its test should print `coverage(1)`, and its validator should read
  `coverage | coverage_high << 64`.

**Re-validated: all seven group H validators.** Reports are in
`build/test-results/physics-world-leaves-20261115/coverage-widen/`. Each
validator was run at the case count of its earlier report. Release, census,
verify, sanitized, sanitized-verify, coverage and branch figures are identical
to the earlier reports, and every mutant is rejected:

| Validator | Comparisons | Ordered stores | Mutants |
|---|---|---|---|
| world leaves | 40,000 | 805,844 | 34/34 |
| contact array | 40,000 | 1,075,898 | 13/13 |
| local buffers | 40,000 | 1,186,399 | 23/23 |
| collection agent | 20,000 | 24,991,033 | 35/35 |
| manifold | 40,000 | 3,597,289 | 54/54 |
| key sort | 20,000 | 587,125 | 17/17 |
| shape key filter | 20,000 | 85,947 | 20/20 |

No shared build was run: the header change is compile-checked by all seven
harnesses. The next shared build picks it up. No new owner is bound.

**Stopped.** A safety classifier stopped this agent while it was reading the
30E930 translated body to plan the port. Nothing of 30E930 was written, and
no other file changed. Notes for whoever resumes:

- 30E930's children, as the canonical body (`run_30ae40`) routes them:
  - the two feature virtual+34 calls, through
    `handoff_native_contact_features`;
  - 30D830 `run_309d40`;
  - 312620 and 30D3F0, through `handoff_native_simplex_helpers`;
  - 30D5D0 `run_309ae0`, twice;
  - 30DE60 `run_30a370`;
  - three support virtual+30 calls through `dispatch`.
- The flags at each child call come from a short tail: SUB ESP before the
  first call, CMP EDX,2 before 30D830, CMP EAX,18 or the last SHL/INC before
  the reductions, ADD reg,ESI before each support call, and TEST AL,AL before
  30DE60. The exit flags come from the last copy-back loop's CMP.
- XMM5-7 at exit are left over from earlier blocks or children. The adapter
  must therefore publish each block's XMM writes at the block's own exit
  points.

The remaining group H order is unchanged: 30E930, 2E4FE0, 2F8630, 2F36F0,
1F6D10, 1F7C10, E8D80, then 1D9EC0/315C90.

## Group H progress: 2F8630, 2E4FE0 and 1F7C10 (2026-11-16, late night)

**Written, proven and bound: 2F8630 and 2E4FE0** (Havok's GSK agent: the
closest point between two convex shapes with its separating-plane cache, and
the game's swept-shape wrapper around it). 2E4550 and 2E4760 run inline at
their own boundaries; 30E930 and 30D350 (under 2F8630) and 283380, 309060 and
2F8630 (under 2E4FE0) are genuine children through the unit's route; the
shapes' getType() and 309150 go through the unit's dispatch. Both use the
second coverage word (bits 64-88).

- Logic `closest_point::find`, `swept_pair::closest_point` and
  `havok_rotation::rotate` (2E4760), with the shared `convex_pair` layouts,
  a generic `monitor::timer_event` and five more x87 operations; adapters
  `kernel_2f8630`/`ClosestPointAbi`, `kernel_2e4fe0`/`SweptPairAbi`,
  `kernel_2e4760`, `X87Mirror` and `genuine_child`.
- Binder `tools/native_convex_pair_replacements.py` (marker `run_2f4bd0`); it
  also gives 2E4760's translated body explicit-destination SSE, because the
  verification reference runs it.
- Proof `tools/validate_native_convex_pair_tu15.py` (CTest
  `native_tu15_convex_pair`): 40,000 comparisons (20,000 per owner),
  3,739,264 ordered stores, 5,972 faults, 115,514 child calls; census 10,000,
  verify 10,000; ASan/UBSan 8,000 + 2,000 verify; branches 10/10 and 15/15;
  65/65 mutants rejected.

**Written, proven and bound: 1F7C10** (the structure collision shape's
getChildShape). 1F76A0 and 1F5FE0 are entered through the unit's route and
run their own bound native bodies; 283450 is a genuine child; 2E88B0, 184E20,
the destructor, 1F67C0 and 1F7330 go through the dispatch.

- Logic `bsp_child_shape::child_shape`; adapter `kernel_1f7c10`/`ChildShapeAbi`;
  binder `tools/native_bsp_child_shape_replacements.py` (marker `run_1f3320`).
- Proof `tools/validate_native_bsp_child_shape_tu15.py` (CTest
  `native_tu15_bsp_child_shape`): 20,000 comparisons, 817,494 ordered stores,
  1,422 faults, 35,900 child calls; census 5,000, verify 5,000; ASan/UBSan
  4,000 + 1,000 verify; 11/11 branches; 36/36 mutants rejected.

**Shared-header recheck.** The seven earlier group H validators pass against
the changed headers (4,000 cases each, release, census and verify; their
mutation anchors are unchanged and unique): world leaves, contact array,
local buffers, collection agent, manifold (8,000), key sort, shape key filter.

**Live, 2F8630/2E4FE0 (23:35 binary, private copy;
`build/test-results/physics-world-leaves-20261115/live-convex-pair/`).**

- Armory verify: 2F8630 9,571 and 2E4FE0 9,571 calls identical, with every
  other group H owner (1F76A0 21,371, 30CF40 10,203, 1F5FE0 9,992, ...);
  6,579,711 ordered accesses (2,473,274 stores) and 104,332 child boundaries
  identical. Live coverage: 2F8630 plane not trusted, apart, closed, contact,
  cached, stream full; 2E4FE0 body A's swept shape slow, body B not swept,
  cache kept (the Armory never sweeps).
- Frame parity: verify, native and reference all give `5627a51badf47762` /
  `8a9b58b38bef`.
- Census on one binary (reference switch on, then off): 257,336 -> 219,782
  per frame; `physics_world_runtime_generated` 69,499 -> 31,945; 2E4FE0
  2,648 and 2F8630 2,367 go to 0, 2E4760 216 -> 57 (its other caller is
  translated). Against the previous native census (224,958): -5,176.

**Live, all three (23:51 binary, private copy;
`build/test-results/physics-world-leaves-20261115/live-bsp-child-shape/`).**

- Armory verify: 1F7C10 10,051 calls identical, with 2F8630 9,571, 2E4FE0
  9,571 and every other group H owner; 6,760,393 ordered accesses (2,572,270
  stores) and 124,434 child boundaries identical. Live 1F7C10 paths:
  structure surface, instance surface (kind 5) and other kinds (the filter
  accepts every Armory key).
- Frame parity: verify, native and reference all give `5627a51badf47762` /
  `8a9b58b38bef`.
- Census on one binary (reference switch on, then off): 251,137 -> 212,420
  per frame; `physics_world_runtime_generated` 69,499 -> 30,783; 1F7C10 1,162
  goes to 0 (with 2F8630 and 2E4FE0). The reference total moved between the
  23:35 and 23:51 binaries because other sessions' natives landed in between.
- MP verify sweep (the same 16 cases, one worker): 1,745,433 verified calls
  (2F8630 182,017, 2E4FE0 182,017, 1F7C10 226,140), 127,024,389 ordered
  accesses (52,036,830 stores) and 2,465,787 child boundaries identical,
  0 refusals, 0 differences; every MP case exits 0. The campaign 03a/05a
  exits are the pre-existing "Scene-copy resources belong to another renderer
  thread", after identical verify lines. Live coverage adds 2F8630's plane
  kept out of the cache by the agent and by the settings; 2E4FE0's swept body
  A (with a parent chain), changed flags, the reset after a sweep and the
  309060 cache set-up; 1F7C10's instance path. Not reached live (the harness
  covers them): 30E930 reporting apart, a written timer event (the stream has
  no room), body B swept, the predicate refusing, the type-5 cache,
  1F7C10's rejected keys, missing buffers and released references.
- MP frame parity on coagulation with motion (3840x2160; 4:90, 4:150 firing,
  1:150 firing): reference, its repeat and the native candidate are
  byte-identical, `PASSED` (`mp-parity-coagulation.json`).

**Group H continuation.**

1. Next owners in the native census: 30E930 4,750 (paused; its own pass),
   E8D80 2,204, 1D9EC0 1,645 and 315C90 722 (phantom units: no binding
   mechanism yet), 2F36F0 1,592, 1F6D10 1,552, 1D7260 1,494.
2. 1F6D10 (canonical `run_1f2420`, 174 instructions): read, not started. It
   expands a body's shape keys: per key of kind 2 (bits 29-31 =
   0x40000000 after AND E0000000) whose definition's +98 count is at most
   0x2000, the instance's 4x3 transform is copied into a local hkTransform
   (twelve MOVSS through the frame, zeros and [465030] in w), 2E4550 makes it
   relative to body A, and the definition's MOPP is queried through vtable
   +2C (the keys appended into the array at ESI); the expanded keys are then
   filtered through 1F76A0 (OR 0xFFFFA000, SHL 10, OR the key), removing the
   rejected ones by shifting the array down. Children: 2E5BC0, 1F26B0
   (both translated, at the start), 2E4550 (inline), the MOPP query (dispatch
   site 1F262E) and 1F76A0 (bound; through the route). Its frame reuses
   [ESP+10]/[ESP+14]/[ESP+18]/[ESP+1C] for several roles.
3. 2F36F0 (575 instructions) needs its own pass; its child 2F33B0 is native.
4. 2E4760 has one more translated caller (57/frame); binding it at its own
   body needs only a bridge and the world leaves harness's fixture.

## Device update callbacks 1179C0/10B840: native and bound (2026-11-17)

Details: [1179C0_10B840_device_update.md](1179C0_10B840_device_update.md).
These are the two `source_scene_device_constructor` owners of the "Paused"
row of the claims table; the rest of that unit stays paused.

**Written to the code standard and bound.** Logic
`native_tu15_device_update_systems.hpp` (`machine_update::run` 1179C0,
`device_update::run` 10B840, on the named layouts `Device`,
`DeviceDefinition`, `DeviceGroup`, `GameTime`, `AnimationChannel`, `Unit`,
`UnitDefinition`, `StructureBsp`/`MachinePortals`/`PortalOpenness`); adapter
`native_tu15_device_update.hpp` (registers, LAHF, frames, XMM file, the x87
stack's C1 and its FST/FISTP frame spills); binder
`tools/native_device_update_replacements.py`, registered in
`spatial_owner_bindings` and the specialization helper list. Children stay
genuine: B7E50, BBFF0, BF050 (1179C0) and B8760 (twice), BF050 (10B840)
through the unit's dispatch, route unchanged (no body in the unit, so the
required owner); 1DA750, 1CE8D0, 1CEBD0, 10C5D0, BD080 (native), BAD60,
10C290, 1181F0, BDA50, 118350, 10CD70, 10CA30, 121890 as the unit's direct
bodies. All `selected()` operands of both bodies are the identity (the
binder checks the unit's resolver). Switches `HALO2_REFERENCE_DEVICE_UPDATE`
and `HALO2_VERIFY_DEVICE_UPDATE`.

**Proof** (`tools/validate_native_device_update_tu15.py`, CTest
`native_tu15_device_update` in `cmake/NativeTu15ObjectTick.cmake`; receipt
`build/test-results/device-update-20261116/validator.json`): 12,000
comparisons against the pinned translated bodies (6,000 per owner; 304,278
ordered stores, 30,079 genuine children, 2,608 faults), census 3,000, verify
3,000 (record/replay), ASan/UBSan 2,400 + 500 verify, 189/189 branch
directions (103 in 1179C0, 86 in 10B840; none declared unreachable), 142/142
mutants rejected (84 logic, 58 adapter; default then verify mode), source
hashes rechecked. Binding checks: install, idempotence, refusal of a changed
body, a missing BD080 body, a changed BF050 route, a B8760 body in the unit
and a non-identity operand. The adapters take every derived value (record
addresses, cursors, stored floats, pushed arguments, the result) from the
logic; the first screen's 11 survivors (rare one-sided door paths, two
equality-only mutants) led to stronger fixtures and sharper mutants.

**Live** (shared build 2026-11-17 00:06, binary pinned as
`build-perf-4k60/halo2_native.du`, sha256 `84a330d258bc7174…`; the
specialized scene device unit carries both prefixes and the references):

- Armory 01a, 300 frames: default, reference and verify all give
  `5627a51badf47762` / `8a9b58b38bef`, 0 validation messages. Verify:
  `Device update verify: 1179C0 9228, 10B840 9231 calls; 899711 ordered
  accesses (231653 stores) and 11721 child boundaries identical`.
- Census (Armory, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`, one binary,
  reference switch on then off, one role at a time): total translated
  65,585,857 → 63,725,141. 1179C0 9,228 entries / 986,298 instructions → 0;
  10B840 9,231 / 874,594 → 0 (about 6,200 instructions per frame). The
  children's counts are identical in both roles (10C5D0 45/900, 1CE8D0
  127/3,556, 1CEBD0 37/1,332, 121890 44/2,034, 10CA30 45/696, 10C290 1/1,305,
  1181F0 86/14,147, 118350 9,185/95,462). Receipt `census/compare.txt`.
- MP sweep (`scripts/du_sweep.sh`: every MP map at 1 and 4 players, 600
  ticks of scripted fire, grenade, jump, use, crouch and movement; verify role
  then native role, two workers). Verify role: 0 differences in all 50 cases.
  22 cases on the 11 maps with devices replayed identically: 42,314 1179C0
  and 42,328 10B840 calls, 4,944,088 ordered accesses (1,439,731 stores) and
  157,125 child boundaries (Zanzibar 3,045/3,047 calls per case, Gemini 4p
  4,802, Backwash 4p 3,000, Ascension, Colossus, Triplicate, Dune, Foundation,
  Cyclotron, Waterworks, Containment; Containment and Dune have controls or
  light fixtures, so 10B840 runs more often). The vehicle maps (Coagulation,
  Headlong, Burial Mounds, Containment, Waterworks, Zanzibar, ...) are in the
  sweep; maps without devices print no device line. 46 cases exit 0; the 4
  stops are the known needle and street_sweeper active_camouflage shader stops
  at 1p and 4p. Native and verify frames are identical in 43 of 46 cases; the
  three that differ are the known nondeterministic ascension, elongation and
  lockout at 4p. Zanzibar 1p's frame equals the 2026-11-16 object-update
  sweep's (`55ba476c22bd60c4`). Receipts `sweep-verify/`, `sweep-native/`,
  `sweep-compare.txt`.
- The pinned binary was deleted after these runs.

**Continuation.** None required for these two owners. The census shows 118350
(9,185 entries, 95,462 instructions per 300 Armory frames, 1179C0's unmoved
child) as the next translated owner in this path; the rest of the scene device
unit (1181F0, 10C5D0, 1CE8D0, 1CEBD0, 121890, 10CA30, 10C290) is small per
frame and stays paused with the unit.

## Group H progress: 1F6D10 written and proven, not yet bound (2026-11-17)

**Written: 1F6D10** (the structure collision shape's queryAabb, canonical
`run_1f2420`). The 2026-11-16 note named its first two children by their
canonical names; their TU1.5 entries are 2E9670 (canonical 2E5BC0, the
structure's own MOPP query) and 1F6FA0 (canonical 1F26B0, the key filter
loop).

- Logic: `structure_aabb_query::query_aabb`, `expand_instance`, `build_box`
  and `remove_key` in `native_tu15_world_leaves_systems.hpp`.
- Adapter: `kernel_1f6d10`/`AabbQueryAbi` and `bridge_1f6d10` in
  `native_tu15_world_leaves.hpp`.
  - 2E9670, 1F6FA0 and 1F76A0 are entered through the unit's route; 1F76A0
    runs its own bound body.
  - 2E4550 runs inline.
  - The definition's queryObb (vtable +2C) goes through the dispatch, at
    canonical site 1F262E.
  - Coverage: the `AabbQueryBranch` bits 100-108, in word 1.
  - The verify summary gains a 1F6D10 entry, as `calls[15]`.
- Binder: `tools/native_structure_aabb_query_replacements.py` (marker
  `run_1f2420`, receipt entry `1f6d10`). **Not yet registered** in
  `spatial_owner_bindings` (`tools/specialize_native_operands.py`) or
  `cmake/NativeOperandSpecialization.cmake`, so the shared build does not
  bind it yet.
- Proof: `tools/validate_native_structure_aabb_query_tu15.py` with
  `tests/native_tu15_structure_aabb_query_tests.cpp`. There is no CTest entry
  yet; add one to `cmake/NativeTu15WorldLeaves.cmake`.
  - Results at 20,000 cases: release 20,000 comparisons, about 4.48M
    ordered stores, about 3.1k faults and 122k child calls; census and verify
    5,000 each; ASan/UBSan 4,000 + 1,000 verify; branches 9/9.
  - Mutants: 41 of 42 are rejected. `query-removal-bound` survives.

**Mutant screen, first-run notes.** Eight first-run mutants survived. Each was
then replaced, or a fixture was added for it:

- Equivalent, replaced:
  - the late matrix-field read PCs, which faults cannot distinguish (now a
    swap of the first two reads, with a straddling-page fixture, kind 14);
  - the EAX mask, after the matrix PCs were taken from the definition-index
    address;
  - the shifted EDX, which a later write always replaces;
  - one anchor that was not unique.
- Covered by new fixtures:
  - a negative size, which the 1F76A0 stand-in sometimes writes;
  - a wrapped collision pointer, so that `ADD ECX,50` carries;
  - a hits header placed inside its own key data, kind 13.

**Next steps:**

1. Make `query-removal-bound` observable, or document it as equivalent and
   replace it. In the kind-13 fixture the shift has to overwrite the size
   field before the bound is re-read.
2. Register the binder and the CTest entry.
3. Build through `scratchpad/build_perf.sh`.
4. Run the live checks: Armory verify and parity, MP sweep, census.

The rest of the order is unchanged: 2F36F0, E8D80, then 1D9EC0/315C90 with
a new phantom binding hook. 30E930 stays paused.

## Group H progress: 1F6D10 bound and live-verified (2026-11-17)

Details: [1F5FE0_world_leaves.md](1F5FE0_world_leaves.md), "1F6D10 structure
AABB query".

**Done: 1F6D10.**

- **Last survivor resolved.** `query-removal-bound` is not equivalent; it
  needed a case where a key moved down overwrites the size the removal
  re-reads. The kind-13 fixture now also places the hits' header one word
  below its key data: the size is key 0 (an instance key) and the capacity
  key 1. Removing key 0 moves the capacity into the size. The changed random
  draws then hid `query-half-order`, so kind 12 also gives the half factor
  and the AABB NaN payloads.
- **Proof** (`native-tu15-structure-aabb-query.json`):
  - 20,000 comparisons, 3,958,312 ordered stores, 4,446 faults and 107,883
    child calls;
  - census 5,000 and verify 5,000;
  - ASan/UBSan 4,000 + 1,000 verify;
  - branches 9/9;
  - 43/43 mutants rejected (41 in release mode, 2 in verify mode).
- **Bound.** The binder is registered in `spatial_owner_bindings` (marker
  `run_1f2420`) and in the cmake helper list. CTest
  `native_tu15_structure_aabb_query` is in `cmake/NativeTu15WorldLeaves.cmake`.

**Live** (shared build 2026-11-17 00:59, private copy
`build-perf-4k60/halo2_native.h8`, sha256 `7660814c6fed9c53…`; results in
`build/test-results/physics-world-leaves-20261115/live-structure-aabb-query/`):

- **Armory verify:** 1F6D10 727 calls identical, with every group H owner;
  7,015,928 ordered accesses (2,679,311 stores) and 130,290 child boundaries
  identical.
- **Frame parity:** verify, native and reference all give `5627a51badf47762`
  / `8a9b58b38bef`.
- **Census**, one binary, reference switch on then off: 251,133 -> 210,865
  per frame. `physics_world_runtime_generated` goes 69,499 -> 29,231, and
  1F6D10 goes 1,552 -> 0.
- **MP verify sweep** (16 cases): 1,786,401 verified calls (1F6D10 48,710),
  129,617,499 ordered accesses (53,106,552 stores) and 2,585,439 child
  boundaries identical. There were 0 refusals and 0 differences, and every MP
  case exits 0. The campaign 03a/05a cases exit with the pre-existing
  renderer-thread error, after identical verify lines.
- **MP frame parity** (coagulation, motion, 3840x2160): `PASSED`.
