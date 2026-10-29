# Campaign behavior-start broadcast, TU1.5

`native_campaign_ai_query_start_tu15` implements the complete 52-site
`1BD9D0` owner with its real `2787F0 -> 278720 -> 278A50` native dependency
closure. These four original owners total 186 instruction sites. The source
contains a structured linked-member loop, typed actor/query arguments and a
typed count/publication result. There is no virtual CPU, source-stack
construction, binary execution, generated fallback or supplied success.

The actual original entry takes four stack arguments. Only actor and query
are read; it returns low16 reservation count and consumes all four arguments.
The two unused arguments have no synthetic native counterpart. Captured actor
pool ownership, initial self-row ownership, perception-group lookup through
`50EEF8` (stride `50`, head `+18`), each captured-pool rows-pointer reread,
and next-member reads before reservation follow the original source order.
Self is compared by row address, including when datum salt differs or rows
ownership changes. Each non-self member calls the real native reservation
with priority 3 and unchanged expiration bits `3F800000`.

`NativeCampaignAiQueryStartResult` returns `reservations` and `did_reserve`.
The latter is an actual publication fact for ABI result transport: any nested
reservation success leaves original XMM0 `[3F800000,0,0,0]`; no success leaves
the incoming SIMD state. No FP arithmetic occurs. The native algorithm does
not synthesize unrelated scratch registers or the original private counter
stack slot.

The offline proof executes unchanged authenticated TU1.5 text on the physical
i386 CPU, with SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Actual original `1BD9D0` calls its real `2787F0` dependency and returns naturally;
the observer supplies no instructions, registers, results or resumed unknown
calls. GCC and Clang ASan/UBSan compare 35 complete original calls, 6,192
instructions, 1,019 ordered mutable reads and 135 writes. All 186 original
sites are authenticated and reached. Retained banks, read/store interleaving,
individual stores, low16 result, original nonvolatile/return-stack contract,
SIMD, occupied 80-bit x87 values and FP controls/status are compared. Native
integer calls preserve host FP controls/status. Mapped finite chains and
masked FP environments are covered; unmapped faults, dead scratch registers,
unmasked traps and the complete outer `278460` entry are not claimed.

Five semantic mutants are rejected: skip self by datum instead of row,
snapshot the mutable rows pointer, read the next link after reservation,
change requested priority, and change the actual expiration bits. Alias cases
publish the rows pointer or overwrite the current next link during a real
reservation; these are data outcomes, not only additional observation counts.

The production module is `cmake/CampaignNativeAiQueryStart.cmake` and reuses
the proved scheduler and single shared data-array helper target. Focused
module validation compiles production and test sources, runs the provided
native executable against the physical original, exercises missing-reference
skip77 (`passed:false`, `validation_not_run:true`) and rejects a wrong image
pin. Reports are in `analysis/campaign-native-query-start-20261028/gcc-v2`,
`clang-sanitizers-v1`, and `module-integration-v1`. Official disposable
wrappers remove raw banks, reference ELF, private mutation sources and builds;
`cleanup.json` records removal and prior-source pin preservation.

The real descriptor's cancellation path `278310 -> 1BD8F0`, actual getter
closure, and full `278460` caller remain separate open dependencies. This
module closes behavior-start broadcast only and enables no larger campaign
readiness gate.
