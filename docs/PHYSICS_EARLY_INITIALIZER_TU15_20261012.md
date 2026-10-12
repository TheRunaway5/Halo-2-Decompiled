# Fresh TU1.5 early physics initializer

The native early initializer now connects source-created game-arena state to
the CRC, impact-pool, physical-component-pool and raw main Havok storage owners.
It reaches the original renderer-overflow call with the same pending ABI as
unchanged TU1.5 execution. It does not return from that call or fabricate its
renderer storage.

## Complete source and joins

[Recovery](../tools/research_physics_early_initializer_tu15_20261012.py) and
[generation](../tools/generate_physics_early_initializer_tu15_20261012.py)
retain two complete bodies, 45 instruction sites and their selected operands:

| Correspondence ID | TU1.5 entry | Responsibility |
| --- | --- | --- |
| `1C5F00` | `1CB130` | Four-byte arena reservation/CRC, impact and component pools, raw main storage and overflow call |
| `1F15B0` | `1F5EA0` | Guarded `1F40h` (8,000-byte) arena reservation and CRC |

Whole normalized bodies, internal branches and relocated operands authenticate
the pairing; prefixes only find candidates. Production is static C++ with
full retained ABI and explicit external calls. Independent recovery and
generation reproduce exactly.

[The native composition](../src/physics_early_initializer_host.cpp) binds CRC
to the new arena runtime, both pool constructors to the complete pool runtime,
and the main raw producer to its complete runtime and virtual-allocation
wrapper. Eight actual call sites, targets and selected return frames are
[exported from original instructions](../tools/export_physics_early_initializer_frames_tu15_20261012.py).
The main producer's allocation-error/TLS owner remains required on failure;
this successful-resource corpus does not execute it.

Four actual native allocations request 5,200, 2,128, 82,075 and 1,074,288 bytes.
Original instructions construct pool headers, names and raw main-storage
alignment records over their real rounded pages. No header, valid flag, object,
allocation cursor, arena pointer or checksum is imported from a capture.

## Original execution and native comparison

[The original-only oracle](../tools/oracle_physics_early_initializer_tu15_20261012.py)
first executes physical, async and arena startup. The source worker uses the
explicit cooperative schedule validated with the arena component. Actual host
resources own the physical and virtual requests, synchronization and two files;
no source instruction, callee result or source field is patched.

It then executes the early initializer to the pending `149680` renderer call,
followed by a separate declared call to the already-ready guarded arena client.
This is **five phases, four returned calls and one required-owner frontier**.
The encompassing early initializer's final pops/return are compiled but are not
executed in this corpus.

The unchanged run contains 1,066,041 original instruction observations and
executes 834 source sites.
[The native test](../tests/physics_early_initializer_tests.cpp) matches
**1,049,789 ordered writes, complete phase GP/applicable-flags/FP ABI, actual
page request/results and all 53,842,968 final retained bytes**. Both real
savegame file extents also match.
Independent original execution reproduces identical files and metadata.

The native test retires the idle async worker before the synchronous pool owner
accesses the same retained bank. The original reference worker is parked with
no remaining queued file jobs. Stop/join produces no compared game-state
mutation; this is declared native lifetime adaptation, not original worker
return, original world teardown or concurrent live-world initialization proof.
The physical bank remains owned throughout construction. Pool/main pages are
released by their actual native owner and verified unavailable afterward.

A second independent fresh native startup verifies the exact first missing CRC
owner frame when no joins are supplied. The positive initializer stops at
canonical `1C5F4F`, selected target `149680`, with actual pushed return
`1CB184`. The full caller ABI is retained through the typed missing-owner stop.
The source renderer load of `51DF44` has not executed; its loader value does
not become usable overflow storage.

All **four checks pass without skips** in each frozen **Release,
ASan/UBSan with leak detection, and ThreadSanitizer** build. The selected build
uses the actual production physical, arena, pool and early-initializer CMake
modules. It compiles the exact raw main-producer source selected by the main
module; the separate full world allocator graph is outside this prefix cohort.
The source manifest includes the compiler-discovered dependency closure and
current production modules.
[Validation](../analysis/multiplayer/sandbox-source/physics-early-initializer-tu15-20261012/validation.json)
SHA256 `632193ed83c45789569d6edc88c3cdfad85a8b396b74e3b2b860620fddfced42`
pins recovery, generation, original fixture and all build/test artifacts.

## Remaining startup and live physics

The `149680` overflow initializer needs source-produced renderer backing from
`134B00` and its two `137460` descriptor constructors. Their child allocations
use the separate installed XAPI process heap, not this main Havok bank.
[Renderer storage obligations](SHIELD_RAW_STARTUP_ALLOCATION_RESEARCH.md) and
[current process-heap startup](SOURCE_PROCESS_HEAP_STARTUP.md) retain that
dependency. A zero pointer, an arbitrary host buffer or a successful callback
would bypass it and is not supplied.

The complete early initializer return, later heap/world construction and mode
transition, occupied bodies/controllers and teardown remain required. The live
match still uses its host collision mover, and Lockout lift/Coagulation vehicle
acceptance remains failing. These component executables do not update the game
launcher or establish full physics-engine or multiplayer parity.
