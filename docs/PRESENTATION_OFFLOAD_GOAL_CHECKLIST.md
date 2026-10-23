# Presentation worker and ordinary-character GPU skinning

Status: **active**. Explicitly requested on 2026-10-22. This is a required
implementation track within the [MCC performance goal](MCC_PERFORMANCE_GOAL_CHECKLIST.md).
Neither a worker component nor a GPU demo completes the goal.

Current display direction (2026-10-23): the user canceled virtual display work.
Use the physical monitor, report its actual presented extent separately from
internal4K rendering, and do not create another virtual display. The current
outputs are DP-1 at2560x1440/179.981Hz and DP-3 at1920x1080/119.982Hz.
The current retained-input comparison uses fitted1280x720 windows with
3840x2160 internal rendering. This does not establish physical4K presentation
or scanout latency.

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

- [x] Implement a persistent worker with a bounded single-frame handoff, owned
      lifetime and deterministic join/error/teardown behavior.
- [x] Hand off immutable presentation inputs after source writers finish;
      keep mutable source state, renderer API calls and material publication
      on their owners.
- [x] First concrete seam: copy the immutable dynamic-vertex prefix after the
      prior GPU fence while the render owner prepares independent draw state.
      Keep the late editable UI tail on the render owner at its original place.
- [x] Prove exact full bytes, a distinct executing thread, actual overlap,
      repeated frames, empty/tiny work, exceptions and shutdown with work pending.
- [x] Compare main-thread FP/errno state and run memory/thread sanitizers where
      supported; keep a serial reference control and require worker execution
      counters in the native acceptance path.

## GPU character skinning

- [x] Freeze a current-source ordinary CPU baseline before implementation.
      Snapshot: 8,608 file hashes in `baseline-source-hashes.json`; native
      baseline `489903ce…` and both focused baseline regressions pass. Ordinary
      displayed FPS originally failed on the output-less display. Historical
      virtual4K A/B results remain recorded; that provider is now removed by
      user direction. Fresh physical-display measurements remain required.
- [x] Preserve original vertex-to-binding identity through selected-section,
      shader filtering, material rebasing and frame upload grouping.
- [x] Implement separately rounded descending-influence skin arithmetic on GPU
      with device/FP admission and complete output comparison to the CPU owner.
      Kernel/service, adaptive normal-lattice qualification and the first real
      ordinary-player readback/frame cohort pass on RTX5080. Remaining authored
      families/cohorts and default enablement are separate open gates below.
- [x] Add storage-buffer use and explicit compute-write/vertex-read barriers;
      retain fence-qualified resource lifetime and resize/teardown behavior.
- [x] Prevent frame deduplication from aliasing different poses that share
      local-space vertex bytes.
- [x] Implement typed GPU resource refusal and exact late CPU materialization
      after the upload worker joins, before unmap; keep malformed jobs/device
      loss hard. Component recovery, complete FP/errno ownership and sanitizer
      gates pass; forced full-frame controls remain a separate runtime gate.
- [x] Repair repeated record/range growth with contiguous mapping and bounded
      geometric allocation; the intended old growth regression fails and the
      new Release/ASan/LSan/UBSan checks pass.
- [x] Retain the coherent GPU input bank and exact CPU record shadow; update
      changing palettes, qualify descriptor reuse by allocation generation,
      preserve per-call admission/failures, and keep a same-binary streaming
      reference. Release and ASan/UBSan bounds proof passes57 real GPU jobs,
      934,515 vertex/sentinel comparisons and151 warm traffic checks. The
      separate Vulkan leak gate remains open.
- [ ] Qualify that retained-input candidate in the fresh full world-body
      frame/source-trace cohort and repeated ordinary FPS measurements; record
      real cache hits and operation counts without equating them with speedup.
      [Retained-input evidence](MCC_GPU_SKIN_RESIDENT_INPUT_20261023.md).
- [x] Fresh retained-input Spartan/Elite1p/4p neutral/fire pairs: exact complete
      720p presented frames and ordered traces at4K internal rendering,
      3,007,440 verified weighted GPU vertices and zero Vulkan errors.
- [ ] Qualify genuine2p weighted GPU work. All four current stationary2p cases
      performed none and correctly fail; full12 quality remains unaccepted.
- [x] Finish the retained-input investigation's evidence closure: exact source
      and reversible diffs archived; all owned source/build/reference binaries,
      sampler libraries and raw captures removed, with independent process and
      path verification. This completes cleanup of that bounded investigation,
      not the performance goal.
