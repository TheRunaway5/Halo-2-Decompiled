# Original Halo 2 clan matchmaking admission and clan skill

Research date: 2026-10-11. This note distinguishes recovered Xbox client rules,
retail launch fallback configuration, dated Bungie service descriptions, and
the trust boundaries of the replacement Discord-authorized service. It does
not claim that the retired Xbox Live service or every historical playlist file
has been recovered. The research controls do not import the replacement service or runtime.

## Findings that can be implemented

For the recovered launch fallback clan hoppers, match **two intact parties from
two different clans**. Every active player in each party must have the same
nonempty clan identity; guests are forbidden. Minor admits 3–4 players per
party with no party-size difference; Major admits 6–8 with a difference up to
two. There is no leadership-role requirement in the admission gate. Use the
clan's selected-hopper rank and XP, and commit the result to the clan rather
than each member's personal ranking. These conclusions come from the original
admission, host request processor, balancing, statistics query, and result
producer described below, with 74 controls executing unchanged retail bytes.

The replacement service must resolve accepted membership and current party
composition from its own authenticated state. That is the replacement for the
original Live membership trust boundary, not a recovered Discord or HTTP rule.
Pending recruits and user-entered clan names must not establish authorization.
Do not silently fill clan teams with unrelated players or split a clan party.

