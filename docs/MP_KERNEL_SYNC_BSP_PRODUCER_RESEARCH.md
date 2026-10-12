# Sourced kernel initializer and original Lockout BSP producer

This separate **four-case** extension executes a pinned compatibility
critical-section initializer, allowing the original process-heap writer to
return. Original full new-map startup then stops at lock entry requiring a
current-thread identity. Separately, original physical/game-arena prefixes and
the original BSP datum-table constructor allow complete **`1C6270`** to return
with original Lockout geometry, wrapper and static rigid-body outputs.
**Whole process/map startup, world insertion and ordinary motion/contact remain
unproved.** Prior frozen tools/reports are unchanged.
[Oracle](../tools/oracle_mp_spawn_kernel_sync_world.py),
[report](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json),
[preceding frozen process-heap/world stops](MP_PROCESS_HEAP_WORLD_STARTUP_RESEARCH.md).

## Frozen evidence and reproduction

| Artifact | SHA-256 |
| --- | --- |
| Original TU1.1 executable | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| New oracle | `ad54453224f4c770be75cf71c6c17d5b06dd957ada8db293ebb370a6c6295c87` |
| New report | `ad0fb0b253d9fbf5b8c40b79ec2a4c142271be284c3570656b8325a7e9b92c54` |
| Canonical original instruction manifest | `53ac366aee661c6948dc42b6265e5dfdb4714ab8b4af0861807fdea6dbbebb63` |
| Primary compatibility source manifest | `0813f92e273e295bfc7fa146555afcd6e752f73cc5295f305681dfd5bf54505e` |
| Generated compatibility C | `9af1aa753d3e32ad157b8637f00530fa81ba448728814183dc0734ce37f841cd` |
| Compiled compatibility text | `964ad3813aab0513e6d8d2c72e3cfda9e72bab1c85033dea69d968444addf221` |
| Compiled compatibility listing | `d275870f50ed15ade1b20fc63d801e3392b81f91ac6fcaa4acae8dde154a84af` |

Run `.tools/python/bin/python tools/oracle_mp_spawn_kernel_sync_world.py`.
The tool verifies thirteen earlier tool/report pins and the source manifest/
individual source-file hashes, compiles the declared initializer expressions,
and checks compiled guest text unchanged in every case. It audits **2,574
executed original TU instruction addresses** separately from **11 executed
compatibility instruction addresses**. The **143 original listings / 13,477
rows** contain 1,853 executed listing rows. Report, all original listings and
the compiled listing reproduce byte-identically in `reproduction`.
[Original instruction evidence](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json),
[separate compatibility listing](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/compatibility-instructions.asm),
[generated initializer](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/initializer.c),
[primary source pins](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/sources.json).

## Exact admitted compatibility initializer

