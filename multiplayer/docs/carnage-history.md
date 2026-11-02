# Persistent carnage history

The master stores a normalized, immutable report alongside its existing result,
player/team standings and skill ledger. A match is recorded only after every
admitted physical client validates and confirms the same native Vista report.
This establishes client agreement; the master does not independently simulate
gameplay or prove anti-cheat integrity.

The current verified-summary publisher preserves the actual 304-byte selected
variant, admission/completion times, authenticated players' chosen screen names
and verified Discord usernames at the match, and accepted player/team summary
standings. `native-verified-accuracy-v3` additionally records the separately
verified shot/hit counters and their coverage. It does not turn client agreement
into independent simulation truth.

Summary publication does not manufacture appearance, medal, general-stat,
weapon or opponent detail. Existing detailed rows retain their source indices:
45 general slots, 32 medal slots, 45×8 weapon fields and authenticated opponent
records. Sparse arrays omit zero entries. Their presence is reported separately
from summary/accuracy coverage; unknown semantics do not receive guessed names.
The consumed report is not persisted as a raw blob.

Guest personal rows, names, appearance, weapon/medal statistics and opponent
columns are excluded. Team totals describe the team as a whole. Stable random
public aliases are separate from Discord subject IDs, private account keys and
match relay identifiers; public history never returns those private identifiers
or authentication tokens. Search uses the currently verified Discord username;
renaming a Discord or Halo name preserves the name snapshot in existing reports.

## Website API

- `GET /v1/history/players?discord_username=exact_username` finds the matching
  verified current username when it has recorded matches. Unknown lookups never
  create accounts. Search is exact and case normalized.
- `GET /v1/history/players/{public_player_id}/matches?limit=20` returns newest
  first. `limit` accepts 1–50. Follow the opaque `next` cursor with `before`;
  completion time plus the public match alias gives stable pagination for ties.
- `GET /v1/history/matches/{public_match_id}` returns the normalized report,
  current names, names at the match and available source details.
- `GET /v1/history/players/{public_player_id}/profile` returns the current public
  customization and career totals, separately from historical PCR appearance.

These are public, read-only endpoints under the existing bounded-request and
rate middleware. The future website may proxy them from its own origin. No
website or CORS policy is added by this change.

Rust now owns these public-read SQL statements, query/cursor validation,
pagination and field/detail projections in `matchmaking_public_history.rs`.
The temporary Python adapter supplies HTTP/SQLite IO. Current-profile validation,
career-total queries and the revision/expiry/LRU cache now also belong to Rust;
history calls that native owner directly. Population observations, general HTTP
middleware and the complete Rust service replacement remain open work.

Totals enter the shared cache only after an owned SQL transaction commits.
Reads inside an outer transaction bypass that cache, preserving uncommitted
visibility without publishing values that may later roll back. The current
[profile ownership receipt](evidence/matchmaking-rust-parity-20261028/profile-validation.json)
records the authorization, transaction and cache controls.

History reads preserve an existing caller transaction and do not advance the
registry's durable clock floor. Duplicate/unknown query fields and malformed
cursor types reject. Pages remain bounded to 50 matches. Detailed weapon rows
are read in 128-row SQL pages without truncating the full 360-row bank. An
ambiguous stale-username search exceeding the native 128-row reply bound rejects
instead of silently returning a partial bank. These are modern API resource
rules, not recovered Xbox Live behavior. See the
[current ownership audit](matchmaking-rust-parity-audit-20261028.md#rust-public-history-read-ownership)
and [validation receipt](evidence/matchmaking-rust-parity-20261028/public-history-validation.json).

The additive schema lives in the existing persistent `registry.sqlite3` volume.
Report details, result summaries and skill progression share one transaction.
A detail failure rolls back the entire new result. Duplicate confirmations
cannot append a second report or award additional XP. Existing summary-only
matches receive public aliases and `legacy-summary-v1`; absent old names,
variant buffers, dates and detailed statistics remain unknown rather than being
invented. Back up the full SQLite volume/WAL using the production stopped-volume
workflow before release activation.

## Rank configurations and the selected era

The requested default target is 2010 plus an additional unranked Action Sack
with Fiesta and couch guests, recorded in `online-default-target.json`.
`title-update-1.5-defaults` is an independently verified executable default
configuration, alongside `retail-launch-2004`. It is not a recovered final 2010
TMS download. Both player and clan XP select and audit their actual table digest.
Existing per-playlist skill cannot be silently interpreted using another policy:
create a separately versioned playlist or perform an explicit operator migration.

The source audit and dated catalog inventories are linked in
`online-historical-config-followup-20261013.md` and
`action-sack-fiesta-source-followup-20261013.md`. Full final 2010 rotations,
complete native Action Sack/Fiesta, host migration, community admission and
original clan-rank publication remain separate unfinished requirements.

## Historical deployment receipt

The retained 2026-10-13 receipt records release `20261013T185856Z` on
`master.halotoforever.com`, 75 frozen source files and 168 passing backend tests
locally and in its VPS image. It records an independent two-process check of an
eight-player match, full Vista PCR, Done and requeue using the TU1.5 policy;
website lookup returns the four authenticated players and excludes the four
guests. Discord authorization is the external fixture in that automated check,
not a fresh live Discord account verification. The stopped pre-release backup
restores in an isolated container, matches all retained-state hashes and accepts
the additive history schema with successful integrity/foreign-key checks.
Evidence: `.local/online-completion-20261013/verification.json`. This is dated
deployment evidence, not activation or verification of the current Rust work.
