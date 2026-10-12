# Original King callback contracts for native integration

Research snapshot: 2026-10-11. This note follows the registered source type4 engine in the unchanged retail, TU1.1 and TU1.5 Xbox executables. It supplies original callback evidence and original-only test inputs/outputs. It does not admit King in the frontend, establish a playable populated match, or complete the all39-variant goal.

The actual executable pins and geometry/data contracts are recorded in [the earlier King source note](MP_KING_ENGINE_SOURCE_RESEARCH.md). The primary new evidence is [the original-only generator](../tools/oracle_king_callbacks.py), [its complete instruction-listing report](../analysis/multiplayer/game-variants/king-callbacks-20261011/report.json) and [the original fixture](../tests/fixtures/king_callbacks_original.json). The generator imports the pinned original images, executes their real registrations, checks the King callback pointers, and executes every reached owner without instruction patches or supplied game/service returns.

| Original input | SHA256 |
| --- | --- |
| [Retail XBE](../assets/disc/default.xbe) | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| [TU1.1 XBE](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03) | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| [Owned imported TU1.5 XBE](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>) | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| [Lockout cache](../assets/disc/maps/lockout.map) | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |

The report records these input pins, generator hash and seven harness dependency hashes as the reproduction manifest. It also contains each complete original listing rather than relying on decompiler signatures.

## Registered owners and lifecycle order

| Literal callback/owner | Retail | TU1.1 | TU1.5 | Contract |
| --- | --- | --- | --- | --- |
| King slot1 / +04 initialization | `2BD140` | `2C34D0` | `2C6F50` | Bind multiplayer globals+FC; clear1DC state bytes; discover source-order groups; initialize original default points. |
| King slot3 / +0C round initialization | `2BD200` | `2C3590` | `2C7010` | No explicit arguments; AL1; selected hill0; countdown=signed16 options+230 times signed16 clock+2; call whole builder(state,0). |
| Shared round preparation | `157AE0` | `159180` | `15A560` | Prepare shared state/reset providers, then dispatch engine+0C. |
| Shared phase transition | `15C550` | `15DBC0` | `15EFA0` | On changed status, perform shared transition work, dispatch engine+90(status), then copy status to signed16 lifecycle+6C. |
| King slot36 / +90 phase callback | `72C70` | `27E760` | `2E4D60` | Exactly `ret4`; no King stores or calls. |
| King slot35 / +8C trait query | `2BD9D0` | `2C3D10` | `2C7790` | Stack(player datum, trait kind), `ret8`, AL boolean. |
| Shared trait fallback | `24DDA0` | `253F40` | `25BCC0` | Same stack contract; global option flags. |
| King slot30 / +78 death statistics | `2BDA60` | `2C3DA0` | `2C7820` | Four stack arguments; `ret10`; first attacker, second victim, third low byte betrayal; fourth ignored. |
| King slot32 / +80 spawn influence | `2BDB00` | `2C3E40` | `2C78C0` | Stack(player datum, influence list), `ret8`; first argument ignored. |
| King slot33 / +84 spawn-zone acceptance | `A5750` | `A1110` | `25BCA0` | Exactly `mov al,1; ret8`; identical literal pointer to Slayer in each image. |
| King slot26 / +68 team rotation | `224240` | `253EE0` | `1CB030` | Exactly `or eax,FFFFFFFF; ret`; identical pointer to Slayer. |

These entries come from actual registrations and complete original listings, rather than subclass virtual-method numbering. The report includes14 complete cross-image instruction pairs for traits, base fallback, death statistics, team predicate, retained-stat adapter, phase transition and round preparation. [Report](../analysis/multiplayer/game-variants/king-callbacks-20261011/report.json), [literal retail/TU callback inventory](../analysis/multiplayer/game-variants/engine-tables.json).

King+0C does not clear team hill timers+180..19C, occupant mask+1A8 or player inside timers+1B8. King+90 does nothing. Only the actual owners of those fields may reset them. Full King slot1 initialization is a different operation and clears the complete1DC engine state. [Retail initialization](../analysis/retail/functions/002bd140.c), [retail round callback](../analysis/retail/functions/002bd200.c), [complete three-image callback listings](../analysis/multiplayer/game-variants/king-callbacks-20261011/report.json).

