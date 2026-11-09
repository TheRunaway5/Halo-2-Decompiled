# 149930 controller DATA seam

The shared native lazy-heap controller and retained ABI adapter pass a complete
canonical-reference seam comparison in `tests/test_native_lazy_heap_data.py`.
The parent reference is the complete unchanged literal `149930` body, pinned
with STATE-normalized SHA-256
`f01d35461465d3460d347dfcb6a5cc3c3af7a438957fcbecf99856763183192a`.
No instruction or parent branch is removed from the canonical reference.

This comparison deliberately uses declared test-owned child algorithms. The
allocator checks a real backed arena and reservation ledger; cleanup releases
ledger pins instead of supplying allocation result lists. Independent modular
counter inputs drive RDTSC and milliseconds. Context, yield, notification and
trim update separate backed records. The shared qualified tick conversion
computes the elapsed result; timeout decisions come from the actual parent
comparison and writable threshold operand. The clock provider retains its real
`33B595` service callback. These providers are not substitutes for missing
production child ownership and are never installed in production.

The final direct gate at
`build/test-results/native-lazy-heap-data-final-20261109/report.json` passed
76 fixtures / 152 read-and-store transactions under ASan/UBSan/LSan. It compares
91,186 child-entry ABI packets, 147,077 ordered reads, 102,718 ordered stores,
complete backed banks, full return GP/flags/all-XMM/MXCSR/x87 control/status,
80-bit live caller values, final cursor and ten explicit residual owner entries.
All 122/122 parent literal source sites execute in the canonical cohort.
Branch assertions cover backed allocation, cleanup-derived success, repeated
failure, notification, retry 90, elapsed deadlines, current/fallback yields,
request halving and both constructor branches. Fixtures also cover all sixteen
allocator/caller alignments, DF, twelve FP precision/rounding environments,
live x87 depth four, volatile child GP/XMM/status changes, stack/header/global
aliases, lazy frame feedback and a saved-register word overwritten after POP.
Read-only comparisons use no store observer; observer-feedback store cases are
recorded separately in the same fixtures.

Ten declared missing child owners stop before their test body and preserve the
same complete entry state and parent publications on both routes. Arbitrary
memory/division faults and their partial state are not qualified. This is a
controller composition proof against canonical source, not unchanged-hardware
whole-controller acceptance, genuine production child closure, live runtime
parity, performance acceptance or production binding. The separate physical
prefix proof remains in the allocator owner's evidence. The parent stays
unbound pending the owning integration decision and stronger required gates.

The test is registered as `native_tu15_lazy_heap_controller_data` through
`cmake/NativeTu15LazyHeapData.cmake`, with bounded report
`native-lazy-heap-data.json`. Its official CMake ASan/UBSan/LSan gate passed at
`build/test-results/native-lazy-heap-data-official-20261109`; the retained
29,962-byte proof has SHA-256
`e7145cb6243cd67fecadd1753ac51417d754448689473c76de5d24e01d069563`.
All completed private binaries, generated includes,
owned banks and build trees are removed; no large artifacts are retained.
