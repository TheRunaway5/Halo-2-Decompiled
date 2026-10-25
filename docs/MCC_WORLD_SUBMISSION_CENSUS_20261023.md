# World submission census

2026-10-23. Diagnostic measurement within the active presentation/performance
goal. TU1.5 remains the gameplay and visual authority. This change adds
observations; it does not change rendering resources, shaders, source selection,
callback order or GPU-skin default admission.

The user subsequently authorized GPU lighting/shadow trials with near-identical
visuals. That widens the visual acceptance of a future presentation algorithm;
it does not change this census or permit simulation/event differences.

## Question

World rasterization and authored vertex/fragment processing already run on the
GPU. The previous main-thread samples identified source effect-light/shadow
preparation and remaining CPU skin work, but did not establish how often the
native raw-world submission services run. Their resource construction must not
be assumed to explain the observed frame time.

All current ordinary renderer constructors leave `triangle_vertices=false`.
Triangle-metadata allocation belongs to the alternate backend. Ordinary raw
shadow submission additionally requires the explicit reference option; running
source shadow selection alone does not activate that service.

## Measurements

`RendererFrameTiming`, the ordinary renderer TSV and the fixed CPU profiler now
report:

- Native prepare attempts, completed/nonempty prepares, prepared draws by
  title/shadow-generation/shadow-application/light role, and successfully encoded
  native draw commands. Empty resets count as attempts and completions.
- `SourceWorldDrawStorage` upload attempts, completions and descriptor draw
  counts for raw shadows/effect lights. Object and packed-shadow storage use
  different services and are excluded from these counters.
- Host elapsed prepare/upload milliseconds, including failed attempts. Service
  constructors are excluded. These times overlap existing renderer phases;
  they are not GPU timestamps or additional phases to sum.

`profile_multiplayer.py` retains these fields with explicit null/missing columns
for older binaries. It also groups CPU skin samples by the outermost matched
helper and full caller return PCs/names. It counts a sample once, separates
observed `finish_skin`/`model_skin_vertices` kernel contexts from other skin/pose
preparation, and retains executable-relative caller addresses for source lookup.
The interrupted helper's variable PC does not fragment caller groups. Top24
groups, eight displayed callers and700-character names bound the report; full
grouping precedes truncation. Inlining/unwind limits remain explicit.

Encoded draws do not prove GPU execution or visible pixels. CPU samples do not
count invocations; a pose helper may only build palettes or validate admission.

## Reproducibility and status

The source freeze contains16070 private files. Only the four declared telemetry
files are refreshed with new mtimes;16066 other inputs remain identical.
[Freeze receipt](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/freeze-ready.json)
and [exact pins](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/census-four-file-pins.json).

The official disposable build of `halo2_native` and `profile_mp_render_cpu`
passes, including the framerate-dimension contract check. Four ordinary physical-Wayland
diagnostics pass: CPU/retained-GPU routes with one/four players,
240 submitted frames each,30-frame warmup,250Hz main-thread sampling,
3840x2160 internal and1280x720 presented, Coagulation and starting weapon0.
Actual worker/GPU counters, map/shader/runtime pins, host contention and owned
raw cleanup qualify each report. These are diagnostic runs, not FPS acceptance.

All12 native prepare/draw/record/storage counters and both corresponding host
timings are exactly zero in all840 sampled frames. This rules out caching these
services as a fix for this neutral workload. It does not count all rendering:
ordinary resident BSP GPU rendering and object/packed-shadow storage are outside
this service census. The light-named source CPU owner still performs expensive
common main-view collection before any light receiver exists.

The profiles retain10,775 bounded CPU samples,840 actual worker frames and420
weighted GPU frames, with no readback/fallback. All nested raw owners and the
diagnostic work directory are removed. External build contention occurred in
three cases; these are not FPS comparisons. [Execution receipt](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/diagnostics/execution.json).

A nonzero authored-light fixture control also passes all15 existing layouts,
illumination/blend/isolation assertions and counter/header/extent checks. Its
final renderer's25 rows contain20 completed native prepares,32 prepared/encoded
light draws,20 completed storage uploads and40 uploaded descriptor draws.
This validates positive counter wiring; cold pipeline/geometry preparation is
included where it occurs inside `prepare`, so its elapsed time is not a warm
ordinary-scene estimate. All75 PPMs (180,819,585 logical bytes), raw TSV/log/cache
and controller work are removed; no raw captures are retained.
[Positive control](../build/test-results/presentation-offload-20261022/world-submit-census-20261023/positive-control/report.json).

[Measured CPU bottleneck and modernization order](MCC_TU15_MODERN_EXECUTION_20261023.md).
No world-resource caching, FPS gain, full-frame parity, default-GPU or launcher
gate is accepted by this census.
