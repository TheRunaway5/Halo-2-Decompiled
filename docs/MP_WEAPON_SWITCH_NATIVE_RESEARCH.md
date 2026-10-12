# Original retained-weapon switch orchestration

The new isolated [native module](../src/unit_weapon_switch.cpp) owns the ordinary
type8/type18 begin, update, finish and interrupt orchestration, put-away, clearing
the current hand, and equipping the requested retained item. It preserves source
leaf order, full item datums, current/requested separation and slot timestamps.
It does not toggle on a fixed delay. The focused differential matches **79
original calls, 443 leaf argument/full-buffer boundaries and 273 ordered stores**
in both normal and address/undefined/leak sanitizer runs. No runtime, CMake or
shared Unit/header edit is made in this extension.

Live positive switching is still unavailable: the current Match owns one genuine
starting SMG. A second carried object, animation-manager/channel state and their
owning update/attachment services must exist before the module can change hands.
The public [service contract](../include/halo2/unit_weapon_switch.hpp) fails when
an owning service is absent; it never supplies an admission success, elapsed
clock, current pose or second item. [Initial live retained inventory](MP_NATIVE_RETAINED_INVENTORY_RESEARCH.md)
is unchanged.

## New measured original execution

[Observation tool](../tools/oracle_mp_weapon_switch_measured_1_1.py) first reruns
the complete immutable equipped-pickup suite, then repeats it with an observer
subclass. All nine original case payloads remain exactly equal. No new guest
write, instruction patch or service result is added. The independently repeated
new report and all 39 complete routine listings are byte-identical: **2,891
calls, 4,203 recorded stores, 3,296 listed instructions, 2,138 executed listed
instructions**. Per-call unit0x800 prefix, resolved signed+12A/+33E/+342/+346
regions, all actual allocated weapon bodies/headers, game clock12, RNG, FPCW and
MXCSR are retained. This is added observation of the existing fixture, not a new
natural-world admission proof.

The inherited preallocated unit/player arenas, explicit near-item player root,
contact/Havok/attachment services, resource residency and supplied continuation
ticks/input boundaries remain declared. Its weapons are genuinely allocated by
the original routines, with authored Lockout BR rows0/1 and SMG row12, retained
original salted identities and constructor ammo. Do not splice this controlled
held-input fixture into the independently frozen active-world/contact fixture
and call the combination a complete live world.

Authoritative measured directory:
`analysis/multiplayer/integration/inventory-source/weapon-switch-measured/admission-complete/`.
The report SHA256 is
`4d69b9dcbf5eaa13877e2d9433db3eeb41e6c2800db197cbfcab6b93694d7244`.
The new observer SHA256 is
`896f032db055cb2f7ac5dcd802be10e126c86154668fd38037c757e3a248331d`.
The old equipped-pickup and all prior dependencies/pins are checked before and
after execution. Original TU1.1 SHA256 remains
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.

## Completion is an animation-stage owner

Case8 remains `allocated_br_short_other_button_switch_to_smg`: unit00010001
retains SMG00010002 in slot0 and BR00020003 in slot1, sequences2/2, requested0
while current1. The bounded source order is `1D0ED0 → C6C50 → 1021D0(SMG)
→ 1021D0(BR) → E7E60 → 112E10 → BDF80`, with the original request producer.

All five measured EA180 updates retain the action. Current selection changes
inside **EA370 during the later 112E10 animation update**, at declared fixture
tick6: E9A90 first writes current=-1; E9D50 then writes current0 and timestamp6.
The owning graph completion/finish callback is necessary. A weapon timer reaching
zero alone is not this transition. Put-away channel206 has source frames
0,0,1,2 before the ready207 selection. The actual finish is not a universal
five-tick rule. [Measured case and per-call records](../analysis/multiplayer/integration/inventory-source/weapon-switch-measured/admission-complete/report.json)
retain each owning instruction and its full before/after buffers.

Original duration calls execute `16A06A → 169F3B → 1DE7A0 → 148160` with real
selected animation records. The source first type4 event search returns NONE
for the four-frame SMG/BR put-away records; selector1 therefore uses frame_count4.
The 19-frame SMG/BR ready records contain first type4 event15, so selector1 uses15.
`16A0DE` reads4650DC bits3D088889, multiplies the selected frame count; `148166`
reads the signed clock rate at518444+2 (30 in this fixture), performs the second
single-precision product, spills it and FISTP converts under the recorded control
word. These observed4/15 results are metadata/clock outputs, not hardcoded
native delays. The report retains the actual definition360, record6C, event
arrays, semantic action/mode, return and full clock/FP state at those calls.

`1CA2B0` clears channel+14/+16, decrements positive+10 delay, otherwise advances
only when+12 bit0 is set and+11 &9 is zero. Its arithmetic is
`((clock.dt * channel.rate) * XBE46516C) + channel.frame`, rounding each scalar
SSE operation. `1CA090` owns event traversal, end/loop flags and callback effects;
the complete original paths execute and are measured here, but are **not ported
by this native orchestration**. Neither visual shot/reload ticks nor an assumed
completed channel substitutes for them.

## Literal fields and required services

