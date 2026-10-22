# Version 1 HTTP API

All public deployment endpoints use HTTPS. Bodies and responses are JSON.
Mutating requests require `Content-Type: application/json`; compressed request
bodies are rejected. Maximum request body: 16 KiB. Metadata is capped at 2 KiB
after canonical serialization. Federation snapshots are capped at 4 MiB.
Unknown object fields, duplicate JSON keys, booleans in integer fields, control
characters in text, and non-finite numbers are rejected.

The machine-readable contract is [protocol.schema.json](../master_server/protocol.schema.json).
The named `$defs` cover metadata, partial updates, discovery, enrollment,
credentials, signed announcements, signed origin listings, sync, and snapshots.
Runtime checks additionally enforce UTF-8 byte limits, signature verification,
cross-field player counts, UUID/key derivation, timestamps, and state transitions.

## Discovery

| Method | Path | Result |
|---|---|---|
| GET | `/v1/servers` | `{"servers": [publicListing, ...]}` |
| GET | `/v1/servers/{server_id}` | One active public listing; otherwise 404 |
| GET | `/v1/masters` | `master: {id, public_key, url}` and configured `peers` |
| GET | `/v1/health` | `status`, `ready`, `master_id`, `uptime_seconds`, peer status |

Filters on discovery are exact, case-sensitive strings: `game_version`, `map`,
`gametype`, `region`, `server_type`. `available_slots` is an integer from 0 to 16
and selects `max_players - players >= available_slots`. Region is an optional,
operator-declared tag such as `US-EAST`, not IP geolocation. Unknown or repeated
query parameters return 400. Results are sorted by server UUID. An empty list is
a successful response and does not trigger failover.

```json
{
  "servers": [{
    "server_id": "39e44990-602b-5a45-a19d-b13397b2d531",
    "owner_id": "a04a138d-174f-48b1-9174-7edd82752f73",
    "origin_master_id": "3583c9e1-f04c-58fc-974d-c6ee6ef112de",
    "registered_at": 1791640000,
    "last_heartbeat": 1791640020,
    "expires_at": 1791640110,
    "reachability": "unverified",
    "name": "Example simulated Halo 2 server",
    "host": "203.0.113.10",
    "port": 2002,
    "map": "lockout",
    "gametype": "Slayer",
    "players": 2,
    "max_players": 16,
    "game_version": "simulator-v1",
    "visibility": "public",
    "server_type": "dedicated",
    "region": "US-EAST"
  }]
}
```

Example UUIDs/times above are illustrative, not live credentials or endpoints.
Names, map, gametype and version are nonempty text, at most 64 UTF-8 bytes each.
Host is an IP literal or DNS hostname, at most 253 bytes, without URL syntax.
Unspecified/multicast addresses are rejected; no reachability inference is made.
Port is 1–65535. Players are 0–`max_players`; capacity is 1–16. Visibility is
`public` or `private`; type is `dedicated` or `player-hosted`. Optional region is
2–16 uppercase ASCII letters, digits, or hyphens. A private listing is omitted
from both public list and individual lookup, even if its UUID is known.

Public responses contain an opaque owner reference, never the Discord subject,
credential ID, server key, or signature. All timestamps are Unix UTC seconds.
Clients should discard expired results even if cached locally. Discovery is
anonymous; game admission/authentication is outside this API.

## Owner provisioning

Use the existing Discord OAuth access token as `Authorization: Bearer <token>`.
This daemon verifies Discord's fixed HTTPS `/oauth2/@me` and `/users/@me`
endpoints, the configured application ID and `identify` scope. It does not
exchange codes, create Discord sessions, or persist access/refresh tokens.

| Method | Path | Request/result |
|---|---|---|
| POST | `/v1/credentials/challenge` | `{server_public_key, ttl_seconds?}` → short-lived enrollment payload |
| POST | `/v1/credentials` | `{server_public_key, ttl_seconds?, proof}` → credential, 201 |
| GET | `/v1/credentials` | Owner's credential IDs, instance IDs, expiry and revoked flag |
| DELETE | `/v1/credentials/{credential_id}` | Owner-only revocation → `{"revoked": true}` |

