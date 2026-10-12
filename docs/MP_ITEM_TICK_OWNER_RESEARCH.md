# Original item tick/clock ownership and exact scheduler stops

Research date: 2026-10-08. The unchanged original clock constructor/reset and whole **139750** execute after the same source-owned three-item preparation. The existing options delivery has zero tick rate: the original reset produces **+infinity**, which is retained. The first whole tick stops on missing watcher storage; constructing that storage with original **68040** permits a complete **inactive** tick. No item phase repeats and the game tick never advances. Subsequent original simulation new-map initialization stops before an unresolved kernel data import is consumed. Continuous settling remains unproved. Production and all preceding freezes are untouched.
[Oracle](../tools/oracle_mp_item_tick_owner_1_1.py), [independent audit](../tools/oracle_mp_item_tick_owner_audit_1_1.py), [measured report](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json), [audit report](../analysis/multiplayer/integration/inventory-source/item-tick-owner/audit.json), [preceding immutable contact/local-node freeze](MP_ITEM_CONTACT_PRODUCERS_RESEARCH.md).

## Authority and unchanged preceding state

TU1.1 XBE SHA-256 is `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`; Lockout SHA-256 is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. Each case independently repeats the complete earlier selected player/item preparation before the new roots. All active, motion, transform and contact runtime sections compare byte-for-byte with their own immutable reports before any extension executes. All **191** earlier pins are checked before and after execution. The same real owned compatibility worker/storage stays alive through the original execution; this is not Xbox hardware execution.
[Exact preparation, all four runtime correlations and source pins](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json), [original player preparation](MP_PLAYER_VISIBILITY_RESEARCH.md), [original active item composition](MP_ITEM_ACTIVE_SCHEDULE_RESEARCH.md).

That preparation retains actual player EC700000, unit E26F0000, carried SMG E2700001, authored start 37/cluster 4 and source PVS FFDF0000. Authored world rows 0/1/12 retain BR E2710002, BR E2720003 and SMG E2730004 and their original current roots, node banks, full datum salts and unsorted registry chains. Its earlier selected **13A380 → B7E00 → 1C7660 → B7F50 → B8040** uses its explicitly declared inherited `3D088889` time-step input. This new extension does not rewrite that earlier evidence or retroactively claim its tick rate was source-owned.
[Immutable earlier motion rate boundary](MP_ITEM_MOTION_BOUNDARIES_RESEARCH.md), [allocated/current-state evidence](MP_ITEM_CURRENT_PRODUCERS_RESEARCH.md), [complete extension entry states](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json).

The inherited root-call ABI adapter writes the return-PC/stack argument routing and initializes FPCW027F/MXCSR1F80/FPSW/FPTAG before each selected root. That existing execution boundary remains explicit; it is not a recovered whole-title floating-environment handoff. Within complete 139750, original control87 and its resulting actual023F control word execute unchanged. All eight x87 and XMM values, control/status words and stack states are retained.
[Owning adapter](../tools/oracle_smg_firing_1_1.py), [measured CPU boundaries](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json).

## Exact clock owner and option boundary

The original 36-byte callback record at **44768C**, index 17, contains initializer **147CC0**, disposal **187550**, map reset **147D20**, activation **147D80**, then five zero words. Ghidra's recovered function boundaries omit 147CC0/147D20 as independent routines; the XBE bytes and callback table establish them. Complete title constructor **139390** calls storage callbacks in ascending index order. Whole map reset **139410** similarly visits map-reset targets after **139540** copies supplied options to game+8. Those complete traversals are listed here; they are not newly executed startup claims.
[Literal callback record](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json), [whole constructor listing](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00139390.asm), [whole reset listing](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00139410.asm), [options-copy owner](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00139540.asm).

**147CC0** consumes exactly **24h** bytes at the current source game-arena cursor, updates the cursor/checksum through original **1656A0**, zeros all nine dwords and binds global **518444** at **147D0E**. In both cases the actual record is **82EBA050**. No clock address, allocation success, rate or tick is supplied by this extension. **147D20** clears that whole record again, reads the signed short at **game+0E** at **147D51**, converts it with CVTSI2SS, and executes original **DIVSS at147D5F** using XBE constant **465030** (`3F800000`). It stores rate+2, dt+4, speed+0C and enabled byte+0 in that order.
[Complete constructor](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00147cc0.asm), [complete reset](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00147d20.asm), [actual operands and ordered stores](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json).

| Clock offset | Source owner/meaning | Actual reset bytes |
| --- | --- | --- |
| +0 byte | 147D71 enables the record | `01` |
| +1 byte | Reset clears; pause consumers may subsequently write it | `00` |
| +2 signed short | 147D63 copies signed game+0E | `0000` |
| +4 float32 | 147D67 stores separately executed source division | `0000807F` (+infinity) |
| +8 dword | Reset clears; 139868 owns admitted game-tick increment | `00000000` |
| +0C float32 | 147D6C copies 465030 | `0000803F` |
| +10..+20 | Whole record is cleared; scheduler/speed interpolation owns later stores | Five zero dwords |

