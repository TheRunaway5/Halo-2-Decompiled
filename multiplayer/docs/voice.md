# Native match, party and clan voice

ONLINE → Party Options → Join Party Voice uses accepted party membership.
Hold V to talk; options include microphone mute and selected-member mute.
The party relay uses per-party handles and does not admit outstanding invitees.
It closes on removal, account revocation or active gameplay admission. One
menu voice owner selects either party or clan, joining its old audio worker
before switching. A party voice opt-in carries into match voice; after PCR Done
joins the match audio worker, the accepted-party channel resumes in the menu.
See [party integration](parties.md) for membership, consent and current limits.

ONLINE → Y → Clan → X → Join Clan Voice enables a live clan voice channel.
Hold V to transmit; release stops and flushes microphone capture. Typing in
the on-screen keyboard or losing window focus stops transmission. Clan Options
has Mute/Unmute Microphone and Leave Clan Voice. A member's options have local
Mute/Unmute Voice, including members whose roles the listener cannot change.
Roster roles show talking/muted status using the imported Vista font and art.

This is an opt-in **native clan channel**, not an emulation of the retired
Xbox service. The microphone stays closed before joining and paused until the
talk key is held. No audio is recorded or stored. The client uses the system
default input/output; unavailable devices produce a visible error. Leaving the
clan, account revocation, expiry, disconnect or gameplay handoff closes voice.
After an account renewal or network failure, select Join Clan Voice again.

## Transport and limits

`GET /v1/clans/voice` upgrades to WebSocket using the independent ONLINE account
bearer in an Authorization header. Discord credentials never enter voice
frames. The server resolves the speaker and clan from the verified session;
clients cannot choose a clan/channel or assert a speaker ID. Both sender and
receiver sessions/membership are checked before delivery. Logout/removal/expiry
closes an idle connection within half a second; queued frames are checked again.
Only one voice connection per account is retained.
Party voice uses `GET /v1/party/voice` with the same account authorization and
queue/framing limits. Its server-owned audience is the accepted party rather
than clan membership. Its transmitted speaker UUID is a per-party handle.

WSS shares the pinned HTTPS master origin, verifies TLS and disallows redirects
and proxy environment overrides. HTTP/WS is allowed only for explicitly enabled
literal loopback fixtures. TLS protects each hop to the master; the operator
terminates TLS and this is not end-to-end encryption. Caddy's existing 443 route
supports the upgrade. No additional public port or Discord bot token is needed.

The native wire format is version1, a big-endian u32 sequence, and a mono
48kHz/20ms Opus packet. The relay adds the trusted 16-byte opaque player UUID;
the UI resolves its chosen name and never renders the UUID. Native encoding
uses 24kbps constrained VBR, complexity5, and packets bounded to256 bytes.
Decoders reject packets that are not exactly960 samples. Frames are rate limited
to50/second with a10-frame burst, maximum16 players per clan voice channel and
256 total connections. Each listener has a five-frame dropping queue and a
250ms send deadline; a slow listener cannot stall other players. Native speaker
queues are also bounded to five frames. SDL playback queues are bounded and
mixed samples are clamped. Reliable TCP transport can stall during packet loss;
this initial service does not claim UDP-grade WAN latency.

## In-game match voice

Match voice is independent of clan voice and follows the source-backed audience
and radio rules in [the original executable research](online-match-launch-evidence.md).
Enable **Match Voice** in Playlist Details, or press **F6** after a match starts.
The default is off; capture devices remain closed until enabled. **F7** toggles
microphone mute. Losing game focus or opening the debug panel pauses capture.

With voice enabled, speech is proximity voice. Tap **N** to arm the radio; speech
or continued press refreshes it, silence ends it after0.75 seconds, and a radio
burst ends at10 seconds. This is an explicit native keyboard binding. Team radio
reaches eligible teammates across the map; free-for-all radio reaches the FFA
radio audience. Nearby opponents can still hear spatial speech. Dead-player
routing uses replicated previous-unit/death-time fields and the original inclusive
one-second active grace. Either direction of player mute prevents delivery.

