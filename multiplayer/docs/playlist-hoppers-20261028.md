# Reconstructed ONLINE hoppers

Source implementation, 2026-10-28. The user approved documented project tuning
where final-era weights are missing. The canonical catalog supplies **13 regular
playlists with 463 ordered map/mode rows**, plus the user-requested **Lab Test**
playlist with96 rows:14 playlists/559 rows total. Regular playlists use revision
`h2f-final-era-v1`; Lab Test uses its separate `lab-test-v1` revision.
The reconstructed catalog is active after qualification and local installation
of its compatible v16 native client. The
[accuracy client/API/website release](shot-accuracy-delivery-20261028.md)
records the exact runtime, image/catalog pins and preserved history checks.
No Epic portal change was needed.
The broader ONLINE/security/migration acceptance remains open.

## Sources and reconstruction policy

The supplied [master reference PDF](reference/halo2-original-matchmaking-master-v5.pdf),
[executed selector/source research](playlist-hopper-sources-20261028.md),
[recovered functional rows](../master_server/data/recovered-hopper-rows-v1.json)
and [dated published membership](../master_server/data/published-hopper-membership-v1.json)
remain separate evidence. No authenticated complete final-2010 download,
complete downloaded custom-option set or final-2010 numeric weights were found.

The late playlist identities and ranked/casual classifications use the released
2007 announcement. Most map/mode pairings use Bungie's published December 2005
tables. The four observed SWAT maps come from 2005 Team Training membership;
they do not establish a complete dedicated 2007/2010 SWAT pool. Clan rotations
retain executable fallback rows with explicit later additions. These are
evidence-supported reconstructions, not a claim that all these configurations
ran together on Xbox Live in 2010.

Eighty-two selected rows retain recovered executable weights and complete
option buffers when their map/mode pairing is admitted. The remaining rows
declare project tuning. A published pair with missing options uses an explicitly
authored equivalent built through the actual native option setters. The manifest
records each row's origin, settings hash, stored weight and exact draw count.
Recovered launch weights are not relabeled as recovered 2005/2010 weights.

## Rotation frequencies

These percentages enumerate all 65,536 high-word draw values of the original
selector with **every catalog map available**. They are not measured from random
sampling. The [machine-readable manifest](../master_server/data/online-hopper-frequencies-v1.json)
also gives exact frequencies for every individual map and named variant.
Displayed percentages can differ from 100% by rounding. Missing maps remove
their rows and change the remaining conditional frequencies.

| Playlist | Players | Ranked | Rows | Slayer | CTF | Assault | Oddball | King | Territories |
| --- | ---: | :---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Rumble Pit | 6–8 | No | 45 | 88.78% | — | — | 4.21% | 7.01% | — |
| Double Team | 4 | Yes | 41 | 77.78% | 8.89% | — | 6.67% | 6.67% | — |
| Team Slayer | 6–8 | Yes | 29 | 100.00% | — | — | — | — | — |
| Team Skirmish | 6–8 | Yes | 55 | — | 39.02% | 21.14% | 13.01% | 17.07% | 9.76% |
| Team Training | 4–8 | No | 100 | 64.94% | 13.51% | 7.47% | 4.60% | 6.03% | 3.45% |
| Team Snipers | 6–8 | Yes | 17 | 100.00% | — | — | — | — | — |
| Team Hardcore | 6–8 | Yes | 33 | 45.00% | 15.00% | 5.00% | 12.50% | 22.50% | — |
| Team SWAT | 6–8 | No | 6 | 100.00% | — | — | — | — | — |
| Big Team Battle | 10–16 | No | 35 | 59.72% | 18.75% | 15.97% | — | — | 5.55% |
| Minor Clan Match | 6–8 | Yes | 28 | 22.22% | 34.57% | 19.75% | 7.41% | 6.17% | 9.88% |
| Major Clan Match | 12–16 | Yes | 21 | 57.89% | 23.68% | 11.84% | — | — | 6.58% |
| Multi-Team | 12 | No | 41 | 81.61% | — | — | 5.75% | 4.60% | 8.05% |
| Action Sack | 6–8 | No | 12 | 100.00% | — | — | — | — | — |
| Lab Test | 2–16 | No | 96 | 37.50% | 12.50% | 12.50% | 12.50% | 12.50% | 12.50% |

Action Sack is exactly **50% Fiesta Slayer / 50% Infection Slayer**. Both are
already bundled custom Slayer extensions, so the family column is Slayer.
It uses six maps per mode, including Tombstone and Desolation. Juggernaut is
supported by the native option representation but has no invented default
row merely to increase the catalog's engine count.

## Lab Test

