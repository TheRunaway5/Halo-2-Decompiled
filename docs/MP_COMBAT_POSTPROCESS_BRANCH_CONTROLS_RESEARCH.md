# Original combat postprocessor branch controls and width clarification

A new independent fixture executes 43 explicitly altered raw-metadata ABI inputs in both owned images: 86 original runs, 82 returns and four real invalid-access stops. All paired original metadata write traces and complete cache outputs match between disc1.1 `164750` and owned1.5 `165B30`. The deliberately altered operands are supplied **test inputs**, not authored game values or proof of initialization producers. No source field, callback, service or successful return is fabricated to admit an invalid branch. Original cache/XBE files and all prior freezes remain unchanged.

## Width clarification, kept separate from the earlier immutable document

The original BR barrel gate is **signed 32-bit** `weap+2D0 > 0`: `16479B MOV EDI,DWORD [ECX+2D0]`, `1647A1 TEST EDI,EDI`, `1647A4 JLE skip`. Its 1.5 counterpart reads the same width and gate. This adds the explicit width to the earlier [combat contract](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md), which describes a signed barrel count without a width; that immutable document/report is not rewritten. The scenario-Type gate remains **signed16** at scenario `+10 == 1`.

The full original fixture proves the distinction: BR counts zero, `FFFFFFFF` and `80000000` skip exactly the two BR error-angle stores (39 writes remain); count `00010000` and count2 execute all41 writes. Treating count00010000 as an int16 would incorrectly reject this boundary input. No alternate barrel or fabricated barrel pointer is supplied: only the count word changes in the disposable mapped copy.

## Exact branch outcomes

The authored Lockout run is the first control and matches the prior whole combat fixture. Every remaining input changes exactly one named operand before execution; its authored bytes, explicit control bytes, file offset, original VA and size are retained. Complete ordered stores retain the original datum/path/relative offset and actual before/after bytes. The complete cache output can be reconstructed from the original raw cache, that one explicit input change, and the measured source stores; the independent audit checks its complete hash.

| Explicit input control | Original return? | Metadata writes | Actual changed writes |
|---|---|---|---|
| `authored_lockout` | yes | 41 | 38 |
| `scenario_type_0` | yes | 0 | 0 |
| `scenario_type_2` | yes | 0 | 0 |
| `scenario_type_3` | yes | 0 | 0 |
| `scenario_type_4` | yes | 0 | 0 |
| `scenario_type_65535` | yes | 0 | 0 |
| `weapon_slot_0_NONE` | yes | 38 | 35 |
| `weapon_slot_1_NONE` | yes | 36 | 33 |
| `weapon_slot_3_NONE` | yes | 39 | 36 |
| `weapon_slot_5_NONE` | yes | 38 | 35 |
| `weapon_slot_7_NONE` | yes | 40 | 37 |
| `weapon_slot_12_NONE` | yes | 35 | 32 |
| `vehicle_slot_7_NONE` | yes | 40 | 37 |
| `BR_barrel_signed32_00000000` | yes | 39 | 36 |
| `BR_barrel_signed32_ffffffff` | yes | 39 | 36 |
| `BR_barrel_signed32_80000000` | yes | 39 | 36 |
| `BR_barrel_signed32_00010000` | yes | 41 | 38 |
| `BR_barrel_signed32_00000002` | yes | 41 | 38 |
| `weapon_slot_1_melee_damage_NONE` | yes | 38 | 35 |
| `weapon_slot_5_melee_damage_NONE` | yes | 38 | 35 |
| `weapon_slot_12_melee_damage_NONE` | yes | 38 | 35 |
| `weapon_slot_0_barrel_projectile_NONE` | yes | 38 | 35 |
| `weapon_slot_1_barrel_projectile_NONE` | yes | 39 | 36 |
| `weapon_slot_12_barrel_projectile_NONE` | yes | 38 | 35 |
| `weapon_slot_0_projectile_impact_damage_NONE` | yes | 38 | 35 |
| `weapon_slot_1_projectile_impact_damage_NONE` | yes | 39 | 36 |
| `weapon_slot_12_projectile_detonation_damage_NONE` | yes | 38 | 35 |
| `grenade_0_projectile_NONE` | yes | 32 | 31 |
| `grenade_0_detonation_damage_NONE` | yes | 33 | 32 |
| `grenade_1_projectile_NONE` | yes | 30 | 28 |
| `grenade_1_detonation_damage_NONE` | yes | 33 | 31 |
| `grenade_count_00000000` | actual access stop | 11 | 11 |
| `grenade_count_00000001` | yes | 41 | 38 |
| `grenade_count_00000003` | yes | 41 | 38 |
| `grenade_count_ffffffff` | yes | 41 | 38 |
| `weapon_slot_0_barrel_count_zero_pointer_retained` | yes | 41 | 38 |
| `weapon_slot_1_barrel_count_zero_pointer_retained` | yes | 41 | 38 |
| `weapon_slot_7_barrel_count_zero_pointer_retained` | yes | 41 | 38 |
| `weapon_slot_12_barrel_count_zero_pointer_retained` | yes | 41 | 38 |
| `runtime_count_zero_pointer_retained` | yes | 41 | 38 |
| `weapons_count_zero_pointer_retained` | yes | 41 | 38 |
| `vehicles_count_zero_pointer_retained` | yes | 41 | 38 |
| `matg_multiplayer_reference_NONE` | actual access stop | 0 | 0 |

