# Directory milestone validation — 2026-10-10

## Discord, clans, clan voice and Vista ONLINE UI follow-up

The public master now runs release `20261010T205335Z`. The following supersedes
the older deployment limitations below concerning Discord login and social clans.

- A real browser authorization passed through native Discord PKCE, upstream
  application/scope verification, the public HTTPS account session, private clan
  snapshot and sign-out. The probe used the player's saved screen name and made
  no clan membership changes. IDs and credentials were absent from the UI/log.
- All **45 backend tests** passed on host Python3.14 and inside the deployed
  Python3.13 image on the VPS. Fixtures use temporary state. Coverage includes
  clan permissions, invitations, concurrent mutations, replay, persistence,
  privacy and authenticated voice isolation/revocation/frame/rate limits.
- **12 focused native checks passed** for discovery, matchmaking, accounts,
  Opus/voice transport, Discord and original frontend bounds. The three ONLINE
  frontend checks passed again after correcting the clan-name keyboard prompt.
- Strict used-material comparisons verify actual Vista texture/font bytes in
  ONLINE, browser, account, queue, playlist, party and clan panels. PC donor banks
  that cannot pass the integer-density pack filter retain their real source
  pages, with explicit source-backed mappings. The actual Vista shell supplies
  the framing. Obsolete decorations with no established PC counterpart are
  hidden. This does not claim complete original PC screen construction parity.
- **30 fresh GPU captures**, at960×720 and1280×720, cover clan creation/keyboards,
  roster, invitations, roles, irreversible-promotion confirmation, messages and
  voice controls. Roster spacing, prompt text, unobstructed panels and Vista
  framing were inspected. Rendering used normal Vulkan without validation layers.
- Two native Opus clients passed32 seconds of silence across heartbeat rounds,
  then synthetic speech, push-to-talk, microphone/member muting and revocation
  through the actual account/clan WebSocket relay. The native UI also exercised
  real SDL dummy devices. No operator microphone was captured by these tests.
- Deployment preserved the signing identity and persistent SQLite volume, with
  a complete stopped-volume backup owned by root, mode0600. Public HTTPS health
  is ready; anonymous account/clan/voice/queue requests are rejected. Only Caddy
  publishes application ports; backend8080 and admin2019 remain private. Both
  root launchers select the rebuilt `build-performance-20261010/halo2_native`
  and the imported Vista source pack.

Clan membership, invites, roles and private text messages are implemented;
[clans.md](clans.md) describes the native policies and controls.
[voice.md](voice.md) distinguishes opt-in clan voice from original match voice.
[online-vista-assets.md](online-vista-assets.md) records the exact asset evidence.
Authenticated parties, clan matchmaking/rank progression, original playlist
rotations, native connection probes, gameplay admission and original proximity/
team match voice remain unfinished. Physical audio quality and two real WAN
players were not verified. A passing synthetic relay is not that validation.

## Public deployment at Halo To Forever

`https://master.halotoforever.com` is deployed on the user's Vultr Atlanta
instance (`155.138.231.132`, Ubuntu 24.04, 1 vCPU, 2 GiB RAM, automatic backups).
GoDaddy's authoritative DNS and a public resolver return the instance address.
Caddy obtained a publicly trusted Let's Encrypt certificate, stored in its
persistent volume with automatic renewal. External HTTPS checks passed without
disabling certificate or hostname validation.

- `/v1/health` reports ready; `/v1/masters` advertises the correct HTTPS origin.
  The compiled native discovery client successfully reads the empty server list.
- The deployed catalog contains **Ranked Lab Slayer** and **Casual Lab Slayer**,
  explicitly custom two-player Lockout/Slayer queues. It does not represent an
  original playlist rotation. Anonymous queue requests and an invalid Discord
  authorization receive401. The real Discord verification endpoint is reachable
  from the backend; a successful player login to this public deployment is still
  unverified.
- All35 backend tests passed in an isolated container on the actual VPS, with a
  read-only root, dropped capabilities and temporary `/tmp`. These tests use
  fixture authorization and separate temporary state, never the production DB.
- Restarting both application containers preserved the master identity,
  certificate and public HTTPS service. Docker starts at boot and the services
  use `unless-stopped`. Signing keys, SQLite and certificates persist in named
  volumes. SSH key access is verified; password authentication is disabled.
- Only Caddy publishes application ports80/443; external TCP probes cannot reach
  backend8080 or Caddy's admin port2019. Operator enrollment is disabled.
- The game defaults to this HTTPS origin for discovery and the separate queue
  API. Environment overrides still select another trusted operator; explicit
  empty values disable the corresponding service.

