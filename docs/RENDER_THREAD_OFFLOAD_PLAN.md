# Render-thread offload plan (handover, 2026-11-04)

Goal context: `docs/NATIVE_4K60_SPLITSCREEN_GOAL.md` (4p split-screen, 3840x2160
internal, 60 fps, frames byte-identical to original TU1.5). This file is the
working plan for the next attack so any engineer (Claude session or Codex) can
continue it. Keep it current: update the "Status" line of each step as you go.

## Where things stand (measured 2026-11-03/04, Coagulation 4p/4K, quiet host)

- Main thread before the render thread: ~22 ms (`host_pre_render_ms`).
  Tick ~4.5 ms, presented-pose relocation (B9460, other session's design)
  ~2.7 ms, physics callbacks ~1.2 ms, state publication ~1 ms, per-view
  translated phases (camera/lighting/cull/model collection) ~13 ms across 4
  views, per-view native posing ~3.3 ms (now on the overlap thread, see
  "Posing lane commit B" in the goal doc), HUD ~1 ms.
- Render thread (`Renderer::draw`, runs on the persistent overlap thread while
  the main thread ticks): ~6.9 ms by host stamps; sync-mode sampler profile
  (`HALO2_REFERENCE_SYNC_RENDER=1`, `prof-sync`, symbols
  `build-perf-4k60/prof5/halo2_native`) attributes ~5.3 ms of CPU:
  - `ParticleRenderer::Impl::prepare` 1.1 ms, of which
    `ParticleRenderer::Impl::material()` 0.85 ms: it calls
    `validate_particle_material(*source)` on every lookup before the cache
    hit (`src/rendering/effects/particle_renderer.cpp` ~264-275). Plus
    `std::vector<ParticleDraw>` copies 0.3 ms.
  - `Renderer::Impl::draw` self 1.3 ms (inlined geometry plan/bind loops,
    renderer.cpp 4650-5275 and 6143-7304).
  - `memcpy` 0.6 ms (FrameVertexUploadPlan::copy_to into the mapped vertex
    buffer; FrameModelSkinPlan::append).
  - `FrameModelSkinPlan::add_bank/append` 0.4 ms (GPU-skin planner scanning
    CPU vertex banks), `native_forward_pipeline` 0.2 ms,
    `SourceBspResidentPlan::merge_into` 0.17 ms, `ResidentDrawStorage::upload`
    0.1 ms, `ModelSkinGpu::prepare` 0.08 ms.
- Per frame the main thread assembles ~35k `Vertex` records per view (152
  bytes each, compiled check; `include/halo2/cache.hpp:117-150`), ~138.7k
  planned vertices = ~21 MB at 4p. The 21 MB copy into the mapped vertex
  buffer runs on the presentation-copy worker (off the render thread's
  critical path; `upload_ms` 0.008). Views 1 and 2 submit byte-identical
  35,526-vertex banks every frame. 44k vertices/frame are GPU-skinned but their
  local records are re-uploaded (6.7 MB) and re-validated twice per frame.
- Six-lens map of the renderer (plan, bind, commands, producers, resident
  precedents, volumes) with a completeness critique: workflow journal
  `~/.claude/projects/-home-eric-Developer-Halo-2-Decompiled/15c22a15-37e6-43b3-8979-151569122d27/subagents/workflows/wf_bf1dd76d-93c/journal.jsonl`
  (type=="result" rows). Headline: render-thread time is CPU bookkeeping, not
  vertex bytes: per-vertex draw-key run walks (renderer.cpp ~4992/5009),
  skin-plan scans, per-object 3712 B uniform aliases and per-view material
  rebuilds in bind, per-frame descriptor pools, and O(105,601 materials)
  scans in the global phase (animated textures ~5816-5825). Material table
  is 105,601 x 3712 B x 5 banks (~2 GB host-visible). GPU-resident precedents exist: the BSP resident plan
  (`SourceBspResidentPlan`, immutable bank uploaded once, per-view part
  bindings) and the GPU skin path (`model_skin_submission.hpp`,
  `frame_model_skin_plan.hpp`, bit-exact admission predicate
  `model_skin_gpu_admission.hpp` with CPU fallback).

## Attack order (render thread first, as requested by the user)

1. **Particle material lookup (0.85 ms, no GPU change).** Status: implemented
   (cache-first lookup, `HALO2_VERIFY_PARTICLE_MATERIALS=1` re-validates hits);
   DONE 2026-11-04: parity vs `build-perf-4k60/halo2_native_idx` (same
   tree) byte-identical with the verifier on; `renderer_ms` 6.10/6.57/6.63 ->
   5.23/5.27/5.30 ms in three interleaved sync-render pairs (-1.2 ms on the
   render thread). The per-view `std::vector<ParticleDraw>` copy (0.3 ms,
   renderer.cpp ~5424-5444) remains.
   `particle_renderer.cpp` `Impl::material`: look the pointer up first and
   return the cached `MaterialGpu`; run `validate_particle_material` only on a
   miss, or on every call under `HALO2_VERIFY_PARTICLE_MATERIALS=1` (keep the
   identity-changed throw there). Materials are immutable (`shared_ptr<const
   ParticleMaterial>`), so bytes cannot change. Also avoid the per-view
   `std::vector<ParticleDraw>` copy (0.3 ms) if the prepare path can take the
   spans directly. Validate: parity (three default cases) + frames with
   particles (4:150:10:110 fires weapons).
1b. **Animated-texture frame scan over all 105,601 materials x 4 stages per
   frame (global phase, renderer.cpp ~5818).** Status: implemented 2026-11-04
   (dirty list filled where `wanted_frames` changes in
   `update_material_constants`; `HALO2_REFERENCE_ANIMATED_FRAME_SCAN=1`
   restores the full scan). DONE 2026-11-04: parity vs
   `build-perf-4k60/halo2_native_pm` (same tree) byte-identical;
   `renderer_ms` 4.93/4.92/5.03 -> 4.72/4.57/4.76 ms in three interleaved
   sync-render pairs (-0.3 ms). Render thread is now ~4.7 ms (was ~6.5 at the
   start of the day).
2. **Geometry plan/bind self time (1.3 ms) and memcpy (0.6 ms).** Status: being
   mapped by the mapping workflow (journal
   `~/.claude/projects/-home-eric-Developer-Halo-2-Decompiled/15c22a15-37e6-43b3-8979-151569122d27/subagents/workflows/wf_bf1dd76d-93c/journal.jsonl`;
   if the session died, re-derive by reading renderer.cpp 4650-5275 and
   6143-7304 with the phase stamps). Expected design: GPU-resident per-model
   banks (immutable `RenderModel` vertex banks uploaded once, keyed like the
   skin admission cache / BSP resident plan) drawn by (model, LOD, section
   or part) ranges with per-draw tables (palette, part->material, colors,
   lighting, stipple), so the per-frame vertex stream shrinks to truly dynamic
   geometry (FP, decals, UI). Parity rule: a resident draw must produce the
   same pixels as the CPU-assembled draw; skinning stays bit-exact through the
   existing admission predicate (CPU fallback keeps the old path per bank);
   material selection moves to tables filled by the same CPU selection code.
   Land it per producer (vehicles first: biggest, PRT-free LOD path already
   has ordinals), each behind a switch with a reference env to restore the
   CPU path, parity gate per step.
3. **GPU-skin planner scan (0.4 ms).** Status: after step 2 (resident banks
   remove the per-frame scan of CPU banks).
4. **Vehicle CPU skinning through the existing GPU path (0.7 ms main thread).**
   Status: not started; measure first (small-object deferral was slower).
5. **Effect-light receiver selection (1.4 ms main thread).** Status: later;
   order-sensitive draw lists.

## Design review outcome (2026-11-04) — read this before continuing

Three designs (render-thread hygiene; GPU-resident skinned banks with
skin-output regions; full table-driven resident banks) were scored by a parity
skeptic and a performance realist (journal
`~/.claude/projects/-home-eric-Developer-Halo-2-Decompiled/15c22a15-37e6-43b3-8979-151569122d27/subagents/workflows/wf_2f868fa6-166/journal.jsonl`;
the synthesis agent failed, this section is the synthesis).

**Decisive finding (verified in overlap-mode runs, `index-B-*`):** under the
shipping overlap mode `Renderer::draw` runs beside only the next frame's tick
and finishes first: `renderer_ms` ~7.2 ms (slower than sync mode because of
contention) vs `host_input_ms` (tick) ~10-12 ms. The frame is prep
(`host_pre_render_ms` ~22 ms) + tick + ~1 ms = ~33 ms. **Render-thread
savings do not shorten the frame until the tick drops below the draw.** The
particle and animated-frame fixes above are real (-1.5 ms render thread) but
are headroom, not frame time. The 60 fps gap (~16 ms) is on the main thread.

**Consequence: use the render thread / GPU headroom instead of shrinking it.**
Move main-thread posing work onto the GPU skin path even though that adds
planner cost on the (hidden) render thread, as long as the draw stays under
the tick. The posing lane is the critical-path target: lane 3.32 ms/frame
(vehicles 2.24, bodies 1.08) vs main_objects 0.93 ms, so ~2.4 ms of lane wait
is on the critical path today.

Revised order (each behind a reference switch, parity gate per step):

1. **Counters first (~1 day):** lane wait, per-producer GPU-admission refusals,
   stipple != 16 counts, memcmp/dedup bytes, alias counts (Design 1 M0 list).
   Vehicles and third-person palettes have never been run through the
   admission predicate at scale (today only FP and view-3 bodies admit).
2. **Dynamic objects through the existing GPU skin path:** the opt-in
   `HALO2_GPU_OBJECT_SKINS=1` (source_dynamic_object_scene.cpp:147-151) was
   rejected when the render thread was synchronous (+1.9 ms render, -0.54 ms
   objects). Re-measure it in overlap mode: if the draw stays under the tick,
   the -0.54 ms main-thread gain is real. Zero code to test. RESULT
   2026-11-04: byte-identical (three cases, `HALO2_VERIFY_GPU_SKINNING=1`),
   `renderer_ms` +0.8 ms (hidden), but no main-thread gain (`objects_ms`
   3.37 vs 3.36-3.56; `host_pre_render_ms` +0.3..0.8): dynamic objects run in
   `main_objects`, which is already hidden under the longer posing lane. Keep
   default off; revisit after the lane shrinks below main_objects.
2b. **Split the lane: vehicles and corpses+bodies as two concurrent jobs**
   (vehicles 2.24 ms vs bodies 1.08 ms, run back to back today). DONE
   2026-11-04: `native_overlap_run2` (second persistent overlap thread) runs
   vehicles on thread 0 and corpses+bodies on thread 1 while the main thread
   runs the scene owners; the two lazily filled model maps
   (`source_secondary_models`, `source_world_hierarchies`) are locked
   (`source_world_lazy_mutex`); pStencil mode and
   `HALO2_REFERENCE_SINGLE_POSING_LANE=1` keep the single lane. Parity
   byte-identical vs `build-perf-4k60/halo2_native_af` (pre-split) on 4:90,
   4:150, 2:150, 1:150; lists oracle identical vs the single lane. Wall:
   `host_pre_render_ms` medians 20.98/20.08/20.20 -> 20.19/19.18/19.37
   (-0.8 ms), interval -0.6..-1.0 ms. The posing window now equals the
   vehicle job (2.49 ms/frame; bodies 1.47 and scene owners 1.05 are hidden),
   so vehicles are the next target (CPU skinning via `vehicle_frame_poses`
   ~1 ms, section select, shader selection, PRT banks, lighting).
3. **Vehicles and third-person bodies palette-only through FrameModelSkinPlan
   (~4-5 days, ~1.5-2 ms wall if admission holds):** Status 2026-11-04:
   live vehicle draws implemented (both the PRT and the plain-LOD branch of
   the `live_vehicle_base` path in `pose_vehicles`: `update_deferred` on
   `vehicle_frame_poses`, a `ModelSkinGeometry` with source ordinals and
   bindings per draw, `ModelObjectDraw::deferred_skins` backed by
   `vehicle_skin_submissions[i]`; gate `vehicle_gpu_skins` mirrors the unit
   gate; `HALO2_REFERENCE_CPU_VEHICLE_SKINS=1` restores the CPU skin);
   pre-change binary `build-perf-4k60/halo2_native_split`.
   First measurement (live route only): byte-identical with
   `HALO2_VERIFY_GPU_SKINNING=1`, but slower (vehicle job 2.49 -> 2.72 ms):
   the deferred pose still copied the whole LOD mesh, and the profile showed
   vehicle "skinning" is mostly that copy. Fix 1: admitted deferred and
   factory poses now borrow the immutable mesh
   (`model_skin_world_pose_borrowed`, `ModelWorldPoseCache::borrowed_`;
   `HALO2_REFERENCE_COPY_DEFERRED_POSES=1` copies): job 2.41 ms, still no wall
   win, because most vehicle skinning (~1.1 ms/frame) is on the NATIVE route
   (vehicles without a live source draw: `choose_lod`/`pose_world_lod`/
   `select`, no visibility test), which always CPU-skinned. Fix 2 (in
   validation): the native route defers too (PRT: `pose_world_lod_view` +
   `update_factory` + `view_skin`, as dynamic objects; plain LODs:
   `update_deferred` + the shared `vehicle_skin` helper). Pre-change binary
   for fix 2: `build-perf-4k60/halo2_native_borrow`. DONE 2026-11-04:
   byte-identical vs `halo2_native_split` on 4:90, 4:150, 2:150, 1:150 with
   `HALO2_VERIFY_GPU_SKINNING=1`, and `HALO2_REFERENCE_CPU_VEHICLE_SKINS=1`
   identical too. Vehicle job 2.49 -> 1.43 ms/frame, posing window 2.5 ->
   1.65 ms; `host_pre_render_ms` 18.51/18.73/18.86 -> 17.96/17.89/18.02,
   interval 28.3-28.8 -> 27.3-27.8 ms; GPU-skinned vertices 43,932 -> 69,405
   per frame; render thread +0.3-0.6 ms, still hidden behind the ~9 ms tick.
   Bodies are next: they now share the critical path (view 0 bodies 0.66 vs
   vehicles 0.52 ms). Known waste there: most third-person bodies are
   CPU-skinned twice (full skin into `body_frame_poses`, then a selected skin
   through `unit_frame_poses` in `selected_world_vertices`), corpses skin
   unconditionally even when a retained source submission is used, and
   `append_selected` copies (~0.5 ms). per owner, CPU fallback
   when refused; vehicle live route multiplayer_runtime.cpp vehicle block
   (`vehicle_frame_poses` already cross-view cached), `pose_bodies`, corpses'
   unconditional skin. Keep the skin compute's output in the frame bank so
   Draw first/count, alias_owner, W-depth/hologram/pStencil consumers and
   `HALO2_VERIFY_GPU_SKINNING` readback stay valid.
4. **First-person copy/select elimination (~0.4-0.5 ms main, 1-2 days):** the
   FP route copies the posed bank into the skin geometry and again into
   `foregrounds[i]`.
5. Only if step 3 pushes the draw past the tick: the posed-scratch registry and
   producer-supplied draw runs (Design 3 M1 / Design 1 M1) to pull the render
   thread back under it. Do not schedule bind/commands caching or cross-view
   bank identity for frame time.

Parity traps the skeptic found (apply to any residency work): removing records
from `view.vertices`/`foregrounds` breaks the active-material root scan that
drives material animation (multiplayer_runtime.cpp ~8832-8851 ->
`animate_bank`), so animated arms/gun materials would freeze; the CPU skin
fallback reads destination records, so a design that leaves gaps there skins
garbage on refusal; `HALO2_GROUP_INPUT_DIAGNOSTICS`/`HALO2_VIEW_DIAGNOSTICS`
disable GPU skinning (`can_skin_models`), so they cannot observe a GPU path.
Even the full hybrid is ~3.5 ms of the ~16 ms gap: the per-view phase split
(goal doc) and the presented-pose relocation (~2.7 ms, other session's design)
remain the larger levers.

## Frame map 2026-11-07 and next target (main thread)

Fresh overlap-mode profile (`prof-1107`, symbols `build-perf-4k60/prof7/halo2_native`,
4p Coagulation, quiet host): interval 28.3 ms = `host_pre_render_ms` 18.5 +
tick (`host_input_ms`) 9.2 + ~0.5; render thread 5.2 ms (hidden). Top
main-thread blocks: `MultiplayerMatch::step` 18.5%, presented-pose scope
constructor 16% (~4.5 ms; `set_object_position` -> `run_actual(0xB95D0)`
for ~70 objects per frame) plus 1.1% teardown, effect lights draw ~13% across
its callbacks, `MultiplayerMatch::state` 4.5%, HUD animate 3.2%.

Leaf attribution (excluding the translated routine bodies themselves): the
translated code's load/store plumbing is ~35% of the tick, ~46% of the
presented-pose scope and ~37% of `state()`: `SourceRuntimeMemory::write`,
`dispatch_write`, `source_runtime_validate_write`, three write observers
(the scope's undo journal re-reads every stored span; `MainReceiptAuthority`;
the host physical-memory observer), `small_lookup`/`locate` for sub-page
reads, `memcpy`, plus per-call dispatch glue (cached services lambda,
`PhysicsInstructionRemapTu15`, `State::required`). Speeding that path is
parity-neutral by construction (same bytes, same callbacks in the same
order) and helps the tick and the scope at once. A mapping/design workflow
with a semantics skeptic is running (journal
`~/.claude/projects/-home-eric-Developer-Halo-2-Decompiled/15c22a15-37e6-43b3-8979-151569122d27/subagents/workflows/wf_51c39d3e-777/journal.jsonl`).

## Memory-path and dispatch-glue results (2026-11-07)

Design workflow `wf_51c39d3e-777` (four mappers, two designs, a semantics
skeptic and a performance realist). Outcomes:

- **Lean store path (inlined admission, generation-gated second lookup):
  reverted.** Byte-identical, but no measurable gain: the production-shape
  microbenchmark is 13.15 ns vs 13.27 ns per store, and frame pairs were
  within noise. The store path is cheap per store; its profile share is
  volume.
- **Landed (byte-identical on 4:90, 4:150, 2:150, 1:150 against
  `build-perf-4k60/halo2_native_1107`, and with all reference switches on):**
  a last-hit hint in `PhysicsInstructionRemapTu15::find`
  (`HALO2_REFERENCE_PHANTOM_REMAP=1`), a compile-time sorted live entry index
  replacing the 743-row linear scan (`HALO2_REFERENCE_LIVE_ENTRY_SCAN=1`), one
  cached `PhysicsPlatformLockServices` instead of three std::function builds
  per cross-unit call (`HALO2_REFERENCE_LOCK_SERVICES=1`), and the presented
  scope's journal entry list kept across frames
  (`HALO2_REFERENCE_PRESENTED_STORE_INDEX=1`). About -0.2..-0.25 ms
  `host_pre_render_ms` and -0.1 ms tick in the two undisturbed pairs.
- **Not landed, needs an owner exception to "every callback sees the same
  stores":** skipping the receipt observer while it is unarmed (it is never
  armed at 2-4 players), declaring the stack range inert for the scope's
  journal, and a physical-guard window. Worth ~0.3 ms together.
- Pre-existing failures seen while validating (not caused by these changes):
  `source_physical_write_guard` counts one mutex lock (the dispatch-owner
  claim on the first store; same result with the original sources), and
  `physics_vehicle_runtime_tests.cpp` no longer compiles against
  `native_vehicle_rate_math_tu15.hpp` (`RateDelta` vs `Rate`).

**Command walk accessor locks.** The native 40E80 command walk called
`PhysicsLiveWorldTu15::memory()` 3-4 times per part (each a call-gate mutex
lock/unlock pair); `part_services` now takes the bank reference once
(`HALO2_REFERENCE_WORLD_MEMORY_CALLS=1` restores per-use calls). Journaled
`write_source_call_input` stores are unchanged. Byte-identical; gain below
this host's noise (~0.1 ms).

**Instruction census tool.** `HALO2_INSTRUCTION_CENSUS=<file>` with
`HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1` writes every executed TU1.5
instruction address with its count (main thread) at exit
(src/physics/physics_live_world_tu15.cpp, observation callback). One 4p run
executed ~202M translated instructions (~1.1M per frame). Top routines by
dynamic count: 145560 8.6% (69 instructions), 144CF0 4.6% (78), BF0C0 2.4%,
14CDB0 1.9% (39), 14CEA0 1.7%, 136EA0 1.6%, 2847D0 1.6%, 14D0E0 1.5%,
1E5C80 1.5% (35), 2835B0 1.4%. Static trees: BF0C0 109 routines/13.4k
instructions, of which 78 routines/5.4k addresses execute.

**Native leaf kernels (landed 2026-11-07).** `145560` (compose two
real_matrix4x3) and `144CF0` (real_orientation to matrix) run as native
kernels in the literal live-support unit
(src/physics/runtime/native_tu15_matrix_leaves.hpp). They perform the
original reads and stores in the same order, addresses and widths through the
same store helper, the same stack pushes/pops and final registers/flags, and
publish instruction addresses to the cursor before each memory access. They
are admitted only when per-instruction delivery is a literal cursor store with
no active trace, and the image is TU1.5. They are installed by the build-time
specializer (tools/specialize_native_operands.py `LEAVES`, bound to the SHA of
each exact canonical body; the checked-in generated sources are unchanged),
as an admission prefix inside the original body.
`HALO2_REFERENCE_MATRIX_LEAVES=1` runs the original bodies;
`HALO2_VERIFY_MATRIX_LEAVES=1` computes each call in a pure overlay, re-enters
the routine with the native route bypassed and requires identical stored
bytes, registers and flags (270 frames at 4p, no mismatch). Frames
byte-identical vs `build-perf-4k60/halo2_native_walk` and with the reference
switch. Gain ~0.1-0.2 ms: the original stores (which the kernels must keep)
dominate these leaves. Adding a leaf: write `kernel_<addr>` against the
`Io`/`Machine` interface, add its canonical body SHA to `LEAVES`.

**The real lever is native kernels.** The translated code pays an
instruction-observation callback per x86 instruction, software register and
flag modelling, and an emulated store (full dispatch) per push. Inside the
presented-pose scope, B9460 splits into BF050/BF0C0 (attachments and node
matrices, 41%), 1CD620 (Havok rigid-body update, 30%) and BA630 (cluster
membership, 17%). BF0C0 (398 translated lines, ~20 callees) and BFB90 also
run in the tick (10% and 6% of it), so a native BF0C0 is worth roughly
1.5-2 ms per frame. Next: scope and build it with a store-sequence oracle,
as the native 40E80 command kernel was.

## Rules, tools, validation (same as the goal doc)

- Frames must be byte-identical: `python3 tools/run_test.py -- python3
  tools/perf_4k60/parity.py REPORT.json REF CAND [cases]` (role env with
  `VAR=VALUE@runtime`; `--keep-lines REGEX` retains and compares log lines).
  Compare only binaries built from the same tree state; keep a copy of the
  pre-change binary (`build-perf-4k60/halo2_native_preB` = before the posing
  lane, `halo2_native_B2` = lane before the section index, current =
  lane + index).
- Measure wall time (`host_pre_render_ms`, `renderer_ms`, `pack_ms`,
  `geometry_plan_ms`, `geometry_bind_ms` in `profile_run.sh` timing.txt), not
  only main-thread CPU (`H2_CPUMON=1`): off-thread work drops CPU without
  changing wall time. Render-thread profiles need
  `HALO2_REFERENCE_SYNC_RENDER=1` (the sampler is main-thread only); keep the
  profiled binary as `build-perf-4k60/<dir>/halo2_native` for symbols.
- Switches added recently: `HALO2_REFERENCE_SERIAL_POSING`,
  `HALO2_VERIFY_POSING_LANE`, `HALO2_VERIFY_SECTION_INDEX`
  (`tools/perf_4k60/README.md`).
- Tests: `native_parallel_tests`, `model_section_index_tests`,
  `multiplayer_engines_world_tu15_tests MAP tests/fixtures/multiplayer_engines_original_tu15.json OUT.json`
  (MAP = `<selected cache>/halo2/h2_maps_win64_dx11/coagulation.map` from
  `gameplay-runtime-selection.json`).
- Scratch under /tmp is wiped on reboot; keep reports you need in-tree or
  under `build-perf-4k60/`.
