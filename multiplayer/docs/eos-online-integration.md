# EOS native transport and session handoff

Updated 2026-10-22. This change adds an opt-in EOS path to the existing native
multiplayer client and master. It preserves Halo playlist, party, team, clan,
rank and measured host-selection rules. Production now accepts the EOS v13
match handoff. Client implementation and source compilation are complete, but
automatic launcher installation is blocked by the map checks recorded below.

## Implemented ownership

| Owner | Responsibility |
| --- | --- |
| Rust `online_protocol::eos_wire` | Bounded framing/reassembly and pair-specific admission messages |
| Rust `online_protocol_bridge::eos_transport` | Configuration, Connect login state, roster/PUID binding, retry deadlines, session validation and admission |
| C++ `eos_sdk.cpp` | Official EOS SDK calls and callbacks using vendor headers; generated CXX ownership boundary |
| Native `OnlinePeerTransport` | Existing gameplay/voice datagram interface using either EOS or the original direct transport |
| Master `match_transport.py` | Authenticated room descriptor, one-time PUID exchange, Halo proposal/phase/result control |
| EOS | Connect device identity, session record/invitations/joining, encrypted peer connections, NAT traversal and relay fallback |

`halo2-native-queue-v13` is isolated from the existing v12 direct pool. The master
issues a random room socket and independent 256-bit key per admitted physical
pair. PUIDs are bound once to authenticated physical slots and cannot be changed
or reused across slots. Couch guests retain their original admitted seats; they
do not create independent device identities.

The smallest admitted physical slot creates the private EOS session. This
directory owner is independent of the simulation host selected from measured
quality. It registers itself before inviting peers, matching the official SDK
sample's lifecycle. Invitees verify the session ID, owner and all playlist,
revision, map and variant attributes before joining. Authenticated peers signal
their completed join; the directory owner registers them before admitting their
datagrams. Setup can recover cached invitations, with bounded polling that
stops once joined.

Only one endpoint initiates each P2P pair. Both sides still accept only identities
already admitted by the master. This avoids simultaneous offer negotiation seen
in the live qualification. The SDK chooses direct UDP when possible and can use
Epic relay infrastructure otherwise. A diagnostic force-relay setting proves
the fallback path without pretending two local processes represent consumer
networks.

EOS permits 1,170 bytes per packet. The Rust layer splits native datagrams of up
to 1,404 bytes into at most two packets and reassembles them without introducing
ordered/reliable delivery. Partial data expires after 250 ms, pending sequences
are capped at 64 per peer, duplicate completed sequences are bounded, and queues
are bounded. Gameplay and existing voice datagrams bypass the master on this
path. This is EOS P2P relay, not generic TURN credentials for libdatachannel.

