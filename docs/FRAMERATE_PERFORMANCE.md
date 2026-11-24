# Native frame-rate performance

## Zanzibar 1p, continued, 2026-11-23

After the 2026-11-22 work Zanzibar 1p still ran near 55 FPS. Its single view
draws fixed shadows, and that disabled the render overlap 2p already uses:
the whole draw ran on the main thread after the per-view collection.

Changes. All are default; the switch restores the old route.

| Change | Restore switch |
|---|---|
| A fixed-shadow frame overlaps its draw with the next tick. Inside the read-only render-memory borrow only each view's completed main camera is checked and sealed (`source_world_depth_sealed_camera_tu15`). The borrow then closes, the presented poses and main-camera observer are released (as the end of the iteration would), and the draw thread builds the depth inputs from the sealed cameras (`source_world_depth_input_sealed_main_tu15`: Cache, Mesh and view selections only) before drawing. A `RendererTargets::Use` is confined to the thread that acquired it, so the draw thread acquires its own use of the same attachments; the depth inputs and the Renderer check generation and device by value. Online play keeps its serial draw (overlap stays off there). | `HALO2_REFERENCE_SERIAL_SHADOW_DRAW` (old route, draw inside the borrow); `HALO2_REFERENCE_MAIN_THREAD_DEPTH_INPUT` (overlap, depth inputs completed on the main thread) |
| The cached decal and contrail renderers kept nothing between frames: every frame they created and freed two buffers (with `vkAllocateMemory`), a descriptor pool and all its sets, and looked up each draw's fragment kernel by a linear scan of 2256 catalogue entries. They now keep persistently mapped upload buffers and a reset-in-place descriptor pool (`src/rendering/effects/frame_upload_resources.hpp`), grown only when a frame needs more, and remember each (layer, program words) pipeline. A single view's decals/contrails are no longer copied into a joined vector. | `HALO2_REFERENCE_DECAL_FRAME_RESOURCES`, `HALO2_REFERENCE_CONTRAIL_FRAME_RESOURCES` |
| The depth input's resident atlas is compared with the renderer's upload copy once per buffer pair instead of every frame (both are immutable while the renderer and plan live). | `HALO2_REFERENCE_DEPTH_ATLAS_EVERY_FRAME` |
| `authored_pass` records the Cache-selected vertex programs of each (shader, LOD, pass, implementation, cull) once, keyed by the Cache's published metadata object (same control block); later frames compare the live program with the record. | `HALO2_REFERENCE_AUTHORED_PASS_LOOKUP` |
| Per-object material uniforms find fog passes in a slot table instead of a `std::map`. | `HALO2_REFERENCE_FOG_PASS_MAP` |
| The presented state is rewritten in place each frame (`multiplayer_presentation(shown,...)`), and the WORLD presentation is `shown` itself when there is no native render frame, instead of two fresh copies. | `HALO2_REFERENCE_FRESH_PRESENTED_STATE` (the `world_shown` alias has no switch: nothing changes `shown` after that point) |
| Scenery views append each LOD's admitted vertices straight into the view stream (`append_shader_vertices`) instead of building a filtered copy first. Zanzibar 4p about 0.4 ms/frame. | `HALO2_REFERENCE_SCENERY_VERTEX_COPY` |

**Where a frame goes now** (main-thread CPU, draw forced onto the main
thread with `HALO2_REFERENCE_SERIAL_SHADOW_DRAW=1 HALO2_REFERENCE_SYNC_RENDER=1`
so one sampler sees all of it):

| | Zanzibar 1p | Zanzibar 4p | Coagulation 1p |
|---|---|---|---|
| Total | 18.9 ms | 46.0 ms | 8.5 ms |
| Per-view collection (translated shadow/command/model routines) | 4.6 | 8.7 | 2.2 |
| Renderer CPU (`Renderer::draw`) | 3.8 | 8.4 | 1.7 |
| Simulation tick | 2.7 | 9.3 (6.6 per tick) | 1.1 |
| Depth input | 1.0 | - | 0.5 |
| Everything else in the frame loop (object vertex streams, skinning, materials, HUD, state) | 6.1 | 17.6 | 2.5 |
| Under any translated routine, including its memory-access layer | 4.0 | 11.1 | 2.2 |
| Leaf `memcpy`/`memset`/`memcmp` | 3.2 | 8.0 | 1.2 |

The translated engine and the per-frame CPU vertex streams (every view
rebuilds world-space vertices for every visible object, and the renderer
repacks them for upload) are the two largest costs in every configuration.
Neither has a small fix: the first needs native ports of the hot routines
(object update B8F40/B9090, per-view shadow and model-render collection
134C00/136230/41C10/41970/23670, relocation B95D0, weapon targeting 1ADF60),
the second retained GPU geometry with bit-exact GPU skinning.

**Parity.** One binary, every switch above on vs the default:
byte-identical 3840x2160 captures on Zanzibar and Coagulation, 1p/2p/4p at
the tick endpoint and with `--motion --render-fraction 0.5` (12 cases), and
the serial-vs-overlap shadow draw alone on the four 1p cases.

**Measured.** 4K, 600 frames, three interleaved rounds, same binary, every
switch above on ("before") vs default:

| Config | FPS | Main-thread CPU/frame | Renderer |
|---|---|---|---|
| Zanzibar 1p | 44.0 -> 64.6 (+47%) | 21.8 -> 12.0 ms | 7.0 -> 3.6 ms |
| Coagulation 1p | 98.9 -> 106.0 (+7%) | 9.3 -> 6.9 ms | 1.9 -> 1.7 ms |
| Zanzibar 2p | 42.9 -> 45.8 (+7%) | 19.6 -> 19.9 ms | 8.2 -> 5.2 ms |
| Coagulation 2p | 83.7 -> 86.1 (+3%) | 9.7 -> 9.4 ms | 2.4 -> 2.3 ms |

The "before" column is slower than the 2026-11-22 "after" numbers for the
same code paths (Zanzibar 1p 44 vs 55 FPS). Between the two sessions the
NVIDIA driver (620.70 -> 620.73) and kernel were updated and other sessions'
work was compiled in; the comparison above is within one binary and host.
After the reboot KDE's baloo file indexer spent a core indexing this repo
(13.6M files); it was excluded from indexing, which removed large
cache-eviction noise from these measurements. GPU time is not the limit
(Zanzibar 1p 3.3 ms, Coagulation 1p 2.3 ms per frame at 4K). glibc malloc
tuning (no trim, 32 MB mmap threshold) cut page faults from 69 to 2 per frame
but changed FPS by about 1%, so it was not adopted.

