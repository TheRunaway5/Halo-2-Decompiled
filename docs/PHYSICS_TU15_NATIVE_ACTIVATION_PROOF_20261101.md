# Native TU1.5 activation and collision-agent recreation

The activation family and the collision-agent recreation listener now have
typed semantic C++ implementations. Their input is live physics data, typed
services and numerical state; neither implementation receives an instruction
decoder, virtual register file, instruction-loop state or recorded outcome.
This is component/composition evidence. Visible multiplayer activation and
complete native dependency closure remain open.

The authority is the English OG Xbox TU1.5 executable with SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
All original instructions in these probes execute physically on this host;
there are no instruction patches or supplied collision/allocation results.

## Activation and reference family

`native_tu15_island_activation.cpp` implements the original `315CB0`,
`316D40`, `3165D0`, `322660` and `283450` algorithms. This includes the
original forward deferred-reference scan, post-removal index increment,
active/inactive array swap and rereads, reverse listener invocation, null
compaction, protected reference range, and actual destruction/growth service
boundaries. The genuine default `2E4D60` listener is a native no-op.

[The admitted physical proof](../build/test-results/native-island-activation-admitted-tu15/report.json)
passes 598 cases/prefixes with GCC and Clang, including 307 complete calls,
six real required-owner entry stops, 24,224 ordered reads and 3,908 stores.
Three deliberately wrong index/compaction/range implementations fail.

Four prewrite read-modify-write observer prefixes remain excluded at
`316D6E`, `316D8D` and `31663A`. The original probe stops before that entire
instruction retires; the abstract native storage API has already observed the
operand read when a write is rejected. This is an explicit open observer
contract, not an accepted byte waiver or a completed fault-path proof.

## Agent recreation listener

`native_tu15_activation_agents.cpp` implements all 194 source sites of
`282870`. The two-site `316200` dispatcher getter is directly native.
Factory selection, fresh/cached reads, pair and body iteration, filter result
handling, rank stores, and manager release occur in their source order.
The function preserves the original absence of automatic manager cleanup if
a service throws. `31B730` is an actual policy allocation/refcount transition,
not a synthetic flag store, and remains an explicit service here.

The physical composition uses protected v1458 BEFORE worlds from Gemini
column0, Gemini column1 and Elongation. It covers seven actual islands,
existing agents, declared missing-agent inputs, actual filtering and missing
filter branches, body cohorts, default-to-allocated policy transitions, and
16 rounding/denormal-control combinations. No captured after-state, GP/SIMD
postimage or expected callback return is supplied to the native side. Every
remaining service executes genuine original code against its current mutable
bank. A separate uninterrupted original parent checks that tracing itself
does not change persistent memory or FP state.

The initial 128-case composition and all three wrong-factory/rank/policy
controls pass under both compilers. It compares 11,582,360,704 persistent bytes
per compiler, with 7,384 ordered operand events, 1,008 real residual service
calls, 120 genuine manager/agent creations and 44 body policy transitions.
Gemini cases compare 91,290,521 persistent bytes each; Elongation compares
89,148,313 bytes each. However, its shared test bridge changed concurrently,
so [the source-coherence addendum](../build/test-results/native-activation-agents-composed-tu15/source-coherence.json)
withholds current-source qualification. The self-contained
[coherent repeat](../build/test-results/native-activation-agents-coherent-tu15/report.json)
now passes the same 128 cases and three controls under both compilers, with
all four pinned sources unchanged. Its
[aggregate](../build/test-results/native-activation-agents-coherent-tu15/aggregate.json)
records the counts above. Its owned scratch is verified removed, recovering
857,530,368 allocated temporary bytes.

## Composed sleeping-island wake

[The complete native wake composition](../build/test-results/native-activation-wake-composed-tu15/report.json)
passes 104 cases under both compilers. These enter actual `315CB0` with bodies
from all 13 genuinely sleeping islands in each of the two retained Gemini
BEFORE worlds. They then compose native `3165D0`, `316D40`, `322660`,
`282870` and `316200`; applicable policy and entity-listener callbacks still
execute their genuine original bodies. Existing/missing-agent and filter
input variants are declared before original execution.

Each compiler compares 9,494,214,184 persistent bytes, 6,840 ordered operand
events, 364 genuine service calls, 104 native island transitions and 104 real
policy transitions. Eight cases create an actual manager and contact agent.
Wrong active-index and active-flag implementations fail against the original.
Independent uninterrupted originals have zero persistent or FP differences.
All pinned source files remain unchanged. Active-array growth and destructive
deferred-reference cases are not invented or claimed by this cohort. Its
[cleanup receipt](../build/test-results/native-activation-wake-composed-tu15/cleanup-verification.json)
verifies removal of 576,045,056 allocated temporary bytes.

The explicit `NativeTu15ObjectActivationContext` velocity-setter overload
threads this native wake owner through body delivery, component activation and
object wake. Its caller owns FP state, genuine typed services and observation;
the existing no-context overload retains its refusal policy. Actual `B9800`
sleeping-body composition is qualified below; production selection and visible
acceptance remain open. No completed live activation claim follows from the API
addition alone.

## Native service closure and reusable object binding

`native_tu15_activation_services.cpp` supplies native `2E9A00` group filtering,
the `281530` discrete wrapper, the `2816A0` body listener, `B9390` ancestor
wake, and seven authenticated constant class/motion/policy getters. The
already-qualified native `303C10`/`3040D0` factory family and native allocator
are composed into their real activation callers.

[The agent-service composition](../build/test-results/native-activation-services-agents-tu15/report.json)
passes 112 cases and three mutation controls under GCC and Clang. Per compiler
it compares 10,121,712,368 persistent bytes, 7,252 ordered operand events,
56 native factories, 44 native group filters and 88 native discrete wrappers.
Remaining genuine service calls fall to 252 in this cohort.

[The wake-service composition](../build/test-results/native-activation-services-wake-tu15/report.json)
passes 104 cases and four mutation controls under both compilers. It compares
9,494,214,184 persistent bytes and 8,940 ordered operand events per compiler,
with 104 native body listeners, 104 actual ancestor-header updates, eight
native factories and 224 remaining genuine service calls. The object dirty
bit is explicitly cleared as a declared BEFORE input to exercise its real
writer. An initial negative-control run on a pairless island correctly stayed
red for unexercised filter/discrete/class mutations; the final controls use
the actual contact-bearing island instead. Those diagnostics remain retained.

The reusable `native_tu15_set_object_velocities_with_residuals` binding assembles
these kernels with actual typed remaining-owner services, caller-owned filter
and discrete result bytes, FP state and observers. It records each native phase,
unconverted type/factory selection, actual residual identity and attempted call.
No virtual register state or old volatile-register postimage enters the kernel.

[Its complete original B9800 comparison](../build/test-results/native-object-activation-binding-tu15/report.json)
passes 20 actual sleeping-object/velocity-presence cases and two deliberately
wrong velocity/wake controls under each compiler. Each compiler compares
1,825,810,420 persistent bytes and complete FP state, with 30 native island
activations, 54 body activation calls, ten native factories/pool allocations,
20 native shape getters, ten motion getters and 30 policy getters. All 20
old no-context refusal cases still refuse. Unconverted getter/factory selection
counts are zero in this cohort.

The 80 actual remaining service calls are `22D460` (30 body updates), `31B730`
(30 policy transitions), `281F10` (ten discrete predicates) and `3232E0`
(ten contact-manager factories). They remain explicit native-conversion work.
Their real instructions execute with an independently owned service arena;
recorded callback frames, GP/SIMD postimages and captured after-results are not
supplied. Actual RET-pop contracts are checked. Full parent ordered-read/store,
fault and alias domains remain separate from this full-memory/FP composition.

[The internal-callsite audit](../build/test-results/native-object-velocity-callsite-caller-tu15/report.json)
authenticates vehicle `F1DD0` calling `B9800` at `F2091`. Its continuation reads
no ECX/EDX/SIMD result, kills arithmetic flags before use and resets its returned
boolean AL. The known direct `F2340` caller consumes only AL and overwrites full
EAX on both successor paths before reading upper bits. This supports a narrow
typed void-call hook; it does not establish a complete indirect-caller inventory
or generic guest-register postimage equivalence.

Source wiring and positive live multiplayer activation counts still need
qualification. The reusable binding's owned build removal recovers 576,049,152
temporary allocated bytes; its reports retain no large artifacts.

## Authenticated internal velocity handoff and transport

The generated vehicle owner now checks an explicitly bound native owner at
the one authenticated `F2091 -> B9800` call. An absent binding retains the
ordinary offline owner. An active unsupported or failed native call refuses;
it never silently falls back. The binding is tied to the actual memory owner,
executor and nested lifetime. It uses no thread-local CPU context.

[The handoff interface proof](../build/test-results/native-object-velocity-handoff-tu15/report.json)
passes GCC and Clang, including wrong-callsite and lost-state-load controls.
It exercises the actual generated helper and `VehicleState` save/load, scope
ownership, invalid packets, partial failures and preserved live extended values.
This interface result alone is not a physics or live-runtime result.

[The packet audit](../build/test-results/native-object-velocity-callsite-live-packet-tu15/report.json)
authenticates object datum ESI, return word `F2096`, linear argument
`word(entrySP+4) == entrySP+1C`, angular argument `EAX == entrySP+34`, and
the original `RET4` exits. The transport owns no GP/SIMD postimage. It preserves
the actual outer registers and live x87 values, publishes actual numerical
controls/status and advances SP by eight bytes on successful source completion.
Remaining real owners execute in a declared caller-owned service arena below
the active argument packet. Unknown owners, full instruction tracing and
mutable store observation are outside admission and fail explicitly.

[The complete transport comparison](../build/test-results/native-object-velocity-live-transport-tu15/report.json)
passes 48 cases under each compiler and two wrong-kernel controls. It composes
the exact generated helper, scoped binding, ABI transport, native velocity
setter and native activation family against unchanged physical original B9800.
Every incoming x87 depth from zero through five is independently exercised;
declared BEFORE extended operands are installed equally in both executions.
Both paths preserve every ten-byte live operand. No original outcomes or
recorded callee frames enter the native kernels.

Each compiler compares 4,381,945,008 persistent bytes and numerical state,
with 132 native body activation calls, 72 native island transitions and 24
native contact-agent creations. All 48 handoffs complete; refusals and
unqualified-owner counts are zero. The 192 genuine remaining calls are
`22D460` (72), `31B730` (72), `281F10` (24) and `3232E0` (24). The owned
scratch removal recovers 576,090,112 allocated bytes. World selection and
positive visible multiplayer use remain separate work.

The actual source stimulus is vehicle righting/Flip, not ordinary driving.
Source action owner `EDB80` writes direction byte `34D`, sets vehicle `348`
bit six and clears timer byte `34E`. Source `F1DD0` checks those fields, its
authored time window and vehicle orientation before reaching `F2091`.
An authored physical rollover followed by the genuine on-foot Flip action
and reentry into the vehicle camera is required for a positive live call.
No flag, velocity, contact or listener is manufactured to exercise a counter.
The world island-step listener count at `world+A0` remains zero in the current
captures; a genuine registration/stimulus has not yet been established.

## Native predicate and manager constructors for the next binding cohort

`native_tu15_activation_payload_filter.cpp` implements actual `281F10` and
the `1D7530` body property lookup. The predicate preserves its first-component
object/header selection and the subsequent fresh header-byte reads rather
than replacing that source behavior with a modern collision rule.

[The predicate comparison](../build/test-results/native-activation-payload-filter-tu15/report.json)
passes both compilers: 72 island cases, 104 authored pairs, 9,442,801,192
persistent bytes and 732 ordered operand events per compiler. Forward and
reversed typed payload arguments and four rounding modes are exercised.
All 48 motion queries select authenticated native getters. Wrong read order
and wrong accepted-byte controls fail. These captured pairs short-circuit
before the body-property/header branch; that branch is not claimed exercised.
No contact pair is created or modified for the proof.

[The property lookup comparison](../build/test-results/native-activation-body-datum-coherent-tu15/report.json)
independently exercises 46 real body identities in 33 island cases under both
compilers, comparing 4,186,510,718 persistent bytes and 218 ordered read events.
Wrong property key and value controls fail. An initial missing-property
mutation correctly remained red because the available actual bodies all had
that property; its unexercised-control diagnostics are retained. This direct
helper proof does not close the predicate's complete deep branch.

`native_tu15_activation_contact_manager.cpp` implements factory `3232E0`,
constructor `323230`, collector initialization `326100`, body setters and
listener registration/removal, including actual native allocation/growth and
reference ownership. Constructor cleanup uses native C++ ownership whose
states and order come from the authenticated TU1.5 unwind maps. True body
destructors and the exceptional base-lock owner remain explicit dependencies.

[Its original B9800 composition](../build/test-results/native-activation-contact-manager-tu15/report.json)
passes 20 sleeping-object cases and three mutation controls under each
compiler. It compares 1,825,810,420 persistent bytes plus numerical state,
with ten native factories, constructors and collectors and 20 native listener
registrations per compiler. Wrong manager type, listener identity and body
reference changes fail. This cohort does not exercise array growth, removal,
collector destruction or fault/unwind cleanup; none is claimed qualified by
these counts. The source inventory and actual unwind maps are retained in
[the residual inventory](../build/test-results/native-activation-residual-inventory-tu15/report.json).