- [x] Qualify the current plan/fallback build in all normal 4K CPU/GPU and
      forced-fallback frame/source/RNG cohorts, then measure its actual cost.
      This candidate regresses ordinary FPS; GPU enablement remains open.
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
- 2026-10-23: Frozen the current CPU source before production worker changes.
  The direct regression first failed with "Presentation copy still executes on
  the render-owner thread". Release, ASan/UBSan and ThreadSanitizer now pass
  the persistent-worker regression: 64 complete byte comparisons, segmented
  subranges, excluded editable UI, distinct persistent executor, main-owner
  FP/errno, invalid ranges, outstanding-ticket rejection, move/unwind,
  asynchronous exception/recovery and deterministic in-flight shutdown.
  Reports: `build/test-results/presentation-offload-20261022/worker-{red,green-final,asan,tsan}`.
  Their private build directories were removed and verified absent.
- 2026-10-23: Connected the worker to the immutable upload prefix after the
  prior GPU fence and buffer sizing, overlapping draw-state preparation.
  UI arithmetic remains at its original late location. Mapped-storage teardown
  joins on every exit; Vulkan calls stay on the render owner. Default work
  threshold is 64 KiB; `HALO2_REFERENCE_SERIAL_PRESENTATION_COPY=1` selects
  the original serial path. Added real executor/bytes/duration/overlap/wait
  counters. **Renderer build, actual native overlap and visual/gameplay parity
  are not yet validated**; component passes do not close those gates.
- Historical baseline owner: `.local/test-scratch/halo2-test-3cp5_42c` was
  supervised by the finite `session2` test session and has since expired and
  been verified removed. Its bounded source/hash/result evidence remains;
  it is not an active or delivered runtime.
- 2026-10-23: Worker native build and three relevant regressions pass. All four
  3840×2160 Coagulation neutral/fire image pairs (one/four players) match every
  RGB byte against frozen baseline `489903ce…`; candidate `a34f1f6d…` also
  compares **completed mapped uploads** against complete original CPU staging.
  All42 sampled candidate frames execute on the separate thread and overlap
  draw-state preparation. Mean copy work is approximately0.110/0.123ms for
  firing/neutral1p and0.835/1.053ms for4p, fully overlapped in these captures;
  final join waits total15.7µs across42 frames. These are host copy measurements,
  not a whole-frame FPS gain. See `worker-native-copy-gate.json` and
  `worker-runtime-captures/cohort.json` in the presentation-offload report root.
- 2026-10-23: The fixed-work serial/worker comparison passes60 submitted ticks
  each for one/four views. Explicit sampled gameplay state signatures, tick
  numbers, body/skin counts and selected-skin counts match. The serial control
  fails the actual-worker gate, while the worker has real overlap. This does
  not establish full memory/event/physical input-latency parity.
- 2026-10-23: Added `model_skin.comp`, renderer-owned `ModelSkinGpu`, explicit
  per-vertex destinations/palette indices, alias/bounds checks, and compute-write
  to vertex-read synchronization. RTX5080 readback passes12,529 complete vertex
  comparisons over eight sizes and two palette owners,0–4 influences, signed
  zero, negative/interior-zero weights, minimum cancellation2^-120, maximum
  bounds, all DAZ/FTZ combinations and64 host FP control cases. SPIR-V validation
  and inspection prove the two float32 execution modes and all eight separate
  MUL/ADD instructions have NoContraction. A no-dispatch control fails the byte
  gate. **The service is not yet used by ordinary characters and removes no
  ordinary CPU skin calls yet.** Reports: `gpu-skin-effects`, `gpu-skin-source.json`,
  `gpu-compiler-tools.json`; numeric proof in `MCC_GPU_SKIN_FLOAT_CONTROLS_20261023.md`.
- 2026-10-23: GPU LeakSanitizer acceptance remains open: kernel/readback and
  Vulkan validation finish successfully, then leak checking reports3,856 bytes
  (3,584 with only the NVIDIA ICD and implicit layers disabled), including
  libdbus and unloaded/unknown modules. Do not label those sanitizer invocations
  clean or attribute every allocation without a proved owner.
  A separate ASan/UBSan bounds run with `ASAN_OPTIONS=detect_leaks=0` passes;
  that result explicitly excludes leak acceptance (`gpu-skin-asan-bounds`).
- 2026-10-23: Ordinary FPS trial rejected and stopped. Xwayland reports a0×0
  screen with no outputs; KScreen lists no outputs; approximately970ms acquire
  waits persist after raising only the owned benchmark window without changing
  keyboard focus. The first baseline run and partial candidate are diagnostic
  rejection evidence, not a performance comparison. See
  `ordinary-fps-presentation-rejection.json`. Other goal work remains actionable.
- 2026-10-23: The ordinary FPS helper now rejects this known output-less X11
  condition before launching the game. The actual current-display regression
  exits immediately with a bounded `headless-preflight-presentation-rejection.json`
  receipt. No presentation override is used to qualify playback FPS.
