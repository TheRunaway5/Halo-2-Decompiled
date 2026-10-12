# Original DSP section lookup and missing audio/title prerequisites

The independent extension executes the original TU1.1 DSP section lookup from
its exact authored request. It selects the actual `DSPImage` header in original
header order and reaches **`XeLoadSection` ordinal327**, without supplying a
section-load result, reference count, device, DSP record or game admission.
The separately attempted original driver and whole title constructor retain
their earlier failures. **This does not execute whole `192D30`, initialize
DirectSound/DSP, admit an active multiplayer frame, age particles or render a
flash.** All earlier freezes and production files are unchanged.

The authority is the user's original `default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
This experiment does not claim TU1.5 execution or address correspondence.
[Original image](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03),
[preceding source configuration](MP_SMG_EFFECT_FRAME_CONFIGURATION_RESEARCH.md).

## Frozen artifacts and reproduction

| Artifact | SHA-256 |
| --- | --- |
| `tools/oracle_smg_effect_dsp_owner_1_1.py` | `749ec42cc7e379899fd2efdc467c818a69373479724895ebe668510457fd59d6` |
| `tools/research_smg_effect_dsp_owner_1_1.py` | `f60c47f2b9705d11cf03cf3de57d4b9bf24f6b00c7f33241a33f246f5ac680a0` |
| `tools/audit_smg_effect_dsp_owner.py` | `179a33333b92ee5087027f7d93957463e4e77df511f6443b7438627433c03745` |
| Measured/repeated report | `01e36f2d46232289678b90637b47f4d2381781c1bb555794258a8390ed96f679` |
| Static/repeated report | `c5f5aca0ffd22d9d5fffa579c36736c59d75a546c8d34b8b97d44301fdeeb180` |
| Audit | `6375e92d64e384e97ea4b774c5dc371dcdc5e327a3dc4e88db19a2cdd52dc583` |

The audit checks **203 prior pins**, compares **51 dynamic and16 static
files byte-for-byte**, audits **7,865 original instruction rows /1,532 executed
original addresses**, and replays **15 complete audio/arena/header buffers**
through their measured stores. It separately checks the83 executed compiled
primary RTL addresses; these are not counted as original Xbox instructions.
The24 original calls,1,074 reads,3,896 stores and1,217 owning points retain full
CPU/stack inputs and explicit source/compatibility classification.
[Measured report](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/measured/report.json),
[static report](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static/report.json),
[audit](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/audit.json).

Reexecute into a new directory; preserve the frozen paths:

```sh
.tools/python/bin/python tools/oracle_smg_effect_dsp_owner_1_1.py \
  --output analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/check/report.json \
  --clock-replay analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/measured/native-clock-inputs.json
.tools/python/bin/python tools/research_smg_effect_dsp_owner_1_1.py \
  --output analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/check-static/report.json
```

The inherited positive particle/item/local-view preparation and complete fixed
`1788B0` phase are checked against their preceding frozen measured fields.
The existing actual platform-clock startup input is reused; **zero new tick or
system-time queries** occur in these prerequisite roots. The native worker stays
alive. Its host clock/thread scope, failed-audio allocation branch and resource
boundaries remain explicit. The first missing-arena probe is preserved under
`probe-missing-arena`, including its original oracle bytes and report.

## Three separately selected source roots

1. Whole original `663B0` returns and produces its configuration capacities,
   exactly as in the preceding freeze.
2. **`2205C0(451220)`** uses the actual policy pointer pushed by `126C8B` before
   natural source call `126C90`. It stops at **`220611 REP STOSD`**, EDI/ESI=0,
   ECX=`AB6`, write address0/size4. Source audio arena `517FF0` isNULL. Original
   `2205FD` has already increased cursor `517FF4` to`2AD8`, and original
   `1656A0` has produced its checksum. Driver publication `220615`,
   `DirectSoundCreate383DD7` and natural `192D30` have not executed. This is a
   selected prerequisite root after incomplete title scheduling, not a resumed
   complete `126B20` controller frame.
3. Whole original **`12000`** is attempted separately to recover that arena's
   actual owner. `12001 XOR EBX,EBX` and `12003 CALL2DC2D8` execute. Retained
   source cache state selects the original `C0000001` error path, which calls
   `2D81D4` and stops at **ordinal301 `RtlNtStatusToDosError(C0000001)`**,
   caller `2D81D8`, return `2D81DE`. No error mapping or disk result is supplied.
   The arena allocation has not yet executed. Finally a separate exact
   **`2DBDA1(45AEA0)`** source section-loader root is attempted. That argument
   is the unchanged `192D39 PUSH45AEA0` immediately before `192D40 CALL2DBDA1`.
   This loader does not require a fabricated audio device or a `192D30` return.

These roots do not resume one another's interrupted stack. The report retains
each full before/after state, original arguments, source store indices and
completion flag. Source audio globals, the complete XBE header, critical
section, complete represented current item/effect/particle state and game/frame
buffers remain literal; no failure is converted into a success.
[Original driver](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static/instructions/002205c0.asm),
[whole title constructor](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static/instructions/00012000.asm),
[source cache prerequisite](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static/instructions/002dc2d8.asm).

## Source arena and device prerequisites, listed only where unexecuted