The new predicate/manager files are separate from the frozen live transport
cohort. [Their exact staged binding composition](../build/test-results/native-object-activation-next-binding-tu15/report.json)
passes 20 original B9800 cases and two mutation controls under both compilers,
with 1,825,810,420 compared persistent bytes per compiler. It performs ten
native predicates, ten native manager factories and 20 registrations. Actual
remaining service calls fall from 80 to 60: only `22D460` (30) and `31B730`
(30) remain in this cohort. The minimal diff, staged source hashes and full
input/source pins are retained in
[the next binding receipt](../analysis/physics/native-activation-next-binding-20261101/source-pins.json).
It has not modified the existing frozen live binding; live selection remains
pending. This composed proof removes another 576,069,632 scratch bytes.
Their completed owned scratch removals recover 857,427,968 bytes for the
predicate, 857,399,296 bytes for the property lookup and 576,016,384 bytes for
the manager composition. No large artifacts are retained.

## Native body policy and the combined velocity binding

`native_tu15_body_policy.cpp` implements actual `31B730`, constructor
`31E820/31E760`, policy installation `1E10A0`, the `31E7F0` destructor,
reserved-reference handling and native allocator ownership. Its constructor
uses native SIMD and x87 with the caller's numerical controls. Shape bounds
remain a typed service and are composed with the qualified native bounds
family, including nested caller-owned transform/list data.

[The native policy/bounds composition](../build/test-results/native-body-policy-native-bounds-tu15/report.json)
passes 20 B9800 cases and three coefficient/extent/reference controls under
both compilers. Each compiler compares 1,825,810,420 persistent bytes and
numerical state, creates 30 native policies and invokes actual native bounds
owners `307E30` (20), `2E5E80` (10) and `2E8120` (10). No genuine bounds
fallback executes. Default-policy replacement, destruction and fault cleanup
are not exercised by these constructor counts and remain distinct domains.

[The final combined F2091 transport proof](../build/test-results/native-object-velocity-policy-transport-tu15/report.json)
passes 48 cases per compiler across every incoming x87 depth from zero through
five, plus the two deliberately wrong velocity/wake controls. It composes the
actual generated helper, scoped handoff, native B9800, activation, predicate,
manager and policy kernels and native recursive bounds. Each compiler compares
4,381,945,008 persistent bytes plus numerical state, completes all 48 handoffs
and constructs 72 native policies. All live extended operands are preserved.
Refusals, unqualified owners, unconverted policy types/destructors and bounds
fallbacks are zero. The only actual remaining service is `22D460` (72 calls).

The [qualified policy binding delta](../analysis/physics/native-activation-policy-binding-20261101/source-pins.json)
records the final staged binding/header/transport hashes, exact minimal diff
and full tested source cohort. It follows the predicate/manager delta. Its
ordinary data arena supplies 32 aligned policy-bound bytes and retained nested
shape data above the genuine service frames; it retains no original CPU frame
or volatile-register postimage. The World integration owner has applied that exact delta to the frozen11
live source cohort; its live acceptance is tracked separately. The final transport proof recovers
576,172,032 scratch bytes after cleanup.

`22D460` is the 189-site model/constraint activation update. It first checks
component flag `800`, the actual active-body predicate and the constraint bank
at component `+80`. Every one of the 25 distinct Gemini/Elongation components
in the protected inputs has zero such rows. The genuine Warthog BEFORE archive
adds five components: one player, two Warthogs and two Ghosts, also all zero.
Its [small census/provenance report](../analysis/physics/native-activation-warthog-constraint-census-20261101/report.json)
retains actual identities and counts; all 106,827,776 extracted bytes are
verified removed while its protected archive remains unchanged. A genuine
constraint-positive input is still required to qualify the deep creation loop;
no contact, constraint or registration row is manufactured to fill that gap.

## Native constraint activation follow-up

The new `native_tu15_body_constraint_update` module owns the actual189-site
`22D460` parent, active-body predicate `1D9EC0`/`315C90`, definition selector
`1F3460`, body-slot reader `DE8C0`, sparse request constructor `20D8B0` and
list append `20D940` through the existing native `16E6A0` allocator. Once
selected, this parent never executes the original whole parent again.
Positive-loop lookup, admission and creation remain explicit typed services;
an unbound service is counted and throws before supplying an outcome.

[Parent qualification](../build/test-results/native-body-constraint-update-tu15/report.json)
passes152 real island/FP cases and two gate/read mutation controls per compiler
under GCC and Clang. The actual168 body calls per compiler cover16 flag skips,
96 inactive skips and56 empty banks, with15,279,981,672 persistent bytes plus
ordered operands, FP controls/status and source RET4 compared. Constraint rows
remain zero, so this does not qualify creation, publication or fault paths.

The parent also owns native `1D8B90`/`2E4650` through the already-native
`2E44D0` geometry kernel. [Frame qualification](../build/test-results/native-body-constraint-frame-tu15/report.json)
passes280 genuine body slots per compiler, comparing all52 frame bytes,
25,504,520,024 persistent bytes, ordered reads and FP state. Eight actual
component transform cases execute the full native composition. The separate
[native230F60 scan qualification](../build/test-results/native-body-constraint-reference-distance-tu15/report.json)
passes the same280 slots, including the real bitmap access. All captured
controller entries are FFFF, so its point-distance arithmetic loop remains
unqualified. No local controller or contact is manufactured for that branch.

The [staged constraint binding delta](../analysis/physics/native-activation-body-constraint-binding-20261101/source-pins.json)
reuses64 ordinary request DATA bytes after the earlier policy/bounds phase has
returned. Its [full generated F2091 transport qualification](../build/test-results/native-object-velocity-body-constraint-transport-tu15/report.json)
passes48 cases per compiler over incoming x87 depths0..5 and two controls,
comparing4,381,945,008 persistent bytes plus numerical state. Every compiler
executes72 native constraint updates (24 flag skips,48 empty banks), completes
48 native handoffs and uses zero original residual calls, unbound services or
refusals within this cohort. The unexercised positive constraint rows remain
open. This delta is prepared for the integration owner and has not modified
the frozen live binding.

The independent `native_tu15_model_constraint_lookup` module converts
`1DD710`, `22DEF0`, `22E040`, `22E000` and `22E0D0`. Genuine component `+20`
instance lists are positive even though their authored `+80` banks are empty.
The validator projects an ordinary query from each real existing descriptor's
datums, nodes, key, class and pose, leaving every instance/contact unchanged.
[Coherent lookup qualification](../build/test-results/native-model-constraint-lookup-coherent-tu15/report.json)
passes152 island/FP cases plus a read-owner mutation control per compiler:
192 queries,98 candidate checks,66 matching keys,34 real SIMD pose checks and
64 visible found results. Both compilers match17,428,921,024 persistent bytes,
ordered operands and numerical state. This qualifies those actual queries; it
does not supply an authored positive `22D460` request or close all pose/fault
branches. This new module is not yet part of the live binding.

The read-only `native_tu15_contact_time_input_observer` API supplies the actual
typed native contact-time inputs before kernel access: island, configuration,
entry, output, time bits, FP controls/status and preserved x87 depth. Its scoped
binding is tied to the memory owner and executor; it retains no guest ABI or
physics result. [Interface qualification](../build/test-results/native-contact-time-input-observer-tu15/report.json)
passes GCC and Clang for nesting, ownership, wrong-executor rejection and
exception cleanup. Live capture and dynamic-pair eligibility remain the World
and multiplayer validation owners' work.

## Native constraint service follow-up

The integration owner has now applied the qualified native lookup/capacity
composition to the live binding. Its source receipt is
[the constraint-services delta](../analysis/physics/native-activation-constraint-services-binding-20261101/source-pins.json).
The earlier staged-only descriptions above record the state at those proofs;
this receipt supersedes their integration status. Positive authored constraint
creation and fault/lifetime branches remain open.

Native `1DD7B0`/`22DD90`
[capacity qualification](../build/test-results/native-model-constraint-capacity-tu15/report.json)
passes 152 real island/FP cases plus a control under each compiler, with 168
actual calls and 15,279,981,672 persistent bytes compared. The real None/slot 0
cases are covered. Capacity exhaustion and the `22D0A0` removal service are
unexercised; an unbound removal service throws rather than executing the
original parent.

Four independent native helpers have physical TU1.5 comparisons under both
compilers, each using 40 eligible real island/FP inputs plus a mutation control
and comparing 5,758,527,040 persistent bytes, ordered operands and FP state:

- [`22E390` body slots](../build/test-results/native-constraint-body-slots-tu15/report.json)
  covers 64 actual instances, 24 primary slots and 68 real child rows, including
  44 matching children.
- [`22EDE0` motion comparison](../build/test-results/native-constraint-motion-compare-tu15/report.json)
  covers 64 actual queries: 48 None-second and 16 paired velocity comparisons.
- [`22FE30` scalar cache](../build/test-results/native-constraint-scalar-cache-repeated-tu15/report.json)
  covers 64 actual first publications and 64 repeated enabled retentions. The
  cooldown/FISTP branch is unexercised.
- [`22E1B0` reference release](../build/test-results/native-constraint-reference-release-tu15/report.json)
  covers 64 real 16-bit decrements. The `22FEA0` removal branch is unexercised
  and remains an explicit typed service.

These helper proofs do not supply a positive authored `22D460` request. They
are separate from the frozen live flip integration cohort.

## Native vehicle flip and its actual consumer

`native_tu15_vehicle_flip` replaces all 174 sites of actual TU1.5 `F1DD0` with
native semantic logic. It preserves the source flag, direction, timer and up
gates, signed direction cross products, authored limits, correction order,
and timer publication. Its x87/SSE calculations use the actual caller's FP
controls and hardware status. The kernel receives ordinary object/velocity
DATA and FP controls/status; it receives no guest GP/SIMD postimage or live
extended operands.

[Parent qualification](../build/test-results/native-vehicle-flip-tu15/report.json)
passes 208 cases and a wrong-axis control per compiler. Four genuine vehicle
objects from the protected Warthog BEFORE archive supply the backing data;
the 13 control/basis variants and four rounding modes are declared test inputs.
Both compilers match 18,664,787,024 persistent bytes, actual getter values,
pre-setter vectors, parent read/store order and numerical state. These positive
control variants are not a recorded gameplay rollover.

`native_tu15_flip_vehicle_with_activation` binds the getter directly to native
`BC200` and the setter directly to
`native_tu15_set_object_velocities_with_residuals`. It does not construct an
`F2091` packet. [Full activation qualification](../build/test-results/native-vehicle-flip-full-activation-tu15/report.json)
passes 288 cases and the control per compiler over preserved x87 depths 0..5:
208 native flips, 416 native body activation calls, zero island wakes and zero
remaining-owner calls. Both compilers match 25,843,551,264 persistent bytes,
AL/RET0 and FP state. Full child ordered reads are not claimed by this bank
comparison; the parent prefix proof supplies the separate order check.

The production handoff is restricted to actual `F2340`'s call at `F24D8` to
`F1DD0`, returning at `F24DD`. It validates that actual stack packet and its
owner bounds, FP presence, non-MMX state, clear direction flag, depths 0..5,
read-only observation and inactive traces. The scoped binding belongs to the
memory owner and executor. It publishes only AL, RET0's SP+4 and genuine FP
controls/status, preserving the other GP/SIMD data and live x87 operands.
Unbound offline operation keeps its ordinary route; an active unsupported
packet refuses before supplying a result. No original whole-parent fallback
runs after native admission.

[The coupled generated-consumer qualification](../build/test-results/native-vehicle-flip-handoff-coherent-tu15/report.json)
uses the exact staged source helper through actual `VehicleState::load/save`.
Each compiler completes 52 handoffs, including 48 native flips, with zero
refusals and zero activation residuals. Both compare 4,666,196,756 persistent
bytes and numerical state while preserving the actual extended operands.
[The separate interface suite](../build/test-results/native-vehicle-flip-handoff-interface-coherent-tu15/report.json)
passes both compilers for nesting, ownership, unsupported packets, exception
cleanup and caller data preservation; that interface suite alone claims no
physics parity.

[The qualified handoff receipt](../analysis/physics/native-vehicle-flip-live-handoff-20261101/source-pins.json)
pins all six new semantic/binding/handoff sources, the exact generated one-call
diff and generator change. The integration owner applied those exact changes
with the World callback, strict FP build units and activity/refusal counters in
[the live freeze 24 receipt](../analysis/physics/native-vehicle-flip-world-live-binding-20261101/source-pins.json).
The selected multiplayer run is a separate acceptance step. A genuine live
rollover/Flip action, positive wake/listener work, and all indirect caller
contracts remain open; this exact consumer proof does not qualify them.

## Genuine typed dynamic-pair reference

The diagnostic capture predicate now follows the actual physical-body pointer
at `word(payload+10)` and separately records `payload-10` as the process entity.
The earlier no-eligible-pair reports used the wrong body lookup and establish
no absence of dynamic pairs. This read-only correction changes no physics.

