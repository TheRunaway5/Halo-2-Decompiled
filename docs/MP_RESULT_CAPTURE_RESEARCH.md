# Original result capture: weapons, spree and lifetime

The admitted FFA/Team Slayer match now binds recovered weapon counters,
running streaks, multikills, retained best spree and lifetime sampling. These
extend the shared original result-stat bank; the Postgame Carnage Report and
the complete source session/frontend lifecycle remain unavailable.

## Source owners and immutable data

| Owner or data | Retail | TU1.1 | U5 |
| --- | --- | --- | --- |
| Add retained weapon stat | `196E60` | `198960` | `19B860` |
| Reporting cause to weapon index | `196EF0` | `1989F0` | `19B8F0` |
| Credited kill | `1EA5B0` | `1EDF70` | `1F2890` |
| Victim death | `1EA770` | `1EE130` | `1F2A50` |
| Player phase containing lifetime | `15BB20` | `15D160` | `15E540` |
| Active-engine predicate | `15B2F0` | `15C930` | `15DD10` |
| Last-betrayer metadata | `162BF0` | `164290` | `165670` |
| Five-second unit replication request | absent | `A8810` | `B1C60` |
| Lethal attribution/cutoff owner | `1E9FA0` | `1ED960` | `1F2280` |
| Seven weapon-stat descriptors | `46E098` | `475580` | `47A658` |
| Compiled 41-case cause table | `196FF8` | `198AF8` | `19B9F8` |
| Multikill span operand | `45DC10` | `465080` | `46A120` |

`OriginalData::load_xbe` imports the selected executable's seven complete
16-byte descriptors, compiled cause dispatch and binary32 multikill operand.
Each descriptor preserves its unsigned16 minimum/maximum and twelve raw tail
bytes. The original dispatch leaves return a literal index; malformed leaves
are rejected. The mapper masks cause with`3F`, maps inputs0..40 through the
actual table and returns original index0 for41..63. All256 byte inputs are
independently executed. No weapon-name or game-variant exceptions provide
these values.

The retained weapon word is at player-word`78 + weapon*8 + kind`. The seven
recovered kinds use their own imported bounds; the eighth word and remaining
unclassified fields stay raw. Add masks the old low15 bits, wraps signed32
addition, clamps to the descriptor bounds and preserves bit15. Recording must
be enabled and unfrozen. Tested native destinations cover sixteen players,
seven kinds and the original mapper's indices0..40.

## Kill, death and clock contracts

The credited-kill owner adds match field2/retained general7, increments weapon
kind0 and, only for`cause & C0 == 40`, weapon kind6. Running spree is match
field9: an original signed16 wrapping increment, separate from the bounded
generic counter writer. A signed comparison against the retained general12
best value publishes a new maximum to the player and, when the selected team
engine and membership permit it, the team. An enabled but frozen recording
still permits the owner's read; the writer itself rejects publication.

Match field10 is the running multikill count. It becomes1 when the previous
signed kill tick is strictly less than the supplied signed cutoff; otherwise
it increments with16-bit wrap. The current world tick replaces the last-kill
word. The original lethal attribution prefix computes its cutoff using the
imported **4.0-second** operand, signed16 clock rate, binary32 multiplication,
x87 nearest conversion to signed32, and wrapping subtraction from the old
world tick. This is distinct from the6.0-second assist/history cutoff.

The victim-death owner adds field3/general9 and weapon kind1. Incoming-self
also adds field4/general11 and weapon kind3. Otherwise a betrayal with a
selected credited player adds that player's field5/general10 and weapon
kind2. Victim and credited team destinations are independent. Death clears
the victim's running spree/multikill fields and sets last-kill tick to NONE.
Last-betrayer metadata, local betrayal UI and entity replication are separate
services; the new native counter module does not claim to bind them.
The credited owner's original engine flag bit20 and per-player replication
notifications likewise remain outside this counter-bank implementation.

Lifetime is retained general13. The original player phase requires raw game
mode other than4, current unit other than NONE, a present selected engine,
lifecycle1 and status1. It adds one whenever the signed world tick divided by
the signed16 clock rate has remainder zero, including tick0. No team lifetime
is published by this branch. Active zero-rate or INT_MIN/-1 divides fault in
the original; native input validation reports those unavailable inputs rather
than inventing a count.