- 2026-10-23: Tightened each GPU record to carry its own palette extent, so a
  malformed bone cannot escape into an adjacent owner's otherwise valid palette.
  The fixed ten-word GPU ABI excludes that host-only validated extent. Live
  owner/section/material/group mappings and elimination of earlier CPU skins
  remain the next implementation work.

- 2026-10-23: Established an isolated KWin virtual output at3840x2160/239.888Hz.
  Protocol/stage probes located SDL window creation hanging while Nvidia's ICD
  connected to the idle isolated Xwayland socket. Native Wayland with DISPLAY
  unset removes that dependency. The actual RTX5080 4K FIFO clear-loop passes:
  470 frames/2.001s,234.95FPS after warmup,3.753ms median acquisition and zero
  Vulkan errors. This qualifies the virtual provider, not game FPS, gameplay
  parity or physical-display latency. `virtual-provider-admission.json` and
  `virtual-fifo-nvidia-4k/evidence/virtual-presentation.json` retain the evidence.
- 2026-10-23: Added original palette-only/prepared-pose APIs and a Vulkan-free
  integer-bit numeric/host admission boundary. Deferred cache mode retains the
  existing owner/model/LOD/WORLD and FP/errno reuse policy; normal CPU mode is
  separate. Original palette arithmetic stays on the CPU at removed kernel
  boundaries, including redundant default/selected paths.
- 2026-10-23: Added explicit expanded ordinals through section repeats and
  shader rejection. The renderer mapping registers source ranges, resolves
  actual grouped copies to distinct destinations, merges palettes by geometry
  identity and rejects omitted/aliased records. GPU-containing appends bypass
  local-byte deduplication. Compute storage use and dispatch/barriers now join
  the real frame fence and upload lifecycle. These integration edits are under
  validation in `gpu-integration-build`; they are not yet ordinary acceptance.
- 2026-10-23: Wired ordinary player-body/held-weapon source bases behind the
  temporary `HALO2_ENABLE_GPU_SKINNING` validation switch. CPU consumers retain
  real posed geometry: active shields (including dead-body shield cases),
  pStencil shadows, temporal upscaler motion inputs and posed-geometry observers
  or diagnostics force CPU routing. Attachments/IK continue using CPU nodes.
  Campaign characters, retained corpses/loose weapons, default enablement,
  complete runtime readback/frame/trace cohorts and launcher delivery remain
  required. The switch is an implementation stage, not a reduced goal.

- 2026-10-23: The first genuine-MCC admission test correctly failed:167Master
  Chief,299Elite and280SMG attribute components fell below the initial fixed
  minimum; Elite also had a2.43278768e-15 palette coefficient. Host admission
  passed. `gpu-real-rejection/tests.log` preserves exact bits and extents.
  Replaced that fixed lower rectangle with an integer exponent/lattice proof
  per actual binding, including a necessary separate one-influence DPH
  translation-cancellation guard. No values are snapped, normalized or omitted.
  `MCC_GPU_SKIN_ADAPTIVE_ADMISSION_20261023.md` records the proof/counterexample.
- 2026-10-23: All five focused integration regressions now pass, including
  actual MasterChief/Elite/SMG section repeats/material filtering and the
  repaired normal-lattice boundary. GPU readback covers12,529 complete vertex
  records, with three exact smallest-normal fixtures and rejected neighboring
  subnormal/cancellation inputs. See `gpu-adaptive-build`.
- 2026-10-23: First ordinary paired runtime cohort passes on the isolated
  Wayland provider: Spartan, one player, neutral,60actual source command ticks
  and60submitted frames per side. GPU side performs679,680weighted vertex
  skins, all679,680completed records match the original CPU bytes. The complete
  640x480RGB frame and301ordered explicit gameplay/presentation callback
  hashes match; CPU reference has zero GPU jobs and both sides have zero
  Vulkan messages. Evidence: `gpu-first-runtime/evidence/model-skin-runtime.json`.
  This is bounded one-case proof, not full roster/campaign/4K/latency/FPS or
  launcher acceptance. The12pair both-species/one-two-four-view/action cohort
  is active in `gpu-broad-runtime`; source/binary remain frozen during it.

- 2026-10-23: Both species now pass the one-player inspection-camera neutral
  and firing cohorts: four CPU/GPU pairs,2,424,240 weighted GPU vertex records,
  exact completed full frames and ordered traces, zero Vulkan messages.
  `gpu-broad-runtime/evidence/model-skin-runtime.json` records those four passes
  and the next two-player actual-work rejection. This does not qualify normal
  first-person rendering: local arms and weapons still used the CPU at that
  checkpoint.
- 2026-10-23: The two-player rejection performed zero GPU work because its
  cameras did not face the other player; source geometry contained local owners
  and loose weapons. Aim was computed before real source spawns were published.
  `gpu-split-diagnostic/evidence/model-skin-runtime.json` retains actual positions,
  camera directions, body counts and host-ray diagnostics. These observations
  do not prove source PVS admission or surviving pixels. Correcting real input
  from prepared positions is underway; zero dispatch is never accepted as a
  successful GPU cohort.
