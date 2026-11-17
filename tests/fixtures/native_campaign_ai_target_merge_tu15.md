# Native target merge273290

`native_campaign_ai_target_merge_tu15.json.gz` is a required offline reference
input from unchanged physical English TU1.5 execution. The image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No instruction patches or supplied child results are used. The fixture is
7,209,581 bytes, SHA-256
`f4418e07aae92a247ac4301746fcbceeed32cf8c28ec04546592eb21bf0ff176`.

273290 takes the destination clump datum in EAX and source clump datum in ECX,
returning with RET0 and no defined scalar result. The native owner marks
existing object targets with the real generation, snapshots the source chain,
validates its salted records, clones eligible targets through274570, associates
their actual actor properties through274670, then merges the source clump's
float maximum, flag union and signed priority/rank fields. The children reuse
the existing native pool, initialization, actor-link and distance owners.

The534 whole-call cases cover all211 original merge instruction sites, all35
BCDF0 sites, all38 clone-wrapper sites, all49 copy sites, and all62 association
sites. Cases include zero/one/multiple actors and targets, duplicate targets,
stale and empty slots, nested object-member chains, allocation refusal,
generation wrap, clump aliasing, signed extrema, NaNs and denormals. They compare
76,919 ordered retained stores, including23,147 stores with exact original PCs,
and3,204 saved-register/ESP/FP assertions. Pre-existing semantic child stores
have no native PC tag; only their explicit original-site allowlist may omit it.
Every retained byte and store order still compares exactly.

The contract excludes dead private frames, volatile registers/XMM, ordered
reads and hardware FP trap delivery. Calls require actual retained x87/FP
state; unmasked SIMD traps refuse instead of borrowing host trap behavior.
Native children receive real pool state and perform actual allocation; there
are no fabricated placement or path outcomes.

An independent original repeat was byte-identical. Eleven production mutations
were rejected; ASan/UBSan passed with the main source and replay driver
instrumented. Child owners have their own qualifications and were borrowed
without instrumentation in that pass. A separate source review found no
actionable mismatch in this contract.

Evidence: `build/test-results/armory-native-progress-20261113/target-merge/closure.json`.
The original binaries, raw banks, mutated sources and probe binaries were
removed with their owned scratch directories. Only the required compressed
reference and small reports/listing remain. Runtime integration and campaign
progress are recorded by the root's separate live Armory reports.
