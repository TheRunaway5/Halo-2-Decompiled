# Performance opportunities from MCC Halo 2 Classic

Investigation date: 2026-10-22. Scope: identify performance work for this port
using the owned MCC Classic executable and measured native multiplayer costs.
This investigation changes no production code, graphics settings or launcher
selection. OG Xbox TU1.5 remains the behavior and visual reference.

## Finding

The largest measured opportunity is reducing the cost of executing the port's
translated Xbox routines. MCC supplies concrete examples of a native CPU
implementation and a retained D3D11 backend, including command lists, instanced
draws, resource-state caching and rotating buffers. Those mechanisms are useful
references, but copying their submission policy alone would address a small
part of the presently measured frame.

The installed two-player Coagulation profile has a nearly saturated main thread
(98.09% of one core), low device-wide GPU utilization (13.32% mean), a 25.30 ms
mean frame interval and a 2.62 ms renderer host interval. Renderer intervals
are host wall times, not GPU timestamps. GPU utilization includes other
applications. These observations together support a CPU preparation bottleneck
for that workload; none establishes a GPU budget at 4K or every other scene.

The official MCC console feature list includes 4K at 60/120 Hz for one/two
players and 60 FPS for four-player split screen. This supports the target's
plausibility, without establishing a matched PC comparison or its cause.
[Halo Support enhancement features](https://support.halowaypoint.com/hc/en-us/articles/360052059432-Halo-The-Master-Chief-Collection-MCC-Xbox-Series-X-S-Enhancements-Features-and-Troubleshooting).

## Evidence and version boundaries

The normal launcher was inspected and still selects:

```text
bin/engine-builds/6c6a9b900ebb46e7ad3fb087d9e70fa4/halo2_native
SHA-256 7db35ababe9ba5e2b351ae5a9ccb96fb5800f66d9e2b3bc14b1dbc37470e7c79
```

That hash was recomputed during this investigation and matches the executable
recorded by the retained
[optimized two-player profile](../build/test-results/splitscreen-optimization-20261022/optimized-profile-2p.json).
This is an existing measurement of the currently selected executable, not a new
profile collected by this investigation. It contains 930 CPU samples over 150
steady frames after 60 warmup frames at actual 1280x720 rendering/presentation.
The scenario is idle Coagulation with native HUD and muted audio. The profile
does not measure action-heavy gameplay, campaign or audio cost.

The earlier unsampled matrix for this executable reports medians of 53.75 FPS
with one player, 31.71 FPS with two and 16.15 FPS with four. These are historical
samples from today's preceding pass, at 720p with variable competing load;
they are not new measurements or 4K results. See
[the complete performance record](FRAMERATE_PERFORMANCE.md#installed-optimization-pass-2026-10-22).

Current source files were separately hashed and compared by basename with the
installed build's retained source inventory. Several files have changed since
that snapshot. Source observations below describe the inspected current tree;
the timing evidence belongs to the executable hash above. The
[budget analysis and source pins](../build/test-results/mcc-performance-audit-20261022/profile-budget-analysis.json)
record the distinction explicitly.

## What MCC actually contributes

The new [MCC binary reference audit](MCC_CLASSIC_PERFORMANCE_REFERENCE_20261022.md)
records exact preferred-image addresses, disassembly, DLL identity and SDK
references. It finds compiled code for:

* Native x64 object access, including direct field loads in a health getter.
  This illustrates an execution model without our register/flag/memory-helper
  layer at those observed instructions. It is not a matched TU1.5 function
  equivalence or a performance measurement.
* Conditional deferred-context creation, command-list completion and playback.
  Their presence does not establish live worker scheduling, receiver identity,
  or which Classic scenes exercise the path.
* Indexed instanced drawing with an instance count taken from a draw record.
  Actual batching frequency and savings require a live API capture.
* A cached shader-resource binding that bypasses a repeated D3D call when the
  pointer already matches.
* A buffer allocator with three GPU vertex/index buffers, modulo-three
  rotation, range updates and a separate immutable/default initialization path.
  This does not prove three complete frames in flight or a particular workload's
  geometry residency.

Earlier evidence already establishes H2C interpolation. Our completed-tick
presentation separation exists too, so interpolation is not a newly available
explanation for the CPU gap. See
[the earlier high-framerate investigation](MCC_HIGH_FRAMERATE_RESEARCH_20261022.md).
Neither audit admits an MCC GPU-skinning implementation or GPU visibility
algorithm from option strings or D3D11 imports.

## Quantified opportunities

The following uses the verified two-player CPU sample. The speedup estimates
are Amdahl calculations for a fixed amount of work:
`speedup = 1 / (1 - fraction * fraction_removed)`. They are ceilings and
planning scenarios, not promised gameplay FPS. GPU overhead, dependencies,
changing tick/render ratios and other bottlenecks can reduce the gain.

| Candidate | Observed CPU share | If its cost halves | If its entire cost vanished |
| --- | ---: | ---: | ---: |
| Translated execution owner, inclusive | 57.42% | 1.40x | 2.35x |
| Selected memory helpers/observers plus instruction cursor, exclusive | 36.24% | 1.22x | 1.57x |
| Source shadow command construction, inclusive | 26.77% | 1.15x | 1.37x |
| Model skinning, exclusive | 7.63% | 1.04x | 1.08x |

**Rows overlap and must not be added or multiplied.** Shadow preparation and
memory helpers occur inside translated execution. Exclusive memory/cursor
rows were formed from disjoint leaf sample counts, not summed inclusive
percentages. The raw rows and exact arithmetic are retained in the budget
analysis. The complete memory-helper/observer subtotal is 285/930 samples;
the cursor adds 52, giving 337/930. Skinning has 71/930.

For comparison, the same profile records:

| Renderer host work | Mean time | Share of mean complete frame interval |
| --- | ---: | ---: |
| Command encoding | 0.317 ms | 1.25% |
| Packing plus upload | 1.034 ms | 4.09% |
| Fence, acquire, submit and present calls combined | 0.103 ms | 0.41% |
| Complete renderer host interval | 2.619 ms | 10.35% |

The named phases are existing instrumentation boundaries and can contain
driver work. They are not independent GPU costs or guaranteed removable time.
Nevertheless, they explain why parallel command recording or a larger frame
queue alone is a lower priority than the CPU work preceding submission in
this scene. A 4K GPU capture could change that ordering for a different scene.

## Implementation order and acceptance

1. **Lower translated execution overhead in measured hot routines.** The
   current state adapter keeps source GP registers, XMM registers, x87 state
   and flags, while generated operations call checked source-address reads,
   writes and instruction hooks. Memory writes also dispatch guards and
   mutation observers. Start with the profiled shadow/model collectors and
   memory operations; generate efficient basic blocks and checked scalar or
   vector accesses where complete source semantics permit it. Avoid repeating
   the operand specialization, instruction-cursor and vehicle-skin work already
   delivered in the preceding pass. Investigate remaining checks with profiles
   of the next actual built version.

   Preserve owner lifetimes, fault ordering, cross-owner transfers, ordered
   observer/device effects, exception PCs, tracing, FP rounding and required
   service callbacks. A blanket switch that drops validation or observers
   would not establish an equivalent fast backend. Require original-versus-
   optimized registers, flags, memory and side-effect comparisons, followed by
   unsampled measurements of the ordinary runtime.

2. **Reduce repeated preparation around each local view.** The measured
   `render_model_view` caller owns 37.74% inclusive CPU samples, and source
   shadow construction owns 26.77%; these overlap. Count original entry calls
   and completed work per frame/view/owner before attempting reuse. Retain
   immutable model metadata, section indexing, inverse binds and compatible
   pose palettes. Share completed results only when their complete inputs and
   lifetime permit it. Per-camera visibility, source queue mutations, LOD,
   fade/lighting and transparent order remain independent. Calling the same
   source entry repeatedly is not by itself evidence that a call is redundant.

   First demonstrate identical ordered selected draws and complete vertex/
   material outputs for one, two and four views, including entry/exit, parent
   changes, owner reuse and different cameras. Then consider workers for
   read-only completed inputs. The live TU1.5 world and ordinary renderer have
   explicit executor/ownership rules; adding threads around existing calls is
   not a valid implementation plan.

3. **Expand the admitted raw GPU geometry path to ordinary model draws.** CPU
   `skin_model` still copies vertices, constructs inverse-bind/palette inputs
   and skins them. Existing `MenuWorldSourceStorage` retains GPU geometry and
   `MenuWorldNativeDraw` issues indexed draws, so this port already has a useful
   corridor. Extend it by real declaration/program/pass family, carrying
   original weights and indices plus current palettes. Keep CPU node banks for
   attachments, bounds and gameplay. Validate shields, PRT, first-person and
   extra passes explicitly; changing a packed declaration class alone does
   not supply the missing input contract.

   Expect a modest standalone CPU benefit in the measured scene: even perfect
   removal of the sampled skinning cost yields about 8.3% ideal speedup. GPU
   geometry can also avoid copies/uploads; those benefits need measurement
   without double counting. Require the original vertex-program/input
   comparisons and actual renderer/framebuffer acceptance before delivery.

4. **Apply MCC-style resource/state and buffer reuse where profiles justify
   it.** Audit dynamic constants, descriptor churn, geometry uploads and
   `require_lens_bitmap` (3.98% exclusive in this profile). Retain full resource
   identity, mutation and image-shape checks rather than bypassing them.
   Multiple frame-owned buffers can improve overlap only with explicit
   fence-protected resource retirement. The present very small wait and
   command-encoding intervals do not justify promising a large gain here.

The native CPU path is the largest plausible route toward MCC-like throughput.
The measured data supports pursuing it first; it does not yet support a
numerical 4K120 promise. Achieving 120 FPS requires complete frames within
8.333 ms, followed by appropriate tail/deadline and live-play acceptance.

## Current source entry points

* [Memory lookup, scalar transfers and write checks](../src/runtime/source_runtime_memory.cpp),
  `locate`, `validate`, `read`, `write`.
* [Write-policy dispatch](../src/runtime/source_runtime_write_protection.inc),
  `source_runtime_validate_write`.
* [Register/flag and instruction adapter](../src/physics/runtime/physics_contact_runtime_internal.hpp)
  and [scalar/vector memory adapter](../src/physics/runtime/physics_step_internal.hpp).
* [Per-view entry](../src/gameplay/multiplayer/multiplayer_object_runtime.cpp),
  `render_model_view`; [world executor](../src/physics/physics_live_world_tu15.cpp),
  `begin_model_view` and `WorldMutationObserver`.
* [Per-view light/model collection](../src/rendering/world/source_bsp_effect_lights.cpp),
  `SourceBspEffectLights::draw`.
* [CPU pose/skin preparation](../src/rendering/models/model_pose.cpp),
  `skin_model`; [resident raw geometry](../src/ui/menu_world_source_storage.cpp)
  and [indexed GPU draws](../src/ui/menu_world_native_draw.cpp).

## Fresh benchmark and storage receipt

Fresh ordinary-runtime benchmark attempts and their outcome are recorded in
[execution.json](../build/test-results/mcc-performance-audit-20261022/execution.json).
The first attempt exceeded a 100-second startup/run bound before any timing
rows were available. Its owned scratch directory was verified removed. A
subsequent retry allowed up to 300 seconds per case and completed three serial
ordinary-runtime samples, with 150 submitted frames and 30 warmup frames each:

| Case | Actual rendering/presentation | Mean frame interval | Mean FPS | Renderer host mean |
| --- | --- | ---: | ---: | ---: |
| 720p A | 1280x720 | 58.90 ms | 16.98 | 5.10 ms |
| Requested 4K | **2544x1002** | 104.95 ms | 9.53 | 7.99 ms |
| 720p B | 1280x720 | 81.13 ms | 12.33 | 6.30 ms |

Each case missed the 8.333 ms deadline in **120/120** sampled frames, so the
benchmark returned exit code 1 as expected. The desktop compositor clamped the
requested 3840x2160 window; this case is **not a 4K measurement**. The 720p
repeat changed substantially, with host load averages around 44–46 and
concurrent compiler work. This cohort cannot isolate a resolution cost or
establish a stable hardware throughput. In each sample the renderer host mean
was only 7.6–8.7% of the complete frame interval, consistent with substantial
work outside the renderer; these intervals do not locate that work by
themselves. The installed-runtime CPU profile above supplies the attribution.

The first completed case took 233.5 seconds including startup and teardown;
the next two took 37.0 and 42.3 seconds. Startup was excluded from the measured
frame intervals. A requested GDB attachment during startup was rejected by the
host's ptrace policy; no stack was collected and no system policy was changed.

Only bounded console/log tails and small frame-time TSVs were retained.
No source fix was attempted from these measurements. A real 4K follow-up must
pin internal scene dimensions independently of the compositor, assert actual
3840x2160 output in the report, and pair the same camera/settings with a live
MCC Classic CPU/GPU trace. Existing detailed profiler support for explicit
internal resolution is documented in
[the benchmark workflow](FRAMERATE_PERFORMANCE.md#repeatable-measurement);
its historical binary must not be substituted for the current runtime without
qualifying the source/build difference.

All probes use `tools/run_test.py`. Scratch/build removal and retained report
sizes are recorded in the final storage receipt in the same directory. The
MCC binary audit has its own storage receipt. No newly generated game binary,
object/archive, shader cache, raw sample bank or framebuffer is a deliverable
of this investigation. Existing installed runtimes and reference inputs remain
preserved.