This deploys one master, not a live federated mesh. It does not prove production
capacity, a successful Discord-to-public-master login, native connection probes,
WAN/NAT traversal, playable matchmaking admission, parties/clans, historical
playlist rotations, or competitive rank progression. Public Lab proposals wait
for trusted connection measurements; no fabricated QoS or gameplay handoff is
enabled. Local deployment logs and operator commands are under `.local/vultr`.

## Native Quickmatch/Optimatch/Playlists connection

The native UI now connects to the coordinator through the Discord-authorized
queue API. Its setup, protocol and explicit reconstruction boundaries are in
[matchmaking.md](matchmaking.md). Discovery remains independent.

- **35 backend tests passed** on host Python3.14 and in the fresh
  `halo2-master-local:matchmaking-native-check` image's Python3.13. The container
  ran read-only with all capabilities dropped and a temporary `/tmp`.
- **8 focused CTest checks passed** after rebuilding the client, identity and
  frontend test targets: directory client/real HTTP, matchmaking client,
  Discord identity, ONLINE frontend, native matchmaking real HTTP, frontend
  bounds and original frontend assets.
- The native real HTTP test uses the actual aiohttp service, SQLite identities
  and playlist ranks, the coordinator and a second authenticated HTTP player.
  Only Discord's external verification and transport connection probes are
  fixtures. Quickmatch uses the server default, Optimatch selects casual,
  lobby Start opens Playlists, matched names stay player-chosen, missing QoS
  blocks readiness, trusted QoS advances status, and Back/rejoin/sign-out revoke
  native tickets and sessions. No native gameplay launch is emitted.
- Queue API tests reject forged identity/rank/roster/QoS fields, reject stale
  catalog revisions, enforce map eligibility and per-player isolation, hide
  casual skill, rotate/revoke/expire sessions, preserve request idempotence,
  and return to connection checks when measurements expire. Unconfigured
  matchmaking leaves directory discovery operational.
- Native client tests cover pinned HTTPS origins, explicit literal-loopback
  development HTTP, private OAuth-to-session handoff, bounded/duplicate-key
  JSON rejection, interrupted-POST idempotence and cancellation racing a late
  response. Ambient proxy settings are poisoned in the HTTP integration test.
- **22 GPU captures** at960×720 and1280×720 use the launcher registry's imported
  Vista HD font/bitmap pack. They include service menu, playlists, details,
  search, connection checks, admission wait and the existing directory states.
  Inspected captures preserve original menu geometry, suppress retained logos
  and avoid unsupported voice indicators. Renderer validation layers were off.
- The actual launcher runtime target `build-frontend/halo2_native` rebuilt
  successfully with this integration. The launcher UI-assets verification also
  passes for both launcher routes and their explicit overrides.

Local logs, launcher report and final captures are under
`.local/matchmaking/validation`, `.local/matchmaking/launcher-ui-assets.json`
and `.local/matchmaking/captures-vista-final`. These checks do not prove a live
Discord-to-public-master session, production capacity, WAN/NAT traversal,
playable Internet admission, social parties/clans or historical rank progression.
The public deployment checks above establish TLS and service availability
separately from these local integration tests.

## Earlier source-informed core and Vista UI milestone

The separate [matchmaking core](matchmaking.md) now implements trusted-input
party queues and match proposals using [primary-source research](halo2-matchmaking-evidence.md).
At that earlier milestone it was not exposed by the directory API or connected
to native Quickmatch; the authenticated native integration above supersedes that limit.
No retail playlist catalog, gameplay admission or rank progression is claimed.

- The full backend suite passed **30 tests** on host Python3.14 and the built
  Docker image's Python3.13. This includes14 matchmaking scenarios and the
  existing16 infrastructure tests. The container ran read-only with dropped
  capabilities and a temporary `/tmp`.
- The seeded demo progressed from a narrow search with no full match to an
  eight-member proposal after widening, kept both three-person parties intact,
  and required all eight readiness acknowledgements before handoff.
- `frontend_online_identity` now loads `assets/asset-sources.tsv`, exactly as
  the desktop launchers select Xbox layout and the imported Vista pack. It
  passed assertions that ONLINE draws enlarged imported font atlases and UI
  bitmap pages.
- Ten fresh GPU captures at960×720 and1280×720 used that registered Vista pack
  for browser, scroll, details, empty and unavailable states. Browser/details
  captures were inspected; the retained logo stays suppressed. Rendering used
  validation layers disabled, as in the earlier captures.
- `tools/verify_launcher_ui_assets.py --build build-frontend` passed for both
  launchers' menu routes, source setup and explicit override handling.
