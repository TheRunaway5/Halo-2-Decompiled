# Original player inputs and target identity for multiplayer damage

Research snapshot: 2026-10-11. The minimum original contract is: player strength comes from the sanitized published configuration; player+194 is zero at birth, receives the signed tick rate on successful spawn, and decrements while positive; damage target identity uses the current controlling player at unit+13C, never the previous-player field+140. An authored unparented nonunit prop resolves to NONE. These facts support the bounded native player-unit/unparented-prop binding; they do not establish every retained lobby property, parented object, vehicle or natural complete combat flow.

Primary new evidence is the [original-only generator](../tools/oracle_damage_player_inputs.py), [fixture](../tests/fixtures/damage_player_inputs_original.json) and [complete instruction/execution report](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/report.json). The generator executes whole unchanged owners in all three pinned Xbox images, with no patched instructions or supplied game/platform returns. Creation identity, requested configuration, backed player/object storage and clock are declared inputs.

## Authority and whole owners

| Input | SHA256 |
| --- | --- |
| [Retail XBE](../assets/disc/default.xbe) | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| [TU1.1 XBE](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03) | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| [Owned imported TU1.5 XBE](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>) | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| [Lockout cache](../assets/disc/maps/lockout.map) | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |

| Original owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Allocated player birth | `14C090` | `14D800` | `14EBE0` |
| Apply requested configuration | `14BE90` | `14D600` | `14E9E0` |
| Configuration sanitizer | `14BC80` | `14D3D0` | `14E7B0` |
| Default configuration producer | `1537F0` | `154F40` | `156320` |
| Complete player spawn | `14F2E0` | `1509C0` | `151DA0` |
| Shared pre-world player update | `15BB20` | `15D160` | `15E540` |

The report retains the complete bodies of all these owners and the target resolver/getter below. Eleven of16 complete earlier-to-TU1.5 comparisons are equal. It explicitly retains the five changed bodies: retail configuration, sanitizer, spawn and pre-world update, and TU1.1 spawn. It does not replace changed bodies with a blanket equivalence claim. Damage-owned outputs agree for every stated execution input. Retail retains the supplied multiplayer representation while the updates map default representation0 to2; the complete birth record comparison finds that single difference at player+88. [Complete bodies, comparison flags and changed produced bytes](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/report.json), [existing representation policy research](MP_PLAYER_VISIBILITY_RESEARCH.md).

## Strength: property producer, signed sanitation and full birth

TU`14D600` receives the requested90-byte configuration pointer in EAX and player datum on the stack, `ret4`. It copies the full request into player+D4, copies it into its local working packet, invokes the actual sanitizer, then copies the full effective packet to player+44. Configuration+7D consequently becomes player+C1. The requested copy remains raw; the effective copy is the actual damage input. [Original application](../analysis/map-pack-update/functions/0014d600.c), [complete executed body and records](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/report.json).

Sanitizer`14D3D0` interprets configuration+7D as a signed byte and clamps it to0..3. Negative values become0, values above3 become3, otherwise it preserves the byte. Its branch is unchanged in the two update images; the complete retail sanitizer differs elsewhere and produces the same strength clamp for this context. [Original sanitizer](../analysis/map-pack-update/functions/0014d3d0.c), [all256 original byte inputs in all three images](../tests/fixtures/damage_player_inputs_original.json).

The new oracle poisons the entire declared allocated player row with A5 except its salt, generates the actual default packet with the whole original default-configuration owner, and supplies a nonlocal creation record with no matching local machine slot, user/controller NONE and the packet at creation+1C. Whole player birth calls the actual configuration and sanitizer owners and returns normally. For all256 requested strength bytes in each image, player+194 becomes0 and player+C1 equals the signed clamp. This proves the owner writes rather than relying on a zeroed native player. It does not execute the allocator, sign-in, activation, lobby or transport producer. [Generator](../tools/oracle_damage_player_inputs.py), [complete birth owner](../analysis/map-pack-update/functions/0014d800.c), [whole execution records](../tests/fixtures/damage_player_inputs_original.json).

| Requested configuration+7D | Effective player+C1 | Damage strength scale from the shared consumer |
| --- | --- | --- |
| Actual default, or explicit0 | 0 | 1 |
| 1 | 1 | 0.75 |
| 2 | 2 | 0.5 |
| 3..127 | 3 | 0.25 |
| 128..255, interpreted negative | 0 | 1 |

