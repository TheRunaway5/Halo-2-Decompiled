# Allocated, attached SMG constructor and weapon-manager state, disc 1.1

The original initial-loadout caller equips the SMG instantly. It leaves `weapon+178 = 9` and `weapon+17C = 0`; it does **not** impose the 15-tick timed-handler delay. The weapon owns a real animation manager. Its authored single-SMG fallback animation has one frame, and original `1021D0` resets the ready/fire/reload state through that manager before running the barrel update. Reload admission in the earlier unit phase consequently sees the previous weapon state. These are executed original-instruction results, not a chosen one-tick delay.

## Authority and reproducibility

- [Independent oracle](../tools/oracle_smg_constructor_equipped_1_1.py), SHA-256 `1c6ce60d4576f37548f822bfbd5667f3e97d806926db593cd98cb9d7f6c9eb99`.
- [Constructor, timed-handler and 12 explicit admission cases](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json), SHA-256 `e071c76ab098696d88f9606026916f042dee169b848872a4eeb49d4acbaf50b1`.
- [Executed routine instruction inventory](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/inventory.json), SHA-256 `50171802696c5385955809601797f1c0fdd4975120ad60ea7088d7c3cddd05ee`.
- [Continuation suite](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/suite.json), SHA-256 `54dad96620c0ee23b2e4be3086984a4f2d0696cdd2b07e1ec8ec1cb91119a764`: six independent full allocation, parent attachment and unit/weapon/postpass cases, **599 ticks and165 projectile requests**. The default report and every case were independently reexecuted and are byte-identical.
- [Post-reload instruction supplement](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/post-reload-inventory.json), SHA-256 `e4d4a6c2ffb3277558f90f228b9837f85ab9a83ad351e9392c07ac9e045a52a6`; together the two instruction inventories contain5923 checked original instruction rows.
- [Independent repeat manifest](../analysis/multiplayer/map-pack/smg-constructor-equipped-reexecution/suite.json), SHA-256 `659ae913e1ec65e358cbfd8217359cd5414379049bf27728c72dcd10e2821b19` (paths differ; every case hash matches the main manifest).
- [Audit report](../analysis/multiplayer/map-pack/smg-constructor-equipped-audit.json), SHA-256 `ef16f1d255fde5a3089a42492e28f262988d6c9c677c0b899cecd4ddcbde0b7b`; [audit/extractor tool](../tools/audit_smg_constructor_equipped_1_1.py), SHA-256 `b5cdd96dfe91c3c31807779a7ba57e6cf2023cb48d3121a3092afeb9b5c44aff`.
- [Raw source integration fixture](../analysis/multiplayer/map-pack/smg-constructor-equipped-source-fixture.json), SHA-256 `8792cebe9429427e9d026dacea2adcee53deba967c18a0ba9c2b3d39fe6597b6`.
- User-owned disc-update `default.xbe.03`: SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`; this is the map-pack disc's **1.1** executable, not proof of the final later title update.
- Original Lockout cache: SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`; original shared cache resources are used through the frozen allocator/resource fixture.
- Frozen dependencies are unchanged: inventory/equip tool `47f56a9236ea2efe870d1ad82af16ace214f4598294f8c826966cced94dee655`, actual attachment tool `9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c`, normal unit-phase tool `cb62dcd1595260984ae37e2e5aedc3a0e09cdc921ad49d104c4ac8d122567a38`.

```sh
.tools/python/bin/python tools/oracle_smg_constructor_equipped_1_1.py
.tools/python/bin/python tools/oracle_smg_constructor_equipped_1_1.py --instruction-inventory analysis/multiplayer/map-pack/smg-constructor-equipped-instructions
.tools/python/bin/python tools/oracle_smg_constructor_equipped_1_1.py --suite analysis/multiplayer/map-pack/smg-constructor-equipped-suite
.tools/python/bin/python tools/oracle_smg_constructor_equipped_1_1.py --output analysis/multiplayer/map-pack/smg-constructor-equipped-reexecution/constructor.json
.tools/python/bin/python tools/oracle_smg_constructor_equipped_1_1.py --suite analysis/multiplayer/map-pack/smg-constructor-equipped-reexecution
.tools/python/bin/python tools/audit_smg_constructor_equipped_1_1.py
```