For transition into status1, original phase dispatch calls shared round preparation first. Preparation clears shared round storage, performs the actual ordinary-mode reset providers, resets per-player fields through `159120` in TU1.1, and tail-dispatches engine+0C. After returning, phase transition updates the shared start clock and ordinary player lives/initial-spawn flags, invokes engine+90(1), and only then stores lifecycle1. Entering status2 freezes enabled retained recording outside playback and installs a5-second countdown; status3 increments round index and installs a1-second countdown before the same +90 dispatch. Unchanged status invokes neither callback. [TU preparation](../analysis/map-pack-update/functions/00159180.c), [TU per-player preparation](../analysis/map-pack-update/functions/00159120.c), [TU phase transition](../analysis/map-pack-update/functions/0015dbc0.c).

The separate TU phase-application owner`15E200` also calls engine+0C when its supplied phase is1 and options+180 is source4. Its other-image addresses are not established here. [TU phase application](../analysis/map-pack-update/functions/0015e200.c). Native orchestration must preserve the actual owner/order instead of replacing both callbacks with a single invented reset.

The frozen fixture executes whole phase transitions for status sequence1,1,2,3,1 in all three images using real Lockout scenario/matg/mulg, an empty player roster and declared mode4. Normalized callback order, complete King states and resulting lifecycle agree across images. A separate ordinary mode1 probe below also executes complete original equipment initialization and the actual reset chain with an empty roster; no original routine return is replaced. Populated-player, world-object, networking, HUD and recording services remain additional integration requirements.

```python
import sys, struct
sys.path.insert(0, 'tools')
from oracle_round_end import RoundEndTrace, G, M
from oracle_king_engine import OWNERS as KING
from oracle_king_callbacks import OWNERS
from oracle_mp_full_scoreboard_1_5 import HudCache, ROOT
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESI
cache = HudCache(ROOT/'assets/disc/maps/lockout.map')
for name, equipment_init in [('retail',0x15ace0), ('tu11',0x15c320), ('u5',0x15d700)]:
    t = RoundEndTrace(name, cache)
    t.setup_lives(dict(limit=0, mode=1, lifecycle=0, status=1, players=[]))
    t.w(M+0xc14,4); t.w(G+0x180,4)
    t.w(KING[name]['scenario'],cache.magic+cache.scenario)
    v = t.u(t.u(t.spec['table']+16))
    watched = {t.u(v+4):'init', t.u(v+12):'round', t.u(v+0x90):'phase',
               OWNERS[name]['prepare']:'prepare'}
    events = []
    def observe(g, address, size, user):
        if address in watched: events.append(watched[address])
    t.g.hook_add(UC_HOOK_CODE,observe)
    t.call(equipment_init,registers=((UC_X86_REG_ESI,M+0x7dc),))
    t.call(t.u(v+4)); t.call(OWNERS[name]['phase'])
    assert events == ['init','prepare','round','phase']
    assert struct.unpack('<h',t.g.mem_read(M+0x6c,2))[0] == 1
    print(name,events)
```

## Traits and death statistics

The trait query reads the signed16 player+1B8 timer. For timer>0 it returns hill flags at options+22C: kind1 camouflage=bit4, kind2 damage=bit2, kind3 resistance=bit3. A false hill bit overrides a true global bit while inside; it does not fall back. For timer<=0 it tail-dispatches the complete shared base query, which uses global options+184 bits2,12,13 for those respective kinds. Other kinds return AL0. Negative wrapped timers therefore differ from the scorer/occupancy-mask test of timer!=0. [Retail King query](../analysis/retail/functions/002bd9d0.c), [TU base query](../analysis/map-pack-update/functions/00253f40.c), [three-image executable trait fixture](../tests/fixtures/king_callbacks_original.json).

