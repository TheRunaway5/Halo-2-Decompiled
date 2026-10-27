# Friends and EOS storage security hardening

The new friend service keeps stable account ownership, friend consent and match
authorization on the existing authenticated master. EOS stores private encrypted
snapshots; cloud content never grants friendship, changes PUID bindings, supplies
ranks or authorizes match admission. Steam remains the login provider.

| Boundary | Implemented control | Validation |
| --- | --- | --- |
| Public website to private social data | Public GET path allowlist; no bearer/cookie forwarding; friend, token and file routes rejected | Website bridge HTTP tests, including normalized traversal paths |
| Unauthenticated friend mutation | Authenticate before body read and revalidate after the await; exact fields and idempotent command IDs | Real local HTTP tests; malformed unauthenticated upload rejected before parsing |
| Friend request spam | 32 incoming and 32 outgoing pending requests, 250 total roster entries, 256 mutations/day/account | Inbox saturation still permits acceptance and blocking; outgoing saturation rejected |
| Wrong-player actions after refresh | Permanent public IDs retain selection in both the Friends and blocked-player rosters | Native HTTP regression with duplicate blocked names failed before the fix and passed afterward |
| Config disclosure or substitution | Absolute private regular file; no final symlink; POSIX ownership/mode checks; 16 KiB bound and exact keys | Public mode, symlink, duplicate, unknown publisher key and oversized file regressions |
| Worker credential bleed | Small environment allowlist; no Steam/publisher secrets, preload hooks or master state paths; private HOME/cache | A real child process asserts excluded variables and cache cleanup |
| Abandoned processes/private caches | Shield process creation; kill/reap before cache removal; bounded operation timeout and retry backoff | Cancellation during launch, failed reply and successful reply regressions |
| EOS privilege confusion | Dedicated storage client with User required and only `datastorage:access`; publisher/storage RPC modes isolated | Seven Rust checks and saved-config SDK platform qualification |
| Wrong-account cloud writes | Connect login must return the already-bound PUID; no CreateUser/LinkAccount recovery | Official-ABI callback fixtures reject wrong/missing users before any write |
| Cloud tampering/stale restore | Owner, schema, size and monotonic revision checks; exact downloaded read-back before ack | Eleven offline callback scenarios, including oversized, malformed and altered data |
| Runtime packaging | Only runtime source/data copied; canonical full-tree image includes its required Rust security ABI | Canonical images and the separate scoped deployed-baseline images both passed checks; the scoped release preserves existing auth/transport behavior |
| Deployment containment | Private master-only temporary cache; read-only secret mount; core dumps disabled; 128 process/thread limit | Applied and verified on the scoped production release; website cannot access the storage config or old rollout backups |

The scoped Friends master and website API release was deployed with the user's
authorization on October 25, 2026, and the static website was published. The
master retains UID/GID 10001, its read-only root filesystem and 256 MiB memory
limit. Existing keys and Caddy remain unchanged. No launcher runtime was promoted.
The storage config is saved outside both
repositories with mode 0600 in a 0700 directory; its secret and encryption key
are absent from the changed source files checked during setup.

Canonical bounded evidence is under `.local/friend-tags-20261025/`: full backend
suite: 352 passes, website suite: 28 passes, SDK worker checks and eleven transfer
callback cases pass. The real saved config initializes the SDK storage platform
and rejects publisher operations. These are component/integration checks, not
a penetration-test guarantee or a verified real-player EOS upload/download.
The exact deployed baseline plus Friends overlay separately passed 52 applicable
backend checks and 28 website checks. Two newer fixtures fail against the unchanged
baseline and are documented separately. Deployment manifests, private backup
metadata, successful rollback/retry and live health/privacy checks are retained in
the website checkout's `.local/friend-deploy-20261025/`. The scoped patch and public
source hashes are committed with its deployment runbook; unrelated match,
enrollment and countdown work from the shared checkout was not deployed.

Both native checks pass in a frozen Release source cohort: identity/layout and
the authenticated Friends/party HTTP flow, including tag search, explicit
acceptance, duplicate-name blocked-list refresh and party invitation. The test
also exposed a missing bilateral playlist-consent barrier in its fixture and a
consumed countdown being redisplayed as zero; the fixture now waits for all
members, and the UI keeps a consumed countdown cleared. Match admission rules
were not relaxed.

The native checks use an operation-owned copy of imported MCC assets. The normal
layout converter refreshes only that disposable copy and the standard provenance
validator passes before testing. Original imported inputs remain unchanged;
no launcher runtime was promoted. Results, exact source hashes, controlled
fixture revisions and the final two passing checks are recorded in
`native-frozen/qualification.json` and `native-frozen/repair-3/report.json`.
Later independent changes in the shared checkout are outside that frozen cohort.

Production storage is enabled. Existing accounts get tags on their next verified
connection; the live postflight found four existing accounts and zero tags or
queued cloud records, confirming no eager backfill or fixture accounts. A real
player's EOS write and exact read-back still require that connection. Keep one
master writer per database. A stale database restore
conflicting with a newer cloud revision requires explicit reconciliation. Back
up the consent ledger and the private encryption key; do not regenerate the key
on restart or import cloud records as player-supplied consent.

For future activation, first back up the coordinating database and private storage
config, then qualify the exact deployment candidate with storage disabled.
Mount the private config only into the master and enable the friends override
after its normal health and account checks pass. Use a verified account to
confirm the bound PUID and a successful cloud read-back before treating storage
as live. If sync needs rollback, disable its feature flag and retain both the
ledger and key; do not delete cloud files or change existing multiplayer client
policies. Deploying unrelated changes from a shared dirty checkout requires
their own qualification.
