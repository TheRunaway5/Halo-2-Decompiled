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
Native v12 uses authenticated Slayer admission and direct player-hosted
gameplay/voice, with confirmed player/team results uploaded afterward. Legacy
relay protocols remain in regression fixtures. An explicit versioned
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
python -m unittest discover -s multiplayer/master_server/tests -t . -v
```

The integration test starts two real processes, registers and updates a server,
stops and restarts its origin, checks discovery failover, then crashes a real
simulator and waits for both directories to expire the listing.

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
