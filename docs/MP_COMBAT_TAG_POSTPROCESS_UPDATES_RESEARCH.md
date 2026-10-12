# Original multiplayer combat tag postprocessing in the owned updates

The complete original routine changes raw Lockout combat metadata before gameplay. Both owned updates produce **41 ordered metadata writes, 38 actual value changes, and 13 changed tags**, with identical complete cache outputs. This is an existing 1.1/1.5 adjustment, **not a newly discovered 1.5-only balance change**. In particular, the SMG bullet upper damage endpoints become float bits `40940000` (4.625), rather than raw-cache `40980000` (4.75); its lower bound remains unchanged.

## Source authority and execution

- Disc 1.1 executable: `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Separately owned 1.5 executable: `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. The parent independently verifies its Xbox header/section signatures and actual 1.5 display producer; this fixture does not substitute annotations for executable bytes.
- Raw Lockout cache: `assets/disc/maps/lockout.map`, SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- Callee `00164750` in 1.1 corresponds to `00165B30` in 1.5. Each has 262 traced instructions, no calls, and no switch tables. Their complete normalized instructions and internal branch topology are equal; every paired original image-memory operand byte is equal. The actual routine executes unchanged in Unicorn in each image, returns normally, and emits the same ordered metadata writes and complete cache output hash.
- Map-load caller `0011DE10` → `0011EF70` iterates the tag group fixups, calls `0011DF00` → `0011F060`, then invokes the combat routine. The two original caller listings retain this call order. **This fixture executes the complete combat callee, not the preceding map loader, group fixups, or preceding callee.**
- Exact raw tag-table bytes and all raw cache bytes are mapped unchanged at their authored virtual addresses. Three declared ABI bindings point the original global slots to this raw table, the actual `matg`, and the actual scenario: 1.1 `4EB114 / 4E791C / 4E7920`; 1.5 `4F0414 / 4ECC1C / 4ECC20`. The pointers are input bindings, not a claim that original initialization executed. No altered tuning values, fabricated gameplay state, service return, or source callback output is supplied.
- Full before/after tag buffers, exact ordered stores (including equal-value stores), executed addresses, metadata/stack write distinctions, schema pins, and both source/caller instruction listings are independently retained under `analysis/multiplayer/title-update-1.5/combat-postprocess/{measured,repeat}`. Whole reports and every binary/listing repeat identically.

The reproducible oracle is [`tools/oracle_mp_combat_postprocess_updates.py`](../tools/oracle_mp_combat_postprocess_updates.py). Its report is `analysis/multiplayer/title-update-1.5/combat-postprocess/measured/report.json`. Field labels below are corroborating annotations from pinned Assembly Halo2 schemas; original addresses, byte values, reference walks, and stores are the behavioral authority.

## Exact admission and reference contract

1. The sole outer gate reads **signed 16-bit scenario `+10 == 1`**. It is the authored scenario Type, not scenario `+1C` (Local North), and not the runtime game mode/options. Raw Lockout satisfies it. If unequal, the source immediately returns before any metadata write.
2. `matg+16C` supplies a multiplayer-globals datum. Resolution is `tag_table[(datum & FFFF) * 10 + 8]`. The source then directly reads the first `mulg` Runtime pointer at `+0C`. It does **not** validate these root references, the Runtime count, the Weapons count, or the Vehicles count. A safe native adapter may reject malformed/missing inputs, but that would be an explicit host safety boundary rather than an added source gate.
3. Runtime `+6C` is the Weapons block pointer. Runtime `+74` is the Vehicles block pointer. Entries are eight bytes; the datum is entry `+4`. Slot identity comes from this authored runtime list, not a hardcoded tag path. Every referenced weapon/vehicle datum is compared against `FFFFFFFF` before resolution. The source uses the low 16-bit datum index, without group or salt validation here.
4. BR slot 3 alone checks the signed weapon barrel count `weap+2D0 > 0` before reading the first barrel pointer `+2D4`. Other first-barrel consumers below directly use `weap+2D4` once their weapon datum is non-NONE; no additional barrel-count gate is present.
5. Melee paths additionally require `weap+1C0 != NONE` before resolving that damage tag. Impact paths require first barrel `+90` projectile datum and then `proj+14C` impact damage datum to be non-NONE. Brute-shot explosion path instead follows `proj+104` detonation damage datum. Each NONE check skips that particular write group only.
6. Grenades are the first two `matg+100/+104` entries, stride `2C`, projectile datum `+28`. The source tests only whether the grenade count is **nonzero**, selects its pointer (or zero), then unconditionally reads the selected entry `+28`. Thus zero count does **not** yield a clean source early-out; raw Lockout has two real entries. Entry 1 is selected even if count is one. The grenade projectile and its detonation `proj+104` damage datum have NONE gates where shown. Arming/timer writes require only a non-NONE grenade projectile.
7. No RNG, clock, object, player, ammo, local-view, physics, sound, or external service enters this routine. Stores are exact 32-bit assignments; no arithmetic or rounding occurs in the postprocess itself.

