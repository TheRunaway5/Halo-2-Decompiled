# Modern execution of TU1.5 owners

2026-10-23. The objective is original gameplay on modern native hardware, with
the user's separate allowance for near-identical GPU lighting/shadow visuals.
The generated C++ is already compiled native code, but retains original-machine
register/flag handling, checked source-address accesses, dispatch and callbacks.

## Measured priority

The private17F8A0 forwarding experiment is closed without canonical adoption.
It preserves64 paired cases (including42 checked-access faults) and reduces the
selected synthetic mechanism by about118ns/call, a9.54% median paired reduction.
It bypasses four service compositions, whose arbitrary capture-copy side effects
remain unqualified: the explicit counterexample observes7 versus3 copies.
No ordinary FPS gain follows from the54 inclusive return-site samples. The
source stack reset is736bytes plus406bytes of ABI assignment and FP/cursor
reset, identically included in both roles. All three scratch owners, workers and
binaries are removed.1E5470 is unexamined/deferred, not resolved.
[Full proof, cost limits and stop decision](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-crt-handoff-20261024/STOPPED.json).

The2026-10-24 full-context four-player profile retains all5,250 raw CPU samples
and3,892 complete contexts. Independent reconstruction matches every stack,
category and deepest-owner partition. Memory/access helpers account for1,527
samples (29.09%). The largest deepest generated owner is `2E6290`, with285
samples (5.43% of the total), of which215 fall in memory-policy helpers.
This differs from an inclusive outer tree: each sample enters the deepest
owner partition once. The existing three native prefixes remain present;
new work must target a material corridor beyond them.

The private checked-write destination-reuse experiment is closed without
promotion. A bank-local generation can be restored through a second shared
backing during callbacks; the actual counterexample redirects the literal
destination but leaves the first candidate copying to stale storage. A private
revision outside the bank representation repairs that case and passes113
paired functional cases, including joined-thread topology changes and revision
exhaustion. Its48 matched cost rows remain mixed; indexed writes with three
synthetic observers measure28.34→27.15ns for4 bytes and28.84→28.96ns for16 bytes.
That does not justify another ordinary memory-owner change. Both canonical and
frozen memory implementations are unchanged, and all six trial owners are gone.
[Preserved counterexample, cost and stop decision](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-exact-write-20261024/TERMINAL.json).

The same profile independently exposes272 samples at the renderer material-map
iteration return (5.18% overall). Source/disassembly association identifies the
global direction-remap scan; missing line-debug data makes this an explicit
inference. The new immutable ordered pass list preserves each live root check
and original remap/update order, with the original map scan as a control. Its
21 three-path component cases, three focused renderer tests and all four actual
1p/4p neutral/fire internal4K output pairs pass.952 submitted frames retain
weighted GPU skinning and the worker. All12 subsequent ordinary timing cases
complete2160 measured frames. Observed renderer global-phase medians fall
2.339→0.358ms for1p and7.208→1.667ms for4p; foreign build activity overlaps every
case and source ticks/work differ. These are diagnostic host timers, not GPU
timestamps or isolated frame gains. All2160 frames miss their120/60FPS deadline;
the broader measured translated gameplay/physics work remains the priority.
[Actual renderer parity](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/renderer-direction-scan-20261024/parity-preparation/results/report.json).
[Independent original timing reconstruction](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/renderer-direction-scan-20261024/timing-review/REVIEW.json).
The finite source/build/runtime owner and copied references are now removed;
the retained reports and source pins describe that completed generation.

The next bounded classifier candidate2E6627–2E6748 was also evaluated without
implementing another tiny kernel. Seventeen exact emitted sites establish a
conservative58-sample inclusion bound:48 checked-store samples,4 already in the
native prefix,2 initial reads and4 body/delivery samples. The other227 owner
samples are not resolved by that bound. Every-store mutation and continuation
serial increments remain required even for source stack writes; the current
ordinary entry state, banks and scratch/policy ownership are not captured.
This corridor is rejected as insufficient justified reduction, with no native
component, activation or timing claim.
[Exact source, mapped sites and missing authority](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/current-source-owner-attribution-20261024/REJECTED_FIRST_CLASSIFIER_CORRIDOR.json).

