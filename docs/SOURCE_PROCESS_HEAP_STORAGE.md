# Retained process-heap backing and original constructor

The process-heap storage owner now reserves real Linux address space with
`mmap(PROT_NONE)`, retains that reservation, commits reached page ranges with
`mprotect`, and releases the complete reservation. Source32-bit addresses remain
host-selected tokens. A reservation does not initialize a game heap.
[Resource owner](../src/source_runtime_host_virtual_memory.cpp),
[API and lifetime contract](../include/halo2/source_runtime_host_virtual_memory.hpp).

`SourceRuntimeMemory` keeps one stable bank for the reservation and tracks its
committed pages. Reads, writes and views reject any uncommitted byte. Cross-page
rejections publish no partial reads or writes. New commitment imports actual
host page bytes; repeated commitment preserves source-written bytes and earlier
views. The separate fresh-allocation adapter remains unchanged.
[Retained memory](../src/source_runtime_memory.cpp),
[memory checks](../tests/source_runtime_memory_tests.cpp),
[resource checks](../tests/source_runtime_host_virtual_memory_tests.cpp).

## Compiled original owners

Nine complete authenticated bodies compile into native arithmetic/control flow:
`2DA83A`, `2DA2EB`, `2D9824`, `2D9D19`, `2D96E3`, `2D9803`, `326B74`, `326BAF`
and `326E20`. The957 original instructions retain their actual register/stack,
flags, FS, partial-register returns, stores and calls. The copy body includes
all231 instructions and signed-index dispatch closure. No runtime decoder or
original-code execution is emitted into production.
[Runtime](../src/source_process_heap_runtime_generated.cpp),
[API](../include/halo2/source_process_heap_runtime.hpp),
[generator](../tools/generate_source_process_heap_runtime.py),
[generation manifest](../analysis/damage-shields-goal-20261011/process-heap-generated/manifest.json).

Every body has independently verified TU1.1/owned-u5 correspondence; native
execution in this increment is tested against TU1.1. Generation authenticates
both images and every retained instruction/table operand. Unknown callees remain
required owners with the live caller ABI. Complete lowering is not proof that
every resource-dependent branch has executed. The SEH helpers construct/restore
the source frame; original handler execution and exception delivery remain open.
[Constructor/segment evidence](SHIELD_PROCESS_HEAP_RESERVE_COMMIT_RESEARCH.md),
[SEH/copy closure evidence](SHIELD_PROCESS_HEAP_SEH_COPY_RESEARCH.md).

## Original comparison

The unchanged TU1.1 executable runs in a bounded experiment whose ordinal184
adapter obtains real Linux reservation/commit operations. Unicorn mirrors the
same host pages with matching accessibility. The declared native resource ABI
returns success0 after actual success, preserves other GP/flags, updates the
two source cells and performs the source stdcall return. It does not recover
Xbox kernel register clobbers or failure statuses. Standalone GP/stack/FS-chain
and floating-point controls are explicit inputs; loaded mutable image bytes are
not claimed as live startup state.
[Measurement tool](../tools/measure_source_process_heap_storage.py).

The startup-shaped flags2 request reserves1MiB, commits4KiB, constructs its actual
header/free-descriptor/list prefix, then stops at ordinal291 with the real
source-created frame and lock pointer. Explicit standalone flags1 inputs test
ordinary successful constructor/segment/free-list/SEH returns, rounded/default
extents and invalid-lock failure without supplying any lock result. Separate
copy inputs exercise all alignments and both overlapping directions; they
execute all231 copy instructions.
[Original fixture](../analysis/damage-shields-goal-20261011/process-heap-measured/fixture.json),
[native comparison](../tests/source_process_heap_runtime_tests.cpp).

Native comparison matches426 cases,6,997 ordered stores,10 real reserve/commit
calls and24,576 retained committed heap bytes, plus GP/arithmetic/direction flags,
FP controls, copy/parameter buffers, caller/SEH stack and FS chain.683 of957
compiled instructions execute in these cases; descriptor exhaustion, growth and
other unentered branches remain explicitly unverified. Two independent original
captures repeat byte-identically across770 files.
[Repeat validation](../analysis/damage-shields-goal-20261011/process-heap-repeat-validation.json),
[CTest results](../analysis/damage-shields-goal-20261011/process-heap-tests.log).

Eight related CTest checks pass without skips, covering the new resource/runtime
owners, retained memory, main heap, body storage, pressure and the existing TU1.1/
u5 world arithmetic. Fresh standalone image/component checks also pass:33
authenticated sections/5,746,990 retained bytes, and3 original component stages/
1,287 ordered stores with actual allocation.
[Regression CTest](../analysis/damage-shields-goal-20261011/process-heap-regression-tests.log),
[body/pressure CTest](../analysis/damage-shields-goal-20261011/process-heap-owner-regression-tests.log),
[image check](../analysis/damage-shields-goal-20261011/process-heap-image-tests.log),
[component check](../analysis/damage-shields-goal-20261011/process-heap-component-tests.log).
After rebuilding the related targets, all10 checks also pass together through
CTest without skips.
[Final CTest](../analysis/damage-shields-goal-20261011/process-heap-final-tests.log).

The full native build succeeds after refreshing concurrent weather/voice edits;
`halo2_native --help` exits0. Earlier failed link/configuration attempts are
retained in the evidence logs. This is build/CLI validation, not a live shield
render or gameplay check.
[Successful build](../analysis/damage-shields-goal-20261011/process-heap-refreshed-native-build.log),
[CLI check](../analysis/damage-shields-goal-20261011/process-heap-native-help.log).

The [validation record](../analysis/damage-shields-goal-20261011/process-heap-validation.json)
pins99 source/header files,770 original capture files, binary/log hashes and local
evidence links. It records the independent world-runtime change during this
increment; that source was rebuilt before the final TU1.1/u5 regression checks.

## Remaining ownership

The new platform owner supports only reached aligned nonzero2000 reserve and
1000 commit requests, protection4/ZeroBits0, plus exact full-extent8000 release.
The source can request zero or sub-page growth in other branches; those requests
stop explicitly. Host errors throw without invented Xbox NTSTATUS. Resource
release/recommit/accessibility are separately checked; original commit-failure
and kernel199 return/output semantics are not dynamically established here.
The reached game cleanup discards199 outputs, so the semantic native release
interface returns void.
[Exact request/failure evidence](SHIELD_PROCESS_HEAP_RESERVE_COMMIT_RESEARCH.md).

The subsequent [native initializer join](SOURCE_PROCESS_HEAP_INITIALIZER.md)
continues the original constructor through291 and returns its source-produced
heap after real reserve/commit. The immutable426-case capture above retains its
earlier stop; no installed-global or allocator completion claim follows.

Actual process startup must still own current critical-section acquisition, earlier
shutdown/hardware services, FS/TLS/exception/callback policy and source caller
that installs566F08. This standalone constructor does not install that global.
Ordinary allocation still requires its lock/cleanup/growth owners before the
renderer can allocate its descriptor buffers. Live shield registration, damage
producers, vitality and pixel presentation remain open in the active goal.
[Renderer handoff](SHIELD_RENDERER_PROCESS_HEAP_HANDOFF.md),
[next initializer binding](SHIELD_PROCESS_HEAP_LOCK_INITIALIZER_HANDOFF.md),
[coverage goal](DAMAGE_SHIELD_COVERAGE_GOAL.md).
