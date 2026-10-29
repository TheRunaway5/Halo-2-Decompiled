# ONLINE matchmaking flow: TU1.5 ports

Native frontend behaviour for searching → start countdown → game → post-game,
ported from the TU1.5 Xbox `c_matchmaking_screen` (mainmenu screen 210) and the
network session manager. Evidence:
[progress evidence](halo2-matchmaking-progress-evidence.md) and the decompiled
sources under `tools/reference/halo2-decompiled/src/`.

Unqualified addresses in the table are retail decompiler locators. Current
unchanged TU1.5 executable controls use SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:
roster `0x25780B`, tab selection `0x2BDF46`, and ordinary start-match timer
`0x76360` / `0x76481`. These are bounded controls, not full Live-service or
framebuffer execution.

The supplied video screenshots have no identified title-update version. They
provide visual references; behavioral authority comes from the unchanged,
authenticated TU1.5 executable controls above.

These changes and their validation are local. No backend rollout or replacement
of the launcher-selected runtime has been performed.

## Flow

| Step | TU1.5 owner | Native |
| --- | --- | --- |
| Enter matchmaking | Quickmatch `0x230F92` / Optimatch selection `0x2B2181` prepare party A and its hopper; lifecycle `0x19B1C9` opens pregame14 | Select the hopper and consent without enqueuing; START GAME runs the lobby countdown before screen210 |
| Pregame presentation | Pane2's Matchmaking / In E43A / E43B / E41F and `0x250332` bitmap2 group0 for session enum6 | Bind the actual retained hopper name/description and protocol caption; retain authored matchmaking illustrations and player-count text; no hardcoded Rumble Pit |
| Pregame Start availability | TU1.5 `0x2572C1` accepts choice0 only for the signed-in leader with status9; `0x25737A` starts the countdown or plays error2 | Eligibility and dispatch share one actual account/party/selected-consent/capability predicate. Missing shared-clock support is visible in the lobby; rejected or busy Start actions explain the failure and never create a private countdown |
| Game Setup | TU1.5 `0x2572C1` choice1 loads `0x2C5666` screen23; enum6 `0x2C505C` rows9/6/7; row9 `0x2C5AA1` loads the original playlist listing (`237` on lobby channel3) | Open the original squad-settings screen and original playlist listing. Both `219` and `237` list the authenticated catalog; selecting an eligible row returns directly to the pregame lobby. Original geometry, skin0, animations, text buffers and button keys are projected from pinned cache definitions. Unavailable ONLINE co-op/custom/privacy owners are explicit; they do not silently become local sessions |
| Searching / waiting | `0x199D7C` UI states 3/4, `0x25142F` panes 0/1 | `MenuState::online_matchmaking_ui()` maps service phases to the original UI states |
| Attempting to join | UI 5 (Xbox pane 0; PC raw state 6 shows pane 2) | PC pane 2, title `0x0c00043d` |
| Start countdown | `c_session_state_start_match::update` retail `0x72700`, TU1.5 ordinary timer `0x76481`: 10000 ms, `(remaining+999)/1000` s, mode 4 at zero | One monotonic service-session clock after readiness; `matchmaking_published_start_countdown()` converts its remaining time; native transfer requires an observed authoritative zero |
| Countdown pane | `0x25142F` pane 3 once `0x59590` has a value; `0x251B39` plays UI sound 12 on entry | `online_matchmaking_progress_pane()`; `FrontendUiSound::MatchmakingAdvance` |
| Countdown text | `0x251D84` text0 "Game starts in" + `0x22D775` `:%02d` / `:--`; variant and "on <map>" rows | PC strings `0x0f001afc`, `0x0f0001e0`, `0x0e0001e2` |
| Roster | `0x2514FD` pane 3 sort (rank/team comparators `0x251384`, `0x2513FC`, `0x2513C9`); `0x2517E2` team color while the countdown exists in a team hopper | `matchmaking_progress_order()`, `matchmaking_progress_team_colors()` |
| Ranked hidden players | TU1.5 `0x25780B`: party A before pane3 for ranked hoppers; matched datum16 / empty NONE; `0x257A11` / `0x257A86` select player_in pages1 /0 | Real party names, then localized Matched Player and blank slots; no opponent name/rank/emblem until the original gate; authored secondary slot background |
| Cancel | `0x25217B`: refused with UI sound 2 for 60000 ms from pane 2 on; otherwise dialog `0x8C` (remote party members) / `0x8D`; `0x252166` closes it from pane 2 | `MenuState::back()`, action `-16` |
| Leave Matchmaking | `0x252150`: party returns to pregame lobby 14 pane 2 | cancel queue, open lobby 14 |
| Member follows the search | `0x19B1C9` presents party A's matchmaking modes as screen 210 on every machine | a member in lobby 14 opens 210 when the observed party search starts |
| Party-wide cancel | `0x252150` sets `g_51ec99` for the canceller only; lobby `0x250804` shows session state 4 as `0xAD` and a member's departure (states 6/14/19) as `0xAE`, once | `MenuState::update_online_party_search()`: a party search (lobby 14 or screen 210) that ends without the local Leave Matchmaking returns to lobby 14 and shows `0xAD`, or `0xAE` when a searched member is gone |
| Y button | `0x251AFA` consumes param 3 from pane 2; `0x148523` closes an open Y menu on a pane change from pane 1 | `MenuState::players()`, pane-change cleanup |
| Post-game | PCR Done `0x233792` → party A pre-game → `0x19B1C9` opens lobby 14; Live matchmaking selects pane 2 (`0x24FF0A`); hopper kept | `MenuState::return_online_lobby()` for PCR Done, abandon and disconnect |
| Report tabs | PCR `0x233423` hides the team tab when no retained teams; TU1.5 `0x2BDF46` cycles visible siblings with wrap and sound9 | Native tab kernel, keyboard Left/Right or PageUp/PageDown, controller dpad/shoulders, and mouse on authored arrows |
| Prior stats | Lobby `0x2510AA` / `0x250B1E` param5 opens retained results only when populated | Detached real PCR185 owner over the same lobby; authored prior-stats prompt, Tab or left shoulder; close retains lobby, hopper and results |
| Post-game, party member | Vista PCR16 `40C01A`: host predicate false → dialog `0x98` (callback `40C00D` → `6151E2(0)`); the host's Done (`5C884A(session,1)`) moves every machine to pregame | `MenuState::online_postgame_host()` is false for a member who is not the party leader; `0x98` Leave Party (action `-18`) closes the report and leaves the party; `update_online_postgame()` continues the report once the party view shows this member in the report room and the leader not (`online_party_leader_left_postgame()`) |
| Search again | Lobby START GAME `0x25106C` → `0x19A78E(controller,10,3)`; `0x6E360` steps once per 1000 ms; `0x250A8B` UI sound 10 on stages 0..3; `0x22D730` `%d:%02d`; at zero `0x6E410` enters mode 6 (screen 210) | "Start Game" entry for the party leader of a lobby that returned from matchmaking; X cancels (status `0x200001F0`) |

