# King and global player-trait consumer contracts

Research snapshot: 2026-10-11. King slot35's boolean answers have concrete shared damage and camouflage consumers. This note records their original contracts and bounded execution evidence, so the native binding can preserve the actual numeric balance, timers and owner ordering. It does not admit King into normal play or complete any of the all39-variant operational goal. Native implementation and presentation validation are concurrent parent work.

The [original-only generator](../tools/oracle_king_trait_consumers.py) executes unchanged whole owners in the pinned retail, TU1.1 and TU1.5 Xbox executables. It records [the original fixture](../tests/fixtures/king_trait_consumers_original.json), [all21 complete owner instruction listings and14 complete cross-image instruction pairs](../analysis/multiplayer/game-variants/king-traits-20261011/report.json), and [literal compiled float words](../analysis/multiplayer/game-variants/king-traits-20261011/compiled-constants.json). It uses actual engine registration, the reached King/base queries and membership callback. No instruction is patched and no game/platform return is supplied.

## Inputs and executable owners

| Input | SHA256 |
| --- | --- |
| [Retail XBE](../assets/disc/default.xbe) | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| [TU1.1 XBE](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03) | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| [Owned imported TU1.5 XBE](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>) | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| [Lockout cache](../assets/disc/maps/lockout.map) | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |

| Whole original owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Damage multiplier | `15DEC0` | `15F520` | `160900` |
| Camouflage trait refresh | `15B270` | `15C8B0` | `15DC90` |
| Player powerup max grant | `1520A0` | `1537F0` | `154BD0` |
| Player powerup timer tick | `1521D0` | `153920` | `154D00` |
| Camouflage activation | `152240` | `153990` | `154D70` |
| Camouflage deactivation | `D0E00` | `D22B0` | `D3340` |
| Unit camouflage fade tick | `C6810` | `C7B70` | `C8CF0` |

All seven complete instruction shapes agree between retail/TU1.1 and TU1.5 after verified address relocation. Execution outputs agree across all three images. The generator discovers the five principal owners from unique original byte patterns and the activation/deactivation owners from actual direct calls; it checks complete original instruction shapes rather than trusting decompiler prototypes.

## Variant flags and selector mapping

The existing [callback research](MP_KING_NATIVE_CALLBACK_RESEARCH.md) establishes King slot35/+8C at retail`2BD9D0`, TU1.1`2C3D10`, TU1.5`2C7790`, with stack(player datum, selector), `ret8` and AL boolean. The signed16 player+1B8 inside timer chooses the source of the answer:

| Selector | Meaning at these consumers | Timer>0: hill flags | Timer<=0: global flags |
| --- | --- | --- | --- |
| 1 | Camouflage | options+22C bit4 | options+184 bit2 |
| 2 | Extra outgoing damage | options+22C bit2 | options+184 bit12 |
| 3 | Incoming damage resistance | options+22C bit3 | options+184 bit13 |

The variant begins at options+13C, so these are variant+F0 and variant+48 respectively. Inside the hill, a false hill bit overrides a true global bit. Negative inside timers follow the global branch. Unsupported selectors return false. TU`1615F0` forwards its two stack arguments to selected engine+8C and returns AL0 when there is no selected engine. [Native boolean kernel](../src/source_king.cpp), [literal parameter descriptors](../src/source_game_variant_descriptors.inc), [original King query](../analysis/retail/functions/002bd9d0.c), [shared fallback](../analysis/map-pack-update/functions/00253f40.c).

The relevant literal descriptor parameters are21(global camouflage, variant48/bit2),104(global damage,48/bit12),105(global resistance,48/bit13),24(friendly fire,48/bit7), and41/42/101(hill damage/resistance/camouflage,F0/bits2/3/4). Parameter103 aliases hill bit3. These descriptors describe authored/editable variant fields; the shared executable consumers below determine what those fields do.

## Damage multiplier and friendly-fire gates

