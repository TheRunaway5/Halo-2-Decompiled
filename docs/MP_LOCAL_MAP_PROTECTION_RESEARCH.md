# Original map-start with applied compatibility memory protection

The unchanged map-start now completes callback16 `125370` through an actually applied emulator access-protection adapter. It then completes callback17 `147D20` on the explicitly inherited clock backing and stops at callback18 `1E2EA0`: `1E2EB2` clears a NULL record because its owning `1E2E50` constructor has not run. Readiness, local membership, controller/view binding, lobby/countdown flow and the actual local launch participant count remain unresolved. No production or previous frozen artifact is changed.
[Independent tool](../tools/oracle_mp_spawn_local_map_protection.py),
[complete report](../analysis/multiplayer/integration/spawn-source/local-map-protection/report.json),
[preceding pool boundary](MP_LOCAL_MAP_POOL_RESEARCH.md).

## Original request and ABI

The original import417BA8 contains encoded ordinal182. `2D7A1A` checks the size argument, returns with `ret0Ch` for zero size, otherwise tail-jumps through that import. Callback16 supplies these actual requests:

| Return address | Base | Bytes | Original protection |
|---|---|---|---|
| `125387` | `82D47000` | `3BE000` | `4` |
| `12539D` | `83105000` | `40000` | `404` |

These partition the existing source-requested `3FE000` game arena. Its earlier actual `217020` protection request, returning at `217070`, is also serviced by the applied adapter. No storage enlargement or relocation occurs.
[Original callback16](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/00125370.asm),
[original thunk](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/002d7a1a.asm),
[original arena owner](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/00217020.asm),
[literal ABI and all three applied calls](../analysis/multiplayer/integration/spawn-source/local-map-protection/report.json).