Pinned Cxbx-Reloaded commit `585c49a50af1255ab155099e06f24505f9c5a800`
implements ordinal291 by initializing an unsignaled synchronization event,
setting lock count −1, recursion count0 and ownerNULL. Its event initializer's
Xbox-structure branch sets type1, size4, signal0 and self-linked wait-list
pointers. This is a primary **compatibility implementation**, not original
Xbox kernel instruction bytes or a verified hardware run. The source's host
Windows-handle test is excluded for the explicitly supplied raw guest storage;
only its actual Xbox-structure initialization expressions execute.
[Primary critical-section initializer](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlRtl.cpp#L1216),
[primary event initialization](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKe.cpp#L954).

The oracle extracts the actual initializer body and event expressions from
the pinned files, with diagnostic logging represented by an empty macro. It
extracts primary structure declarations and the pinned nxdk list-initialization
macro, compiles 32-bit freestanding C, and checks event size16, critical-section
size28 and field offsets16/20/24 with compile-time assertions. The original
kernel ordinal call is routed to compiled entry **`09000030`**; its actual
stdcall epilogue consumes one argument and returns to unchanged TU code.
There is no supplied NTSTATUS or success return: the API is void, and generated
register effects are retained without assigning EAX a success meaning.
[Primary structure layout](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/common/types.h#L1787),
[primary list macro](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h#L118),
[exact extraction/build and guest routing](../tools/oracle_mp_spawn_kernel_sync_world.py).

| Guest critical-section offset | Executed compatibility output |
| --- | --- |
| `+0` byte | event type1 |
| `+1` byte | preserved |
| `+2` byte | event size4 |
| `+3` byte | preserved |
| `+4` dword | signal0 |
| `+8/+C` dwords | both pointer to record+8 |
| `+10` signed dword | −1 |
| `+14` signed dword | recursion0 |
| `+18` pointer | NULL |

The independent dirty-platform-storage case begins with all28 bytes `A5` and
confirms both untouched bytes remain `A5`. This guards against an inferred
whole-record clear. Compatibility stores and source original stores have
separate instruction identities and report fields.
[Dirty case and all compiled writes](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json).

## Original heap writer return and next full-map stop

With only the frozen raw NT reserve/commit services plus that compiled
compatibility initializer, original **`2DA83A`** returns and original
**`2D7FDE`** writes heap **`08000000`** to **`566F08`**. The source produces
the heap segment/free lists and lock pointer; FS head returns to `FFFFFFFF`.
The original XAPI shutdown/event/hardware interval remains excluded exactly as
in the preceding freeze. This proves the bounded heap writer with declared
platform services, not completed ordinary `XapiInitProcess`.
[Original heap initializer](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/002da83a.asm),
[original XAPI writer](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/002d7f8e.asm),
[complete original fragment outputs](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json).

Full original **`1C5FF0`**, with original Havok registration and component-pool
producers, passes `2DED30` and enters its first C++ manager allocation.
Original heap allocator **`2DAC5E`** calls lock entry through thunk `417BE0`
at **`2DACAD`**, ordinal **277**, argument **`08000610`**. The suite stops
before that service. Its pinned primary implementation calls
`KeGetCurrentThread` before choosing/recording lock ownership. No current-thread
identity, thread/TLS fields, ownership, contention, wait result or lock-entry
return is provided. Its own source notes incomplete/faked thread context,
so the implementation does not supply original hardware identity evidence.
[Original lock caller](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/002dac5e.asm),
[primary lock entry](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlRtl.cpp#L756),
[primary current-thread boundary](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKe.cpp#L863),
[retained original failing call state](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json).

The source SEH frame remains registered at this deliberate stop, as expected
before the blocked allocation returns. The case reaches no complete manager,
impact-pool reset, BSP/world ordering, body insertion or world step.

## Original physical arena and game-arena prefix

Full original **`12CB70`** clears its arena bookkeeping, stores physical
low **`61000`** and high-exclusive **`3145000`**, then calls original
**`XPhysicalAlloc 2D79AA`** with `(30E4000,61000,0,4)`. The original wrapper
computes highest acceptable address **`3144FFF`** and requests
`MmAllocateContiguousMemoryEx(30E4000,61000,3144FFF,0,4)` through ordinal165.
The admitted service supplies only raw backing in the source's address range
**`80061000..83145000`**. Existing literal cache mappings are retained where
this bounded emulator view overlaps; this does not claim a complete physical
RAM/cache residency layout or original startup allocation order.
[Source physical arena root](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/0012cb70.asm),
[original wrapper/protocol](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/002d79aa.asm),
[recorded raw OS service arguments](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json).

The unchanged **`125280..1252BA`** prefix calls **`217020`** with source
EAX=`40000`, ECX=`3BE000`, rounding total **`3FE000`** to pages and subtracting
it from the original arena upper cursor. It produces **`82D47000`**, invokes
original `XPhysicalProtect` for that exact range/protection4, stores it in
**`4ED650`**, and clears **1,046,528 dwords** through actual original `rep stosd`.
Only raw platform backing/protection is supplied. The later **`217090`**
savegame resource/thread I/O and remaining game-arena startup allocations are
excluded, without successful I/O replacements.
[Actual arena prefix](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/00125280.asm),
[source suballocation](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/00217020.asm),
[protection wrapper](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/002d7a1a.asm),
[excluded asynchronous producer](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/00217090.asm).

Full original **`1F15B0`** then takes **8,000 bytes** from the source-produced
arena/cursor, advances the cursor to8,000 and updates the source checksum through
`1656A0`. It stores **`52625C=82D47000`**. This observed address is specific to
the bounded excluded allocation order; it is not claimed to be the complete
original game's BSP-table address.
[Actual table constructor](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/001f15b0.asm),
[checksum producer](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/001656a0.asm),
[retained arena/table state](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json).

## Complete original BSP representation setup

With that table and the frozen original Havok memory/static registration
producers, full **`1C6270`** executes. Original **`1F1600`** clears all2,000
datum slots toNONE, while scanning the inherited object pool. Original
**`1C74D0→1F2710`** derives the geometry bounds from actual Lockout collision
vertices, then **`1C7480`** builds the source BSP wrapper from its real cache
definition. Full **`318100`** builds the source static rigid body, and complete
`1C6270` returns with FS head restored to `FFFFFFFF`.
[BSP caller](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/001c6270.asm),
[datum-table clear](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/001f1600.asm),
[geometry construction](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/001c74d0.asm),
[actual vertex-bounds producer](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/001f2710.asm),
[wrapper producer](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/instructions/001c7480.asm).

| Original output global | Source-produced pointer / retained logical record |
| --- | --- |
| `526260` | `060051F0`, geometry shape30 bytes, vtable`45D278` |
| `526264` | `06005220`, wrapper18 bytes, vtable`45D2E0` |
| `486550` | `06005250`, rigid bodyA0 bytes, vtable`41A950` |
| `52625C` | `82D47000`, 8,000 bytes all`FF` |

The cleared table SHA is
`bbc0f58b6ba541901c4e0a54d7c4dc92e24c2f2f8abd5a83839188b6ec7db0ba`.
Every listed geometry, wrapper and rigid-body byte is retained as an original
instruction output. Actual cache/BSP context, earlier raw object/component
storage, memory backing and resource-delivery boundaries remain declared.
No new geometry field, physical body field, pose or contact was written as a
fixture.
[Full source call order and retained records](../analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json).

This BSP case has **`526234=0`** throughout: representation construction is
distinct from complete world construction and insertion. The next full-map
gate remains independently established current-thread lock-entry context,
followed by original manager/impact-pool/BSP/world scheduling. No production
or prior frozen artifact is edited by this extension.