TU`15F520` receives the full attacker player datum in ECX and full victim player datum in EAX, returns binary32 in XMM0 and uses plain `ret`. Its literal operation order is:

1. Start at compiled1.0. If the selected engine is absent, attacker equals victim, or attacker is NONE, return1.0 immediately. None of the remaining strength, trait, protection or membership operations execute.
2. Read the attacker's signed byte at player+C1. Raw1 selects0.75,2 selects0.5,3 selects0.25; every other byte selects1.0. The signed load and unsigned range gate mean negative byte values also take the default.
3. If the victim is NONE, return this strength scale. Both trait queries, the victim protection field and friendly-fire predicate are skipped. A player's King damage trait therefore does not automatically scale every nonplayer object request.
4. Query attacker selector2. If true, multiply by compiled1.5.
5. Query victim selector3. If true, multiply by compiled0.5.
6. If the victim's signed32 player+194 is positive, multiply by compiled0.1. This note does not establish that field's producer or assign it an unsupported semantic name.
7. If global variant+48 bit7 is set, return the accumulated scale.
8. Otherwise invoke the actual selected engine slot27/+6C membership predicate through TU`15F680`. Its arguments are the signed low16 membership values produced from attacker/victim signed player+C0 bytes. King/shared TU`253F10` compares those two shorts and returns AL1 for different values, `ret8`. A false result returns0; a true result retains the accumulated scale.

The separate selected-engine team-mode query TU`160150` reads options+184 bit0 (variant+48 bit0), but the damage predicate introduces no additional team-mode gate. The [source inspection generator](../tools/inspect_king_trait_consumers.py) saves actual registered +6C pointers, complete membership/dispatcher/team-query instructions in all three images, and24 whole team-query calls over engine present/absent and flags0/1/128/129. All three images agree. Native FFA currently assigns distinct memberships by player slot; team play uses the selected team. Equal memberships with friendly fire disabled produce zero even when the declared configuration otherwise says FFA. Complete original instructions, not the decompiler's incomplete floating-point prototype, are the numeric authority. [TU damage owner](../analysis/map-pack-update/functions/0015f520.c), [query wrapper](../analysis/map-pack-update/functions/001615f0.c), [membership dispatcher](../analysis/map-pack-update/functions/0015f680.c), [actual registered King/shared predicate](../analysis/multiplayer/game-variants/king-traits-20261011/team-friendly-source-inspection.json), [complete executed instructions](../analysis/multiplayer/game-variants/king-traits-20261011/report.json).

| TU1.1 compiled address | Little-endian bytes | Binary32 value |
| --- | --- | --- |
| `465030` | `0000803f` | 1 |
| `46509C` | `0000403f` | 0.75 |
| `46502C` | `0000003f` | 0.5 |
| `465090` | `0000803e` | 0.25 |
| `446548` | `0000c03f` | 1.5 |
| `4650D8` | `cdcccc3d` | 0.10000000149011612 |
| `4650D4` | `abaaaa3e` | 0.3333333432674408 |

Each multiplication is the literal ordered SSE `mulss` with the owner's intermediate binary32 stores. With default raw strength, extra damage plus resistance yields0.75 before the remaining gates. The values are compiled executable behavior; weapon damage ranges, damage groups, material/armor factors and weapon camouflage disruption parameters remain tag data.

The original damage caller TU`DB160` resolves the target object with whole`D90E0`, places damage-event attacker player+8 into ECX, then calls`15F520` at`DB2B8`. It multiplies the computed authored/event request by the returned value at`DB2C7`. Its zero-scale branch clears damage-event byte+68. Target resolver`D90E0` validates object kind/salt, obtains a controlling player when appropriate and follows actual parent objects; the new multiplier corpus supplies resolved player datums and does not execute this complete target-identity chain. [Original caller](../analysis/map-pack-update/functions/000db160.c), [target resolver](../analysis/map-pack-update/functions/000d90e0.c).

