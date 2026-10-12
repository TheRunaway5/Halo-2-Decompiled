# Halo 2 host departure, migration, rejoin and backfill evidence

## Finding and scope

Original Xbox Halo 2 has an in-game simulation authority handoff, not just a
pregame party or search-host change. Session election alone is insufficient:
the original world watcher follows the new session host, promotes the client's
existing simulation world, repairs replication/object/engine/recording owners,
and reestablishes the surviving machines. The current native ONLINE runtime has
fixed host/client roles and no authoritative-world restoration API. Changing
the relay's `host` field would not implement the original behavior.
Sources: [world watcher, retail 0x83220](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_083a40.cpp:825),
[world promotion, 0x6AEE0](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_067e10.cpp:1840),
[current native roles](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_runtime.cpp:217).

This is a bounded source and static executable investigation. No original
Xbox session, emulator migration, live WAN migration, original rejoin or
backfill was exercised. Product code, existing documentation, builds,
launchers and deployments were not changed by this investigation.

## Authority and reproducible artifacts

The examined retail executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The separately imported Xbox copy has the same hash. The support directory is
[`.local/host-migration-research-20261012`](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012).
It contains:

- [collector](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/collect.py), input
  [hash manifest](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/manifest.json), and
  [numbered recovered-source excerpts](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/source-excerpts.txt);
- retail byte ranges and `objdump -D -b binary -m i386 -M intel` disassemblies
  for host loss, migration start/update/completion, join admission, world watcher,
  promotion/demotion and network defaults;
- the retrieved Bungie overview and GDC slide-page HTML, with retrieval URLs,
  sizes and hashes in [public-source-status.json](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/public-source-status.json);
- [retail-config.tsv](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/retail-config.tsv), linking
  recovered default values to the executable's stores. Configuration storage is
  in the executable's zero-fill region; the values come from initializer code,
  not a pretend static read of live configuration memory.

Reproduce with `PYTHONDONTWRITEBYTECODE=1 python3 .local/host-migration-research-20261012/collect.py`.
The collector only reads source/XBE, retrieves public evidence, and writes its
own support directory. It does not execute Xbox instructions or run fixtures.

## Public behavior versus pregame gathering

