# Rust matchmaking service runtime

This executable owns HTTP dispatch, provider receipts, the SQLite connection,
transactions, account grants and publication of committed cache changes. It
uses the same `matchmaking_owners::Owners` and policy modules as the existing
C ABI. Requests cannot submit SQL, native constructors or owner handles.

Implemented routes cover account sessions, clans, current profiles, public
history, matchmaking sessions, playlist eligibility, accepted parties and their
pregame countdowns, ticket creation/polling/cancellation, search WebSockets,
room descriptors, room signal/relay sockets, content-failure reports and
configured account/room EOS token and proof routes, Steam browser/device sign-in,
remembered sign-in, dual-provider linking, accepted friends and observed
population, Steam relationship lookup, Steam-backed party invitations and the
optional private friend-file outbox.
Steam login checks its private
persisted grant and rechecks every signed local inside the canonical
transaction. Discord verification uses the existing fixed HTTPS
application/scope and user checks in Rust. No Python server participates in
these routes.

The opt-in [production delivery candidate](../../multiplayer/docs/rust-matchmaking-delivery.md)
provides a Rust-only service image and a Caddy/Compose route alongside a
discovery-only legacy process sharing the canonical state. Both `/v1/health`
and `/health` serve the same component health, with full `ready:false` retained.
This candidate does not change the running production service or selected
Linux runtime. Host TLS/WSS acceptance and actual OCI/provider/installed-client
acceptance remain distinct.

Supply a local catalog with `--matchmaking-catalog PATH`. Rust parses and bounds
its bytes, owns the admitted catalog and entropy streams, and publishes its
SHA-256 in health. Queue responses use the existing session/catalog/party/rank/
gathering owners. Missing search sockets cannot produce successful QoS facts or
a match. HTTP cannot supply a roster, rank, consent receipt or source handle.

Startup records a hash of each normalized playlist profile in
`registry.sqlite3` under its playlist ID and revision. Change the revision
when changing a rotation, compatibility rule, rank table or other bound
profile field. Equivalent JSON formatting can reuse the revision, and an
exact rollback can reuse the original binding. The first startup with this
check records the profiles supplied by the configured catalog.

Clients using the full-settings v15/v16 profiles can select a stock playlist
only when every stock row supplies its complete original settings. A legacy
name-only rotation is ineligible as a whole, so its weights are never changed
to fit a newer client. Fiesta Slayer and Infection Slayer are explicitly
named modern extensions with separate rules ownership.

The game-start countdown comes from the current admitted loading room for
the authenticated queue generation. Native loaders opt in with
`start=1&start_room=1` and check the room ID against their installed
descriptor before accepting a shared zero. The opted-in response retains
the admitted playlist and match data after the search tickets are consumed.
Older requests retain the scalar response format. Deployments that upgrade
the native loader need a service that supplies the opt-in room field.

Search WebSockets use the existing Rust relay QoS owner. The socket adapter
supplies actual connection generations, closure and received bytes; the shared
owner checks grants again after upgrade and owns challenges, heartbeat expiry,
probe measurements, leases and queue readiness. Replaced or cancelled futures
publish closure immediately, and a late detach cannot remove a newer socket.
These are modern relay-cycle measurements; original Xbox XNet and service
correspondence remain open.

`GET /v1/friends` and `POST /v1/friends/commands` use the shared Rust friendship
directory and a private Rust block cache. Original duplicate query pairs reach
that owner; `party=1` includes its current joinability/context policy. Search
returns public identity and tag data without disclosing a nonfriend's presence.
The native owner enforces sender/recipient consent, bilateral blocks, replay
IDs, capacities, Unicode tag matching and transactional invitation retirement.
Cache plans publish after COMMIT and are retired on failure. The existing
250-entry roster uses bounded SQL pages; HTTP cannot supply private owners or
replace a party context.

