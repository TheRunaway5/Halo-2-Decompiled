# Original participant and life-based round endings

The source recovery and verification below retain their original snapshot.
[The subsequent one-round menu integration](MP_FINITE_LIFE_MENU_INTEGRATION.md)
now connects the life field through normal Start actions; it does not complete
Elimination, spectator behavior or multi-round reset.

The native match now runs the original participant/life end check before world
updates. Diagnostic single-round finite-life FFA and Team Slayer matches can
finish through real SMG impacts, win awards, final statistics, native UDP and
the automatic font-backed results scoreboard. Ordinary Team Slayer also ends
when only one of its retained teams remains. Complete round transitions/reset
and related HUD messages remain unfinished; finite-life menu settings and the
authored Elimination variants stay gated. No additional variant gains admission.

## Original owners

| Owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Occupied player / retained team count | `158AB0` | `15A0F0` | `15B4D0` |
| Present and eligible membership collection | `158C00` | `15A240` | `15B620` |
| End decision, force flag and winner | `158D20` | `15A360` | `15B740` |
| Active-state wrapper and round-end call | `15BA20` | `15D060` | `15E440` |

Eight complete source pairs preserve 650 instruction rows across the three
images, with literal relocated operands retained. The wrapper calls the same
[original end/award owner](GAME_MODE_PROGRESS.md#original-match-end-and-native-final-results) already recovered for
score-based endings. Its caller `15EBA0` runs the clock owner `15E870` first and
the participant check before world object updates. Frame owner `13AAC0` calls
that engine owner before object updates and `15F450` finalization. The native
binding follows that order. This is a static caller correspondence and an
independently executed owner chain; it does not claim an executed whole
original engine/world frame.

## Count, type and winner contracts

For team play, entrant count is the population of teams 0..7 in the original
16-bit retained mask at engine+0E. High mask bits do not count. For FFA it is
the occupied player count, including allocated records excluded by later
membership/flag checks. The threshold is one entrant when this count is below
two, otherwise two.

The collection visits occupied slots in ascending bitmap order. It excludes
flags bit1 and signed memberships outside 0..15. It counts distinct present
memberships and distinct eligible memberships separately. Eligibility is the
recovered [respawn-admission query](MP_PLAYER_LIVES_RESEARCH.md), or the original
pending-record case where flags bits0/1 are both clear. A waiting unlimited-life
player remains eligible. An absent unit with zero remaining lives can be
excluded; a present unit with zero lives is still eligible. Several teammates
therefore count as one eligible membership, and one exhausted teammate does
not by itself eliminate the team.

An eligible membership count below the threshold requests a round end. A
present membership count below that same threshold requests the force flag.
Exactly one eligible membership supplies its last eligible full player datum
in FFA, or that player's signed membership in team play. Other branches retain
the caller's winner input. The active wrapper initializes that input to NONE,
so an all-exhausted roster can end without a winner. The query's nullable force
output and untouched winner behavior are preserved.

The original CTF selector1/9, options+240 and engine+2F0 suppression branch is
also retained and checked against registered original CTF bindings. Its raw
fields do not become invented native rules or prove CTF operation. Native
admission still supplies the recovered Slayer selector2.

The wrapper requires an engine, lifecycle1 and either raw mode4 or status1.
It awards through the original winner/round-selector contract. A nonfinal
round changes pending status and emits the original notifications; a final
round additionally begins game over. The oracle checks complete stat-bank
effects and phase/game-over fields for both single-round and first-to-three
inputs. Native multi-round phase/reset ownership remains unbound.

## Whole original execution and tag backing

[The instruction oracle](../tools/oracle_round_end.py) executes 9,801 complete
original calls across retail, TU1.1 and TU1.5. Each image executes 1,313 queries
twice, with present and NULL force-output pointers, and 641 complete active
wrappers. Inputs cover empty and sparse rosters, all 16 slots, shared and
invalid memberships, pending/excluded flags, signed life boundaries, retained
empty teams, high mask bits, raw modes, engine absence, lifecycle/status gates,
winner preservation, CTF fields and saturated win counters.

The exact departure regression retains a leading team0 after both members
leave while team1 remains, including one member waiting to respawn. The
original wrapper ends for team1 before the later time deadline. The native
deadline regression separately leaves at the deadline: the earlier clock
owner retains the score-based departed-team result. These establish the two
distinct triggers and their ordering.

All reached original instructions execute without patches or supplied game,
platform or service returns. Occupied pools, field values, clock and stat
banks are declared API inputs. Replicated entities and local users are NONE;
recording is disabled. Both query calls leave the complete 16-player record
bank and stat bank unchanged; wrapper calls preserve player records and are
compared against the complete resulting 0x254-byte stat bank and phase fields.

Nonfinal notification dispatch needs the actual tag data. An initial execution
stopped at retail `19DF16` on absent matg+16C. The successful oracle maps the
literal original Lockout cache, resolves its matg/mulg references through the
original tag-table lookup and executes the notification owners. No empty
replacement tag or successful notification return is supplied. This declared
cache backing does not establish original cache loading, a local HUD display,
sound playback, live participant creation or a frontend/session callback.

Fixture SHA256:
`6dcf6bd4fbc0579cc823da28ab5866ff45ea8df29bd50dfccf2e3a0cf4bb84c7`.
A second independent execution reproduces the fixture, complete report and
all 66 reached-owner instruction listings byte for byte. Those listings contain
4,045 static instruction rows; listing a complete owner does not claim every
branch ran in every case.

## Native integration and verification

The production match invokes the shared check after time expiry and before
world updates. A fatal impact consumes lives and releases unit ownership during
one world pass; life exhaustion then ends the match in the following engine
pass. It does not end recursively inside the damage callback. Existing result
owners award wins, complete totals and freeze retained recording after that
ending world pass. Authoritative UDP and automatic scoreboard consumers receive
the resulting fields.

The native test joins two local participants and an actual localhost UDP
participant. In FFA and team cases it fires two real production SMG projectiles.
One exhausted player leaves the match active; exhausting the other opposing
participant ends for the survivor. Both shots, hits, victim deaths, retained
win record, final totals, peer state and the automatic font-backed scoreboard
are checked. Subsequent 300-tick updates complete the game-over countdown
without respawning exhausted players. Separate cases cover all-exhausted
no-winner endings and ordinary unlimited-life Team Slayer departure.
Arena collision, positions and vitality are declared test stimuli. The
no-winner case uses the explicit native fatal-input adapter, not recovered
general damage attribution. These are bounded runtime cases, not natural stock
encounters or normal finite-life frontend sessions.

All 35 rebuilt focused regressions pass. ASan, UBSan and leak checks pass with
the query/life core, comparison harness and existing award/stat writers
instrumented. The native integration instruments the query/life core, match,
network, scoreboard and test units; other linked native objects retain their
ordinary production builds. Exact commands, scope and source/binary pins are
recorded in `analysis/multiplayer/game-variants/round-end-verification.json`.

The fresh strict audit executes 78 normal menu starts, admits 2/39 variants
and blocks 37 (expected exit1). The fresh production four-local-player Team
Slayer command reaches 300 ticks with `finished=1`; these are not 300 active
combat ticks or finite-life frontend coverage. Complete round reset/transition,
related HUD messages, general weapons, objective engines, edited-rule flows,
Carnage Report/session return, rendered end-to-end play and physical LAN remain
open. The all-variant goal remains active with its vanilla tag/data scope.

```sh
.tools/python/bin/python tools/oracle_round_end.py
cmake --build build --target round_end_tests round_end_native_tests -j2
ctest --test-dir build -R '^(original_round_end|native_round_end|native_team_match)$' --output-on-failure
.tools/python/bin/python analysis/multiplayer/game-variants/round-end/run_sanitized.py
```
