# Direct player-hosted ONLINE

The requested architecture is player authority and direct player transport.
The master verifies Discord identity, finds a match, distributes authenticated
connection setup and receives the post-carnage report afterward. It does not
run the simulation or forward gameplay/match voice in the new v12 protocol.

## Transport

`halo2-native-queue-v12` uses ICE to find peer UDP paths and DTLS to authenticate
and encrypt them. Descriptions and certificate fingerprints travel through the
authenticated room coordinator. Physical connection slots identify endpoints;
Discord IDs and persistent account IDs are not disclosed to other players.
Four local players share one physical connection. Existing guest exclusions
still govern personal post-carnage publication.

The pinned libdatachannel 0.24.6 stack uses an unordered data channel with zero
retransmissions and bounded queues. The native protocol already supplies its
own admission, snapshot and input semantics. ICE-TCP, TURN and WebSocket media
fallback are disabled. STUN discovers public addresses; it does not relay game
packets. A lobby needs a player who can connect directly to every other PC;
nonhost PCs need not connect directly to each other.

The separate peer worker continues simulation datagrams and match/postgame
voice while the master control connection reconnects. A transient signaling
socket loss does not revoke the admitted room. Report retries must reproduce
the original bytes, and confirmations must match the report digest. This is
not yet durable result recovery across a master process restart: live room
admission still resides in memory.

Legacy v10 relay clients remain in their own compatibility pool. They do not
silently become peers of a v12 room, and v12 refuses binary media at the master.
Party/clan menu voice retains its separate existing service; this transport
change concerns active matches and their postgame voice.

## Host quality and migration requirement

The user explicitly requested automatic host choice from connectivity and
speed measurements among the actual lobby PCs, then reelection among survivors
when the host leaves. Initial v12 admission records mutual direct reachability
and SCTP RTT, excludes unreachable host candidates, and permits pregame host
optimization after gathering. It does not yet measure upload capacity or claim
the complete requested connectivity-and-speed selection policy.

Microsoft documents peer QoS on the actual gameplay channel and describes
latency/bandwidth requirements for host selection:
[QoS measurements](https://learn.microsoft.com/en-us/gaming/gdk/docs/features/console/networking/game-mesh/qos-networking),
[target session initialization](https://learn.microsoft.com/en-us/gaming/gdk/docs/services/multiplayer/matchmaking/concepts/live-matchmaking-target-session).
These describe later GDK platforms, not proof of the original Xbox coefficients.
The recovered Halo 2 host comparator has unresolved property units, as recorded
in [the original host audit](halo2-host-migration-evidence.md). Do not label
those fields ping/bandwidth without evidence.

Gameplay host migration remains gated on a complete authoritative continuation
owner. A direct connection, election or replicated display snapshot does not
restore the complete world. Follow [the continuation inventory](player-host-continuity-progress-20261013.md).

## Reproduction

The first CMake configure retrieves the commit-pinned ICE/DTLS/SCTP dependency.
Offline builds may set `FETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL` to a matching
recursive checkout. No separate libdatachannel shared library is needed at run
time; its third-party notices must accompany packaged binaries.

```sh
cmake --build BUILD --target online_peer_transport_tests online_match_launch_tests online_couch_launch_tests
BUILD/online_peer_transport_tests
python3 tests/online_match_launch_http_test.py BUILD/online_match_launch_tests
HALO2_TEST_MASTER_OUTAGE=1 python3 tests/online_match_launch_http_test.py BUILD/online_match_launch_tests
python3 tests/online_couch_launch_http_test.py BUILD/online_couch_launch_tests mixed
python3 -m unittest discover -s multiplayer/master_server/tests -t .
```

For production, install Ubuntu's `coturn` package, disable its default TURN
service and install `master_server/halo2-stun.service`. Allow UDP3478 and set
`H2_PEER_STUN_URLS=stun:master.halotoforever.com:3478` on the master. The custom
unit uses `--stun-only`: TURN allocations are ignored. This is address discovery,
not dedicated gameplay authority or a media relay.

Local socket/native-process checks do not establish consumer-router NAT,
physical headset, Internet bandwidth or live gameplay migration acceptance.

The separate WAN probe exchanges actual encrypted native UDP datagrams with an
isolated aiortc echo endpoint on Vultr; SSH carries only authenticated SDP/ICE
setup and final counts. `tools/run_online_peer_wan_probe.py BUILD/online_peer_wan_probe`
reproduces it after installing the documented isolated endpoint. This is a
one-consumer-network/public-server path check, not a two-consumer-NAT gameplay
or headset test.

## Delivered verification

The source and native delivery are recorded in
[the private verification capsule](/home/eric/Developer/Halo-2-Decompiled/.local/direct-peer-20261013/verification.json).
Both installed launchers select its frozen runtime. The deployed backend passed
205 regressions and isolated retained-state restore initialization; all36 old
retained tables and the master key were unchanged, with one new empty migration
receipt table.

Two actual native processes exercised direct gameplay, real Opus capture/decode,
muting, passive postgame voice, source PCR agreement, Vista Done and requeue.
During a forced2.5-second master outage, simulation advanced89 ticks and
playback delivered86/88 audible frames before reconnection and successful
report publication. Rendered native clients also completed the game/PCR/requeue
flow and used the real Vista pause confirmation to leave. Eight-player couch
checks, including Tombstone Fiesta with the authored animation bank, retained
local guest PCR rows while omitting guest personal history.

The native-to-isolated-Vultr probe sent50 real encrypted1400-byte datagrams,
received48 echoes, and observed41ms SCTP RTT; the remote received49 packets.
This establishes one consumer/public-server UDP path, not two-home NAT coverage
or headset/gameplay-WAN acceptance. Party/clan menu media and complete gameplay
migration remain outstanding.
