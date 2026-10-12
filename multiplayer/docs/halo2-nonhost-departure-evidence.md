# Original nonhost departure, retained results and quit ownership

2026-10-12. Bounded original-source investigation and unchanged Xbox instruction
controls. Ordinary remote-player loss keeps a salted, allocated **left-player**
record; it is not the later deletion/slot-reuse operation. This is the concrete
prerequisite for retaining chosen names, statistics and final placements. No
live Xbox, live native departure, WAN continuity, retired service arbitration
or full combat teardown was exercised here.

## Authority and reusable controls

[Collector](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/collect.py),
[report](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/report.json),
and [frozen raw departure fixture](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/departure-fixture.json)
are new support artifacts. Reproduce from the repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python .local/nonhost-departure-research-20261012/collect.py
```

The report pins every examined executable and recovered source file and saves
CFG-complete assembly listings with original instruction bytes. Executable
hashes are retail `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
TU1.1 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and
U5 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Addresses below are **retail** unless explicitly labeled U5. The suggested
`62090 -> 635C0` pair was not established in these images; `62090` is not a
function entry in the supplied U5 image. Use the pinned retail chain below.

Three controls execute unchanged `84F30 -> 152DF0 -> 157DE0`, actual Slayer
registration, original event recipients, full retained remap and machine
binding. No original instruction or game/platform service return is replaced.
Declared inputs include four allocated players, an original Lockout cache,
no current unit/local view/controller/simulation world/replication entity,
mode1, engine2, options `+184=1`, both initial team masks3 and clock tick12345.
The remaining external paths are static evidence, not successful executions.

| Initial flags/current salt | First result | Final flags/leave tick | Duplicate result |
|---|---|---|---|
| `0001/8001` | AL1 | `0002/12345` | AL0 |
| `4001/8001` | AL1 | `4002/12345` | AL0 |
| `8041/1234` | AL1 | `8042/12345` | AL0 |

All preserve salt, identity12, profile144, all four allocated players'437-word
statistics and their4x4 PvP entries; active census changes4 to3 exactly once.
Wrong key, index outside high-water, `NONE` index and zero current salt each
return AL0 without player/census mutation or calling `152DF0`. Input contains
**index and key12, no incoming salt**: it constructs the current full datum
from current salt. Do not claim a nonexistent stale-input-salt check.
The fixture includes before/after four complete `21C` rows, census `2C`, numeric
`4040` bank, retained `DE78` bank, machine roster96, options, pool bitmap,
recording bytes, original calls and memory writes. Its after-rows are captured
before the rejection controls change input records.

Machine binding `199310` reads96 bytes directly from `[4E8C20]+30`.
These departure controls bind that pointer to the declared census record, whose
roster is zero-initialized; the earlier helper's patterned roster is unused.
`resolved_source_machine_roster_hex` and `machine_roster_hex` export the actual
zero roster; every captured `199250/1991F0` query confirms those six-byte inputs.
`320560` is stack probing, not a platform machine-identity provider.
[Actual machine binding bytes](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/retail-owner-00199310.asm).

## Session loss becomes a simulation leave

Stable host state5 in `5A090` checks each connected member's observer channel;
a failed individual invokes `5FDA0`, sending disband to that member and calling
`5FE20`. The latter removes that member's up-to-four session player entries,
compacts machine membership, adjusts affected member indices and increments
membership/watcher generations. `60200` clears the session player mask and
member slot, not the game-player pool or retained statistics. Graceful
`5C9E0` sends leave acknowledgement and uses the same member-removal owner.
[Host update/removal](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:4677),
[member removal](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:524),
[session player removal](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/network_session_membership.cpp:63).

Watcher `83610` observes the membership generation and emits **type0**, with
old slot/key, for previously present players absent from new membership.
`84270` also produces type0 when a player's machine vanishes. Collection
`84C90` marks left/time and clears machine/controller binding but keeps its
mask, key and configuration. Per-tick simulation block application `683A0`
dispatches type0 to `84F30`; this is an actual producer/consume chain, not just
a decoded client's cosmetic roster change.
[Watcher](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_083a40.cpp:531),
[machine watcher](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_083a40.cpp:307),
[collection](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_084a90.cpp:140),
[block application](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_067e10.cpp:2115).

`84F30` requires an in-range, allocated current slot, matching key12 and
flags bit2 clear. It calls `152DF0` with the current salted datum. Direct
`152DF0` has no repeated-leave or salt guard of its own.
[Exact dispatcher](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_084a90.cpp:250),
[original bytes](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/retail-owner-00084f30.asm).

## Exact player transition and external-service order

Player stride is `21C`: salt `+0`, flags word `+2`, identity12 `+4`, leave tick
`+10`, machine6 `+14`, machine index short `+1A`, machine user short `+1C`,
machine controller long `+20`, controller index long `+24`, local user short
`+28`, unit long `+2C`, previous unit long `+30`, profile144 `+44`, team signed
byte `+C0`. Do not confuse identity account flags with game-player flags `+2`.
[Player layout and complete transition](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_1502e0.cpp:12),
[byte listing](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/retail-owner-00152df0.asm).

