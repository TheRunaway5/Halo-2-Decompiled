# Opening camera-cut performance, 2026-10-19

This follows the [renderer corrections](RENDERER_CORRECTNESS_20261018.md).
The user's visible playback was smoother, with remaining drops at camera cuts.
These corrections preserve source clocks, scene commands, geometry, materials,
effects and draw ordering. Camera smoothing affects native presentation only;
the source camera banks and original animation sampler remain authoritative.
Xbox TU1.5 remains the behavior authority.

**Runtime test scope correction:** the user requires the launcher-selected MCC
cache. All earlier opening timing and live camera diagnostics below used
Original disc maps and are reference diagnostics only. They do not validate
MCC-cache playback. Runtime qualification after that correction uses only
`assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b/halo2/h2_maps_win64_dx11`.
The current MCC opening is blocked before rendering by the verified
rigid/one-influence model projection boundary. No Original or Vista opening
is substituted for that failed MCC test.

## Reproduction

`analysis/camera-cut-performance-20261019/profile_cuts.py` runs the actual
00a opening on the desktop, Original graphics, PulseAudio and 2560x1378.
After the user's cache correction it selects only MCC map/shared-map paths;
earlier receipts explicitly retain their Original-map commands and hashes.
It retains phase timers and source-tick metadata, without video, GPU readback,
geometry checks or a debugger in the clean measurement. Invoke it with
`tools/run_test.py`; the receipt records the exact executable hash and command.

The first prefix reproduced these transition-frame intervals:

| Source scene | Tick | Largest interval near transition |
| --- | ---: | ---: |
| x01_scene_01b | 743 | 91.84 ms |
| x01_scene_02 | 1012 | 52.34 ms |
| x01_scene_03 | 1490 | 60.78 ms |
| x01_scene_04 | 1676 | 49.78 ms |
| x01_scene_05 | 1940 | 96.29 ms |
| x01_scene_06 | 2164 | 97.64 ms |

Start-to-start intervals include the *previous* renderer submission and the
current upstream preparation. For the 91.84 ms gap, the previous renderer took
6.03 ms, leaving 85.81 ms outside it. The 96.29/97.64 ms gaps likewise leave
89.51/93.47 ms outside the previous submission. This locates the large stalls
upstream of the GPU submission without attributing them all to one routine.

The initial feedback command returns 1 when a transition window exceeds 50 ms.
That is a coarse symptom detector, not a permanent machine-independent FPS
test. A separately running native validation was observed during this work;
uncontrolled system activity limits comparisons of separate executions.

## CPU sampling and bounded correction

The separate asynchronous GDB/MI diagnostic collected 350 interrupted main
thread stacks around ticks 695..983. 120 contained campaign view collection,
60 simulation stepping, 53 renderer submission and 95 source write validation.
These are overlapping inclusive counts, not additive CPU percentages. The
sampled write path includes the physical protection callback and its mutex.
The debugger changes pacing; its frame intervals are excluded from performance
claims. The first synchronous debugger attempt failed and is explicitly
superseded, with its owned scratch removed.

`SourceRuntimeHostPhysicalMemory::allocate()` publishes only upper-half source
tokens (`physical | 0x80000000`). Ownership restoration independently enforces
that namespace. Its registered guard formerly took the physical-page mutex
and searched the protected-page set for every ordinary translated store too.
Ranges ending at or below `0x80000000` cannot intersect those pages. They now
return from this specific guard before acquiring its mutex. Cross-boundary
ranges and physical/native-alias writes keep the full check. Generic memory
permissions, commitment, observers and other resource guards are unchanged.

`HALO2_REFERENCE_ALL_PHYSICAL_WRITE_GUARDS=1`, read at physical-owner creation,
retains the prior guard for same-executable controls. Normal play uses the
bounded check.

The actual adapter regression performs two million ordinary stores with a
registered physical owner and mutation observer. Before correction it fails:
two million physical-page mutex acquisitions. After correction that count is
zero, with all two million observer calls retained. It also verifies read-only
ordinary resources, physical permissions, native aliases, mutable borrowing,
rejected boundary-crossing writes without prefix mutation and permission
restoration. Six focused CMake checks pass, including existing physical,
shared-backing and snapshot contracts/adoption checks.

