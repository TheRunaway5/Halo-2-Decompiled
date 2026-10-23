# EOS completion goal

Active goal requested October 22, 2026. Keep Halo playlist, party, rank and
accepted-result rules; use EOS for account-bound general stats, standings,
Sessions and peer traversal/relay. Full carnage history remains on the master.

- [x] Install the qualified client and single Play → intro → title → menu route.
- [x] Bind two distinct real Steam accounts to EOS from installed clients.
- [x] Read back both profile revisions through the trusted publisher and public
  website API: 27 fields for the legacy account, 29 for the new account.
- [x] Preserve the legacy unknown join date and publish the new registration's
  actual timestamp. Live aggregate counts show two users and zero guests.
- [ ] Finish a two-account match through the production EOS handoff.
- [ ] Verify the accepted native carnage report, guest exclusion, both players'
  history, exact career/accuracy increments and duplicate-safe EOS read-back.
- [ ] Verify ranked-result publication and playlist isolation.
- [ ] Verify an actual match through forced relay, then automatic routing.
- [ ] Verify two consumer networks and real microphone capture/playback. The
  user can arrange that hardware later; two local clients do not close this item.
- [ ] Complete native world/presentation/input/audio adoption and pre-election
  readiness; test graceful, abrupt and repeated host losses through EOS.
- [ ] Retire legacy transport after compatibility and gameplay cutover pass.
- [ ] Deliver profile-scoped remembered Steam sign-in, Steam/clan invite tabs,
  and corner party notifications using the project's controller artwork.

## Current regression

The two installed clients reach “Attempting to join a game,” remain in joining
for approximately 40 seconds and retry. Both WebSocket signaling connections
receive HTTP 101; neither account reaches playing. The exact public symptom is
captured by:

```sh
python3 -B tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/check_online_population_phase.py \
  --phase playing --users 2 --timeout 65 \
  --report analysis/eos-two-account-join-20261022/before.json
```

This command failed against the live repeated attempt. It checks aggregate
activity only; native session diagnostics and actual gameplay must establish
the cause and subsequent acceptance. No test scores or fabricated reports are
inserted into the production account database.

The minimized official-SDK regression reproduces `EOS_AlreadyConfigured` after
the profile platform releases the last SDK owner and matchmaking attempts to
initialize again. Platform release now retains the process SDK; explicit main
shutdown occurs after session owners are destroyed and before static teardown.
The real SDK profile/recreate/concurrent-platform/shutdown regression passes.
The original two-client playable check remains required after installation.

The real host OS credential-store regression also passes profile/origin
isolation and deletion. Refresh credentials remain in that secure store;
non-secret stable profile slots survive profile renames. The master issues
eight-hour sessions from fixed thirty-day refresh credentials and revokes the
refresh credential and its issued sessions on explicit Sign Out.

The scoped production master rollout for remembered sign-in and Steam/clan
invitations passed eighteen applicable HTTP regressions, image schema/routes,
health, and source-hash checks. The separate in-progress party roster appearance
extension is excluded from that rollout. Native delivery and live acceptance
are still pending. No Steam passwords or private account identifiers appear
in the evidence receipts.

## Migration boundary

Current source has native retention, role preparation and several foreground
checkpoint owners. Ordinary matchmaking still leaves `peer_host_migration`
disabled. `prepare_peer_migration` has no production caller, and
`prepare_foreground_handoff` explicitly remains a private partial preparation.
`MultiplayerRuntimeAuthority::prepare_takeover` currently requires an already
learned decision; complete preparation before readiness/election still needs its
own implementation. EOS Sessions or an election alone do not close this gap.
Preserve these guards while completing the owning transactions.

## Evidence

- `analysis/single-play-installed-20261022/report.json`
- `analysis/eos-second-account-20261022/report.json`
- `analysis/eos-second-account-20261022/profile-readback.json`
- `analysis/eos-two-account-join-20261022/before.json`
- `analysis/eos-two-account-join-20261022/social-and-sdk-lifetime.json`
- `analysis/eos-social-ui-20261022/scoped-master-focused.json`

Private test-client preferences, device caches and diagnostic logs belong to
the active `tools/run_test.py` owner and are removed when that client closes.
Retain only bounded redacted findings and source hashes afterward. Preserve the
installed runtime, original assets and unrelated active work.