## Original executable and reproducible controls

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The decompile is a reading aid; the controls load the original XBE sections and
execute its instructions with Unicorn. No original instruction or call is
patched or replaced, and no native matchmaking implementation is imported.
All identities and session/result inputs in these controls are explicitly
synthetic. They contain no Discord account identifiers or credentials.

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_online_clans.py .local/clan-matchmaking-oracle > .local/clan-matchmaking-oracle.json
```

Artifacts:

- [Control script](../../.local/clan-matchmaking-research-20261011/oracle_clan_controls.py).
- [74 passing controls and exact function-span hashes](../../.local/clan-matchmaking-research-20261011/oracle-clan.json).
- [Executed original instructions](../../.local/clan-matchmaking-research-20261011/retail-clan-executed.txt).
- [Undecompiled match initializer, full original disassembly](../../.local/clan-matchmaking-research-20261011/retail-195a40.txt).
- [Host queue processor, full original disassembly](../../.local/clan-matchmaking-research-20261011/retail-06d4c0.txt).

Recorded script SHA-256:
`f9069c51148131498be213d4dc233b9504fcd32e867934f451a2165efabaa810`.
Recorded JSON SHA-256:
`9611f5300f1aace4572a05aa95803495a293790f956e440a80d67d2632a0d61d`.
Executed-instruction trace SHA-256:
`2babcc88b6fd7e8d64681fe93da37ae4c129dc0fd8e30ce2f3ca275c612c06a3`.

Full original `0x66050`, `0x7E210`, `0x90D90`, `0x91FC0`, `0x74720`,
`0x741F0` and `0x73E00` execute in the relevant controls. The full original
`0x6D4C0` runs from entry to either its real rejection-message call `0x630F0`
or its general join evaluator `0x62FE0`, where observation stops. Reaching
the evaluator proves that the preceding clan gates passed; it does not prove
that a final reservation succeeded. The `0x195A40` team-snapshot controls
execute the complete unchanged loop `0x195BEC` through `0x195CCB`; its earlier
platform/timer setup and later transport bookkeeping are outside that control.
No Live service, downloaded configuration, final report arbitration, complete
network join, title-update comparison, live match, or rendered UI is emulated.

## Launch fallback configuration, not one timeless catalog

Original constructor `0x193C70`, cases 4 and 5, calls the type-5 builder
`0x193C00`. The original executable constructs these settings:

| Fallback hopper | Type | Statistics namespace | Teams | Minimum party | Maximum party | Allowed size difference |
|---|---:|---:|---:|---:|---:|---:|
| Minor Clanmatch, case 4 | 5 | 25 | 2 | 3 | 4 | 0 |
| Major Clanmatch, case 5 | 5 | 26 | 2 | 6 | 8 | 2 |

Source: [unknown_1936e0.cpp, `0x193C70`, cases 4/5, lines 305–329](../../tools/reference/halo2-decompiled/src/unknown_1936e0.cpp),
plus the executable constructor controls and saved
[original bytes](../../.local/clan-matchmaking-research-20261011/retail-193c70.txt).
The original English cache resolves the constructor's name IDs `0x1A0007AC`
and `0x180007AE` to those labels. Its bounded
[string extraction](../../.local/clan-matchmaking-research-20261011/original-clan-strings.json)
records the source offsets and map SHA-256.
The field named `team_size` in this reconstructed constructor is **team count**
in its consumers: `0x193440` returns it; `0x1932C0` multiplies it by minimum
party size; `0x193300` multiplies it by maximum party size.
[Consumers](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp).

Two teams are proven for these presets. Generic structural validator
`0x1936A0` does not hardcode two; a synthetic three-team type-5 configuration
passes that validator. This is not evidence that Bungie shipped a three-clan
hopper or that every larger structurally valid configuration is supported by
the complete balancing system.
[Validator](../../tools/reference/halo2-decompiled/src/unknown_193560.cpp).

Contemporary public descriptions are corroboration with explicit dates:

- The Bungie playlist page preserved in the
  [6 April 2005 capture](https://web.archive.org/web/20050406005054/http://www.bungie.net/Stats/page.aspx?section=FAQInfo&subsection=playlists&page=1)
  describes Minor parties of 3–4 and Major parties of 6–8, each against another
  clan, with two teams. Its metadata contradicts those descriptions by listing
  minimum party sizes 1 and 5. Preserve that contradiction instead of silently
  treating the metadata as actual admission. The retained original HTML's
  Minor/Major blocks are lines 593–617 in
  [the source capture](../../.local/online-rank-research-20261011/bungie-playlists-20050406005054.html).
- Bungie's [Stats Overview](https://web.archive.org/web/20141202005503/http://halo.bungie.net/Stats/Content.aspx?link=h2statoverview),
  page dated 30 January 2007, retains an Allowed Party Imbalance table with
  Minor 3–4/difference 0 and Major 6–8/difference 2. It still names Major
  despite its earlier removal, so it cannot establish that the table was a
  current 2007 catalog. See
  [retained original HTML, lines 987–1000](../../.local/online-rank-research-20261011/bungie-stats-20141202005503.html).
- Bungie's [2 December 2005 update, playlist-change bullets](https://bnetarchive.haloman30.com/news/content29ba.html?cid=7271&type=topnews)
  announces Major's removal and Minor's replacement by Clanmatch with teams
  of 4–5, retaining Minor's existing ranking. This is a different dated policy.
  [Fresh preserved primary article](../../.local/clan-matchmaking-research-20261011/bungie-20051202.html).

The archive renderer failed on the two Wayback URLs during this run; their
previously retrieved primary HTML files were read directly. The weekly update
was read in the web tool and retrieved anew. Mirrors are used here for Bungie's
own article, not for a mirror operator's explanation. The fallback executable
settings resolve the launch-default bounds; they do not recover later TMS
files, complete rotations, weights, variant options, or later rank tables.
Label a new launch-based lab catalog accordingly.

## Admission: every active player, guests, roles, host capacity

`0x63E90` delegates to `0x66050` for an active session. `0x66050` first looks
up the selected valid hopper and applies these gates:

| Gate | Original outcome |
|---|---|
| Any active player's identity flags contain guest bits `&3` in type 5 | 12 |
| Any active player's flags equal offline sentinel `0xBAD00000` | 13 |
| A machine's selected-hopper availability state is not 1 | 14, 2, or 11 according to state |
| Any active player's signed property `+0x81` is below configured `+0x5E4` | 15 |
| Type 5 with an active player missing its clan identity | 8 |
| Type 5 without a common clan, including differing nonempty identities | 9 |
| Party smaller than configured minimum / larger than maximum | 6 / 7 |
| Type 5 selected host capacity below configured maximum total lobby size | 10 |
| All those gates pass | 0 |

Source: [unknown_054fe0.cpp, `0x66050`, lines 1474–1561](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp).
The `+0x81` eligibility property's exact product meaning is unresolved here;
it must not be relabeled as clan role, clan rank, or party leadership.

Common-clan helper `0x7ED20` examines the active player mask and compares
each active player's **12-byte clan identity** against the empty sentinel and
the first nonempty identity. Its success requires at least one nonempty
identity, no missing identities, and no mismatches. In session coordinates,
the mask is `+0x111C`, the player stride is `0x13C`, and the first clan ID is
`+0x1238`, equivalent to player properties `+0x70`. Inactive slots do not
participate. This is every active player, including additional players on
one machine, not just the party leader.
[unknown_07a9a0.cpp, `0x7ED20`, lines 641–689](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp).

The capacity output of `0x7E100` is derived from measured member quality and
the network configuration's capacity table. It is not clan level. The retail
initializer's capacity floor is eight: low measurements still admit the
Minor maximum of eight, while the Major maximum of sixteen fails the tested
low-measurement fixture. Do not interpret outcome 10 as a minimum clan-rank
requirement.
[`0x7E100`, lines 725–779](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp).

Neither this helper nor the admission gate checks Peon/Member/Staff/Overlord.
The controls vary controller clan-role metadata across all four values without
changing acceptance. The supplied original clan-permissions pane 236/0
explicitly allows Peons to participate in clan games; its asset provenance is
recorded in [the original clan UI note](halo2-clan-evidence.md).
The original permission body `0x100018A7` reads `> Play in Clan matches`, paired
with the Peon heading `0x0C0018A6`; both were read directly from the supplied
map for this run and are in the same bounded string extraction.
Bungie's [7 January 2005 Peon answer](https://bnetarchive.haloman30.com/News/contente4c3-2.html?cid=2308&type=topnews)
also says their remaining ability is play. This does not grant Peons member
management or recruitment privileges. Party leadership remains a separate
session responsibility.

The original membership-property path obtains the user's teams through
`XOnlineTeamEnumerateByUserXUID` (`0xABC70`), gets team details in `0x18F62B`,
and publishes the team XUID as the session clan identity in `0x18FD94`/
`0x18FE9E`. Member details/privileges are obtained separately. This establishes
a service-provided identity source, not a rule accepting arbitrary asserted
clan IDs. The exact retired service's treatment of pending recruits, membership
races, revocation, and hostile client assertions has not been recovered.
[Enumeration](../../tools/reference/halo2-decompiled/src/unknown_0abc70.cpp),
[controller update, lines 115–167](../../tools/reference/halo2-decompiled/src/unknown_18f62b.cpp),
[property producer, lines 225–291](../../tools/reference/halo2-decompiled/src/unknown_190001.cpp).

## Two intact parties and distinct opposing clans

`0x70280` constructs a matchmaking summary for the whole original party as
one group and records its common clan identity. The later gathered summary
can contain multiple such party groups; its group terminology must not be
mistaken for the physical number of PCs in a party.
[unknown_058dd0.cpp, `0x70280`, lines 1613–1660](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp).

`0x7E210` applies type-5 party minimum/maximum, team count, size-difference,
and capacity rules. For a waiting Minor party of three it produces add-mask
`0x4`: another party of three is eligible. For a waiting Major party of six it
produces `0xE0`: another party of six, seven, or eight is eligible. Completed
3+3 and 4+4 Minor fixtures have add-mask zero and no imbalance; completed
6+8 and 7+8 Major fixtures also have add-mask zero, with imbalance 2 and 1.
These masks are actual original output, not inferred coefficients.
[unknown_07a9a0.cpp, `0x7E210`, lines 1102–1294](../../tools/reference/halo2-decompiled/src/unknown_07a9a0.cpp).

The original host queue processor `0x6D4C0` then requires incoming request
sizes to be in that add-mask. For type 5 it rejects an empty clan identity with
reason 14 and any clan identity already present in the gathered summary with
reason 15, through `0x63280`. The decompile body is disabled with `#if 0` for
shared calling-convention/build reasons; these gates were independently
verified by executing the original retail function from entry to its real
call boundaries. Minor 3+2 and 3+4 fail reason 4; 3+3 from a distinct clan
reaches the general join evaluator. Major 6+5 fails reason 4; 6+9 fails reason
3; 6+8 from a distinct clan reaches that evaluator.
[Queue reading aid, lines 972–1133](../../tools/reference/halo2-decompiled/src/network_session_client.cpp),
[original gate instructions `0x6D801`–`0x6D87A`](../../.local/clan-matchmaking-research-20261011/retail-06d4c0.txt),
[identity lookup `0x63280`](../../tools/reference/halo2-decompiled/src/network_session_membership.cpp).