Original `12000` owns `517FF0`: `12008..12014` pushes
`VirtualAlloc(NULL,23000,1000,4)`, `12015` calls `33785C`, **`1201A` stores the
actual returned EAX to`517FF0`**, and `1201F` stores its source-zero EBX to
`517FF4`. This fixture has not reached that allocation. The existing earlier raw
heap reservation service supports its declared100000/1000 process-heap
requests; it is not evidence for this title arena or a full VM-manager result.
There is no assigned replacement arena. After the allocation the constructor
has real cache/FP/control/graphics/source memory initialization before its
conditional `126000` and `126B20` calls.

The natural `2205C0` sequence allocates/zeros2AD8, publishes`526474`, then
calls `DirectSoundCreate(NULL,&driver[2AB0],NULL)`. It requires actual device,
caps, distance/rolloff/speaker/headroom/doppler and LightHRTF stages before
`220719` reads that retained device and `220720` calls`192D30`. A preceding
independent audio freeze already reaches its actual kernel allocation boundary
with source-owned audio storage; this new missing-arena root does not supersede
that caller provenance.
[Natural audio/device prefix](SMG_AUDIO_DEVICE_PREFIX_1_1_RESEARCH.md),
[original whole driver](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static/instructions/002205c0.asm).

## Executed header lookup and exact next request

Original `2DBDA1` calls`2DBCFB`. The latter executes actual section count33
and header-table pointer from `1011C/10120`, walks **stride38 in unchanged
header order**, and uses the actual section-name pointer at each header`+14`.
It constructs33 source string descriptors and executes32 comparisons with
case-insensitive byte1. The first31 comparisons returnAL0; the actual
`DSPImage` comparison returnsAL1. The source returns **header`00010A38`**
(original index31); the outer loader passes it to`2DBD62`.

At original **`2DBD68 CALL[417C48]`**, the exact request is:

| Field | Captured input |
| --- | --- |
| Kernel target / ordinal | `80000147` /327 |
| Return address | `002DBD6E` |
| Section header argument | `00010A38`, full38-byte record retained |
| Actual section virtual address | `00585AC0` |
| Virtual/file size | `7060` =28,768 bytes |
| Raw file offset | `49E000` |
| Section name pointer | `00010BC6` → actual`DSPImage\0` |
| Source section reference count | 0 |
| Head/tail page-reference pointers | `00010AD4` /`00010AD6` |

The latter pointers have **16-bit** counter types in the pinned primary
`XBEIMAGE_SECTION`. The report retains4 bytes at each pointer as raw neighboring
storage, with both current2-byte counters zero. No counter width is inferred
from those snapshot lengths. Raw file and preloaded virtual section bytes both
hash to **`e54e4d29d02a3cdaba4be942d702b5e5b29f945d18215d99187603988fa34745`**.
Those bytes already exist because the oracle maps original image sections;
their presence does not establish loader/commit/reference ownership.
[Lookup](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static/instructions/002dbcfb.asm),
[kernel-load caller](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static/instructions/002dbd62.asm).

## Verified compatibility helper and unresolved loader ownership

Complete unchanged primary Cxbx `RtlInitAnsiString`, `RtlEqualString` and
`RtlUpperChar` bodies from pinned commit
`585c49a50af1255ab155099e06f24505f9c5a800` execute as compiled32-bit guest code.
The explicit8-byte STRING layout and stdcall argument removal are checked.
Their external `strlen` calls read actual captured NUL-terminated source bytes
through the host libc, whose file hash and returned lengths are recorded; the
observer does not write a chosen length, descriptor or comparison result.
All65 helper calls and33 actual libc calls, CPU/stack/descriptor inputs/outputs,
compiler bodies and compiled instructions are retained. Only the compiled
compatibility code produces these stores. This does not prove original Xbox
kernel machine-code parity.
[Helper contract](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/provider/rtl-contract.json),
[pinned primary RTL](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlRtl.cpp#L811-L868).

The pinned primary `XeLoadSection` first finds the original section, then calls
its VM manager on an unloaded section before copying bytes and owning the page
and section references. Actual `Xbe::FindSection` compares raw offset, virtual
address and virtual size in original section order. Full VM commit requires
initialized VMA/PTE/PFN/physical-page state; the existing permission-only
adapter does not provide that ownership. **Ordinal327 is stopped without an
NTSTATUS, handle, reference increment or successful commit.** The full bodies
and their hashes are retained for the next implementation decision.
[Pinned primary section loader](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlXe.cpp#L56-L99),
[pinned section lookup](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/common/xbe/Xbe.cpp#L711-L727),
[full VM owner](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/memory-manager/VMManager.cpp#L1269-L1525).

Only after a real section load can original`192D30` allocate48 bytes from its
owned audio arena, write the sevenFFFFFFFF entries/FFFF selector/zero fields,
publish`518480` at`192DBE`, and call`381EAD` with the actual authored section,
location array`{9,10}`, flags1 and output record. A successful original result
then unloads the section and requests effect data4..7/offset20/size8 from the
actual device. These paths are **listed only** here. Current `518480` staysNULL,
`4F4864` stays0 and game readiness stays0; that startup state does not imply an
admitted multiplayer effect would fail to age.
[Original DSP initializer](../analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static/instructions/00192d30.asm).

The next live prerequisites remain the source title/cache error-mapper and arena
startup, natural controller/device ownership, actual section VM/reference
admission and DSP download/data. Their outputs must be produced before
resuming the original frame path toward147DD0/139640 and particle aging.
