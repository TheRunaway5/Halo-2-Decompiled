# Actual MOPP ray setup caller `30B890`

This private typed kernel prepares the real cache, quantization variables and
ray bounds, invokes a genuine native `30AA70` traversal child, and returns
through the actual saved-register epilogue. No production prefix or public
facade change is installed by this component.

The DATA kernel is
`include/halo2/native_tu15_mopp_ray_setup_kernel.hpp`; its raw caller view is
`src/physics/runtime/native_tu15_mopp_ray_setup_frame.hpp`. The kernel computes
inverse scale, native SIMD deltas and normalized bounds from current inputs.
The view preserves lazy collector/code/ray/filter reads, the source's cached
translation/deltas and later fresh ray-end read, the staged nonpop/pop x87
stores, and actual argument/private-stack storage. Genuine child execution
stays outside provider FP isolation. The shared raw scope carries its actual
returned FP state back to the caller.

`native-mopp-setup-frame-full-temporary-prefixes-20261109/report.json` passes
both GCC and Clang sanitizers: all 66 original caller sites, 96 complete returns
and 384 interruptions, 1,152 comparisons / 48,144 complete pre-I/O GP, flags,
XMM, FP and live80 packets per compiler. The unchanged original observations
contain 18,024 packets. Actual ordinary-bank reads, ordered stores, observer
preimages, final banks and the complete return ABI are compared. Each compiler
records 584 genuine native child entries, including interrupted child prefixes.
No child outcomes or register tuples are supplied.

The corpus includes IEEE edge values, precision/rounding/sticky/TOP variants,
dense live counts 0/1/4/7, hostile returning and throwing providers, unrounded
caller SP, ray/cache/code overlap, collector/cache overlap, and the shifted ray
alias that requires the genuine fresh end read. Three compiled defects are
rejected on both compilers: inverse multiplication, replacing that fresh read
with the entry end vector, and widening the one-byte hit store.

Numerical prefixes export their actual in-flight x87 temporaries. The complete
returns still independently prove incoming live80/topology preservation. An
earlier fixture incorrectly required every numerical prefix to have the entry
topology and is retained as a failed receipt; no FP outcome is normalized.
The first fixture also exposed the physical observer's explicit first-two-formal
installation contract; the corrected fixture supplies collection and code in
those declared slots. Both earlier failed owners were removed.

Independent review against the complete Death and AI owners found no remaining
lazy-acquisition, XMM-publication, staged-store or returned-register issue in
this scope. The registered sanitizer/Release pair now passes 2/2, with its owner removed.
The actual caller also passes its nonempty split/recursive MOPP-to-scene-query
smoke on GCC sanitizers: 24 original calls / 96 comparisons / 38,344 complete
packets, 48 actual queries and 16 real hits. The full proof now passes on its own immutable source under both sanitizer
compilers: 1,064 complete caller/query returns + 1,068 interruptions, 6,392
comparisons / 2,319,846 complete packets per compiler, 3,192 real queries and
912 hits. The entire frozen source/build owner is removed. Receipt:
`native-mopp-actual-setup-query-full-20261109/report.json`; source manifest
`2c549466441889810bf8ac86c36c22b1b4bd6c8d801cc6168c515d8d378bf728`. Production default/source authentication, MapPack ownership,
unsupported FP/MMX/provider domains, vector partial-write/page-fault semantics,
public-facade reuse, whole-parent/native-only runtime closure and delivery
remain open. All completed scratch owners are removed, with no large artifacts
retained.