The multiplayer owner captured one genuine native contact-time BEFORE pair
from the visible rocket case with actual vehicle camera. Both distinct
physical bodies have motion kind 5/getter `31DD70` and zero body `+40`; actual
entity/property identities and source typed input are retained beside the
archive. The required input is
`analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-entity-recovery-original-v1464-20261101/iteration-04-native-sphere-feature-processor-simplex-rockets-vehicle-camera/required-native-dynamic-pair-before/actual-native-before.tar.zst`,
12,006,078 bytes, SHA-256
`9403d05a5d2ea0fd27ad22b0f734b1b2d1f0c007e673b325d64df5ef3a47577d`.
The capture contains 4033 BEFORE banks / 89,795,481 uncompressed bytes and no
captured guest ABI, SIMD state, after banks or supplied outcomes. Its archive
roundtrip was checked and 224,862,208 temporary watcher bytes removed. Deep
dynamic-pair semantic qualification remains separate work using this input.

## Native model-constraint removal and construction

`native_tu15_model_constraint_removal` owns complete `22D0A0`, composing the
qualified reference/slot/cache helpers and native `16E8A0`. The
[removal proof](../build/test-results/native-model-constraint-removal-tu15/report.json)
passes 40 island/rounding invocations per compiler, comprising 64 genuine
existing-instance removals: 64 list removals, 28 list frees, 68 reference
decrements and 44 local rows. Both compilers compare 5,758,527,040 persistent
bytes, ordered accesses, FP state and RET4. No handle child, runtime removal
child or global authored row executes in this cohort. The
[removal binding transport proof](../build/test-results/native-constraint-removal-binding-transport-tu15/report.json)
passes 48 F2091 cases per compiler over depths 0..5, comparing 4,381,945,008
persistent bytes with zero residuals/refusals. Removal calls are zero in that
ordinary empty-authored-row transport; its independent removal proof supplies
the positive instance-deletion evidence.

Native `22EBA0` composes existing native `1D8B90` body frames and `144090`
inverse frames, with two distinct caller-owned 52-byte DATA buffers.
[Point qualification](../build/test-results/native-constraint-local-points-declared-tu15/report.json)
compares 64 descriptor inputs per compiler: 64 first points, 16 second points,
80 frames/inverses and 5,758,527,040 persistent bytes, plus ordered accesses,
FP and RET8. Forty inputs retain their actual resolved slots. The remaining
24 explicitly declare slot zero from the same genuine component body bank
because the dormant descriptor resolves no current slot. No descriptor,
request or contact input row changes. The retained-node branch is unexercised.

Native `122B60` uses actual x87 for the source-associated extended length and
actual SSE for publication. Its
[normalization proof](../build/test-results/native-vector-normalize-tu15/report.json)
passes 176 profiles per compiler: 11 actual/declared vectors, four rounding
modes and preserved depths 0..3. The 112 normalizations and 64 fallback copies
match all vector/return bits, ordered accesses and FP status, while the
original preserves the declared live extended operands. Declared vectors
include NaNs, signaling NaNs, infinities, denormals, threshold values, signed
zero and finite extremes. Its five temporary x87 operands impose the explicit
depth limit; unsupported numeric fault profiles refuse before native work.

Complete native `22CEA0` now composes native allocation, reference distance,
normalization, motion comparison, point setup and reference release. The
[constructor proof](../build/test-results/native-model-constraint-creation-declared-tu15/report.json)
contains 40 island/rounding test invocations and **256 independent original
and native constructor calls per compiler**, enumerating the real existing
descriptors and declared depths 0..3. The
[hashed call aggregate](../build/test-results/native-model-constraint-creation-declared-tu15/constructor-call-aggregate.json)
records this expansion explicitly. Each compiler performs 256 source/native
allocations, 160 point setups/reference releases, 64 second definitions and
224 frame inverses, comparing 23,034,108,160 persistent bytes, ordered accesses,
FP, EAX and RET8. Every source/native instance is constructed by its actual
algorithm from declared request DATA projected from genuine descriptors;
no input contact/constraint row or original result is supplied. Ninety-six
profiles explicitly project a genuine body slot where dormant resolution is
absent. This does not constitute a captured authored `+80` caller request;
the reference-distance point loop remains unexercised.

The [creation binding delta](../analysis/physics/native-activation-constraint-creation-binding-20261101/source-pins.json)
and [combined creation/handle delta](../analysis/physics/native-activation-constraint-handle-binding-20261101/source-pins.json)
have 48-case F2091 transport proofs per compiler with 4,381,945,008 persistent
bytes, preserved depths 0..5 and zero residuals/refusals. The live owner has
applied their exact two-file binding composition. The constructor callback
requires 192 ordinary DATA bytes and admits creation depths 0..3, exposing
storage/numerical refusals. These ordinary transport cases invoke no
constructor or capacity-removal callback; their positive component proofs and
future positive live caller acceptance remain separate.

## Native runtime-handle lifetime frontier

The [read-only lifetime census](../analysis/physics/native-model-constraint-lifetime-input-census-20261101/report.json)
finds 26 actual salt-bearing runtime handles in the protected Gemini and
Elongation BEFORE inputs. They are independent real runtime records, not
invented attachments to constraint descriptors. Native `17A0F0` and `17A030`
[qualification](../build/test-results/native-constraint-runtime-handles-tu15/report.json)
passes 280 original/native calls per compiler: 208 valid operations from those
26 handles, two deletion arguments and four rounding modes, plus 72 declared
None/index/salt guards. Both compilers compare 25,407,106,904 persistent bytes,
ordered accesses, FP and RET4. All valid records take retained marking and
their definitions skip the tagged-field branch. Positive tagged-field and
immediate-destruction branches are not inferred from this result.

Native `C6120` reuses existing native `16EA60` and the actual current runtime
tag table. The [tag-value proof](../build/test-results/native-constraint-tag-values-tu15/report.json)
passes 96 original/native calls per compiler, with actual referenced-group
datums, a real context and declared None arguments. Its 24 native iterator
queries and 72 invalid-input returns compare 8,695,339,360 persistent bytes,
ordered accesses, FP and RET8. Every retained tagged-value pool is enabled
but has extent zero. Candidate matching, numeric clamping and stores remain
unexercised; no value row is manufactured to fill this gap.

A new public semantic `17A030` API is staged as a wrapper over the unchanged
private native implementation, avoiding duplicated owner logic or GP state.
[Its requalification](../build/test-results/native-constraint-handle-cleanup-api-tu15/report.json)
passes the same 280 handle calls per compiler. The staged source and original
base hashes are recorded in
[the API receipt](../analysis/physics/native-constraint-handle-cleanup-api-20261101/source-pins.json).

Native `17A4A0` closes the real destruction composition through `17A420`,
`BCDF0`, `17A380`, shared `17A030`, `17A230`, `17A2F0` and native `16E8A0`.
[Destruction qualification](../build/test-results/native-constraint-handle-destruction-tu15/report.json)
passes 140 original/native calls per compiler: 104 genuine valid-handle
destructions and 36 argument guards. Each compiler observes 104 actual object
reference detachments and handle-row frees, 264 event-node frees, 168 event
lists and 1,664 tail pairs, comparing 12,703,553,452 persistent bytes, ordered
accesses, FP and AL/RET0. No remaining child executes. The positive child-link
owners `177FA0`/`177020` and deferred destruction remain explicit unexercised
domains. The new destruction module and staged wrapper are separate from the
current live source until their combined binding proof is accepted.

The [final destruction/tag-value binding receipt](../analysis/physics/native-activation-constraint-destruction-binding-20261101/source-pins.json)
pins the exact three-file binding/API overlay. Its
[combined F2091 transport proof](../build/test-results/native-constraint-destruction-binding-transport-tu15/report.json)
passes 48 cases per compiler across preserved depths 0..5, comparing
4,381,945,008 persistent bytes with zero residuals/refusals. These ordinary
cases invoke no destruction/tag-value callback; the positive independent
destruction proof above remains separate. The new unknown child-link owners
are counted and throw. This overlay is queued for the integration owner.

Native `230A60`/`230AE0`
[qualification](../build/test-results/native-constraint-handle-clear-tu15/report.json)
passes 256 clear/presence compositions per compiler, comparing
23,034,108,160 persistent bytes plus ordered accesses, FP and both return
contracts. The four clear-argument combinations are declared ordinary inputs
on genuine descriptors. Their link/handle fields are all None, so positive
clears/presence and the actual `22FEA0` caller remain open.

`22FEA0` still has 775 sites and 29 callsites in the authenticated inventory.
Its remaining source dependencies include effect selection/publication and
relative-point owners; it is not replaced by an original whole-parent retry.
The [lifetime frontier inventory](../analysis/physics/native-model-constraint-lifetime-frontier-20261101/report.json)
and [child inventory](../analysis/physics/native-model-constraint-lifetime-children-20261101/report.json)
retain those original routines. Its child-link pool and tagged-value store
also have extent zero in
[the newest genuine typed pair BEFORE census](../analysis/physics/native-model-constraint-actual-pair-lifetime-census-20261101/report.json).
That census uses the protected v1465/02 archive without manufacturing a guest
ABI, row or result; its owned extraction is verified removed.

## Native constraint math and geometry frontier

The new `native_tu15_constraint_auxiliary_math` module owns actual `30BD0`,
`18BBC0`, and `1D8C80` with native scalar SSE/x87 arithmetic and ordinary DATA
interfaces. Both compilers pass [176 axis-normalization profiles](../build/test-results/native-constraint-axis-normalize-tu15/report.json)
and [352 direction-packing profiles](../build/test-results/native-constraint-direction-pack-tu15/report.json),
including declared nonfinite/tiny vectors, four rounding modes, and preserved
x87 depths 0..3 and 0..7 respectively. The tiny `30BD0` branch leaves its input
unchanged, unlike `122B60`'s fallback copy. The
[body-velocity proof](../build/test-results/native-constraint-body-velocity-tu15/report.json)
covers 280 genuine body-slot calls per compiler, including 16 actual default
vector branches and 25,504,520,024 persistent bytes. It also checks read order,
12-byte vector output, FP and the private-output return contract.

Native `22E210` [basis composition](../build/test-results/native-constraint-point-basis-tu15/report.json)
passes 320 actual original/native calls per compiler, with 640 native `30BD0`
normalizations and 32 alternate object-axis choices. Its
[aggregate](../build/test-results/native-constraint-point-basis-tu15/actual-call-aggregate.json)
distinguishes these calls from the forty island/rounding test invocations.
Both compilers match 28,876,701,504 persistent bytes plus the 36-byte caller
output, ordered operands, FP, RET8, and all preserved extended values.

The full 406-site native `22E4F0`
[geometry composition](../build/test-results/native-constraint-geometry-frame-tu15/report.json)
passes 512 actual calls per compiler. These cover 352 simple-axis paths,
128 alternate-axis branches and 160 measured-bounds paths scanning 352 genuine
matching local rows, with 1,024 normalizations. Both compilers match
46,068,216,320 persistent bytes plus 56 bytes of caller outputs, exact ordered
accesses, FP, RET32 and preserved extended values. Its
[aggregate](../build/test-results/native-constraint-geometry-frame-tu15/actual-call-aggregate.json)
records every evidence hash. Retained-node frames and paired measured-basis
selection execute zero times and remain open. The complete source algorithm
preserves its different first/second projection indices; no matrix convention
or additional contact row is inferred.

The geometry ABI is `EAX=normal_out`, `EDI=position_out`, followed by stack
arguments `{instance, constraint_datum, first_body_slot, first_axis_out,
second_axis_out, first_extent_out, second_extent_out, flag}`. In particular,
its second stack argument is the constraint datum. Native kernels receive
ordinary typed DATA, never captured volatile registers or postimages.
`22FEA0` parent composition and production selection remain separate work.

The [math/geometry cleanup receipt](../analysis/physics/native-constraint-native-math-geometry-20261101/cleanup-verification.json)
verifies all six completed private owners are absent, recording 3,237,711,872
parent allocated bytes removed. Nested compiler trees had already been
removed by their official runners. No large artifact is retained.

## Native runtime parameter and link owners

The new `native_tu15_constraint_runtime_parameters` module owns actual
`17DEC0`, `18D4B0`, and `18D510`. The
[parameter proof](../build/test-results/native-constraint-runtime-parameters-tu15/report.json)
passes 1,120 original/native calls per compiler, including 832 genuine valid
handles, 416 position updates, 416 bounds updates, and 672 actual frame lists
with 1,056 frame-node copies. It matches 101,628,427,616 persistent bytes,
ordered reads/writes, FP, and AL/RET12. Optional argument masks are declared
on existing handles, and the copied frame comes from genuine retained frame
DATA. No input row is manufactured. An initial physical harness incorrectly
cleared ES; preserving the original ELF segment context fixed the probe
without a semantic-kernel edit.

The [link proof](../build/test-results/native-constraint-runtime-links-tu15/report.json)
passes 232 creation/flag compositions per compiler: 208 actual `18D4B0`
allocations through native `16E6A0`, followed by `18D510`, and 24 None guards.
Both compilers match 21,059,437,224 persistent bytes, ordered operands, FP,
created datum and RET4. New rows are created by the actual original/native
algorithms from genuine handle arguments. The first smoke report had equal
final memory/FP/returns but omitted REP STOS clears from its original access
ledger; adding those authenticated source operands fixed the harness only.
Allocator saturation and overlap/fault/mutable-observer profiles remain open.
Both reports have hashed actual-call aggregates and removed private owners.

