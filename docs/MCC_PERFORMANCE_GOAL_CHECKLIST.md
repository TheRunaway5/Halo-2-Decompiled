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

## Measurement loop

- [x] Freeze and hash a scoped current-source baseline, preserving unrelated
      changes and the launcher-selected runtime. Record source/build drift.
      The active third runtime session pins8,518 copied source inputs;
      existing game/reference/dependency inputs remain external links. Its
      manifest records four files changed since the second source snapshot.
      [Session](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/session/session.json)
      and [source receipt](../build/test-results/mcc-performance-goal-20261022/runtime-attempt3/freeze.json).
- [ ] Build the required current-source diagnostic/runtime targets through
      `tools/run_tests.py` within an owned disposable context.
- [ ] Pin logical internal scene resolution independently of window size and
      assert actual render/presentation dimensions in every report.
      The CLI/benchmark implementation and three runner-contract tests pass;
      actual native4K admission remains pending the current build.
- [ ] Profile the current baseline at one, two and four local players, separating
      source simulation, per-view preparation, renderer CPU work and waits.
- [ ] Record host contention and collect repeated unsampled comparisons with
      identical runtime flags, assets, map/camera and player count.
- [ ] Keep instrumentation overhead out of FPS acceptance; retain all samples,
      failures and counterfactual controls.
- [ ] Collect GPU timestamp/API evidence when the CPU improvements expose a GPU
      bottleneck; host renderer intervals alone are not GPU execution time.

## Implementation, ordered by measured cost

- [x] First component: resolve retained vector operands once instead of four
      scalar resolutions; preserve ordered device reads, faults and wraparound.
      12,536 comparisons pass. The pre-change component gate fails; the first
      candidate improves median CPU cost from21.52 ns to7.75 ns (2.78x).
      [Source-pinned comparison](../build/test-results/mcc-performance-goal-20261022/vector-read/session.json).
- [ ] Admit that vector-read change in a fully rebuilt ordinary runtime and
      measure its integrated frame-time effect with the same-executable
      `HALO2_REFERENCE_SCALAR_VECTOR_READS` control.
- [ ] Optimize hot translated memory/instruction paths while preserving complete
      original results, exceptions, source callbacks and observer effects.
- [ ] Measure actual integrated improvement and reject regressions or gains
      explained by changing host load.
- [ ] Reduce verified repeated model/shadow/visibility preparation across
      compatible calls; retain camera-specific selection and mutable source
      owner boundaries.
- [ ] Reuse immutable hierarchy/section/palette inputs where profiling and full
      cache identities justify it.
- [ ] Expand resident raw GPU geometry/skinning where it provides a measured
      benefit and complete declaration/program/pass admission is available.
- [ ] Apply resource/state caching, changed-range uploads, buffer rotation,
      compatible instancing or parallel read-only preparation as remaining
      profiles justify them. Do not claim MCC's static code presence proves
      its live scheduling or an available percentage gain.

## Correctness and runtime validation

- [ ] Red/green differential at the actual changed boundary: registers, flags,
      complete memory/vertex outputs, fault behavior and ordered effects.
- [ ] Relevant original-owner, lifecycle, source-memory and concurrency checks.
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

All CMake validation uses `tools/run_tests.py`. Other probes use
`tools/run_test.py` and `HALO2_TEST_SCRATCH`; small reports remain outside
scratch. See [test storage](TEST_STORAGE.md). Existing assets, frozen evidence,
installed runtimes and unrelated active work are preservation inputs.