The kernels are in `include/halo2/frontend_matchmaking_progress.hpp`.

The service's actual setup clock/readiness and loading/running decision now live
in Rust (`matchmaking_start.rs`, `matchmaking_room_setup.rs`). Authenticated early
loader reports remain pending until the source clock reaches zero. The new
[room setup receipt](evidence/matchmaking-rust-parity-20261028/room-setup-validation.json)
includes a full ten-second HTTP wait and unchanged-original numerical replay.
The C++ helpers above remain presentation/legacy component evidence; full signed
outlier transport/presentation and native played/delivered acceptance are separate.

Legacy v11 relay continuity now also lives in Rust. Setup installs its authority
owner from the immutable admitted physical roster; Python forwards world proofs,
handoff/rebind messages and clock observations. The same Rust owner supplies the
result finalization epoch, so a Python-supplied generation cannot become stored
provenance. This is declared native coordination over peer-reported complete-world
digests. Actual engine continuity and original host election ordering remain
separate gates; see the [continuity receipt](evidence/matchmaking-rust-parity-20261028/continuity-validation.json).

## Service and presentation boundaries

Rust now also owns the room descriptor, bearer/capability revisions and socket
connection generations, upgrade eligibility and bounded peer signaling. Room
proofs revalidate the same capability after body/provider work. EOS room identity
guards execute before provider binding and native proof publication. Pending
signaling drains in bounded native batches, and retired sources cannot publish
queued messages. These are declared native transport contracts, separate from
original Live protocol recovery and actual engine continuity. See the
[room wire receipt](evidence/matchmaking-rust-parity-20261028/room-wire-validation.json).

