# Original terrain endpoint retreat and relocalization

The previously unexecuted **`16BA30`** branch now has an independent original-instruction suite: **24 cache-derived probes execute 2,122 retreat iterations**, including positive-fraction exits, repeated failed relocalizations, and zero-clamp exits. Contacts, normals, fractions and locations are all produced by the unchanged original routines using actual Lockout collision data. These are explicit terrain probes with mask`24800001`; they are **not claimed as live `C57A0` emissions**. The frozen pickup visibility suite and its unexecuted-retreat coverage statement remain unchanged. [Oracle](../tools/oracle_mp_pickup_retreat_1_1.py), [report](../analysis/multiplayer/integration/inventory-source/pickup-retreat/report.json), [audit tool](../tools/oracle_mp_pickup_retreat_audit_1_1.py), [audit](../analysis/multiplayer/integration/inventory-source/pickup-retreat/audit.json).

The authority is untouched TU1.1 XBE SHA`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, Lockout cache SHA`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`, and exact resident animation resource bytes from the disc caches. No source instruction, terrain contact or location result is replaced. [Frozen outer-query scope](MP_PICKUP_VISIBILITY_RESEARCH.md), [frozen raw collision layout](MP_PICKUP_SPATIAL_RESEARCH.md#raw-terrain-inputoutput-contract).

## Source inputs and execution order

The independent fixture inherits actual initial SMG allocation, assigned/equipped Spartan, original combat idle185, original right-hand attachment and current graph pose. It allocates authored netgame BR rows0/1 and SMG row12 through the original paths, preserving their full datums and source registry order. Original authored start0 facing/root store, `BDF10` pose/bounds and `11D4A0` location execute before `C57A0`. The first emitted start point is the source-produced capsule top, with exact bits **`40E11301 BE09D990 C0F7540C`**. That output is copied without arithmetic as the endpoint of twelve reversed vertex→capsule probes. Heap backing, allocated unit/player arenas, resource residency and excluded first-person view retain their inherited boundaries. [Oracle `main`](../tools/oracle_mp_pickup_retreat_1_1.py), [pose/attachment dependency](MP_WEAPON_ATTACHMENT_RESEARCH.md), [C57A0](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/000c57a0.asm), [capsule](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/000e0c20.asm).

Twelve probes use actual BSP vertex records **188,756,761,768,770,773,774,775,783,784,895,914** as origins and the source capsule top as endpoint. Another twelve use authored collision-edge endpoints, retaining the edge's u16 vertex indices at`+0/+2`. The edge stride is`0C`; the vertex stride is`10`, with xyz at`+0/+4/+8`. The original surface walk independently uses these same vertex-index offsets and stride at`1E2669–1E2670` and`1E2698–1E26A3`. The report records each complete edge/vertex record and exact cache offset; no endpoint is invented or moved to force contact. These boundary-point probes do not claim valid player trajectories or authored weapon placements. [Raw records](../analysis/multiplayer/integration/inventory-source/pickup-retreat/report.json), [original edge consumer](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/001e25c0.asm).

Each probe first executes `11D4A0` for both actual endpoints, then **`16BAC0`** with start in EAX, endpoint in ECX, ignored unit datum in EDX, and stack words mask`24800001`, other ignored datum`FFFFFFFF`, result pointer. Original separately rounded endpoint−start forms delta; `16BB06` calls `16B240`, whose recorded caller is **`16BB0B`**. The result buffer's stale hit fields are not supplied contacts. [Wrapper](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/0016bac0.asm), [dispatcher](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/0016b240.asm), [oracle `segment`](../tools/oracle_mp_pickup_retreat_1_1.py).

The lower service executes **`1E2020→1E20C0`** on actual current Lockout collision BSP`800AB3C8`: flags`9`, root0, range[0,1], initial best`7F7FFFFF`, prior leaf/planeNONE, class0 and the original intact breakable bits. Mask`24800001 & E` is zero, so these probes need neither the registered-object ray branch nor active Havok. The source capsule visibility producer also executes eight original ordinary-terrain casts; the new report therefore has **32 outer casts: 30 terrain contacts and two clear side clips**. Full heap/resource and registry producers retain their existing explicit boundaries. [Lower wrapper](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/001e2020.asm), [raw traversal](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/001e20c0.asm), [frozen descriptor/input contract](MP_PICKUP_SPATIAL_RESEARCH.md#raw-terrain-inputoutput-contract).

## Retreat ownership and per-iteration contract

On a hit with mask bit`20000000`, source first forms point=`origin+delta*fraction`. Ordinary terrain validates the point against the recorded final leaf; a disagreement reaches `16B96B`. Original `16B96B–16B9A7` adds oriented plane normal×**`39800000`**, epsilon **0.000244140625**, then `16B9AC→11D4A0` relocalizes into result`+1C`. All24 selected probes return leafNONE after this normal offset. That real failed classification reaches the retreat setup at`16B9D4`; no failure predicate is supplied. [Correction/setup instructions](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/0016b240.asm), [per-query `retreat_trace`](../analysis/multiplayer/integration/inventory-source/pickup-retreat/report.json).

Source computes the segment-normal dot in separately rounded float32 order **z product + y product + x product** at`16B9DB–16B9FC`. In all24 selected cases, the dot is nonzero. Original x87 `FABS/FDIVR/FSTP` at`16BA0F–16BA1B` produces the float32 step **epsilon/abs(dot)**; `16BA1F` loads it into XMM4. The alternate zero-dot branch loads`3D000000`(0.03125) at`16BA27`. That fallback remains unexecuted in this suite. [Setup instructions and constants](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/0016b240.asm), [independent dot/step audit](../tools/oracle_mp_pickup_retreat_audit_1_1.py).

Each iteration preserves caller/mask/contact ownership and:

1. `16BA30–16BA48` subtracts the constant step from the current fraction, clamps nonpositive results to zero, and stores result`+4`.
2. `16BA4D–16BA7B` recomputes each point component from the original origin/delta and new fraction. This replaces the prior normal-offset point; it does not accumulate a positional nudge.
3. `16BA80→11D4A0` writes the recomputed location at result`+1C`.
4. `16BA85–16BA89` exits when fraction is zero, even if the location remainsNONE. For a positive fraction, `16BA8B–16BA8E` repeats if the terrain leaf isNONE. An ordinary-terrain valid leaf exits at`16BA90–16BA96` because the additional collision-BSP pointer at stack`+14` is zero.

The report snapshots before subtraction (`16BA30`), after fraction store (`16BA4D`), after point formation/before relocalization (`16BA80`), after relocalization (`16BA85`), each relevant exit/loop branch and return (`16BAA6`). Every snapshot includes caller mask, initial/final location8, point bits, fraction bits, plane, registers and XMM step. Snapshot values before their owning store remain explicitly pre-store values. [Full trace](../analysis/multiplayer/integration/inventory-source/pickup-retreat/report.json), [original routine](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/0016b240.asm).

## Executed outcomes

All twelve vertex→capsule cases execute one iteration and clamp to fraction0. Eleven relocalize to a valid original vertex leaf. Vertex188 starts outside a valid leaf and remainsNONE after retreat; source nevertheless returns hitAL1 with fraction0. A hit return is therefore not proof of a valid final location. [Cases and original endpoint locations](../analysis/multiplayer/integration/inventory-source/pickup-retreat/report.json).

The twelve authored-edge cases all exit with positive fraction and a valid terrain leaf:

| Edge | Direction | Iterations | Final fraction bits | Final leaf / cluster |
|---|---:|---:|---|---|
|399|0|1454|`3F6399EC`|273 / 6|
|406|0|16|`3F7D7FF0`|2181 / 10|
|887|0|7|`3F7A9F36`|362 / 6|
|1101|1|549|`3F53E2E2`|1479 / 9|
|1198|0|76|`3F767FF0`|2225 / 14|
|1234|1|1|`3F7FF824`|2222 / 14|
|1391|0|1|`3F7FFEAB`|1296 / 9|
|1412|0|1|`3F7FFB00`|160 / 2|
|1416|1|1|`3F7FF621`|1297 / 9|
|1421|1|1|`3F7FFD57`|1912 / 9|
|1433|0|1|`3F7FFDDC`|1507 / 9|
|1481|1|2|`3F7FF6C6`|807 / 9|

Direction0 is edge vertex0→vertex1; direction1 reverses that exact pair. In particular, edge399 does not stop after a guessed fixed iteration count: original repeated relocalization fails until the 1,454th iteration. The bounded fixture instruction budget is an oracle execution bound, not a recovered gameplay iteration cap. [Iteration evidence](../analysis/multiplayer/integration/inventory-source/pickup-retreat/report.json).

Edge1481's actual surface has BSP material indexNONE(`FFFF`). Source `16B317–16B336` skips the collision-material array and uses default material at`484DE0`, producing global materialNONE. The new tool explicitly omits a material-array record for that absent index. This does not alter the frozen outer-query tool/report. [Original material branch](../analysis/multiplayer/integration/inventory-source/pickup-retreat/instructions/0016b240.asm), [cache/result audit](../tools/oracle_mp_pickup_retreat_audit_1_1.py).

## Audit, reproduction and remaining scope

The independent audit verifies every instruction listing against untouched XBE bytes, every captured cache/resource record, exact source endpoint subtraction, normal offset, ordered dot, step, every fraction subtraction/clamp and every recomputed point. Its separate packed24-bit 3D BSP classifier evaluates original float32 plane arithmetic from untouched cache data and checks every captured `14B9D0` return plus every `11D4A0` location, including all failed/intermediate retreat locations. It also checks final contact/material ownership, unchanged RNG`12345678`, and all nine earlier frozen report hashes. [Audit](../analysis/multiplayer/integration/inventory-source/pickup-retreat/audit.json), [byte-identical reexecution](../analysis/multiplayer/integration/inventory-source/pickup-retreat/reproducibility.json).

This closes the ordinary-terrain nonzero-dot retreat loop's numerical/location execution gap. The verified native scope can preserve source fraction/point/location transitions and the zero-fraction exit's possiblyNONE location. It does not establish that these explicit vertex/edge probes are normal live pickup side clips. The zero-dot fallback and unordered-input behavior, additional instanced/object collision-BSP relocalization path, active object-ray broadphase winners, world-state producers, complete Havok motion/settling and dropped-item trajectories remain unresolved. No world/Havok service was needed or supplied for the selected ordinary-terrain probes. Animated capsule production retains the [exact Xbox reciprocal boundary](SPARTAN_LIVE_POSE_1_1_RESEARCH.md#reciprocal-instruction-emulator-boundary). Production and every earlier frozen artifact remain untouched.

Run the independent suite and audit:

```sh
.tools/python/bin/python tools/oracle_mp_pickup_retreat_1_1.py
.tools/python/bin/python tools/oracle_mp_pickup_retreat_audit_1_1.py
```

Frozen source hash`f13bd25eca32aea56fbe0955222ffc8636989f25f1376c8cb1b0c0e1a0ecf0b5`; report/reexecution hash`3b19c3140ea0681a9f1be85580c41d6dd0e28c49583977eb2f58aa6001300713`. The separate audit/reproduction records own all dependency, document and listing hashes.
