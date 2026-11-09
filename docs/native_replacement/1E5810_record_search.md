# Native 1E5810 record search

The TU1.5 table lookup now uses a native signed-key binary search in
`src/physics/runtime/native_tu15_record_search.hpp`. It has no child or external
service outcomes. The retained adapter transports the actual saved registers,
caller result cell and RET12 ABI. The arithmetic midpoint uses the signed half
of the wrapping 32-bit bound sum, retaining duplicate choice and the original
endpoint rule. The table base, stride, unequal-comparison key and result cell
are reread at their original semantic boundaries so writable aliases and
observer feedback remain visible.

All three compatible copies are native by default: physics live support, AI
actor and AI frame. `tools/native_record_search_replacements.py` authenticates
the complete retained body. Selected-shaped copies additionally require their
complete selected-address resolver pins; literal-shaped copies have no resolver
dependency. The shared generator installs the same authenticated prefix for
all three modules. `HALO2_REFERENCE_RECORD_SEARCH` retains the original route.
Conflicting real instruction service sites, tracing and incompatible observer
admission retain that route too.

The final prebinding gate at
`build/test-results/native-record-search-final-prebind-20261109/report.json`
passed 360 comparisons across the three production copies against 120
unchanged-i386 original cases under ASan/UBSan. It compares all GP/flags/XMM,
MXCSR, x87 control/status and live 80-bit caller values, complete banks and
ordered stores. Cases include empty/negative/wrapping counts, duplicate keys,
all sixteen caller alignments, zero/negative stride, stack/result/key/header
aliases, DF and caller FP modes. Eight further canonical-reference cases
compare exact ordered reads and stores under observer feedback; six admission
checks preserve the reference/service route. Fourteen body/resolver mutations
are rejected. The native path emits zero translated instruction callbacks.

Fresh primary, AI actor and AI frame regeneration and full-unit syntax checks
passed. Their small receipts are in
`build/test-results/native-record-search-primary-generation-20261109`,
`native-record-search-actor-generation-20261109`, and
`native-record-search-ai-generation-20261109`. The bound official CMake gate
also passed at `build/test-results/native-record-search-official-20261109`,
retaining its final `evidence/native-tu15-record-search.json` proof. The AI frame
replay authenticated two consumed missing
reference banks from the existing archive into owned scratch, then removed
those reconstructions without changing original evidence.

This qualifies the bounded normal-return ABI and store/read-order cases. It
does not establish arbitrary read-fault partial state, whole-engine or image
closure, performance gain, visual acceptance or a promoted runtime. The earlier
15,726,968 statement observations in the eight-workload migration queue belong
to a frozen engine predating this kernel and are not timing measurements.
All completed private probe/generator files were removed; no large artifacts
were retained.