This rule also prevents forming a clan team by adding a second independent
party from the same clan during this gather path. The already formed party
must carry its clan members together into matchmaking. Candidate filtering
`0x90D90` separately rejects detailed search results advertising the searching
clan's identity. That search filter applies once details are present; the
host gate is additional proof, so distinct-clan admission is not based only
on an asymmetric outgoing client query.
[online_session_search.cpp, lines 536–594](../../tools/reference/halo2-decompiled/src/online_session_search.cpp).

Type 5 uses party-preserving `balance_teams_by_count` and `balance_teams`.
The final gather path `0x71730` checks permitted imbalance and calls the
balancer with parties intact. `0x193470` returns true for type 5, preventing
the fallback that retries without party preservation. In the full unchanged
optimized balancer controls, deliberately divergent synthetic player weights
still assign each 3-, 4-, 6-, or 8-player party entirely to one of the two
opposing teams.
[Final gathering](../../tools/reference/halo2-decompiled/src/unknown_071730.cpp),
[party-preserving assignment `0x91FC0`, lines 392–435](../../tools/reference/halo2-decompiled/src/team_balancing.cpp),
[classification helpers](../../tools/reference/halo2-decompiled/src/unknown_1932c0.cpp).

Do not use `0x7E210` alone as the admission policy. Synthetic 2+4 or duplicate
clan summaries can pass that function, because earlier party admission and
the host request checks are separate. The complete chain above establishes
the valid two-clan/two-party path. Disconnects, reconnection, and late party
mutation still require the session's other readiness/membership checks; those
flows are not fully executed by this fixture.