The explicit testing playlist starts gathering at **two players**, permits up
to16 and uses a **one-second population countdown**. Its ordinary authenticated
connection checks, loading/readiness barriers and game-start countdown still
apply. One signed-in player alone continues searching. Guest seats count toward
the two-player minimum; a primary plus guest can enter the service's handoff on
one connection. Test results do not advance ranked or hidden XP because this
playlist has no rank configuration. General carnage history remains scoped to
its separate `lab-test` ID.

All eight named variants have equal configured frequency: Team Slayer, Team
Snipers, Team SWAT, Multi Flag CTF, Single Bomb, Team Ball, Team King and3 Plots.
Each spans the same twelve MCC maps: Lockout, Midship, Beaver Creek, Foundation,
Warlock, Sanctuary, Desolation, Tombstone, Zanzibar, Coagulation, Terminal and
Waterworks. This gives each variant12.5% with all maps available. Settings retain
their native team rules; this playlist does not reinterpret FFA Fiesta/Infection
as team variants. Its rotation, population bounds and clock are explicitly
authored for testing, with no claim of historical service provenance.

When Lab Test was added, the original13 regular entries remained byte-for-byte
identical after canonical JSON serialization, with combined SHA-256
`7ab51fe2c51e577681bf79b0503698c05f10e5bdeec766a5f1193d6f2db78cb3`.
The subsequent accuracy release changes their required client capability to v16;
their rows, options and frequency manifest remain unchanged.
Quickmatch still defaults to Rumble Pit. Lab Test is an intentional exception to
the removal of the old Lab prefixes from regular playlist names.

[Lab service validation](../../build/test-results/playlist-lab-20261028/service/report.json)
exercises two solo accounts and a primary/guest pair with the real short clock,
real local QoS sockets and actual gathering/team/transport-descriptor owners.
Identity is an explicit fixture; this is not a new installed-client, played-match
or Internet/EOS SDK session check.

## Regular rotation tuning

For unrecovered weights, primary Slayer/rifle/duals rows generally use8 and
objective/novelty rows2. Hardcore uses8 for its primary Slayer rows and4 for
objectives; Snipers uses4 per map. BTB Team Slayer BR uses12 to retain the
published Slayer emphasis. Recovered matching weights take precedence over
these defaults. MCC additions have explicit independent weights in the builder.
These values are project choices, not facts extracted from published row order.

Tombstone uses MCC scenario key **`highplains`**, scenario-owned map ID1402.
Desolation uses `derelict`, ID1400. The generator validates actual per-engine
team capacity from the retained MCC scenario metadata, including its overrides
for stale menu IDs. Suitable Tombstone rows cover FFA/team Slayer, CTF, Assault,
Snipers, SWAT, clans, Multi-Team and both Action Sack modes. Original Xbox
filenames are not used as current MCC runtime identities.

## Runtime contract

The [Rust selector](../../rust/online_protocol/src/hopper_selection.rs) implements
the unchanged TU1.5 ordered draw: unavailable rows weigh0, available signed
weights clamp to1..1000, one original LCG step feeds the cumulative draw, and
duplicates retain separate intervals. Empty pools spend no RNG. Authoring
rejects more than100 rows and effective totals above32766, where the original
signed-short corridor overflows. The initial service seed and upstream service
call ordering are replacement ownership, not recovered Xbox Live behavior.

Both gathering and legacy service selection return the same immutable complete
row through [the Rust ABI adapter](../master_server/hopper_selection.py).
Gathering creation retains its selected row; later joiners must own that map.
Authentication binds identity; reported installed-map capabilities are not
anti-cheat attestation. Empty intersections cannot consume a draw or admit a
proposal. Casual, ranked and clan team assignment uses the existing original
Rust caller with a complete 0x614-byte header. Reconstructed headers carry
validated executable-default rank-map bytes, with provenance in the manifest;
later downloaded rank maps remain unknown.

The queue response carries the row ID, all304 native settings bytes and SHA-256.
The native client verifies and retains those settings instead of resolving an
unfamiliar objective name as default Slayer. Peer configuration includes the
same complete buffer. Accepted results validate the admitted engine/team rule
and store the selected options in `match_result_options`, in the existing result
transaction. This does not claim the complete future immutable admission/result
envelope required by the broader parity/security goal.

The canonical catalog requires **`halo2-native-queue-v16`**. Older clients see
it as ineligible and cannot enqueue. Native configuration datagrams use
at least **protocol121** for the [checked native accuracy projection](shot-accuracy-delivery-20261028.md);
the source's EOS account-mode default selects v16. Client,
Rust service library and catalog must be released coherently. Peer gameplay and
voice transport ownership are unchanged. EOS Sessions/traversal does not supply
Halo map/mode weights; no Epic portal edit is required for this catalog.

