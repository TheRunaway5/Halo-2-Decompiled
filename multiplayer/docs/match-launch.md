# Native ONLINE match launch

Quickmatch, Optimatch and the party lobby's Playlists route now hand an
authenticated solo player or accepted party into the native Slayer/Team Slayer runtime.
Manual server discovery remains independent. The deployed lab catalog offers
ranked and casual two-player Lockout Slayer and four-player Lockout Team Slayer
with parties of two; it is not a restored original playlist rotation. Its explicitly
versioned [rank progression](ranked-progression.md) uses recovered retail launch
tables, independently of the custom search tuning in this validation catalog.

## Admission and ownership

1. The existing Discord authorization creates a private queue session. Screen
   names remain player chosen and may be identical; authenticated room slots
   identify participants independently of names.
2. The coordinator reserves its selected map, authored variant and roster. The
   native client obtains a private room bearer through
   `GET /v1/matchmaking/transport`, then upgrades
   `GET /v1/matches/{room}/relay` with that bearer in an Authorization header.
3. The service measures three challenge echoes from each participant. Its native
   relay quality score is `max(0,100-floor((median RTT A+median RTT B)/5))`.
   This is an explicit replacement score, not recovered Xbox QoS. The existing
   coordinator checks every link and selects the host; clients submit neither
   scores nor host choices.
4. All participants must connect before the service consumes the proposal and
   starts loading. The native frontend resolves the catalog variant against the
   actual mainmenu goof row, removing only its authored U+E008 display suffix and
   preserving the source rule writes. Unsupported native rules fail admission.
5. The shared room connection survives frontend/queue teardown. Native players
   load the selected cache, original executable/update, runtime scene and Vista
   HUD assets. All must report the same full-data fingerprint and native protocol
   version28. A mismatch or missing player closes the room for everyone.
6. The selected host admits the exact authenticated slots/names/teams. Clients
   retain their chosen source appearance. Gameplay begins only after every real
   participant is present; there are no generated peers or placeholder players.
7. The host runs the existing source-backed Slayer simulation; clients receive
   complete native snapshots and send controls. Leaving/disconnecting closes the
   room while a game is active. Completed rooms release players independently.
   This integration has no host migration or backfill. Final reports require
   unanimous native snapshot confirmation; this is not independent server
   simulation or cheat arbitration. Configured ranked and casual playlists
   update private XP/skill in the same transaction as the unanimous result
   ledger; casual levels stay hidden.

The game-data fingerprint covers map/shared caches, source/update executables,
BSP/starting weapon and original font files. Admission also hashes all304 bytes
of the actual selected source variant, including score/time/life limits. It detects differing installations;
it is not a client integrity or anti-cheat proof. All participants require the
same native build/protocol and gameplay data. Vista sampling assets affect
presentation while retaining original metric/layout owners.

## Relay contract

The master shares HTTPS/WSS443 with Caddy. It requires no new public UDP port.
It relays the native protocol, not the retired Xbox wire format. The chosen host
remains authoritative for simulation and voice audiences. TLS authenticates the
master and encrypts each hop; the relay operator terminates TLS.

Binary frames contain a route byte followed by at most1400 bytes of native data.
The gateway overwrites sender slots from the authenticated connection. Clients
can send hello/input/leave/voice only to their host; hosts can send
snapshot/reject/voice only to admitted peers. The native reader separately
validates protocol, message shape and voice gains. The final PCR publication is
a single bounded JSON control (112,880 hexadecimal characters for the 56,440-byte
consumed bank); other controls remain small. Room credentials stay in memory
and never enter UI snapshots or logs. Native queue compatibility is now
`halo2-native-queue-v5`; older native queue builds are explicitly refused.

The service bounds rooms to64, members to16, queues to512 frames per connection,
and native voice queues to80 pending frames. Reader limits are1000 packets/second
with a512-packet burst; writes time out after0.5 seconds. Connecting has a25-second
bound, loading180 seconds, and sessions2 hours. Challenges continue every5 seconds
in loading/gameplay and unanswered challenges close the room. Postgame delivery
requires all confirmations within30 seconds, then retains the completed voice
session for at most15 minutes. Disconnect, stale
proposal, invalid route, loading mismatch or stalled reader closes all members.
Slow TCP recovery can stall game and audio traffic; direct UDP, traversal and an
impaired-WAN latency budget remain unfinished.

## Completed matches and return

The host waits for the actual source game-over countdown and frozen recording,
then publishes the complete PCR-consumed numeric/identity/configuration bank.
The relay accepts it only from the selected host, validates finalized admitted Slayer,
all authenticated room identities, chosen names, bounded placements, actual admitted team memberships and absence
of extra players/teams, and sends the same publication to every participant.
Each native client drains its final snapshots and checks the exact source variant,
roster, chosen names, scores, original placement, kills, deaths, assists and
suicides before confirming the SHA-256 of the entire report. Disagreement,
missing confirmation or disconnect aborts without a stored competitive result.
Clients do not reconstruct medals, weapons or opponent statistics from summary
snapshots.

Only after every admitted player confirms the same digest does the relay commit
one immutable result and all participant statistics in one SQLite transaction,
then send the completed acknowledgement. Duplicate confirmations cannot repeat
the write. The private ledger retains playlist/revision/map/variant, completion
time, digest and numeric participant results; it does not retain Discord tokens,
the complete report, chosen names or persistent public account identifiers.
Validation is explicitly `native-unanimous-v1`: agreement with host snapshots
does not prove an honest host or prevent collusion. The optional, explicitly
versioned retail launch policy freezes server-owned skill at admission and writes
XP/rank updates in this transaction for both ranked and hidden casual playlists.
Later downloaded policies remain unresolved; see
[rank progression](ranked-progression.md).

