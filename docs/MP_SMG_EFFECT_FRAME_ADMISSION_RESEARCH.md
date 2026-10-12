# Original frame-state writers and natural effect-frame startup boundary

The real writers of TU1.1 global **4F4864** are identified in unchanged original instructions. A separate natural **12CBC0** execution now completes six actual platform permission operations, then reaches the original **ROCKALL_FRONT_END heap-constructor failure** before any of those writers or the gameplay elapsed/particle-aging owners. This is a startup boundary; **admitted active multiplayer effect aging remains untested**. No frame state, readiness, AL result, game tick, elapsed amount, pose, contact, effect or particle field is assigned. [Execution tool](../tools/oracle_smg_effect_frame_admission_1_1.py), [measured report](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/report.json), [preceding immutable clock freeze](MP_SMG_EFFECT_TICK_CLOCK_RESEARCH.md).

## Literal state ownership and loader admission

Global4F4864 lies in the `.data` virtual tail: sectionVA468620, virtual size1061656, raw size126956, offset574020 (`8C244`). Its four bytes are beyond the file-backed section data. The XBE reader supplies zero for this region and the prepared mapped backing measures zero. Neither fact proves execution of an original stored initializer, whole-title startup or an Xbox loader. The new static report records this distinction and checks every listed instruction/table against the pinned original executable. [Static exporter](../tools/research_smg_frame_admission_1_1.py), [section scope and complete owner listings](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/report.json).

| Original instruction | Owner | Exact store | Coverage here |
|---|---|---|---|
| `190624` | `1905B0` cleanup | `4F4864 = 0` | Listed only |
| `190652` | `190650` | `4F4864 = 1` | Listed only |
| `190714` | `190690` | `4F4864 = 2` | Listed only |
| `1907C8` | `190730` cleanup | `4F4864 = EBX`, sourceEBX0 | Listed only |
| `190870` | `1907E0` | `4F4864 = 3` | Listed only |
| `1908C4` | `190890` | `4F4864 = 5` | Listed only |
| `190B6C` | `190B30` temporary loading state | `4F4864 = 5` | Listed only |
| `190B8A` | `190B30` after loading calls | `4F4864 = 0` | Listed only |

All eight are literal absolute stores, independently byte-audited. None executes in the measured frame extension. Original190624 is retained even though the corresponding generated C omitted that explicit store. No semantic enumeration is assigned to these numeric states. [Exact instruction references](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/report.json), [independent byte/coverage audit](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/audit.json).

Original12CBC0 reads4F4864 at **12CBDA**. States2 and5 skip its automatic menu request. Otherwise the clear4F496D path calls190BF0 with0. The existing source options constructor then owns the request and the real platform tick reads; the resulting pending request enters **190240→1902E0**. Original1902E0 calls1905B0, clears its six request flags, then calls **190930** before **1904D0** or **190A30**. The new dynamic trace stops inside190930's allocation callback, preserving that order. [Outer owner](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/0012cbc0.asm), [startup order](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/001902e0.asm), [actual frame entries and pending calls](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/report.json).

The shortest listed positive loader chain is **1904D0→190650→124040→190690**. Original19E070 returns the cache status consumed by table190598: statuses0/1/2 go to1904EB and190B30; status3 goes to1904F5; status4 goes to19054D. Before190650, **190507 executes `xor al,al`**; the caller therefore owns AL0. It must not be replaced by a supplied mode or success return. Original190510 calls124040. Only its trueAL result reaches11DE10, then original19051F again produces AL0 before190521 calls190690. That routine executes its cache/resource/reset branch before storing2 at190714 and copying the path. Original loader failure instead takes190537/190539 into190730. These are complete original listings, not positive execution in this suite. [Loader and literal table](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/001904d0.asm), [state1 owner](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/00190650.asm), [state2 owner](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/00190690.asm), [failure cleanup](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/00190730.asm).

## Independent multiplayer readiness and elapsed ownership

