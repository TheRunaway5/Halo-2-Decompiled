# Native TU1.5 convex contact parent

The semantic `2F8630` parent, native `2E4760` rotation and native `30D350`
result construction pass70 complete physical comparisons per GCC/Clang with
three rejecting mutations. The [qualified report](../analysis/physics/native-convex-contact-query-tu15-20261101-qualified/report.json)
and [aggregate](../analysis/physics/native-convex-contact-query-tu15-20261101-qualified/aggregate.json)
retain commands, exact source pins and results. The sole remaining dependency
is the genuine `30E930` simplex solver; that solver is not yet a native kernel.

The native parent implements all200 authenticated sites: cached separation
projection, early return, relative inverse transform, solver invocation,
contact result/normal construction, threshold selection, contact point updates
and the guarded direction-cache update. Its source arithmetic runs on native
x87/SIMD kernels with original operation and NaN operand order. It has no
virtual CPU, GP file, opcode loop or supplied original return-state image.

`2E4760` has24 sites and preserves packed X+Y+Z rotation, including lane W.
`30D350` has45 sites and retains first/second/both simplex point paths, source
distance, normal sign, interpolation weight and publication order. Existing
native `2E4550` performs the relative inverse geometry. The result builder's
cache argument is unused by the authenticated source.

The70 cases use all seven protected producer entries and ten actual/declared
states: real cached input, forced slow query, closed cached early exit,
projection crossing into the genuine solver, signaling NaN, accepted/rejected
distance thresholds, displaced geometry and both guarded policy-update states.
They compare6,326,070,230 persistent bytes per compiler, the80-byte contact
output,460-byte retained DATA packet, full FP controls/status and4,032 strict
ordered before/read/write events across17,375 original parent/helper
instructions. There are58 genuine solver returns,12 early exits without a
solver and15 valid contacts per compiler. The result builder takes its first
simplex path51 times and its both-simplex path7 times. The second-only path
and a nonzero solver rejection remain unexercised by this cohort.

## Proper DATA provenance

The policy-update direction pointer is source-owned DATA. At `2F878F`, ECX is
the current contact output+40. The two argument PUSHes at `2F878B` and
`2F8792` precede `2F8793`'s write to `[ESP+1C]`, so the destination is DATA+14.
It is not DATA+18, a saved-register postimage or an arbitrary old frame pointer.
The [CFG dominance proof](../analysis/physics/native-convex-contact-query-data-lifetime-tu15-20261101/report.json)
establishes that this writer and completed normal rotation dominate the
direction-pointer read at `2F889D`. Independent variations of the prior word
are overwritten before that use. The first native probe exposed the incorrect
offset; its RED report remains saved and the actual source dataflow was fixed.

The same probe exposed incorrect Capstone access metadata for x87 FST/FSTP
memory operands. They are real writes; the observer now marks them explicitly
as writes. No original instruction, comparison byte or expected bank changed.

The [local context audit](../analysis/physics/native-contact-convex-query-context-tu15-20261101-first/report.json)
covers all reachable paths of `2F8630`,`30E930`,`30D350`,`2E4760`. It finds no
incoming XMM lane read before definition and no GP data dependency beyond the
typed ECX receiver and original test ABI arguments. Authentic prologue saves
are classified separately as control-only. The11-target solver switch and
its byte-index table are authenticated. Calls remain explicit boundaries;
this is not transitive solver/virtual-method liveness closure.

## Complete query composition

The subsequent [full query composition](../analysis/physics/native-contact-query-convex-composed-tu15-20261101-first/report.json)
passes42 cases per compiler across all seven producers and six declared
states, rejecting three query mutations. Query/body eligibility, class getters,
cache initializers, shape center/support/encoding, inverse geometry, convex
parent, rotation and result construction all execute native semantic kernels.
Only37 genuine `30E930` returns remain original; five convex calls exit early.
Each compiler compares3,795,642,138 persistent bytes and6,019 ordered events
across35,427 original parent/helper instructions, including complete query
DATA, contact output and FP state. This remains component composition, not
native simplex-solver conversion, full `304350` raw parity or live binding.

Qualified production sources:

| Source | SHA-256 |
| --- | --- |
| `include/halo2/native_tu15_convex_contact_query.hpp` | `3eb4c9db5f9a4e6306a1c974f7eafd5c3e63b1b2d7219f1bc147bc608fc0dd68` |
| `src/physics/native_tu15_convex_contact_query.cpp` | `920ba32a35fa99c74ab0deba2e4a251e5f2da3fc6128e8d89b8d9a5fdd310fbc` |

Fault/observer prefixes, arbitrary aliases, unsupported FP trap owners,
profiling-clock publication, the remaining native solver and live multiplayer
integration remain separate gates. The complete producer's persistent opaque
constructor lifetime remains open without byte waivers or control metadata.
All authority is unchanged English TU1.5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The final parent and composed query scratch owners are absent:866,615,296 and
866,746,368 allocated bytes were removed. Adjacent cleanup receipts record
the verification. Required old reference inputs remain untouched; no new
large artifact is retained.
