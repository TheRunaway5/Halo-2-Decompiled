# Original TU1.1 ordinary FFA Slayer accounting and endings

The original ordinary Slayer callback, score writer, ranking and score/time-end
routines are executed against explicit salted player pools. This provides a
bounded arithmetic/ranking contract for integration. Damage attribution is a
separate executed slice with the marker/death-event writer explicitly supplied;
it is not a complete original live kill, damage, respawn or network runtime.
Production code was not edited by this research.

## Reproduction and authority

```sh
.tools/python/bin/python tools/oracle_mp_spawn_slayer_accounting.py
```

The [oracle](../tools/oracle_mp_spawn_slayer_accounting.py) and its ignored
[report](../analysis/multiplayer/integration/spawn-source/slayer-accounting-report.json)
are frozen at these SHA-256 values:

| Artifact | SHA-256 |
| --- | --- |
| tool | `462a23a83a8a3525ae958d615c35ea19064c56509119c858a2d9b5cdd1ae7563` |
| report | `284cbcd6bb293bb8e9edc74e62dc9ffe3b3a2929877983884abc93d05b66f128` |
| instruction listing | `a6ae11dc395a361eb7662976c5f9e939777f630f7fd2a867a826bc0b43b1cf9d` |
| original TU1.1 XBE | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| original Lockout cache | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |

The byte/instruction authority is the unmodified TU image at
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`.
[tu11-slayer-accounting-instructions.txt](../analysis/multiplayer/integration/spawn-source/tu11-slayer-accounting-instructions.txt)
contains original bytes and disassembly for the addresses below. The report
records every dependency hash. Readable supporting source is
[`unknown_157450.cpp`](../tools/reference/halo2-decompiled/src/unknown_157450.cpp)
at base `15B7C0`/`15B3A0`, and the TU Ghidra files for
[`15CE00`](../analysis/map-pack-update/functions/0015ce00.c),
[`15C9E0`](../analysis/map-pack-update/functions/0015c9e0.c),
[`15D490`](../analysis/map-pack-update/functions/0015d490.c),
[`15C970`](../analysis/map-pack-update/functions/0015c970.c),
[`241500`](../analysis/map-pack-update/functions/00241500.c) and
[`1ED960`](../analysis/map-pack-update/functions/001ed960.c).
Instruction operands own the contract when inferred decompiler types differ.

The report has 30 complete Slayer callbacks, 13 complete score-writer cases,
nine complete ranking/winner cases, 18 ending cases, 14 bounded attribution
cases and four complete damage-history-writer cases. None patches executable
bytes. Earlier spawn/ownership/timing/queue/rounding/placement/aftermath suites
are preserved.

## Narrow integration contract: score arithmetic

The actual TU Slayer object is `487188`, its vtable is `463B48`, and slot 30
at `+78` points to original death callback `2C2350` (base `2BC000`). Its stack
arguments are full attacker datum, full victim datum, relationship flag and
cause class. The victim must be a valid player supplied by the death-event
producer. Attacker NONE is supported. Player membership/designator byte `+C0`
is distinct from native combat-team identity; the fixtures assign 0/3/15 to
their matching absolute slots with full datums `00070000`, `00160003`,
`0052000F`. Source ordinary FFA is team-mode bit zero at options `+184`.

For ordinary flags `options+22C = 2`, mode byte `options+0C != 4`, and a victim
without flag bit one at player `+2`, source `2C2350` makes these score requests:

| Source input | Requested score delta |
| --- | --- |
| credited enemy, relationship flag false | attacker +1 |
| credited attacker equals victim, relationship flag true | attacker -1 |
| attacker NONE | no score request |
| victim player flag bit one set | no attacker request |

The callback checks the relationship flag, not native `Unit.alive` or a native
last-damager slot. In its general branch, a true relationship flag with distinct
attacker/victim requests attacker -1. The source ordinary FFA producer uses
different membership bytes for distinct players; that general input is an
explicit callback fixture, not a claim that ordinary FFA has allied players.

The complete callback also executes option-mask cases 0/1/3/4/6. Bit one enables
the same-player suicide penalty; bit two independently requests victim -1 for
death. With bits one and two both set, suicide makes two -1 requests to the
same player. Bit zero has a separate bonus branch involving cause class and
placement, which the tested cause-class NONE fixtures reach with a zero
additional request. These cases establish source option operands; this report
does not claim every bonus-rule/cause-class combination is covered. Ordinary
flags two avoid that bonus branch.

Original writer `15CE00` takes delta in EAX and full player datum at stack
`+4`. Its required source gates are engine object present, signed short
global `+6C == 1`, and mode four or signed global `+C04 == 1`; player NONE
returns false. It does not independently validate the salt: its caller must
already have a valid datum. Source storage is signed short at:

```text
engine_globals + 0x308 + absolute_slot * 0x1C
```

The write is exactly:

```text
stored_delta = signed_i16(low_16_bits(original_signed_i32_delta))
score = signed_i16(clamp(int32(old_signed_i16_score) + stored_delta,
                        -30000, 30000))