## Authored Lockout slot resolution and ordered output groups

These original runtime-slot walks and constants are the bounded native consumer contract. Native content loading should expose a processed view before constructing gameplay definitions, while preserving the original cache file. The source walk determines which shared damage tag receives a write, so every weapon referencing that same datum observes the change.

| Order | Source walk | Destination | Exact output bits/value |
|---|---|---|---|
| 1 | Weapons slot 3 (BR), first barrel | barrel `+74/+78` error-angle endpoints | `3AE4C388` / 0.001745329238474369 |
| 2 | Weapons slot 12 (brute shot), melee `weap+1C0` | damage `+1C/+20/+24` | `42B40000/42F00000/42F00000` / 90,120,120 |
| 3 | Weapons slot 5 (rocket), melee | damage `+1C/+20/+24` | `425C0000/42B40000/42B40000` / 55,90,90 |
| 4 | Weapons slot 1 (SMG), melee | damage `+1C/+20/+24` | `425C0000/42960000/42960000` / 55,75,75 |
| 5 | matg grenade 0, projectile detonation `+104` | damage `+0/+4/+1C/+20/+24/+44/+48/+4C` | `3F000000/3FE00000/42480000/43200000/43200000/3FC00000/3F400000/3F400000` / .5,1.75,50,160,160,1.5,.75,.75 |
| 6 | matg grenade 0 projectile | projectile `+CC` arming time | `3FA66666` / 1.2999999523162842 |
| 7 | matg grenade 1, projectile detonation `+104` | same eight damage fields | `3F000000/3FE00000/42480000/43480000/43480000/3FC00000/3F400000/3F400000` / .5,1.75,50,200,200,1.5,.75,.75 |
| 8 | matg grenade 1 projectile | projectile `+CC/+D4/+D8` arming/timer endpoints | `3FA66666/3F666666/3F666666` / 1.2999999523,.8999999762,.8999999762 |
| 9 | Weapons slot 0 (magnum), barrel projectile impact `+14C` | damage `+1C/+20/+24` | `40B00000` at all three / 5.5 |
| 10 | Weapons slot 7 (plasma rifle), first barrel | barrel `+64` dual-wield damage scale | `3F333333` / .699999988079071 |
| 11 | Weapons slot 1 (SMG), barrel projectile impact `+14C` | damage **only `+20/+24`**, lower bound untouched | `40940000` at both / 4.625 |
| 12 | Weapons slot 12 (brute shot), barrel projectile detonation `+104` | damage `+1C/+20/+24` | `41F00000/42700000/42700000` / 30,60,60 |
| 13 | Vehicles slot 7 (h_turret_ap) | vehicle `+4` bounding radius | `3F000000` / .5 |

## Complete measured field changes

