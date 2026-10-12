# Complete startup bodies and retained shutdown registration

The native process-heap runtime now includes complete original `2D7F8E`,
`2DC5FF` and `2DC5D2`, independently paired to the owned u5 image. These add
169 original instructions to the previous nine-body constructor/SEH/copy graph:
12 complete bodies,1,126 instructions. External owners retain their actual
pending frames. Compiling complete bodies does not establish complete execution.
Regenerating the production body and both manifests repeats byte-identically.
[Generator](../tools/generate_source_process_heap_runtime.py),
[generated correspondence](../analysis/damage-shields-goal-20261011/process-heap-startup-generated/manifest.json),
[runtime](../src/source_process_heap_runtime_generated.cpp).

The selected-image loader now imports each authenticated image's literal8KiB
header as well as section-declared storage and zeroed virtual tails. Startup's
reserve/commit operands remain the actual header words10134/10138. Read-only
header mounting is an explicit native immutable-input policy; original Xbox
header-page protection is not recovered. Importing these bytes does not execute
startup or initialize any process heap, hardware record or pool.
[Loader](../src/source_runtime_image.cpp), [image checks](../tests/source_runtime_image_tests.cpp).

## Native registry and actual game frame

`SourceProcessShutdownRegistry` owns a persistent self-linked head in the same
retained bank as the real source records. Its insertion/removal expressions
follow the pinned primary Cxbx47 body and list helpers: descending signed priority,
FIFO equal priorities, exact intrusive write order, identity-based first removal,
no-op absent removal and unchanged removed-record links. It does not deduplicate
records. The actual duplicate insertion produces its measured cyclic topology;
subsequent native traversal rejects that malformed domain explicitly.
[Registry](../src/source_process_shutdown.cpp), [ownership API](../include/halo2/source_process_shutdown.hpp),
[primary list and synchronization evidence](SHIELD_PROCESS_HEAP_STARTUP_PLATFORM_HANDOFF.md).

A host mutex serializes registration. This is a declared native contract, not
recovery of Xbox/Cxbx PCR.IRQL, interrupt masks, pending interrupt dispatch or
callback execution. Destruction releases the owned head and does not invoke
shutdown callbacks. The pinned compatibility callback-draining loop is disabled;
it cannot establish an active shutdown lifecycle.
[Primary limitations](SHIELD_PROCESS_HEAP_STARTUP_PLATFORM_HANDOFF.md).

The platform join accepts only the actual selected startup47 call, return and
arguments: canonical2D7FA5, target8000002F, source record469228/u546E2E8,
Register1, and actual return2D7FAB/u52DBA5B. Its declared native void ABI consumes
12 stack bytes and preserves the other live CPU state. Wrong caller/target,
return, arguments, tail delivery and foreign-bank ownership are checked before
registration. Other reached services keep their real ABI and required boundary.
[Join](../src/source_process_heap_platform.cpp),
[direct ABI checks](../tests/source_process_heap_platform_tests.cpp).

## Independent original and primary comparisons

The original measurement executes both complete selected-image entries without
patching game instructions. Explicit standalone inputs comprise a patterned stack,
FS exception-chain head, GP/XMM values, empty x87 state and controls; each entry
runs with two initial flag patterns and three stack alignments. A first capture
stops at each real first platform call, before supplying any platform return.
Its36 cases contain228 ordered game stores.
[Measurement](../tools/measure_source_process_heap_startup.py),
[first boundaries](../analysis/damage-shields-goal-20261011/process-heap-startup-first-boundary-measured/fixture.json).

