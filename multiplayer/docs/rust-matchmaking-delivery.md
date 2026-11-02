# Opt-in Rust matchmaking delivery candidate

The default production Compose file still runs the legacy service. The new
`master_server/compose.production-rust.yaml` overlay is an opt-in candidate for
moving all public account, Steam/EOS, profile, friends, clan, party, matchmaking,
history and room/voice routes to `halo2-matchmaking-service`. It is not activated
by building or testing this checkout. Full matchmaking readiness remains false.

The legacy process runs with `H2_DIRECTORY_ONLY=1`. It creates only the signed
listing/enrollment/federation store and its maintenance. It does not construct
account, queue, party, clan, room, friend or voice owners, register their routes,
or clean up their state. Directory enrollment continues to use Discord; Steam
account sign-in belongs to Rust. The public proxy sends only the explicit
listing/enrollment/federation allowlist to that process. Every other path goes
to Rust, including unimplemented paths, so a missing capability cannot select a
Python fallback.

Both processes mount the existing `master-data` volume at `/state` and open
`registry.sqlite3`. Keep its canonical identities, ranks, history, clock floor,
`master-key.json`, `eos-openid-key.pem` and operator configuration together.
Do not substitute a fresh Rust database. The directory starts first and retains
its master identity. Rust owns the matching/account tables and their runtime
transactions. Processes use UID/GID 10001; state must have mode `0700` and the
database and private keys mode `0600`.

The Rust backend still requires `--behind-tls-proxy` with a loopback listen
address. Compose shares Caddy's network namespace with
`network_mode: service:caddy`; Rust listens at `127.0.0.1:8081`. No backend HTTP
port is published. Caddy retains the public HTTPS ports. Compose's namespace
and restart dependency semantics are documented in the
[official services reference](https://docs.docker.com/reference/compose-file/services/).

`Caddyfile.rust` permits 49,152 bytes at the outer request boundary. Rust enforces
its own limits of 16,384 bytes for ordinary requests and 49,152 bytes for voice
preferences; the latter supports the existing 2,000-subject preference record. The Rust
upstream response-header timeout is 100 seconds so it does not cut off the
95-second relationship/invitation budget. No upstream read timeout or WebSocket
stream deadline is added. Caddy documents these transport settings in its
[reverse proxy reference](https://caddyserver.com/docs/caddyfile/directives/reverse_proxy).

`GET /v1/health` and the retained `GET /health` are the same Rust health view.
Check `component_ready`, `catalog_sha256`, configured capabilities and EOS
identity status. `ready:false` remains the full-system qualification boundary.
The legacy directory's private health endpoint describes only discovery.

The candidate pins the Rust 1.96.0/bookworm build base, bookworm runtime base and
Caddy 2.11.7 image indexes by registry digest. The Rust image contains the Rust
service, native TLS/curl dependencies and a curl health probe. It has no Python
matchmaking process. Its Dockerfile-specific allowlist excludes build targets,
state, environment files and private keys from the build context. An image
build and executed-image dependency/SDK audit are still required; a successful
host executable does not qualify the container image.

From `multiplayer/master_server`, inspect the concrete configuration with an
explicit domain and catalog:

```sh
export MASTER_DOMAIN=your.registered.domain
export MATCHMAKING_CATALOG=/absolute/operator/playlists.json
docker compose -f compose.production.yaml -f compose.production-rust.yaml config
docker compose -f compose.production.yaml -f compose.production-rust.yaml build
```

Use this overlay in place of `compose.production-matchmaking.yaml`. The overlay
explicitly disables the Python catalog; applying a nonempty catalog to a
discovery-only process fails before it opens state. Before any actual cutover,
stop both writers and take a complete private state backup. A raw copy of a
live SQLite file without its WAL is not a backup. Qualify the candidate on a
clone, preserving the original backup and identity hashes. Do not run a legacy
matching coordinator and the Rust coordinator against the same state.

Add deployment-specific private mounts/environment in an operator overlay.
`H2_STEAM_KEY_FILE` names a private mounted Web API key. EOS requires the
registered HTTPS public issuer, `H2_EOS_OPENID_AUDIENCE`, the actual server
verifier at `H2_EOS_WORKER`, its SDK library and private server configuration.
The server verifier is distinct from the Linux game's client worker. Neither
SDK credentials nor the private test provider interposer belong in an image.
The optional friend-file worker also needs its existing private configuration.
Without these inputs their capabilities remain unavailable.

Actual activation, once qualified, uses the same explicit Compose file pair:

```sh
docker compose -f compose.production.yaml -f compose.production-rust.yaml up -d --wait
```

When recreating Caddy, recreate its namespace-sharing matchmaking service in
the same Compose operation. Check actual public health, catalog identity,
Steam/EOS configuration, login, playlist selection, Start/search/cancel and
played-world/result continuity. Source and proxy tests do not replace this
acceptance on the installed Linux route.

For a catalog-only rollback, stop Rust, restore the qualified catalog bytes and
restart it against the same state. Runtime grants expire across restart and
must be renewed; canonical accounts and ranks remain in the database. The
fixture qualifies this config rollback and key continuity. Rollback to a legacy
service image, actual OCI restart/recreation and production restoration still
require their own state-clone checks. Do not activate a legacy matching owner
while Rust is running.

Run the host delivery regression from the repository root:

```sh
python3 tools/run_test.py -- python3 tools/test_rust_matchmaking_delivery.py \
  --output build/test-results/rust-matchmaking-delivery
```

The helper freezes the service, core, legacy discovery and configuration inputs;
builds the production ABI and Rust service with two jobs; validates Compose
without starting Docker; downloads the pinned official Caddy release and checks
its pinned SHA-512; then runs private HTTPS/WSS and database controls. All
binaries, state, certificates and provider fixtures belong to the enclosing
scratch owner. Only bounded reports remain. Genuine Valve/Epic authorization,
original Xbox Live behavior, played worlds, installed physical input and the
complete PDF G01-G12 gates stay open.

The populated-state regression is separate:

```sh
python3 tools/run_test.py -- python3 tools/test_rust_matchmaking_delivery.py \
  --legacy-state-only --output build/test-results/rust-matchmaking-state-cutover
```

It creates a private store through the current legacy HTTP adapter and its
production Rust ABI. A real room exchange keeps the full ten-second start gate;
the map facts, provider grants, linked aliases and submitted PCR remain declared
fixtures. The committed store contains hidden XP, result/admission/history,
linked accounts, profile customization, two clans, an accepted party,
friendship and voice preferences. SQLite's backup API clones that store and its
signing keys. Rust opens the clone, rejects obsolete runtime grants, preserves
all existing table contents and equivalent API views, replays a legacy party
command once and writes a new profile revision. The legacy process then opens
the Rust-written clone and reads that revision with the same identity/history
views. The original source store is checked for changes afterward.

The [populated-fixture receipt](evidence/matchmaking-rust-parity-20261028/legacy-state-cutover-rust-validation.json)
records 67 tables, including 33 nonempty tables and 75 rows, along with hashes
and cleanup. It qualifies host process handoff using the current checked source.
It does not qualify a real production database, an older deployed service image,
container rollback, genuine providers or natural gameplay. Those gates still
require the actual deployment cohort and a protected production state clone.
