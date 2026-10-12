# Versioned native playlist progression

The optional catalog field `rank_configuration: "retail-launch-2004"` selects
the literal retail executable's initialized XP tables and original
`746B0 → 74800 → 74970` arithmetic. It is an explicit launch-era configuration,
not a claim that the retired Live service used unchanged tables in every era.
Downloaded `network_configuration.dat` files could replace these values.
See [original evidence](online-rank-playlist-evidence.md).

This field applies to both ranked and casual playlists. Original hopper types
1/3 and 2/4 use the same recovered update routines; only 2/4 display their
level. Authenticated casual catalog responses retain `level: null`, and queue
responses publish no skill or XP. Search admission still uses the authoritative
private per-playlist skill. Changing only a playlist's visibility or revision
preserves its skill under the same playlist ID. See the
[executed casual controls and contemporary Bungie evidence](halo2-casual-skill-evidence.md).

The server freezes previous raw rank and XP when admitting a room. Native
clients cannot submit rank, XP, effective skill or persistent identities. Each
currently admitted native player has one machine and one authenticated account;
the persistent raw rank is the effective pairwise skill in that model. Search
policy and search-party averaging remain separate owners. Original same-machine
skill adjustment is documented in the evidence and is not generalized to remote
parties.

After every admitted player validates the final native snapshot and confirms the
same result digest, one SQLite transaction writes the immutable result, numeric
statistics, actual team standings/memberships, XP deltas and updated playlist
skill. A tie remains in the opponent divisor; teammates are excluded. The code
preserves signed truncation, low-level loss mitigation, XP boundaries and the
strict midpoint for one-rank demotion hysteresis. Team standings, rather than
individual scoring order, determine team wins and losses.

Duplicate confirmations cannot award XP twice. A changed authoritative skill
snapshot aborts the whole transaction. Unknown configurations and existing
nontrivial legacy levels without known XP require an explicit migration; the
service does not manufacture XP from a level. Initial level1 safely migrates
to raw0/XP0. Private audit rows retain the exact table digest and before/after
values. Public authenticated catalog responses expose only the requesting
player's displayed level on ranked playlists; chosen names remain independent
of identity.

Stored ranks 0–49 display as levels 1–50. The original final `wigl` artwork
range groups raw49–127 into the halo frame, so the internal threshold routine
retains its exact raw127 boundary while numeric presentation uses that final
level50 grouping. No level above50 is published.

The bundled catalog remains a clearly labeled validation pool: two-player
Slayer and four-player Team Slayer, each offered ranked and casual. Both casual
entries explicitly select the recovered retail XP policy. Their final search
stage removes skill eligibility limits after a custom 30 seconds, retaining
whole-party constraints and measured connection admission. The original client
supports removal of rank filters, but its composite candidate scoring is not
reproduced by the native QoS adapter. Reciprocal eligibility, initial radius,
timing, high-rank radius and rotation remain declared native tuning.
Historical downloaded configurations, original PCR rank-change artwork
ownership, quit penalties and independent simulation/arbitration remain
unresolved. Discord supplies identity only.

Clan hoppers use the separate recovered `746B0 → 74720 → 74970` path.
The launch Minor/Major lab entries require accepted common-clan parties and
keep the full private clan raw rank for search. Their unanimous result writes
one XP change per opposing clan, with actual team standings, into
`clan_playlist_skill` and immutable `match_result_clan_skill` rows. Personal
playlist XP is untouched. Clan audit identities survive dissolution; the live
skill row is deleted with its clan. Frozen-snapshot conflicts roll back the
entire result. See [the original clan controls](halo2-clan-matchmaking-evidence.md).

Reproduce the unchanged-original controls and compare the pinned fixture:

```sh
.tools/python/bin/python tools/oracle_online_rank.py > /tmp/halo2-original-rank.json
cmp /tmp/halo2-original-rank.json multiplayer/master_server/tests/fixtures/retail-rank-original.json
python3 -m unittest multiplayer.master_server.tests.test_playlist_skill \
  multiplayer.master_server.tests.test_ranked_results -v
python3 -m unittest multiplayer.master_server.tests.test_casual_results -v
```

The expected 30 outputs and literal tables are captured from the pinned original
XBE, without importing the native implementation or replacing its rank calls.
The separate HTTP tests verify unanimity, public level refresh, persistence,
idempotence, original team results, migration rejection and transactional rollback.
