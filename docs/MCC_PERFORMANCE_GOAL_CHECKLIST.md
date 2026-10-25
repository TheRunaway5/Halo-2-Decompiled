# MCC-class performance goal checklist

Status: **active; performance targets unmet**. Created 2026-10-22 at the user's
request. This checklist tracks implementation, measured gains and final
acceptance separately. Component results do not complete the goal.

Resolution contract, reaffirmed2026-10-24: four players share one3840×2160
internal target in a2×2 grid, giving each player1920×1080. The60FPS gate applies
to the complete frame. No additional per-player resolution reduction is allowed.

2026-10-24 whole-query timing is complete. The private timer surrounds the
genuine2E7C34 call after its real return-address push, including recursive
descendants. Both ordinary GPU runs pass120 submitted frames at3840×2160
internal /1280×720 presented, with actual worker execution and weighted GPU
work, zero fallback, and no timing-domain errors. The observed1p/4p owner
lifetimes contain3,840/11,160 completed queries taking188.056/608.960ms, all
inside44710 cull scopes. The full120-row parent cull totals are233.522/749.971ms.
This establishes a material whole-query target; it does not establish removable
per-frame cost or a speedup. The1p lifetime includes239 cull scopes versus120
TSV calls;4p has480 in each. Lifetime windows, descendants and preemption remain
explicit, and no90-row denominator is applied to cumulative counters.
The actual header also passes109 component checks, including eight return/throw
cases preserving callee-written x87 CW/SW, MXCSR, errno and exception identity.
That is component proof, not the complete live query ABI. The diagnostic overlay
is removed; the official rebuild passes all four focused contracts and reproduces
the qualified baseline native/harness bytes exactly. The finite build owner is
still needed for the genuine capture/native-kernel successor; raw runtime,
component and reader owners/processes are removed. The launcher is unchanged.

- [x] Measure the complete genuine query and independently reconcile all
      original owner reports against the complete120-row parent measurements.
- [x] Check the actual timer's FP/errno/exception behavior and restore the
      exact qualified baseline after the diagnostic run.
- [ ] Capture the genuine complete entry/exit state, memory transfers, faults
      and owner lifetimes without changing bank identity or source callbacks.
- [ ] Replace the material whole-query owner with a native kernel, prove
      source/gameplay/frame equivalence and measure its actual performance gain.

[Actual whole-query timing](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/chain-census-20261024/whole-query-timing-20261024/runtime-results-attempt-1/report.json),
[independent actual totals](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/chain-census-20261024/whole-query-timing-20261024/INDEPENDENT_ACTUAL_REVIEW.json),
[restored baseline and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/chain-census-20261024/whole-query-timing-20261024/restored-build/RESTORED_SEAL.json).

2026-10-24 direct-owner follow-up is active in a new finite owned build.
The canonical timing successor preserves101 columns and adds47 nested elapsed/
successful-return fields for actual Match.step, state projection, post-step
work, shared pre-view work and native model/light collection, including44710,
40E80, shadow selection and shadow roots. Inner accounting uses integer
nanoseconds with errno-preserving clock reads; floating conversion occurs only
in the existing writer. Two source reviews find no call/order/observer changes.
The fresh official build now passes all four input/trace/extent/benchmark
contracts. All18,597 copied source hashes remain unchanged; both native binaries
and the four actual critical compile recipes are sealed. Actual148-column
measurements now complete both visible4K-internal GPU cases. The1p/4p means
are53.446/14.106FPS; both overlap a compiler, and all360 measured frames miss
their120/60FPS budgets. No causal gain is accepted. In4p, model collection
takes14.271ms/frame, including7.274ms in44710 culling and3.224ms in40E80
commands. These nested values are not additive. Match.step takes15.765ms/frame
over380 actual tick calls (7.468ms/call), with6 one-tick,148 two-tick and26
three-tick measured frames; projection/post-step take6.483/5.792ms/frame.
This justifies a bounded cull-chain census; it does not assign all cull cost to
the plane-query interpreter. Compiled clock FP inertness remains unqualified.
The selected-model/LOD guard now reports its real owner/model/context/caller
tuple without changing its predicate. Its exact diagnostic patch is merged
after the timer patch with a byte-exact inverse proof.

- [x] Implement and source-review the direct returned-call timing boundaries
      and failure-only model-selection tuple diagnostic.
- [x] Complete the fresh official native/GPU-harness build and four focused
      CTests; seal both executables and fresh critical objects. The finite
      owner remains active for actual runs; the launcher is unchanged.
- [x] Retain the actual firing tuple: caller=body, WORLD Master Chief model
      replaced by FP arms, with valid WORLD-LOD storage andLOD4. The CPU guard
      remains RED before the GPU comparison; repair must preserve source selection.
- [ ] Fix the proven owner boundary and rerun original CPU/GPU/source/frame checks.
- [x] Measure all148 actual timing columns and identify material native owners.
      Calls-equal-tick-advance is checked only for the declared idle/no-revert
      workload; a real checkpoint restoration can violate that relation.
- [x] Run the bounded actual plane-query dispatch/chain census after proving
      its parent material; preserve the original parser RED and stop unused work.

The producer-marked FP/WORLD cache boundary is now corrected and compiled in
both runtime copies. Both ordinary4p WORLD-body common-cut cohorts, neutral and
firing, pass at3840×2160 internal /1280×720 presented. Each verifies153,360
complete weighted GPU records, with zero post-identity vertices, CPU fallback or
validation messages; all60 frames execute the worker. Each role draws12 bodies
per frame. Full presented PPM, ordered/passive traces and161,224/176,194 actual
input records match. Submitted rectangles retain2×2/1920×1080 per view. The
original1p inspection case completes both60-frame roles without the model guard,
then stays RED because it performs zero GPU work: forced WORLD capture does not
create actual main-prefix admission and suppresses FP drawing. No positive-work
gate was weakened. Constructor/teardown, full ABI, per-body GPU ownership,
physical4K scanout, FPS, latency and launcher acceptance remain open.
[Current4p neutral/fire proof](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-owner-isolation-20261024/world-cache-fix/ordinary-4p/report.json),
[independent review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-owner-isolation-20261024/world-cache-fix/ordinary-4p/INDEPENDENT_REVIEW.json),
[inspection-route RED and source diagnosis](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-owner-isolation-20261024/world-cache-fix/zero-gpu-route-diagnosis/EVIDENCE.json).

- [x] Correct the FP/WORLD completion-cache ownership boundary and qualify
      actual ordinary4p neutral/firing WORLD GPU work against complete common cuts.
- [ ] Resolve the separate1p capture-only GPU admission gap without inventing
      main draw units or weakening positive-work/source/vertex/frame gates.

The material-cull follow-up now has actual one-/four-player counts:44,867 and
148,545 completed query entries,866,085 combined dispatches, zero unknown/
unbound/foreign/fault/overflow counters and zero selected jump-handler/chain
executions. Stop the jump-chain candidate for these observed lifetimes. The1p
diagnostic passes; the original4p campaign stays RED on an unsupported parser
assumption: `connected` prints boolean `joined` (1), not local-player count (4).
The retained native exit, workload, owner counters and cleanup are independently
valid. Bound decoding/classification dominates dispatch frequency; current query
cost, complete entry/exit ABI, faults and view lifetimes still need direct proof.
Counts do not establish per-target cost or a native-kernel speedup.
The private counter header/controller are retired and both original source files
restored. The official rebuild passes all four contracts; native and harness
are byte-identical to the qualified pre-census body-fix binaries. Canonical
shadow/physics sources and launcher remain unchanged. The finite owner stays
active for the whole-query cost/contract/capture seam.
[Private build seal](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/chain-census-20261024/overlay-build/BUILD_SEAL.json).
[Actual counts and independent STOP decision](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/chain-census-20261024/INDEPENDENT_ACTUAL_CENSUS_REVIEW.md),
[exact restored baseline](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/chain-census-20261024/restoration/BUILD_SEAL.json).

[Current source preparation and finite owner](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-owner-isolation-20261024/PREPARE_SOURCE.json),
[official active build](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-owner-isolation-20261024/full-build/report.json),
[exact diagnostic merge](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-owner-isolation-20261024/BODY_DIAGNOSTIC_MERGE.json).

2026-10-24 host attribution: the new diagnostic-only timing packet builds in
the current frozen native runtime and passes both official input/benchmark
contracts. It uses the existing timing-file gate, installs no observer and
adds no source reads or pose copies. Two actual GPU runs complete240 submitted/
180 measured frames each at4K internal/720p presented. The4p run has no sampled
compiler and averages54.270ms/frame (18.426FPS): input/simulation18.251ms,
camera/lighting/shared view preparation15.582ms, object preparation5.920ms,
first-person2.211ms, common tail2.159ms, bodies1.398ms, HUD1.230ms,
static world0.367ms and renderer6.798ms. Current pre-render totals47.117ms;
only0.355ms remains after aligning each interval with its previous renderer.
These are host elapsed measurements, not GPU timestamps or thread CPU clocks.
Input work depends on cadence:67 one-tick frames average12.069ms there, while
113 two-tick frames average21.916ms. Do not treat18.251ms as fixed per-frame
simulation cost at60FPS. Camera and object preparation alone exceed the16.667ms
complete-frame budget. The1p run overlaps a compiler and averages50.108FPS;
neither run grants a causal gain, parity, default or launcher acceptance.
Next isolate the actual Match step, state projection and post-step work, then
the native collection inside the broad camera bucket before choosing a kernel.
[Actual measurements](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/host-phase-timing/report.json),
[independent original-TSV attribution](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/host-phase-timing/INDEPENDENT_HOST_PHASE_REVIEW.md).

- [x] Measure the upstream host phases without installing the expensive frame
      observer; preserve all69 existing columns and independently reconstruct
      actual ticks, per-view phases and prior-renderer interval alignment.
- [ ] Split the measured input/simulation and native view-collection buckets
      into actual owner costs, then implement a material exact native kernel.

This finite source/build session is now closed. Its3,163,014,343 logical bytes
(3,225,100,288 allocated bytes), all nested raw test owners and their processes
are removed. All480 borrowed reference targets retain their previous existence,
size and filesystem identity. Only small source archives, commands, reports,
TSVs and logs remain; no generated large binary/frame/bank is retained. The
goal remains active, with FPS, firing, cold-start, latency and launcher gates
open. Later canonical source changes require their own qualification.
[Final phase source/evidence seal](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/CURRENT_PHASE_TERMINAL_SEAL.json),
[verified owner removal](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/OWNER_REMOVED.json).

2026-10-24 current native timing: the newly built native executable completes
eight visible X11 runs at3840×2160 internal /1280×720 presented resolution.
Each run submits240 frames and measures180 after60 warmup frames. Observed
CPU/GPU run means are70.323/63.041 and62.256/57.735FPS for1p, and
19.886/18.373 and21.711/16.664FPS for4p. All1,440 measured frames execute
the real presentation worker; all720 GPU frames perform weighted deformation,
with no CPU fallback or verification readback. The native hash remains unchanged.
The first CPU run overlaps a compiler; the other seven contain no sampled
compiler process. Elapsed-cadence source ticks differ between runs, so these
are throughput observations without a matched-work causal speedup claim.
All but one measured frame miss the120/60FPS budgets. GPU skinning has no
accepted performance gain. Renderer host time is5.09–7.09ms in4p intervals
of46.06–60.01ms; the remainder is unmeasured interval residual, not established
GPU or CPU execution time. Fine host-phase attribution remains required.
The nested raw owner and its children are removed; small TSV/log/report/source
evidence remains. Physical4K scanout, parity, latency and launcher remain open.
[Current eight-case timing](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/current-native-timing/report.json),
[native provenance and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/current-native-timing/BASELINE_AND_CLEANUP.json).