New playlist IDs have explicit revisions and TU1.5 rank configuration. Existing
historical IDs/configurations are not silently reassigned to a new XP table.
Regular display names and active example metadata no longer use the old Lab convention.
The explicit user-requested Lab Test retains its descriptive testing name.
Historical receipts retain their original names and bytes.

## Deliberately unadmitted published rows

The manifest preserves eight open source records covering eleven map/mode
pairs. They are omitted from active pools rather than silently replaced with
Slayer or guessed game rules:

- Team Training: Fiesta Team Slayer on Ascension, Ivory Tower and Lockout needs
  admission of a native team extension; Team Showers on Backwash and Lockout
  lacks complete recovered rules.
- Multi-Team: CTF Classic/Neutral Flag on Warlock and Neutral Bomb on Foundation
  need per-row two-team admission; this catalog has an explicit four-team rule.
- Multi-Team: Team Gulch Wars on Coagulation, Containment and Waterworks lacks
  complete recovered rules.

These omissions, incomplete dedicated SWAT membership and unrecovered final-era
downloads prevent a claim of full historical restoration. The remaining usable
pools retain the supported published roles: Skirmish has no Slayer, Doubles
includes objectives, Snipers includes its published large maps, and Hardcore
retains its published ten-map membership.

## Rebuild and validate

From the repository root:

```sh
python3 tools/run_test.py -- python3 -B tools/build_online_playlists.py
python3 tools/run_test.py -- python3 -B tools/build_online_playlists.py --check
python3 tools/run_test.py -- python3 -B tools/validate_playlist_hoppers.py --output build/test-results/playlist-hoppers
python3 tools/run_test.py -- python3 -B tools/validate_playlist_hoppers.py --native --output build/test-results/playlist-hoppers-native
```

Compiler, Cargo, source copies and CMake products remain inside owned scratch.
The generated catalog/manifest are required small source data. The selector
oracle can be regenerated separately through `tools/run_test.py`; its retained
fixture and original execution receipt identify the source executables and
explicit synthetic read/row inputs. They are not a downloaded hopper file.

The local compose overlay now mounts `matchmaking-online.json`. Production uses
an explicitly reviewed `MATCHMAKING_CATALOG` path and a compatible client/library;
adding this file does not change a running server. For small development-only
queues mount `matchmaking-development.json` explicitly.

## Focused completion checklist

- [x] Retain source evidence, original table bytes, published pairs and unknowns.
- [x] Build all13 reconstructed hoppers with named/versioned provenance.
- [x] Add the separately versioned Lab Test with96 rows, eight variants, twelve
  maps, two-player minimum, short countdown and no ranked/hidden XP progression.
- [x] Use original ordered selection and retain all selected settings.
- [x] Validate MCC map identity and per-engine team capacity for every row.
- [x] Enumerate exact per-row, per-map and per-mode frequencies for all14 pools.
- [x] Exercise every row and actual gathering finalization through the Rust ABI.
- [x] Verify authenticated two-PC/four-seat service admission, complete options
  through the EOS descriptor boundary and PCR validation using explicit fixtures.
- [x] Isolate older clients and remove development labels from regular catalogs.
- [x] Pass locked Rust tests and strict Clippy, with416 backend tests in the
  Lab follow-up. The initial regular-pool cohort also passed eight selected
  native CTests; no native runtime change was needed for Lab Test. Verify
  owned build/scratch removal after each run.
- [x] Qualify and install the coherent v16 client, deploy the pinned library/API
  and activate all 14 hoppers. Verify production health and unchanged history;
  remove owned temporary builds and failed candidates.
- [ ] Exercise real rendered queue→load→play→PCR flows for every playlist/mode
  on MCC imports. The release's native Slayer/PCR, launcher/frontend and
  submitted-frame map checks remain distinct from this full acceptance matrix.
- [ ] Recover remaining historical downloads/rules and admit omitted variants
  only after their actual native behavior is supported and verified.

Evidence: [service report](../../build/test-results/playlist-hoppers-20261028/service-final/report.json),
[native report](../../build/test-results/playlist-hoppers-20261028/native/report.json)
and [native CTest log](../../build/test-results/playlist-hoppers-20261028/native/cmake/tests.log).
The native run pins an earlier coherent source cohort; its full-buffer client,
configuration, ingress and UDP tests passed, with unrelated concurrent source
drift listed explicitly. The final service cohort pins the revised 463-row
catalog and current Rust/service sources. Neither run proves installed delivery,
live identity consent, Internet routing, physical voice, all-mode gameplay or
full Xbox parity. No generated binary, archive or framebuffer was retained.
