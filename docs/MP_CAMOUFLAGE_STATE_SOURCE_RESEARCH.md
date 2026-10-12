# Camouflage state, lifetime, ordering and disruption

Research snapshot: 2026-10-11. The original Xbox owners establish zero cold unit camouflage state, allocation-owned zero player timers, timer retention on leave, refresh before King occupancy, unit fade before post-world timer expiry, and tag-authored disruption amounts consumed by compiled arithmetic. These are bounded source contracts for native binding. King normal startup, transport and the full all39-variant goal remain open.

The new [state-owner generator](../tools/oracle_camouflage_state.py) saves [complete caller listings and executed lifecycle evidence](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json). The [preserved-FP generator](../tools/oracle_camouflage_fp.py), [disruption generator](../tools/oracle_camouflage_disruption.py) and [declared timeline generator](../tools/oracle_camouflage_timeline.py) execute unchanged whole owners against declared backed records. None imports native implementations. The original execution boundaries below are material to the conclusions.

## Primary inputs

| Input | SHA256 |
| --- | --- |
| [Retail XBE](../assets/disc/default.xbe) | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| [TU1.1 XBE](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03) | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| [Owned imported TU1.5 XBE](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>) | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| [Lockout cache](../assets/disc/maps/lockout.map) | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |

All addresses below are TU1.1 unless explicitly labeled. The whole refresh/grant/timer/fade/activation/deactivation instruction bodies agree between retail, TU1.1 and TU1.5 after verified relocation; the corrected corpus records twelve complete instruction-shape comparisons. All three disruption bodies and the intrinsic delegate also agree after relocation, with four complete comparisons. Full bodies are retained in their reports rather than inferred from decompiler signatures. [FP source listings](../analysis/multiplayer/game-variants/camouflage-state-20261011/report.json), [disruption source listings](../analysis/multiplayer/game-variants/camouflage-state-20261011/disruption/report.json).

## Cold and player lifetime ownership

The actual original normal player creation reached whole unit constructor `C52C0` from `10CC76`. Pinned prior call23 returned AL1, grew the actual body from5302 to5750 bytes, and retained zero in every camouflage field before and after:

| Unit offset | Cold value | Consumer |
| --- | --- | --- |
| +134 | DWORD0, including bits3 and4 clear | Shared unit flags |
| +2B0 | binary32 positive zero | Primary amount |
| +2B4 | binary32 positive zero | Secondary amount |
| +2B8 | binary32 positive zero | Primary fade speed |
| +2BC | byte0 | Intrinsic suit mode |
| +2BE | signed16 zero | Intrinsic suit timer |

The [pinned whole-constructor report](../analysis/multiplayer/integration/spawn-source/unit-constructors/measured-final-stop/report.json), SHA `8e03219bae06eaf2968d2dfb24142a9446cee1d9ff36da9b246b229c821d3c0e`, is re-read evidence, not a fresh execution of complete creation. It preserves its compatibility, storage, game-options and resource-service boundaries. The [new owner report](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json) extracts the exact before/after fields and inherited boundaries. A fresh separate TU primitive executes whole `BD170` over poisoned storage supplied by the explicit `18FCE0` arena service; the original allocator/header/clear owners produce an all-zero800-byte body. The following bounded `C52C0` prefix ends at `C5464` exclusive and leaves the fields above zero. **That prefix alone does not initialize the fade fields**; allocation clearing and the independently captured whole constructor are the authorities. [Constructor research](MP_UNIT_CONSTRUCTORS_NATIVE_RESEARCH.md), [constructor-tail boundaries](MP_UNIT_CONSTRUCTOR_TAIL_RESEARCH.md), [datum ownership](MP_DATUM_OWNERSHIP_RESEARCH.md).

Player+198 and+19A are exactly two signed16 timers. Whole birth preserves poisoned `A5A5A5A5` in those bytes in all three images: birth itself does not establish their zero default. Actual requested allocation clears the new row, so allocation followed by whole birth leaves both zero. Whole spawn `1509C0` writes both zero; the new whole no-allocation mode4 branch demonstrates this for positive and negative seeded values without supplying spawn success. Mode4 is not ordinary live creation. The earlier source-owned ordinary spawn remains separately bounded prior evidence. [Player-input source research](MP_DAMAGE_PLAYER_INPUT_SOURCE_RESEARCH.md), [all-image lifecycle and full birth/spawn instructions](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json).

Whole leave `14DCB0`, including its reached `14E240` binding owner, preserves both timers. All-image nonlocal/current-unit-NONE cases retain `(15,2)`, `(-1,32767)` and `(-32768,0)` byte-for-byte. Three stronger TU cases at slots0/3/15 execute actual object callbacks on leave and retain `(15,2)` while detaching the current unit; same-slot allocation/rejoin then returns both to zero. These object cases inherit the declared datum oracle's storage arena, object/type records and constructor/death prefixes; no supplied return executes inside whole leave. Salt/occupancy release and retained row bytes remain distinct. [New whole leave evidence](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json), [existing stale-slot authority](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/report.json).