- [x] Build the current native runtime and measure actual worker/GPU routes
      under the required internal4K extent; retain the failing FPS gates.
- [x] Add and compile a separate post-constructor common-cut comparison,
      preserving the original cold-constructor test and all complete bank bytes.
      The three driver/input/trace CTests pass. Its first real1p FP-neutral
      comparison at640×480 also passes: all90,859,075 region bytes and
      89,867,843 committed bytes across4,261 regions/runs,54,533 actual
      runtime inputs, ordered traces and complete final PPM match;620,640
      complete GPU Vertex records are verified with zero validation messages.
      All60 frames per role execute the presentation worker. The measured
      4,261-region failure is retained; the test cap is now8,192, without any
      byte exclusion. Constructor and teardown remain outside this additional
      tape; it cannot grant ordinary full-lifetime acceptance.
      [Actual paired qualification](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/common-cut-runtime-cap8192/evidence/model-skin-common-cut-runtime.json).
- [ ] Qualify the new complete-common-cut CPU/GPU runtime comparison, then
      resolve the original constructor/teardown trace and repeatability gates.
      The four-player4K-internal/720p-presented FP-neutral successor passes:
      161,224 exact input operands,2,635,920 verified weighted GPU vertices,
      12 body draws per frame and60 worker frames per role. All60 submitted
      rectangles per view match the required2×2/1920×1080 layout. Complete
      source bank and final presented PPM match; this does not independently
      observe physical Vulkan viewports or prove per-view surviving body pixels.
      The1p inspection-camera body-fire successor is RED on the CPU role before
      comparison: `Third-person draw lost the original selected model/LOD`.
      Both nested raw owners are removed. Broader species/action/visibility/
      fallback coverage remains open.
      [Four-player result](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/common-cut-4p-4k/report.json),
      [body-fire failure](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/common-cut-body-fire-1p-4k/report.json).
- [ ] Attribute the large host interval remainder and adopt a material native
      kernel only after genuine workload, semantic and integrated-cost proof.

2026-10-24 constructor-input follow-up: the existing external-operand tape now
has an explicit isolated-local construction route through PreparedWorld,
ObjectRuntime and PhysicsLiveWorldTu15. It binds before native initialization
and remains through Match/player construction, runtime and native teardown.
Existing constructors keep their normal route. Complete-call journals are
rejected; foreign-executor requests fail before sampling a provider. The
ordinary CPU/GPU harness now records/replays genuine inputs and compares
ordered passive node/source/input/camera records, including all60 preparation
bank checkpoints and the final bank. No clock or bank bytes are normalized.
Four actual-TU syntax checks and the trace component pass. The frozen full
systems/renderer/runtime-harness build now passes. Genuine startup testing
exposed an unconditional projectile-callback metadata invalidation; its fix
preserves that rejection for complete-call journals while permitting operand-only
Match construction. The unchanged real callback and executor/publication guards
remain active. A subsequent1p run matches all three complete89,867,843-byte banks,
published Match states and its full436-input transcript through teardown.
The suite remains RED: cold startup's recycled async task payloads can differ,
including a4p constructor cut with matching published Match state but different
completed tasks retained in the same free node. Both malformed-input controls
now pass at the real148D40/33A4D3 input boundary; the observed bridge error
wrapper is retained, and fresh capture recovers after both rejected constructors.
A separate4p run also matches all three full banks and its1,440-input tape.
These successes occurred in different runs; the final cold-start suite remains
RED. No bytes are excluded or normalized; startup repeatability, actual GPU
trace and performance acceptance remain open.
[Exact source seam](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/SOURCE_SEAL.json),
[active official build/test](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/full-build-startup/report.json).
[Actual both-player-count startup result](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/startup-both-context/evidence/ordinary-startup-inputs.json).
[Current source/build seal and limits](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/CURRENT_IMPLEMENTATION_SEAL.json),
[final startup and negative controls](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/startup-wrapped-negatives/evidence/ordinary-startup-inputs.json),
[async task source attribution](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-constructor-inputs-20261024/ASYNC_STARTUP_ATTRIBUTION.md).

2026-10-24 gameplay-trace follow-up: the old animation observer forces CPU
skinning and cannot qualify the ordinary GPU route. Canonical runtime taps now
borrow the actual completed/interpolated world and first-person node spans
before route selection. The existing GPU predicates and legacy observer copy
condition are unchanged. A bounded trace sink records explicit node bits,
ordered source/input/camera records and committed-bank digests. All77 component
controls pass, followed by the official CTest and actual runtime syntax check;
no ordinary trace or whole-game link has run. Genuine external clock inputs
still need recording/replay before full source-state comparisons can be accepted.
[Scoped source, component and lifecycle evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-source-trace-seam-20261024/CANONICAL_DELIVERY.json).

The private17F8A0 CRT forwarding experiment is stopped without adoption. All64
paired controls pass, including42 access faults. The bounded mechanism saves
about118ns/call (9.54% paired median), but arbitrary service capture copies differ
(7 versus3). Those semantics remain unresolved, and sampled inclusive callsite
work is not removable wrapper work. No ordinary benefit or native activation
is claimed.1E5470 was deferred at that stop; its subsequent audit is below. All three private raw owners,
workers and binaries are removed; only small source/report evidence remains.
[Stop decision](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-crt-handoff-20261024/STOPPED.json).

The subsequent read-only `1E5470` audit stops it as a standalone kernel
candidate: only one of its59 attributed samples executes the generated body;
most execute required helpers. Its transform callees already use SSE. Inclusive
callsite samples do not establish removable wrapper cost. No kernel or runtime
change follows from this audit.
[Pinned owner audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/OWNER_1E5470_AUDIT.md).

The memory/access audit also stops before a backend experiment. The1,527
historical samples partition into325 explicit lookup/allocation/extent samples,
613 checked access bodies,572 policy/callback samples,10 mount samples and seven
ambiguous symbols. The current memory owner already has a two-level full-page
index. These samples do not identify how often it uses that index versus its
map fallback, or the guest-address distribution. Actual path/address evidence
is required before another memory-backend trial. No additional speedup is
claimed, and neither production source nor the active build was changed.
[Source and retained-sample audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/memory-access-feasibility-20261024/FINDINGS.md).

2026-10-24 GPU fallback follow-up: a real handler gap is now fixed in the
canonical source. Host/device out-of-memory results from `vkBindBufferMemory`
previously became hard errors; they now use the existing typed input-allocation
refusal and CPU fallback path. The original service fails all eight relevant
stub OOM cases, while the one-handler fix passes all16 cold/growing,
retained/streamed OOM and hard-error cases. The official RTX5080 GPU test also
passes with both new bind-failure controls:9 resource cases,1,152 FP/errno cases,
81 programming-error precedence cases,15,606 complete vertex comparisons,
1,000,095 persistent-path vertex comparisons and zero validation errors.
The shader is unchanged. This qualifies the service/component fix, not actual
device-wide exhaustion, whole-frame fallback, defaults or launcher delivery.
All generated binaries and scratch are removed; small reports and source remain.
[Canonical delta and hardware evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/gpu-skin-bind-refusal-20261024/CANONICAL_DELIVERY.json).

Constructor recovery is also integrated in canonical source: optional GPU-skin
service allocation exhaustion leaves the existing CPU-skin capability path
available. Known hard capability/shader/device errors remain hard, including
allocation failure while creating their diagnostic. That combined-pressure bug
is reproduced before the fix and preserved as RED. The successor passes all36
factory cases and six direct-constructor compatibility checks; all36 preserve
the32-byte caller FP environment/errno, retire partial handles and recover on
the next cold creation. The official actual RTX5080 regression also passes,
with unchanged shader and complete GPU outputs. This does not qualify rendered
ordinary startup fallback, an engine rebuild, gameplay traces or launcher delivery.
Both focused official build trees are removed; no large artifact is retained.
[Final source/component evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/gpu-skin-bind-refusal-20261024/ctor-combined-pressure/CANONICAL_DELIVERY.json).

2026-10-24 complete-context profile: the unchanged current four-player GPU
route completed240 submitted frames and210 measured frames at3840×2160
internal resolution. Every submitted frame used the presentation worker;
every measured frame performed weighted GPU skinning without fallback or
verification readback. All5,250 original CPU samples and3,892 complete stack
contexts are retained and independently reconstructed. Memory/access helpers
account for1,527 samples (29.09%). The largest deepest generated owner is the
plane-query interpreter `2E6290`:285 samples (5.43%), including215 memory-policy
samples (4.10% of all samples). These categories overlap. This prioritizes a
larger native owner over the tiny distance kernel; it establishes no FPS gain.
The original suite remains RED because its inherited final-summary helper
expected210 submitted frames despite the predeclared and actual240. Offline
review confirms the declared workload without rerunning or changing that result.
The displayed window was1280×720; physical4K presentation remains unqualified.
[Retained-data review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-full-context-profile-preparation/results/ROOT_RETAINED_REVIEW.json),
[independent raw reconstruction](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/independent-full-context-review-20261024/independent-raw-report.json).

The same profile locates272 samples (5.18% of all samples) at a material-map
iteration return in the renderer. Bounded disassembly and source control flow
identify the global camera-direction remap scan; line-debug information is
absent, so this source association is explicit inference. The canonical
renderer now builds an ordered list of immutable passes with nonempty direction
mappings, preserving the existing per-frame root predicate, remap calls and
dirty-update order. The original scan remains available through
`HALO2_REFERENCE_VIEW_DIRECTION_SCAN`. Only this delta is authenticated for the
frozen runtime. All21 three-path remap cases and21 omission/order negatives
pass, followed by all three focused renderer tests. All four actual1p/4p
neutral/fire reference-versus-list pairs pass:952 submitted frames perform
weighted GPU skinning, with exact full internal4K RGB, meaningful depth/stencil,
final printed source/RNG summaries and every non-timing renderer counter.
The ordinary timing batch also completes all12 cases (2,160 measured frames).
Foreign build activity makes every case diagnostic only; all2,160 frames miss
the120/60FPS deadlines. The97,496 material counter measures resident slots,
not draw calls. No isolated FPS gain is accepted.
[Hotspot evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/independent-full-context-review-20261024/RENDERER_MATERIAL_FINDING.json),
[exact source delta](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/renderer-direction-scan-20261024/PREPARED.json).

Three alternating ordinary timing pairs per player count give these observed
medians. Global preparation is an existing renderer host-phase timer, including
its original waits; this is not process CPU utilization or GPU timestamp data.
Elapsed-cadence simulation ticks and work distributions vary between runs, and
the reversed second pair regresses in both player counts. Keep these as
contended diagnostics, not a causal matched-work FPS gain.

| Internal4K ordinary timing | Reference map scan | Ordered pass list |
| --- | ---: | ---: |
|1p observed median FPS|42.747|47.790|
|1p global preparation, median run mean|2.339ms|0.358ms|
|4p observed median FPS|14.245|14.471|
|4p global preparation, median run mean|7.208ms|1.667ms|

All2520 submitted frames execute the worker. Every measured frame performs
weighted GPU skinning without fallback or verification readback; the single
zero-work startup frame in each role is retained as warmup evidence. Internal
rendering remains3840×2160, with1280×720 presentation. Gameplay/latency,
physical4K, GPU defaults and launcher gates remain open.
[Actual ordinary timing](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/renderer-direction-scan-20261024/timing-preparation/results/report.json).
[Independent original-TSV reconstruction](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/renderer-direction-scan-20261024/timing-review/REVIEW.json).

