# Authored finite-life settings in one-round Slayer

The native Slayer adapter now accepts the original **Lives Per Round** field
for its existing one-round path. Normal Split Screen and System Link menu
edits can deliver one or two lives into actual FFA and team combat, respawning,
exhaustion, win awards, retained statistics, native UDP and the font-backed
results scoreboard. This is a bounded native integration, not completion of
Elimination, the spectator system or the all-variant goal.

## Original authority and admission

Parameter16 is the signed32 field at variant+7C. Its original validator permits
0..32767; configured zero initializes the player's signed16 remaining count
to NONE, while a positive value initializes that count. The recovered shared
owners already consume the field for initialization, active deaths, respawn
admission and participant/life endings. See [the player-life contracts](MP_PLAYER_LIVES_RESEARCH.md)
and [the participant/life end owner](MP_ROUND_END_RESEARCH.md).

The production change permits this descriptor's bytes in the existing source
field capability mask. It does not recognize variant names or invent weapon,
team or elimination rules. Original ordered setters, signedness, validation
and getters continue to own the settings buffer. Tests also cover0,1,2 and32767,
reject negative and overflowing limits, and retain multi-round rejection.

The distinction from later rounds is visible in the complete TU1.5 `15DDC0`
listing. At `15DF96` it reads the round selector at game-options+188
(variant+4C). Selector0 with round index0 takes the final path through
`15E005` and `15E019`, rather than the nonfinal path at `15E04E`. The native
adapter still rejects changes to that round selector and the authored
Elimination reset settings. Enabling this life field therefore does not supply
empty reset callbacks or bypass a required later-round service.

Fresh executions reproduce9,252 original life-owner calls and9,801 complete
round-ending query/wrapper calls across retail, TU1.1 and TU1.5. Both fixtures,
both full reports and all105 instruction listings equal the previous evidence
byte for byte. No original instructions or reached return values are replaced.
The existing oracle input and whole-call limits still apply.

## Native flow exercised

`round_end_native_tests` creates and signs in a profile through normal menu
actions, selects the actual Slayer or Team Slayer row, opens its authored
editor, and finds Lives Per Round, Score to Win and Round Time Limit through
the loaded parameter tags. It edits lives to1 or2, disables score/time limits
to isolate life endings, returns through the real menu stack and presses Start.
All eight starts succeed: two life values, FFA/team, Split Screen/System Link.
Their complete original variant buffers agree between the two protocols;
the selected Lockout map, protocol and signed-in profile are retained.

Four controlled match cases then consume those actual launch settings. The
selected profile reaches native join and the original sanitizer; its complete
configured record survives UDP delivery. Each case adds a second local
participant and an actual localhost UDP participant. These additional joins
are explicit engine inputs, not a demonstration of three users signing in
through the frontend.

Across the four cases, twelve actual SMG projectiles hit and kill their
victims. A nonfinal death genuinely respawns with its remaining life count;
final exhaustion prevents respawn. Exhausting one of two opposing teammates
does not end the team match. Exhausting the last eligible opposition ends on
the next engine pass, after the death's world pass. Each case checks its exact
shots, hits, kills, deaths, winner, once-only win award, final totals and
retained win statistic. The native UDP peer receives the same finalized fields.

The exhausted participant remains out of the ordinary dead-timer automatic
scoreboard branch after the first three seconds. Manual scoreboard input still
opens the actual font-backed display. Final game-over opens results without
manual input. Subsequent300-tick updates complete the game-over countdown and
retain exhausted ownership as NONE. Separate cases preserve all-exhausted
NONE winners and ordinary unlimited-life retained-team departure behavior.
The earlier life integration also retains exhaustion ranking, icon and dimming
coverage.

Arena collision and kill triggers are disabled in the SMG cases; positions,
radius and vitality are declared stimuli. Projectile simulation, damage,
attribution, respawn timers, life/end owners, results and transport execute
their production paths. The no-winner cases use the explicit fatal-input
adapter. These cases do not establish natural stock encounters or complete
original death attribution for every cause.

## Verification and remaining work

All15 focused CTest checks pass, covering the original variant buffers,
normalization and ordered writes; life, round-end, lifecycle and scoreboard
owners; native life/end/team/profile flows; LAN startup and retail multiplayer.
The production application builds and accepts the authored life parameter in
three-participant FFA and team commands. Both run300 world ticks with
`finished=0`; those startup runs are not completed combat matches.

ASan, UBSan and leak detection pass for the complete round-end comparison
harness and the scoped native integration. The native instrumented units are
the menu state, variant buffer, life/round/end owners, Slayer, match, network,
scoreboard and test. Other linked libraries use their ordinary builds. The
final checks use an isolated `frozen-source`/`frozen-build` pair so concurrent
shared-checkout edits cannot change their inputs. Its8,930-file manifest pins
the source, headers, build files, shaders, dependencies, tools and copied test
fixtures. Original assets and analysis backing are retained through symlinks;
the selected original images and maps are independently hash-checked.

Evidence, exact sanitizer commands, source/binary hashes, logs and the strict
catalogue audit are in
`analysis/multiplayer/game-variants/finite-life-menu-20261011/verification.json`.
The audit runs78 normal menu Start actions and still admits2/39 built-in rows,
blocking37. Its strict exit1 is expected and means the full goal is unfinished.

Complete spectator/death-observer behavior, later-round world reset and
transitions, related HUD/announcement delivery, general weapons, objective
engines and the remaining editable settings still need their actual owners.
The separate Vista offline session/PCR integration has its own one-player
boundary; these finite-life tests do not extend it to team or LAN sessions.
Rendered end-to-end play, physical LAN and original Xbox transport parity are
not established here. No additional built-in variant is declared operational.

```sh
.tools/python/bin/python analysis/multiplayer/game-variants/finite-life-menu-20261011/run_frozen_checks.py
.tools/python/bin/python analysis/multiplayer/game-variants/finite-life-menu-20261011/run_frozen_sanitized.py
.tools/python/bin/python analysis/multiplayer/game-variants/finite-life-menu-20261011/verify.py
```