## Clan rank is the statistic subject; generic averaging still occurs

The selected-hopper skill chain is:

1. `0x74AA0` selects **clan identity at cached group `+0x10`** for type-5
   statistics requests, with namespace `hopper.field_8`. Other hopper types
   use player XUID at `+4`. It deduplicates repeated requests for the same clan
   and namespace. This is not a personal-rank request with a different label.
2. `0x65530` publishes that user's cached selected-hopper rank and XP at
   player properties `+0x8A` and `+0x8C`; for a type-5 hopper those cached
   values came from the clan-subject request.
3. `0x72540` collects those published rank/XP pairs into the party summary
   and calculates rank minimum, maximum and arithmetic average. `0x70EA0`
   passes the aggregate to search initializer `0x90880`.
4. `0x7E210` also calculates generic gathered summary averages and adjusted
   values. These computations still exist for type 5, but their initial
   ranks are the selected **clan** ranking, repeated for its members. They
   are not an average of unrelated personal playlist ratings.

Sources: [statistics query, `0x74AA0`, lines 1462–1560](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp),
[properties, `0x65530`, lines 1412–1470](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp),
[collection, `0x72540`, lines 1092–1174](../../tools/reference/halo2-decompiled/src/unknown_058dd0.cpp),
[search caller, `0x70EA0`, lines 59–84](../../tools/reference/halo2-decompiled/src/unknown_070ea0.cpp),
[search initializer, `0x90880`](../../tools/reference/halo2-decompiled/src/online_session_search.cpp).