- The six focused C++ discovery, identity and frontend checks passed again
  after rebuilding the final ONLINE test. A later full dependency rebuild
  encountered a concurrent `static_scenery_collision.cpp` incomplete-type error;
  that file has since gained its missing metadata include. The final test was
  relinked against the already built libraries; this milestone does not claim
  a clean build of all concurrently changing native sources.

The earlier GPU captures below used original Xbox textures/fonts. This follow-up
corrects the verification path; the normal launchers already selected the Vista
pack. No production painter or launcher change was necessary for HD assets.

## Native discovery follow-up

The C++ `OnlineDirectory` client and **ONLINE → Server Browser** are now
connected. The browser and separate details panel retain original disc bounds,
skins and font metrics, while presenting new directory data. Quickmatch stays a
separate, unavailable native route. The directory integration itself adds no
queues or matching algorithm; the later core above is a separate milestone.

- `online_directory_client`: async fetch, HTTPS-origin validation, failover,
  valid empty response, bounded/strict parsing, cached expiry, cancellation,
  failed-refresh clearing and recovery passed.
- `online_directory_http`: compiled C++ client against two real master
  processes passed signed registration, updates, federated failover after an
  origin kill, expiry, restart recovery and removal. Real HTTP fixtures also
  verified that redirects, compressed/oversized bodies and ambient proxies do
  not redirect discovery outside the configured endpoint.
- `discord_identity`, `frontend_online_identity`, `frontend_bounds` and
  `retail_frontend` passed. The expanded ONLINE test uses controlled identity
  and directory fixtures with the real loopback callback and original assets;
  it covers 20 rows, scrolling, details, refresh selection, expiry, outage,
  retry, exit cancellation, direct entry, ID privacy, names and logo suppression.
- GPU captures at 960×720 and 1280×720 exercised listing, scrolling, details,
  empty and unavailable states. Vulkan validation layers were unavailable, so
  captures used normal rendering without validation-layer diagnostics.
- Both `build-multiplayer/halo2_native` and the desktop launchers' current
  `build-frontend/halo2_native` were rebuilt.

Configuration and reproduction commands are in [native-discovery.md](native-discovery.md).
No public master origin is bundled. This follow-up does not validate a live
Discord authorization, public TLS deployment, native owner provisioning,
gameplay admission, WAN/NAT traversal, original matchmaking, clans or rankings.

## Independent master-service foundation

The following were exercised locally against this implementation:

| Check | Result |
|---|---|
| Infrastructure suite, host Python 3.14 | 16 tests passed |
| Same suite in the built Python 3.13 Docker image | 16 tests passed |
| Two independent master processes with separate keys/SQLite | Registration, update, heartbeat, federation and discovery passed |
| Process outage/restart and real simulator crash | Failover, origin persistence, retry, convergence, and expiry on both passed |
| Two actual Compose masters with separate persistent volumes | Both health checks passed |
| Signed simulator against Compose services | Registration and metadata update propagated to B |
| Stop/start Docker master A during simulator operation | B answered failover discovery; simulator survived; A retained identity and converged |
| Abrupt simulator exit (code 23), four-second test leases | Both Docker masters automatically stopped returning the listing |
| JSON Schema draft 2020-12 | Schema valid; example metadata and real credential, signed announcement, origin listing and discovery matched |
| Production Compose syntax and Caddy validation | Accepted; no public domain or certificate issuance attempted |
| Existing built native tests | `discord_identity`, `frontend_online_identity`, `frontend_bounds`, `retail_frontend` passed |

Tests include invalid signatures, wrong request binding, replay, unauthorized
ownership changes, credential expiry/revocation, duplicate registration,
malformed/oversized/compressed requests, rate limits, peer authentication,
equivocation, stale resurrection, trusted/untrusted TLS certificates, and
upstream compression rejection. Docker state permissions were also checked:
mode 0700, owned by the service's UID 10001.

Each Compose master used approximately 35 MiB under this small local exercise
and its 256 MiB container limit. This is an observation, not a load benchmark.
The final test containers were stopped/removed; their named state volumes and
the built image remain available. Docker Desktop remains running.

No native auth or gameplay source changes were needed for this service. Native
checks used the existing built binaries; no full game build/playthrough was
performed. Discord owner validation used controlled verifier/HTTP fixtures,
not a live user's token. Public deployment, native owner provisioning, WAN/NAT
behavior, matchmaking, clans, rankings and playable Internet multiplayer are
outside the completed milestone.

To rerun the automated suite, use the commands in the
[directory README](../master_server/README.md). To reproduce the Docker
lifecycle, use the [deployment instructions](deployment.md).
