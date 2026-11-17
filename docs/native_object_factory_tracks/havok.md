# Track `havok` (Havok owners the factory tracks refuse)

**Status (2026-11-14):** the Havok allocator, array growth,
body activation (315CB0), component activation (1D9F10), the component
reset (1D8470), the reservation predicate (283450), the AABB phantom leaves
(31C3B0, 3154E0), the hk3AxisSweep broadphase add/remove (2EC4B0/2EC9B0),
hkWorld add/remove phantom (318550/318680), the phantom wake (1CE340) and
the standalone 1CD6A0 component-velocity owner are native. New: the
component's world membership 1D9510 (removal) and 1D97F0 (insertion), with
hkWorld removeEntity 3198A0 and addEntity 319370, and the 1D73D0 shape
refresh dispatch. They bind the existing `src/physics` owners where those
exist and are proven by strict-write physical-original oracle cases.
1CC310 itself still refuses at 1D9510 (active rows) and 1D73D0 (after the
native 1D8470 reset and the flag store); 1D97F0 is bound. The units track
proves its composed 1CC310 cases up to those two stop entries (its fixture
has no entity world), so binding `havok_component_remove` there needs that
stop list updated first; 1D73D0's shape builders (1DEE30, 1DD9A0, 1DDBF0,
1DD950, 1DEF50) are named refusals (see the continuation plan). Placement
binds 1D9510 (282C10) and 1D97F0 (BA8C0) natively.

Files: `src/gameplay/objects/native_object_factory_havok_tu15.cpp`,
`include/halo2/native_object_factory_havok_tu15.hpp`,
`tests/native_object_factory_havok_kernels.cpp`,
`tools/native_object_factory_cases_havok.py`.

## Native owners

