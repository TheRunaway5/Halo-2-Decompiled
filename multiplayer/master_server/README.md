# Halo 2 federated directory

An independent, self-hostable directory for the port's future online servers.
It advertises simulator metadata today. Its separate
[matchmaking coordinator core](../docs/matchmaking.md) supports trusted-input
party queues and match proposals. Native Quickmatch/Optimatch/Playlists now use
its verified-identity solo/accepted-party queue API with the selected Vista HD UI.
Steam OpenID is the primary native login; optional Discord consent links friends
and invitations to the same private player record. See the
[Steam migration and acceptance record](../docs/steam-identity-migration-20261016.md).
Configure the queue origin and catalog as described in the matchmaking document.
Persistent [clans and account sessions](../docs/clans.md) support creation,
invitations, original role permissions, leaving, and clan text broadcasts.
When EOS transport is configured, the native queue adapter advertises v16,
including complete 304-byte match settings, with Rust-owned EOS admission/framing,
Sessions and player-hosted direct/relay gameplay and voice. The master retains
Halo gathering rules and accepts confirmed player/team results afterward.
The explicit direct-transport configuration and native migration fixtures still
use v12; legacy relay protocols also remain in regression fixtures. EOS failure
does not silently select either older transport. Full legacy retirement waits
for the [remaining playable cutover checks](../docs/eos-completion-goal-20261022.md).
An explicit versioned
[retail launch XP policy](../docs/ranked-progression.md) updates private playlist
skill in the result transaction and displays levels only on ranked playlists.
Accepted parties support invitations, leader authority and per-member playlist
consent. Clan matchmaking/ranking, later historical skill policies and complete
historical playlists remain unfinished.

The default public service is `https://master.halo2forever.net`, deployed on
Vultr with HTTPS and explicitly custom Lab queues. See
[deployment validation and remaining limits](../docs/validation.md).

Two masters keep separate Ed25519 identities and SQLite databases. Owners
provision instance credentials once; the instance signs subsequent requests
without interactive login. Masters exchange signed listings with pinned peers.

From the repository root:

```sh
python3 -m venv .local/master-venv
. .local/master-venv/bin/activate
python -m pip install -r multiplayer/master_server/requirements.txt
python tools/run_test.py -- python tools/test_master_server.py
```

The integration test starts two real processes, registers and updates a server,
stops and restarts its origin, checks discovery failover, then crashes a real
simulator and waits for both directories to expire the listing.

The service requires the Rust security ABI. The test helper builds it inside
owned scratch and removes it with the enclosing runner. For a source deployment,
build `rust/online_security_ffi/Cargo.toml` with the pinned toolchain and set
`H2_SECURITY_LIBRARY` to the absolute installed library path. Missing or
incompatible libraries fail closed. Docker includes the library automatically;
Compose uses the repository root as its build context.

The native client also uses the mandatory CXX bridge for connected-team
readiness, eligibility, lobby error selection/expiry and final loading handoff.
Their policy lives in `rust/online_protocol/src/matchmaking_readiness.rs`;
`original_matchmaking_readiness.hpp` contains declarations and native facts.
Native callers must link `halo2_online_protocol`; there is no C++ policy
fallback. The [native readiness qualification](../docs/evidence/matchmaking-rust-parity-20261028/native-readiness-rust-validation.json)
separates original kernel comparisons from declared native transport inputs.
The complete [Rust matchmaking migration and acceptance goal](../docs/matchmaking-rust-parity-audit-20261028.md)
still requires genuine providers, native worlds, installed input and delivery.

Complete variant buffers use the shared Rust validator in
`rust/online_protocol/src/matchmaking_variant.rs`. Catalog loading rejects
settings that fail the original 304-byte rules, including signed byte/short
fields. The Rust queue controller calls that owner directly; native frontend
admission reaches it through CXX. Matching preserves accepted bytes and rejects
invalid input; it never
substitutes the gameplay normalizer's Slayer reset for a selected game.
Native gameplay retains its separate reset operation.

The native queue controller lives in
`rust/online_protocol/src/matchmaking_client.rs`. Rust owns configuration
admission, session grants and lease renewal, queue request identity and retries,
command generations, strict catalog/ticket/transport parsing, local-seat and
roster binding, terminal failures and the single loading handoff. C++ supplies
typed login, clock, cancellable HTTP, native connection and presentation IO.
Successful session exchanges remain owned for cancellation cleanup, including
late responses. After gameplay takes the room, lease rotation cannot enqueue
another search or install a second connection; a new explicit command retires
that handoff fence. Invalid native profile input stays rejected until a valid
profile refresh.

The [queue-controller qualification](../docs/evidence/matchmaking-rust-parity-20261028/client-controller-rust-validation.json)
distinguishes private provider/native IO controls from original kernel evidence.
This replacement service client protocol does not recover original Live wire
behavior. Remaining native peer policy, genuine providers, played worlds,
installed Linux menu input and delivered acceptance remain separate gates.

The native search QoS worker now also resides in Rust, in
`rust/online_protocol/src/matchmaking_native_qos.rs`. It owns challenge parsing,
frame assembly, Ping/Pong, partial/AGAIN retries, deadlines and cancellation;
C++ supplies typed socket IO. The shared origin validator is
`matchmaking_origin.rs`.

