# Native TU1.5 contact-time advance

`native_tu15_advance_island_contact_time` is the semantic native port of actual
English TU1.5 `3205B0`, including all 437 authenticated source sites. It uses
native x87 arithmetic and typed memory/service inputs; production contains no
guest instruction decoder, GP register machine or supplied contact result.

The stable source SHA-256 values are:

| Source | SHA-256 |
| --- | --- |
| `src/physics/native_tu15_island_contact_advance.cpp` | `f35807c5a1eadc22d978d1a4ae4523d1f9a0823432a6c538697c6d00de7b72a8` |
| `include/halo2/native_tu15_island_contact_advance.hpp` | `e8c145cdcfc72dc29be66dee9f1e580113eb8cff0bd9bbfd51b5f48e505a7fc1` |

The original image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

## Component proof

`build/test-results/native-island-contact-advance2-tu15/report.json` is GREEN
with GCC and Clang: 144 cases comprising 37 complete source paths, 54 genuine
motion-entry prefixes, 52 read faults and one store fault. The observer executes
7,436 unchanged original instructions, 2,109 reads and 28 stores. The source
half-factor mutation is rejected. Genuine post-owner-return paths were left
open by that component report; they are qualified separately below.

The original private x87 depth reaches two, so the semantic kernel admits at
most six preserved incoming values. Component tests guard complete 108-byte
host FP state and host MXCSR. Unmasked traps and original platform unwind are
not claimed.

## Real multiplayer BEFORE input

The required reference is:

`analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-input-original-v1461/iteration-00-contact-time-before-vehicle-camera/required-contact-time-before/actual-before-input.tar.zst`

Its SHA-256 is
`3f87636519456e84f8d8bf6cc3754bda002f7dfb42b361bd65fd803c7c4b7185`;
the archive is 11,997,363 bytes and contains 4,033 BEFORE banks totalling
89,795,481 bytes. The coordinator's `retention.json` records complete per-file
hashes and lossless unpack roundtrip. `provenance.json` records the selected
source/runtime and visible Warthog reversal using imported MCC data and the
actual vehicle camera. Captured-after raw banks are not retained or consumed
as expected physics.

The audit in `native-island-contact-advance-input-audit-tu15/report.json`
authenticates the real motion owners `325810` (160 sites), `08FDB0` (RET8), and
the real compound process owner `2F7A50` (40 sites). The captured dynamic motion
has zero source linear vector at +C0 and angular vector at +D0; the second motion
is the no-op owner. Cached-time changes alone therefore cannot move this initial
geometry into an empty-contact single-motion interval.

`native-island-contact-advance-hardware-tu15/report.json` preserves the first
unchanged-input GREEN proof with both compilers: 89,734,533 persistent bytes,
102,845 unchanged instructions, 48 ordered kernel reads, six stores, three
genuine motion returns, two genuine process returns and a paired restore.

## Genuine bisection composition

`build/test-results/native-island-contact-advance-loops2-tu15/report.json` is
GREEN with GCC 16.2.1 and Clang 23.1.4. Both compilers' retained
`evidence/composition.json` independently record:

| Check | Result |
| --- | ---: |
| Complete cases | 11 |
| Compared persistent bytes per case | 89,734,533 |
| Genuine motion returns | 73 |
| Genuine process returns | 47 |
| Single-motion bisection iterations | 14 |
| Paired-motion bisection iterations | 22 |
| Ordered kernel reads | 1,111 |
| Ordered source stores | 141 |
| Unchanged original instructions | 1,855,292 |

The cases include the untouched captured entry and explicitly declared input
variants: equal cached times, the actual source step and its half, the source's
own -1000 restore-classification sentinel, and source linear-vector Z inputs
-8/-30/-60. The latter use authenticated `325810` subtract-old/add-new position
arithmetic to separate geometry naturally; their genuine collision results
produce 3/5/6 single-motion iterations. These variants are not labeled unchanged
live captures. No contact count, distance, success value or returned motion
state is authored or supplied.

Every retained byte, ordered kernel read, ordered store PC/before/written bytes
and complete modeled FP control/status agrees with independent original
execution. Negative paired restore is exercised; negative single restore is
still unexercised by this full-owner input family. The earlier
`native-island-contact-advance-loops-tu15` overall RED remains preserved: its
eight differential cases matched, but its single-iteration coverage gate failed.

## Explicit service and ABI limits

The genuine original callees execute on current native persistent bank inputs,
not recorded outputs. This offline composition also supplies their actual
independent original entry GP/flags/private caller bytes and initial actual
SIMD context. It therefore qualifies the typed math with those dependency
inputs; it does not prove production construction of that private caller ABI.
All actual incoming x87 stacks in this concrete input family are empty.

The declared void API excludes returned callee/argument storage from
`entrySP & ~0xFFFF` through `entrySP + 20`. Caller-owned output/header storage
above that range remains compared. Source private error payload, unmasked
traps, native conversion of the remaining motion/agent services and original
Xbox CPU bit identity beyond this physical host remain separate gates.
Live activation and full world/visible multiplayer acceptance require their
own coherent runtime proof; these component results do not establish complete
physics parity.

## Storage lifecycle

All compiler/reference execution uses `tools/run_test.py` and disposable
`tools/run_tests.py` builds with one build job. The final loop owner
`.local/test-scratch/halo2-test-lkx_1rzk` was removed, including 299,835,392
allocated temporary bytes measured after inner build cleanup. The first
unchanged-input and earlier loop owners were also removed, each accounting for
299,831,296 allocated temporary bytes. The read-only input audit removed
106,827,776 allocated temporary bytes. Each report directory has a cleanup
receipt. The required BEFORE archive remains retained with its coordinator
receipt; no duplicate raw banks or test binaries remain.
