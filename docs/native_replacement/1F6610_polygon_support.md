# 1F6610 polygon support selection

The new shared DATA algorithm selects the greatest dot-product vertex using
the source's signed count guard, ordered SIMD reduction and strict first-win
comparison. Serialized vertex/scratch storage belongs to its view. The core
has no virtual registers, flags or source instruction dispatch. The separate
raw view preserves the actual caller frame, all escaped GP/flags/XMM and live
80-bit state, scratch aliases, lazy reads, ordered stores and alignment faults.
The public contact-shape support branch delegates to the same DATA core through
a value view that protects geometric FP controls around providers/observers.

The expanded GCC and Clang ASan/UBSan proof passes 1,059 unchanged-i386 paths:
256 declared cases and 803 read/write/completed-observer prefixes, covering all
46 actual instruction sites and 90 real alignment faults. Each compiler makes
2,630 native comparisons and checks 37,928 complete pre-I/O packets. Four
independently compiled wrong policies are rejected: reversed comparison,
reversed reduction, eager direction acquisition and discarded selected index.
The public API independently passes 512 ordinary and 512 hostile-FP comparisons
and 736 interrupted-prefix pairs against its preserved complete pre-edit value
implementation. Its existing depth/admission domain is unchanged.

Receipts: `build/test-results/native-polygon-support-expanded-20261109/` and
`native-polygon-support-values-first-20261109/`. Their owned trees are removed
and absence verified. No generated binaries, banks or frames are retained.
An independent review against the full actual source found no ordering or
alias defect in the qualified views.

Both compiled source copies remain in scope: Contact's canonical-PC
`run_1f1d20(State&)` and World's actual-PC `run_1f1d20(ObservedWorldState&)`.
Their complete bodies, selected resolvers and dispatchers are authenticated in
`tools/native_polygon_support_replacements.py`; the selected initial bound
really maps `447020` to `44AAC8`. Actual observation PCs are canonical PCs plus
`48F0`. The raw/prepared source-route proof now passes GCC and Clang: each of four
native routes makes 2,630 comparisons with zero retained statements or generic
callbacks; each forced-reference route makes 332 complete-return comparisons
and executes 26,364 independently counted retained statements. A cursor-only
prepared reference correctly has zero generic callbacks while retaining its
positive literal-body count. This is a compiled exact-source route proof,
supported independently by the unchanged-i386 raw proof above.

That check exposed a real legacy fallback mismatch: its `vw` helper split one
MOVAPS into two eight-byte stores. The exact qualified TU1.5 fallback now owns
one aligned sixteen-byte store through the same raw vector adapter. Exact
body-local spellings also preserve immediate-SHL flags and SIMD operand order;
full original body authentication reverses only those enumerated spellings.
Other images and unsupported FP contexts retain their existing store policy.
The initial failure remains in `native-polygon-support-source-first-20261109/`;
the corrected proof is `native-polygon-support-source-ordered-store-20261109/`.
Both owners are removed with no generated large artifacts retained.

Both TU1.5 canonical copies now have authenticated native defaults. Full
477-owner World and 82-owner Contact regeneration is byte-identical; all four
raw/prepared whole-unit syntax checks pass. The exact original observation
baseline and both prepared target hashes are preserved. Nine observation and
eleven specialization regressions pass. The standalone syntax gate caught a
missing direct include hidden by the fixture pre-includes; the Frame now
includes its physics-step store dependency explicitly. This integration
receipt is `native-polygon-support-generation-final-20261109/report.json`.
The final registered sanitizer/Release/full-generation gate passes all three
tests against the installed support and feature bindings. GCC and Clang in
each mode verify 10,520 native source transfers and 1,328 forced-reference
returns. Each source process also proves 92 admission refusals reach the first
real retained statement without changing ABI, cursor, memory or observation
state. Sanitizer runs reject the four compiled DATA-policy mutants. The
production raw/value headers exclude GNU backend definitions before parsing
on unsupported compilers; their retained fallback remains explicit. This does
not constitute an actual unsupported-platform build/runtime qualification.
The consolidated bounded receipt is
`build/test-results/native-polygon-support-closure-20261109/report.json`.
All eight recorded proof owners, including the full official dependency/CMake
tree, are removed and verified absent; no generated large artifacts remain.

The same DATA core separately passes the genuine older MapPack1.1 image in
`native-polygon-support-mappack-first-20261109/report.json`: 1,059 paths, all 46
sites, 90 alignment faults and 2,630 comparisons per compiler. Its private
view owns the original `447020` bound and canonical PCs, with complete live80
and FP/access/bank comparisons. Its owned tree is removed, with no large
artifacts retained. MapPack now has production native delegation through the
same existing entry prefix. Canonical bodies, resolver/dispatcher pins and
specialization spellings remain unchanged. The runtime bridge and qualified
fallback primitives select the genuine legacy bound/PC view by image.

The final registered both-image cohort passes all five tests: TU1.5 sanitizer
and Release, complete 477/82 regeneration, and MapPack sanitizer and Release.
Both compilers run the original-image/source proofs in each mode; two compiled
legacy defects (wrong bound address and wrong PC namespace) are rejected in
addition to the four TU DATA defects. Each compiler/image/mode verifies 10,520
native source transfers, 1,328 forced-reference returns, and 92 untouched
fallback policies per source process. All default routes have zero retained
body work; reference routes have independently positive literal counts.
The consolidated receipt is
`build/test-results/native-polygon-support-both-images-closure-20261109/report.json`.
The complete official dependency/build/oracle owner is removed and absence
verified; no generated large artifacts are retained. Opaque MMX, unsupported
provider policies, full native-only runtime and delivery qualification remain
open.