New `22FEA0` parent composition is under qualification. Effect selection
`22EEC0`, publication `18C440`, and handle creation `178E30` are explicit typed
services. Missing services count and throw; there is no whole-parent original
fallback. Source-private request padding remains caller-owned ordinary DATA.
The runtime frame's scale word can be unwritten before `17DEC0` copies and
then unconditionally overwrites it with one; no guest private bytes are
invented or supplied as an outcome to hide this boundary.

## Bounded native22FEA0 reference binding

The [bounded parent proof](../build/test-results/native-constraint-runtime-update-bounded-tu15/report.json)
passes 256 genuine existing-instance original/native calls per compiler,
including 768 counter updates and 23,034,108,160 persistent bytes, exact
ordered accesses, FP, RET12 and preserved extended values. Every actual input
has kind1 and flag11 zero. No state transition, effect service, handle
creation, or runtime geometry branch executes. Implementing the 775-site
algorithm does not qualify those zero-positive parent branches.

The [qualified input join](../analysis/physics/native-constraint-runtime-update-20261101/qualified-kind1-counter-profile.json)
matches these exact datums and unchanged authenticated BEFORE contexts against
original/native clear-and-presence evidence. It proves all four link/handle
fields `30/34/38/3C` are None. The preliminary admission diagnostic excluded
kind1 and admitted zero cases; that classifier is superseded by this explicit
join and corrected production seam, without additional original executions.

The [final two-file stage](../analysis/physics/native-activation-constraint-runtime-kind1-binding-20261101/source-pins.json)
wires the qualified parent through both native removal and creation reference
services. It requires 512 caller-owned DATA bytes and admits only kind1,
flag11zero, and all four None fields. Other native parent profiles count and
throw; no whole-parent backend runs. Its
[transport qualification](../build/test-results/native-constraint-runtime-kind1-binding-transport-tu15/report.json)
passes 48 cases per compiler across incoming depths0..5, comparing
4,381,945,008 persistent bytes. Runtime-parent calls and every new refusal
are zero in that ordinary transport cohort. Its live reference-release
activation remains separate from direct component proof.

The [runtime-parent cleanup receipt](../analysis/physics/native-constraint-runtime-update-20261101/cleanup-verification.json)
verifies all five completed owners removed, including the preliminary profile
cohort and failed negative-control harness. No large artifact is retained.
The [runtime-parameter cleanup receipt](../analysis/physics/native-constraint-runtime-parameters-20261101/cleanup-verification.json)
separately verifies its five owners removed. Protected source references and
required compressed BEFORE archives remain untouched.

## Native constraint effect location

New `native_tu15_constraint_effect_location` owns actual `11E600`, `14CDB0`,
and `11E850`. Its [physical composition](../build/test-results/native-constraint-effect-location-tu15/report.json)
passes 1,008 cases per compiler: 12,960 BSP node visits and 672 outside-BSP
results, comparing 91,679,890,032 persistent bytes, exact read/write order,
FP, 8-byte location/optional2-byte output and both return contracts. Inputs
are genuine retained object points and declared zero/large vector arguments.
No BSP, material, physics, or contact row changes. Both compilers reject an
incorrect branch-direction negative control.

The [aggregate](../build/test-results/native-constraint-effect-location-tu15/actual-call-aggregate.json)
records every evidence hash. Positive location-key selection, its halfspace
branch, and no-active-BSP execute zero times and remain explicit. This proof
does not activate a selector in production. Its
[cleanup receipt](../analysis/physics/native-constraint-effect-location-20261101/cleanup-verification.json)
verifies both completed private owners removed with no large artifact retained.

## Native constraint effect selection and camera queries

`native_tu15_constraint_effect_selector` owns `22EEC0/22EFD0/18B1D0`
and the native event/material/radius/proximity children. Its qualified CPP is
`f5df452cf91ace6bb488ea0af9316bc73389597aa3a1f6d01d093a3aa5877859`;
the HPP is `742cd21e38fd80c097b0b3ebde0d94f9dc278185045f1348f9bf9fbe90fcfa86`.
The unchanged location CPP/HPP remain
`5ec2454898db6b87eec3c99bb6abede2fbb55642c523b236ecc8fee650517138` /
`48f8df0bb2e58e7a9540e4f6edd9f6c64d48a5bb3dfa533b9c82f17ddd4671b5`.
Original instructions execute only in the physical oracle. Native C++ decodes
authenticated read-only classification-table DATA into native branches.

The [selector composition](../build/test-results/native-constraint-effect-selector-tu15/report.json)
passes 768 calls per compiler, exact access order, 24-byte output, FP, RET40,
preserved incoming extended values and 69,102,324,480 persistent bytes. All
768 inputs refuse by camera distance because their genuine source-view slots
are None. Event queries, selected effect rows and positive camera tests are
zero in this full-selector cohort.

The [authored query proof](../build/test-results/native-constraint-effect-queries-tu15/report.json)
separately passes 1,536 direct native/original `18AF20/18B0C0` compositions
per compiler, all with positive original row returns. It compares
139,125,429,760 persistent bytes, ordered accesses, FP and pointer DATA
returns. The [declared scalar controls](../build/test-results/native-constraint-effect-query-controls-tu15/report.json)
add 816 calls per compiler: 480 positive selections and 1,116 event fallback
steps. They use declared enum/material arguments and unchanged authored
parent chains; they are not manufactured physics contacts.

The [positive camera composition](../build/test-results/native-constraint-effect-proximity-entry-tu15/report.json)
reuses the user's protected original Scorpion camera RAM reference. Its
64 MiB SHA is `905fc8afb1215727f2319aa0f12775725de4c97cc10fa4cf21e20ab9eadfea97`.
An offline page-table walk finds physical directory `F000` and authenticates
all 3,611,948 original text bytes. Only five unchanged data pages enter this
read-closure proof. Both compilers pass 504 `18B160 -> 1755B0` compositions,
336 near / 168 far results, 504 actual camera tests, exact ordered reads,
FP and AL return. Points and classification/scaled-mode arguments are
declared ordinary inputs. This is original TU1.5 camera DATA, not current MCC
view backing or a captured caller ABI. The adjacent GDB snapshots represent
a different phase. No view registration is manufactured.

The [read-only instance census](../analysis/physics/native-constraint-source-view-reference-census-20261101/original-instance-census.json)
finds no occupied runtime instances or instance lists in that original RAM;
its 78 occupied components also have zero authored `+80` counts. Its world
island-step listener count `+A0` is zero. These facts do not qualify a positive
whole selector, authored constraint, or island-step listener path.

The [queued binding seam](../analysis/physics/native-activation-constraint-effect-selector-binding-20261101/source-pins.json)
composes the selector through native runtime-update services with 512 bytes
of caller-owned scratch. Its [transport proof](../build/test-results/native-constraint-effect-selector-binding-transport-tu15/report.json)
passes 48 cases per compiler, depths0..5 and 4,381,945,008 persistent bytes.
Runtime-parent/selector calls remain zero in this transport cohort. Production
admission stays bounded to kind1, flag11zero and four None handle/link fields;
unsupported parent profiles count and throw.

Audio `18C440` and Effects `178E30` remain separate required typed service
owners. Existing native effect-sound presentation and first-person raw sound
submission do not establish this producer's complete request/FP contract.
No GP-shaped sound wrapper, invented private request padding, discarded
service effect or whole-parent backend is introduced.

The [selector aggregate](../analysis/physics/native-constraint-effect-selection-20261101/qualified-aggregate.json)
records exact case counts and evidence hashes. Its [cleanup receipt](../analysis/physics/native-constraint-effect-selection-20261101/cleanup-verification.json)
verifies eight completed private owners absent and 3,041,296,384 recorded
parent scratch allocation bytes removed. This cumulative allocation figure
is not an isolated free-space delta. No new large artifact remains; the
borrowed protected original RAM and other required reference inputs remain
unchanged.

## Genuine multiplayer authored-row creation input

The later visible multiplayer09 census retained one eligible frame724
component BEFORE archive, SHA
`0fd26e268228c50ea01c07a5fea7b68a1e06c515eac2ec90562f303c5873caac`
(13,613,721 compressed bytes). The [read-only identity census](../analysis/physics/native-constraint-source-view-reference-census-20261101/actual09-component-census.json)
independently authenticates all 4,414 banks / 114,953,457 bytes. Actual component
datum `E16B0003` has one type1/index0 authored record, a None runtime handle,
two genuine physical bodies, and matching component/body-slot properties.
Its object datum is `E2A10032`, definition `E5834676`, and object+AA signed
type1. The run's selected Warthog definition matches. No corpse/ragdoll
identity is inferred.

The [complete native parent composition](../build/test-results/native-body-constraint-actual09-classification-tu15/report.json)
passes GCC and Clang, 16 declared formal `22D460(component_datum)` calls per
compiler: four rounding modes and preserved depths0..3. Only depth0 was
captured; the others are declared numerical input profiles. Each actual
authored row passes native lookup/capacity, allocates and constructs one
instance, allocates an instance list, appends and publishes the handle, and
finishes native normalization/reference release. Every case matches all
114,953,457 persistent bytes, the 337-event ordered ledger, FP, RET4 and
preserved incoming extended values. Unknown removal/runtime owners are zero.
This authentic frame-level input is not a captured original `22D460` caller
ABI, nor proof of positive live native invocation.

The initial positive proof matched final memory and FP but exposed one
missing `1F3460` read of the authenticated definition classification table.
The [qualified minimal source stage](../analysis/physics/native-body-constraint-positive-actual09-20261101/source-pins.json)
adds that exact DATA read and validates its source target before entering
the native enum branch. It never executes the target as code. The staged
parent CPP SHA is
`9f205511188175bb15b7c18480ac932cdf444810376e1ccdc36806f968d7e95d`.
Both compilers reject a wrong positive-loop gate. Production application,
actual caller activation, other authored kinds, saturated removal, positive
distance points and fault/alias domains remain separate gates.

The [positive-input cleanup receipt](../analysis/physics/native-body-constraint-positive-actual09-20261101/cleanup-verification.json)
verifies both completed probe owners removed, including the initial strict
ledger failure: 770,711,552 cumulative parent allocation bytes, no new large
artifact retained. The required compressed BEFORE archive remains protected.

The subsequent [native listener composition](../build/test-results/native-body-activation-actual09-scope-tu15/report.json)
invokes genuine body slots0 and1 sequentially through declared original
`2816A0` formal arguments. Both compilers pass 16 profiles: 32 listener/body
updates, 16 creations/publications, then the second body correctly observes
the published handle. It compares the 386-event complete ordered ledger,
all 114,953,457 bytes per profile and FP/RET/live extended values. Object wake
queries execute32 times; header marking remains zero on these already awake
headers.

The [authored lifetime aggregate](../analysis/physics/native-constraint-authored-lifetime-actual09-20261101/qualified-aggregate.json)
then composes that listener sequence with original/native `22D0A0` removal.
Each process independently consumes its own newly computed published handle.
No result crosses from the original execution into native. GCC and Clang each
pass 16 profiles, positively exercising 16 authored global-row/reference
releases, list removals/deletions and instance deletions. The 448-event ordered
ledger, FP/RET/live extended values and 1,839,255,312 persistent compared bytes
per compiler match. A wrong native authored-reference clear is rejected.
Required handle/runtime child callbacks remain explicit refusals and execute
zero times here. This proves the lifecycle on source-produced state; actual
live invocation and other authored/runtime profiles remain separate.

Root applied the exact qualified parent SHA to production after the isolated
parent proof. The lifecycle cohort independently pins that same current SHA.
Its [combined cleanup receipt](../analysis/physics/native-constraint-authored-lifetime-actual09-20261101/cleanup-verification.json)
supersedes the two-owner subtotal above: all six completed probe owners are
absent, 2,312,798,208 cumulative measured parent allocation bytes removed,
including the failed reporting-scope compiler harness. The additional owned
read-only census extraction is absent; its allocation size was not measured.
No new large artifact remains beyond the existing required protected archive.

## Native World merge activation frontier

The [authenticated live-owner audit](../analysis/physics/native-activation-live-frontier-20261101/report.json)
identifies the real World wake path: deferred `3190D0` island merge calls
`316D40` for either inactive island, then `322660` dispatches the actual body
listener `2816A0 ->22D460`. Existing object/flip binding counters alone do not
cover it. The separately reached `2815F0` is an enqueue/contact callback, not
the activation listener. `3187C0` regroup is a separate 482-site partition and
lifetime owner and remains open.

The [actual09 queue census](../analysis/physics/native-activation-live-frontier-20261101/actual09-queues.json)
finds one genuine pending merge pair between the two Warthog islands; both
are active. Source `3190D0` therefore correctly performs no wake on this pair.
Its [contact-record census](../analysis/physics/native-activation-live-frontier-20261101/actual09-contact-record-census.json)
finds distinct authored record keys39/40, with no duplicate contact. No island
flags, bodies, contacts or registration rows are manufactured to hit a gate.

Three new native units own `3190D0`, sorted contact-record merge `318DE0`,
temporary constructors/destructors `317A40/317AD0`, and island lifetime
`322100/3213A0`. Their [qualified pins](../analysis/physics/native-island-merge-native-children-actual09-20261101/source-pins.json)
record exact CPP hashes `5a2808b5...` (merge), `8eb44844...` (contact merge),
and `e71f76f9...` (lifetime), plus their complete HPP hashes and reports.
The native algorithms use typed services and native allocator kernels.
Duplicate cleanup, unsupported allocator and activation requirements never
replay the native parent.

