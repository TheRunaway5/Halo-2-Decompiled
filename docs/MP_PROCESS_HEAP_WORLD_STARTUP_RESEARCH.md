# Original process-heap and direct world startup gates

This independent **four-case** suite executes the original XAPI process-heap
producer up to kernel allocation/synchronization boundaries and separately
executes the original Lockout world-construction entry. The core `hkWorld`
constructor returns; later direct-entry setup fails an original fixed-memory
allocation. **Complete process heap, complete map-world initialization and
ordinary motion/contact remain unproved.** The prior seven-case physics prefix
and seven-case platform extension remain frozen.
[Oracle](../tools/oracle_mp_spawn_process_heap_world.py),
[report](../analysis/multiplayer/integration/spawn-source/process-heap-world/report.json),
[platform context evidence](MP_BIPED_PLATFORM_CONTEXT_RESEARCH.md).

## Authority and reproducibility

| Artifact | SHA-256 |
| --- | --- |
| Original TU1.1 executable | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| New oracle | `580df674c30e514c99a9334b89d541e97aca598790ff6e05434473d05a68013c` |
| New report | `805c2ef2059202d24799af5dc86d241ebe8b6cc79ff66dd4105c1ffec02ababf` |
| Canonical instruction manifest | `6ef3f6bab34083bf55b9ae502543449b1ad4da99e1e0fe34d88853434546ba9b` |

Run `.tools/python/bin/python tools/oracle_mp_spawn_process_heap_world.py`.
The suite checks eleven earlier tool/report pins before and after execution,
audits **3,162 executed original instruction addresses**, and publishes
**131 listings / 12,918 rows**, including 1,633 executed listing rows,
with individual hashes and execution counts. Full report/listings
reproduce byte-identically in the independent `reproduction` directory.
Original instructions are unchanged; unresolved kernel ordinal targets are
intercepted before execution. The cache's overlapping address backing is never
interpreted as kernel instructions.
[Verification data and exact service scope](../analysis/multiplayer/integration/spawn-source/process-heap-world/report.json).

## Literal executable header and source heap producer

The new suite installs the literal original **8,192-byte XBE header** at its
authored base `00010000`; earlier suites load executable sections. Header
fields are original `InitFlags=80000005`, `SizeOfHeapReserve=00100000` and
`SizeOfHeapCommit=00001000`. They are executable data, not inferred heap defaults.
The raw header SHA and field bits are recorded in every case.
[Original XBE](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03),
[literal header evidence](../analysis/multiplayer/integration/spawn-source/process-heap-world/report.json).

Full original **`XapiInitProcess 2D7F8E`** first calls
`HalRegisterShutdownNotification` through original thunk `417BD0`, ordinal
**47**, at `2D7FA5`. The strict case stops at that unresolved kernel boundary;
no shutdown result is supplied. The following source helper **`2DC5FF`** owns
event/timer/hardware initialization and platform flags, and is also excluded
from the separately executed heap fragment below.
[Process initializer](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002d7f8e.asm),
[platform helper](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002dc5ff.asm),
[strict executed case](../analysis/multiplayer/integration/spawn-source/process-heap-world/report.json).

The separate cases execute the original XAPI prologue **`2D7F8E..2D7FA5`**,
then original **`2D7FB0..2D7FE3`**. The skipped `2D7FA5..2D7FB0` interval is
declared, not replaced by success. Original instructions clear the twelve-word
stack parameter record, put size **`30`** in its first word, read both real
header fields, and call **`2DA83A`** with
`(flags=2, base=0, reserve=100000, commit=1000, lock=0, parameters=record)`.
The original caller writes returned EAX to **`566F08`** at **`2D7FDE`**, only
after the constructor returns. Neither bounded heap case reaches that write.
Loaded zero XAPI flags are retained explicitly; the omitted platform helper
has not proved them as ordinary startup outputs.
[Exact argument and global writer instructions](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002d7f8e.asm),
[heap construction](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002da83a.asm),
[original decompile](../analysis/map-pack-update/functions/002d7f8e.c).

## Raw platform memory and next synchronization boundary