The existing native `damage_amount` computes `(lower*(1-fraction) + selected_upper*fraction) * event_scale * multiplayer_scale` with its established arithmetic. The player multiplier belongs before the shared vitality/material kernels. It is neither the material/armor multiplier nor a post-death score hook. Random upper-bound selection remains its own original owner.

## Camouflage refresh, timers and fades

**FP verification correction:** the earlier generator's setup requested four control words, but inherited `Source.call` reset FPCW to027F and MXCSR to1F80 before execution. Its four labeled cohorts therefore all ran nearest rounding. The old fixture/report remain unchanged; they do not prove alternate rounding modes. The [corrected preserved-control oracle](../tools/oracle_camouflage_fp.py) and [new fixture](../tests/fixtures/camouflage_state_original.json) observe actual FPCW at every reached original FISTP and its produced integer. At31Hz the corrected027F/067F/0A7F/0E7F cases produce16/15/16/15 respectively. [Corrected complete source/execution report](../analysis/multiplayer/game-variants/camouflage-state-20261011/report.json).

TU`15C8B0` receives player datum in ESI and uses plain `ret`. It queries selected engine+8C with selector1. False, missing engine, or current-unit NONE causes no writes. True with a current unit computes signed16 clock tick rate times compiled0.5, stores binary32, then uses x87 `fistp` under the actual floating-point control word. It calls whole`1537F0` with EAX player, EDX0 (powerup index) and DI converted low16 ticks. At30Hz this grants15 ticks. At31Hz the15.5 conversion yields16 with round-to-nearest/up and15 with down/truncate; replacing it with an unconditional integer division changes source behavior. [Refresh owner](../analysis/map-pack-update/functions/0015c8b0.c), [complete instruction evidence](../analysis/multiplayer/game-variants/king-traits-20261011/report.json).

`1537F0` accesses signed16 timer player+198+2*index. If its old value is exactly0 it calls activation before storing the signed16 maximum of old timer and DI. A negative nonzero timer does not trigger activation. A refresh of an already nonzero timer does not reactivate a cleared unit flag. [Grant owner](../analysis/map-pack-update/functions/001537f0.c).

For index0, activation`153990` resolves the current unit from the actual object header, ORs unit+134 bit3 and writes compiled0.25 to unit+2B8. It preserves other flags and does not write unit+2B0, unit+2B4 or bit4. When actual unit replication entity+0D4 is not NONE it invokes dirty owner`B6760(entity,800000)`. [Activation owner](../analysis/map-pack-update/functions/00153990.c).

The separate player powerup tick`153920` receives player in EAX and uses plain `ret`. It examines exactly two signed16 timers, player+198 and+19A. Positive values decrement by1; nonpositive values stay unchanged. When index0 reaches0 it calls deactivation`D22B0` with the actual current unit in EAX and XMM0=compiled1.0. Expiry of index1 does not call that owner. [Timer owner](../analysis/map-pack-update/functions/00153920.c).

Deactivation clears unit+134 bits3 and4, preserving every other flag, and sets unit+2B8 to supplied XMM0 times0.25. Timer expiry therefore sets fade speed0.25. It invokes the same dirty entity/800000 owner when an entity exists. It does not instantly zero either fade amount. [Deactivation owner](../analysis/map-pack-update/functions/000d22b0.c).

Unit tick`C7B70` receives unit in EAX and owns two gradual amounts:

| Unit field | Update in measured multiplayer branch |
| --- | --- |
| +134 bit3 | Selects primary fade direction |
| +2B0 | Add +2B8*clock_dt while bit3 is set, cap at1; otherwise subtract it, floor at0 |
| +134 bit4 | Selects secondary fade direction independently |
| +2B4 | Add compiled0.3333333432674408*clock_dt while bit4 is set, cap at1; otherwise subtract it, floor at0 |
| +2B8 | Primary fade speed installed by activation/deactivation and other real controllers |