The original client can temporarily hold incomplete or inconsistent cached
member values. This note does not establish a retired-service conflict rule
for such races. The central service can obtain one authoritative clan record
per hopper and supply a coherent snapshot, with that authority choice stated
as the replacement trust boundary. Clan queues retain the ranked/clan search
classification, not the casual unlimited-rank fallback documented in
[the casual skill note](halo2-casual-skill-evidence.md).

## Clan-owned postgame XP and rank

The undecompiled original `0x195A40` initializer supplies a missing provenance
link. Its team loop at `0x195C00`–`0x195CAA` selects the first active player
encountered on each team. For a clan match it copies:

| Retained player source | Team-result destination | Meaning |
|---|---|---|
| `+0x80` | team `+0x0C`, 12 bytes | Clan identity |
| `+0x9A` | team `+0x04`, signed short | Previous raw clan rank |
| `+0x9C` | team `+0x08`, signed long | Clan XP |

It does **not** use retained player's adjusted skill at `+0x98`. In the
unchanged loop control, adjusted skill 60 with raw rank 15/XP 5500 produces
team rank 15/XP 5500. A later synthetic same-team player with a different rank
does not overwrite it. This is source provenance, not authorization for
inconsistent member-supplied stats. The valid admission/statistics chain
supplies common clan identity and coherent clan ranking.
[Original initializer bytes](../../.local/clan-matchmaking-research-20261011/retail-195a40.txt),
[captured team snapshot controls](../../.local/clan-matchmaking-research-20261011/oracle-clan.json).

`0x741F0` branches on hopper type. Type 5 iterates the team entries rather
than personal player result rows. It requires an active team, nonempty clan
identity, known rank and known XP; calls `0x74720` for XP change; derives new
raw rank with `0x74970` and its hysteresis; and passes the **clan identity**
to batch writer `0x73E00`. The three statistics namespaces are
`field_8`, `field_8+32`, `field_8+64`; they are not three clans. With the
Minor fallback key 25, the original operation batch addresses namespaces
25, 57 and 89 and only the two synthetic clan identities. Adding valid-looking
personal result rows leaves the complete batch byte-identical.
[Result producer, `0x741F0`, lines 1300–1397](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp),
[batch writer, `0x73E00`, lines 1065–1110](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp).

`0x74720` uses 16 team-entry slots at result collection `+0xDC4`, stride 24.
For each other active entry whose skill is known, it calls the shared original
table exchange `0x746B0` using the two **raw clan ranks and final standings**.
It averages eligible contributions with signed division truncating toward
zero, applies XP floor/ceiling protection, then configured per-match delta
bounds. The retail initializer supplies the same recovered exchange tables
and thresholds used in the player path; it does not call a separate Elo or
TrueSkill formula. Controls establish equal-rank win/loss +100/−100, ties 0,
rank differential 16 beating 15 as +90/−90, XP boundary clamps, tie entries
in the average denominator, and configured +7/−9 delta caps.
[`0x74720`, lines 463–497, and `0x746B0`, lines 340–357](../../tools/reference/halo2-decompiled/src/unknown_072db0.cpp),
[rank/configuration evidence](online-rank-playlist-evidence.md).

The arithmetic helper does not itself validate opposing clan identities; a
synthetic active opponent with a zero identity still affects its calculation.
Admission and authenticated team construction must precede it. The type-5
producer does skip the selected team if its clan identity is empty. Recovered
arithmetic alone does not validate a match report or prevent repeated reports.

## Replacement boundaries and remaining unknowns