- 2026-10-23: Added ranged campaign object submissions and original selected
  section/instance pose caching. The genuine Cairo Station component passes
  four main units,71 byte/FP-effect cases,11 GPU cases,45 deferred packets and
  73,857 final grouped destinations. `gpu-campaign2-component` retains the small
  receipt and original CTest log. Current ranged renderer/header/app integration
  builds successfully in `gpu-ranged-build`; campaign framebuffer/runtime and
  all character-family acceptance remain open.
- 2026-10-23: First-person source route is being migrated at its original final
  kernel calls. Its later identity placement has observable signed-zero
  behavior, so the GPU ABI now carries an explicit post-placement mode: eleven
  GPU words plus a validated host-only palette extent. Unweighted vertices
  still receive placement. Exact shield selection keeps posed arms on CPU;
  callbacks that can alter controls between skin and placement retain CPU work.
  `MCC_GPU_SKIN_FIRST_PERSON_SEAM_20261023.md` documents the boundary. This new
  route is under component/runtime validation and is not default enablement.

- 2026-10-23: Post-placement shader compiles and passes SPIR-V validation and
  the explicit operation-graph gate:15 separate NoContraction MUL/ADD
  instructions, float32 RTE and signed-zero/Inf/NaN preservation. Shader SHA-256
  `a459ed832bda189410708883df218e2573ac2c670335dbe00f86bf8874a2aaec`.
  Kernel and frame-plan regressions now include mixed placement modes, all-zero
  influences, signed-zero witnesses, independent owner/mode identity, unchanged
  local input bytes, host controls and malformed-mode rejection. Their current
  execution and the ordinary FP cohort are pending in `gpu-fp-components` and
  `gpu-fp-runtime`; compiled graph admission alone is not runtime acceptance.

- 2026-10-23: All five post-placement component regressions pass:
  15,606 complete vertex comparisons,128 post-placement sticky/DAZ/FTZ cases,
  224 host-control cases and zero Vulkan errors (`gpu-fp-components`). The first
  normal first-person runtime pair then correctly FAILS the full-frame gate:
  Spartan one-player neutral submits60 frames per side, all620,640 weighted
  GPU/post-placement vertices match the CPU readback oracle, and241 ordered
  source callbacks match, but the final full PPM differs. `gpu-fp-runtime`
  preserves the failed receipt. Benchmark candidate construction rejects that
  failure; no FPS run or enablement follows it. An independent original full
  CPU skin/select/placement bank comparison and bounded pixel diagnostics are
  the next discriminator. Focused GPU bytes do not establish final-frame parity.

- 2026-10-23: ASan/UBSan post-ABI bounds regressions pass for the GPU service
  and frame mapping (15,606 complete vertex comparisons,zero Vulkan errors).
  `gpu-post-abi-asan-bounds/qualification.json` explicitly excludes leak
  acceptance because leak checking is disabled. Its private build was removed.
- 2026-10-23: The independent original full CPU FP skin/select/identity assembly
  matches the GPU-materialized source bank on all60 frames, but the original
  full-frame mismatch persists. Pixel diagnosis localizes it to31 pixels in a
  green world effect (x73..79,y208..212), not the hands/weapon. The required
  1,843,230-byte PPM pair and PNG previews are temporarily retained under the
  active owner for this investigation (`gpu-fp-reference-images.json`).
- 2026-10-23: A deliberately nonaccepting CPU/CPU control also differs in that
  effect:18 pixels (x74..80,y209..212), identical ordered traces, zero GPU work.
  The failed CPU/GPU logs have matching source primary/desktop seeds but
  different original source auxiliary RNG values and world particle counts.
  The finite runtime harness omitted `reference_auxiliary_seed`; matching that
  existing replay input before scenario factories, recording actual words and
  extending RNG traces is underway. No pixels are ignored and no tolerances
  are introduced. `gpu-fp-reference-control` remains failed/control-only;
  source auxiliary seed causality awaits the identical-input rerun.

- 2026-10-23: Matching the existing declared source auxiliary replay input
  resolves the first-person mismatch: source RNG readback is1 before scenario
  factories; both words are traced after every submitted frame. Spartan/Elite,
  neutral/fire, one/two views now pass all eight exact full-frame, original FP
  assembly, GPU readback and ordered source/RNG trace pairs. Four views are
  still running in `gpu-fp-pinned-runtime`; the complete cohort is not yet
  accepted. No production particle/RNG behavior changed. The original unpinned
  CPUCPU/CPUGPU rejection reports remain reproducible diagnostic evidence.

