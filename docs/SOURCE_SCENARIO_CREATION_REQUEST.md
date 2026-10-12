# Original scenario creation request

The native port can now build the original 196-byte creation request directly
from retained cache placement and palette bytes. This closes the request
producer before generic object construction. It does not allocate, activate or
publish a game object, or admit an object to the live shield collector.

## Original authority and implementation

The [source audit](SHIELD_SCENARIO_REQUEST_SOURCE_RESEARCH.md) freshly pairs
complete TU1.1/u5 request, initializer, policy, Euler, color and selection
owners. The [direct kernel](../src/source_scenario_request.cpp) implements
`D6660`, including type6 game-mask replacement, signed palette admission,
positive-scale fallback, optional variant/color copying, identifier/flag
writes and the defined AL return. Each reached lower owner is mandatory.
Failure at an unavailable owner retains its preceding writes and throws;
it does not become a supplied successful or ordinary false return.

A palette definition of NONE returns false **after** writing UID, identifier,
flags and ordinal. Earlier type/game/index failures leave the request intact.
The prior C4 contents therefore remain an explicit caller input. A rejected
request must not be treated as an atomic operation or a completed object.

The [byte binding](../src/source_scenario_request_binding.cpp) composes the
existing complete `B8820` initializer, the existing verified Euler arithmetic,
complete raw `D7070` policy and byte-to-float color conversion. D7070 retains
signed policy/source bytes and masked x86 type shifts, including raw values
outside the decoded ScenarioObject validator. Color conversion ignores alpha.
Each byte-times-literal-float product fits exactly in binary64 before its one
binary32 rounding in the declared nearest configuration.

`source_cache_scenario_creation_request` requires the actual selected scnr
datum, type, ordinal, force byte, prior request contents, caller FP controls
and optional current game-option fields. It resolves the placement and palette
through actual signed type-record `+A/+C` words and stride `+E`, then reads
retained source-addressed cache bytes. Missing type6 game fields stop when
reached. Descriptor NONE, invalid native selection and escaping pointers stop
explicitly. Those bounds exceptions are native boundary guards, not reproduced
Xbox exception behavior. The returned request owns its C4 bytes.

[OriginalData](../src/original_data.cpp) imports the two initializer basis
pointers and one operand, request zero/color operands and all thirteen raw
descriptor fields from the selected TU1.1/u5 image. The unverified base image
exposes none of these providers. [Typed metadata](../include/halo2/object_construction_tables.hpp)
keeps `+A/+C/+E` separate from the existing bounded general read ranges.

## Floating-point boundary

The direct D6660 owner shares explicit COMISS state with its lower services,
including its existing unmasked-exception stop. The supplied byte binding
accepts source x87 CW `027F` and SSE controls `1F80` only when math is reached.
It temporarily establishes host nearest rounding without host DAZ/FTZ and
restores the caller's host environment. Euler inputs must be finite and within
the previously tested magnitude range; the [earlier Euler evidence](source-euler.md)
and the new cases are bounded comparisons, not a proof of every transcendental
argument or platform implementation. Non-nearest Euler, callee arithmetic FP
exception/status propagation, full x87 stack state and Xbox exception delivery
remain outside this byte-binding contract. Returned comparison state covers
the original COMISS boundary only. No undefined EAX from B8820/11F680 is used.

## Differential validation

The [driver](../tools/measure_source_scenario_request.py) runs unchanged
TU1.1 and u5 D6660 with all real initializer, lookup, policy, Euler and color
callees. Every executed instruction is checked against the pinned image.
It replaces no instruction or call result. Inputs are explicit ABI backing,
every raw authored Lockout placement, declared game-type/engine fields and
labeled branch controls. Type-null/extent controls modify named metadata
fields only; they are not claims about a live game history.

Both images produce identical results for 623 cases each. The
[native comparison](../tests/source_scenario_request_tests.cpp) matches all
1,246 defined returns, complete C4 outputs, 97,324 ordered request stores and
6,524 actual lower call ABIs. Forty-eight image/case combinations cover all
24 authored placements, including all fourteen power modules. Those cases
also compare requests produced afresh through the actual retained cache and
selected-image imports. Controls exercise all thirteen types, signed palette
clamps, NONE failure writes, flags/force, game masks, signed raw policies,
scale fallback, every color-byte value and finite Euler inputs. Missing-owner
prefixes, unsupported FP configuration and host rounding restoration are
checked separately.

The [capture](../analysis/damage-shields-goal-20261011/scenario-request-measured/report.json)
and [independent repeat](../analysis/damage-shields-goal-20261011/scenario-request-repeated/report.json)
have four byte-identical files. Fixture SHA-256 is
`2f6936304c54752a9eff34c2bd87be3f47e43fcfd2ac9277bf790f8ce36cc98e`.
[Repeat comparison](../analysis/damage-shields-goal-20261011/scenario-request-repeat-comparison.json).
Scoped build, test, source and binary pins are retained in the
[validation manifest](../analysis/damage-shields-goal-20261011/scenario-request-validation.json).

The current native executable build and `--help` pass. All ten focused request,
constructor, immutable-provider, storage, shared-lifetime, Euler and checkpoint
checks pass, with none skipped. A separate concurrent renderer selection test
now stops because its original selection fixture is unavailable. Its earlier
resident-metadata failure was resolved by concurrent alias handling changes.
Both failures are retained alongside the passing suite. The
build needed a minimal correction of that reader's nonexistent `Bytes::u8`
calls to the existing bounds-checked byte-span API; a concurrent animation
dependency move was already corrected before the final rebuild. No renderer
parity follows from the successful build or these request comparisons.

The [pressure handoff](SHIELD_POWER_MODULE_PRESSURE_HANDOFF.md) identifies
complete1482A0 over retained host storage as the next actual state producer;
existing complete148190/22E4E0 constructors can then be reused. Renderer backing,
lazy second overflow, clock/reclaim and current-world owners remain explicit.
The next required binding is actual `148360` main/overflow pressure state and
services before generic allocation. The existing persistent body arena does
not supply those owners. Complete type preparation/construction, physics,
pose/flags, activation and registration packet extraction remain required.
The broader [damage/shield coverage goal](DAMAGE_SHIELD_COVERAGE_GOAL.md) stays
active; these comparisons add no shield pixels or natural gameplay proof.
