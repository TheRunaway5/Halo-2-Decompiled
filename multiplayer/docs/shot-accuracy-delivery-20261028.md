# Native shot accuracy and profile delivery

The 2026-10-28 playlist release also carries verified native shot counters to
the public service record. This is a versioned extension to the existing
accepted-result ledger; it does not infer accuracy for old summary reports.

## Counter ownership

[Original execution evidence](shot-accuracy-sources-20261028.md) identifies the
TU1.5 producers `15F6E0`/`15F7D0` and their actual `51EEF4` retained bank.
`source_result_accuracy` projects words `78 + weapon*8 + category` for reporting
types5–38 and categories4/5. There is no parallel keypress, pellet or damage
counter. A shotgun's pellets consume one shot mark; authored burst discharges
retain the original individual-round behavior.

The equivalent PCR address is `0xECC + 2*(slot*437 + weapon*8 + category)`.
An older test fixture used word77; its adjacent values must not become shots.
Counts preserve the original low15 bits. An interrupted recording window can
retain more hits than shots, so neither storage nor display imposes a fabricated
100% cap or silently rewrites the original counts.

## Delivery contract

- The completed native state carries an optional16×34×2 counter projection.
  Protocol121 introduces it; older bodies leave it unknown. Native publication
  requires the original frozen bank. Report confirmation compares every admitted
  player's shot/hit words with the terminal state.
- `halo2-native-queue-v16` identifies this capability. The canonical14 hoppers
  require v16, and different wire/queue cohorts remain isolated. The complete
  selected304-byte game options still travel with the match.
- Rust's additive `h2_result_accuracy_projection` preserves only the known
  shot/hit words plus the existing accepted summary. Legacy clients retain the
  summary-only projection. Unrelated raw PCR fields remain excluded.
- New `carnage_player_accuracy` and `carnage_weapon_accuracy` tables store those
  accepted counters in the result transaction. Non-authenticated guest seats
  have no persistent personal rows. Duplicate commits cannot add totals twice.
- Public profiles sum hits and shots across covered matches. Career accuracy is
  `100*sum(hits)/sum(shots)`, not an average of match percentages. Coverage stays
  separate from all recorded matches. EOS enrichment cannot overwrite those
  native-ledger counts with an older cloud projection.
- Website percentages have a hover and keyboard-focus tooltip such as
  `8 hits · 12 misses · 20 shots`. Career and tooltip counts use compact suffixes
  such as `10.7K` and `1.2M`. Misses are the difference of the same accepted
  shots/hits; an inconsistent recording window leaves misses unavailable.
  Missing historical accuracy and known zero shots both avoid a fake percentage.

## Acceptance checklist

- [x] Execute unchanged original TU1.5 shot-mark/counter controls and retain
  source/layout receipts, including pellets and interrupted recording windows.
- [x] Reproduce the missing publication: a20-shot/8-hit accepted fixture
  initially produced0/0 on the profile; its regression now passes.
- [x] Pass the frozen Rust tests, strict Clippy and425 backend tests. The release
  uses that coherent source cohort, excluding subsequent unrelated shared edits.
- [x] Qualify the scoped overlay on the exact running server baseline:182
  selected tests pass; one original-options fixture was initially absent from
  that local context and is separately covered by the built-image check.
- [x] Build the website, pass TypeScript and focused projection/format tests,
  and inspect the actual production build in the browser. Verified40.0%, zero
  shots, missing historical coverage, tooltip reveal/dismiss and keyboard focus.
- [x] Complete the MCC native firing→actual hit→completed retained bank→PCR
  regression, including terminal counter transport to an admitted UDP peer.
- [x] Qualify and install a compatible client, deploy the pinned service/catalog
  and activate the new catalog after client qualification. Both production
  phases and the website are live; see below.
- [x] Remove the private build/source/SDK owner and failed candidate packages;
  retain the installed deliverable and small receipts.

Small receipts are under `build/test-results/shot-accuracy-20261028/`.
Tests use owned scratch and remove their Cargo/CMake/Next products. Required
delivered runtimes, server image pins and private rollback backups are recorded
separately when activated. Local fixtures never write production player data.

The deployment uses scoped overlays on the pinned master/API images. Existing
credentials, private storage, original assets and unrelated active work remain
owned by their existing deployments. Gameplay and voice remain player hosted;
these changes publish counters only after an accepted completed report.

## Production storage and website phase

