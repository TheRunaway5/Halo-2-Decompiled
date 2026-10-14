# Persistent carnage history

The master stores a normalized, immutable report alongside its existing result,
player/team standings and skill ledger. A match is recorded only after every
admitted physical client validates and confirms the same native Vista report.
This establishes client agreement; the master does not independently simulate
gameplay or prove anti-cheat integrity.

New reports preserve the actual 304-byte selected variant, admission/completion
times, authenticated players' chosen screen names and verified Discord usernames
at the match, appearance, all 45 general stat slots, 32 medal slots, 45×8 weapon
fields, authenticated opponent records and admitted team statistics. Sparse
source-index arrays omit zero entries. Unidentified source fields retain their
indices instead of receiving guessed names. The original 15-bit stat accessors
own decoding. The complete consumed report is not persisted as a raw blob.

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

These are public, read-only endpoints under the existing bounded-request and
rate middleware. The future website may proxy them from its own origin. No
website or CORS policy is added by this change.

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

Release `20261013T185856Z` is active on `master.halotoforever.com`. All 75
frozen source files match the running container. 168 backend tests pass locally
and in the VPS image. An independent two-process native check completes an
eight-player match, full Vista PCR, Done and requeue using the TU1.5 policy;
website lookup returns the four authenticated players and excludes the four
guests. Discord authorization is the external fixture in that automated check,
not a fresh live Discord account verification. The stopped pre-release backup
restores in an isolated container, matches all retained-state hashes and accepts
the additive history schema with successful integrity/foreign-key checks.
Evidence: `.local/online-completion-20261013/verification.json`.
