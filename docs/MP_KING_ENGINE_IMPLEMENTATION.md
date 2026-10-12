# King engine implementation and remaining bindings

The shared source King engine now implements original initialization, authored
hill construction, player occupancy, scoring and the complete slot18 update
control chain. This is an engine recovery milestone. All five King variants
remain blocked by the normal native setup adapter, and none is declared fully
operational. The [full game-mode goal](GAME_MODE_PROGRESS.md) remains active.

## Implementation

[source_king.hpp](../include/halo2/source_king.hpp) preserves the original
`0x20`-byte scenario marker contract and complete `0x1DC`-byte engine state.
[source_king.cpp](../src/source_king.cpp) implements the owners recovered in
[the original-source research](MP_KING_ENGINE_SOURCE_RESEARCH.md). It is part
of `halo2_systems` and has strict floating-point compile options.

The engine consumes the supplied scenario rows and options, including signed
fields, source-order groups11..18, identifier0 spline controls, identifier1
height markers, the original32-sample spline matrix, movement interval and
King option bits. There are no named map or variant branches. Original
compiled constants remain compiled constants. The state retains unused and
stale fields, including the original partial-marker build behavior.

The occupancy owner uses the actual current-unit world-position provider,
inclusive vertical bounds, strict radius test and all32 polygon edges. Inside
timers retain signed16 wrapping. The scorer preserves uncontested admission,
phase/status gates, team clock wrapping, multiplier behavior, signed timer
tie-breaks, last-awarded timer reset and getter/recorder/award/publication order.
The world owner publishes occupant changes before scoring and moves the hill
after scoring, consuming the retained shared RNG. Required position, score,
recording, engine-event and replication services throw when unavailable;
original NONE-entity and inactive branches naturally need no provider.

## Original executable verification

The tools below pin and execute unchanged retail, TU1.1 and TU1.5 XBEs. They
import no native implementation, patch no instructions and supply no gameplay,
geometry or service return:

- [oracle_king_engine.py](../tools/oracle_king_engine.py):54,897 complete
  initialization/build/selection/occupancy calls. Includes all12 disc maps,
  all57 authored hills,14 synthetic scenarios, stale/odd/capped control banks,
  8,747 occupancy inputs and512 selection inputs per image. Original unit
  position calls execute against declared unparented object records.
- [oracle_king_scoring.py](../tools/oracle_king_scoring.py):4,386 complete
  scorers using real registered engines, source pools/getters/score writers and
  recorder gates. There are1,017 actual award calls per image. Signed timer
  boundaries, team aggregation, suppression flags, unitNONE, phase/status,
  playback and contested/multiplier settings are covered.
- [oracle_king_world.py](../tools/oracle_king_world.py):2,016 complete world
  updates, preceded by4,032 complete original initialization/start calls.
  Each image produces234 hill relocations and497 category6 engine-event
  deliveries. Literal Lockout `matg`/`mulg` data back notification lookup;
  scenario geometry comes from all12 maps.

All65,331 complete original calls return without fabricated success. The three
images have byte-identical observed output for every shared input. Reports
also pin38 complete function-shape correspondences against TU1.5, relocated
data operands and complete instruction listings. Reports and the reproducible
verification manifest are under
[king-engine-20261011](../analysis/multiplayer/game-variants/king-engine-20261011/).

[king_engine_tests.cpp](../tests/king_engine_tests.cpp) compares the native core
against all three images:879 complete geometry states,26,241 occupancy pairs,
1,536 selections,4,386 scorer cases and2,016 world updates. Comparisons include
all engine-state/score-bank bytes and callback order, not approximate visual
shapes or score totals alone. Native score services use the already recovered
shared player/team writers. Event and disabled-recorder callbacks observe
delivery; they do not prove native HUD/audio or an enabled recorder.

The x87 perimeter and determinant intermediates use the original control
word`027F` precision, while spline and score geometry follow the original SSE
binary32 operation order. Invalid source counts/indices or literal signed
division traps are explicit unavailable inputs rather than native undefined
behavior. These checks do not claim an original contract for malformed maps.

Reproduce from the repository root with `.tools/python/bin/python`:

```sh
.tools/python/bin/python tools/oracle_king_engine.py
.tools/python/bin/python tools/oracle_king_scoring.py
.tools/python/bin/python tools/oracle_king_world.py
cmake -S . -B build-king-20261011 -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
cmake --build build-king-20261011 --target king_engine_tests -j2
ctest --test-dir build-king-20261011 -R '^original_king_engine$' --output-on-failure
```

## Operational gates still open

These are isolated original-owner and native-core comparisons with declared
ABI inputs. Player/object allocation, normal setup/profile handoff, natural
movement/combat, enabled recording, live local HUD/audio and original transport
are not established by them. Local users and replication entities areNONE in
the successful original world corpus. The native callbacks must be bound to
their actual runtime owners before corresponding settings become admissible.

The subsequent [bounded native binding](MP_KING_NATIVE_BINDING.md) now connects
initialization/round/update dispatch, the shared RNG, per-player timer/position,
death statistics, authored equipment/spawn consumers and shared score/end/result
owners. Its frozen build passed seven focused CTests and scoped ASan/UBSan checks.
Those native fixtures use explicit event observation sinks; they do not establish
normal presentation, enabled recording or native objective transport.

The later [shared player-damage binding](MP_PLAYER_DAMAGE_NATIVE_BINDING.md)
connects native shots/area requests to the actual hill/global damage and
resistance query, sanitized strength, spawn countdown and friendly-fire owner.
King still needs live camouflage, authored hill display, complete HUD/event
and recording delivery, live replication, and normal complete local/network
matches through score limits, rounds and results. Phantom King requires actual
camouflage behavior. No startup gate was relaxed, and the current2-admitted/
37-blocked inventory remains the goal's status.