The default configuration producer sets+7D to0. A live requested value4 therefore becomes effective3 and scales0.25; passing raw4 directly to the damage consumer would instead take its raw default1 branch. The native binding must use its actual sanitized configuration. [Original byte corpus](../tests/fixtures/damage_player_inputs_original.json), [damage consumer order and compiled scales](MP_KING_TRAIT_CONSUMERS_SOURCE_RESEARCH.md), [existing native source sanitizer](../src/source_player_profile.cpp).

The upstream local property is controller handicap storage+200, copied to published configuration+7D by TU`1917C6`; the actual local reset producer clears that controller field. Host property assignment copies+7D independently of its membership assignment. Those prior source paths are documented with explicit controller reset, property delivery and prefix boundaries in [the earlier strength research](MP_SMG_EVENT_STRENGTH_RESEARCH.md), with all three original offline conversion/default/sanitizer owners covered by [the profile oracle](../tools/oracle_player_profile.py). A local reset cannot establish every remote or retained lobby player's handicap. Existing native profile requests and sanitizer outputs carry the required input; silently forcing all players to0 would discard authored/published state.

With a current unit, `14D600` additionally invokes real membership/strength change callbacks and unit appearance work. TU`159A70` can alter the current unit's damage state when handicap changes. The new exhaustive birth cases naturally have current-unit NONE and do not prove those live configuration-change branches. Cold join input and live rebinding are separate contracts. [Strength-change callback](../analysis/map-pack-update/functions/00159a70.c), [configuration caller](../analysis/map-pack-update/functions/0014d600.c).

## Player+194: birth zero, spawn grant and unconditional countdown

Whole TU`14D800` explicitly writes dword player+194=0 at`14D96A`. Whole successful spawn`1509C0` reads signed16 clock+2 and writes its sign-extended value to player+194 at`150D6E`, after resetting player powerup timers and before final local/UI/presentation work. The source store is in the common successful continuation, reached by ordinary spawn and the mode4 bypass branch. Failed ordinary spawn returns before this continuation. [Birth owner](../analysis/map-pack-update/functions/0014d800.c), [complete spawn owner](../analysis/map-pack-update/functions/001509c0.c), [all complete original listings and actual store observations](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/report.json).

The new oracle executes the complete spawn owner in mode4, game type2, user NONE and current-unit NONE. That original branch skips placement/allocation, reaches the success continuation and returns1 without supplied returns. It verifies rates-32768,-1,0,1,15,30,31,60,32767 in all three images. The signed value is preserved:30 produces dword30, negative rates produce their sign-extended negative dwords. **This is an explicit mode4 consumer probe, not new ordinary populated spawn evidence.** [Declared input boundary and timeline fixture](../tests/fixtures/damage_player_inputs_original.json).

Separately, the pinned earlier TU1.1 ordinary-spawn report contains two complete original normal-spawn returns and the actual`150D6E` store of0000001E into player+194 on both. The new report re-reads that prior report with SHA256`f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd`, extracts the exact stores and lists its supplied lower-owner service entries. That earlier environment includes explicitly supplied resource/audio/attachment/other services; it is not reclassified as new no-service execution or a complete natural game flow. [Pinned earlier report](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json), [prior scope](MP_PLAYER_VISIBILITY_RESEARCH.md), [new bounded extraction](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/report.json).

Shared TU`15D160(player)` examines signed32 player+194 at entry. Positive values decrement by1; zero and negative values are unchanged. This occurs before any current-unit, recording, local-user or input gate. The new whole-owner timelines execute with current-unit/user NONE and show30,29,...,1,0,0; the60Hz case likewise reaches0 after60 decrements. The rate32767 case verifies its first two decrements only. [Original player owner](../analysis/map-pack-update/functions/0015d160.c), [timelines](../tests/fixtures/damage_player_inputs_original.json).

The original shared pre-world player loop calls camouflage refresh, then`15D160`, then other shared owners, then selected engine slot15 (King occupancy), before world-object movement. Thus the +194 countdown belongs in the shared player phase, including Slayer, before current tick's object damage. The complete natural spawn/update/firing frame alignment is outside the new isolated-owner corpus. [Pre-world order](../analysis/map-pack-update/functions/0015d7c0.c), [related trait ordering](MP_KING_TRAIT_CONSUMERS_SOURCE_RESEARCH.md).