- 2026-10-23: Ten normal-FP pairs pass with12,660,120 complete GPU records.
  The next repeated device construction in the same process produces NVIDIA
  loader messages: `libnvidia-tls.so.615.78.08: cannot allocate memory in static
  TLS block`, then the NVIDIA ICD is ignored. The suite rejects that run,
  including the eleventh case, and the twelfth is not executed. Its warning is
  not suppressed or classified as a shader success. `gpu-fp-pinned-runtime`
  retains the ten accepted entries and failed loader evidence. A per-pair
  process coordinator is being validated so each pair starts its intended
  driver with the same native binary; remaining pair coverage stays open.
- 2026-10-23: The2,542,604 bytes of generated failed-pair PPM/PNG references
  were inspected and retired after the auxiliary-input diagnosis. Hashes,
  pixel metrics, CPUCPU rejection and removal receipt remain in
  `gpu-fp-reference-retirement.json`; no generated frame pair remains retained.
  The fixed CPU-cost profiler now declares the same finite source auxiliary
  replay input, traces actual RNG words and excludes its final capture from
  timing means; the ordinary profiling route remains unchanged.

- 2026-10-23: Both remaining Elite four-view pairs pass in fresh per-pair
  processes. `gpu-fp-qualified-coverage.json` aggregates all12 individually
  qualified normal-FP pairs from the same `ff99a908…` test binary:17,822,520
  complete GPU vertex records, exact full frames and expanded source/RNG
  traces, zero validation messages. The ten valid prefix entries remain tied
  to their original failed-suite report; only the two new strictly passing
  entries complete coverage. No failed loader case is admitted. The permanent
  coordinator passes failure/control/timeout/cleanup tests and removes each
  child's raw bank/frame/log directory immediately.
- 2026-10-23: Current native/profile executables build successfully against the
  qualified FP/renderer ABI (`gpu-fp-native-build`); source-profile refresh and
  binary hashes are retained separately. Focused3840x2160 Spartan1pneutral and
  Elite4pfiring frame/trace/readback pairs are active before pinning and actual
  performance measurement. GPU default enablement and launcher delivery remain
  open; no FPS gain is claimed from these quality checks.

- 2026-10-23: The first focused4K CPU reference was rejected because the
  presented framebuffer was smaller than3840x2160 even though the internal
  render target was4K. GPU comparison was not reached. The isolated fullscreen
  rule had remained disabled for the smaller fixtures; restoring that owned
  rule passes a mapped SDL3840x2160 window/drawable probe. The unchanged Vulkan
  binary is rerunning the strict4K PPM/trace/readback pairs. This display setup
  rejection is not an admitted4K comparison or an FPS result.
- 2026-10-23: Resource-shortage CPU materialization and recoverable GPU-prepare
  failure handling are being implemented/reviewed in canonical source while
  the frozen quality/performance candidate remains unchanged. These edits are
  uncompiled and do not yet close resize/quota/failure acceptance. Active main
  source/build/reference owner is about3.09GiB; isolated virtual provider is
  about11.1MiB. Both remain finite supervised active owners, not delivered or
  permanently retained artifacts.

- 2026-10-23: Both focused actual3840x2160 Vulkan comparisons now pass after
  the owned fullscreen correction: Spartan1pneutral and Elite4pfiring,60 real
  command ticks/submitted frames per side, complete25MB RGB-frame equality,
  exact ordered source/RNG traces and complete GPU vertex readbacks, zero
  validation messages. Reports: `gpu-fp-4k-{1p,4p}` and
  `gpu-fp-4k-execution.json`. The same qualified native/profile plus complete
  shader inventory is now pinned for separate fixed-work and three interleaved
  ordinary CPU/GPU FPS pairs per one/four views. No FPS gain is established yet.

- 2026-10-23: The equal-work diagnostic passes all60 declared command/frame,
  source RNG/state and actual GPU/worker gates for1p/4p. This first GPU version
  regresses49-frame mean iteration thread CPU:16.84→18.71ms(1p),
  55.86→69.74ms(4p). Geometry-plan work rises0.093→1.273ms and
  0.820→11.549ms respectively. It is not admitted as a performance gain.
  A concrete repeated-allocation cause is under repair: exact reserve on every
  grouped append copies all preceding GPU records; per-vertex hash nodes add
  further packing work. Canonical interval/run planning with geometric growth
  is under review and is uncompiled. The frozen benchmark candidate is intact.
- 2026-10-23: The first ordinary FPS CPU run is rejected for actual overlapping
  Cargo/Rust compilation in another owned checkout. A gated FPS-only retry
  preserves that rejected receipt and waits finitely for another unrelated
  native/C++ build to finish. No FPS comparison or target acceptance follows
  the compiler-confounded sample. Temporary pinned references are448,399,472
  bytes(99 files) under the active virtual owner until this measurement ends.

