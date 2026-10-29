# Native matchmaking and coordinator core

The [2026-10-28 master-PDF audit and Rust replacement goal](matchmaking-rust-parity-audit-20261028.md)
records the current twelve parity gaps, reproduced dispatcher and guest-rank
differences, and the requirement that all production matchmaking code move to
Rust. Its source-bound findings and explicit evidence limits take precedence
over older operational descriptions below. Full original-service parity is open.

The current source catalog is the [versioned playlist reconstruction](playlist-hoppers-20261028.md),
with13 regular playlists/463 ordered map/mode rows plus Lab Test's96 rows,
complete native option buffers and exact frequency
manifests. It preserves recovered weights/settings and dated published membership
where available; missing final-2010 weights are explicitly project-tuned under
the user's approval. Its v15 client/library/catalog release has not been deployed
by this work. Dated production milestones below describe their original cohorts.

Rust `matchmaking_coordinator.rs` owns automatic party queue selection and
match proposals separately from manual server discovery. The Python model and
coordinator are temporary typed IO. Rules retain their
[primary-source evidence](halo2-matchmaking-evidence.md); supplied historical
inputs and explicitly declared native replacements stay separate. Missing
original configuration, service ordering and arbitration remain open.

Native **ONLINE → Quickmatch / Optimatch** and the ONLINE lobby's Start →
**Playlists** route now use authenticated HTTP queues. The client reuses the
existing provider-aware authorization; Steam is the current account route and
the retained Discord route validates its application, scope and subject. The
player keeps their chosen local screen name.
The browser remains a separate anonymous directory consumer.

This connects menus, verified identities and accepted parties, stored per-playlist skill, queue
selection, measured connections and native game admission. The
[match launch integration](match-launch.md) owns the loading barrier, authenticated
roster and game voice. Versioned retail-default skill updates apply to ranked
and hidden casual playlists. Clan playlists use accepted clan membership,
intact opposing parties and separate clan-owned progression. Historical
exact final-2010 configuration remains unrecovered. No endpoint accepts client-provided identities, ranks, party
rosters or QoS. There is no second login flow.

## Implemented decisions

- One active ticket per party/player; identity uses opaque internal keys.
  Chosen screen names are display data. Rankings belong to the selected playlist.
- Ranked and casual queues are isolated, versioned policies. Both use skill;
  `ranked` controls eventual disclosure, not whether selection considers skill.
  Queue status never exposes player IDs, rank values or hidden casual skill.
- Configured retail XP policies retain private raw ranks0–127 for search, even
  when several ranks share the final visible level. Original0x72540 averages
  unadjusted raw ranks using integer division. Rust owns raw statistics and the
  original raw-weight assignment path. The retained legacy inferred mixed-party
  floor helper does not feed those original owners.
  Party-size eligibility, compatibility fingerprints and required map ownership
  constrain admission. Policies without recovered XP retain their explicit
  legacy level units; mixed-unit parties/candidates fail eligibility.
- Public v8 queues use actual gathering advertisements and the recovered
  candidate selector. Completed queries widen the raw-rank envelope; outstanding
  probes keep the query pending. The original150-result budget and final casual
  unbounded retry apply. Failed targets remain excluded for the current search.
  Rust owns role/progress state, signed clocks, jitter, the seen-session bank
  and player-count countdowns. Its explicit native provider orders structurally
  admitted sessions by birth/private ID and limits the bank to50; the original
  service ordering remains unrecovered. Relay plumbing remains native policy.
  The legacy coordinator retains its explicit elapsed/reciprocal stage policy
  only for catalogs that do not select the gathering profile. No outcome history,
  engagement target, forced easy/hard sequence or win-rate quota is an input.
- Team formation uses the Rust original hopper caller through the bounded
  service ABI. Complete headers retain type, count, imbalance and retry fields;
  casual type3 can perform its original individual retry. Ranked/clan policies
  retain their admitted party and lineup restrictions. See the
  [assignment evidence and limits](matchmaking-rust-parity-audit-20261028.md).