Selected-engine initializer slot5/+14 is mode-specific. The registered Slayer callbacks execute seven instructions and write **DWORD FFFFFFFF** to player+1B8. The registered King callback is exactly `RET4`, preserving either fresh0 or the declared poison word. Twelve whole calls test fresh/poisoned state in all three images. This explains why a freshly allocated King player has inside timer0 while Slayer's initializer leaves-1.

| Image | Slayer slot5 | King slot5 |
| --- | --- | --- |
| Retail | `2BBF70` | `72C70` |
| TU1.1 | `2C22C0` | `27E760` |
| TU1.5 | `2C5D40` | `2E4D60` |

The generator resolves the callbacks through the actual registered vtables and passes the actual stack player argument. [Slot5 instruction/execution evidence](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json).

## Refresh, grant, expiry and fades

The [King/global trait contract](MP_KING_TRAIT_CONSUMERS_SOURCE_RESEARCH.md) owns selector1: a positive player+1B8 chooses hill flags variant+F0 bit4, including a false overriding a true global option; otherwise global variant+48 bit2 applies. Shared `15C8B0` gets player in ESI, queries selector1, and returns without writes when false, engine absent, or current unit NONE. When true it multiplies signed16 clock rate by compiled0.5, stores binary32, and converts through actual x87 `FISTP` before granting timer index0. [Original refresh](../analysis/map-pack-update/functions/0015c8b0.c), [corrected whole calls](../tests/fixtures/camouflage_state_original.json).

`1537F0` receives player in EAX, signed16 index in DX and signed16 ticks in DI. It accesses player+198+2*index and stores the signed16 maximum of old and requested ticks. If old is exactly0 it first calls `153990`; a negative nonzero timer does not activate. Index0 activation ORs unit+134 bit3 and sets +2B8 to compiled0.25. It leaves primary/secondary amounts and bit4 unchanged. Index1 activation returns without changing unit camouflage. [Original grant](../analysis/map-pack-update/functions/001537f0.c), [activation](../analysis/map-pack-update/functions/00153990.c).

`153920` receives player in EAX and processes exactly two timers. A positive timer decrements by1; nonpositive values remain unchanged. Index0 reaching0 calls `D22B0` with the current unit and XMM0=1.0. That owner clears bits3 and4 and writes supplied XMM0 times0.25 as primary speed. It does not zero either amount. Index1 expiry skips that call. Activation and deactivation use original dirty owner `B6760(entity,800000)` only when unit+0D4 is not NONE. The new execution has actual entity NONE and does not replace this service. [Timer](../analysis/map-pack-update/functions/00153920.c), [deactivation](../analysis/map-pack-update/functions/000d22b0.c).

Whole `C7B70` gets the unit in EAX. In the measured multiplayer actor-NONE branch, bit3 selects primary addition toward1 versus subtraction toward0 using +2B8 times binary32 clock dt. Bit4 independently selects secondary addition/subtraction using compiled `0x3EAAAAAB` (0.3333333432674408) times dt. Only the endpoint in the chosen direction is clamped; the owner does not unconditionally sanitize an initially out-of-range amount. The full owner includes campaign/actor overrides, outside this branch corpus. [Unit fade](../analysis/map-pack-update/functions/000c7b70.c), [complete executed instructions and raw outputs](../analysis/multiplayer/game-variants/camouflage-state-20261011/report.json).

## Floating-point correction and source frame control

The old trait harness requested four control words during setup, but inherited `Source.call` then reset FPCW to027F and MXCSR to1F80. The old fixture is immutable and proves only that effective control state; its labels do not establish alternate rounding. The corrected [FP oracle](../tools/oracle_camouflage_fp.py) preserves controls, observes actual FPCW at every reached `FISTP`, captures the stored binary32 input, and observes the produced signed integer immediately after the instruction. At31Hz the input15.5 produces16/15/16/15 for actual027F/067F/0A7F/0E7F. All864 reached conversion observations record the requested controls. [Corrected fixture](../tests/fixtures/camouflage_state_original.json), [source/execution report](../analysis/multiplayer/game-variants/camouflage-state-20261011/report.json), [independent byte-identical reproduction](../analysis/multiplayer/game-variants/camouflage-state-20261011/reproduction.json).

