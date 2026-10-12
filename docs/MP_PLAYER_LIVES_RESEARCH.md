# Original limited-life fields and respawn admission

This document retains the original recovery and its earlier gated native
snapshot. [The subsequent one-round menu integration](MP_FINITE_LIFE_MENU_INTEGRATION.md)
now admits the life field; Elimination and complete spectator/multi-round
flows remain unfinished.

The shared native match now initializes, consumes and publishes the original
signed16 remaining-life field. Respawn admission and the full scoreboard use
that field. This supplies a common engine contract needed by finite-life
variants and edited settings; Elimination and finite-life menu settings remain
unavailable until their complete round flow is bound. No variant gains admission.

## Original owners and data contracts

| Owner | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Whole player initializer | `157BE0` | `159280` | `15A660` |
| Whole death owner | `15CE70` | `15E4D0` | `15F8B0` |
| Player exhausted | `15DB30` | `15F190` | `160570` |
| Respawn admission | `158A50` | `15A090` | `15B470` |
| Team exhausted | `15DB80` | `15F1E0` | `1605C0` |

Parameter16 is the original signed32 configured life limit at variant+7C,
or game-options+1B8. The remaining count is a signed16 field at player+1AC.
The initializer writes NONE=-1 for configured zero, otherwise the low16 bits
of the configured value. Engine absence and raw game mode4 skip the store.
The shared core preserves the source narrowing, including boundary inputs.
The native match API validates0..32767 configured lives; the normal Slayer
variant adapter still rejects nondefault parameter16.

The death owner decrements only with an engine present, game type2, raw mode
other than4, lifecycle1 and status1. A remaining count of NONE survives;
other counts decrement with a lower bound of zero. Configured zero does not
independently gate this store: the remaining NONE value owns unlimited lives.
The life store precedes base respawn timing and engine score callbacks, so
the death that ends an active match still consumes a life. The source dirty
entity notification following the store is a separate original transport owner.

A player is exhausted only when configured lives are positive, the current
unit is NONE and remaining lives equal zero. An alive current unit with a zero
count is not exhausted. Respawn admission requires flags bit0, clear bit1 and
membership other than NONE. A current unit passes those gates; without a
current unit, flag4000 and life exhaustion each prevent admission.

The team query requires an engine, team mode and a positive configured limit.
It visits occupied player slots through the original iterator, which excludes
flags bit1. Any matching member with a current unit or a nonzero remaining
count prevents exhaustion. With no qualifying member, the team is exhausted,
including an empty team. This query is recovered and independently checked.
The later native [ending check](MP_ROUND_END_RESEARCH.md) follows the original
distinct-membership collector and player admission query; it does not replace
that collector with this separate team-exhaustion query.

## Whole original execution

[The instruction oracle](../tools/oracle_player_lives.py) executes9,252 complete
owner calls across the retail, TU1.1 and TU1.5 executable images. Each image
executes128 player initializers,652 death owners,1,024 pairs of player queries
and256 team queries. Engine registration and every reached game routine use
their unchanged original instructions. There are no instruction patches or
supplied game, platform or service returns.

Occupied pools, flags, membership, current-unit values, configured limits,
signed16 counts, lifecycle/status and clock are declared API inputs. For
initialization/death, replicated entities, local users and current units are
NONE; death cause41 avoids a separate notification branch. Recording is
disabled. Query cases also supply present unit values. These inputs do not
establish live original player allocation, world combat or original transport.
The initializer/death comparisons check their life-field effect, not every
unrelated original store. All query cases also verify that the complete
16-player raw record bank is unchanged.

The fixture covers signed32 narrowing, signed16 boundaries, all recovered
active-state gates, flag/membership combinations, alive versus absent units
and occupied slots0/3/15. The native shared core matches every result. Fixture
SHA256:
`2e6252dff1a4aef1d61df695b6b2c805ff202ddab00c28ece27c029aac0f32c6`.
A second independent execution reproduces the fixture, complete report and
all39 reached-owner instruction listings byte for byte. The listings contain
3,746 static instruction rows, rather than3,746 individually executed branch
claims. Six complete query-owner pairs preserve214 instruction rows of
retail/TU1.1-to-TU1.5 correspondence, including literal relocated operands.

