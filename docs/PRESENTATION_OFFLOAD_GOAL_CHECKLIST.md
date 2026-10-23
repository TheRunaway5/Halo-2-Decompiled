# Presentation worker and ordinary-character GPU skinning

Status: **active**. Explicitly requested on 2026-10-22. This is a required
implementation track within the [MCC performance goal](MCC_PERFORMANCE_GOAL_CHECKLIST.md).
Neither a worker component nor a GPU demo completes the goal.

## Required outcome

- [ ] Enable meaningful presentation work on a separate CPU thread in the
      ordinary runtime, with actual overlap and no extra frame of latency.
- [ ] Move ordinary character base skinning to the GPU, eliminating its CPU
      skin computation rather than adding duplicate GPU work.
- [ ] Preserve OG Xbox TU1.5 gameplay and visuals 1:1: input/tick/event/callback
      order, current completed/interpolated poses, attachments, visibility,
      per-view LOD/sections, lighting/material/PRT, shadows and shields.
- [ ] Qualify one/two/four-player and supported campaign character cohorts;
      record remaining ordinary character families and scenes explicitly.
- [ ] Measure ordinary runtime performance at 3840×2160 toward 120 FPS for one
      player and 60 FPS for four players; preserve the full performance goal.
- [ ] Qualify the delivered launcher runtime and verify cleanup of owned builds.

## CPU worker

- [ ] Implement a persistent worker with a bounded single-frame handoff, owned
      lifetime and deterministic join/error/teardown behavior.
- [ ] Hand off immutable presentation inputs after source writers finish;
      keep mutable source state, renderer API calls and material publication
      on their owners.
- [ ] First concrete seam: copy the immutable dynamic-vertex prefix after the
      prior GPU fence while the render owner prepares independent draw state.
      Keep the late editable UI tail on the render owner at its original place.
- [ ] Prove exact full bytes, a distinct executing thread, actual overlap,
      repeated frames, empty/tiny work, exceptions and shutdown with work pending.
- [ ] Compare main-thread FP/errno state and run memory/thread sanitizers where
      supported; keep a serial reference control and require worker execution
      counters in the native acceptance path.

## GPU character skinning

- [ ] Freeze a current-source ordinary CPU baseline before implementation.
- [ ] Preserve original vertex-to-binding identity through selected-section,
      shader filtering, material rebasing and frame upload grouping.
- [ ] Implement separately rounded descending-influence skin arithmetic on GPU
      with device/FP admission and complete output comparison to the CPU owner.
- [ ] Add storage-buffer use and explicit compute-write/vertex-read barriers;
      retain fence-qualified resource lifetime and resize/teardown behavior.
- [ ] Prevent frame deduplication from aliasing different poses that share
      local-space vertex bytes.
- [ ] Route ordinary character base submissions through the GPU path before
      unnecessary CPU skin construction. Preserve actual CPU consumers such as
      pStencil caster construction and read-only posed-geometry diagnostics.
- [ ] Cover both player species, authored influences/compounds, attachments,
      corpses, shields, camouflage, different cameras/LODs and interpolation
      discontinuities. Unsupported driver/family fallbacks remain explicit;
      they do not certify an unmigrated ordinary character cohort.
- [ ] Require GPU skin counters and a reference-only negative control that
      fails the actual-work gate; verify CPU ordinary-base skins are eliminated.

## Acceptance and delivery

- [ ] Relevant original-owner/palette/vertex-input regressions pass.
- [ ] CPU/GPU full vertex and geometry identities match for admitted inputs.
- [ ] Actual neutral/action full frames match the CPU reference with Vulkan
      validation enabled; no quality or simulation shortcuts.
- [ ] Gameplay/input/tick traces and source observer order remain identical.
- [ ] Repeated unsampled FPS and separate equal-work CPU/GPU diagnostics include
      dimensions, hashes, warmup, percentiles, misses and host contention.
- [ ] Latest source and launcher-selected runtime are qualified together.
- [ ] Completed private helpers/builds/raw captures are removed; bounded reports
      and explicit required reference inputs remain documented.

## Work log

- 2026-10-22: Goal created. Current-source audit identifies the immutable
  upload prefix and ordinary base-model shader selection as concrete seams.
  The prior finite test session is closed and removed. A new owned baseline
  must be frozen before changing renderer/runtime production inputs.