- 2026-10-23: The old-source growth regression now fails for the intended
  allocation/copy symptom (`gpu-plan-growth-red`), with exact old header and new
  test hashes. The replacement keeps all source/owner/ordinal/local-byte/alias
  gates and destination order, stores contiguous runs/intervals and grows
  records/ranges/source/palette vectors geometrically. Empty CPU-only banks
  keep required late-registration/address checks and avoid needless map clones.
  The new source still requires its green build, runtime and performance proof;
  the measured11.55ms plan cost is not wholly attributed to one operation.
- 2026-10-23: The finite old-candidate FPS retry expires on the concrete
  compiler-idle prerequisite before launching a new case. This leaves no valid
  FPS pair. The old version is rejected from its equal-work CPU regression.
  Its pins/raw outputs/caches are retired:464,074,133 logical bytes and
  464,338,944 allocated bytes removed; named paths and capture directories are
  absent. Bounded fixed-work and rejection receipts remain. New candidate
  validation will use the optimized plan and explicitly separate resource
  fallback controls from actual GPU acceptance.

- 2026-10-23: The optimized-plan/resource-fallback native, profiler and runtime
  harness build successfully and all6 focused CTests pass. New native
  `c19d31d6…`, profiler`d1639f20…`, harness`a15206a6…`; shader remains
  `a459ed83…`. GPU service passes7 typed refusal controls,896 full-FP/errno
  cases,63 malformed-job precedence cases,2 hard negatives,10 actual recovered
  GPU jobs and9 empty resets, alongside15,606 complete vertex checks and zero
  Vulkan errors. CPU materialization passes4096 SSE and774 independent x87
  ownership cases, full bytes/no-partial-writes/no-heap positive paths.
- 2026-10-23: Current CPU plan/materializer pass default ASan/LSan plus UBSan;
  GPU service passes ASan/UBSan bounds with leak detection explicitly disabled.
  Both private sanitizer roots are verified removed and frozen hashes remain
  unchanged. Eleven completed private helpers(74,385 source bytes) are retired
  with compact source/hash receipts. The active new runtime cohort runs all12
  normal-FP pairs at actual4K, then four separately labelled forced CPU
  materialization pairs including WORLD mode0 and FP mode1. These pairs are
  still unaccepted; the queued benchmark requires strict terminal proof first.

- 2026-10-23: All12 normal first-person CPU/GPU pairs now pass on the CURRENT
  optimized-plan/resource-fallback build at actual3840x2160: both species,
  one/two/four views and neutral/fire.17,822,520 complete weighted GPU vertex
  records match, every complete RGB frame and ordered source/RNG trace match,
  zero Vulkan messages and zero late CPU fallback on ordinary routes. All
  per-pair raw directories are removed. `gpu-resource-normal-4k` retains the
  single strictly passing fresh-process suite. The forced resource controls
  are separately classified and remain pending as a set; no FPS gain or
  campaign/launcher/full-game acceptance follows from this normal cohort.
- 2026-10-23: All four separately classified resource-refusal runtime controls
  pass exact full 4K frames and ordered source/RNG traces: quota (1p FP), device
  allocation (2p FP), mapping range (4p FP), and host mapping (1p WORLD).
  Each performs actual weighted CPU materialization and records its exact
  refusal reason, with zero GPU dispatch/readback and zero Vulkan messages.
  They remain control-only and cannot satisfy ordinary GPU acceptance.
  `gpu-resource-quality-execution.json` records the complete normal/control set.
- 2026-10-23: Three clean interleaved ordinary 4K CPU/GPU pairs per view count
  complete with matching hashes, actual RTX5080 work, separate-thread uploads,
  no verification readback, zero fallback and no compiler interference. Median
  1p FPS regresses 71.109→65.319 (−8.14%); 4p regresses 19.287→17.828 (−7.56%).
  The 120/60 targets remain unmet. The separate equal-work diagnostic adds
  0.902ms/4.052ms of iteration thread CPU for 1p/4p; phase totals do not identify
  the leaf cause. GPU skinning stays opt-in while CPU leaf profiling and an
  exactly equivalent palette-admission optimization are investigated.
  `virtual-resource-runtime/compact-performance-summary.json` retains the
  bounded comparison. Temporary pins/raw outputs are removed (468,749,981
  logical bytes); the active source/build/reference and provider owners remain
  required for the next declared investigation.
- 2026-10-23: The palette-row summary optimization passes310,308 exact
  old/new predicate cases, including complete FP/errno ownership and no heap
  allocation. Six genuine MCC body/FP models across192 authored poses compare
  1,170,720 complete source records, with no admission drift. The standalone
  predicate also passes ASan/default LSan/UBSan; its private build is removed.
  This removes repeated row scans without widening the arithmetic domain;
  current native quality and performance remain unmeasured for this version.