This finite runtime investigation is closed. The owned source/build/runtime,
historical copied references and cache are removed (3,565,720,673 logical bytes);
the supervisor, coordinator, session and all case children are absent. Exact
source, patches, commands, reports and original small TSVs remain. No large
artifact is retained. The goal remains active; native arithmetic, isolated
performance, gameplay/latency, defaults and launcher delivery are unfinished.
[Owner removal](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/renderer-direction-scan-20261024/OWNER_REMOVED.json).

The private checked-write destination-reuse trial is closed without promotion.
Its repaired revision mechanism passes113 paired functional cases, including
callback-driven remounts and the original aliasing counterexample, but48 matched
cost rows show mixed results and no material ordinary benefit. Canonical and
frozen memory owners are unchanged; all six private raw/build owners are removed.
[Trial decision and evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-exact-write-20261024/TERMINAL.json).

Latest native component result: the private eight-PC first-distance kernel
passes genuine operand/state and fallback/fault comparisons. Three interleaved
timing pairs measure median current-literal/native cost of101.213/61.440ns per
call, including admission checks and matched resets. This39.30% reduction saves
only39.77ns per call in a tiny block. Scaling by the observed132 calls per
four-player source tick gives about0.00525ms per tick; this is arithmetic scale,
not measured game time or FPS. The kernel stays private while the next larger
measured CPU owner is selected. Ordinary activation and all120/60FPS gates remain
open.
[Cost evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/private-first-distance8-prototype/cost-attempt-3/execution.json).

Earlier four-route evidence: all four fresh one/four-player CPU/GPU attribution
runs pass, with10,221 main-thread samples across840 measured frames. The GPU
routes perform weighted deformation on every frame without fallback/readback.
In4p GPU, source-runtime `required_entry` contains45.76% of samples, the
memory/access/instruction category26.63%, remaining `skin_model`/`finish_skin`
leaves7.69%, and GPU preparation0.224%. Inclusive rows overlap; these are
attribution under recorded build interference, not FPS gains. The scoped
internal-color/GPU-validation successor now builds and passes32 focused CTests.
All four neutral/fire1p/4p CPU/GPU pairs match full internal4K RGB, meaningful
depth/stencil and captured source summaries. The strict210-frame GPU-work gate
fails only on unmeasured warmup frame0; the repaired runner retains its exact
row and table. Frames1–209, including all180 measured frames, perform weighted
GPU work. The precise startup eligibility cause is unassigned. A separately
declared steady-state benchmark reports warmup work and requires weighted work
on every measured frame. All12 cases finish:2,520 submitted frames,2,160 measured
frames, all1,080 measured GPU frames weighted. Under recorded contention,
median CPU/GPU FPS is35.32/33.68 for1p and11.61/8.96 for4p; all2,160 measured
frames miss their target deadline. These are diagnostics, with no accepted
gain or clean qualification. Old strict REDs remain. The private CPU-skin
census build passes32 focused CTests. Actual1p CPU/GPU census distributions
complete, while both4p runs correctly reject their full1024-row table and
preserve dropped-count diagnostics. Its4096-row successor now completes all
four distributions with no dropped/fault/in-flight work across840 submitted
frames. The largest observed job is12,960 vertices; the earlier16,384-vertex
worker benefit does not qualify a production threshold for this workload.
The hardware-backed native component exposes an existing wrapper's
failure to restore x87 C1 after a precision-store fault in both software roles.
Its distinct exact-restoration successor now passes all eleven normal shapes,
657 checked faults,42 callee exceptions and11 fallback controls. Ordinary
contexts, trusted activation and a measured benefit remain open.
[Current profiles and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-runtime-baseline-20261023/diagnostics-routes/TERMINAL_SUMMARY.json).

Earlier component result: private GPU-skin structural-validation reuse passes
Release on RTX5080. Cache/full controls produce the same complete outputs and
151 semantic events; actual record-validation visits fall813→186 with214 reuse
hits. The first sanitizer run remains RED for7,360 leaked bytes in58 allocations
and an unknown-module teardown SEGV followed by timeout; attribution is open.
At that stage sanitizer, cost and ordinary-runtime gates remained open; that
result alone did not justify canonical/default enablement or an FPS claim.
[Execution receipt](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/resident-skin-validation-20261023/execution-release.json).

The subsequent bounds-only ASan/UBSan run passes. A driver-only control also
leaks without constructing a skin service; this isolates a baseline leak but
does not explain every prior allocation. A warm public-prepare cost diagnostic
passes complete GPU output and work gates. At1,024 records, median reuse/full
cost is2.027/9.686 microseconds; the full control includes new summary/counter
work absent from unchanged canonical code, so this is mechanism evidence only.
That initial diagnostic alone did not justify promotion.
[Bounds](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/resident-skin-validation-20261023/execution-bounds.json),
[driver control](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/resident-skin-validation-20261023/execution-driver-lsan.json)
and [component cost](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/resident-skin-validation-20261023/execution-cost.json).

A separate fair comparison now compiles exact canonical CPP/HPP bodies with
token-only type renaming and verifies their byte-exact inverses. Median warm
candidate/canonical prepare cost is1.366/1.933us at64 records,1.810/8.479us at
1,024 and18.251/112.943us at16,384. All36 GPU output checks and209,664 complete
vertices pass with matching inputs/traffic/fullFP/errno. These are short
component batches under recorded build load; ordinary FPS/default/launcher
acceptance remains open. The exact logic is now integrated into the existing
opt-in service with persistent regressions. Official Release and ASan/UBSan
bounds runs pass cached/full controls and the intended require-cache failure;
outputs/151 semantic events match and the frozen runtime remains unchanged.
Leak qualification stays open; GPU defaults and launcher selection are unchanged.
[Fair source, measurements and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/resident-skin-validation-fair-cost-20261023/FINDINGS.md).

2026-10-23 user clarification: GPU lighting/shadow trials may use a
near-identical visual target with measured differences. Gameplay remains exact;
retain the source comparison route and distinguish rendering deviations from
simulation changes. No approximation is accepted merely from a faster result.

## Goal and invariants

- [ ] Verify 120 FPS at actual 3840x2160 internal rendering with one local player.
- [ ] Verify 60 FPS at actual 3840x2160 internal rendering with four local players.
- [x] Verify the four-player resolution contract: one 3840×2160 internal target
      split into a 2×2 grid gives each player a 1920×1080 viewport. The target
      is 60 FPS for that complete 4K frame, with no further per-player resolution
      reduction. Camera preparation repeats per player.
      [Viewport and auxiliary-pass audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/split-view-coverage-audit.json).
- [ ] Verify the qualified ordinary runtime through the installed launcher.
- [ ] Preserve OG Xbox TU1.5 gameplay, simulation timing and visual quality.
- [ ] Preserve authored visibility, LOD, lighting, effects, HUD, material order
      and floating-point behavior; retain required source ownership and effects.
- [ ] Finish with reproducible commands, hashes, frame-time reports and verified
      removal of task-owned scratch/build outputs.

## Evidence already established

- [x] Audit the owned MCC Classic DLL with exact version/hash/callsites.
      [Reference mechanisms](MCC_CLASSIC_PERFORMANCE_REFERENCE_20261022.md).
- [x] Identify translated execution and memory/observer overhead as the largest
      measured CPU opportunities in the selected runtime's retained profile.
      [Assessment and gain estimates](MCC_PERFORMANCE_OPPORTUNITIES_20261022.md).
- [x] Verify that the earlier skinning-only opportunity cannot close the full gap.
- [x] Run a fresh red-capable ordinary-runtime deadline check: all three cases
      miss the 120 FPS deadline in 120/120 sampled frames. Retain the actual
      compositor-clamped dimensions and host-load limitations rather than
      claiming a new 4K result.
      [Diagnostic evidence](../build/test-results/mcc-performance-audit-20261022/validation.json).
- [x] Verify cleanup for that completed read-only investigation; no large
      generated artifacts retained.
- [x] Audit remaining current-source CPU candidates and reject a new topology
      cache for the already cheap write lookup. A pure branch specialization
      saves about1 ns per isolated dispatch; fresh profiling must establish its
      gameplay importance before implementation.
      [Next CPU owner evidence](MCC_PERFORMANCE_NEXT_CPU_OWNER_20261022.md).
- [x] Authenticate the historically hot plane-volume MOPP query and both output
      streams against the original TU1.5 routines and their consumers. Existing
      native ray/integer visitors are different owners; the146-case shadow
      fixture does not cover this query.
      [Spatial owner and required proof](MCC_PERFORMANCE_SPATIAL_OWNER_20261022.md).

## Measurement loop

The previous finite source/build owner ended when a malformed FIFO request
closed its session. Its supervisor removed the temporary builds, cache seed
and large oracle inputs; completed reports and canonical changes survive.
The rendered native/reference controller never started. Recovery requires a
fresh declared source cohort, focused qualification and a newly acquired cache
input before further runtime comparisons.
[Incident evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-pixel-launch-failure.json).

- [x] Reproduce the session-protocol failure, reject malformed requests without
      closing the owner, and prove subsequent commands execute. The original
      parser fails; the fixed parser passes13 malformed-request controls and
      seven live submission-helper controls. No GPU work or large retention.
      [Protocol controls](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/session-protocol-controls.json).
- [x] Admit the fresh recovery cohort with explicit source drift:16,258 source
      inputs,32 focused CTests and97 authenticated shaders. Acquire the actual
      current-key9,681,379B pipeline cache and verify its import in all four
      fresh attribution runs. Preserve the unchanged baseline while evaluating
      new canonical opt-in skin-validation code separately.
      [Build admission](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-runtime-baseline-20261023/build-admitted.json),
      [cache acquisition](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-runtime-baseline-20261023/vulkan-cache-seed.json).
- [x] Build the declared four-substitution/one-addition internal-color and
      GPU-validation successor. All32 focused tests pass; independent terminal
      review rehashes16,259 inputs, five binaries, maps and97 compiled shaders.
      Native`486a31ce…` has no inherited pixel/FPS acceptance. The copied cache
      is bound as a compatibility input; every runtime role must prove import.
      [Independent terminal review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ROOT_TERMINAL_BUILD_REVIEW.json).
- [x] Complete the successor's bounded ordinary rendered differential: all
      four neutral/fire1p/4p CPU/GPU pairs match all24,883,200 internal4K RGB
      payload bytes, low24 depth/stencil, final printed source/RNG summaries
      and per-submission prefix counters. All952 actual worker frames pass;
      GPU deformation processes12,917,688 vertices across476 frames without
      fallback/readback. All eight roles import the cache and pass validation;
      raw captures and processes are removed. Physical presentation is720p.
      Complete camera/all-node/ordered-state traces, alpha, presented RGB,
      latency, broader gameplay and launcher acceptance remain open.
      [Independent rendered review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ROOT_PARITY_TERMINAL_REVIEW.json).
- [x] Prepare the existing renderer's full4K D24S8 capture comparison and pass
      19 comparator controls. Compare all meaningful low24 depth bits and
      all stencil bytes; ignore the undefined high8 bits in depth transfers.
      Single-bit changes, missing/truncated data and invalid capture receipts
      fail. These controls do not establish native rendered equality.
      [Comparator controls](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/recovery-runtime/pixel-depth-comparator-controls.json).

- [x] Freeze and hash a scoped current-source baseline, preserving unrelated
      changes and the launcher-selected runtime. Record source/build drift.
      The active third runtime session pins8,518 copied source inputs;
      existing game/reference/dependency inputs remain external links. Its
      manifest records four files changed since the second source snapshot.
      [Session](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/session/session.json)
      and [source receipt](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/freeze.json).
