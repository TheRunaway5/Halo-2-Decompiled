# Original damage delivery and native wiring

2026-10-12. The shared native damage path selects **90 complete paired
TU1.1/TU1.5 bodies, comprising 8,976 original instructions**. This extends the
previous 65-body delivery module with unit response selection, authored
thresholds, animation/reaction prefixes, impulse handling and their reached
lower owners.
Complete bodies do not imply that every branch or lower service works.
[Interface](../include/halo2/source_damage_delivery_runtime.hpp),
[implementation](../src/source_damage_delivery_runtime_generated.cpp),
[authenticated generator](../tools/generate_source_damage_delivery_runtime.py),
[manifest](../analysis/damage-shields-goal-20261011/feedback-next-runtime/generated/manifest.json),
[delivery authority](DAMAGE_VITALITY_DELIVERY_HANDOFF.md),
[feedback authority and exact ABI](DAMAGE_FEEDBACK_DELIVERY_HANDOFF.md),
[next four complete services](DAMAGE_FEEDBACK_NEXT_SERVICES_HANDOFF.md).

## Wiring

The [shared composition](../src/source_damage_runtime.cpp) uses the generated
ownership function and complete delivery module. Owned calls retain the same
memory bank, request, stack, GP/SSE/x87/flags and MMX state. Lower calls continue
through the caller's actual service. Absent services retain the pending frame
and the state already produced, with no supplied successful callback.

Both [native multiplayer object dispatch](../src/multiplayer_object_runtime.cpp)
and [object/world dispatch](../src/death_object_phases.cpp) select this shared
composition. The ordinary object-update `BD417` vitality call also uses it.
Existing native weapon, construction, animation and vehicle-motion bodies
participate through actual object/world dispatch. The multiplayer owner also
has its startup/effect allocator service; that broader runtime is distinct
from the object/world-only comparison scope below.
[Build integration](../cmake/SourceDamageDeliveryRuntime.cmake),
[prior vitality consolidation](SOURCE_DIRECT_DAMAGE_RUNTIME.md).

The live [body-material animation loop](../src/multiplayer_runtime.cpp) now maps
each player material range to its owner and evaluates that biped's authored
object-function exports through
[`damage_feedback_shader_input`](../src/damage_feedback.cpp). Its scalar and
activity handler reads the current player's health, shield, alive state and
recent damage. The existing armor/emblem animation consumer receives this
callback. The original alias fixture checks the same helper. This connects a
real material input consumer; it does not admit or draw a shield overlay.
[Comparison](../tests/damage_feedback_tests.cpp),
[live rendering seam and retained original instructions](SHIELD_LIVE_BODY_WIRING_HANDOFF.md).

The host `Simulation::damage` and scalar recharge still own separate host unit
storage. The live callback consumes that host snapshot, while the generated
source routines write retained source-object banks. Natural contact ownership,
complete source aftermath and the live shield collector/transparent queue
remain open; the full goal is active.
[Live simulation](../src/simulation.cpp),
[complete coverage goal](DAMAGE_SHIELD_COVERAGE_GOAL.md).

## Original comparisons

Captures execute unchanged original executables with the full Lockout tag image
and original engine-registration constructor. All 96 local damage definitions
and both multiplayer character models remain included. Object/header/player/
actor/component banks, maxima, fractions, stun counters, clock and replication
context are declared diagnostic inputs; metadata overlays are recorded. No
instruction patch or native expected outcome is used. This revision declares
cold feedback/reaction/action fields explicitly and adds 216 independent
`DAC40` report cases and 16 authored `CAA10` threshold cases.
[Damage measurement](../tools/measure_source_damage_feedback_runtime.py),
[vitality measurement](../tools/measure_source_damage_feedback_vitality.py).

| Comparison | Cases | Complete | Instructions | Ordered stores |
| --- | ---: | ---: | ---: | ---: |
| Delivery module, each original image | 2,231 | 1,775 | 963,116 | 194,122 |
| Vitality through delivery module, each image | 3,584 | 3,550 | 1,016,185 | 118,671 |
| Actual object/world composition, TU1.1 damage | 2,231 | 1,811 | 1,045,253 | 209,770 |
| Actual object/world composition, TU1.1 vitality | 3,530 | 3,514 | 1,004,431 | 117,085 |

