# Original title arena prerequisite after the real error-mapper delegate

The new independent continuation executes the complete pinned primary
`RtlNtStatusToDosError` body through its actual native NtDll delegate. Original
`12000` proceeds beyond ordinal301 and reaches its **real title-thread TLS
receiver failure at `2D81CB`**, before the audio-arena `VirtualAlloc` request.
No TLS slot, arena, pointer, allocation result, device, DSP record, game flag or
elapsed value is supplied. The preceding driver/DSP lookup freeze remains
immutable. **Complete title startup, natural audio/DSP admission, active
multiplayer effect aging and rendered muzzle flash are not established.**
[Preceding DSP/constructor freeze](MP_SMG_EFFECT_DSP_OWNER_RESEARCH.md).

## Immutable evidence and repeat

| Artifact | SHA-256 |
| --- | --- |
| `tools/oracle_smg_effect_title_arena_1_1.py` | `bdd55202e9677227e9fe0afa95fe5f95f163f98853a753cc19c89c1809befbba` |
| Measured/repeated report | `0bdb5442cc67ba9f0df9118d75852a629bd95b4cc69c41cde969316d333fca41` |
| Final audit-v2 | `48936c93344cef99e4225502a30fddfba3129490010ded7fd31b85c2db5f5fe2` |

Authority remains the user's original TU1.1 executable SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
No TU1.5 execution/address equivalence is claimed here. The audit checks209
prior pins and all200 files in the preceding DSP manifest; compares52 measured
files byte-identically; audits6,793 original instruction rows/1,544 executed
original addresses; and replays15 complete owning buffers. The original
configuration/failed-driver records remain exactly equal to the preceding
freeze. All65 section-lookup RTL inputs/outputs and the final327 stop remain
equal, independently of the extra original title-error instructions.
[Report](../analysis/multiplayer/integration/inventory-source/smg-effect-title-arena/measured/report.json),
[audit](../analysis/multiplayer/integration/inventory-source/smg-effect-title-arena/audit-v2.json),
[oracle](../tools/oracle_smg_effect_title_arena_1_1.py).

The earlier core `audit.json` is preserved; `audit-v2.json` adds the separately
listed actual XBE TLS/entry owner without changing measured evidence. The
read-only extension reuses the frozen compiled RTL library; it does not invoke
the old RTL builder. Platform-clock initialization reuses its captured actual
inputs. There are no new system-time or tick queries, assigned increments or
particle state changes.

Reexecute into another output directory:

```sh
.tools/python/bin/python tools/oracle_smg_effect_title_arena_1_1.py \
  --output analysis/multiplayer/integration/inventory-source/smg-effect-title-arena/check/report.json \
  --clock-replay analysis/multiplayer/integration/inventory-source/smg-effect-title-arena/measured/native-clock-inputs.json
```

## Actual error mapping and original consumer order

The source request is unchanged: whole `12000` calls`2DC2D8`, whose retained
startup/cache state selects `2DC2FB PUSH C0000001` followed by`2DC300 CALL
2D81D4`. Original `2D81D8 CALL[417B70]` requests ordinal301 and expects return
`2D81DE`. The SDK gives `ULONG NTAPI RtlNtStatusToDosError(NTSTATUS)` with one
32-bit stdcall argument and a32-bit mapped error return.

