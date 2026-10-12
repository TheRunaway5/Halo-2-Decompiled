# Shared round lifecycle and native boundaries

This is the phase owner's original verification snapshot. The later
[one-round life menu integration](MP_FINITE_LIFE_MENU_INTEGRATION.md) admits
the life field without entering the still-unbound later-round services.

`source_round_lifecycle.cpp` now implements the recovered shared phase stores
and non-game-over post-world controller. Its authority is the complete original
retail, TU1.1 and TU1.5 evidence in
[the earlier research](MP_ROUND_LIFECYCLE_RESEARCH.md), plus a separate original
cap-order fixture. Neither original instructions nor return values are replaced.

The native `MultiplayerMatch` binds the first-active transition before joining
participants, and phase2 after the ending world pass. The first transition now
sets the configured signed16 clock and the original low stat-validity byte to1.
The final transition clears that byte, freezes enabled recording, and loads
five times the original signed16 tick rate. At30Hz its round countdown is150;
final game-over leaves this countdown alone while the separate seven-second
owner advances. Current HUD/network state consumes the shared clock field.
The registered Slayer initializer has no memory stores and its phase callback
is a literal return, so these two specific callbacks are implemented directly.

The core retains the signed16 phase/index/clock, signed32 pending status and
countdown, unsigned32 clock stamp and flags, and the complete unknown108-byte
ranking bank. It applies the existing exact stat-bank carry, player-life and
netgame-row owners. Entering phase3 increments the index with signed16 wrap,
marks an existing match entity dirty with flag8 and loads the signed rate. Expiry in phase3 requests active
status1. Expiry in phase2 at an index of31 or more starts game-over before
applying any independently pending phase. The added cap fixture checks that
ordering, including preservation of score during a pending phase1 carry.

World reset, netgame object unlink, reached replication and game-over begin
and replication are explicit owner services. Reaching a missing service throws.
In particular, a later active round requires the complete original object/map
reset owner even when all netgame rows are empty. The original respawn-reset
replication branch is separate from generic dirty flags: it requires session
state4 and entity tracking flag4 before setting bit1. That branch has no live
native binding; non-NONE entities require its actual service. NONE entity inputs
in the original fixture do not establish original network behavior.

The preparation wrapper's NONE-entity gate is now implemented directly;
see [the source replication contract and new validation](MP_ROUND_PREPARATION.md).
The native adapter installs only the two verified Slayer callback leaves.
It cannot enter a later round through these services. Full live body/world
teardown, authored map reset, reached replication and subsequent spawning,
combat, HUD and replication remain unfinished. Stock Elimination still requires
its authored **Reset Map Every Round** setting. No variant or editable round/life
rule gains admission in this slice.

## Validation

The C++ comparison covers1,746 complete original phase/post-world records:
145 phase and432 post-world inputs per image, plus five independent pending
statuses at the round cap per image. It compares the complete stat/ranking
banks, all16 raw player records, engine respawn rows, all32 scenario netgame
rows, recording bytes and scalar control fields. The positive later-round
fixtures declare an empty object pool, NONE units/replication/local users and
disabled full-map reset. Test-only empty-world callbacks never reach the live
native binding. Missing-owner regressions retain reset, event, cap-begin and
respawn-replication failures.

The separate cap oracle reproduces its full fixture byte for byte on a second
execution. The16 focused CTest checks pass: original variant buffers and
normalization, round/end/life/game-over owners, native FFA/team end and life
flows, accounting, multiplayer startup and inventory. The existing end test
now checks initial phase/clock/stat validity, final phase2/countdown, and the
unchanged round countdown throughout225 postgame UDP ticks per case. Its
existing controlled SMG impacts, HUD/result-board queries and audio checks pass.
Its fatal stimuli, geometry/vitality controls and other original limits remain.

Scoped ASan/UBSan passes both the1,746-record comparison and the native end
flows. Ten translation units are instrumented: both tests, the changed shared
phase and native match owners, and their direct stat/life/netgame/game-over/sound
writers. Other libraries remain ordinary builds; this is not whole-engine
instrumentation. A separate existing UDP first-person test also passes its
16-participant and fragment recovery cases.

The rebuilt production application completes90 headless Lockout ticks with
four local participants using the actual TU1.1 executable data. The actual menu
audit completes78 start actions and still admits2 of39 variants, blocking37;
`--require-all-admitted` therefore returns1 as expected. These are startup and
focused ownership checks, not a rendered full stock match or a physical LAN
session. The all-variant operational goal remains active and incomplete.

## Reproduce

Wait for any other shared `build` operation to complete before compiling or
reading its archives/executables.

```sh
.tools/python/bin/python tools/oracle_round_lifecycle_cap.py
.tools/python/bin/python tools/oracle_round_lifecycle_cap.py \
  --fixture analysis/multiplayer/game-variants/round-lifecycle-native/cap-reproduced.json \
  --verify-fixture tests/fixtures/round_lifecycle_cap_original.json
cmake --build build --target round_lifecycle_tests game_end_native_tests \
  player_lives_native_tests round_end_native_tests multiplayer_tests team_match_tests \
  slayer_accounting_tests halo2_native audit_game_variants -j2
ctest --test-dir build -R '^(original_round_lifecycle|native_game_end|native_player_lives|native_round_end|native_team_match|retail_slayer_accounting|native_multiplayer)$' --output-on-failure
.tools/python/bin/python tools/sanitize_round_lifecycle.py
./build/halo2_native --multiplayer --map assets/disc/maps/lockout.map \
  --shared-maps assets/disc/maps \
  --gameplay-xbe assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03 \
  --local-players 4 --simulate 90
./build/audit_game_variants assets/disc/maps \
  analysis/multiplayer/game-variants/round-lifecycle-native/runtime-audit.json --require-all-admitted
```

Retained logs, scoped sanitizer commands and exact source/fixture/binary pins
are under `analysis/multiplayer/game-variants/round-lifecycle-native/`.
The previous research manifest remains historical; this native slice has its
own verification record and does not reinterpret that earlier scope.