The zero at game+0E is the actual retained options-input value, **not an asserted valid live match tick rate**. Original 139540 copies it from options+6; the session/options producer that supplies a valid field remains open. No 30Hz value, finite dt or corrected options field is substituted. The new clock record is never passed into an admitted item-motion phase in these cases. Camera-time global **51845C** is a distinct record, owned by **156A30/156AD0**; it does not initialize 518444.
[Literal option/clock buffers](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json), [separate camera storage execution](MP_LOCAL_COMMAND_SCHEDULING_RESEARCH.md).

## Whole tick ABI, admission and exact stage order

**139750 has no externally supplied active argument in these executions.** Original **326AF0** allocates its 4048-byte caller frame; the measured helper returns with ESP delta **-4044h**, which the observer retains. **139764** executes original **327866**, with actual stack pushes `FFFFF` and `9001F`. Complete **684F0 → 694A0** produces the frame, then **685B0** consumes its pre-player deliveries. **13977D** reads byte **frame+4**, produced by original **68460**, and **139783** selects the active branch. The decompiler's apparent `char param_1` must not be implemented as a supplied active flag.
[Whole tick listing](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00139750.asm), [actual stack allocator](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00326af0.asm), [original control-word call](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00327866.asm), [frame producer](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/000684f0.asm), [admission producer](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00068460.asm).

The listed active path orders player **14EAE0**, intermediate subsystems **13A520/1CBDB0/1FEED0/20F030**, netgame **15D7C0**, **1978E0/20CCD0**, PVS **13A380**, then ordinary **B7E00**, physics **1C7660**, dirty **B7F50** and new objects **B8040**. All remaining subsystems follow before camera countdown/time writes **139838/13984B**. Complete **686F0** then processes the frame. Only after that return does **139868** increment `[518444]+8` when the source frame+4 flag is nonzero. None of these active-branch calls, camera stores or tick increments is executed by this extension.
[Complete original owner and branch coverage](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00139750.asm), [measured executed listing](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/executed-tick-owner-instructions.asm).

The outer title path **12CBC0 → 68300 → 68460 → 147DD0 → 188AD0 → 1396E0** is statically listed. 147DD0 produces a tick count from real frame time, simulation/readiness and its clock record; 1396E0 loops with that produced count and cancellation **12D600**. The network/catch-up caller **69B20** separately gates on simulation/source packet state, ready game state and **69540** before calling 139750. No frame duration, count, caller admission, command or local-device/session state is supplied to make either caller run. The direct whole-tick root is a bounded ABI execution, not proof of whole-title scheduling.
[Whole ordinary loop](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/0012cbc0.asm), [count producer](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00147dd0.asm), [bounded loop owner](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/001396e0.asm), [network caller](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00069b20.asm), [separate device/readiness boundary](MP_LOCAL_COMMAND_SCHEDULING_RESEARCH.md).

## Executed cases and earliest unavailable dependency

Both fresh cases execute the identical preceding composition and original **147CC0 →147D20**. The first then calls complete **139750**. It executes original control87 and frame clearing, reaches **694A0 →68560**, and stops at **68570**, reading byte NULL+84 because **[4D6D48]=0**. The actual in-progress frame, registers, failed access, partial stores and source states remain in the report. No failure/success result is supplied and no tick is retried after this stopped frame.
[Missing-watcher case](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json), [exact stop instruction](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00068560.asm).

The second fresh case executes original storage callback 23, **68040**, before **68300 →139750**. That constructor owns `[4D6D44]=54C670`, `[4D6D48]=54D894`, `[4D6D4C]=54E4C8`, its two source handle tables and enable byte `[4D6D38]=1`. It does not execute the simulation map reset before this tick: actual source BSS kind/state remain **0/0**, and actual **game+1120=0**. Complete **68300** returns at the readiness gate. Complete **139750** now returns with **frame+4=0**, command mask zero and game tick zero. Original **686F0** advances **simulation+28 from0 to1**, which is a frame sequence rather than the game clock. Original control87 leaves actual FPCW **023F**. No ordinary/physics/dirty/new-object phase is called.
[Source constructor](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00068040.asm), [complete readiness gate](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00068300.asm), [full frame/sequence/CPU outputs](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json).