Pinned primary compatibility code independently declares ordinal182 as `void MmSetAddressProtect(BaseAddress, NumberOfBytes, NewProtect)` and delegates to `VMManager::Protect`. The original caller does not test a status. The adapter preserves EAX and the other general registers, removes exactly three stack arguments and resumes at the actual return address after applying permissions.
[Primary declaration](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlMm.cpp#L433-L452),
[retained untouched source](../analysis/multiplayer/integration/spawn-source/local-map-protection/primary-source/EmuKrnlMm.cpp),
[register and stack evidence](../analysis/multiplayer/integration/spawn-source/local-map-protection/report.json).

## Declared compatibility adapter

The pinned compatibility header defines `4` as READWRITE and `400` as WRITECOMBINE. Its validator accepts these masks; its patcher adds execute access; its Windows converter preserves the cache modifier. The unchanged host-facing `UpdateMemoryPermissions` deliberately removes WRITECOMBINE/NOCACHE before calling `VirtualProtect`, explaining that this compatibility implementation does not emulate caches.
[Primary flags](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/init/CxbxKrnl.h#L110-L122),
[primary conversion bodies](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/memory-manager/PhysicalMemory.cpp#L377-L432),
[primary patch/conversion](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/memory-manager/PhysicalMemory.cpp#L523-L643),
[primary host-facing application](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/memory-manager/VMManager.cpp#L2303-L2317).

The new tool compiles those four function bodies unchanged, with declared compilation types/constants/logging and a `VirtualProtect` transport. It records the validator's derived PTE mask as evidence. The transport actually calls `Unicorn.mem_protect` on existing pages, checks every overlapping region's permissions and compares complete before/after content hashes before the void return. The previous access mask is derived from the actual emulator regions. All three operations return from the applied platform call without modifying memory contents or guest general registers.
[Compiled exact bodies and transport](../analysis/multiplayer/integration/spawn-source/local-map-protection/compiled-compatibility-access.cpp),
[source and library pins plus each content/permission audit](../analysis/multiplayer/integration/spawn-source/local-map-protection/report.json).

| Request | Validator PTE mask | Patched Xbox | Converted Windows | Applied host access | Unicorn permissions |
|---|---|---|---|---|---|
| `4` | `63` | `40` | `40` | `40` | `7` (read/write/execute) |
| `404` | `6B` | `440` | `440` | `40` | `7` (read/write/execute) |

This is an explicitly supplied compatibility access service. Full `VMManager::Protect`, its memory-manager/PTE ownership, Xbox cache behavior and hardware write-combine semantics are not executed or claimed. No guest PTE, game field, readiness, clock value, participant, profile, admitted input or successful world result is provided by the adapter.
[Primary full MM boundary](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/memory-manager/VMManager.cpp#L1056-L1095),
[explicit boundary ledger](../analysis/multiplayer/integration/spawn-source/local-map-protection/report.json).

## Actual source advancement and remaining dependency

Earlier actual `125280` tail reserves `1288` bytes, owns `[4ED664]=82D47000` at `1252FD` and sets its arena cursor/checksum. Callback16 now clears and fills that complete metadata record, writes its flags and copies the existing game options. The excluded original `217090` savegame/thread I/O remains the preceding fixture's declared boundary; this extension supplies no I/O completion. Complete metadata and before/after callback records are retained.
[Actual metadata owner](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/00125280.asm),
[unchanged excluded I/O listing](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/00217090.asm),
[complete metadata and source stores](../analysis/multiplayer/integration/spawn-source/local-map-protection/report.json).

The actual ascending reset order is `2→11E330`, `9→187550`, `13→19F720`, `16→125370`, `17→147D20`, `18→1E2EA0`. The first five return. Callback17 uses the earlier explicitly supplied legacy clock record at `01016000`; the literal full24-byte entry/exit buffers are retained. Its true `147CC0` storage owner is listed but not added here. This reset observation does not close game-clock ownership.
[Original traversal](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/00139410.asm),
[clock reset](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/00147d20.asm),
[unexecuted clock storage owner](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/00147cc0.asm),
[independent owning clock research](MP_ITEM_TICK_OWNER_RESEARCH.md).

At `1E2EB2`, actual EDI/EBX are zero, ECX is `39`, EAX is zero and callback18's `[4FCDC4]` remains NULL. Its original storage callback `1E2E50` reserves `E4` bytes from the existing arena, executes checksum `1656A0`, clears39 dwords and binds `[4FCDC4]` at `1E2E92`. That next owner is statically recovered and unexecuted in this freeze. No NULL field is filled to admit it.
[Exact stop and retained records](../analysis/multiplayer/integration/spawn-source/local-map-protection/report.json),
[callback18 reset](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/001e2ea0.asm),
[next owning constructor](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/001e2e50.asm).

Full `190A30` remains interrupted inside `139410`, before `1395B0` membership, `11D780` BSP startup or `190A6E` readiness. Source player membership remains empty; original menu/lobby/countdown and local participant-count producers have not executed. Those cannot be inferred from the number of level handles, camera slots or available split-screen views.
[Map-start order](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/00190a30.asm),
[unexecuted membership producer](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/001395b0.asm),
[complete retained state](../analysis/multiplayer/integration/spawn-source/local-map-protection/report.json).

## Reproduction and freeze

The same existing measured platform clock inputs replay verbatim. Native context and mmap addresses are kept in separate files because host addresses can change; no new clock or device input is selected. The tool verifies every prior pin and every executed original instruction against the untouched XBE and emulator bytes. An existing differing report is refused.

One bounded case verifies12,914 executed original instruction addresses and75 instruction files (73 routines, complete executed listing and254 observed store sites). The independent report and every listing match byte-for-byte, with172 previous artifact/input pins preserved. The compiled compatibility source/library and all8 untouched primary files are also pinned in the report and verification.

```bash
.tools/python/bin/python tools/oracle_mp_spawn_local_map_protection.py \
  --output analysis/multiplayer/integration/spawn-source/local-map-protection/reproduction/report.json
```

[Executed original bytes](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/executed-original-instructions.asm),
[observed source store sites](../analysis/multiplayer/integration/spawn-source/local-map-protection/instructions/observed-map-protection-source-store-sites.asm),
[independent verification and artifact pins](../analysis/multiplayer/integration/spawn-source/local-map-protection/independent-freeze-verification.json).