Hold **Tab**, use **Up/Down** to select a matched player, and press **M** to toggle
that player's voice. Selection/toggle feedback currently appears in the desktop
window caption and console; it does not replace a source HUD row. The original
HUD speaker bitmap and name show only the selected audible remote speaker after
four consecutive talking updates. Local radio arming never drives that icon.

The host derives audience decisions from its authoritative match state and
verified transport slots. Protocol 98 signs microphone origin with a per-room
Ed25519 key bound to the admitted physical owner. The host preserves that proof
while forwarding; it cannot sign speech as another owner's couch seats. Both
the coordinator relay and direct native receivers verify the proof before using
the speaker for block policy. The host can still set delivery gains and radio
mixing. Media uses bounded20ms Opus frames through the selected match transport;
playback has per-talker sequence checks, five-frame queues and stereo mixing.
The source five-speaker directional calculation is explicitly downmixed to the
native stereo output. Opus DTX replaces the unavailable XHV speech detector.
Original codec, physical headset routing, voice masking, echo cancellation and
radio sound effects are not implemented or claimed as retail parity. Keyboard
controls do not establish the original controller decoder mapping.

## Verification

The backend suite covers anonymous/nonmember denial, clan isolation, trusted
speaker metadata, replacement connections, removal/logout/expiry, frame limits,
replay and flood closure. `online_voice_codec` covers real Opus encode/decode,
20ms duration enforcement, packet bounds and non-finite input sanitation.
`online_voice_http` runs two native clients through the real account/clan relay
using synthetic speech and checks32 seconds of silence across several heartbeat
rounds, opt-in capture, talk, both mute paths, revocation
and leave. `frontend_clans_http` joins the same relay with actual SDL dummy
devices and drives the native Vista UI voice controls.
`online_party_voice_http` repeats the real native codec/transport checks with
accepted-party authorization, including idle heartbeats. `frontend_parties_http`
drives party voice controls using SDL dummy devices. Backend party checks also
cover invitation denial, isolated parties, private speaker handles, leader
transfer, removal, logout and active-gameplay revocation.

`game_voice_policy` verifies original audience, mute, death grace, proximity,
radio timing and remote-speaker selection boundaries. `online_match_launch`
queues two native processes through the real backend and exchanges actual native
movement/fire snapshots and synthetic microphone audio. It checks opt-in,
per-player mute/unmute, microphone mute, focus loss, disable/re-enable and playback
recovery. The rendered variant loads Lockout in two native Vulkan windows.

