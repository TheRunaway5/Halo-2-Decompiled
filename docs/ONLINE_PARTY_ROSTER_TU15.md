# ONLINE party roster and UI cues

The accepted-party lobby now binds each available saved appearance to the
existing player-skin compositor. It retains the authored emblem shader and
separate material bank for each row, primary-color stripe, source name-color
conversion, and the original speaker artwork. It does not assign colors by
name, row index, party size, or talking state.

## Original TU1.5 leaves

The genuine `u5English/default.xbe` SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`tools/oracle_party_lobby_tu15.py` runs its instructions unchanged with explicit
widget, voice, unavailable-rank and final audio/render submission providers:

| Owner | TU1.5 address | Behavior |
| --- | --- | --- |
| Empty matchmaking slot | `257A86` | Secondary skin background page 0; item flags `20`, `value5e=true`; speaker hidden |
| Speaker state | `257C97` | Talking page 0, idle enabled page 1, unavailable/disabled page 2 |
| Player quantity | `25676C..256785` | Normalize quantity 4 to 1 before selecting stripe page |
| Player-skin consumer | `2B9BBE` | Corresponds to older retail `2AFEAE`; appearance, name, background and emblem remain separate bindings |
| UI sound request | `23CC7E` | Thirteen source slots, slot 11 NONE, request scale exactly 1.0; out-of-range slot omitted |

The party adapter displays the source empty-slot background for vacant rows in
its sixteen-slot lobby. This is the blank outlined square and bar seen in the
legacy matchmaking screenshot. It does not put that placeholder around a
talking speaker. The speaker bitmap's authored disabled/muted page is retained.
The square and bar are the matchmaking skin's separate `player_in` and
`item_background` assets. The normal lobby skin's profile-picker stripe cannot
substitute for that pair.
MCC retains both bitmap tags but removes the Xbox matchmaking skin group.
The native loader admits those real tags, and the qualified logical view binds
their stock32px pages to the two original widget definitions. It does not
manufacture a missing skin tag or supply replacement pixels.
The ordinary older-retail local-lobby fixture keeps its existing visibility
contract. A native party does not supply a recovered Xbox subparty quantity or
rank, so occupied rows use the ordinary stripe page and omit rank icons.

## Profile delivery

`GET /v1/party?local_players=1&appearance=1` adds nullable, exact nine-byte
appearance hex to accepted members and authenticated remote couch seats.
Unopted clients retain the earlier schema. The authenticated member identity
selects the profile; duplicate chosen names do not conflate it. Local seats use
their actual saved local profiles. Missing remote/guest profiles omit unknown
emblem/color artwork rather than borrowing another player's appearance.

## Native notification policy

The replacement service owns these membership deltas. These trigger choices
are native UI policy, not recovered retired Xbox Live notification timing:

| Event | Authored UI slot |
| --- | --- |
| Enter party / accepted member joins | 3, Advance |
| Leave party / accepted member departs | 4, Back |
| New party invitation | 8, Receive Message |
| Failed party command | 2, Error |

The first authenticated snapshot is a baseline. Stable polls, renames,
appearance changes, leadership transfers, readiness and voice activity do not
replay membership sounds. A bounded one-shot queue reaches the frontend's
existing sound playback callback and honors its current volume/mute settings.

## Validation

The original leaf report is retained at
`analysis/frontend/party-lobby-tu15-20261022/original-leaves.json`.
Focused HTTP tests cover accepted-member profile delivery and legacy-schema
compatibility. Native party tests check two identical display names with
different real profile bytes, independent emblem materials, occupied stripes,
empty-slot backgrounds, speaker occupancy and notification deduplication.
With `--party-render 1` and DISPLAY available they also render the party through Vulkan, compare
each emblem against a suppressed-emblem framebuffer, and remove the captures
with the enclosing test scratch. All ten unique authored UI sounds are decoded
and checked for nonzero native offline-mixer output.

The optional CMake `HALO2_MCC_PARTY_UI_TEST_CACHE` selects the prepared MCC cache
for `mcc_frontend_parties_http`. The default legacy fixture currently stops at
an incomplete imported `xbox_live_menu` bank; its asset admission is unchanged.
For this validation, an owned hardlink view of the MCC assets uses the official
`mcc_ui_layout.repair` importer to prepare/verify its layout sidecar privately.
Source-cache provenance and any existing layout are hashed before and after
the run; these tests never repair the installed cache.
Physical speakers/headsets, WAN play, original network notification timing and
full-frame Xbox parity are separate acceptance boundaries. Backend publication
and launcher runtime selection are separate from these source/test changes.

Concurrent Steam bridge integration initially blocked this cohort. Its CXX
parameter labels now use `credential_namespace` instead of the C++ keyword
`namespace`, and the identity target receives its generated include directory
after `OnlineProtocolRust.cmake` defines that directory. Parameter types and
credential behavior are unchanged by these build fixes.

The completed party flow also covers returning to Account and editing the
chosen name. An authenticated `return_online()` now clears the pending-sign-in
condition; retaining it made the next Account action enter Quickmatch instead.

Final checks passed: `mcc_frontend_parties_http`,
`retail_frontend_lobby_roster` (144 original cases plus TU1.5 quantity control),
and `original_frontend_sound_slots` (all13 slots/10 unique sound payloads).
The focused backend cohort passed17 tests, and the account/activity cohort
passed2. Native one-/two-player emblem framebuffer controls passed; the
two-player image was visually inspected. The retained native log reports no
VUID/validation errors. `native_vista_flow_tests` also compiled the actual
frontend loop containing the notification drain.

Small reports, source hashes and commands are under
`analysis/frontend/party-lobby-tu15-20261022/` and
`build/test-results/party-lobby-tu15-final/`. Disposable builds, private asset
views and framebuffer captures were removed. No large artifacts were retained.
