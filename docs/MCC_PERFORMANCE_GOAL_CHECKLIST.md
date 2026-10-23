# MCC-class performance goal checklist

Status: **active; performance targets unmet**. Created 2026-10-22 at the user's
request. This checklist tracks implementation, measured gains and final
acceptance separately. Component results do not complete the goal.

## Goal and invariants

- [ ] Verify 120 FPS at actual 3840x2160 internal rendering with one local player.
- [ ] Verify 60 FPS at actual 3840x2160 internal rendering with four local players.
- [ ] Verify the qualified ordinary runtime through the installed launcher.
- [ ] Preserve OG Xbox TU1.5 gameplay, simulation timing and visual quality.
- [ ] Preserve authored visibility, LOD, lighting, effects, HUD, material order
      and floating-point behavior; retain required source ownership and effects.
- [ ] Finish with reproducible commands, hashes, frame-time reports and verified
      removal of task-owned scratch/build outputs.

## Evidence already established

- [x] Audit the owned MCC Classic DLL with exact version/hash/callsites.
      [Reference mechanisms](MCC_CLASSIC_PERFORMANCE_REFERENCE_20261022.md).
- [x] Identify translated execution and memory/observer overhead as the largest
      measured CPU opportunities in the selected runtime's retained profile.
      [Assessment and gain estimates](MCC_PERFORMANCE_OPPORTUNITIES_20261022.md).
- [x] Verify that the earlier skinning-only opportunity cannot close the full gap.
- [x] Run a fresh red-capable ordinary-runtime deadline check: all three cases
      miss the 120 FPS deadline in 120/120 sampled frames. Retain the actual
      compositor-clamped dimensions and host-load limitations rather than
      claiming a new 4K result.
      [Diagnostic evidence](../build/test-results/mcc-performance-audit-20261022/validation.json).
- [x] Verify cleanup for that completed read-only investigation; no large
      generated artifacts retained.
- [x] Audit remaining current-source CPU candidates and reject a new topology
      cache for the already cheap write lookup. A pure branch specialization
      saves about1 ns per isolated dispatch; fresh profiling must establish its
      gameplay importance before implementation.
      [Next CPU owner evidence](MCC_PERFORMANCE_NEXT_CPU_OWNER_20261022.md).
- [x] Authenticate the historically hot plane-volume MOPP query and both output
      streams against the original TU1.5 routines and their consumers. Existing
      native ray/integer visitors are different owners; the146-case shadow
      fixture does not cover this query.
      [Spatial owner and required proof](MCC_PERFORMANCE_SPATIAL_OWNER_20261022.md).

## Measurement loop

- [x] Freeze and hash a scoped current-source baseline, preserving unrelated
      changes and the launcher-selected runtime. Record source/build drift.
      The active third runtime session pins8,518 copied source inputs;
      existing game/reference/dependency inputs remain external links. Its
      manifest records four files changed since the second source snapshot.
      [Session](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/session/session.json)
      and [source receipt](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/freeze.json).
- [x] Build the required current-source diagnostic/runtime targets through
      `tools/run_tests.py` within an owned disposable context. The third
      snapshot built successfully and all seven focused CTests passed.
      Runtime SHA-256:
      `efe69297809e687bb928dba551934e5a4be2aaeffebfc14e24b759463df5bcee`.
      [Build/test report](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/validation/report.json).
- [x] Pin logical internal scene resolution independently of window size and
      assert actual render/presentation dimensions in every report.
      The CLI/benchmark implementation and three runner-contract tests pass;
      admitted native runs prove 3840×2160 internal rendering. Mid-run
      presentation changes were rejected and retained as failed evidence.
- [x] Profile the current baseline at one, two and four local players, separating
      source simulation, per-view preparation, renderer CPU work and waits.
- [x] Record host contention and collect repeated unsampled comparisons with
      identical runtime flags, assets, map/camera and player count.
- [x] Keep instrumentation overhead out of FPS acceptance; retain admitted
      frame-time tables, failure diagnostics and counterfactual controls.
      [Current runtime pass and limits](MCC_PERFORMANCE_RUNTIME_PASS_20261022.md).
- [ ] Collect GPU timestamp/API evidence when the CPU improvements expose a GPU
      bottleneck; host renderer intervals alone are not GPU execution time.

## Implementation, ordered by measured cost

- [x] First component: resolve retained vector operands once instead of four
      scalar resolutions; preserve ordered device reads, faults and wraparound.
      12,536 comparisons pass. The pre-change component gate fails; the first
      candidate improves median CPU cost from21.52 ns to7.75 ns (2.78x).
      [Source-pinned comparison](../build/test-results/mcc-performance-goal-20261022/vector-read/session.json).
