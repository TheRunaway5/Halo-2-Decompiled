# Partial startup records and the imported hardware-data boundary

The native process-heap platform join now provides the actual startup calls to
107 and113 using the independently compiled pinned primary compatibility
initializers. Both selected original images continue through those calls and
stop before the hardware-info import322 load. This advances the renderer's
process-heap dependency; it does not install the heap or produce shield pixels.
[Implementation](../src/source_process_startup_records.cpp),
[platform join](../src/source_process_heap_platform.cpp),
[prior startup increment](SOURCE_PROCESS_HEAP_STARTUP.md).

## Initializer storage and native ABI

The DPC initializer writes Type, Inserted, DeferredRoutine and DeferredContext
in an existing28-byte record. Importance, list links and system arguments remain
untouched. The timer initializer writes Type, Inserted, Size, SignalState, its
self-linked wait head, zero timer-list links, zero DueTime and Period in an
existing40-byte record. Absolute and Dpc remain untouched. The complete writable
record extent is validated before the first write. No queue, timer, callback or
interrupt dispatch is created by these initializers.
[Partial-record API](../include/halo2/source_process_startup_records.hpp),
[primary extraction and compiler limits](SHIELD_STARTUP_INITIALIZER_CAPTURE.md).

The join accepts only canonical2DC611/target8000006B and
2DC61E/target80000071, their selected-image return addresses, actual record and
callback addresses, zero context and timer type0. The declared native void ABI
consumes the actual16/12-byte frames and preserves all other CPU state. Direct
checks cover wrong caller/target/return/arguments, tail delivery, readonly and
short records, x87/XMM/flags/FS preservation and an explicit active MMX alias.
These are native contracts derived from compatibility-source expressions; they
do not recover original Xbox kernel clobbers, IRQL or scheduler behavior.
[Frame checks](../tests/source_process_heap_platform_tests.cpp).

## Stop before importing unowned hardware data

Immediately before original2DC624/u52E00D4 executes, the production join
requires a retained data owner for the8-byte hardware-info record referenced
by import cell417BCC/u541B66C. The original cell contains encoded80000142;
those four image bytes are neither the hardware record nor a callable service.
Absent ownership raises a typed request before the source instruction callback
or load. A supplied owner must leave the cell pointing to readable retained
storage. Returning from an empty owner still fails before the original load.
No hardware bits or persistent settings are selected by this increment.
[Data-owner API](../include/halo2/source_process_heap_platform.hpp).

## Independent comparisons

The new measurement executes unchanged complete startup/setup/timer-arm bodies
with literal image headers and sections, a patterned stack, explicit FS/CPU/FP
inputs, two flag patterns and three stack alignments in each image. It joins
the compiled primary registry and initializer instructions under the declared
native ABI. The36 cases retain504 original instruction executions across32
sites,372 ordered game stores and408 separately observed primary stores:
48 registry,96 DPC and264 timer stores. Startup and setup stop before hardware
import in24 cases; the12 timer-arm cases retain their actual unresolved149
call. No startup return, hardware payload, heap result or scheduling result is
supplied.
[Measurement tool](../tools/measure_source_process_heap_startup.py),
[capture](../analysis/damage-shields-goal-20261011/process-heap-startup-records-measured/fixture.json),
[native comparison](../tests/source_process_heap_startup_tests.cpp).

The initializer object is authenticated against the prior primary extraction,
then relinked in a separate arena from the registry. Its complete180-byte
instruction stream remains byte-identical after relocation. The fixture,
relocated binary, ELF and symbols independently repeat byte-identically.
Game instructions are not patched. The ordering of compatibility stores is
specific to the retained compiler profile, not a universal Xbox/C++ guarantee.
[Repeat verification](../analysis/damage-shields-goal-20261011/process-heap-startup-records-repeat-verification.json).

A separate native comparison consumes the independently produced32 dirty-record
cases and240 ordered stores, including actual and synthetic callback/context
inputs and timer types0/1. It also compares16 unique original dirty setup
prefixes, each with both an absent and an empty data owner: complete source
trace/stores, partial-record stores/final bytes, pending stack and live ABI.
The repeated admission runs total32 prefixes,480 initializer stores and256
original source stores. Neither run consumes hardware data.
[Dirty-record and prefix comparison](../tests/source_process_startup_records_tests.cpp),
[primary evidence](SHIELD_STARTUP_INITIALIZER_CAPTURE.md).

All20 related CTest checks pass without skips in7.05 seconds. The full native
build and `halo2_native --help` also pass. The validation freezes119 stable
implementation/configuration inputs and preserves all earlier captures and
source snapshots. Concurrent cache/audio/datum headers and the live root
CMake file changed during validation; their observations and dependency records
are retained separately. Complete compiled-header stability or equivalence to
the later whole checkout is not claimed.
[Validation record](../analysis/damage-shields-goal-20261011/process-heap-startup-records-validation.json),
[CTest results](../analysis/damage-shields-goal-20261011/process-heap-startup-records-tests.xml),
[final build](../analysis/damage-shields-goal-20261011/process-heap-startup-records-final-build.log),
[recorded drift](../analysis/damage-shields-goal-20261011/process-heap-startup-records-check-status.json).

The older first-boundary and shutdown-only fixtures retain their historical
owner sets and remain unchanged. The complete damage goal still requires
actual hardware/settings owners, timer scheduling, the process-heap installation,
ordinary allocation, the shield rendering dependencies and the remaining damage
and vitality producers. Passing these bounded comparisons is not live gameplay
or original shield-pixel parity.
[Remaining startup contracts](SHIELD_PROCESS_HEAP_STARTUP_PLATFORM_HANDOFF.md),
[full goal](DAMAGE_SHIELD_COVERAGE_GOAL.md).
