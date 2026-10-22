# EOS offload assessment and accepted data ownership

Verified 2026-10-22 against Epic's public documentation and the supplied EOS SDK
1.19.2.1 headers. This is an architecture decision and feasibility review, not a
claim that every service below is integrated or that production data has moved.

The accepted split is **EOS for general player statistics and the top 25 players
in each playlist; our master for full carnage reports and each player's match
history**. Keep Halo's playlist, party, eligibility and rank rules. A custom Halo
profile is our game/website UI combining identity, EOS statistics and master
history; it is not arbitrary fields added to an Epic account's public profile.

## Verified capabilities

| Workload | Reasonable EOS ownership | What remains ours |
| --- | --- | --- |
| Peer connections | NAT traversal, direct P2P and relay fallback | Native protocol, admission, simulation, host selection/migration |
| Game discovery | Sessions or lobbies, invitations, searchable game attributes | Halo queue selection, widening skill search, party/team/clan rules |
| Party synchronization | Lobby membership, events and optional EOS voice | Halo-specific party rules, permissions and existing provider bridges |
| Playlist delivery | Title Storage files containing versioned playlist/configuration data | Preparing verified definitions and executing their rules |
| General player statistics | EOS Stats, queried by Product User ID | Validating results and publishing trusted, recoverable updates |
| Playlist top 25 | EOS Leaderboards backed by selected stats | Defining the metric and rendering/caching the first 25 rows |
| Complete carnage reports | Player-private copies are possible, but not a shared public archive | Accepted owner: master report store and public read API |
| Player match history | Private per-user file manifests are possible | Accepted owner: master chronological index and pagination |
| Player identity | EOS Connect account linking; optional Epic Account Services | Existing account migration, public aliases, custom Halo profile fields |

The first four rows follow Epic's [P2P documentation][p2p],
[lobbies/sessions comparison][lobbies], and [Title Storage overview][title].
These are service capabilities, not a turnkey Halo implementation. Sessions
support attributes and filtered discovery; the documented API does not execute
our rank exchange tables or custom party-balancing algorithm. That final
boundary is an inference from the API and the existing matcher.

## Why leaderboards cannot index complete matches

The SDK's `EOS_Leaderboards_LeaderboardRecord` contains a Product User ID, rank,
signed 32-bit score and display name. It has no match ID, JSON payload, roster,
map or custom metadata fields. A leaderboard ranks a user's aggregate statistic
over a configured interval; it is not an append-only collection of that user's
matches. See [the record schema][record] and [leaderboard setup][setup-lb].

Current published limits are 1,000 global entries plus 1,000 overflow entries,
and 100 unique start/end time milestones across the game's leaderboards in a
deployment. Reusing times shares milestones; an unending board uses its start
milestone. These limits readily accommodate displaying a top 25, but do not
describe an unlimited history store. [Leaderboards reference][lb-limits]

Encoding a match reference into a score would still leave one player aggregate,
without a report payload or a way to enumerate that player's individual match
records. Creating a definition for every match also depends on developer-managed
definitions and bounded milestones. **Neither is a viable replacement for our
match/player/date index.** This conclusion is an architectural inference from
the documented schema and operations, not a claim about an undocumented API.

For a playlist's skill board, publish a dedicated trusted stat for that
playlist. Decide whether the ordering is visible level, underlying XP/raw rank,
or another explicit metric; visible level alone creates many ties. The current
Halo rank/XP arithmetic remains authoritative. Global board queries return the
SDK's ranked results; showing 25 means selecting the first 25 cached records,
not assuming a special 25-row server API.

## General statistics and profiles

EOS Stats supports `SUM`, `LATEST`, `MIN` and `MAX` aggregation, and the SDK
ingests and returns signed 32-bit integer values. It can represent kills,
deaths, assists, matches and verified shot/hit counters. Store the underlying
counters, then calculate lifetime K/D from total kills/deaths and accuracy from
total hits/shots; do not average per-match percentages. A ratio leaderboard
would need a separately published integer/fixed-point ratio stat and an explicit
minimum-sample policy. EOS does not infer that formula. [Aggregation semantics][aggregation]
[SDK ingest value][ingest-data] [SDK returned value][stat]

