# Biped construction with an independently sourced Xbox compatibility ABI

This independent **seven-case** extension completes three original rigid-body
constructors and one full Havok singleton initializer. It supplies only the
declared exception-chain platform input below. Actual biped creation then stops
at missing world `526234`; full new-map initialization stops at missing process
heap `566F08`. There are **zero completed live bipeds, worlds, ordinary motion
steps or contacts**. The earlier seven-case constructor prefix remains frozen.
[Oracle](../tools/oracle_mp_spawn_biped_platform.py),
[report](../analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json),
[earlier frozen prefix](MP_BIPED_PHYSICS_CONSTRUCTION_RESEARCH.md).

## Frozen evidence and reproduction

| Artifact | SHA-256 |
| --- | --- |
| Original TU1.1 executable | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| New oracle | `709a0621c00c48146a7a8b143f2210671f20814e0a8a76b7c95e05a9b0be1647` |
| New report | `02a3e800af3c579d4635b159f6ce29d5bd0a3dc57e6877ed70bb70d5b0033633` |
| Canonical instruction manifest | `a80d8ed8e97d4e05029fb510f0ad8ea1f5aa38d38f028747431a86aedddce543` |
| Primary ABI source manifest | `02e1f9ab338ca3171d3772132b46054b8fc8e1fdbb3b15086f824d2192ad8675` |

Run `.tools/python/bin/python tools/oracle_mp_spawn_biped_platform.py`. The
oracle verifies nine earlier tool/report pins and the primary source manifest,
then its individual source-file hashes. It audits **12,795 executed original
instruction addresses** and publishes **113 listings / 11,224 rows**; 4,639
listing rows execute. Full report and all listings reproduce byte-identically
in the separate `reproduction` directory. Original instructions are unchanged.
[Verification and per-listing counts](../analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json),
[pinned primary source metadata](../analysis/multiplayer/integration/spawn-source/biped-platform-context/sources.json).

## What establishes the platform input