- 2026-10-23: The scoped admission/campaign-capture build passes all11 targets
  and10 focused CTests, with16 affected object rebuilds authenticated. Native
  `8df3d3a8…`, profiler`2afbb2f2…`, runtime harness`a76ca71f…`; baseline and
  complete shader inventory are unchanged. Two rejected build attempts retain
  their diagnostics; repairs distinguish source tokens from native addresses
  in diagnostic receipts and add the capture test's SDL dependency. The new
  finite campaign capture still requires actual advancing-clock actor GPU
  frames; its focused checks do not establish campaign runtime acceptance.
- 2026-10-23: Old-candidate1p CPU/GPU CPU-clock profiles complete with matching
  fixed source traces. Hot PCs establish repeated integer admission in both
  model-pose production and frame-plan validation. The sampled4p600 control
  crashes and remains rejected; an uninstrumented600-frame control completes
  all declared commands/state/RNG/4K/worker gates, with compiler contention
  explicitly recorded and no timing acceptance. Sampler interference is a
  possible cause, not an established diagnosis. A shorter complete4p profile
  pair is being qualified before current-candidate quality/performance runs.
- 2026-10-23: The admission/capture candidate subsequently passes four focused
  actual4K normal pairs (Spartan1p and Elite4p, neutral/fire): complete frames,
  ordered source traces and all weighted GPU readbacks match, with no late CPU
  fallback or Vulkan messages. The shorter4p sampled pair completes; sampling
  estimates remain separate from ordinary FPS. No new admission-candidate FPS
  result was obtained before the finite owners expired.
- 2026-10-23: Authored Cairo Johnson capture advances60 real source ticks and
  verifies379,620 weighted GPU vertices, exact complete RGB output and ordered
  source/actor/node traces. Acceptance correctly fails because presentation is
  3824x2122 although internal rendering is3840x2160. The natural opening has no
  ordinary actors and cannot qualify GPU character work. A false-GPU control
  also fails its extent gate. The campaign caption changes before mapping;
  a fresh isolated display now admits a narrow rule covering that caption,
  with an unrelated-title negative control. Campaign reruns remain required.
- 2026-10-23: Both previous finite source/build and display owners have expired
  and are verified absent. Their compact reports and authenticated reference
  sources remain; their binaries and active-path claims are retired. None of
  the installed launcher runtimes matches the current GPU source. A fresh
  finite-owned current-source build is authorized, with unrelated source drift
  declared, followed by fresh parity and performance measurements. The new
  validation-enabled native-Wayland provider presents3840x2160; its clearloop
  proves provider availability only. GPU default and launcher delivery remain
  open, as do4K120/60, remaining families and full gameplay/visual acceptance.
- 2026-10-23: Fresh whole-source candidate `c50e8f5a…` now builds all12
  declared targets and passes11 focused CTests, including the presentation
  worker. Profiler`3fecf520…`, runtime harness`cea11858…`,97 shader files and
  16,063 private source files are pinned. Only unrelated network/prospective
  role edits drifted in canonical source after freezing; the candidate remains
  fixed. Fresh standalone predicate sanitizers, full12 normal4K pairs and the
  authored campaign/false-GPU reruns are separate pending gates.
- 2026-10-23: A subsequent service-only row-summary trial is prepared outside
  canonical and the baseline. Identical new tests pass against the original
  and proposed service:37,440 validation cases (27,456 hard rejections,9,984
  quota refusals), complete FP/errno preservation, all3 real GPU records at
  rows4094/4095/4096,15,606 original full-vertex comparisons and zero Vulkan
  errors. A test-only unsigned declaration repair retains its initial build
  rejection separately. Release owners are removed. This is component proof;
  bounds sanitizers and a measured runtime improvement remain unaccepted.
- 2026-10-23: The initial expanded row-cache sanitizer test fails its complete
  FP/errno invariant without an ASan/UBSan bounds diagnostic. An instrumented
  old/new diagnostic variant subsequently passes both37,440-case matrices;
  this does not explain the initial failure. The next production patch remains
  unlanded while the instrumentation boundary is investigated. All completed
  Release/sanitizer/probe scratch is removed; compact rejection evidence stays.
- 2026-10-23: The user turned on the physical1440p180Hz monitor, then canceled
  virtual display work. The owned virtual KWin/session/DBus/runtime/config,
  sockets, FIFO and scratch are removed;303,665bytes of compact reports and
  small authenticated source references remain, with no binaries/raw captures.
  The fresh source/runtime owner remains required. Physical1440 client sizing
  clamps to2544x1362 due to the desktop workarea, so campaign capture proceeds
  in a fitted1080p window. The full12 MP4K attempt was rejected because its
  final images followed physical presentation dimensions. The1080p retry was
  canceled after two pairs; fresh full12 parity remains open. Future FPS
  measurements distinguish internal4K from their actual presented dimensions.