Original **190A30 does not write4F4864**. It calls whole139410, D6B60, C1870,1395B0 and11D780; only the trueAL result of11D780 reaches the byte store **Game+1120 = 1 at190A6E**. Its false branch calls139470 and returns false. The existing selected local-map initializer freezes recover substantial prefixes of this chain but stop before that whole readiness owner returns. Latest local-map-clock evidence stops at actual ordinal128 in callback23. The earlier grenade preparation executes the actual12D840 texture constructor, but12D950 stops at its separate `(80061000,02CE6000,404)` protection operation. None of these closes the frame-state writer or admitted full gameplay frame. [Readiness owner](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/00190a30.asm), [local-map clock freeze](MP_LOCAL_MAP_CLOCK_RESEARCH.md), [local-map protection freeze](MP_LOCAL_MAP_PROTECTION_RESEARCH.md), [grenade texture boundary](MP_GRENADE_ACTION_SOURCE_RESEARCH.md), [earlier static startup note](SMG_AUDIO_STARTUP_1_1_RESEARCH.md).

After its current-state update, original12CBC0 calls **68460 at12CD71**. That complete predicate requires:

1. Byte4D6D38 is nonzero.
2. Global4EDF18 points to a game record.
3. Game+1120 is nonzero.
4. The signed shortGame+2 differs fromNONE (`FFFF`).
5. Byte4D6D3A is zero.
6. The record at global4D6D44 has DWORD+18 equal4.

Only its trueAL enters **147DD0 at12CD8A**, followed by188AD0,1396E0 and **139640 at12CDB1**. The latter consumes the float output of147DD0; the integer step output goes separately to1396E0. Thus changing4F4864 alone would not establish gameplay admission or the correct elapsed amount. The measured simulation still has the existing source kind1/state2 boundary, whose positive transition/membership/input chain remains open. [Exact gameplay predicate](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/00068460.asm), [caller and output aliasing](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/0012cbc0.asm), [elapsed owner](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/instructions/00147dd0.asm), [existing command admission limits](MP_LOCAL_COMMAND_SCHEDULING_RESEARCH.md).

## New unchanged execution and actual platform operations

This new observer extends the immutable system-time/tick-clock preparation. It preserves the captured native startup/query prefix and all earlier source objects, pools, marker/effect/particle state. The actual host clock producer supplies **92 original DWORD reads/returns** through the already verified417CD4→0B000000 backing relocation. No dt or tick increment is chosen. Complete original337A40/337A45/337A47 instructions still perform each read and return. [Raw clock inputs, deliveries and returns](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/report.json), [captured deterministic native inputs](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/native-clock-inputs.json), [clock ABI/startup provenance](MP_SMG_EFFECT_TICK_CLOCK_RESEARCH.md).

The new path reuses the **immutable, compiled primary compatibility permission bodies** from the local-map-protection freeze. Every original requested protection4 converts to host0x40; the callback actually applies Unicorn access7 to the requested range. Each operation preserves all raw memory bytes and general registers, returns through the original stdcall stack boundary and records its conversion, request, mapped regions and before/after CPU. This closes the actual compatibility access request, not Xbox PTE/cache/MM or hardware parity. No cache/game/admission success is supplied. [Permission provider provenance](MP_LOCAL_MAP_PROTECTION_RESEARCH.md), [actual operations and whole frame buffers](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/report.json).

| Original return | Original base | Original size | Original protection |
|---|---|---|---|
| `82227` | `82D46000` | `1000` | `4` |
| `822F4` | `82D3D000` | `9000` | `4` |
| `823E2` | `82D17000` | `26000` | `4` |
| `824E8` | `82D16000` | `1000` | `4` |
| `8260B` | `82D0A000` | `C000` | `4` |
| `8270E` | `82C67000` | `A3000` | `4` |

At original caller **82828**, actual2D7A1A arguments are `(82C67000,0,4)`. Its unchanged zero-size branch returns directly with `ret 0xC`; no kernel permission operation or adapter is invoked for this call. Original82190 therefore computes and retains its own zero-size allocation outcome. The report stores literal original inputs and stores rather than adjusting capacity, physical bounds or arena cursors. [Complete allocator](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/instructions/00082190.asm), [zero-size thunk](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/instructions/002d7a1a.asm), [actual request/return ledger](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/report.json).