TU`15C8B0` supplies `(player,1)` to engine+8C, then updates the actual current unit's camouflage through the original controller only if AL is true and a unit exists. TU`1615F0` tail-dispatches its existing two stack arguments to engine+8C and returns AL0 when no engine exists. The combat consumer`15F520` queries attacker kind2 and victim kind3. Having the boolean kernel does not implement the unit/controller/damage consumers or determine their numeric balance values. [Camouflage consumer](../analysis/map-pack-update/functions/0015c8b0.c), [query wrapper](../analysis/map-pack-update/functions/001615f0.c), [combat consumer](../analysis/map-pack-update/functions/0015f520.c).

For a non-NONE, nonself, nonbetrayal credited attacker, slot30 adds retained stat28 if the victim's signed inside timer is>0, then independently adds retained stat27 if the attacker's timer is>0. Both events credit the attacker. Betrayal tests only the third argument's low byte;256 therefore behaves like0. The fourth argument has no load anywhere in the complete callback. No score, player match-bank field or inside timer is written by this callback. [Retail whole callback](../analysis/retail/functions/002bda60.c), [original complete callback listings](../analysis/multiplayer/game-variants/king-callbacks-20261011/report.json).

| Actual shared dependency | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Selected-engine/team-bit predicate | `15EAF0` | `160150` | `161530` |
| Retained-stat player/team adapter | `15B930` | `15CF70` | `15E350` |
| Retained general-stat ADD | `1967D0` | `1982D0` | `19B1D0` |
| Original45-entry descriptor table | `46DDC8` | `4752B0` | `47A388` |

The adapter receives EAX attacker datum, CL team-mode boolean and stack(stat,delta), `ret8`. It passes attacker low16 as retained player slot, the attacker's signed team when team mode is active (otherwise NONE), and delta1 to the actual general-stat ADD. It does not call the separate match-stat writer. Original descriptors27/28 both have minimum0 and maximum32767 in all three images; their other12 bytes remain literal original descriptor data. ADD observes recording-enabled/frozen flags, low15-bit accumulation, descriptor bounds and preservation of bit15. Native mapping is the existing `source_result_stat_add(publication,27_or_28,attacker_slot,team_or_NONE,1)` contract. Inventing matching player-bank fields would add behavior absent from this source callback. [TU predicate](../analysis/map-pack-update/functions/00160150.c), [TU adapter](../analysis/map-pack-update/functions/0015cf70.c), [actual retained ADD](../analysis/map-pack-update/functions/001982d0.c), [native shared retained writer](../src/source_result_statistics.cpp), [descriptor bytes in fixture](../tests/fixtures/king_callbacks_original.json).

Original shared death owners retail`15CE70`/TU`15E4D0`/TU1.5`15F8B0` dispatch slot30 at the tail after their common death/life/respawn/event/stat work. Separate original-only calls with current units NONE, mode4, cause41 and both inside timers30 returned in all three images. The actual tail arguments were `(attacker9ABC0003,victim9ABC000F,0,NONE)` and both inside timers remained30. The fourth callback argument therefore is not the death-cause input41 in this reached branch. [TU shared death caller](../analysis/map-pack-update/functions/0015e4d0.c), [pinned shared-death harness](../tools/oracle_player_lives.py).

## Spawn and equipment consumers

King's spawn-zone acceptance is exactly Slayer's always-true callback, independently checked by actual registered pointers and three complete calls. The shared zone consumer retail`23B170`/TU`23D270` filters source4 with mode-mask bit2, then invokes slot33. Team designator/membership filtering remains shared. King does not need a custom zone-acceptance predicate. [Retail zone consumer](../analysis/retail/functions/0023b170.c), [literal table report](../analysis/multiplayer/game-variants/engine-tables.json), [existing native source zone bank](../src/spawn.cpp).

The shared influence collector retail`23BC40`/TU`23DD40` runs scenario zones, moving-object influences, player/death-history influences, then engine slot32. King slot32 appends type8 followed by type7, both at the literal state centerXYZ+164, only when polygon count+60 is>=4 and even. It does not alter center Z or flip a weight's sign. Shared append retail`23BA10`/TU`23DB10`/TU1.5`244D00` copies each imported type's inner radius, outer radius and weight plus the global upper/lower distances. Each append independently checks capacity128; a list already containing127 entries admits type8 only. [Whole retail collector](../analysis/retail/functions/0023bc40.c), [whole King callback](../analysis/retail/functions/002bdb00.c), [TU shared append](../analysis/map-pack-update/functions/0023db10.c).