The shared damage multiplier checks victim+194>0 and multiplies by compiled0.1 after strength and both trait scales, before friendly-fire membership filtering. It does not make a player wholly invulnerable. Native permanent0 would omit this actual post-spawn reduction. No tag supplies this compiled0.1 or the clock-rate grant/countdown behavior. [Original damage owner](../analysis/map-pack-update/functions/0015f520.c), [numeric source contracts](MP_KING_TRAIT_CONSUMERS_SOURCE_RESEARCH.md).

## Target resolver: controlling player, parent walk and limits

The complete source resolver is discovered independently in each pinned image from its unique original body prefix; its actual called object getter is recovered from the direct call. It takes a full object datum in EAX, returns a full player datum or NONE in EAX and uses plain `ret`. The report records all three exact addresses and complete bodies, and all four earlier-to-TU1.5 resolver/getter comparisons agree. [Original TU resolver](../analysis/map-pack-update/functions/000d90e0.c), [actual mask3 getter](../analysis/map-pack-update/functions/000bbcb0.c), [discovered owners and complete instructions](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/report.json).

For each object in the chain:

1. NONE returns NONE immediately.
2. The resolver checks low16 slot against pool high-water+38, obtains the header using pool stride+24 and data base+44, checks nonzero signed salt against the datum's signed high16, checks `(1 << (header_kind &31)) &3`, and requires a nonnull body pointer. Valid kinds0/1 are the unit mask accepted here.
3. On that valid unit branch it calls the actual checked getter with mask3 and returns body+13C, or NONE if that getter returns null. The return is immediate even when the controlling-player field itself is NONE.
4. If classification fails, it reads the raw object's parent at body+14 through the object-header table and repeats. This fallback does not itself perform a safe checked-body lookup.

The returned player field is not validated against the player pool by this resolver. The independent cases preserve a declared foreign full player value`1234000F` exactly. Player identity admission/validity remains a separate caller/lifetime responsibility; a native mapping must not convert a non-NONE full datum into a slot-only identity. [Whole resolved values and input records](../tests/fixtures/damage_player_inputs_original.json), [complete original instructions](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/report.json).

| Bounded target context | Original result |
| --- | --- |
| Current player biped, valid unit header | Its exact unit+13C controlling player |
| Valid unit with current player NONE, previous player+140 present | NONE; previous player is ignored |
| Valid unit with current player NONE and a parent unit | NONE; no parent walk after the valid-unit return |
| Authored unparented nonunit prop | NONE |
| Nonunit prop with a valid unit ancestor | That ancestor's unit+13C |
| Backed stale/zero-salt or out-of-range header with parent NONE | NONE through the declared raw parent fallback |

The last two rows describe declared backed original inputs, not proof of live parent producers or safe arbitrary-invalid-handle handling. The source's raw fallback can access a null/invalid body, and a cyclic parent chain can fail to terminate. Such contexts are not supplied successful results by the oracle. Native current player units and actual unparented props need only the first four bounded mappings. [Resolver corpus and boundary](../tools/oracle_damage_player_inputs.py), [fixture](../tests/fixtures/damage_player_inputs_original.json).

The resolver does not read unit+140, membership, source shot-owner player, damage-history owner or a prop's tag-level identity as a substitute for+13C. The attacker still comes from the damage event's captured player field; victim target resolution is separate. A corpse whose current player has been cleared can still retain previous-player attribution, but its damage multiplier target is NONE. [Original resolver](../analysis/map-pack-update/functions/000d90e0.c), [caller before multiplayer scaling](../analysis/map-pack-update/functions/000db160.c), [native current/previous unit fields](../include/halo2/simulation.hpp).

## Captured attacker after leave and same-slot reuse

**The reached original chain does not normalize a captured stale attacker to NONE before the multiplayer damage multiplier.** TU`BD080` raw-copies projectile+C4 into event+08 and projectile+C8 into event+0C. The original direct-impact producer calls this owner before`D9180`. `D9180` loads the actual damage tag and calls`DB160` before its later history/attribution work. In`DB160`, the selected-engine branch resolves the target through`D90E0`, then at`DB2B5` loads ECX directly from event+08 and at`DB2B8` calls`15F520`. No checked player lookup or salt test intervenes. [Actual owner copy and direct-impact evidence](MP_SMG_EVENT_STRENGTH_RESEARCH.md), [original damage caller](../analysis/map-pack-update/functions/000d9180.c), [whole amount owner](../analysis/map-pack-update/functions/000db160.c), [complete new instruction/execution report](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/report.json).

