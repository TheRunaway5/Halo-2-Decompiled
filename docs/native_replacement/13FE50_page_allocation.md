# TU1.5 13FE50 page allocation

Status: native default in the complete Primary and Shadow 13FE50 owners.
`HALO2_REFERENCE_PAGE_ALLOCATE` independently selects their retained reference
body. The existing legacy scratch-flags page allocator CPP is unchanged.

`include/halo2/native_tu15_page_allocate_kernel.hpp` owns the complete semantic
algorithm: rounded page demand and policy budgets, the lazy 256-entry caller
ring, three selection policies, bitmap scanning, overlap/victim retirement,
datum allocation and ordered list publication. It reuses the shared stack-probe,
window preference, retirement, datum and bitmap DATA kernels. Its semantic
child resume values retain the real owner, row, handle and predecessor acquired
after callbacks or saved-frame aliases. The thin adapter in
`src/physics/runtime/native_tu15_page_allocate_abi.hpp` publishes GP, flags,
stack and cursor state at genuine I/O/call boundaries. Control and numerical
loops belong to the DATA kernel; no opcode executor is used.

The original complete owner is 388 instructions in a 1,213-byte span, SHA-256
`0221277cd0ba57b61aa637b4e0df13eed97a84db1906b01ba521378d7f8a795b`.

The first official DATA gate passes 61 fixtures and all 388 original parent
sites under ASan/UBSan. It compares the computed return handle or genuine child
stop, complete heap banks, 3,743 ordered heap reads and 1,036 heap stores. Cases
cover all selection policies, fragmentation, capacity/victim paths, failed span
searches, budgets, shift/rounding edges, empty bitmap extents, a full ring and
signed ring/frame feedback. Fifty-four allocations, ten retirements and sixteen
window preferences use shared DATA children. The original independently computes
all outcomes; its instructions and assets remain unchanged.

Six busy-callback cases execute the separately rendered, genuine canonical
12FCB0 test provider. Its 96 translated instruction callbacks remain explicitly
counted; this provider never enters production. The semantic owner/row/handle
inputs agree with the six original entry packets. One associated case stops at
the actual 4057F0 entry without a supplied device result or continuation.
Retirement callback inputs are null in this DATA cohort. These results do not
establish native callback/device closure.

That DATA-only gate uses declared owned descriptors/records with DF clear and real
caller-frame storage. Its test transport composes the shared stack-probe DATA
and saved input words. It does not qualify the complete caller bank, all GP and
flags, FP payloads, full callee-entry ABI, exceptional partial state or physical
instruction/MMIO/page-fault atomicity. Its scope remains separate from the
subsequent complete ABI gates below.

Two ordering gaps in the preceding legacy GP helper were found during the
original-body audit: original 14007A reads the candidate tick before the 14007D
next-index store, and original 14008F reads required pages before the 140093
candidate-pages store. The new DATA owner follows that original order. Migration
of the legacy scratch-flags API needs its own original-backed validation.

Official receipt:
`build/test-results/native-page-allocation-data-20261109/official-cmake/evidence/native-page-allocation-data.json`
(5,477 bytes, SHA-256
`9e796a3f057f4b76647b05f48aebe5458dc1c2141bfbd96b12399530a0693e0e`).
The private CMake owner `halo2-test-jlbwdy3j` and nested test owners were verified
removed. Private DATA proof owners were removed as well. No large generated
artifacts are retained.

The complete ABI cohort now passes 132 unchanged-original hardware transactions
(131 returns and one real 4057F0 entry stop), all 388 parent sites, 23,242 complete
parent-I/O/callee GP/flags/FP/XMM packets, 449 independent canonical child-entry
packets, 18,236 ordered reads and 10,327 stores. Caller and heap banks agree.
It includes 20 genuine initialized-owner/descriptor cases across all 16 caller
alignments, declared DF-set cases, all selection policies and 36 record aliases
over caller saves/live frame fields. Those aliases genuinely change restored GP
words and selected fields; 41 declared memory-feedback events test lazy rereads.