| Owner | API | Binds | Refuses (named) |
|---|---|---|---|
| [0x48C938] +10 149DA0 | `havok_array_allocate`, `havok_allocator().allocate` | `native_tu15_route_array_allocation`, pool +10 233250 `native_tu15_pool_allocate` | a non-pool heap method, the lazy default heap 149930 |
| [0x48C938] +14 149E00 | `havok_array_release`, `havok_allocator().release` | `native_tu15_route_array_release`, pool +14 233420 `native_tu15_pool_release` | a non-pool heap method, 149930, a release above 0x2000 bytes (import free) |
| 2E3250 (+2E2E30) | `havok_grow_array` | `native_tu15_grow_array` over `NativeTu15ArrayGrowthMemory`, with the allocator above | (the allocator's) |
| 315CB0 (+3165D0, 316D40, 322660) | `havok_activate_body` | `native_tu15_activate_physics_body`; growth through `havok_grow_array` | 322660 world listeners other than 2E4D60 (for example 282870), entity listeners other than 2E4D60 (for example 2816A0), the 3165D0 body destructor virtual |
| 1D9F10 | `havok_component_activate` | 315CB0 per body (+70 rows of 0x60, body +40, count +74 reread) | (315CB0's) |
| 1CD6A0 (+1D9EC0, 1D8EB0) | `havok_object_component_velocities` | complete force/no-force component velocity delivery, the existing native 1D9080/1D9100 setters, 315C90/315CB0 and 1D9F10; six literal motion-kind getter owners | unknown motion-kind/setter virtuals and 315CB0's unported listeners/destructors |
| 283450 | `native_tu15_reserved_body_reference` (kernel `havok_reserved_body`) | as is | — |
| 31C3B0 (+31FCD0, 31FCA0) | `havok_aabb_phantom_info` | ported | misaligned MOVAPS (an original fault) |
| 3154E0 | `havok_aabb_phantom_construct` | ported | misaligned MOVAPS |
| 1CE340 | `havok_phantom_wake` | 31C3B0, the object's AABB (SSE, live MXCSR), [0x48C938] +10 (0x90, class 0x2C), 3154E0, 318550, 315CB0 per overlapping awake entity, 318680, the last reference (283450 or the 315620 chain) | (its owners') |
| 2EC4B0 (+2EAFB0, 2E9CF0, 2EBA20, 2EB850, 2E31E0) | `havok_broadphase_add_object` | ported: quantization (packed SSE under the live MXCSR), node append (`havok_grow_array`), endpoint insertion (reserve `native_tu15_reserve_array`), the MMX position repair, the marker-seeded x sweep bitset in the arena, pair/marker appends | misaligned MOVAPS (AABB, broadphase, bitset) |
| 2EC9B0 (+2E9FD0, 2E9DF0) | `havok_broadphase_remove_object` | ported: the same sweep, removed pairs and marker-list removal, endpoint removal, position repair, last-node move with its axis/handle/marker references, the shrink reserve | misaligned bitset |
| 318550 (+3177E0, 315170, 322280, 317870) | `havok_world_add_phantom` | 2EC4B0, 37CBF0 `native_tu15_normalize_pairs`, 37C960 `native_tu15_merge_pairs` with filter 2E9A00 `native_tu15_activation_group_filter`, create 31FFC0 `native_tu15_create_phantom_pair` → 315300 `native_tu15_add_simple_phantom_overlap` | phantom +18 other than 315170, broadphase +4 other than 2EC4B0 (both before any store), other filters/handlers, overlap owners other than 315300, overlap-added (314F80) and world phantom listeners other than 2E4D60 |
| 318680 (+3222F0, 315620 chain: 315570, 31FAA0, 315400, B4150) | `havok_world_remove_phantom` | 2EC9B0, 37CBF0, 37C960 with delete 320040 `native_tu15_delete_phantom_pair` → 315100 `native_tu15_remove_simple_phantom_overlap` or 2E4D60, 283450 | broadphase +C other than 2EC9B0 (before any store), overlap owners other than 315100/2E4D60, removed/deletion listeners other than 2E4D60, phantom destructors other than 315620, a phantom shape whose last reference drops (315400), a flagged allocation (B4150 → 2E12A0) |
| arena ([0x48C938] +8/+C/+14; +24 2E10A0, +28 2E1110) | (internal) `arena_array_construct/destroy` | fast path ported; slow paths `native_tu15_allocate_stack_arena` / `native_tu15_release_stack_arena` with `havok_allocator()` | +24/+28 owners other than 2E10A0/2E1110 |
| 1D8470 (+31C450, 31C5E0, 283450) | `havok_component_reset` | `native_tu15_reset_physics_component` (now in `halo2_native_pair_consumer`), with `havok_allocator()` for property growth and record releases | body destructors other than 2E4D60 |
| 3198A0 (+3181E0, 322210, 31FDE0, 318360, 320150, 317B70/317C40/317CD0/317D20 collector) | `havok_world_remove_entity` | deferral (+12C locked: +F0 list), 2EC9B0, removed pairs through 321510 `native_tu15_consume_pairs` or the 0x41D9A0 collector and 37C960 `native_tu15_merge_pairs`; entity pairs through 320260 `native_tu15_delete_pair`, phantom pairs through 320040; 322100 `native_tu15_destroy_physics_island`, 3174A0 `native_tu15_compact_body_references` | pending world operations (+24 > 0: 319750), contact agents/managers of a deleted entity pair, entity destructors, listeners other than 2E4D60 |
| 319370 (+316950, 320DE0, 318020, 3221A0) | `havok_world_add_entity` | 3212C0 `native_tu15_construct_physics_island`, shape bounds 2E8D90/2E8120/306760/307E30 natives, x87 margin, 2EC4B0, new pairs through 321510 or the collector, filter 2E9A00, 320EF0 `native_tu15_create_pair` with 316D40 `native_tu15_activate_physics_island` and 320DA0 deferral, phantom pairs 31FFC0 | other shape bounds owners, 317200 body merges and contact manager/agent creation (an admitted, undeferred entity pair) |
| 1D9510 (+315D60, 315DA0, 282C10) | `havok_component_remove` | 3198A0, 282C10 `native_tu15_process_dirty_island_objects` (1CC310/1D9510 bound here), [0x48C938] +14, 318680 | constraints (+80: 316AA0), +94 attached-object wake (BBBC0), model constraints (22D0A0), 282C10's 1496F0/1D7CA0/16E8A0/BA570/BA8C0 |
| 1D97F0 (+315E50, 315E90, 1CC560, 315CE0/316C90, 1D9AD0, 31E820/31E760) | `havok_component_add` | 318550, 319370, 1D9080/1D9100 `native_tu15_set_phantom_body_velocity`, [0x48C938] +10 (0x70, class 0x26), shape bounds, 1D9C80 `native_tu15_destroy_gameplay_body_policy`, 283450 | the entity limit (1CDD60), a non-unit model scale (31E430/31E560/31E590), constraints (+80: 317350), other record destructors |
| 1D73D0 | `havok_component_refresh` | (dispatch: map 0x1D74BC, table 0x1D74A4); 1CD6A0 `havok_object_component_velocities` after a builder | every shape builder at its entry: 1DEE30, 1DD9A0, 1DDBF0 (after +4 \|= 0x80), 1DD950, 1DEF50; types without a builder |
| 1CC310 | `havok_component_state_flags` | `native_tu15_set_component_state_flags`: `havok_component_reset`, the flag store, 1D97F0 (`havok_component_add`) | 1D9510 (active row, before any store) and 1D73D0 (after the reset and flag store) at their entries: units' stop list |

The 2E4D60 default listener (`RET 4`) is handled inside the physics native.

The memory adapter `HavokMemory` forwards to the world memory with two
exceptions. Every owner here runs through it, and nested owners reuse the
outer adapter (`with_havok`), so their private frames never shadow each
other.

- Counter updates go to the world's `NativeTu15RetainedCounterStorage` when
  it has one. Otherwise they take one read, then one store of the adjusted
  value, like the original's `DEC [mem]`.
- [0xFFFFE000, 2^32) is private: the original's stack locals, never
  forwarded. The top 16 bytes (0xFFFFFFE0) hold 1D8470's two 31C450
  results; below them owners push zeroed, 16-byte aligned frames: 1CE340's
  phantom info, 318550/318680's pair headers and AABB, the sweep bitset
  header, 37C960's argument slots and the overlap event packets. No private
  address is ever stored into the world.

## Proof

Oracle: `tools/validate_native_campaign_script_builtins_tu15.py` with
`HALO2_ORACLE_STRICT_WRITES=1 HALO2_FACTORY_TRACKS=havok`.

- Havok kernels, all ten: 3,660 cases per seed, with 69k–76k ordered
  stores (seeds 23617, 4660, 40503, 777, 31337). The per-kernel counts are: allocate 500,
  release 400, grow 400, activate body 600, component activate 500,
  283450 300, 31C3B0 100, 3154E0 200 and 1CC310 60.
- 1D8470 (`havok_component_reset`, 600 cases) together with 1CC310 (60):
  660 cases, 28,898 ordered stores. The fixtures cover:
  - +7C records, +70 rows, +98 and +9C;
  - references that reach zero, through the 2E4D60 destructor;
  - property removal and re-add, with growth through the allocator;
  - record releases into the P2 pool.

  Coverage misses only 1D8590/1D8595 and 31C450's not-found path. The
  former is the inlined 283450 branch for bodies in [0x80000000, 0x80061000),
  which cannot be placed in a bank. The latter is unreachable, because
  1D8470 searches for the key first.
- Full run (base builtins + havok, strict writes, after the 1D8470
  binding): **PASS 30600 physical original evaluator tails, 61200 native
  comparisons, 252422 ordered original stores.**
- With all tracks loaded, units `units_place` (closure reaches 1D97F0's
  indirect call) and weapon `weapon_new`/`weapon_set_state` (10DF30) fail
  closure authentication. This also happens without the havok track. Those
  owners relied on modelstate's former global stops.
- Mutation screen (20 mutations of the track source): 19 are rejected. These
  cover:
  - 1D9F10's flag bit, stride, body offset and signed count;
  - pool size and release;
  - the release limit;
  - the allocator method slot;
  - the counter adapter;
  - both growth strides;
  - 1CC310;
  - 31C3B0's +14, +8 width and AABB order;
  - 3154E0's +88, AABB read order, +28 and refcount.

  The survivor drops the allocation class. It is equivalent: 233250 never
  reads its class argument. A second screen hit the new binding: the
  adapter's locals forwarded to the world, the reset without the allocator,
  and the counter adapter storing the old value. All three were rejected.
  The source was verified against pre-run copies after both campaigns.

### Broadphase, hkWorld phantoms and 1CE340 (second pass)

Kernels (strict writes): `havok_broadphase_add` (2EC4B0, 600 per seed),
`havok_broadphase_remove` (2EC9B0, 600), `havok_add_phantom` (318550,
500), `havok_remove_phantom` (318680, 500), `havok_phantom_wake` (1CE340,
500). All fifteen havok kernels together: **6,360 cases per seed, 520,787 to
524,750 ordered original stores** (seeds 23617, 4660, 40503, 777, 31337; all
PASS). The ten first-pass kernels account for 69k-76k of those stores.
Full run, base builtins + these fifteen kernels, strict writes (default
seed): **PASS 33300 physical original evaluator tails, 66600 native
comparisons, 708877 ordered original stores.**

- Coverage (two seeds, every instruction of each static body): 2EC4B0,
  2EC9B0, 2EAFB0, 2E9CF0, 2E9DF0, 2E9FD0, 2EBA20, 2E31E0, 2E10A0, 2E1110,
  3177E0, 317870, 315170, 37CBF0, 37CB20, 314F80, 314FE0, 315100, 322280,
  3222F0, 315620, 315570, 31FAA0, 2E9A00, 315CB0, 316D40, 322660, 3154E0,
  31C3B0, 2E3250 and 2E2E30 are complete. Not reached, all unreachable from
  these callers or refused:
  - 1CE340's free of its phantom-info array and 2EB850's free of an owned
    bitset (both headers are private and never grown);
  - 318550/318680's null collidable (a phantom at 0xFFFFFFF4) and 318550's
    null filter (37C960 would call through null);
  - 37C960's interior merge (needs both ranges; only 321510 reaches it);
  - 31FFC0/320040's null pair members, 315300's search (31FFC0 always
    clears the flag byte);
  - 315400's shape release at zero (refused; its reserved branch, like
    283450's, needs an address at 0x80000000+), B4150's flagged
    allocation (refused at 2E12A0), 3165D0's destructor (as before).
- Mutation screen (41 mutations of the track source, out of tree: a scratch
  copy compiled with the kept build's flags and linked into a scratch
  binary; goldens computed once): 40 rejected. They cover 2EAFB0's loop
  bound, position, store order and reserve doubling; 2E9FD0's shift and
  count order; 2E9CF0's thresholds, x-lane mask and signed compares (a
  detached node whose position words have bit 15 set); quantization clamp
  order and live MXCSR; the minimum/maximum bit forms; the bitset size (a
  32..44-node variant) and zeroing; the marker seed's clear; the x sweep's
  minimum-only test; the overlap mask; pair word order; the marker-list
  not-found index; the moved node's y/z references; the shrink reserve; the
  arena fast-path bound and release boundary; 318550's id store order and
  reference; the filter's +8; normalization; listener compaction; 318680's
  store order and not-found index; the destructor's vtable, release size
  and overlap stride; 1CE340's radius sign, live MXCSR, fixed-owner and
  entity-type tests, size word and last-reference release. The survivor
  (the moved node's marker test `>` against `>=`) is equivalent: it compares
  two different edges' axis positions, which are never equal. The in-tree
  source hash was verified unchanged after both screens.

Fixtures:

- Pools P (0x420000) and P2 (0x430000) live in the image `.rdata` range as
  `world.extra` banks: the header (+0..+0x340), page 0 (+0x4340, 0x2000
  bytes) and the counters (+0xF0340).
- The allocator, vtables, world, islands, bodies, listeners, deferred list
  and component rows are in TRACK.
- The fixtures cover modes 0 and 1. The fixed heap is P or P2. The secondary
  range (P2 +18/+1C) covers TRACK.
- Each pool path is covered: free-list hits, small-region carving, a new
  page, an exhausted primary (the secondary then serves), and the fixed tail
  with its `>0x2000` null.
- Broadphase and phantoms (TRACK): BP +0x2400 (vtable +0x2480: +4 2EC4B0,
  +C 2EC9B0), nodes +0x2500, axes +0x2600/+0x2680/+0x2700, marker cells
  +0x2780 with lists +0x2800, the kernel pair header/data/AABB
  +0x2900/+0x2910/+0x2980, handles +0x2A00, four hkAabbPhantoms +0x2B00
  (0xA0 apart; image vtable 0x41D87C, or a copy at +0x2D80 whose +20 is
  2E4D60), arena blocks +0x3000, the 2E10A0 backings BIG +0x3800 and BIG2
  +0x6800 (P class 14: one per add and remove), the dispatcher +0x4900
  (phantom handler for type pairs {1,2}), filter +0x4A40 (vtable 2E9A00),
  world phantom listeners/array +0x4B40/+0x4B80, overlap arrays +0x4C00,
  phantom listener arrays +0x4D00, and the large variant's nodes/axes/
  handles/pairs at +0x5400..+0x6200. The world is `physics_world`'s
  (T+0x100) plus +54/+C4/+D0/+120/+130/+138/+234; entities are its bodies
  (handle at body +1C, +40 fixed byte). Extra banks: the image's 0x41D87C
  vtable and 0x41C680 quantization constants; globals [0x52BE48], [0x48B874]
  and [0x46A11C].
- The SAP state is consistent: node 0 is the sentinel (axis ends), marker
  nodes span one x bucket each (their lists hold the objects overlapping
  them, sometimes perturbed), object endpoints are distinct, ties follow
  insertion order. Capacities sometimes force node growth (2E3250), axis
  reserves (2E31E0, including a capacity at most half the count), a short
  node capacity on removal, pair/marker-list growth, and small arena
  remainders (the 2E10A0/2E1110 slow paths).

Closure declarations: REGISTER_JUMPS for every indirect transfer the cases
execute (2E3250 → 149DA0/149E00, the 149DA0/149E00 heap methods → 233250 or
233420, and 322660's listener slots → 2E4D60). Refused targets are declared
with no admitted target. 149930 is a global closure stop: it is reachable only
through the allocator virtuals, and its closure has unbounded virtuals.

Coverage (reachable instructions): 315CB0, 316D40, 322660, 2E3250, 2E2E30,
2E4D60, 1D9F10, 283450, 31C3B0, 31FCD0, 31FCA0 and 3154E0 are complete. The
only instructions not reached are paths the natives refuse:

- 3165D0: the refcount-zero branch (283450, then the destructor virtual, or
  the reserved reset that needs a body at 0x80061000 or above) and the
  unreachable not-found index.
- 149DA0/149E00: the 149930 default-heap path.
- 233420: the release above 0x2000 bytes (import free).
- 1CC310: everything past the component check.

### 1CD6A0 component velocity delivery (2026-11-13)

`havok_object_component_velocities` takes retained linear/angular vector
addresses, not snapshots. It resolves the salted component handle, honors
1D9EC0 when `force` is false, selects the signed root-body slot for vehicle
headers (including the original five-bit type shift), and otherwise visits
every row with a live signed count. Fixed bodies and motion kind 6 are
skipped. Linear/angular publication uses the existing
`native_tu15_set_phantom_body_velocity` owner, preserving three input reads,
aligned 16-byte stores with a zero fourth lane, the static-component angular
gate, and real native island activation. Forced delivery also performs
1D9F10 when component flag bit 5 is set.

This is the reusable 1D73D0 velocity tail. The 1D73D0 shape constructors are
still refused, so this increment does not change live object-birth routing
or qualify a complete B9B70 birth.

The new direct-entry kernel has 1,600 cases per seed. Fixtures include no
component, arbitrary datum salts, negative/empty counts, invalid signed root
slots, all six motion-kind getters, fixed and detached bodies, active and
inactive islands, null and aliased vector inputs, raw NaN/denormal words,
the low-byte force convention, and activation listeners/allocation already
admitted by this track.

Seed 23617 passes **1,600 physical-original cases, 3,200 native comparisons
and 7,171 identical ordered stores**. It executes all 114 instructions of
1CD6A0, all 39 of 1D9EC0, all 12 of 1D8EB0, both setters (41/41 and 47/47),
315C90, all six motion getters and both final MOVAPS owners. No original
instructions or outcomes are supplied.
Seed 4660 independently passes another **1,600 cases, 3,200 comparisons and
7,961 ordered stores** (3,200 cases / 6,400 comparisons / 15,132 stores in
the two focused seeds).
The final regression also passes **8,140 original cases, 16,280 native
comparisons and 541,631 ordered stores**: all 16 Havok-track kernels (7,960
cases) plus the three existing Havok-record initialization kernels (180).

The out-of-tree mutation screen rejects nine of ten changes: force gating,
datum salt masking, motion-kind rejection, vehicle-type selection, root-slot
upper bound, linear/angular dispatch, and final activation gating. Removing
the outer fixed-body precheck survives on the admitted getter set because
both downstream setters independently reject fixed bodies. This equivalence
is scoped to these fixtures; it does not qualify invoking an unknown getter
on a fixed body. The production precheck remains intact.

Receipts, source hashes and mutation recipes are in
`build/test-results/resume-havok-velocity-20261113/`. `qualification.json`
preserves the driver's strict all-mutants-rejected failure;
`reviewed-acceptance.json` records the accepted fixture-scoped equivalence
and passing original/regression results. The Release CMake build and native
harness self-test pass. Native sources and fixtures stayed unchanged during
qualification. `cleanup.json` verifies that the owned builds, reference
executables, banks, native binaries, mutant objects and one-off coordinator
were removed. Only small reports remain; no large artifacts are retained.

### Entity membership: 1D9510, 1D97F0, 3198A0, 319370 and 1D73D0 (2026-11-14)

Census (Armory 01a, 300 frames): 1D9510 ran 67 times and 1D97F0 176 times
(from 1CC310 and BA8C0); every census-executed instruction of both, of
3198A0/319370 and their callees is native, except the 319750 drain (16 of
151 removals: refused) and what 282C10 does when an island is dirty.

New kernels (strict writes; the last argument is the harness SP, the
original entry ESP):

| Kernel | Entry | Cases/seed |
|---|---|---|
| `havok_remove_entity` | 3198A0 (ECX world; result, entity) | 300 |
| `havok_add_entity` | 319370 (ECX world; entity) | 300 |
| `havok_component_remove` | 1D9510 (ECX row) | 300 |
| `havok_component_add` | 1D97F0 (ECX row) | 300 |
| `havok_component_refresh` | 1D73D0 (EAX datum), stops at the builder entries | 200 |
| `havok_component_state_flags` | 1CC310, now with components; stops at 1D9510 or 1D73D0 | 400 |

Results (strict writes, no supplied original outcomes):
- The four entity kernels (1,200 cases/seed) plus 1D73D0/1CC310 (600):
  seed 23617 PASS 1,800 cases, 368,154 ordered stores (1CC310 then still
  stopping at the builders); seed 4660 PASS 1,800 cases, 350,845 stores.
  After 1CC310's refusals moved back to 1D9510's and 1D73D0's entries
  (units' stop list): 1CC310 + 1D73D0 PASS 600 cases with 3,227 (23617)
  and 3,106 (4660) stores.
- All 21 havok kernels, seed 23617: PASS 9,700 cases, 19,400 native
  comparisons, 904,569 ordered stores (with 1CC310 then stopping at the
  builders; the later 1CC310 change touches only that kernel, whose
  fixture and random draws are unchanged).
- Composed tracks on the final harness (seed 23617, each track's own
  kernels): placement PASS 14,050 / 652,105 stores, objtypes PASS 10,400 /
  123,320, modelstate PASS 9,700 / 162,835, units PASS 13,000 / 329,595
  (all unchanged from their previous receipts).
- Mutation screen (out of tree, scratch copies compiled with the kept
  build's flags and linked into scratch harnesses; in-tree source SHA-256
  verified unchanged): 34 mutations, all rejected. They cover the 315D60
  not-found index, the +60 listener offset, the [0x48B868] count, 322210's
  array, 320150's moved index and memory sign, 318360's list choice,
  island destruction and +118 clear, the collector's duplicate test, lists
  and +24 restore, both collector stack identities, the fixed-body test,
  the deferred reference, 319370's fixed id and lock restore (rejected
  after the fixture varied the saved lock byte), the island +24 index, the
  x87 margin and transform offset, 315CE0's gate, the +64 property key,
  31E760's addition order and half factor, the record vtable order, +5C,
  the old record release, the +4 bit 3 clear, the angular input, 1D73D0's
  machine flag and type map, 1CC310's ESP model and the stack aliasing.

Fixture (`entity_world`, on top of `phantom_world`): up to six entities in
the broadphase, usually clustered so that they overlap, with consistent
SAP marker lists (no detached node, no perturbed members: several bodies
are removed in sequence); full 0xB0 islands at TRACK +6200 (0xC0 apart,
releasable by 322100) on the world's active/inactive lists, plus the fixed
island (+2C); sorted pair records (+68, no agents) and deferred pairs (+74)
of entity pairs in one of their islands; motions (vtables with 323420,
the six kind getters and 3239F0/323A00), sphere and capsule shapes, the
+60/+70/+7C/+88/+94 arrays, properties, the world's entity listeners, +CC
tolerance, +F0/+108/+114 lists, +12C lock, +23C. A `spread` mode (six
entities, one fixed, each other in its own island) makes a fixed body
collect five islands. Add cases admit entity pairs only when every
admitted pair must defer (a new island, +30 clear elsewhere), otherwise
the filter's single group system rejects them. P's page bank covers three
pages (arena slow paths can take two).

Oracle declarations: `ENTITY_JUMPS` (motion getters, island +10 320130 and
destructor 322100, shape bounds 2E8D90/2E8120, broadphase +4/+C, allocator
and arena slots, 2E4D60 listeners, 1D9C80, 37C960's 320EF0/320260 and
317B70/317C40 owners; refused agent/manager and entity-destructor sites
admit nothing), `JUMP_TABLES[0x1D74A4] = 5`, and per-routine closure stops
for every refused callee (316AA0, 317350, 22D0A0, BBBC0, 1CDD60, 319750,
31E430/31E560/31E590, 317200, 2E12A0, 282C10's 1496F0/1D7CA0/16E8A0/BA570/
BA8C0 and the five builders). With every track loaded all 400 kernels
authenticate (64,143 sites).

**The stack identity.** 3181E0 and 318020 publish the address of their
local pair collector (vtable 0x41D9A0) at the dispatcher's +24 slot for the
merge, then restore +100. The native keeps the collector in a private frame
aliased at the original's address, computed from the entry ESP passed as
`sp` (1CC310 -0x10 -> 1D9510 -0x28 -> 3198A0 -0x10 -> 3181E0, collector
-0x30; 1D97F0 aligned -0x3C -> 1CC560 -0xC -> 319370 -0x14 -> 318020,
aligned -0x34). With `sp = 0` (production, and the placement binding) the
collector keeps its private address.

Coverage (seeds 23617 and 4660, every instruction of each static body):
complete for 315D60, 315DA0, 315E50, 315E90, 315CE0, 316C90, 322210,
3221A0, 31FDE0, 320150, 320DE0, 316950, 319370, 1D9AD0, 31E820, 31E760,
317CD0, 317D20, 322100, 3174A0, 3173C0, 317430, 316D40, 320DA0, 2E8D90 and
2E8120; 317B70/317C40 (including the collector's growth past its four
inline islands) miss only their null-handle branches. Not reached, each
refused or unreachable:
- 1D9510: +80 constraints (316AA0), +94 attached objects, 22D0A0;
- 1D97F0: +80 constraints (317350), the scale path, a new record whose
  count reaches zero (it is 1 after the install);
- 3198A0/318360: 319750, references reaching zero (283450's reserved
  range lies at 0x80000000+, entity destructors are refused);
- 3181E0/318020: the island +94/+88 reserve (the masked capacity is never
  negative); 318020's null filter (37C960 would call through it);
- 1CC560: the entity limit (1CDD60);
- 320EF0: null pair handles and everything from contact manager creation
  on (refused); 320260 (231/247): agent destruction (refused) and three
  null-handle branches; 321510: owned record reallocation (needs created
  records);
- 1D9C80: the flagged allocation (2E12A0); 3213A0: the +3C, +68 and +74
  releases (those arrays stay in the FACTORY bank, not releasable there);
- 1D73D0: everything after the builder calls (37 of 69 executed); 1CC310:
  the 1D97F0 call after its 1D73D0 stop. (Measured while 1CC310 still ran
  1D9510 natively; 1D9510's coverage comes from its own kernel.)

Receipts (small reports, commands, coverage, mutation results, source
hashes): `build/test-results/havok-entity-20261114/`.

## Oracle support added by this track (case module)

- **SEH frames.** The observer had no FS segment, so any original with an
  exception registration (`mov fs:[0], esp`) faulted at its prologue.
  `install_fs_tib()` (once, on the validator module itself) gives the
  reference a TIB: the bridge calls `set_thread_area` with a segment based
  at STACK + 0x40 and loads FS; FS-segment operands are authenticated at
  that base. fs:[0] is a declared stack word, and its stores are not
  compared, like every stack store.
- **MOVQ m64, mm stores.** Capstone reports this destination as a read (the
  validator already corrects x87 and MOVHPS/MOVLPS stores the same way), so
  these stores went unlogged. Only the four broadphase sites
  (2E9DCC/2E9DD6/2E9ECC/2E9ED6), which only this track executes, are
  corrected. The validator owner should add `movq` with a memory destination
  to its store set.
- With every track loaded, all 385 kernels authenticate (370 without this
  track): the declarations below break no other owner.

## Blockers and findings

1. **1CC310 completes only with 1D73D0's shape builders.** 1D9510,
   1D8470, the flag store and 1D97F0 are native, and so is the 1D73D0
   dispatch (standalone): it reaches 1DEE30 (unit), 1DD9A0 (vehicle),
   1DDBF0 (machine), 1DD950 (crate) or 1DEF50 (creature), all shape
   creation (types without a builder jump through the table's word 5, not
   an owner). Until a builder is native, 1CC310 refuses at 1D73D0's entry
   (and at 1D9510 for an active row); the oracle proves 1CC310 up to those
   stop entries, as the units kernels do (`units_physics_mode` stops at
   0x1D9510 and 0x1D73D0). Binding either earlier breaks that stop list.
2. **The allocator was not a blocker.** It is native already
   (`native_tu15_route_array_*`, `native_tu15_pool_*`), and
   `NativeTu15IslandPartitionAllocatorBinding` composes it the same way. The
   pool vtable is 0x4638B8 (+10 233250, +14 233420); the allocator vtable is
   0x45F4CC (+10 149DA0, +14 149E00).
3. **1CE340 is native (was blocked by the broadphase).** 318550/318680
   and the hk3AxisSweep add/remove are semantic natives in this track. The
   register-level replica `src/physics/runtime/native_tu15_broadphase_add_object.hpp`
   was used only as a reading aid; nothing binds it.
4. **Oracle conflict with modelstate (resolved).** modelstate's closure
   stops, 0x1CC310 included, are now per-routine, so the 1CC310 kernel
   registers in full runs. Its probe was removed: a global 0x1CC310 stop
   would now fail the run instead of silently skipping the kernel.
5. **Global closure declarations made by this track.** These are
   REGISTER_JUMPS at 2E327B, 2E32B0, 149DBC, 149DCD, 149DEF, 149DDF, 149E32,
   149E7A, 23347D, 32267F and 3226F2, plus the stop at 149930, and (second
   pass, merged with any other track's targets) `PHANTOM_JUMPS`: 2E31E0's
   allocator slots, the arena slow paths (2EBA6E/2EB89D/2E10C5/2E114A,
   31782E/3178BD), 318550/318680's +18/+4/+C/destructor slots, 37C960's
   filter/create/delete slots (2E9A00/31FFC0/320040), 31FFC0/320040's
   overlap slots (315300; 315100 or 2E4D60), the 2E4D60 listener slots of
   322280/3222F0/314F80/314FE0/31FAA0, the 315620 chain's frees and 1CE340's
   allocator/destructor slots; 315459 (a phantom shape destructor) admits
   nothing. 2E12A0 is a per-routine stop. Other owners
   whose closure contains 2E3250 or 322660 now also admit the
   allocator/pool/2E4D60 bodies, which is a relaxation. With all tracks
   loaded, the metadata stays well-formed: 337 owners, 34,061 sites.
6. **Fixture gotchas.**
   - 2E3250 copies into whatever 149DA0 returns, with no null check. In
     mode 1 the fixed heap must therefore not be exhausted.
   - P2 +18/+1C serve two roles: the small carving window for allocation
     and 149E00's secondary release range. A P2 page carve would move the
     range. So outside the direct allocate kernel, P serves every
     allocation (pages 0, fixed heap = P), and P2 only takes releases.
   - A block that has been handed out gets overwritten (its first word
     becomes array data). Each pool class therefore owns a chain of
     distinct 0x100-byte blocks (P: T+0xA000, P2: T+0xC000), and released
     data lives in a separate area (T+0x7000).

## Hook shapes for other tracks

All of these are wired (integration, 2026-11-12): objtypes' 1D9F10 and
1CC3F0, the services track's `phantom_wake` binding, modelstate's 1CE0E0 and
placement's `component_set_flags` (1CC310 from B92C0/B9330 and the dirty
island walk). Since 2026-11-14 placement's BA8C0 1D97F0 publication and its 282C10
1D9510 service also call this track (`havok_component_add`,
`havok_component_remove`, with `sp = 0`); 1CC190's creation (1D7360) keeps
its refusal. objtypes' `objtypes_object_wake_bodies` borrows
`havok_component_activate`'s fixture (read only, through
`tools/native_object_factory_track_fixtures.py`) to prove BBBC0 → 1D9F10 →
315CB0; keep that builder's name and its single component argument.

- **objtypes**
  - 1D9F10 (`physics_component_activate` in `object_wake`): call
    `havok_component_activate(m, component, fp)`. `fp` is
    `NativeTu15PhysicsFrameFp{x87, 0, mxcsr}`.
  - `ObjectTypeServices::phantom_wake`: bind
    `[](M& m, std::uint32_t o){ HavokFp fp{...}; havok_phantom_wake(m, o, fp); }`.
    It is now native end to end (same signature, no rewiring needed). It
    refuses by name only for owners outside the proven set (see the owners
    table); objtypes' `objtypes_scenery_new_failed` can drop its per-routine
    0x1CE340 closure stop once its fixture provides the world [0x52BE48],
    broadphase and arena (borrow `phantom_world`-style state from this
    track's case module).
- **Anyone needing hkWorld phantoms or the broadphase:**
  `havok_world_add_phantom(m, world, phantom, fp)` (returns the phantom),
  `havok_world_remove_phantom(m, world, phantom, fp)`,
  `havok_broadphase_add_object(m, broadphase, handle, aabb, pairs, fp)`,
  `havok_broadphase_remove_object(m, broadphase, handle, pairs, fp)`. The
  broadphase calls take the caller's AABB and `{data,count,capacity}` pair
  header as world addresses (the hkWorld owners pass private ones).
  - 1CC3F0: call `havok_component_state_flags(m, object, 0, fp)`. It gains
    nothing over today's refusal until 1D73D0 is native.
- **modelstate** (1CE0E0 → 1CC310) and **placement** (`component_set_flags`,
  BA8C0/1CC190): use `havok_component_state_flags(m, object, flags, fp)`;
  it binds 1D9510/1D8470/1D73D0/1D97F0. A caller that models the original
  stack passes its entry ESP as the trailing `sp` (only the fixed-body pair
  collector's published address depends on it).
- **Anyone removing/adding a component's bodies:** `havok_component_remove`
  (1D9510), `havok_component_add` (1D97F0), and the hkWorld entity owners
  `havok_world_remove_entity` (3198A0, `result` is the byte the original
  writes) and `havok_world_add_entity` (319370), each with optional `sp`.

## Continuation plan

Measured static closures (direct calls; 149930 and 2E3250 are stopped),
and the part any census executed (the 04b-era `factory_union.json` union of
executed PCs; it overcounts, since a function executed anywhere counts):

| Owner | Static functions / instructions | Census-executed functions / instructions |
|---|---|---|
| 1D73D0 (dispatch + all handlers) | 154 / 9,854 | 123 / 6,375 |
| 1DEE30 biped | 131 / 8,917 | 101 / 5,516 |
| 1DD9A0 vehicle | 140 / 8,821 | 110 / 5,492 |
| 1DDBF0 machine | 125 / 8,259 | 95 / 4,987 |
| 1DD950 crate | 126 / 8,283 | 96 / 5,011 |
| 1DEF50 creature | 68 / 3,909 | 66 / 3,161 |
| 1CD6A0 (1D73D0 tail) | 12 / 541 | 12 / 477 |
| 1D9510 / 1D97F0 | 2,207 / 180,773 | 1,006 / 59,510 |

Ranked (2026-11-14, after the entity membership pass):

1. **1D73D0's shape builders** (the dispatch is native). Armory 01a ran
   1DDBF0 machine 114 times, 1DD950 crate 59, 1DEE30 unit 3, 1DEF50 0 and
   1DD9A0 0 (vehicles appear elsewhere). Biped and vehicle first per the
   coordinator; machine/crate are the most executed. Each builder is
   census-sliced (shape constructors 1DE170/1DEC80/1DF6B0/185C00/185910,
   the hkShape vtables, 31B*/31C*/2E4* helpers, 184920/184A70/184B90,
   1E0980, 212C70, BC190, BBE00, E3310/E32C0). When one is native, remove
   it from `SHAPE_BUILDERS` (stop entries) and `REFRESH_STOPS`, bind
   `havok_component_refresh` as 1CC310's refresh service (its 1D73D0 stop
   then moves to the remaining builders: update units' 1D73D0 stop entry
   with their owners); the dispatcher's tail (1CD6A0
   and the +4 flags) is already written and will then be covered.
2. **Contact agents of entity pairs.** 320EF0's manager/agent creation
   (after an admitted pair is not deferred, including 317200's merge
   request) and 320260's agent/manager destruction are refused. The live
   world composes natives for these in
   `src/physics/native_tu15_world_integration_residuals.cpp`
   (`native_tu15_create_activation_contact_manager`, the convex/GSK agent
   factories, `native_tu15_delete_bound_pair_in_native_scope` with
   `native_tu15_dispose_contact_reference`); bind them here with fixtures
   that own real agents. This also unlocks 321510's owned record growth.
3. **319750** (pending world operations before a removal): bind
   `native_tu15_drain_world_operations` with
   `native_tu15_merge_physics_islands` (316D40, 318DE0, 2E31E0, 322100
   services) and fixtures with world +20/+24 pairs.
4. **282C10's dirty path**: its 1496F0 (core track), 1D7CA0, 16E8A0
   (`native_tu15_data_array_delete`), BA570 and BA8C0 (placement's
   `object_defer_physics`) services, then fixtures with dirty islands.
5. The smaller refusals: 1D9510's +80 constraints (316AA0) and +94
   attached-object wake (objtypes' BBBC0), 22D0A0 model constraints,
   1D97F0's +80 (317350) and the scaled record (31E430/31E560/31E590),
   1CC560's entity limit (1CDD60).
6. **Bind 1D9510 into 1CC310**: give units' `units_physics_mode` fixture an
   entity world (borrow this track's `entity_world`), drop 0x1D9510 from its
   stops, then bind `havok_component_remove(m,row,state,below(sp,0x10u))`
   (the comment in `havok_component_state_flags` shows it). Callers that
   model the original stack pass the entry ESP as `sp`; placement passes 0.
7. **Listener natives for 315CB0:** 282870 and 2816A0. Their current
   natives call original code for unconverted agents.
8. **Wider phantom bindings** when a fixture can prove them: 315B00/315A70
   (shape phantoms' overlap owners, natives exist), the 282700 gameplay
   overlap listener (`native_tu15_notify_gameplay_phantom_added`, needs the
   object deletion service), 37C960's interior merge (both ranges
   non-empty; only 321510 reaches it).

Leave 1DBBD0 and 1DC7A0 to the nodebuild continuation.
- Validator follow-up (havok agent, 2026-11-13): capstone reports `movq m64, mm` destinations as reads, so MMX stores are not in the store set. Only the four broadphase sites are corrected per-case; add memory-destination MOVQ to the validator's store classification (like x87/MOVHPS) once the streaming change lands.
