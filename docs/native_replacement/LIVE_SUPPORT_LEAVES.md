# Group L: live-support census honesty and leaves (2026-11-15)

Group L of [REMAINING_TRANSLATED_20261115.md](REMAINING_TRANSLATED_20261115.md)
holds the `physics_live_support` remainder and its multi-copy owners. This
note records two pieces of work:

1. **Census honesty.** Most of the group's large owners were already native.
   They appeared in the census only because their natives either refuse
   under the census callbacks or report every original PC to them.
2. **Five translated leaves ported.** C9550, 1EC970, 1F7F60, C1E90 and
   110350 are now hand-written systems code with thin boundary adapters,
   bound into every matching unit.

Later batches (sections 4 to 6) port the rest of the group's list, add the
child-call boundary, and bind the canonical copies.

## 1. Census honesty

The census (`HALO2_INSTRUCTION_CENSUS` with
`HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1`) counts every call of
`services.instruction`. Two kinds of native owner inflated it.

**Cursor-gated bridges.** These refuse full callbacks without a literal
cursor, so under the census their translated fallback ran and was counted.
In production (literal cursor) they are admitted. They now honour
`HALO2_CENSUS_NATIVE_BRIDGES=1` via `native_census_detail::uncursored`, or
the new `native_partition_temporary_detail::admitted_census(s,first,last)`.
That function is `admitted()` plus the census branch. No range holds a live
service PC (0x38846E, 0x21F590, 0x33B595).

| Owner | Native | Range |
|---|---|---|
| 13FE50 page allocate | `native_page_allocate_detail::run` | 13FE50..14030B |
| 32A660 stack probe | `native_stack_probe_detail::run` | 32A660..32A6A0 |
| 13FE00 page window | `native_page_window_detail::run` | 13FE00..13FE4C |
| 140310 page retire | `native_page_retire_detail::run` | 140310..140387 |
| 12FCB0 / 4057F0 resource busy | `native_resource_busy_detail` | 12FCB0..12FD02, 4057F0..405871 |
| 2EC9B0 broadphase remove | `native_broadphase_remove_detail::run` | 2EC9B0..2ECF67 |
| 120FA0 segment/sphere | `native_segment_sphere_detail::admitted` | 120FA0..1210AC |
| 1E5810 record search | `native_record_search_detail::bridge` | 1E5810..1E5870 |
| 1E5C80 node resource lifecycle | `native_node_resource_lifecycle_detail::bridge` | 1E5C80..1E5D5D |
| 1E3150 node descriptor lookup | `native_node_descriptor_lookup_detail::bridge` | 1E3150..1E31D3 |
| 1E5AF0 node resource | `native_node_resource_detail::bridge` | 1E5AF0..1E5B24 |

**Reporting natives.** These always run natively in TU1.5 but report each
original PC through `services.instruction`, for cursor and trace parity:

- D80A0: `damage_model_lookup_native_tu15`;
- BCB70: `native_object_function_scalar_detail::SourceIo::at`;
- the weapon marker owners BAD60, 1E1200, 144910, 145560, BACD0, FC510 and
  1E11B0: the `instruction` lambdas in `native_weapon_marker_*_tu15_internal.hpp`.

Under the census switch with full callbacks and no cursor, they now skip that
report (`native_census_detail::skip_report`), but never for a live service
PC. Production (cursor present) and every trace or diagnostic keep it.
`HALO2_CENSUS_COUNT_NATIVE_REPORTS=1` restores the reports, so a census can
measure what they contribute.

Some natives were left counted on purpose:

- **The effect protocol leaves** (16EA60 slot scan, 16E6A0 datum allocation,
  and the 145560 transform-product fast path) were tried and reverted. They
  write the cursor through `instruction_cursor.position` unguarded (for
  example `native_tu15_effect_slot_scan.hpp`, line 83), so admitting them
  without a cursor crashed the census run (SIGSEGV on `halo2_native.L2`).
  Making them census-honest needs those writes guarded first.
- **10DE40** (`native_object_type_query_owner_detail`) re-admits each nested
  piece with its literal-cursor rule, so it was not given the census branch.

None of this changes production admission or behaviour. The census admission
skips only the cursor, trace and observer checks, as the existing census
bridges do.

### Census on one binary (`halo2_native.L1`, Armory, 300 frames)

Both runs use `HALO2_CENSUS_NATIVE_BRIDGES=1`. Both give frame
`5627a51badf47762` and trace `8a9b58b38bef`.

The reference run adds `HALO2_CENSUS_COUNT_NATIVE_REPORTS=1` and these
switches: `HALO2_REFERENCE_PAGE_ALLOCATE`, `_PAGE_ALLOCATOR`,
`_RESOURCE_BUSY`, `_BROADPHASE_REMOVE`, `_SEGMENT_SPHERE`, `_RECORD_SEARCH`,
`_NODE_RESOURCE_LIFECYCLE` and `_NODE_DESCRIPTOR_LOOKUP`.

| Run | Translated instructions/frame |
|---|---|
| Reference (counted as before) | 308,533 |
| Honest | 292,715 (−15,818) |

The reference reproduces the pre-change census of these owners exactly: the
group's 42 tracked routines total 28,826/frame on both the old
`halo2_native.Lbase` and the new reference run. The removed counts per frame:

| Routine | Before | After | Routine | Before | After |
|---|---|---|---|---|---|
| 1E5810 | 1,917.5 | 0 | 145560 | 957.3 | 0 |
| 2EC9B0 | 1,685.2 | 0 | 1E1200 | 929.8 | 21.3 |
| 120FA0 | 1,656.9 | 0 | D80A0 | 880.8 | 0 |
| 13FE50 | 1,452.6 | 0 | BCB70 | 851.2 | 0 |
| 32A660 | 1,399.3 | 0 | BAD60 | 817.2 | 24.3 |
| 1E5C80 | 1,011.4 | 0 | 144910 | 810.4 | 0 |
| 1E3150 | 766.6 | 0 | 1E11B0 | 473.7 | 0 |
| | | | BACD0 | 145.7 | 0 |

## 2. Five translated leaves

The code follows the code standard in REMAINING_TRANSLATED_20261115.md. It
is split into two layers.

**The systems** are in `src/physics/runtime/native_tu15_live_support_systems.hpp`,
namespace `halo2::native_live_support`. This is the game logic only. The game
data is named: globals, `data_array`, `object_header`, and the unit, iterator,
channel-table and level-block fields. Control flow is plain C++, and comments
describe each system:

| System | Original | What it does | Census before (instr/frame) |
|---|---|---|---|
| `unit_level_block::update` | 110350 | ramps a unit block's level toward fixed bounds while attached; otherwise decays it, rescales twelve signed bytes and steps an LCG countdown byte | 1,008.6 |
| `flagged_datum_iterator::advance` | 1EC970 | next in-use datum of a data array, optionally only flagged datums | 746.0 |
| `animation_channel_iterator::next` | 1F7F60 | next populated entry of a group/sub-table channel table | 552.6 |
| `unit_animation_mode::listed` | C9550 | is the unit's animation mode one of the five at 473990 | 453.2 |
| `bit_vector::test` | C1E90 | one bit of a dword bit vector | 54.2 |

The game meaning of the 110350 block is not established, so its names
describe the arithmetic.

The logic reaches game memory through a `Memory` object with typed loads and
stores. Each access names its role with the system's `Access` enum. Three
things are kept exact, with comments where they matter:

- memory access order;
- the SSE operations (ADDSS, SUBSS, MULSS, COMISS, UCOMISS, CVTSI2SS,
  CVTTSS2SI), with the destination operand first;
- the values the original leaves in caller-visible scratch registers, which
  are returned in each result's `Residue` for the adapter.

**The boundary adapters** are in `src/physics/runtime/native_tu15_live_support_leaves.hpp`,
namespace `native_tu15_leaves::live_support_boundary` and the `kernel_*`
functions. Their callers are still translated, so each adapter owns only
that caller's ABI:

- arguments from registers and stack slots;
- results back into the original registers, including partial writes;
- the final EFLAGS;
- the stack frame: register saves and restores, and 110350's counter and
  spill slots, through a small `Frame`;
- the cursor, via a per-system table that maps each access role to its
  original instruction.

Delete an adapter when its callers go native.

The leaf framework (`native_tu15_leaves::run`) runs the adapters. It admits a
call for the TU1.5 image with a literal cursor or none, no active trace, and
under the census switch. `HALO2_REFERENCE_MATRIX_LEAVES=1` restores the
translated bodies, and `HALO2_VERIFY_MATRIX_LEAVES=1` checks every admitted
call against them.

171010 (interval table lookup, 65/frame) was ported in the first,
machine-shaped draft. That draft was withdrawn for the code standard, and
171010 stays translated until it is rewritten (done: section 4).

### Binding

`tools/native_live_support_leaves_replacements.py` is called from
`spatial_owner_bindings` in `tools/specialize_native_operands.py` and is a
dependency in `cmake/NativeOperandSpecialization.cmake`. The receipt key is
`native_spatial_owners`. `ENABLED = False` holds every leaf on its translated
route.

A leaf is installed only when its complete canonical body matches a pin. There
are two pin forms: the literal live-support form, and the identical body whose
`selected()` operands the unit maps to itself. The second form binds the
copies in the AI frame (C9550, 1EC970, C1E90), combat action (1EC970) and lens
query (C1E90) units. A changed body or a nonidentity operand keeps the
translated route.

### Proof

`tools/validate_native_live_support_leaves_tu15.py` (CTest
`native_tu15_live_support_leaves`, `cmake/NativeTu15LiveSupportLeaves.cmake`)
runs `tests/native_tu15_live_support_leaves_tests.cpp` against the
unmodified bodies in their production cursor form.

The fixtures author:

- object pools and unit records;
- iterators with negative, past-the-end and 0x7FFFFFFF indexes, sparse and
  dense bit vectors, a null datum on a backed page 0, and flagged datums that
  loop back;
- channel tables with -1 positions, null sub-tables, non-positive counts and
  in-place entries above 0x7F;
- negative and 0x80000000 bit indexes;
- level blocks with special floats and exact signed-zero compares;
- every MXCSR rounding, DAZ and FTZ mode with random sticky flags;
- removed pages (faults).

Each case compares:

- the ordered stores (pc, address, preimage, bytes);
- the whole bank;
- GP, EFLAGS, XMM, the x87 status and stack, MXCSR and the control word;
- the cursor.

A fault compares the exception, the cursor and the ordered stores outside the
leaf's own dead stack frame. The adapter makes the register saves at entry,
while the original interleaves some of them with its first reads. Every
non-fault store order is exact, the frame included.

Results (2026-11-15, after the rewrite):

| | |
|---|---|
| Release comparisons | 36,000 (7,200 per leaf), 256,513 ordered stores, 2,409 faults |
| Leaf verify mode | 5,606 |
| Census mode (full callbacks, no cursor) | 6,000 |
| ASan/UBSan | 9,000 + 1,126 verify |
| Branch directions | C9550 6/6, 1EC970 17/18, 1F7F60 17/18, 110350 26/26 (C1E90 has none) |
| Mutants rejected | 41/41: 32 in the logic, 9 in the adapters |
| Refusals | MapPack11, full callbacks without the census switch, an active trace |
| Binding | live-support install, helper idempotence, changed body and nonidentity operand refused |

Two branch directions cannot be reached:

- 1EC970 `CMP EDX,-1` at 1EC9EE. A hit index passed the JS test, so it is
  non-negative.
- 1F7F60 `CMP ECX,-1` at 1F7FD7 after an advance. The index is a byte or a
  non-negative loop index.

Some first-draft mutants were equivalent for every reachable input, so they
were replaced:

- the SAR→SHR of a non-negative index;
- sign- versus zero-extension of a word shifted left by 16;
- clamp bounds at their own value;
- the ZF-only test of an ordered compare;
- CVTSI2SS into zeroed upper lanes.

Source hashes are rechecked after the screen. The receipt is
`build/test-results/live-support-leaves-20261115/validator.json`.

## 3. Live results (`halo2_native.L2` / `.L3`, shared `build-perf-4k60` tree)

L2 (sha256 `fb13e610…`) carries the rewritten leaves bound in live support
(five) and in the copies: AI frame (three), combat action (one) and lens
query (one).

L3 (sha256 `82cd0763…`) is L2 without the reverted effect-protocol census
branch.

