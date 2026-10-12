# Original vitality update, health recovery and overshields

The current [shared delivery module](SOURCE_DAMAGE_DELIVERY_RUNTIME.md) also
connects these routines to newly completed shield/body/material and section
owners. The comparisons below retain the original 14-entry snapshot.

2026-10-11. Fourteen complete, independently paired TU1.1/TU1.5 bodies now
compile 976 original instructions. The module includes the complete vitality
update, health restoration, overshield activation, damage-info lookup, recent
damage fading, attribution, object/player resolution, multiplayer shield
traits, difficulty scaling and HUD shield-history adjustment.
[Interface](../include/halo2/source_vitality_runtime.hpp),
[native generated bodies](../src/source_vitality_runtime_generated.cpp),
[generator](../tools/generate_source_vitality_runtime.py),
[complete correspondence](../analysis/damage-shields-goal-20261011/vitality-runtime/pairing-full/report.json),
[generated manifest](../analysis/damage-shields-goal-20261011/vitality-runtime/generated/manifest.json).

| Owner | TU1.1 | TU1.5 | Instructions |
| --- | --- | --- | --- |
| Complete vitality update | D73E0 | D8330 | 545 |
| Health restoration | D7D90 | D8CE0 | 29 |
| Overshield activation | D80F0 | D9040 | 22 |
| Damage-info lookup | D7160 | D80A0 | 24 |
| Recent-damage fading | DA150 | DB0A0 | 99 |
| Object replication notification | A87E0 | B1C30 | 13 |
| Pause/mode predicate | 139FF0 | 13B360 | 7 |
| Damage attribution | D7CD0 | D8C20 | 52 |
| Unit-player resolver | 14F5D0 | 1509B0 | 9 |
| Player HUD shield history | 13BE58 | 13D1C8 | 13 |
| Typed object lookup | BBCB0 | BCDF0 | 35 |
| Team/difficulty selection | 1ED0B0 | 1F19D0 | 48 |
| Authored difficulty lookup | 1ED030 | 1F1950 | 26 |
| Multiplayer shield traits | 159EF0 | 15B2D0 | 54 |

Seven owners also have complete retail-to-TU1.1 instruction/internal-branch
correspondence, including retail D5DE0, D6790 and D6AF0. No address delta or
reconstructed semantic comment selects a TU1.5 body. Every emitted instruction,
operand width/register/address correspondence and the trait jump table is
authenticated against both unchanged executables. Production executes compiled
C++ arithmetic and static calls, without an instruction decoder or original
binary execution.
[Retail verification](../analysis/damage-shields-goal-20261011/vitality-runtime/retail-report.json),
[normalized correspondence](../analysis/damage-shields-goal-20261011/vitality-runtime/generated/normalized-correspondence.json).

## Preserved behavior and ownership

Health restoration requires a nondepleted body and an ordered body fraction
strictly below one. It writes the fraction before the actual replication
notification and sets the returned AL only afterward. Overshield activation is
a different owner: it admits shield fractions at most one, sets flag bit4,
changes exactly zero (including negative zero) to the original 0.01 literal,
and clears the signed shield-stun word. It does not introduce an additional
body-death gate, alter shield-depletion state or immediately assign three
shields. Both owners preserve their original NaN tests and partial-register
results.
[Complete recovery listing](../analysis/damage-shields-goal-20261011/vitality-runtime/pairing-full/instructions/000d7d90-old.asm),
[activation listing](../analysis/damage-shields-goal-20261011/vitality-runtime/pairing-full/instructions/000d80f0-old.asm).

The complete update retains pending section-response countdowns, actor-bound
lookup, deferred death/reaction flags, mode gates, player shield maxima,
overshield charge and multiplayer drain, ordinary shield/body recharge,
recharge-effect requests, region damage-level decay and recent-damage fading.
The actor-bounds stack slot remains distinct from damage-info; ordinary players
use the original fade defaults. A change in state is not substituted for the
source's returned status. All flags, exact-width stores and external-call order
remain observable.
[Whole original update](../analysis/damage-shields-goal-20261011/vitality-runtime/pairing-full/instructions/000d73e0-old.asm).

The raw API retains the full GP/SSE/flags/x87/FP/stack ABI on one caller-owned
memory bank. Its caller must own real object headers/bodies, selected cache
metadata, actor/player state, clock, region states and replication storage.
Effect allocation/attachment, section responses, actor vitality lookup and
replication delivery remain actual required callees. The module never returns
a supplied successful effect, actor query or notification. An absent owner
stops with the already-produced source prefix and real pending call frame.
[Runtime contract](../include/halo2/source_vitality_runtime.hpp),
[native comparison](../tests/source_vitality_runtime_tests.cpp).

## Original and native evidence

The measurement tool executes both unchanged original images over actual
Lockout Spartan/Elite bipd, hlmt and matg metadata. Object/player/header/clock
banks are explicit standalone diagnostic inputs; controlled vitality, flags,
signed timers, type, user/trait selection, section state, body-rate and region
decay overlays are retained per case. No external routine is hooked or supplied
a return, and native expected values do not generate the original corpus.
[Measurement tool](../tools/measure_source_vitality_runtime.py),
[final original report](../analysis/damage-shields-goal-20261011/vitality-runtime/original-v5/report.json).

