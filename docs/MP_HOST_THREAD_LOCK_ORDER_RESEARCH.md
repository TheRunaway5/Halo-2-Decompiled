# Real host identity at the original heap lock boundary

This separate four-case extension executes pinned compatibility lock entry and
release bodies with a declared real host-thread identity/atomic provider. Those
bodies do not dereference a thread record. Full original `1C5FF0` advances past
its first heap lock cycle and retains an actual heap allocation, then stops at
original `2DB3A0` reading `[FS:[20] + 250]`. **The manager constructor, full BSP
and world scheduling, insertion and ordinary motion/contact remain unproved.**
No production or earlier frozen tool/report/document is changed.
[Oracle](../tools/oracle_mp_spawn_host_lock_order.py),
[report](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json),
[preceding frozen initializer/BSP suite](MP_KERNEL_SYNC_BSP_PRODUCER_RESEARCH.md).

## Frozen evidence and reproduction

| Artifact | SHA-256 |
| --- | --- |
| Original TU1.1 executable | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| New oracle | `ae50e2c243d1a534008fd187f62ad8eaea3e0503fc7048b9708866d133411500` |
| New report | `4cff6712198ba763550115d184030d25f69eeb46b414b2a5e73f229e05d2a7f8` |
| Canonical original instruction manifest | `13fccda03bd4e2b0795af866c55e137a50d6e74c6f5a9b475e5023634eeecfef` |
| Generated compatibility C++ | `8eef34903408af275b487f641a1025cec44ed9bd107c3e0bb94774d74f4739df` |
| Compiled compatibility text | `dbb95f6cd6a167a62ab9efc1ad3fbe97bc61640ca935dfa1263114ff827d11e4` |
| Compiled compatibility listing | `17de942b08d1953c0e6cb52dc5779965f38eac8cf48106ab936e6067a673e2f4` |

Run `.tools/python/bin/python tools/oracle_mp_spawn_host_lock_order.py`.
The oracle verifies nineteen previous tool/report/document pins and all
primary compatibility source hashes before and after execution. It loads the
previously frozen initializer bytes without rebuilding or rewriting that
suite. New compiled lock text is audited separately from original TU bytes.
The 159 original listings / 15,187 rows contain 1,568 executed listing rows.
The oracle audits 1,897 executed original addresses separately from 56 compiled
compatibility lock addresses. Original listings and the semantic report
reproduce byte-identically; real OS
thread IDs are deliberately stored separately in `run-host-identities.json`
and vary between runs. The normalized report retains each token and verifies
it was paired with an actual kernel-assigned host ID.
[Exact builds, audits and identity normalization](../tools/oracle_mp_spawn_host_lock_order.py),
[separate compatibility instructions](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/compatibility-lock-instructions.asm).

## Exact admitted compatibility operations

