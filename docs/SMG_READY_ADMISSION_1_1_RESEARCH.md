# Disc 1.1 equipped SMG ready countdown and firing admission

The stock equipped SMG's initial weapon+17C value15 is produced by the **real first-person ready animation event**, not by the bare weapon constructor or a guessed equip duration. Original `E9D50 -> 1040F0 -> 16A06A` performs the lookup and stores the result. This independently frozen evidence adds no production changes and leaves all previous suites unchanged.

## Authority and executable evidence

```sh
.tools/python/bin/python tools/oracle_smg_ready_admission_1_1.py
.tools/python/bin/python tools/oracle_smg_ready_admission_1_1.py --instruction-inventory analysis/multiplayer/map-pack/smg-ready-admission-instructions
```

| Authority / output | SHA-256 |
|---|---|
| Disc 1.1 `default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Original `lockout.map` | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |
| [Oracle tool](../tools/oracle_smg_ready_admission_1_1.py) | `cc5d445c1d048d17be6189d2e340c18f794c4e7dd17ed9d4baaff429ec49961b` |
| [Original-execution report](../analysis/multiplayer/map-pack/smg-ready-admission-1.1-oracle.json) | `c5a128b7fb32dac666e573595f660c4b1569845bf524cbe0e20e5f919596a2e3` |
| [Instruction inventory](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/inventory.json) | `d132eab864487fb57a87304cdc2635c959bdfd825b836db22f0deeb62d5369a1` |
| Unchanged normal unit-phase dependency | `cb62dcd1595260984ae37e2e5aedc3a0e09cdc921ad49d104c4ac8d122567a38` |

The stock idle22/held22 cases record **44 normal-wrapper ticks**, exact countdown writes and original ready lookup call/return records. Held emits four requests at14/16/18/20 and eight shared RNG writes; idle emits neither. Seven separately labeled admission-state cases and seven varied-clock queries execute unchanged source routines with explicit modified boundary inputs. Expected values are original instruction outputs, independent of native implementations.

The supplied storage, resource residency, clear world query, failed projectile allocator, animation/audio/effect service boundaries are the same as the frozen [normal unit-wrapper suite](SPARTAN_EQUIPPED_UNIT_PHASE_1_1_RESEARCH.md). This proves source state/control behavior under those inputs, not the complete original physical/object/world update. Allocator request component labels inherited from the earliest firing tool are not used as authority for descriptor semantics; only original requests/counts and the established random-output record are relevant here.

## Authored first-person event and exact producer

For Lockout SMG `weap E57B0406`, the first-person block is **weap+2A8/+2AC**, stride16. Record0 at file31366008 contains jmad **E5AA0435**, `objects\characters\masterchief\fp\weapons\rifle\fp_smg\fp_smg`. The assigned player fixture's original first-person selector byte PLAYER+88 is0. Original [169F3B](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/00169f3b.asm) follows weapon+154 to its assigned owner, owner+13C to the player, checks this selector against -1, and calls [11CF00](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/0011cf00.asm) to choose the authored first-person record. This route is distinct from the weapon-own graph E5860411 and the Spartan third-person graph E9180793.

| Source-selected ready data | Exact value |
|---|---|
| First-person animation index / name | 29 / `first_person:ready` |
| Animation record file offset / mapped pointer | 31396928 / `8021DC40` |
| Animation name StringID | `12000AFD` |
| Animation+14 frame count | 19 |
| Animation+4C/+50 events block | one stride4 entry |
| Event file offset / raw bytes | 31397496 / `04000F00` |
| Event type / frame | **4 / 15** |

The exact record, block pointers and event bytes are independently read from the cache and recorded again from the selected original lookup result. The third-person ready clip207 also has19 frames, but its length is not the producer of weapon+17C.

Original [1040F0](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/001040f0.asm) receives EDI=weapon datum and two stack byte arguments. The executed ordinary equip receives `(0,0)`. At1042B2..1042C2 it calls **16A06A** with ECX=weapon datum, stack `(ready StringID05000024, selector1)`. At **1042C9** it stores returned AX as a signed16 countdown at weapon+17C. The explicitly tested second-argument1 route stores zero at10426E and bypasses the timed lookup; it is not the ordinary fixture's argument.

## Event selection and clock arithmetic

Original [16A06A](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/0016a06a.asm) obtains the animation via169F3B. It reads frame count+14, sets ESI=4 and calls [1DE7A0](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/001de7a0.asm) with ECX=record. This helper scans event stride4 in authored order for type4 and returns the event's signed16 frame. Selector1 uses that frame when valid and less than the frame count, otherwise the frame count. Here it returns15.

At16A0DA..16A0EC the source:

1. Converts the selected integer15 to float32.
2. Multiplies it by original data at **4650DC, bits3D088889** (float32 one-thirtieth); the result is bits3F000000.
3. Calls [148160](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/00148160.asm), which sign-extends the clock's short at `[518444]+2`, converts it to float32, performs another separately rounded MULSS, loads that float with FLD, and converts it with **FISTP**.
4. For a positive selected frame, clamps a returned short below2 to1.

The stock clock input is30, so the result is exactly15. The oracle explicitly supplies FPU control word027F and MXCSR1F80 for execution. Thus the varied-clock results establish nearest-even FISTP under that recorded control boundary; these queries are not a physical Xbox capture or proof of every global FPU-state producer. The actual stock product15 is integral and does not depend on a half-way rounding choice.

| Explicit clock short input | Ready result |
|---:|---:|
| 1 | 1 |
| 15 | 8 |
| 24 | 12 |
| 30 | 15 |
| 45 | 22 |
| 60 | 30 |
| 90 | 45 |

The separately supplied absent first-person byte PLAYER+88=-1 causes the original lookup to return no record, and16A06A returns0. That is an explicit alternate input, not a replacement default for equipped production players.

## Countdown update and native integration contract

Original [1021D0](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/001021d0.asm) at10273C..102753 reads weapon+17C as signed16 and decrements it **only when positive**. At1027AC..1027D4, after the decrement, it blocks the trigger/barrel route if any of these hold:

- weapon+16E input bit5 (`0x20`) is set;
- weapon+17C remains positive;
- weapon+16C flag bit1 (`0x02`) is set.

Therefore the current native `SmgState.blocking_ticks` decrement-before-test agrees with this source countdown/admission operation. It still needs the real equip producer to initialize the countdown. The earlier bare-constructor firing fixture deliberately starts17C=0 and first fires tick0; that remains a valid bounded fixture, not evidence that a normally equipped weapon begins ready.

The stock full unit wrapper creates input flags000A at held tick0 and0002 thereafter. It **does not add an extra input0x20 just because the ready countdown is positive** under the real idle/secondary-ready state. The ready delay is consumed by the weapon update itself:

| Tick | +17C before unit/weapon update | +17C after weapon update | Original request |
|---:|---:|---:|---|
| 0 | 15 | 14 | none |
| 13 | 2 | 1 | none |
| 14 | 1 | 0 | first |
| 15 | 0 | 0 | none |
| 16 | 0 | 0 | second |

Seven explicit modified admission states prove the local boundary:

| Supplied +17C | Input flags | Weapon flags | Updated +17C | Requests |
|---:|---:|---:|---:|---:|
| -1 | 2 | 0 | -1 | 1 |
| 0 | 2 | 0 | 0 | 1 |
| 1 | 2 | 0 | 0 | 1 |
| 2 | 2 | 0 | 1 | 0 |
| 15 | 2 | 0 | 14 | 0 |
| 0 | 34 | 0 | 0 | 0 |
| 0 | 2 | 2 | 0 | 0 |

The weapon+19E and weapon+16C bit5 arming state are separate from+17C. In the stock fixture equip leaves+19E=0 and clears bit5; the first source weapon update decrements+19E to-1 and sets bit5. The existing native unconditional bit5 arming matches this particular authored starting case, not arbitrary definitions or starting states.

## Related hand and grenade control gates

The complete original [C6C50](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/000c6c50.asm) is executed every stock tick, rather than replacing its output with held-trigger guesses. AtC6CD8..C6CFB it decrements nonzero byte timers unit+1FA/+1FB before weapon input; their fixture values and actual post-shot changes are retained. Native `smg_input`'s nonzero byte decrement agrees with this bounded operation. The source timer-decay region itself is skipped when authored unit+BC bit11 is set; that alternate authored-unit behavior is not ported by a universal SMG timer rule.

Original [112270](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/00112270.asm) is an **activity-dependent unit action gate**, not a second universal ready countdown. It computes primary weapon+17C<=0, forces that result false when a present secondary weapon's countdown is positive, reads manager flag+14 bit4, then evaluates the actual primary and secondary activity StringIDs and C8410. C6C50 uses its result while composing each hand's input bit0x20; explicit action-state bits22/26/27 and hand-specific8/18 can force that bit, whereas positive unit+1EC clears the earlier gate result. Those exact branch/bit operands remain in the listings and call snapshots. It is unsafe to collapse this into `ready countdown positive => unit blocked`.

For grenade selection, C6C50 changes current unit+23C only when desired+23D differs,112270 returns false, and original [CD310](../analysis/multiplayer/map-pack/smg-ready-admission-instructions/000cd310.asm) returns an available selection. CD310 searches the two signed-positive count bytes unit+23E/+23F with its supplied index/direction inputs. This establishes the selection gate, not the full grenade throw/spawn/timer producer. Reload additionally follows the proven E8740 gate path in the equipped action suite. Successful grenade allocation, ordinary moving-player DE960/classification, device ownership and the entire world tick remain separate evidence gates.