Plan a stat-name budget: the published limit is 500 definitions per deployment.
The current carnage representation has 45 general slots, 32 medal slots and
45 × 8 weapon fields per player: 437 fields before playlist duplication or
opponent records. Mirroring every report field as lifetime/per-playlist stats
would exhaust the budget quickly. Start with a deliberately selected set of
general counters and playlist skill values. [Stats reference][stats-limits]
[Current history schema](../master_server/carnage_history.py)

Use the original source-backed counter owners when mapping hits/shots. Existing
PCR presentation already computes accuracy from its consumed hit/shot fields;
unknown report indices must remain unknown. A signed 32-bit lifetime counter
also needs an explicit overflow/season policy before its limit is reached.
[PCR presentation](../../src/vista_pcr_screen.cpp)

The Epic User Info service exposes account information such as display name and
requires an Epic Account Services application plus basic-profile consent.
Remote users' exposed information is restricted for privacy. Its documented
schema is not a writable container for Halo ranks, emblems or match arrays.
Use EOS Stats for game statistics and our own UI for the combined profile.
[User Info documentation][userinfo]

EOS Connect provides product-specific user identities and links external login
providers; an Epic account is not required for EOS Game Services. Before using
EOS as durable progression storage, bind each authenticated main player's
existing identity to a stable, account-linked PUID. Device ID alone can lose all
progress if its local credential/profile is lost. Guests must stay excluded
from persistent personal stats, as they are today. One connection's transport
PUID must not silently become the identity of every signed-in couch player.
[Connect reference][connect]

The supplied SDK explicitly supports Discord access tokens with
`EOS_Connect_Login`, in addition to Steam and Epic credentials. Existing Discord
identity therefore need not force a new Epic account. However, the current
Device ID transport login does not establish that association: verify Discord
provider/application configuration, token acquisition/audience and refresh flow
before claiming our existing login tokens can be reused. Do not merge accounts
or discard an existing PUID's progression implicitly.
[External credential types][credentials]

## What the storage services actually offer

**Player Data Storage:** encrypted files of any format, accessible to the same
signed-in user across devices. Files are private to each user. Current limits:
400 MB total per user, 1,000 files per user, 200 MB individual upload, and
1,000 read/write requests per minute. The documentation also describes
400 MB auto-delete handling; do not use that as an increased supported upload
size. Querying a user's file list returns file metadata, not a cross-user match
index. [Player Data Storage overview][pds] [Title/PDS access distinction][title]

A player could keep their own report JSON plus a private manifest, with file
rotation or chunking. This does not give a public website access to another
user's files, and user-writable files cannot replace a trusted rank ledger.
Under the accepted split, there is no need to build this duplicate archive now.

**Title Storage:** publisher-managed files common to the product's players,
with tag queries and optional overrides. Current limits are 10 GB and 100 files.
The client SDK queries/downloads files uploaded through the Developer Portal;
it has no upload/append method. Files can be replaced by the publisher, so this
is not immutable storage, but it is appropriate for relatively static playlist
catalogs and configuration. It is unsuitable as an automated file-per-match
history database. [Title Storage overview][title]
[Portal file management][title-management]

## Trusted publication and website access

Use a separate server-only client policy/credential for accepted stat writes.
The untrusted game client should receive only the reads it needs; do not enable
arbitrary-user stat ingestion there. Epic explicitly warns against distributing
trusted-server credentials in a public game build. [Client-policy guide][policy]

