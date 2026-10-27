# Native matchmaking and coordinator core

`master_server/matchmaking.py` implements automatic party queue selection and
match proposals separately from manual server discovery. Its rules are based on
[the primary-source evidence](halo2-matchmaking-evidence.md): Bungie's GDC 2005
presentation, contemporary Bungie documentation, and Max Hoberman's explanations.
The user authorizes explicit best-effort reconstruction when code is absent.

Native **ONLINE → Quickmatch / Optimatch** and the ONLINE lobby's Start →
**Playlists** route now use authenticated HTTP queues. The client reuses the
existing Discord PKCE authorization; the server independently validates its
application, scope and subject. The player keeps their chosen local screen name.
The browser remains a separate anonymous directory consumer.

This connects menus, verified identities and accepted parties, stored per-playlist skill, queue
selection, measured relay connections and native Slayer gameplay admission. The
[match launch integration](match-launch.md) owns the loading barrier, authenticated
roster and game voice. Versioned retail-default skill updates apply to ranked
and hidden casual playlists. Clan playlists use accepted clan membership,
intact opposing parties and separate clan-owned progression. Historical
playlist restoration remains unfinished. No endpoint accepts client-provided identities, ranks, party
rosters or QoS. There is no second login flow.

## Implemented decisions

- One active ticket per party/player; identity uses opaque internal keys.
  Chosen screen names are display data. Rankings belong to the selected playlist.
- Ranked and casual queues are isolated, versioned policies. Both use skill;
  `ranked` controls eventual disclosure, not whether selection considers skill.
  Queue status never exposes player IDs, rank values or hidden casual skill.
- Configured retail XP policies retain private raw ranks0–127 for search, even
  when several ranks share the final visible level. Original0x72540 averages
  unadjusted raw ranks using integer division. The optional inferred mixed-party
  floor belongs to native team balancing and does not alter that search average.
  Party-size eligibility, compatibility fingerprints and required map ownership
  constrain admission. Policies without recovered XP retain their explicit
  legacy level units; mixed-unit parties/candidates fail eligibility.
- Public v8 queues use actual gathering advertisements and the recovered
  candidate selector. Completed queries widen the raw-rank envelope; outstanding
  probes keep the query pending. The original150-result budget and final casual
  unbounded retry apply. Failed targets remain excluded for the current search.
  Original role clocks and player-count countdowns replace the former timed
  rank stages. Server result ordering and relay plumbing remain native policy.
  The legacy coordinator retains its explicit elapsed/reciprocal stage policy
  only for catalogs that do not select the gathering profile. No outcome history,
  engagement target, forced easy/hard sequence or win-rate quota is an input.
- Team formation preserves parties and equal player counts, applies the dated
  significant-party restrictions, minimizes effective-rank sum difference, then
  randomizes equally fair assignments. Parties can still make an imbalanced
  match unavoidable; the system never splits them to obtain a perfect score.
- Clan launch rules are separate: Minor accepts two equally sized parties of
  3–4; Major accepts 6–8 with a difference up to two. Every member must belong
  to the same accepted clan, and opposing clans differ. Peons may play. Clan
  raw rank drives search, and results update the clan rather than its players.
  See [74 executed retail controls](halo2-clan-matchmaking-evidence.md).
- Host choice requires fresh connection measurements for every remote member.
  Missing/stale measurements fail eligibility. The relay measures fresh challenge
  round trips and converts them to explicit native scores. This measures the
  relay path; it does not recover Xbox QoS or establish direct peer reachability.
- Map and variant entries use their configured weights. The core never inserts
  a guessed map rotation. Every player must own every map required by that policy.
- Proposals reserve **queue tickets**, not server slots. Every member must
  acknowledge readiness before handoff. A missing member, cancellation or timeout
  releases the other tickets with their original waiting age. Handoff does not
  claim a game started and does not change competitive ranks.

## Policy and evidence boundaries

No restored original playlist catalog is bundled. Each `Playlist` requires a revision,
provenance, game pool, eligibility and search stages. An actual original hopper
restore needs dated map/variant definitions and matching game configurations.
The demo/test pools, map weights, connection scores and timings are **LAB ONLY**.
They are not advertised as an original Team Skirmish or casual playlist.