The separate whole-round initializer `15EFA0` also owns life initialization
at round start. Its retained static listing establishes that store. A
diagnostic execution stops at `15D77C`, reading unavailable scenario+120;
it does not establish a successful whole-round transition. An earlier player
initializer probe stopped on an absent replicated-entity dependency; declaring
the source NONE entity slots allowed the complete player initializer to return.
The successful canonical oracle uses those declared NONE inputs. The retained
diagnostic queries are exploratory and are not the comparison authority.

Subsequent [round-lifecycle research](MP_ROUND_LIFECYCLE_RESEARCH.md) closes that
missing-scenario initializer stop with real Lockout data. It retains the later
authored full-map-reset and live-object dependencies; native multi-round
operation remains unbound. The earlier stopped probe remains historical evidence.

## Native consumers and verification

Local and UDP joins initialize the field from match rules. The death boundary
consumes it exactly once before score callbacks, through both actual SMG
attribution and the explicitly bounded native fatal-input adapter. The two
respawn scans pass the original admission query to the existing timer owner.
Native protocol16 carries the signed16 remaining count and signed32 configured
limit in authoritative snapshots. These are native transport fields, not an
original Xbox packet claim. Full scoreboard inputs now use the actual count
and authoritative configured limit for exhaustion ranking, icon and dimming.

The native integration test supplies a diagnostic finite-life rule to the
engine API while retaining normal frontend admission gates. On actual Lockout
content, a two-life participant dies, genuinely respawns with one life,
dies again and remains absent for300 ticks without decrementing its gated
respawn countdown. An actual two-view localhost UDP peer verifies authoritative
counts and new-participant initialization. The original font-backed scoreboard
places the exhausted high-score participant below living participants and
applies the recovered icon/dimming behavior. The high score is a declared
presentation stimulus, not a live score producer.

FFA and Team Slayer cases also fire a real production SMG projectile to end
a one-score match with a one-life victim. The victim consumes its final active
life before game over; subsequent world updates and a postgame death neither
consume an inactive life nor respawn either participant. Those cases declare
positions, vitality and absent arena collision. They do not establish a natural
stock encounter. The separate fatal-input adapter checks the same ending-order
contract without claiming original damage attribution.

All33 rebuilt focused regressions pass, including respawn timing, attribution,
match/game end, selected profiles, native UDP, frontend and HUD/scoreboard
owners. ASan, UBSan and leak checks pass for the complete shared core and its
comparison harness. The native integration additionally instruments the life
core, match, network, scoreboard and test units; other linked cache, systems
and platform objects retain their ordinary production builds.

A fresh strict audit executes78 normal menu Start actions and admits2/39
variants, blocking37 (expected exit1). A freshly built production four-local
Team Slayer command advances300 ticks on Lockout with `finished=1`; these are
not300 active combat ticks or a finite-life frontend session. Exact commands,
source/binary pins and logs are retained in
`analysis/multiplayer/game-variants/player-lives-verification.json`.

This verification snapshot covered life fields and admission. Subsequent
[participant/life ending recovery](MP_ROUND_END_RESEARCH.md) now binds the
automatic single-round check; complete round transitions/reset and related
HUD messages remain unbound. General weapons, objective engines,
edited-rule flows, complete retained participants/Carnage Report and actual
session return still prevent completion of the all-variant goal. Source-field
comparisons and diagnostic native integration do not establish complete
finite-life matches, rendered end-to-end play, physical LAN or Xbox parity.
The goal remains active with its vanilla tag/data scope.

```sh
.tools/python/bin/python tools/oracle_player_lives.py
cmake --build build --target player_lives_tests player_lives_native_tests -j2
ctest --test-dir build -R '^(original_player_lives|native_player_lives)$' --output-on-failure
.tools/python/bin/python analysis/multiplayer/game-variants/player-lives/run_sanitized.py
```