Server-side access is supported by the public ABI: Stats ingest/query options
allow a null `LocalUserId` for dedicated servers, while still specifying the
target player's PUID. Leaderboard query options similarly do not use a local
user for dedicated servers. The predefined `TrustedServer` policy does not
require an authenticated player; the current policy reference also requires an
IP/CIDR allowlist for trusted servers. Configure a separate server client with
only needed stat/leaderboard actions, not the existing user-required P2P client.
Use server platform initialization (`bIsServer=EOS_TRUE`) and its client
credentials. This verifies the supported model, **not a live successful ingest**
from our deployment; server initialization, permissions and read-back still
need a sandbox prototype. [Client-policy reference][policy-reference]
[Stats ingest options][ingest-options] [Stats query options][query-options]
[Leaderboard ranks options][rank-options] [Platform options][platform-options]

Our master currently commits report history, result summaries and player/clan
skill in one SQLite transaction, deduplicated by match ID and report digest.
It requires the admitted physical clients' report agreement. This is consistency
validation, not independent server simulation or anti-cheat proof.
[Result ledger](../master_server/match_results.py)
[Player skill](../master_server/playlist_skill.py)
[Clan skill](../master_server/clan_skill.py)

Retain that compact trusted journal and add an outbox for EOS publication.
`EOS_Stats_IngestStatOptions` has no match-ID/idempotency-key field, and a
successful completion only means the ingest reached the service, not that all
processing succeeded. Blindly retrying `SUM` after an ambiguous outcome risks
double-counting. Design deduplication, serialized publication and reconciliation;
absolute snapshots or monotonic aggregates have different correction/ordering
tradeoffs. EOS does not offer our existing multi-table transaction across
reports, rank state and several players' counters.
[SDK ingest request][ingest-options] [SDK completion guarantee][ingest-complete]

No documented Stats, Leaderboards, Player Data Storage or Title Storage HTTP
interface was found in the current official Web API reference. Epic directs
clients/servers toward the SDK and forbids directly calling undocumented HTTP
endpoints. Plan a small Rust service using the existing generated CXX/official
SDK boundary, with bounded cached results exposed to the TypeScript website.
Do not put trusted credentials in browser JavaScript or query EOS afresh for
each page visitor. This service layout is our recommendation, not an EOS-hosted
serverless function feature. [Web API introduction][webapi]

## Cost implications and remaining verification

Epic currently advertises EOS services without royalty or hosting fees. This
does not remove the documented quotas or guarantee unlimited file retention,
unlimited request rates, dedicated game-server compute, custom application
execution or a general-purpose public database. [Official EOS pricing statement][pricing]
[Service throttling/quota behavior][quotas]

The accepted split can move most reusable online-service functions onto EOS,
especially relay fallback, optional voice, directory/lobby state, general stat
reads and leaderboard maintenance. It still needs a small trusted master for
account associations, Halo matchmaking/rank logic, immutable result acceptance,
the public match-history index and report payloads. Existing v12 direct-peer
gameplay already bypasses master media relay, so EOS does not remove an existing
master media stream on that path. Measure before promising a savings percentage.
[Existing peer design](direct-player-hosted-online.md)
[Current transport](../master_server/match_transport.py)

For storage planning, measure serialized report bytes, SQLite pages/index/WAL
growth per match, backup copies and public-history egress. Multiply measured
bytes/match by matches/month and retention; do the same for request counts and
peak concurrent searches. Native PCR input is 56,440 bytes, but current storage
normalizes sparse fields rather than retaining that raw blob, so this input size
is not a measured database growth figure.
[History ownership and existing public routes](carnage-history.md)

Before any data cutover:

1. Verify account-linked PUID mapping for each main couch player, guests excluded.
2. Create sandbox stat definitions and playlist leaderboard definitions with a
   written name/type/overflow budget; retain trusted writes on the server only.
3. Test accepted-report publication, ambiguous retry, duplicate report,
   out-of-order update, read-back and outage recovery without duplicate awards.
4. Test top-25 queries, tied ranks, absent/new players, per-playlist isolation,
   cache freshness and request throttling using real EOS sandbox accounts.
5. Verify website profile/history joins with existing public aliases and the
   new PUID mapping; back up existing data and define rollback before migration.