These are directional endpoint clamps, not unconditional initial-value sanitization: for example an already out-of-range value moving toward the other endpoint is handled by the actual arithmetic and branch. The whole owner also has a campaign actor/debug override, outside this multiplayer actor-NONE corpus. Reaching a primary endpoint can mark replication dirty when an entity exists. [Unit fade owner](../analysis/map-pack-update/functions/000c7b70.c), [complete cross-image instruction listings](../analysis/multiplayer/game-variants/king-traits-20261011/report.json).

Activation at0.25 speed gradually raises the primary amount; retaining the15-tick timer at30Hz supplies a half-second grant rather than an instantaneous hill-exit toggle. These individual-owner observations do not establish exact natural world frame counts across player/world update scheduling.

## Original owner order and additional camouflage consumers

The original shared pre-world player loop TU`15D7C0` calls `15C8B0` camouflage refresh, `15D160`, `1635D0`, selected engine slot15/+3C (King occupancy), then `1640D0`. Thus camouflage refresh reads the retained inside timer before the current occupancy update. The current native `MultiplayerMatch::update_game_engine` starts with occupancy; simply adding refresh after occupancy would change this order. Separate player update`14EFF0` calls`153920` when the player has a current unit; the actual object update owns`C7B70`. The new corpus executes each of those consumer owners, not the complete caller/world sequence. [Pre-world caller](../analysis/map-pack-update/functions/0015d7c0.c), [player update caller](../analysis/map-pack-update/functions/0014eff0.c), [native King orchestration](../src/multiplayer.cpp).

Other inspected consumers remain necessary for complete behavior:

- TU`D2310` (retail`D0E60`) reduces unit+2B0 by its supplied amount and caps +2B8 to its supplied limit, using0.25 when the limit is0. Its primary lower bound is0.05 while bit3 is set. If unit+2BC is nonzero it also delegates to intrinsic suit-camouflage owner`C6AA0(0)`.
- Damage feedback`CB410` resolves the jpt! definition from result+8, then calls`D2310(jpt_definition+30,0)` only when actual result body+44 or shield+48 is positive; weapon owner`107160` supplies weapon-definition+27C and+280 after accepted firing and a controlling player exists; `E8F30` calls it with0,0. These disruption amounts are authored tag fields. The older result+30 description was incorrect; the original complete caller is authoritative.
- TU`161510` can cap a current player's unit primary amount at0.5 while bit3 is set and then mark replication. Its upstream caller contract remains unresolved here.
- Unit+2B0 reaches original weapon-parent rendering, shadows and model shader context. Native renderer helpers already accept resolved camouflage context; they still require the actual player/unit fade state and parent mapping.
- Intrinsic suit state at unit+2BC/+2BE and its`C6AA0` controller is separate from the global/hill timer. They cannot be conflated into one generic camouflage boolean.

These additional consumers were inspected in the original listings/decompile but are outside the new whole-owner executable corpus. [Disruption owner](../analysis/map-pack-update/functions/000d2310.c), [damage caller](../analysis/map-pack-update/functions/000cb410.c), [weapon caller](../analysis/map-pack-update/functions/00107160.c), [other caller](../analysis/map-pack-update/functions/000e8f30.c), [current-player cap](../analysis/map-pack-update/functions/00161510.c), [intrinsic controller](../analysis/map-pack-update/functions/000c6aa0.c).

## Native integration requirements

The read-only native inspection found these concrete missing bindings:

- `MultiplayerPlayer` preserves current full player/unit datums, membership, flags and King inside timer, but does not yet preserve the raw strength byte+C1, positive-check field+194, or both powerup timers+198/+19A. Their real initialization/reset/configuration producers must be retained; declaring raw bytes in the oracle does not establish their live defaults or property import.
- `Unit` lacks the actual camouflage flag bits and primary/secondary/speed state above. Unit allocation/reset, refresh before occupancy, player timer tick, unit fade tick and disruption consumers require their own source owners and correct lifetime/order.
- `Simulation::damage` passes multiplayer_scale1 for its supported default SMG authored request. `Simulation::area_damage` and the prop damage paths also use1. Supplying an amount bypasses that default SMG computation. Each route needs the shared resolved attacker/victim multiplier at the correct request stage. Native `damage_hook_` runs after `apply_unit_damage`; it cannot faithfully perform this scaling after vitality has changed.
- The source damage owner can identify a nonplayer target through parent/controller resolution; a convenient native victim pointer alone does not establish that source identity contract. Preserve `DamageOwner` full captured player/unit/designator and the original target resolver behavior for shots, grenades and props.
- Material/armor factors in `unit_damage.cpp` remain separate. Render camouflage input, actual replication dirty calls and native snapshot/LAN ownership must consume the actual state when those normal flows are implemented.

[Native player/unit state](../include/halo2/multiplayer.hpp), [native unit/damage ownership](../include/halo2/simulation.hpp), [damage and area consumers](../src/simulation.cpp), [material kernels](../src/unit_damage.cpp), [native orchestration](../src/multiplayer.cpp). This inspection is a snapshot; it does not validate concurrent native changes. King normal startup and transport remain gated until the real damage, camouflage, rendering and snapshot consumers have normal-flow validation.

## Executed coverage and reproduction

The fixture has1,668 damage inputs and1,608 camouflage inputs, each run in all three images: **5,004 whole damage-owner calls and4,824 whole camouflage-owner calls**. All normalized outputs agree across the three images. Fourteen complete instruction-shape comparisons pass.

Damage inputs include eight strength bytes0/1/2/3/4/127/128/255; independent inside/outside attacker/victim states, hill/global damage/resistance bits and positive/zero protection; missing-engine, attacker-NONE, victim-NONE and self gates; 72 explicit cases over the real variant+48 team bit0, friendly-fire bit7, independently declared attacker/victim memberships-1/0/7 and global traits off/on; and512 deterministic cases including negative signed timers, equal/different memberships, friendly-fire flags and signed32 protection boundaries. The explicit cases execute the actual selected King predicate when bit7 is clear and preserve each declared NONE/same/different membership input. These consumer calls do not prove live team allocation, profile sanitization or natural friendly-fire gameplay. The oracle asserts all16 player records and options remain unchanged.

Camouflage inputs include960 refresh calls with four requested x87 control-word labels027F/067F/0A7F/0E7F, rates30/31/60, timer values-1/0/1/15/32767, independent hill/global flags, inside/outside and current-unit NONE/present;72 timer calls including signed boundaries and independent secondary timers; and576 fade calls spanning both flag bits, initial primary/secondary amounts, fade speeds and30/60Hz dt. The earlier call wrapper actually executes all of them under027F/1F80, as corrected above. It verifies all unowned player/unit bytes remain unchanged and records the actually observed selector calls. Alternate-rounding proof belongs to the new preserved-control fixture.

The fixture declares current units/object headers, clock/FP state and original player/options records. Unit replication entity+0D4 and actor+12C are actual NONE inputs, so those branches return naturally without substituted dirty/actor service returns. The execution proves the reached original arithmetic, flags and guards for this stated context; it does not prove allocated live player defaults, campaign overrides, dirty-entity replication, complete parent target resolution, powerup pickup, normal firing/damage/render sequences, populated matches or original transport.

Reproduce with:

```sh
.tools/python/bin/python tools/oracle_king_trait_consumers.py
.tools/python/bin/python tools/inspect_king_trait_consumers.py
.tools/python/bin/python tools/oracle_king_trait_consumers.py --fixture analysis/multiplayer/game-variants/king-traits-20261011/reproduced-fixture.json --output analysis/multiplayer/game-variants/king-traits-20261011/reproduction/report.json
```

Fixture SHA256: `29d80e3df288b7d6732ead10805ca6c6d21d8321be8efaa0f33b6f0b34aa2102`. The [independent second-run record](../analysis/multiplayer/game-variants/king-traits-20261011/reproduction.json) records exact fixture/report byte equality and current source/dependency hashes.
