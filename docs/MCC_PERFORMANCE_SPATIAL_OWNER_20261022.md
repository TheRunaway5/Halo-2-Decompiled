# Hot shadow visibility accelerator owner

Research date: 2026-10-22. Read-only source/original-executable research; no
production edits, game launches, full builds, or new native-performance claim.

## Finding

`44E80 → 2E79A0 → 2E6290` is the original **plane-batched visibility accelerator
query**, using the MOPP bytecode family. It classifies subtree axis-aligned
bounds against the current shadow planes and produces two different outputs:
individual intersecting keys and endpoint pairs for fully contained subtrees.
It already avoids descending every contained leaf. Replacing it with an
ordinary BSP/AABB query, the existing native ray visitor, or a flat list of
visible keys would change its contract.

This descriptive owner/algorithm identification follows the authenticated
instructions and original consumers below. The original proprietary C++ class
or method name has **not** been recovered. `hkpMopp*` names must not be attached
to these addresses as verified symbols. The image contains `HK_SHAPE_MOPP`
at `41C898` and `hkMoppBvTreeShape` at `41D5B0`, but these strings alone do not
identify this particular routine. The format identification also rests on its
complete shared opcode grammar, code at header `+30`, key-base/property state,
and the independently represented MOPP owners, rather than merely opcode
`<=6B`. [Original interpreter](../analysis/map-pack-update/functions/002e2790.c#L68),
[native MOPP grammar](../src/physics/physics_bsp_children.cpp#L76),
[ray grammar](../src/runtime/source_instance_accelerator.cpp#L91).

## Original authentication and current-source coverage

The read-only probe authenticated these supplied inputs:

| Image | SHA-256 |
| --- | --- |
| Owned English TU1.5, `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe` | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| Earlier map-pack update, `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |

Complete CFG instructions, internal branch targets by instruction ordinal and
normalized image operands agree for all four corresponding functions. Each
current generated instruction-PC set exactly equals the TU1.5 CFG set. The
main interpreter's 108-entry opcode-to-target switch topology also agrees.
This is static correspondence, not execution equivalence or a fixture result.

| Earlier owner | TU1.5 owner | CFG instructions | TU1.5 complete span SHA-256 |
| --- | --- | ---: | --- |
| `2E3EA0` | `2E79A0` | 201 | `5fa07e2b44b35790b77bee54932453066cf42e41007dbf0537c6c6e70b9c44a6` |
| `2E2790` | `2E6290` | 825 | `9fcaeca0baf94bf7c75e73b505ebd64bbfbb518e8839c8509a6961485d444674` |
| `305B00` | `309600` | 198 | `5e211a54678501b8d323a0d76396c6bf43a93fb0d895e86a1077c7563be31e4c` |
| `3057A0` | `3092A0` | 204 | `135fd24ed2cb908905708409abe0b1e9cb9367d8fbc3c7d9c5cff423315d5240` |

The `2E6290` complete linear span has two additional unreachable alignment
LEAs; the 825 CFG instructions exclude them. Original bytes, all opcode target
pairs, direct-call sites and current source hashes are in
[`evidence.json`](../build/test-results/mcc-spatial-owner-20261022/evidence.json).
The current generated file hash at inspection was
`cf7341f8fd24518b5eb478e136dcc48ea8517c2a7df4906924c5e2ffca177d42`.
The earlier retail image used by the ray-contact oracle has a different hash
(`03215919…`); that fixture is not represented here as the same executable.
[Generator authentication](../tools/generate_shadow_frame_tu15.py#L41),
[retail ray oracle](../tools/audit_instance_accelerator_original.py#L22).

## Exact caller and output ABI

Let `S` be `44E80`'s ESP at `451F4`, before its argument pushes. Tracking every
push matters: the printed stack displacement changes its effective address.
The call at `4520E` passes, in C argument order:

| Argument | Actual source address/value | Meaning established by callee/consumer |
| --- | --- | --- |
| 1 | `[S+20]` | Selected accelerator header; `+0/+4/+8` origin, `+C` scale, bytecode begins `+30` |
| 2 | `S+44` | `{plane_count, plane_rows}`; this caller sets count 6 and pointer `S+50` |
| 3 | `S+B0` | Descriptor for individually intersecting keys |
| 4 | `S+10C0` | Descriptor for paired contained-subtree endpoints |

Each output descriptor is `{uint32 bank, uint32 count, uint32 capacityFlags}`.
This caller initially sets both counts to zero and both capacity flags to
`80000400` (1024 elements, high bit retained). Their backing starts at `S+BC`
and `S+10CC`. The descriptors persist through the caller's query loop; do not
silently reset or replace their storage. The caller constructs six current
plane equations, including current object transform/scale and the negated
fourth coefficient, before this call.
[Descriptor initialization](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L5065),
[six-plane producer](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L5107),
[actual pushes](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L5202).

`2E79A0` is caller-cleaned (`ret`); `44E80` removes its 16 argument bytes at
`45222`. The recursive `2E6290` has context in ECX and three stack arguments:
six-word key/property/quantization state, bytecode pointer, and a mutable
32-byte min/max-bounds packet. Its return pops 12 argument bytes. It preserves
the source saved-register convention, while its GP/XMM/flags/x87 output state
remains part of the existing full runtime ABI.
[Initializer entry](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L18934),
[recursive entry](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L17351),
[recursive return](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L18153),
[public ABI](../include/halo2/physics_contact_runtime.hpp#L16).

For the initializer's ESP `F` after all three saved-register pushes, its query
context is `C=F+50`. The individual-key descriptor is stored at `C+0`, scalar
inverse scale at `C+10`, contained-range descriptor at `C+20`, plane-batch count
at `C+30`, plane coefficients at `C+40`, and absolute coefficients at `C+240`.
The six-word quantization state starts at `F+18`; initial bounds start at
`F+30`, with minimum zero and maximum broadcast from
`float(float(1/scale)*16777216)`. The fourth row/padding and x87 spills must
follow the actual source instructions, not a mathematically equivalent
rearrangement. Exact constants: `46A0D0=3F800000`,
`41C4E8=4B800000`, `461950` contains four `7FFFFFFF` absolute-value masks.
[Initializer body](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L18938),
[initializer-to-interpreter ABI](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L19119).

The contained descriptor is passed as `458C0/45A70` argument 3 with mode 1;
the individual descriptor becomes argument 4. The consumers read two endpoint
keys, map them through the authored reorder table, expand the inclusive range,
and advance by two. Individual keys are consumed one at a time. This confirms
the pair interpretation from actual consumers, independently of a proposed
native algorithm. [Caller](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L5224),
[range/direct consumer](../analysis/map-pack-update/functions/000458c0.c#L39),
[subpart-expanding consumer](../analysis/map-pack-update/functions/00045a70.c#L67).

## Algorithm, already present optimizations, and mutations

The initializer transposes plane equations into groups of four SSE lanes,
repeats the final supplied plane in remaining lanes, computes absolute
coefficients and accounts for the accelerator origin. Six planes therefore
produce two groups. The traversal interprets shifts/translations, jumps,
key-base changes, axis/subtree bounds, terminal keys and property commands.
It forms `center2=min+max`, `extent2=max-min`, then computes plane distance
and radius in the source's exact SSE operation order. `MOVMSKPS(distance-radius)`
and `MOVMSKPS(distance+radius)` distinguish rejected, fully contained and
partially intersecting bounds. These are sign-bit tests: replacing them with
ordinary comparisons can change negative-zero or NaN behavior. FMA contraction
or changing x87-to-single spills also requires independent proof.
[Original preparation](../analysis/map-pack-update/functions/002e3ea0.c#L43),
[original classification](../analysis/map-pack-update/functions/002e2790.c#L302),
[literal TU1.5 SSE](../.local/mcc-shadow-collision-20261018/2e79a0-v1/2e6290-disassembly.txt#L286).

When fully inside, it calls `309600` and then `3092A0` on the same subtree.
These choose opposite branch descendants, skip classification/bounds opcodes
and append endpoint keys. They are not full key-enumeration visitors.
Partial subtrees recurse or continue in the original order. Existing fast
acceptance/range compression must be retained, not proposed as new work.
[Opposite branch selection](../analysis/map-pack-update/functions/003057a0.c#L116),
[other endpoint](../analysis/map-pack-update/functions/00305b00.c#L26),
[TU1.5 accepted subtree](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L17932).

Every terminal appends the key first and increments its descriptor count
second. At capacity, the source calls genuine grow owner `2E3250` (earlier
`2DF780`) with element size 4 before appending. The three traversals have no
ordinary per-key game callback; their explicit external calls are growth and
the fatal diagnostic path. Reserved/unknown opcodes read mutable global
`48C99C`, call its diagnostic virtual with `unknown command %i\n` at `41C4D4`,
then the retained fatal owners, ultimately the source null store. A native
`throw FormatError`, an empty result, or a fake allocation is not that path.
[Terminal/growth/fatal path](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L18113).

Absence of a normal per-key callback does **not** make scratch writes private.
All generated scalar/vector/stack stores use the source-memory interface;
the live world and continuation register observers that advance mutation
serials for each write. Optional ordered store traces also include their
preimages. `vw` emits two 8-byte stores. Guard callbacks run before bytes are
transferred, and source views can be invalidated by release/rebind/replacement.
Thus replacing source scratch with host locals or using one bulk final memcpy
would change observable write counts/order, possible callback mutations and
fault timing. A prevalidated mutable view only admits access; it does not
deliver write observers. [Store helpers](../src/physics/runtime/physics_step_internal.hpp#L22),
[world observer](../src/physics/physics_live_world_tu15.cpp#L225),
[continuation observer](../src/gameplay/multiplayer/multiplayer_object_runtime.cpp#L676),
[write transfer](../src/runtime/source_runtime_memory.cpp#L439),
[view contract](../include/halo2/source_runtime_memory.hpp#L85).

## Existing native owners and the next concrete seam

| Existing owner | What is reusable | Why it cannot replace this call |
| --- | --- | --- |
| `source_instance_accelerator_visit` | Authored code/header admission and much of opcode grammar | Ray/segment clipping, fraction-changing collector callbacks, retail image; no paired contained-range outputs or retained source scratch ABI. It deliberately skips property effects only for its exact collector. |
| `source_query_physics_mopp_1_1` (`305E40`, corresponding TU1.5 `309940`) | Explicit quantized traversal and ordered source stores, source allocator/diagnostic ownership | Integer bounds classifier, 14-word state, one key output and different scratch layout. Existing tests cover that owner, not the plane query. |
| Generated `2E79A0/2E6290/309600/3092A0` | Complete authenticated implementation and reference path | Still expresses arithmetic, register state and memory/cursor work instruction by instruction. |

[Ray contract](../include/halo2/source_instance_accelerator.hpp#L21),
[ray property exception](../src/runtime/source_instance_accelerator.cpp#L112),
[integer native owner](../src/physics/physics_bsp_children.cpp#L76),
[integer-owner tests](../tests/physics_bsp_children_tests.cpp#L80),
[authenticated integer correspondence](MP_POWER_BSP_CHILDREN_RESEARCH.md#authenticated-tu15-correspondence).

The actionable larger seam is a **dedicated native port of these four exact
plane-query bodies**, at the existing `2E79A0` boundary. It can expose named
quantization/bounds/plane-batch variables and direct SIMD arithmetic while
retaining source memory and explicit source ABI. This is a new port, not an
available switch to an existing native query. Begin with the inner repeated
classification blocks (`2E66A8..2E6748`, `2E6810..2E688C`,
`2E6A44..2E6AE2`), preserving their exact loads, spills, stores and cursor PCs;
these have no direct allocator or diagnostic calls inside them. Keep the
surrounding original VM dispatch, state-copy and endpoint helpers as the
reference until the narrower blocks have differential proof. This reduces the
proof scope before tackling the whole recursive owner. Merely removing its
already source-native SSE intrinsics is not an optimization.

Admission/fallback must be decided **before the first source mutation**:

1. Initially admit only the exact authenticated TU1.5 owner with full real ABI,
   backing and FP controls, and only a native body proved to retain all source
   accesses/stores and final state. Retain the generated path for the other
   image, generic memory providers, unimplemented opcodes or unsupported FP/ABI.
2. Preserve all normal source-memory calls and arbitrary write guards/observers.
   Do not introduce a raw-pointer/unchecked stack path under the assumption
   that ordinary observers are harmless. If a later optimization needs an
   immutable borrowed span or sealed local state, it needs a separate genuine
   owner contract; the current view API supplies no such execution lease.
3. A trace or arbitrary instruction callback requires the complete original
   instruction sequence and its intermediate source state. Keep the generated
   body unless the new body emits that exact sequence. The existing admitted
   cursor optimization checks callback type, executor and live trace flags;
   an entry-only `tracing=false` check is insufficient because traces may change
   during a call. [Existing cursor contract](../src/physics/runtime/physics_contact_runtime_internal.hpp#L115).
4. Growth/diagnostic/missing-backing paths must retain their exact boundary ABI
   and prefix writes. Do not run an optimistic native traversal, observe a fault
   or growth need, then restart the reference on already-mutated source memory.
   Either handle it exactly or decline admission before entry. An immutable
   grammar cache may decline admission on malformed input and use the reference;
   it must not turn an unvisited malformed branch into an earlier source error.

An arithmetic-only substitution that keeps those semantics is safe to develop
incrementally. The tempting larger gain from eliminating all source scratch
writes is **not currently authorized by the memory/observer contracts**.

## Falsifiable experiment and acceptance limits

The historical `optimized-profile-2p.json` ranks `2E79A0` at 19.14% inclusive
and `2E6290` at 18.39%; these nest, so they cannot be added. Its executable
predates the current source. Even eliminating the whole historical 19.14%
would cap CPU speedup near 1.237x in that workload; this is an Amdahl ceiling,
not an achievable forecast or FPS result. Obtain a matching current-source
profile before assigning priority against other owners.
[Historical evidence/context](MCC_PERFORMANCE_NEXT_CPU_OWNER_20261022.md#evidence-provenance-and-source-drift).

Use the unchanged original `44710` oracle on a genuine retained before-bank,
or add read-only entry/return hooks around these four original routines. Freeze
real plane/header/bounds/output owners, full entry/exit GP/XMM/flags/x87/MXCSR,
recursive ordering, all source scratch/final banks, every read/store and external
owner call. Compare canonical generated and proposed native paths in the same
scratch snapshot. Add explicitly controlled negative cases for each reserved
opcode, truncated operand/target, each read fault, protected/uncommitted write,
aliasing packet, capacity boundary/real grow owner and callback mutation. Compare
the prefix state at failure, not just exception text. Generic device reads and
vector two-store granularity remain separate controls.
[Available actual-main oracle](../tools/audit_live_shadow_main_cull_tu15.py#L1).

The existing 146-case `shadow_entry_tu15_original.json` fixture has **zero
instruction hits in all four hot bodies**, freshly checked. Its GP/SSE/flags/
memory checks are useful test machinery, not coverage of this seam. The
integer-MOPP suite likewise cannot certify it. Require isolated matching-source
timing after complete equality, followed by ordinary 1p/4p runtime profiling
with the frozen executable hash and same view/scene schedule. No such execution,
optimization or current FPS acceptance was performed by this research.

## Storage and reproduction

Read-only Python probes ran through `tools/run_test.py` with
`.tools/python/bin/python` (Capstone 5.0.6) and scratch supplied by the runner.
They authenticated image hashes, walked only the four named CFGs, compared the
full switch and current generated PC sets, and hashed the bounded source list.
The standard Python interpreter lacked Capstone; its failed probe created no
retained binary or helper. Existing Oct 18 disassemblies/reference inputs were
read in place and preserved. No active parent build was opened or changed.

Retained new evidence: `build/test-results/mcc-spatial-owner-20261022/evidence.json`
(about 17 KiB) and this report. No new XBE copies, raw source banks, object files,
executables or captures were retained. The evidence lists owned scratch paths;
cleanup verification is recorded separately in `storage-receipt.json`.