The runtime completed240 frames, with210 measured GPU frames and actual worker
execution on every submitted frame. The original suite remains RED from its
inherited final-summary helper expecting210 submitted frames. Independent
offline review confirms the predeclared240-frame workload and preserved raw
inputs; it does not promote the suite or establish FPS, latency or gameplay
acceptance. Internal4K consists of four1920×1080 views; displayed output was
1280×720. [Raw reconstruction](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/independent-full-context-review-20261024/independent-raw-report.json),
[workload and original-stack review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-full-context-profile-preparation/results/ROOT_RETAINED_REVIEW.json).

The earlier unchanged-baseline [four-route profiles](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-runtime-baseline-20261023/diagnostics-routes/TERMINAL_SUMMARY.json)
contain10,221 main-thread samples across840 measured frames at3840x2160
internal/1280x720 presented. All frames execute the presentation worker. All420
GPU-route frames execute weighted skinning (11,397,960 vertices total), with
no CPU fallback or verification readback. Each four-player internal viewport
is1920x1080; the CPU still prepares four cameras.

| Earlier four-route sampled CPU work | 1p CPU | 1p GPU | 4p CPU | 4p GPU |
| --- | ---: | ---: | ---: | ---: |
| Source-runtime physics `required_entry`, inclusive |49.37%|48.81%|45.42%|45.76%|
| Physics live-support owner, inclusive |22.17%|22.77%|20.48%|19.80%|
| Memory/access/instruction category |26.30%|26.86%|27.19%|26.63%|
| Plane-query2E79A0, inclusive |7.18%|8.11%|7.24%|7.66%|
| Ordinary skin planning, inclusive |12.84%|10.32%|9.86%|10.91%|
| GPU arithmetic admission, inclusive |0%|0.41%|0%|1.15%|
| GPU prepare, inclusive |0%|0.08%|0%|0.224%|

Rows overlap and must not be added. The memory category also includes literal
instruction/access helpers; it is not a pure memory-bandwidth measurement.
The4p GPU exclusive `skin_model` and `finish_skin` leaves total7.69%; retained
caller contexts need source attribution before deciding which consumers can
move.2E6290 remains a bounded7.38% owner inside the7.66% query tree, while the
broader physics tree requires an inner-owner audit. Another small GPU-prepare
optimization is lower priority than those source and CPU-skin paths.

The subsequent [retained-consumer audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/retained-skin-consumers-20261023/FINDINGS.md)
maps all113/326 recognized context samples in1p/4p GPU, with explicit inlining
and caller-tail limits. Dynamic objects account30/114 samples and scenery27/50;
both pose all six LODs. Dynamic definitions are eqip/garb/bloc/proj and are
separate from player bipeds. The ordinary base producer has mutually branched
CPU/deferred providers, so the audit does not establish duplicate deformation
of an admitted GPU character. Vehicle-associated frame-pose reuse already
exists; several direct-main consumers remain unresolved.

A bounded next presentation trial splits the pure vertex skin kernel within
one already-allocated LOD. Integer-only eligibility checks must leave uncertain
or faulting inputs on the exact serial route before any mutation. A worker may
process a disjoint suffix while the main thread processes the prefix, then
join and merge actual SSE sticky status before consumption. This preserves
per-LOD callbacks/allocation/publication order. Thread controls, errno, aliases,
failure prefixes, output and actual queue/join cost still require proof; no
canonical parallel skinning or gain is claimed from this design.

All four runs overlap external build/test activity (28/12/85/2 unique observed
processes). Sampling and cadence feedback also add overhead. This cohort
selects the next owner; it does not establish route FPS, a gain or a regression.
All raw diagnostic owners and observed children are removed. Complete reports,
compressed logs and hashes remain; the unchanged baseline and fresh read-only
pipeline cache remain in their finite owner for the next declared comparison.