| Native module owner | Source stores or decision |
|---|---|
| E9FE0 | Ordinary request32 type8/type18, immediate+4/silent+5. Actual112270/action22/capability gates; interrupts types0,10,27,22 in order. Pending bit is written at resolved component+4 (EA160), not fixed unit+704. |
| E9B40 | Actual current item, semantic08000025/0D000539 and115800 active query;1042E0 put-away and115650 channel request. A failed1042E0 follows original forced clear and returns true; it is not a guessed failure return. |
| EA180 | Requested==current channel cleanup; otherwise original busy/put-away gates. AL is retained-action status. No timestamp delay comparison is invented. |
| EA370 | Finish forces put-away, clears current, then equips requested. |
| EA270 | Tests actual component action bit before the same clear/equip chain. All10 measured calls have that bit absent; the positive branch is **listed-only**. |
| E9A90 | Weapon+16E word0 (E9B06), trigger/barrel cleanup, world removal, then current byte=-1 (E9B21) and feedback. Slots, full identity and ammo survive. |
| E9D50 | Actual other-hand exclusion; definition+288 class/+28C name;111490/111500; attachment before current store. Current byte(E9EC1), source tick timestamp(E9ECB), then ready/feedback and1C8/1CC/1D0=NONE,1E4/1E8=0,1E0 word0. |

Services receive explicit ABI packets through `WeaponSwitchLeafCall`; the entry
identifier is the original TU1.1 routine, with semantic arguments in this order:

| Entry | Arguments |
|---|---|
| 112270 | unit |
| E8610 | unit, action type |
| CEAF0 | unit, NONE |
| E7FF0 | unit, action type |
| 115800 /115870 | unit, semantic action |
| 1042E0 | full item, immediate |
| 105750 /108230 /FB3F0 /104500 | full item |
| C9A40 | unit,0 |
| EFE30 | unit, source event |
| 115650 | unit, channel0/3, semantic action, blend bits3E88B439,0 |
| 111490 | unit, mode, definition class, definition name |
| 111500 | unit, primary boolean, class, name |
| D1D20 | unit, hand0/1 |
| EFEE0 | unit, primary boolean |
| 1040F0 | full item, silent, immediate (actual pushed argument order) |
| E9C50 | unit, primary boolean |
| 10A140 | full item,0 |

EFEE0's AL overwrites an earlier special-mode stack byte in E9D50. Later event22
and the special ready semantic use this actual returned byte. The final early
exit tests the original immediate argument, not EFEE0's return. The source stack
aliasing and full boundary comparisons distinguish these owners.

Existing `SourceInventory` maps unit210/214,212/213,216/217,218..224,228..234.
It does not supply unitAA/134/1FC, current-player/manager/capability state,
resolved action component or actual channels. `SmgState.ready_ticks` represents
the SMG's17C countdown; slot timestamps are a different owner. Generic BR own
manager/state/duration needs its actual producer. At the clear/current stores,
an eventual live adapter must move the old active weapon into its retained
inactive slot and move the genuine requested item into `Unit.weapon`, preserving
all ammo/clocks/full IDs. It must not rebuild ammo or keep an authoritative
active copy in both places. FB3F0 removes membership, not the retained item.

The positive capability/put-away/finish/equip branches measured above pass. The
EA180 requested==current cleanup branch, positive EA270, alternate special/seat
capability failures and exhausted storage variants are listed-only or unavailable
in these cases. Type8/18 and source slots0..3 are the bounded public domain;
other native storage inputs fail explicitly. Lower attachment, pose, world and
network producers remain required. No new Havok or Xbox reciprocal result is
supplied by this extension.

## Independent native fixture and build seam

[Exporter](../tools/export_mp_weapon_switch.py) emits only literal original
snapshots/leaf arguments/results/stores; native implementation output is never
an expected value. Each callback compares the complete entry buffers, then the
differential alone supplies its recorded complete exit buffers. This verifies
orchestration and ABI, not the callback implementation. [Test](../tests/unit_weapon_switch_tests.cpp)
also rejects a missing owning service before changing that measured path.

Fixture:
`analysis/multiplayer/integration/inventory-source/weapon-switch-native/fixtures.tsv`,
SHA256 `bf4c762714875c66231cdfbb4990787b41f3f3e0eefce93db4d3335c30dd490d`.
Export report SHA256
`8505845633be2a0102cea266e28e7e1d73ef2db2f267d15234ad0571bc9cb681`.
Native validation SHA256
`7f7c21dcc1b099dc96ecbb9485f517d341da949a45459d68bdbed690a1be76ec`.
The two export files independently repeat byte-identically. Validation pins all
new source files and normal/sanitizer binaries/logs.

Root-owned CMake additions: add `src/unit_weapon_switch.cpp` to `halo2_systems`;
create `unit_weapon_switch_tests` from the new test, link `halo2_systems`, and
register `original_unit_weapon_switch` with the fixture above, skip return77.
Standalone builds use C++20, normal `-O2`, and sanitizer
`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`.

## Authenticated1.5 correspondence remains static

The prior immutable complete-body evidence maps E9FE0/E9B40/E9D50/EA180/EA370/
EA270 to EAF00/EAA60/EAC70/EB0A0/EB290/EB190. The new [focused extension](../tools/research_mp_weapon_switch_clock_u5.py)
adds full normalized bodies/internal branches and exact instruction bytes:
E9A90→EA9B0 (53 rows),1CA2B0→1CEB70 (29 rows),1CA090→1CE950 (152 rows).
All three are unique matches. Report plus six listings independently repeat
byte-identically; report SHA256
`e23fba7b03f22ec5b22ed10f09e40a1f5b78b8d7c3907258f010b93329c3d12f`.
Original1.5 SHA256 remains
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No TU1.5 object/action/clock call is asserted. The callback entry identifiers and
ordered native instruction observations remain TU1.1 addresses. Full-body
orchestration correspondence does not prove unmapped leaf bodies or live1.5
pose/world/frame readiness.