- Clan launch rules are separate: Minor accepts two equally sized parties of
  3–4; Major accepts 6–8 with a difference up to two. Every member must belong
  to the same accepted clan, and opposing clans differ. Peons may play. Clan
  raw rank drives search, and results update the clan rather than its players.
  See [74 executed retail controls](halo2-clan-matchmaking-evidence.md).
- Host choice requires fresh connection measurements for every remote member.
  Missing/stale measurements fail eligibility. The relay measures fresh challenge
  round trips and converts them to explicit native scores. This measures the
  relay path; it does not recover Xbox QoS or establish direct peer reachability.
- Map and variant entries remain one ordered row. Rust applies the original
  availability filter, 1..1000 signed-weight clamp, LCG and cumulative draw.
  A group needs a common installed row; it does not need every map in the pool.
  Its selected map must remain available to every later gathering member.
  The complete 304-byte settings travel with the row into native configuration
  and the accepted-result options record. Unsafe signed-short totals fail catalog
  admission. See the [executed selector evidence](playlist-hopper-sources-20261028.md).
- Proposals reserve **queue tickets**, not server slots. Every member must
  acknowledge readiness before handoff. A missing member, cancellation or timeout
  releases the other tickets with their original waiting age. Handoff does not
  claim a game started and does not change competitive ranks.

## Policy and evidence boundaries

A best-effort final-era catalog is bundled in
[`matchmaking-online.json`](../master_server/examples/matchmaking-online.json).
Each `Playlist` requires a revision, provenance, game pool, eligibility and search
policy. Recovered executable defaults, published 2005 map/mode pairs, 2007
playlist identities and project tuning remain distinct sources. No complete
final-2010 download or matching numeric weights are claimed. The separate
`matchmaking-development.json` retains small development queues.

| Policy | Current treatment |
|---|---|
| Per-hopper ranks and party average | Configured XP policies use private raw ranks0–127. Recovered retail0x72540 uses integer division over unadjusted available ranks. The display cap applies only to presentation. |
| January 2007 rank extensions, levels1–35 | Published table retained as an optional policy |
| Level36+ | Original final row repeats35 and gives an upward cap50, without a reliable lower bound. A radius must be supplied explicitly; the lab's15 is unverified tuning, not a recovered value. A level50 player's radius is never incorrectly calculated as50−50. |
| Later published-table lookup | The January2007 table is expressed in visible levels. Only its lookup converts a raw integer average to a visible level; candidate distance retains raw units. Mixing this later table with retail XP is an explicit policy synthesis. |
| Mixed party12+1 treated as12+6 | Published example and legacy inferred helper retained. The Rust original assignment and recovered search average consume raw ranks; the inferred helper does not overwrite them. |
| Ranked limits in casual | Never enabled implicitly. Casual requires an explicit policy; the demo/test casual radius is custom tuning. |
| Hidden casual progression and final rank-filter removal | Recovered retail-default XP routines and final query envelope; public level remains null. The lab timer, reciprocal admission and candidate scoring are separate native choices. |
| Significant-party size constraints | Dated table retained for Skirmish, BTB, Double Team, Preview and clan names. Those names do not establish ranked classification or implement clan membership. |
| Multiple significant parties on a team | Conservative one-to-one sorted pairing across teams; an inference beyond the published one-large-party examples |
| Team fairness | Rust original hopper/count/weighted assignment corridor with raw weights and type3-only individual retry; full upstream/publication and version correspondence remain open. |
| Candidate traversal | Gathering queries use the explicitly named Rust birth/private-ID provider and50-record cap. Legacy non-gathering ticket assembly and global work budgets remain separate native policy. Original Live procedure ordering is unrecovered. |
| Host scoring | Best worst-link normalized score, random ties; unverified replacement for the original connection-scoring formula |
| Search cadence, leases and readiness timeouts | Rust uses recovered retail completion/clock/countdown contracts with explicit profile inputs. Native ticket leases and route/allocation waits remain declared replacement timing; dated downloaded overrides remain unknown. |
| Map ownership | Select only rows whose map is reported installed by every group member. This is not client-integrity attestation; complete historical DLC admission rules remain open. |