The [coherent native-child aggregate](../analysis/physics/native-island-merge-native-children-actual09-20261101/qualified-aggregate.json)
passes GCC and Clang, 48 profiles each: actual queued order plus declared
swapped/identical formal arguments, four rounding modes and depths0..3.
It compares 5,517,765,936 persistent bytes per compiler, strict parent access
order, child DATA contracts, FP, surviving-island return, RET8 and live extended
values. It exercises32 native contact merges/reserves/body moves/temporary
lifetimes/island disposals and16 physical constraint moves per compiler.
All remaining child CPU execution and unknown-owner counters are zero.
Independent native contact/lifetime proofs qualify their own ordered access
and child contracts:32 sorted-merge profiles and80 lifetime profiles per
compiler. Positive duplicate cleanup, action/extra-row movement and slow-arena
fault/unwind domains remain open.

The [private shared-service stage](../analysis/physics/native-shared-island-activation-binding-20261101/source-pins.json)
retains one activation-service builder for existing B9800 setters and the new
typed `native_tu15_activate_island_with_residuals` operation. Its unchanged
setter transport passes48 cases per compiler. Its [direct activation proof](../analysis/physics/native-shared-island-activation-binding-20261101/direct-qualification-aggregate.json)
passes24 original/native calls per compiler on genuine inactive Gemini
islands, depths0..5:24 native wakes,12 agent/manager creations,24 body-update
calls and zero residual owners, comparing2,190,972,504 persistent bytes per
compiler and FP. Authored rows are zero in this direct-wake cohort; genuine
authored-row creation was qualified separately above. No velocity packet or
guest postimage stands in for a World activation call.

Production World callback selection and visible native merge/wake receipts
remain separate from these component/private binding results. Retained DATA
must remain above any actual residual frame; the existing World call stack
near its scratch end cannot safely share lower retained activation storage
without a dedicated lower service frame.

The [merge-family cleanup receipt](../analysis/physics/native-island-merge-native-children-actual09-20261101/cleanup-verification.json)
verifies13 completed private owners absent, including failed compiler/fixture
harnesses, and5,148,389,376 cumulative measured parent allocation bytes removed.
No new large artifact remains; required original/MCC BEFORE inputs are intact.

## Qualified private World merge binding

The [rebased World merge stage](../analysis/physics/native-world-island-merge-binding-stage-v2-20261101/source-pins.json)
qualifies eight private files over the latest generic-manager guard. It replaces
World's actual deferred3190D0 callback with the native merge/contact/lifetime
family and the sole shared native activation-service builder. Retained merge
DATA occupies aligned scratch below the existing contact packets. Any remaining
legacy child reached during this scope is counted and refused before executor
invocation, including nested point/destructor requirements beneath a known
cleanup owner. The activation remaining-owner callback also refuses; it never
synthesizes a B9800/F2091 transport packet or guest postimage.

Both GCC and Clang compile the coherent100-CPP closure exactly once and pass
three [World model cohorts](../analysis/physics/native-world-island-merge-binding-stage-v2-20261101/qualified-aggregate.json):
the default model preserves all ten generic-manager profiles, the genuine02
factory prefix remains green, and the genuine09 queued-pair prefix reaches
source31979C after a completed native operation drain. The old general model's
opaque3190D0 fixture had inconsistent list/body island membership; its invalid
pending merge and expected opaque callback were removed, with no claim that it
became a whole-merge model. Independent48-profile physical parent proof per
compiler supplies exact positive merge coverage.

The genuine09 World prefix imports all4414 authenticated BEFORE banks
(114,953,457 bytes), declares an ordinary owned adapter frame and previous-step
DATA, and stops through an explicit store observer after31979C. It produces
one native merge/contact merge/body move/island disposal and90 source-labeled
stores, with zero original-service invocations, duplicates or refusals. Both
actual source islands are active, so its native activation count is correctly
zero. It does not compare the entire original World routine or demonstrate a
live invocation. Positive World wake remains an explicit acceptance gate.

The exact staged Getter and gate fragments pass both compilers with156 distinct
scope sentinels. All35 general creation/lifetime/runtime counters, five selector
counters and the gameplay-policy counter now include World, velocity and flip
activity; object-velocity-specific counters remain scoped. Twelve new flat World
fields appear in both JSON serializers. Optional
`HALO2_REQUIRE_NATIVE_WORLD_MERGE` and `HALO2_REQUIRE_NATIVE_WORLD_ACTIVATION`
gates reject missing work, remaining owners and refusals. Root must still apply
the qualified stage, compile the full LiveWorld/acceptance cohort and validate
visible multiplayer using the actual vehicle camera.

The [cleanup receipt](../analysis/physics/native-world-island-merge-binding-stage-v2-20261101/cleanup-verification.json)
verifies all four completed owned scratch trees absent, including the stale
model-expectation failure, with763,932,672 cumulative measured parent allocation
bytes removed. This is not an isolated filesystem free-space delta. No new large
artifact is retained; protected reference archives remain intact.

## Connectivity graph and island metadata frontier

The remaining482-site3187C0 parent is a genuine connectivity repartitioner.
Its [authenticated dependency inventory](../analysis/physics/native-island-regroup-dependencies-20261101/report.json)
and [actual09 operand census](../analysis/physics/native-island-partition-actual09-census-20261101/report.json)
preserve phase distinctions: all seven active islands have partition flag0;
five of six inactive islands retain flag1. The parent visits active islands.
No flag, body list, row or caller state was altered by the census.

New native `island_partition_graph` owns37C170/37C190/37C250 (140 sites).
The [both-compiler physical proof](../analysis/physics/native-island-partition-graph-v2-20261101/source-pins.json)
passes192 explicitly declared valid-forest profiles per compiler,6,592 strict
operand events and3,145,728 persistent bytes. It rejects wrong root selection
and missing edge compression. This closes source root-index union and both
compression phases; it is not a captured live positive repartition case.

New native `island_partition_metadata` owns3200B0/320100/320130/323420
(48 sites). Its [both-compiler proof](../analysis/physics/native-island-partition-metadata-20261101/source-pins.json)
passes256 declared initialized-info profiles per compiler,2,000 strict operand
events and4,194,304 persistent bytes, rejecting unsigned maximum comparison
and wrong removal sign. Authenticated island table41F080 selects3200B0 at+8
and320100 at+C. The [320100 read/escape audit](../analysis/physics/native-island-partition-metadata-20261101/info-read-escape-audit.json)
confirms reads only info+4/+8/+C, no first-word read, no packet escape, no child
owner, and stores only island+C/+10/+18. Actual point-parent packet provenance
and receiver authentication remain separate before live composition.

The complete37C2D0/37C460/37C3D0 graph build/group extraction/lifetime
module now passes the [coherent all-native child proof](../analysis/physics/native-island-partition-native-children-qualified-20261101/source-pins.json)
with GCC and Clang:32 profiles per compiler,64 unions/group allocations/frees,
96 reserves and32 group-header releases,3,678,510,624 persistent bytes and5,840
strict parent events. Declared topology is kept separate from authentic09 heap
backing. No native-side original CPU child remains in the admitted primary-pool
profile. Caller-owned parent/root temporary arrays retain their own lifetimes.

The [independent raw-family proof](../analysis/physics/native-raw-allocator-actual09-qualified-20261101/source-pins.json)
closes149BF0/149C40/233160/2331A0 with native233250/233420:112 profiles per
compiler,96 tagged allocations/frees and16 null releases,12,874,787,184 persistent
bytes per compiler. Fixed/secondary/lazy-default and unsupported large/exhausted
profiles remain explicit requirements. Their paths are not treated as closed.

Native68-site3201B0 now passes [authentic1470 direct-domain proof](../analysis/physics/native-island-constraint-detach-actual1470-qualified-20261101/source-pins.json)
with native325E90 info filling and320100 metadata removal:16 profiles per
compiler,1,436,727,696 persistent bytes,336 strict events, FP/RET4/live80 and
both mutations rejected. Actual input pointcount1/ref2 remains unchanged before
the formal call. The kernel naturally sets the partition flag and releases
ref2 to ref1; zero-reference destruction is unexercised. Actual point-parent
aftercount0 composition is owned and qualified separately by the contact work.

The full normal-completion482-site3187C0 parent now passes both compilers,
with native graph, allocation, constructor, metadata and temporary lifetimes.
The [six family receipts](../analysis/physics/native-island-regroup-parent-qualified-20261101/qualification.json)
include the209-site edge collector,202-site4/8-byte temporary family,49-site
island constructor and sole production native service builder. The builder
passes72 profiles per compiler across4 FP policies and actual depth0..5:
8,276,648,904 persistent bytes,7,176 strict parent operands, FP/RET/live80,
and two rejected mutants per compiler. Its actual09 World inputs cover168
natural flag skips; explicit coherent private1/6-body controls cover24 fully
connected phases,120 new islands and moved bodies, and144 paired native group
allocations/releases and temporary constructions/destructions. These split
controls are declared inputs, not genuine multiplayer repartitions.

The edge collector independently exercises the authentic09 constraint pair;
action enumeration and positive dynamic/dynamic contact collection remain open.
Actual1470 point removal composes3201B0 after pointcount0 with ref2 to ref1 in
the contact pipeline. Its receiver is inactive: setting its partition flag
does not make it an active input to3187C0. Genuine wake followed by active
repartition, positive row redistribution, empty-island and callback fault/unwind
profiles remain separate checks. Unknown owners are counted and refused; no
native parent replays through an original CPU executor.

Later [genuine04 active-positive proof](../analysis/physics/native-activation-genuine-frontiers-qualified-20261102/source-pins.json)
now exercises the unchanged captured World's existing partition flags and real
dynamic/dynamic contact pair:24 profiles per compiler,2,155,091,544 persistent
bytes,552 strict parent events,48 graph builds/fully-connected phases and144
paired native temporary lifetimes. No contact rows, flags or body membership
are manufactured. The ordinary3187C0 formal is declared separately from its
uncaptured caller. This domain creates no new islands. An independent genuine
1470 point-removal→declared wake→regroup sequence also matches every89,795,481
captured byte and FP after each phase,12 profiles per compiler; its natural
flag and native wake reach a fully-connected graph without remaining owners.

The same actual-frontier receipt includes selected22E1B0 input
`instance1004886C/datumED76000D`: both compilers match fast ref5→4, every captured
bank, ordered operations, FP and return. Runtime22FEA0 is unselected. Native
public31D630/31F840 info leaves are now qualified separately on74/111 profiles
per compiler; the [narrow info service stage](../analysis/physics/native-world-constraint-info-binding-stage-20261102/source-pins.json)
retains unknown-owner refusal. Its genuine1471/04 selected31D630 input also
matches all114,953,457 bytes, strict stores, FP scalar policy, RET4 and typed
info identity. Actual live x87 depth was not captured by that native API; the
integer-only oracle's empty tag stack is explicitly declared.

The [reverse process stage](../analysis/physics/native-reversed-contact-process-binding-stage-20261102/source-pins.json)
owns complete public2F7480 and2F7FB0,39 sites each. Both compilers pass48
whole-public no-contact controls with real original/native children,80
independent positive suffix vector controls,48 same-production-binding
profiles and the coherent121CPP World models. The suffix reads the actual16B
461960 mask and preserves signed counts and current header/data rereads.
Generated-contact public positives remain open. Remaining304350 child counts
and diagnostics are preserved; this stage is not a full native child-closure
claim.

The private [World regroup stage](../analysis/physics/native-world-island-regroup-binding-stage-v2-20261101/source-pins.json)
replaces the actual typed World phase with that same native service builder,
preserves point07/06 and keeps distinct native-phase and positive-repartition
gates. Its callback-only default fixture disables its prior opaque regroup
phase because its empty-island lifetimes are inconsistent; no synthetic live
flags or membership are used to claim coverage. Production/live qualification
must be checked from the stage receipt before applying it.

Completed graph and metadata private owners are verified absent:20,484,096
and20,471,808 measured parent allocation bytes removed. An earlier interrupted
negative-control owner was independently retired after preserving its small
failure results (24,461,312 bytes); the source operand-order defect and unbounded
negative-control loop were corrected before the green cohort. No new large
artifact remains and the protected genuine09 archive is unchanged.

## Authored function and conveyor dependency

The separate [native function extension receipt](../analysis/physics/native-function-evaluation-extended-qualified-20261102/source-pins.json)
qualifies all eleven authored kinds and all492 sites of the13DE70 parent in
its admitted normal numerical domain. Both GCC and Clang pass1,476 whole-parent
profiles,3,205,384,920 compared declared bytes,138,358 original instructions,
308 native power calls and780 control-point queries. The original public
parent and its unchanged children execute independently as the physical oracle.
No original result, register file, control frame or instruction loop supplies
the native result. The input reference is exactly8B{signed count,blob pointer},
which the conveyor caller obtains at its20B authored row+0C.

