# Retained original main-heap storage

The native port now has the complete TU1.1 `1482A0` memory/EAX owner over real
platform storage. Its retained allocation can be consumed by the existing
full-ABI `148190` → `22E4E0` constructor, then by the same main heap's allocation,
free and pressure owners. This increment does not initialize a complete game
world or admit an authored power module. The focused native comparison passes.

## Authority and resource boundary

The [raw startup research](SHIELD_RAW_STARTUP_ALLOCATION_RESEARCH.md) uniquely
pairs nine complete owners across TU1.1 and owned u5. The original
[1482A0 instructions](../analysis/damage-shields-goal-20261011/overlay-pass/registration/raw-startup/paired-complete/instructions/001482a0-old.asm)
require `(base0, bytes106470, type101000, protection4)` from
[33785C](../analysis/damage-shields-goal-20261011/overlay-pass/registration/raw-startup/paired-complete/instructions/0033785c-old.asm).
The native [virtual allocation leaf](../src/source_static_allocation.cpp)
separates complete33785C from the existing11E150 wrapper. Only11E150 calls the
last-error getter on its own zero-base branch;1482A0 owns its distinct getter.
This avoids calling the getter twice when composing a raw producer.

[SourceRuntimeHostStorage](../src/source_runtime_host_storage.cpp) obtains actual
anonymous Linux pages and retains their bytes in
[SourceRuntimeMemory](../include/halo2/source_runtime_memory.hpp). Its32-bit
tokens and host page rounding are declared platform adaptation. Host failures
throw at that resource boundary; Linux errno is not translated into guessed
Xbox NTSTATUS or TLS state. Original negative-status conversion and last-error
services remain mandatory when those source branches are reached.

## Producing and retaining the original state

[1482A0](../src/source_main_heap_storage.cpp) computes
`aligned = (raw + 1000h) & FFFFF000h`, writes105470h at aligned−8 and
aligned−raw at aligned−4, then stores aligned at480D90. An already aligned raw
allocation advances by one page. Success returns the original **raw** EAX.
On base0, it calls the actual last-error getter, writes480D90=0 and returns
that getter's EAX. A nonzero EAX therefore does not prove successful storage
initialization. Missing owners retain the preceding source state and throw.
The [contract](../include/halo2/source_main_heap_storage.hpp) represents these
memory/EAX semantics; it does not claim the raw producer's full caller GP/stack
ABI or its kernel wrapper's by-reference stack cells.

The separate full-ABI [world runtime](../include/halo2/physics_world_runtime.hpp)
already owns148190 and22E4E0. It consumes the actual aligned bank to construct
480D9C/480DA0 and the main pool's class tables, freelists, pages and counters.
The existing [22E950/22EB20 allocation/free owners](../src/physics_body_state.cpp)
mutate that same bank. The complete
[148740/22ECB0 pressure owners](../src/object_preparation_pressure.cpp) read its
resulting bytes and the original selected-image constants. Main storage is
separate from generic object-body storage and the512-element component pool.

## Original execution and native comparison

The [measurement driver](../tools/measure_source_main_heap_storage.py) runs
unchanged TU1.1 instructions for raw allocation, construction, varied size classes,
free/reuse, reset and page exhaustion. At ordinalB8 only, it adapts the actual
five kernel arguments to a real mmap, writes both actual output cells and
resumes the original code. Every executed instruction is checked against the
pinned image. Standalone GP, stack, empty x87 and027F/1F80 controls are explicit
experiment inputs. No game allocation return, pressure result, constructed
record or instruction patch is supplied.

The [capture](../analysis/damage-shields-goal-20261011/main-heap-measured/report.json)
contains2 sequences,326 operations and5,254 ordered retained stores. Actual
allocation reaches all four pressure levels and three source page-exhaustion
returns. Freeing blocks preserves the source page count, so pressure can remain
high afterward. The [repeat](../analysis/damage-shields-goal-20261011/main-heap-repeated/report.json)
and all328 captured files are byte-identical in the
[comparison](../analysis/damage-shields-goal-20261011/main-heap-repeat-comparison.json).
Fixture SHA-256: `dd062602ae07a74fce27f6c6f841b001730c08e4a2d441d6f051af861276f1ff`.

