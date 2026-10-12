# Fresh TU1.5 game arena and async startup

The complete original game-arena constructor now runs as native C++, including
its async queue, source worker, two savegame files, CRC and allocator publication.
The physical stage and partitioner supply its actual reservation. No captured
arena, ready flag, job completion, file result or checksum is supplied.

English TU1.5, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
is the current authority. TU1.1 addresses are correspondence IDs only.

## Source and native ownership

[Recovery](../tools/research_physics_game_arena_tu15_20261012.py) authenticates
17 complete paired bodies and 730 instruction sites. Ambiguous short bodies
also require their actual paired caller or function-pointer operand. The
[generator](../tools/generate_physics_game_arena_tu15_20261012.py) statically
lowers every body; production contains no original instruction decoder or
original-code execution. Independent recovery and generation reproduce exactly.

The selected constructor is `126640` (correspondence `125280`), async setup is
`123450` (`1221B0`), thread entry is `1235F0` (`122350`), worker is `123600`
(`122360`), CRC is `166A80` (`1656A0`), and the arena client allocator is
`127210` (`125E50`). Complete queue, file callback, partition/file setup and
CRC-table bodies accompany them.

[The native owner](../src/physics_game_arena_native.cpp) authenticates all 37
external call sites, selected targets and actual pushed return addresses.
It owns real native physical pages, two mutexes, the zero-count semaphore with
maximum 150, a native thread and actual temporary savegame files. Original
instructions own queue links, callback dispatch, job completion and all arena
state. The two files reach their original 4,239,360-byte extents through actual
open, seek and truncate operations.

The declared native adaptation includes 32-bit resource aliases, worker scratch
stack and initial CPU state, Linux thread priority policy, file namespace and
interruptible synchronization waits. These are not Xbox kernel clobber or
timing claims. Unsupported owners stop at the actual pending source call.
Failed file opening reaches a still-required source error owner; this corpus
does not claim that fallback. The original worker is immortal. Native stop is
consumed before synchronization acquisition; stop/join and RAII release do not
pretend to be an original worker return or original world teardown.

## Original comparison and builds

[The original-only oracle](../tools/oracle_physics_game_arena_tu15_20261012.py)
executes unchanged selected instructions and checks every executed byte against
the owned executable. Its declared cooperative schedule lets the unchanged
worker run from a main-thread yield until its next unavailable semaphore wait.
The native comparison uses a real thread with that same declared schedule;
schedule hooks neither grant permits nor alter source state. Host anonymous
pages are initially read/write in the oracle; native protection uses actual
`mprotect` through the separately validated physical owner.

Thirteen calls cover fresh physical/queue/arena startup, repeated ready arena
construction and allocator sizes 0, 1, 3, 4, 5, 16, 257, 1024 and 5120.
The unchanged corpus contains 1,065,823 original instruction observations.
The native comparison matches **1,049,656 ordered writes and all 52,667,416
final retained bytes**, plus complete
main-call GP ABI, applicable flags, XMM/FP state and both actual file extents.
It executes 577 of the 816 compiled arena/physical sites. The original and
native runs produce arena `82D47000`, offset `2BB4` and checksum `842AA3DD`.

A separate actual-thread run uses no schedule gate. It compares all main-call
returns, final arena metadata/CRC and actual file extents after the worker
retires its four file jobs. Async queue bytes under a different host schedule
are not claimed identical. Fresh negative banks retain the original missing
queue/partition-owner frontiers. Stop is idempotent, new work after stop is
rejected, and destruction releases the physical bank.

The frozen production CMake modules and compiler-discovered dependency closure
build and pass both checks in **Release, ASan/UBSan with leak detection, and
ThreadSanitizer**. There are no skips. Repeated original execution is identical.
[Validation](../analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/validation.json)
SHA256 `fd369fef4f66d4fa7c14696dd5cc39e680ab32e2b814e17a97134ede9c57acc2`
pins the source, generated code, fixture, build commands and results.

## Remaining physics integration

This closes fresh arena startup, not occupied-world or live-match physics.
The subsequent [early-initializer composition](PHYSICS_EARLY_INITIALIZER_TU15_20261012.md)
joins the arena clients and pool/main storage, stopping at the actual required
renderer overflow owner. Original mode transition,
body/controller creation and complete teardown remain required. The live match
still uses its host collision mover, and the known Lockout lift and Coagulation
vehicle acceptance failures remain open. These component builds do not refresh
the launcher or establish complete multiplayer parity.