**Armory parity** (01a, 300 frames, 1280x720, `scratchpad/parity.sh`):
native, reference (`HALO2_REFERENCE_MATRIX_LEAVES=1`) and verify
(`HALO2_VERIFY_MATRIX_LEAVES=1`, which throws on any difference) all give
frame `5627a51badf47762` and trace `8a9b58b38bef`. Both L3 census runs give
the same pair.

**Multiplayer parity** (Coagulation, `tools/perf_4k60/parity.py`, 3840x2160,
cases 4:90, 4:150 and 1:150):

- Reference against native is identical: `9c14a330…`, `13fa114e…` and
  `05f31264…`, unchanged from the B8F40 checkpoint.
- Reference against verify is identical as well.

Receipts: `build/test-results/live-support-leaves-20261115/mp-parity-{native,verify}.json`.

**Census on one binary (L3, Armory, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`).**
The reference run adds:

- `HALO2_REFERENCE_MATRIX_LEAVES=1` (this also restores other groups' matrix
  leaves, hence its higher total);
- `HALO2_CENSUS_COUNT_NATIVE_REPORTS=1`;
- the eight bridge switches above, plus `HALO2_REFERENCE_NODE_RESOURCE=1`.

| Run | All translated/frame | Group L's 42 tracked routines/frame |
|---|---|---|
| Reference | 365,005 | 28,848 |
| Native | 290,575 | 9,811 (−19,037) |

The removed group L counts per frame:

- census honesty: 16,222;
- the five ports: 2,815 (110350 1,008.6, 1EC970 746.0, 1F7F60 552.6,
  C9550 453.2, C1E90 54.2).

Seven routines increased by 14.5/frame in total: 217090 6.6, 12EDF0 2.5,
140430 1.2, and the rest under 0.2. These are children whose route differs
when the bridge reference switches send their parents to the translated
bodies.

**Multiplayer sweep** (L2; every MP map in 1p and 4p, 50 cases). It uses
the node-rebuild sweep's scripted input: fire, grenade, jump, use, crouch,
move and look, for 600 ticks at 960x540. There are two roles: native, and
verify (`HALO2_VERIFY_MATRIX_LEAVES=1`, which throws on any leaf difference).
Receipt: `build/test-results/live-support-leaves-20261115/mp-sweep.json`.

- No verify difference in any case.
- 43 cases: native and verify frames are identical.
- 3 cases (ascension, elongation, lockout 4p): the frames are not
  deterministic. A second native run differs from the first as well.
- 4 cases (needle and street_sweeper, 1p and 4p): both roles stop on
  `Source dynamic model resource unavailable: ... active_camouflage`. The
  failure is pre-existing. It reproduces with
  `HALO2_REFERENCE_MATRIX_LEAVES=1` and on `halo2_native.Lbase`, the shared
  binary from before any group L change. The node-rebuild sweep3 (06:00)
  still passed these maps, so the regression came from elsewhere between
  then and 19:12.

## Remaining translated in group L (honest L3 census, instructions/frame)

| Owner | Census | Note |
|---|---|---|
| 32CC0C | 2,079 | CRT float→int64 (x87 FISTP/FILD/FSUBP); six unit copies with different spill forms |
| 1D6FB0 | 1,601 | cluster owner update; children are group H's phantom world |
| 1F8200 | 1,017 | calls 144090 (614, in another unit), 1E6980 (native), 1F7E90 and the now-native 1F7F60 |
| 16E600 | 937 | data-array reset (REP STOS fills, 32A4E0 child) |
| 144090 | 614 | |
| 10DE40 | 563 | native (`object_type_query`), not census-admitted |
| 179CD0 | 549 | |
| 16E6A0 / 16EA60 | 540 / 306 | native effect leaves; census admission needs guarded cursor writes first |
| 22A790 | 503 | calls 4057F0 |
| 243F20 | 495 | vertex position decompression (jump table, SSE); a good next system |
| 1211C0 | 450 | calls the 123210 leaf |
| 171010 | 65 | first draft withdrawn for the code standard |
| 138D90, 16E450, 16E4B0 | 24, 12, 11 | |

## 4. Second batch: five more leaves and four census branches (2026-11-15, evening)

### Ported to the code standard

| System | Original | What it does | Census before (instr/frame) |
|---|---|---|---|
| `float_to_int64::convert` | 32CC0C | the C runtime's `_ftol2`: x87 value to int64 toward zero (FISTP by the control word, then a one-step correction from the FSUBP difference) | 2,079 |
| `data_array_reset::delete_all` | 16E600 (+ 32A4E0) | `data_delete_all`: clear the counts, reseed the salt from the name (its strncpy call, n = 2), debug-fill, zero each datum's salt, clear the in-use bits | 937 (+34 strncpy) |
| `vertex_position::read` | 243F20 | one vertex position in five layouts (uncompressed, int16-compressed into the part's bounds, each with or without a node byte, or the default point) | 495 |
| `triangle_point::test` | 1211C0 (calls 123210) | is a point on a triangle (plane distance and projected edge tests at 1e-4), and its barycentric coordinates | 450 |
| `interval_table::find` | 171010 | the record of a 0x48-byte table whose word interval holds a key | 65 |

The logic is in `native_tu15_live_support_systems.hpp`, the adapters in
`native_tu15_live_support_leaves.hpp` (`kernel_32cc0c<StoresSetC1>`,
`kernel_16e600`, `kernel_243f20`, `kernel_1211c0`, `kernel_171010`). The
data_array header gained its remaining named fields (name, flags, next index,
active count, next identifier).

Points worth knowing:

- **32CC0C and the x87 stack.** The leaf framework's `Machine` now carries the
  caller's x87 stack (`Machine::x87`, published and verified only when a kernel
  sets `x87_stack_owned`; `native_tu15_matrix_leaves.hpp`, a surgical
  addition). The x87 operations (`x87::store_single`, `store_int64`,
  `load_int64`, `subtract`) run the original instructions on the extended
  value. The units translate the routine with two x87 status models: the
  live-support copy's spills do not report C1, the other five copies' do. The
  adapter takes the caller's model as a template argument and the binder picks
  it per pin (`kernel_32cc0c<false>` for live support, `<true>` elsewhere).
  Both forms are proven separately (`32cc0c` and `32cc0c_tracked`).
- **16E600 performs its strncpy call (32A4E0) natively.** The count is always
  2, so the copy is: first character, then the second or a zero pad. The
  adapter keeps the call frame (return address and strncpy's three saves) and
  the instruction sites of the alignment-dependent byte loops. The binder
  requires 32A4E0's pinned body in the same unit (`CHILDREN`). The block fills
  (REP STOSD then STOSB) honour EFLAGS.DF.
- **243F20's jump table** (24417C, XBE code) is replaced by the layout switch;
  its four XBE constants are read where the original reads them. The
  dequantization keeps the original order ((q*2 + 1)*step + 1/2)*range + min,
  and the coordinate spills keep their store order.
- **1211C0 calls the 123210 leaf** through `kernel_123210`, as the 14CF.. owner
  already does; its x87 C1 and XMM0/XMM1 residue come from that kernel. The
  routine's local block (normal, offset, e2, e1) is addressed by axis, so its
  stores stay in the logic as named locals. The binder requires 123210's pin
  where the unit holds it; the three copies without it dispatch to 123210 by
  its address.
- **171010** re-reads the table count for its exit test, so an offset store
  that overwrites the count changes the result; the fixture authors that.

### Census honesty (no production change)

- **16EA60 / 16E6A0 / 145560 fast path.** `native_tu15_effect_slot_scan.hpp`
  wrote the cursor unguarded at line 78 (the census SIGSEGV). It is now
  guarded, and `admitted_protocol` admits full census callbacks without a
  cursor (`census_callbacks`). The datum allocator and transform product
  already guarded their cursor writes.
- **10DE40** (`native_tu15_object_type_query_source_owner.hpp`): the census
  branch is in `admitted<true>`, which the nested continuation checks reuse;
  the virtual callee keeps its own admission.
- **144090** (`native_tu15_affine_inverse.hpp`): census branch in `admitted`.

Existing validators rerun after these edits: effect slot scan (1,190
comparisons), effect datum allocate (1,210 cases), object type query owner,
affine inverse: all pass.

### Proof

`tools/validate_native_live_support_leaves_tu15.py` (CTest
`native_tu15_live_support_leaves`) now covers ten leaves, 32CC0C in both x87
models, and the two children the references call (32A4E0, 123210). New
fixtures: vertex descriptors in every layout (with wrapping indexes, special
floats, unaligned points and a point aliasing the bounds); x87 values for
every conversion path (integers, halves, +-2^32, >= 2^63, NaNs, infinities,
denormals, signed zeros) under every rounding and precision mode, and empty or
full x87 stacks; data arrays with unaligned names, debug fill, negative
counts, datums aliasing the header, and a set direction flag; interval tables
with overlapping and wrapping intervals, a clear that empties the table, an
offset store over the count; triangles near and off their plane, degenerate,
flat, with repeated vertices and a perturbed projection table.

| | |
|---|---|
| Release comparisons | 79,200 (7,200 per harness leaf), 1,103,667 ordered stores, 4,483 faults |
| Leaf verify mode | 12,448 |
| Census mode | 13,200 |
| ASan/UBSan | 19,800 + 1,136 verify |
| Branch directions | 243F20 8/8, 32CC0C 6/6, 16E600 6/6, 171010 10/10, 1211C0 14/14, 123210 6/6, 32A4E0 22/48 (26 unreachable with count 2: the zero-count exit, every dword loop and the second padding byte), the first five unchanged |
| Mutants rejected | 104/104 (63 new: logic, adapters, and the framework's x87 publication) |
| Binding | ten units; 32CC0C's kernel per pin; 16E600 and 1211C0 child pins |

Harness notes: the references' REP STOS deliveries are rewritten to the
production cursor form (`cursor_form`), like the other deliveries. In verify
mode a reference that faults on a store is matched by any fault of the
overlay run (the overlay never faults on stores); release mode still compares
every fault exactly. Four first-draft mutants were equivalent for every
reachable input and were replaced: the y spill against the z bound re-read
(needs the bounds inside the routine's own frame), the re-read of a record's
first value (no store can intervene), and two anchors that matched more than
one site.

### Live results (`halo2_native.L5`, shared tree, sha256 `653d5c4f…`)

L4 (`124ccd31…`) carried 243F20/32CC0C/16E600 and the effect/10DE40 census
branches; L5 adds 171010, 1211C0 and the 144090 census branch.

**Armory parity** (01a, 300 frames, `scratchpad/parity.sh`): native,
reference (`HALO2_REFERENCE_MATRIX_LEAVES=1`) and verify
(`HALO2_VERIFY_MATRIX_LEAVES=1`) give frame `5627a51badf47762` and trace
`8a9b58b38bef` on both L4 and L5, as do both L5 censuses.

Live runs need disk-backed scratch: run `tools/run_test.py --scratch-root
.local/test-scratch`. Under the default tmpfs scratch the MCC sound DAT
refuses its snapshot ("requires a complete private disk-backed snapshot")
in every role.

**Census on one binary (L5, Armory, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`).**
The reference run adds the switches of section 3 plus
`HALO2_REFERENCE_EFFECT_SLOT_SCAN`, `_EFFECT_DATUM_ALLOCATE`,
`_EFFECT_TRANSFORM_PRODUCT`, `_OBJECT_TYPE_QUERY` and `_AFFINE_INVERSE`.

