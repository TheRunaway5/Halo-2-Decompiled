# Complete compatibility context and original new-map ordering

The complete pinned **host-thread branch of `EmuGenerateFS<true>`** now executes
with real Wine32 thread/TIB/stack/allocation services. Its whole KPCR and ETHREAD
outputs allow unchanged original `1C5FF0` to pass the earlier PRCB callback
stop, construct its manager/callback, enable/reset the original pools, execute
complete `1C6270`, and obtain a core world from `316710`. It then stops at
original **`2DD621`**, after the original fixed-memory allocator returns NULL.
**Full new-map completion, body insertion and ordinary motion/contact remain
unproved.** The PRCB unknown-byte zeros are compatibility-produced, not recovered
Xbox defaults. Prior freezes and production remain unchanged.
[Oracle](../tools/oracle_mp_spawn_complete_kpcr.py),
[report](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/report.json),
[preceding immutable host-lock suite](MP_HOST_THREAD_LOCK_ORDER_RESEARCH.md).

## Frozen evidence

| Artifact | SHA-256 |
| --- | --- |
| Original TU1.1 executable | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| New oracle | `a29845443fe79c75572c127f6e22a6da25cc0a266bc249d073f0d94b6df0b336` |
| New semantic report | `f99ab55c6a3de3fbe48087b96d9db31e8a50e38c73b959b263b52b0bec98f8a6` |
| Canonical original instruction manifest | `1e8561374c3e378be01232f0dc37449d31961e53f0e01e6102fcfef7421aacb7` |
| Generated native compatibility C++ | `c212e2992861a478229ae68752580a6b22425bb2d031cadd3f795f269773947b` |
| Compiled native32 compatibility PE | `4d8de99dd61203a801e256770edeba0af50ca05068203abdb8040faa0ed3967e` |
| Native compatibility disassembly | `c97e77c3d3e8c32801f533da9b976ed36ddb5f0cd84a13a8095552c8f1a06156` |
| Frozen raw native context snapshot | `3e8c77e5b7e79aad1ac49bab2621b85049cd66e47e2c6e01ada923f18fffefaa` |

Run `.tools/python/bin/python tools/oracle_mp_spawn_complete_kpcr.py`;
use `--output .../reproduction/report.json` to keep a distinct runtime snapshot.
The oracle verifies 22 prior pins and nine additional primary source-file pins,
extracts literal source operation bodies, executes the complete native producer,
and audits **4,557 executed original TU addresses**. The 159 original listings /
15,187 rows contain 2,884 executed listing rows. The semantic report, producer metadata and all 159 original listings
reproduce byte-identically in a separate execution with a newly created real
host thread. Its raw host snapshot differs as expected. Native compiled code
has a separate disassembly; no original instruction is patched. Per-run native thread
IDs and full host stack contents are separate from the semantic report and may
vary. `run-native-evidence-pins.json` pins the frozen snapshot, producer metadata
and actual Wine runtime binaries.
[Build/extraction and transfer checks](../tools/oracle_mp_spawn_complete_kpcr.py),
[native disassembly](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/native-compatibility-instructions.asm),
[native snapshot](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/raw-host-context.json),
[per-run native evidence pins](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/run-native-evidence-pins.json).

## Exact complete compatibility producer