The supported central-service implementation is two accepted party snapshots,
each bound to one authoritative nonempty clan, different clans on opposite
teams, configured bounds/imbalance, party preservation, and a clan-owned
selected-hopper skill/result record. Peons can participate. The same selected
clan record should supply subsequent searches; personal skill changes must
not be substituted. Clan ID and Discord identity remain private authorization
keys, while chosen game screen names remain presentation.

HTTP schemas, reciprocal central queue eligibility, queue timing, relay/QoS
measurement, identity-handle design, accepted-membership rechecks, report
signing/idempotence, storage transactions, disconnect policy, and revocation
are native service responsibilities. Original Xbox Live task wrappers do not
establish their modern design. Rank-display fallbacks or eligibility reasons
need an explicit adapter contract; this research does not establish historical
Vista clan-queue UI labels or exact error text.

Unresolved historical evidence includes downloaded later-era hopper/rank
configuration, full map/variant weights and options, complete hostile-client
membership/report arbitration, every split-screen/membership-change edge,
complete join/rejoin/disconnect execution, and live two-clan match behavior.
The retained Vista clan screens are asset/layout evidence recorded separately;
neither their presence nor these arithmetic controls proves final rendered
UI or end-to-end multiplayer parity.

## Replacement implementation

`master_server/matchmaking.py` now has separate launch Minor/Major lineup
policies. The API resolves every accepted member through `ClanApi`, keeps
two distinct clan parties intact, and reads the full private clan raw rank
0..127 for selection. Public level artwork never replaces search skill. The
January2007 personal-rank radius table is not silently applied to these launch
clan rules; the lab uses an explicitly custom radius and timing.

`clan_skill.py` implements the separate original0x74720 aggregator and clan
XP owner. It writes two clan audit rows within the same unanimous result
transaction as the complete individual/team ledger. No member receives a
personal XP update from a clan match. The immutable audit survives dissolution
of its clan.

Native authority choices: one active lineup per clan/playlist serializes XP;
removing/leaving during a queue cancels that lineup and its consent, and during
an admitted match returns409 until completion. Accepted membership is checked
again before admission. These are replacement consistency choices, not claims
about retired Live membership races.

Queue compatibility v6 adds an optional `clan_match:true` catalog flag. Clan
nonmembers receive `level:null` and `eligible:false`; members receive only
their own displayed clan level. The existing Vista219/237 panels display
clan rank or the membership requirement without adding another layout.

The portable [oracle](../../tools/oracle_online_clans.py) independently
reproduced all74 original controls with identical instruction-span/trace hashes
and output values. The committed arithmetic fixture retains the original XBE
pin and spans. Backend checks cover Minor3vs3 and Major6vs8, Peon leadership,
pending/mixed clan rejection, queue revocation, full unanimous commits,
idempotence, persistence, and transaction rollback on stale XP. Native/live
validation is recorded separately in the completion audit.

The native multi-invite check exposed an account-worker race: an older read
could publish after a newer command was queued, clear `busy`, and allow the
next invitation to replace that command. A deterministic blocked-poll/blocked-
command regression first failed with `Older account poll cleared a pending
command's busy state`. Clan and party commands now advance the publication
generation so that prior reads cannot replace their state. This is a native
client consistency fix, unrelated to original clan permissions.

Release20261012T001914Z activates both clan lab playlists. The frozen native
build passed Minor3vs3, Major6vs8 and Major8vs8 with6,14 and16 independent
processes, including real invitations, accepted membership, fresh per-member
consent, native gameplay/Opus, unanimous once-only clan progression, original
Vista PCR16, Done and group requeue. The full boundary exposed and fixed
separate media, fragmented-snapshot and postgame preference fanout limits;
see [voice](voice.md). These use external Discord fixtures and local processes,
not real two-clan WAN play.97 backend tests also pass in the actual VPS Docker
image; deployment retains accepted party/account state and private keys.