The earlier cohort below remains historical evidence with its own source and
binary identities:

[Four fresh ordinary profiles](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/diagnostics/execution.json)
complete at3840x2160 internal/1280x720 presented, CPU/retained-GPU routes with
one/four players. They contain10,775 bounded main-thread CPU samples across840
sampled frames. Actual worker execution is present throughout; all420 GPU-route
frames perform weighted deformation without verification readback or fallback.

| CPU owner or cost | 1p CPU | 1p GPU | 4p CPU | 4p GPU |
| --- | ---: | ---: | ---: | ---: |
| `run_actual`, inclusive |59.70%|56.96%|48.80%|50.85%|
| `SourceBspEffectLights::draw`, inclusive |25.35%|23.46%|20.31%|15.35%|
| Generated shadow frame, inclusive |24.08%|21.05%|17.81%|13.72%|
| Nested plane-query entry2E79A0, inclusive |13.31%|9.74%|10.44%|7.44%|
| Memory/validation/lookup/three observer leaf subset |34.98%|30.68%|29.06%|28.25%|
| `skin_model` + `finish_skin` distinct leaves |8.37%|8.69%|8.11%|7.70%|

Inclusive rows overlap. The memory subset counts distinct leaf samples and is
inside those owners; do not add it to them. These are CPU attribution, not
predicted speedups or percentages of GPU time. Three cases overlap external
build processes (0/66/41/610 unique observed PIDs respectively), so elapsed
CPU/GPU-route differences establish no optimization gain or regression.

The raw native-world/storage services record exactly zero activity in all840
sampled frames. Their caching is not a priority for this neutral workload.
Common main-view collection remains inside the light-named source owner even
when no raw light receiver is emitted. Ordinary resident BSP rendering is
outside those census counters and already uses the GPU.

## Execution changes, in priority order

1. **Replace measured hot source routines with semantic native owners.** Start
   with the generated44E80/2E79A0/2E6290 plane/visibility corridor. Use native
   loops over typed, bounded data instead of repeating emulated scalar accesses
   and register bookkeeping. Preserve ordered outputs, source publications,
   external calls and ABI/FP state at observable boundaries. Keep the generated
   route as a same-binary comparison/fallback. This is a bounded first owner,
   not a promise to remove the full49–60% execution tree.
2. **Qualify memory spans and policy lifetimes at native boundaries.** Stable
   resource identity, topology/commit/access policy and complete extents can
   permit direct host reads within admitted kernels. Invalidate on release,
   restore, rebinding or policy changes. Retain source fault order, resource
   guards, write observers and mutation serial behavior. A whole-tree eager
   validation is not automatically equivalent to faults on visited branches.
   Shared-lock/observer cost may be amortized only where those exact effects
   remain proved; disabling checks or dropping serial increments is not a fix.
3. **Use modern parallel arithmetic on independent presentation inputs.** SIMD
   batches must retain each item's original arithmetic order and rounding;
   preserve SSE/x87/NaN/signed-zero behavior and avoid unintended contraction.
   Runtime CPU dispatch can add AVX2 or another qualified backend without
   changing the baseline. Workers/GPU jobs consume immutable completed poses,
   visibility queries or draw packets with explicit lifetimes and ordered merge.
   Tick/RNG/physics/object publication remains on its authoritative owner.
4. **Tune code layout and specialization after the semantic boundary is clear.**
   PGO, inlining and measured block specialization can reduce dispatch and
   instruction-cache cost. Measure them on identical work and preserve precise
   floating-point compilation. They complement the owner changes; a compiler
   flag alone does not remove per-access policies or observer callbacks.

Existing optimizations must not be counted again: `SourceRuntimeMemory::locate`
already has a source-page index, small single-region transfers already avoid
staging, `read_vector` already bulk-loads admitted16-byte operands, and write
policy dispatch already caches callback arrays by policy epoch. Each write
still takes a shared registry lock and invokes guards/observers, including
independent atomic mutation serials.