The distance/type banks come from actual `mulg` runtime fields and scenario overrides, initialized by complete retail`23AEA0`/TU`23CFA0`/TU1.5`244190`. A separate original-only probe ran these initializers with real Lockout data and then63 whole King slot32 calls covering counts0,1,3,4,5,6,32 and preexisting list counts0,127,128 across all three images. All complete appended bytes agreed. For explicit center(1.25,-2.5,3.75), actual Lockout data produced type8 inner4/outer8/upper1/lower3/weight-1, then type7 inner16/outer20/upper1/lower3/weight0.05. These are observations of that map's imported rows, not native balance constants. [Retail initializer](../analysis/retail/functions/0023aea0.c), [TU initializer](../analysis/map-pack-update/functions/0023cfa0.c), [native import preserving actual rows/overrides](../src/content.cpp).

The generic source selector retail`15A090`/TU`15B6D0`/TU1.5`15CAB0` accepts source4 explicitly, plus selectors12,13,14. With no selected engine it instead accepts selector0. ABI is EDX selected source type, EDI signed16 selector bank and stack count, `ret4`; return EAX first admitted index or NONE. A separate60-call original probe covered10 selectors with engine present/absent in each image and all outputs agreed. The same source consumer is called by world netgame update`15C4B0` and starting-equipment selection`1A28A0`; those source rules need no invented King weapon list. [Retail selector](../analysis/retail/functions/0015a090.c), [TU selector](../analysis/map-pack-update/functions/0015b6d0.c), [world caller](../analysis/map-pack-update/functions/0015c4b0.c), [starting-equipment caller](../analysis/map-pack-update/functions/001a28a0.c), [native generic selector](../src/source_netgame_weapon.cpp).

Read-only native audit found the generic providers already preserve raw netgame row selectors, raw spawn zones,12 spawn-type rows and scenario overrides. Initial native consumers still passed source2 in the spawn call, world-weapon scene admission, pickup admission and starting-weapon lookup; these were reported for the parent's concurrent integration. They cannot be treated as source4 merely because a raw provider accepts4. The parent has since threaded selected source type through the inspected callers. Legacy prefiltered Slayer banks in `content.cpp` remain source2 views; the full raw starting-equipment/source-zone banks are the required source4 inputs. This note does not certify the concurrently edited native integration. [Native zone provider](../src/spawn.cpp), [content provider](../src/content.cpp), [match consumers](../src/multiplayer.cpp), [scene admission](../src/source_netgame_weapon_scene.cpp), [pickup admission](../src/multiplayer_inventory.cpp).

Multiplayer game type2 and source engine4 are distinct enums. World-weapon birth/scale and scene context still receive multiplayer game type2; selectors receive source engine4. Passing4 into a game-type argument would choose a different source branch. Other unimplemented vehicles/equipment/controllers, full original spawn-point candidate admission, world poses and physical spawn services are not established by these selector/slot32 tests. [Native birth/selector contracts](../src/source_netgame_weapon.cpp), [raw player-start fields](../src/cache.cpp), [original spawn oracle boundaries](../tools/oracle_mp_spawn.py).

## Enabled FFA recording contract

The score owner's `(4,NONE)` call is a recording event, separate from the retained-stat ADD used by death statistics. Complete owners are retail`1972A0`/TU`198DA0`/TU1.5`19BCA0`; ABI EBX player datum, two stack arguments, `ret8`. Recording-disabled or frozen returns before wall time or timestamp mutation. Enabled, unfrozen execution obtains a first time32 sample and continues only if literal unsigned32 `(last-first_now)>3`; do not replace this expression with a guessed monotonic three-second throttle. [TU recording owner](../analysis/map-pack-update/functions/00198da0.c), [retail owner](../analysis/retail/functions/001972a0.c), [TU1.5 original scoring-owner listing](../analysis/multiplayer/game-variants/king-engine-20261011/scoring/instructions/u5-0019bca0.asm).