Native17F8A0 transition and17F790 periodic curves additionally pass640 profiles
per compiler with exact result bits, ordered semantic reads, FP, returns and
preserved x87 values. Their finite remainder uses real native FPREM. The
original CRT zero-numerator owner331F68 was separately authenticated: native
FXAM, operand removal and FLDZ preserve its observable condition flags. The
13E9A0 control-point resolver has one shared native owner. Native power uses
FYL2X, F2XM1 and FSCALE with the original arithmetic and rounding order.

The extended owner also repeats all504 bounded common-clamp/early-return
profiles, including signed zeros, denormals, NaN payloads, infinities and four
SSE policies. The previously qualified kind0/1 module remains unchanged for
the independently frozen contact/conveyor composition. Component controls
cover authored second layers, ramp clamps and positive/negative integer
powers. Explicitly declared control-point coordinates select both source
negative-power lower clamps; these are component DATA controls, not captured
MCC rows or live multiplayer inputs.

This is not complete function parity over every numerical environment.
Unmasked traps, nonfinite/domain/range CRT publication, alternate-CW power
precision publication, source stack faults, memory-fault prefixes and mutable
read observers remain separate. Ordinary positive power admits preserved x87
depth through5; the negative-base integer path needs four temporary values and
admits depth through4. The actual power call currently requires CW027F;
source guard shortcuts retain other masked profiles. Unknown domains count and
throw instead of replaying the parent through an original executor. Same-host
physical arithmetic comparisons also leave PentiumIII transcendental
approximation identity separate.

The typed production seam can call the extended evaluator with the actual8B
reference, two float bit patterns, current FP and actual x87 depth. The three
native units are the extended evaluator, function curves and shared control
point resolver. Genuine non0/1 authored runtime use and the subsequent coherent
conveyor binding remain distinct integration evidence.

The later [whole contact-parent composition](../analysis/physics/native-function-parent-composition-qualified-20261102/source-pins.json)
passes261 complete PART cases per compiler, including44 positive authored
function cases. Every kind0..10 has four positive parent profiles and sixteen
actual native function calls per compiler. All418 PART sites,62 material sites
and26 property sites are covered, with39,046 ordered reads and3,296 stores.
The same typed production builder preserves the current caller's depth0 and
FP. Its original finite remainder child executes unmodified to its real return;
that child's internal operand ledger is separate from the composed parent
ledger and is independently covered by the640 curve profiles. No child result
is supplied to either parent. Exact point callback and whole gameplay caller
composition, followed by genuine runtime authored-material selection, remain
separate integration checks.

The same bundle records additional isolated math owners. General infinite
power classification reuses the existing native positive-exponent owner and
passes76 typed result/event/full-DATA cases per compiler. Its Source37CF1C
FNSAVE and37CF28 FRSTOR caller discards the classifier's internal FP state;
direct37D948 raw FP identity is not claimed. Native pole and precision error
policies pass96 whole3334F0 cases per compiler, including actual original TLS,
typed native errno storage, all FP words and preserved x87 values. These
policies are not yet composed into the frozen extended evaluator. A later
source audit distinguishes ordinary alternate-CW normalization from precision
publication:332192 tests the saved control word's precision-mask bit. Masked
alternate-CW admission still needs its own coherent qualification; unmasked
precision faults remain separate.

## World contact-manager factory

The [narrow World factory stage](../analysis/physics/native-world-manager-factory-binding-stage-20261102/source-pins.json)
removes the lifted3232E0 call from the native320EF0 manager service. The sole
typed builder uses the existing native pair-storage allocation service,
constructor, collector and listener kernels. Both compilers pass20 genuine
sleeping-body/B9800 contexts, with ten native factory completions and twenty
listener registrations per compiler, and reject three semantic mutations.
The source's third factory argument is unused; no guest register postimage is
introduced to carry its return value.

Both124-CPP coherent World models pass the default ten profiles, the positive
factory prefix and the genuine09 merge prefix. The factory prefix executes
the production builder directly, checks one completion/two registrations/zero
refusals, and rejects any old3232E0 residual transport. Its readonly publication
stop remains a prefix check. Nineteen exact Getter fields, both JSON outputs
and a positive live gate pass normal, unknown-owner and first-read-throw
counter checks. Body destructors and registered base-lock release remain
counted refusals; the missing-registry base-lock owner is native. The stage
preserves the immutable1472 initial source cohort and does not itself claim
Root application or visible multiplayer/frame parity.

## Native object spatial packet and chain

The [qualified spatial receipt](../analysis/physics/native-object-spatial-packet-qualified-20261102/source-pins.json)
covers whole16E290 flags, BA910 packet construction, 1D30A0 chain update and
29FFF0 row search/copy. Both GCC and Clang pass239 original physical profiles,
all160 source sites,2601 compared readonly DATA reads,707 ordered stores, five executed
fault prefixes and two rejected mutants. The identity-backed API preserves
packet aliases, late scalar argument reads, signed slots, overlapping forward
DWORD/byte copies and the outer link reread after a child changes it. DF0 is
the source copy contract; no CPU state is represented in the kernel.

Eight additional cases within that239-case corpus execute the real original
BA910 first, then an ordinary original1D30A0 public entry consuming the packet
it computed. The native side independently builds a disjoint20-byte host POD
and passes that POD to the native chain. Original packet contents are used only
for comparison. The original records2634 reads; the mutable-key observer case
compares writes, the mutation phase and full banks, while the other cases
compare readonly read ledgers. Full persistent banks and FP match; no result, GP postimage or
guest stack is supplied to the native implementation. This declared service
sequence is separate from the actual1472 BA570 deletion caller and visible
multiplayer acceptance, which the live owner composes independently.

The production seam uses `native_tu15_build_object_spatial_packet`, followed by
the host-packet overload of `native_tu15_update_object_spatial_chain`. It has no
numeric private packet allocation, external callees or FP arithmetic. The
qualified CPP hash is40ba5fcd873bd7291e41af353a2b35b6488bc5eba3142f0cb0253c0724fbddd9;
the HPP hash is321ba2c89667a93f3dac90d9a2b3a0d5d38df7b85c001d48f3e784c975ab4c75.

## Native pair-creation island wake seam

The [wake-only staged binding](../analysis/physics/native-world-pair-island-wake-stage-v2-20261102/source-pins.json)
replaces the320EF0 service's old316D40 call with the same shared native
activation builder used by World merge. It owns a distinct low retained DATA
segment, counts and refuses named unknown children, and contributes all shared
lifecycle activity to the existing World activation totals. Three additional
fields report pair-wake attempts, completions and refusals through the Getter,
both JSON serializers and a positive gate that rejects old316D40 execution.
The staged diff is based on the exact frozen1472/02 cohort; it does not import
unrelated working-tree factory changes or claim Root application.

Both compilers pass32 complete original320EF0 cases each. Twenty-eight keep
the original pair context or previously declared parent controls and execute
no wake. Four cases use the same genuine child entry, body and island
identities with an explicitly declared coherent inactive-list preimage. Each
positive performs one native wake and two body updates with zero unknown
owners, matching89,738,181 persistent bytes, FP, current formal DATA, parent
read/store order and callback sequence. The inactive list is ordinary DATA;
no new constraint rows, captured wake invocation or live flags are claimed.
Other services in this offline parent proof execute their unmodified original
bodies explicitly; the wake subtree has no original fallback.

The source sequence matters:316D40 clears the island's ready byte, so320EF0
requests deferred pair publication before it can create an agent. The positive
World model therefore stops readonly at the remaining320DA0 service request
and supplies no queue result. Both130-CPP coherent World builds pass that
prefix, the default ten profiles, factory prefix and genuine09 merge prefix.
The separate counter proof checks the exact three Getter fields, two JSON
outputs and seven gate rejections. Genuine multiplayer wake selection, deeper
listener/constraint profiles and full-frame acceptance remain open.

## Pair-deletion formal DATA frontier

The existing320260 prototype omitted a real write at3203C9: on the missing
cached-record branch, the source repurposes its four-byte formal argument to
hold the second handle. The first direct original comparison caught the
missing store even though heap bytes and FP matched. A [private typed overload](../analysis/physics/native-pair-delete-formal-stage-20261102/source-pins.json)
adds `NativeTu15PairDeletionScratch` for that ordinary caller-owned DATA word;
it introduces no guest register or frame representation. The old prototype
overload keeps its previous behavior and remains unqualified.

Both compilers pass36 complete public320260 returns each with the overload,
matching3,230,427,060 persistent bytes per compiler, source formal DATA,
ordered parent operands/stores, FP and genuine child callbacks. Profiles use
actual cached/contact record identities, missing records, other-island
records, and pending rows produced by the real unchanged320DA0 before the
independent deletion comparison. No physics row or deletion outcome is
supplied to the native kernel. Two native mutants are rejected. These are
declared public boundaries using authenticated source DATA, not captured live
320260 caller ABIs.

The receipt explicitly covers188 of247 source sites and lists the remaining59
sites. Multiple-record copy/shift and further search branches need additional
source-authentic inputs. The offline child services execute real original
2F3100 and323220; production must compose their existing native World owners
and refuse every unknown nested owner before execution. Exact World builder,
coherent model, counter wiring and visible multiplayer acceptance remain open.
The private two-file patch has not changed Root's live pair-handler files.

## Pair-deletion native-child composition

The same typed pair-deletion helper now passes the36 full public-return
profiles per compiler with its selected cleanup children composed natively.
The [native-child receipt](../analysis/physics/native-pair-delete-native-children-tu15-20261102-v4/report.json)
checks that the native side executes no original child CPU service. Its
selected owners are2F3100/3051A0,323220/3232B0/31F660 and149E00/233420.
Unknown manager-update listeners, zero-reference body destructors and
registered lock owners remain explicit refusals. This closes that bounded
composition, not the59 remaining parent sites or production World binding.

A separate original3190D0 preparation produces four real contact records from
the protected09 input. The resulting
[multi-record deletion diagnostic](../analysis/physics/native-pair-delete-merged-input-tu15-20261102-first/report.json)
stops at2F82D0, a known native cleanup family needing the point-service
composition. It is retained as a diagnostic, without supplying records or
cleanup outcomes to the native parent. The genuine unmerged09 census contains
thirteen one-record contact islands; no multi-record row is manufactured.

The [private World pair-deletion stage](../analysis/physics/native-world-pair-delete-stage-20261102/source-pins.json)
now routes320260 through the same typed helper with four bytes of formal DATA
at retained scratch end minusA4. Its per-operation RAII scope rejects every
unconverted nested owner before the generic adapter touches FP, GP, stack,
the original executor or the executed-residual census. Refused owners have a
separate map and count. Both compilers pass six refusal-before-transport
controls, nested and exceptional restoration, and four subsequent outside
executions. The scoped helper also passes the same36 complete original/native
parent cases per compiler with zero old executor calls and no additional
ledger exemption:89,734,085 persistent bytes per profile, formal DATA, FP and
ordered parent operands/stores. The larger captured input is89,795,481 bytes;
the normal returned private stack is outside the persistent-byte comparison.

Both136-CPP coherent builds preserve the ten manager profiles, factory prefix,
genuine09 merge prefix and native pair-wake queue prefix. The older opaque
process-agent World fixture does not define valid deletion owners. Its
declared positive deletion control therefore uses a null agent; genuine
native cleanup children retain the independent36-case proof. A separate
unsupported descendant control verifies that no original invocation or
executed-residual entry occurs. Both seven-scalar/one-owner-map Getter and
two-JSON transports, plus the positive gate and seven rejection controls,
pass. This stage is pinned against the latest1473 source including Body and
Snapshot modules. It has no live320260 claim, and the59-site and genuine
multi-record cleanup limits remain unchanged.

## Reached reversed-list scalar destructor

The new native-only World deletion scope correctly refuses the actual next
owner2F81B0 instead of running an old backend. Its authenticated15-site body
calls the one-site2F81E0 jump to the already native2F7E90 pointer-storage
destructor, then reads the current disposing bit, size16, allocator and release
owner to free class1A. A new typed semantic wrapper reuses the sole native
2F8090 storage/release algorithm; no existing frozen cleanup module changes.
The [component receipt](../analysis/physics/native-reversed-list-agent-destructor-qualified-20261102/source-pins.json)
passes both compilers on ten declared/source-allocated object profiles,
all15 wrapper, one thunk and28 storage sites, and three independent rejected
mutants. A genuine1474 typed BEFORE capture now authenticates receiver
268950624, argument1, RET4 and actual FP/depth0. Both the core and exact
native-only binding pass four independent FP profiles on all114,429,109
captured bytes, strict read/store ordering and return identity, with two
rejected release mutants. No guest GP, afterimage or child result is supplied.
The [private World stage](../analysis/physics/native-world-reversed-list-destructor-stage-20261102/source-pins.json)
passes both136-CPP coherent builds, the original ten cleanup profiles plus a
new reversed-list profile, four counter/one-map Getter and both JSON outputs,
and six negative gate controls. The guard remains active even when this
destructor is selected outside pair deletion. Unknown owners have a separate
family refusal map; original execution stays blocked. Visible full-frame
return remains open until the new multiplayer cohort finishes.

The [readonly child-input stage](../analysis/physics/native-pair-delete-child-input-capture-stage-20261102/source-pins.json)
observes the actual generic adapter's typed owner, optional receiver,
arguments, return-pop, FP and depth before the active native-only scope
refuses. Its bounded first2F81B0/depth0 capture writes only BEFORE banks and
manifest under owned scratch. It supplies no GP state, afterimage, result or
native admission. Both interface models verify unchanged source memory,
observer forwarding and the original named refusal. No live input is inferred
from that model.