The [QoS/frontend qualification](../docs/evidence/matchmaking-rust-parity-20261028/native-qos-rust-validation.json)
adds native menu controls, SDL mouse input and Vulkan rendering through the
Rust service and the unchanged 14-playlist catalog. Blocked choices report a visible reason;
supported selection returns to the lobby and follows the shared countdown,
search and cancel path. Private Steam handoff, explicit transport capability
selection and a party-reply IO delay are declared fixtures. It does not qualify
positive EOS play, original Live, Xbox framebuffer parity or physical installed
input. Full readiness stays false.

The [native room/session qualification](../docs/evidence/matchmaking-rust-parity-20261028/native-session-rust-validation.json)
adds the Rust room state owner and complete WebSocket worker. It covers strict
control parsing, phases/membership, full-settings fingerprints, result receipts,
signed voice and actual native socket retries. 437 Rust tests and 15 native
checks pass, including a real Rust-service/native room round trip with the
unshortened ten-second start. Provider grants, datagrams and PCR are declared
fixtures. Native peer polling/capability/host coordination and mesh-quality
policy remain Rust migration work; genuine worlds/providers, installed input
and complete delivered acceptance remain open.

The [peer-mesh/host qualification](../docs/evidence/matchmaking-rust-parity-20261028/peer-mesh-host-rust-validation.json)
adds the complete Rust H2QO measurement owner and the Rust migration-candidate
caller. The preserved C++ reference and Rust owner match 104,760 public API
commands, including packet bytes and reciprocal candidate ordering. 449 Rust
tests and 14 native checks pass. This retains the replacement transport
contract; original XNet, genuine worlds/providers and delivery gates remain
open. C++ peer polling and native capability/resource orchestration still
require Rust migration.


With an admitted MCC cache and Xvfb, run these focused native checks through
the owned test lifecycle:

```sh
python3 tools/run_tests.py --target frontend_online_lobby_buttons_tests \
  --target online_search_qos_tests --jobs 2 --test-jobs 1 \
  --filter '^(rust_frontend_lobby_(blocked|complete|rendered)|online_search_qos_client)$' \
  --cmake-arg=-DHALO2_ONLINE_RUST_OFFLINE=ON \
  --cmake-arg=-DHALO2_MCC_PARTY_UI_TEST_CACHE=/absolute/admitted/MCC-cache
```

```sh
python3 tools/run_tests.py --target original_matchmaking_readiness_tests \
  --filter '^original_matchmaking_readiness$' --jobs 2 --test-jobs 1 \
  --cmake-arg=-DHALO2_ONLINE_RUST_OFFLINE=ON
```

For Docker, start in this directory:

```sh
docker compose build
docker compose --profile tools run --rm init
docker compose up -d --wait master-a master-b
curl --fail http://127.0.0.1:18081/v1/health
curl --fail http://127.0.0.1:18082/v1/health
```

Continue with the simulator commands in [deployment](../docs/deployment.md).
The local example publishes loopback HTTP only; the production example uses
Caddy for HTTPS. Neither requires retail assets or a multiplayer executable.

The opt-in [Rust delivery candidate](../docs/rust-matchmaking-delivery.md) uses
`compose.production-rust.yaml` with the production base. Its public matching,
account, party and room routes go directly to the Rust executable; the legacy
process runs only signed discovery/federation. It preserves the existing state
volume and Rust's loopback TLS-proxy boundary. It is not activated by default;
executed-container, genuine-provider, installed Linux input and full parity
acceptance remain required.

- [Architecture and future client integration](../docs/architecture.md)
- [Native ONLINE browser and master configuration](../docs/native-discovery.md)
- [Native clans, account sessions and privacy](../docs/clans.md)
- [Matchmaking core and primary-source evidence](../docs/matchmaking.md)
- [HTTP API and signing contract](../docs/discovery-api.md)
- [Federation and failure behavior](../docs/federation.md)
- [Security and privacy](../docs/security.md)
- [Development, Docker, and production operations](../docs/deployment.md)
- [JSON Schema](protocol.schema.json)
- [Observed milestone validation](../docs/validation.md)

The C++ Steam browser sign-in supplies an in-memory, proof-bound authorization
handoff for account and matchmaking sessions. The backend verifies the Steam
OpenID assertion directly with Valve. Its optional Web API key supplies verified
profile names and stays in a mounted server secret file. Discord PKCE remains
available for explicit social consent and dual-proof account linking. Native owner provisioning
for signed server instances remains the separate seam in the architecture doc.

The source native frontend search/launch controller now runs in safe Rust via
typed CXX. Accepted-party consent, playlist intent, profile generations, B's
published countdown and launch checks are Rust policy; native resource/cache IO,
rendering and input remain native. [Qualification](../docs/evidence/matchmaking-rust-parity-20261028/native-frontend-search-rust-validation.json)
binds the tested native client to rendered private-service menu reports.
It does not activate a production service or replace the selected Linux runtime;
four broader frontend failures and full original/delivery acceptance stay open.

Native progress/roster/rank/voice/input decisions and widget selectors now use
the safe Rust policy as well. [Qualification](../docs/evidence/matchmaking-rust-parity-20261028/native-frontend-progress-rust-validation.json)
records 2,087 fresh original controls and 14 focused native passes, including
the explicit Linux MCC clan flow. Four broader frontend failures remain open;
the selected runtime and production service have not been replaced.