- [x] Build the required current-source diagnostic/runtime targets through
      `tools/run_tests.py` within an owned disposable context. The third
      snapshot built successfully and all seven focused CTests passed.
      Runtime SHA-256:
      `efe69297809e687bb928dba551934e5a4be2aaeffebfc14e24b759463df5bcee`.
      [Build/test report](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/validation/report.json).
- [x] Pin logical internal scene resolution independently of window size and
      assert actual render/presentation dimensions in every report.
      The CLI/benchmark implementation and three runner-contract tests pass;
      admitted native runs prove 3840×2160 internal rendering. Mid-run
      presentation changes were rejected and retained as failed evidence.
- [x] Profile the current baseline at one, two and four local players, separating
      source simulation, per-view preparation, renderer CPU work and waits.
- [x] Record host contention and collect repeated unsampled comparisons with
      identical runtime flags, assets, map/camera and player count.
- [x] Keep instrumentation overhead out of FPS acceptance; retain admitted
      frame-time tables, failure diagnostics and counterfactual controls.
      [Current runtime pass and limits](MCC_PERFORMANCE_RUNTIME_PASS_20261022.md).
- [ ] Collect GPU timestamp/API evidence when the CPU improvements expose a GPU
      bottleneck; host renderer intervals alone are not GPU execution time.

## Implementation, ordered by measured cost

- [x] First component: resolve retained vector operands once instead of four
      scalar resolutions; preserve ordered device reads, faults and wraparound.
      12,536 comparisons pass. The pre-change component gate fails; the first
      candidate improves median CPU cost from21.52 ns to7.75 ns (2.78x).
      [Source-pinned comparison](../build/test-results/mcc-performance-goal-20261022/vector-read/session.json).
- [x] Admit that vector-read change in a fully rebuilt ordinary runtime and
      measure its integrated frame-time effect with the same-executable
      `HALO2_REFERENCE_SCALAR_VECTOR_READS` control.
      Four complete 4K RGB comparisons (neutral/fire, one/four players) match
      with zero validation errors. Integrated speedup remains qualified by
      host variation, rejected dimension-changing runs and control overhead;
      this does not pass either final FPS target.
- [ ] Optimize hot translated memory/instruction paths while preserving complete
      original results, exceptions, source callbacks and observer effects.
      The private-state alias qualifier produced identical tested assembly in
      GCC and Clang. A wrapper retaining the original checked read and failure
      prefix added overhead without a demonstrated gain. Neither is admitted;
      A private three-unit LTO trial now tests caller/memory specialization;
      its normal runtime and archives remain preservation inputs.
      [Rejected alias and exception-edge hypotheses](MCC_PERFORMANCE_PRIVATE_STATE_ALIAS_20261022.md).
- [ ] Measure actual integrated improvement and reject regressions or gains
      explained by changing host load.
- [x] Second measured CPU component: prepare invariant motion-sensor sampling
      metadata once per covered blip/sweep/mask and borrow one private output
      writer. The corrected pre-change speed gate fails; the candidate passes
      at 1.414 ms → 1.106 ms (1.279x) for the live 128-pixel raster. All 339
      exact RGBA/metadata/FP/errno cases and 2,712 copy-isolation checks pass.
      Actual-artwork HUD/viewport regressions and two independent original-xemu
      radar pixel captures pass with zero differing channels. Ten CTests pass;
      one separate original motion-sensor input fixture is unavailable/skipped.
      [Component proof](../build/test-results/mcc-performance-goal-20261022/motion-composite/green-component-gate.json)
      and [original pixels](../build/test-results/mcc-performance-goal-20261022/motion-composite/original-pixel-captures.json).
- [x] Admit the motion-sensor change's full 4K neutral/fire image cohort and
      repeated ordinary-runtime FPS comparisons. Use the preserved qualified
      binary as the reference, with the same vector path in both executables.
      All four images match exactly, with zero validation errors; all twelve
      510-sample FPS runs complete. Rising host load makes their whole-frame
      effect inconclusive, and every sample misses its 120/60 FPS deadline.
      The 339-case ASan/UBSan differential also passes. This completes the
      measurement, not the FPS or delivery gates.
      Runtime hashes and the single changed engine file are recorded in
      [admission](../build/test-results/mcc-performance-goal-20261022/motion-composite/runtime-admission.json).
- [x] Admit or reject selective LTO using actual public-symbol/link-map closure,
      changed hot code generation, callback/fault/FP checks linked to the LTO
      objects, and ordinary-runtime evidence. Preserve all checks and the
      generic adapters; no project-wide IPO or source semantic shortcuts.
      [Bounded trial design](MCC_PERFORMANCE_SELECTIVE_LTO_20261022.md).
      [Execution and remaining gates](MCC_PERFORMANCE_SELECTIVE_LTO_TRIAL_20261022.md).
      The three-unit trial passes twelve semantic CTests and four exact 4K
      image pairs, but four of six fixed-work CPU pairs cost more CPU time.
      No consistently positive result was established. LTO is rejected for
      production; ordinary binaries/objects/archives are preserved and private
      trial artifacts are removed. This does not certify a whole-frame gain.
- [ ] Reduce verified repeated model/shadow/visibility preparation across
      compatible calls; retain camera-specific selection and mutable source
      owner boundaries.
      The [selected WORLD skin trial](MCC_PERFORMANCE_SELECTED_SKIN_TRIAL_20261022.md)
      passes its red/green component gate and 384 exact FP/vertex cases. Its
      synthetic repeated workload improves 9.160x. The four-view live differential
      passes 36 exact selected submissions with 17/32 selected requests reused;
      all four complete 4K neutral/fire image pairs match with zero Vulkan
      validation errors. All twelve ordinary FPS runs complete; every one of
      2,880 sampled frames misses its deadline. Four-player pairs improve
      7.2%/25.1%/19.0%, but reference drift and mixed single-player results leave
      isolated CPU attribution and stable target acceptance open.
      The corrected equal-work diagnostic matches all source/geometry
      signatures and eliminates 378/1,080 selected requests per run. Its CPU
      ratios are 1.024x/1.022x/0.977x, so a consistently positive whole-frame
      CPU gain has not been established. Completed trial copies, private
      assemblies/helpers and child owners are removed. The finite incremental
      session has subsequently closed and its entire owner is verified removed;
      the next executable experiment needs a new declared source snapshot.
- [ ] Reuse immutable hierarchy/section/palette inputs where profiling and full
      cache identities justify it.
- [ ] Expand resident raw GPU geometry/skinning where it provides a measured
      benefit and complete declaration/program/pass admission is available.
- [ ] Apply resource/state caching, changed-range uploads, buffer rotation,
      compatible instancing or parallel read-only preparation as remaining
      profiles justify them. Do not claim MCC's static code presence proves
      its live scheduling or an available percentage gain.

## CPU/GPU offload track

- [x] Implement and component-qualify MCC-style retained skin-input storage:
      persistently mapped coherent bank, exact CPU record shadow, palette-only
      warm updates, allocation-generation-qualified descriptor reuse and
      same-binary streaming control. Preserve all per-call admission and fault
      controls. [Implementation and evidence](MCC_GPU_SKIN_RESIDENT_INPUT_20261023.md).
- [ ] Establish an ordinary FPS benefit and full current runtime quality for
      that retained-input candidate; traffic reduction alone is insufficient.
- [ ] Qualify reuse of GPU-skin structural validation for byte-identical
      successful nonempty records, exact destination extent and palette row
      count. Preserve fresh full palette/used-row checks, complete caller
      arithmetic admission, all failure ordering and lease generations. The
      reviewed seam uses an owned512-byte row mask only through4096 rows;
      larger banks and changed records keep literal validation. A private
      source prototype is being prepared; compilation, output/fault tests and
      performance proof are open.
      [Validation-reuse source audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/GPU_SKIN_VALIDATION_REUSE_AUDIT.md).
- [x] Fresh actual4K/720p-presented main-thread profiles for CPU/retained GPU,
      one/four players:8,292 bounded samples, real worker/GPU route receipts and
      raw cleanup. Source effect-light/shadow costs overlap; no CPU/GPU total
      percentage or speedup follows from them.
      [World CPU/GPU split and concrete next boundaries](MCC_WORLD_RENDERING_GPU_SPLIT_20261023.md).
- [ ] Measure actual world/light submission calls, draw counts and host time;
      implement generation/fence-qualified resource reuse only if this owner
      materially contributes to ordinary frame cost. Triangle-metadata resources
      belong to an alternate backend; ordinary Linux uses indexed geometry.
      Keep source draw admission/callback order and mutable palette updates exact.
- [x] Add reviewed actual native-world prepare/record and source-world storage
      counters with inclusive host timers, plus sampled CPU-skin caller return
      addresses. Preserve rendering policies and explicit missing legacy fields.
      [Census protocol](MCC_WORLD_SUBMISSION_CENSUS_20261023.md).
- [x] Complete the fresh frozen-runtime census: four ordinary profiles,
      10,775 CPU samples and840 sampled frames. All raw native-world/storage
      counters are zero; do not implement a cache of this inactive owner as a
      neutral-scene fix. TU1.5 execution dominates49–60% inclusive CPU samples;
      distinct memory/validation/lookup/observer leaves account for28–35%.
      [Measured modernization priorities](MCC_TU15_MODERN_EXECUTION_20261023.md).
- [x] Validate positive counter wiring with the existing authored-light GPU
      fixture:15 rendered layouts pass, final25 timing rows record32 native
      draws/40 uploaded descriptor draws, and all75 raw captures are removed.
