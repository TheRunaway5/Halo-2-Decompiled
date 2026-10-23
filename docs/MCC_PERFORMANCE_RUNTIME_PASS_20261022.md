# Current runtime performance pass, 2026-10-22

The current source snapshot builds and runs. The first integrated vector-read
comparison does **not** establish 4K120 or four-player 4K60. Two completed
four-player pairs favor the optimized path, while the one-player result is
inconclusive. Further improvements remain necessary.

## Version and admission

Runtime SHA-256:
`efe69297809e687bb928dba551934e5a4be2aaeffebfc14e24b759463df5bcee`.
It was built through `tools/run_tests.py --build-worker` in the finite third
owned session, from 8,518 hashed source inputs. All seven focused CTests pass:
vector reads and scalar reference, write dispatch and reference, benchmark
dimensions, full-size capture admission and instruction cursor. The first
component's separate Release and ASan/UBSan acceptance remains supporting
evidence, not a complete runtime sanitizer run.

The snapshot's authored-data symlink was rejected by release archive ownership
validation. Copying its four small definitions into the same owned source
repaired this packaging input without changing original data or engine source;
the existing compiled objects were reused.

Source/build/commands are retained in
[the session](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/session/session.json),
[source receipt](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/freeze.json),
[authored-data repair](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/refresh-authored-data.json)
and [build/test result](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/validation/report.json).
Existing launcher selection was not changed by this work.

## Fresh CPU attribution

These profiles use idle Coagulation, muted audio, actual 1280×720 scene and
presentation, 210 frames with 30 warmup, and the optimized vector path. Sampling
adds overhead; the intervals below are **not unsampled FPS acceptance**.

| Diagnostic | One player | Two players | Four players |
| --- | ---: | ---: | ---: |
| CPU samples | 1,327 | 2,319 | 5,417 |
| Mean complete frame interval | 30.56 ms | 52.47 ms | 123.70 ms |
| Mean renderer host interval | 3.19 ms | 5.32 ms | 11.63 ms |
| Main-thread CPU, one-core scale | 96.5% | 98.2% | 97.2% |
| Device-wide GPU utilization | 9.9% | 8.8% | 14.9% |
| Plane-volume MOPP interpreter, inclusive | 8.06% | 9.19% | 7.14% |
| Model skinning, exclusive | 10.02% | 11.43% | 8.82% |
| Motion-sensor composite, exclusive | 1.96% | 3.41% | 4.67% |

Renderer intervals are host wall time. GPU utilization includes unrelated
applications. Inclusive owners overlap. The source-execution owner accounts
for roughly half the one/two-player samples, and checked memory/observer work
remains material. The historically hot visibility query has a smaller share
here; its old 19% share must not be presented as this build's opportunity.

The exact function rows, environment, hardware and dimensions are in the
[one-player](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/profiles/1p-profile.json),
[two-player](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/profiles/2p-profile.json)
and [four-player](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/profiles/4p-profile.json)
reports. These results support further CPU work; they do not measure a 4K GPU
execution budget or campaign/action-heavy acceptance.

## Unsampled internal 4K comparisons

All admitted runs render at 3840×2160 and present through an ordinary visible
1280×720 X11 window. This is internal 4K rendering, not physical 4K presentation.
The same executable, map, native HUD, default authored weapon and settings are
used. Each run samples 180 frames after 30 warmup. Pair 2 reverses execution order.

| Players / complete pair | Scalar diagnostic control | Optimized vector path | FPS ratio change |
| --- | ---: | ---: | ---: |
| One / 1 | 40.26 FPS | 40.82 FPS | +1.38% |
| One / 2 | 38.05 FPS | 33.28 FPS | −12.53% |
| Four / 1 | 17.82 FPS | 19.10 FPS | +7.15% |
| Four / 2 | 16.77 FPS | 18.40 FPS | +9.69% |

Three runs in pair 3 were correctly rejected because presentation changed
between 1280×720 and 2560×1378 during sampling. The remaining unpaired one-player
optimized result, 52.15 FPS, is not included in paired gains. All admitted runs
missed their respective 8.333/16.667 ms deadline in 180/180 sampled frames.

Host load changed substantially across the cohort. The one-player effect is
inconclusive; two four-player pairs are encouraging but insufficient for a
stable pre-change speedup claim. The scalar control also passes through the
new vector adapter. Its isolated calibration measured 15.08 ns versus 12.90 ns
for the direct original scalar loop, so it is not a zero-overhead pre-change
executable. The candidate itself measured 8.33 ns versus 22.44 ns for the direct
scalar loop in its separate component run. Clock/frequency variation between
those runs prevents combining their absolute costs into an FPS correction.

[Complete cohort and failures](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/capture-and-benchmarks/cohort.json)
and [component-control calibration](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/component-control-calibration.json)
retain exact commands, controls, frame-time reports and limitations. A runner
correction now retains bounded timing tables even when dimension rejection
prevents summary creation; it changes no engine behavior.
After that helper-only refresh, all seven focused CTests passed again and
the runtime binary hash remained unchanged.
The two rejected four-player raw timing tables were removed with the original
scratch before that correction. Their dimension-rejection diagnostics remain;
the rejected one-player timing table was recovered before cleanup.

## Image and next-change gates