The [native comparison](../tests/source_main_heap_storage_tests.cpp) uses fresh
actual host storage rather than copying captured heap state into production.
It checks complete retained heap/global bytes after each operation, ordered
stores, applicable EAX, constructor GP/defined flags, and actual statistic and
pressure bits/SSE status. Missing allocator and actual host token-overlap
failures leave the source globals untouched. All326 operations,5,254 ordered
stores and351,199,800 retained bytes match. Four related checks pass without
skips: component storage, object preparation/pressure, and TU1.1/u5 world runtime.
The [validation manifest](../analysis/damage-shields-goal-20261011/main-heap-validation.json)
pins the source, binaries, captures and logs.

The regular native build and help check passed before a later concurrent menu
catalog update. The final current-tree build then failed in
`compile_menu_world_vertex_catalog.py` with `catalog entry is not actual class1`.
That renderer work was preserved. The final heap test was independently compiled
from its current production sources and the already built unchanged world-runtime
archive, with strict FP controls and unused-function removal. Its
[focused build](../analysis/damage-shields-goal-20261011/main-heap-focused-build.log)
and [comparison](../analysis/damage-shields-goal-20261011/main-heap-focused-tests.log)
pass; this does not claim that the final current-tree native build passes.
The initial separate renderer compile error was only a mixed-type `auto`
declaration; splitting its existing uint16/uint8 declarations preserved behavior.

The initial comparison found a separate oracle limitation: Unicorn's scalar
results matched, but it omitted SSE precision status. Rather than ignoring those
flags, a [Linux ELF32 shim](../tools/measure_source_heap_pressure_hardware.py)
calls the unchanged original22ECB0/148740 bodies directly on the host CPU over
the original captured banks. All324 scalar results agree;309 cases have status
differences from Unicorn. Native SSE status is compared with this
[hardware capture](../analysis/damage-shields-goal-20261011/main-heap-hardware-measured/report.json).
Its [independent repeat](../analysis/damage-shields-goal-20261011/main-heap-hardware-repeated/report.json)
and all6 files are byte-identical in the
[comparison](../analysis/damage-shields-goal-20261011/main-heap-hardware-repeat-comparison.json).
Hardware fixture SHA-256:
`1aca463e8bc475a1c25ebf554cece596f974250236618c520d60b89e2d017b36`.
The shim's stream loads, standalone ABI and RWX source-bank mapping are explicit
experiment adaptation. It does not modify original instructions or use native
implementation output. This establishes host x86 instruction behavior, not
physical Xbox hardware behavior; the original emulator captures remain intact.

## Remaining live binding

This executes TU1.1 dynamically; owned u5 contributes separately paired static
authority. Negative kernel status/TLS error branches are statically recovered,
not dynamically validated by this successful-resource experiment. The raw
producer's non-aligned allocation branch also remains outside its page-aligned
host adapter's measured outcomes.

The initial image's overflow slots are read unchanged in the standalone
experiment. They are not a substitute for the authored root's current-world
overflow state. Complete133760/1360F0 renderer backing and child allocations,
1482F0 overflow construction, actual startup/map composition, lazy1485A0,
clock and reached reclamation services remain required. This experiment does
not complete world bootstrap or validate concurrent platform-service work. See the
[renderer process-heap handoff](SHIELD_RENDERER_PROCESS_HEAP_HANDOFF.md),
[pressure handoff](SHIELD_POWER_MODULE_PRESSURE_HANDOFF.md) and
[damage goal](DAMAGE_SHIELD_COVERAGE_GOAL.md). No live shield registration,
shield pixels, natural gameplay or broad damage/vitality completion is proved.