Declared FP inputs include 0/1/4/8 live 80-bit values, arbitrary nonzero XMM
payloads, all three masked precision settings, four rounding modes and DAZ/FTZ.
An owned, authenticated observer copy installs these inputs before the original
entry and samples real GETFPREGS state. Shared oracle files, original executable
instructions and assets stay unchanged; no original result is supplied.

Six direct modes preserve independent parent/Window/Busy/Effect/Datum controls.
Default accepted calls execute zero instruction callbacks or residual children
in this cohort. The ordinary forced-reference path still uses the independently
retained complete canonical bodies. A separately asserted counterexample is
retained: at 13FF45 the canonical SHL model reports flags 68 while original
hardware reports 84 (undefined AF). The native immediate-opcode backend matches
hardware. This disagreement is neither masked nor presented as canonical
intermediate-state agreement.

Eleven selected scenarios pass 5,667 exceptions before atomic Memory-provider
operations and 680 completed store-observer throws, including 222 PUSH provider
prefixes and 74 PUSH observer states. Complete partial GP/flags/FP/live values,
cursor, banks and store prefixes agree with independent original hardware
packets and original ordered-store reconstruction. Parent-owned PUSH commits SP
after a successful write and before its completed observer. These results
qualify that declared provider protocol; they do not establish real signal,
instruction/MMIO/page-fault atomicity or every descendant's exceptional domain.

Both exact production prefixes and their specialized variants pass the 132-case
copy matrix (18 total modes). Actual no-op-hook prefixes provide 22,889 complete
parent-I/O/entry snapshots through the Memory interface; forced child-reference
routes provide 23,203. The single nested-query entry-stop case explicitly uses
a diagnostic trace fallback; ordinary 131-case native routes use zero parent
fallbacks/callbacks. Twelve admission refusals preserve untouched state before
I/O. Seven complete-body/dispatcher/resolver/duplicate mutations are rejected.
The binder pins the full owners, full image resolver and complete dispatchers,
including guarded fresh-FA and exact ten-owner reference contexts.

Model's full owner audit and Root's limited transport review found no remaining
actionable normal-path issue. GCC ASan/UBSan and independent Clang sanitizer
cohorts pass. All four raw/specialized production units pass Clang syntax. Fresh
Shadow generation is byte-identical. Fresh Primary generation preserves the
exact parent, authenticates its fresh-FA dispatcher and compiles; unrelated
whole-file generation differences are explicitly recorded, so Primary whole-
file regeneration equality is not claimed. Every unrelated production byte was
preserved by this two-prefix insertion.

Bound official receipts:

- `build/test-results/native-page-allocation-abi-20261109/bound-official-cmake/evidence/native-page-allocation-abi.json`
  (32,789 bytes, SHA-256
  `d7de26d1b9cad1a6c700cdfbb0dd9f4d95c7511bd67f9b91bd6328596e038275`),
  passed in 438.32 seconds.
- The same cohort's `native-physical-allocation-abi.json` passes 104 dependent
  caller cases (9,936 bytes, SHA-256
  `972d1b5dbbbbed68da3eceb8955d6f69289768a49d37b359276678f33c4aee6e`).
- `build/test-results/native-page-allocation-abi-20261109/clang-alias-provider-prefix-private-validation.json`
  (17,696 bytes, SHA-256
  `bed04484522e87a7b72deb4f0945d3a8e029733e763b2e20698b4fb495e4f53f`).
- `build/test-results/native-page-allocation-abi-20261109/specialization-assertion-cmake/report.json`
  passes all ten specialization tests. The original dispatcher mutation now
  requires the stricter specific whole-owner rejection instead of an emitted
  soft-refusal receipt; no numerical kernel or authentication pin was changed.

The initial combined CMake result was 2/3 because of that outdated assertion;
its passing numerical receipts remain intact, and the focused corrected
specialization gate is separately retained. Private owners `afl2jawf`,
`a1c7jp2g`, generation/syntax/probe owners and nested test trees were verified
removed. No generated binary, raw bank or large artifact is retained.

Associated retirement callbacks through real 12FD10/device/fence/synchronization
owners remain separately counted/open. The full parent DATA algorithm and its
default binding do not claim complete allocator-family child/platform closure,
legacy scratch-flags migration, whole-engine qualification or launcher
promotion. The r9 frozen engine snapshot predates this binding.
