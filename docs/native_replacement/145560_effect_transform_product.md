# Effect transform composition (`145560`)

The Effect owner now shares `native_tu15_leaves::kernel_145560` with the matrix
leaf owner. It composes the four rows using explicit SSE operand order and the
original 4/8-byte access widths/order. The adapter transports all GP/XMM fields,
preserves full EFLAGS and live logical 80-bit x87 values, and publishes FP state
and cursor before I/O. SSE controls activate at the first arithmetic operation
(`14558B`), preserving earlier memory-failure behavior and restoring host FP state.

Whole helper and actual API route authentication retain the original 69-site
diagnostic body. A genuine hardware comparison exposed a pre-existing NaN payload
reversal in that body: plain commutative intrinsics returned `7FC05678` where the
original returned `7FC01234`. Both the shared native kernel and retained helper
now preserve the original two-address operand order. Diagnostic observations
remain present on the retained route. The binder authenticates the exact original
body and only normalizes the named ordered-operation spelling before hashing;
changed operands or control flow still fail authentication.

GCC ASan/UBSan, GCC Release and Clang each pass 1,632 comparisons: 192 unchanged
physical cases through four native routes plus the retained diagnostic route,
and 336 physical memory/observer prefixes through two native routes. All 69
source sites, full ABI/FP/banks, nine ordered stores, true eight-byte accesses,
implicit RET reads and 43,776 pre-I/O/observer packets across native and retained
routes compare exactly. The corpus includes NaNs with distinct payloads,
infinities, denormals, signed zero, rounding modes, DAZ/FTZ, unaligned/high and
stack/source/output aliases, and live x87 depths up to eight. Six authentication
mutations and nine unsupported admission cases are rejected.

Qualified native paths emit zero literal callbacks. Own-reference, shared-matrix
reference and shared-matrix verification controls each measure an actual refusal,
69 retained callbacks and final PC `14566E`. Unknown/tracing observers, unsupported
FP/MMX, MSVC and non-x86 retain diagnostic execution. Qualified birth-color
collector and empty-store-observer routes are tested through the real Effect API.

The official six-test cohort also passes matrix fault/alignment, specializer and
real emission caller regressions. Evidence and cleanup receipts:
`build/test-results/native-effect-transform-closure-20261109/report.json`.
All owned builds, executables and banks were removed; no large artifacts remain.
Arbitrary observer mutation, partial physical writes/page-fault atomicity and
whole-game/runtime delivery remain open. The frozen r4 runtime predates this port;
its 20,752,371 observations are historical counts, without a measured speedup or
claim that every production caller now admits the native route.
