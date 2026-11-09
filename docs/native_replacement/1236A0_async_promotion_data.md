# Typed async task promotion

The sole TU1.5 `1236A0` owner now defaults to a typed generation-check,
unlink, priority-publication and reinsert policy in
`include/halo2/native_tu15_async_promotion_kernel.hpp`. The public
`source_async_reprioritize` operation shares that policy. Its existing slot,
chain, generation and lease checks remain in force. The old instruction-shaped
`native_tu15_async_promote.hpp` implementation is preserved byte for byte under
`tests/reference`; the production header now delegates through a retained ABI
adapter. Exact whole-helper and public-owner pins protect that installation.

The adapter retains actual mutex wait/release owners and the qualified typed
queue child. It reads the requested job after the genuine wait, computes and
checks the live generation, preserves lazy predecessor-link rereads, acquires
priority after unlinking, and publishes actual argument/CALL stores. It uses
the child's resumed GP/FP state and the actual resumed BL at return. No lock,
job, insertion or completion result is supplied. The World synchronous I/O
policy is unchanged.

The [registered GCC sanitizer and optimized Release gate](../../build/test-results/native-async-promotion-data-official-20261109/report.json)
passes all five tests. Each physical configuration checks 512 real native
mutex transactions and 1,536 unchanged-i386 segments bracketing those actual
service calls. Complete GP/flags/XMM/MXCSR/x87 control/status, banks, ordered
stores, source access/cursor streams, genuine child packets and exact caller
80-bit values at depths zero/one/four/eight and all TOP positions agree.
Another 267 interrupted-read pairs compare full retained source state through
generation misses, head/middle/tail moves and saved-slot aliases. The fixture
captures source outcomes before test-owned teardown releases any actual host
mutex still held after an injected exception; normal returns must preserve the
unowned checkpoint without teardown intervention.

Fourteen live admission refusals and two independently compiled incorrect
DATA policies are rejected. The exact installed Primary prefix is exercised
in default and `HALO2_REFERENCE_ASYNC_QUEUE=1` modes over another two 512-case
runs. Native instruction callbacks are zero; the reference route forwards the
original observations. The [independent installed Clang sanitizer proof](../../build/test-results/native-async-promotion-data-installed-clang-20261109/report.json)
passes the same controls. A mandatory caller-seeded assertion protects the
live80 fixture against accidentally seeding only admission-guard setup.

The public policy passes 7,168 cases against preserved source controls:
5,376 promotions and 1,792 stale-generation no-ops. Existing retail queue/read
fixtures also pass with sanitizers and leak detection. A separate genuine
file-worker/wait regression passes 72 native/retained waits and 60 original
CPU segments after this installation.

Fresh complete Primary generation retains all 2,026 owners and the exact
promotion body/prefix. Both raw and prepared whole-unit Clang syntax checks
pass. Whole-unit byte equality remains open: 289 unrelated bodies have
pre-existing FP-lowering differences, including FLD C1 clearing and
status-aware single/double spills. Their bounded hashes/diff samples are
retained; no unrelated canonical body, generator or body/resolver pin was
changed to hide that drift.

These are CPU and real-host-service segment proofs, not whole Xbox mutex or
scheduler equivalence. Live World registry/epoch ownership, other images,
unsupported FP contexts, observer preimage failures, write/page-fault
atomicity and arbitrary child/provider exceptions remain separate gates.
All private CPU images, binaries, raw banks, input streams, real file/task
backing and CMake trees were removed. Only bounded reports, hashes, commands
and failure diagnostics remain. The consolidated receipt is
`build/test-results/native-async-promotion-data-closure-20261109/report.json`.
This port is later than immutable catalog r17 and frozen engine R14.