- [x] Admit that vector-read change in a fully rebuilt ordinary runtime and
      measure its integrated frame-time effect with the same-executable
      `HALO2_REFERENCE_SCALAR_VECTOR_READS` control.
      Four complete 4K RGB comparisons (neutral/fire, one/four players) match
      with zero validation errors. Integrated speedup remains qualified by
      host variation, rejected dimension-changing runs and control overhead;
      this does not pass either final FPS target.
- [ ] Optimize hot translated memory/instruction paths while preserving complete
      original results, exceptions, source callbacks and observer effects.
      The private-state alias qualifier produced identical tested assembly in
      GCC and Clang. A wrapper retaining the original checked read and failure
      prefix added overhead without a demonstrated gain. Neither is admitted;
      A private three-unit LTO trial now tests caller/memory specialization;
      its normal runtime and archives remain preservation inputs.
      [Rejected alias and exception-edge hypotheses](MCC_PERFORMANCE_PRIVATE_STATE_ALIAS_20261022.md).
- [ ] Measure actual integrated improvement and reject regressions or gains
      explained by changing host load.
- [x] Second measured CPU component: prepare invariant motion-sensor sampling
      metadata once per covered blip/sweep/mask and borrow one private output
      writer. The corrected pre-change speed gate fails; the candidate passes
      at 1.414 ms → 1.106 ms (1.279x) for the live 128-pixel raster. All 339
      exact RGBA/metadata/FP/errno cases and 2,712 copy-isolation checks pass.
      Actual-artwork HUD/viewport regressions and two independent original-xemu
      radar pixel captures pass with zero differing channels. Ten CTests pass;
      one separate original motion-sensor input fixture is unavailable/skipped.
      [Component proof](../build/test-results/mcc-performance-goal-20261022/motion-composite/green-component-gate.json)
      and [original pixels](../build/test-results/mcc-performance-goal-20261022/motion-composite/original-pixel-captures.json).
- [x] Admit the motion-sensor change's full 4K neutral/fire image cohort and
      repeated ordinary-runtime FPS comparisons. Use the preserved qualified
      binary as the reference, with the same vector path in both executables.
      All four images match exactly, with zero validation errors; all twelve
      510-sample FPS runs complete. Rising host load makes their whole-frame
      effect inconclusive, and every sample misses its 120/60 FPS deadline.
      The 339-case ASan/UBSan differential also passes. This completes the
      measurement, not the FPS or delivery gates.
      Runtime hashes and the single changed engine file are recorded in
      [admission](../build/test-results/mcc-performance-goal-20261022/motion-composite/runtime-admission.json).
- [x] Admit or reject selective LTO using actual public-symbol/link-map closure,
      changed hot code generation, callback/fault/FP checks linked to the LTO
      objects, and ordinary-runtime evidence. Preserve all checks and the
      generic adapters; no project-wide IPO or source semantic shortcuts.
      [Bounded trial design](MCC_PERFORMANCE_SELECTIVE_LTO_20261022.md).
      [Execution and remaining gates](MCC_PERFORMANCE_SELECTIVE_LTO_TRIAL_20261022.md).
      The three-unit trial passes twelve semantic CTests and four exact 4K
      image pairs, but four of six fixed-work CPU pairs cost more CPU time.
      No consistently positive result was established. LTO is rejected for
      production; ordinary binaries/objects/archives are preserved and private
      trial artifacts are removed. This does not certify a whole-frame gain.
- [ ] Reduce verified repeated model/shadow/visibility preparation across
      compatible calls; retain camera-specific selection and mutable source
      owner boundaries.
      The [selected WORLD skin trial](MCC_PERFORMANCE_SELECTED_SKIN_TRIAL_20261022.md)
      passes its red/green component gate and 384 exact FP/vertex cases. Its
      synthetic repeated workload improves 9.160x. The four-view live differential
      passes 36 exact selected submissions with 17/32 selected requests reused;
      all four complete 4K neutral/fire image pairs match with zero Vulkan
      validation errors. All twelve ordinary FPS runs complete; every one of
      2,880 sampled frames misses its deadline. Four-player pairs improve
      7.2%/25.1%/19.0%, but reference drift and mixed single-player results leave
      isolated CPU attribution and stable target acceptance open.
      The corrected equal-work diagnostic matches all source/geometry
      signatures and eliminates 378/1,080 selected requests per run. Its CPU
      ratios are 1.024x/1.022x/0.977x, so a consistently positive whole-frame
      CPU gain has not been established. Completed trial copies, private
      assemblies/helpers and child owners are removed. The finite incremental
      session has subsequently closed and its entire owner is verified removed;
      the next executable experiment needs a new declared source snapshot.