- [ ] Implement a bounded native hot visibility/plane-query owner with qualified
      memory access and original source/ABI/fault/observer effects preserved.
  - [x] Prioritize native hot TU1.5 kernels per the user's latest instruction;
        original gameplay remains exact. Full original2E79A0 corpus passes
        1,398 queries plus five natural failure controls, exercising2E6810
        2,561 times. This establishes the original reference, not C++ parity.
  - [x] Implement the first bounded27-instruction SIMD prefix at2E6810–2E686F,
        qualified read spans, literal fallback and real admission counters.
        Initial strict testing exposed a NaN-payload difference; original
        execution agrees with the native result. Conservative numeric refusal
        now preserves the existing literal payload and complete FP state.
  - [x] Pass strict prefix regression:128 original contexts,4,757 cases,
        4,226 actual admissions,38,034 vector bridges removed and513 explicit
        numeric refusals. Complete GP/XMM/flags, source/host FP, errno, cursor,
        missing operands and all reference controls pass; memory tests pass.
  - [x] Pass the qualified whole-query differential:1,398 normal queries and
        five natural fault prefixes in both modes, including all415 hot queries
        (5,122 native executions across observer pairs). Qualified original
        normal ABI differences are zero; complete native/literal source and
        caller FP status, ordered stores, observers and memory remain strict.
        The final canonical harness passes both modes; explicit emulator-C1 and
        original-fault ABI probes fail at their declared gates.
        [Final query evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-query-final/summary-and-cleanup.json).
  - [ ] Close full original FP/fault authority, prove ordinary runtime admission
        and measure actual benefit. There are583 emulator C1 disagreements and
        two original fault ABI differences shared by both modes. A genuine
        original FSTP operand proves identical physical/emulated float output
        with different C1/inexact status; emulator C1 authority is limited.
        Three10,000-call public-query component pairs have mixed CPU ratios
        (0.941x/1.028x/1.014x reference/native), establishing no accepted gain.
        The rendered pilot times out before its first frame. Bounded debugger
        stacks plus linked return-PC disassembly identify NVIDIA compilation
        of a forward-material graphics pipeline under cold isolated caches and
        Vulkan validation; this is distinct from steady-state performance.
  - [x] Pass standalone helper/memory ASan+UBSan with leak checking enabled:
        4,757 native cases and149 reference cases; all506 inputs unchanged and
        scratch removed. Assembly loads have the documented instrumentation
        limitation; public-wrapper/runtime proof remains separate.
  - [x] Isolate the actual prefix's CPU cost from whole-query setup: three
        alternating956,928-call pairs retain complete128-context pre/post
        state/FP/memory checks and2,870,784 actual admissions. Median native
        thread CPU is43.9% of the frozen literal prefix, including equal reset
        costs. Host contention invalidates wall-time attribution; this does
        not establish an ordinary FPS gain. Temporary binary/scratch removed.
        [Pure-prefix evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-pure-timing.json).
  - [x] Extend proved native coverage to the sibling read-only prefixes at
        2E66B2 and2E6A50. Preserve ordered store tails and complete ABI effects.
        An isolated common-body prototype passes24 ASan/UBSan controls with
        leak checking,4,763 strict cases per prefix. Only2E6810's128 contexts
        are original; both128-context sibling datasets are explicitly synthetic
        relocations. A subsequent four-shard original-only replay completes all
        1,019 selected queries and authenticates2,523/2,431 unique real sibling
        contexts. Private ASan/UBSan qualification passes all24 controls and
        83,798/80,762 strict sibling cases, with83,261/80,225 admissions.
        Canonical integration adds only two guard/resume pairs; the stripped
        literal remainder is byte-identical. GCC/Clang each pass32 sanitizer
        controls and169,323 full-matrix cases. The official integrated rebuild
        passes all32 focused CTests and links native`75fccef3…`;97 shaders and
        the renderer remain unchanged. Full1,398-normal/five-fault native and
        global-reference query comparisons pass with15,440 exact native entries
        (5,094/5,122/5,224) and complete native/literal state/memory agreement.
        Existing original-C1/fault authority probes remain red. Rendered
        parity and FPS gates remain open.
        [Prototype and limits](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-sibling-prototype.json).
        [Completed original capture and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/plane-query-sibling-shards/execution.json).
        [Real-context native qualification](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-sibling-original.json).
        [Integrated full-query evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/full-query/worker.json).
  - [ ] Qualify capability hoisting across the larger no-store classifier loop,
        preserving reject2E68B5, partial-mask exhaustion2E68B9 and complete-mask
        exhaustion2E688E, reloads, precise faults and FP state. Measure a
        substantial whole-owner improvement before broader adoption.
        Recovery replay matches all1,403 historical original case summaries,
        instruction/access/store hashes and hot counts. Corrected private
        capture then authenticates415 reference/capture query pairs and2,151
        weighted phases (2,127 unique),2,561 iterations and410 reloads, including
        all three exits. The compiled prototype passes12/13 sanitizer controls
        but fails complete host FP/errno equality on a synthetic edge. A minimal
        diagnostic identifies unequal x87 metadata already at test entry; kernel
        outputs/MXCSR match. Restoring the same complete host environment
        immediately before each kernel passes the minimized514-case native
        and reference runs, including full32-byte entry/output FP state and
        errno. The helper and numerical comparisons are unchanged. The full
        rerun then passes all13 sanitizer/leak controls and70,748 native cases,
        with70,193 admissions, using a fresh415-reference/415-capture cohort.
        This initial pass qualified disjoint State/cursor/source backing only.
        The subsequent direct cursor-overlap regression and conservative
        fallback are qualified separately below. It remains private and no
        speedup is accepted. Ended raw storage and10,516,656B binary are removed;
        the final strict increment comparison then uses the exact compiled
        current27-PC prefix plus literal tail with full State publication
        barriers. Three thread-CPU ratios are0.8180/0.7880/0.8553, median saving
        15.240ns per phase. This is contended component evidence, not a
        whole-query/FPS gain; the40-PC candidate remains private. All timing
        scratch/binaries and the13,376,211B temporary reference are removed.
        [Fresh original replay](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-classifier-phase-independent-attempt5/report.json)
        and [phase capture / strict component failure](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-classifier-phase-independent-attempt7/report.json).
        [Balanced minimized diagnostic and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-classifier-phase-edge-diagnostic3/cleanup.json).
        [Full strict disjoint-cursor component and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-classifier-phase-independent-attempt8/cleanup.json).
        [Final exact-compiled-form increment](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-classifier-phase-increment-timing3/report.json).
  - [x] Reproduce and repair the public cursor/source-backing alias admission
        gap in focused compiler/sanitizer controls. A real shared uint32 backing reproduces
        XMM1`002E6807` versus literal`002E6810` while final bank/cursor match;
        capability refusal and global reference restore equality. The new
        persistent regression also fails with the old header on source FP
        state after its128 ordinary quick contexts pass. A conservative
        integer-address overlap guard runs before arithmetic, with
        all-nine-operand regressions. GCC and Clang each pass30 prefix controls
        under ASan/UBSan/leak checking:6,924 matrix cases and5,286 admissions.
        All ended sanitizer scratch/binaries are removed. The preceding
        disjoint runtime was kept frozen through its rendered attempt before
        the explicit two-file guarded refresh below.
        [Direct red reproduction](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-alias-probe/report.json).
        [GCC qualification](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-cursor-alias/gcc-asan-green.json)
        and [Clang qualification](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-cursor-alias/clang-asan-green.json).
  - [x] Rebuild the overlap-guard successor through the official worker,
        including both whole-query tests in the32-test focused filter.
        All five targets and32 checks pass; native`9e633e5f…` is distinct from
        the preceding runtime. The16,168 other frozen inputs, generated source
        and97 shaders are unchanged. Old source bytes/inventories/admissions
        are archived; this does not transfer prior pixel/FPS acceptance.
        [Successor admission](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-cursor-alias/runtime-successor/build-admitted.json).
  - [x] Qualify the guarded successor's bounded ordinary rendered differential
        and actual route counters. All four neutral/fire1p/4p pairs match
        complete1280x720 presented RGB, internal3840x2160 low24 depth/stencil,
        printed source/RNG/input/player fields and non-timing counters. Each
        role has119 submitted frames at source tick120; native records
        2,510,918 admissions and reference admits none. Split-stream capture
        preserves complete stdout summaries and stderr cache/validation logs.
        All capture/raw children are removed. Complete source/camera ABI,
        ordered per-invocation trace, full internal4K RGB, FPS and launcher
        acceptance remain open.
        [Sealed four-pair evidence and limits](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/recovery-runtime/ordinary-rendered-depth-streams/final-evidence.json).
  - [ ] Audit a whole2E6290 native owner with private register/flag locals.
        Prove memory/State non-aliasing and exact publication at calls, faults
        and observer-visible boundaries before measuring it. Trace activation
        during callbacks needs an exact continuation; borrowed views still
        expire before source stores/calls. This is an unproved next hypothesis,
        not an accepted gain or permission to remove source effects.
        The source audit identifies eight direct calls plus five dispatch
        boundaries, including growth/virtual/error paths. The public entry
        cannot resume arbitrary interior PCs, and existing10-case fixtures do
        not qualify growth/fatal owners or arbitrary callbacks.
        [Whole-owner boundary map](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/WHOLE_2E6290_NATIVE_BOUNDARIES.md).
  - [ ] Prototype deferred integer flags within the whole2E6290 owner while
        retaining all GP/XMM operations, checked accesses, cursor delivery and
        callbacks. The audit identifies173 producers and32 branches using
        CF/ZF/SF/OF; PF/AF still require exact return/fault snapshots. Preserve
        zero-count shifts, prior OF/CF where the baseline retains them, and
        synchronize13 original calls plus three native-prefix helpers on
        success and exception. The private component passes10,772,096 operations
        and27,907,968 state comparisons under GCC Release plus GCC/Clang
        ASan/UBSan/leak checks, with complete host FP/errno and4,096 scoped
        return/unwind/callee-publication cases per cohort. All component scratch
        is removed. The shared-body whole-owner adapter compiles as one primary
        routine plus its cold exception section, without role-specific clones.
        Its first functional run remains RED: an injected call-push failure at
        2E690D matches saved ABI, banks, ordered effects and restored public FP
        state, but differs in pre-restoration host FP metadata. The minimized
        unwind diagnostic retains all32-byte checks; full owner/dispatch and
        timing proof remain open. All ended scratch and the10,661,040B binary
        are removed; complete disassembly is losslessly retained in136,728B.
        [First whole-owner execution](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-whole-owner-flags/attempt2.json).
        Additional prethrow/catch observation passes the minimized eight-role
        diagnostic and full40-query/8-callee/5-INC/5-owner-injection controls,
        with65 entries per role,68 native-prefix admissions, zero unsupported
        internal dispatches and the intended missing-publication negative
        control. All three complete32-byte environment checks stay strict.
        Instrumentation changes the failing path, so this does not resolve
        the original RED. Hardware-state investigation and a private cost
        diagnostic on the five completed queries are separate open work.
        [Instrumented controls](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-whole-owner-flags/instrumented-strict1.json).
        The five-query private cost diagnostic then passes all normal source,
        work and three-stage FP/errno checks. Three deferred/eager thread-CPU
        ratios are0.8465/0.9757/1.0259; the mixed median2.4% reduction includes
        1,720,320B workspace reset per cycle and FP capture cost, under host
        contention. This does not justify promotion. Each measured role/pair
        completes8,490 queries,18,678 owner entries and57,732 native prefixes;
        checksums and work match. Ended scratch and12,585,032B binary are
        removed. The fresh four-route profiles below now complete that
        prerequisite; the flags trial remains unpromoted.
        [Private cost and limits](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-whole-owner-flags/query-cost2.json).
        Canonical activation and performance acceptance remain open.
        [Deferred-flags source audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/WHOLE_2E6290_DEFERRED_FLAGS_AUDIT.md).
        [Functional component handoff and limits](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-deferred-flags/handoff.json).
        [Independent boundary review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/WHOLE_2E6290_DEFERRED_FLAGS_INDEPENDENT_REVIEW.md)
        requires partial callee-ABI-load, INC fault-phase, trace activation and
        all16 bridge controls; component arithmetic alone does not close them.
  - [ ] Evaluate private general-register values independently with literal
        Flags; do not inherit the deferred-flags trial's unresolved acceptance.
        The source audit finds494 direct GP occurrences,342 accessor reads
        and393 writes, all with literal register indices. These are lexical
        sites, not measured redundant loads. A first version can leave ESP
        in State and hold seven other values privately; caching ESP also
        requires preserving40 push and15 pop sites, three return adjustments
        and their fault prefixes. Publish/reload at all16 bridges, preserve
        partial AL and failed callee ABI loads, then measure actual code and
        whole-query cost. A bounded private seven-register implementation is
        now in progress; ESP, Flags, source accesses and ordered effects stay
        literal. The fresh4p GPU owner leaf is only0.867% of samples, so adopt
        nothing from lexical counts and stop if whole-query cost is unclear.
        No canonical implementation or gain is established.
        [Private-register source audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/WHOLE_2E6290_PRIVATE_GP_AUDIT.md).
        The bounded private trial is now stopped:40 query comparisons,8
        partial-load controls and20 GP edges progressed, then the first real
        trace-flip injection failed the unchanged pre-scope host-FP metadata
        gate. ABI/banks/effects/PC, entry/public-exit FP and errno matched at
        that point; causality is unassigned and the gate was not relaxed.
        No timing ran. The shared adapter also adds488 out-of-line callsites
        and provides no clear compact value frame. All three raw owners and
        its12,024,632B binary are removed; no activation follows.
        [Terminal decision](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-whole-owner-gp/FINAL_HANDOFF.json).
  - [x] Reprofile the current baseline before choosing another native owner.
        All840 measured frames execute the worker;420 GPU frames deform
        11,397,960 weighted vertices with no fallback or verification readback.
        GPU prepare is0.224% in4p GPU, versus7.38% in2E6290 and45.76% in the
        broader source-runtime entry tree. Categories overlap. All four raw
        owners are removed; the reference baseline/cache remain temporarily
        owned. Do not derive FPS or speedup from sampled runs under build load.
        [Fresh terminal evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-runtime-baseline-20261023/diagnostics-routes/TERMINAL_SUMMARY.json).
  - [ ] Select and qualify the largest feasible bounded native semantic owner
        from the fresh source/physics tree; independently resolve retained
        CPU-skin callers before treating them as removable duplication.
  - [x] Retain complete sampled call contexts and original sampler inputs to
        resolve caller-path dilution. Independent raw reconstruction conserves
        all5,250 samples;2,659 have a deepest generated owner and2,591 remain
        explicitly outside that partition. `2E6290` leads with285 samples.
        Its existing three native prefixes already run, so a new corridor must
        address measured work beyond them. Native qualification remains open.
  - [x] Implement ordered dense traversal for the measured global material
        direction-remap scan, retaining the original scan as a control.
        Authenticate its exact inverse against the preceding canonical source
        and seal only that delta for the frozen renderer refresh.
  - [x] Qualify dense/reference complete material words, ordered dirty updates,
        partial failure behavior and32-byte fenv/errno in21 three-path cases;
        all21 actual omission/order negatives reject. This is not complete
        hardware FP-register metadata qualification.
        [Component results](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/renderer-view-direction-component-20261024/SUMMARY.json).
  - [x] Build the exact renderer delta and pass material residency, original
        shader fog and render-view resolution tests. An existing residency
        fixture carried mip metadata into a raw-image update API; the original
        scan reproduces that failure. Only the fixture payload is corrected.
        [Actual build closure](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/renderer-direction-scan-20261024/FULL_CLOSURE_AND_CACHE_BINDING.json).
  - [x] Compare actual internal4K output for1p/4p neutral/fire with retained
        GPU skinning on both roles. All four pairs match complete RGB,
        meaningful depth/stencil, final summaries and all non-timing counters.
        Every one of952 submitted frames runs the worker and weighted GPU skin.
        [Actual four-pair result](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/renderer-direction-scan-20261024/parity-preparation/results/report.json).
  - [x] Measure ordinary dense/reference frame times without profiler/capture
        overhead. All12 cases complete2520 submitted/2160 measured frames,
        preserving actual worker, weighted measured GPU work and cache imports.
        Foreign builds overlap every case; all2160 measured deadlines miss.
        Retain observed values and original tables without accepting an FPS gain.
  - [ ] Qualify an isolated ordinary improvement and the120/60FPS targets.
  - [x] Evaluate exact checked-write destination reuse and stop the private
        trial without promotion after mixed matched cost. The first bank-local
        generation design fails a genuine shared-backing alias counterexample;
        its repaired private revision passes113 paired cases, but does not
        establish a material ordinary benefit. Both memory owners are unchanged.
        [Preserved RED, repaired proof, cost and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-exact-write-20261024/TERMINAL.json).
  - [x] Evaluate the first larger plane-classifier corridor2E6627–2E6748
        against actual emitted callsites. Its conservative58-sample bound
        includes48 checked-store samples and4 already in the native prefix.
        Those stores retain ordered mutation/continuation serial effects;
        respelling the arithmetic does not justify removing that cost. Stop
        this narrow candidate without a component or promotion. The other227
        owner samples remain unresolved by this mapping, and ordinary entry
        state/banks and scratch/policy ownership are still missing.
        [Exact corridor and rejected reduction](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/REJECTED_FIRST_CLASSIFIER_CORRIDOR.json).
  - [x] Resolve the retained recognized CPU-skin contexts against the exact
        baseline. All113/326 recognized samples fit the retained rows, with
        explicit stack/inlining limits. Dynamic objects account30/114 and
        scenery27/50; both pose all six LODs before selection. The dynamic
        definitions are eqip/garb/bloc/proj, not player bipeds. Actual ordinary
        GPU work coexists with these separate consumers; no duplicate admitted
        character-base deformation is established. Frame vehicle pose reuse
        already exists; several direct-main consumers remain unresolved.
        [Consumer evidence and probe cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/retained-skin-consumers-20261023/FINDINGS.md).
  - [ ] Evaluate a same-frame, typed CPU skin-kernel split for the remaining
        consumers. Keep each LOD's original allocation, callback, selection,
        lighting and publication boundaries; join before consumption. Qualify
        complete vertices, FP/errno, faults, lifetimes and actual queue/join
        cost before canonical integration. Arbitrary pose callbacks are not
        parallel jobs, and skipping unselected LODs is not yet proved safe.
        A private shared-kernel prototype passes2,048 complete257-vertex pairs
        with full32-byte FP/errno checks from an explicitly zeroed metadata
        baseline, original fault prefixes and preflight refusal/after-flight
        join controls. Candidate includes gate/queue/join. Median serial/split
        wall cost is6.75/10.30us at1,024 vertices,129.86/110.72us at16,384 and
        491.90/402.91us at65,536 under14 observed build processes. Total CPU
        rises. No real-model size distribution, arbitrary nonzero metadata,
        sanitizer, integration or frame gain is qualified. Actual job sizes
        are the next gate; do not choose a production threshold from this run.
        [Component results and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/world-skin-worker-prototype-20261023/FINDINGS.md).
        A private three-TU census now preserves all original LOD/kernel calls
        and labels dynamic/scenery jobs without guessing unknown source ticks
        or owners. Its header/shared-kernel probe passes3,072 complete
        vertex/32-byte FP/errno pairs, four fault/verification pairs and
        fixed-table overflow/output-failure controls. All three modified TUs
        compile with the exact frozen Ninja flags; every actual project
        dependency matches the full source inventory. Scratch objects are
        removed. A declared incremental refresh now links native`e34c8c46…`
        and passes32 focused CTests. Independent review verifies exactly three
        TU substitutions and one header addition across16,260 frozen files;
        prior runtime references and97 unchanged shaders are preserved. The
        runner passes10 adapter,104 parser and66 inert authentication controls.
        Actual1p CPU/GPU distributions complete:45,099/44,241 calls including
        startup and210 rendered frames. Their elapsed source ticks differ, so
        total work is not a reduction comparison. Both4p backends pass their
        rendering/cache checks but their1024-row census is incomplete:
        55,496/53,968 calls lack per-key rows. Original reports remain RED.
        The declared4096-row successor preserves all keys/labels and bounds
        lossless compressed retention. Instrumentation cannot qualify FPS.
        No threshold, skipped LOD or frame gain is admitted.
        [Header proof](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/cpu-skin-job-census-proposal/HEADER_PROBE_FINDINGS.md)
        and [exact three-TU compilation](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/cpu-skin-job-census-proposal/THREE_TU_COMPILE_FINDINGS.md).
  - [x] Complete the4096-row ordinary CPU-skin census for CPU/retained GPU and
        one/four players. All840 submitted frames pass route/cache/worker
        checks. Four whole-process call/vertex totals are47,333/36,093,957;
        47,546/34,709,199;160,357/103,007,190;156,584/93,700,524. Source ticks
        differ, so these totals do not measure a CPU/GPU work reduction.
        Dynamic jobs reach2,130 vertices, scenery1,686 and unlabelled jobs
        12,960. No observed job reaches16,384; no safe worker threshold follows
        from the earlier larger synthetic timings. All original rows and full
        observations are retained losslessly; all case owners/processes are
        removed. Role0 provenance, safe batching and measured benefit remain
        open.
        [Independent complete census review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/cpu-skin-job-census-proposal/diagnostics-preparation/results-4096/ROOT_TERMINAL_REVIEW.json).
  - [ ] Qualify a checked native scalar-return read bridge against an unchanged
        literal control. Retain per-access bounds/commit/wrap/cross-region
        faults, custom memory callbacks and reference modes; remove only the
        temporary-span/byte-pack return ABI. Source page indexing, small
        transfers and write-policy epoch caching already exist. This is a
        private implementation/cost trial, not canonical activation. The
        component passes33,031 reads per cohort with fault/custom-callback
        controls; hot32-bit reads save about0.8ns, with mixed other widths and
        a slower3-byte fallback. The unchanged strict query stops on row18:
        ordered reads, ABI/banks/stores and public FP/errno match, while the
        pre-restoration host error-pointer metadata differs. Cause remains
        unassigned. All raw owners and17,001,616B of binaries are removed;
        no whole-query timing, canonical adoption or frame gain follows.
        [Sealed scalar trial](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-scalar-read-bridge/FINAL_HANDOFF.json).
  - [x] Audit host x87 metadata consumers before defining another native-owner
        qualification. All16,259 successor inputs match their pins. No trusted
        gameplay/serialized-state FIP/FDP/opcode consumer is identified; logical
        guest FP and complete caller-environment restoration remain strict.
        Public custom memory/policy/observer callbacks remain unqualified, so
        this supplies no global gate removal or reinterpretation of old REDs.
        An enforced trusted ordinary-owner admission is still open.
        [Pinned consumer audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/host-fenv-fidelity-audit-20261023/AUDIT.md).
  - [ ] Qualify a typed whole CB520 seat-geometry predicate with its five
        original callees retained in order. Source/stack evidence selects this
        bounded owner; no authenticated ordinary seat corpus or native gain
        exists yet. Keep original caller proof, genuine runtime contexts,
        trusted admission and generic fallback as separate gates. Its sampled
        inclusive cost overlaps its callees and is not a predicted saving.
        [Native-owner ranking and obligations](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/next-native-owner-shortlist-20261023/SHORTLIST.md).
        Original TU1.5 bytes now independently authenticate all121 instruction
        starts, the441-byte caller and all five call sites. Literal extraction
        reverses exactly; a private structured native candidate is staged.
        The first private execution caught a missing spilled-float reload;
        its corrected successor passes nine caller shapes,521 access-fault
        pairs and32 callee-exception pairs, then stops on a precision-flag
        disagreement with Unicorn. Physical execution of the four original
        arithmetic opcodes confirms that FST32 sets PE while Unicorn misses
        it for this input; values and stored bits agree. This calibration
        does not qualify complete caller/fault status or intermediate C1.
        Both failed attempts remain RED, with no field masked. Genuine
        ordinary contexts and complete callee integration remain unqualified.
        Synthetic callee hooks prove only the caller.
        [Original caller authority](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/authority.json).
        [Hardware status calibration and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/status-calibration/CALIBRATION.md).
        Seven narrow physical IA32/ptrace admission controls now pass, including
        unchanged complete flags, logical x87 C1/PE/active values, GP/XMM-only
        outputs and real CALL/RET stack behavior. All seven children and raw
        outputs are removed. This admits the capture mechanism; the complete
        caller capture was the next gate. All eleven complete original-caller
        hardware cases now pass:1,095 original instructions,42 real CALL/RET
        pairs and657 inferred access events. Complete source arenas and
        selected guest ABI agree; the precision case captures hardware PE32
        where the historical emulator reports0. Raw hardware status is retained
        unchanged. A separate C++ qualification is prepared against these
        hardware exits and callee boundaries, with strict software per-PC ABI
        comparisons and checked-fault controls. Its first execution passes ten
        normal shapes and545 checked-fault pairs, then fails both literal and
        native public32-byte environment restoration: C1 remains set after a
        precision FST32 checked-write fault. Exact local libc implementation
        explains the old wrapper's status merge; no gate or expected word is
        changed. A distinct private full-environment scope now passes the
        unchanged complete eleven-case component, all657 checked-fault pairs,
        42 callee exceptions,11 fallbacks and1,095 normal software checkpoints.
        Explicit reference and both old omitted-read negative controls fail
        as required. All generated binaries/scratch are removed; bounded
        actual assembly passes independent normal/cold restoration-order review.
        The private contract still requires an empty physical host x87 stack.
        Intermediate
        hardware C1, genuine ordinary contexts, complete callees and native
        activation remain open; historical failed attempts remain RED.
        [Actual hardware admission](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/ia32-oracle-proposal/admission/execution.json).
        [Complete hardware caller and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/ia32-oracle-proposal/caller-prepared/ROOT_HARDWARE_REFERENCE_REVIEW.json).
        [Complete exact-scope component](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/hardware-component-exact-scope-v2/attempt-1/execution.json).
  - [x] Implement bounded genuine CB520 observation without replacing its
        five callees or current guarded instruction delivery. Removing the
        hooks reproduces the frozen source exactly. Six focused comparisons
        of original, disarmed and armed bodies pass, including a checked-store
        fault, callee exception, alias and trace controls; the fixed eight-record
        capacity and forced-disarmed rejection pass. This proves hook behavior
        with synthetic callees, not genuine ordinary execution. Its initial
        missing-helper compile failure remains preserved.
        [Focused capture evidence and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/ordinary-capture-inertness-v2/attempt-1/LIFECYCLE.json).
  - [x] Build the private capture generation through the actual specialization
        and function-splitting pipeline. The initial attempt fails before C++
        compilation because the wrapper changes the splitter's terminal pattern.
        A catch-placement-only successor preserves that pattern, passes the
        same six component comparisons, rebuilds all33 affected objects and
        passes32 focused CTests. Native`9d46833a…` is a new diagnostic cohort;
        its16,261-file scope and unchanged97 shaders are recorded. No native
        arithmetic activation or FPS acceptance follows from this build.
        [Build review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-c951c-capture-build-v3/ROOT_TERMINAL_REVIEW.json).
  - [x] Capture genuine ordinary caller/callee states and qualify same-native
        disarmed/armed output before native admission. First-eight capture is
        a bounded prefix; it cannot establish full input coverage or a complete
        ordered source trace. Keep the real LiveState/MMX domain explicit.
        All four neutral/fire1p/4p pairs pass complete internal4K RGB,
        meaningful depth/stencil, final summaries and non-timing work equality
        across952 submitted frames. The four armed runs count39,600 genuine
        calls, retaining the first eight records per workload. Each normal
        call must execute CD4A0; exactly one callee per call globally proves
        the later distance phase is inactive throughout these workloads.
        The hot target is the earlier distance/range calculation. Source banks,
        full ordered gameplay state and native arithmetic admission remain open.
        All nested raw/native owners are removed. The same finite source/build
        and required references retain3,535,403,561 logical bytes for the next
        kernel step; no large capture or new runtime copy is retained.
        [Actual four-pair review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-c951c-capture-preparation/results/ROOT_TERMINAL_REVIEW.json).
        [Cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-c951c-capture-preparation/TERMINAL_CLEANUP.json).
  - [x] Capture bounded real source-access inputs and immediate result snapshots
        for the hot first-distance calculation. Preserve its four float32 spills,
        subsequent reloads, aliases,
        checked-fault partial State and callbacks. A typed spelling alone is
        not a measured optimization; the unexecuted later phase supplies no
        performance opportunity in these workloads.
        The bounded next capture needs11 existing read values, four write
        values and the immediate CB5BC exit state. The observed candidate is
        CB5A2–CB5BA; preserve its extended-z times spilled-z distinction and
        duplicate reloads. The32 retained records contain16 distinct raw
        records and one arithmetic path, with startup bias and no source-bank
        or physical-host-stack inference.
        [Actual domain and next input proposal](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/ordinary-domain-observation/HOT_FIRST_DISTANCE_INPUT_PROPOSAL.md).
        The private15-event/phase-exit capture now passes exact source inverse,
        actual specialization/splitting, six original/disarmed/armed component
        comparisons and12 reader contracts. The precision-store fault retains
        its eight-event successful prefix and no completed phase. Original
        helpers/access counts/callees remain unchanged. The declared two-source
        incremental build now passes all32 focused CTests; all33 affected objects
        rebuilt and the97 shaders remain unchanged. Actual operand packets and
        native gains remain unqualified.
        [Capture qualification and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/ordinary-first-distance-capture-v4/LIFECYCLE.json).
        [Actual build review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-cb520-hot-distance-build/ROOT_TERMINAL_REVIEW.json).
        All four ordinary neutral/fire1p/4p capture0/1 pairs now pass across952
        submitted worker frames. Full internal4K RGB, meaningful depth/stencil,
        final source summaries and same-route counters match. The32 retained
        first-eight records contain480 operand events and32 phase snapshots;
        all eight original files replay through the strict reader. This remains
        bounded observation, with no full source-bank or ordered-trace proof.
        All completed capture raw/native owners are removed.
        [Four-pair review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-cb520-hot-distance-preparation/results/ROOT_TERMINAL_REVIEW.json).
  - [ ] Qualify and measure the native CB5A2–CB5BA kernel against the exact current
        guarded literal, including complete admission cost. Its private V2
        component now passes eight genuine operand cases and19 fallback/fault
        controls, plus reference and expected V1 errno-alias failure controls.
        Complete captured phase snapshots match. The initial unaligned test
        reservation failure is preserved; only that fixture was corrected.
        The first matched cost cohort now passes: three interleaved pairs measure
        literal/native101.213/61.440,100.122/64.256 and105.811/60.015ns per call.
        Every row executes1,761,136 calls; every native call is admitted. Actual
        stored State/bank/cursor and the original after-clock float spill match
        the genuine snapshots. The two compile-only timing-helper failures
        remain preserved. This is a small component saving under recorded host
        contention, with no ordinary/FPS gain accepted. Physical host-stack
        occupancy and ordinary activation remain open. No canonical/default/
        launcher adoption follows.
        [Private component](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/private-first-distance8-prototype/attempt-2/execution.json).
        [Matched cost](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/private-first-distance8-prototype/cost-attempt-3/execution.json).
  - [x] Rebuild the Linux-qualified first-prefix runtime/profiler and pass all
        ten focused CTests after the platform-gate and final query-harness
        correction. New native SHA`63ce085d…`; actual gameplay/FPS admission
        remains separate.
        [Build admission](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-perf/build-admitted.json).
  - [x] Resolve the pre-frame startup ambiguity with a bounded passthrough probe:
        577 graphics-pipeline calls complete in152.326s total (10.331s maximum),
        followed by210 actual4K-internal/720p-presented frames. Native exits
        normally in214.18s. Forwarding/FP/errno and cancellation controls pass;
        all completed probe scratch is removed. This is cumulative compilation,
        not evidence of one permanently stalled call.
        [Diagnostic and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-perf/vulkan-init-diagnostic/execution.json).
  - [x] Capture and authenticate a current-key pipeline-cache bank before
        cleanup. The supplied30.65MB bank has
        a stale UUID and neither cache receives its bytes. A naturally exported
        9.68MB current-key bank is observed but removed before stable acquisition;
        no available warm-cache input is claimed from its filename alone.
        A subsequent owned acquisition completes210 frames and retains the
        actual9,681,386B bank read-only with stable source identity, exact header
        and SHA. Its child/native/raw owners are removed.
        [Current-key cache receipt](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/vulkan-cache-seed.json).
  - [x] Verify actual import of the same current-key bank in both ordinary
        comparison modes and complete three interleaved pairs each1p/4p.
        All12 cases pass exact9,681,386B imports,69-column/210-frame extents,
        real worker/native-route counters, zero GPU-skin traffic and cleanup.
        Contended medians(reference/native) are35.970/49.522FPS(1p) and
        14.764/13.706FPS(4p). Wide pair variation prevents causal gain acceptance;
        all cases miss their120/60FPS target. The separate pilot is excluded.
        [Full cohort and limits](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/ordinary-fps-current-cache/report.json).
  - [ ] Establish repeatable uncontended whole-frame benefit and rendered/source
        equality for the three-site runtime. Proceed toward the larger native
        owner; the current arithmetic seams do not close the performance gap.