The recovered retail candidate core in `rust/online_protocol/src/session_candidates.rs` matches264
complete original banks,27 admission cases,124 selections and100 radius
thresholds; the Python module only marshals typed IO. Rust additionally owns
search/gather progress and clocks, new-search generations, the seen bank and
the named native provider ordering in `search_lifecycle.rs`.
The previous frozen lifecycle/backend cohort and its original-control scope are
[recorded here](evidence/matchmaking-rust-parity-20261028/lifecycle-validation.json).
The v8 [native gathering bridge](native-gathering-bridge.md) connects
it to actual partial sessions, stable birth, mapped raw extrema, title language
and server-timed relay-route probes. Original role choice, query exhaustion,
rank widening, peak growth clocks, all seven countdown tables and the150-ID
rejection bank are integrated. Founding hosts remain hosts at gameplay admission.
Historical production release20261012T022410Z enabled this profile in six development queues.
That dated cohort reported four-client ranked/casual and6/14/16-client clan
gameplay/voice/results/requeue checks plus142 backend tests locally and in VPS
Docker. Current Rust library/catalog delivery still requires fresh qualification.
See [candidate evidence](halo2-candidate-scoring-evidence.md),
[lifecycle evidence](halo2-gathering-session-evidence.md),
[follow-up controls](halo2-gathering-lifecycle-followup.md) and
[bandwidth owners](halo2-bandwidth-input-evidence.md). Original service query
ordering remains open. Relay transport, native admission and authored game pools
retain explicit replacement policies. The Rust queue/proposal owner in
`matchmaking_coordinator.rs` now also owns ticket admission and leases, catalog
validation, structural gathering fit checks, legacy subset assembly, measured
host ranking, reservations, readiness and both draw streams. The temporary
Python coordinator facade returns immutable snapshots and performs typed IO.
Tickets pin the complete admitted profile; same-revision catalog changes require
new admission. Selected rows must exactly match the pinned catalog.

The scheduler/host RNG is the explicitly modern v2 stream, separate from the
original hopper draw. Neither the central subset scheduler nor route host
ranking claims original Live service parity. The gathering/query registry and
advertisement projection now also run in `matchmaking_gatherings.rs`. Rust owns
query caches, failure/seen transitions, role and radius progression, game
preservation and proposal publication. Private IO continuations request specific
transport facts/probes; Python executes those requests and returns immutable
snapshots. Canceled or changed admissions retire saved views and continuations.

