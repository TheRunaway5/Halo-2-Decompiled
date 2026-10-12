# Native frame-rate performance

## Locked 60 FPS at 4K, four players — current target

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