## Native body-contact update

The new [body-update component](../analysis/physics/native-contact-body-update-qualified-20261102/source-pins.json)
implements the full1D77D0 semantic algorithm with native323020/323040 point and
property getters,1D7B60 property publication,323A10 point velocity,186C30 mass
arithmetic,1E1170 body impulse and324F80 motion impulse. Genuine wake is a typed
native service using the existing315CB0 owner. Unknown virtual owners remain
named requirements; there is no original executor or CPU state in this kernel.

Both compilers pass72 complete original/native parent returns with ordered
reads/stores, all persistent banks and FP. These use declared source DATA;
one profile is a consistent one-body inactive World and exercises real native
315CB0→316D40 activation without listeners or allocations. Full production
body-listener/factory composition remains separate. The getter and motion
helpers also pass108 independent cases covering aliases, NaNs, denormals and
the required reread after linear-velocity publication. Six source mutations
across the helper/parent suites are rejected.

The parent executes243 of245 source sites. The
[authenticated predicate proof](../analysis/physics/native-contact-body-update-zero-guard-proof-20261102/report.json)
shows why1D78AF/1D78B2 cannot execute in the admitted stable masked-FP domain:
the same XMM0/XMM2 zero predicate is tested twice without changing either
operand or FP control. An ordered zero leaves at the first test; a nonzero or
unordered input cannot become an ordered zero at the second. Unmasked trap
handlers and asynchronous register mutation are outside that proof. This is
not an assertion that all245 sites executed.

The strict parent ledger caught and corrected three source-order errors:
the row ID precedes the manager table read, the first reversed normal precedes
its sign constant read, and the negative scalar branch rereads its property.
The frozen component is not wired into World yet. A genuine typed1D77D0 input,
the same production service/scratch builder, broader callback/alias/fault
domains and visible whole-frame acceptance remain open.

## Native wake factory bridge

The captured1473 activation call reaches2F8030 with real typed first/second
payloads, context, manager, FP and depth0. The
[sealed private World bridge](../analysis/physics/native-world-wake-factory-services-stage-v4-20261102/source-pins.json)
adds an optional typed native-factory service while preserving existing
activation APIs. Its supplier forwards ordinary DATA to the exact World
factory closure and composes303C10/3040D0 natively. Attempts, successful
returns and refusals have separate counters; unknown owners throw before any
original backend execution. The readonly input observer is preserved.

Both compilers pass four independent FP profiles against the unchanged
original2F8030 on all114,429,109 captured BEFORE bytes, RET0, FP, ordered
stores and semantic DATA operands. The
[qualified actual-factory report](../analysis/physics/native-world-wake-factory-services-stage-v4-20261102/actual-factory-qualified-report.json)
records the correct actual Wake source and retains the raw report with its
inherited template label corrected separately. No GP state, child result or
expected afterimage supplies a native result. The same staged lambda is the
qualified production supplier.

Two instruction-dispatch table fetches in18502E/185035 are explicitly outside
the semantic DATA ledger. The original still executes both instructions.
The [dispatch qualification](../analysis/physics/native-world-wake-factory-services-stage-v4-20261102/dispatch-classification-qualification.json)
authenticates the complete8-byte target and24-byte selector tables against
both the captured banks and TU1.5 XBE, plus the complete original opcodes.
It proves the native AL classification for the entire32-bit input domain;
both compiled switches also pass all256 byte values and five wide values.
No physics or callback DATA read is waived. The earlier raw-ledger failure
is retained alongside this narrowly justified comparison.

Both134-CPP coherent World builds pass the default ten manager profiles,
factory prefix, genuine09 merge prefix and native pair-wake queue prefix.
The old/new-empty-supplier APIs pass32 whole-caller profiles per compiler.
The separate counter proof checks four Getter fields, both JSON outputs,
the positive gate and seven gate rejections. A positive supplier is directly
proved with the actual captured factory input; genuine shared-wake selection
and whole-frame visible multiplayer acceptance remain separate pending work.
The sealed stage is a narrow delta and must preserve the newer qualified
WrappedFactory and PairDiscrete branches when integrated.

## Remaining integration and storage

Native dependency closure still requires the applicable manager/filter,
shape-class/factory, motion/discrete-query, body-policy lifetime, body-listener
and destructor owners to be composed natively. Callback mutation/fault/alias
domains and live object/network wake behavior remain separate acceptance.
Working-tree wiring alone does not close them. Visible multiplayer must record
positive native activation/listener work and pass the actual vehicle camera,
host/peer and strict original-frame checks.

All compiler products, original ELFs, mutable bank copies and private control
frames live under `tools/run_test.py` scratch; CMake uses `tools/run_tests.py`.
The completed runs retain bounded reports, commands, source hashes and cleanup
receipts. Protected original inputs remain reference assets. No new large
artifact is retained by these component proofs.

The [final flip/constraint probe cleanup verification](../analysis/physics/native-vehicle-flip-live-handoff-20261101/all-flip-constraint-probe-cleanup-verification.json)
checks 16 completed owners, including superseded red harness runs. Every
private owner is absent, with 8,087,474,176 recorded parent scratch bytes
removed and no large component artifacts retained. Child compiler trees had
already been removed by their official runners and are excluded from this
allocation total. Required original reference archives remain unchanged.

The authored-function extension's [cleanup receipt](../analysis/physics/native-function-evaluation-extended-qualified-20261102/cleanup-verification.json)
verifies13 additional disjoint owners absent, including superseded diagnostic
cohorts, with243,408,896 measured parent allocation bytes removed. The cumulative
activation/graph ledger now records66 disjoint completed owners and
11,710,783,488 measured parent bytes removed. These are deduplicated private
owner allocations, not a global filesystem free-space measurement. No new
large artifact is retained; original/reference inputs are preserved.

The later contact-parent and World-factory receipts add twelve and five
completed owners respectively. The cumulative ledger now verifies83 disjoint
owners absent and13,541,769,216 measured parent allocation bytes removed.
Small frozen source overlays and reports remain; temporary compilers, original
ELFs and duplicate bank extractions were removed.

The exact extended point callback, three spatial probes and a setup-only
masked-CW diagnostic bring the cumulative ledger to88 disjoint completed
owners absent and13,811,179,520 measured parent allocation bytes removed.
The spatial probes account for147,152,896 of those bytes. The masked-CW
diagnostic did not reach native compilation and does not broaden the frozen
function evaluator's admission. All small receipts and frozen source overlays
remain; no new large artifact was retained.

The pair-wake proof adds eight measured completed owners, all absent, with
1,992,478,720 parent allocation bytes removed. One interrupted setup-only
owner is separately verified absent but has no recorded allocation amount and
is excluded from byte totals. The cumulative measured ledger now contains96
disjoint owners and15,803,658,240 bytes; this remains a deduplicated allocation
ledger rather than a global disk free-space measurement.

Four pair-deletion probes add1,236,492,288 measured parent bytes, all removed.
The cumulative measured cleanup ledger now verifies100 disjoint owners and
17,040,150,528 bytes removed, with the separately unmeasured interrupted
pair-wake owner still excluded from byte totals. No large new artifact remains.

The later native-child and wake-factory probes add twenty measured disjoint
owners, all verified absent, with9,825,353,728 parent allocation bytes removed.
The [cumulative ledger](../analysis/physics/native-island-regroup-parent-qualified-20261101/cumulative-cleanup-ledger.json)
now verifies120 owners and26,865,504,256 bytes removed. Two setup-only failed
cohorts have no top-level measured report and are excluded from these totals.
Reports, source pins and necessary original BEFORE archives remain; there are
no new retained large artifacts from these probes. These amounts describe
removed owned allocations, not an isolated global free-space delta.

Seven later scoped pair-deletion proof owners add824,164,352 measured parent
bytes, all removed. The cumulative ledger verifies127 disjoint owners absent
and27,689,668,608 bytes removed. Early setup failures without a recorded owner
allocation are excluded. The World stage retains only small source overlays,
reports and pins; protected BEFORE archives and user-requested multiplayer
videos remain with their original owners.

Six source audits/capture-interface/destructor component owners add54,181,888
measured parent bytes, all removed. The cumulative ledger now verifies133
disjoint owners absent and27,743,850,496 bytes removed. Small reports and
immutable overlays remain; no new large reference artifact has been created.

Sixteen later body-contact and actual reversed-list/World proof owners add
2,872,209,408 measured parent bytes, all removed. The cumulative ledger now
verifies149 disjoint owners absent and30,616,059,904 bytes removed. All small
reports, source pins and required protected BEFORE references remain.

### Actual multiplayer Body input and native live seam (2026-11-02)

The protected1474/02 typed BEFORE archive `c9b82a7a03f578115b3529a2a43efd65f8ed9f54f3e280570b376ccba92bc517` contains4033 genuine banks/89,795,481 bytes, formal manager/row/skip/reversed/component/slot and actual FP/depth0. No GP, expected results or after-values were supplied. Both direct1D77D0 and its exact native-only production helper pass four original/native full returns per compiler, including the captured FP profile, all persistent bytes, ordered semantic operands/stores and FP. This captured sample selects neither impulse nor wake.

`native-world-contact-body-stage-v2-20261102` privately replaces the old1D77D0 route after the preserved readonly observer. Its low caller-owned DATA retains point velocity/impulse across a separate native315CB0 activation region. The additive body-activation export reuses the sole qualified activation/listener/constraint builder; unknown children are named, counted and refused, with no original executor. The existing World activation counters include this work, and separate Body counters expose queries, impulse requests, wake and refusals. Both137-unit World compilations retain all four prior model families and pass; thirteen scalar fields/one map, both serializers, one positive and ten negative gates pass. The former opaque Body model callback is replaced by explicitly declared valid native manager/body DATA, not a supplied result. The shared builder then passed all72 full original/native parent cases per compiler,4574 ordered reads/890 stores and three rejected semantic mutations. Independent source/spec review found no issues; the SHA-pinned receipt is separate. The sealed World manifest is `550efcc3697bb7916087d63dac0051625b8222f744fe5198cf2c0673f19dd99b`. Visible native1D77D0 multiplayer remains pending.

The earlier native2F81B0 and320260 routes now pass1474/01 visible multiplayer normal450 and grenade1200 with the actual vehicle camera and strict original-frame bytes/FP. Native destructor returns once per case without refusal; native pair deletion returns3/6 times. This does not close the59 unexercised pair-deletion sites, positive vehicle Flip, or the remaining frame-owner list.

Eleven further measured probe owners add2,575,458,304 bytes of removed parent allocations. The cumulative ledger verifies160 disjoint owners absent and33,191,518,208 bytes removed. One additional readonly-interface owner was verified absent without a recorded byte measure and is excluded from byte totals. Source, small reports, pins and protected references are preserved; no new large artifacts were retained.

### Native post-step owner and source-trigger qualification

Native282C10 plus native1CC310/BA8C0 helpers pass207 unchanged-original cases per compiler:72 full returns,18 genuine deeper-owner entry prefixes,8951 ordered reads/588 stores and246/276 parent sites. Three preinstruction RMW write-fault constructions were excluded as invalid phase models; the earlier RED diagnostics remain. They are not claimed as actual RMW fault parity.

The exact native-only production builder passes the same207 cases, with unknown parent/state owners counted and thrown without any executor interface. The private Frame stage preserves the actual typed readonly observer before invocation; all prior Frame models and FP-clear observation remain. Both9-unit Frame builds pass, and23 scalar fields/one owner map reach both JSON serializers. Main phase and optional positive-cleanup gates remain separate. Source seal3654d00c87729d5569146fcd75728961333f9c7eeec50b0692a043aad1554cbc; independent peer ef145d80eb0e9b6be771bb6e80efec58ca15d0f482b70d086d83265aa9e2088e. Live/native phase qualification is pending1475/02.

The trigger audit authenticates all five literal pending-byte references. Only282BB3/282C03 set it, when actual149B90 secondary/default allocation totals differ across collision/merge monitoring; they also set island+AE.1475/01 observed669 normal and2105 grenade direct Frame calls with pending disabled; its sampled641-skip status is not a total. No positive dirty input or cleanup is inferred.

A borrowed-span lifetime error in the unpublished capture helper was caught during independent review before publication. Corrected helper4402a231 uses optional scalar copies inside visit callbacks and synchronous bank writes inside serialization callbacks. Both compilers pass a reused-buffer/poison-after-every-callback model and full bank roundtrip; the same test rejects oldcc02. No Source physics control/admission was changed by this correction.

The1475/02 visible normal450 and grenade1200 cases now pass strict original114,363,573/114,887,921-byte comparisons plus FP and the actual vehicle camera349/290 frames. Native post-step attempts, completions, entries and pending skips balance at691/2159, with zero dirty selections, children, refusals or unknown owners. The former top-level282C10 Frame residual is absent; positive dirty cleanup remains unexercised. Common World residuals in these two cases are2E5320,2EA380 and304350. Earlier1475/00 grenade execution also reached31D93078 times, so its conditional constraint-builder frontier remains open rather than being removed from the all-native checklist.

### Isolated native component reset and next constraint builder