The original outer frame supplies a concrete production control owner: TU `139750` calls CRT `327866(9001F,FFFFF)` before its game phases. Executing whole CRT and its actual direct callees from all four initial control words returns API value9001F, guest FPCW **023F**, round-to-nearest, and unchanged MXCSR1F80. Retail frame/CRT are `137FE0/3212D6`; TU1.5 `13AAC0/32B3D6`. These twelve calls execute unchanged instructions without supplied returns. Reserved bit6 differs from the027F consumer fixtures; this note reports the guest result and does not assert hardware-specific reserved-bit readback. Alternate RC fixtures are declared consumer-control coverage, not a claim that normal original frames use all four modes. [Full frame/CRT listings and observed controls](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json).

## Actual caller order and declared timeline

Complete TU outer-frame instructions call `15D7C0` before world/object phases `B7E00`, `B7F50`, `B8040`, then `14EFF0` after them. Within the actual pre-world player loop:

1. `15C8B0` refresh uses retained inside/timer state.
2. `15D160` handles the existing player countdown/input owner.
3. `1635D0` executes.
4. Selected-engine+3C, King slot15, updates current occupancy.
5. `1640D0` executes.

Thus a refresh moved after King occupancy changes the source order. These are complete original caller instructions, not a newly executed full outer frame. [Pre-world source](../analysis/map-pack-update/functions/0015d7c0.c), [outer and complete caller listings](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json).

The inherited original unit update callback `C7E90(unit stack argument,RET4)` executes `C5D10 → C6C50 → C7420 → C7210 → C7AA0 → 116420 → C7CF0 → C7B70 fade → C6400 intrinsic`. Type dispatch `10CDF0` reaches this through the unit descriptor inherited by the biped, with actual call at `10CE3B`; the recursive object owner `BD360` handles the unit before its carried weapon. This is supported by whole callback call365 and children366–374 in the pinned [source-owned unit phase report](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json), SHA `4b9e9ed383e0081fe7d64c57636731304d66d322b3aef23259024a27061733cb`. The new [owner report](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json) extracts that exact ordered window and preserves the inherited clock/compatibility/resource boundaries. It does not claim new successive whole frames. [Unit-phase research](SPARTAN_EQUIPPED_UNIT_PHASE_1_1_RESEARCH.md).

Post-world `14EFF0` calls `153920` at `14F34D` only with a current unit, before pickup broadphase `151F80` at `14F44A`. A standalone `153920` with current-unit NONE and expiring first timer is not an admitted natural caller context. [Post-world source](../analysis/map-pack-update/functions/0014eff0.c), [complete caller instructions](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json).

The new [timeline fixture](../tests/fixtures/camouflage_timeline_original.json) is explicitly a **declared composition of whole owners**: zero cold camouflage/timers, actual backed current unit, actor/entity NONE, registered King with global/hill camouflage false,30Hz and source-frame nearest guest CW023F. It grants index0/ticks15, then calls whole refresh, fade and timer in the above relative order for30 steps. A second case grants index1/ticks2. Each step stores raw states after each owner. All546 principal calls agree across three images. Step15 fades primary to0.125 before expiry clears the flags; at step30 binary32 subtraction leaves a small positive primary `31C00000`, not mathematical exact0. Index1 never activates either amount. [Original-only timeline generator](../tools/oracle_camouflage_timeline.py), [report](../analysis/multiplayer/game-variants/camouflage-state-20261011/timeline/report.json), [byte-identical second run](../analysis/multiplayer/game-variants/camouflage-state-20261011/timeline/reproduction.json).

This timeline provides independent expectations for a native explicit-grant phase test. It does not prove authored pickup, natural elapsed world ticks, genuine movement, physics, rendering, replication or normal King admission.

## Disruption and authored balance inputs

Whole `D2310` receives the unit in EAX and binary32 amount/limit on stack, `RET8`. In the measured intrinsic-mode0 branch it:

1. Replaces limit equal to zero with compiled0.25.
2. Stores the lesser of that limit and current +2B8, preserving literal SSE comparison/operation order.
3. Stores primary minus amount via `SUBSS`.
4. If unit+134 bit3 is set and the result is below compiled0.05 (`0x3D4CCCCD`), stores0.05. Inactive primary may remain negative until the later fade owner clamps it.
5. Preserves all flags, secondary, intrinsic mode and intrinsic timer. Actual entity NONE naturally skips dirty publication.

The floor applies even for amount0 when an active primary is below0.05. Retail/TU1.1/TU1.5 bodies are `D0E60/D2310/D33A0`. The [new disruption fixture](../tests/fixtures/camouflage_disruption_original.json) contains2,304 finite rows per image, **6,912 whole executions**, with all unowned unit/player bytes checked. It spans primary-0.1 through1, speeds0/0.1/0.25/1, both camouflage flag bits independently preserved, amounts0/0.05/0.1/0.2/0.75/1, and limits0/0.1/0.25/1. Finite negative primary is attainable after inactive disruption. Negative amount/limit/speed, negative zero, NaN/infinity, unbacked objects, and replicated dirty-service execution are excluded; no behavior for those inputs is silently admitted. [Whole instructions and tag inventory](../analysis/multiplayer/game-variants/camouflage-state-20261011/disruption/report.json), [independent byte-identical reproduction](../analysis/multiplayer/game-variants/camouflage-state-20261011/disruption/reproduction.json).

