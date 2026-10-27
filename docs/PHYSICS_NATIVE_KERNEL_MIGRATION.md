# Physics native-kernel migration audit

The [physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md#required-end-state-native-kernels-with-unchanged-tu15-behavior)
requires native semantic kernels and 1:1 English Xbox TU1.5 behavior together.
This is an evolving source audit with bounded conversion and multiplayer
evidence. It is not a completed inventory or a native-only runtime qualification.

| Area | Current source evidence | Required completion |
| --- | --- | --- |
| Live player and world scheduling | `src/physics/physics_live_world_tu15.cpp` still enters player scheduler14FEC0 and object phase B8F40 through the virtual ABI. The world phase now invokes typed `step_native_physics_frame`; its named collision/contact/publication dependencies remain residual owners. The narrow v1455 frozen integration passes all nine visible multiplayer cases. | Finish the native dependency closure while preserving scheduler/callback order and published state. Historical v1396-r2 physics delivery and later launcher selections are separate from these disposable physics cohorts. |
| World and solver execution | The production binding selects typed world/island, collision/broad-phase, island-agent scheduling, contact-time advance, dynamic accumulator, row solver, initializer, built-in custom callbacks and motion-commit owners. v1461/03 exercises native endpoint/compound/box/centered bounds, but45 residual bounds calls keep its coverage gate red. Qualified native31F770/31F860 producers and329720 base are wired in the working tree; their next visible cohort is pending. Generated bodies and ABI transport remain for named unconverted dependencies. | Finish contact producers, remaining controller/geometry and ownership. Count actual residual identities and require zero fallback before native-only closure. |
| Vehicle rate kernels | `src/physics/vehicles/physics_vehicle_runtime_generated.cpp:1448` and1516 select structured rate kernels. `source_vehicle_rate_native_tu15_internal.hpp:27` can decline for observers, tracing or backing conditions. | Preserve the proved kernels and remove live lifted fallback as remaining branches are converted. Instrumented validation must exercise the native path. |
| Object motion and lifetime | `include/halo2/native_tu15_object_motion.hpp:18` leaves teleport placement unported; lines47–61 identify activation/vehicle-child limits. `tests/native_tu15_object_motion_tu15_tests.cpp:380` skips refused cases. | Convert and validate teleport, activation, sleeping islands, wake listeners, deferred bodies and applicable child updates. Refusals are remaining coverage, not passes. |
| Live execution proof | `tests/physics_multiplayer_acceptance.cpp` now records authority/local per-world native-frame and residual-call counts, in addition to process-wide rate activity. The frame gate requires actual native-frame activity. | Record every path and assert zero lifted/emulated gameplay fallback before native-only closure. Requalify visible multiplayer and actual vehicle cameras. Nonzero residual-call counts keep the closure open. |
| Floating-point fidelity | `src/physics/physics_live_world_tu15.cpp:1109` explicitly leaves Xbox reciprocal-square-root estimate bits for separate comparison. | Preserve observable precision, rounding, edge cases and estimate behavior. Original instructions executed on the same host cannot independently prove Xbox-specific estimate bits. |
| Existing vehicle differential tests | `tests/physics_vehicle_runtime_tests.cpp` checks state, ordered writes and retained banks. The TU1.5-only validator now passes 2,230 cases for complete typed rate algorithms, including 792 plain value-owner cases independent of the compatibility state. | Extend this proof across the composed engine and unrestricted live observation. The earlier primitive extraction alone did not qualify the complete algorithms. |

Original executable-backed replay and emulation remain offline reference tools.
The current source includes real native kernels and substantial instruction
lifting; it does not yet satisfy the complete native-kernel end state. Ordinary
native arithmetic and verified compatibility data layouts may remain where
they preserve TU1.5 semantics. Replacing those semantics with approximate host
behavior would not satisfy parity.

## Native conversion evidence in progress

- Native lift scalar `1DA530`: [retained report](../analysis/physics/native-lift-scalar-tu15-20261026-final/report.json)
  compares 8,832 complete unchanged original calls and 912 authentic stops at the
  required power owner. Results, MXCSR and x87 exception flags match over 48
  precision/rounding/DAZ/FTZ combinations; a private arithmetic mutation fails.
  Positive-exponent native power and live force integration remain open.
- Native world frame `1CC8D0`, body iterator, publication decision and deferred
  reference operations: [current retained report](../build/test-results/native-physics-frame-read-phases-tu15/report.json)
  passes 65 original root-prefix/tail cases, 248 ordered persistent stores and
  1,330 ordered data reads checked relative to those stores. Cached typed
  property/component/queue handles preserve lazy field reads, queue-pointer
  timing and the post-store reference-count decision. Both stepping and eager
  property-read mutations are rejected. Additional model regressions cover
  missing queue backing and observer changes during publication.
  Physical i386 execution of unchanged original instructions verifies retained
  banks, semantic boundary arguments and complete FP state. This avoids known
  missing exception flags in the emulator oracle. Full residual closure remains
  open; v1456 below compares two actual composed calls. The optional
  typed world-step extension is independently rechecked in the
  [subsequent proof](../build/test-results/native-physics-frame-world-step-override-tu15/report.json);
  its default path preserves the prior frame composition.
- Native body forces `3252D0` and angular impulse `3250A0`: the stronger
  physical ordered-store oracle found an actual distinct-NaN mismatch in the
  prior intrinsic implementation. Source operand order is now fixed through
  real native arithmetic operations. The expanded 174-case suite passes 1,032
  ordered stores and full FP state under
  [Clang](../build/test-results/native-motion-forces-paired-nan-clang-tu15/report.json)
  and [GCC](../build/test-results/native-motion-forces-paired-nan-gcc-tu15/report.json).
  A gravity-removal mutation fails. The 32 paired-NaN controls include 26 cases
  whose physical original stores differ from the emulator's stores; emulator
  results cannot supply their expected payloads. The original failing report
  remains bounded evidence, and [cleanup is verified](../build/test-results/native-force-order-cleanup-tu15.json).
- Native motion pose `325170` and its closed quaternion product `323430`,
  normalization `1E1010` and matrix `37C5D0`:
  [physical original proof](../build/test-results/native-motion-pose-tu15/report.json)
  passes 210 cases under GCC and Clang, with 8,229 ordered reads, 4,301 ordered
  stores, seven read-fault prefixes and 24 paired-NaN controls. All 206 native
  RSQRTSS input/output observations match unchanged original instructions on
  this host; substituting precise reciprocal square root fails. Xbox/Pentium
  III estimate-bit identity remains unqualified. The current native world
  binding selects this module for its authenticated pose owner and records
  per-world activity. Its [binding model](../build/test-results/native-world-integration-pose-binding-tu15/report.json)
  verifies actual retained body movement with no lifted pose invocation; live
  composed qualification is queued after the initial v1457 frozen cohort.
- Complete typed vehicle rate algorithms:
  [retained verification](../analysis/multiplayer/physics-native-rate-observer-tu15-20261026/verification.json)
  passes 2,230 cases, 47,505 ordered writes and read/write fault prefixes;
  792 cases invoke plain typed value owners independently of `VehicleState`.
  Exceptional inputs and the arithmetic mutation control pass their expected
  gates. Outer controllers and unrestricted observed live paths remain open.
- Native lift power, special arguments and masked CRT error behavior:
  [retained report](../analysis/physics/native-lift-power-tu15-20261026-crt-final/report.json)
  passes 16,560 complete original calls, including 6,168 CRT error records and
  4,416 real original thread-errno lookups. Pre-clamp power bits, composed
  scalar results, numerical exception flags, errno writes and binary64 CRT
  intermediates match. Logarithm, errno and underflow-adjustment mutations fail.
  The scene-owner bypass is now fixed and independently qualified through
  576 original parents. v1458 iteration03 exercises all four scalar sites
  across actual Midship/Lockout multiplayer, with 240 power calls and zero
  refusals; both captured whole-frame original comparisons pass. Pentium III
  transcendental identity and full native force-parent closure remain open.
- Native array growth/copy and actual pool routing:
  [176 physical original calls](../analysis/physics/native-array-growth-tu15-20261026-final2/report.json)
  preserve ordered reads/stores, alias mutations, fault prefixes and allocation
  lifetime under both compilers. The typed world transition now invokes this
  module; its original-prefix/binding regressions pass and its declared live
  refresh is running. Unknown allocator/default/oversize owners remain explicit.
- Native constrained dynamic accumulator setup `325410` and transpose `2E4820`
  compose the actual native angular impulse. The
  [322-case physical proof](../build/test-results/native-dynamic-accumulator-tu15/report.json)
  covers 74 fault prefixes and 32 paired-NaN cases with both compilers and a
  rejected transpose mutation. Constraint orchestration/core-solver integration
  and complete post-solver lifetime qualification remain open.

## Native-frame multiplayer cohort v1455

[All nine visible LAN cases pass](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-live-v1455/matrix.json):
players, vehicle/camera, objects, gravity lift and all five Cyclotron support
cases. The declared frozen source is qualified v1396-r2 plus the v1453 input
fixture and the native-frame changes; this is not qualification of unrelated
current-checkout edits. The source refresh is hashed and documented before live
testing, and all five native frame files match their independent original proof.

The actual worlds record 3,515 native frame calls and 2,215 exact LAN comparisons.
The vehicle case uses 458 seated source-camera frames, approximately 0.6 radians
of camera look and 41 post-exit camera frames. The
[vehicle recording](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-live-v1455/vehicle-camera/run/lan-vehicle/window.mp4)
shows the local third-person vehicle camera alongside the peer's first-person
view. The 26,731 residual calls explicitly keep native-only closure open.

[Cleanup verification](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-live-v1455/cleanup-verification.json)
confirms the owned source/build tree is removed: 1,125,720,064 allocated file
bytes, plus earlier per-case raw cleanup not estimated here. Retained evidence
is 57,926,174 bytes, including 43,682,121 bytes of nine requested recordings and
no binaries. Concurrent unrelated work prevents attributing a volume-free delta
to this cleanup.
## Composed native-frame original replay v1456

[Both actual multiplayer captures pass](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-original-v1456/matrix.json):
Coagulation vehicle/camera compares 89,729,945 persistent bytes and Lockout
gravity lift compares 89,185,177, each with no differences. The unchanged
English TU1.5 executable runs only in the isolated Linux i386 reference process;
the live runtime uses the prepared imported MCC cache. Both live windows have
continuously verified desktop mirrors and native-frame activity. The vehicle
case records 476 seated source-camera frames, approximately 0.6 radians of look
and 41 post-exit camera frames; the cases have 633 exact LAN comparisons total.

The explicit `native_tu15_void_world_frame_v1` contract compares every captured
persistent byte, complete MXCSR/control/status fields and live x87 values, with
an independently checked empty x87 tag. It excludes only the returned private
callee stack `600E0000..600F0000`; volatile GP/SSE postimages are not outputs of
this native void API. Heap and full-MXCSR negative controls reject mismatches.
This proves these two composed calls rather than only the earlier prefix
fixtures. It does not qualify all trajectories, all callback domains, newer
world/island integration or native-only closure: these runs still record 7,204
residual calls. The measured instruction count is unavailable and is not inferred.

[Cleanup verification](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-original-v1456/cleanup-verification.json)
confirms removal of the owned source/build tree: 1,037,934,506 logical bytes and
1,120,522,240 allocated file bytes, in addition to earlier raw-case cleanup.
No raw banks, replay binaries, runtime export or duplicate video is retained.
The input-hash reports are losslessly compressed; the requested v1455 recordings
remain available. Source hashes, commands, comparison reports and return traces
preserve the exact proof boundary.

## Native world and pose cohort v1457

The [frozen initial world cohort and subsequent pose cohort](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-original-v1457/matrix.json)
pass every visible MCC multiplayer case and every required native activity gate.
The initial cohort passes five of eight complete original frame comparisons;
the pose cohort adds the player case and passes six of nine. Both retain the
same three failing Cyclotron raw-bank comparisons. The lower-platform diagnostic
finds 95 differing bytes across 22 ranges. A separately retained
[physical original lifetime diagnosis](PHYSICS_TU15_V1457_CONTACT_SCRATCH_DIAGNOSIS_20261026.md)
identifies inactive temporary contact records freed before frame return. This
diagnosis does not alter the raw comparison or its exclusions; the matching
native allocation/write/free trace remains a separate requirement. No native-only
closure is claimed, and the constraint residual still owns additional forces.

The [cleanup receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-original-v1457/cleanup-verification.json)
verifies removal of the owned source/build tree (1,044,092,276 logical bytes,
1,126,776,832 allocated bytes). Seven required failing captures are retained in
a lossless 83,572,194-byte archive after all original probes finished; every one
of 28,264 files survives round-trip hash verification. Removing the raw copies
reclaims 832,573,440 allocated bytes net of archive and manifest. The
[retention receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-original-v1457/required-failure-inputs-retirement/report.json)
contains the archive hash, exact members and an owned-scratch unpack command.
No program, archive library, build tree or duplicate recording is retained.

## Native deactivation and lift integration cohort v1458

The [initial frozen cohort](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-00-summary.json)
adds native island deactivation and the four scalar consumer seams, with
explicit read-only observer admission. Exact reconstruction from the prior
qualified source chain, 48 guarded operations and 43 proof-pinned source
matches precede the build. All five official regressions pass. All nine
visible cases exercise native frame, world, island, force, pose and sleep/wake
kernels, with 2,215 exact LAN comparisons. Eight acceptance reports pass;
gravity lift's new native-scalar coverage gate correctly fails because all
four scalar call counts remain zero. No observer refusals occur. A dedicated
authored force-field interaction remains required to qualify those live seams.

Six complete original-frame comparisons pass. The same three Cyclotron raw-bank
cases remain red, with unchanged exclusions. The actual lower-platform input
has [307 differing bytes across 73 ranges](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/lower0-byte-difference-diagnostic/differences.json),
so the earlier 95-byte diagnosis is not applied to this cohort. The lower-platform
trace covers the exact captured native frame: 243 stores, 32,637 bytes, no dropped
records, including allocator head/in-use/free-count transitions. Its lifetime
interpretation remains separate from the strict comparison gate.

The original v1458/00 vehicle recording was superseded after verifying and
visually inspecting the v1459 recording
(12,380,299 bytes). Its [retirement receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-00-vehicle-camera/requested-video/replacement-retirement.json)
preserves the old hashes and redirect; the old clip and samples are removed.
The [current replacement](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-06-island-agents-vehicle-camera/requested-video/window.mp4)
shows the actual vehicle chase camera and independent peer.
Residual ownership and native-only closure remain open.

The [incremental motion/policy cohort](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-01-summary.json)
adds three native modules through 17 guarded edits and 35 matching proof pins.
The exact frozen memory interface also passes a fresh 275-case physical original
kinematic comparison under both compilers, including both rejected arithmetic
mutations; its owned fixture/builds were removed. Seven main regressions pass.
Ten visible cases record 2,779 exact LAN comparisons, 5,541 native keyframed pose
calls, 5,541 prescribed-motion calls, 1,286 body-policy calls and 87 island
transitions. Fixed-pose live calls remain zero; that return-only owner retains
component qualification only.

Nine acceptance reports and seven complete original-frame comparisons pass.
The dedicated authored Midship field matches the original world frame but still
reports zero calls at all four scalar seams, so its required scalar gate remains
red. This cohort exposed a live coverage or wiring gap, resolved by the scene-route refresh below. The earlier gravity
scalar failure is preserved; the subsequent gravity case keeps the same physics,
frame/world/pose/checker/original gates while the scalar requirement moves to the
authored field. No physics comparison or tolerance changes. The three Cyclotron
raw-bank comparisons remain red. No field video is retained without a positive
scalar gate. The repeated vehicle clip is retired with its
[size/hash receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-01-vehicle-camera/requested-video/redundant-video-retirement.json);
the inspected fresh vehicle recording remains available above.

A [test-only diagnostic iteration](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-02-summary.json)
keeps the same motion/policy physics and records 664 native writes (91,415 bytes,
no drops) from the exact captured frame, including array header/count publications
and both allocator-class metadata filters. This particular frame grows its
private array to capacity eight with six appended records; its history is not
inferred from an earlier cohort. The complete frame's raw comparison still fails.

The actual same-frame `319D50` residual input separately
[passes unchanged-original replay](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-02-collision-private-entry-replay/report.json):
90,131,353 final memory bytes, full GP/SSE/x87/FP state and ordinary default
comparison rules. A persistent-byte negative control is rejected. The shared
replay tool initially refused this entry, so the owned diagnostic copy adds only
`319D50` to its entry admission literal; its exact diff and both source hashes are
retained. No semantic flag, extra exclusion or expected register postimage is
supplied. The private programs and scratch banks were removed; the authentic
reference input remains explicitly retained for the ongoing audit. This proves
the lifted residual for that input, not a completed native collision port or a
passing whole-frame comparison.

The scene-owned duplicate `1DA750` routine was the actual scalar bypass. The
[scene-route refresh](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-03-summary.json)
replaces only that body with actual ABI/FP delegation to the dedicated force
runtime, preserving exactly one internal entry/return observation. The entire
frozen file matches the independent proof's exact include-relocation hash.
[Physical original qualification](../analysis/physics/native-scene-lift-parent-tu15-20261026/proof/report.json)
passes 576 complete parents, 1,152 native scalar calls, 249,480 stores and
540,000 retained bytes with full FP/defined flags; a zero-result mutation is
rejected. Both proof builds and their owner were removed.

All ten visible acceptance reports now pass, with 2,809 exact LAN comparisons.
Actual scalar counts are `[240, 54, 94, 240]`, with 240 native power calls and
zero observation refusals. Midship exercises the first/fourth sites; Lockout
gravity exercises the second/third. The earlier zero counters reflected this
production bypass, so they do not establish that gravity was an unsuitable
scalar scenario. Their failed reports remain immutable. Both scenarios pass
their complete world-frame original comparisons. Seven of ten whole-frame
comparisons pass; the same three Cyclotron raw-bank cases remain red.

The first successful scalar-field clip (7,685,463 bytes) was later retired
after independent review of the [new native-solver Midship recording](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-01-midship-platform-scalar/requested-video/window.mp4).
Its small hash/redirect receipt remains. The exact 19,160,144-byte
phase trace survives losslessly as 120,544 bytes, round-trip verified before
owned raw cleanup. Repeated vehicle recordings are retired with their hashes;
the current inspected vehicle-camera replacement remains. Native array/allocator
integration is the next declared cohort. Residual owners and full native-only
closure remain open.

The [native array/allocator cohort](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-summary.json)
passes all 16 visible acceptance reports and 6,917 exact LAN comparisons.
It records 13 native growth operations, 13 pool allocations and 11 pool releases,
with scalar counts `[5064, 578, 1078, 5472]`, 356 power calls and no refusals.
All six additional Turf/Gemini/Colossus/Elongation cases have required scalar
coverage. Ten whole-frame original comparisons pass. Gemini's two cases and
Elongation add three raw-bank failures to the three Cyclotron cases; their
separate paired diagnostics find 129/20, 129/20 and 27/11 differing bytes/ranges.
[Exact ranges and first bytes](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/expanded-field-difference-summaries.json)
remain unclassified by lifetime; authentic inputs are preserved for the audit.

The [subsequent native collision/listener cohort](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-05-summary.json)
passes all ten core/Midship visible reports and the required native collision
gates, with 2,586 exact LAN comparisons. It records 4,307 native collision and
4,307 native world-listener entries; island-listener live counts remain zero.
Seven complete world-frame original comparisons pass; the three Cyclotron cases
remain red. Independent kernel qualification covers 300 physical original
cases, including integer write-fault prefixes and rejected mutation. The narrow
public stats declaration keeps the full definition in actual consumers, reducing
future compile fanout without changing physics behavior.

[Explicit owned cleanup](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/cleanup-verification.json)
removes 1,048,385,343 logical bytes and 1,131,307,008 allocated bytes of source/build
storage. No runtime binary or build is exported. Two unique requested videos
and required authentic original-input banks remain explicitly retained; duplicate
vehicle recordings have size/hash retirement receipts. The next finite owner
has already reproduced all 8,270 final source hashes through 100 guarded edits
from the immutable qualified archive and saved requests. This is source-provenance
validation, not acceptance of the pending constraint integration.

## Constraint and motion-commit cohort v1459

The new finite owner reconstructs the complete qualified v1458 source through
100 guarded operations and all 8,270 final source hashes. Its initial constraint
transport placed a retained builder packet below the callee stack pointer.
Preflight found this ownership bug before execution: the initial artifact was
held non-executable, and all ten initial attempts record no game execution or
completed physics capture. The guarded correction places the common callee
stack at `end-0x200`, with the builder packet at `end-0x80` above private callee
storage. A model poisons 0x180 bytes below SP and verifies the packet survives;
the old-layout mutation is rejected. Both initial and corrected source records
are preserved. No comparison or tolerance changes.

The [corrected cohort](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/iteration-01-summary.json)
passes eight official regressions and all ten visible/native coverage reports,
with 2,629 exact LAN comparisons. It records 2,736 constraint steps, 2,992 dynamic
accumulator setups, 3,740 motion commits and 8,775 island collision entries.
Commit kinds are `[748, 2090, 902]` for kinematic/dynamic/restore, all exercised
by actual cases. The solver's independent entry audit covers 2,208 CFG sites and
three jump tables, with no incoming GP/XMM dependencies beyond actual stack
arguments and explicit FP state. Seven complete world-frame comparisons pass;
the three Cyclotron raw-bank cases remain red.

The v1459 vehicle-camera recording has a verified size/hash receipt
(12,380,299 bytes). That vehicle run passes its
whole-frame original comparison and all required constraint/accumulator/commit
coverage gates. The current recording was visually inspected and SHA-verified before retiring
the older v1458/00 clip and its decoded samples, preserving their size/hash
redirect receipt. Both recordings now have later independently inspected
replacements: the v1460/06 vehicle clip and v1460/01 Midship scalar clip linked
above. Their original results and source hashes remain preserved.

A [read-only source/link audit](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/vehicle-rate-source-audit.json)
confirms these frozen cohorts do **not** contain the newer typed vehicle-rate
headers or `source_vehicle_native_rate_activity()` symbol. Their separate
2,230-case root-tree component proof is not live vehicle-rate acceptance.
Observed rate/controller integration remains separate work; native-only physics
closure remains open.

- These proofs do not establish Xbox-specific approximation bits or unmasked
  platform fault delivery. Reported masked numerical domains and required
  owners remain explicit. Large fixtures, banks and private programs are
  disposable; the retained proof packages contain small reports only.

Each listed migration needs independent original comparisons, live integration,
multiplayer evidence and disposable-storage verification before it can be
marked complete in the linked checklist.

## Native row solver and initializer integration

The [complete native row solver](../build/test-results/native-constraint-solver-tu15/report.json)
passes 3,000 physical original cases under GCC/Clang: all twelve primary row
families, secondary and export branches, 49,155 stores and 296,611 reads. The
[native initializer](../build/test-results/native-constraint-solver-initializer-tu15/report.json)
passes 113 cases, and their [ready-zero composition](../build/test-results/native-constraint-solver-initialized-composition-tu15/report.json)
passes 103 cases. No initialization success or clock outcome is invented.
The production binding now selects these typed owners instead of the whole
326A40 residual. Clock37C5C0 and logger2E3850 remain actual named dependencies;
the world transport checks their preserved live x87 value bits. Registered
custom-schema owners require their own native or authenticated ABI proof and
currently produce explicit refusals. A small strict-FP library isolates these
new native units from unrelated systems rebuilds. Live solver acceptance is
pending; the vehicle case must exercise positive iterations and primary rows.

The broad-phase refresh was interrupted after nine of sixteen cases. All nine
visible/native gates pass, with 10,438 native broad-phase calls and 2,414 exact
LAN comparisons; six strict original frames pass and three Cyclotron frames
remain red. The [interruption receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/iteration-02-interruption-summary.json)
records unfinished coverage and confirms the old scratch is absent. The new
bounded v1460 owner reconstructs exact authenticated source before any refresh.

The final built-in custom callback proof now covers 336 physical cases,
including rejected destination stores and committed stores whose observer
throws. The [all-native solver composition](../build/test-results/native-constraint-solver-native-clock-composition-tu15/report.json)
passes 143 cases across all twelve primary row families. It includes native
FILETIME arithmetic, initialization and all three custom callbacks. Production
clock input comes from the existing world receipt and UTC adjustment.

The [final vehicle-rate scope proof](../analysis/physics/native-vehicle-rate-final-handoff-tu15-20261026/report.json)
adds 1,382 physical original calls and 6,252 x87 instruction snapshots to the
2,230-case retained/current-header comparisons. It corrects actual NaN operand
ordering and commits pop/C1 after a successful destination write but before
observer delivery. Native world scope, per-world counters and a mandatory rate
coverage gate are wired. The separately declared ordinary Warthog reversal case
passes with239 acceleration/60 approach/299 cursor calls, no refusal,334 actual
vehicle-camera frames and a strict89,729,945-byte original sample. Earlier
zero-approach runs stay recorded. Outer controller execution remains lifted.

Native array reserve2E31E0 passes [57 original cases](../build/test-results/native-array-reserve-tu15/report.json)
with actual native allocation and pool routing, 2,833 reads and 2,226 stores.
The expanded [64-case proof](../build/test-results/native-array-reserve-input-phases-tu15/report.json)
includes actual caller argument cells and late-header aliases across allocation.
The broad-phase binding invokes this owner and counts it separately, but all
sixteen v1460/02 cases report zero activity. Internal actual2E31E0 helpers in
both generated world/contact runtimes still bypass that typed branch and need
native binding with proven caller ABI. The existing growth kernel is unchanged.
Raw Gemini/Elongation failures retain their strict comparison gates.

Native schema14 emission is active in the combined cohort:15,478 calls through
all three original callers, matching completion/cursor counts, zero refusals.
Native island-agent scheduling321D20/321A70 is now wired with explicit real
agent/finisher/time services and caller-owned contact packets above callback
frames. Its physical composition proof requires authentic callback-entry GP
and private-stack inputs; the normal production transport and live whole-frame
comparison must independently qualify that boundary. A neutral-context raw
failure remains recorded rather than excluded. The separately declared
v1460/06 Warthog passes its live native-phase/camera/rate gates and strict
89,729,945-byte sample:986 flag-set calls,1,169 process/finish calls each and
986 balanced buffer lifetimes. Flag-clear/minimum remain zero; five real
contact-time dispatches still invoke the unconverted3205B0 service. Its native
kernel and genuine before-input capture are the next owned work.