Room phases, lifecycle clocks, PCR publisher/confirmation eligibility, review
completion, departure decisions, voice-host/member publications and frozen-ticket
cleanup now belong to Rust. Python forwards authenticated socket observations
and executes IO/publication effects. A SQL failure leaves native review pending;
postgame requires either committed full-electorate results or an explicitly
unrecorded surviving-owner presentation. Transport and provider code, signaling,
packet/voice policy and actual engine continuity remain separate migration and
acceptance work. See the [lifecycle receipt](evidence/matchmaking-rust-parity-20261028/room-lifecycle-validation.json).

- Native friend-tag entry uses the original keyboard's unrestricted
  `0x237998` branch for invite/search purposes. Its Extra Symbols page supplies
  the authored `#` key (`0x516`); profile and variant keyboards keep their
  restricted branch. Friend tags are replacement-service identity, not an
  assertion about original Xbox Live account lookup.
- Account, party, discovery and queue clients belong to `FrontendSession` and
  cross renderer/world transitions through the same `NativeTu15OnlineServices`
  owner. Rebinding the same client is idempotent. Renderer teardown releases
  its SDL voice/display adapters, preserving authentication and party intent.
  Done/abandon/disconnect explicitly cancel the transferred queue intent before
  opening the next lobby, so an old Loading snapshot cannot block a new Start.
- Session lease renewal uses optional private `renew_session` on the existing
  `POST /v1/matchmaking/session`. The external account grant and every signed-in
  couch player's grant are reverified. Only an owned, unexpired session with
  unchanged full configuration retains its token, guest identities, ticket,
  party consent, QoS and relay. Native renewal starts at 75% of the lease
  (675 seconds of the default 900), without deleting that session. Expired or
  foreign sessions and invalid identities are rejected; changed configuration
  retains the replacement/invalidation path. This is replacement-service
  lifetime management, not recovered original Xbox Live authentication.
- Party post-game presence: `GET /v1/party?postgame=1` adds each member's
  `postgame` flag, true while that member holds an open connection to a
  reviewing or post-game match room. Without the query the view is unchanged.
- A search a member cancels leaves the other members' consent in place, so a
  party leader's queue client would enqueue its playlist again; the notice
  path cancels the local queue intent first. The party view and the queue are
  polled separately, so a departure seen only after the queue drop shows
  `0xAD` instead of `0xAE`.
- `GET /v1/matchmaking/ticket?start=1` adds nullable `start_countdown_ms` for
  the authenticated member's loading room, including after its core ticket
  retires. It is separate from gathering growth wait. Delayed observers read
  the same session clock; advancing a UI clock cannot release native launch.
  An older service produces an explicit update-required error. These backend
  changes must be rolled out before the new native client can launch online.
  Legacy callers that do not request this property keep their existing schema.
- The qualified MCC frontend now projects the pinned Xbox screen210's four
  authored panes, alongside original16/185 report geometry and skins15..20.
  It no longer compresses the original final pane into the PC three-pane
  panel. The right roster includes its original nested name/emblem/rank fields.
  The final pane has all six source quick-option rows, actual matched-map
  preview, game icon, source-gated clan-versus text and player total. Complete raw selected
  MCC definitions and sprite mappings qualify the projection; unknown
  definitions are rejected. Source geometry and states use admitted MCC
  higher-definition artwork through finite logical aliases. No DVD bitmap,
  font, emblem or sound payload is linked into the production UI. The selected
  custom celestial rank pages and controller artwork survive both canonical
  and logical-alias routes; unrelated MCC resources remain unchanged.
  Twenty-three map previews join actual scenario paths to their admitted MCC
  bitmap owners. The map-preview marker follows the original widget identity
  before aliasing, so the matched pane replaces its authored static Zanzibar
  placeholder with the actual scenario's MCC image. Export/profile:
  [exporter](../../tools/export_matchmaking_ui_tu15.py),
  [original definitions](../../resources/frontend/tu15-matchmaking-layout.json).