`152DF0` performs these operations in order:

1. If current unit exists, set unit damage word `+10A` bit `400`, then `B7360`.
2. In mode4, enumerate units and clear any matching player backlink.
3. `14CAD0(player,NONE)` unbinds the current unit; previous unit becomes `NONE`.
4. If simulation active `4CF770` and not `4CF77B`, call `69580` with world/player.
5. Clear machine index/address/user/controller.
6. If local user exists, clear its controlled unit through `1872D0`, call
   `1682BF(NONE,user,NONE)`, clear census user slot, decrement user count and
   clear player local index. Clear controller census similarly when present.
7. Set player flags bit2, write actual game tick, clear flags bit1, decrement
   active player census once, then call `157DE0`.

`157DE0` requires a selected engine, recomputes team presence with `158140`
when mode is not4 and options `+184` bit0 is set, calls engine virtual slot7,
sends event category0/subtype11 with original full cause datum and team,
remaps retained results through `196470`, then marks the player's engine
replication entity dirty `200` if one exists. Actual retail Slayer engine2
object `47FC80`, vtable `45C6D8`, slot7 points to **`72C70: C2 04 00`**.
U5 engine2 object `48C498`, vtable `468BE8`, slot7 points to
**`2E4D60: C2 04 00`**. Both are complete no-ops returning with one argument;
there is no Slayer-specific leave score subtraction in this callback.
[Engine transition](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_157450.cpp:2555),
[original slot7](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/retail-owner-00072c70.asm),
[U5 slot7](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/u5-owner-002e4d60.asm).

### Current body is deferred damage, not immediate deletion

`B7360` sets object-header mask2 up the object's parent chain. `B6F10` admits
active headers with masks1+2 and no mask8, then `BC470` calls `D5DE0`, the
object damage tick. With damage info present, `D5DE0` consumes bits5/6/10;
on authoritative mode other than4 it calls `DBFB0(object,NULL,bit6,bit10,false)`
then clears those three bits. Leave alone therefore requests
**`DBFB0(object,NULL,false,true,false)`** on that subsequent pass.
[Header owner](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0b58c0.cpp:178),
[object pass](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0b68c0.cpp:283),
[damage dispatch](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0bbf40.cpp:803),
[flag consumer](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:3443).

`DBFB0` skips already body-depleted objects and absent default damage tag;
otherwise it uses the actual globals default damage (`+144`, entry `+14`),
initializes `D6660` damage data, sets damage flag4 and bit80 from its second
boolean, and enters `D7B80`. The normal damage/body-depletion/type-callback
chain owns subsequent effects; no direct body deletion follows merely from
header mask2. The frozen departure controls do not execute this live-unit
path or establish exact later weapon drop/body cleanup timing.
[Default damage owner](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:1272),
[body depletion](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:626).

`14CAD0` clears old object's player `+13C`, writes previous player `+140`,
then invokes `CBF60(old_unit,false)`. `CBF60` refreshes object flags0/1 from
remaining owners `12C/130/13C` and body-depleted bit `10A_2`, calls `10CCC0`
for each still-held weapon and `CC810` for driver/gunner relationships.
`10CCC0` refreshes the item's held-by-player flag. These bodies do not directly
drop/detach weapons. Preserve a required native unit/damage service boundary;
do not substitute immediate player deallocation, object deletion or a guessed
suicide event.
[Original unit unbind bytes](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/retail-owner-0014cad0.asm),
[unit activation refresh](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/units.cpp:188),
[held-item refresh](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/items.cpp:337),
[seat relationships](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unit_object_type.cpp:4074).

## Retention, placement, and final publication

`196470` selects by nonzero salt/high-water, **not connected/flags1**. Its
12-byte identity remap preserves retained properties,437-word player statistics,
both PvP dimensions and event references for an allocated left player.
True type2 removal is separate: `850C0` requires matching key and flags2 set,
then `14C540` releases the pool record. Watcher `83610` emits this deletion
when an occupied left slot must be replaced, followed by type3 admission.
[Retention](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_1967d0.cpp:1034),
[delete gate](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_084a90.cpp:51),
[actual pool release](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_14b560.cpp:1279).

Fifteen additional unchanged controls across retail/TU1.1/U5 prove active
versus left flags have identical remap output. Clearing salt before finalization
zeros the unmatched numeric player statistics, even though old retained
property bytes can remain. After retained header byte1 is finalized, remap
changes nothing even if salt disappears. Keeping only a name/profile tombstone
while freeing the slot is therefore insufficient to preserve statistics.
[All controls](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/report.json),
[existing faithful remap seam](/home/eric/Developer/Halo-2-Decompiled/src/source_result_retention.cpp:48).