Actual callers distinguish tag attributes from compiled balance logic:

| Original caller | Gate and supplied disruption inputs |
| --- | --- |
| `107160` accepted weapon firing | Requires controlling player; amount/limit from `weap` definition+27C/+280 |
| `CB410` damage feedback | Body result+44 or shield result+48 positive; resolves `jpt!` definition from result+8; amount from definition+30 and limit0 |
| `E8F30` | Calls amount0, limit0; upstream effect context remains outside the execution corpus |

The damage input is **not result+30**. The full original caller and its tag resolution establish this. Lockout's authored weapon rows include SMG/battle rifle0.1, carbine0.2, brute shot0.75, and shotgun/sniper/rocket/beam rifle1.0; their limit fields are0. Lockout SMG bullet and frag/plasma explosion `jpt!`+30 are0.1. The report records every actual Lockout `weap`+27C/+280 and `jpt!`+30 raw field rather than substituting a universal amount. These complete callers were inspected, not newly executed through an accepted shot or positive damage event by this corpus. [Weapon source](../analysis/map-pack-update/functions/00107160.c), [damage source](../analysis/map-pack-update/functions/000cb410.c), [other caller](../analysis/map-pack-update/functions/000e8f30.c), [literal tag fields and full instructions](../analysis/multiplayer/game-variants/camouflage-state-20261011/disruption/report.json).

When unit+2BC is nonzero, `D2310` delegates to actual `C6AA0(0)` and skips the numeric branch. The controller requires a controlling player and either player representation byte+88=1 or its actual campaign/debug override. Intrinsic mode1→2, cooldown timers, source sounds/notifications and its `C6400` scheduling are separate from the shared two player timers. These bodies are preserved for future binding but intrinsic-nonzero execution is not supplied a successful replacement in this fixture. Current native Spartan representation2 and cold intrinsic0 are the bounded context. `161510` also caps a player's active primary at0.5 and marks replication; its upstream contract remains unresolved here. [Intrinsic source](../analysis/map-pack-update/functions/000c6aa0.c), [intrinsic update](../analysis/map-pack-update/functions/000c6400.c), [current-player cap](../analysis/map-pack-update/functions/00161510.c), [full static bodies](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/report.json).

## Reproduction and native boundary

The fixtures and first reports remain immutable. Each independent second generation matches its corresponding fixture and report byte-for-byte:

| Corpus | Fixture SHA256 | Calls |
| --- | --- | --- |
| Corrected FP refresh/timer/fade | `4813fb60a0d23f965c45f666389e895214b44da6ffd5c666013b7011cd17a396` | 4,968 whole principal owners;864 actual conversions |
| Intrinsic-zero disruption | `8217c703eae398081e6b70835eed5f6bff50079de1dde2d4e7866939cab5a699` | 6,912 whole disruption owners |
| Declared timeline | `7b6770e01c6c3afa7dde4648f078f7a3eac6ad733c2e2984b73ab64d3fa65408` | 546 whole principal owners |

```sh
.tools/python/bin/python tools/oracle_camouflage_fp.py
.tools/python/bin/python tools/oracle_camouflage_disruption.py
.tools/python/bin/python tools/oracle_camouflage_timeline.py
.tools/python/bin/python tools/oracle_camouflage_state.py
```

Each script accepts a separate `--output` report path; fixture scripts also accept `--fixture`. [FP reproduction](../analysis/multiplayer/game-variants/camouflage-state-20261011/reproduction.json), [disruption reproduction](../analysis/multiplayer/game-variants/camouflage-state-20261011/disruption/reproduction.json), [timeline reproduction](../analysis/multiplayer/game-variants/camouflage-state-20261011/timeline/reproduction.json), [owner reproduction](../analysis/multiplayer/game-variants/camouflage-state-20261011/owners/reproduction.json).

The source-supported native seam is actual unit flags/amounts/speed with zero allocated defaults; both signed player timers with allocation/spawn reset and leave retention; pre-world refresh before player countdown/current King occupancy; fade in the unit phase; and post-world timer with a current-unit guard. Disruption takes real weapon/damage-definition fields at the actual accepted-fire/positive-result stages. Any explicit grant API is a declared timer operation until its authored pickup owner is bound. Current full player/unit identities, parent rendering, visual camouflage, snapshot/LAN publication, replicated dirty entities, intrinsic suit behavior and normal gameplay validation remain their own required owners. This research neither alters native admission gates nor establishes that the all39-variant goal is complete.