Each image has 3,584 cases: 3,543 complete and 41 stop before an unimplemented
callee. Each executes 1,014,087 original instructions and 117,762 ordered
stores. The corpus dynamically executes 840 of the 976 compiled instructions,
including every health-restoration, overshield-activation, damage-info,
typed-object, HUD-history and difficulty-lookup instruction. Compiling the
remaining instructions does not prove their dynamic or live admission.
[Per-body dynamic coverage](../analysis/damage-shields-goal-20261011/vitality-runtime/coverage.json).

Each image also executes a continuous 2,801-phase activation/charge/drain
sequence, retaining the entire original-produced body and HUD bank between
phases. At the declared 30-tick clock and ordinary multiplayer trait, activation
produces 0.01 shield, charge reaches three at update90, and drain reaches one
at update2,790. The HUD history starts at three and ends at one through the
actual original HUD owner. Native execution carries its own previous produced
banks forward and compares them to the next original entry before continuing.
This establishes the admitted sequence on the declared bank; its initial pickup,
clock and world scheduling are separate owners.
[Retained trajectory audit](../analysis/damage-shields-goal-20261011/vitality-runtime/trajectory-audit.json),
[comparison implementation](../tests/source_vitality_runtime_tests.cpp).

Both native comparisons pass: 7,168 cases, 2,028,174 instruction visits,
235,524 ordered stores and 12,933,644 retained-state bytes. They compare full
defined CPU/FP state, instruction traces, pending frames, exact ordered stores
and final memory, including untouched bytes. Eighty-two additional absent-owner
prefix runs reject the missing service without changing the produced prefix.
The original captures and generation independently repeat byte-identically.
This does not establish hardware floating-point exception-status behavior.
[Native TU1.1 result](../analysis/damage-shields-goal-20261011/vitality-runtime/native-disc-v5.log),
[native TU1.5 result](../analysis/damage-shields-goal-20261011/vitality-runtime/native-u5-v5.log),
[repeated original report](../analysis/damage-shields-goal-20261011/vitality-runtime/original-v5-repeat/report.json),
[repeated generation](../analysis/damage-shields-goal-20261011/vitality-runtime/generated-repeat/manifest.json).

All 26 selected combined checks passed, as did the full native build and CLI
help check. Results are recorded with the bounded compiled-source inputs and
actual object dependencies. Other chats are
editing this checkout; observed configuration and a full native build do not
assert stability or compiled-header equivalence for the entire checkout.
[Validation](../analysis/damage-shields-goal-20261011/vitality-runtime/validation.json),
[combined checks](../analysis/damage-shields-goal-20261011/vitality-runtime/delivery-tests.xml).

## Remaining live work

The native multiplayer runtime and object/world dispatcher now route completed
damage/vitality entries through one shared memory and CPU composition. Ordinary
object update `BD360` calls the paired vitality owner at `BD417`; the older
internal vitality/fading copies have been removed. Composed original-state
comparisons exercise the new route.
[Current native wiring and validation](SOURCE_DAMAGE_DELIVERY_RUNTIME.md).

The ordinary Simulation update still calls its scalar recharge adapter. This
new module does not replace live unit storage or fabricate the raw object,
actor, replication or effect owners required by the complete original update.
Equipment/health-pickup admission, natural overshield grants, full dispatcher
delivery, section responses, actor/team campaign branches and actual effect
attachment remain required. Ordinary and overshield world/first-person shaders,
their source scheduler and an original pixel comparison remain open. The full
damage, health/shield and shield-visual goal is active.
[Current live update](../src/simulation.cpp),
[coverage goal](DAMAGE_SHIELD_COVERAGE_GOAL.md).

The complete dispatcher/amount bodies and registered damage-strength callbacks
now compile and join the native composition. The shared delivery module also
compiles the reached shield/body, material and section owners. Subsequent result,
aftermath and effect calls retain their actual state and required lower boundaries.
[Current delivery runtime](SOURCE_DAMAGE_DELIVERY_RUNTIME.md).

Reproduce the final source comparison from the repository root:

```sh
.tools/python/bin/python tools/generate_source_vitality_runtime.py \
  --output src/source_vitality_runtime_generated.cpp \
  --manifest analysis/damage-shields-goal-20261011/vitality-runtime/generated/manifest.json
UC_IGNORE_REG_BREAK=1 .tools/python/bin/python tools/measure_source_vitality_runtime.py \
  --output analysis/damage-shields-goal-20261011/vitality-runtime/new-original-capture
cmake --build build-damage-shields-20261011 --target source_vitality_runtime_tests -j 2
ctest --test-dir build-damage-shields-20261011 \
  -R '^original_source_vitality_(disc|u5)$' --output-on-failure
```

Capture output must be a new directory. The configured CTests use the retained
final v5 corpus; compare a newly generated capture's hashes with that corpus
before changing a validation input.
