# Retained ordinary-character skinning inputs

2026-10-23. Implementation and evidence for the first retained-resource change
from the [MCC architecture audit](MCC_CLASSIC_ARCHITECTURE_BORROW_AUDIT_20261023.md).
OG Xbox TU1.5 remains the gameplay, ordering, arithmetic and visual authority.
This change retains the compute service's input bank. It does not yet make the
ordinary model mesh/section/pose preparation resident or eliminate its per-view
work, and it does not establish MCC's ordinary skinning placement.

## Behavior

`ModelSkinGpu::prepare` still validates the entire current palette and every
record before resource refusal or reuse. There is no cached validation result.
The GPU program and its exact descending-influence/post-placement arithmetic
are unchanged.

The service retains coherent mapped input memory and a private CPU shadow of
the complete48-byte host records. Exact record equality, counts and palette
shape allow it to keep the existing44-byte GPU records and update the palette
only. Equality never reads mapped Vulkan memory: coherent memory need not be
host-cached. Incompatible input or replacement storage repacks the records.

A descriptor hit additionally requires the same input allocation/range,
destination/range and a nonzero caller-owned destination allocation generation.
The renderer advances that generation before replacing its dynamic vertex
buffer, even if Vulkan recycles the numeric handle. Generation0 conservatively
refreshes descriptors on every successful call, preserving existing callers
without a lifetime promise. Replacement input storage invalidates descriptors
independently of handle equality.

