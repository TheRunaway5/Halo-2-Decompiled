# Pinned-peer federation

Each origin is the sole writer of its listings. It increments a durable
`revision` and signs the entire origin record on register, update, heartbeat,
unregister, or revocation. A copy never acquires a new lease merely because a
peer forwarded it. There is no primary role required for discovery: each
master independently serves the active records it knows.

## Peer configuration and exchange

`peers.json` is an array of `{id, public_key, url}` objects, at most eight.
The ID must derive from the pinned public key. Operators exchange fingerprints
out of band and configure both ends. `/v1/masters` advertises configuration; it
does not establish trust. HTTPS authenticates transport; pinned Ed25519 keys
authenticate exchanges and records. Restart after changing peer configuration.

Every 15 seconds by default, each master concurrently POSTs a signed envelope
to each configured peer's `/v1/federation/sync`. The payload is:

```json
{"v":1,"kind":"sync","sender_id":"UUID","recipient_id":"UUID","issued_at":1791640000,"nonce":"fresh UUID"}
```

The receiving peer verifies its pinned sender key, recipient, version, type,
30-second timestamp window, and unused nonce. It stores nonces durably for
61 seconds, longer than the complete accepted timestamp window. It returns a
signed `snapshot` payload with the same fields/nonce, its own sender ID, the
requester's recipient ID, and `records`, an array of original signed listing
envelopes. The requester verifies the responding peer and bound nonce, then
verifies **each original origin signature** before committing the batch.
Responses cannot be replayed into a different request or requester.

All signatures use the domain and canonical encoding documented in the API.
Requests are at most 16 KiB; responses at most 4 MiB and 1,000 records. Version 1
uses full snapshots, not deltas. Peer requests have five-second total timeouts;
redirects and ambient proxies are disabled. Failures are independent, and local
registration/discovery keeps working during a partition.

Trust applies to both transport peers and listing origins. A three-master mesh
must pin all three origins on each member. Receiving C's signature through B
does not implicitly trust C. An untrusted origin or invalid record rejects the
whole batch. No key downloaded from a peer is trusted automatically.

## Record ordering and expiration

Signed origin listing fields are `v`, `kind: listing`, `origin_master_id`,
`server_id`, `server_public_key`, opaque `owner_id`, `credential_id`, `revision`,
`registered_at`, `last_heartbeat`, `expires_at`, `state`, and `metadata`.
States are `active`, `deleted`, and `revoked`. A receiver checks UUID/key binding,
schema, the configured origin signature, non-future heartbeat, and a maximum
180-second signed lease. It never accepts a peer as a writer for its own origin.

Records are stored by `(origin_master_id, server_id)`:

1. Higher revision replaces the older signed record.
2. Lower revision is ignored.
3. Equal revision with identical bytes is a duplicate; different signed bytes
   are origin equivocation and reject the batch.
4. Discovery requires `state == active` and `expires_at > local_now`.

For the unusual case of one instance key enrolled at multiple origins, discovery
deduplicates the UUID and chooses the lexicographically first matching active
origin. Updates remain origin-scoped; credentials and owner mappings do not
replicate. This is not global enrollment or automatic writer failover.

The last signed record per instance is retained even after expiry, including
deletion/revocation tombstones. These durable high-water marks block stale
resurrection, including across restarts and repeated gossip loops. Expired
records are removed from active discovery automatically rather than deleting
their anti-replay evidence. The persisted clock floor also prevents an expiry
already observed by this master from being undone by a clock rollback.

Retaining original envelopes makes forwarding idempotent: B sending A's record
back to A or C cannot extend expiry or increase its revision. Version 1 bounds
the total stored record count to 1,000, including watermarks. It intentionally
refuses new instances at capacity instead of deleting security history silently.
Compaction/epochs and incremental sync are future protocol work.

## Failures and recovery

If A stops, B continues discovery until A's signed leases expire. The test client
tries its configured endpoints sequentially with bounded HTTP timeouts. A
successful empty result does not fall through to a different master. Discovery
availability does not guarantee a game server is reachable.

Instances enrolled at A renew with A; B cannot accept their A-scoped credentials.
The simulator retries temporary origin failures and re-registers if its lease
expired. If A returns with its original state/key, new higher revisions converge
on the next successful poll. During a long outage, listings correctly disappear
until their origins renew them. Automatic multi-origin credential provisioning
is outside v1.

Revocation and DELETE propagate on the next successful poll. A partitioned peer
can continue advertising a previously active record until its signed lease
expires (at most 180 seconds plus allowed clock skew); revocation cannot be
instant across an unavailable link. This is bounded eventual consistency, not
consensus or a globally authoritative availability check.

Operate with synchronized clocks. A forward clock jump can prematurely expire
listings and the clock floor deliberately will not reverse that decision. A
clock error exceeding the freshness window rejects signatures; do not reset
the database/key to conceal it.