- Goal fonts use admitted MCC atlases and the existing authenticated original
  logical metrics. Goal emblems retain all 32 MCC background and 64 foreground
  pages and the selected MCC shader; exact named animation inputs bind the
  original appearance semantics. DVD pixel/audio providers remain offline
  comparison controls only. The one missing original quick-options image uses
  its explicit same-widget role join: source210/pane1/bitmap0, logical512x256,
  to MCC210/pane1/bitmap0's actual1024x512 frame1. The profile records both
  complete widget owners; an arbitrary ordinal is not substituted.
- Original multiplayer map-copy wait is reached through TU1.5
  `0x728B0 → 0x1916A0 → 0x191890 → 0x191EF0 → 0x166770 → 0x12E200`.
  Its actual frame callback `0x2C9E0` clears black, and `0x229DB0` draws the
  localized loading percentage with font3. The native adapter uses admitted
  MCC font3 and real renderer/resource completion, retaining the departing
  GPU device through map preparation and joining before its first ordinary
  draw. The reverse handoff joins after mainmenu resources exist and before
  the real PCR factory opens. A fixed dwell or estimated percentage is not
  introduced. Actual full-flow/GPU acceptance of this new adapter is pending.
  The separate ring compositor `0x289C0` is directly activated by campaign209;
  no multiplayer caller for that activation has been established. All five
  permitted MCC loading banks and all eleven resident pages/mips have exact
  offline source controls, but their availability alone does not activate the
  campaign compositor or its audio policy during matchmaking.
