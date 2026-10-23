# Selected WORLD skin reuse trial

Status: **correctness gates passed; ordinary FPS targets failed; no consistently
positive whole-frame CPU gain established**.
This is part of the active [performance goal](MCC_PERFORMANCE_GOAL_CHECKLIST.md).
The [owner audit](MCC_PERFORMANCE_SELECTED_WORLD_SKIN_20261022.md) establishes
the identities and remaining proof boundaries.

## Change

The third-person selected-geometry helper previously rebuilt a complete skin
for every body and held-weapon request. It now borrows a full immutable skin
from the existing frame-local unit pose cache, then copies the collector's
sections in their original order, including repeated sections. Camera-specific
selection, materials, shields and source execution still run for each request.
The cache key includes full owner, LOD, model address, complete WORLD-node
bytes and floating-point controls. No completed pose or visibility decision is
invented by this helper.

A new strict cache entry point admits reuse only for the qualified masked x64
SSE path, when the current sticky flags already contain the recorded effects
and x87 controls/status and errno match. It never clears or replays exceptions.
Changed inputs, controls or diagnostics force a fresh skin. Other platforms
retain fresh computation at this boundary. This receipt is not a claim about
allocation failure, custom allocation hooks or every external-library effect.

## Component evidence

- The prerequisite probe reproduces why the existing cache predicate is
  insufficient at this new boundary: after clearing flags, fresh arithmetic
  sets SSE precision while an old cache hit does not. Vertex bytes alone agree.
- The direct selected-helper regression first fails because it still rebuilds
  all twelve full skins. After enabling the strict provider, it performs one
  full build and eleven reuses with exact section order and duplicate vertices.
- All 384 full-vertex/SSE/x87/errno cases pass across four rounding modes,
  DAZ/FTZ combinations, incoming sticky flags, denormals and signaling NaNs.
  These cases include 72 admitted hits and 312 forced builds. Owner salts,
  model/LOD/node identities, diagnostics and failed-fill recovery also pass.
- A separate synthetic 16,384-vertex, 24-request CPU benchmark measures median
  fresh time 65.045 ms versus cached time 7.101 ms: **9.160x for that component**.
  This deliberately repeated workload does not establish a game FPS gain.
  Rechecking the tightened receipt measures 70.298 ms versus 7.102 ms,
  **9.898x**, with the same complete-output checksum and all correctness gates.

[Red result](../build/test-results/mcc-performance-goal-20261022/selected-skin/component-red-result.json),
[green result](../build/test-results/mcc-performance-goal-20261022/selected-skin/component-green-result.json)
and [component timing](../build/test-results/mcc-performance-goal-20261022/selected-skin/component-speed-gate.json)
retain commands and actual outputs.

## Runtime gates

- [x] Verify compiled SSE arithmetic closure and effect-receipt ordering.
- [x] Pass the four-view live differential with complete selected body/weapon
      vertices, pose banks, identities, order and actual reuse counters.
- [x] Make the live regression fail when selected skins are rebuilt.
- [x] Pass focused ASan/UBSan with the actual helper and skin arithmetic.
- [x] Compare all four complete 4K neutral/fire images with the saved executable.
- [x] Measure repeated ordinary FPS separately from component/diagnostic timing.
- [ ] Expand the integration matrix to remaining vehicle, offhand, owner reuse,
      interpolation discontinuity and supported campaign cases.

The tightened four-view test passes all 32 ordered source-unit draws and 36
selected body/weapon submissions, including complete vertex and node bytes.
It reports 17 reuses in 32 selected requests. Its separate reference control
`HALO2_REFERENCE_REPEAT_SELECTED_SKINS=1` leaves existing base reuse enabled;
the same live test then fails specifically because no selected skin is reused.
[Live outputs](../build/test-results/mcc-performance-goal-20261022/selected-skin/integration-tightened-actual-output.log)
and [actual-work red gate](../build/test-results/mcc-performance-goal-20261022/selected-skin/integration-reference-red.json)
record these results. The observer covers bodies and primary held weapons;
offhand, shield framebuffer, vehicle and death/respawn acceptance remain open.

The isolated sanitizer project compiles the actual selected helper, pose-cache
headers, skin arithmetic, palette and transform files. ASan and UBSan pass,
with vptr instrumentation excluded because unrelated discarded world routines
retain unresolved RTTI metadata; the selected helper/skin closure has no virtual
dispatch. Its initial link failure is retained separately. This is focused
boundary coverage, not a sanitized playable game.
[Sanitizer scope and hashes](../build/test-results/mcc-performance-goal-20261022/selected-skin/sanitizers/source-receipt.json)
and [passing report](../build/test-results/mcc-performance-goal-20261022/selected-skin/sanitizers/report.json)
also establish removal of the completed private build.

The [tightened linked audit](../build/test-results/mcc-performance-goal-20261022/selected-skin/linked-x64-audit-tightened.json)
records exact function identities, arithmetic and external boundaries. The
receipt snapshots state after diagnostic environment reads and after retiring
the previous vectors. New/delete, diagnostics and exception paths remain
explicit limits; this does not prove every host allocator or fault scenario.

The required pre-change ordinary executable has SHA-256
`6c31c802d97015086e9884bebd6d8e5717c433d288897650030a78907e34f312`.
Its transient reference copy was staged in the active owned session at
`.local/test-scratch/halo2-test-0yux_h0i/selected-skin-reference/halo2_native`.
Both sides retain the earlier vector and radar changes; whole-frame comparisons
could therefore isolate this selected-skin extension. The reference copy and
raw frames have now been removed after their completed cohorts. The rebuilt
candidate and incremental build remain required inputs for the active goal
and must be removed when this owner closes. No launcher selection has changed.
The tightened candidate hash is
`861e1f997c615c67c85201ab1af89ef7ea73eb942a315f885812b9586fef4def`.

