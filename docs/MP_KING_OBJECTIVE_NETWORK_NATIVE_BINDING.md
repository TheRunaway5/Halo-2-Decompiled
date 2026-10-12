# Native King objective delivery

The native port protocol now carries the selected source engine, King's signed
hill index and occupant mask, and each player's signed inside timer. Match
snapshots expose these fields from the existing authoritative source state.
The host admits the bounded source4 match binding. Normal frontend King
admission remains gated. Protocol38 requires matching native clients.
[Snapshot types](../include/halo2/multiplayer.hpp),
[authoritative publication](../src/multiplayer.cpp),
[UDP encoder/decoder](../src/multiplayer_network.cpp),
[protocol version](../include/halo2/multiplayer_datagrams.hpp).

The actual UDP test loads the original Lockout cache and owned Xbox TU1.5
executable, joins a full16-player roster, and runs stationary/moving FFA/team
King. It compares complete objective fields, player generations/timers/scores,
team scores and final results. The client rebuilds the authored hill from its
received index and compares the first180 hexadecimal bytes of the geometry
state. Actual packets pass through a UDP proxy exercising fragmentation,
reverse ordering, duplicates, partial loss, recovery and stale replay. An
occupied player slot leaves and rejoins while the match is active; its timer
clears and its complete source identity changes.
[Test](../tests/king_objective_network_tests.cpp),
[original geometry contract](MP_KING_ENGINE_IMPLEMENTATION.md).

The first frozen test failed because it attempted the rejoin after stationary
King had ended. The runtime correctly rejected that join. The failed source,
build and logs remain preserved. The corrected test reuses the same pinned
production objects; its overlay, link inputs, binaries and commands are hashed.
Five baseline CTests, both FP/scope network regressions, the corrected objective
test and a scoped ASan/UBSan/leak run pass. The sanitized owners are the actual
match, network and objective test; their linked libraries remain ordinary.
[Failed baseline](../analysis/multiplayer/game-variants/king-objective-network-native-20261012/verify-20261012T024545.653433Z/commands.json),
[repair verifier](../tools/verify_king_objective_network_repair.py),
[frozen verification](../analysis/multiplayer/game-variants/king-objective-network-native-20261012/verification.json).

Positions are explicit debug inputs before actual gameplay ticks. Recording,
score-event and notification callbacks observe their mandatory boundaries.
This establishes native objective delivery and preserves the source geometry;
it does not establish normal movement, live recorder/HUD/announcer consumers,
Xbox wire compatibility, frontend admission or a complete playable match.
The all39-variant goal remains active and incomplete.
