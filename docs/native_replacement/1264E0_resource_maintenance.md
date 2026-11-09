# Queued resource maintenance

The sole compatible TU1.5 `1264E0` owner now defaults to the shared typed
two-pass controller in `include/halo2/native_tu15_resource_maintenance_kernel.hpp`
and its retained caller adapter in
`src/physics/runtime/native_tu15_resource_maintenance.hpp`. Complete original body
and dispatcher authentication protect the binding; the original body remains
available through `HALO2_REFERENCE_RESOURCE_MAINTENANCE`.

The first sweep prioritizes mode1 requests. The second considers the remaining
requests. Both apply the signed eight-record limit, retain the live saved count
and cursor words, and obtain iteration, allocation, scheduling and deletion
results from their real owners. Register continuations are acquired again after
children; no page, job, clock or completion result is supplied by this controller.
The adapter reuses the qualified datum allocator's exact CMP-to-IMUL flag helper
for header-read boundaries.

The registered sanitizer and Release gates both passed:
`build/test-results/native-resource-maintenance-official-20261109/report.json`.
Each configuration verifies 391 native transfers: 96 full returns, 32 genuine
child-entry stops and 263 before-parent-read stops. All 96 parent sites execute.
The native route compares 1,589 complete child-entry GP/flags/SIMD/FP/live-80-bit
packets, full banks, ordered reads and stores, and return/entry cursors. Another
128 retained-reference normal/entry transfers compare 592 packets. Nine
body/dispatcher/prefix mutations and seven admission refusals pass. Proof source
hashes remain stable during each gate.

The sanitizer report is 12,170 bytes, SHA-256
`40446199c01249c38bbd79be76433b1822e1b1794a7ab54935c4ffc7be85de35`;
Release is 12,103 bytes, SHA-256
`93cd6477081039736ce661eb001a688df61bf158e49938032c55e79237b620c2`.
Both are retained under their configuration's
`evidence/native-tu15-resource-maintenance.json`.

Complete fresh primary generation, explicit maintenance binding authentication,
and full raw/specialized CPP syntax passed. The bounded combined receipt is
`build/test-results/native-resource-maintenance-closure-20261109/report.json`;
generation details are in
`build/test-results/native-resource-maintenance-primary-generation-20261109/report.json`.

Page cases mount the authenticated captured `1017D000` owner and original static
mode tables. A real 4,096-byte request succeeds; a `10000000`-byte request fails
the genuine span search against 7,889 total pages. Its `7FFFFFFF` policy budgets
are unchanged. The separate declared inactive-datum array has valid empty salt
words; unchanged `16E770` computes its requested allocation. Normal page cases
use DF0. Parent resident/entry cases retain both DF values, while a small
diagnostic preserves the DF1 original fault at `140277`.

The observer authenticates exact bitmap `TEST` opcodes before correcting their
omitted read metadata. Original `PREFETCHT0` at `140071` still executes and counts
toward source coverage; its advisory operand is not reported as a scalar data
read. No original instruction or child result is patched. Native read-prefix
flags compare completely to physical instructions; retained emulated IMUL's
undefined flags are not used as a prefix oracle and no flag bits are masked.

`126060`, `13FE50`, `16E770`/`16E860`, and `1260C0` remain separately tracked CPU
descendants. Job submission stops at the genuine `1260C0` entry in this physical
proof. The existing `126139` native animation I/O owner reads real bytes and
publishes completion after the read/write; this port creates no substitute
provider. Whole job-return/resource-subtree closure, physical page-fault
atomicity, store/observer exceptions and Xbox asynchronous scheduling are outside
the controller-transfer qualification. The report's conservative
`whole_controller_qualified` field remains false for that broader scope.

All private CMake trees, binaries, original-text copies, page banks and fixtures
were removed. Only bounded reports, source hashes, commands and failure
diagnostics remain. Later changes are outside the frozen r7 engine cohort.
