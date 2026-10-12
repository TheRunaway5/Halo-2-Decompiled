# Original TU1.1 consumed pickup options and transfer store order

Research date: 2026-10-08. This separate, read-only extension closes the missing consumed options-byte input in the frozen [held-input transfer measurement](MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md). It does not change that measurement, its allocated/equipped environment, any earlier freeze, or production. At original instruction **CFE66**, the actual options byte at `ECX+0C` is **01 in all ten executed comparisons**. This is measured at the consumer; neither its semantic mode name nor the complete live options producer is established here. [Observed report](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/report.json), [measurement tool](../tools/oracle_mp_pickup_transfer_options_1_1.py).

Primary authority remains unchanged TU1.1 `default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and original Lockout cache, SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. All **154 inherited pins** are checked before and after execution. The measurement and its independent audit compare fifteen observed instruction sites directly with original executable bytes. There are no instruction patches, new guest stores, supplied options values, or new supplied callback results. [Byte audit](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/audit.json), [audit tool](../tools/oracle_mp_pickup_transfer_options_audit_1_1.py).

## Owning options records

Original `CFE60` loads `ECX=[4EDF18]`, then `CFE66` executes bytes `80 79 0C 04`, `cmp byte ptr [ecx+0C],4`. Every observed pointer is `0101A000`; each site retains the sixteen literal options-prefix bytes, actual global/consumed pointers, registers, phase and ordered observation ID. `sites[].cfe60_call_id` identifies the new owning call; that call's `frozen_transfer_call_id` identifies its unchanged full-unit input/output record in the earlier transfer report. This correlation checks actual registers, stack words, return EAX and the entire unit prefix through `+7FF`. [Source CFE60 listing](../analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/instructions/000cfe60.asm), [new owning observations](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/report.json).

| Case | New CFE60 call IDs → earlier frozen transfer call IDs | Consumed byte |
|---|---|---|
| `starting_smg` | `3→32` | `01` |
| `primary_br` | `3→32`, `7→113` | `01`, `01` |
| `secondary_smg` | `3→32`, `7→115` | `01`, `01` |
| `switch_back_to_smg` | `3→32`, `7→113` | `01`, `01` |
| `drop_br` | `3→32`, `7→113`, `13→166` | `01`, `01`, `01` |

The byte is part of the inherited frozen fixture's supplied options environment. Observing its actual consumption closes the stored-input gap without converting it into proof of authored full-session initialization. The value-4 early-return branch is source-listed but unexecuted by these five cases. No new variant/mode constant follows from this evidence.

For each executed non-4 call, `CFE87` reads the low byte of unit `+210`, increments and masks it with `0F`, then **CFE9B writes the 16-bit word at +210 before CFEA2 writes +214**. Actual stores, not reconstructed arithmetic, are retained. The `FFFF` comparison after masking remains in the original listing. Raw return EAX is provenance; this routine has no newly asserted Boolean return contract.

## Exact insertion and drop sequence

`CFED0` consumes stack unit datum, `ECX` item datum, signed `DX` slot, and `EAX` removal selector. It returns with `ret 4`. The initial SMG insertion is unit `00010001`, item `00010002`, slot 0. Ordinary BR/secondary-SMG insertion retains actual target datum `00020003`, slot 1, removal selector `FFFFFFFF`. No new allocation occurs in this transfer. [Source CFED0](../analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/instructions/000cfed0.asm), [correlated actual input records](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/report.json).

Insertion executes, in order:

1. `FB120(item, owner=unit)` assigns ownership and completes its bounds/location work while retaining world registry membership.
2. `FB3F0(item)` finishes world removal; its return already has empty registry chains.
3. **CFF0F** stores the same full item datum into `unit+218+slot*4`.
4. **CFF16** writes zero to `unit+228+slot*4`.
5. **CFF59** tests the network entity at `unit+D4`; the observed `NONE` branch proceeds to `CFE60`.
6. **CFE66**, **CFE9B**, **CFEA2**, then **CFEB6** consume options, advance both sequence words, and test the network entity again.

Owner assignment, registry removal, slot storage and parent attachment retain the distinctions already pinned in the [earlier transfer freeze](MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md#ownership-registry-membership-and-parent-attachment-are-separate). The new audit verifies this exact four-store insertion order inside every observed `CFED0` call; it does not turn ownership assignment into registry removal.

The inherited direct BR-drop root consumes `ECX=FFFFFFFF`, `DX=1`, `EAX=0`, stack unit `00010001`. In `drop_br`, new call 9 correlates with earlier frozen call 148. **CFF30 clears slot 1 before CF8C0**. Actual CF8C0 entry has `EAX=0`, `EBX=00010001`, `ESI=00020003`; incidental captured ECX/EDX values remain provenance rather than additional ABI arguments. Source `FB490` detach precedes `FB120(owner=NONE)`. The observed CFB00 call uses `ECX=00010001`, stack item `00020003` and scale word `3F800000`. The later CFE60 call advances sequence words 2→3. [CF8C0 source](../analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/instructions/000cf8c0.asm), [actual drop call/store observations](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/report.json).

The exact observed drop stores are order 42, **CFF30**, address `0100A21C`, `03000200→FFFFFFFF`; order 53, **CFE9B**, `0100A210`, `0200→0300`; order 55, **CFEA2**, `0100A214`, `0200→0300`. This branch does not execute the CFF16 ready-word reset. Its inherited unavailable Havok return remains explicit; the drop result is not proof of live physical placement or settling.

## Network callback boundary

All **twenty actual CFF59/CFEB6 comparisons** capture both loaded EAX and stored `unit+D4` as `FFFFFFFF`. **B6760 executes zero times**. Thus there are no measured non-NONE callback inputs or outputs to invent. [Literal network gate observations](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/report.json).

The unexecuted branches are exactly source-listed: at CFF74/CFF75, the slot dirty callback pushes mask `(1 << ((slot+12h)&31)) | (1 << ((slot+0Eh)&31))`, then the entity, and calls B6760 at **CFF76**, before CFE60. At CFEBC/CFEC1 it pushes `2000h`, then the entity, and calls B6760 at **CFEC2**, after both sequence stores. These are instruction-derived prospective arguments, distinct from the twenty executed NONE gates. The removal-selector-4 branch also remains unexecuted. [Original CFED0 and CFE60 listings](../analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/instructions/000cfed0.asm).

## Freeze and reproduction

Coverage is **5 cases, 45 observed calls, 69 sites, 39 exact correlated stores, and 20 CFE60/CFED0 full-unit input/output correlations**. Untouched preflight and measured runs both reproduce the entire old attachment report at SHA-256 `657104018ea629e0f58c26f87ef08e9778d75d4c0657419515b695bdfeffc67f`; every original listing write is compared with frozen bytes and suppressed. A separate process reproduces the new report, both old-baseline copies and audit byte for byte. [Reproducibility](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/reproducibility.json), [compact schema](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/schema.json).

New report SHA-256: `db8b94107fe570e29e0a75d0fc62c91f667fce565a116efd2d9216a9ad94f411`. Measurement tool: `48deb72187702c3660fd3d018cabbe22d2647c6fb9fa94118213d3437904022d`. Auditor: `361b8697415c9894a030d3d5df8e9be0c855c2bd45f390e749a9ddcb20ba6ce6`. [Manifest](../analysis/multiplayer/integration/inventory-source/pickup-transfer-options/manifest.json) pins the document, tools, payloads and prior evidence.

```sh
.tools/python/bin/python tools/oracle_mp_pickup_transfer_options_1_1.py
.tools/python/bin/python tools/oracle_mp_pickup_transfer_options_1_1.py --output-dir analysis/multiplayer/integration/inventory-source/pickup-transfer-options/reexecution
.tools/python/bin/python tools/oracle_mp_pickup_transfer_options_audit_1_1.py
.tools/python/bin/python tools/oracle_mp_pickup_transfer_options_audit_1_1.py --output-dir analysis/multiplayer/integration/inventory-source/pickup-transfer-options/reexecution
```

Earlier nearby-root, arena/residency, bounded scheduler and direct-drop Havok boundaries remain unchanged. The separately frozen [current item producers](MP_ITEM_CURRENT_PRODUCERS_RESEARCH.md) establish initial source heap/node/bounds/location/unsorted registry production with genuinely zero player masks; they do not establish active current player/item motion or contact. Genuine player/view/PVS activation, current animated poses, full world motion/contact, live options initialization and Xbox reciprocal parity remain open producer gates. This extension is a stored-state input/ordering contract for native integration, not a live pickup completion claim.