The supplied game/options boundary is multiplayer: game state `2`, mode byte `1`, world state `4`, original Slayer vtable `00463B48`. The source allocator creates SMG datum `00010002` at fixture address `04000000`, equips it on unit `00010001`, and executes the real parent/marker path. The parent is `00010001` and the selected right-hand node is `20`. Heap arena addresses, tag residency, player input samples and recorded external world/effect/audio services remain explicit inputs. The actual field snapshots resolve that allocator datum; the inherited older helper's fake weapon datum `00010000` is not used as expected state. [Oracle constructor/attachment records](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json).

| Manifest case | Ticks | Projectile requests | Original report SHA-256 |
| --- | --- | --- | --- |
| `idle` |22|0| `b38f5618fbedf708c2b22f9f5803df6d9371548be6378b419c383154267bd9a4` |
| `held` |22|11| `5d9b9cd2e07e5c86125748e997d1c62107eeb01383a142b983dad9fdb20e2331` |
| `fire` |60|23| `eb6c691ac7525dfd34938334f74626c356d4e213dfeb17ed4216a1dea2ff7cc2` |
| `reload_rejected_45` |135|23| `cfc302b76636345ed4e5a31eccc148e1eb058a86583aa43392215bd151f48a54` |
| `reload_accepted_46` |135|23| `f8764ee7869bc1c424b3c72506f31cf44f8c78ced6640b5ba0160f431483cf5a` |
| `held_empty` |225|85| `e7331d0deeeb2113411f81587cd355104aec1dd14f27f47d1ef3d71e56418c12` |

Use only the manifest's case paths. Earlier unlisted working probes in the directory are not part of this frozen attached suite.

## Constructor and initial equip

The actual caller chain is `1A28A0 -> B8A30 -> 101BB0 -> CE510 -> E9D50 -> 1040F0`.
`CE510` receives `(unit10001, weapon10002, mode1)`; `E9D50` receives `(unit10001, 07000101, 1, 1)` with `EAX = 1`; `1040F0` receives weapon datum in `EDI`, stack arguments `(1, 1)`. The instant branch writes zero to `+17C` at `10426E`, bypassing the timed-duration lookup. `1041E4` sets ready state `+178 = 9` after the own-manager request succeeds. The separate explicitly supplied `1040F0(0, 0)` handler, called after initial equip, produces `+17C = 15`; that case does not replace the actual initial caller. [Initial-loadout and timed-handler call/write records](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json), [1040F0 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/001040f0.asm).

| State | Constructor return | Actual initial equip return | Executed producer |
| --- | --- | --- | --- |
| `+178` weapon state | `0` | `9` | `101BF5`, constructor own idle `109B82`, then `1041E4` |
| `+17C` blocking ticks | `0` | `0` | zeroed object backing and instant branch `10426E` |
| `+17E` animation index | `-1` | `-1` | `101D17` |
| `+224` magazine mode | `0` | `0` | constructor; reset mode `108230 -> 109C30` |
| `+226` magazine ticks | `0` | `-1` | reset writer `109D10` |
| `+22A`, `+22C` reserve/loaded | `120`, `60` | `120`, `60` | `101C93`, `101C7D` |
| `+230`, `+232` transfer/other magazine timer | `0`, `0` | `-1`, `-1` | reset writers `109D14`, `109D18` |
| `+1A4` signed barrel counter | `127` | `127` | `101CFB` |
| Remaining barrel scalars `+1A5..+1D3` | zero | zero | actual zeroed allocation and constructor |
| `+1D4` barrel effect handle | `FFFFFFFF` | `FFFFFFFF` | `101CFE` |
| `+20C` trigger state, `+20E` ticks, `+210` flags | zero | zero | actual zeroed allocation/constructor |
| `+214` trigger effect handle | `FFFFFFFF` | `FFFFFFFF` | `101CC4` |
| `+16C/+16E` weapon/input flags | zero | zero | actual constructed/equipped object |