The website commit `ad1527c7fbebec90ca7ccdea2a4f92ec430149ca` passed34 tests and
[Pages deployment](https://github.com/TheRunaway5/Halo-2-PC-Website/actions/runs/40738033093).
The production master/API images are now respectively
`sha256:4b707525fdf25956722afd1f74b48838d0ff229a95775a5f76c68404b0dcffdc` and
`sha256:7b50ce3bbc90bb5f63745499d903a3c9cf96546244d53fa77f46f6dee7eeb8f7`.
The initial compatible storage phase retained the prior catalog. The later
hopper transaction activated the reconstructed catalog after v16 client
qualification and installation.

The built server image passed20 actual isolated HTTP/ledger/hopper and original
family tests. Its local scoped service cohort passed182 tests with one absent
reference-input case subsequently covered in that image check. Production
postflight verified the new schema, unchanged history hashes, private-key
identity, website secret containment and public health. The620,863-byte private
SQLite/key backup is outside the website-mounted volume at
`/opt/halo2/hoppers-accuracy-20261028T173500Z/accuracy/backup`.
No production result or account was manufactured for testing.

The first native cohort passed all eight existing selected network/options
checks. The original MCC Slayer run produced exactly25 shots/25 hits from
ordinary firing, and its terminal bank matched the actual retained source
bank. PCR publication correctly refused the test's unselected profiles.
The fixture now declares actual selected local and remote profiles through
normal admission; the guard remains intact. The later release cohort below
completes PCR/peer transport acceptance; client installation is tracked separately.

The public profile and history endpoints now return the existing1-match/4-kill/3-death
record with0 verified accuracy matches,0 shots and0 hits. The unverified historical
weapon rows remain preserved in storage. Live static bundles contain the new
tooltip and unavailable-data presentation. The real production browser check
stalled; the popup visual/focus checks use the local production build and the
live data/asset routes are independently verified by HTTP.

The scoped release cohort now passes all ten selected CTests. Its original
Slayer run retains25 shots/25 hits, publishes and validates the complete PCR,
rejects modified counters and missing observations, and confirms the same
terminal counters through an admitted UDP client. Selected local/remote
profiles are explicit fixture inputs. The additional original raw-shadow
fixture uses its actual asset-owner path, preserving strict resource admission.
See `native-final/cmake-release/` in
the receipt directory; the earlier failing fixture/build receipts are preserved.

## Scoped client boundary

Packaging exposed two unrelated player-shadow/source-depth activation failures
in the shared development cohort. The existing MCC relocation repair preserves
actual World/Cache ownership for BSPs, sections and complete pass preimages.
The shader validator now admits the compiler's exact `source-depth` family,
with executable binding, hashes and SPIR-V audits still required.

The ordinary player-shadow feature itself is still awaiting its own runtime
acceptance. This release restores the prior explicit reference-only submission
boundary in its private frozen cohort; it does not modify that ongoing work in
the shared checkout. The original reference inputs and rejection guards remain.
No smoke failure is accepted or skipped. The small
`.local/accuracy-deploy-20261028/native-shadow-release.patch` and
`native-final/release-shadow-boundary.json` record the exact release adjustment.
This release makes no new ordinary fixed-shadow delivery claim.

`native-final/source-coherence-release.json` verifies 10,834 frozen source files
against their initial manifest and nine declared refreshes. The packaged client
independently verifies that same source inventory throughout qualification.

## Activated client and catalog

The installed runtime is
`bin/engine-builds/be779b6e514943e5ac2c27f6737258b8/halo2_native`, SHA-256
`bc29d58d0cb773ea5b3e1071aba5e9b64bdc27d1a3608bf60f15ee95f16ffa05`.
Both normal launchers select it. The actual packaged client passed ten selected
native/accuracy checks, fifteen launcher/loading checks, thirteen submitted-frame
MCC map cases across one/two/four local players, and intro/title/main-menu/ONLINE
sign-in presentation. These bounded checks do not establish rendered matchmaking
and completed online play for every playlist or full Xbox parity.

The production catalog SHA-256 is
`3be11cfe6101ad61d4d949cae438d01130e9b18ee52fe386703724acede2e670`:
fourteen playlists, 559 rows, including Lab Test's 96 rows/eight modes/twelve maps.
The 547 ordinary rows carry 304-byte option buffers; twelve Action Sack rows use
the existing bundled Fiesta/Infection Slayer extension contract. Public health,
website API and static website checks pass. Existing history table hashes and
private-key identities remain unchanged. The 645,439-byte second private backup
is at `/opt/halo2/hoppers-accuracy-20261028T173500Z/hoppers/backup`.

The [native installation receipt](../../build/test-results/shot-accuracy-20261028/native-final/installation.json),
[client qualification](../../build/test-results/shot-accuracy-20261028/native-final/client-release/report.json),
[production verification](../../build/test-results/shot-accuracy-20261028/production-hoppers.json)
and [storage receipt](../../build/test-results/shot-accuracy-20261028/storage-cleanup.json)
record the release. The required installed runtime/resources retain 3,182,007,197
apparent bytes, 2,065,514,496 bytes on disk. The owned build/source/SDK tree and
all five failed candidate packages were removed, releasing 5,327,147,008 bytes
of temporary disk storage. Existing assets, references, runtimes and unrelated
active work were preserved. The superseded one-off website fixture helper was
retired; permanent accuracy/format/ledger regression tests and its small visual
receipts remain. The full ONLINE/security goal remains open.