| Dependency/global | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| time32 owner | `321AAE` | `32803E` | `32BBAE` |
| Player position sampler | `1994D0` | `19AFD0` | `19DE80` |
| Actual object-position owner | `B9DD0` | `BACC0` | `BBE00` |
| Whole ring append | `197120` | `198C20` | `19BB20` |
| Enabled/frozen bytes | `510CA0/+1` | `518490/+1` | `51DFA0/+1` |
| Ring index | `510CA4` | `518494` | `51DFA4` |
| Last time | `510CA8` | `518498` | `51DFA8` |
| Recording start time | `511024` | `518814` | `51E324` |
|1000-entry ring base | `515C34` | `51D424` | `522F34` |

The sampler prefers player current-unit+2C, falling back to player previous-unit+30. Both NONE returns AL0; otherwise it calls the actual object-position owner, preserving parent/node-transform behavior. Record bytes are type2 at+0, low player byte at+1, FF at+2, zero at+3, aligned XYZ binary32 at+4/+8/+C, second stack argument at+10, first stack argument at+14, and second time32 sample minus recording-start at+20; all other bytes are initialized0. The second clock call precedes pose resolution even when both units are NONE. The whole append copies36 bytes and advances index modulo1000. After the admitted sampling branch, last time becomes the first sample even if no position was available and no record was appended. The later [whole enabled recording research](MP_KING_RECORDING_EVENT_SOURCE_RESEARCH.md) corrects the earlier offset transcription and verifies these bytes against unchanged execution. [TU sampler](../analysis/map-pack-update/functions/0019afd0.c), [TU ring append](../analysis/map-pack-update/functions/00198c20.c), [whole TU recording owner](../analysis/map-pack-update/functions/00198da0.c).

This research inspected the enabled time/pose/ring paths; the callback corpus does not execute a live enabled recording timeline. Its enabled/frozen cases concern retained-stat writing. Actual recording wall time, unit pose, retained identity remapping and complete final report consumers remain native work; returning a supplied recorder success would not satisfy them.

## Reproduction and measured boundary

Run `.tools/python/bin/python tools/oracle_king_callbacks.py` from the repository root. The original fixture currently has SHA256`51efc01caddc6b4b754ed4fe7de4f0ce72ea0c19fbb818d505200f7a9441900d`. It contains1,260 trait cases and516 death/stat cases per image, plus complete initialization/five phase-transition calls:5,346 whole calls. It checks entire player/shared-match storage for unowned writes; complete retained4040-byte banks are deduplicated in `raw_blobs`, with each death case referencing before/after hashes and actual retained-ADD call arguments. Fourteen full instruction-shape pairs match across images, with21 complete owner listings in the report. [Generator](../tools/oracle_king_callbacks.py), [fixture](../tests/fixtures/king_callbacks_original.json), [report](../analysis/multiplayer/game-variants/king-callbacks-20261011/report.json).

Trait cases cover signed timer boundaries, global and hill flag independence, forced false hill overrides and unknown kinds. Death cases cover NONE/self/credited attackers, low-byte betrayal, ignored fourth argument, team/NONE membership, recording disabled/enabled/frozen, signed timer boundaries, descriptor clamps and retained bit15. After correcting the retail preparation observation address to the original CALL target157AE0, the final corpus additionally checks normalized phase equality and complete preparation/phase pairs. Two separate processes reproduced the final fixture and report byte-for-byte; report SHA256 is`b628254df997204b5a58b98b0030ef8860fcddf867d2e71485f1de8e6e058d66`. No native output generated this fixture.

Complete source callbacks and isolated ordinary/playback transitions are necessary evidence, but they do not prove populated native round reset, objective HUD, source pose ownership, score/trait combat effects, enabled event recording, native UDP replication, final results or physical Xbox/LAN behavior. Those gates remain part of the active all39-variant, tag-driven operational goal.