Both unit hand timers are observed zero. This does not claim that the weapon constructor owns or clears the unit's hand-timer fields. Original `BD170` zeroes allocated object storage with `REP STOSD`/`REP STOSB` at `BD1C6/BD1CD`; the report retains the individual subsequent writes rather than treating a guessed native default as the source result. [101BB0 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00101bb0.asm), [BD170 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/000bd170.asm), [109C30 reset instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00109c30.asm), [original snapshots and writes](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json).

Authored SMG magazine data at cache file offset `31366088` contains `+06 = 180` initial total rounds, `+0A = 60` loaded limit and `+0C = 180` reserve limit. The executed constructor loads 60 and puts the remaining 120 in reserve. These fields do not mean 60 loaded plus 180 initial reserve. [Authored records and constructor writes](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json).

## Real weapon-own manager and request producer

The allocated own-manager offset is `+12A = 1116` (`045C`). Graph `E5860411`, `objects\weapons\rifle\smg\smg`, has precisely two authored records in this cache:

| Index | Original name | Frame count | Cache file offset |
| --- | --- | --- | --- |
| `0` | `any:dual:idle` | `5` | `31375196` |
| `1` | `any:idle` | `1` | `31375304` |

The stock single equipped SMG resolves the following requests through original `1D0A60` to own index `1`, not to invented own ready/fire/reload clips:

| Producer/request | Requested StringID | Weapon state |
| --- | --- | --- |
| `1040F0` ready | `05000024` | `9` |
| `109A60(weapon, 0, 1)` forced idle | `0400000C` | `0` |
| `109A60(weapon, 1, 0)` primary fire | `06000006` | `1` |
| `109A60(weapon, 5, 0)` magazine-zero reload | `08000002` | `5` |
| `106E70 -> 109A60(weapon, 3, 0)` post-reload request | `09000004` | `3` |

`109A60` permits a non-forced request when state is zero, or when `0 < current state < 3` and `current <= requested state`. With a valid own manager it selects the activity via the jump table at `109AE6`, requests the graph lookup at `109B71`, and writes the requested state at `109B82` when `AL` succeeds. A manager-absent fixture takes `109B88` without this write; therefore earlier bare-weapon state and reload traces are explicitly **manager-absent boundaries**, not expected state for this allocated/equipped object. The source still invokes the owning unit's `CB060` after its own request. [109A60 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00109a60.asm), [initial source manager requests](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json), [continuation source calls and manager snapshots](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/suite.json).

The stable own primary channel observed after these stock requests is graph `E5860411`, identifier `0001FFFF`, delay `+10 = 0`, flags `+11 = 02`, flags `+12 = 003F`, event words `+14/+16 = 0`, rate `+18 = 3F800000`, frame `+1C = 00000000`. Full raw manager bytes and each request's requested activity are retained. This is an observed stock single-SMG result, not a claim about dual or arbitrary animations. [Per-phase snapshots](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/suite.json).

Original `1021D0` resolves the selected own animation before calling `1CF100` at `102330`. After the clock call it tests own channel event/flag fields. On the non-bit0-event path, it calls `1099B0` when channel flag `+11` bit1 is set **or** the retained selected animation record has exactly one frame (`10234A..10235D`). `1099B0` forces the idle request when state `< 7`, or when state `> 8` except `10`; it preserves `7`, `8`, `10`. For stock index1 this executes for initial ready9, fire1, reload5 and post-reload3. Do not implement this as an unconditional state reset for every weapon or animation. [1021D0 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/001021d0.asm), [1099B0 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/001099b0.asm).

The actual clock executes `1CF100 -> 1CA2B0 -> 1CA090`. `1CA2B0` clears channel event words, decrements a positive delay, and otherwise advances only with `+12` bit0 set and `+11 & 9 == 0`. Its single-precision operation order is `((time_step * channel_rate) * original46516C) + current_frame`; constants here are `time_step = 3D088889` and `46516C = 41F00000` (30). The original `1CA090` performs frame and event processing; the oracle records the actual channel state before/after and original selected animation record. The one-frame state-reset proof above does not require inventing a decoded animation duration. [1CA2B0 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/001ca2b0.asm), [1CA090 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/001ca090.asm), [1CF100 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/001cf100.asm).