The native1D8470 component-reset kernel and its existing reservation/property helpers pass254 unchanged-original profiles per compiler,98 full returns, four genuine allocator-entry boundaries,7938 ordered reads and2907 stores. All190 parent sites execute. The native API uses two caller-owned4-byte result DATA slots; source-owned property changes, reference decrements, record flags and destructor order remain typed semantic operations. Actual2E4D60 is an authenticated no-op destructor for this caller. Other selected destructor owners remain named typed services. The proof does not close owned-record allocator release past its real entry, unknown body lifetime, callback mutation/alias/error domains, or actual positive live reset. Three semantic mutations are rejected. No production reset binding is published. Seal6e7324d18e3a5a540ea50ad754a4b5b92338e23a652b8a253c0d0aa750db5fc3 records the exact source and proof hashes.

The next207-site31D930 owner is authenticated together with328870,3288B0,329100,3294E0,328A00 and37C6B0. The latter is an authored approximate-atan x87 polynomial, not a memory copy or native-libm substitution. A typed readonly observer now records the actual constraint, Jacobian-input identity, caller-owned builder packet and its four current words, FP and depth before unchanged dispatch. It captures no GP, SIMD registers, expected output or after-values, and all bank serialization remains inside committed-span callback lifetimes. Both compiler interface suites and independent parent/Host reviews pass.1475/03 completed the full ordinary two-grenade1200 case with strict original114,887,921 bytes/FP and actual camera302 frames but did not reach31D930; no input archive was manufactured. A previously authorized ordinary rocket recipe is being tested separately with the same immutable source and finite owner deadline.

Five additional measured owners, including superseded reset diagnostics and the constraint-input interface, are verified absent, removing348,057,600 parent allocation bytes. The cumulative ledger now verifies177 disjoint measured owners absent and34,158,362,624 parent allocation bytes removed. These are measured owned allocations rather than a global filesystem delta. Reports, source pins, required BEFORE archives and requested multiplayer recordings are preserved; no new large reset artifact is retained.

The1475/04 rocket case then reached an actual232D20 release requirement before a full original-frame comparison; no31D930 capture or successful frame is inferred from that failure. Source audit identifies232D20 as a five-site wrapper forwarding the data pointer to current allocator virtual+C, then returning its three-argument frame. The authenticated54-site232C80 child is allocation-ledger removal with signed counts, backward search and lazy current-field rereads during word-by-word overlapping compaction. It is not a host free or a substitute for233420 pool semantics.

The new native wrapper/ledger module, its sole native-only production helper and the exact privateWorld release lambda each pass102 unchanged-original profiles per compiler,66full returns, two genuine unknown-child-entry prefixes and all59sites. Ordered reads977/stores202 and three rejected mutations qualify the declared DATA domain. Unknown virtual+C owners remain counted typed refusals before any original transport. The private World overlay also passes both141-unit/four-model compiler cohorts and exact ten-counter/map/two-serializer/gate transport models. It retains the233420 branch and its own counters. The genuine allocator-table association, actual full-bank helper/callback proof and native visible delivery remain pending; no production admission is claimed by the component seal5f9d038d40256a9ce4ef0d90c83016228e258da7111dc2398930cd493b455c70.

The first readonly pool hook had an undeclared depth identifier in its World callback context, despite passing its isolated interface model. Complete World compilation caught this before runtime. The old immutable publication/review receipts were explicitly revoked. Revision795338b3 changes only that hook argument to actual state.x87_depth and passes both complete World translation-unit compiles plus both eleven-model interfaces. Independent corrective review is GREEN. Its Host capture still changes no physics, input, heap flags or native admission.

Sixteen further measured completed probe owners, including superseded contextual compile failures, are absent, removing1,374,887,936 parent allocation bytes. The cumulative activation ledger now verifies193 disjoint measured owners and35,533,250,560 bytes removed. Source, bounded diagnostics and protected reference archives remain. MP's retired1475 coordinator independently removed1,157,173,248 allocated regular-file bytes; that separate owner is excluded from this activation-only cumulative total to avoid accidental double counting.


## November2 continued native frontier

- [x] Genuine1476/02 native-pool BEFORE archive ca502c9f... establishes allocator10061000/table46393C/current virtual+C232C80 without an allocator alias guess. SAME exact World callback plus native232D20/232C80 passes BOTH four FP profiles/all114,429,109 captured bytes,884ordered reads228stores/FP/RET12. Final Worldstage011997cb... and independent peerc000394c... are GREEN.
- [ ] Native main-heap release live positive remains open:1476/03 normal/rocket full strictOriginal+FP+actualvehiclecamera passed but selected main-heap phase0; no release claim inferred. The earlier actual refusal/reference remains protected.
- [x] Pure37C6B0 angle kernel source polynomial/native x87, BOTH339profiles/316full/23readfault prefixes/all48sites/1830reads/three rejected mutations; seal981a7943... . No FPATAN/libm, original outcomes, GP/PC emulation or result image.
- [x] Native328870 six-word schema header, BOTH89profiles/35full/54fault+observer prefixes/all21sites/326reads392stores/three mutations; seal7458c11f... . Immutable typed runtime/length public DATA and source ordering preserved.
- [ ] Native3288B0 angular-row candidate is written and physical qualification is running;329100motor/328A00limit and whole207-site31D930 parent remain unqualified. Contacts owns source3294E0 linear-row kernel. Genuine31D930 formal BEFORE input still not selected in latest cases; no rows or invocation manufactured.
- [x] Native18A2A0 reorientation core AND SAME FP-field-only binding pass BOTH251profiles/210full/41fault+observer prefixes/all30sites/1711reads494stores/three mutations. Signedzero/NaN/Inf/precision/rounding/alias/index wrapping exercised. Binding guards mutable observer/nonempty x87 before DATA access; source18A2A0 caller source-fresh FP/liveness audited. Seal d5cb3c7c... .
- [ ] Actual Host18A2A0 typed input/caller composition, production stage and visible multiplayer remain open. Readonly three-file BEFORE stagefb7de805... passes BOTH11models plus completeactual LiveWorldTU contexts, pending independent review. It captures index/forward/completed direction/currentFP/depth and calleridentities only, no GP/after/results.
- [x] Contacts native18A300 look/CD4A0 bounded prequery pipeline v2 independently reviewed:2e1b6893.../peer02d89718...; complete same-builder actualonfoot89,795,481B and seated114,429,109B profiles, both154CPP/four Worldmodels,15counter/Host/observer/context proofs. Both observer-present BAD60 paths refuse BEFORE packet writes; geometric query/exceptionalCRT remain counted unknowns.
- [x] Exact1477 nativeLook+ordinary rocket actual-source-camera SDL composite028fea76... independently reviewed30cd1bb4... . Nativekernel inputs unchanged; only two approved SDL camera-selection/diagnostic predicates differ. MP1477/00 cold fullnormal450/frag1200/rocket1200 is running; no live result yet.

The1476 owner unexpectedly ended143 before its unexpired hard deadline, preserving reports/references/sourceops. Historical SIGTERM sender remains unknown. A new owned sibling-isolation probe passed: completing or cancelling an independent run_test watcher did not signal the coordinator. MP1477 traces only its outer parent/coordinator with non-following strace-D to identify any future sender without colliding with original-oracle ptrace. No TTL/cancellation-cause claim is made.

Cumulative activation-owned cleanup now verifies203 disjoint measured owners absent/36,634,173,440 parent allocation bytes removed. The latest schema/reorientation proofs add84,066,304 bytes; no global free-space claim. Required original BEFORE archives, source snapshots, imported MCC cache and requested videos are preserved. MP1476 owner cleanup1,157,414,912 bytes is separately measured and excluded from this activation-only sum.

The next actual live frontier is16C620 camera geometry. NativeLook1477/00 correctly refused that unknown owner before completing any case; its eight official regressions passed, but no successful live native-query frame is implied. The readonly1477/01 input archive dec05828... retains89,795,481 genuine BEFORE bytes and ordinary typed mask/origin/displacement/ignore/result/FP/depth fields only. MP retired that build/source owner intentionally and saved the exact8657-file source checkpoint.

The query dependency work now includes sole native204-site camera leaves,252-site BSP surface/polygon family and106-site geometry/context family, all both-compiler qualified and composed by Contacts into its native BSP/dynamic parents. Instanced shape/code-reference lifetime94new sites and source30B890/2E9750 setup93sites are qualified separately; all four vector lanes and32 variable bytes are source-defined arithmetic, never reconstructed caller registers. MP owns the sole native30AA70 MOPP geometry traversal; its82b0... freeze has2041 component profiles/788of896sites and direct genuine-input four-FP/full-bank proof.

Source24DC90 leaves one private ray word unwritten. The fully-defined public numeric setup continues to copy that word exactly. A distinct private-lifetime specialization shares the same algorithm and omits only the unavailable ray+20 to cache+40 copy, while every native consumer receives the same memory view that refuses overlapping reads/writes of either word before delegation. The public176-profile physical requalification and meaningful guard models pass both compilers; independent peer55cc8d1a... accepts only that bounded contract. No default zero, saved control value, GP image or raw-private observer equivalence is supplied. Collector+8/+C were independently proved source-written mask/filter DATA at24DD6B/24DD94 and are not omitted.

Native24DC90 now completes a source-derived standalone entry on the genuine1477 BEFORE world: first cluster46 has160 authored instance rows; original and native traversal genuinely clip before all leaf callbacks. Both compilers/four FP profiles match all89,795,481 captured bytes,140 ordered persistent reads,8 persistent stores, FP and AL, through native constructor/setup/traversal/destructor references. Seal509622b3... links the exact source and private guard/core stages. No captured24DC90 call ABI, full16C620 caller or visible-query delivery is claimed. The new75-site provider/forwarder/collector family has87 physical profiles,52 executed sites and genuine16C000 entry prefixes; its23 accepted-instance suffix sites await native returned-child composition rather than supplied hit results.

The cumulative cleanup ledger now verifies219 disjoint completed owners absent and37,913,419,776 parent allocation bytes removed. No raw banks/builds from these component proofs remain; required compressed original inputs, bounded diagnostics, source pins and requested visible recordings are protected. MP coordinator allocations remain separate to prevent double counting.

The collector suffix is now physically composed through native16C000 and its genuine sphere/inverse/BSP/publication children:453 profiles per compiler,309full returns,24,301 ordered reads and1,842 stores, with two rejected collector mutations and no supplied hit Boolean. An independent309-case original-only census executes all46collector sites. Its earlier zero coverage metadata came from bypassing the inherited tracker and remains preserved with an erratum. The core comparison itself passed.

The production instanced helper uses the sole Host-key overlay, preserving public numeric behavior through both87prefix and453returned-child requalifications. Source30AA70's body key is ordinary typed DATA. Known24DEF0→24DF20 never consumes its caller-private hit packet; the native Host value is not serialized into a fake source stack and no instruction PC is invented. The whole same-helper24DC90→MOPP→provider→collector→native instance query→cleanup passes418 declared source-coherent profiles,107full returns,27,910reads1,895stores and three semantic mutants in both compilers. Fast-mask and recursive MOPP controls produce real original/native hit decisions. Every provider receives the same unprovided-field guarded memory. Overlay597d3598.../peer0ebe7feb... and bindingbeeae270.../peer79335f52... are qualified within this retained-DATA domain.

Contacts is composing the whole camera query and its World/Look seam. A true source difference surfaced when the previously unreachable CD4A0 second query became observable: sourceCD6CA addresses normalized delta at native data+36, whereas the old native suffix used scaled data+12. The private one-token correctionc5f122... has independent source/read-order audit136fafbf...; default-FP composed proof is being completed, and alternate acos policy owners remain explicit refusals. The frozen Root camera file has not been silently replaced, and no new live pass is inferred.

Current cleanup verifies228 disjoint completed owners/phase directories absent,38,785,548,288 cumulative parent allocation bytes removed. The additional872,128,512 bytes include successful private qualifications and preserved harness-only failure cohorts. Bounded results/source hashes/diagnostics remain; no new large generated artifact is retained.

The next constraint builder31D930 is now semantically native through all selected children. The existing numeric3294E0 linear-emitter214-profile/all155-site proof was authenticated against current665e36a2...; its additive Host-Vec4/active-FP interface shares the sole algorithm and independently requalifies public deferred reads/aliases. The parent transforms all four point lanes, including W, as ordinary native DATA. The complete parent and SAME native-only binding pass1373 profiles per compiler:771full returns and602read/write/observer prefixes, all207parent sites, three x87 precisions/four rounding modes, finite/SNaN W and three rejected mutants. Four additional helper guards refuse unsupported FP/depth before DATA access. Parent17908e49... and Hostlinear dabb51d4... are component-qualified; actual input, peer, World binding and visible delivery remain pending.

The unqualified parity audit initially reversed TESTAH44/JNP incorrectly. Real source comparison exposed64native versus56original stores. Reverting that temporary change restores the correct even-parity emission; final original/native comparisons and the inverted-parity mutant validate it. No qualified/live file was changed by that failed candidate. The raw failure, fixture and generator diagnostics are retained rather than hidden.

Cumulative measured cleanup is now237 disjoint completed activation probe owners and39,571,435,520 parent allocation bytes removed. These nine additional owners account for785,887,232 bytes; MP coordinator and MP-owned emitter probes remain outside this ledger for deduplication.