| Run | All translated/frame | Group L's 42 tracked routines/frame |
|---|---|---|
| Reference | 351,580 | 28,848 |
| Native | 271,096 | 3,778 (−25,070) |

This batch's owners per frame (reference → native): 32CC0C 2,079 → 0,
16E600 937 → 16, 32A4E0 34 → 27, 243F20 495 → 0, 1211C0 450 → 0, 171010 65
→ 0, 16E6A0 540 → 0, 16EA60 306 → 0, 10DE40 563 → 0, 144090 614 → 0. Against
the previous honest census (L3) these routines total 6,082 → 43 (−6,040).
(The reference also restores 123210 1,087 and 145560 979, which L3 already
counted as native.) The 16E600 residue is three calls in 300 frames that
reach the translated body (one 576-datum salt loop); the strncpy residue is
16E4B0's 32-character copy.

**Multiplayer sweep** (L5; every MP map in 1p and 4p, 50 cases, native and
verify roles, the node-rebuild scripted input; receipt
`build/test-results/live-support-leaves-20261115/mp-sweep-L5.json`): no verify
difference in any case; 43 cases with identical native and verify frames; the
3 known nondeterministic 4p cases (ascension, elongation, lockout) differ; the
4 needle and street_sweeper cases stop in both roles on the known
`Source dynamic model resource unavailable: ... active_camouflage` error. This
is the same outcome as the L2 sweep.