## Tick order and reload admission

Each new continuation tick executes original input-record conversion, body-manager blend `1D0ED0`, normal unit wrapper `C7E90`, real weapon update `1021D0`, unit action callbacks `E7E60`, unit clock `112E10`, body local-channel decoder `BDF80`, then projectile postpass `1019F0`. Captured states are before the tick, after unit admission, after weapon update, after body clock, and after postpass. The exact held/buttons sample remains an explicitly supplied input. [Oracle continuation method](../tools/oracle_smg_constructor_equipped_1_1.py), [normal wrapper source evidence](SPARTAN_EQUIPPED_UNIT_PHASE_1_1_RESEARCH.md).

`106030` returns one **only** when all three operands are zero: byte `weapon+20C`, byte `weapon+218`, and dword `weapon+178`. It returns zero for every other tested combination. The new report includes 12 independently supplied operand fixtures (states 0/1/2/5/9/-1/10 and nonzero trigger bytes), with exact original outputs. `106C30` checks this before reserve and loaded-limit checks, then starts own state `5 + magazine_index` and the original magazine mode/timers. [106030 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00106030.asm), [106C30 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00106c30.asm), [explicit operand cases](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json).

For hold45/release with shots on even ticks through44, the shot leaves state1 at the end of44. A reload input at45 is rejected by `106030` during the unit phase; later the weapon-own clock resets state0. A reload input at46 is admitted. The actual reload starts mode1/timer50/state5 in the unit phase, and that same tick's weapon update resets own state0 and decreases the magazine timer to49. This is a phase ordering distinction; it does not authorize buffering or deferring the rejected request. [Rejected-45 and accepted-46 continuation cases](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/suite.json).

The accepted-46 trace transfers23 rounds during weapon tick95: loaded becomes60, reserve97, magazine mode5 and timers-1. Tick96's magazine progression invokes `106E70`, which checks `106030` before the original post-reload own request at `106EEE -> 109A60(weapon, 3, 0)`, writes state3 through `109B82`, and changes magazine mode6/timer0. Tick97's own clock resets state0 before the magazine returns mode0/timers-1. Preserve this state3 and its phase; it was absent from the earlier manager-absent expected state. [Accepted-46 source calls/writes and per-phase rows](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/reload_accepted_46.json), [106E70 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00106e70.asm).

With initial held fire, tick0 begins ready9, retains9 through unit admission, resets0 through the own manager, then prepares/emits the first projectile and sets fire1. Tick1 sees state1 during the unit phase and resets0 in the weapon phase. The 22-tick case emits11 projectile requests at0,2,...20 and ends loaded49. The initial idle22 case emits none and leaves state0. [Held and idle cases](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/suite.json).

The fire1 request occurs inside the weapon update: original `107120` calls `109A60(weapon10002, 1, 0)` at `1078A7` (return `1078AC`), then `109B82` writes state1. It precedes projectile postpass; setting state only after successful projectile creation would produce a different phase. [107120 instructions](../analysis/multiplayer/map-pack/smg-original-instructions/00107120.asm), [held source calls](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/held.json).

## Automatic reload and duration provider

Original `106580` requires reserve rounds, loaded below magazine limit, input-flag bit5 clear, and **object+14 parent != FFFFFFFF** before invoking `101D50`. At `10664E` the parent check is explicit. The earlier supplied `FB560` fixture never established this parent, even though owner `+154` was valid; its lack of automatic reload is consequently a recorded attachment boundary. The new suite executes original `FB560` and hand marker lookup, so it can validate the real automatic path. [106580 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00106580.asm), [frozen actual attachment evidence](../tools/oracle_mp_weapon_attachment_1_1.py).

In the attached empty-hold trace, projectile60 is requested at118. At123 the original path is `106580 -> 101D50 -> E7EF0 -> E88B0 -> E8740 -> 105640 -> 106C30 -> 106030`. It admits with state178/triggers zero and starts magazine mode1, timer50 and own state5 **during the weapon phase**. Tick124 begins with state5, resets own state0 and decreases the timer49. This late automatic start differs from the earlier unit-phase manual start; preserve the phase. [Held-empty continuation](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/suite.json), [101D50 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00101d50.asm), [E8740 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/000e8740.asm), [105640 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00105640.asm).