All four one-/four-player neutral/firing image pairs match every RGB byte at
3840×2160, with zero Vulkan validation errors. This is a differential between
two native builds, not physical-Xbox framebuffer parity. The
[capture cohort](../build/test-results/mcc-performance-goal-20261022/selected-skin/runtime-captures/cohort.json)
retains both binary hashes, full-image hashes, dimensions and commands. The
ordinary FPS cohort uses the same two binaries and does not enable
the image observer, validation or fixed-tick capture controls.

## Ordinary FPS result

All twelve runs complete with 3840×2160 internal rendering and 1280×720
presentation. Each samples 240 ordinary frame intervals after 60 warmup frames.
Execution order alternates by pair. All **2,880 sampled frames** miss the
applicable 120/60 FPS deadline; the performance goal remains unmet.

| Players / pair | Saved executable | Selected-skin candidate | Observed FPS change |
| --- | ---: | ---: | ---: |
| One / 1 | 53.16 | 34.87 | -34.4% |
| One / 2 | 31.45 | 39.07 | +24.2% |
| One / 3 | 23.12 | 25.47 | +10.2% |
| Four / 1 | 6.41 | 6.87 | +7.2% |
| Four / 2 | 5.98 | 7.48 | +25.1% |
| Four / 3 | 5.09 | 6.05 | +19.0% |

Every four-player pair improves, but the unchanged reference also varies
substantially over the cohort. Recorded host load at run starts ranges from
18.6 to 38.8 on this 16-logical-CPU host. Real-time tick/render ratios also
change as frame time changes. These observed ratios do not isolate CPU savings
or establish a stable single-player gain. The
[full cohort](../build/test-results/mcc-performance-goal-20261022/selected-skin/runtime-benchmarks/cohort.json)
and [pair summary](../build/test-results/mcc-performance-goal-20261022/selected-skin/ordinary-fps-summary.json)
retain every run, binary identity, dimensions, phase statistics, p99/maximum,
deadline misses and longest miss streak. No failed run is dropped.

The first equal-work CPU diagnostic is rejected: its optimized window stops
submitting while source ticks continue, producing repeated frame-zero rows.
Its owned process was stopped, and no CPU pair was admitted. The harness now
requires every fixed scene frame to submit, uses actual input facing the spawn
center to exercise overlapping requests, and runs on an isolated X11 surface.
This diagnostic surface is suitable for CPU work comparisons; it is not an
ordinary FPS or physical-display measurement. The
[rejection and scoped harness change](../build/test-results/mcc-performance-goal-20261022/selected-skin/cpu-surface-rejection-and-refresh.json)
preserve the failure; its exact surface/minimization cause was not distinguished
from other swapchain rejection paths. The ordinary native executable is unchanged.

The completed image/FPS owner and both sanitizer owners are removed. The
[storage/restoration receipt](../build/test-results/mcc-performance-goal-20261022/selected-skin/storage-and-restoration.json)
also verifies removal of 453,663,210 generated bytes, including superseded
reference binaries, raw assembly, private probe products, helper sources and
copied integration cache. Bounded helper texts/hashes remain for reproduction.
The normal candidate hash is unchanged after official worker validation. The
outer incremental owner subsequently closed; its journal, absent monitor and
absent directory now verify complete removal. The performance goal remains
active, and future executable validation needs a new declared source snapshot.

## Equal-work CPU result

The corrected diagnostic completes all six runs with every frame submitted.
It uses one executable with the isolated selected-skin reference switch,
fixed seeds, 60 actual preparation ticks facing the spawn center, one actual
source tick per frame and interpolation 0.5. Each run discards 30 of 120 frames.
The kept source-state signatures, tick/frame identities, dynamic geometry
counts and selected-request counts match in every pair. Each optimized run
reuses 378 of 1,080 selected requests; each reference run reuses zero.

| Pair | Fresh selected skins: main CPU/frame | Reuse: main CPU/frame | CPU speed ratio |
| --- | ---: | ---: | ---: |
| 1 | 119.814 ms | 117.032 ms | 1.024x |
| 2 | 117.167 ms | 114.648 ms | 1.022x |
| 3 | 116.136 ms | 118.891 ms | 0.977x |

The first two pairs save about 2% CPU and the third costs about 2% more. Linux
thread CPU excludes descheduling, but frequency, cache/SMT contention and driver
host work remain. This does **not** establish a consistently positive complete
frame CPU gain. The direct component gain and eliminated request count remain
valid; neither can be promoted to a 9.9x game speedup or target acceptance.
The [admitted CPU cohort](../build/test-results/mcc-performance-goal-20261022/selected-skin/fixed-work-cpu-retry/cohort.json)
retains the six commands, controls, binary hash, per-frame tables and identities.
Its `passed` field means the comparison was admitted, not that performance won.

The diagnostic-only harness rebuild preserves the ordinary native executable's
hash. Its copied cache and helpers are retired, with bounded sources retained
and [cleanup verified](../build/test-results/mcc-performance-goal-20261022/selected-skin/cpu-retry-cleanup.json).
No launcher runtime was promoted. Further broad timing repeats are deferred in
favor of the much larger authenticated translated CPU owners.