Static complete U5 listings establish the order: world`13AAC0` calls
`15EBA0`; that owner first runs clock/time-expiry`15E870`, then iterates player
`15E540`. Object updates, finalizer`15F450` and the later world-clock increment
follow. Native lifetime sampling therefore uses current ownership and active
status after time expiry, before object updates and the clock increment.
Timed expiry at tick30 suppresses that tick's sample. Death detachment stops
later samples, and an actual respawn binding allows them again.

Retail's complete player-phase shape differs from TU1.1/U5. It omits the later
five-second replication request and has additional metadata/HUD differences.
The three independently executed phases establish the tested shared stat13
branch; this does not claim that all their other services are equivalent.

## Native binding and boundaries

Production SMG attribution supplies its recovered reporting cause to these
shared producers. Its score callback and later victim-stat write retain the
existing end-of-world freeze order. The fatal-input adapter has no recovered
cause producer, so it explicitly leaves weapon records untouched while
publishing known general/streak counters. It does not substitute SMG or
unknown-weapon index0 for that missing cause.

The native bank still uses live-slot indices. Original retained identities,
remapping, names/ranks/scores and valid/complete publication remain open.
General headshot/damage reporting, other weapons, PvP, medal and event
producers are not established by an all-cause mapper comparison. The bank
itself is not transported over UDP; existing replicated combat counters are
checked. PCR panes, their navigation, and the original session callback chain
must consume complete retained records before the report is operational.

## Verification

`tools/oracle_result_capture.py` independently executes5,955 whole original
cause/weapon/kill/death/player-phase calls and108 original lethal-owner
prefixes across retail, TU1.1 and U5:6,063 cases total. Every native stat
comparison checks the complete`4040`-byte retained bank, poisoned in every
word. Whole kill/death/player-phase cases also check all`254` match-bank bytes.
The cutoff prefix stops after the original cutoff store, before unrecovered
history/medal/event dependencies. It executes the original NONE-object lookup
return; no instruction or service return is replaced.

Fixtures explicitly supply recording flags, banks, occupied pool records,
clock/game/engine inputs, NONE replication entities/local users and NONE unit
parent/seat. First-binding notification is not requested. Every reached game
routine executes. These inputs do not establish original platform services or
a complete original world/combat/session tick.

Fourteen complete cross-image pairs cover1,027 paired instruction rows, with
one declared retained-base ADD-immediate relocation in each credited-kill
pair. The27 saved listings contain2,790 rows, including four complete static
ordering/cutoff callers. An independent second run reproduces the entire
report, fixture and all27 listings. Fixture SHA256:
`e17f5eed723730583e0a9b50318303017b216169939949306ce21e15d53f7eeb`.

The native test extends two real SMG/loopback-UDP winning timelines with weapon,
best-spree and lifetime checks. Two additional ongoing FFA/team timelines use
actual projectile kills, the original multikill window, victim resets and stock
respawn binding to check continued capture. A normal release tick separates
fire pulses through the recovered SMG barrel recovery. The six existing ending
cases continue225 postgame updates each; retained records remain frozen.
Controlled body positions/vitality and disabled arena collision are declared
inputs. These are neither authored full-match playthroughs nor physical LAN
validation.

Eighteen rebuilt focused regressions pass, together with three-image imports,
ASan/UBSan/leak checks on the complete capture/stat core and on the native
ending slice. The native sanitizer instruments multiplayer/capture/stat/end
sources; remaining linked cache/systems/sound archive objects are not
instrumented. The rebuilt production Team Slayer command advances300 ticks
with four local participants on actual Lockout content.

Current pins and logs are in
`analysis/multiplayer/game-variants/result-capture-verification.json`. The fresh
strict audit runs78 normal menu starts and still admits2/39 authored variants,
with37 blocked and expected exit1. No mode, weapon or edited rule gained
admission. The operational goal remains active and incomplete.

```sh
.tools/python/bin/python tools/oracle_result_capture.py
cmake --build build --target result_capture_tests result_capture_data_tests game_end_native_tests -j2
ctest --test-dir build -R '^(original_result_capture|original_result_capture_import|native_game_end)$' --output-on-failure
```
