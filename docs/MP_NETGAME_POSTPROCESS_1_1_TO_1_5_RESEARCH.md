# Original multiplayer netgame postprocess difference: 1.1 to owned 1.5

The owned 1.5 executable adds a real Relic map correction before the unchanged combat-tag tuning. Its source lowers the two authored teleporter sources by float bits `3DCCCCCD` (0.10000000149011612). The original 1.1 routine performs no corresponding Relic write. This is a bounded, executed multiplayer map behavior difference, **not a claim that weapon tuning changed in 1.5**.

## Authority and complete original execution

The two unchanged original entries are `1.1 0011DF00` and `1.5 0011F060`. Their actual map-load callers `0011DE10 / 0011EF70` invoke them immediately before `00164750 / 00165B30` combat postprocessing. The complete callers and combat routines are retained in the separate [combat freeze](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md). Original XBE pins are `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` (disc 1.1) and `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` (owned 1.5).

[`tools/oracle_mp_netgame_postprocess_updates.py`](../tools/oracle_mp_netgame_postprocess_updates.py) independently executes both entire original routines on raw owned `beavercreek.map`, `lockout.map`, and `dune.map`, then repeats all six cases. All reports, original instruction listings, and complete before/after netgame record buffers match byte-for-byte. The original source requests permission changes through its actual XAPI wrapper `002D7A1A / 002DB4CA` and kernel ordinal 182; no XAPI function is stubbed as successful.

The actual raw scenario pointer is a declared input binding at `4E7920 / 4ECC20`. Full raw cache metadata is mapped at its authored virtual addresses. This fixture executes the complete bounded tag consumer, not the original cache loader, preceding tag fixups, whole map activation, teleport traversal, or player/world simulation. The map-ID/reference walk is unchanged source code, not a supplied map-ID selector.

## Exact source guards and field ownership

Both routines first require signed 16-bit scenario `+10 == 1` (multiplayer Type). They resolve the map identifier through:

```text
level_record = *(u32 *)(scenario + 39C)
multiplayer_level_record = *(u32 *)(level_record + 14)
map_id = *(i32 *)multiplayer_level_record
netgame_count = *(u32 *)(scenario + 118)
netgame_pointer = *(u32 *)(scenario + 11C)
```

These pointer walks have no count/NONE checks in this routine; the fixture uses actual valid authored records. Netgame flags have stride `20`, position `+0/+4/+8`, and signed16 Type `+10`. Pinned Assembly Halo2 `scnr.xml` corroborates the labels/offsets (Type7 teleporter source, Type8 destination). No tag path or displayed map name replaces the actual ID gate.

The 1.1 routine recognizes map ID100 only. The 1.5 routine recognizes ID100 and the added ID1200 branch. Other map IDs return without permission requests or netgame stores, as independently executed on Lockout ID50.

## New 1.5 Relic branch

The actual owned `content/c/4d53006400000003/dune.map` resolves ID1200. Its authored rows2/3 are Type7 and exactly match these original constants:

| Row | Constant addresses X/Y/Z | Exact target bits X/Y/Z | Raw position |
|---|---|---|---|
| 2 | `46A5BC / 46A5B8 / 46A5B4` | `425BC1BE / C33ADAA0 / BFABFB62` | 54.93920135498047, -186.85400390625, -1.343609094619751 |
| 3 | `46A5B0 / 46A4FC / 46A5AC` | `4280C8CE / C3070000 / C00A38B4` | 64.39219665527344, -135, -2.1597108840942383 |

The source reads these two exact rows in order, not all netgame flags. After checking row Type7, it computes each target-minus-current component in separate `SUBSS` operations. It squares Z, Y, X independently using `MULSS`, accumulates **`(Z² + Y²) + X²`** in that order with `ADDSS`, then executes `COMISS threshold, distance_squared / JBE skip`. Threshold at `46A0EC` is bits `38D1B717` (0.00009999999747378752); equality and unordered comparisons skip the correction. Every listed operation rounds as single precision.

On admission, the source writes X as `+0 + current_X`, Y as `+0 + current_Y`, and Z as `current_Z - float(3DCCCCCD)` with three separate `ADDSS/ADDSS/SUBSS` stores. Preserve these operations, including equal-value X/Y stores; do not replace the routine by an unconditional coordinate override.