Original3302B0 at caller828B4 enters with ECX/ESI82C67000 and stack prefix`{1,0,82D81E24,0}`; its actual return is EAX82C67000. Original330DE0 at caller82939 enters with ECX08000860, ESI82C67000 and literal stack words:

```
475978 475B78 800 0 400 100000 475C88 82C67000 1 0 4 400
```

The report records full CPU, stack, operands and owning stores for both. The latter never returns. Its unchanged chain **330DE0→3313F0→3313B0→327F10→338289** formats the actual message `*** Software Failure: Heap constructor failed in ROCKALL_FRONT_END ***` with newline. Original338297 consumes its72 bytes includingNUL. At the diagnostic, actualANSI descriptor021FCD20 has length71, maximum72 and buffer021FCD34. Original **3382B4 (`CD2D`) executes INT2D**, producing `UC_ERR_EXCEPTION`21; stoppedEIP3382B6 is the following unexecutedINT3. The audit reconstructs that message solely from the actual per-character source reads and validates its raw stack descriptor. No diagnostic bypass, constructor return, expanded heap or presumed failure cause is supplied. [Heap/diagnostic calls and stop](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/report.json), [original constructor bytes](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/instructions/00330de0.asm), [original diagnostic bytes](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/instructions/00338289.asm), [diagnostic audit](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/audit.json).

Only frame entries12CBC0 and1902E0 are observed. Current global4F4864 remains0 and Game+1120 remains0; source4F496D becomes1 and the separate source4F4860 memory-class field becomes1. All represented current effects, particles, player, unit/items, FP channels, game clock, RNG and PVS bytes remain identical. No1904D0,190650,190690,147DD0,139640,1789D0,176410,1764B0,24A820 or249210 is entered. The two retained particles remain age0 because the elapsed/aging path has not been admitted, rather than because that path was tested and failed. [Literal frame snapshots and original store ledger](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/report.json), [complete source-address coverage](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/instructions/all-executed.asm).

## Independent freeze and next producer

The new report contains290 calls,2 deduplicated current states,3880 reads,6969 original stores and86 instruction files. Its executed listing contains2385 original addresses; complete listings include674 original store sites. The static companion separately retains19 original routine listings,11 global references and both dispatch tables. Every instruction byte and table byte is checked against TU1.1 SHA256`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. All191 prior pins remain intact. [Static byte report](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static/report.json), [executed byte listing](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/instructions/all-executed.asm), [store listing](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/instructions/all-stores.asm).

Independent original execution repeats **89 deterministic files** byte-for-byte: report, all86 instruction files, native clock inputs and empty stderr. Native private host records are retained separately and are not deterministic address expectations. The audit replays **76 whole represented buffer intervals** from actual original stores and verifies the six real protection operations, original zero-size return,92 tick deliveries/returns, actual diagnostic, unchanged current state and absence of positive frame writers/elapsed owners. Two earlier observer-only failures are retained under`probe-before-invalid-observer/` and`probe-before-cpu-exception-observer/`; the final observer records the genuine CPU exception instead of assuming every stop is a memory fault. Neither correction changes a guest input or original result. [Repeat](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/independent-repeat/report.json), [audit tool](../tools/audit_smg_effect_frame_admission.py), [audit](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/audit.json), [immutable manifest](../analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/manifest.json).

```bash
.tools/python/bin/python tools/oracle_smg_effect_frame_admission_1_1.py \
  --clock-replay analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/measured/native-clock-inputs.json \
  --output analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/independent-repeat/report.json
.tools/python/bin/python tools/audit_smg_effect_frame_admission.py
```

The next executable gate is the original front-end heap constructor and its owning physical capacity/startup context, before1904D0 can establish the loader state. The zero-size record is a measured outcome, not a license to enlarge storage or force heap success. The shortest later admission still requires the actual loader/resource owners, full readiness callback and simulation state4. These remain explicit. This freeze does not claim continuous particle settling/aging/expiry/free, a rendered muzzle flash, successful sound, full title execution, actualTU1.5 execution or Xbox hardware parity. No production file or previous freeze is edited.