| 1.1 store | Datum / authored tag | Tag-relative field | Raw bits/value | Processed bits/value |
|---|---|---|---|---|
| `001647b4` | `e192001d` `objects\weapons\rifle\battle_rifle\battle_rifle` | `+00000580` Barrels[0]/Error Angle | `3bab92a6` / 0.005235987715 | `3ae4c388` / 0.001745329238 |
| `001647b9` | `e192001d` `objects\weapons\rifle\battle_rifle\battle_rifle` | `+00000584` Barrels[0]/Error Angle endpoint+4 | `3c2b92a6` / 0.01047197543 | `3ae4c388` / 0.001745329238 |
| `001647fe` | `e300018b` `objects\weapons\damage_effects\slice_melee` | `+0000001c` Damage Lower Bound | `428c0000` / 70 | `42b40000` / 90 |
| `00164803` | `e300018b` `objects\weapons\damage_effects\slice_melee` | `+00000020` Damage Upper Bound | `428c0000` / 70 | `42f00000` / 120 |
| `00164808` | `e300018b` `objects\weapons\damage_effects\slice_melee` | `+00000024` Damage Upper Bound endpoint+4 | `428c0000` / 70 | `42f00000` / 120 |
| `00164845` | `e28e0119` `objects\weapons\damage_effects\smash_melee` | `+0000001c` Damage Lower Bound | `42200000` / 40 | `425c0000` / 55 |
| `0016484a` | `e28e0119` `objects\weapons\damage_effects\smash_melee` | `+00000020` Damage Upper Bound | `42f00000` / 120 | `42b40000` / 90 |
| `0016484f` | `e28e0119` `objects\weapons\damage_effects\smash_melee` | `+00000024` Damage Upper Bound endpoint+4 | `42f00000` / 120 | `42b40000` / 90 |
| `0016488c` | `e28c0117` `objects\weapons\damage_effects\strike_melee` | `+0000001c` Damage Lower Bound | `41a00000` / 20 | `425c0000` / 55 |
| `00164891` | `e28c0117` `objects\weapons\damage_effects\strike_melee` | `+00000020` Damage Upper Bound | `42700000` / 60 | `42960000` / 75 |
| `00164896` | `e28c0117` `objects\weapons\damage_effects\strike_melee` | `+00000024` Damage Upper Bound endpoint+4 | `42700000` / 60 | `42960000` / 75 |
| `0016490c` | `e7ae0629` `objects\weapons\grenade\frag_grenade\damage_effects\frag_grenade_explosion` | `+00000000` Radius | `3f400000` / 0.75 | `3f000000` / 0.5 |
| `00164910` | `e7ae0629` `objects\weapons\grenade\frag_grenade\damage_effects\frag_grenade_explosion` | `+00000004` Radius endpoint+4 | `3fe00000` / 1.75 | `3fe00000` / 1.75 |
| `00164915` | `e7ae0629` `objects\weapons\grenade\frag_grenade\damage_effects\frag_grenade_explosion` | `+0000001c` Damage Lower Bound | `42480000` / 50 | `42480000` / 50 |
| `0016491a` | `e7ae0629` `objects\weapons\grenade\frag_grenade\damage_effects\frag_grenade_explosion` | `+00000020` Damage Upper Bound | `43160000` / 150 | `43200000` / 160 |
| `0016491f` | `e7ae0629` `objects\weapons\grenade\frag_grenade\damage_effects\frag_grenade_explosion` | `+00000024` Damage Upper Bound endpoint+4 | `43160000` / 150 | `43200000` / 160 |
| `00164924` | `e7ae0629` `objects\weapons\grenade\frag_grenade\damage_effects\frag_grenade_explosion` | `+00000044` Rider Direct Damage Scale | `3f800000` / 1 | `3fc00000` / 1.5 |
| `00164929` | `e7ae0629` `objects\weapons\grenade\frag_grenade\damage_effects\frag_grenade_explosion` | `+00000048` Rider Maximum Transfer Damage Scale | `3f000000` / 0.5 | `3f400000` / 0.75 |
| `0016492e` | `e7ae0629` `objects\weapons\grenade\frag_grenade\damage_effects\frag_grenade_explosion` | `+0000004c` Rider Minimum Transfer Damage Scale | `3f000000` / 0.5 | `3f400000` / 0.75 |
| `00164964` | `e7a80623` `objects\weapons\grenade\frag_grenade\frag_grenade` | `+000000cc` Arming Time | `3fc00000` / 1.5 | `3fa66666` / 1.299999952 |
| `001649b8` | `e7c3063e` `objects\weapons\grenade\plasma_grenade\damage_effects\plasma_grenade_explosion` | `+00000000` Radius | `3f400000` / 0.75 | `3f000000` / 0.5 |
| `001649bc` | `e7c3063e` `objects\weapons\grenade\plasma_grenade\damage_effects\plasma_grenade_explosion` | `+00000004` Radius endpoint+4 | `3fc00000` / 1.5 | `3fe00000` / 1.75 |
| `001649c1` | `e7c3063e` `objects\weapons\grenade\plasma_grenade\damage_effects\plasma_grenade_explosion` | `+0000001c` Damage Lower Bound | `42480000` / 50 | `42480000` / 50 |
| `001649c6` | `e7c3063e` `objects\weapons\grenade\plasma_grenade\damage_effects\plasma_grenade_explosion` | `+00000020` Damage Upper Bound | `42f00000` / 120 | `43480000` / 200 |
| `001649cb` | `e7c3063e` `objects\weapons\grenade\plasma_grenade\damage_effects\plasma_grenade_explosion` | `+00000024` Damage Upper Bound endpoint+4 | `42f00000` / 120 | `43480000` / 200 |
| `001649d0` | `e7c3063e` `objects\weapons\grenade\plasma_grenade\damage_effects\plasma_grenade_explosion` | `+00000044` Rider Direct Damage Scale | `3f800000` / 1 | `3fc00000` / 1.5 |
| `001649d5` | `e7c3063e` `objects\weapons\grenade\plasma_grenade\damage_effects\plasma_grenade_explosion` | `+00000048` Rider Maximum Transfer Damage Scale | `3f000000` / 0.5 | `3f400000` / 0.75 |
| `001649da` | `e7c3063e` `objects\weapons\grenade\plasma_grenade\damage_effects\plasma_grenade_explosion` | `+0000004c` Rider Minimum Transfer Damage Scale | `3f000000` / 0.5 | `3f400000` / 0.75 |
| `00164a13` | `e7b2062d` `objects\weapons\grenade\plasma_grenade\plasma_grenade` | `+000000cc` Arming Time | `3fc00000` / 1.5 | `3fa66666` / 1.299999952 |
| `00164a1b` | `e7b2062d` `objects\weapons\grenade\plasma_grenade\plasma_grenade` | `+000000d4` Timer | `3fc00000` / 1.5 | `3f666666` / 0.8999999762 |
| `00164a23` | `e7b2062d` `objects\weapons\grenade\plasma_grenade\plasma_grenade` | `+000000d8` Timer endpoint+4 | `3fc00000` / 1.5 | `3f666666` / 0.8999999762 |
| `00164a81` | `e33401bf` `objects\weapons\pistol\magnum\damage_effects\magnum_bullet` | `+0000001c` Damage Lower Bound | `41000000` / 8 | `40b00000` / 5.5 |
| `00164a86` | `e33401bf` `objects\weapons\pistol\magnum\damage_effects\magnum_bullet` | `+00000020` Damage Upper Bound | `41000000` / 8 | `40b00000` / 5.5 |
| `00164a8b` | `e33401bf` `objects\weapons\pistol\magnum\damage_effects\magnum_bullet` | `+00000024` Damage Upper Bound endpoint+4 | `41000000` / 8 | `40b00000` / 5.5 |
| `00164ab6` | `e4b2033d` `objects\weapons\rifle\plasma_rifle\plasma_rifle` | `+0000050c` Barrels[0]/Dual Wield Damage Scale | `3f400000` / 0.75 | `3f333333` / 0.6999999881 |
| `00164b11` | `e59d0428` `objects\weapons\rifle\smg\damage_effects\smg_bullet` | `+00000020` Damage Upper Bound | `40980000` / 4.75 | `40940000` / 4.625 |
| `00164b16` | `e59d0428` `objects\weapons\rifle\smg\damage_effects\smg_bullet` | `+00000024` Damage Upper Bound endpoint+4 | `40980000` / 4.75 | `40940000` / 4.625 |
| `00164b71` | `ec920b0d` `objects\weapons\support_low\brute_shot\damage_effects\shot_grenade_explosion` | `+0000001c` Damage Lower Bound | `41c80000` / 25 | `41f00000` / 30 |
| `00164b7e` | `ec920b0d` `objects\weapons\support_low\brute_shot\damage_effects\shot_grenade_explosion` | `+00000020` Damage Upper Bound | `42480000` / 50 | `42700000` / 60 |
| `00164b83` | `ec920b0d` `objects\weapons\support_low\brute_shot\damage_effects\shot_grenade_explosion` | `+00000024` Damage Upper Bound endpoint+4 | `42480000` / 50 | `42700000` / 60 |
| `00164ba1` | `e72e05b9` `objects\vehicles\h_turret_ap\h_turret_ap` | `+00000004` Bounding Radius | `3ed70a3d` / 0.4199999869 | `3f000000` / 0.5 |

## Limits and next research gate

This fixture establishes the whole combat postprocessor's result on actual raw Lockout metadata, including exact source map-type admission and reference-walk behavior. It does not establish a whole title-update simulation, earlier fixup outputs, damage application order, explosion activation, player contact/physics, any native integration, or all 1.1→1.5 differences. **No new 1.5-only combat change is claimed from this matching routine.** Other changed routines/callers/globals remain a separate investigation. No immutable prior oracle/report is changed.

Reproduce with `.tools/python/bin/python tools/oracle_mp_combat_postprocess_updates.py --output <new-directory>`, then compare the whole report, both source listings, and full original tag buffers against the frozen audit. Original assets must remain untouched.