Original **`2DAA75`** invokes `NtAllocateVirtualMemory`, thunk `417C34`, ordinal
**184**, with five arguments: base-address pointer initially holding 0,
ZeroBits=0, size pointer holding **1,048,576**, allocation type **`2000`**,
protection **4**. The strict heap case stops there. The admitted raw-storage
case maps exactly that source-requested reservation at fixture address
**`08000000`**, writes only the OS output base pointer, and declares NTSTATUS0.
Original **`2DAAAE`** then requests a **4,096-byte** commit at that same base,
type **`1000`**, protection4; the raw service preserves those source-provided
pointer/size outputs. The ABI has in/out base and size pointers, matching the
pinned primary declaration. No game, physics or synchronization fields are
supplied.
[Original allocation calls](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002da83a.asm),
[recorded arguments/output pointers](../analysis/multiplayer/integration/spawn-source/process-heap-world/report.json),
[pinned primary NT allocation ABI](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h#L3281).

Original heap instructions then produce size/flag/signature fields, the eight
source lookaside links and 128 doubly-linked free-list heads in raw storage.
For flags2 with no caller lock, the source chooses its embedded critical
section. At **`2DABB6`**, original thunk `417C3C`, ordinal **291**,
`RtlInitializeCriticalSection` receives **`08000610`**. The suite stops before
that unavailable kernel service; it neither writes synchronization fields nor
supplies a successful return. Full heap construction, segment admission,
`566F08` installation and ordinary heap allocation remain open.
[Original metadata/critical-section branch](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002da83a.asm),
[retained raw heap bytes and original stores](../analysis/multiplayer/integration/spawn-source/process-heap-world/report.json),
[pinned primary initializer ABI](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h#L2090).

## Separate original world construction and its actual failure

The direct world case executes original static registrations, raw Havok backing,
complete **`148190`** and complete **`2DED30`** from the frozen extension, then
enters **`1C66A0`** directly. It excludes the full new-map manager, BSP shape
construction and map-pool scheduling. The actual Lockout BSP pointer and bounds
come from the inherited declared cache/BSP context; no world field is supplied.
Original **`317720`** produces the stack construction configuration, then
`1C66A0` subtracts/adds its original margin `465100` from/to actual BSP
`+1EC..+200` bounds, adjusts its source configuration scalars, allocates
**`260` bytes / class `2A`**, and calls **`316710`** with version **`4F4C`**.
[Actual world entry/configuration](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/001c66a0.asm),
[configuration constructor](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/00317720.asm),
[core world constructor](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/00316710.asm),
[source-produced fields and real BSP bits](../analysis/multiplayer/integration/spawn-source/process-heap-world/report.json).

Full **`316710`** returns **`06005290`**, and original `1C66A0` stores it at
**`526234`**. Later source setup grows the world's arrays, installs helpers and
reaches original temporary allocator **`2DD5F0`**. It asks source hkMemory for
**`2010` bytes / class `12`**. Under original allocation mode1, full original
**`22E1D0`** returns **NULL** after prior allocations/frees in the source
**`15000`-byte fixed arena**. At **`2DD621`** the unchanged caller writes
through NULL. The report retains the complete world record, original allocator
record, allocation request/result and failing instruction/registers. The suite
does not expand the arena, switch mode early, or supply allocation success.
[Temporary allocation](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002dd5f0.asm),
[source fixed allocator](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/0022e1d0.asm),
[retained direct-world case](../analysis/multiplayer/integration/spawn-source/process-heap-world/report.json).

A returned core world and populated `526234` do not prove complete `1C66A0`
or whole-map initialization. This direct case reaches no body insertion,
ordinary world step or source-generated contact.

## Original complete-map scheduling and next producers

Static unchanged **`1C5FF0`** calls `148190`, the registered Havok initializer,
then original C++ allocation for manager `526538` and callback object `52653C`.
It enables/resets impact pool **`52648C`**, impact-array pool **`526490`** and
component pool **`526248`**, sets live byte **`486560=1`**, then calls
**`1C6270` before `1C66A0`**. Its ordinary unpaused branch switches allocation
mode to2 only afterward. Therefore that mode switch cannot be moved earlier
to conceal the direct case's failure.
[Exact new-map order](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/001c5ff0.asm).

Original **`2284C0`** is the two impact pools' initializer, with original raw
requests `1450` and `850` and original `16D070` calls. Original **`1C6270`**
first invokes **`1F1600`** when its source gate admits it; that routine clears
2,000 datum entries through global **`52625C`**. Its initializer **`1F15B0`**
reserves **8,000 bytes** from original game arena `4ED650+4ED654`, advances the
arena cursor, and checks it through `1656A0`. Complete arena startup remains a
dependency, not a manufactured array field.
[Impact pool producer](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002284c0.asm),
[BSP representation caller](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/001c6270.asm),
[datum table initializer](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/001f15b0.asm),
[datum table clear/consumer](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/001f1600.asm).

The next independent extension may source kernel critical-section behavior
from a pinned primary compatibility implementation, while retaining that ABI
scope. It must also execute the missing game-arena, impact-pool and BSP-shape
producers in original order before claiming complete new-map construction.
All prior resource/backing/platform boundaries persist. Production and prior
frozen tools/reports remain untouched.
