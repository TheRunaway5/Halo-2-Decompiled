# Renderer corrections and opening visual check, 2026-10-18/19

This follows [the original-code flicker audit](RENDERER_FLICKER_ANALYSIS_20261018.md).
Original Xbox/TU1.5 programs, declarations and retained arithmetic contracts remain
the visual authority. The changes address demonstrated defects and remove work
without replacing materials, reducing geometry/effects, or guessing new clipping
thresholds.

## Corrections

* `shaders/vertex_program.glsl` now clamps the signed reciprocal itself for RCC.
  The former `sign(input) * clamp(abs(reciprocal))` returned zero for both signed
  zero operands, collapsing a camera-plane vertex toward an unrelated screen
  position. The corrected signed endpoints agree with the production CPU
  interpreter and retained NV2A translation contract, including infinite inputs.
  This is a demonstrated position defect; the intermittent reported gameplay
  frames have not been captured and attributed to it.
  A subsequent NaN differential case also failed on GPU (finite `-2^64` instead
  of the CPU interpreter's NaN). The signed clamp now leaves NaNs unchanged;
  finite comparisons remain bit-exact rather than accepting tolerance drift.
* `src/renderer.cpp` keeps each sorted transparent step's optional lighting value
  alive through frame consumption. It formerly retained a pointer into a copied
  binding vector destroyed after planning. Geometry remains caller-owned; this
  correction does not assert that vertices were freed.
* `src/material_descriptor_range.hpp` selects a material's existing contiguous
  descriptor range with ordered-map bounds. Four descriptor-update loops
  formerly searched every unrelated material/lightmap alias. Contents, ascending
  lightmap order, Vulkan writes, update timing and samplers remain identical.
* Original campaign rendering now defers material preparation until the actual
  frame consumers are known, as the optional pStencil path already did. Multiple
  requests coalesce into one ordered update. Animation still advances through
  the existing producers; hidden materials receive their current values on
  admission. Every owner of a shared child remains selected in root order.
  The consumer traversal also covers shader-callback transparent geometry,
  bank-only light volumes, lens quads and pStencil specular companions.
* Campaign CPU vertex buffers retain capacity between immediate submissions.
  When no dynamic objects are appended to the BSP stream, the frame borrows the
  unchanged mesh instead of making a second full copy. GPU lifetime protection
  and uploaded vertex bytes remain unchanged.

The transparent geometry producer and ordinary clipping/depth boundaries listed
in the audit remain explicit. These changes do not substitute a guessed sort or
claim the whole renderer is equivalent to an Xbox framebuffer.

## Performance evidence

`tests/material_descriptor_range_tests.cpp` compares exact membership and order
in 136 cases, including missing materials and maximum unsigned keys. Its
alternating seven-sample CPU benchmark measured the following medians on this
host, with identical descriptor-order checksums:

| Materials / aliases | Materials selected | Complete scan | Bounded range |
| --- | ---: | ---: | ---: |
| 512 / 2,048 | 512 | 2.145070 ms | 0.014809 ms |
| 2,048 / 8,192 | 1,024 | 17.018700 ms | 0.113065 ms |
| 8,192 / 32,768 | 1,024 | 74.028500 ms | 0.160045 ms |

These are descriptor-search costs, not whole-frame or FPS improvements. The
before-change helper control still took the same time as the complete scan.
Receipts: `analysis/renderer-fixes-20261018/descriptor-{baseline,after}.json`.

The normal opening replay was CPU limited: the observed process used roughly
one logical core while its sampled GPU SM utilization was typically 1% (device
total includes other applications). A 200-sample main-thread GDB profile found
46 samples (23%) inside full-bank material updates, including vertex-bank and
declaration packing. Skinning arithmetic itself was not the largest sampled
cost. The profile also caught accumulated vertex copies/reallocations. These
findings motivated the material and buffer changes above; they are not an MCC
utilization measurement. See `cpu-profile/report.json` and
`opening-repeat/gpu-cpu-samples.json` in the same analysis directory.

The MCC Classic/OG distinction and remaining GPU-skinning bridge are documented
in [the source and binary architecture audit](MCC_CLASSIC_RENDERER_ARCHITECTURE_20261018.md).
An inspected DX11 backend does not establish MCC's CPU/GPU scheduling, culling
or frames-in-flight policy. The objective here is less CPU starvation and
better frame throughput, not an arbitrary GPU-busy percentage.

## Regression and visual evidence

The fresh native build and nine focused CTests passed, with no skipped checks.
The current receipt is `material-workbench/attempt-08` beneath
`analysis/renderer-fixes-20261018`. Performance comparison and bounded transient
GPU readbacks are recorded separately; virtual presentation is not a
desktop frame-rate measurement.

* The permanent GPU reciprocal regression was run before correction and failed
  at signed zero: expected `+2^64`, actual zero. It covers 16 independent exact
  operands plus eight positions through an unchanged original shadow program.
  After correction, all 24 CPU cases and all 24 GPU cases passed exactly, with
  no Vulkan diagnostics (`reciprocal-after.json`).
  The extended regression has 26 CPU and 26 GPU cases, adding both signed NaNs.
  `reciprocal-nan-before.json` records the finite-output mismatch;
  `reciprocal-nan-after.json` and the final CTest receipt pass. All finite
  outputs compare exactly; NaN classification is retained without assuming a
  hardware NaN payload.
* The real renderer descriptor comparison builds a second renderer with the
  exact former complete scan, sharing all other engine services and shaders.
  It compares complete framebuffers across individual/all animated-image
  changes, restoration, repetition, resize and late material admission with
  512 material/lightmap aliases, including filtered versus full-bank updates.
  All 18 complete image comparisons passed, including a root that becomes
  visible after being animated off-screen. These are controlled fixtures,
  not substituted original game materials.
* The lifetime validation instruments the real renderer planning/consumption
  and object-lighting reads with ASAN/UBSAN. External GPU-driver leak detection
  is disabled; this is scoped ownership instrumentation, not every translated
  game routine under sanitizers.
  Its eight-frame ASAN/UBSAN run passed without diagnostics; the private CMake
  build was removed (`lifetime-validation/report.json`).
* The opening playback uses the normal source-owned campaign clock, Original
  graphics, geometry validation, SIMD/scalar verification, borrowed/upload
  comparison, exact renderer snapshots and a 60 Hz recording of the game's own
  visible window. Capture/validation overhead excludes an uncontended FPS claim.
  A finite playback cannot guarantee absence of a random one-frame defect.

Three complete opening runs reached all 13 expected scene calls in order with
no script faults. The checked run also passed geometry, SIMD/scalar,
borrowed/copy upload and Vulkan synchronization checks. The third run verified
that its 2560x1378 external capture covered the complete window. Desktop-video
triplets contain short flat/unlit or white samples; this remains unresolved
and is not a flicker-free verdict. Those samples have not been established as
completed GPU presents. A bounded final-color GPU-readback diagnostic is being
used to distinguish renderer output from presentation/capture effects.

The corrected current runtime subsequently completed all 13 opening scenes in
order. At all 20 selected source-clock targets, the actual frame, a repeated
filtered-material draw, a full-bank draw and a repeated full-bank draw were
byte-identical. There were no script or Vulkan diagnostics. See
`opening-material-controls-v2/report.json` and its contact sheet. This run used
a private X server after every physical display connector became disconnected;
its presentation timings cannot establish normal gameplay FPS.

The preceding diagnostic run was stopped after 11 captured targets to investigate
two small pixel differences (the first: 7,155 pixels, maximum channel delta 8).
It is retained as a failed/incomplete diagnostic, not counted as a passing
opening. The next complete run added both filtered and full-bank repeat controls;
all four images matched at every target. The earlier difference was not
reproduced or conclusively attributed. Neither those checks nor a finite
capture sequence certify every random gameplay frame as flicker-free.

A separate source-tick 1050..1350 prefix captured all 74 submitted final-color
GPU frames in that interval. The candidate ranking inspected every readback;
manual review of all three ranked triplet pages showed camera motion and an
authored cut, without a reproduced stretched triangle or flat full-frame flash.
Readback serialization changes timing, so this does not establish the cause of
the earlier desktop-video samples. The prefix is explicitly not a full-opening
run (`opening-gpu-prefix/report.json`).

After the private build/source tree was deleted, the retained candidate also
passed 60 frames of Original Lockout using the source object world, original
weapon HUD selection and Vulkan/synchronization validation, with no diagnostics.
Weather, shadow, decal, particle and lens submissions remained source-owned.
The smoke covers initialization/submission, not an entire multiplayer match or
the unresolved authored event-lifecycle warnings in its log
(`multiplayer-smoke/report.json`).

## Same-runtime CPU preparation comparison

Both normal-clock opening runs completed all 13 scenes using the same executable
at 2560x1378. The control enabled the earlier full-bank material preparation and
BSP copying; the selected path used the new consumer filter and borrowing.
Both retained vertex-buffer capacity. The first 100 submitted frames were
excluded from these preparation medians.

| Path | Resident roots | Prepared roots, median | Material preparation, median |
| --- | ---: | ---: | ---: |
| Full-bank control | 12,076 | 12,076 | 6.80297 ms |
| Selected path | 12,076 | 221 | 1.27367 ms |

That is 81.28% less CPU time in this phase (5.34x). It is not 81% more whole-game
FPS. Both runs were on private virtual displays, with other system jobs
uncontrolled. Virtual presentation and GPU utilization do not establish normal
desktop performance or equivalence to MCC. Process SM samples were 2% in both
runs; a higher GPU-busy percentage is not a verified result here. The small
frame-fence medians (about 0.005 ms) also do not identify GPU saturation.
Receipt: `opening-performance-comparison.json`.

Ordinary packed base-model skinning still has a CPU bridge, and the translated
simulation/render admission owners remain CPU work. This change does not claim
that those architectures have been replaced or that MCC-level throughput has
been certified.

The initial broader CTest attempt passed 12/14 tests. Its king-boundary test
could not load an existing required original-reference report
(`analysis/multiplayer/game-variants/king-hill-emission-source-20261012/complete/report.json`).
That unchanged fixture was not replaced with invented evidence. The first
descriptor-control attempt exposed an invalid synthetic test material (animated
updates require forward-pass admission); the controlled fixture was corrected
before rerunning the real-renderer comparison.

## Storage

CMake validation uses `tools/run_tests.py`; one-off probes and playback use
`tools/run_test.py`. Temporary compiler products, raw screenshots and continuous
video used owned scratch. The installed runtime, rollback launchers, small
receipts and selected review images are explicitly retained.

The 1,092,874,240-byte private workbench tree was removed. The explicitly retained
installed runtime, shaders, launchers and admission receipt are 93,182,454 bytes
(88.87 MiB), at
`analysis/renderer-fixes-20261018/material-workbench/runtime`; its executable
SHA-256 is `8e31e561103bb773a5f40b2171e9c6732bfc98846efc42e682a707334fcac2a3`.
Selected JPEG review evidence is 19,126,672 bytes (18.24 MiB); small receipts
also remain. Raw frames, continuous
video, object files, archives and test binaries were temporary. See
`analysis/renderer-fixes-20261018/cleanup.json` for exact retained image paths,
sizes and removal checks.

Both normal launchers now select the installed runtime above, with its compiled
default shader directory. The previous `build-multiplayer/halo2_native` and its
shaders were preserved. The old launcher pair is retained for rollback at
`analysis/renderer-fixes-20261018/launcher-rollback` (436,224 bytes). Both installed
`--launcher-info` results still report the native MCC Lockout test available;
`delivery.json` records their exact selection and hashes.

Delivery first passed a separate 60-frame MCC Lockout source-object-world test,
with the selected private cache, exact executable and 105-file shader inventory
pinned in `mcc-delivery-validation/report.json`. This tests this native port with
MCC content, not Microsoft's MCC engine, and does not grant full conversion or
gameplay parity. Twelve missing legacy shader/reference files were copied from
the existing qualified package after checking their hashes; no current
production shader or original runtime was overwritten.

The launcher reader's exact 97-file restriction rejected an explicitly pinned
new renderer shader. It now permits an expanded qualified family while retaining
the baseline minimum, exact directory membership and per-file hashes. The new
regression failed before correction, then all 14 capability-parser tests passed;
undeclared additions and changed programs still reject. The live MCC validation
was required before publishing the updated sidecar and switching launchers.