Seven alternating same-binary adapter controls measured 41.943 ms median with
the prior guard and 36.2707 ms bounded, about 13.5% less time in that store
workload. A candidate compiler ran concurrently. This is not a claimed 13.5%
game FPS gain or proof that all transition stalls have gone away.

Evidence is under `analysis/camera-cut-performance-20261019`: `baseline/`,
`stack-profile-v2/`, `guard-before/`, `guard-after/` and `guard-benchmark.json`.

## Pixelated outgoing fade

The original capture target was 640x480, the original scene resolution. The
native adapter kept that fixed capture size while drawing the current scene
at 2560x1378. The outgoing image therefore suddenly lost detail when captured
for a crossfade. This was a GPU image-resolution mismatch, not texture quality.

History now uses the scene target's actual pixel dimensions. Its image is
preallocated at startup; a different extent is adopted only at the next
capture. Resizing a window during a fade preserves the already captured
outgoing image until that capture is replaced. The source fade clock, program,
point sampling, alpha quantization, blend and color behavior are unchanged.
Original 640x480 input/output remains covered by the original fixture.

The real Vulkan regression uses alternating single-pixel stripes to detect
lost detail. The previous capture fails with 3,527,680 channel mismatches.
The correction passes 11,657,282 exact BGRA8 pixels across original-size,
2560x1378, resized 853x479 history reuse and recapture cases, with validation
and synchronization checks enabled. Its images exist only in test memory.
Evidence: `fade-before/report.json`, `fade-after/report.json`.

## Camera presentation between source ticks

The original generated animation-camera path samples integer game ticks
(`source_campaign_camera_tu15_generated.cpp`, original instructions
16FED8..16FEE7). Calling the owner every render frame does not make those
animation samples fractional. A read-only debugger probe of the running
opening found 28 adjacent same-tick frame pairs, all with identical completed
source camera poses. The updater was still called 121 times. That measured
stepping justified a native presentation adapter rather than changing clocks
inside the original routine.

`CinematicCameraInterpolation` consumes consecutive completed source cameras
and the existing render fraction. It interpolates position, field of view,
aspect and aim displacement, and uses quaternion interpolation for orientation.
This uses the usual previous/current completed-tick presentation interval,
with up to one simulation tick of presentation delay. Source camera banks,
script waits, animations, audio and physics retain their original timing.
Authored camera commands, checkpoint restores, BSP changes, missing history,
skipped/backward ticks and level restarts invalidate the interval. Actual cuts
therefore take their new source pose immediately.

Each camera channel must first demonstrate held source output between ticks
before smoothing is enabled. Channels already updating per frame pass through;
a channel that starts updating within a tick passes through on that first
update. A paused/reset fractional clock cannot rewind a displayed interval.
The adapter allocates no memory per frame and does not write source banks.

The regression exercises the real campaign frame clock at 30/60/90/120/144/165/
240 Hz, including 1,274 moving zero-tick frames, exact endpoints, orientation
roll, invalid fractions, every invalidation class and native per-frame input.
The original pass-through control fails on held zero-tick frames. A separate
rate-switch control also fails before the immediate pass-through correction
and passes afterward (`camera-transition-regression.json`).

## Whole-opening timing limits

Two complete normal-clock visible playbacks used the same earlier CPU/fade
candidate, changing only the physical-guard diagnostic switch. Both completed
all 13 scenes at 2560x1378 and returned zero. Results were mixed:

| Physical guard | Frames | Median interval | 95th percentile |
| --- | ---: | ---: | ---: |
| Prior full guard | 9,641 | 16.0063 ms | 44.3544 ms |
| Bounded guard | 9,434 | 18.6332 ms | 38.2378 ms |

