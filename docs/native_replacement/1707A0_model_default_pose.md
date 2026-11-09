# Native model-default pose gather

Actual TU1.5 `1707A0` now gathers each model node's quaternion and translation
from its 96-byte record into a 32-byte pose row, followed by the authored scale
word. The shared typed loop is in
`include/halo2/native_tu15_model_default_pose_kernel.hpp`; a narrow retained ABI
adapter owns caller arguments, saved registers, final flags and cursor. It
tests the initial signed count before entering the copy loop.

Both compatible copies, in primary live support and the AI frame, have
authenticated whole-body prefixes. The common generator installs the same
binding. Selected operand resolution is pinned for the AI copy. Older image
variants and diagnostic/service/observer cases outside admission keep their
explicit reference route, controlled by `HALO2_REFERENCE_MODEL_DEFAULT_POSE`.

The loop preserves seven separate source-word reads and interleaved stores per
row. It reloads the model through the caller's argument cell before publishing
the scale and rereads the node count afterward. Model descriptors, source rows,
outputs and saved-stack words may alias. The scale is a cached bit pattern;
loading signaling NaNs must not perform floating arithmetic or raise flags.

The registered CMake sanitizer gate passes 384 full comparisons against 96
unchanged original i386 cases, covering all 46 sites. GCC Release and Clang
ASan/UBSan independently pass the same cases. Counts include negative/zero and
255 nodes; half of each gate uses real empty observers. Comparisons cover all
GP registers, EFLAGS, XMM lanes, x87 control/status and live 80-bit callers,
MXCSR, complete banks and ordered stores. Ten additional canonical-reference
mutation cases compare the exact read/store histories. Eight admission tests
and five body/resolver drift controls pass. Forced reference execution measures
eight real instruction callbacks, final PC `170813`, and the original EAX.

Both complete fresh generators and syntax checks pass. The primary replay
caught a missing direct SIMD-header include, which is fixed in the final
adapter. The AI replay authenticates 48 existing original reference reports;
two required archived banks were reconstructed and removed only inside owned
scratch. The final official proof's recorded source hashes match the current
kernel, adapter, test, binder and two bound copies.

Evidence and cleanup receipts are indexed in
`build/test-results/native-model-default-pose-closure-20261109/report.json`.
All completed private builds, generated sources, banks and executables were
removed; no large artifacts are retained.

The later retained-fault extension publishes the narrow ABI at typed copy and
memory phases, including each saved-register push/pop, component access and
late model/count reload. It emits no source instruction callbacks. All 418
retained-memory fault prefixes and 66 observer exceptions compare against the
pinned canonical owner in both observed and observer-free fixtures, including
complete partial GP/flags/XMM/FP, live caller values, cursor, banks, access
history and store events. GCC sanitizer, GCC Release and Clang sanitizer
cohorts all pass, and all 384 unchanged-i386 normal comparisons still pass.
The registered combined pose/tick integration gate passes 3/3 after the
physical-epoch dispatcher/library addition. Both current complete source
copies pass syntax with the new phase adapter.

These fault checks qualify the retained memory-service protocol. Physical
page-fault atomicity and implicit stack-read coverage, other images and
whole-engine runtime/visual/performance qualification remain open. This port
is outside the frozen r3 execution-accounting source cohort.