The previous renderer fence still retires GPU access before preparation. A new
input allocation is created, bound and mapped privately before the old bank is
unmapped/retired. Destruction retains the existing device-idle boundary and
unmaps before freeing memory. Persistent mapping itself supplies no GPU-access
synchronization; that remains the caller's responsibility under
[the Vulkan mapping contract](https://docs.vulkan.org/refpages/latest/refpages/source/vkMapMemory.html).
No extra frame, submission, presentation queue or simulation tick is introduced.

Every prepare resets its dispatch count and operation counters first. Empty
work publishes no dispatch. Nonempty failure invalidates record reuse and keeps
count0; existing typed CPU fallback still joins vertex copying before restoring
exact CPU output. All existing quota, packing, create, allocation, map and hard
failure controls remain effective on warm jobs. The FP/errno environment guard
is retained on success and failure.

`HALO2_REFERENCE_STREAMED_SKIN_INPUTS=1`, selected at service construction,
retains the former vector-pack/map/copy/unmap/descriptor-update policy in the
same executable. Overall GPU skinning remains opt-in. This control isolates
input-storage policy rather than comparing different gameplay implementations.

## Observable operations

The service and ordinary/fixed-runtime tables expose actual allocation,
map/unmap calls, record bytes packed/uploaded, palette bytes uploaded,
descriptor writes and record-cache hits. Failed real calls remain counted;
synthetic fault controls are not reported as actual Vulkan calls. The CPU
shadow copy is distinct from the serialized44-byte GPU payload. Empty jobs
reset all counters. The counters do not assert that an ordinary scene achieves
a particular reuse rate or speedup.

[Six-file source pins](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/candidate-six-file-pins.json)
identify the service/header, GPU regression, renderer timing/header, renderer
integration and fixed profiler changes. Source-world, visibility, callback and
simulation owners are unchanged by this implementation.

## Component evidence

The [owned Release and bounds-sanitizer cohort](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/component-attempt2/execution.json)
passes with identical counts:

| Evidence | Result |
|---|---:|
| New actual GPU jobs |57|
| Complete destination vertex/sentinel comparisons |934,515|
| Warm traffic checks |151, zero violations|
| Complete FP environment and errno comparisons |212|
| Typed refusals / hard errors / empty resets |8 /11 /8|
| Vulkan validation errors |0|

The existing15,606 kernel/post-placement vertices and37,440 referenced-row
cases also pass. The new matrix exercises equal records at different host
addresses, in-place field changes, changing palettes/extents/counts, flattened
palette offsets, growth and failed growth recovery, two destination buffers,
changed destination generations, generation0, stale-dispatch guards and
independent rounding/DAZ/FTZ/sticky states. The stable native x87 bookkeeping
fixture is unchanged; arithmetic control/status/tag/MXCSR and every fenv byte
are compared, with the previously documented scheduling limitation retained.

The streaming control completes the same correctness cohort but has nonzero
traffic violations and fails the deterministic traffic gate. Its explicit
correctness-only run reports that the traffic gate was skipped. Disabling GPU
dispatch fails the earlier recovered-output byte gate. The first controller
attempt expected a later byte gate and rejected this valid negative result;
that controller error is retained in `component/execution.json` and corrected
in the successful retry. It was not a production output failure.

ASan/UBSan is bounds/undefined-behavior evidence with `detect_leaks=0` explicitly
set for the Vulkan run. It does not close the existing separate leak gate.
All completed component source/build owners were verified removed. Only bounded
reports, commands, source references and hashes remain.

## Runtime and acceptance

A fresh16,068-file baseline snapshot built12 targets and passed11 focused
checks. The candidate's six-file refresh also built12 targets and passed all11
checks; the other16,062 private inputs and97 shader binaries remain frozen.
The [candidate admission](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/candidate/build-admitted.json)
pins native `e811b30b391c6c5982d255d0d70bb79ade0f44c3a8f96fe66b7c64492d36845a`.
The baseline, candidate, source and required reference binaries were held under
one finite test owner through validation. Both use the same prepared MCC maps
and original reference owners. Exact scoped source and reversible original
diffs are archived in small reports before that disposable owner is retired.

Ordinary performance comparisons interleave CPU skinning, streamed GPU
inputs and retained GPU inputs in the same candidate binary, three times each
for one and four players. They request3840x2160 internal rendering and1280x720
physical presentation, fitted to either connected monitor; they do not claim
physical4K output. A prior prelaunch attempt rejected a changed monitor
configuration. Another controller attempt completed one native run but failed
a bare assertion before preserving its table/summary; the exact failed check
is unresolved and that result is excluded. The retry preserves both before
assertions and records a traceback on failure. No native implementation changed
between these attempts. Actual worker/GPU
execution, absence of readback/fallback, operation counters, dimensions, host
contention and deadline misses must be retained. An operation-count component
improvement is not an FPS result.

The [completed18-case ordinary cohort](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/performance/execution.json)
reports these median FPS values on RTX5080:

| Players | CPU skinning | Streamed GPU | Retained GPU |
|---|---:|---:|---:|
|1|64.092|58.840|56.796|
|4|18.248|17.354|16.555|

**All18 cases overlap unrelated repository tests.** The retained medians are
3.47%/4.60% below streaming and11.38%/9.28% below CPU in this current-load
snapshot. This does not establish a clean regression or a stable gain; no
ordinary performance improvement is accepted. These idle-world measurements
also do not assert identical simulation ticks across timing policies.

Across the three retained runs, record-cache hits are496/540 sampled1p frames
and540/540 sampled4p frames. Record packing/upload falls from245,773,440 to
20,025,984bytes (91.85%) for1p and from1,043,824,320 to0bytes for4p. Sampled
map/unmap calls fall from540 each per player-count cohort to0; descriptor
writes fall from1080 to88/0. Current palette bytes remain uploaded. This is
actual ordinary traffic evidence, separate from FPS acceptance.

All3,780 submitted frames execute the actual presentation worker. All2,160
sampled GPU-on frames contain real GPU skin work; timing has zero verification
readback and zero late CPU fallback, while CPU controls dispatch none. Retained
1p misses531/540 sampled120FPS deadlines;4p misses540/540 sampled60FPS
deadlines. Every nested raw owner and the FPS reference-copy workspace is
verified removed. Exact completed helper sources are archived in a small JSON
receipt and their obsolete executable scripts are retired.

The [fresh12-case quality attempt](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/quality/runtime/execution.json)
finishes **failed overall:8 qualified pairs,4 unqualified**. All neutral/fire
Spartan/Elite1p and4p pairs match complete presented PPM bytes and ordered
callbacks, with3,007,440 weighted GPU vertices fully verified,480 source frames
and5,776 ordered callbacks per role, and zero Vulkan validation messages.
Every submitted frame independently proves3840x2160 internal /1280x720
presented dimensions. GPU work counters are frame-level; these results do not
assert weighted character work in every view. The trace covers its explicit
published fields and callbacks, not all engine state or a full playthrough.

All four2p cases stop at the required actual-weighted-GPU gate. Recorded
host-triangle rays hit terrain before the other player; that is scene evidence,
not proof of which source PVS/frustum/model-selection step excluded bodies.
No missing GPU work is accepted as equivalence. The current stationary2p
fixture remains unqualified. A future genuine movement cohort can use real
`world.match.step(Controls)` preparation, with every actual command/tick/state
and RNG receipt recorded; straight movement has unproved visibility. Do not
substitute direct placement, inspection cameras, native spawn overrides or a
weakened actual-GPU gate. Changing session_random_seed alone is not an
established authored-spawn selector: the deterministic chooser151A20 uses
options+0x10/+0x14 and the player's spawn count, rather than options+0x18.

The first quality preflight rejected an omitted KScreen `enabled` property,
before launching any native case. Its corrected observation retains that
unknown value as null and records connected physical connectors, current
compositor geometry and the backend's explicit `disable unsupported` text.
Quality records display changes instead of requiring an unchanged physical
configuration: actual every-frame extents and complete bytes are independent
gates. Strict FPS observations remain unchanged in their closed evidence.
No display setting changed. All12 nested raw owners and quality work storage
are removed; no large mismatch reference was needed.

Full12 ordinary quality, full4K presented pixels, latency and launcher
acceptance remain open. Neither120FPS single-player nor60FPS four-player,
default GPU enablement, complete resident model rendering or full goal
completion is claimed here.

## Next boundaries, source audit only

The [fresh CPU/GPU world-rendering audit](MCC_WORLD_RENDERING_GPU_SPLIT_20261023.md)
records the completed four-profile diagnostics and concrete world/light
resource-retention boundaries. Actual world rasterization and shader work are
already GPU based; source preparation and repeated graphics API work remain.

Animation changes palette values and need not invalidate this record bank.
Frame-relative destinations, selected part/order and merged palette bases can
invalidate the complete record array. A resource-owned immutable binding/local
attribute bank with small per-append destination/palette jobs would survive
those changes. The current frame-selected `ModelSkinGeometry` pointer is not a
cross-frame resource lease; an implementation needs exact prepared resource,
LOD, generation and expanded-ordinal identity. Initially preserve separate
destinations for each view and the existing CPU material/metadata upload.

The ordinary runtime also admits deferred base geometry before the body loop
(`multiplayer_runtime.cpp:5066`), but still prepares full body/held models and
selected vertices later (`5862`/`5880`), before appending the accepted base
(`5894`). A second successful admission duplicates host palette/copy/admission
work. A rejected full-model admission can run `finish_skin` whose output is
then discarded. The occurrence and cost of that fallback in measured scenes
are unmeasured. This is not proof of another CPU deformation on every GPU frame.

Any removal must preserve actual WORLD/node/attachment/IK and source collector
owners, exact shield selection and its error cases, CPU-consumer fallbacks,
complete FP/errno effects and failure prefixes. Unit-effect lighting consumes
its own authored raw geometry/current palettes, not `BodyFramePose`; that does
not authorize bypassing its source callbacks. No additional production change
or FPS gain is accepted by this source audit.