## Remaining translated in group L (honest L5 census, instructions/frame)

| Owner | Census | Note |
|---|---|---|
| 1D6FB0 | 1,601 | group H's children; left alone |
| 1F8200 | 1,017 | children 144090, 1E6980 (state-based natives), 1F7E90, 1F7F60 |
| 179CD0 | 549 | children are group E's 17C140/17C370/17C5C0 plus two dispatches |
| 22A790 | 503 | child 4057F0 (resource busy bridge) through dispatch |
| 1F7E90 | 232 | pure leaf: the channel iterator's begin |
| 138D90, 16E450, 16E4B0 | 24, 12, 11 | children 138510/138C20; 16E4B0's strncpy has count 32 |

## 5. Third batch: five leaves, a child-call boundary, and the 16E600 residue (2026-11-16)

### Ported to the code standard

| System | Original | What it does | Census before (instr/frame) |
|---|---|---|---|
| `animation_channel_iterator::begin` | 1F7E90 | starts a channel iteration: the owner, position -1, and the sixteen selection bytes (zeroed, copied from a source selection, or remapped through a 0x10-byte-record table whose count is clamped to 16) | 232 |
| `bounded_string_copy::copy` | 32A4E0 | the C runtime's `strncpy`, in its own access pattern: bytes to an aligned source, dwords with the `(w + 7EFEFEFF) ^ ~w` zero-byte test (each dword read twice), the tail, and the zero fill (bytes to an aligned destination, dwords, bytes) | 27 (after L5) |
| `data_array_setup::initialize_header` | 16E4B0 | a data array's header: cleared, the name copied (strncpy, 32), its last character zeroed, the counts, alignment, allocation, bit vector and the `d@t@` signature | 11 |
| `data_array_setup::initialize` | 16E450 | `data_initialize`: the datums after the header aligned to 2^bits (shift count modulo 32), the bit vector after them, the header (16E4B0), flag bits 0-1 cleared, the bit vector zeroed | 12 |
| `resource_references::any_busy` | 22A790 | is any hardware resource an owner references still busy: kind-2 references with an id locate a holder (a structure, or one 32-byte element of its table) in the caller's block; 4057F0 (the resource busy query) is called for each non-null resource; the first positive count stops the scan | 503 |

16E600 now performs its strncpy through the general system (`frame.copy_name`, 32A4E0's kernel after the original's pushes and CALL); the two-character special case and its alignment-dependent site table are gone. 32A4E0 is also bound by itself (nine units), so every translated caller's strncpy is native.

### The child-call boundary (`native_tu15_leaf_children.hpp`)

A native whose original calls a routine that must stay itself (a native of
another framework, or another group's owner) enters it as a genuine child:

- `ChildIo::call(m, target, route)` publishes the Machine to the live state
  (the adapter has already set the original's registers at the CALL and
  pushed the return address), runs the child through the unit's own route
  (`dispatch(r, target, site, false)` or the local `run_` function), and
  reloads every register, flag, XMM and the x87 stack into the Machine. A
  fault inside a child leaves the child's own state (the framework does not
  republish over it).
- `run_with_children(s, kernel, reference, 0)` has `run()`'s admission
  (now the shared `admitted()` in `native_tu15_matrix_leaves.hpp`, a
  surgical extraction with no behaviour change).
- **Verify mode** (`HALO2_VERIFY_MATRIX_LEAVES=1`): such a leaf cannot run on
  the pure overlay. The native runs on the live bank, recording its ordered
  accesses and each child boundary (target, position among the accesses,
  entry and exit registers). The binder's reference copy of the translated
  body (`native_live_support_reference_<name>`, its child calls routed to
  `replay_child`) then runs on a replay bank: reads are served from the
  record in order, address and width; stores must match pc, address and
  bytes; each child call must match target, position and entry registers and
  receives the recorded exit; final registers must be identical. Nested
  leaves inside the reference run their translated bodies (`bypass()`).
  This is group H's 303EE0 scheme on the leaf framework.
- **Binder**: `CHILD_CALLS` in `tools/native_live_support_leaves_replacements.py`
  names each child's route; the reference copy is built from the canonical
  body, and any call that is not a declared child refuses the binding.

22A790's adapter keeps the original's registers with access hooks
(`BusyScanMemory`: each load writes the register the original loads into),
so the 4057F0 call sees EAX = ECX = the resource, EBX = the index, EDI =
index * 16, EBP = block + 8 and the flags of `TEST EAX,EAX`.

### The 16E600 residue

The three calls per 300 frames that still reached a translated 16E600 came
from the physics world unit's canonical copy, `run_16d220`
(ObservedWorldState, `world_observation_pc(16D220.., 16E600.., s)`), which
the binder never matched (it looked only for `run_16e600`). Its TU1.5 form
(each `world_observation_pc` taken at its TU1.5 PC, the `selected()` return
address mapped by the unit, labels and the `run_326970` child renamed) is
byte-identical to the proven 16E600 body, and `run_326970` to 32A4E0's. The
binder now installs `kernel_16e600` there (`CANONICAL`), guarded by
`s.selected_observation_pcs` as the unit's 2E9CF0 leaf is. A changed body or
a remapped return-address operand refuses (checked by the validator).

### Proof

`tools/validate_native_live_support_leaves_tu15.py` (CTest
`native_tu15_live_support_leaves`) now covers fifteen bound leaves plus
32CC0C's tracked form (17 harness leaves). New fixtures: 1F7E90 iterators in
the caller's frame (a negative group byte's selection store reaches the
argument slots holding the loop's count and the iterator), counts of 16 and
17, negative counts, -1 groups, choices and sub-tables; strings with every
alignment of source and destination, overlapping copies, counts 0..71 and
runaway counts, bytes 01/7F/80/81/FF that trip the zero-byte test without
being zero; data arrays with unaligned blocks, shift counts above 31,
negative counts and a set direction flag; 22A790 owners with every reference
kind, -1 ids, whole-structure and element holders, null, idle, busy and
negative-count resources. The 4057F0 child is a deterministic stand-in in
both runs (it reads the resource, changes EAX/ECX/EDX and the flags, leaves a
digest of its entry registers in XMM2, and sometimes rewrites the owner's
count, which the caller re-reads).