- [ ] Prototype early same-frame GPU static-world visibility/light/shadow query
      preparation; compare generated TU1.5, native CPU and GPU on identical
      inputs, including readback/wait. Qualify source ownership/gameplay and
      near-identical visual output before ordinary adoption.

The user explicitly requested implementation as a goal. The dedicated
[presentation offload checklist](PRESENTATION_OFFLOAD_GOAL_CHECKLIST.md) is
authoritative for its worker, ordinary-character GPU and 1:1 acceptance gates.

- [x] Audit current source and the pinned MCC binary for actual offload seams.
      Resident raw model/BSP geometry and admitted GPU vertex processing
      already exist. Ordinary base-model CPU skinning and serial host
      preparation remain candidates. See the
      [offload assessment](MCC_PERFORMANCE_OFFLOAD_20261022.md).
- [x] Prototype a worker on immutable completed-frame inputs for host
      palette/skinning or vertex packing. Preserve lifetime, math/FP controls,
      original result/effect ordering and the current presentation endpoint;
      join before consumption without introducing an extra frame of delay.
- [ ] Extend ordinary base-model GPU vertex processing through the existing raw
      geometry/palette path, family by family. Preserve declarations, weights,
      lighting/PRT, materials, shadows, shields and per-view LOD/section order;
      retain CPU nodes and required CPU geometry consumers.