The zero grenade count is **not** a successful no-op. The source has already performed11 preceding weapon/melee stores when it selects a zero grenade pointer, then stops on `MOV ECX,[ECX+28]`: 1.1 instruction `001648AF`, 1.5 `00165C8F`, read address `00000028`, size4, ECX0. No mapped zero page, callback, pointer, guard, or return is supplied. The partial original metadata stores remain in the report. A native safe rejection is an explicit host safety adaptation and must not be presented as the original function returning normally.

The grenade count1, count3 and `FFFFFFFF` controls all return with41 writes because the real second-entry pointer is deliberately retained. This fixture does not claim that a one-entry buffer contains a second entry: it specifically demonstrates that the source count check is nonzero rather than `>=2`. Similarly, Runtime, Weapons and Vehicles count0 controls retain their actual valid pointers and still emit41 writes; those counts are not validated by this routine. Non-BR first-barrel count0 controls likewise retain the actual pointer and still emit41 writes.

Changing the root `matg+16C` multiplayer reference to NONE yields a separate actual stop: 1.1 `0016477F` / 1.5 `00165B5F`, read address `0000000C`, size4, zero ECX after low16 tag lookup. No metadata store precedes it. The host's raw tag-table backing beyond real entries is zeroed storage, an explicitly recorded mapping boundary; the original source does not validate that root datum here.

## Reproduce and preserve

[`tools/oracle_mp_combat_postprocess_controls.py`](../tools/oracle_mp_combat_postprocess_controls.py) imports the prior oracle's constants/types read-only and creates separate mappings. It does not invoke or modify the prior execution report. Run into a fresh directory using `.tools/python/bin/python tools/oracle_mp_combat_postprocess_controls.py --output <new-directory>`. Reports and original262-instruction listings are retained under `analysis/multiplayer/title-update-1.5/combat-postprocess-controls/{measured,repeat}`; the whole repeat is identical. The separate audit checks all source bytes, control operands, ordered stores, full reconstructed cache outputs, original stops, and earlier manifest hashes.

This evidence supplies native differential expectations for the bounded processed metadata view. It proves neither whole map/cache initialization, unsafe malformed-data admission, full gameplay, native integration, a new1.5 weapon tuning change, nor Xbox hardware behavior. The separate executed [1.5 Relic correction](MP_NETGAME_POSTPROCESS_1_1_TO_1_5_RESEARCH.md) remains distinct.