| Policy | Current treatment |
|---|---|
| Per-hopper ranks and party average | Configured XP policies use private raw ranks0–127. Recovered retail0x72540 uses integer division over unadjusted available ranks. The display cap applies only to presentation. |
| January 2007 rank extensions, levels1–35 | Published table retained as an optional policy |
| Level36+ | Original final row repeats35 and gives an upward cap50, without a reliable lower bound. A radius must be supplied explicitly; the lab's15 is unverified tuning, not a recovered value. A level50 player's radius is never incorrectly calculated as50−50. |
| Later published-table lookup | The January2007 table is expressed in visible levels. Only its lookup converts a raw integer average to a visible level; candidate distance retains raw units. Mixing this later table with retail XP is an explicit policy synthesis. |
| Mixed party12+1 treated as12+6 | Published example; optional generalized floor affects native team balancing. Recovered search averages use the unadjusted raw ranks. The generalized floor remains inferred. |
| Ranked limits in casual | Never enabled implicitly. Casual requires an explicit policy; the demo/test casual radius is custom tuning. |
| Hidden casual progression and final rank-filter removal | Recovered retail-default XP routines and final query envelope; public level remains null. The lab timer, reciprocal admission and candidate scoring are separate native choices. |
| Significant-party size constraints | Dated table retained for Skirmish, BTB, Double Team, Preview and clan names. Those names do not establish ranked classification or implement clan membership. |
| Multiple significant parties on a team | Conservative one-to-one sorted pairing across teams; an inference beyond the published one-large-party examples |
| Team fairness | Minimum effective-rank sum difference with random ties; documented goal, reconstructed optimizer |
| Candidate traversal | Random eligible sampling and bounded search; global queue ordering, reciprocal checks,50-ticket windows and budgets are replacement service choices |
| Host scoring | Best worst-link normalized score, random ties; unverified replacement for the original connection-scoring formula |
| Search cadence, leases and readiness timeouts | Explicit configuration; no recovered retail values claimed |
| Map ownership | Conservative all-pool requirement; playlist-era DLC rules still need restoration |

The recovered retail candidate core in `session_candidates.py` matches264
complete original banks,27 admission cases,124 selections and100 radius
thresholds. The v8 [native gathering bridge](native-gathering-bridge.md) connects
it to actual partial sessions, stable birth, mapped raw extrema, title language
and server-timed relay-route probes. Original role choice, query exhaustion,
rank widening, peak growth clocks, all seven countdown tables and the150-ID
rejection bank are integrated. Founding hosts remain hosts at gameplay admission.
Production release20261012T022410Z enables this profile in all six lab queues.
Four-client ranked/casual and6/14/16-client clan gameplay/voice/results/requeue
checks pass;142 backend tests pass locally and in actual VPS Docker.
See [candidate evidence](halo2-candidate-scoring-evidence.md),
[lifecycle evidence](halo2-gathering-session-evidence.md),
[follow-up controls](halo2-gathering-lifecycle-followup.md) and
[bandwidth owners](halo2-bandwidth-input-evidence.md). Service query ordering,
relay transport, team balancing, admission and custom game pools remain explicit
replacement policies, not full original multiplayer parity.

The original service used distributed searching/gathering parties. This core
replaces that plumbing with one serialized authoritative queue owner. Calls
must be serialized; it is designed for the existing single-event-loop service,
not concurrent threads. Queues and proposals are ephemeral, expire locally and
are never federated as directory records. After process failure, players must
requeue with the same dated catalog; no cross-master reservation consistency or
exactly-once gameplay allocation is implied.

## Run and verify

From the repository root, with the existing master requirements installed:

```sh
python3 -m multiplayer.master_server.matchmaking_demo
python3 -m unittest multiplayer.master_server.tests.test_matchmaking -v
python3 -m unittest discover -s multiplayer/master_server/tests -t . -v
```

The demo queues two three-person parties and two solos. Initial narrow search
cannot fill a match; after30 simulated lab seconds it proposes two intact
four-player teams and requires all eight readiness acknowledgements. Output
uses chosen screen names and explicitly states that no gameplay was admitted.

Tests cover published table entries, mixed-party behavior, widening and hard
caps, newcomer bounds, casual hidden skill, party preservation, significant-party
restrictions, compatibility, missing maps, fresh/expired QoS, all-member readiness,
cancellation, queue/proposal leases, duplicate admission, capacity, policy errors,
an isolated old party with60 later tickets, varied eligible opponents and weighted
game selection over400 seeded proposals. These are behavioral checks against the
declared reconstruction, not a differential comparison with live original service.

