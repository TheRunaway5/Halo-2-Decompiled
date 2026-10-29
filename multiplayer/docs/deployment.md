# Development and self-hosting

Commands below run from the repository root unless stated otherwise. Python
3.13+ and Linux are supported; the simulator uses `fcntl` to lock its identity
file. No game assets are needed.

## Local Python setup

```sh
python3 -m venv .local/master-venv
. .local/master-venv/bin/activate
python -m pip install -r multiplayer/master_server/requirements.txt
python -m multiplayer.master_server.cli init-demo --a .local/master-a --b .local/master-b
```

This creates separate master keys, admin tokens, and mutually pinned peer
configuration. `init-demo` deliberately rewrites both peer files; use it only
for the demonstration, not to maintain a production trust set.

Run A in one terminal:

```sh
H2_MASTER_STATE=.local/master-a H2_MASTER_PORT=18081 \
H2_MASTER_PUBLIC_URL=http://127.0.0.1:18081 H2_MASTER_ALLOW_HTTP=1 \
H2_MASTER_ADMIN_TOKEN_FILE=.local/master-a/admin-token H2_MASTER_SYNC_SECONDS=1 \
python -m multiplayer.master_server
```

Run B in another terminal with `master-b`, port `18082`, and the matching URL
and token path. The bind address defaults to loopback. Stop with SIGTERM/Ctrl-C;
the daemon cancels peer polling, closes HTTP and SQLite, and exits cleanly.

Provision and run a simulator in a third terminal:

```sh
python -m multiplayer.master_server.cli enroll --identity .local/simulator.json \
  --master http://127.0.0.1:18081 --allow-http \
  --admin-token-file .local/master-a/admin-token
python -m multiplayer.master_server.cli run --identity .local/simulator.json \
  --metadata multiplayer/master_server/examples/server.json --allow-http \
  --heartbeat 20 --update-every 10 --maps lockout ascension \
  --gametypes Slayer 'Team Slayer'
```

Enrollment generates the instance key if missing. `init-server --identity PATH`
can also generate it in advance. Reuse that file on restart; it includes the
credential and sequence. Running two simulators with the same file is rejected.
SIGINT/SIGTERM unregisters gracefully. Add `--duration 30` for a timed graceful
run or `--crash-after 10` to exit with code 23 without unregistering. After a
crash, the listing expires when the last signed lease ends. Temporary master
outages are retried; invalid/revoked credentials fail explicitly.

Query real HTTP results with optional filters:

```sh
python -m multiplayer.master_server.cli list --allow-http \
  --masters http://127.0.0.1:18081 http://127.0.0.1:18082 \
  --map lockout --available-slots 1
```

Stop A and run the same command to see `master_used` switch to B. B retains
unexpired A-origin listings; a long origin outage eventually expires them.
Restart A with its existing state, and simulator heartbeats/updates converge
again. Revoke with `cli revoke --identity .local/simulator.json --allow-http
--admin-token-file .local/master-a/admin-token`.

For owner provisioning against HTTPS, replace the admin option with
`--discord-token-stdin` and provide an **existing** access token on stdin. Avoid
putting tokens in command arguments or shell history. The game-to-owner-API hook
is documented in [architecture](architecture.md); it is not wired into the game
yet. Dedicated instances use their saved credential and key after provisioning.

## Two-master Docker setup

The image contains only the directory and CLI. Named volumes persist SQLite,
keys, peer configuration, and bootstrap tokens. The local services run as UID
10001 with a read-only root, dropped capabilities, a temporary `/tmp`, and a
256 MiB memory limit each.

```sh
compose() { docker compose -p halo2-directory -f multiplayer/master_server/compose.yaml "$@"; }
compose build
compose --profile tools run --rm init
compose up -d --wait master-a master-b
```

For a faster expiry experiment, prefix `compose up` with
`H2_MASTER_LEASE_SECONDS=10`, and use simulator heartbeat `--heartbeat 2`.
Both services expose loopback ports 18081 and 18082; their authenticated sync
uses the private Compose network. To provision using the local bootstrap token:

```sh
mkdir -p .local/docker-demo
compose cp master-a:/state/admin-token .local/docker-demo/admin-token
chmod 600 .local/docker-demo/admin-token
python -m multiplayer.master_server.cli enroll --identity .local/docker-demo/server.json \
  --master http://127.0.0.1:18081 --allow-http \
  --admin-token-file .local/docker-demo/admin-token
python -m multiplayer.master_server.cli run --identity .local/docker-demo/server.json \
  --metadata multiplayer/master_server/examples/server.json --allow-http --heartbeat 2
```

Use the same discovery client as above. `compose stop master-a` and
`compose start master-a` demonstrate failover and recovery. `compose logs
--tail=50` shows structured logs, while `/v1/health` reports peer failures.
`compose down` removes containers and the network while retaining state volumes.
Do not add `-v` unless intentionally discarding this demonstration's identities
and database.

The integration suite can also run in the actual image:

```sh
docker run --rm halo2-master-local:v1 python -m unittest discover \
  -s multiplayer/master_server/tests -t . -v
```

The native **ONLINE → Server Browser** can read these local masters using
`H2_MASTER_URLS` and the explicit loopback HTTP opt-in. See
[native discovery setup](native-discovery.md) for launcher commands and limits.

## Production: one master per VPS

Use `compose.production.yaml`. Supply a DNS name resolving to the VPS, permit
inbound TCP 80/443 (and optionally UDP 443 for HTTP/3), and synchronize the host's
clock. Caddy obtains and renews its certificate; the backend has no published
host port. Public DNS/ACME provisioning is operator work and is not established
by the local demonstration.

```sh
export MASTER_DOMAIN=directory-a.example.org
production() { docker compose -p halo2-directory-prod -f multiplayer/master_server/compose.production.yaml "$@"; }
production build
production --profile tools run --rm init
production up -d --wait directory caddy
curl --fail "https://$MASTER_DOMAIN/v1/health"
```