U5 scoreboard builder `2486F0` enumerates allocated bitmap entries, including
left players. Comparator `248A70` selector0 sorts FFA flags2 players below
finishers before score; in team mode this selector returns zero. Team selector10
in `248C90` checks actual playing-team presence. `158140` builds playing mask
`engine+0C` from allocated players with flags2 clear, and OR-retains every
allocated player's team at `engine+0E`. A team with one remaining player
remains playing. Original wins/accumulated-score/extended ordering and exact
encoded ties still apply within groups; reuse the existing
[full scoreboard comparator](/home/eric/Developer/Halo-2-Decompiled/src/full_scoreboard.cpp:44)
and [PC publication evidence](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/online-postgame-evidence.md:17),
not a new disconnected-last rule in team mode.
[Team masks](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_157450.cpp:444),
[U5 roster bytes](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/u5-owner-002486f0.asm),
[player comparator bytes](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/u5-owner-00248a70.asm).

Bungie's primary Stats Overview agrees: retain quitters in final standings;
rank teams with any finisher above wholly departed teams regardless of score,
then sort within those groups. A quitter whose team wins gets the same credit
as finishing teammates. This supports quit placement, **not an extra fixed XP
penalty**. [Bungie overview, archived original](https://web.archive.org/web/20141202005503/http://halo.bungie.net/Stats/Content.aspx?link=h2statoverview).
The original paragraph is preserved in
[the copied primary HTML](/home/eric/Developer/Halo-2-Decompiled/.local/nonhost-departure-research-20261012/bungie-stats-overview.html:962);
fresh web access failed and the archive returned429 during this pass.

## Result acknowledgement and XP boundary

Original `741F0` builds rating procedures from retained participants.
`74800` uses each retained rank in FFA, or retained **team rank** in team mode,
with normal pairwise XP tables/clamps. Eligibility's `player.flags &3` reads
account identity flags at retained row `+A`, not live player's flags2 at `+2`.
No additional fixed quit subtraction is demonstrated by these owners.
[Rating inputs](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_072db0.cpp:498),
[procedure producer](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_072db0.cpp:1300).

`196390` finalizes the retained header, submits the bank through `73CA0` for
a matching tracked round, and publishes through `B3E90` when locally owned.
`73CA0` validates playlist/key and local retained machine, builds `741F0`,
and invokes `925C0` with session/round keys and host/report flags. `925C0`
submits **XOnlineArbitrationReport** asynchronously. A changed tracked session
sets `51051C/D=true`; `755D0` can then submit `73CA0(NULL)` with zero procedures.
The normal finalization fallback sets the former flag but the latter false.
These are separate original result/report states, not an all-player identical
native-bank digest protocol.
[Final result owner](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_1967d0.cpp:879),
[submission/update](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_072db0.cpp:1696),
[arbitration call](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_092220.cpp:166),
[session-change flags](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_058ee0.cpp:122).

The original `game-results` message has separate two-word headers and retained
result codec; its registration/receive checks do not establish a retired
server's quorum, fraud resolution, quit-report reconciliation or penalties.
Those policies remain unresolved. The current
[native transport](/home/eric/Developer/Halo-2-Decompiled/multiplayer/master_server/match_transport.py:244)
and [ledger](/home/eric/Developer/Halo-2-Decompiled/multiplayer/master_server/match_results.py:66)
require every admission token to confirm the digest. Replacing that with
survivor-only submission would be a **new native policy**, not recovered parity.
[Original result codec](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0b2440.cpp:248).

## Actionable native boundary

The recovered source transition is now implemented separately from native
`leave`/pool release in
[source_player_departure.hpp](/home/eric/Developer/Halo-2-Decompiled/include/halo2/source_player_departure.hpp)
and [source_player_departure.cpp](/home/eric/Developer/Halo-2-Decompiled/src/source_player_departure.cpp).
It preserves the original index/key/current-salt gate and flag/tick/census
order. Unit/world/local-control/engine/event/remap/replication services are
mandatory when reached; the module does not manufacture their completion.
The no-current-unit control binds the existing real `source_result_remap`.

[player_departure_tests.cpp](/home/eric/Developer/Halo-2-Decompiled/tests/player_departure_tests.cpp)
compares all four complete allocated records, census and retained `DE78`
bytes against the three unchanged original executions, their duplicates and
twelve rejected gates: **18 original controls**. Additional tests exercise
declared external callback order and feed the new left flags into the existing
original FFA/team comparator. These additional cases do not execute original
live unit/world/local-control services. Both CMake Release and address/undefined
sanitizer runs pass. The adjacent **1224** result-retention controls and **9606**
identity/profile controls also pass. Frozen source, build logs and provenance
are in `.local/player-departure-20261012/`; the new test is
`ctest -R '^original_player_departure$'` and uses
`tests/fixtures/player_departure_original.json`.

This is a source-kernel implementation, not an activated native departure
binding. Keep admitted identity, chosen profile,
original stats and left allocation until proven deletion/slot reuse or frozen
results. Rank allocated finishers and quitters with original player/team
comparators. The existing native immediate-clear
[leave owner](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer.cpp:773)
and connected-only
[postgame input](/home/eric/Developer/Halo-2-Decompiled/src/online_postgame.cpp:75)
need different lifetimes; loosening result unanimity alone cannot repair them.
Native authoritative departure transport, complete live-unit deferred damage,
quit result confirmation policy, postgame party membership and physical WAN
continuity still require separate implementation/evidence.
