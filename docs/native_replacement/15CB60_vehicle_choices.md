# TU1.5 vehicle selector DATA kernels

The four Primary owners `15CB60` (light), `15CC70` (heavy), `15CDF0`
(turret) and `15CD30` (flying) now attempt the typed native DATA kernel by
default. Their complete original bodies remain pinned, and the unchanged
instruction-shaped `source_vehicle_choice_native_tu15_internal.hpp` helper
remains the explicit reference path. Its historical native name and shared
call counters do not distinguish DATA execution from reference execution.

`native_tu15_vehicle_choice_kernel.hpp` owns the signed-byte selector,
authored jump-table/reference-row selection, wrapping LCG update and actual
NONE-reference retry loop. The separate ABI adapter preserves lazy caller
slots, real recursive stack frames, saved-register aliases, escaped GP/flags
and ordered memory access. It calls the existing compiled-symbol namespace
owner rather than supplying mapped symbol outcomes. Generic instruction
observers, active traces, unsupported image/backend contracts and
`HALO2_REFERENCE_VEHICLE_CHOICE=1` retain the reference path.

The [final registered GCC sanitizer and Release gates](../../build/test-results/native-vehicle-choice-data-official-final-20261109/report.json)
pass both tests. Each gate compares 164 unchanged-original i386 cases,
328 native normal executions (direct and installed), 934 atomic-provider or
completed-store observer prefixes, every original source site (266/266), and
2,812 full pre-I/O ABI packets for each normal native route. The original
executes 21 real LCG draws, including positive retries in all three families
with distinct alternatives. Floating controls/status, all XMM lanes, caller
80-bit x87 values through depth eight, complete banks, ordered accesses and
stores agree. Native routes execute zero instruction callbacks. Another
328 forced-reference/literal cases and 164 generic traced cases preserve the
full forwarded observation sequence; ten admission refusals, nine complete
body/dispatch drift controls and a rebuilt incorrect LCG are rejected.

The [independent Clang sanitizer gate](../../build/test-results/native-vehicle-choice-data-clang-final-20261109/report.json)
passes the same cohort. [Fresh complete Primary generation](../../build/test-results/native-vehicle-choice-data-primary-generation-fixed-20261109/report.json)
authenticates all four routes, preserves them through operand preparation,
and passes both whole-unit Clang syntax checks (2,026 owners / 174,245 source
instructions). The initial replay exposed only include-layout idempotence;
the binder now preserves an existing include's exact preamble position.

This is bounded source/component qualification. Real page-fault atomicity,
observer preimage-read failures, MCC Cache/symbol-lease integration, other
platforms/images, live vehicle births, whole-frame parity, performance and
launcher delivery remain separate gates. Original instruction outcomes were
not supplied or patched. Private binaries, banks and build trees were removed;
only bounded commands, source hashes and reports remain.