Keys are lowercase 64-character Ed25519 public-key hex. Credential lifetime is
1 second to 90 days, default 30 days. A credential response contains
`credential_id`, `server_id`, `owner_id`, `origin_master_id`, and `expires_at`.
There is no bearer server secret: signing requires the instance private key.
An instance key can be enrolled only once per origin; re-enrollment returns 409.
First request a challenge with the same bearer identity, key and lifetime.
Sign the returned payload with the instance private key using the directory
signing envelope (`proof: {payload, signature}`). The `enrollment` payload binds
the master, authenticated owner, public key, lifetime, random nonce and expiry.
It expires after 120 seconds and is consumed atomically with enrollment. Each
owner may hold eight pending challenges; replay and cross-owner proofs fail.
The CLI performs both requests automatically.

When `H2_MASTER_ADMIN_TOKEN_FILE` is configured, operator bootstrap supports
`POST /v1/admin/credentials` with an additional optional `owner_label`, and
`POST /v1/admin/credentials/challenge` using that same label and key proof, plus
`DELETE /v1/admin/credentials/{credential_id}`. Its bearer token grants master
administration, not a Discord identity. These routes are unavailable without
the secret. The production Compose service leaves them disabled.

## Signed server lifecycle

| Method | Path | Signed `metadata` | Result |
|---|---|---|---|
| POST | `/v1/servers` | Complete metadata | New/expired/deleted instance registration, 201 |
| PATCH | `/v1/servers/{server_id}` | Partial metadata | Validated merge; renew lease, 200 |
| POST | `/v1/servers/{server_id}/heartbeat` | `{}` | Renew active registration, 200 |
| DELETE | `/v1/servers/{server_id}` | `{}` | Remove immediately and sign tombstone, 200 |

The JSON body is an envelope:

```json
{
  "payload": {
    "v": 1,
    "kind": "announcement",
    "master_id": "3583c9e1-f04c-58fc-974d-c6ee6ef112de",
    "server_id": "39e44990-602b-5a45-a19d-b13397b2d531",
    "credential_id": "979697e8-fddf-4c45-ac21-1510f8dd811b",
    "sequence": 1,
    "issued_at": 1791640000,
    "method": "POST",
    "path": "/v1/servers",
    "metadata": {}
  },
  "signature": "128 lowercase hex characters"
}
```

Replace `{}` with full metadata for registration. The example signature is a
placeholder; use the supplied CLI to make valid signatures. Credential and
server IDs must match enrollment. Master, method, and path must match the
actual destination exactly. `issued_at` must be within 30 seconds of the
master's clock. Sequence is an integer 1–9007199254740991, strictly greater than
the last accepted sequence. Persist the next sequence before sending; gaps are
allowed and lost replies must not cause sequence reuse. HTTP method/path and
origin audience are inside the signature.

Signing bytes are ASCII `halo2-directory-v1` followed by one LF byte, then the
canonical JSON payload, without a trailing newline. Canonical JSON sorts object
keys, uses `,` and `:` with no whitespace, escapes non-ASCII text with lowercase
`\u` escapes (surrogate pairs above U+FFFF), leaves `/` unescaped, and permits
only the contract's integer numbers. This is the Python
`json.dumps(sort_keys=True, separators=(",", ":"), ensure_ascii=True, allow_nan=False)`
encoding, **not** RFC 8785/JCS. Keys/signatures are hex, not Base64. `protocol.py`
is the reference implementation.

The master creates `registered_at`, `last_heartbeat`, and lease expiry; it does
not adopt the client's freshness claim. Default lease is 90 seconds, maximum
180, clamped to credential expiry. PATCH and heartbeat require an active lease;
after expiration they return 404 and the instance must register again. A
duplicate active registration returns 409. Successful mutation responses use
the public listing shape; DELETE returns its last metadata as acknowledgement,
but the instance is already absent from discovery. Revoked credentials cannot
register again; a voluntary DELETE can be followed by fresh registration.

## Errors and readiness

Errors are JSON `{"error": "reason"}`. Common status codes: 400 invalid schema,
401 missing/invalid signature or stale timestamp, 403 wrong owner/audience
authorization or revoked credential, 404 missing/inactive listing, 409 replay
or duplicate, 413 size limit, 415 encoding/content type, 429 rate limit, 502/504
upstream failure, 503 capacity or unavailable worker. Retry transport/upstream
failures with bounded backoff; do not retry an invalid credential indefinitely.

Health is ready when SQLite responds and the maintenance worker is running.
A failed peer is reported with a failure count and last-success timestamp, but
does not make the local directory unready. Peer failure counts are in memory;
the registry, nonces, master identity, and replay watermarks are durable.