Remaining authenticated endpoint and accepted-party/clan consent policy, QoS
sensor/transport policy and session/world transitions still require Rust
ownership and native played/delivered acceptance. The
[queue authority audit](matchmaking-rust-parity-audit-20261028.md#rust-queue-and-proposal-authority)
records this increment and its validation boundary.

Result admissions and physical-owner consent now live in Rust
`matchmaking_results.rs`; original default XP arithmetic and skill domains live
in `matchmaking_skill.rs`. Rust freezes queue/profile/roster/guest/clan/skill
facts, creates opaque presentation IDs, validates complete selected304-byte
options and quarantines conflicting uncommitted reports or digests. Unfamiliar
variant names require complete settings; the existing native Fiesta/Infection
assets have exact, separately declared legacy registrations.

`matchmaking_persistence.rs` owns skill schemas, migration guards and generated
SQL for atomic ledger/admission/envelope/XP/PCR/outbox completion. The temporary
Python SQLite adapter performs generic IO on the same transaction/connection.
Every originally admitted physical connection must confirm, including owners
that later depart. Cast votes survive departures; abstentions never become votes.
Result control and SQL pumping remain synchronous on one service event loop.
Original Live arbitration, the effective-to-result comparison-skill producer,
pending-result process recovery, future parallel dispute serialization and the
full executable/map/native-delivery chain remain unqualified. See the
[result/XP authority audit](matchmaking-rust-parity-audit-20261028.md#rust-result-xp-and-transaction-authority).

`matchmaking_catalog.rs` now parses and owns the complete supplied JSON catalog,
normalizes its named policies/retail clock aliases, applies compatibility and
party eligibility gates, selects public rank storage and builds the public
playlist payload. Catalog metadata, required client version and rank-table
identity are included in the same profile pin used by tickets, gatherings,
queries, IO continuations, proposals and result admission. Metadata changes at
the same visible revision require new admission. Invalid replacements preserve
the current registry. Python catalog entries are immutable projections.

`matchmaking_sessions.rs` now owns native authentication sessions, local-seat
validation/renewal, opaque bearer hashes, generations, leases, request UUIDs and
session-bound failure storage. Fresh primary/local provider grants complete
before account writes. Canonical linked accounts, other PC seats and active room
owners are checked by Rust over trusted identity/party/room facts. Unchanged
renewal keeps guest IDs, queue clocks and consent; changed connections rotate and
retire the old queued state. Session revocation also retires Rust gathering/query/
IO state while a running match retains its separate capability.

Whole-party preflight rejects incompatible clients, disallowed guests, excess
seats and missing common maps before skill IO. Final enqueue rechecks native
session generations and every ordered seat/owner/map fact atomically with the
request/ticket publication. The HTTP adapter uses read-only session/request
projections and a typed Rust failure facade. Its weak view cache holds no separate
admitted registry. See the [session authority audit](matchmaking-rust-parity-audit-20261028.md#rust-authentication-session-local-seat-and-request-authority)
and [validation receipt](evidence/matchmaking-rust-parity-20261028/sessions-validation.json).

`matchmaking_parties.rs` now also owns search consent and accepted-party queue
selection. Consent binds party revision/member handle, the full catalog/rank-table
profile, bearer hash/generation and connected roster; clan queues bind accepted
clan identities. Same-revision profile changes or replacement connections require
fresh remote consent. Rust projects valid readiness, emits revision/handle-guarded
SQL plans and issues private permits for final queue publication. Proposals and
result source bindings retain ordered party/session identities. The generic
SQLite adapter executes the native plans inside the caller's transaction.
See the [party consent audit](matchmaking-rust-parity-audit-20261028.md#rust-accepted-party-consent-and-queue-admission)
and [receipt](evidence/matchmaking-rust-parity-20261028/party-consent-validation.json).

`matchmaking_party_countdowns.rs` owns pregame state, serials, original signed
clock/press/parameter updates, requester projection, cancellation, zero
consumption and command replay/SQL plans. It preserves an unchanged mode-1 tuple's
first requester and clock, matching both pinned original writers. Publication
follows the guarded SQL commit; failures abort the unpublished continuation.
Complete profile, accepted roster and native connection generations fence both
staged and live countdowns. Python supplies observations, generic SQL execution
and read-only views. See the [countdown contract](native-party-countdown-tu15.md)
and [validation receipt](evidence/matchmaking-rust-parity-20261028/countdown-validation.json).

Python still supplies provider/account, accepted directory and room observations,
and owns social/clan membership/invitation CRUD, raw network IO and remaining
high-level room/host/world policy.
Retirement acknowledgement/retry and future parallel/distributed fact/SQL/queue
coordination, including crash recovery between countdown SQL and memory commits,
remain unqualified. Those matching decisions still require Rust
migration; generic HTTP/file/authentication/SQLite IO remains temporary.
900-second leases and client compatibility are declared modern rules. These
source checks do not activate a service or qualify a native played match.

`matchmaking_queue_api.rs` owns enqueue skill/roster planning, guest inheritance,
clan rank, poll/legacy/optional views, artwork/appearance selection, cancellation,
room-failure provenance and destroyed-room retry policy. Rust requests skill and
appearance snapshots through one transactional `DatabaseBank`. A failed local
profile migration rolls back the whole snapshot group before queue publication.
Native continuations recheck the actual profile/session/party permit and request.
The HTTP adapter executes the requested IO and returns the native response.

Failure ownership now includes the latest admitted room and session generation.
A late report for a destroyed older room cannot overwrite a newer launch's
failure on the same session. The existing native 15-second retry window remains
bounded and private. Room failure/close hooks delegate to Rust. Running host/world/
transport policy and OS-process crash
recovery remain separate boundaries. See the [queue endpoint audit](matchmaking-rust-parity-audit-20261028.md#rust-queue-endpoints-and-admission-bound-failures)
and [receipt](evidence/matchmaking-rust-parity-20261028/queue-api-validation.json).

`matchmaking_room_setup.rs` now owns the admitted setup roster, rolling RTT and
direct reports, physical connectivity masks, original team readiness, native
QoS/host ranking, acknowledgement, original start clock and loading/running setup
decisions. Python executes its effects and exposes read-only projections. Matching
loader reports cannot bypass the original ten-second clock. A separate real-clock
HTTP control exercises the full interval; component helpers advance their clock
explicitly. Original full-width timer outputs are replayed against the unchanged
TU1.5 blocks; full native-client outlier-domain and played-world acceptance remain
open. See the [room setup audit](matchmaking-rust-parity-audit-20261028.md#rust-room-setup-readiness-qos-and-start-clock)
and [receipt](evidence/matchmaking-rust-parity-20261028/room-setup-validation.json).

`matchmaking_continuity.rs` now owns the legacy v11 relay's physical ownership,
authority epochs, bounded immutable completed-tick seals, standby proofs,
candidate selection, acceptance, rebinds and expiry. The setup owner installs it
from its frozen roster and observed host only after the source start gate.
Python attaches a read-only view and forwards authenticated controls. The Rust
result owner suppresses late world proofs after a report arrives, and the result
database bridge obtains its finalization epoch from the same native room owner;
caller-supplied epochs are rejected before SQL. Packet/expiry state readers do
not copy world-proof history. Election ordering and timeout remain declared native
policy. Peer digests do not establish actual world import or original host order.
High-level review/postgame, departure and transport effects, direct-peer native
continuation, and full played/delivered acceptance remain open. See the
[continuity audit](matchmaking-rust-parity-audit-20261028.md#rust-legacy-relay-complete-world-coordination)
and [receipt](evidence/matchmaking-rust-parity-20261028/continuity-validation.json).

The Rust room owner also keeps phase, host/PCR authority, lifecycle deadlines, departure
masks, review completion and postgame grants in `matchmaking_room_lifecycle.rs`.
Python room fields are read-only native projections. Submission and confirmation
adapters use the same native phase/authority checks as the socket route. A failed
result transaction leaves review pending; native postgame publication follows
successful finalization. TCP loss alone preserves direct UDP admission, while
explicit leave and relay loss use their native policies. Frozen ticket IDs own
cleanup. Room clock observations are separate from account/queue clocks, and
expired rooms cannot publish a late report before writer cleanup. These remain
declared native lifecycle policies; see the [lifecycle receipt](evidence/matchmaking-rust-parity-20261028/room-lifecycle-validation.json).

The Rust room owner also handles bearer generation/rotation, authenticated upgrade and
connection generations, immutable public descriptors, EOS room proof/identity
binding and peer signaling to `matchmaking_room_wire.rs`. Expired rooms fail
before WebSocket upgrade. An old reader cannot detach a newer connection.
Rust owns signaling directions, protocol/physical-recipient guards, counters,
description uniqueness and queued messages. Signaling snapshots carry bounded
counts; pending payloads drain in batches of at most16, with retired senders
filtered out. Python supplies socket/backpressure and verified provider facts,
performs provider storage IO and executes native publications. See the
[room wire receipt](evidence/matchmaking-rust-parity-20261028/room-wire-validation.json).

`matchmaking_room_packets.rs` now owns relay framing, loaded native protocol
binding, physical sender/recipient roles, continuity epochs, postgame traffic,
voice key generation and proof verification, signed-local/guest privacy masks
and queued delivery. Rust selects account pairs for privacy lookup; Python
supplies account-service facts and executes the resulting socket effects.
Queued frames carry both native connection generations and recheck them before
delivery. No mutable Python roster or standalone limiter can replace these
native grants. `matchmaking_packet_budget.rs` owns the per-connection control,
game-destination and speaker/destination budgets, including denied-audio charging
and monotonic refill. These are modern H2N2/Ed25519/abuse policies, with the
existing voice envelope; original Xbox wire/voice coefficients are unrecovered.
See the [packet ownership receipt](evidence/matchmaking-rust-parity-20261028/room-packets-validation.json).

`matchmaking_room_control.rs` now owns raw room text decoding, including
duplicate decoded-key rejection, dispatch, connection-bound challenge nonces,
monotonic RTT clocks, probe expiry, presence-seat eligibility and publications.
Rust selects result/completion/voice recipients from frozen connection grants
and socket features, and emits bounded socket/queue/retirement/transaction
effects. Python executes those effects. Detached socket generations are recorded
and consumed in Rust; an old callback cannot end a replacement connection.
Late pongs do not renew an expired probe or supply another QoS sample. A direct
game's UDP membership survives this transport failure. The existing0.2/5-second
probe cadence and15-second deadline remain explicit native policy.
See the [room control receipt](evidence/matchmaking-rust-parity-20261028/room-control-validation.json).

Remaining endpoint/service replacement, search QoS allocation/probe sensors,
population/history readers, generic provider/account/storage IO, cross-store
recovery and full native played/delivered/original acceptance remain open.

The original service used distributed searching/gathering parties. This core
replaces that plumbing with one serialized authoritative Rust queue owner.
The private ABI serializes individual owner commands; multi-call service
workflows still run on the existing single event loop. Queues and proposals are ephemeral, expire locally and
are never federated as directory records. After process failure, players must
requeue with the same dated catalog; no cross-master reservation consistency or
exactly-once gameplay allocation is implied.

## Run and verify

From the repository root, with the existing master requirements installed:

```sh
python3 -m multiplayer.master_server.matchmaking_demo
python3 tools/run_test.py -- python3 -B tools/build_online_playlists.py --check
python3 tools/run_test.py -- python3 -B tools/validate_playlist_hoppers.py --output build/test-results/playlist-hoppers
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
- UI rendering uses launcher-selected **MCC-imported fonts and bitmap assets**;
  OG TU1.5/Vista remain references. No title logo, account IDs,
  internal player/ticket IDs, session tokens or casual skill values are displayed.
  Player rows do not claim voice/presence status that the API does not provide.

The native client has one pinned queue origin, separate from `H2_MASTER_URLS`.
It defaults to `https://master.halo2forever.net`; override it before launching:

```sh
H2_MATCHMAKING_URL=https://your-trusted-master.example ./Halo2-Launcher
```

The origin must be HTTPS, with no credentials, query or path. An explicitly
empty `H2_MATCHMAKING_URL` disables matchmaking. There is no automatic queue
failover. This origin receives the existing
account authorization once to create the session, so configure a trusted operator.
The token and service bearer remain in memory; neither enters menu snapshots,
preferences or logs. A service session lasts15 minutes and rotates automatically
while the provider authorization remains valid. Server cancellation is best effort
on network loss; unrefreshed queue tickets expire after90 seconds.

The backend requires `H2_MATCHMAKING_CATALOG=/path/to/playlists.json`. The strict
version1 JSON format is demonstrated in
[`matchmaking-online.json`](../master_server/examples/matchmaking-online.json).
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
The new complete-option catalog requires `halo2-native-queue-v15`, the current
EOS account-mode source default. The service retains older compatibility routes
for separate catalogs; an older client cannot enter these new hoppers. Native
game datagrams use protocol119 to preserve full options between peers. All
party members and match tickets must use the same compatibility version.
`candidate_selection: "retail-candidates-native-relay-v1"` requires v8 from
all party members and an explicit `gathering` configuration. Use
`{"retail_hopper":0..6}` only with that launch hopper's exact min/max dimensions,
or provide sixteen `countdown_ms` values, `timeout_ms` and `provenance` for
explicit native dimensions. The public example enables the gathering profile;
old catalogs still retain the legacy coordinator. v7 clients see these queues
as ineligible and must update before searching.

For the local reconstructed catalog, run from `multiplayer/master_server`:

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

Only literal loopback HTTP is allowed with that development opt-in. Compose
selects the reconstructed catalog by default. Lab Test starts at two players;
the regular playlists retain their normal population bounds. For small service-only checks explicitly mount
`examples/matchmaking-development.json` instead. Each independently signed-in
player uses their own provider account; additional local seats can be guests.

## Authenticated queue API

The HTTP queue routes return JSON with `Cache-Control: no-store`. Session creation uses the
existing provider authorization. Subsequent requests use the returned random
service bearer, stored as a hash server-side. Sessions are not directory-owner
credentials. The backend persists a private provider/subject → opaque-player
mapping and per-playlist skill in SQLite. New playlist ranks start at1. Clients
cannot write ranks or XP. Only unanimously confirmed, snapshot-validated native
match results advance configured ranked or hidden casual skill in the private
result transaction; see [progression](ranked-progression.md).

| Method and route | Request / response |
|---|---|
| POST `/v1/matchmaking/session` | Provider Bearer; `{screen_name,maps,compatibility,language}` and optional authenticated local seats/appearance → `{token,expires_in}` |
| DELETE `/v1/matchmaking/session` | Service Bearer; revoke own session and cancel own ticket |
| GET `/v1/matchmaking/playlists` | Service Bearer; catalog with eligibility and own ranked level; casual level is null |
| POST `/v1/matchmaking/ticket` | Service Bearer; `{playlist_id,revision,request_id}`; UUID request key; no identities or skills |
| GET `/v1/matchmaking/ticket` | Service Bearer; refresh own lease and return own queue/match view |
| DELETE `/v1/matchmaking/ticket` | Service Bearer; cancel own ticket and release its proposal |
| GET `/v1/matchmaking/qos` | v8 service Bearer; WebSocket upgrade; server challenges only, no client-owned targets or measurements |

Queue views contain `{state,playlist_id,queued_players,match}`. A match contains
`{map,variant,players:[{name,team,self}]}` for older clients. v15 additionally
binds `game_id`, `settings` and `settings_sha256`; authored Fiesta/Infection
retain their separately bundled native extension definitions. Names are mutable display text; every
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
the same native protocol before starting simulation. No public endpoint accepts a client QoS
score or host choice. Unadmitted proposals expire and release waiting tickets
with their original queue age. See [the transport contract](match-launch.md).

## Remaining integration

Unanimous native result agreement is implemented; independent server arbitration
remains unresolved. Original composite candidate scoring, complete dated
playlist/configuration restoration, rejoining,
backfill and active-game host migration remain unfinished. Native
[clans](clans.md), accepted parties and [voice](voice.md) share verified identity.
The [completion audit](online-completion-audit.md) tracks the full remaining scope.