The shared lock also keeps the cached borrowed callback objects alive until
dispatch completes. Dropping it after an epoch lookup changes policy retirement
and concurrent replacement semantics. The existing native-publication hold is
read-only and rejects source writes; it does not admit a hot write lease.
Neither a raw callback cache nor skipped mutation increments is qualified.

A private checked scalar-return API saves about0.8ns per hot32-bit read in
bounded component measurements. Other widths are mixed; the3-byte fallback
regresses. Its strict integrated query stops at row18 on pre-restoration host
x87 error-pointer metadata, with matching ordered reads/ABI/banks/stores and
public FP/errno. The cause is unassigned and the trial stays private. This
component ratio establishes no frame gain.
[Terminal scalar evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-scalar-read-bridge/FINAL_HANDOFF.json).

The subsequent [host-FP consumer audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/host-fenv-fidelity-audit-20261023/AUDIT.md)
rehashes the full16,259-file successor closure. It finds no trusted gameplay or
serialized-state consumer of internal host x87 FIP/FDP/opcode addresses. Guest
logical x87 values/depth/TOP/status, controls, MXCSR and errno remain fidelity
state; existing opaque caller-environment restoration remains exact. Arbitrary
public memory/policy/observer callbacks can still inspect the host environment.
A future qualification must enforce a trusted ordinary-owner boundary and
literal fallback for those public extensions before separating internal host
metadata diagnostics from gameplay acceptance. No existing assertion or
historical RED is changed by this audit.

Source: [memory owner](../src/runtime/source_runtime_memory.cpp),
[write policies](../src/runtime/source_runtime_write_protection.inc),
[world mutation observer](../src/physics/physics_live_world_tu15.cpp),
[object continuation](../src/gameplay/multiplayer/multiplayer_object_runtime.cpp).

## GPU visibility/light/shadow trial

The ordinary raw vertex/light projection and shading stages already execute on
the GPU. The new target is earlier candidate generation, culling and draw
preparation. An early static-BSP query batch can potentially overlap CPU
preparation within the same frame. The final transformed source planes appear
immediately before the current query call, so dispatching only there provides
little overlap. Early results need exact input/resource provenance checks;
dynamic/lazy-node queries retain the CPU route until admitted.

The current source consumer needs ordered direct keys and contained endpoint
pairs on the CPU before it publishes masks/callbacks. An indirect draw alone
cannot supply that contract. Compare generated TU1.5, the existing native
query0 mathematical control and the GPU batch on identical tree/plane jobs,
including submit/readback/wait and any CPU work retained for ownership. The
native query0 owner is2DD930→2DC220; it is not an already proved replacement
for the hot TU1.5 whole-query ABI.

[Detailed GPU boundary audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/GPU_LIGHT_SHADOW_PREPARATION_AUDIT.md).
Near-plane visual differences are trial observations under the user's steering;
simulation/event differences or an additional frame of latency fail the gate.