Pinned Cxbx-Reloaded commit `585c49a50af1255ab155099e06f24505f9c5a800`
declares `NT_TIB.ExceptionList` at offset **0**, `KPCR.NtTib` at offset **0**,
and `X_EXCEPTION_CHAIN_END=FFFFFFFF`. Its `KiInitializeContextThread`
explicitly places that sentinel in the initial context-switch exception-list
field. These are primary implementation sources for an Xbox compatibility
ABI; they are **not original Xbox kernel bytes or verified hardware behavior**.
The same implementation's `KiThreadStartup` is unimplemented, so these fields
do not establish completed actual kernel thread startup.
[types](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/common/types.h#L1799),
[sentinel](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/common/types.h#L1939),
[context initialization](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKi.cpp#L1026).

Cxbx's FS support identifies the Xbox FS base as the KPCR and separates that
layout from host thread routing. Pinned nxdk commit
`14d5ee97e73347c973f1f57b68b79ec08c9e77f2` independently calls
`PsCreateSystemThreadEx` with its thread-start wrapper and establishes TLS
access in its own startup path. That corroborates the declared thread ABI,
without furnishing an original kernel exception-chain dump.
[FS implementation](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/support/EmuFS.cpp#L50),
[nxdk thread implementation](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/winapi/thread.c),
[nxdk kernel ABI declarations](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h).

The extension maps raw GDT backing `06300000` and thread-prefix backing
`06400000`, each 4,096 bytes. Only backing `+0=FFFFFFFF` is a supplied thread
field. Flat 32-bit CS/data/SS descriptors and an FS-base descriptor route the
emulator to that backing: CS=8, DS/ES/SS=16, FS=24. These descriptor values,
addresses and unused backing bytes are fixture choices; they are not actual
Xbox selectors, whole initialized KPCR/TLS/thread structures, or exception
dispatch. All other thread/TLS fields are explicitly unprovided.
[Exact descriptor bytes and original FS stores](../analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json),
[routing implementation](../tools/oracle_mp_spawn_biped_platform.py).

## Original XAPI registers its own SEH frame

Original TU `CreateThread` **`2D8606`** supplies **`2D856E`** as the startup
wrapper to original `PsCreateSystemThreadEx` import thunk `417C04`.
The bounded unchanged prefix `2D856E..2D857A` pushes size `18` and handler
`418088`, then calls **`326B74`**. The original SEH prologue reads FS:+0,
builds its linked frame, and writes the new head through FS:+0 at `326BA8`.
The retained head is `021FCFEC`, linked to supplied prior `FFFFFFFF`; the
24-byte source frame is
`ffffffffc86b320088804100ffffffff0000000000000003`.
The prefix stops before `2D857E` reads FS:+28 and before later TLS access;
no current-thread structure or start-function result is supplied. This proves
the original wrapper's registration protocol within the declared ABI context,
not the original kernel's initial sentinel producer.
[CreateThread](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/002d8606.asm),
[startup wrapper](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/002d856e.asm),
[SEH prologue](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/00326b74.asm),
[executed prefix](../analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json).

## Original rigid-body and body-slot completion

For actual Lockout starts **0, 32, 65**, the extension inherits the frozen
original shape fixup, complete fixed-memory allocator and real 512-entry
physical-component pool. Original `B8A30` allocates the actual biped; original
animation/cache sampling produces its matrices; original `1DAAC0` allocates
body `06001030`. With the separately sourced FS head, full original **`318100`**
returns that body, stores vtable **`0041A950`**, and produces motion pointer
**`060010D0`** with motion vtable **`0041B870`**. Original **`1D3A70`** then
grows the component body-slot array with stride `60` and sets count `+74=1`.
Full body, motion prefix, transform, component and slot-array bytes are retained;
none is a supplied physics field. The constructor restores FS:+0 to `FFFFFFFF`.
[Rigid-body constructor](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/00318100.asm),
[body-slot producer](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/001d3a70.asm),
[source outputs and call order](../analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json).

Creation continues through **`1D4F30→1C7320→315820`**. At **`315824`**,
the original world method reads receiver `+12C` while ECX/global
**`526234=NULL`**, so all three cases stop before world insertion.
The source location and authored root float bits remain those of the earlier
prefix: start0 `40E11301 BE09D990 C10476D3`, start32
`C10FDFCB 3F834A5C C1223342`, start65 `BE273FEF 413EC528 C1423332`.
This does not establish the result of later completed world placement or motion.
[Insertion caller](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/001d4f30.asm),
[global world dispatch](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/001c7320.asm),
[exact blocked world method](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/00315820.asm).

## Original static registrations and complete Havok singleton initializer

The missing `487668` producer is an original factory registration, not a
literal direct text store. Original CRT `_cinit` **`2D80FD`** walks its authored
table `468630..468CB8`. The extension executes only eight relevant original
callbacks, in their actual table order; complete process-wide CRT startup is
excluded. Each source callback prepends its literal twelve-byte record to
original registration head `487694`.
[CRT iterator](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/002d80fd.asm),
[record bytes, table positions and source stores](../analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json).

| CRT word | Original callback | Record | Factory | Target global |
| --- | --- | --- | --- | --- |
| `46863C` | `37F110` | `469284` | `2DD960` | `48763C` |
| `468640` | `37F130` | `469298` | `2DE4F0` | `487668` |
| `468644` | `37F150` | `4692A4` | `2DEF40` | `487680` |
| `468650` | `37F1E0` | `46931C` | `2E0830` | `4876F4` |
| `468658` | `37F240` | `469380` | `2E26C0` | `4877C4` |
| `46873C` | `37FC70` | `46E278` | `378A60` | `48C058` |
| `468740` | `37FC90` | `46E284` | `378CA0` | `48C060` |
| `468744` | `37FCB0` | `46E290` | `378D40` | `48C064` |

Full original **`2DED30`** installs original game hkMemory through
`148B00→2DD5B0`, constructs its helper objects through original memory methods,
and calls full **`2DE250`**. That source walker invokes each registered factory
and stores its returned object through original record `+8`. Factory `2DE4F0`
produces **`487668=06005140`**. The initializer executes the resulting
object's original virtual callback and returns **EAX=0**, with original flag
**`48766C=1`** and FS head restored. With the registrations excluded, the
negative case stops at **`2DEE15`** on missing `487668`; no successful return is
substituted. This is completed singleton initialization, not world creation.
[Full initializer](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/002ded30.asm),
[factory walker](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/002de250.asm),
[factory](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/002de4f0.asm),
[positive and negative executed cases](../analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json).

## Exact remaining source dependency

With the original registration callbacks, ABI head, source component pool and
raw original-requested Havok storage, full **`1C5FF0`** passes `2DED30`.
Its subsequent original C++ allocation chain **`32684B→3293A6→2DAC5E`**
reaches **`2DAC7F`**, reading a NULL process-heap receiver's `+18` at address
`00000018`; original global **`566F08`** has not been initialized. It stops
before complete map pools, world `526234`, physical body insertion or step.
[New-map caller](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/001c5ff0.asm),
[C++ allocation](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/0032684b.asm),
[malloc](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/003293a6.asm),
[actual heap entry and stop](../analysis/multiplayer/integration/spawn-source/biped-platform-context/instructions/002dac5e.asm),
[retained failing registers and source stage order](../analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json).

The next independent gate is original **`XapiInitProcess 2D7F8E`** and its
heap construction from original XBE reserve/commit fields, followed by original
new-map/world startup. The resource-delivery, raw object heap, cluster/BSP and
earlier startup inputs remain exactly the declared frozen boundaries. No new
world/body/pose/contact fields, Havok success return, native production edit,
or claim of complete startup/Xbox parity is introduced here.
[Original XAPI decompile](../analysis/map-pack-update/functions/002d7f8e.c),
[first-world consumer listings in frozen prefix](MP_BIPED_PHYSICS_CONSTRUCTION_RESEARCH.md).