These checks do not establish physical microphone/speaker quality, two real
players over the Internet, impaired WAN latency or complete original voice parity.
Transport APIs follow [libcurl's receive](https://curl.se/libcurl/c/curl_ws_recv.html)
and [send](https://curl.se/libcurl/c/curl_ws_send.html) contracts; the codec follows
the [official Opus API](https://www.opus-codec.org/docs/opus_api-1.5/group__opus__encoder.html).

### Production proxy regression

The two real accounts accepted a native party invitation and the user confirmed
both chosen names in the roster on 11 October 2026. Voice briefly transmitted,
then both clients reported `Server returned nothing (no headers, no data)`.
Production request durations showed connections closing at about 10.003 seconds.
The direct native relay test passed; putting those same two native clients
behind the production Caddy configuration reproduced the exact error twice.

The cause was the proxy's `read_timeout 10s`, which also applies after a
WebSocket upgrade and raced the relay's ten-second heartbeat. The proxy now uses
`response_header_timeout 10s` to bound admission without that upgraded read
deadline. The write timeout remains ten seconds. Removing only the read timeout
passed the isolated check. Both party and clan paths then passed 32 seconds of
silence through the actual Caddy 2.11.7 binary, followed by real Opus transmission,
member/microphone mute and revocation checks. The public proxy was reloaded at
22:28 UTC; the backend process and account sessions were retained. A further
party regression passed 65 seconds of silence before transmitting audio. The
user then confirmed that, after rejoining and waiting at least 30 seconds, voice
stayed connected and worked in both directions. This confirms the reported live
disconnect is fixed; it does not establish physical WAN/headset quality.

### Match relay fanout budget

The six-native-client clan check exposed a separate match-relay limit: charging
every host-forwarded audio recipient against the host's shared 1,000-packet/s
budget can disconnect an otherwise valid lobby. Match audio now has a separate
75-packet/s, 75-packet-burst budget per validated speaker/destination stream,
including the native5Hz mute/enable preference announcements. The full
sixteen-client check exposed that those preference broadcasts alone exceed
the old shared control limit. The native codec produces 50 20ms frames/s.
Game datagrams have a separate
512-packet burst and 1,000-packet/s budget per validated destination. Text
controls and ignored postgame simulation packets retain a shared 512/1,000
budget. These are replacement
transport limits, not recovered Xbox codec constants.

Only authenticated, roster-validated routing keys create a stream budget.
Malformed routes still fail and ignored postgame packets still consume their
appropriate budget. Tests exercise full 16-player audio fanout alongside 30Hz
game traffic for three simulated seconds, including 24 fragments per maximum
32KiB snapshot for every recipient. The fourteen-native-client Major check
confirmed that fragmented snapshots also exceed a single host-wide budget.
Independent game/control flooding and real authenticated WebSocket game/audio
bursts still close the offending room.
The six-, fourteen- and sixteen-client native clan checks also completed
gameplay and postgame Opus, mute/unmute and microphone controls. Test-only
barriers coordinate fresh-catalog consent and postgame microphone checks
before the independent testers press Done; production consent is still
revalidated and never silently retried. This is local process evidence, not a
16-player physical WAN/headset result.

With Caddy installed, configure `HALO2_CADDY_EXECUTABLE` and run
`ctest -R 'online_(clan|party)_voice_proxy'`. The same regression can run directly:

```sh
python3 tests/online_voice_http_test.py build-online-social-20261011/online_voice_tests party --caddy /path/to/caddy
python3 tests/online_voice_http_test.py build-online-social-20261011/online_voice_tests clan --caddy /path/to/caddy
```

The fixture binds only loopback and uses disposable accounts and membership;
it reads no real Discord grants and changes no production social state. Proxy
configuration comes from the actual production Caddyfile. See Caddy's primary
[transport timeout and WebSocket documentation](https://caddyserver.com/docs/caddyfile/directives/reverse_proxy#transports).

## Discord relationship blocks

Release20261012T003658Z adds a private full-snapshot bridge from the optional
Discord Friends SDK owner to the native voice service. A relationship block
silences both directions in accepted-party, clan, active-match and postgame
voice. Manual unmute cannot remove the separate service-owned block. The host
continues to relay audio to unblocked listeners while its local playback is
silenced for blocked peers. No Discord IDs or block lists enter UI snapshots;
each match client receives only its own room-slot silence mask.

The last successfully synced list survives SDK disconnects and server restart.
Reconnect Discord Friends to refresh changes made while it was disconnected.
Failed updates retain the previous successful list and retry. This is a
client-provided preference bridge; the service does not independently verify
Discord relationships. See [Friends](friends.md) for limits and authentication.

101 actual VPS Docker backend tests passed. Real HTTP/WebSocket checks cover
bilateral blocks in all relay types, host transit to a third unblocked member,
private persistence and manual-preference isolation. Two native processes
verified actual Opus silence, talking-state suppression, unmute isolation and
explicit unblock recovery during gameplay and postgame. The full16-client
clan gameplay/voice/results/PCR/requeue regression passed with v7, as did the
native party and clan32-second idle voice checks. External Discord endpoints
remain deterministic fixtures; no live SDK relationship was changed by these
checks. The active Caddy header-only timeout and private account/party state
were reverified after activation.

## Postgame match-session voice

The original passive lifecycle audience continues through PCR16, including
opposing teams. One native GameVoice device/codec owner and the admitted relay
connection survive world teardown; a background pump routes session media while
the menu loads. No destroyed-world positions are consulted. Both participants
must enable voice and neither may mute the other. Actual talking/idle/disabled
state drives the original Vista player-row icon; PC185 still hides it.

F6 toggles voice, F7 toggles microphone mute, and M toggles the selected player
on a player-stat pane. Done disables the departed session microphone and closes
its relay. The room has a native15-minute report lifetime. A disconnected channel
stops capture while leaving the committed report available for inspection.

Two-process core/team checks verify real Opus playback after finalization,
mutual mute/unmute, enabled/talking state and capture shutdown on Done. They use
synthetic microphone speech; physical headsets and WAN quality are still untested.