`GET /v1/online/status` accepts no filters and reports anonymous user/guest and
playlist counters from admitted Rust account, matching-session and room owners.
It deduplicates signed accounts and retains transient guests separately. The
shared presence owner selects idle, queued, joining, playing and postgame
activity. Anonymous reads do not renew account activity or grants; the current
activity window is thirty seconds. These are modern observations, not
recovered Xbox Live presence flags. Friend views and clan presence use the
same actual owners and socket observations.

`GET /v1/account/steam-friends` requires an account grant and a configured
Steam Web API key. The shared Rust owner selects the fixed GetFriendList/
GetPlayerSummaries sequence, at most1000 IDs,100-ID profile batches, online
profile filtering, canonical registration, bilateral blocks, presence and
public ordering. The native cache retains its60-second lifetime, serialized
upstream lease,128 waiting requests,1000 subjects and64MiB byte bound. These
are modern service policies. Valve documents the provider methods in its
[ISteamUser reference](https://partner.steamgames.com/doc/webapi/ISteamUser).

`invite_steam` on the party command route obtains a private one-use proof from
that Rust owner, bound to the account, target, exact command and credential
configuration. The party directory consumes it after SQL COMMIT. Failed COMMIT
rolls back invitation/replay state; committed commands replay without upstream
IO. Proofs and native handles never enter HTTP payloads or replies. Account
grants and canonical aliases are checked again after provider IO and committed
SQL observations.

Each lookup retains its HTTP request slot through actual IO and cleanup.
Existing global request capacity bounds those workers at64; the native owner
serializes upstream work. Provider calls hold neither the runtime mutex nor
a SQL transaction. Cancellation retires its native owner after actual IO ends.
TLS checks, disabled ambient proxies/redirects, strict decoding and1MiB
provider bodies remain explicit.

The request owner retains its75-second deadline. Typed relationship requests
can extend their connection budget within an absolute95-second ceiling;
ordinary connections retain30 seconds and parser/body reads retain five.
The native OnlineAccount client retains its existing three-second HTTP
transfer timeout. The33-second lookup control qualifies the server boundary;
native slow/queued lookup behavior remains unqualified.

Room admission selects its roster, personal/clan namespace and snapshot bank
through the Rust owner. The runtime reads current SQL facts, revalidates the
prepared admission, and publishes credentials after COMMIT. Failed admission
rolls back result/setup/capability state as well as SQL. Room observations now
feed session seating, queue/party gates, shared start reads and presence.

Room IO supplies actual socket generations, closure and queue capacity to the
existing Rust lifecycle, control, signaling, packet and result owners. The
adapter checks packet destinations again when writing. Replacement and late
detach retain their generation fences; frontend logout preserves a transferred
room bearer. Pending signals drain in bounded batches, preserving the native
512-frame pending capacity. Original start/readiness kernels retain their
ten-second gate.

The native result owner decides unanimity and quarantine. SQLite stores the
bound admission, XP and PCR before completion is published. A failed result
COMMIT leaves review pending; live room ticks retry the write with a one-second
IO backoff while the native review lease continues. This is modern storage
recovery policy; original Live arbitration remains unproven.

The exclusive runtime keeps provisional queue changes private until SQLite
commits. A rejected transaction restores the complete queue, including grants,
consent, countdown, continuation and RNG state, and rolls back its SQL. Account
accounting and owner handle generations are retained. The native countdown
budget is separate from ordinary social reads; a Rust timer drives the original
countdown tick without client clocks.

Enable Steam issuance with `--steam-enabled`. Set `--public-url` or
`H2_PUBLIC_URL` (`H2_MASTER_PUBLIC_URL` is the fallback) to the public HTTPS
origin, without a path, query or fragment. A loopback listener with explicit
`--allow-http` can use its actual bound address for development. Production
uses the isolated TLS proxy. Disabling Steam leaves an EOS-only issuer's
configuration independent of Steam's origin validation.

Steam routes are `POST /v1/auth/steam/start`, `poll`, `resume` and `forget`,
`GET /v1/auth/steam/callback/{state}`, `GET`/`POST
/v1/auth/steam/confirm/{state}` and `GET /v1/auth/steam/complete`. Start requires
the native client's SHA-256 PKCE challenge and game-code confirmation hash.
The browser must complete fixed-provider verification and supply its private
HttpOnly cookie, matching origin and twelve-digit hexadecimal game code.
Five incorrect codes retire the pending attempt. Poll verifies the original
PKCE secret and publishes credentials once, after SQL COMMIT. Pending state
expires after five monotonic minutes and is not restored after restart.

Rust retains the existing native client's fixed HTTPS endpoint and identity
contract. Valve documents browser sign-in returning a SteamID through
[OpenID](https://partner.steamgames.com/doc/features/auth).
The callback checks exact destination, identity, signed fields and nonce,
then directly verifies the assertion with the fixed provider using
[`check_authentication`](https://openid.net/specs/openid-authentication-2_0.html#check_auth).
An optional 32-hex-digit Web API key enables sign-in persona lookup at the
fixed GetPlayerSummaries endpoint; configure `H2_STEAM_KEY_FILE`,
`H2_STEAM_KEY` or `STEAM_KEY` in that priority order. Without a key the verified
Steam identity uses the default display name. No endpoint is configurable.
Provider HTTP disables ambient proxies and redirects, verifies TLS and bounds
connect time, total time and response bytes. Steam OpenID proves the Steam
identity; it supplies no game-ownership claim here.

Issued `h2s_` credentials last eight hours. Optional `h2r_` remembered
credentials retain their original thirty-day expiry when resumed. Only hashes
are persisted. Resume and forget use the remembered bearer with an empty JSON
object; a remembered bearer cannot authorize account APIs. Forget atomically
revokes that remembered credential and only its mapped active credentials.
Failed COMMIT leaves pending confirmation or revocation available for retry.
Rust retains at most four active credentials per Steam subject, 1000 active
credentials overall and 100000 remembered credentials; a full remembered store
still allows verified sign-in without a new remembered credential.

Anonymous pending attempts have limits of four per source, eight per network
prefix and 128 overall, within 1000 total pending attempts. Rejected occupancy
does not consume the separate global start allowance. Two provider workers
retain their HTTP slots through IO; queue wait is two seconds and the owned
operation deadline is twenty seconds. Cancellation before or after blocking
preflight retires pending verification and cannot publish a late receipt.
These bounds and credential lifetimes are modern service policy.

`POST /v1/account/link-discord` verifies both current Steam and Discord proofs,
then uses the shared Rust canonical adoption policy. SQL commit, clan/profile
preservation and retirement of the adopted bare account share that transaction.
The common runtime transaction clock refreshes after acquiring the SQLite
write lock, so a proof that expires during the wait cannot allocate an alias.

EOS-purpose signing uses a persistent private RSA key, RS256, the canonical
player/name and the existing 120-second lifetime. Public JWKS expose only the
public key. Configure `H2_PUBLIC_URL` as the registered HTTPS issuer,
`H2_EOS_OPENID_AUDIENCE` as the registered audience and `H2_EOS_WORKER` as an
absolute path to the existing official-SDK server verifier. Equivalent options
are `--public-url`, `--eos-openid-audience` and `--eos-worker`. This requires the
isolated TLS-proxy mode; explicit development HTTP cannot enable the issuer.
The verifier inherits its private `H2_EOS_SERVER_CONFIG` and SDK settings.
The server worker is the Rust verifier in the website repository's
`backend/eos-worker`; its C++ layer only calls the official SDK.

`GET /v1/auth/eos/jwks`, `POST /v1/account/eos-token` and
`POST /v1/account/eos-proof` use the account grant. The corresponding
`POST /v1/matches/{id}/eos-token` and `eos-proof` routes require that room's
physical-owner bearer. Room grants remain independent of social account
sessions. Cancelling a search before gameplay retains its existing cancellation
semantics. Guests cannot invent a physical-owner credential.

An unverified JWT subject only routes a request to the SDK. It cannot authorize
or persist a PUID. Account/room capabilities are rechecked after the body,
after SDK IO and after acquiring the owned SQL transaction. The shared Rust
profile policy enforces immutable, unique PUID links and idempotent outbox
revisions; the legacy adapter uses this same SQL policy. SQL and room-local
proof publication roll back together on rejected COMMIT.

The verifier retains the shared 32-principal admission bound, one in-flight
request per principal, a two-second queue wait and a twenty-second owned
operation deadline. Only the stream owner can retire its child. Queued
cancellation leaves the active stream intact; owner cancellation, invalid
replies and timeouts reap the owned process before reuse. Responses have a
262144-byte total bound and at most 64 lines. SIGINT/SIGTERM shutdown cancels
active verification and reaps the child. Room signing/proof requests retain a
separate 120-per-minute owner allowance. These are modern service policies.

The process is an implementation increment toward the full parity goal. Real
Steam/Discord/EOS authorization, EOS native peer qualification,
full service integration and native/delivery
acceptance remain open. `/health` reports `component_ready`
and `ready: false`; unavailable capabilities return503. The selected production
service and launcher runtime are not changed by building this crate.

The HTTP listener uses explicit connection/header/body/work limits and native
source/prefix/principal budgets. These are modern service limits, not original
Xbox Live behavior. Header and body reads expire after five seconds; queued
requests expire and retain their work permits through blocking IO. TLS is
provided by an isolated loopback proxy, or HTTP is explicitly enabled for
development. The backend uses a separate private state directory and SQLite
file. This partial runtime must not share authority with an active legacy
matchmaking process; cross-process recovery and delivery remain unqualified.

Ordinary HTTP replies close their connections; valid WebSocket upgrades keep
the upgraded stream open. Search IO has 1000 connection slots, 32 pending blocking
operations, 256-byte frame/message limits, masked client frames and 500ms send
deadlines. The native heartbeat retires silent peers after 15 seconds. Transport
limits and heartbeat timing are modern service policy, not original Live data.
Room IO has 1024 socket slots, 120000-byte frame/message limits and per-peer
queues bounded by 512 entries and 2MiB. Native room challenges, close reasons,
loading/running transitions and content-rejection failures use the shared
owners. Original world preservation, full native gameplay and capacity costs
remain separate acceptance requirements.

Dependencies are pinned in `Cargo.toml` and `Cargo.lock`. The
[Hyper builder](https://docs.rs/hyper/1.12.0/hyper/server/conn/http1/struct.Builder.html)
provides the actual parser deadline and header limits. The
[Axum WebSocket upgrade](https://docs.rs/axum/0.8.9/axum/extract/ws/struct.WebSocketUpgrade.html)
provides the bounded upgraded stream. The
[rusqlite transaction implementation](https://docs.rs/rusqlite/0.41.0/src/rusqlite/transaction.rs.html)
backs the storage adapter; SQL and native effects remain selected by the shared
Rust owners. The pinned
[OpenSSL signer](https://docs.rs/openssl/0.10.81/openssl/sign/struct.Signer.html)
provides RSA signing; this crate retains `unsafe_code = "forbid"`.

Run qualification in owned storage:

```bash
python3 tools/run_test.py -- python3 tools/test_rust_matchmaking_service.py
python3 tools/run_tests.py --jobs 2 --test-jobs 1 \
  --target rust_service_account_probe --target rust_service_matchmaking_probe \
  --target rust_service_qos_probe \
  --target rust_service_room_probe \
  --target online_login_probe \
  --target rust_service_social_probe \
  --filter '^rust_matchmaking_service_http$' \
  --retain-report rust-service/report.json \
  --retain-report rust-service/source-manifest.json
```

The helper copies and hashes the Rust inputs, uses the real executable and HTTP
listener, injects provider grants only into its private test database, tests
failed COMMIT/retry and process restart, and optionally drives the real native
account, matchmaking, party and search QoS clients. It uses the existing fourteen-playlist
catalog, verifies the v12/v16 eligibility differential, and checks shared
Start/cancel publication. Real socket controls exercise malformed frames,
heartbeat expiry, session replacement, restart and measured Lab proposals.
Room controls also use actual relay/direct sockets and the full real start
timer, failed admission/result COMMITs, replacement, departure, content failure
and requeue. Optional native OnlineMatchSession probes exercise real client
loading, relay and PCR completion with explicitly synthetic datagrams, PCR and
map fingerprint. Declared v10/v12 fixtures cover retained transport seams;
the source v16 catalog retains its complete-options compatibility gate and
has separate descriptor and EOS identity/signal coverage. EOS controls verify
real signatures against JWKS, actual failed COMMITs, body/provider revocation,
queue/operation deadlines, cancellation, stream replacement and SIGTERM/restart.
Their SDK replies are private fixtures. Real EOS authorization/peer transport
and full v16 played flows remain
open. These fixtures do not attest an installed map or a native world.
The existing authored Lab profile is a bounded component control; it does not
qualify the required original profiles or natural played matches.
The native clients use a deterministic provider
handoff fixture; there is no real Steam grant, rendered UI, played game or
original parity acceptance claim. All
generated executables, databases and copied source remain under
`HALO2_TEST_SCRATCH` and are removed by the owner.

Steam controls run the default Rust executable with only fixed-provider curl
IO interposed by a private Linux test library. Independent HMAC assertions
exercise actual callback parsing, browser confirmation, one-time issuance,
failed COMMIT/retry, renewal, canonical adoption, cancellation and restart.
The optional native `online_login_probe` drives the actual SteamLogin browser
URL/device/code/poll flow, OnlineAccount clan view and sign-out against this
Rust server. The provider fixture attests no real Steam or Discord grant.
Separate owned SQL tests exercise 1000/100000 credential capacities and the
actual write-lock/expiry boundary. The test library, databases and compiled
probes are disposable; production has no provider-fixture configuration option.

Social controls exercise real friend consent, party privacy, failed block
COMMIT/retry, body-upload revocation, the 250-row roster, signed/guest queue
population, restart and the full thirty-second activity expiry. Private SQL
roster/provider fixtures are declared explicitly. The native social probe uses
actual OnlineAccount friend commands, party context/join and block views over
Rust HTTP with only its provider handoff replaced. Room controls check counts
through the actual loading/running/postgame transitions and frontend logout.
Private friend-file IO uses the same Rust revision, claim, retry and completion
owners. A queue-only room owner receives its public history identity in the
same transaction as EOS binding and outbox enqueue; no social-account login is
required. Set `H2_EOS_FRIEND_STORAGE=1` only with the configured HTTPS EOS issuer,
audience, absolute `H2_EOS_WORKER` and absolute
`H2_EOS_FRIEND_STORAGE_CONFIG`. The dedicated config must be a bounded private
regular file owned by root or the service user. Final symlinks, FIFOs, public
permissions, duplicate keys and publisher credentials are rejected. Its exact
keys are `H2_EOS_PRODUCT_ID`, `H2_EOS_SANDBOX_ID`, `H2_EOS_DEPLOYMENT_ID`,
`H2_EOS_FRIEND_CLIENT_ID`, `H2_EOS_FRIEND_CLIENT_SECRET`,
`H2_EOS_FRIEND_CACHE_DIR` and `H2_EOS_FRIEND_ENCRYPTION_KEY`. The cache root must
be an absolute private directory owned by the service user; an optional cache
environment override retains the same checks.

The worker receives only the existing EOS account's signed delegation and
committed public record. Each operation owns a new private cache and a child
process with a restricted environment. Steam/publisher credentials, preload
hooks and master paths are excluded. The full 130-second deadline, 64-line and
131072-byte line limits apply. Exact read-back is required; the child is reaped
before cache removal. New revisions or changed PUIDs cannot be acknowledged by
an older result, and completion COMMIT failure retries its sealed claim without
repeating the upload. Shutdown cancels the child and leaves its revision pending
for restart. This optional storage implementation requires Unix private-file
support; its current end-to-end fixture qualification is Linux only.

`rust_friend_cloud_service_http` exercises the actual Rust service, SQL,
signatures, worker processes, retry deadline and restart. Only SDK/cloud replies
are fixtures; real EOS private-file authorization and storage remain unqualified.

Steam relationship controls run the default Rust executable with only the
private upstream curl interposer. They cover account/canonical revocation
during IO, failed invitation COMMIT/retry, strict response handling,1000
selected profiles, the full sixty-second cache expiry and a33-second lookup.
The native Steam browser/account probe also reads the complete relationship
reply of more than256KiB. These fixtures authorize no real Steam relationship or
native played match.

The native account adapter gets its HTTP timeout from the shared Rust bridge.
Steam relationship reads and strictly parsed `invite_steam` party commands
allow95 seconds, matching the service's absolute connection ceiling; ordinary
account calls retain3 seconds. Native stop-token cancellation still applies.
The `rust_native_steam_budget_http` CTest uses real native account clients to
check slow and queued thousand-row replies, a cold invitation after the full
sixty-second cache expiry, and cancellation during an active lookup. Its only
provider substitute is the private upstream Steam fixture.

Social account registration uses the shared Rust `registration_clock` SQL
operation inside the login transaction. Its persisted floor and profile date
commit together. Credential expiry keeps the Steam provider's wall clock.
Both observations refresh after the actual SQL lock wait. Returning profiles
retain their original dates; legacy unknown dates remain NULL. Discord linking
uses the same Rust registration-clock operation.

`rust_registration_clock_service_http` checks these separate domains, restart,
SQL-wait updates, deferred COMMIT failure and retry with private provider grants.
The service unit tests also deny the actual SQLite COMMIT and observe the
blocked BEGIN before advancing the private registration floor. These tests do
not change the system clock or attest real provider authorization.

```bash
python3 tools/run_tests.py --jobs 2 --test-jobs 1 \
  --target rust_service_account_probe --filter '^rust_registration_clock_service_http$' \
  --retain-report rust-registration-clock/report.json \
  --retain-report rust-registration-clock/source-manifest.json
```

`PUT /v1/account/voice-blocks` replaces the verified provider owner's private
Discord preference snapshot through the existing shared Rust directory.
It accepts the existing 48KiB body bound and at most 2000 distinct subjects.
These client-supplied preferences do not attest a provider relationship graph.
Steam grants are rechecked after acquiring the actual SQLite transaction;
failed COMMIT preserves the previous snapshot.

`GET /v1/clans/voice` and `GET /v1/party/voice` require a current account grant
and accepted membership before upgrade and recheck both after upgrade. The
existing shared Rust relay owns scoped identity, membership incarnations,
bilateral blocking, replacement, sequencing, delivery expiry and media budgets.
The service publishes attach/frame/poll effects only after successful COMMIT.
Old socket cleanup cannot retire a replacement. Transport IO provides masked
WebSockets, a 262-byte message bound, ten-second idle heartbeat, five-second
response deadline and 250ms send deadline. Shutdown fences both attached peers
and already upgraded transport owners before sending Going Away. These Opus
transports and limits are modern service behavior, not recovered Xbox voice.

`rust_social_voice_service_http` uses private provider grants and canonical
aliases with the real Rust HTTP/SQLite/WebSocket process. Its optional existing
native `OnlineVoice` clients exercise synthetic PCM through actual Opus, idle
heartbeat, opt-in capture, push-to-talk, participant/microphone mute and identity
revocation for clan and party channels. It does not exercise physical audio,
real provider authorization or a played native world. Run it in owned storage:

```bash
python3 tools/run_tests.py --jobs 2 --test-jobs 1 \
  --target online_voice_tests --filter '^rust_social_voice_service_http$' \
  --retain-report rust-social-voice/report.json \
  --retain-report rust-social-voice/source-manifest.json
```

The native client also delegates progress/roster/rank/voice/input presentation
policy and original widget kernels to safe Rust via CXX.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-progress-rust-validation.json)
records 613 Rust passes and the rebuilt private-service Linux playlist/Start
flow. This does not activate a production endpoint, replace a selected runtime
or certify genuine providers and full original/natural-match acceptance.