The new [stale-attacker generator](../tools/oracle_damage_stale_attacker.py) executes486 complete original amount calls across retail, TU1.1 and TU1.5. All calls use the actual Lockout stock SMG damage tag, amount4.75, declared unit headers/bodies and selected King engine. Its inputs cover old/new full attacker salts at the same slot and NONE; captured unit NONE, a valid actorless corpse and a mismatched-salt corpse handle; friendly-fire enabled/disabled, actual global team bit0, same/different/NONE memberships, retained positive hill time and its hill/global trait selection. For every case, old/new attacker salts read the same raw slot bytes; captured-unit failure does not clear the attacker. NONE follows the consumer's existing early1.0 multiplier. All player bytes remain unchanged by each amount call. The additional54 original`D9180` prefixes stop at their actual`DB160` call entry and observe the exact unchanged event+08/+0C bytes. These prefixes supply no return and do not claim complete later history/attribution execution. [Declared inputs, complete outputs and call-entry observations](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/report.json).

| Original owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Captured projectile owner copy | `BC190` | `BD080` | `BE1C0` |
| Direct damage caller | `D7B80` | `D9180` | `DA0D0` |
| Complete damage amount | `D9B60` | `DB160` | `DC0B0` |
| Complete player leave | `14C540` | `14DCB0` | `14F090` |
| Requested slot allocation | `16B990` | `16D410` | `16E7F0` |
| Record clear/new salt | `16BA00` | `16D480` | `16E860` |
| Record release | `16BA40` | `16D4C0` | `16E8A0` |

The generator independently discovers these owners from unique actual prefixes and derives release/clear targets from the original callers. It retains full bodies, together with the target resolver/getter and multiplier. All20 earlier-to-TU1.5 complete body comparisons agree. This does not extend the earlier changed birth/spawn bodies' equivalence beyond their recorded comparisons. [Source addresses, full instruction inventories and relocated operand comparisons](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/report.json).

### Leave retains the numeric slot inputs

TU`14DCB0` performs actual unit detach, local user/controller cleanup when bound, actual`16D4C0` record release, selected-engine notification/recording gates and actual`BC7E0` object callbacks. With the actual player pool's default flags0004, release clears the occupancy bitmap and record salt and updates pool counters/cursor/high-water. It leaves the remaining record bytes intact. The optional pool poison flag bit3 takes a separate BA fill branch; that flag is absent in the actual default player initializer. **Zeroing the full native player record on leave changes the source damage behavior.** [Original leave](../analysis/map-pack-update/functions/0014dcb0.c), [original release](../analysis/map-pack-update/functions/0016d4c0.c), [actual initializer and callback ownership evidence](MP_DATUM_OWNERSHIP_RESEARCH.md), [new whole leave records](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/report.json).

| Field | Seeded before complete leave | After complete leave |
| --- | --- | --- |
| Salt | `9ABC` in the all-image probe; actual `EC70` in the TU callback lifecycle | `0000` |
| Membership+C0 | `07` | `07` |
| Strength+C1 | `02` | `02` |
| Spawn countdown+194 | `1D000000` (29) | `1D000000` |
| King hill timer+1B8 | `1100` (17) | `1100` |

The18 all-image leave probes use current unit/user/controller NONE and an empty object pool at leave, with no supplied returns. Separately, six complete TU leave calls reuse the existing original object-callback lifecycle for slots0,3,15, with retained and deleted old corpses. These cases start from actual original player allocation/join and actual unit binding, seed the listed valid live numeric values before leave, and retain all four fields after the complete leave and real type callbacks. The original biped callback clears its matching player-removed field, while the projectile retains its captured full old player/unit through leave and new-salt same-slot rejoin. These six contexts retain the older oracle's explicit body-storage service, constructor/death prefixes and optional control-notification boundary; no supplied service executes inside the complete leave owner. [New raw player records, real object callbacks and captured projectile bytes](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/report.json), [prior lifecycle boundary](MP_DATUM_OWNERSHIP_RESEARCH.md).

### Reuse clears storage, then real producers establish new state

Whole`16D410` requested allocation reaches actual`16D480`, clears the entire stride and assigns a new salt. The three-image probe observes C0,C1,+194,+1B8 all zero immediately after that owner. The following whole birth consumes the new configuration: effective strength3, +194=0 and +1B8=0 in this birth-only context. Membership is0 with the actual default FFA packet and-1 with that default packet in team mode. The separate complete TU join additionally invokes its real selected-engine/player initialization; its Slayer callback lifecycle produces +1B8=-1 after rejoin. Thus zero after allocation/birth is not a universal final initialized hill-timer value. Preserve the selected mode's actual initialization. [Actual allocation and birth records](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/report.json), [join wrapper](../analysis/map-pack-update/functions/0014d350.c), [shared player initialization](../analysis/map-pack-update/functions/00159280.c).

