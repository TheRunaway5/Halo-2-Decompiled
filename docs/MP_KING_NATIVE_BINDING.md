# Bounded native King engine binding

The native match now dispatches the recovered King lifecycle, occupancy, world,
scoring and death-stat owners. This is a binding milestone within the active
[all39-variant goal](GAME_MODE_PROGRESS.md). Normal King menu startup and native
King transport remain gated pending complete presentation, camouflage and replication
consumers. The five built-in King variants are not declared operational.

## Data and lifecycle

The existing native rules adapter now carries the registered source engine type
independently of the UI/catalog engine ID. `king_rules` decodes a source-valid
King buffer using the original descriptors and defaults. It admits the supported
shared settings, hill uncontested/multiplier bits and movement interval by actual
field/bit coverage. Aliases therefore retain their original meaning: parameter29
changes King's multiplier bit, while parameter30 aliases the hill damage bit
and receives its same admission. There are no named-variant or map branches.

The bounded binding consumes signed16 movement seconds at variant+F4 and the
literal32-bit King flags at+F0. Hill damage/resistance now have verified native
consumers; camouflage remains gated. It copies every
literal32-byte scenario marker in original row order. Player inside timers retain
signed16 width/wrap and full salted current-unit identities; occupancy receives
the actual native unparented unit position.

`MultiplayerMatch` performs complete King initialization before its initial active
phase. The shared phase owner dispatches King round-start, which builds hill0 and
sets its countdown without clearing team/player timers. Both original registered
phase callbacks are literal `ret4`; the binding preserves that fact rather than
inventing an additional King reset. The original callback contracts and ordinary
mode1/playback call-order evidence are in
[the focused research](MP_KING_NATIVE_CALLBACK_RESEARCH.md).

The pre-world native engine phase now runs each occupied player's King callback,
then complete King slot18, then the shared round-end query and world objects.
King consumes the same retained RNG as other world consumers. Its awards call
the actual shared native player/team score writers, including score-limit end,
winner awards, post-world phase application and result accumulation. Timers are
published back to native players before callback delivery and after the update.

Death dispatch preserves ordinary attribution/respawn/stat ownership while
selecting the registered engine callback. King adds retained statistics28
(credited kill of an occupant) then27 (credited kill while occupying), using the
actual imported descriptors and enabled/frozen gates. Those writes can publish
to the attacker's selected team. King deaths do not request Slayer kill scores.
The shared King trait query is implemented and compared to originals. The later
[shared damage binding](MP_PLAYER_DAMAGE_NATIVE_BINDING.md) connects actual shots
and area requests to hill/global damage, resistance, sanitized strength, spawn
protection and friendly fire. Camouflage remains unbound.
The later [announcer consumer binding](MP_KING_PRESENTATION_NATIVE_BINDING.md)
imports and plays actual King response tags, and selects the registered King
start publication at the native activation boundary. Its independent source/
PCM checks do not yet establish a normal King world/presentation timeline.

## Authored equipment and spawn consumers

Native spawn-zone and weapon-row selectors now receive the selected original
engine. King slot33 shares Slayer's literal true callback, while its slot32
appends authored spawn types8 then7 at the retained hill center after ordinary
influences. Each append respects the original128-entry limit and imports its
distances, bounds and weight from the existing map settings.

Content loading retains all ordered starting-equipment flags/mode banks. Native
spawn selects the first eligible row for its source engine. The starting-weapon
lookup, pickup owner, complete world-weapon timer and world-weapon scene accept
the selected source type. Multiplayer game type2 remains a separate input to
weapon/object creation; it is not a Slayer-only selector. These changes preserve
original selector behavior rather than creating King weapon lists.

## Consumer and validation boundary

Constructing a native King match requires explicit recording-event,
score-notification and objective-notification consumers. Missing consumers fail
at construction. The ordinary menu still rejects King, and both LAN host
constructors reject engines whose objective replication is unbound. A local
native King state getter exposes the actual retained geometry for the future
presentation owner; existing player snapshots do not imply a King hill protocol.

[king_runtime_tests.cpp](../tests/king_runtime_tests.cpp) exercises the native
binding with a declared synthetic arena and observation sinks. It targets FFA
and team occupancy/scoring, uncontested admission, multiplier/tie selection,
movement, authored equipment selection, natural combat/respawn, retained hill
statistics and score-limit results. Its event sinks observe actual delivery;
they do not play HUD/audio or implement enabled recording.

[oracle_king_runtime_boundary.py](../tools/oracle_king_runtime_boundary.py)
executes the exact synthetic marker bank and declared positions in all three
original images. All three reject the exact center, which lies on a discrete
spline ray crossing, and accept the tested nearby interior points. The fixture
uses those original-verified points without changing the boundary predicate.

[king_callback_tests.cpp](../tests/king_callback_tests.cpp) compares3,780 trait
queries and1,548 whole retained-stat banks against unchanged retail/TU1.1/TU1.5
execution, including forced false hill overrides, signed timer edges, NONE/self
credit, low-byte betrayal, ignored fourth argument, team membership, recording
gates, descriptor bounds and retained bit15. The original generator reproduces
5,346 complete calls including initialization/phase transitions; its complete
report and fixture reproduce byte-for-byte.

Run `.tools/python/bin/python tools/verify_king_native.py` for a frozen full-source
build, original reproduction, focused CTests and scoped native ASan/UBSan checks.
The successful [verification manifest](../analysis/multiplayer/game-variants/king-native-20261011/verification.json)
records the resumed unchanged snapshot `verify-20261011T204625.057091Z`. All seven
focused CTests passed, including the native King arena and the21-map world-weapon
scene regression. The scoped sanitizer instruments the changed match, inventory,
network, King and content owners plus the arena test; other runtime libraries
retain their ordinary build. The separate callback executable is fully
instrumented. Both sanitizer executions passed. Source-only callback and arena
oracles reproduced successfully before this build. Asset pins are checked before
and after tests, and live checkout drift is recorded separately from the frozen
source. These results do not establish normal gameplay or presentation.
The verifier pins copied sources, tested binaries and logs and records live
checkout drift separately. Its native sanitizer instruments the changed owners
and test while using ordinary libraries for the remaining runtime. A separate
callback executable is fully instrumented.

The remaining operational work includes real hill rendering/markers,
HUD/announcer delivery, live camouflage, enabled recording, objective state
replication, later-round reset and complete normal local/network flows for all
five King variants. Controlled native arena checks do not prove physical Xbox,
original transport, or a full retail match.
