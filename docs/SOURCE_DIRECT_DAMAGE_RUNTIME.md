# Original direct damage and native composition

2026-10-12. Fifteen complete TU1.1/TU1.5 bodies compile 1,028 authenticated
original instructions. These include the direct dispatcher `D9180`, amount
`DB160`, target/player/actor resolution, campaign difficulty and relationship
queries, and the originally registered Slayer/King damage-strength callbacks.
Every instruction, operand width/register, translated address, internal branch
and jump table is checked against both unchanged executables.
[Interface](../include/halo2/source_direct_damage_runtime.hpp),
[compiled implementation](../src/source_direct_damage_runtime_generated.cpp),
[generator](../tools/generate_source_direct_damage_runtime.py),
[authorities and manifest](../analysis/damage-shields-goal-20261011/direct-damage-runtime/generated/manifest.json).

## Native wiring

The shared [damage composition](../src/source_damage_runtime.cpp) now selects the
[90-entry delivery module](SOURCE_DAMAGE_DELIVERY_RUNTIME.md) over the caller's existing memory,
stack and CPU state. Owned lower calls run through that same composition;
unimplemented calls preserve their actual pending frame and produced state.
GP, SSE, defined flags, x87 values/control, FS and MMX state cross the boundary.

Both the [native multiplayer runtime](../src/multiplayer_object_runtime.cpp)
and [object/world phase dispatcher](../src/death_object_phases.cpp) select this
composition. The ordinary object-update `BD360` now sends its original
`BD417` call to the independently paired vitality implementation. The older
internal `D73E0/DA150` copies were removed from the object-phase generator and
entry table. All 234 other retained complete bodies are byte-identical; only
`BD360`'s dispatch choice changes.
[Generator](../tools/generate_death_object_phases_1_1.py),
[consolidation audit](../analysis/damage-shields-goal-20261011/direct-damage-runtime/consolidation/body-audit.json).

This connects completed systems at their original source-runtime callers.
The ordinary `Simulation::damage` and scalar recharge path still use separate
host unit storage. Connecting that path requires actual source object, physics,
delivery and effect ownership; a mirrored scalar or supplied successful effect
would not establish that integration.
[Host simulation](../src/simulation.cpp),
[remaining coverage](DAMAGE_SHIELD_COVERAGE_GOAL.md).

## Original comparisons

The measurement executes both unchanged original images with all 96 actual
Lockout damage definitions and the Spartan/Elite model/global metadata. Raw
object/header/player/actor/options/RNG banks are explicitly declared diagnostic
inputs. The actual original constructor installs the engine registrations;
no indirect callback or external return is supplied. Controlled metadata
overlays are recorded, including the random upper range: all 96 authored
Lockout upper-bound pairs are equal and finite.
[Measurement](../tools/measure_source_direct_damage_runtime.py),
[original report](../analysis/damage-shields-goal-20261011/direct-damage-runtime/original-v7/report.json),
[lower-owner source evidence](DAMAGE_AMOUNT_RUNTIME_HANDOFF.md).

Each image has 1,447 cases: 991 complete and 456 stop at the first unimplemented
owner. Each executes 182,300 original instructions and 30,933 ordered stores,
covering 790 of 1,028 compiled instructions. The comparisons check instruction
traces, exact stores and untouched retained bytes, full GP/SSE, defined flags,
FP controls, active x87 values/TOP and actual pending call frames. FP exception
status and original hardware behavior are not established by these comparisons.
[Per-body coverage](../analysis/damage-shields-goal-20261011/direct-damage-runtime/coverage.json),
[native comparison](../tests/source_direct_damage_runtime_tests.cpp).

In the retained 26-entry composition snapshot, native comparisons passed for
both images. The same TU1.1 cases also passed
through the native object/world composition, as do the existing 3,584 vitality
cases. Additional missing-owner runs reject the absent service with the same
produced prefix. The 32-phase random-amount trajectory carries the actual RNG
word forward between calls, comparing native previous output with the next
original input. Its 6..10 upper range is a declared overlay.
[TU1.1](../analysis/damage-shields-goal-20261011/direct-damage-runtime/native-disc-v7.log),
[TU1.5](../analysis/damage-shields-goal-20261011/direct-damage-runtime/native-u5-v7.log),
[composed damage](../analysis/damage-shields-goal-20261011/direct-damage-runtime/composition-v7.log),
[composed vitality](../analysis/damage-shields-goal-20261011/direct-damage-runtime/vitality-composition-consolidated.log),
[RNG continuity audit](../analysis/damage-shields-goal-20261011/direct-damage-runtime/rng-trajectory-audit.json).

All 34 selected combined checks passed for that retained snapshot, including isolated and composed damage,
vitality, falling producers, source-process startup and the four original
object-phase modes. The latter retain 52 original `BD417` vitality calls across
the captured cycle. The native executable builds and its CLI help check passes.
Generation and both original captures repeat byte-identically. Validation pins
the compiled core and actual object dependencies; concurrent work in this
checkout prevents a whole-checkout stability claim.
[Combined checks](../analysis/damage-shields-goal-20261011/direct-damage-runtime/delivery-tests.xml),
[Validation](../analysis/damage-shields-goal-20261011/direct-damage-runtime/validation.json).

A retained earlier diagnostic with two distinct quiet NaN payloads differs
between Unicorn and this host. The exact original `ADDSS XMM1,XMM0` bytes,
executed alone with identical operands, reproduce the difference. Literal
ordered SSE operations preserve instruction operand order in production;
the passing corpus uses one quiet NaN payload. Distinct-payload Xbox behavior
remains unresolved, and the earlier failing capture is retained.
[Isolated instruction probe](../analysis/damage-shields-goal-20261011/direct-damage-runtime/nan_operand_probe.json),
[earlier original capture](../analysis/damage-shields-goal-20261011/direct-damage-runtime/original-v4/report.json).

## Remaining delivery

Shield/body processing, material lookup, section handling and effect-request
prefixes now compile in the shared delivery module. The original stops recorded
above remain historical evidence; current captures execute beyond those owners.
Full death/aftermath, physics impulses, effect allocation/attachment, naturally
admitted producers, source-authoritative live banks and final shield pixels
remain active work.
[Current delivery and exact remaining boundaries](SOURCE_DAMAGE_DELIVERY_RUNTIME.md),
[vitality implementation](SOURCE_VITALITY_RUNTIME.md).