| Source store | Row/member | Exact raw bits | Exact 1.5 output bits/value |
|---|---|---|---|
| `0011F296` | 2/X | `425BC1BE` | unchanged |
| `0011F2AB` | 2/Y | `C33ADAA0` | unchanged |
| `0011F2BD` | 2/Z | `BFABFB62` | `BFB8C82F` / -1.4436091184616089 |
| `0011F296` | 3/X | `4280C8CE` | unchanged |
| `0011F2AB` | 3/Y | `C3070000` | unchanged |
| `0011F2BD` | 3/Z | `C00A38B4` | `C0109F1A` / -2.2597107887268066 |

The two 1.1 Relic cases return without any write or permission request; the two complete 1.5 executions perform six stores with precisely two value changes. Both request protection4 then2 for the authored netgame block `801E6814 / E40`. No new metadata is created and no player teleport or physics result is supplied.

## Retained Beaver Creek branch

Owned `assets/disc/maps/beavercreek.map` resolves map ID100. Both originals admit authored Type8 rows5/6 close to their exact source target constants. They perform six XYZ stores with two actual X changes:

| Row | Exact raw X bits/value | Both measured output X bits/value |
|---|---|---|
| 5 | `41932711` / 18.394075393676758 | `41992711` / 19.144075393676758 |
| 6 | `C1838AAA` / -16.442707061767578 | `C1898AAA` / -17.192707061767578 |

The source adds/subtracts its `3F400000` (0.75) constant and retains the authored Y/Z through explicit stores. Complete outputs and store order match on the actual authored cache in both versions. This statement does not generalize floating-point boundary behavior to every altered input; the two full instruction listings remain authority for each image's arithmetic order.

## Genuine access-service boundary

The original XAPI adapter checks whether requested size is zero, then tail-jumps through the unmodified import to ordinal182 `MmSetAddressProtect(BaseAddress, NumberOfBytes, NewProtect)`. Pinned primary compatibility source declares a void ABI; no status is tested by these game callers. The adapter preserves all guest general registers, consumes exactly the three stack arguments, and resumes only after an actual applied permission operation.

This fixture reads the existing independently frozen compiled compatibility artifact; it never calls its builder or rewrites that earlier freeze. SHA-256 pins: compiled unchanged primary bodies `7658c40ade81ad9513652926c561fd2a9f0b4526cc87764742f266501afd48a4`; library `61b354c312ced6fa882eeb1371715eafc82328f5e780017c1688e29509741e23`. Its source-derived validator/PTE mask, Xbox patch, Windows conversion and host-facing `UpdateMemoryPermissions` execute in the actual library. The declared host transport covers pages intersecting the original address/size and calls `Unicorn.mem_protect`; permission regions and complete before/after page-content hashes are measured. Protect4 becomes compiled host40 and applied R/W/X7; Protect2 becomes host20 and applied R/X5. The service changes no content, field, RNG, gameplay state or source return register.

This proves the stated compatibility access operation, **not** original Xbox PTE ownership, complete `VMManager::Protect`, caches, hardware memory protection or Xbox physical address behavior. The exact primary declaration and conversion bodies are retained under `analysis/multiplayer/integration/spawn-source/local-map-protection/primary-source`, with original [declaration](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlMm.cpp#L433-L452), [conversion](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/memory-manager/PhysicalMemory.cpp#L377-L643), and [host-facing application](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/memory-manager/VMManager.cpp#L2303-L2317). The prior [access-provider freeze](MP_LOCAL_MAP_PROTECTION_RESEARCH.md) remains unchanged.

## Reproduction and limits

Reports/listings/full netgame buffers are in `analysis/multiplayer/title-update-1.5/netgame-postprocess/{measured,repeat}`; reproduce into a new output directory with `.tools/python/bin/python tools/oracle_mp_netgame_postprocess_updates.py --output <new-directory>`. The independent audit retains all prior combat/sandbox evidence unchanged.

This closes one exact source difference for a future native processed metadata view. It does not close native teleporter consumption, full latest-update gameplay, grenade/melee/projectile integration, explosives, gravity lifts, live map activation or Xbox hardware parity. No production file is edited.
