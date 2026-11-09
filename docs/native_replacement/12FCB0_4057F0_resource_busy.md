# TU1.5 resource busy queries

Status: native default for complete 12FCB0 and 4057F0 CPU owners. Explicit
reference routes remain available through `HALO2_REFERENCE_RESOURCE_BUSY`.

The shared DATA kernels in
`include/halo2/native_tu15_resource_busy_kernel.hpp` own object policy and the
resource/header/fence algorithm. The retained adapter publishes actual GP,
flags, stack and cursor state at real access boundaries. It reads the declared
device counter/timeline backing; it never supplies a completion result.

12FCB0 is 27 instructions/83 original bytes, SHA-256
`1df96e347163f4151ccb82bf996d1aea2abae0e06e9c425b1525e2b450b4861a`.
4057F0 is 48 instructions/130 bytes in the original **D3D section**, SHA-256
`26b21d4d8c3d411324b2a5c9f96dff30ef8103fe860c412947cdd57bf6cd6e8b`.
The physical fixture maps the unchanged original executable sections, including
the original SDK global at 411568. Its local ELF layout extension changes no
executed instruction. Whole original image/section hashes and layout/observer
hashes are retained in each proof receipt.

The binder authenticates complete bodies and complete image resolvers in all
three production bodies: both DeathCycle functions and Combat's 4057F0. The
common generator preserves these bindings. Actual-owner dispatch has a separate
authenticated image group, activity map and receipt. Existing Raw, Tick, Epoch,
Datum, BSP and Stack groups and their reference controls are preserved.

The registered physical gate passes 470 transactions: 168 normal returns,
300 read-prefix comparisons and two unresumed instruction-entry stops before
unbacked counter/device operands. All 75 original sites are covered. The direct
adapter compares 3,628 complete pre-I/O GP/EFLAGS/XMM/x87-control/status/MXCSR and
live-80-bit packets, 2,721 ordered reads, 244 normal stores, complete banks and
return/cursor state. Inputs cover all 16 alignments, both DF states, saved-word
aliases, type/parent decisions, unsigned wraparound and 0/1/4/8 live x87 values.
The original computes every returned result; original instructions are neither
patched nor replaced with supplied outcomes.

Each complete route also passes the retained Memory/observer exception model:
249 baselines, 1,680 injected Memory prefixes and 112 observer throws. Partial
GP/full flags/FP/XMM/live values, banks, accesses, stores and cursor agree with
the independently retained complete canonical bodies. This model does not
establish physical instruction/page-fault atomicity. The two unbacked stops are
entry-state comparisons, not resumed hardware faults or proof of signal state.

Thirteen mode checks cover direct, both source units and actual dispatch,
forced reference, plus Page/Raw/Tick/Epoch/Datum control isolation. Source and
actual routes compare 518 complete child-entry packets (304 in the Combat-only
subset), final ABI and ordered I/O/banks. Direct template hooks provide the
3,628 pre-I/O packets; production prefixes use a no-op hook and do not claim
those hook deliveries. Admitted native routes execute zero instruction
callbacks, translated fallbacks or reference query calls. Five complete
body/resolver mutations and two dispatcher mutations are rejected.

Independent source review found no actionable mismatch. Both complete
production units pass Clang syntax checks. The official sanitizer CMake cohort
passes ResourceBusy, PageAllocateData and operand specialization (3/3); the
same registered ResourceBusy proof separately passes optimized non-sanitized
Release (1/1).

Bounded receipts:

- `build/test-results/native-resource-busy-20261109/official-cmake/evidence/native-resource-busy.json`
  (25,477 bytes, SHA-256
  `8f1b9cfc57b26e006d219a3076f27c5e9240c0a17ac3433ee86e4ab79d7164b7`).
- `build/test-results/native-resource-busy-20261109/release-cmake/evidence/native-resource-busy.json`
  (25,411 bytes, SHA-256
  `3d15d4bc815badca6fded3062332040b61776c685955716f4209dd697cd154e0`).
- `build/test-results/native-resource-busy-20261109/full-source-syntax.json`
  (1,171 bytes, SHA-256
  `57f13dcb2959a0050d38189f2b381e812175b0605bc219e6d13b39203057dc74`).

Private owners `wqpjek24`, `5qr4yflq`, `k2h6uueq`, `ytmvw97d` and their nested
test scratch were verified removed. No generated binaries, raw banks or large
artifacts are retained. Device/timeline backing ownership and freshness,
physical MMIO and physical instruction/page-fault atomicity remain separate
qualification items. These tests do not establish complete 13FE50/12FE00 or
whole runtime/launcher closure.