Only then the second case executes complete **68170** empty disposal and attempts original **680F0** new-map initialization. **68E10** sets simulation kind **1** from the retained actual game mode, clears its early fields and enters original **337A40**. At **337A45**, EAX is the literal encoded import target **8000009C** from slot **417CD4**. That is unresolved kernel data ordinal **156 (KeTickCount)**. In this fixture its address happens to overlap genuine Lockout bytes **FDFFBF2F**, file offset **1BD389C**, cache magic **7E42C800**. The observer stops **before** `mov eax,[eax]` consumes those unrelated cache bytes. No kernel clock, zero result, handle, backing pointer or source return is substituted. Original return PC is **68E87**. This is an explicit unavailable-data boundary, not an unmapped access, and the stopped instruction is absent from executed-address coverage.
[Owning partial initialization and exact pre-consumption stop](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json), [original imported-data reader](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/00337a40.asm), [original new-map caller](../analysis/multiplayer/integration/inventory-source/item-tick-owner/instructions/000680f0.asm), [kernel ordinal identity](../tools/reference/halo2-native-reference/scripts/vendor/xbox_kernel_exports.py).

Partial source stores are retained: kind is **1**, state remains **0**, game tick remains **0**. No additional tick runs after the stop. Later **6B650** state-two initialization, second boot-clock read, **197450 →32803E →336983** system-time query and remaining map-reset work are source-listed only here. Preserved exploration drafts that allowed an encoded import to alias cache bytes are not fixture authority; the canonical report never consumes that value. The spawn agent's separate recorded real-host clock providers are an existing distinct explicit platform scope and are not added to this extension.
[Canonical measured coverage](../analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json), [separate measured-host clock evidence](MP_COMMAND_INPUT_RESEARCH.md).

## Stored-state contract, repeat and remaining gate

The report retains complete item/current-node/header/bitmap/registry buffers, exact world/player/PVS/queue records, both full clock/context records, game1200, simulation1224, watcherC34, arena/checksum owner words, literal imports and callback17. Each owning root has before/after full state IDs and CPU, exact source calls/arguments, all observed operand reads, ordered stores and actual unfinished call frames. Complete 684F0 input/output4048 and its store stream prove the inactive frame. Full-buffer replay and original-byte auditing use original execution outputs only, with no native-derived expectations.
[Report and compact schema](../analysis/multiplayer/integration/inventory-source/item-tick-owner/schema.json), [source-output audit](../analysis/multiplayer/integration/inventory-source/item-tick-owner/audit.json).

The shortest still-open chain is: valid options+6/session delivery → correct source callback lifetime → a genuine kernel clock input for simulation new-map initialization → complete readiness/session scheduling → source-produced caller tick count and frame+4 admission → complete 139750 subsystems → repeated original four object phases. Existing source contacts/local nodes remain exact earlier single-cycle evidence. This freeze supplies no positions, impulses, contacts, local/current poses, PVS membership, queue state, Havok result, manually advanced tick or active flag. Positive dynamic Havok contact, continuous settling, full frame/title startup and Xbox reciprocal/hardware parity remain unresolved.

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_item_tick_owner_1_1.py \
  --output analysis/multiplayer/integration/inventory-source/item-tick-owner/reproduction/report.json
.tools/python/bin/python tools/oracle_mp_item_tick_owner_audit_1_1.py \
  --output-dir analysis/multiplayer/integration/inventory-source/item-tick-owner/reproduction
```

Final pins and independent byte-identity verification are saved with the separate manifest. No earlier oracle, report, document or production file is changed.
[Frozen file manifest](../analysis/multiplayer/integration/inventory-source/item-tick-owner/manifest.json).

The canonical suite has **2 cases, 48 calls, 10 full states, 362 consumed reads and 10,326 ordered stores**. Its **57 instruction files /4,006 rows** repeat byte-identically in a fresh process, as do the complete report and audit. The independent auditor replays **571 complete stable-buffer intervals /278 overlapping read/store observations**, plus the complete source4048-byte frame store stream. All **191** prior pins and all four earlier runtime sections remain exact in each case. **Game-clock advances=0; admitted item phases=0.**
[Audit and repeat](../analysis/multiplayer/integration/inventory-source/item-tick-owner/audit.json), [independent verification](../analysis/multiplayer/integration/inventory-source/item-tick-owner/independent-freeze-verification.json).

| Frozen artifact | SHA-256 |
| --- | --- |
| Oracle | `ce3b32bb2ac32d4030215d2ec94dd8ebb587d402c8b4fe9d39a5b6cc0d6ae750` |
| Auditor | `5b3a572940587ff17e30936569820249e7c3f9e0601e201eb4cfbf989a017233` |
| Report and independent report | `f5cc1f4fd58ca9d80c800bd6178dc980931392bf832288e2ac21b50051995982` |
| Audit and independent audit | `df2406ae4032ada7ef41bfb22dc117c88a81618ff7ca8650f26cb437b04b4796` |
| Compact schema | `6961721ab933297d90911b897ec5442e104843cc3086c150a627f7b646cd6609` |