- [ ] Reuse immutable hierarchy/section/palette inputs where profiling and full
      cache identities justify it.
- [ ] Expand resident raw GPU geometry/skinning where it provides a measured
      benefit and complete declaration/program/pass admission is available.
- [ ] Apply resource/state caching, changed-range uploads, buffer rotation,
      compatible instancing or parallel read-only preparation as remaining
      profiles justify them. Do not claim MCC's static code presence proves
      its live scheduling or an available percentage gain.

## CPU/GPU offload track

The user explicitly requested implementation as a goal. The dedicated
[presentation offload checklist](PRESENTATION_OFFLOAD_GOAL_CHECKLIST.md) is
authoritative for its worker, ordinary-character GPU and 1:1 acceptance gates.

- [x] Audit current source and the pinned MCC binary for actual offload seams.
      Resident raw model/BSP geometry and admitted GPU vertex processing
      already exist. Ordinary base-model CPU skinning and serial host
      preparation remain candidates. See the
      [offload assessment](MCC_PERFORMANCE_OFFLOAD_20261022.md).
- [ ] Prototype a worker on immutable completed-frame inputs for host
      palette/skinning or vertex packing. Preserve lifetime, math/FP controls,
      original result/effect ordering and the current presentation endpoint;
      join before consumption without introducing an extra frame of delay.
- [ ] Extend ordinary base-model GPU vertex processing through the existing raw
      geometry/palette path, family by family. Preserve declarations, weights,
      lighting/PRT, materials, shadows, shields and per-view LOD/section order;
      retain CPU nodes and required CPU geometry consumers.
- [ ] Measure per-thread command recording only after the preparation cost is
      isolated. Give workers independent command/descriptor pools and keep
      resource updates, dependency barriers and submission ownership explicit.
- [ ] Validate one/two/four-player outputs and gameplay/input/tick traces,
      synchronization/lifecycle failure paths, full GPU images and unsampled
      FPS. The source arenas, callbacks and source visibility/shadow writers
      cannot be parallelized merely by putting the current view loop in jobs.

## Correctness and runtime validation

- [ ] Red/green differential at the actual changed boundary: registers, flags,
      complete memory/vertex outputs, fault behavior and ordered effects.
- [ ] Relevant original-owner, lifecycle, source-memory and concurrency checks.
      Next material source candidate: obtain an actual before/after execution
      oracle for the authenticated `2E79A0`/`2E6290` plane-volume MOPP query
      and both ordered output streams. The 146-case shadow fixture does not
      cover this owner; no native replacement is admitted without that proof.
- [ ] Relevant sanitizer checks where ownership or memory accesses change.
- [ ] One/two/four-view integration, including different cameras, owner reuse,
      vehicle entry/exit, attachments and interpolation discontinuities.
- [ ] Actual GPU geometry/material/framebuffer validation for renderer changes.
- [ ] Audio-enabled ordinary gameplay smoke; no simulation or quality shortcuts.
- [ ] Representative action-heavy gameplay plus every registered multiplayer map
      measured or explicitly recorded as an unresolved acceptance case.
- [ ] Campaign performance checked on playable supported paths; campaign startup
      blockers stay visible rather than being passed by a component fixture.

## Final performance and delivery gates

- [ ] Repeated unsampled 4K one-player runs meet 120 FPS / 8.333 ms frame budget.
- [ ] Repeated unsampled 4K four-player runs meet 60 FPS / 16.667 ms frame budget.
- [ ] Reports include mean, median, p95, p99, maximum, deadline misses and longest
      miss streak, plus warmup/sample counts and actual scene dimensions.
- [ ] Verify stable pacing and playable behavior through the delivered runtime;
      distinguish render throughput from physical-display presentation timing.
- [ ] Qualify the runtime/resources with source and binary hashes before changing
      launcher selection; retain the previous qualified runtime for recovery.
- [ ] Verify both normal launcher selections and audio-enabled gameplay after
      delivery; retain any material platform/scene limitations explicitly.
- [ ] Update the goal/checklist to complete only after the final gates pass.

## Work log

- 2026-10-22: Goal created. Read-only MCC audit and prior selected-runtime profile
  are available; no performance implementation from this goal has been admitted.
  Current working-tree changes and source-layout drift must be preserved and
  authenticated before evaluating a new optimization.
- 2026-10-22: Implemented the first vector-read component and registered its
  regression. Three focused Release CTests and four ASan/UBSan CTests pass;
  complete-runtime acceptance remains separate. Added ordinary-runtime `--render-size` and
  benchmark dimension rejection, with three passing clamping/internal-size
  contract tests. Neither these fixture timings nor the vector microbenchmark
  establishes playable FPS. No launcher selection has changed.
