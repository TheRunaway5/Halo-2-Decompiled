# Fresh TU1.5 physics pool construction

The native port can now run the original impact-pool and physical-component
pool constructors against fresh host storage. The compiled paths preserve the
original caller ABI, virtual allocator dispatch, pool headers, bitmap writes,
publication order, reset, salt identity, capacity failure and impact teardown.
They do not import a captured heap or execute original opcodes in production.
This resolves three earlier allocation prerequisites; it does not create an
occupied physics world or replace the live `Simulation::move_unit` terrain mover.

English TU1.5 remains the sole current parity target. The owned executable SHA256
is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The earlier TU1.1 image supplies internal correspondence IDs, not another live
acceptance target.

## Actual source ownership

[Recovery](../tools/research_physics_pool_startup_tu15_20261012.py) starts from
actual writes to the TU1.5 globals and retains complete constructor bodies,
internal branch topology and relocated operands in both owned executables.
Prefixes only locate candidates; acceptance requires a unique complete match.
The complete caller `1CB130` calls `22CDC0` and `1D6EE0` at `1CB170/1CB175`.
Its earlier arena and later main-heap owners are retained as context and are
not executed by this component.

| TU1.5 constructor | Pool global | Capacity | Element bytes | Alignment exponent | Actual allocation request |
| --- | --- | --- | --- | --- | --- |
| `22CDC0`, first pool | `52D68C` | 32 | 160 | 0 | `1450h` |
| `22CDC0`, second pool | `52D690` | 32 | 64 | 0 | `850h` |
| `1D6EE0` | `52BE5C` | 512 | 160 | 4 | `1409Bh` |

The literal image binds allocator pointer `474CB8` to object `486034`, table
`45F048`, allocation `11F3F0` and release `11F420`. These methods execute their
complete virtual-memory wrappers `33B3AC/33B3DA`, reaching actual kernel import
ordinals `B8/C7`. Names come from the selected image's original operands.
The source constructor does not mark pools valid. Validity and reset are
separate operations in the later world initializer.

[Static lowering](../tools/generate_physics_pool_startup_tu15_20261012.py) emits
15 complete native bodies and 561 instruction sites into
[the production runtime](../src/physics_pool_startup_runtime_generated.cpp).
The retained containing caller adds 24 source sites to the research report but
is not a native entry here. Generation also supports the existing raw pool
reset, allocation, unchecked release and full-salt lookup owners. No decoder,
binary blob or result replay is emitted. The current shared world runtime and
its separately corrected qword load are left unchanged.

## Native resource and call contract

[SourcePhysicsPoolHostStorage](../include/halo2/physics_pool_host_storage.hpp)
adapts only the reached TU1.5 allocation/release imports. It requests real
anonymous Linux pages through the existing raw storage owner, retains their
actual bytes in `SourceRuntimeMemory`, and releases the exact owned banks.
32-bit address tokens and page rounding are explicit host adaptation. No header,
salt, validity or physics payload is supplied by that adapter.

The declared native stdcall ABI preserves incoming registers, flags and FP
state except EAX and consumed stack words. Allocation writes actual base/size
outputs. Release leaves its dead by-reference inputs unchanged. This is not a
claim about Xbox kernel clobbers or free output cells. Host failures throw at
the resource boundary; Linux errors are not translated to guessed Xbox status
or TLS values. Unhandled source calls stop with their actual ABI. The negative
comparison verifies that omitting the native allocator stops at the exact
original allocation frontier before publishing any pool.

Native RAII cleanup releases remaining host banks on owner destruction. It is
not the original occupied physical-component destructor or full game teardown.
Callers must retain this owner for the lifetime of all dependent source state.

## Independent execution and native comparison

[The original-only oracle](../tools/oracle_physics_pool_startup_tu15_20261012.py)
executes unchanged TU1.5 instructions. Only actual kernel allocation/release
requests use host resources. Three independently fresh address layouts exercise
the complete declared sequence: creation, validity/reset, filling all 32/32/512
slots, exhaustion, live lookup, alternating retirement, slot reuse, stale-salt
rejection, reset/reallocation, impact disposal, recreation and another disposal.
The standalone incoming GP/XMM/FP/stack state is declared, rather than a claim
that the original game caller produced it.

The corpus contains **3,552 calls, 122,451 ordered retained writes and 27 actual
allocation/release requests**. It reaches 481 of the 561 compiled source sites.
The remaining 80 sites, including unexercised error/debug/name-copy branches,
have static complete-body evidence rather than dynamic acceptance. All 15
compiled entries are reached. Native comparisons check GP, applicable flags,
retained FP state, each write's actual source instruction/address/before/after
bytes, pool globals, resource arguments/results/lifetimes, initial host pages,
and all 258,048 final retained bytes. No native implementation output is used
to construct the original fixture.

The independently repeated recovery, generation and original execution, source
cohort, Release build, ASan/UBSan/leak build and selected test logs are pinned in
[the validation manifest](../analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012/validation.json).
The build cohort compiles the exact new production CMake module with its real
production memory, image and host storage dependencies; it is not a full launcher
build or a live-match test.

## Remaining integration

The original early initializer still needs its arena/checksum and main/overflow
heap producers composed with the later world initializer and actual clock.
Source-created authored bodies, collision/contact/solver delivery and solved
readback must then replace the live host mover. The current visible TU1.5
[acceptance results](PHYSICS_MULTIPLAYER_VISIBLE_TU15_20261012.md) still include
the failing Lockout lift and missing live Coagulation vehicles. This work does
not change those results, establish map-wide sandbox parity, or finish the
active multiplayer/physics goal.