The initial eight capture attempts failed because this host has no system
Khronos validation layer. They remain failed evidence. A capture-only retry
uses the existing repository layer, with manifest/library hashes recorded in
[the environment receipt](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/capture-validation-environment.json).
Validation remains enabled. All four comparisons, neutral and sustained fire
with one and four players, have identical complete 3840×2160 RGB output and
zero reported validation errors. All eight native capture executions exited
successfully. Capture-only acceptance does not pass the separate FPS gate.
The actual neutral image was visually inspected and contains the rendered
Coagulation scene, weapon and HUD; it is not a blank-frame comparison.

[Capture cohort](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/validation-captures/cohort.json)
is the authoritative status. Equality between two native controls does not
establish complete OG Xbox framebuffer parity.

The next bounded investigation is CPU motion-sensor rasterization: invariant
LOD/mip selection repeats per pixel, and output byte access repeatedly enters
the copy-on-write adapter despite private unshared output. A source-pinned
RGBA/metadata/fingerprint/FP differential and component measurement must
justify a patch before an ordinary-runtime comparison. The alias qualifier
and exception-wrapper hypotheses were rejected by
[their separate experiment](MCC_PERFORMANCE_PRIVATE_STATE_ALIAS_20261022.md).

## Motion-sensor integration checkpoint

That investigation subsequently justified a narrow change in
`src/ui/hud/motion_sensor.cpp`: prepare constant mip selection once for each
nonempty clipped blip and for the sweep/mask, then borrow one output writer
while the new texture remains private and unshared. Coverage, FP expressions,
sampling, blending, quantization and output exposure remain covered by direct
differentials. No new scalar-reference wrapper was introduced for this change.

The new runtime is
`6c31c802d97015086e9884bebd6d8e5717c433d288897650030a78907e34f312`.
Only that engine file changed relative to the qualified runtime above. Its
original binary is temporarily preserved inside the same test owner as the
required A/B reference (212,380,176 bytes); both use the optimized vector path.

The corrected unchanged-component speed gate fails at 0.970x; the candidate
passes at 1.414 ms → 1.106 ms, or 1.279x (about 21.8% less CPU time), for the
128-pixel raster. The persistent direct regression passes 339 complete
RGBA/metadata/fingerprint/FP/errno comparisons and 2,712 copy-isolation checks.
Ten focused CTests pass, including actual-artwork HUD and original viewport
checks. The separate `original_motion_sensor` input fixture is unavailable and
was skipped; it is not a passed test. Two existing independent original-xemu
radar captures, pulse and contacts, separately match with zero differing
channels. Their inputs were copied into owned scratch, authenticated and
verified unchanged; that completed scratch was removed.

[Source refresh](../build/test-results/mcc-performance-goal-20261022/motion-composite/candidate-refresh.json),
[build/tests](../build/test-results/mcc-performance-goal-20261022/motion-composite/candidate-validation/report.json),
[red component](../build/test-results/mcc-performance-goal-20261022/motion-composite/red-component-gate.json),
[green component](../build/test-results/mcc-performance-goal-20261022/motion-composite/green-component-gate.json)
and [independent original pixels](../build/test-results/mcc-performance-goal-20261022/motion-composite/original-pixel-captures.json)
retain the results. The first gate harness sampled transparent corner pixels;
its checksum rejection remains separately recorded and is not the red
performance proof. The corrected harness selects a nonzero reference pixel
before timing.

The longer [two-executable runtime cohort](../build/test-results/mcc-performance-goal-20261022/motion-composite/runtime-cohort/cohort.json)
completed all twelve FPS runs at 600 frames/90 warmup, with 510 admitted samples
per run. All four complete 4K neutral/fire comparisons at one/four players are
byte-identical and report zero Vulkan validation errors. Both executable hashes
remain unchanged. The timing comparison does **not** establish a whole-frame
gain:

| Players / pair | Pre-motion reference | Candidate | FPS ratio change |
| --- | ---: | ---: | ---: |
| One / 1 | 59.17 FPS | 69.86 FPS | +18.06% |
| One / 2 | 41.34 FPS | 55.62 FPS | +34.55% |
| One / 3 | 60.64 FPS | 48.28 FPS | −20.38% |
| Four / 1 | 12.02 FPS | 5.65 FPS | −52.95% |
| Four / 2 | 7.16 FPS | 7.13 FPS | −0.45% |
| Four / 3 | 6.10 FPS | 6.39 FPS | +4.79% |

Host load rose from about 8 to 34 on 16 logical processors, and the unchanged
reference slowed sharply too. These pairs cannot distinguish the change's
whole-frame effect from contention. All 12 runs miss their respective deadline
in 510/510 sampled frames. The isolated 21.8% compositor CPU reduction and exact
image evidence stand; neither FPS target nor launcher delivery passes.

The subsequent [ASan/UBSan differential](../build/test-results/mcc-performance-goal-20261022/motion-composite/sanitizers/report.json)
passes all 339 cases. Its private build and the completed runtime cohort owner,
including raw captures/timing files/caches, were verified removed.

The next bounded experiment is [selective LTO](MCC_PERFORMANCE_SELECTIVE_LTO_20261022.md)
over the complete memory, live-world and specialized shadow translation units.
It uses private replacement objects and a separate runtime output inside the
same owned session, preserving the normal executable and archives. No compiler
gain is established until link closure, actual code generation, semantic and
runtime checks pass.

The full runtime build is still required for this active goal and remains
inside its finite owned session. Completed measurement, sanitizer and preview
owners have been removed. No generated game binary or framebuffer has been
retained as a delivery artifact.
Close the session and verify cleanup after the active
runtime investigation no longer needs it; the
[goal checklist](MCC_PERFORMANCE_GOAL_CHECKLIST.md) remains open.