The supplied Caddyfile bounds upstream response headers to ten seconds. Keep
voice and gameplay WebSocket reads free of a ten-second transport read deadline:
an idle voice connection's heartbeat starts at that same interval. The
[native proxy regression](voice.md#production-proxy-regression) checks silence
across multiple heartbeat rounds and audio afterward through the actual proxy.

To enable authenticated matchmaking, also add
`compose.production-matchmaking.yaml` and set `MATCHMAKING_CATALOG` to the
absolute host path of a reviewed catalog. The supplied
`examples/matchmaking-online.json` supplies the reviewed-source final-era
reconstruction described in [the hopper runbook](playlist-hoppers-20261028.md).
It requires the matching v16 native client and current Rust service library.
The 2026-10-28 accuracy/hopper release activated all 14 playlists after client
qualification; see the linked runbook for image, catalog and client pins. The separate
`examples/matchmaking-development.json` contains small custom development queues.
Its player-facing names describe the queue and mode. The
coordinator's database and signing identity remain in the same named state
volume across image rebuilds and catalog changes.

```sh
export MATCHMAKING_CATALOG=/absolute/path/to/reviewed-playlists.json
production() { docker compose -p halo2-directory-prod \
  -f multiplayer/master_server/compose.production.yaml \
  -f multiplayer/master_server/compose.production-matchmaking.yaml "$@"; }
production up -d --wait directory caddy
```

`init` prints only the master ID and public key. Although it also creates a
bootstrap token file for local tooling, the production service does not load
that file, so operator enrollment is disabled. Owner provisioning uses the
existing Discord authorization. Change `H2_DISCORD_CLIENT_ID` if intentionally
operating with a different already configured application.

A standalone master works with the default empty `peers.json`. To federate two
VPS instances, initialize each independently, exchange their public ID/key over
a trusted channel, and create a local `peers.json` for A containing B:

```json
[
  {
    "id": "B's actual master UUID",
    "public_key": "B's actual 64-character public key hex",
    "url": "https://directory-b.example.org"
  }
]
```

Configure the reciprocal entry on B. The placeholders intentionally fail
validation; use actual initialized identities. Copy the configuration and restart:

```sh
production cp peers.json directory:/state/peers.json
production restart directory
```

The copied file must be readable by UID 10001 (public peer pins may use mode
0644). Keep the key, admin token, and database private. For three masters, pin
all three origins at every member, excluding self. Changing peer URLs does not
change identities. Removing a pin rejects further synchronization from that
origin; previously accepted active records age out by their signed expiry.

For direct HTTPS without Caddy, set `H2_MASTER_TLS_CERT` and
`H2_MASTER_TLS_KEY` to a certificate chain/key readable by the service, set the
HTTPS public URL, and leave both HTTP opt-in and proxy mode off. Direct TLS
health checks must also use HTTPS with certificate validation. If using your
own reverse proxy, enable explicit proxy mode only on an isolated backend and
have it overwrite/append `X-Forwarded-For` as the supplied Caddy configuration
does.

## Configuration reference

| Variable | Default | Purpose |
|---|---|---|
| `H2_MASTER_STATE` | `.local/master` | Persistent directory for key and SQLite |
| `H2_MASTER_BIND` | `127.0.0.1` | Listener address |
| `H2_MASTER_PORT` | `8080` | Listener port |
| `H2_MASTER_PUBLIC_URL` | `https://localhost:8080` | Advertised base URL; no path/query/userinfo |
| `H2_MASTER_PEERS_FILE` | `<state>/peers.json` | Pinned peer array, read at startup |
| `H2_MASTER_TLS_CERT`, `H2_MASTER_TLS_KEY` | unset | Direct TLS certificate and key |
| `H2_MASTER_BEHIND_TLS_PROXY` | unset | `1` trusts an isolated TLS proxy |
| `H2_MASTER_ALLOW_HTTP` | unset | `1` enables explicit development HTTP |
| `H2_MASTER_ADMIN_TOKEN_FILE` | unset | Enable bootstrap with a secret file, ≥32 characters |
| `H2_MASTER_LEASE_SECONDS` | `90` | Lease, 1–180 seconds |
| `H2_MASTER_SYNC_SECONDS` | `15` | Peer interval, 0.1–3600 seconds |
| `H2_MASTER_RATE_PER_MINUTE` | `600` | Per-source token bucket, 1–100000 |
| `H2_DISCORD_CLIENT_ID` | `1558522073272426519` | Existing Discord application's public ID |

See [examples/env.example](../master_server/examples/env.example) for local
values. Shell exports or Compose environment mappings configure the service;
it does not load arbitrary `.env` files itself. Without direct TLS, explicit
proxy mode, or development opt-in, startup fails.

## Storage, upgrades, and resource expectations

Back up each complete state directory, including its original master key and
SQLite database. Stop that service for a filesystem copy (including any WAL),
or use SQLite's online backup API. Restore the matched key/database together.
Do not clone a live master's state to run a second independent master: it would
reuse the signing identity and authoritative revision bank. Rebuilding images
and recreating containers preserves named volumes. The current schema uses
idempotent creation; future schema changes need explicit migration planning.

Version 1 caps owners, credentials, and total origin-instance records at 1,000
each; expired record watermarks count toward capacity. Maximum peers: eight.
Monitor capacity and database size; do not delete watermarks to make room without
a protocol-level retirement plan. Expired listing payloads are excluded from
active discovery; replay nonces are physically cleaned up automatically. Owner
authorization records persist for revocation and restart safety.

One service, one SQLite file and full snapshots keep initial costs low. A
1–2 GiB VPS is a reasonable starting target, not a benchmark guarantee. Local
Compose caps the Python process at 256 MiB; measure actual workload before
increasing registry limits. Docker Desktop's VM overhead is separate from VPS
daemon usage. Federation bandwidth grows with stored records × peers × polling
frequency, so do not use one-second development sync for a large public mesh.
There is no paid managed service requirement; VPS/domain/egress costs depend on
the chosen provider and are not estimated here.
