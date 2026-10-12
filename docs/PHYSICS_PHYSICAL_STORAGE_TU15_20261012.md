# Fresh TU1.5 physical memory stage and partitions

The native port now compiles the complete original physical-memory stage and
game-memory partitioner against the TU1.5 executable. Their actual allocation
and protection imports use the existing native page owner. No captured physical
bank, prepared stage record, arena pointer or readiness value is supplied.

English TU1.5 is the current parity authority, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
TU1.1 addresses remain internal correspondence IDs only.

## Complete source ownership

[Recovery](../tools/research_physics_physical_storage_tu15_20261012.py) searches
the actual selected executable and accepts unique complete normalized bodies,
internal branches and relocated operands. Short prefixes only locate candidates.
All four bodies and 86 source instruction sites are retained and statically
compiled by [the generator](../tools/generate_physics_physical_storage_tu15_20261012.py).
Production has no opcode decoder, original-code execution or output replay.

| Correspondence ID | TU1.5 entry | Source responsibility |
| --- | --- | --- |
| `12CB70` | `12DF10` | Clear and initialize the physical stage, request its complete reservation |
| `217020` | `21B920` | Round, carve, protect and publish a partition |
| `2D79AA` | `2DB45A` | Complete contiguous-memory allocation wrapper |
| `2D7A1A` | `2DB4CA` | Complete protection wrapper, including zero-length return |

The stage's actual contiguous allocation requests `30E4000h` bytes with lowest
physical address `61000h`, highest `3144FFFh`, alignment zero and protection 4.
These bounds leave one valid physical interval. The native owner retains it as
32-bit token `80061000h`, backed by actual anonymous Linux pages. The source
writes stage state at TU1.5 `4F2CF0` and its adjacent fields.

The actual game-arena caller's `EAX=40000h, ECX=3BE000h` request carves
`3FE000h` bytes and returns `82D47000h`. This address is produced by the
unchanged source arithmetic over the real source-created reservation. It is
not supplied as input. Partition pointer/size/readiness publication occurs at
`50EBB4/50EBB8/50EBB0`, including the original failure behavior. A zero result
does not cause the native adapter to rewrite the source's readiness flag.

[The native composition](../src/physics_physical_storage_host.cpp) authenticates
actual TU1.5 allocation/protection import slots `41B640/41B648`, ordinals
`A6/B6`, the canonical source call sites and the allocation return `2DB489`.
The source caller owns a tail-dispatched protection return. The original
allocation/protection wrappers retain their full ABI and execute normally.

The reached resource contract is explicit native adaptation: anonymous host
pages, the physical token namespace selected by the original bounds, page
rounding and actual host `mprotect` for the reached read/write protection. The
native stdcall preserves GP/FP/flags except allocation EAX and consumed stack.
Protection preserves EAX. This is not evidence of Xbox kernel clobbers, physical
hardware layout or timing. Unsupported requests and host failures stop without
guessed Xbox status/TLS outputs. The wrapper's `2DBC56` error owner remains
required if source allocation fails.

## Original differential execution

[The original-only oracle](../tools/oracle_physics_physical_storage_tu15_20261012.py)
executes unchanged TU1.5 instructions, checking every executed site against
pinned original bytes and the complete correspondence. Only the actual kernel
allocation/protection imports use host resources. Registers, stack arguments
and empty x87 state are declared standalone caller inputs. Native output does
not construct the oracle fixture.

Three fresh sequences exercise stage construction, the actual arena partition
request, zero/unaligned/aligned and repeated partition requests, original range
exhaustion, an oversized partition and zero-length protection. Their physical
address layout is necessarily identical because the original allocation bounds
admit exactly one interval; these are different request sequences, not three
arbitrarily relocated physical reservations.

The corpus contains **48 calls, 1,689 original instruction observations,
216 ordered writes, three actual allocations and 35 protection calls**. It
reaches 82 of 86 compiled sites and every compiled entry. The allocation
wrapper's unrestricted-range branch and allocation-failure/error call remain
static-only evidence in this corpus.

[Native tests](../tests/physics_physical_storage_tests.cpp) compare full GP ABI,
applicable flags, retained FP state, each ordered write's selected source PC,
address and before/after bytes, resource arguments/results, initial pages,
publication globals and all 153,796,608 final retained bytes across the three
sequences. The resource owner is destroyed before checking that its bank is
unavailable. This RAII retirement is native ownership, not original world
teardown. A separate fresh negative bank verifies the exact original import
frontier and preceding source publications when its platform owner is missing.
The negative run's source writes do not contaminate the positive sequence.

Recovery, generation and original execution reproduce independently. The exact
production CMake module, compiler-discovered source dependencies, Release and
ASan/UBSan/leak builds and results are sealed in
[the validation manifest](../analysis/multiplayer/sandbox-source/physics-physical-storage-tu15-20261012/validation.json).
The selected build compiles this component with the real production memory,
image and native physical-page implementations. It does not build a game launcher.

## Remaining original startup

This closes the physical stage and partition prerequisite for the game arena.
The subsequent [TU1.5 arena increment](PHYSICS_GAME_ARENA_TU15_20261012.md)
joins this prerequisite to complete async savegame/thread/file startup and the
CRC/allocator tail. That separate comparison does not bypass any of those
stages. The earlier TU1.1 death startup is not used as unverified TU1.5 code.

Renderer overflow storage, the original mode transition, occupied
world/body/controller construction and teardown remain required. The live
match still uses its host collision mover. The known Lockout lift and
Coagulation vehicle acceptance failures remain open, and this component does
not establish full physics-engine or multiplayer parity.
