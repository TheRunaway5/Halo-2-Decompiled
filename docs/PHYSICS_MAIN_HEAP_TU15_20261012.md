# Fresh TU1.5 main Havok heap

The missing raw main-heap producer is compiled and connected to fresh native
storage. It uses the existing world runtime's complete constructors, size-class
allocator, fixed arena allocator and free paths. It supplies no captured heap,
constructed allocator record, mode transition or allocation result.

English TU1.5 is the current authority. The selected executable SHA256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
TU1.1 addresses below are correspondence IDs used by the existing native graph.

## Source paths and native ownership

[Complete recovery](../tools/research_physics_main_heap_tu15_20261012.py)
independently locates the unique complete TU1.5 producer `149630`, corresponding
to `1482A0`. All 22 instructions, internal branches and relocated operands are
retained. Prefixes locate candidates; acceptance requires the complete match.

The source requests `106470h` bytes through the actual `33B3AC` wrapper and
kernel allocation import. It advances the returned base by `1000h`, masks to
a page boundary, writes the size/padding at aligned-base minus eight/four, and
publishes global `48605C`. Its return EAX remains the raw base. The failure
branch calls `2DBC2E`; that owner remains required rather than returning an
invented error or success value.

[The new generated runtime](../src/physics_main_heap_runtime_generated.cpp)
lowers only this missing body. [The composition](../src/physics_main_heap_host.cpp)
joins its actual call to the already compiled full virtual-memory wrapper and
native page owner. The existing world graph still owns these complete bodies:

| Original correspondence ID | Source responsibility |
| --- | --- |
| `148190` / TU1.5 `149520` | Construct and publish fixed and size-class heaps |
| `22E4E0`, `2DD590` | Build class mapping, descriptors and allocator state |
| `22E950`, `22EB20` | Size-class allocation and free |
| `22E1D0`, `22E380`, `22E430` | Aligned fixed-arena allocation, free and gap selection |
| `22E400`, `22E420` | Fixed allocator virtual adapters |
| `148A10`, `148A70` | Allocation facade and source address-range free dispatch |

The real constructor selects mode 1. Tests exercise that mode through the
original facade. Size-class tests call the actual allocator entry with a
declared standalone caller ABI. They do not write mode 2, claim the complete
game initializer ran, or supply renderer overflow backing.

The native page boundary is the same explicit host adaptation as the fresh
physics pools: actual anonymous Linux pages, owned 32-bit tokens, page rounding,
and a stdcall ABI preserving GP/FP/flags except EAX/ESP and allocation output
cells. This is not recovered Xbox kernel-clobber behavior. Host failures and
missing external source owners throw. The caller must retain the resource owner
for the lifetime of dependent source state. RAII releases native backing; it
is not the original heap destructor or game teardown.

## Independent comparison

[The original-only oracle](../tools/oracle_physics_main_heap_tu15_20261012.py)
runs unchanged TU1.5 instructions over actual fresh host pages in three address
layouts. Only the actual platform allocation import is adapted. Entry registers,
stack arguments and client payload bytes are declared caller inputs. Every
executed original instruction is checked against the pinned executable and
complete correspondence. No native implementation output builds the fixture.

Each layout constructs both heaps, tests 32 class-boundary sizes from 1 through
8192 bytes, rejects 8193, exhausts real source pages, frees alternating blocks,
reuses them and retires all live blocks. The fixed-heap facade separately fills
the arena to failure, frees holes, refills them, retires all blocks and handles
a null free. No oversized CRT free or renderer fallback result is supplied.

The corpus contains **1,839 calls, 402,072 original instruction observations,
24,114 ordered writes, three actual native allocations and 3,231,744 final
retained bytes**. Native comparison checks full GP ABI, applicable flags,
retained FP state, source write PC/address/before/after bytes, actual resource
arguments and outputs, initial pages, publication globals and complete final
banks. A missing owner stops at the exact original producer call frontier
before publication. Owner destruction is checked to release its native banks.

The corpus reaches 633 instruction sites across the new producer and existing
shared bodies. It reaches 18 of the producer's 22 sites. Its four allocation
failure sites and other unreached shared paths have static evidence, not dynamic
acceptance from this corpus.

The selected source cohort uses the actual production CMake modules and real
memory/image/storage implementations. Existing world bodies are linked from
the independently sealed qword-corrected Release and sanitizer archives; their
complete dependency pins are checked against the frozen originals and current
compiled inputs. No existing heap body is copied into the new generated file.
The initial driver omitted the earlier pool test source and failed CMake
configuration; that unbuilt `source-v1` and its logs are preserved. `source-v2`
adds the compiler-discovered pool test dependency without changing production
C++ behavior.

[Validation](../analysis/multiplayer/sandbox-source/physics-main-heap-tu15-20261012/validation.json)
records Release and ASan/UBSan/leak results for the fresh main heap and prior
fresh pool lifecycle tests. Recovery, generation and the original run are
independently repeated. The audit pins their bytes and selected archive inputs.

## Remaining world integration

This closes fresh main-heap storage and its bounded constructor/client paths.
The encompassing early initializer still needs its original game arena/CRC
producers and renderer overflow storage. The mode-2 transition, occupied
world/body/controller construction and teardown also remain required. The
live match still uses the host terrain mover. This component does not change
the failing Lockout lift or Coagulation vehicle acceptance results, establish
whole-engine parity, or produce an updated game launcher.
