# Trust, security, and privacy

## What signatures prove

An instance announcement proves possession of an enrolled Ed25519 private key.
The origin's enrollment binds that key to an owner verified through Discord or
an explicitly configured operator bootstrap credential. The master signature
then attests that this origin accepted the record. Neither signature proves
reachability, map availability, truthful player counts, gameplay integrity, or
compatibility with the Halo 2 executable.

Masters trust configured origins to authorize their own servers and use correct
clocks. A malicious trusted master can invent its own listings or keep issuing
fresh revisions; remove its pin to stop receiving them. It cannot sign another
origin's records. A compromised server key can update that instance until the
credential is revoked or expires. A compromised master key requires removing
its trust at peers and provisioning a new master identity; v1 has no key-rotation
or global account-consensus protocol.

## Controls in v1

- Owner authorization uses the existing Discord OAuth access token, verified
  against fixed Discord HTTPS URLs, application ID, `identify` scope, and user.
  Client-supplied Discord IDs and screen names are never identity evidence.
- Server requests bind the credential, instance UUID, origin, HTTP method/path,
  timestamp and monotonic sequence inside the signature. Successful sequence
  updates and listing revisions commit together in SQLite.
- Credentials default to 30 days, cap at 90 days, and are owner-revocable.
  Instances require no interactive authentication at ordinary startup.
- Federation uses pinned keys, recipient/nonce binding, durable replay nonces,
  original origin signatures, absolute leases, and durable revision watermarks.
- Public HTTPS is required by default. Direct TLS uses TLS 1.2 minimum and peer
  HTTP clients verify certificates. Plain HTTP requires explicit development
  opt-in. TLS-proxy mode is only safe with an isolated backend and trusted proxy.
- Request/response sizes, field lengths, collection sizes, peers, and rate-limit
  bookkeeping are bounded. Compression is disabled on incoming requests. The
  upstream client requests uncompressed responses and rejects content encodings
  without decompression, keeping the response-size cap effective. The
  daemon does not fetch, resolve, or probe advertised game endpoints, so game
  metadata cannot direct an HTTP request to a private service.
- A token bucket permits 600 requests/minute per source IP by default, with an
  initial burst of that size. Owner API calls additionally allow 20/minute per
  source and verified owner; authenticated peer sync allows 600/minute per peer.
  Buckets are bounded to 4,096 keys and reset on process restart.
- SQLite writes are serialized in one process, with WAL and transactions. The
  database is bound to its master ID; a missing/mismatched key fails startup.

Rate limits include health requests; leave headroom for monitoring, especially
if many users share NAT. Limits are local to each master. Service-level limits
do not replace a host firewall or optional provider edge protection against
traffic saturation. No bot detection or global abuse-ban network is claimed.

## Data visibility

The origin stores the Discord subject privately, mapped to an opaque owner
UUID. OAuth tokens are used in memory for verification and are not written to
SQLite, configuration, or logs. HTTP logs include route templates, status and
duration, not raw queries, source IPs, Discord subjects, or bearer credentials.
Federation and discovery never contain Discord subjects. Owner references are
pseudonymous and allow correlation of that owner's servers at the same origin.

`private` means excluded from anonymous discovery. Trusted federation members
still receive private metadata so a later public update or tombstone converges.
Do not place passwords, access tokens, personal information, or private invite
secrets in listing metadata. The owner API lists credentials for the authenticated
owner only; it does not expose another owner's records.

Persist master keys, simulator keys, admin tokens, and databases in protected
storage. Generated private files use mode 0600. Production containers run as UID
10001 with a read-only root filesystem and no capabilities. Bind mounts must
permit that UID to write the state directory. Protect the entire state volume,
not just the private-key JSON: SQLite contains owner mappings. Operators control
retention and backups of those mappings; no user-facing erasure API exists yet.

## Deployment boundary

The supplied production Compose setup publishes only Caddy's 80/443 ports.
The directory trusts the last `X-Forwarded-For` address appended by Caddy only
when explicit proxy mode is enabled. Never publish the backend port in that
mode or place untrusted containers on its network. With direct TLS, proxy mode
stays off and source accounting uses the socket peer. Operator bootstrap is
disabled publicly by leaving `H2_MASTER_ADMIN_TOKEN_FILE` unset.

No TLS bypass flags, client secrets, tokens embedded in URLs, downloaded trust
keys, or hardcoded owner subjects are used. The Discord application public key
is unrelated to server/master Ed25519 keys and is not an OAuth client secret.

## Validation boundary

Automated tests cover lifecycle, filtering, ownership, forged/stale/replayed
requests, expired/revoked credentials, duplicates, malformed/oversized requests,
rate limiting, peer authentication, lease resurrection, direct TLS certificate
validation, and two independent master processes across outages and a simulator
crash. They use an injected owner verifier or fixed-endpoint HTTP stubs rather
than a real person's Discord token. Existing native Discord authentication is
unchanged; a live native-to-owner provisioning action is future integration.

Local Docker checks exercise the actual image, volumes, two masters, signed
registration, federation, and failover. They do not establish public DNS/ACME,
real WAN partitions, load capacity, NAT traversal, or playable multiplayer.
