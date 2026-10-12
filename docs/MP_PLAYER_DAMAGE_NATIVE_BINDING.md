# Native shared player-damage binding

The native match now supplies the recovered shared player-damage consumer with
its actual variant fields, sanitized player strength, current target identity and
spawn countdown. Shots and authored area requests apply the result before the
shared shield/body/material kernels. This is a bounded combat milestone within
the active [39-variant goal](GAME_MODE_PROGRESS.md); King startup and objective
transport remain gated.

## Original contract and live inputs

[Whole original damage and trait owners](MP_KING_TRAIT_CONSUMERS_SOURCE_RESEARCH.md)
establish the literal operation order and constants. The new
[player-input research](MP_DAMAGE_PLAYER_INPUT_SOURCE_RESEARCH.md) establishes
birth, property sanitation, successful-spawn stores, countdown and target lookup.
Those executable owners, alongside the authored weapon/material tags, determine
balance. No built-in variant name selects a multiplier.

The native rules adapter retains the actual variant+48 word and King+F0 word.
The source field masks admit global damage/resistance and friendly fire, plus
King hill damage/resistance. Parameter aliases receive the same admission as
the bits they write: parameter30 now reaches the same King damage bit as41;
parameter103 reaches the same resistance bit as42. Camouflage remains gated.

Player+C1 comes from sanitized configuration+7D, including its signed-byte
clamp. A default join uses the recovered default configuration. Birth starts
player+194 at0; successful native spawn stores the30Hz tick rate, and the shared
pre-world player pass decrements positive values before combat. This field is
separate from shield recharge and respawn eligibility. King traits use the
signed retained inside timer: a false hill bit overrides a true global bit.
Slayer's registered trait callback uses the shared global branch.

The multiplier preserves the original early returns for absent engine, attacker
NONE and self identity. A nonplayer target consumes only attacker strength.
Other targets consume outgoing1.5, incoming0.5 and positive-field1940.1 in that
order, then the original friendly-fire bit and selected-engine membership
predicate. There is no additional team-mode gate in this damage owner. Each
operation retains a separate binary32 multiplication.

For example, a declared40-point request against a fresh100-health player becomes
4 while field194 is positive. The hill's damage bit changes it to6; its
resistance bit changes it to2; both produce3. These numbers illustrate the
consumer arithmetic with a synthetic request, not an authored weapon retune.

## Actual damage routes

`Simulation::damage` applies the multiplier after authored/event amount
construction and before `apply_unit_damage`. Its existing post-vitality
attribution hook remains a separate owner. Precomputed area amounts pass through
the same multiplication once. Stock direct SMG event interpolation retains its
source-produced event scalars and random-upper-bound gate.

For actual native biped targets, the original target resolver returns current
unit+13C. It ignores previous-player+140, including on detached corpses. Native
authored scene props are unparented nonunit targets and resolve to NONE; both
direct and area prop requests therefore retain attacker strength while skipping
player traits and target countdown. This does not establish the full parented
item/vehicle/control resolver or replace authored material/armor multipliers.

## Verification

The standalone normal arithmetic/query comparison passes all5,004 whole
unchanged original damage calls, including the actual query order, and also
passes with ASan/UBSan. The fixture contains independent inside/outside inputs,
strength bytes, signed countdown boundaries, identity gates and friendly-fire/
membership combinations across retail, TU1.1 and TU1.5.

The successful [verification manifest](../analysis/multiplayer/game-variants/king-damage-native-20261011/verification.json)
pins a completed frozen dependency build and a subsequent immutable five-owner
overlay. All seven focused checks pass: original arithmetic, native King,
Slayer, LAN startup, retail multiplayer, actual-map props and grenade delivery.
The King arena exercises actual native shots, source field admission/aliases,
sanitized strength, spawn countdown, hill/global precedence, same-team rejection
and authored area self damage. The scoped native ASan/UBSan run instruments the
five changed production owners and King test; remaining libraries retain their
ordinary build. The standalone arithmetic executable is fully instrumented.
Both sanitizer runs pass. The complete original consumer corpus was freshly
reproduced, and original asset and frozen-source hashes are checked throughout.
Live checkout drift is recorded separately.

The initial frozen build passed six of seven checks: its King equipment fixture
used NONE first-person model references, rejected by the newer original-backed
pickup gate. The successful overlay supplies declared model references for that
synthetic selector fixture. The original failure and immutable base remain in
the evidence directory. [Full verifier](../tools/verify_king_damage.py) and
[overlay verifier](../tools/verify_player_damage_overlay.py) record their distinct
scopes; this does not claim a full build of the concurrently changing checkout.

Captured attacker identity across leave/reuse is also verified against unchanged
original owners. Actual leave retains the raw membership, strength, countdown
and inside fields; allocation clears them before a later birth/engine callback.
The damage consumer projects the captured datum to its raw low16 slot, including
after release or reuse. Attribution retains its separate validated full-datum
lookup. Native area tests therefore read retained traits after leave and the
new occupant's strength after reuse, while a fatal stale attack gives the new
occupant no kill credit. Independent source reproduction covers486 whole amount
calls,24 complete leaves,54 caller prefixes and20 complete owner comparisons;
its exact callback/prefix limits are in the
[player-input research](MP_DAMAGE_PLAYER_INPUT_SOURCE_RESEARCH.md).

Complete
camouflage timers/fades/disruption/rendering, enabled objective recording,
hill/HUD consumers, objective replication and normal complete King matches
remain required.
