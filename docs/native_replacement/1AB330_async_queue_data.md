# Typed async queue ordering and insertion

The TU1.5 comparator `1AB330` (Arena symbol `1A5080`) and insertion
`1232C0` (Arena symbol `122020`, plus the actual Shadow copy) now use the
shared typed algorithm in `include/halo2/native_tu15_async_queue_kernel.hpp`.
The public `source_async_before` and `source_async_insert` operations use this
same DATA policy through their existing queue types. Priority, read-callback
affinity, unsigned file/offset locality, live history publication, traversal
and insertion links belong to that core. It has no register bank, flag
operations or source-PC decisions.

`src/platform/native_tu15_async_queue_abi.hpp` separately preserves the real
caller frame, lazy memory acquisitions, escaped GP/flags, actual child-resumed
predecessor/successor, saved-slot aliases and ordered stores. The public queue
keeps its existing 150-slot, lease, cycle, generation and ownership checks.
The old `native_tu15_async_order.hpp` API is a thin delegate. Its exact former
instruction-shaped implementation is preserved under `tests/reference`;
the generated original bodies remain the production reference fallback.
The exact installer authenticates the complete old helper and both complete
public algorithm bodies, and verifies the unchanged validation owners.

The [registered installed-route gate](../../build/test-results/native-async-queue-data-official-20261109/report.json)
passes all seven tests. GCC ASan/UBSan/LSan and optimized Release each compare
1,024 complete unchanged-i386 cases and 500 atomic-provider or completed-store
observer prefixes. Complete GP/flags/XMM/MXCSR/x87 control/status, exact caller
80-bit values at depths zero/one/four/eight and every TOP, banks, ordered
reads/stores, cursors and 480 genuine comparator-entry packets agree. Native
routes have zero literal callbacks. Twenty-eight live admission refusals and
two independently compiled incorrect DATA policies are rejected. The
[independent installed Clang sanitizer proof](../../build/test-results/native-async-queue-bound-clang-20261109/abi.json)
passes the same cohort. Private direct-adapter tests additionally cover a
hostile optional before-I/O hook; that hook is not part of the installed API.

The public view passes 32,768 comparisons and 5,264 insertions against the
preserved original value algorithm. Existing retail fixtures pass 6,483 queue
cases (3,623,481 exact words, 21 negative ownership controls) and 8,706 read/
protocol cases (112,058 exact words, 11 lifetime controls), with sanitizers and
leak detection. Real promotion integration remains tested through its existing
wait/release and insertion calls. Fresh complete Arena (17 owners) and Shadow
(147 owners) generation is byte-identical and whole-unit syntax passes.

The [registered source-route and resource regression](../../build/test-results/native-async-queue-composition-official-20261109/report.json)
passes all three tests. Six 512-case runs execute the exact authenticated
Arena/Shadow entry prefixes in default and `HALO2_REFERENCE_ASYNC_QUEUE=1`
modes. Their complete output and ordered memory/cursor streams match the
separately physical-qualified retained bodies. This is a source-route proof,
not another original-CPU or imported-dispatcher proof. The actual callback
operand is authenticated before crossing Arena's older observation namespace
into Shadow's identity namespace. The backed resource-controller composition
also passes again in sanitizer and Release after installing the shared queue.

Admission requires the live TU1.5 literal-observation/observer contract and a
supported dense masked FP context. Cleared/rebound callbacks, stale cached
witnesses, active traces, unsupported images and unsupported FP states retain
the original routes. Arbitrary GP-mutating providers, observer preimage-read
failures, physical page-fault atomicity, MapPack, Xbox scheduling, live pending
worker/World epoch ownership, whole-frame parity and performance remain open.
The World synchronous I/O policy is unchanged. At this queue checkpoint,
promotion `1236A0` still had an instruction-shaped controller. Its subsequent
[typed promotion replacement](1236A0_async_promotion_data.md) has separate
qualification and does not retroactively expand this checkpoint's scope.

All original CPU images, private binaries, raw banks, input streams and CMake
trees were removed. Only bounded reports, source hashes, commands and small
failure diagnostics remain. The consolidated receipt is
`build/test-results/native-async-queue-data-closure-20261109/report.json`.
These installed changes are later than frozen engine cohort R13 and catalog
r16; those historical receipts are not relabelled as current execution proof.
