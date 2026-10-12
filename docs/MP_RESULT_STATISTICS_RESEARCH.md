# Original retained result-stat writers

This adds a shared original stat store and binds verified admitted-match
producers to it. It does not make the Postgame Carnage Report operational.
Retained participant properties, identity remapping, the other stat producers,
report construction, report navigation and frontend/session return remain open.

## Original owners and data

The unchanged images establish these complete owners:

| Owner | Retail | TU1.1 | U5 |
| --- | --- | --- | --- |
| Set retained general stat | `1968B0` | `1983B0` | `19B2B0` |
| Add retained general stat | `1967D0` | `1982D0` | `19B1D0` |
| Add player match stat and publish | `1E9CE0` | `1ED6A0` | `1F1FC0` |
| Add team match stat and publish | `1E9DF0` | `1ED7B0` | `1F20D0` |
| General-stat descriptors | `46DDC8` | `4752B0` | `47A388` |
| Recording/frozen raw bytes | `510CA0/+1` | `518490/+1` | `51DFA0/+1` |
| Contiguous retained stat store | `511BF4` | `5193E4` | `51EEF4` |

`OriginalData::load_xbe` imports all45 actual16-byte descriptors from the
selected supported image. Each descriptor starts with unsigned16-bit minimum
and maximum. Its remaining12 bytes are retained exactly, including source
pointers; this implementation assigns them no additional runtime semantics.
The bounds are not named-mode or named-weapon replacements.

The raw retained store is exactly`4040` bytes:

| Offset | Original records |
| --- | --- |
| `0000` | 16 player records, each437 unsigned16-bit words (`36A` bytes) |
| `36A0` | 16×16 PvP pairs, each two unsigned16-bit words |
| `3AA0` | 16 team records, each45 unsigned16-bit words (`5A` bytes) |

Only the first45 words of each player record are used by these general-stat
writers. Remaining player and PvP bytes stay raw. No weapon, medal or other
unrecovered producer is fabricated. This view omits the separate retained
player/team properties and source valid/complete flags; having the raw bank
does not imply that original`19B3A0` getters or a PCR may consume it.

Recording requires a nonzero enabled byte and zero frozen byte. A destination
index of`-1` is NONE. Both player and team may be selected. Set clamps signed32
input to the imported unsigned16 bounds. Add first masks the old word with
`7FFF`, adds with32-bit wrap, then interprets the result as signed32 for the
same clamp. Writes preserve bit15 and replace only the low15 bits. The
decompiled setter's temporary-variable reconstruction is misleading at its
lower bound; the literal original instructions and executed outputs establish
the minimum branch for both destinations.

The separate match bank uses signed16 counters clamped to±30000. A player
write publishes its resulting value to the retained player stat, then may
increment/publish the matching team field through the original team writer.
That optional team destination requires the caller's actual selected team
engine, occupied live player slot, high-water bound and non-NONE membership.
Replication entity is NONE in this oracle; no successful replication service
or Xbox wire codec is supplied. Native adapters validate their known live
membership before providing a team destination. Out-of-view indices are
rejected before mutation, rather than extrapolated into unknown records.

## Native producers and end ordering

The actual original call operands, including`1F2890`, `1F2A50`, `1F2280`
and`15DDC0`, establish these mappings:

| Verified producer | Match field | Retained general stat |
| --- | --- | --- |
| Credited kill | 2 | 7 |
| Victim death | 3 | 9 |
| Suicide | 4 | 11 |
| Assist | 6 | 8 |
| Win award | 7 | 6 |

`MultiplayerMatch` now uses the shared writer for these admitted callbacks and
mirrors its match-field outputs in the existing player state/UDP counters.
The existing non-SMG fatal-input adapter also uses the same counter writer;
its last-damager attribution remains a declared native boundary. Suicide
inputs now publish their own original counter as well as the victim death.
The round-end win owner publishes team and matching member wins, preserving
the existing flag2 exclusion and FFA full-datum winner validation.