One source execution makes the behavior concrete: with friendly fire enabled and hill damage selected, the seeded C1=2 and positive hill timer produce stock SMG amount3.5625 before leave and3.5625 after leave, despite zero salt. After that slot receives the new strength3 configuration and its birth-only hill timer0, the **unchanged old full attacker datum** produces1.1875. It reads the new slot values rather than retained old values or a NONE substitute. With global damage selected but a false hill override, a separate case changes2.375 before/after leave to1.78125 after reuse because the new zero hill timer selects global damage. [Actual float bits and multiplier-entry field snapshots](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/report.json).

The native damage service therefore needs a bounded raw low16 player-field projection for this specific original consumer, including a released but still backed slot and a reused slot. Keep current full-datum validation for attribution, statistics and ordinary identity lookup. Preserve the four live numeric fields on leave until allocation/actual initialization replaces them; do not normalize captured old player data to NONE. Original exact full-datum self/NONE early returns still precede raw reads. Arbitrary unbacked/out-of-range player slots, local-user/controller removal, actor-owned or driver-redirection damage and complete natural grenade scheduling are outside the new no-service corpus. The earlier bounded firing/impact/lifecycle source supports captured ownership, but this is not a new complete weapon/world/transport proof. [Shared multiplier contract](MP_KING_TRAIT_CONSUMERS_SOURCE_RESEARCH.md), [new generator boundary](../tools/oracle_damage_stale_attacker.py).

## Coverage, reproduction and native requirements

New execution totals are795 whole birth calls (768 exhaustive strength cases plus27 timeline preparations),27 whole mode4 spawn calls,465 whole shared player updates and408 whole target resolver calls. Whole birth transitively executes its real configuration/sanitizer. All three images agree on the damage-owned birth fields, complete numeric timelines and resolved full player datums. Complete records retain the retail representation difference rather than masking it away. The408 resolver calls cover NONE; ten source kind bytes; current controlling player NONE, valid and declared foreign; previous-player NONE/present; parent NONE/unit; zero/mismatched salt; backed header/high-water mismatch; and one/two/three nonunit parent links. [Fixture and report](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/report.json).

The native player join can use the existing source sanitizer's effective+7D. The default-only join must use the actual source default packet rather than an unrelated struct zero. Birth should preserve +194=0, successful spawn should grant signed tick rate, and the shared pre-world player phase should decrement it while positive. Damage target resolution for currently supported units must use current source_player and stop on a valid unit even if NONE; unparented props use NONE. Parent traversal requires actual source parent/kind/identity ownership when those additional contexts are admitted. These are source contracts, not a validation claim for concurrent parent implementation. [Existing native property producer/sanitizer](../src/source_player_profile.cpp), [native match join/spawn/update](../src/multiplayer.cpp), [unit ownership fields](../include/halo2/simulation.hpp).

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_damage_player_inputs.py
.tools/python/bin/python tools/oracle_damage_player_inputs.py --fixture analysis/multiplayer/game-variants/damage-player-inputs-20261011/reproduced-fixture.json --output analysis/multiplayer/game-variants/damage-player-inputs-20261011/reproduction/report.json
```

Fixture SHA256: `26161a6d225572bcb4ead25c305e1ed13fa357c29730fab21b32394eeaaa104d`. The [independent second-run record](../analysis/multiplayer/game-variants/damage-player-inputs-20261011/reproduction.json) records exact fixture/report byte equality and the preserved source/dependency hashes. Ordinary live spawning, changed-property notifications on existing units, every parent producer and full combat/transport remain additional validation requirements; the active all39-variant goal is not completed by this research.

Reproduce the additional retained-attacker evidence:

```sh
.tools/python/bin/python tools/oracle_damage_stale_attacker.py
.tools/python/bin/python tools/oracle_damage_stale_attacker.py --output analysis/multiplayer/game-variants/damage-stale-attacker-20261011/reproduction/report.json
```

Its [independent reproduction record](../analysis/multiplayer/game-variants/damage-stale-attacker-20261011/reproduction.json) records byte equality for the complete report, which contains all raw inputs, outputs, original instructions, image/cache pins and Python source dependency hashes. The earlier cold-input fixture remains unchanged.