- [ ] Measure per-thread command recording only after the preparation cost is
      isolated. Give workers independent command/descriptor pools and keep
      resource updates, dependency barriers and submission ownership explicit.
- [ ] Validate one/two/four-player outputs and gameplay/input/tick traces,
      synchronization/lifecycle failure paths, full GPU images and unsampled
      FPS. The source arenas, callbacks and source visibility/shadow writers
      cannot be parallelized merely by putting the current view loop in jobs.

## Correctness and runtime validation

- [ ] Red/green differential at the actual changed boundary: registers, flags,
      complete memory/vertex outputs, fault behavior and ordered effects.
- [ ] Relevant original-owner, lifecycle, source-memory and concurrency checks.
      Next material source candidate: obtain an actual before/after execution
      oracle for the authenticated `2E79A0`/`2E6290` plane-volume MOPP query
      and both ordered output streams. The 146-case shadow fixture does not
      cover this owner; no native replacement is admitted without that proof.
- [ ] Relevant sanitizer checks where ownership or memory accesses change.
- [ ] One/two/four-view integration, including different cameras, owner reuse,
      vehicle entry/exit, attachments and interpolation discontinuities.
- [ ] Actual GPU geometry/material/framebuffer validation for renderer changes.
- [ ] Audio-enabled ordinary gameplay smoke; no simulation or quality shortcuts.
- [ ] Representative action-heavy gameplay plus every registered multiplayer map
      measured or explicitly recorded as an unresolved acceptance case.
- [ ] Campaign performance checked on playable supported paths; campaign startup
      blockers stay visible rather than being passed by a component fixture.

## Final performance and delivery gates