Tick173 transfers60 rounds (loaded60/reserve60/mode5); tick174 selects post-reload own state3 and magazine mode6/timer0. Tick175 resets own state0, finishes magazine mode0, and immediately prepares/emits the next shot, leaving state1/loaded59. Subsequent requests are175,177,...223; the225-tick case therefore contains85 requests. [Held-empty per-phase rows and request ticks](../analysis/multiplayer/map-pack/smg-constructor-equipped-suite/held_empty.json).

## Frozen fixture format

The full reports retain source instructions, calls, register/stack inputs, writes, actual attachment records and per-phase raw objects. Some low-level calls retain the tracked starting SMG as a **context snapshot**, even when the callee operates on the target BR or body graph. The snapshot's `datum` is explicit; identify a callee object from its actual stack/register arguments. The row-level phase snapshots consistently represent actual starting SMG10002.

The [source fixture](../analysis/multiplayer/map-pack/smg-constructor-equipped-source-fixture.json) extracts only the manifest cases. Each case has `initial`, then `rows[tick]` with supplied input bytes, original converted input bytes, five `phases` (`before`, `after_unit`, `after_weapon`, `after_animation`, `after_postpass`) and raw projectile-request descriptors. Each phase retains full244 weapon bytes `+16C..+25F`, all52 barrel bytes, trigger12/magazine16 bytes, native scalar fields, original manager144 bytes, selected own record/channel fields, unit hand timers and shared seed. This yields2995 raw phase snapshots; the extractor verifies all decoded fields against their original raw offsets. Projectile creation fails at the explicitly supplied service after the descriptor request; the fixture does not invent success outputs. [Audit/extractor](../tools/audit_smg_constructor_equipped_1_1.py), [audit report](../analysis/multiplayer/map-pack/smg-constructor-equipped-audit.json).

`109C30` determines whether the magazine is empty, calls `109D50` to choose the first-person **duration provider** event index, then `1680B3` and `16A06A`. For reload mode1, empty selects index7 and nonempty selects index8. Actual cache first-person graph `E5AA0435` resolves them to `first_person:reload_empty` index30 or `first_person:reload_full` index31; both are50 frames and contain zero timing-event records here. Original total/transfer duration lookup therefore returns50. The report includes both authored records and executed lookup calls; the own graph still uses one-frame any:idle. [109C30 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00109c30.asm), [109D50 instructions](../analysis/multiplayer/map-pack/smg-constructor-equipped-instructions/00109d50.asm), [authored first-person records](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json).

## Boundaries

The new suite executes a real allocated, equipped and attached weapon manager. It supplies failed projectile allocation **after** the successful original starting-loadout allocation and supplies a clear muzzle query; it records projectile requests and shared-seed progression without claiming successful-world projectile lifetime. It does not execute physical `DE960`, the item `F9B10` ancestor update, or the full recursive `BD360` world producer. Those boundaries are unchanged and explicit in each case.

First-person globals and view assignment are not proven complete. The fixture's player local-view field is observed `0`, first-person selector is `0`, and most first-person globals remain `FFFFFFFF`; `16A356` remains a supplied notification service. Reading actual first-person duration records proves the duration provider, **not** a rendered first-person ready/reload clock, pose or presentation phase. `first_person:ready` index29 has19 frames and a type4 event at15, but the actual instant initial-loadout caller still bypasses timed blocking. Xbox reciprocal approximation uncertainty in marker/matrix routines also remains separately documented; it does not alter the proved manager lookup, integer admission or stock one-frame state-reset branch. [Presentation boundary and authored ready record](../analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json), [timed ready source evidence](SMG_READY_ADMISSION_1_1_RESEARCH.md).

No production runtime files or earlier frozen oracle suites are changed by this research. `native_gameplay_update_applied` remains a production integration claim requiring its own native differential; these original traces alone do not set it true.