```

The original writer returns true and sets global flags bit `0x20` even for a
zero effective delta. Failed gates return false without modifying the tested
engine state. Replicated engine entity `+28` is NONE in the fixtures, so no
transport/dirty-entity behavior is supplied or established. FFA does not call
the team-score path `15CC90`.

| Old score | Original delta | Stored signed delta | New score |
| --- | --- | --- | --- |
| 29999 | 10 | 10 | 30000 |
| -29999 | -10 | -10 | -30000 |
| 0 | 65535 | -1 | -1 |
| 0 | 65536 | 0 | 0 |
| 0 | 32768 | -32768 | -30000 |
| 0 | -32769 | 32767 | 30000 |

All callback, writer and ranking cases leave both global RNG words unchanged.
Scoring and tie resolution in this slice do not draw a random winner.

## Score-limit and time-limit outcomes

After storing FFA score, `15CE00` checks **the original signed 32-bit delta**
for `> 0`. If that is true, score limit at options `+18C` is nonzero, and the
new stored signed score is at least that limit, it invokes `15C9E0` with the
modifying full player datum in EBX and force byte zero. It does not rank players
to choose this score-limit winner.

With round-limit kind zero at options `+188` and round index zero at engine
`+6E`, `15C9E0` gives a valid winner one round win, then writes engine
`+C04=2` and flags bit `0x20`. The external session-completion call `13A130`
is supplied as a named no-op **after** the original state write. This is the
ordinary one-round ending boundary, not full results-screen/session teardown.
Other round-limit kinds, team scoring and future round initialization remain
outside the narrow port contract.

The report executes reaching/crossing 25, staying below it, a negative delta
that produces score 25, and unlimited score zero. It also proves why the
original-delta test must precede any narrowing assumption:

| Old score / delta / limit | Source outcome |
| --- | --- |
| 25 / 65536 / 25 | score remains 25; ending triggers |
| 26 / 65535 / 25 | narrowed delta -1 produces 25; ending triggers |
| 24 / -65535 / 25 | narrowed delta +1 produces 25; ending does not trigger |

An explicitly active-state fixture has another player already at 25 before a
positive update makes the updater's score 25. Source awards the updater the
round win even though the final leading scores are equal. This fixture does
not claim an earlier ordinary score event could leave that state active.
Two executable ordered-callback cases start both competitors at 24. The first
kill callback reaches 25 and ends; the second callback still reaches the source
writer, but its `+C04` gate rejects the score change. Reversing callback order
reverses the winner. Neither case claims simultaneous-event ordering outside
the declared input order.

Time limit uses seconds at options `+190`. Original `15D490` calls `163B10`,
whose Slayer virtual timer callback retains its input:

```text
elapsed = max(signed_i32(game_tick - round_start_tick), 0)
remaining = max(signed_i32(signed_rate * time_limit_seconds - elapsed), 0)
```

These are executed ordinary non-overflow fixtures. The underlying subtraction
and multiplication remain original 32-bit instructions; this prose does not
replace their wrap behavior with unbounded integers. The timer display converts
single-precision `remaining * seconds_per_tick` to an integer using truncation.
At 30 Hz, the supplied time state is rate 30 and seconds-per-tick float bits
`3D088889`. The updater asks for a winner and ending once remaining is less
than one. It does not wait for the displayed seconds to transition below zero.

| Current tick, round start zero, five-second limit | Display | Source `+C04` |
| --- | --- | --- |
| 0 | 5 | 1 |
| 119 | 1 | 1 |
| 120 | 1 | 1 |
| 149 | 0 | 1 |
| 150 | 0 | 2 |
| 151 | 0 | 2; no second end request |

For unlimited time zero, the source displays elapsed seconds capped at signed
short maximum 32767 and does not trigger a time ending. Negative elapsed is
clamped to zero. The report executes both limits and the clamp boundary.

Original caller scheduling is visible in the preserved
[TU timing listing](../analysis/multiplayer/integration/spawn-source/tu11-timing-instructions.txt):
`1397AC` calls engine update `15D7C0`; that routine calls `15D490` first.
The object/world passes follow at `1397C0` (`B7E00`), `1397C5` (`1C7660`),
`1397CA` (`B7F50`) and `1397CF` (`B8040`). The game tick increment is later
at `139868`. Thus the inspected source time-ending check precedes those world
passes on the current tick. This research does not execute every intervening
world subsystem or declare the order of all live damage events.

## Ranking and ties

Original `15C970` asks `241500` for ranking mode zero with deterministic
tie-break flag false. Ranking collects actual active pool slots with their
full salted datums, executes original sorting/comparison and assigns each
player a rank code. The default mode-zero metric bytes at `460E68` are:

```text
00 02 03 0A 0D 12 05 FF
```

The original comparators `241880`/`241AA0` inspect eligibility/lives/membership
before the score metric. Under the ordinary fixture's unlimited lives, valid
membership and equal player eligibility, the final FFA comparison is signed
score. The score occupies statistic field zero; kills and deaths do not resolve
a mode-zero FFA tie. General finite-lives, departed/suppressed players and team
ranking require their source eligibility state; a native loop should not treat
all such states as automatically equivalent.

`15C970` returns the first ranked full player datum only if its rank code is
zero. A tied top has rank code one and returns NONE. It does not resolve equal
scores with absolute slot, first scan order, kills, deaths or RNG. Executed
cases include unique/equal/all-zero/negative scores, equal score with differing
kills or deaths, and a high-score player explicitly suppressed by flag bit
one or membership NONE. In those suppression cases, source selects the next
eligible highest score.

At five-second expiry, a unique leader returns its full datum and receives one
round win. Equal leading scores return NONE, yet the one-round ending still
writes `+C04=2`. No player receives a round win. The source treats this as an
ending with no winner, not a random winner or a first-slot tie-break. Results
presentation of the tie remains outside this report.

## Separate damage attribution and counters

The source attribution boundary is [`1ED960`](../analysis/map-pack-update/functions/001ed960.c),
fed by the damage consumer before aftermath `CB410`. Source history producer
[`CBA60`](../analysis/map-pack-update/functions/000cba60.c) is later called by
`CB410` at `CB777`. Its damage amount in XMM0 is the original single-precision
sum of damage-result floats `+48` then `+44`; the sum is formed at
`CB436/CB43B` and saved at `CB479`. Its attacker designator comes from the
damage result's signed short `+14`, separately from full attacker datum `+0C`.
That designator must be preserved; deriving it from a native combat-team field
changes the source enemy/credit gate.

The oracle's history cases execute complete `CBA60` nondeath history recording
with explicit amounts, datums and ticks. Their input is not a newly simulated
projectile or a complete `CB410` feedback path. Each of four entries is:

| Unit offset | Original field |
| --- | --- |
| `+2F0 + i*10` | 32-bit tick; empty timestamp is NONE |
| `+2F4 + i*10` | single-precision accumulated damage |
| `+2F8 + i*10` | full source object datum |
| `+2FC + i*10` | full attacker player datum |
| `+330 + i` | packed cause byte |

Offsets in the table are hexadecimal. A hit matching a non-NONE player or a
non-NONE source object adds single-precision damage, refreshes tick and cause,
and keeps the entry's original player/object identities. The same-object case
with a different player proves this ownership retention. Otherwise the writer
takes the first empty timestamp. If all four entries are occupied, it protects
the strict maximum-damage entry (first on equal maxima), then replaces the
oldest timestamp among the other three. The full-table cases retain attacker
zero and replace the oldest other entry with attacker 15. No expiration timer
or fabricated aging service is supplied.

`1ED960` validates full salted datums against the original player pool. It uses
membership/designator relationships through `1E2F50` and Slayer slot 27
`253F10`, whose exact relation is designator inequality. For the recent-history
candidate branch, timestamp must be **strictly greater**, with an unsigned
comparison, than `game_tick - nearest(6.0 * rate)`. Candidate damage must be
strictly greater than the current maximum; equal candidates keep the first.
Direct attacker/designator inputs control whether other enemy entries can
override the input or only matching entries can be selected. A matching direct
entry can remain a fallback outside the recent window; a valid direct datum is
also a final fallback. Do not substitute a generic latest-damager slot.

The full attribution/counter cases supply `15E4D0` as a named marker/countdown,
death-event and score-writer boundary, recording its actual source arguments.
This avoids inventing a head pose or a live event interface. After that
boundary, original attribution executes the original death-stat helper. The
actual Slayer callback is then separately executed with the recorded credited
player/victim/relationship arguments and cause-class NONE. It is an explicit
composition of two bounded source slices; intermediate medals/kill-feed,
marker capture, timer changes and loadout do not execute in this report.

| Fixture, current tick 200 / 30 Hz | Credited player / score |
| --- | --- |
| direct valid enemy | attacker +1 |
| environment, no history | NONE; victim death +1, no score penalty |
| environment, enemy history at tick 199 | that enemy +1 |
| environment, history at tick 20 (exact cutoff) | NONE |
| environment, history at tick 21 | enemy +1 |
| environment, older 0.8 vs latest 0.1 damage | older larger-damage enemy +1 |
| environment, equal recent amounts | first entry credited; other qualifying entry receives an assist |
| direct valid enemy plus larger other enemy history | direct enemy retained |
| direct self with no enemy history | victim death/suicide +1; score -1 |
| direct self/designator self with recent enemy history | enemy credited +1; victim death/suicide counters still increment |
| environment, only self history | NONE |
| stale player salt in direct input or history | NONE |

The last mixed self/enemy case matters: final suicide counting compares the
original input player with the victim, while the score writer receives the
source-selected credited enemy. These are distinct outputs. The no-op
`15E4D0` boundary means this experiment does not independently prove that
case's complete marker/respawn-penalty/event behavior. Existing timer evidence
remains the authoritative bounded writer contract.

At explicit current clock two, recent-history tick one fails the source unsigned
cutoff comparison after subtraction wraps. This is a declared clock fixture,
not a claim that an ordinary loaded game's absolute clock starts there.

The source statistic block is engine `+304`, valid byte at zero, and player
records begin at `+4` with stride `1C`. The executed helpers identify these
signed-short record fields:

| Field index | Meaning in executed source calls |
| --- | --- |
| 0 | Slayer score |
| 2 | credited kills |
| 3 | deaths |
| 4 | suicide counter |
| 5 | betrayal counter |
| 6 | qualifying assists |
| 7 | round wins |

Kill/death helpers use the same ±30000 clamp for their generic counter stores.
Streak/multikill fields and player `+1C4` changes are recorded without assigning
them new native semantics. Persistent profile recording is explicitly disabled
at `518490`, replicated engine entity is NONE, and local user is NONE. These
fixtures do not prove platform/profile/medal/network delivery, damage-history
unit initialization/reset on a real successful spawn, all cause-class options,
team modes or complete death-event idempotence.

For a narrow ordinary FFA port, the proved separable slice is the source
callback requests, signed score arithmetic/gates, mode-zero eligible-player
ranking, one-round score/time ending and winner NONE on ties. Full attribution
should remain an explicit boundary until the native damage-result inputs and
four-entry history producer are mapped to this source contract.