Bungie's archived matchmaking overview says that a matchmade host can leave
but cannot end the game for everyone, and that departure, power-off or loss of
connection causes automatic replacement while the match continues without that
player. This is an explicit in-game continuity claim. Its earlier discussion
of choosing an advertised game and retrying failed joins describes forming a
match before game start. These are different operations.
[Bungie matchmaking overview, archived original page](https://web.archive.org/web/20151019104400/http://halo.bungie.net/stats/content.aspx?link=h2matchmaking).
The original HTML was retrieved using the archive's `id_` variant; relevant
paragraphs are preserved at lines 223 and 227 in
[the text artifact](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/bungie-matchmaking-overview.txt:223).

Recovered client code also invokes host selection before starting a match:
`c_session_state_start_match::update`, retail `0x72700`, waits for
`0x6E720` and `0x6E6B0`, becomes leader, then runs the ten-second countdown.
`0x6E6B0` can request a host transfer while the session is stable state 5.
That call site proves a pregame optimization seam, not continuity of a running
world. The separate watcher and authority promotion below establish the latter.
Sources: [start-match owner](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_058dd0.cpp:1785),
[pregame transfer gate](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_058dd0.cpp:907).
The GDC primary slide pages identify the search/gather distinction
([search slide 39](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=39),
[gather slide 40](https://halo.bungie.org/misc/gdc2005_recreating_lanparty/index.html?slide=40));
the image content was not independently reinspected in this bounded pass.

## Session handoff state and invariants

Field names below retain offsets where semantic names are incomplete. The
recovered `c_class_58D20` has host member at `+40`, membership generation `+4C`,
leader field `+50`, member count `+54`, local member `+72D8`, session state
`+741C`, transition scratch `+7420`, and watcher membership generation `+7618`.
Do not collapse the leader, host, local member and native account identity into
one value. [Session layout](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/include/unknown_059ad0.h:211),
[leader/local query 0x58D50](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_058cb0.cpp:165).

| Path | Exact source-backed requirement/result |
|---|---|
| Graceful host leave, `0x614A0` | With more than one member, state 7 stores leave intent, candidate mask, search time and selected `NONE`. Candidates exclude the local host unless a specific peer was requested. With one member, leave closes normally. |
| Host choice, `0x61AC0` | Candidate must acknowledge current membership **and** parameter generations. For an optimization rather than leaving, its connectivity mask must cover every session member. Proposal is message 15, commit is 17; selected peer must acknowledge before commit. |
| Abrupt loss, `0x62AB0` | Retail jump table and comparisons admit election only from session state **3**, with valid membership generation and signed member count greater than one. State 4 goes directly to cleanup. Old host observer channel closes before `0x616E0` starts state 9. |
| Election start, `0x616E0` | Build a `0xC0` peer map, select local initially, seed local pending mask and old-host failed mask, increment `+7650`, clear 52-byte tracking at `+761C`, reset `+7654/+7658`, replace scratch with the `0x118` election state, enter state 9. |
| Election receive, `0x5E7F0` | Sender must be a known nonlocal member. A newer valid membership generation wins. At equal generation, selected identity/index must agree; compare reachable count, then greater `map+50`, then lower candidate index. Track per-peer pending/failed and connected masks. |
| Election update, `0x61EF0` | Local candidate can promote only when every nonfailed member is present and connected in the peer map, with the configured failure/selection waits. Failed selected candidates cause reselection. Message 22 distributes the peer map; hard timeout closes the session. |
| Reestablish, `0x61570` / `0x62550` | Reset membership snapshots if previously nonhost; set host to local; enter state 8. Preserve old-host index, local acknowledgement and failed masks. Request channels where needed, then send host-reestablish messages. |
| Completion, `0x61E00` | All members acknowledged/failed, or the configured deadline, ends state 8. Enter host state 5, remove failed/unacknowledged remote peers, publish membership and parameter updates. Leave or further optimization intent is then applied. |

Sources: [leave](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:2098),
[choice](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:3700),
[host loss](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:2188),
[exact retail loss instructions/jump table](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/host-lost.asm),
[election start](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/network_session_membership.cpp:420),
[election receive](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:4153),
[election update](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:3795),
[reestablish](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:3164),
[completion](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:4410).

Ordinary remote-player loss is separate again: the steady host update in state
5 calls `network_session_disband_member` for an individual member whose observer
channel reaches state 1. It does not unconditionally close every member's
session. The watcher then removes vanished machine views/players as described
below. Current native active-room closure on **any** departure is broader than
this source behavior. [Session update](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:4690).

The peer map carries old-host/candidate member identity blocks, membership
generation, candidate index/quality field, six-byte machine addresses, and
reachable/connected masks. It contains **no world checkpoint**.
[Peer-map structure and builder, 0x62B70](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/network_session_membership.cpp:144).
Membership and parameter replication use different generations and previous
banks (`0x2494` and `0x14B0` bytes); updates are suppressed during state 8.
[Membership publisher 0x62640](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:4324),
[parameter publisher 0x627E0](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:4367).

The host-choice quality comparator `0x619B0` differs from the election-map
comparator above: it compares a properties-bit count, local player count,
clamped `unknown94`, and a nonlinear difference of another properties field,
with a `>0.3` result threshold. Its caller treats a true comparison as replacing
the selected candidate. Several field names/units remain unresolved; do not
rename it to a measured-ping or lowest-RTT algorithm.
[Comparator](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:3642).

## Default timeouts, not universal service constants

| Config offset | Default ms | Use |
|---|---:|---|
| `1474` | 3000 | Host proposal acknowledgement wait |
| `1478` | 10000 | Host-choice search bound |
| `147C` | 10000 | Committed candidate expiry |
| `1480` | 12000 | Host-reestablish completion bound |
| `1484` | 5000 | Delay before also attempting old-host reestablishment |
| `1488` | 45000 | Hard peer-election bound |
| `148C` | 4000 | Peer-election silence/failure wait |
| `1490` | 6000 | Stable candidate wait after failed members |
| `1494` | 1000 | Peer-map broadcast period; changed selection also permits a 500ms send |

These are recovered initializer defaults, corroborated by retail stores in
[network-defaults.asm](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/network-defaults.asm)
and [the values table](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/retail-config.tsv).
The configuration has an online-file owner named `network_configuration.dat`;
therefore the executable defaults do not prove the tuning used by every Xbox
Live revision. [Initializer/defaults](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0662e0.cpp:787),
[online-file definition](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0662e0.cpp:854).

## The running world, not just transport ownership

`0x83220` runs with a real world and initialized game-session globals in state
3 or 8. A host session is usable only in stable session state 5; nonhost live
states use `0x58D70`. While unusable, it calls `0x6B040`, which resets an active
world substate to 2. Once usable, session ownership is compared against world
authority (`0x67E10`: world state is neither 3 nor 5). A new host with a client
world invokes `0x6AEE0`; an old authority that is now a client invokes `0x6AE20`.
It then remaps machine membership and follows the authority-specific view path.
[Watcher](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_083a40.cpp:825),
[authority query](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_067e10.cpp:256),
[substate reset](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_067e10.cpp:1005).

For distributed multiplayer client world state 5, `0x6AEE0` resets the substate,
writes world state 4 and game mode 5, repairs replication through `0x89A20`,
resets views with `0x6A770`, admits missing scenario objects with
`0xD5560(true)`/`0xD5640`, runs `0x185A30`/`0x185630`, calls engine promotion
`0x162060`, and restores result-recording participants/teams with
`0x196470`/`0x196780`. It does not discard the entire existing distributed game
world and start a new match. The separate state-3 branch refuses promotion
while join data is still being received and reconstructs its simulation world.
[Promotion source](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_067e10.cpp:1840),
[retail writes at 0x6AF02 / 0x6AF09 and call chain](/home/eric/Developer/Halo-2-Decompiled/.local/host-migration-research-20261012/world-promotion.asm),
[scenario preservation](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0b7300.cpp:676),
[recording identity remap](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_1967d0.cpp:1056),
[active team retention](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_1967d0.cpp:1206).

Watcher membership `0x83F00` requires the local machine to exist in the updated
membership, remaps machine indices, removes vanished views, and marks affected
players for removal on authoritative worlds. Thus leaving peers and surviving
players must be treated differently; restarting everyone with new player/object
identities loses this source-owned continuity.
[Membership remap](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_083a40.cpp:739).

## Rejoin and backfill: what is established

The generic session join gate `0x62FE0` requires a host, rejects privacy field
`+498C > 1`, rejects host-transition/disband states 6/7/8, permits an existing
member identity, and otherwise checks machine/player capacity and reservations.
`0x19A148` maps privacy selections 0/1 to values 0/1 and other selections to 2.
This is a session admission rule; its successful existing-identity branch is
not proof of a public reconnect button or matchmaking replacement policy.
[Join gate](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:3098),
[privacy producer](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_19987f.cpp:2185).

Reservations are keyed by a 12-byte player identity plus session ID, timestamp,
timeout and joined flag. Unjoined reservations reduce open player slots;
expired reservations cease reserving capacity. Adding a local player also
requires the listener's `player_can_join(identity)` when a listener exists.
That listener policy was not recovered by this bounded trace, so bypassing it
would invent admission behavior.
[Reservation creation 0x62EB0](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:2218),
[expiry/open slots 0x62DE0 / 0x62E30](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/network_session_membership.cpp:179),
[player admission 0x60000](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_059ad0.cpp:2001).

The simulation has an establishment identity and baseline sequence:
`0x85E70` ignores older establishment IDs and marks out-of-sequence baselines
stale. `0x85DC0` / `0x85ED0` begin and receive bounded join data, failing the view
when buffering fails. Those owners prove state synchronization mechanics;
they do not by themselves authorize a fresh queue entrant in a running
matchmade game or establish automatic return of a removed player.
[Baseline and join-data owners](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_085540.cpp:574).

No primary source inspected here proves that original ranked or casual
matchmaking automatically backfilled vacancies with new queue players. No
primary evidence here settles the exact availability/timeout of reconnecting
to an already-started matchmade game after a player's removal. Keep both
policies unresolved. Continuing **without** the departed host is established;
automatic restoration of that departed player's old slot is a different claim.

## Current native gaps and the actionable boundary

The completion audit already marks migration/rejoin/backfill unfinished and
the launch document explicitly closes active rooms on disconnection.
[Completion audit](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/online-completion-audit.md),
[launch contract](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/match-launch.md).
The concrete owners confirm the following:

| Native owner | Missing requirement for game continuity |
|---|---|
| Relay `Room` / connection finalizer | Active departure closes all peers. There is no migration phase, authority generation, survivor acknowledgement, reserved reconnect lifetime or replacement admission. Only postgame voice selects another remaining host. |
| Runtime `run_multiplayer` | Constructs `LanHost` or `LanClient` once. Failure exits to ONLINE. No freeze/elect/promote/reestablish/resume path exists. |
| `LanClient` | Holds decoded `MultiplayerState`, fragment assemblies, cookie and sequence counters. There is no API to turn that bank into a live authoritative match or rebind its host safely. |
| `MultiplayerMatch` / `Simulation` | Authoritative objects, allocation salts, inventories/timers, attribution, deterministic RNG, projectiles, round lifecycle, engine state and recording banks remain internal. Presentation snapshots do not serialize/restore all these owners. |
| Result publication | Relay results require the current original host and agreement from the original admitted players. A migrated roster/authority needs explicit retained-participant and completion ownership; merely deleting departed confirmations would change result policy. |

Sources: [active close / postgame voice change](/home/eric/Developer/Halo-2-Decompiled/multiplayer/master_server/match_transport.py:299),
[fixed relay route validation](/home/eric/Developer/Halo-2-Decompiled/multiplayer/master_server/match_transport.py:274),
[fixed runtime roles](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_runtime.cpp:217),
[failure exit](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_runtime.cpp:1054),
[client banks](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer_network.cpp:1008),
[snapshot scope](/home/eric/Developer/Halo-2-Decompiled/include/halo2/multiplayer.hpp:114),
[authoritative match owners](/home/eric/Developer/Halo-2-Decompiled/include/halo2/multiplayer.hpp:296),
[simulation owners/RNG](/home/eric/Developer/Halo-2-Decompiled/include/halo2/simulation.hpp:228),
[result authority gate](/home/eric/Developer/Halo-2-Decompiled/multiplayer/master_server/match_transport.py:220).

For a native implementation, the prerequisite is a versioned, complete
authoritative continuation owner with a proven import path, or an equally
complete replicated world that can be promoted. It must preserve surviving
identities/object handles, current tick and RNG, scores/round state, cooldowns,
damage attribution, persistent world changes and retained PCR participants.
The subsequent transport handoff needs one acknowledged new authority and
generation, rejection of old-authority packets, fresh connection baselines,
voice reattachment and a bounded failure route. These are engineering
requirements inferred from the traced owners, not recovered native wire bytes
or a claim that a source-perfect migration is already implemented.

The first useful proof would be continued gameplay through actual host
departure with at least two surviving players, preserved state and a completed
PCR; graceful leave and abrupt loss should be tested separately. Repeated
handoffs, old-host packet rejection, disconnect during election, and failure
to reach a viable survivor also need evidence. An election fixture or a
postgame voice-host swap cannot replace that proof.