## Native menus and configuration

- Quickmatch uses the last selected playlist in this menu session, otherwise the
  catalog's explicit `quickmatch` default. It never silently chooses an arbitrary
  playlist. Optimatch loads Playlists; selecting a playlist shows details and
  **Find Match**. The ONLINE lobby's Start action opens that same catalog.
- The client presents joining, searching, players found and connection/admission
  status in original screen210's waiting pane. Back cancels the ticket; leaving
  matchmaking, sign-out, identity expiration and frontend teardown revoke the
  queue session. An interrupted POST retries with the same random request key.
- Screens219/237 use context0 for playlists and context1 for manual server
  discovery. Discovery never creates a matchmaking session or ticket.
- UI rendering uses the launcher-selected imported **Vista HD fonts and bitmap
  pack**, with the existing Xbox menu geometry. No title logo, Discord IDs,
  internal player/ticket IDs, session tokens or casual skill values are displayed.
  Player rows do not claim voice/presence status that the API does not provide.

The native client has one pinned queue origin, separate from `H2_MASTER_URLS`.
It defaults to `https://master.halotoforever.com`; override it before launching:

```sh
H2_MATCHMAKING_URL=https://your-trusted-master.example ./Halo2-Launcher
```

The origin must be HTTPS, with no credentials, query or path. An explicitly
empty `H2_MATCHMAKING_URL` disables matchmaking. There is no automatic queue
failover. This origin receives the existing
Discord access token once to create the session, so configure a trusted operator.
The token and service bearer remain in memory; neither enters menu snapshots,
preferences or logs. A service session lasts15 minutes and rotates automatically
while the Discord authorization remains valid. Server cancellation is best effort
on network loss; unrefreshed queue tickets expire after90 seconds.

The backend requires `H2_MATCHMAKING_CATALOG=/path/to/playlists.json`. The strict
version1 JSON format is demonstrated in
[`matchmaking-lab.json`](../master_server/examples/matchmaking-lab.json).
It configures names, revisions, ranked/casual classification, player and party
limits, weighted map/variant pools, widening stages, explicit skill policy and
provenance. Unsupported or malformed catalogs fail startup; an absent catalog
leaves discovery working and returns503 from matchmaking endpoints.
An explicit four-team offering is supplied in
[`matchmaking-multiteam-native.json`](../master_server/examples/matchmaking-multiteam-native.json).
Select it with `MATCHMAKING_CATALOG=/absolute/path/to/matchmaking-multiteam-native.json`
when applying `compose.production-matchmaking.yaml`, or set `H2_MATCHMAKING_CATALOG`
to that path for a directly run service. It offers twelve players, four teams,
guests and parties up to twelve, using the original TU1.5 assignment/count
kernels and authenticated raw hopper ratings. Its single Lockout/default Team
Slayer pool is an explicit native offering, not a reconstruction of missing
historical rotation weights or downloaded variant buffers. The full-count-only
gathering uses recovered candidate/rank selection; its finite service timeout
is declared native policy. No deployed catalog changes merely by adding this file.
Optional `rank_configuration: "retail-launch-2004"` chooses recovered XP
arithmetic independently of rank visibility. Optional stage field
`unbounded_skill: true` is accepted only on a final casual stage after zero;
ranked, intermediate and malformed unrestricted stages fail startup.
Optional `clan_match: "minor-retail-launch-2004"` or
`"major-retail-launch-2004"` requires its exact lineup dimensions, ranked
visibility and the recovered XP configuration. Public gathering queues use
recovered retail raw-rank candidate windows; the optional later personal-rank
and legacy stage policies are not silently substituted into that selector.
Active production accepts `halo2-native-queue-v7` and `halo2-native-queue-v8`;
v7 adds private service-owned voice block control, while v8 adds preflight
sockets and a required native title-language index. Native source defaults to
v8. All party members and match tickets must use the same compatibility version.
`candidate_selection: "retail-candidates-native-relay-v1"` requires v8 from
all party members and an explicit `gathering` configuration. Use
`{"retail_hopper":0..6}` only with that launch hopper's exact min/max dimensions,
or provide sixteen `countdown_ms` values, `timeout_ms` and `provenance` for
explicit native dimensions. The public example enables the gathering profile;
old catalogs still retain the legacy coordinator. v7 clients see these queues
as ineligible and must update before searching.

