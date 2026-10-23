# World rendering: current CPU/GPU split

2026-10-23. Current-source audit and fresh instrumented diagnostics for the
retained-input candidate `e811b30b…`. TU1.5 remains the visual/gameplay authority.

## Already on the GPU

| Work | Current implementation |
|---|---|
| Static BSP vertex storage | Grouped/uploaded once; resident ranges drawn per view (`renderer.cpp:1188–1227`) |
| Projection and authored vertex programs | GPU world vertex shader, including selected lighting calculations (`world_vertex.glsl`) |
| Texture/lightmap sampling, combiners, alpha/depth | GPU fragment processing (`world.frag`) |
| Fog/compositing and pStencil raster passes | GPU render passes; source selection/setup still CPU owned |
| Raw world/model effect-light geometry | Raw bindings/palettes feed native GPU shader draws; no CPU vertex conversion (`effect_light_draw.hpp:21`, `menu_world_native_draw.cpp:491`) |
| Admitted ordinary-character deformation | Opt-in GPU skin service; exact component and bounded frame evidence in the retained-input report |

This is not a percentage of total frame time. Host CPU timings and device-wide
GPU utilization cannot supply that percentage; per-pass GPU timestamps remain
an open measurement gate.

## Fresh CPU evidence

[Four diagnostics](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/diagnostics/execution.json)
pass at actual3840x2160 internal /1280x720 presented, using the same native
binary and exact97-shader/three-map pins. CPU and retained-GPU routes each run
with1p/4p,240 submitted frames,30-frame warmup and250Hz main-thread CPU sampling.
The8,292 samples stay inside the declared presentation boundaries. All840
sampled frames execute the real presentation worker. All420 GPU-route sampled
frames perform weighted GPU deformation, with no verification readback or late
CPU fallback; CPU controls dispatch none. Owned sampler libraries/raw outputs
and the diagnostic workspace are removed. These are instrumented diagnostics,
not additional FPS acceptance runs.

The4p retained route uses98.48% of one main-thread core on average. Selected
inclusive CPU rows are:

| Owner | CPU samples |
|---|---:|
| Source required-entry dispatcher |46.39%|
| Source BSP effect-light draw |19.44%|
| Generated shadow-frame execution |16.94%|
| Nested plane-MOPP entry2E79A0 |9.98%|
| Nested plane traversal2E6290 |9.26%|
| CPU `skin_model` |7.51%|
| CPU `finish_skin` |3.56%|

**Inclusive rows overlap; do not add them or treat them as predicted savings.**
CPU skin rows do not attribute work to a particular body/weapon/consumer.
Device-wide GPU utilization changes substantially across these profiles and
includes other applications. It cannot establish this runtime's GPU saturation.

`SourceBspEffectLights::draw` is not a software light rasterizer. Its tree
executes original fades, source model collection, main/shadow callbacks,
secondary object admission/culling and ordered command production. Those
callbacks mutate source contexts, masks and command banks before raw GPU packets
are built. `source_run_shadow_frame_tu15` is the generated multi-entry source
executor, including nested44E80→2E79A0→2E6290, rather than only projection math.
The small pure projection helper immediately feeds source publications and
secondary admissions. Neither inclusive tree is a ready asynchronous GPU job.

## Concrete next changes

1. **Measure world/light submission before retaining more resources.** `MenuWorldNativeDraw::prepare`
   (`src/ui/menu_world_native_draw.cpp:436–489`) resets its frame, creates fresh
   uniform buffers and descriptor pools/sets, and can create/copy triangle
   metadata per draw despite already resident geometry. All ordinary renderer
   constructors currently use indexed geometry (`triangle_vertices=false`), so
   triangle-metadata allocation is not demonstrated ordinary Linux traffic.
   `SourceWorldDrawStorage::upload`
   (`src/rendering/world/source_world_draw_storage.cpp:82–135`) retains capacity
   but resets/allocates sets, maps both banks, copies complete material/vertex
   banks and copies six texture descriptor bindings plus two/three updates per
   draw. Qualified capacity/mapping/descriptor/template reuse is an MCC-style
   graphics-overhead opportunity. It requires explicit resource revisions,
   ranges and retirement/fence ownership; mutable palettes/constants still
   update. Its isolated cost and gain are unmeasured. CPU source light/shadow
   selection does not establish that these submission services run: ordinary
   raw-shadow submission requires its explicit reference option. Count actual
   calls, prepared draws and encoded draws before prioritizing this change.
2. **Remove unnecessary host skin preparation and finish deformation coverage.**
   Existing admitted bases are followed by duplicate full-model preparation.
   Remaining CPU skins include unmigrated families and real CPU consumers.
   Preserve node/attachment/IK work, exact shield selection, source lighting
   callbacks, FP/errno effects and failure ordering. Obtain caller-specific
   evidence before attributing the sampled CPU skins to removable work.
3. **Pack immutable completed packets on a worker; specialize hot source code
   natively.** Workers can own copied geometry/material templates, palettes and
   constants after source admission/range selection finishes, with deterministic
   merge order. The plane query needs an exact native CPU implementation/oracle;
   existing ray/integer query ports are different owners. Keep source writers,
   portal/collision/gameplay decisions and original callbacks serialized.

The [MCC binary audit](MCC_CLASSIC_ARCHITECTURE_BORROW_AUDIT_20261023.md)
establishes retained resources and connected worker/context/job machinery, not
live occupancy or the cause of a particular MCC frame rate. The
[retained-input change](MCC_GPU_SKIN_RESIDENT_INPUT_20261023.md) cuts actual
record traffic91.85%/100% in the ordinary1p/4p cohort, but its contended FPS
measurements establish no stable gain. These next boundaries are source-backed
opportunities, not completed changes or promised speedups.