The user's latest priority is to replace hot translated TU1.5 routines with
native kernels while keeping gameplay identical. The first implementation
replaces the27-instruction SIMD plane-classification prefix at2E6810–2E686F
inside the measured2E6290 corridor. Three qualified read-only spans replace
nine checked vector reads; the original branch at2E6872 and remaining routine
continue unchanged. Unqualified memory, active traces and reference controls
retain the literal prefix. NaN-producing outputs and unmasked live SSE
exceptions also retain the literal path. A strict synthetic test exposed
different NaN payloads between the fixed-order native operations and compiled
literal intrinsics. The exact original27-instruction oracle agrees with the
native value (`FFC54321` versus literal `7FC12345` in XMM1 lane3), but this
performance change preserves existing literal behavior for those inputs.
The kernel tests output bits with integer SSE, restores speculative MXCSR
flags and returns before publishing state when declining a NaN result.
This is a bounded native kernel, not a replacement
of the complete visibility routine. The Release component and standalone
ASan/UBSan checks pass4,757 cases, including4,226 real admissions and513 numeric
refusals; leak checking is enabled. Full1,398-query/five-fault diagnostics also
preserve complete native/literal state, ordered writes, original memory outputs
and instruction paths. The native run admits all415 hot query cases, with5,122
kernel executions across observer/no-observer pairs. Original-oracle acceptance
remains open:583 cases disagree on emulated x87 C1, and two fault cases disagree
on SF or ESP. Both native and reference have the same differences. A genuine
original2E6612 FSTP operand stores identical float bits (`423DF484`) in Unicorn
and physical x87, but physical status is`0220` versus emulated`0000`, matching
the C++ result. This establishes a status-fidelity limit for that operand,
not a proved legacy bug or full physical-Xbox qualification. C1/sticky bits
are therefore unqualified emulator fields; complete native/literal source
status remains strict, and an explicit emulator-C1 gate is retained.
The final canonical harness passes all1,398 normal queries and five natural
fault prefixes in both modes with zero qualified original normal ABI
differences; explicit C1/fault authority probes remain red. Three interleaved
10,000-call instrumented public-query pairs produce CPU ratios of
0.941x/1.028x/1.014x (reference/native), establishing no consistent benefit.
An isolated prefix comparison then runs three alternating956,928-call pairs
with equal exact State/FP reset, full128-context comparisons before/after and
2,870,784 real admissions. Native thread CPU is58.8–64.5ns per call versus
134.6–146.9ns for the frozen literal prefix; the median ratio is0.439.
This shows the bounded arithmetic/read seam is cheaper in this diagnostic,
while the larger query's measured improvement remains inconclusive. All101
observed repository build processes and erratic elapsed ratios remain in the
[component report](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-pure-timing.json);
no ordinary FPS acceptance follows. Native admission is now restricted to the
qualified Linux x86 backend; other platforms keep the literal route.
The rendered pilot times out inside renderer initialization before its first
frame. Two bounded debugger snapshots identify NVIDIA GPU/SPIR-V compilation;
the deeper stack and linked return-PC disassembly resolve the exact call as
`vkCreateGraphicsPipelines` for a forward material in `Renderer::make_pipeline`.
These runs use fresh isolated caches and Vulkan validation. This is a cold
startup issue, not measured steady-state FPS or evidence about prefix cost.
The admitted binary and controller remain unchanged throughout the probes.
[Pipeline evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-runtime/gdb-native-pilot/pipeline-return-pc.json).
The subsequent passthrough diagnostic finishes normally in214.18 seconds:
577 graphics-pipeline calls complete, consuming152.326 seconds in total,
with a10.331-second maximum. Thus compilation progresses through many variants.
It submits210 actual4K-internal/720p-presented frames, with real worker activity
on every frame and all170,280 measured prefix attempts taking the reference
route. These instrumented, contended timings do not qualify an FPS gain.
Both cache creations receive zero initial bytes despite the supplied30.65MB
seed. Natural teardown exports9,681,474 bytes under a different compatibility
UUID (suffix`7ac472064ba6b947`, versus the old seed's`6f9c57eeaf41849b`).
The seed directory/copy/environment match; the supplied bank's key is stale.
The raw owner is removed before a stable snapshot of that new bank can be
acquired, so its bytes are not retained or claimed as an available warm input.
The next comparison therefore requires a captured current-key bank. A separate
owned acquisition now completes210 frames and retains9,681,386 bytes under the
actual key, mode0400, SHA`290d62602dcb1cb3b33c92a21fa8d16ef2e981e24402a120f075706d18e033f4`.
Its exact32-byte header, filename and stable source identity are authenticated
before raw cleanup. This proves an exported input exists; actual import and
paired ordinary FPS remain separate gates.
[Current-key receipt](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/vulkan-cache-seed.json).
The subsequent12-case ordinary cohort completes three interleaved native/global-
reference pairs each for1p/4p. Every case proves actual import of those9,681,386
bytes,210 submitted4K-internal/720p frames, real worker execution and the expected
native/reference routing with zero GPU-skin input traffic. Contended median FPS
is35.970/49.522(reference/native,1p) and14.764/13.706(4p). Wide pair variation and
overlapping repository work prevent causal gain acceptance; every case misses
its target. The separate pilot is excluded from these cohort statistics.
All case/cache scratch was removed. The parent retention snapshot recorded
4,304,618,273 logical bytes of required frozen builds, reference runtimes,
original inputs and cache seeds. That owner subsequently ended after a
malformed FIFO request; its supervisor removed those temporary inputs before
the rendered controller started. The snapshot is historical. Completed reports
and canonical changes survive; a fresh owned build and cache acquisition are
required for further runtime checks. No launcher promotion follows.
[FPS evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/ordinary-fps-current-cache/report.json)
and [retention audit](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/active-retention.json).
[Owner closure and unexecuted capture](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-pixel-launch-failure.json).
[Completed initialization diagnostic](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-perf/vulkan-init-diagnostic/execution.json).
Rendered comparisons and FPS measurements remain diagnostic while those
authority gates are open. No FPS gain is accepted yet. The goal remains active.
See [final query results and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-query-final/summary-and-cleanup.json).

The full original cohort also identifies two sibling read-only prefixes as the
next incremental CPU targets:2E66B2–2E6711 (2,547 entries) and2E6A50–2E6AAF
(2,612 entries). Together they execute46,431 checked vector reads in this
corpus, compared with23,049 for the first prefix. These are exact original
instruction-entry counts, not sampled percentages or predicted FPS gains.
Their store tails remain separate because they publish three and two ordered
source stores respectively. A larger subsequent kernel can retain qualified
spans across the no-store2E6810–2E688C classifier loop, provided it preserves
the reject2E68B5, partial-mask2E68B9 and complete-mask2E688E exits, re-entry
loads, register/flag state and source fault boundaries.
The existing semantic query0 helper matches list outputs but lacks the whole
TU1.5 register/FP/partial-store contract, so it cannot simply replace2E79A0.

An isolated three-trait prototype passes24 ASan/UBSan controls with leak
checking and4,763 strict cases per prefix. It shares the exact SSE graph while
varying plane-base register, mask destination, stack operand and final PC;
the third variant snapshots EDX before reusing it for the result mask.
The siblings'128-context datasets are explicitly synthetic relocations, so
original sibling acceptance remains false. After the initial full replay hit
its120-second limit, four bounded original-only shards complete all1,019
selected queries. Every completed query matches the original full-query
GP/XMM/flags/FP, banks and instruction/access/store hashes before its contexts
are admitted. The merged captures contain2,523/2,431 unique actual sibling
contexts, spanning2,547/2,612 entries and both branch outcomes. Private native
qualification passes all24 ASan/UBSan controls with leak checking and
83,798/80,762 strict sibling cases, including83,261/80,225 actual admissions.
The comparisons preserve full GP/XMM/flags, source/caller FP, errno, cursor
and refusal/fault behavior. All five shard/merge scratch
owners are removed;23,469,097 bytes of required reference inputs remain in the
active parent owner. Canonical three-site integration is staged: the shared
arithmetic body is unchanged from the qualified prototype, all81 original
instruction bytes/PCs are authenticated, and stripping only native guards and
resume labels leaves the generated literal remainder byte-identical.
A global`HALO2_REFERENCE_SHADOW_NATIVE_PREFIXES=1` switch disables all three
prefixes; the individual switches remain independent. GCC16.2.1 and
Clang23.1.3 each pass all32 ASan/UBSan controls with leak checking, including
169,323 full-matrix cases and167,712 actual admissions per compiler.
All inputs remain unchanged; their10.23MB/7.77MB executables and scratch are
removed. Independent source review finds no blocking issue.
The official11-file incremental rebuild now passes all32 focused CTests and
links native`75fccef3…` plus its profiler. All97 shaders and the renderer remain
unchanged; the first-kernel native/profiler/shader reference is preserved.
The dedicated full1,403-query check passes native/global-reference modes:
all1,398 normal cases and five natural fault prefixes preserve complete
native/literal registers, source/caller FP, ordered stores, callbacks, bytes
and fault/cursor effects. Exact entry totals are5,094/5,122/5,224, with15,440
actual native admissions spanning all1,019 hot cases. Qualified original normal
ABI differences remain zero. The existing583 emulator C1/sticky disagreements
and two original fault-state differences remain explicit; their expected-red
authority controls still fail. Ordinary rendered parity/FPS gates remain open.
[Prototype evidence](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-sibling-prototype.json)
and [completed original capture](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/plane-query-sibling-shards/execution.json).
[Real-context native qualification](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-sibling-original.json).
[Canonical Clang qualification](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site-isolated-clang.json).
[Canonical GCC qualification](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site-isolated-gcc.json).
[Integrated build admission](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/build-admitted.json).
[Integrated full-query comparison](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-three-site/full-query/worker.json).

Subsequent independent review found an admission gap for an otherwise valid
public cursor contract: its uint32 position can occupy shared source backing.
Literal PC publication then changes a later operand, whereas the native prefix
publishes only its final PC. A concrete isolated probe reproduces unequal XMM
bits despite equal final banks/cursor; reference and capability-refusal controls
match. The ordinary live cursor is separate from source storage, so its frozen
cohort remains a disjoint-cursor comparison. A pre-arithmetic overlap refusal
and all-nine-operand persistent regressions pass30 focused prefix controls
under each of GCC/Clang ASan/UBSan with leak checking (6,924 matrix cases and
5,286 admissions per compiler). The official two-file successor builds all
five targets and passes32 focused checks, including both whole-query controls;
native`9e633e5f…` retains identical generated source and97 shaders. Its ordinary
rendered comparison remains pending. The guard compares unsigned host
address differences and covers partial four-byte overlap without forming an
overflowing range end. No original instruction or arithmetic graph changes.
[Direct reproduction and controls](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/native-prefix-alias-probe/report.json).

The149-entry generator reconstruction does not reproduce the historical whole
canonical file with either the old or current generator. Their native-injection
deltas agree apart from inherited selected/raw operand spelling. This is
separate from original-query validation; historical whole-file regeneration
remains unresolved, and the canonical source was not replaced by that replay.

The latest ordinary CPU/GPU timing cohort completes all twelve elapsed-cadence
cases, with2,520 submitted and2,160 measured frames. Recorded build contention
prevents gain qualification. Observed median CPU/GPU FPS is35.32/33.68 for1p and
11.61/8.96 for4p; every measured frame misses its target deadline. All measured
GPU frames perform weighted deformation. The next source-only diagnostic
generation counts remaining CPU skinning jobs by role/view/LOD while preserving
all original LOD work; its32 focused CTests pass. The complete four-role
distribution now shows a maximum12,960 vertices per job; identified dynamic
and scenery jobs reach only2,130 and1,686. Earlier worker benefits at16,384 and
65,536 vertices do not establish a production threshold for this workload.
Safe batching and a measured benefit remain separate requirements.
[Timing review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ROOT_FPS_V6_TERMINAL_REVIEW.json).

The next native owner is the complete441-byte CB520 caller. Hardware execution
of its original TU1.5 bytes now passes eleven synthetic caller shapes,1,095
instructions and42 real callee CALL/RET pairs. This provides independent normal
exit/callee ABI authority, including precision status that the older emulator
misses. It does not qualify genuine ordinary seats, full callee bodies or
intermediate hardware C1. A distinct C++ component now passes with unmodified
native semantics, complete hardware boundary expectations and strict software
per-PC/fault/order comparisons:11 normal cases,657 checked faults,42 callee
exceptions and11 fallbacks. Its private full-environment scope restores x87 C1
that the existing library-based scope retains after a precision-store fault;
the old failure remains unchanged. Actual compiled normal/cold restore order
and full32-byte public restoration pass independent review. Genuine ordinary
contexts, trusted activation, public scope adoption and gains remain open.
[Hardware review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/ia32-oracle-proposal/caller-prepared/ROOT_HARDWARE_REFERENCE_REVIEW.json).

2026-10-24: The private ordinary capture patch now preserves actual LiveState,
partial MMX state, guarded instruction delivery and all five genuine callees.
Its exact inverse reproduces the frozen source. Six original/disarmed/armed
component comparisons pass normal, checked-fault, alias, callee-throw and
trace-active controls, with eight-record capacity and a failing no-capture
negative control. The test uses synthetic callees and a private FP wrapper;
the ordinary runtime's FP scope is unchanged. Genuine gameplay capture and
same-native rendered inertness are the next requirements. No native activation
or performance gain follows from these hook tests.
[Capture component and cleanup](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/ordinary-capture-inertness-v2/attempt-1/LIFECYCLE.json).

The splitter-compatible successor builds and passes32 focused checks. All four
ordinary capture-off/on neutral/fire1p/4p pairs then pass complete internal4K
RGB/depth/stencil and bounded summary/work equality across952 submissions.
Actual counters cover39,600 genuine calls, with no throwing/nested/foreign
entries. Every call must invoke CD4A0; total callee entries equal caller
entries, proving no later callsite or CB689 distance phase executes in these
workloads. Native work should therefore start in the earlier distance/range
path, preserving its four float32 spills and checked reloads. The first-eight
records per workload supply state observations, not replayable source banks,
complete input coverage or a full ordered gameplay trace. All capture raw
owners and processes are removed; no FPS gain or native activation follows.
[Actual ordinary capture review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-c951c-capture-preparation/results/ROOT_TERMINAL_REVIEW.json).

The hot first-distance successor now captures the eleven existing read values,
four actual float32 stores and immediate raw result State without adding source
accesses. Its build passes32 focused checks and rebuilds all33 affected objects.
All four same-native capture-off/on neutral/fire1p/4p pairs pass full internal4K
RGB, meaningful depth/stencil, source-summary and non-timing counter equality
across952 worker submissions. The32 retained first-eight packets contain480
operand events and32 phase snapshots. All eight original files pass the strict
reader and lossless-retention checks; completed capture owners are removed.
These observations still do not provide full banks, ordered observer/callee
replay or complete domain coverage.
[Hot-distance capture review](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/ordinary-internal-rgb-successor-20261023/ordinary-cb520-hot-distance-preparation/results/ROOT_TERMINAL_REVIEW.json).

A private CB5A2–CB5BA scalar kernel keeps all eight guarded PCs and five ordered
loads while removing the software-stack representation and checked-read bridges
only for admitted ordinary disjoint storage. It preserves extended-z times
spilled-z, arithmetic order, inactive lane history, count/TOP and status. Its V2
fix limits errno preservation to admission so a cursor that aliases errno keeps
the original PC writes. Eight genuine cases and19 fallback/fault controls pass,
with reference and actual V1 errno failure controls. The component explicitly
starts with an empty physical host stack; ordinary activation remains open.
Actual emitted code retains the guards/loads and uses at most two physical x87
temporaries. Three matched100ms-or-longer thread-CPU pairs give median literal/
native101.213/61.440ns per call including all admission/reset work. All six rows
execute1,761,136 calls and pass actual stored-output and public-restoration checks.
Host contention is recorded. This39.30% block-cost reduction saves only39.77ns
per call, about0.00525ms per source tick at the observed four-player call count;
the latter is a scale estimate, not a game/FPS measurement. The kernel stays
private and work proceeds to a materially larger measured CPU owner.
[Component and original failures](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/private-first-distance8-prototype/attempt-2/execution.json).
[Complete-admission timing](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/private-seat-cb520-20261023/private-first-distance8-prototype/cost-attempt-3/execution.json).