With imported descriptors available, the native selected multiplayer engine
enables the raw recording view, following the recording flags in`19A430`.
It does not claim to execute that owner's full player-property capture or
controller/platform services. Synthetic content lacking original data leaves
retained recording unavailable instead of supplying guessed descriptors.

Original`15DDC0` makes scoring inactive synchronously. `15EFA0` freezes
retained recording at lifecycle2 later in the world pass. The native
finalizer follows that ordering: the winning lethal hit's subsequent assist,
victim-death and suicide writes can still publish before recording freezes.
Postgame attribution/stat gates prevent new counter publication while the
world, ownership cleanup and existing countdown continue.

This bank remains indexed by the native live-slot adapter. Original`19AE70`
identity-based retained-slot remapping, retained names/ranks/scores,
additional weapon/PvP/medal/event capture and final`19ACB0` row construction
are not bound. Later [result-capture work](MP_RESULT_CAPTURE_RESEARCH.md) binds
verified SMG weapon counters, running/best spree, multikills and lifetime;
this document's verification manifest records the earlier general-writer
snapshot. The bank is not serialized over native UDP. Existing replicated
player counters are checked; a complete peer-side report remains unavailable.

## Verification

`tools/oracle_result_statistics.py` executes actual engine registration and
all four complete writers in each unchanged image, with explicitly declared
recording bytes, banks, options and occupied-pool records. It also executes
the actual U5`15DDC0` round-end owner with recording enabled, including its
inlined team-member win publication. No original instruction or game/platform
return is replaced. Nonzero poison covers every unclassified bank word.

The fixture contains10,996 independently executed cases: 9,006 retained
set/add cases, 1,920 whole player/team match-writer cases and70 recorded
round-end awards. Every native comparison checks the entire retained bank,
not just target counters, and every whole match-writer/award comparison checks
all`254` match-bank bytes. Eight complete cross-image instruction pairs cover
510 paired rows;12 saved listings contain765 instruction rows. Incoming and
outgoing known writer calls are constrained to the actual matched owners.
Six additional complete U5 caller listings contain1,245 rows and preserve the
actual initialization, freeze and combat/win call operands as static evidence.
An independent second execution reproduces the same fixture and listings.

The three-image import test checks all45 descriptors' bounds and every
unclassified tail byte. The standalone stat/end implementation passes the
complete fixture under ASan, UBSan and leak detection.
The native ending test also passes those sanitizers with the changed
multiplayer, retained-stat and end-stat sources instrumented; other linked
cache/systems/sound archive objects remain uninstrumented.

The expanded native ending test checks six actual local/loopback-UDP FFA/team
score/time endings and225 postgame updates per case. It compares frozen
retained counters against actual match counters/wins and verifies that the
existing four postgame production-SMG fatal hits change no retained bytes.
Two additional winning production-SMG/UDP timelines reach real attribution,
score-limit completion and the victim-death publication before freezing.
Projectile placement/vitality and absent arena collision are declared inputs;
this is not an authored full-match playthrough or physical LAN proof.

Sixteen rebuilt focused regressions pass. A fresh strict catalogue audit
still executes78 normal Split Screen/System Link starts and admits2/39 rows,
leaving37 blocked (expected exit1). The rebuilt production Team Slayer
command completes300 ticks with four explicit local participants on actual
Lockout content. No new game mode, weapon or edited rule is admitted.

Reproduce the core evidence:

```sh
.tools/python/bin/python tools/oracle_result_statistics.py
cmake --build build --target result_statistics_tests result_statistics_data_tests game_end_tests game_end_native_tests -j2
ctest --test-dir build -R '^(original_result_statistics|original_result_statistics_import|original_game_end|native_game_end)$' --output-on-failure
```

The runtime goal remains incomplete. Authored PCR panes and navigation must
consume complete original retained properties and producer records before
the report and session return can be declared operational.