The complete primary Cxbx body from pinned commit
`585c49a50af1255ab155099e06f24505f9c5a800` calls its native NtDll function.
The independent32-bit Wine process resolves and invokes that real named export.
The captured result is **`C0000001 -> 0000001F` (31)**. The observer transports
that actual EAX value and the exact one-argument return routing, with no guest
memory writes. Original `2D81DE` then copies EAX toEDX, original`2D81E0` pushes
EDX, and original`2D81E1` calls`2D81A6 SetLastError`.
[Pinned complete primary mapper](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlRtl.cpp#L1494-L1548),
[measured ABI/native return](../analysis/multiplayer/integration/inventory-source/smg-effect-title-arena/measured/report.json).

The runtime reports `C:/windows/system32/ntdll.dll`; its actual first16 export
bytes equal the named PE32 export in the pinned local native DLL, SHA-256
`3ef26240ab8d96a9a4ee6fde8eba5ef89d893a3158f8f40ccc1d2d2b27a6bb2b`.
The audit checks machine`014C`, actual export RVA`2EEB0`, file offset192176 and
the complete16-byte match. The generated wrapper retains the complete primary
body, compiler command, actual DLL identity and inputs/outputs. The compiled
executable SHA is
`8253dbeb7d26af9b01ff9662e170515e8e3851e1d59eb1afba74eefa0acfc9d9`.
Native Wine TEB side effects, volatile-register/flags behavior and retail Xbox
kernel instruction parity are outside this bounded EAX/ABI service; they are
not transported as title-thread state or admission.
[Provider contract](../analysis/multiplayer/integration/inventory-source/smg-effect-title-arena/provider/mapper-contract.json).

## Exact source title-thread/TLS stop

The unchanged `SetLastError` body produces these actual reads in order:

| Original instruction | Actual address/input |
| --- | --- |
| `2D81A6 MOVZX EAX,byte FS:[24]` | `00AC0024` →IRQL byte0 |
| `2D81B2 MOV EAX,FS:[28]` | `00AC0028` →`00AD0000` |
| `2D81B8 MOV ECX,FS:[4]` | `00AC0004` →`00ABFCB0` |
| `2D81BF MOV EAX,[487620]` | Retained TLS index0 |
| `2D81C4 MOV EAX,[ECX+EAX*4]` | `00ABFCB0` →literal`0000027F` |
| `2D81C7 MOV ECX,[ESP+4]` | `021FCDAC` →actual error31 |

**`2D81CB MOV[EAX+8],ECX` attempts write31 at `00000287`, size4**, and the
real memory dependency stops execution. It does not complete a TLS/error store.
Full CPU/stack and source reads are retained. The slot is neither replaced nor
clamped; no scratch title TLS record is allocated or filled.
[Original error writer](../analysis/multiplayer/integration/inventory-source/smg-effect-title-arena/measured/instructions/002d81a6.asm).

`FS:[4]` belongs to the complete native-produced compatibility KPCR/TIB and
points into its actual captured host stack. That fixture is explicitly not an
ordinary XAPI title thread: it does not own the image entry's TLS index or a
source-created title TLS allocation. Treating a host stack word as a usable
title slot would manufacture the missing owner. This stop concerns the bounded
compatibility fixture; it is not evidence that normal retail startup fails.
[Existing whole-context boundary](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md).

## Original missing layout owner, listed only

The actual XBE entry is **`2D6F2E`**. Its authored TLS directory at`418070` is:

```text
raw start=00000000, raw end=00000000, index address=00487620,
callbacks=00000000, zero-fill=00000090, characteristics=00000000
```

Original `2D6F46..2D6F63` computes
`align16(zero_fill - raw_start + raw_end) +4`; original`2D6F66` stores that size
to`566EF8`. It then divides by-4 at`2D6F6C`, loads the actual directory's index
address, and stores the index there at**`2D6F80`**. The literal header inputs
imply size`94` and index`FFFFFFDB` (-37). These are the source formula's listed
results, **not measured initialization outputs in this fixture**. Both owning
stores are unexecuted in the measured suite.

Original `2D6F82` requests `CreateThread` with zero security/stack/parameter/
flags/thread-id and actual startup`2D6EBA`. Real thread/TLS allocation and its
startup must then own the storage addressed through the negative index.
Executing the index store alone cannot convert the captured host stack into a
title thread. The audit verifies the complete prior original-entry listing and
all literal TLS-directory/owning-store bytes against the original image.
[Original entry/layout owner](../analysis/multiplayer/integration/spawn-source/process-heap-world/instructions/002d6f2e.asm),
[original process-startup gates](MP_PROCESS_HEAP_WORLD_STARTUP_RESEARCH.md).

## Remaining arena/device/frame boundary

Because the real error writer stops first, **`12015 VirtualAlloc(NULL,23000,
1000,4)` and stores`1201A/1201F` remain unexecuted**. No available generic raw
heap fixture or permission-only adapter proves the title arena's allocation.
The complete source-created title-thread/TLS owner is required before retrying
this natural constructor. Whole title scheduling and the disk/cache branch
remain explicit, even though the current branch fails before a disk request.

The selected driver still stops at`220611` withNULL audio arena. The exact
authored DSP section loader still reaches ordinal327 with actual header`10A38`,
section bytes and reference0; its full VM/reference service remains
unavailable. Neither route publishes a driver or `518480` DSP record. Source
`4F4864` and readiness remain0, and represented current effect/particles are
unchanged. These are startup-boundary observations, not a conclusion about
effect aging after a genuinely admitted active multiplayer frame.
