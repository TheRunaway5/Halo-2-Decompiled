# Native TU1.5 constraint base construction

`native_tu15_build_constraint_base` is the semantic native port of actual
English TU1.5 `329720`. Its production code uses typed identities, declared
memory access and native x87/SSE arithmetic. It contains no guest register
file, instruction decoder, Xbox instruction execution, recorded Jacobian,
recorded cursor or result provider.

## Authority and stable source

The original image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
All 921 instructions are authenticated by
`analysis/physics/native-constraint-producer-inventory-tu15-20261026/base.json`.
The listing SHA-256 used by the independent observer is
`52108e908c380d528a5f3358b930b8675277e3402fdd6d8be8f2e5d803f34c32`.

| Source | SHA-256 |
| --- | --- |
| `src/physics/native_tu15_constraint_base.cpp` | `345217d2f9d74395faaaf8e2f196890daf906fa31a06016f8414887a9294c347` |
| `include/halo2/native_tu15_constraint_base.hpp` | `a712fb4ef236841179520d53e3691cbe67bde0f617b518a881cdb7a1c903e447` |

The original six cdecl arguments become the immutable native request fields
`row_data`, `row_count`, `runtime`, `frame`, `input` and `builder`. The original
reads the two accumulator identities from `input`, and constructs output rows
and schema words using the mutable builder cursors. Source row, runtime and
Jacobian strides remain 32, 28 and 48 bytes; the final optional angular-only row
is 32 bytes. Raw source fields retain their authenticated offset meanings.

The original has no calls. Its native kernel therefore needs no physics
service callbacks. The containing producers `31F770` and `31F860` and their
genuine helper/emitter composition require their own qualification before live
replacement.

## Physical differential result

`build/test-results/native-constraint-base-final-tu15/report.json` is GREEN with
GCC 16.2.1 and Clang 23.1.4. The independent i386 process executes unchanged
original instructions against declared retained input storage; the compiled
native algorithm starts independently from those same inputs.

| Check | Result |
| --- | ---: |
| Original calls and bounded prefixes | 837 |
| Full original calls | 105 |
| Authenticated instruction sites exercised | 921 of 921 |
| Read rejection prefixes | 410 |
| Store rejection prefixes | 158 |
| Completed-store observer rejection prefixes | 164 |
| Original ordered reads | 60,160 |
| Original ordered stores | 21,388 |
| Unchanged original instructions | 333,624 |

The cases cover ordinary and coupled rows, aggregate pair/axis construction,
retained-axis repair including each component choice, radius early exit,
source clamp/zero-threshold branches, signed admission corners, shared bodies,
retained data/output aliases and paired NaNs. Large signed counts are stopped
at a declared genuine first-row store boundary, never allowed to manufacture
rows or run an unbounded synthetic allocation.

Native read traces are checked separately from observers' bookkeeping reads.
Every retained byte, ordered public store and its before/written bytes,
source store PC, x87 control/status and MXCSR match the corresponding original
call or prefix. Source FST/FSTP rejection preserves the pre-store x87 state;
an observer exception after accepted publication retains the actual pop and
status. The 108-byte caller host FP state and host MXCSR are restored.

The suite includes all four SIMD rounding directions with DAZ/FTZ variants,
three source x87 precision settings across the four x87 rounding directions,
and zero or one preserved genuine incoming extended value. It retains the
original RSQRTSS estimate and single Newton refinement, x87 private float
rounding points, ordered vector association and late rereads. Four private
mutations are rejected by both compilers: wrong metadata opcode, reversed
cross product, omitted reciprocal-square-root refinement, and reassociated
basis addition. The association control uses exact cancellation so its result
is observable; the earlier weak-control RED receipt remains in
`native-constraint-base-third-tu15` and is not promoted to acceptance.

## Floating point and interface limits

`build/test-results/native-constraint-base-abi-tu15/report.json` independently
traces every reachable source site. The maximum private x87 depth is seven;
all normal returns have private depth zero, and no incoming x87 value is read.
Consequently this base admits at most **one** preserved incoming x87 value.
The wrappers' separate private-depth-one / preserve-seven bound does not
increase this base's capacity. Wrappers must forward the actual original
incoming depth and enforce this base's limit at construction.

Read/store fault prefixes check control/status, retained writes and preservation
of original borrowed values. The typed FP request does not transport a source
private raw extended temporary on exceptional return. That private error
payload remains open; these results do not claim original exception/unwind
parity. Unmasked FP platform exceptions remain an explicit unsupported owner.
The API declares ordinary immutable argument values and private native cached
locals; guest control-frame aliases are outside this qualification.

This is component proof. It does not establish native producer composition,
production activation, actual MCC data coverage, visible multiplayer physics,
original Xbox CPU identity on another host, or complete physics parity.

## Storage lifecycle

The proof runs through `tools/run_test.py`; both compiler builds use
`tools/run_tests.py` with one build job. The final owner
`.local/test-scratch/halo2-test-kv40hrac` was removed after completion, including
69,939,200 allocated temporary bytes measured after inner build cleanup.
`cleanup.json` records the removal. No new large artifact was retained. Small
reports, commands, source hashes and historical failure diagnostics remain.
