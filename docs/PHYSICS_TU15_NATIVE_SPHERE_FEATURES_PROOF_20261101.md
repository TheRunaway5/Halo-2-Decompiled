# Native TU1.5 sphere features and live refusal diagnostics

Live cohort 03 measured sphere virtual34 owner `2E8D40`; the earlier feature
adapter correctly refused that unported owner. This revision adds its full
21-site semantic body and qualifies the coherent kernel/adapter source.
It does not infer an additional Gemini method from an outer exception frame.

The [component report](../analysis/physics/native-contact-features-sphere-tu15-20261101-qualified/report.json)
passes 147 comparisons per compiler: the prior 98 genuine virtual dispatches
plus 49 explicitly declared original sphere-body invocations using current
arguments/output at genuine solver boundaries. These sphere cases are not
captured Warthog dispatches. All persistent bytes, complete output, FP and
strict ordered reads/writes including before/written values match on GCC and
Clang. Four mutations, including an incorrect sphere origin, are rejected.

The sphere method reads neither shape nor feature IDs. For nonnegative
signed `count-1`, it writes an aligned zero vector then copies forward
`((count-1)&0x0FFFFFFF)*4` words from output to output+16, preserving the
original overlapping REP propagation and its exact per-word accesses.
Counts 0/1/2/3/4/-1 and irrelevant signaling-NaN shape data are exercised.
There is no FP arithmetic, guest register file, instruction decoder or
supplied original output. The adapter requires the ordinary clear-DF ABI.

The first sphere probe retained a strict oracle-ledger RED for count 1:
Capstone lists REP operands even when ECX is zero, but the processor executes
no memory access in that case. The oracle now emits no REP memory event for
zero ECX. This corrects architectural access metadata; no actual byte,
original store or observed operand is removed to force a match. The earlier
diagnostic remains in its report.

The [actual adapter/WorldState model](../analysis/physics/native-contact-features-sphere-handoff-tu15-20261101-qualified/report.json)
passes both compilers, including a three-vector sphere result, a sixth direct
owner counter and explicit refusal of DF=1. The [coherent original-caller
regression](../analysis/physics/native-contact-features-sphere-handoff-full-return-tu15-20261101-qualified/report.json)
also passes 28 roots per compiler with strict persistent/FP/preserved-ABI
comparisons and independent volatile poison. That older root cohort contains
the previously qualified feature shapes; genuine live sphere activation is a
separate integration result, not supplied by the declared component calls.

Unsupported adapter messages now include actual source, target and outer
shape. Native kernel failures include the recursively requested owner,
shape, feature IDs, count and output. Both contexts survive the exception,
so an outer GP dump is no longer used to guess the failed recursive owner.

| Current frozen source | SHA-256 |
| --- | --- |
| `src/physics/native_tu15_contact_features.cpp` | `61943960d90c5d4bf3a366ab9ef533bafcad49c462f55498f884ad7f31a4a816` |
| `include/halo2/native_tu15_contact_features.hpp` | `e9e37d0f66ed2106b0091ab7f2a46c6a5bd92f2bb92a41af041fd953f2113941` |
| `src/physics/native_tu15_contact_features_handoff.cpp` | `eeda132eb95bb5ea11626d4c2b8d092c532d1edcadd9ed367a5a9ee6364e7f20` |
| `include/halo2/native_tu15_contact_features_handoff.hpp` | `5b49e3596d000f9ba3b49edee6816b0eda09d469fffe9bdaa62748d03ef0510b` |

The runtime helper is unchanged at
`ec22763d57f8337467b217587be0ab99c9ccca2f30ed1664d927e5aff7d1ca54`.
Old frozen qualification reports remain distinct. The three final scratch
owners are absent after 866,709,504, 9,314,304 and 866,566,144 allocated bytes
were removed. No new large reference or build artifact is retained.
