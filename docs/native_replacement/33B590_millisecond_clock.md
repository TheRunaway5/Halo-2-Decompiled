# Shared millisecond clock owner

The three compatible TU1.5 `33B590` owners now default to the shared typed
clock sampler in `include/halo2/native_tu15_millisecond_clock_kernel.hpp` and
its adapter in `src/platform/native_tu15_millisecond_clock.hpp`. Complete body
and selected-operand resolver pins authenticate the primary, input-startup and
combat-action bindings. `HALO2_REFERENCE_MILLISECOND_CLOCK` retains the original
route. The private, unbound lazy-heap adapter delegates to the same owner.

The sampler reads the clock-cell pointer, publishes it in EAX, invokes the real
`33B595` runtime service, then reads the cell selected by the callback's actual
EAX. It preserves all three source observations. Its kernel produces no clock
value, substitutes no provider and makes no host-time call. The existing live
runtime remains responsible for clock refresh and execution-input ownership.

The registered sanitizer gate passed both clock and lazy-heap DATA regressions:
`build/test-results/native-millisecond-clock-official-20261109/report.json`.
The retained clock evidence is
`evidence/native-tu15-millisecond-clock.json` (13,205 bytes, SHA-256
`0da1cd29eb8de3cebe9a44a24fd062e4a13d100eb71e86b34bb18eeae374130c`).
It records 162 unchanged-i386 comparisons across all three compatible copies,
covering all three source sites and 4,011,120 exact result bytes. Complete
GP/flags/SIMD, MXCSR, x87 control/status and live 80-bit values, memory banks,
ordered stores and the return cursor agree. The static original cases have
backed clock cells and an observation-only callback; they supply no clock
producer or time outcome.

Separate live integration performs four actual reads from the existing
`native_tu15_rng_clock().monotonic_milliseconds` backend. The genuine
`SourceExecutionInputs` journal replays those external inputs in twelve native
comparisons, including active tracing, generic callbacks and a cell alias.
Eight actual wrong-executor rejections preserve the same pre-service state.
Six canonical alias/observer supplements and sixteen authentication mutations
also pass. A separate Release physical gate passed the same 162 comparisons.
Both independent read-only reviews found no normal-return mismatch.

Full fresh primary/input/combat generators and complete generated CPP syntax
checks passed; their small receipt is
`build/test-results/native-millisecond-clock-generators-20261109/report.json`.
The current canonical-backed lazy-heap DATA comparison passes 152 transactions,
91,186 child-entry packets and complete parent-site coverage after delegation.
Its declared backed test services do not establish physical whole-controller
or production child closure, and `149930` remains unbound.

Accounting retains every raw count. Only this exact audited shared clock source
hash is classified as native service observations; changed or unknown sources
remain unclassified. Historical engine cohorts retain their original hashes
and counts. Memory-fault prefixes, whole service dependency closure, visual
parity and a promoted runtime remain outside this qualification.

The runner removed all private build, probe, memory-bank and generated-source
outputs. No large artifacts were retained.
