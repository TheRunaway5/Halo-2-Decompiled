# Native TU1.5 simplex helper source-call binding

Integration update: the maintained generator hooks and actual World scope are
wired. v1464/04 driving completes4,956 native helper calls, comprising3,886
triangles and1,070 segments, with zero refusal and a strict89,729,945-byte
original match. Rocket04 completes9,499 native helper calls with zero refusal
and114,887,921 compared bytes. Both live kind counters and per-source sums
pass. Gemini also passes the native helper gate but its strict heap comparison
remains red; full-native parent and constructor lifetime gates stay open.
[Live results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-entity-recovery-original-v1464-20261101/iteration-04-summary.json).


The actual scoped adapter passes 28 complete original root returns per
compiler, including every intermediate typed helper return. Its actual
`WorldState` save/load model passes GCC and Clang and rejects three adapter
mutations. The integration owner has a minimal generated-call and maintained
generator patch; production World activation remains a separate gate.

The [physical adapter report](../analysis/physics/native-simplex-helpers-handoff-full-return-tu15-20261101-qualified/report.json)
and [aggregate](../analysis/physics/native-simplex-helpers-handoff-full-return-tu15-20261101-qualified/aggregate.json)
compare all 2,530,428,092 persistent bytes per compiler, complete FP words/
empty tags and preserved root ABI. Every opaque heap/contact byte remains
included. The original solver and all remaining callers continue to `1CCACA`.
All 73 native helper-return DATA/mask/FP stages match before execution resumes.
At 38 calls, independent volatile GP/SIMD/arithmetic-flag poison is installed
after publishing the typed EAX result and normal return contract. No original
register postimage or expected result enters the adapter or semantic kernel.

The only admitted source calls and observed executions per compiler are:

| TU1.5 source call | Owner | Normal return | Calls |
| --- | --- | --- | ---: |
| `30DAD9` | triangle `30D3F0` | `30DADE`, RET12 | 16 |
| `30E3B6` | triangle `30D3F0` | `30E3BB`, RET12 | 8 |
| `30EB85` | triangle `30D3F0` | `30EB8A`, RET12 | 30 |
| `30ECE2` | triangle `30D3F0` | `30ECE7`, RET12 | 8 |
| `30EACC` | segment `312620` | `30EAD1`, RET0 | 11 |

Thus 73 attempts complete, with zero refusals in the physical cohort. The
triangle region-mask mutation is rejected at its typed return even when a
later caller discards the temporary. Separate wider component qualification
is retained for the [137-site triangle kernel](PHYSICS_TU15_NATIVE_SIMPLEX_TRIANGLE_PROOF_20261101.md)
and [190-site segment kernel](PHYSICS_TU15_NATIVE_CLOSEST_SEGMENTS_PROOF_20261101.md).

The [ABI/model report](../analysis/physics/native-simplex-helpers-handoff-tu15-20261101-qualified/report.json)
uses the actual semantic kernels, registry and runtime helper. Its owned
simple triangle and crossing-segment inputs establish the typed results 7
and 0, actual vector publication and normal RET12/RET0 behavior. It covers
memory/executor identity, LIFO policy restoration, scope cleanup, all five
source-call counters, numerical depths through 7 for triangle and 6 for
segments, actual `WorldState` transport, read-only observers and native store
progress. Wrong source/target/tail, missing FP/MMX, overfull x87, wrong return,
argument overflow, scalar-area underflow and active trace are refused.
Observer failure preserves genuine store/FP prefixes without inventing EAX
or finishing RET. Wrong RET, wrong source and wrong mask mutations all fail.
The model's three read-only scalar constants are authenticated against the
pinned English TU1.5 executable; they are operands, never physics outcomes.

The registry owns only memory/executor, immutable admission policy and
optional counters. It stores no register/control image or physics result and
uses no TLS. Active failures throw and never invoke an original fallback.
An absent binding returns false for the original caller's ordinary policy.

The segment scalar area is the genuine source-written numerical DATA at
`alignDown(entryESP-4,16)-0x30`, offsets C through 2C. Its bounds are checked
inside the real stack owner before any native arithmetic. Only nine scalar
fields enter the kernel; saved GP, return and control fields are excluded
from its interface. Exact private DATA bytes are compared at every return.
This is an owned scalar lifetime, not serialization of a historical frame.

| Frozen source | SHA-256 |
| --- | --- |
| `include/halo2/native_tu15_simplex_helpers_handoff.hpp` | `6615071fcf59d9df949bb4656e7e6d863fc2b621c39b465065cb15977e81b112` |
| `src/physics/native_tu15_simplex_helpers_handoff.cpp` | `e02974cef14e2e13c2d621752cef7523ec34fcd1ce19b8bafc015676df86989a` |
| `src/physics/runtime/native_tu15_simplex_helpers_handoff_internal.hpp` | `8e575a9f02c84353e7b9ce2418189aedb49879438cfbf45ec3d157a73664a39d` |

## Integration recommendation

Apply the [five-call generated/generator patch](../analysis/physics/native-simplex-helpers-handoff-full-return-tu15-20261101-qualified/recommended-world-generated.patch).
It preserves every genuine return push and wraps only canonical source calls
`309FE9`, `30A8C6`, `30AFDC`, `30B095`, `30B1F2`, with the five authenticated
TU1.5 source identities above. The helper admits only TU1.5. The generator
authenticates each selected source and immediate target mapping.

Add the two kernel sources and the handoff `.cpp` to the live target. In
actual `run_actual` scope, construct `NativeTu15SimplexHelpersHandoffBinding`
for `bank`, stack `[0x60000000,0x60100000)`, existing read-only-observer
qualification, first-person/weapon-effect trace guards, instruction callback
updating `last_instruction`, and the persistent native activity counter.
Report attempts/completed/refusals, triangle/segment totals and the five
source-call counters. No additional arena is needed. Normal EAX is the typed
mask; all other GP/SIMD and live extended values are preserved, with native
FP words published and ESP advanced by 16 or 4 as the real callee requires.

The component investigation supplied the narrow patch; the integration owner
subsequently wired generated/World/CMake sources and qualified the04 live
paths above. Other source calls, the remaining native solver parent/helpers
and full-native `304350` lifetime remain separate gates. Raw failures
and the preexisting returned-private-stack differences remain explicit;
there is no byte waiver and no original-control postimage adapter.

The physical owner is absent after 866,566,144 allocated bytes were removed;
the ABI/model owner is absent after 9,318,400 bytes were removed. The primitive
composition probes were retired after this actual-adapter regression
superseded them. Exact small qualified source snapshots and hashes remain in
the [retirement receipt](../analysis/physics/native-simplex-helpers-full-return-tu15-20261101-staged/retired-helpers.json).
No new large artifact is retained; required original reference inputs remain.