The native application waits for the committed relay acknowledgement, closes the gameplay world, retains the opted-in session audio,
loads the retained mainmenu resources and creates the original Vista PCR16.
Its six actual panes, rows, locale, appearance, original tied placement and fade
remain source-owned. Random per-room eight-byte keys identify native participants;
they are neither Discord IDs nor retail XUIDs and never appear as display text.
[Original postgame evidence](online-postgame-evidence.md) records the exact FFA
ranking and result-publication owners.

Original Done/Back input returns solo players to ONLINE navigation and accepted
party members to their native party. Every party member chooses the next playlist
before the leader starts the group search again. These continuation permissions
are explicit native integration; original party-host continuation is not claimed. Discord authorization remains in memory across renderer
and world teardown. Chosen profiles remain selected, and Quickmatch/Optimatch/
Playlists can start another search. Escape or an active-match disconnect returns
to ONLINE without manufacturing a completed report. A window close still quits.
Completed peers can leave independently while the other report is delivered.

Team publication uses original player composite ranks, retained/playing team masks,
all nine actual engine team stat fields and signed accumulated team totals. The
ledger stores team standings and membership separately from individual scores.
The actual Vista team-name unic provides all names; no replacement labels are
painted over the original report. See [team/party evidence](online-party-integration-evidence.md).

One opted-in microphone/codec owner survives world teardown. Postgame voice uses
the original passive session audience, without world proximity or team filtering.
Its background transport continues during menu loading; mutual mute, enabled
preferences, sequence/rate bounds and actual Vista talking icons remain active.
Done stops capture and releases the departed channel. Persistent premade-party
voice, complete historical skill policies and retail metadata still require further work.

## Voice and presentation

[Voice controls and source audience rules](voice.md) cover opt-in, proximity,
team/FFA radio, the dead-player grace, both mute directions and the original
selected remote speaker HUD. Opus, DTX and the stereo device adapter are native
replacements for opaque platform services. The source evidence is recorded in
[the executable and asset audit](online-match-launch-evidence.md).

Clan screen26 uses the actual authored640x480 canvas, eight41-pixel roster rows,
footer and heading fields. The foreground scales uniformly inside widescreen
windows; it is not stretched to their width. Imported Vista fonts and bitmaps
supply sampling where authored. Vista itself retains the original clan geometry
and several legacy-resolution assets; copying shared HD skin dimensions into
that geometry would inflate the rows and overlap the footer. The member count
uses its own50-pixel heading field and fits100 members. Native status text no
longer creates an extra footer over the final roster row. The gameplay scoreboard
also samples the selected Vista font while retaining source metrics.

## Validation on 2026-10-11

- All53 backend tests passed locally and inside the staged Vultr Docker image.
  They cover authenticated rooms, challenged connection measurements, all-member
  readiness, game-data mismatch, full1401-byte relay frames and forged routes.
- The team result/session voice revision passes56 backend tests locally, including unanimous
  completion, disagreement, review disconnect, idempotence and on-disk persistence.
  Both native processes pass final snapshot validation, altered-score refusal
  and persisted-result checks. `online_match_variant_mismatch` proves differing
  source Slayer settings are refused before simulation starts.
- `frontend_matchmaking_http` passed actual native Quickmatch/Optimatch/Playlists,
  chosen names, private Discord identity, real connection admission and gameplay
  handoff, cancellation/requeue/sign-out, and Vista texture/font byte comparisons.
- `online_match_launch` passed two native processes with identical chosen names,
  actual Lockout caches, measured relay paths, authenticated slot admission,
  replicated movement/fire through210 simulation ticks and real Opus audio.
  Capture stayed closed before opt-in and stopped for microphone mute, focus loss
  and disable. Remote mute/unmute and playback recovery passed.
- The rendered two-process mode finishes an explicitly shortened test Slayer
  match through the source rules/countdown, shows both real players in Vista
  PCR16, executes Done, requeues without another Discord login, launches a second
  Lockout match and checks clean Escape/disconnect outcomes. The shortened
  test variant does not change the deployed catalog or its authored rules.
- Native multiplayer, LAN startup/replication, HUD layout, voice policy/codec and
  the existing33-second clan voice transport check passed.
- Clan create/invite/join/roles/Overlord transfer confirmation/message/leave and
  privacy checks passed. Captures include a full100-member roster and its final
  page at960x720,1280x720,1280x800 and2560x1080;15 ONLINE panel/context pairs passed
  Vista bitmap/font byte comparison. The full roster and footer were inspected.

Only external Discord endpoints are fixtures in these repeatable two-process
checks; the backend, transport, caches, simulation, Vulkan runtime and Opus codec
are real. These checks do not demonstrate two physical Internet players, physical
microphone/speaker quality, retail XHV/network compatibility, all original game
modes, original playlist rotation, historical skill progression or WAN packet-loss
latency. The source reconstruction boundaries remain in the linked evidence notes.

Run focused checks from the repository root:

```sh
python3 -m unittest discover -s multiplayer/master_server/tests -t . -v
ctest --test-dir build-online-20261011 -R 'online_match_launch|frontend_matchmaking_http|game_voice_policy' --output-on-failure
python3 tests/online_match_launch_http_test.py build-online-20261011/online_match_launch_tests render
python3 tests/frontend_clans_http_test.py build-online-20261011/frontend_online_tests --capture .local/clans/captures-online-20261011
```