- 2026-10-23: User prioritized performance measurement immediately and the
  owned full12 quality run was canceled; two completed1080p pairs do not
  qualify the full set. Twelve ordinary FPS cases (three interleaved CPU/GPU
  pairs each1p/4p) complete on the physical monitor, with internal3840x2160
  rendering and actual1920x1080 presentation. Median1p CPU/GPU: 65.552/60.834FPS (-7.20%). Median4p: 18.846/17.187FPS (-8.81%).
  Unrelated repository tests overlap EVERY case; this is a requested
  current-load snapshot, not clean optimization acceptance. All sampled frames
  execute the real presentation worker; GPU-on performs weighted deformation
  with no verification readback or CPU fallback, CPU controls dispatch none.
  All raw/caches/pinned-reference storage is removed with its benchmark owner;
  compact JSON/TSVs remain underphysical-c50-immediate-performance. Targets
  remain unmet, GPU stays opt-in, launcher/default/full-quality gates stayopen.

- 2026-10-23: Applied the authenticated two-file referenced-row summary trial.
  Official owned Release rebuild passed the GPU component:37,440 row cases,
  15,606 original/post-placement complete vertices and three boundary vertices,
  zero Vulkan validation errors. Native hash `ad4021caf17048eb8c2c31cc932e88dec146e730ae1d72dbe0232e3f0c665072`.
  Three complete single-player GPU-on old/new pairs measured median61.646 to
  62.175FPS (+0.857%), with no repository build interference detected. This
  small partial result does not establish a stable gain. The retained parent
  build directory and all three known processes disappeared during4p sampling;
  its stale journal is not liveness evidence. The incomplete4p cohort and
  missing final input closure are rejected. Both benchmark scratch owners and
  the parent source/build/reference owner are verified absent; no large trial
  artifact remains. Cause of parent removal is unproved.
  [Build, partial measurements and terminal observation](../build/test-results/presentation-offload-20261022/row-cache-physical-trial/).

- 2026-10-23: User requested direct MCC architectural evidence. The new audit
  links the named render queue, worker-indexed contexts, job recording and
  completion/submission owners; live activation and speedup remain separate.
  [Architecture evidence and transfer gates](MCC_CLASSIC_ARCHITECTURE_BORROW_AUDIT_20261023.md).
  Implementing native hot owners, resident ordinary-model inputs/palette
  updates, broader immutable packing jobs and command-state suppression remains
  open. Existing BSP residency, pose reuse and pipeline/material caches must
  not be counted again. Preserve ordered source/FP owners, per-view metadata,
  current presentation endpoint and original failure prefixes. Any next runtime
  trial requires a new owned frozen build; GPU remains opt-in and the goal active.

- 2026-10-23: Implemented retained coherent skin-input storage, an exact48-byte
  CPU record shadow, palette updates and descriptor reuse qualified by an
  explicit destination-allocation generation. All existing validation remains
  per call; zero generation conservatively refreshes descriptors. Release and
  ASan/UBSan bounds cohorts pass934,515 complete vertices across57 real GPU jobs,
  151 warm traffic checks,212 full FP/errno checks, refusal/growth/empty/stale
  controls and zero Vulkan errors, in addition to the existing15,606 vertices
  and37,440 row cases. The old streaming policy passes correctness and fails
  the new traffic gate; disabling dispatch fails actual output bytes. Leak
  checking was explicitly disabled for the Vulkan bounds run; its separate
  unresolved leak gate remains open. All completed component scratch is removed.
  [Component receipts](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/component-attempt2/execution.json).
  Fresh owned baseline/candidate rebuilds and ordinary-runtime measurements
  remain underway; no full-frame, FPS, default-GPU, latency or launcher gate is
  accepted by these component results. The goal remains active.

- 2026-10-23: Fresh baseline and retained-input candidate each build12 targets
  and pass11 focused checks; only the declared six production/test files differ
  and97 shaders are unchanged. The18 ordinary measurements at4K internal /
  720p presented complete, with median CPU / streamed GPU / retained GPU FPS
  of64.092 /58.840 /56.796 (1p) and18.248 /17.354 /16.555 (4p). Every case
  overlaps unrelated repository tests: this is a current-load result, with no
  clean regression or stable gain accepted. Retained record uploads fall91.85%
  for1p and100% for4p, with no sampled map/unmap calls; those operation savings
  do not qualify FPS. All3,780 submitted frames run the real copy worker and
  all2,160 sampled GPU frames have actual skin work, with zero readback/fallback.
  FPS raw/reference-copy storage is removed. A separate test-only harness
  refresh passes13 extent-contract tests, preserving all110 production
  libraries/objects, native executable and97 shaders. Fresh full12 frame/trace
  qualification is running; full4K presented pixels, latency, other families,
  default enablement, target FPS and launcher delivery remain open.
  [Candidate and measurement receipts](../build/test-results/presentation-offload-20261022/resident-skin-input-20261023/).