For a local two-player lab, run from `multiplayer/master_server`:

```sh
docker compose -f compose.yaml -f compose.matchmaking.yaml build
docker compose --profile tools run --rm init
docker compose -f compose.yaml -f compose.matchmaking.yaml up -d --wait master-a master-b
```

Then launch from the repository root:

```sh
H2_MATCHMAKING_URL=http://127.0.0.1:18081 \
H2_MATCHMAKING_ALLOW_LOOPBACK_HTTP=1 ./Halo2-Launcher
```

Only literal loopback HTTP is allowed with that development opt-in. The lab
contains ranked and casual two-player Slayer and four-player Team Slayer queues
plus launch-based Minor/Major Clan Lab Slayer on Lockout. Names, timing,
high-rank tuning, casual radius and rotations are explicitly custom. It is not
an original playlist catalog. Each human must use their own Discord account;
a second sign-in for the same account rotates the first service session.

## Authenticated queue API

The HTTP queue routes return JSON with `Cache-Control: no-store`. Session creation uses the
existing Discord OAuth access token. Subsequent requests use the returned random
service bearer, stored as a hash server-side. Sessions are not directory-owner
credentials. The backend persists a private Discord-subject → opaque-player
mapping and per-playlist skill in SQLite. New playlist ranks start at1. Clients
cannot write ranks or XP. Only unanimously confirmed, snapshot-validated native
match results advance configured ranked or hidden casual skill in the private
result transaction; see [progression](ranked-progression.md).

| Method and route | Request / response |
|---|---|
| POST `/v1/matchmaking/session` | Discord Bearer; `{screen_name,maps,compatibility}` plus required title `language`0–8 for v8 → `{token,expires_in}` |
| DELETE `/v1/matchmaking/session` | Service Bearer; revoke own session and cancel own ticket |
| GET `/v1/matchmaking/playlists` | Service Bearer; catalog with eligibility and own ranked level; casual level is null |
| POST `/v1/matchmaking/ticket` | Service Bearer; `{playlist_id,revision,request_id}`; UUID request key; no identities or skills |
| GET `/v1/matchmaking/ticket` | Service Bearer; refresh own lease and return own queue/match view |
| DELETE `/v1/matchmaking/ticket` | Service Bearer; cancel own ticket and release its proposal |
| GET `/v1/matchmaking/qos` | v8 service Bearer; WebSocket upgrade; server challenges only, no client-owned targets or measurements |

Queue views contain `{state,playlist_id,queued_players,match}`. A match contains
`{map,variant,players:[{name,team,self}]}`. Names are mutable display text; every
identity comes from the verified session. Installed-map and compatibility values
are reported capabilities, not proof of client integrity or network reachability.
The native bridge enqueues an authenticated solo player or an accepted party.
The server owns membership and leader authority; every member must explicitly
consent to the same playlist/revision. See
[accepted parties](online-party-integration-evidence.md).
Clan catalog rows add `clan_match:true`; a nonmember receives `level:null`
and `eligible:false`. A member sees only their own displayed clan level,
including when their party is too small. Private raw rank0..127 is retained
for selection. One active lineup per clan/playlist serializes progression.
Membership changes cancel a queued clan lineup and its consent; admitted
players finish their clan match before removal/departure. These concurrency
and revocation rules are native service policies.

A grouping without measured connectivity remains `checking_connections` and
cannot pass readiness. The authenticated native relay supplies measurements and
all-member connection admission. The native owner then enters `loading`, transfers
to gameplay, and waits for every matched player to load identical game data and
protocol28 before starting simulation. No public endpoint accepts a client QoS
score or host choice. Unadmitted proposals expire and release waiting tickets
with their original queue age. See [the transport contract](match-launch.md).

## Remaining integration

Unanimous native result agreement is implemented; independent server arbitration
remains unresolved. Original composite candidate scoring, complete dated
playlist/configuration restoration, rejoining,
backfill and active-game host migration remain unfinished. Native
[clans](clans.md), accepted parties and [voice](voice.md) share verified identity.
The [completion audit](online-completion-audit.md) tracks the full remaining scope.