Sessions provide admission/directory state here. The master still groups Halo
queue tickets and owns control, rank calculations and report acceptance. This
does not claim that Epic now executes our matchmaking algorithm. EOS Sessions
do not migrate their directory owner automatically; existing simulation host
migration is a separate owner and future session rediscovery/rejoin needs its
own acceptance coverage. See [Epic's Sessions guide][sessions].

## Configuration and credentials

Builds can use `HALO2_EOS_SDK_ROOT` pointing at the official `SDK` directory, or
`HALO2_EOS_SDK_ARCHIVE` pointing at the official ZIP. The supplied
`EOS-SDK-58105819-Release-v1.19.2.1.zip` is detected when present. Headers and the
selected platform library are extracted only into the build tree. The native
target bundles the EOS runtime next to its executable. Linux x64 is tested;
Windows, Linux ARM64 and universal macOS selection are configured but untested.

Select the path with `H2_ONLINE_TRANSPORT=eos`; `direct` retains v12. The default
uses the packaged `eos-client.env` beside the executable when present. Its
`H2_EOS_ENABLED=1` selects EOS, and `H2_EOS_PROFILES=1` enables account linkage.
Unconfigured development builds retain the existing direct path.
`H2_EOS_FORCE_RELAY=1` is a diagnostic override, not the normal routing policy.

Rust reads these environment settings, the optional `H2_EOS_CONFIG` file, or
the packaged `eos-client.env` beside the executable
(plain `KEY=VALUE`; never shell evaluation):

```text
H2_EOS_PRODUCT_ID=...
H2_EOS_SANDBOX_ID=...
H2_EOS_DEPLOYMENT_ID=...
H2_EOS_CLIENT_ID=...
H2_EOS_CLIENT_SECRET=...
H2_EOS_CACHE_DIR=/absolute/persistent/device/cache
```

Environment settings override file values. Without a cache override, Rust uses
`$XDG_DATA_HOME/halo2-native/eos` (or `~/.local/share/halo2-native/eos`) on Linux,
`~/Library/Caches/halo2-native/eos` on macOS, and
`%LOCALAPPDATA%/halo2-native/eos` on Windows. Cache/device identity state belongs
on the player's PC. These product/client settings are shared application
configuration, not a different credential issued to each player. Device ID is
currently used for transport only; durable profile progression requires explicit
account linkage before publication.

The first five settings are saved as GitHub Actions secrets in
[`TheRunaway5/Halo-2-PC-Website`][github-secrets], as requested. An ignored local
configuration exists at `.local/eos-setup/client.env` with mode `0600`. It must
never be committed or included in logs. GitHub secrets do not automatically
configure a running game. A qualified Linux launcher build can package the
settings and SDK using:

```bash
python3 tools/build_launcher_engine.py \
  --eos-client-config /absolute/private/client.env --jobs 8
```

The packager strips the build machine cache path, rejects server-only settings,
and enables transport/profile linkage in the delivered configuration. Runtime
promotion requires the existing launcher routing and submitted-frame checks.
The trusted publisher credential is never included.

The created client uses the user-required **H2Forever P2P** policy with the
approved Sessions actions. Stats, storage and rank-writing access remain
disabled. The deployed trusted stats publisher uses a separate server-only policy and
credential; arbitrary-user stat writes remain unavailable to the game client.

## Accepted profile and history split

The concrete feasibility review is
[EOS offload assessment](eos-offload-assessment-20261022.md), including current
schemas, storage quotas, stat budgets, identity requirements and official sources.

| Data/service | Accepted destination | Current delivery |
| --- | --- | --- |
| Peer NAT traversal and relay fallback | EOS | Native live qualification passed |
| Private match session/invites/join | EOS | Native + master handoff qualification passed |
| Halo matchmaking/rank/party rules | Master | Retained |
| General verified lifetime counters | EOS Stats | Trusted publisher/outbox deployed; real-player linkage acceptance pending |
| Top 25 per playlist | EOS Leaderboards | Nine definitions and public cached read path deployed |
| Full carnage reports and each player's match index | Master | Retained |
| Combined Halo profile view | Game/website joins identity + EOS stats + master history | API and website deployed; delivered-client acceptance pending |
| Versioned static playlist catalog | EOS Title Storage | Feasible future delivery offload; rules remain ours |

The master retains its deduplicated accepted-result journal and rank transaction.
The website-owned Rust publisher drains a recoverable outbox and checks SDK
read-back before acknowledging a revision. Lifetime counters use absolute MAX
values; mutable ranks and appearance use LATEST. No client can award its own
rank or statistics. A real player's first account-linked publication still
requires delivered-client acceptance.

## Validation and remaining acceptance

Run live tests explicitly with a private configuration:

```bash
H2_EOS_CONFIG=/absolute/private/client.env \
H2_EOS_LIVE_TEST=1 H2_EOS_TEST_SESSIONS=1 \
python3 tools/run_tests.py \
  --target online_eos_transport_tests --target peer_mesh_quality_session_tests \
  --filter '^(online_eos_relay_live|online_eos_matchmaking_live)$' \
  --jobs 10 --test-jobs 1 \
  --retain-report test-artifacts/eos-relay-live.json \
  --retain-report test-artifacts/eos-match-quality/native-session-results.json \
  --output build/test-results/eos-admission-current
```

Ordinary offline CTest skips those tests with code 77. Generated test device
identities, caches, descriptors, binaries and SDK extraction belong to disposable
test scratch. Retained evidence contains only small reports/logs, not credentials
or private descriptors. The original SDK ZIP is preserved as a reference input.

Verified locally:

- Rust protocol suite: 18 tests passed, including malformed, unordered, lost,
  duplicate and pair/session admission cases.
- Master Python suite: 257 tests passed on 2026-10-22.
- Five native checks passed in `build/test-results/eos-final-admission`: legacy
  measured session, legacy direct UDP, native matchmaking client, live EOS
  forced-relay/session echo, and live EOS master/session admission.
- Live admission uses two real native processes/EOS device identities and the
  real local master API. Discord authorization is a test fixture. It measures
  host quality and admits eight seats across two physical peers, then exchanges
  native datagrams beside control telemetry. It does not inject a host score or
  a native prepared-world certificate.

The retained JSON reports live under the corresponding
`build/test-results/.../evidence/test-artifacts/` directories. These checks do not
establish two-consumer-NAT behavior, a playable game/world, full WAN voice or
host-migration acceptance, launcher packaging, production deployment, or the
future stats/profile publishing path. Those remain separate acceptance work.

[sessions]: https://dev.epicgames.com/docs/epic-online-services/multiplayer/lobbies-and-sessions/sessions-interface/sessions-intro
[github-secrets]: https://github.com/TheRunaway5/Halo-2-PC-Website/settings/secrets/actions

## Account profiles and website ownership (2026-10-22)

The website repository now owns `backend/eos-worker` (Rust + thin official SDK
ABI), `scripts/eos-service.ts`, the EOS cache/publication scheduler, stat manifest
and container/compose integration. Its worker runbook describes configuration,
identity trust, publication reconciliation, rollback and removal criteria.

The master adds owner-only `/v1/account/profile`, account-purpose EOS token/proof
routes and public `/v1/history/players/{public_id}/profile`. Current appearance is
exactly the nine-byte `FrontendAppearance`: four signed colors, Spartan/Elite,
and all four emblem bytes. A separate four-character service tag is saved with
the local Halo profile and synchronized on change. New account join dates are
server-issued and stable; migration does not invent dates for older accounts.
Historical PCR appearance and detailed match records remain immutable.

EOS profile identity is gated by `H2_EOS_PROFILES=1`; it requires the configured
proprietary OpenID provider and public master JWKS endpoint. It does not promote
a PC's device ID into permanent account ownership. Server SDK proof verification
checks provider/account, game client, product, sandbox, deployment and PUID.
Trusted stat credentials are separate from the user-required game client.

The new website route `/v1/history/top25?playlist=...` reads cached EOS standings.
The previous website SQL leaderboard and its sort/pagination implementation were
removed locally. Halo skill calculations, result validation and carnage/history
SQL are still required. Legacy direct-peer transport remains in use by the
current default deployment; removing it requires EOS cutover and actual runtime
acceptance first. The master/profile/population API and website are deployed;
native gameplay cutover and production player-account migration remain pending.

The master also owns `/v1/online/status`: unique recently authenticated accounts,
separate transient guest seats, and per-playlist queued/joining/playing/postgame
counts. Thirty-second activity expiry and live challenged connections share the
existing clan-presence boundary. Accounts are deduplicated across sessions and
couch seats; queue-to-room transitions count once, and a new search takes
priority over an old postgame connection. The website caches the aggregate for
five seconds and keeps advertised public-server occupancy separate. No private
account IDs or online roster are published.

The trusted Stats/Leaderboards publisher, 32 stat definitions and nine current
playlist leaderboards are configured in Epic. Real SDK qualification verified
23 profile/career fields, duplicate-safe absolute publication, customization
clears and a ranked-level decrease. OpenID trust is mapped to Live. A JWT signed
by the production master passed the real EOS Connect exchange, and the trusted
production SDK worker verified the returned token's exact account subject and
product/client/sandbox/deployment scope. No master account was created by that
qualification. Real player login/profile binding from a delivered native client
and two-machine relay gameplay remain acceptance work. The website was published
through TheRunaway5; its worker runbook contains the current evidence and
deployment workflow.

## Production transport cutover (2026-10-22)

Production image `halo2-master-eos:20261022T214749Z-transport` adds only EOS v13
admission, measured host selection, and the matching signed-voice keys to the
previously deployed profile/stat/population image. Existing v12 rooms remain
isolated and retain their deployed behavior. No unrelated result-consensus or
account-session changes from the shared checkout were included.

Twenty-three scoped Python checks passed. The actual native SDK test then used
that exact production source with fixture identity authorization: two native
processes, eight seats, EOS Sessions and forced relay, 185-187 game packets and
185-187 signed synthetic voice packets per peer. This verifies encoded packet
transport and speaker ownership, not microphone capture/playback or gameplay.
The relocated Rust bootstrap test and twelve launcher promotion-policy checks
also passed. Evidence: `build/test-results/eos-production-source` and the website
repository's ignored `.local/deploy/eos-transport` receipts.

The scoped master image passed startup with the actual nine-playlist catalog,
then deployed with a protected SQLite backup and automatic health rollback.
Real player account linkage, a playable two-consumer-network match, microphone
playback, complete-world host migration and legacy transport removal remain
acceptance work. EOS Title Storage playlist distribution remains optional future
work.

## Final client delivery status (2026-10-22)

The Rust request-ordering fix passed real WAN qualification between the Linux PC
and public server. Forced relay received 273-293 intact 1,404-byte packets per
peer; normal EOS routing selected direct transport and received 284-287. Both
used EOS Sessions. The delayed-signaling native regression also passed.

Each independently authenticated couch player now gets an account-profile worker
using the same pinned API and Rust identity bridge. It synchronizes that player's
appearance and service tag, with a 20-second authenticated heartbeat and no
clan/party/voice polling. Guests get no profile worker. Duplicate local identities
are excluded. Primary profile sync uses the signed-in host profile, so temporarily
editing a secondary profile cannot replace the primary account's customization.
The account regression and all three changed UI translation-unit syntax checks
passed.

The full native source build, transitive wiring audit and launcher routing checks
passed. Automatic runtime promotion did **not** pass: eight of twelve actual MCC
frame cases passed, including Lockout, Gemini, Containment, Headlong, Warlock,
Waterworks, and Coagulation with two/four local players. Ascension and Beaver
Creek timed out; Dune exited with zero submitted frames. One-player Coagulation
was intentionally interrupted for diagnosis and is not an independent failure.
The previously selected runtime, which has no EOS library linked, also failed
the same Ascension check. A bounded one-frame debugger sample found active MCC
texture decompression before frame submission; it did not identify a complete
root cause. No gameplay/source state was fabricated to bypass these checks.

The selected launcher runtime remains unchanged. The failed candidate, SDK
extraction, object archives and owned build scratch were removed. Small source
hashes, commands, traces and results remain under
`analysis/eos-client-release-fixed-20261022`, with the concise receipt at
`analysis/eos-client-delivery-status-20261022.json`. Native changes remain local
in the game checkout. Real Steam/Discord profile binding through a delivered
client, completed-match publication, two-consumer-network play, microphone
playback and complete-world host migration still need runtime acceptance.

A subsequent diagnostic ran the previously selected client on an isolated Xvfb
display using the same NVIDIA GPU, four-frame CLI and 180-second budget.
Ascension passed in 117 seconds and Dune in 69 seconds. Both submitted all four
frames. This does not establish why the earlier desktop runs failed, or qualify
the EOS candidate. A fresh EOS source build is running the complete matrix on an
isolated display, recorded in `analysis/eos-client-release-isolated-20261022`.
The reference diagnostic scratch was removed; its small receipt remains at
`analysis/eos-isolated-display-20261022.json`.

The subsequent production image `halo2-master-eos:20261022T235241Z-profile-cache`
adds a bounded cache to the master profile reader. Unchanged native heartbeats
and public profile/history reads reuse totals instead of scanning all accepted
matches and weapon rows. The durable accepted-result/profile revision invalidates
each player's entry immediately; entries also expire after five minutes and
the cache is limited to 1,024 players. A direct regression reproduced the prior
scan, and all 25 scoped production-master tests plus the deployment-image cache
check passed. Rollout retained a SQLite backup and the previous transport image
for rollback. The website repository owns the source patch and deployment receipt.