All primary compatibility code is pinned to Cxbx-Reloaded commit
`585c49a50af1255ab155099e06f24505f9c5a800`. The oracle extracts the complete
`EmuGenerateFS` body and runs its actual host-thread template branch, with
unmodified operation bodies for `KeInitializeThread`, `KiInitializeContextThread`,
APC/semaphore/timer initialization, `RtlZeroMemory`, `GetNtTib`,
`EmuGetTIBStackBase`, `EmuGenerateStackSize` and `EmuKeSetPcr`. Diagnostic logging
is empty. The primary function-pointer-to-PVOID expression requires GCC's
`-fpermissive`; its assignment remains unchanged and the diagnostic is saved.
This is execution of a primary **compatibility implementation**, not original
Xbox kernel bytes or hardware execution.
[Primary full generator](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/support/EmuFS.cpp#L657),
[primary complete thread construction](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKe.cpp#L316),
[primary context construction](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKi.cpp#L1026),
[literal extracted bodies and compiler evidence](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/producer-metadata.json).

The actual primary startup captures ESP, calls the stack-size producer with
TLS size 0, and calls `EmuGenerateFS<true>(NULL,base,size)` **before**
`ObInitSystem`, `PsInitSystem`, `KiInitSystem` and `RtlInitSystem`. The exact
primary declaration `KiUniqueProcess` therefore has static BSS zero at this
stage. The oracle does not supply a quantum or invoke a fabricated process
initializer. This is the compatibility kernel-initialization host branch;
ordinary title-thread `EmuGenerateFS<false>` and its object/thread producers
remain separate unresolved work.
[Primary actual startup order](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/init/CxbxKrnl.cpp#L1067),
[context before system initializers](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/init/CxbxKrnl.cpp#L1162),
[primary static process and later producer](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKi.cpp#L96).

| Complete executed native producer stage | Retained source-produced output |
| --- | --- |
| Actual reserve/commit request for KPCR `280` | native allocation `00AC0000`, mapped size 1000 |
| Whole KPCR zero, real host TIB copy and self fixup | all 640 bytes retained |
| SelfPcr/Prcb/DPC/IRQL assignment | SelfPcr=`00AC0000`, Prcb=`00AC0028`, DPC self-links, IRQL 0 |
| Actual reserve/commit request for ETHREAD `140` | native allocation `00AD0000`, mapped size 1000 |
| Whole ETHREAD zero and complete thread construction | header/APC/semaphore/timer/wait lists and source context retained |
| Exact context producer, TLS size0 | TlsData=NULL, FX area210, start/switch framesC/C, exception sentinelFFFFFFFF |
| Final stack/current-thread assignments | actual backed host stack; CurrentThread=`00AD0000` |
| Pointer installation and `_controlfp` calls | actual host user slot; captured FPU control027F |
| Unknown PRCB bytes `+34..+257` | all224 bytes remain compatibility-produced zero |

The real runtime is `wine-11.19`, with native32 compilation by
`i686-w64-mingw32-g++ (GCC) 16.2.0`. A real `CreateThread` worker supplies its own
TIB and stack. Its captured ESP passes through the exact size/alignment
producer; GCC `alloca` supplies the backed source range with an explicit extra
64-byte wrapper margin. Actual `ntdll!NtAllocateVirtualMemory` receives the
source base/size/zero-bits/flags/protection, plus the current-process handle.
The two statuses are real OS compatibility results, both 0; the wrapper assigns
no success constant. The worker and allocations remain alive until all source
consumer cases finish.
[Actual wrapper/services and snapshots](../tools/oracle_mp_spawn_complete_kpcr.py),
[primary host thread API](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread),
[primary native memory ABI](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntallocatevirtualmemory).

The whole 640-byte KPCR,320-byte ETHREAD, actual backed host stack snapshot and
source static process object transfer at their actual addresses into the
original-code emulator. Every mapping is rejected if it overlaps an existing
source/cache/resource mapping. No pointer is relocated and no selected field
is supplied just to bypass `[FS20+250]`. That callback zero comes from the
complete compatibility producer's whole-record clear and retained assignments.
It establishes no ordinary Xbox hardware default. Actual native stack/ID
snapshots are declared fixture inputs to the separate original consumers.
[Complete transfer and untouched source audits](../tools/oracle_mp_spawn_complete_kpcr.py),
[all produced context and OS backing](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/raw-host-context.json).

Current-thread equality reads the produced `Prcb.CurrentThread`. The preceding
pinned compatibility lock bodies and atomic provider remain explicit.
Scheduler/exception delivery, contended waits/wakes and stored
`KiSuspendNop/KiSuspendThread/KiThreadStartup` callback behavior are not provided;
unavailable endpoints stop if invoked. The title-thread false branch's stack
move/delete and XBE TLS initialization are not replaced. The host branch's
actual context producer sets TLSNULL, so the conditional XBE TLS read is not
reached. None of these inputs supplies a game/Havok/body/pose/contact field.
[Exact declared boundaries](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/report.json).

## Original full-order cases and next exact stop

Both cases execute the original bounded heap writer, Havok registrations,
component pool producers and raw memory backing with the prior exclusions
preserved. With the complete compatibility context, original `2DB3A0` sees the
whole-producer callbackNULL and returns the source allocation. `27DAE0` completes
manager `08000690`; a second original heap allocation produces callback
`080006D0`, whose original `1C6037` write sets vtable `461B9C`. Each allocation
performs original acquire/release at `2DACAD/2DB25E`. The negative case then
stops at `1C6058` writing impact pool+29 through its uninitialized NULL pointer;
both original objects are retained and FS head is restored to the captured
native chain `00ABFF88`.
[Original full-map order](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/instructions/001c5ff0.asm),
[original manager constructor](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/instructions/0027dae0.asm),
[precise negative case and heap stores](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/report.json).

The prepared case additionally executes the original physical/game-arena
prefix, BSP datum-table producer and full `2284C0` impact pool constructor.
Within actual `1C5FF0`, the source enables/resets impacts, impact arrays and
Havok components, sets486560, then calls **`1C6270` before `1C66A0`**. Complete
`1C6270` produces actual Lockout shape `060051F0`, wrapper `06005220` and static
body `06005250`. Original `316710` returns core world **`06005470`**, which the
source assigns to526234. This full-order core address differs from the previous
direct-world fixture, which omitted preceding BSP construction.
[Original BSP producer](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/instructions/001c6270.asm),
[original world caller](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/instructions/001c66a0.asm),
[all exact calls/returns and retained objects](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/report.json).

The prepared case stops inside the later original world setup tail:
`2DD5F0` requests2010 bytes; original `22E1D0`, in source fixed-memory mode1,
returns NULL, and **`2DD621`** attempts its store through NULL. The original
fixed arena remains15000. No allocator mode, expansion, free-list or object
field is altered to force a return. Source `1C60A1` would assign mode2 only
**after `1C66A0` returns**, and that instruction is not reached. Its SEH frame
remains registered at the deliberate stop. Completed map scheduling and world
body insertion remain false despite the retained core constructor result.
[Original failed allocation consumer](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/instructions/002dd5f0.asm),
[original allocator](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/instructions/0022e1d0.asm),
[actual full ordering and later mode assignment](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/instructions/001c5ff0.asm),
[precise failure/retained world](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/report.json).

The next required contract concerns original memory-mode/capacity/startup
ownership at that world tail, or an earlier source producer that changes its
allocator state. The prior excluded XAPI shutdown/hardware and savegame/thread
I/O intervals and incomplete authored startup allocation order remain explicit.
This suite contains **zero completed new-map world initializations and zero
ordinary motion/contact cases**, and does not close the live biped physical
component or Havok world-step boundary.
[Bounded completion counts and preserved exclusions](../analysis/multiplayer/integration/spawn-source/complete-compat-kpcr/report.json),
[earlier original biped stop](MP_BIPED_PHYSICS_CONSTRUCTION_RESEARCH.md).