- [x] Prepare the new cohort's ordinary CPU/retained-GPU unsampled comparison:
      three interleaved pairs each1p/4p,210 frames with30 warmup, normal source
      time accumulation, internal4K/720p presentation, actual per-frame worker
      and GPU work, exact cache import and interference accounting.87 offline
      controls pass, including valid target misses and late failure removing
      qualified comparisons. These are synthetic controller controls; build,
      rendered parity, actual launch/timing and target acceptance remain open.
      [Prepared method review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-gpu-fps-preparation/ROOT_PREPARED_REVIEW.json).
- [ ] Repeated unsampled 4K one-player runs meet 120 FPS / 8.333 ms frame budget.
- [ ] Repeated unsampled 4K four-player runs meet 60 FPS / 16.667 ms frame budget.
- [ ] Reports include mean, median, p95, p99, maximum, deadline misses and longest
      miss streak, plus warmup/sample counts and actual scene dimensions.
- [ ] Verify stable pacing and playable behavior through the delivered runtime;
      distinguish render throughput from physical-display presentation timing.
- [ ] Qualify the runtime/resources with source and binary hashes before changing
      launcher selection; retain the previous qualified runtime for recovery.
- [ ] Verify both normal launcher selections and audio-enabled gameplay after
      delivery; retain any material platform/scene limitations explicitly.
- [ ] Update the goal/checklist to complete only after the final gates pass.

## Work log

- 2026-10-22: Goal created. Read-only MCC audit and prior selected-runtime profile
  are available; no performance implementation from this goal has been admitted.
  Current working-tree changes and source-layout drift must be preserved and
  authenticated before evaluating a new optimization.
- 2026-10-22: Implemented the first vector-read component and registered its
  regression. Three focused Release CTests and four ASan/UBSan CTests pass;
  complete-runtime acceptance remains separate. Added ordinary-runtime `--render-size` and
  benchmark dimension rejection, with three passing clamping/internal-size
  contract tests. Neither these fixture timings nor the vector microbenchmark
  establishes playable FPS. No launcher selection has changed.
- 2026-10-22: Full native build is active through the owned session recorded
  above, using official `tools/run_tests.py --build-worker` inside
  `tools/run_test.py`. An initial missing Rust snapshot input was corrected and
  recorded in `refresh-rust.json`; no production repair was required. The
  session removes its source/build when closed or cancelled. Its source/build
  must remain available while native validation and measurement need them;
  this active session is not a retained delivery artifact.
- 2026-10-22: Added a finite ordinary-runtime cohort for same-executable vector
  reference/optimized comparisons, full-resolution fixed-tick RGB comparisons,
  repeated unsampled deadline checks and separate CPU profiles. Five capture
  admission regressions pass, including early validation errors outside the
  retained log tail. These synthetic runner tests do not qualify gameplay,
  image parity or FPS. Full-resolution captures use isolated X11 only for
  quality evidence; performance runs keep ordinary visible presentation.
- 2026-10-22: An intentional build job-limit adjustment exposed an expired idle
  deadline in the first ad hoc coordinator. Closing it removed its partial
  source/build, losing that compilation work; the cleanup is verified. A
  second frozen source build now uses the existing repository command session,
  whose idle timer runs only while awaiting input, and six compiler jobs.
  The first attempt is interrupted evidence, not a production build failure
  or runtime acceptance.
- 2026-10-22: A turn interruption retired the second interactive owner before
  build completion. Its PID/tool handle and scratch are confirmed absent;
  its stale running report is not live-build evidence. The third owner uses
  the same repository session detached from a chat PTY, with a bounded command
  pipe and twenty-minute idle expiry. Build-gated profiles, full4K fixed-tick
  comparisons and alternating ordinary-runtime benchmarks are queued. No
  interrupted attempt has produced playable-runtime acceptance.
- 2026-10-22: The third snapshot's linked `data` directory was rejected by the
  authored release archive's ownership check. Copied its four definitions
  (15,418 bytes) into that same owned snapshot, authenticated their hashes and
  resumed the existing incremental build. Original data and completed objects
  were preserved. The prior failure is retained separately; measurements were
  skipped by their build-admission guard and requeued behind the resumed build.
  [Input repair receipt](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/refresh-authored-data.json).
- 2026-10-22: The same third snapshot completed the full native runtime build
  and seven focused CTests. The scoped production/helper/test files still
  match their frozen hashes. One/two/four-player CPU profiles started on this
  admitted binary; full internal4K captures and unsampled A/B pairs remain
  pending. Build admission alone does not establish performance or delivery.
- 2026-10-22: Current one/two/four-player CPU profiles completed. Internal4K
  unsampled pairs show inconclusive one-player behavior and two four-player
  improvements of7.15%/9.69% against the diagnostic scalar control. Three
  dimension-changing runs were rejected. Host-load variation and measured
  reference-wrapper overhead prevent a stable pre-change FPS claim; all
  admitted samples miss the120/60 FPS deadlines. The original capture attempts
  correctly failed for missing system validation; a retry selects the existing
  repository layer and preserves those failures. Full4K neutral/fire one-player
  and four-player images match exactly with zero reported validation errors.
  No launcher delivery has occurred.
- 2026-10-22: Integrated the measured motion-sensor component in one engine
  file. Its persistent differential compares the actual implementation with
  the frozen pre-change host rasterizer; the independent original-xemu pulse
  and contacts captures separately match at zero channel error. The first
  speed-gate harness attempt sampled transparent corner pixels and failed for
  its checksum, so that rejected attempt is retained. After selecting a real
  nonzero reference pixel before timing, the unchanged version fails the
  intended performance gate and the candidate passes. The native incremental
  build and ten CTests pass, with one unavailable separate input fixture
  explicitly skipped. A two-executable 4K image/FPS cohort is active; no
  whole-frame speedup or final target is inferred from the component result.

- 2026-10-22: The motion-sensor cohort completed all four exact 4K image pairs
  and twelve longer FPS runs. Images and Vulkan validation pass; contention
  rose from about 8 to 34 host load, preventing a stable whole-frame gain claim.
  Every sampled frame misses its target. The subsequent 339-case sanitizer
  check passes. Both completed owners are removed. A private three-unit LTO
  experiment reuses the active build with separate output and explicit
  normal-object/archive preservation checks; no LTO gain is established yet.

All CMake validation uses `tools/run_tests.py`. Other probes use
`tools/run_test.py` and `HALO2_TEST_SCRATCH`; small reports remain outside
scratch. See [test storage](TEST_STORAGE.md). Existing assets, frozen evidence,
installed runtimes and unrelated active work are preservation inputs.

- 2026-10-23: The dedicated offload track implements an enabled immutable
  vertex-upload prefix worker and exact ordinary GPU skinning behind its
  qualification switch. Current source/native/components and all12 normal
  first-person CPU/GPU actual4K pairs pass(17,822,520 complete GPU records,
  exact fullframes/sourceRNGtraces,zeroVulkanmessages). The first GPU version
  regressed fixed-work CPU due to added planning costs; its intended growth
  regression is repaired and passes Release/sanitizer gates. Current runtime
  fallback controls pass separately. Three clean interleaved actual4K pairs
  measure median CPU→GPU FPS of71.109→65.319(1p) and19.287→17.828(4p), regressions
  of8.14% and7.56%. GPU skinning stays opt-in while its added CPU cost is profiled.
  No120/60FPS, campaign-full-frame, default-GPU or launcher gate is closed.

- 2026-10-23: User prioritized performance measurement immediately and the
  owned full12 quality run was canceled; two completed1080p pairs do not
  qualify the full set. Twelve ordinary FPS cases (three interleaved CPU/GPU
  pairs each1p/4p) complete on the physical monitor, with internal3840x2160
  rendering and actual1920x1080 presentation. Median1p CPU/GPU: 65.552/60.834FPS (-7.20%). Median4p: 18.846/17.187FPS (-8.81%).
  Unrelated repository tests overlap EVERY case; this is a requested
  current-load snapshot, not clean optimization acceptance. All sampled frames
  execute the real presentation worker; GPU-on performs weighted deformation
  with no verification readback or CPU fallback, CPU controls dispatch none.
  All raw/caches/pinned-reference storage is removed with its benchmark owner;
  compact JSON/TSVs remain underphysical-c50-immediate-performance. Targets
  remain unmet, GPU stays opt-in, launcher/default/full-quality gates stayopen.

- 2026-10-23: Direct MCC architectural follow-up now traces its named render
  worker queue into per-worker native contexts and command-list jobs rather
  than relying on imports alone. The evidence pack separates compiled paths
  from live activation, and maps useful transfers to current port gaps.
  [Architecture evidence and acceptance plan](MCC_CLASSIC_ARCHITECTURE_BORROW_AUDIT_20261023.md).
  Native completed-owner execution, resident ordinary-model data and immutable
  packing work are implementation priorities; existing caches/resident BSP
  resources are already present. No percentage gain follows from static code.
- 2026-10-23: Referenced-row summary trial rebuilt and passed its GPU component.
  Three complete1p GPU-on old/new pairs:61.646→62.175FPS (+0.857%);4p/full-cohort
  input closure failed when the parent test runtime disappeared. Partial results
  are preserved without target/default acceptance. Parent and benchmark scratch
  are verified removed; any further executable trial needs a new frozen owner.
  [Trial receipts](../build/test-results/presentation-offload-20261022/row-cache-physical-trial/).

- 2026-10-23: Implemented retained coherent skin-input storage, an exact48-byte
  CPU record shadow, palette updates and descriptor reuse qualified by an
  explicit destination-allocation generation. All existing validation remains
  per call; zero generation conservatively refreshes descriptors. Release and
  ASan/UBSan bounds cohorts pass934,515 complete vertices across57 real GPU jobs,
  151 warm traffic checks,212 full FP/errno checks, refusal/growth/empty/stale
  controls and zero Vulkan errors, in addition to the existing15,606 vertices
  and37,440 row cases. The old streaming policy passes correctness and fails
  the new traffic gate; disabling dispatch fails actual output bytes. Leak
  checking was explicitly disabled for the Vulkan bounds run; its separate
  unresolved leak gate remains open. All completed component scratch is removed.
  [Component receipts](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/component-attempt2/execution.json).
  Fresh owned baseline/candidate rebuilds and ordinary-runtime measurements
  remain underway; no full-frame, FPS, default-GPU, latency or launcher gate is
  accepted by these component results. The goal remains active.

- 2026-10-23: The fresh retained-input six-file candidate builds12 targets and
  passes11 focused checks. Eighteen ordinary4K-internal/720p-presented cases
  complete: median CPU / streaming GPU / retained GPU FPS is64.092 /58.840 /
  56.796 (1p),18.248 /17.354 /16.555 (4p). All18 are contended by unrelated
  repository tests, so no clean regression or stable gain is accepted. Actual
  record packing/uploads fall91.85%/100%; this does not meet the120/60FPS
  targets. Raw FPS owners/reference copies are removed. A test-only extent
  harness refresh passes13 contract tests while110 production artifacts,
  native/profile binaries and97 shaders remain identical. The full12 current
  world-body frame/source-trace cohort is running at4K internal/720p presented.
  [Retained-input implementation, measurements and remaining boundaries](MCC_GPU_SKIN_RESIDENT_INPUT_20261023.md).

- 2026-10-23: The guarded recovery runtime's four-pair rendered cohort passed
  before its owned parent later terminated with SIGTERM143. The termination
  cause remains unresolved. Source/build/cache and known processes are absent;
  zero runtime bytes remain. Existing source hashes and small green reports
  identify a historical cohort, with no executable available for another run.
  The next private flags-owner prototype uses separately pinned source inputs.
  [Terminal facts and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/recovery-runtime/unexpected-terminal.json).