| | |
|---|---|
| Release comparisons | 115,200 (7,200 per harness leaf), 2,415,772 ordered stores, 8,926 faults |
| Leaf verify mode | 17,737 (22A790 through the record/replay child boundary) |
| Census mode | 19,200 |
| ASan/UBSan | 28,800 + 1,123 verify |
| Branch directions | 1F7E90 18/18, 32A4E0 48/48 (the 26 directions unreachable with count 2 are now all exercised), 22A790 14/14, 16E600 6/6; 16E450/16E4B0 have none; the earlier leaves unchanged |
| Mutants rejected | 157/157 (53 new: the five systems, their adapters and the child boundary's publish and reload) |
| Binding | 18 units checked: the live-support unit (15 leaves), the copies (32A4E0 in nine units, 16E4B0/16E450 in four, 22A790 in three), the canonical 16E600 copy (changed body and remapped operand refused), the pool startup unit (refused) |

Mutant notes: six first-draft mutants were equivalent for every reachable
input and were replaced (a looser zero-test bias only adds harmless false
alarms; masking one more byte masks the zero terminator; an elided re-read
of the same address; SAR against SHR of counts whose fills fault either
way; two flag forms that agree; ECX at the strncpy call, which strncpy
overwrites). One survivor was a real gap: 16E600's adapter pushed a literal
2, so the logic's `salt_characters` drove nothing; the adapter now pushes the
named constant. Three old anchors became ambiguous with the new code and were
made unique (every anchor is now checked to be unique).

Receipt: `build/test-results/live-support-leaves-20261115/validator-L6.json`.

### Live results (`halo2_native.L6`, shared tree, sha256 `8d973b94…`)

**Armory parity** (01a, 300 frames, 1280x720): native, reference
(`HALO2_REFERENCE_MATRIX_LEAVES=1`) and verify (`HALO2_VERIFY_MATRIX_LEAVES=1`)
all give frame `5627a51badf47762` and trace `8a9b58b38bef`. The verify run
checked 798 calls of 22A790 through the child boundary: 51,204 ordered
accesses and 2,969 child boundaries identical. Both L6 censuses give the
same pair.

**Census on one binary (L6, Armory, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`;
the reference adds the switches of sections 3 and 4).**

| Run | All translated/frame | Group L's 42 tracked routines/frame |
|---|---|---|
| Reference | 350,910 | 28,848 |
| Native | 269,622 | 3,238 (L5: 3,778) |

This batch's routines per frame (reference → native): 22A790 502.6 → 0,
1F7E90 231.7 → 0, 16E600 936.7 → 0 (L5 native 16), 32A4E0 34.2 → 2.2,
16E450 11.9 → 0.4, 16E4B0 10.9 → 0.5; together 1,728 → 3.1. The small
residue is the physics pool startup unit's canonical copies (16D070, 16D0D0,
326970), entered three or four times at load: they report canonical store
PCs, so their TU1.5 form is not the proven body and they keep their
translated route.

**Multiplayer sweep** (L6; every MP map in 1p and 4p, 50 cases, native and
verify roles, the node-rebuild scripted input; receipt
`build/test-results/live-support-leaves-20261115/mp-sweep-L6.json`):
no verify difference in any case; 43 cases with identical native and verify frames, and all 43 native frames identical to the L5 sweep; the verify role checked 387,384 22A790 calls through the child boundary (23,992,972 ordered accesses, 1,382,749 child boundaries identical); the 3 known nondeterministic 4p cases (ascension, elongation, lockout) differ; the 4 needle and street_sweeper cases stop in both roles on the known `Source dynamic model resource unavailable: ... active_camouflage` error. This is the L2/L5 outcome.

Live runs use `tools/run_test.py --scratch-root .local/test-scratch` (the MCC
sound DAT refuses tmpfs snapshots).

## Remaining translated in group L (honest L6 census, instructions/frame)

| Owner | Census | Note |
|---|---|---|
| 1D6FB0 | 1,601 | group H's children; left alone |
| 1F8200 | 1,017 | the channel ray cast; children 144090 (dispatch) and 1E6980 (local) through the child boundary, 1F7E90/1F7F60 inline |
| 179CD0 | 549 | the effect update loop; five children (16EA60, 17C370, 17C5C0, 17C140, 17A380) |
| 138D90 | 24 | x87 spills, two IDIVs, four children |
| BAD60, 1E1200 | 24, 21 | weapon-marker natives' remaining reports (unchanged since L3) |
| 16E450, 16E4B0, 32A4E0 | 0.4, 0.5, 2.2 | the pool startup unit's canonical copies (canonical store PCs) |

### Continuation (group L, after the third batch)

The child-call boundary is in place and proven on 22A790. The remaining
owners all need it:

1. **1F8200** (1,017/frame; RET 14, a 0x98-byte frame plus four saves).
   Arguments: [+4] a channel set (+4 remap table, +8 owner, +C source
   selection, +10 a table of 0x34-byte entries), [+C] a point, [+10] a
   vector, [+14] the output (its +8 is first set to the float at 45695C).
   It begins a channel iteration with the iterator as a local at frame +8C
   (1F7E90, then 1F7F60 per step: run both kernels inline). Per entry:
   - 144090 (dispatch at 1F8284, return 1F8289; ECX = the 0x34-byte entry
     of the record id byte, EAX = the local matrix at frame +18) builds the
     inverse transform;
   - the point is scaled by the matrix scale unless it equals 1.0 (UCOMISS
     then LAHF/TEST AH,44/JNP), then transformed into frame +58..60, the
     vector into +4C..54;
   - the query block (frame +4C..+88: the local direction and origin, the
     two pass-through arguments, the output's best fraction at [output+8]
     clamped to [0, 1] into frame +14, -1 markers) is tested by 1E6980
     (local, call at 1F84A4, return 1F84A9; four arguments), the ray test;
     a hit stores the iterator position (EBX), the record id (SI) and the
     group byte (frame +A3, the iterator's +17) into the output and sets the
     found byte at frame +13.
   It is the ray cast against an object's animation channels.
   AL = the found byte at exit. Every XMM register is caller-visible at exit
   (the last entry's arithmetic), and the SSE operand order must be kept.
   `CHILD_CALLS['1f8200'] = {0x144090: ('dispatch', 0x1f8284), 0x1e6980:
   ('local', '1e6980')}`.
2. **179CD0** (549): the effect update loop over the data array at 4F720C
   (the time step at [51DF54]+4 spilled to the frame); children 16EA60
   (dispatch, the native slot scan), 17C370, 17C5C0 and 17C140 (local,
   group E's owners: keep them genuine children) and 17A380 (dispatch).
3. **138D90** (24): a bitmap mip-level and region computation: x87 FISTP
   spills under the control word (`integer_spill`), two IDIVs (#DE on a zero
   divisor must stay the original's fault), and children 1386E0/1385D0
   (dispatch), 138510 and 138C20 (local).
4. 1D6FB0 stays with group H.

## 6. Fourth batch: three owners with children, and the pool startup copies (2026-11-16)

### Ported to the code standard

| System | Original | What it does | Census before (instr/frame) |
|---|---|---|---|
| `collision_model_ray::cast` | 1F8200 | a ray against every collision BSP of a posed collision model: per BSP, the node's matrix inverted (144090), the ray scaled unless the inverse's scale is exactly 1.0 and brought into the node's space, the BSP segment traversal (1E6980) limited by the result's fraction clamped to [0, 1]; a hit records the BSP's position, node and region | 1,017 |
| `bitmap_texel::address` | 138D90 | the address of a texel at (u, v) in one mip level of a bitmap: the level from the detail through the x87 (FISTP), the level's extents (signed word shifts, at least 1, rounded up to 4 when block-compressed), the texel wrapped by mask or by IDIV's remainder twice, the level offset by bitmap type (138510 2D, 1385D0 3D, 1386E0 cube) and the address (138C20) | 24 |
| `effect_update::update` | 179CD0 | one tick of every live effect: the attachment update (17C370), the visibility update (17C5C0), then delete a retiring effect with no particle systems (17A380) or advance it by the time step (17C140); the first effect from the slot scan (16EA60), each next one by the inline data-array scan | 549 |

All three run through the child boundary of section 5 (`run_with_children`):
their children stay genuine (1F8200: 144090 and 1E6980; 138D90: 138510,
1385D0, 1386E0 and 138C20; 179CD0: 16EA60, group E's 17C370, 17C5C0 and
17C140, and 17A380). None of group E's owners was ported. 1F8200 runs the
region iteration (1F7E90, 1F7F60) inline through their kernels.

The logic is in `native_tu15_live_support_systems.hpp`, the adapters in
`native_tu15_live_support_leaves.hpp` (`kernel_1f8200`,
`kernel_138d90<StoresSetC1>`, `kernel_179cd0`). Points worth knowing:

- **Registers at the children.** A genuine child sees every register, so
  each adapter reproduces the original's registers at each call. Loads that
  land in a register write it through `LoadingMemory` hooks (for example
  1F8200's initial fraction into XMM0 and the BSP position into EBX); values
  the original computes into scratch registers come from the logic as named
  residues (1F8200's `TestRegisters`, the XMM0..XMM7 values at the BSP test;
  138D90's `Residue`; 179CD0's `Found` and `Checked`). Each child stand-in in
  the harness folds a digest of all its entry registers (GP, flags, every XMM
  lane, x87 status, MXCSR) into the compared bank, so a wrong register at any
  call fails the comparison.
- **1F8200's frame.** Its local matrix, local ray, the BSP query block
  (named after the traversal's 0x28-byte context: flags, BSP, surface bitmap
  and limit, origin, vector, output, previous leaf and kind, plane) and the
  iterator live in a 0x98-byte workspace; the found flag is a frame byte,
  re-read for AL at the exit. The SSE arithmetic keeps each component's own
  product and sum order (the three origin and three vector components differ).
- **138D90's x87.** Each FLD m32 / FISTP m32 pair runs on the host
  (`x87::load_single`, `x87::store_int32`, new). The units translate it with
  two status models: the live-support, combat-action and model-render copies
  keep the FISTP's C1, the map-world-support copy leaves C1 as the FLD cleared
  it (`kernel_138d90<true>` / `<false>`, chosen per pin as for 32CC0C). A full
  caller x87 stack faults at the first FLD, as the translated body. The IDIV
  wraps cannot fault: no extent is zero (at least 1, rounded up at most to
  0x8000, which widens to -32768).
- **179CD0's walk** uses the data array's inline scan, now shared with
  1EC970 (`data_array::first_in_use`, returning the scan's limit and bit
  vector as well). Two branch directions are unreachable and declared: the
  walk's handle is never -1 inside the loop (EBX is callee-saved across the
  children), and the next index `(handle & FFFF) + 1` is positive.
- **1F7F60's access order.** The child-boundary replay requires the exact
  order of reads as well as stores. `kernel_1f7f60` made its three saves
  before its first read and its restores after its exit stores; it now places
  them exactly (an `OrderedMemory` with before/after hooks; the logic is
  unchanged), so 1F8200 can run it inline under verification.
- **Per-unit child routes.** The binder's `CHILD_CALLS` gained a `('call',
  site, name)` route: a local `run_<name>` call where the unit's body has one,
  otherwise a dispatch at the site (138D90's and 179CD0's copies route their
  children differently).

### The pool startup copies (store-site form)

The physics pool startup unit's copies of 16E450, 16E4B0, 16E600 and 32A4E0
(`run_16d070`, `run_16d0d0`, `run_16d220`, `run_326970`) map their cursor to
TU1.5 but report every store (push and `w` sites) at its canonical, disc-build
PC. The binder now forms their TU1.5 body with those store PCs mapped through
the unit as well; when that form is the pinned body (it is, for all four), it
installs the kernel through `run_with_store_sites`, whose Io reports each
store at the canonical PC that a binder-generated site map gives for the
kernel's TU1.5 site (an unmapped site throws). Admission and verify mode are
`run()`'s. The physics world unit's `run_326970` (observation-PC form) now
binds as a plain canonical 32A4E0 copy, beside its 16D220.

### Proof

`tools/validate_native_live_support_leaves_tu15.py` (CTest
`native_tu15_live_support_leaves`) now has 24 harness forms (7,200 cases
each by default): the 18 bound leaves, 32CC0C's tracked form, 138D90's
C1-clearing form (the map world support unit's body, with its own
verification reference), and the four pool startup copies in their
store-site form (the unit's bodies with their TU1.5 cursor and canonical
store PCs, run through `run_with_store_sites` with the binder's generated
site maps, so each store's PC is compared exactly).

New fixtures: posed collision models (regions, permutations and BSPs with
in-range and unauthored nodes, remap tables and selections), node matrices
at varying offsets with scales of exactly 1.0, specials and perturbed
constants, results whose fraction is negative, zero, NaN, inside (0, 1) or
above 1; bitmaps with power-of-two, other, unit, zero, negative and
0x7FFC..0x7FFF extents, every type word, compressed flags, mip counts,
absent pixels, coordinates past the int32 range and full x87 stacks; effect
arrays (two, so the stand-ins can move 4F720C between them) with sparse and
dense bit vectors, retiring effects with and without particle systems, a
null datum address, salts of FFFF and datum sizes other than 0x190.

The mutation screen now also runs leaf verify mode when the release run
passes, so access-order mutants (registers and stores unchanged, reads
reordered) are rejected by the exact replay.

| | |
|---|---|
| Release comparisons | 172,800 (7,200 per harness form), 5,236,484 ordered stores, 14,996 faults; this batch's eight forms 57,600, 2,824,320 ordered stores, 6,093 faults |
| Leaf verify mode | 26,525 (1F8200 1,097, 138D90 1,061 + 1,080, 179CD0 1,102, pool forms 4,341) |
| Census mode | 28,800 |
| ASan/UBSan | 43,200 (3,730 faults) + 1,110 verify |
| Branch directions | 1F8200 16/16, 138D90 22/22, 179CD0 26/28 (2 unreachable, declared), the earlier leaves unchanged |
| Mutants rejected | 215/215 (58 new: 1F8200's logic and adapter, 138D90's, 179CD0's, the store-site Io, and access-order mutants for 1F7F60's restores, 1F8200's argument read, 138D90's height re-read and 179CD0's time-step slot) |
| Binding | 1F8200 (live support); 138D90 (four units, kernel per pin); 179CD0 (live support, combat action); the pool startup unit's four copies (store-site form, idempotent; a changed store PC or a remapped instruction refuses); the physics world unit's 16D220 and 326970 (plain canonical) |

One first-draft mutant was equivalent and was replaced: 138D90's SUB EAX,0
against TEST of the type (identical flags, AF included). One survivor was a
fixture gap: the flags of the ADD that indexes 1F8200's node matrices never
differed from a TEST's at page-aligned bases (no carry into AF); the fixture
now varies the base. The first draft's hook for 1F8200's region load (MOVZX
AX) was removed: 1F7F60 overwrites EAX before anything reads it.

Receipt: `build/test-results/live-support-leaves-20261115/validator-L7.json`.

### Live results (`halo2_native.L7`, shared tree, sha256 `68e3322f…`)

**Armory parity** (01a, 300 frames, 1280x720): native, reference
(`HALO2_REFERENCE_MATRIX_LEAVES=1`) and verify (`HALO2_VERIFY_MATRIX_LEAVES=1`)
all give frame `5627a51badf47762` and trace `8a9b58b38bef`. The verify run
checked 2,449 calls of the leaves with children (22A790, 1F8200, 138D90,
179CD0): 362,161 ordered accesses and 15,701 child boundaries identical.
Both censuses give the same pair.

**Census on one binary (L7, Armory, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`;
the reference adds the switches of sections 3 and 4).**

| Run | All translated/frame | Group L's 42 tracked routines/frame |
|---|---|---|
| Reference | 345,367 | 28,259 |
| Native | 263,608 | 1,646 (L6: 3,238) |

This batch's routines per frame (reference → native): 1F8200 1,016.8 → 0,
179CD0 549.0 → 0, 138D90 24.3 → 0, and the pool startup residue (L6 native:
16E450 0.4, 16E4B0 0.5, 32A4E0 2.2) → 0. The reference also restores the
natives 144090 (613.9) and 16EA60 (271.3). The genuine children stay as they
were: group E's 17C370 978.5, 17C140 743.2 and 17C5C0 433.2, and 138D90's
138C20 11.3 and 138510 10.0 (translated); 144090, 1E6980, 16EA60, 17A380,
1385D0 and 1386E0 0 (native or not reached).

**Multiplayer parity** (Coagulation, `tools/perf_4k60/parity.py`, 3840x2160,
cases 4:90, 4:150 and 1:150): reference against native and reference against
verify are identical, `9c14a330…`, `13fa114e…` and `05f31264…`, unchanged from
L2. Receipts: `mp-parity-native-L7.json`, `mp-parity-verify-L7.json`.

**Multiplayer sweep** (L7; every MP map in 1p and 4p, 50 cases, native and
verify roles, the node-rebuild scripted input; receipt `mp-sweep-L7.json`):
no verify difference in any case; 43 cases with identical native and verify
frames, all 43 native frames identical to the L6 sweep; the verify role
checked 470,287 calls of the leaves with children (50,518,331 ordered
accesses, 3,524,786 child boundaries identical); the 3 known
nondeterministic 4p cases (ascension, elongation, lockout) differ; the 4
needle and street_sweeper cases stop in both roles on the known `Source
dynamic model resource unavailable: ... active_camouflage` error. This is the
L2/L5/L6 outcome.

## Remaining translated in group L (honest L7 census, instructions/frame)

| Owner | Census | Note |
|---|---|---|
| 1D6FB0 | 1,601 | group H's children; left alone |
| BAD60, 1E1200 | 23.7, 20.7 | weapon-marker natives' remaining reports (unchanged since L3) |
| 138C20, 138510 | 11.3, 10.0 | 138D90's children (bitmap texel address and 2D level offset), translated; not in group L's list |

### Continuation (group L, after the fourth batch)

Every owner in group L's list is native except 1D6FB0 (group H's) and the
two weapon-marker report residues. Next, in order of value:

1. BAD60/1E1200 (44/frame): find which reports still reach the census (the
   natives' `native_census_detail::skip_report` paths) and make them honest,
   or port the remaining translated piece.
2. 138C20 and 138510 (21/frame), 138D90's children: pure bitmap arithmetic
   (the level offset and the swizzled/linear texel address); they fit the
   leaf framework as plain leaves, and 138D90's children would then run
   inline instead of through the boundary.
3. Group E's 17C370/17C5C0/17C140 (2,155/frame) are reached from 179CD0 as
   genuine children; when group E ports them, 179CD0's routes need no change.