- ONLINE start and departure now present the original screen231
  `network_transition_progress` dialog (`PLEASE WAIT`) over black in place of
  that percentage wait. It reads `Loading map...` during world preparation,
  `Setting up game...` from renderer admission to the first draw, and
  `Leaving game...` on the way back to the menu. The percentage wait remains
  only for caches without screen231. TU1.5 opener/update/status addresses and
  the port's status mapping are in
  [loading presentation](../../docs/MCC_LOADING_PRESENTATION.md#original-loading-screens-october-26-2026).
  TU1.5 also opens this dialog for system link; the native system-link start
  does not use it yet.
- The collapsed Y FRIENDS caption is the separate original global-channel
  screen27, maintained by TU1.5 `0x23C254` and rendered by `0x23C223`, rather
  than part of background5 or a screen button key. Its actual bitmap, text,
  font2 and geometry render after the goal's base channel only with an
  available authenticated native or explicitly enabled/ready SDK friends
  provider. Expanded social navigation owns its own channel. The original
  session UI6/7 suppression uses the documented replacement-service phase
  adapter; it does not claim to reproduce Live's raw session-key producer.
- Source typewriter captions retain the original50ms interval, glyph219 and
  0.3 scale. All59 global animation tracks match the selected MCC bank exactly.
  Original rotating tips come from downloaded `message_of_the_day.dat` through
  `8CA70/8CC20`; that data and its primary UI random-state owner are unavailable.
  The actual original missing-data fallback retains the authored tip. No
  substitute random selection or invented tips are published.

## Scope and remaining gaps

Widget decisions use authenticated TU1.5 `0x257F2D` (title), `0x256D51`
(game icon), `0x256C3A` (option category), and `0x2580B2` (matched pane).
The matched pane formats six actual category entries, with its title state
retained at `+0x610` until the pane changes. Pane2 retains its authored
"Configuring game..." text. All59 Xbox/MCC shared UI animation tracks compare
equal, as do typewriter interval50ms, cursor219 and scale0.3. Progress-number
pages, done/dot artwork and their delays come from the original pane records.
Their FPS and scroll inputs are zero; no invented progress animation is added.
Quick-option values retain the original first-exact-choice lookup. The actual
authored round-time list contains minute values and Unlimited; the finite
eight-second gameplay-validation duration has no authored label. Twenty-one
unchanged TU1.5 `0x23D499` controls verify that its value stays blank while the
Round Time Limit caption remains visible. A guessed seconds label is not added.

The rotating-tip producer is TU1.5 `0x6F150`/`0x6F310`, reading downloaded
`message_of_the_day.dat`. Its exact bounded primary-RNG index and the strict
15-second clock are ported and checked against unchanged instructions. The
downloaded list and an admitted frontend primary-RNG owner are unavailable;
the original failed-read path preserves the authored default tip. Secondary
menu-effect RNG is not substituted. This remains separate from the complete
widget/tag/session callees and any physical Xbox framebuffer comparison.

- The ONLINE lobby uses the authored pane2 title/buttons/illustrations and
  retained roster/text bindings, with actual service party/playlist details.
  Exact Xbox framebuffer parity remains unverified. Party privacy has no
  service owner and is left unavailable rather than claiming OPEN.
- Each member still consents to the leader's playlist (service policy). The
  [party-A countdown](native-party-countdown-tu15.md) now runs the original
  press/tick leaf through the native Rust ABI. Every machine observes one
  authenticated clock and parameter publication, including shortening,
  cancel/restart, delayed observers and the original member mode1 request.
  Confirmed B departure leaves the actual party before closing the lobby.
- TU1.5 readiness (`0x720D0`, connected-team helpers `0x721B0`/`0x62DE0`)
  and its deferral decision are ported, including policy bit31. The original
  Live policy parser (`0x25DCB6`, via `0x669A0`) supplies that bit; replacement
  Discord/EOS authorization is not treated as an original Live policy grant.
  The replacement service gates its shared start clock on admitted team
  diversity and the actual measured physical-owner connection matrix, clearing
  the clock on loss before accepting start acknowledgements.
- TU1.5 error selector `0x256AF7` has 55 unchanged-byte controls: status1 selects
  dialogAB, status5 AC, remote status4 AD, and statuses6/14/19 AE. Native cache
  rejection now posts an authenticated failure and preserves AC across room
  deletion. Terminal failures latch until an explicit fresh action. These are
  adapters to the replacement service, not recovered Xbox Live transport.
- Four-team casual policies now explicitly configure `team_count=4`. Native
  [team balancing](../../rust/online_protocol/src/team_balance.rs) executes the
  original greedy assignment, swap optimizer and count/capacity gates. The
  [gathering owner](../../tools/reference/halo2-decompiled/src/unknown_071730.cpp)
  first preserves parties, then retries casual assignment with party splitting.
  Service weights come from authenticated unadjusted raw hopper ratings, as
  the original `0x7E210` player list supplies at `+0xD4`. A bounded oracle calls
  unchanged TU1.5 `0x9E1A0`, `0x9E010` and `0x9E0E0` in full: 256 cases,
  768 original calls and 1,111 distinct original instructions match the native
  port, with no replaced callees. Native twelve-seat rendered flow validation
  is separately required; an assignment test alone does not establish it.
- Exact final-era downloaded hopper/network configurations remain unrecovered.
  Published playlist names and dimensions do not supply numeric rotation
  weights or complete downloaded variant buffers; see the
  [historical configuration evidence](online-historical-config-followup-20261013.md).
- When opponents actually depart, the original participant/end owners retain
  their rows and allocated team records and determine the surviving-team
  outcome. Surviving owners may finish local PCR presentation after confirming
  the same source report. Persistent history/rank writes still require every
  originally admitted owner's consent; `result_recorded=false` distinguishes
  presentation-only completion. Host retention remains separately gated by
  native-handoff/peer-migration qualification and does not enable migration in
  the default matchmaking flow.
- Screen210 opts into exact nine-byte appearance on admitted match/gathering
  rows. Verified primary/signed-local profiles use their actual session profile
  or stored customization; guest profiles remain session-only. Cosmetic renewal
  preserves guest identities, ticket and party consent. Party A and game B use
  their original row visibility gates; matched/empty slots disclose no profile.
  Speaker pages follow source `0x251977` (talking0, idle1, unavailable2) only
  where actual joined-party voice or retained match audio/preferences provide
  an identity-bound observation. An unobserved remote headset and an independent
  guest microphone are unavailable; the UI hides their icons instead of
  inventing capability or talking state.
- The report has no original Live player-card service. Its native details
  predicate remains unavailable, so the source footer hides that action.
  The original report projection uses the recovered native waypoint-emblem
  pass with the actual consumed appearance. Its selected-cache/source metadata
  qualification and rendered checks remain separate from Xbox framebuffer
  equality and the unavailable Live player-card service.
- The report's original Xbox row projection retains the source semantic fields
  through the native carrier adapter. Seven complete retail/TU1.5 producer/row
  bodies have identical normalized instructions and internal branches. The
  unchanged TU1.5 percentage corridor and complete UTF16 append match 52
  arithmetic and seven eight-medal buffer controls. Its actual `0x437544`
  medal delimiter is one space and its buffer holds 256 UTF16 units including
  NUL; the readable retail decompilation's comma annotation is incorrect.
  The Vista projection retains its separate 512-unit capacity. These controls
  do not execute the external widget, locale, identity or Live services.
- Matched-pane text11 is a clan-versus field. TU1.5 `0x2580B2` calls `0x257FBF`
  only for original hopper-description type5, joining the first active member's
  actual account/clan string per represented team in original session order.
  Replacement match rows do not publish those source clan-name buffers, so
  the field stays empty; team color names are not substituted. Original stock
  map previews use their exact scenario-to-bitmap descriptors. Downloaded-map
  version3 `.lvl` records have a separate recovered owner: TU1.5 `0x1A5590`
  copies 207 rows of 220 DWORDs from record `+0xC70` into a real preview
  resource, whose original mainmenu descriptor has format11, one frame and no
  sequences. Twelve supplied DLC scenario roles have pinned catalog/CRC/preview
  hashes and 26 unchanged original row-copy controls, including padded output
  pitch. Those descriptors do not infer a resource-slot ordinal or support an
  unidentified custom-map image.

## Current original-flow qualification, 2026-10-25

The [current acceptance matrix](../../build/test-results/og-matchmaking-complete-20261025/acceptance-matrix.json)
separates source controls, rendered acceptance and launcher/service delivery.
The replacement service passed 375 backend tests with the original team,
countdown and readiness leaves enabled. The full original four-pane/roster,
seven-family end/PCR, terminal-loss and actual Start/Game Setup qualification
is still in progress; earlier PC-layout rendered tests below do not establish
that acceptance. The live service was observed returning 404 for the shared
countdown endpoint, so it cannot support this client's Start contract until a
compatible backend is qualified and delivered.

## Earlier PC-layout validation, 2026-10-25

[Validation receipt](../../build/test-results/online-flow-20261025/validation.json)
records 19 passing focused native checks across the retained source cohorts
and 349 passing backend tests. Earlier failed iterations remain in their logs;
the affected frontend checks were fixed and rerun successfully. The shared
checkout had concurrent edits, so each cohort retains its tested source hashes.

Native coverage includes initial pregame/countdown, ranked hidden roster and
rank pixels, real complete-match/PCR/prior-stats/requeue, four-account party
consent, eight couch seats, and a real Multi Bomb report/return. The rebuilt
native executable passed compilation. External account verification uses local
fixtures; no physical-controller or original Xbox framebuffer claim follows.

The native Vulkan run drew all 12 PC16/185 report panes. Rank and party-roster
GPU checks also passed. Validation layers were explicitly disabled because
`VK_LAYER_KHRONOS_validation` is unavailable. Unchanged TU1.5 instruction
controls cover 504 roster cases, 24 tab cases, 16 normal-start controls and 30
readiness controls, within the bounded scopes documented above.

[Cleanup](../../build/test-results/online-flow-20261025/cleanup.json) confirms
that the owned build, frozen source copies, binaries and captures were removed.
Only small reports, bounded logs, commands, source hashes and source fixtures
remain. No large generated artifact was retained.