- 2026-10-22: Full native build is active through the owned session recorded
  above, using official `tools/run_tests.py --build-worker` inside
  `tools/run_test.py`. An initial missing Rust snapshot input was corrected and
  recorded in `refresh-rust.json`; no production repair was required. The
  session removes its source/build when closed or cancelled. Its source/build
  must remain available while native validation and measurement need them;
  this active session is not a retained delivery artifact.
- 2026-10-22: Added a finite ordinary-runtime cohort for same-executable vector
  reference/optimized comparisons, full-resolution fixed-tick RGB comparisons,
  repeated unsampled deadline checks and separate CPU profiles. Five capture
  admission regressions pass, including early validation errors outside the
  retained log tail. These synthetic runner tests do not qualify gameplay,
  image parity or FPS. Full-resolution captures use isolated X11 only for
  quality evidence; performance runs keep ordinary visible presentation.
- 2026-10-22: An intentional build job-limit adjustment exposed an expired idle
  deadline in the first ad hoc coordinator. Closing it removed its partial
  source/build, losing that compilation work; the cleanup is verified. A
  second frozen source build now uses the existing repository command session,
  whose idle timer runs only while awaiting input, and six compiler jobs.
  The first attempt is interrupted evidence, not a production build failure
  or runtime acceptance.
- 2026-10-22: A turn interruption retired the second interactive owner before
  build completion. Its PID/tool handle and scratch are confirmed absent;
  its stale running report is not live-build evidence. The third owner uses
  the same repository session detached from a chat PTY, with a bounded command
  pipe and twenty-minute idle expiry. Build-gated profiles, full4K fixed-tick
  comparisons and alternating ordinary-runtime benchmarks are queued. No
  interrupted attempt has produced playable-runtime acceptance.
- 2026-10-22: The third snapshot's linked `data` directory was rejected by the
  authored release archive's ownership check. Copied its four definitions
  (15,418 bytes) into that same owned snapshot, authenticated their hashes and
  resumed the existing incremental build. Original data and completed objects
  were preserved. The prior failure is retained separately; measurements were
  skipped by their build-admission guard and requeued behind the resumed build.
  [Input repair receipt](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/refresh-authored-data.json).
- 2026-10-22: The same third snapshot completed the full native runtime build
  and seven focused CTests. The scoped production/helper/test files still
  match their frozen hashes. One/two/four-player CPU profiles started on this
  admitted binary; full internal4K captures and unsampled A/B pairs remain
  pending. Build admission alone does not establish performance or delivery.
- 2026-10-22: Current one/two/four-player CPU profiles completed. Internal4K
  unsampled pairs show inconclusive one-player behavior and two four-player
  improvements of7.15%/9.69% against the diagnostic scalar control. Three
  dimension-changing runs were rejected. Host-load variation and measured
  reference-wrapper overhead prevent a stable pre-change FPS claim; all
  admitted samples miss the120/60 FPS deadlines. The original capture attempts
  correctly failed for missing system validation; a retry selects the existing
  repository layer and preserves those failures. Full4K neutral/fire one-player
  and four-player images match exactly with zero reported validation errors.
  No launcher delivery has occurred.
- 2026-10-22: Integrated the measured motion-sensor component in one engine
  file. Its persistent differential compares the actual implementation with
  the frozen pre-change host rasterizer; the independent original-xemu pulse
  and contacts captures separately match at zero channel error. The first
  speed-gate harness attempt sampled transparent corner pixels and failed for
  its checksum, so that rejected attempt is retained. After selecting a real
  nonzero reference pixel before timing, the unchanged version fails the
  intended performance gate and the candidate passes. The native incremental
  build and ten CTests pass, with one unavailable separate input fixture
  explicitly skipped. A two-executable 4K image/FPS cohort is active; no
  whole-frame speedup or final target is inferred from the component result.

- 2026-10-22: The motion-sensor cohort completed all four exact 4K image pairs
  and twelve longer FPS runs. Images and Vulkan validation pass; contention
  rose from about 8 to 34 host load, preventing a stable whole-frame gain claim.
  Every sampled frame misses its target. The subsequent 339-case sanitizer
  check passes. Both completed owners are removed. A private three-unit LTO
  experiment reuses the active build with separate output and explicit
  normal-object/archive preservation checks; no LTO gain is established yet.

All CMake validation uses `tools/run_tests.py`. Other probes use
`tools/run_test.py` and `HALO2_TEST_SCRATCH`; small reports remain outside
scratch. See [test storage](TEST_STORAGE.md). Existing assets, frozen evidence,
installed runtimes and unrelated active work are preservation inputs.