These are pending acceptance criteria. This review created no new credentials,
changed no service permissions and migrated no player data. Relay/Sessions
integration is a separate implementation track; capability verification here
does not certify its production deployment or full gameplay acceptance.

## Evidence provenance

Official public pages were retrieved on 2026-10-22. The supplied archive was read
in place, without retaining extracted SDK copies for this review:

`EOS-SDK-58105819-Release-v1.19.2.1.zip`

SHA-256: `56a3bd805df426606946d74ba662f25223ffe24159e7884a625225ba80b582fb`.

Checked `SDK/Include/eos_leaderboards_types.h`, `eos_stats_types.h`,
`eos_playerdatastorage_types.h`, `eos_userinfo_types.h`, `eos_connect.h`,
`eos_common.h` and `eos_types.h`.
Published docs and SDK occasionally describe different ingest batch ceilings
(the SDK allows 3,000; the current stats rate table also mentions 500/request).
Use conservative small batches and verify the deployment's effective quotas
before launch rather than treating either as proof of unlimited ingestion.

[p2p]: https://dev.epicgames.com/docs/epic-online-services/multiplayer/nat-p2p-interface/p2p-overview
[lobbies]: https://dev.epicgames.com/docs/epic-online-services/multiplayer/lobbies-and-sessions/lobbies-and-sessions-introduction
[record]: https://dev.epicgames.com/docs/api-ref/structs/eos-leaderboards-leaderboard-record
[setup-lb]: https://dev.epicgames.com/docs/epic-online-services/player-and-game-data/leaderboards-interface/leaderboards-guide/set-up-leaderboards
[lb-limits]: https://dev.epicgames.com/docs/epic-online-services/player-and-game-data/leaderboards-interface/leaderboards-reference
[aggregation]: https://dev.epicgames.com/docs/glossary#aggregation-type
[ingest-data]: https://dev.epicgames.com/docs/api-ref/structs/eos-stats-ingest-data
[stat]: https://dev.epicgames.com/docs/api-ref/structs/eos-stats-stat
[stats-limits]: https://dev.epicgames.com/docs/epic-online-services/player-and-game-data/stats-interface/stats-reference
[userinfo]: https://dev.epicgames.com/docs/epic-online-services/accounts-and-social/eos-user-info-interface
[connect]: https://dev.epicgames.com/docs/epic-online-services/eos-fundamentals/connect-interface/connect-reference
[credentials]: https://dev.epicgames.com/docs/api-ref/enums/eos-e-external-credential-type
[pds]: https://dev.epicgames.com/docs/epic-online-services/player-and-game-data/player-data-storage-interface/player-data-storage-overview
[title]: https://dev.epicgames.com/docs/epic-online-services/player-and-game-data/title-storage-interface/title-storage-overview
[title-management]: https://dev.epicgames.com/docs/epic-online-services/player-and-game-data/title-storage-interface/title-storage-management
[policy]: https://dev.epicgames.com/docs/epic-online-services/eos-fundamentals/client-and-client-policy/client-policy-guide
[policy-reference]: https://dev.epicgames.com/docs/epic-online-services/eos-fundamentals/client-and-client-policy/client-policy-reference
[ingest-options]: https://dev.epicgames.com/docs/api-ref/structs/eos-stats-ingest-stat-options
[query-options]: https://dev.epicgames.com/docs/api-ref/structs/eos-stats-query-stats-options
[rank-options]: https://dev.epicgames.com/docs/api-ref/structs/eos-leaderboards-query-leaderboard-ranks-options
[platform-options]: https://dev.epicgames.com/docs/api-ref/structs/eos-platform-options
[ingest-complete]: https://dev.epicgames.com/docs/api-ref/structs/eos-stats-ingest-stat-complete-callback-info
[webapi]: https://dev.epicgames.com/docs/web-api-ref/web-api-introduction
[pricing]: https://onlineservices.epicgames.com/
[quotas]: https://dev.epicgames.com/docs/epic-online-services/working-with-the-eos-sdk/conventions-and-limitations#service-usage-limitations