**Remaining, Zanzibar 1p** (main thread about 12 ms busy of a 15.5 ms frame;
wall-clock sampling with a monotonic timer):
- The main thread waits about 2.7 ms per frame for the draw thread at the end
  of the overlap: the draw thread's depth-input build plus draw (about 4.5 ms)
  is longer than the overlapped tick (about 2.9 ms). It also waits about
  0.7 ms in the posing lanes, whose vehicle/body jobs outlast the main job.
- Per-view collection 4.5 ms: shadow root frames, the 40E80 command frame,
  model draws (translated or native-bridged routines; the native-replacement
  backlog).
- Pre-render host work: presented scope 0.5 ms (B95D0 relocation of moving
  roots), scenery `shader_vertices` copies 0.3 ms, material animation and
  constants about 0.5 ms.
- The weather point/line/plate renderers still create their buffers and
  descriptor pool every frame, like decals did; this affects weather maps.

## Zanzibar and Coagulation, 2026-11-22

Question: why do Zanzibar and Coagulation run far below the other maps? The
GPU is not the limit (4K Original: Zanzibar 6.4 ms, Lockout 3.4 ms of GPU
time). Main-thread profiles against Lockout showed several costs that grow
with these maps' content:

- **Machine LOD publication** (Zanzibar's windmill, gate). Every frame, each
  machine copied its one shared skin into all six LOD banks with the material
  remap. Views read only their selected LOD.
- **Translated BCB70 per material input.** Material animation asked the
  translated object-function routine for every prop/control shader input.
- **Presented-pose relocation of resting roots.** The shown root is
  `node0_shown ∘ inverse(node0_sim) ∘ placement`. For an object at rest that
  product only adds rounding, so parked vehicles and dropped weapons, which
  have children or attachments, missed the in-place path and ran the full
  B95D0 relocation each frame. This was about 3.7 ms/frame on Zanzibar and
  2.9 ms on Coagulation, measured with `HALO2_RELOCATION_PATH_CENSUS`.
- **1p ordinary depth input.** Each frame it deep-copied the whole resident
  BSP atlas, then expanded every BSP vertex only to keep the visible parts.
- **Translated BC200 in `state()`**: object linear velocity per presented
  object.

Changes. All are native by default; the switch restores the old route.

| Change | Restore switch |
|---|---|
| `SourceObjectGeometry` records a cached publication and copies a LOD bank on its first read (`publish_pending`). | `HALO2_REFERENCE_EAGER_LOD_PUBLICATION` |
| `object_shader_input` runs the native `object_function_value` (the kernel proven at BCB70's light callers). It uses the same caller frame and value slot. | `HALO2_REFERENCE_OBJECT_SHADER_INPUT`; oracle `HALO2_VERIFY_OBJECT_SHADER_INPUT` |
| `object_linear_velocity` runs `native_tu15_object_velocities`. | `HALO2_REFERENCE_OBJECT_VELOCITY`; oracle `HALO2_VERIFY_OBJECT_VELOCITY` |
| A root whose shown node 0 is bit-identical to its simulation node 0 takes the in-place route even with children or attachments. Its root words are unchanged. | `HALO2_REFERENCE_RESTING_ROOT_RELOCATION` |
| The depth input borrows the plan's immutable atlas, and `SourceBspResidentPlan::expand_parts` expands only the selected parts. Same bytes, same order. | `HALO2_REFERENCE_COPY_DEPTH_ATLAS`, `HALO2_REFERENCE_FULL_DEPTH_EXPANSION`; oracle `HALO2_VERIFY_DEPTH_GEOMETRY_RUNS` (now checks the filtered path) |

`HALO2_RELOCATION_MOVED_DIAGNOSTICS=N` now prints moved roots from frame N.

**Parity.** One binary, every switch above on as the reference vs the
default. Byte-identical 3840x2160 captures on Zanzibar and Coagulation:
- 1p/2p/4p at the tick endpoint;
- 1p/2p/4p with `--motion --render-fraction 0.5`.

The three oracles ran clean on Zanzibar, Coagulation and Waterworks 1p. One
rejected variant: presenting resting roots exactly at their simulation
placement changed one pixel by one LSB on Zanzibar 1p, so the root words keep
the original composition.

**Measured.** Quiet host, 4K internal, 600 frames, three interleaved pairs on
the same binary:

| Map | FPS | Main-thread CPU/frame |
|---|---|---|
| Zanzibar 1p | 38.8 → 55.4 (+43%) | 24.9 → 17.2 ms |
| Coagulation 1p | 79.9 → 108.4 (+36%) | 11.6 → 8.4 ms |
| Zanzibar 2p | 41.3 → 51.7 (+25%) | 22.5 → 17.1 ms |
| Coagulation 2p | 85.8 → 97.8 (+14%) | 9.6 → 8.2 ms |

In free-running play the per-tick `match_step` looks slower: fewer ticks per
frame leave each tick's working set colder. At one tick per frame
(`--mp-reference-tick-cadence`) it is unchanged (Coagulation 3.08 → 2.9 ms).

**Remaining**, Zanzibar 1p after the change, in ms per frame:
- `Renderer::draw`: 4.2. It stays on the main thread in 1p because fixed
  shadows keep the render overlap off.
- Per-view model collection: 4.3.
- Depth input: 1.2. `authored_pass` reloads pass metadata every frame.
- Translated per-frame work, about 2 ms in total. The largest pieces are the
  shadow frame (0.7) and 41C10's model render (0.4).

Coagulation 1p after the change: 7.0 ms of main-thread CPU per frame in a
long steady-state run. In ms per frame:
- Draw plus depth input: 2.2 (`Renderer::draw` 1.7, depth input 0.5).
- Per-view collection: 1.9.
- Tick share: 0.7.
- Effect frame: 0.4.
- Translated per-frame work, about 1.2 in total: shadow frame 0.33,
  contrail geometry 0.19, 41C10 0.17, first-person light pose and particle
  frame about 0.1 each.

## Continued Coagulation optimization, 2026-10-22 (qualification interrupted)

The new performance targets are still **unverified**. A fresh visible two-player
Coagulation baseline at 1280x720 measured **21.36 FPS**, with all 60 measured
frames exceeding 16.67 ms. This development host has a Ryzen 7 9800X3D and
RTX 5080; concurrent builds became heavily contended during this follow-up.
Component timings below are not playable frame-rate results.

The current source adds five reductions in rendering preparation overhead:

* Reuse the ordered memory guard/observer dispatch list while retaining the
  original shared lock through every callback. Successful policy changes replace
  a retained epoch identity; bank changes, recursive calls and large callback
  banks retain checked handling. Exceptions and callback order are preserved.
* Compile literal instruction PCs through a small inline helper after checking
  the exact native service-site set, callback type and executor. Traces remain
  live at every instruction; required service PCs keep their original callback.
  The generic fallback stays out of line. A per-PC template experiment was
  cancelled before selection because its compilation cost was excessive.
* Memoize complete pixel FNV1a identities only for immutable pixel storage.
  Mutable access invalidates the identity before exposing a writer. Exposed
  pointers and iterators keep full hashing; shared immutable readers publish
  through atomics. Bitmap shape, GPU mutation and residency checks still run.
* Use fixed-size memmove for checked single-region scalar transfers. Extent,
  commitment, access, guards, observers and post-guard lookup still run in their
  original order; cross-owner staging remains intact.
* Share exact frame-local skins for admitted players, corpses and world/held
  weapons, extending the vehicle work below. The full owner, LOD, immutable
  model, node bytes and floating-point controls must agree. Original ordered
  sections, shader selection, coverage and lighting remain per view.

The dispatch microbenchmark measured 39.50 -> 30.08 ns per write (elapsed-time
medians). The final literal helper measured 6.70 -> 1.13 ns per instruction in
its isolated cursor loop. A separate seven-pair checked-memory probe measured
5.32 -> 4.38 ns per four-byte read and 26.43 -> 25.65 ns per write using process
CPU time. These are bounded owner-level comparisons under changing host load.
They do not establish an aggregate FPS gain.

The standalone cursor/specialization checks and dispatch/pixel sanitizer cases
pass. Counterfactually caching exposed writable pixels makes the exact-byte
test fail. Full original/native CMake checks, four-player ordered unit-vertex
comparisons, final installed-runtime qualification and matched FPS acceptance
are still pending for this source revision. The expanded current-source build
completed, but the user interrupted the runtime-test turn. Its owned supervisor
removed the disposable source/build tree; no new runtime was promoted. The
unit helper also has a subsequent local allocation cleanup that still needs
compilation and its vertex checks. Reports are under
`build/test-results/splitscreen-followup-20261022`; `checklist.json` records the
remaining work. The previously qualified runtime in the next section remains
selected. `interruption-storage.json` records cleanup and incomplete acceptance.

`profile_mp_render_cpu` also accepts a final `fixed` argument after its existing
UI argument. This mode uses actual prepared players, fixed session/auxiliary
seeds, 60 real preparation ticks, one original 30 Hz tick per rendered frame and
an interpolation fraction of 0.5. It records Linux main-thread CPU stamps and a
bounded player/vehicle/pose signature alongside per-view timings and skin reuse
counts. It compares equivalent CPU work; its accelerated tick cadence is a
diagnostic mode and is not ordinary-playback FPS acceptance. The installed
profiler remains available through `tools/profile_multiplayer.py`.

## Installed optimization pass, 2026-10-22

**The 120 FPS single-player / 60 FPS four-player targets remain unmet.** Both
launchers now select the qualified runtime
`bin/engine-builds/6c6a9b900ebb46e7ad3fb087d9e70fa4/halo2_native`, SHA-256
`7db35ababe9ba5e2b351ae5a9ccb96fb5800f66d9e2b3bc14b1dbc37470e7c79`.

This pass implements:

* Build-local specialization of 190,096 known literal operands in 135 generated
  modules. Canonical generated sources stay unchanged; dynamic/unmapped operands
  keep their checked helper. Build receipts retain input/output/mapping hashes.
* A guarded instruction cursor at 575,667 generated instruction sites. Active
  traces, required service/exception PCs, other executors and replaced/wrapped
  callbacks retain full delivery. The source simulation remains at 30 Hz.
* Frame-owned vehicle skins keyed by complete owner, LOD, model, exact WORLD
  nodes and floating-point controls. Source-selected draws skin their selected
  LOD once; fallback PRT banks reuse complete skins across compatible views.
  Every draw still performs its original ordered section/shader/lighting work.

The [MCC investigation](MCC_HIGH_FRAMERATE_RESEARCH_20261022.md) confirms Classic
interpolation and documents the inspected DLL. It does not establish an MCC
GPU-skinning or worker implementation. This pass adds no GPU skinning; the
remaining CPU skinning share is too small to explain the full FPS deficit.

Three alternating reference/optimized pairs for each player count used the
**same installed executable**, visible X11, 1280x720 rendering/presentation,
map-authored starting weapons, native HUD, muted audio, 150 submitted frames and
30 warmup frames. Operand specialization is enabled in both sides; only
`HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1` and
`HALO2_REFERENCE_REPEAT_VEHICLE_SKINS=1` differ from their unset optimized values.
These comparisons do not isolate the FPS effect of operand specialization.

| Local players | Reference median | Optimized median | Target |
| --- | ---: | ---: | ---: |
| 1 | 39.76 FPS | 53.75 FPS | 120 FPS |
| 2 | 30.78 FPS | 31.71 FPS | 60 FPS diagnostic gate |
| 4 | 14.89 FPS | 16.15 FPS | 60 FPS |

Host/workload variation was substantial: single-player reference samples ranged
37.19–58.25 FPS. The first two/four-player optimized samples regressed relative
to their adjacent references; the next two pairs improved. **A stable
split-screen speedup is not established.** All 18 deadline gates failed. These
RTX 5080 development-host measurements do not establish midrange-PC acceptance,
action-heavy play or physical-display timing.

A separate optimized two-player profile collected 930 CPU samples across 150
steady frames after 60 warmup frames. Main-thread utilization averaged 98.09%
of one core; device-wide GPU utilization averaged 13.32%. Exclusive samples
include skinning 7.63%, write validation 6.34%, the instruction cursor 5.59%,
memory writes 5.48%, reads 4.84% and region lookup 3.66%. Inclusive model-view
preparation accounts for 37.74%; inclusive memory writes account for 22.47%.
Inclusive owners overlap. The next substantial work is translated per-view
preparation and checked memory/observer overhead. GPU migration of a 7.63%
skinning share alone has an idealized speedup ceiling of about 1.08x.

Validation: 32 focused checks pass, including original instruction/register/
flag/memory fixtures, model skins and actual two/four-player ordered vehicle
draws with complete vertex-byte equality. Forcing repeated skins makes the
reuse assertion fail as intended. Eight launcher/source checks pass, and all
ten registered maps submit four frames; Coagulation also passes with two/four
players. This is bounded runtime/geometry evidence, not full parity acceptance.

Reports and every timing sample are in
`build/test-results/splitscreen-optimization-20261022`: `matrix/execution.json`,
per-run reports/TSVs, `optimized-profile-2p.json`, `specialization.json`,
`counterfactual.json`, `delivered-validation`, and `delivery`. The older copied
reference binary was excluded after concurrent camera changes; final comparisons
use one executable. An oversized PTY request closed the first temporary session;
its build was removed. The final session uses file-based requests. The qualified
runtime/resources are intentionally retained (190,837,312 bytes); original
assets, earlier runtimes and unrelated work are preserved. See `storage.json`
for final scratch cleanup verification.

The source tree was reorganized concurrently after this runtime snapshot.
Launcher info therefore reports source-path differences from its retained
inventory. The tested binary above remains selected; the new source layout and
unrelated work were preserved. The five Python specialization/cursor checks were
also rerun successfully against the reorganized tree. Qualification here belongs
to the frozen build recorded in `delivery/source-hashes.json`.

## Coagulation CPU profiling, 2026-10-22

The current targets are **120 FPS with one local player** and **60 FPS with four
local players**, preserving graphics quality and source behavior on a modern
midrange PC. **Both targets remain unmet.** This investigation adds an installed
runtime profiler and baseline measurements. The installed optimization pass
above follows this baseline; GPU migration and the FPS targets remain open.

Visible X11, idle Coagulation, map-authored starting weapons, muted audio,
1280x720 rendering and presentation, 110 submitted frames with 20 warmup frames:

| Local players | Mean FPS | Mean frame interval | Renderer host time | Target |
| --- | ---: | ---: | ---: | ---: |
| 1 | 26.99 | 37.05 ms | 2.13 ms | 120 FPS |
| 2 | 19.04 | 52.53 ms | 3.57 ms | 60 FPS diagnostic gate |
| 4 | 9.08 | 110.16 ms | 7.30 ms | 60 FPS |

All 90 sampled frames in each case miss their requested deadline. The separate
two-player resolution probes measured 18.49 FPS at 640x480 and 17.63 FPS at 1920x1080.
Existing emulator/development workloads continued throughout. This machine has
an NVIDIA RTX 5080; these samples do not establish midrange-PC acceptance or an
isolated before/after speedup. The runtime is the installed
`bin/engine-builds/a1f4d063d70a48cbbda5a5ed4f90e59d/halo2_native`, SHA-256
`bbcea619a10e4fcbe8b85c3521c01b8bb42c72c72558373bb0f349c3806233cc`.

### Installed-runtime profiler

`tools/profile_multiplayer.py` profiles the ordinary Linux SDL/Vulkan executable
without rebuilding the engine. It compiles its optional sampling library inside
`tools/run_test.py` storage and samples the actual main thread's CPU clock.
Presentation boundaries exclude loading, warmup and device destruction. Runtime
helper processes are excluded by executable identity, even when they inherit
`LD_PRELOAD`. Reports reject unmatched presentation counts, out-of-range samples
and unresolved executable samples.

The bounded JSON report contains renderer host phases, actual render/presentation
dimensions, executable/map hashes, CPU function and inclusive caller samples,
main-thread utilization, and optional NVIDIA device utilization. Raw stack
records, sampler binaries and raw runtime logs are removed with their scratch
owner. Sampling adds overhead, so FPS acceptance uses the separate unsampled
benchmark. Renderer host timings are not GPU execution timestamps. The existing
`profile_mp_render_cpu` developer harness supplies finer per-view observer
timings when a separately built harness is needed.

```bash
python3 tools/profile_multiplayer.py \
  --runtime bin/engine-builds/a1f4d063d70a48cbbda5a5ed4f90e59d/halo2_native \
  --map assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2/h2_maps_win64_dx11/coagulation.map \
  --players 2 --frames 240 --warmup 30 \
  --output analysis/framerate/coagulation-2p-profile.json
```

The tested one/two/four-player profiles contain 681/1136/2178 CPU samples across
120 steady frames each. Main-thread utilization averages 98.48/98.87/98.42% of
one core; corresponding device-wide GPU utilization averages 10.38/9.87/10.30%.
GPU utilization includes other applications and NVML's sampling window.

The two-player profile attributes roughly 10% of sampled CPU time to model
skinning, 60% to the translated source-runtime execution owner, 27% to simulation
steps and 23% to source shadow/visibility command construction. Inclusive caller
percentages overlap and must not be added together. Source shadow command
construction consumes CPU while the resulting rendering is already performed
by Vulkan.

### GPU offload candidates and limits

* Model skinning is the clearest candidate: `src/model_pose.cpp` currently
  transforms positions, normals, tangents and binormals on the CPU before
  uploading them. Keep authored bone weights, inverse binds, compound nodes,
  descending influence order and separately rounded arithmetic. Retain source
  node banks on the CPU for attachments, bounds and gameplay. GPU output needs
  comparison against the existing full-byte skin oracle and actual renderer
  acceptance, including shields, PRT and first-person geometry.
* Keep immutable model vertices resident and reuse completed skins across
  compatible views. GPU packing or indirect draws can then reduce CPU copies
  and uploads while preserving full draw keys, selected sections, pass order
  and per-owner lighting. Current two-player renderer packing is around 1 ms;
  this is a smaller cost than the source-runtime work.
* Radar compositing is another candidate: its CPU compositor accounts for
  roughly 2.5% of the two-player sample and 3.6% with four players. Preserve
  source contact selection and compare exact output pixels.
* The source-runtime instruction, memory, simulation, visibility and shadow
  command owners need CPU optimization as well. Moving the 10% skinning share
  entirely off the CPU has an idealized upper bound of about 1.11x CPU speedup
  in this workload, before dispatch/upload/synchronization costs. It cannot
  establish either FPS target by itself. Visibility decisions and ordered
  source side effects require their original ownership and validation.

The benchmark now accepts `--map`, `--shared-maps`, `--local-players`,
`--starting-weapon 0` and `--native-hud` for this exact MCC path, and
`--report-output` for a retained summary outside disposable raw output:

```bash
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/benchmark_framerate.py \
  --runtime bin/engine-builds/a1f4d063d70a48cbbda5a5ed4f90e59d/halo2_native \
  --map assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2/h2_maps_win64_dx11/coagulation.map \
  --shared-maps assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2/h2_maps_win64_dx11 \
  --scenario split --local-players 4 --starting-weapon 0 --native-hud \
  --frames 210 --warmup 30 --repetitions 3 --min-fps 60 \
  --max-frame-ms 16.666667 --output '@HALO2_TEST_SCRATCH@/frames' \
  --report-output analysis/framerate/coagulation-4p-benchmark.json
```

Small reports, actual commands, frame interval TSVs and bounded log tails are in
`build/test-results/splitscreen-performance-20261022`. One/two/four-player live
profiling, the actual MCC benchmark/resolution matrix, strict sampler compilation
and the forced-timeout cleanup check passed their diagnostic contracts. The FPS
gates remain failing. All owned scratch directories were verified removed;
no large generated artifacts were retained. Graphics, simulation and launcher
runtime selection are unchanged by the profiling tools.

## Historical locked 60 FPS at 4K, four players, 2026-10-11

**The target remains unmet.** This pass integrates borrowed immutable frame uploads, flat stable BSP grouping, reuse of unchanged selected object views, and exact current-WORLD-node weapon skins. It preserves resolution, authored mips, visibility/LOD, effects and simulation timing.

The final frozen Release cohort, using the normal launcher's registered Vista HUD assets under the existing competing load, measured **52.38 and 56.06 FPS** optimized. Respectively **626/780 and 423/780** sampled intervals exceeded 16.666667 ms. Reference runs were 35.57/56.97 FPS; load changed during the alternating batch, so the median difference is not a stable gain claim. Every valid sample is retained. Rendering was 3840×2160 with all four views, presented in a visible 1280×720 window.

Thirteen CPU/HUD/frontend tests and four GPU tests pass. The final 4K framebuffer matches the retained copy/scatter renderer byte for byte (maximum RGB error zero); audio-enabled gameplay/title validation checks pass. The earlier object-view/world-weapon FPS batches had invalid reference controls and are excluded from gain claims. The final batch records all four intended control flags correctly. Historical stock-HUD diagnostics are separate from the final registered-Vista-HUD cohort.

The exact tested [game launcher](/home/eric/Documents/Codex/2026-10-10/on-a-separate-note-work-on/outputs/halo2-performance-locked60/Halo2-Launcher) and [title launcher](/home/eric/Documents/Codex/2026-10-10/on-a-separate-note-work-on/outputs/halo2-performance-locked60/Halo2-Title-Launcher) select `/home/eric/Developer/Halo-2-Decompiled/build-performance-locked60-20261011/halo2_native` (SHA-256 `38e23b3b2007f2308be275cc09a93069fc0667b96476d0a9c5fc6d303903bc83`). The root launchers were preserved and currently select the concurrent `build-armor-framebuffer-20261012` feature cohort, which has not received these FPS measurements. Full run order, failing deadlines, quality proof, hashes and limits are in the [report](/home/eric/Documents/Codex/2026-10-10/on-a-separate-note-work-on/outputs/halo2-performance-report.md). Muted idle Lockout measurements do not prove action-heavy play or physical 4K TV output.

## Historical passes — earlier target and cohorts

The prior 100–120 FPS target below is superseded by the current locked 60 FPS requirement.

## Further CPU optimization pass — 2026-10-11

**The 100–120 FPS four-player 4K target is still unmet.** Exact SIMD skinning, unchanged-prop pose reuse, contiguous model-part appends and direct first-person appends are integrated into the shared source. No graphics settings, source LOD/visibility rules, effects, authored mips or simulation rate were reduced.

A lighter-load matched pair measured 50.13 FPS reference / 59.44 FPS optimized, followed by an extra optimized 60.23 FPS sample. The final three alternating pairs encountered sharply changing competing load: reference 13.87/37.58/32.73 FPS; optimized 10.44/38.11/25.53 FPS. The final medians (32.73/25.53) do not establish a stable gain under that workload. All six 100 FPS gates failed. External input/resize samples were rejected and retained. Rendering was 3840×2160 with four views and visible 1280×720 presentation; all four views share the complete-frame rate.

The frozen Release cohort passes 20 focused CPU/frontend/loading/physics/GPU tests, original-path full-byte oracles, a live 120-frame material/pose oracle, and an exact 4K paired BSP framebuffer comparison (maximum RGB error zero). Audio-enabled native and launcher validation smoke runs pass. Arena and parallel-planner experiments were removed after inconclusive/regressing measurements. Saved 1440p title/profile menus measured 46.85/51.76 FPS in one optimized pair under the current load.

Tested runtime: `build-performance-4k-continued2-20261011/halo2_native`; SHA-256 `345a2029d4d4ec1a9020374c323a4b816c93c8bd643e0bd98daccea35ae232bd`. Concurrent newer frontend/gameplay changes and the default launcher target were preserved. The tested cohort has a separate [performance launcher](/home/eric/Documents/Codex/2026-10-10/on-a-separate-note-work-on/outputs/halo2-performance-continued2/Halo2-Launcher) and [title launcher](/home/eric/Documents/Codex/2026-10-10/on-a-separate-note-work-on/outputs/halo2-performance-continued2/Halo2-Title-Launcher). The normal root launchers now select `build-performance-4k-current-20261011/halo2_native`, which is a different cohort and is not assigned these FPS measurements. Source hashes, every run, rejected samples and the delivery receipt are in the [performance report](/home/eric/Documents/Codex/2026-10-10/on-a-separate-note-work-on/outputs/halo2-performance-report.md).

## Historical results

The following sections retain earlier artifacts, tests and launcher selections. The current source/delivery boundary and measured results are stated above.

## Continued 4K delivery — 2026-10-11

Installed Release runtime: `/home/eric/Developer/Halo-2-Decompiled/build-performance-4k-integrated-20261011/halo2_native`; SHA-256 `9c8a410c1f3ab7e8e61dfc3234b901a9ce5ff5399bbe4ef9c17394d540406950`. Both launchers select it. Full details and archived evidence: [performance report](/home/eric/Documents/Codex/2026-10-10/on-a-separate-note-work-on/outputs/halo2-performance-report.md).

The final same-source comparison at 3840×2160 with four local players measured **24.56 FPS serial control / 51.28 FPS optimized** across three alternating 360-frame pairs (60 warmup frames). Optimized runs ranged 49.40–52.30 FPS, with p95 intervals 21.41–23.39 ms. **The 100–120 FPS target is still unmet.** Existing load was retained at the user's request. Rendering used a visible 1280×720 presentation window; every run asserts the 4K render dimensions. The saved 1440p title/profile menus measured 107.70/114.81 FPS in one matched pair.

Additional work overlaps CPU planning with the previous GPU frame, shares only full byte-identical frame-local vertices/poses, animates referenced material closure roots in original order, retains coherent mappings/owner descriptors and BSP capacities, and borrows ordered single/separated object ranges until upload. The detach-only graph shortcut is valid only while producers mutate values or recursively clone existing children; use the general graph check for new cross-root pass links.

No graphics settings, authored mips, source LOD/visibility/effects or simulation rate were reduced. Final 4K four-view RGB pixels match the original copy/scatter path exactly. CPU oracles, material/pose full-bank comparisons, focused GPU checks and an installed-launcher audio/validation smoke pass. The newer 42,047-root feature cohort differs from the earlier 44 FPS artifact; those numbers are not a direct before/after comparison. Full campaign/action stress and physical TV output remain unmeasured. Earlier measurements below describe their own cohorts.

Use a Release runtime for play and check `./Halo2-Launcher --launcher-info`
after installing it. An explicitly configured Debug build remains Debug even
though fresh single-config CMake builds default to RelWithDebInfo. The installed
launcher had selected `build-frontend/halo2_native` (Debug) during this audit.

The renderer now retains each material's immutable view-direction extern map,
computes view camera/fog inputs once, and resolves owner lighting once per root
and view. It prepares per-view material banks from the actual packed draw lists,
including every associated environment, overlay, planar-fog and bloom pass,
first-person geometry, owner object and reflected sky. The existing visibility,
LOD, layer checks and draw order still decide which of those passes render.
Previously it rebuilt and uploaded every resident material for every view.

Each material slot receives a complete first upload in each view. Later uploads
retain only its immutable decoded vertex instruction stream, and rewrite all
other bytes, including current animated programs/constants, lighting, water,
fog and camera data. A newly visible slot still receives its complete upload.
No material quality, geometry, authored animation, effects, HUD, resolution,
source visibility rules or simulation timing is reduced.

Sorted geometry grouping now records contiguous input runs under the existing
full draw key, then appends each key's runs in encounter order. It retains the
sorted draw/descriptor order and complete vertex bytes. The alpha-layer path
still retains its authored order. Multiplayer retains CPU vertex-staging
capacity between frames; every bank is cleared and rebuilt from current state.
No pose or draw-content cache is retained.

Forward world pipelines specialize the exact authored register-combiner control
words. Packed color constants remain live uniforms. Each frame checks global,
view and object-owner programs against the compiled controls; a changed control
word selects the original generic pipeline for that material. Texture, vertex,
lighting, fog and raster-state behavior is retained.

Material updates skip rebuilding absent extended vertex banks and use a
vectorizable float comparison for mutable vertex constants/attributes. The
comparison retains NaN and signed-zero equality results. Dynamic geometry scans
complete key runs before looking up their sorted output bucket.

Movie/radar image updates now retain staging capacity and join the next draw's
command buffer after its fence, with explicit image barriers. Repeated updates
keep the latest pixels and copy caller-owned data at the update call. A previous
720p experiment did not establish a gain; three later alternating 4K pairs
measured 30.64 FPS with serial uploads versus 39.92 FPS with queued uploads.
Those runs had competing workloads, so they are bounded diagnostic evidence.
`HALO2_REFERENCE_SERIAL_IMAGE_UPLOADS=1` retains the original serial path for
comparisons; `HALO2_REFERENCE_GENERIC_COMBINERS=1` disables combiner specialization.

## Repeatable measurement

Build the ordinary runtime and the optional detailed multiplayer profiler:

```bash
cmake -S . -B build-performance -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-performance --target halo2_native profile_mp_render_cpu -j4
python3 tools/benchmark_framerate.py --runtime build-performance/halo2_native \
  --output analysis/framerate/current --gameplay-xbe '/path/to/selected/default.xbe'
```

The benchmark opens visible windows by default, submits 210 frames per run,
discards 30 warmup frames, and reports median FPS across three runs. It covers a
fixed title view, profile/model menu and idle Lockout with one/four local players.
`--audio` includes normal audio; `--min-fps` adds an explicit performance gate.
Initial map/device setup is excluded. Saved settings are not changed. Runs reject
menu-screen changes from external input. Record competing workloads when
comparing results; this session deliberately retained the existing load. Menu preferences can restore a saved fullscreen
mode even when `--viewport` was supplied. New timing output records actual render
and presentation dimensions, and the benchmark rejects dimension changes during
sampling. Compare those dimensions between binaries.

For the hypothetical four-player 4K target, use the profiler's explicit internal
resolution path. Desktop compositors can clamp a requested 3840x2160 window to
the connected monitor. This command renders the scene at 3840x2160 and presents
it through a visible 1280x720 window; it asserts the actual render dimensions.
Source coverage, shader detail and screen-space effects use the internal scene
resolution, independently of the presentation surface. SSAA does not change
that logical viewport.

```bash
python3 tools/benchmark_framerate.py \
  --runtime build-performance/profile_mp_render_cpu --profile-multiplayer \
  --output analysis/framerate/4k --scenario split \
  --gameplay-xbe '/path/to/selected/default.xbe' \
  --width 3840 --height 2160 --frames 900 --warmup 120 \
  --min-fps 60 --max-frame-ms 16.666667
```

The current target is 60 complete four-view frames per second. The strict
16.666667 ms gate fails if any sampled interval exceeds the budget; the report
includes miss count, longest consecutive miss streak, p99 and maximum interval.
The default HUD/UI pack is resolved from the same asset-sources.tsv registry as
the launcher; --ui-assets can select an explicit pack containing pack.tsv and hud.tsv.
All four players share that frame rate. The internal-resolution measurement
includes downsampling/presentation costs and is not a physical 4K TV test.
The threshold remains a failing check when the measured target is unmet; the
report is written before returning failure.

`HALO2_RENDER_TIMINGS=/absolute/path/report.tsv` enables the same timing output
for ordinary runtime invocations. With no variable and no existing frame
observer, renderer draws do not read the profiling clock. The interval includes
previous-frame cleanup, current CPU preparation, GPU fence, acquire and present
waits. Named renderer phases are host timings, not GPU timestamps. Material
preparation now follows packing, so compare total frame time rather than just
the previous `view_constants` phase.

Hidden Wayland windows were throttled to approximately 20 FPS in this session;
Xvfb also incurred substantial presentation overhead. Neither was a valid
measurement of visible gameplay. Use the same visible backend, resolution,
source executable and audio settings for before/after comparisons.

## Measured result (2026-10-10)

Visible Wayland windows on an RTX 5080, requested 1280x720, audio muted, selected u5English
update executable, fixed title/profile views and idle Lockout. These compare
Release binaries from the same frozen source, with only the renderer changes.
Each run submits 210 frames, discards 30, and includes host acquire/present waits.
Lockout rendered at 1280x720. Surrounding menu captures show saved fullscreen
preferences restoring 2560x1440; the older per-frame rows do not independently
record their dimensions. The table reports the median FPS across three runs (two valid title baseline
runs; one was excluded because external input changed the sampled screen).

| Scene | Previous Release | Optimized Release |
|---|---:|---:|
| Title | 94.58 | 94.54 |
| Profile/model menu | 83.87 | 88.97 |
| Lockout, one local player | 179.92 | 119.91 |
| Lockout, four local players | 47.54 | 63.55 |

Single-player presentation varied between approximately 120 and 180 FPS even
within the baseline runs. Median acquire wait grew from 1.65 to 4.49 ms between
these batches; the material/view preparation and packing work decreased from
approximately 0.95 to 0.34 ms. Consequently the single-player FPS row does not
establish a performance improvement. Four-player CPU work was the substantive
repeatable gain: per-view preparation plus packing decreased from approximately
9.12 to 3.65 ms, and mean frame interval from 21.03 to 15.74 ms. The title remains
GPU/fence limited. The launcher installation additionally replaces the observed
Debug selection with the Release runtime; that build-setting benefit is separate
from this same-Release comparison.

The table above isolates the material optimization on the earlier frozen
source. Current-source geometry measurements are recorded separately. An initial
additional comparison was interrupted by other game instances and external
menu input; it was excluded.

Current-source alternating runs of the geometry changes (three runs per binary,
210 frames each, 30 warmup frames, actual 1280x720, idle four-view Lockout) measured
62.32 FPS before and 64.39 FPS after. This baseline already includes the material
optimization and current radar implementation. The final candidate ranged from
62.26 to 66.68 FPS, with frame-time tails near 19 ms; it is not a locked 60 FPS
claim. At that checkpoint, the installed runtime hash matched this measured candidate.

## Four-player 4K target (2026-10-11)

The delivered working snapshot is `build-performance-4k-20261011/halo2_native`
(Release). Later concurrent loading/inventory integrations in the shared checkout
are excluded from this executable after match initialization failed. Their
source is preserved; the performance changes remain integrated in the checkout.

Three alternating repetitions per mode measured:

| Scene | Actual render size | Generic combiners / serial uploads | Optimized defaults |
|---|---|---:|---:|
| Title |2560x1440|68.72 FPS|142.02 FPS|
| Profile/model menu |2560x1440|42.81 FPS|130.59 FPS|
| Four-player Lockout |3840x2160|21.98 FPS|44.38 FPS|

Both modes share the CPU optimizations. Gameplay presents through a visible
1280x720 window, with source screen-space selection at the 4K scene resolution.
Four-player runs discard60 of360 frames; menu runs discard30 of210. CPU load
varied substantially. Four-player optimized runs ranged28.18-50.84 FPS, so the
100 FPS gate remains failing. These samples do not establish a hardware ceiling.
One profile repetition initially failed a Vulkan memory allocation and passed
on a complete retry; that intermittent failure remains unisolated. Interrupted
pre-restart runs and clamped oversized-window attempts were excluded.

The full report, raw timings, source manifest, executable/shader hashes and
launcher checks are saved at
`/home/eric/Documents/Codex/2026-10-10/on-a-separate-note-work-on/outputs/halo2-performance-report.md`.
The delivered build passes HUD, special-texture, device-contract and float
comparison checks, plus the four-player audio/validation smoke run. Full campaign,
all maps/menus, physical TV refresh and other platforms remain unverified.

## Verification

An instrumented build compares the complete 3,712-byte uniform from the previous
preparation path with the optimized output at each call, compares each complete
GPU-bound upload after partial writes, and verifies every ordinary draw binding
was included in the current view's upload set. Real four-view Lockout and the
profile/model menu pass. This comparison concerns the existing native renderer;
it does not claim new original-Xbox rendering parity.

The special-texture GPU regression additionally compares reused four-view banks
with independently initialized renderer images after material visibility,
vertex constants and camera changes. Existing Elite volume/mip and one/four-view
scope-copy, resize and isolation checks remain in that test. Dynamic image
checks additionally compare one/four updated pages with independent initial
uploads, including repeated updates and caller-buffer
mutation after the update call. The independent CPU grouping comparator passes
554 complete vertex/draw/descriptor comparisons and 1,082 failure-state controls
in normal and ASan/UBSan executions. The original-data
HUD framebuffer and renderer logical-device tests also pass.

Full campaign, action-heavy/full-roster matches, all maps, physical refresh
measurement and other operating systems are outside this measurement scope.

## Campaign: Armory first room (2026-11-13)

This continues Codex's Armory investigation (`analysis/graphics-regression-20261113/`).
A sampled main-thread profile (`native_cpu_profiler.c`, 500 Hz, frames 360–479)
put 17.7% of the time in `Cache::load_source_bsp_metadata`. Every
`load_menu_world_source_bsp_owner` call copied the whole resident sbsp payload
(megabytes) into a `SourceTagMetadata` vector just to read one 88-byte instance
row. It is reached for every BSP-instance draw, in native world owner validation,
in pStencil projected-light palettes and in point casters, once per part.

Fixes (all exact; same bytes, same order, same errors):

- `src/ui/menu_world_source_geometry.cpp`: BSP owner/section reads borrow the
  resident cache bytes (`resident_bsp_metadata`) instead of copying them.
- `src/ui/menu_world_native_draw.cpp`:
  - the per-draw pipeline lookup first tries a shortcut key that names the
    static catalog SPIR-V span, instead of copying the full SPIR-V into a
    `std::map` key on every draw;
  - the previous frame's uniform buffers, descriptor pool and sets are reused
    (the caller already guarantees the GPU finished, as the old destroy
    required). A set is rewritten only when its buffer ranges change, and
    always in triangle-vertex mode.
- `MenuWorldSourceStorage::store_borrowed`: draws are stored from borrowed
  bindings, without per-draw `MenuWorldSourceBinding` copies.
- `src/app/main.cpp`: `campaign_world` keeps the unchanged BSP mesh as its
  prefix between frames (`mesh_generation` advances on a BSP switch). Only the
  per-frame debug/water tail is rewritten. `HALO2_REFERENCE_COPY_CAMPAIGN_WORLD`
  still forces the full copy.

Measured on the same tree (only the fix files differ), 1920x1018, interleaved
pairs, host load about 4–5:

| Mode | Before | After |
|---|---|---|
| Original | 82.3 / 78.0 ms (12.1 / 12.8 FPS) | 54.7 / 56.8 ms (18.3 / 17.6 FPS) |
| pStencil | 101.9 / 73.7 ms | 48.5 / 52.5 ms (20.6 / 19.0 FPS) |

In Original mode, renderer time went from 22 to 13.5 ms and native world
prepare from 13.6 to 5.2 ms.

Parity:

- Deterministic `--campaign-reference-capture .05 0 305419896` on 01a: 300
  frames at 1280x720 give identical frame and trace hashes for both builds,
  repeated twice.
- With `--validation` (Khronos layer), both builds produce zero messages and
  identical frames.
- pStencil cannot be captured in reference mode (the mode is rejected); its
  changed paths are the same owner bytes.
- `retail_menu_world_source_storage` and `retail_menu_world_source_pass` pass.

Remaining main-thread cost after the fixes (about 56 ms/frame CPU) is mostly
translated TU1.5 execution:

- the shadow frame (30%): 134C00 portal visibility 11%, 136EA0 BSP
  admission 7%, 44E80 4%, 136230 3%, shadow-mode 40E80 2%;
- the pStencil light receiver, which runs translated 31570 once per projected
  light (11%, included in the shadow-frame figure);
- campaign object/AI simulation (21%) and campaign view collection (17%:
  particles 6.7%, frustum cull 5%, live model draws 3.3%);
- material root selection, which scans every world vertex per frame (about 3%).

## Campaign native conversion, 2026-11-14

This follows the user's direction: "we shouldnt be using translated shit at all
it should just be native kernels". Each item below is proven against the
physical original, keeps a `HALO2_REFERENCE_*` switch that restores the old
route, and was checked for parity with the deterministic Armory capture. Every
capture of the 1280x720 Armory run gives frame `5627a51b…` and trace `8a9b58b3…`.

| Area | Native owners | Measured effect | Reference switch |
|---|---|---|---|
| Visibility bridges | Bridges under 136EA0 (direct-page reads, silent hooks, empty-stack `Scope::io`) | Armory 44 → 33 ms/frame on the same binary | `HALO2_REFERENCE_SCOPED_ACCESS=1` |
| PVS flood | 1532A0 → 170C80 → 170FB0 → 44680 | −96.5k translated instructions/frame, about −0.5 ms | `HALO2_REFERENCE_PVS_FLOOD=1` |
| Render commands, every view mode | 40E80 family at the projected-light and MP shadow-root sites; new 45550 and 458C0 kernels | about −22% of the census | `HALO2_REFERENCE_LIGHT_COMMANDS`, `_SHADOW_COMMANDS`, `_SECONDARY_CULL`, `_SPHERE_CULL` |
| Projection executor | 44E80, 2E79A0, 2E6290, 2E7C40, 2E6ED0, 45A70, 309600, 3092A0 | census −41% (1.14M → 0.67M translated instructions/frame); family 141M → 1M per 300 frames | `HALO2_REFERENCE_PROJECTION_QUERY=1` |
| Campaign render overlap | The draw runs on the render thread beside the next frame's input and simulation; an input-latency guard prevents added latency | least-loaded pairs 50 → 40 ms | `HALO2_REFERENCE_SYNC_RENDER=1` |
| Node rebuild | BF050/BF0C0/BFF20/BFB90 at every call site, with C9070 and the function-value callbacks, the 283E10/284090/296890 codec owners, and C3750/1692E0/145070 placement | Census: Armory −13.4%, Coagulation 4p −27.9%. Armory frame rate 25.84 → 26.33 fps at load 3.5. The 2026-11-15 sweep of every MP map (1p and 4p, scripted input) and campaign 01a–08b: 1,594,436 calls, 0 differences, 0 refusals | native default; `HALO2_REFERENCE_NODE_REBUILD_TU15=1` |

Notes on the measurements:

- **Census.** Use `HALO2_CENSUS_NATIVE_BRIDGES=1` when taking an instruction
  census. Without it, cursor-gated native bridges fall back to translated code
  under `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1`, and the census overcounts
  translated work.
- **Light-model build (4B5C0).** The native light-model build (4B5C0) is
  opt-in (`HALO2_NATIVE_LIGHT_MODEL_BUILD=1`). A per-byte store trace found
  different node matrices in 5,100 of 13,193 calls, even though frames matched.
  Work on that is continuing.
- **Shared tree builds.** Builds of the shared tree go through
  `scratchpad/build_perf.sh` (flock). A/B runs compare the same binary with a
  reference switch on and off.

Cumulative same-binary A/B (2026-11-14 23:00). One frozen shared build was run
with every reference switch above set (old routes plus the synchronous draw)
and again with all of them unset. The workload was the Armory first room at
1904x1018, in interleaved pairs. Host load was 7–24, from concurrent agent
builds, so absolute times are inflated about 2x against the quiet-host
figures above.

| Mode | Pair 1 | Pair 2 | Pair 3 |
|---|---|---|---|
| Original | 110.6 → 88.5 ms | 109.1 → 85.5 ms | 105.7 → 80.6 ms |
| pStencil | 161.2 → 57.6 ms | 75.4 → 36.8 ms | 102.7 → 69.4 ms |

Every pair favours native. On that build, with the Khronos validation layer,
frames match the original baseline (`5627a51b…`, trace `8a9b58b3…`) with 0
messages.