Another native validation process was present at the reference run's start;
compilation and system workload were uncontrolled. These runs establish
completion and reproduce remaining stalls. They do not establish a causal
whole-game FPS improvement, fixed frame pacing, higher GPU utilization, or
performance equivalence to Microsoft's MCC engine. Source view collection and
simulation work remain substantial CPU costs. Further reductions need bounded
reuse/batching proven against their mutation and lifetime contracts; skipping
simulation, lowering effects or changing scene detail is outside this change.
Evidence: `opening-comparison.json`, `opening-reference/`, `opening-bounded/`.

## Combined candidate validation and delivery

The first camera candidate exposed a packaging error before camera sampling:
its shader directory still named its removed private build. This candidate
was never installed. The replacement explicitly sets
`HALO2_RUNTIME_SHADER_DIR` to its retained runtime bundle. The failed startup
and its diagnostics remain under `camera-held-control/`; it is not counted as
a passed gameplay or camera check.

The final combined runtime SHA-256 is
`279e6d53193a509d2e1479bc5b1c2a43a80e50122818bf9e1a668fd4d8832cb4`.
Its frozen source inventory is `candidate-v3/source-sha256.json`, and the exact
build/check commands are in `candidate-v3/command.json`. All 17 selected CMake
checks pass. The original camera-owner reference regression also passes under
the unchanged aggregate 60 MiB, zero-swap containment.

The live **Original-map diagnostic**, explicitly not MCC acceptance, observed
134 held same-tick camera pairs in the reference control, with zero moving
outputs. The presentation adapter observed 12 held source pairs, five moving
outputs and an exact new source pose at the first sampled cut, with zero cut
mismatches. Debugger overhead changes pacing; these counts are not an FPS
comparison. Other held frames need not move when the source shot/fraction is
stationary or history was invalidated. `camera-render-comparison.json` retains
both controls. The subsequent Original-map full replay was cancelled when the
user corrected the cache requirement.

`mcc-opening-prefix/report.json` records the actual MCC 00a launch, cache/map
hash and failure before a rendered frame:

```text
MCC model vertex projection requires verified rigid or one-influence rigid-boned geometry
```

The rejecting owner is `src/mcc_model_vertex_layout.cpp`; no unverified weighted
vertex conversion or Original/Vista asset fallback was added. This is an open
MCC campaign admission boundary, so MCC cutscene smoothness, fade behavior and
flicker-free opening playback remain **unverified**.
The previous launcher-selected runtime fails the same MCC launch with the same
diagnostic (`mcc-opening-reference-prefix/report.json`), establishing that this
admission failure predates the combined renderer candidate.

The MCC Lockout source-object-world 60-frame smoke passes with Vulkan and
synchronization validation and zero renderer/script errors. Both local
launchers now select the qualified combined candidate and retain the existing
interim MCC Lockout capability. This is our native renderer using MCC assets,
not a test of Microsoft's MCC executable. Qualification, shader/map hashes,
launcher information and the explicit opening limitation are in
`delivery-mcc.json` and `qualification-mcc-v3-mcc/report.json`.

The additional traced MCC Lockout run also passes all 60 frames, with no
renderer/script/validation diagnostics. Its actual map file opens are exclusively
the selected MCC cache's `lockout.map`, `mainmenu.map`, `shared.map` and
`single_player_shared.map`. No ISO/disc or Vista map path was observed.
`mcc-asset-open-audit.json` retains the command, map paths and executable hash;
the raw syscall trace and source-world test state are removed with scratch.
The TU1.5 executable remains a code/reference input, not a map-asset fallback.

`cleanup.json` and `final-verification.json` confirm owned scratch/build trees
are removed, including the cleanup runner itself. Superseded, never-installed
candidate exports totaling 185,886,550 bytes were removed after process,
launcher, tracked-file, link and ownership checks. Retained large outputs are
the launcher-selected `candidate-v3/runtime/` (93,305,278 bytes, executable,
shader bundle and qualification) and `launcher-rollback/` (436,224 bytes).
Source hashes, commands and small reports remain. The previous installed
runtime and unrelated work are preserved. No new raw frames, video, memory
banks, syscall traces, private builds or compiler object/archive copies remain.