A second36-case capture joins the independently compiled primary registry
expressions with the explicitly declared preserving native void ABI. Complete
startup then reaches the next real DPC initializer107. It contains300 ordered
game stores and48 separately observed primary list stores. Native comparison
checks the instruction sequence, ordered store addresses/old/new bytes, actual
pending call frame, GP/XMM, defined arithmetic/direction flags, FS/FP state and
final stack, source-record, registry, DPC/timer and heap-global banks. Standalone
setup and timer-arm entries still stop at107/149. No hardware, settings, heap or
timer outcome is supplied.
[Joined capture](../analysis/damage-shields-goal-20261011/process-heap-startup-shutdown-measured/fixture.json),
[native comparison](../tests/source_process_heap_startup_tests.cpp).

The separate list oracle compiles the actual pinned registration/helper/type
expressions, excluding only logging and IRQ raise/lower wrappers. It independently
captures24 signed-priority/permutation cases,288 operations and768 list stores,
including absent/repeated removal, stale removed links, equal priorities,
INT_MIN/INT_MAX and duplicate insertion. Native comparison uses these produced
bytes and order, rather than a handwritten expected list algorithm. Both startup
captures and the complete primary capture independently repeat byte-identically.
Earlier captures from the measurement tool's prior documentation revision are
preserved separately; only their tool hash differs.
[Primary oracle](../tools/oracle_source_process_shutdown.py),
[primary fixture](../analysis/damage-shields-goal-20261011/process-shutdown-measured/fixture.json),
[native registry comparison](../tests/source_process_shutdown_tests.cpp).

All18 related CTest checks pass without skips in7.30 seconds, including the
earlier constructor fixtures and both world-image regressions. The full native
build succeeds and `halo2_native --help` exits0. Goal implementation inputs stay
stable through the final checks. Concurrent frontend/audio/configuration edits
required rebuilds; their revisions and the actual configured CMake snapshot are
recorded separately. Equivalence to every later edit in the shared live checkout
is not claimed.
[CTest log](../analysis/damage-shields-goal-20261011/process-heap-startup-tests.log),
[JUnit results](../analysis/damage-shields-goal-20261011/process-heap-startup-tests.xml),
[final build](../analysis/damage-shields-goal-20261011/process-heap-startup-final-configuration-build.log),
[CLI check](../analysis/damage-shields-goal-20261011/process-heap-startup-native-help.log).

The [validation record](../analysis/damage-shields-goal-20261011/process-heap-startup-validation.json)
pins119 goal source/configuration files, complete repeated captures, generated
bodies, primary expressions, native artifact hashes and final build/check logs.
The validation covers bounded original game execution and declared native
platform semantics; it does not prove original Xbox kernel clobbers, IRQL or
physical hardware behavior.

## Remaining dependency and goal scope

The historical startup comparison above stops at107. The subsequent native
initializer increment now continues through107/113 and stops before actual
imported hardware data322,
conditional persistent-setting query24/status conversion301 and timer149. The
source constructor return and actual566F08 installation occur after those owners.
No zero hardware record, settings preset or successful timer return bypasses
them. Later startup also contains further external owners.
[Next owner contracts](SHIELD_PROCESS_HEAP_STARTUP_PLATFORM_HANDOFF.md).

Independent research retains32 dirty-record initializer cases and16 original
setup prefixes across both images. Production initializers now compare against
those bytes and a new36-case complete startup capture, preserving untouched
record fields and stopping before the hardware-import load.
[Initializer integration, evidence and limits](SOURCE_PROCESS_STARTUP_RECORDS.md).

The previous actual reserve/commit plus critical-section constructor comparison
remains separate evidence. Ordinary renderer allocation additionally needs the
real current-thread, contended wait/wake, FS20 callback and reached growth/error/
exception owners. This increment does not install the process heap, allocate
renderer descriptors or produce original shield pixels. All damage sources,
shared health/shield behavior and complete visual validation remain the active
goal.
[Constructor evidence](SOURCE_PROCESS_HEAP_INITIALIZER.md),
[renderer obligations](SHIELD_RENDERER_PROCESS_HEAP_HANDOFF.md),
[full goal and completion gates](DAMAGE_SHIELD_COVERAGE_GOAL.md).