The pinned primary Cxbx-Reloaded implementation, commit
`585c49a50af1255ab155099e06f24505f9c5a800`, implements ordinal277 by obtaining
`KeGetCurrentThread`, incrementing `LockCount` and comparing/storing that
identity. Its entry body performs no KTHREAD/ETHREAD dereference. Ordinal294
decrements recursion and lock count, clears ownership when recursion reaches
zero, and wakes the event only when the updated count is nonnegative.
This is a primary compatibility implementation, **not original Xbox kernel
instruction bytes or a hardware result**.
[Primary entry](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlRtl.cpp#L756),
[primary release](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlRtl.cpp#L1323).

The complete entry/release function bodies are extracted literally from the
pinned file and compiled as 32-bit freestanding C++. Diagnostic logging is an
empty macro. The preceding frozen primary structure declarations are reused;
their C `_Static_assert` spelling becomes C++ `static_assert`. Event and
critical-section sizes remain16 and28, with `LockCount`, `RecursionCount` and
`OwningThread` at `+10/+14/+18`. Compiled entries are `09400000` and
`09400090`; their stdcall returns consume the original single argument. The
void APIs receive no fabricated NTSTATUS/success result.
[Generated wrapper](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/locks.cpp),
[primary layouts](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/common/types.h#L1800),
[exact extracted bodies and compiled bytes](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json).

The declared provider pairs each actual Python `current_thread()` object with
`get_native_id()` and assigns a stable guest equality token: `096F0000`, then
`096F0004`. Python documents the latter as the kernel-assigned native thread
ID, unique for the thread's lifetime, and documents primitive lock operations
as atomic. A real `threading.Lock` serializes the guest 32-bit increment/
decrement, returning the updated modulo2^32 bits. Guest tokens are unmapped;
attempted thread-record dereference would fault. **No thread layout, TLS,
FS28, game state or Havok field is provided by these services.**
[Primary host thread/lock API](https://docs.python.org/3.14/library/threading.html#threading.get_native_id),
[provider and unmapped-token assertion](../tools/oracle_mp_spawn_host_lock_order.py).

The compiled provider uses a private stdcall helper protocol, separately from
the original Xbox import ABI; it does not claim to execute original kernel
FASTCALL Interlocked instructions. Helpers at `09500000/+10/+20` provide
identity/increment/decrement. Wait, wake and abort at `+30/+40/+50` remain
unavailable and deliberately stop before receiving a result. Unicorn execution
is sequential across real host threads; no concurrent Xbox scheduler behavior
is claimed.
[Primary Interlocked ABI declaration](https://github.com/XboxDev/nxdk/blob/14d5ee97e73347c973f1f57b68b79ec08c9e77f2/lib/xboxkrnl/xboxkrnl.h#L4512),
[all helper inputs/results and strict stops](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json).

| Actual host operation | Source-compatible count / recursion / owner |
| --- | --- |
| Initialized | `−1 / 0 / NULL` |
| First thread enters | `0 / 1 / 096F0000` |
| Same thread enters recursively | `1 / 2 / 096F0000` |
| First release | `0 / 1 / 096F0000` |
| Final release | `−1 / 0 / NULL` |
| Second actual host thread enters after release | `0 / 1 / 096F0004` |
| Second thread releases | `−1 / 0 / NULL` |
| Separate second-thread contention case | `1 / 1 / 096F0000`; stops before wait |

The contended case reaches compiled call `0940003A`, with wait arguments
`(critical_section,0,0,0,0)`, and stops at helper `09500030`. No owner transfer,
wait success, wake or contended release is substituted.
[Complete stage bytes and blocked state](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json).

## Exact original advancement and PRCB stop

Full original `1C5FF0` calls `148190`, the registered `2DED30`, then requests
`new(28)` at `1C6007`. Original CRT `329353` rounds that request to `30` before
calling original heap allocator `2DAC5E`. Original `2DACAD` invokes ordinal277
through thunk `417BE0`, with heap lock `08000610`. Original heap instructions
split/unlink their source-created free blocks and retain payload `08000690`.
Original `2DB25E` invokes ordinal294 through `417BDC`, restoring count−1,
recursion0 and ownerNULL; no lock service changes any heap object fields.
[Original scheduling](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/001c5ff0.asm),
[original CRT rounding](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/00329353.asm),
[actual allocator/lock call sites](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/002dac5e.asm),
[retained source stores](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json).

Before returning that allocation, original `2DB38D..2DB396` creates notification
words `(08000690,30,0)` on its actual source SEH stack. `2DB39A` loads
`FS:[20]`; `2DB3A0` then reads pointer+250. This suite provides no FS20/PRCB
fields, so that read faults at address250. If nonzero, untouched source code
calls its `+14` function with operation10 and the notification words. The
payload has been allocated, but **`27DAE0` has not run**, `526538/52653C` remain
NULL, and no full-map impact reset, `1C6270`, `1C66A0` or insertion is reached.
The source heap frame remains registered at the deliberate stop.
[Unchanged caller instructions](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/002dac5e.asm),
[precise fault registers, stack words and retained allocation](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json).

Pinned primary compatibility types identify FS20 as `KPCR.Prcb` and its `+0`
as `CurrentThread`. Its `+250` lies in the declared `Unknown` range. The
primary compatibility current-thread routine itself reports incomplete/faked
context. Original executable callers establish only an optional callback
record: heap allocation operation10, free operation11, unhandled exception
operation12, and reboot operation2. **It is not a thread identity.** No original
Xbox PRCB producer or ordinary value for this callback has been established.
[Primary context types](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/common/types.h#L2140),
[primary current-thread caveat](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlKe.cpp#L861),
[original free caller](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/002db3d9.asm),
[original exception caller](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/002d8532.asm),
[original reboot caller](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/002d6db9.asm).

The separately pinned primary `EmuGenerateFS` allocates/zeroes a whole
compatibility KPCR, copies the host TIB, points `Prcb` to `PrcbData`, initializes
DPC fields and attaches a produced ETHREAD. Its zeroing is evidence for that
compatibility setup, not an original Xbox producer. These bytes are inspected
only here; no partial KPCR, callback-null assumption or successful process
startup is supplied. Its ETHREAD producer allocates/zeroes a record, constructs
kernel stack/thread state and inserts an object handle, all additional services
beyond an opaque identity adapter.
[Primary compatibility context producer](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/support/EmuFS.cpp#L657),
[primary compatibility thread producer](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlPs.cpp#L346),
[saved inspected source pins](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json).

| Newly inspected primary compatibility file | SHA-256 |
| --- | --- |
| `EmuFS.cpp` | `b228854d936ed37b6b1a511924d8bf9397106f530b7d4fdf336cca2235fef3d5` |
| `EmuKrnlPs.cpp` | `bca7c1fbd1f1c26b8ec5650eed85ed08893389e68cfc52d6c639b27eb726591c` |

## Original preparation remains separate from complete scheduling

The second full-map case additionally executes the frozen original physical/
game-arena prefix and BSP datum-table producer, then full original `2284C0`.
That routine requests raw `VirtualAlloc(0,1450,101000,4)` and
`VirtualAlloc(0,850,101000,4)`, and constructs impact pools with32 entries and
strides `A0` and40. Only source-requested raw OS storage is supplied at
`09300000/09302000`; the original routine produces all headers. Both remain
inactive (`+29=0`) because full new-map startup stops before enable/reset.
Source physical arena/savegame exclusions and all prior cache/object/component/
SEH/process-heap backing boundaries are retained.
[Original impact initializer](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/002284c0.asm),
[actual requests, source headers and full-map stop](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json).

The previous independent complete `1C6270` BSP representation case remains
frozen. That direct constructor return does not establish complete original
`1C5FF0` ordering: the latter first needs its heap notification context,
manager/callback creation and pool resets before reaching the BSP/world
construction order. This extension contains **zero completed new-map world
initializations and zero ordinary motion/contact cases**.
[Previously frozen direct BSP evidence](MP_KERNEL_SYNC_BSP_PRODUCER_RESEARCH.md),
[original full order](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/instructions/001c5ff0.asm),
[bounded completion counts](../analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json).