The raw damage corpus executes 3,106 distinct sites; composed damage executes
4,020 including existing lower modules. Comparisons check every instruction
site, ordered store, retained byte, full GP/SSE state, defined flags, FP
controls, active extended values/TOP and pending frame. Missing-service reruns
must preserve the same produced prefix. Eight successive nonlethal shield hits
carry object state into the next original input; the 32-phase RNG and
2,801-phase vitality trajectories remain covered. Composed captures include
the actually reached unit-animation module rather than stopping at a call
which native dispatch already owns.
[Coverage and trajectories](../analysis/damage-shields-goal-20261011/feedback-next-runtime/coverage.json),
[strict comparison](../tests/source_direct_damage_runtime_tests.cpp),
[actual composition scope](../tools/damage_feedback_composition_scope.py).

The deeper vitality composition excludes 54 older diagnostic rows whose
nonzero permutation masks refer to absent permutations: both actual character
sections have zero permutation records. All 3,584 raw rows remain retained and
tested. The explicit exclusions and failed preliminary deeper capture remain
available; no authored permutation or successful return was invented.
[Exclusion report](../analysis/damage-shields-goal-20261011/feedback-next-runtime/vitality-composition-v1/report.json),
[prior preliminary failure](../analysis/damage-shields-goal-20261011/delivery-runtime/vitality-composition-capture-v1.log).

Ordinary damage now executes the actual response choice and `DAC40` stores to
recent body/shield damage and timers before reached reaction services. First
pending entries in composed damage are `179B90`, `B8770`, `329168`, `E6E10`,
`E86E0` and `1C9D80`. These remain real lower boundaries, not successful effects
or completed aftermath. No naturally admitted shot or shield pixel is proven
by these declared-bank comparisons.
[Raw report](../analysis/damage-shields-goal-20261011/feedback-next-runtime/original-v1/report.json),
[composed report](../analysis/damage-shields-goal-20261011/feedback-next-runtime/composition-v1/report.json).

The new `A8ED0` successes cover its cold session-mode return (7 of 170 sites),
not its mode-5 report packing or network enqueue. Raw both-image diagnostics
do not reach `DFC50/CA320`; actual TU1.1 composition executes 108/247 and 38/38
sites respectively. Both raw and composed inputs execute 30/40 `115C60` sites.
Their complete paired bodies are compiled and owned without claiming all
runtime branches. The independent paired data audit retains `DFC50`'s zero
and up-vector pointer slots and pointees.
[Exact new service boundaries](DAMAGE_FEEDBACK_NEXT_SERVICES_HANDOFF.md).

## Retained discrepancies

Eight paired acos/CRT bodies were withheld from native ownership after a
provisional 94-body composition diverged at its x87 precision-status branch.
Original case 908 does not enter `32E4D7`; the provisional native run does and
requests an unrecorded stack extent. The failing generated code, original
capture and native failure remain retained. The exact original-hardware result
is unresolved; no precision flag or return was forced to pass. Current native
composition stops at the actual `329168` call.
[Mismatch record](../analysis/damage-shields-goal-20261011/feedback-runtime/withheld-crt.json),
[paired original listings](DAMAGE_FEEDBACK_DELIVERY_HANDOFF.md).

Retail and title-update replication layouts differ: retail's dirty-mask stride
is `0x20`, whereas TU1.1/TU1.5 use `0x24`. Native implements the paired update
instructions. Distinct quiet NaN payload preference on original hardware also
remains unresolved.
[Retail boundary](DAMAGE_VITALITY_DELIVERY_HANDOFF.md),
[retained literal ADDSS probe](../analysis/damage-shields-goal-20261011/direct-damage-runtime/nan_operand_probe.json).

## Validation

The native build, CLI check and all **39 selected checks** pass. Original
captures and generated sources repeat byte-identically. Validation freezes the
measured core sources and actual compiled object dependencies. Concurrent work
continues in this checkout; the record does not establish whole-checkout
stability, physical Xbox behavior, complete native gameplay or shield pixels.
[Validation](../analysis/damage-shields-goal-20261011/feedback-next-runtime/validation.json),
[combined checks](../analysis/damage-shields-goal-20261011/feedback-next-runtime/delivery-tests.xml),
[reproducibility](../analysis/damage-shields-goal-20261011/feedback-next-runtime/reproducibility.json),
[previous 86-body validation](../analysis/damage-shields-goal-20261011/feedback-runtime/validation.json),
[previous 65-body validation](../analysis/damage-shields-goal-20261011/delivery-runtime/validation.json).
