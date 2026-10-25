# Native 4K60 split-screen goal

Status: **active**. Created 2026-10-24 at the user's request.

Objective: four-player split-screen at 3840×2160 internal (four 1920×1080
views) sustaining 60 FPS, with gameplay and presentation faithful to TU1.5.
The translated TU1.5 code is replaced owner by owner with native gameplay
replicas and a modern native rendering pipeline. Work is ordered by measured
cost: the most expensive owner goes first.

## Acceptance rules

- **Gameplay parity.** A native replica must leave simulation state identical
  to the translated owner it replaces. The translated owner stays in the build
  as the oracle and the reference route.
- **Presentation parity.** Captured frames must be byte-identical to the
  translated route at full 3840×2160 internal resolution, using the
  deterministic capture (scripted input, fixed tick cadence, render fraction 0,
  auxiliary seed 1). Run reference and candidate back to back with repeats;
  the Vulkan pipeline cache and window placement change results between
  sessions. Unset `WAYLAND_DISPLAY` and set `SDL_VIDEODRIVER=x11` under
  `xvfb-run`, then check the PPM header size.
- **Threading.** Parallel work must be deterministic and merged in original
  order. Overlapping a gameplay tick with presentation of the previous tick
  adds up to one frame of latency; it needs an explicit user decision before
  becoming a mode.
- **Measurement.** Report main-thread CPU ms per frame from interleaved A/B
  pairs, not wall-clock FPS alone; this host often runs other heavy work.

## Threading and native-kernel strategy

Translated TU1.5 code runs against one emulated memory bank, with
single-threaded semantics, write guards/observers and serialized public
calls. It cannot run on several threads. Parallel work therefore comes from
native code with explicit inputs and outputs, and translated owners are
replaced by native kernels first so their work can then be parallelized.

**Replacing a translated owner (the main-cull pattern).**
1. Read the translated owner's actual published inputs from emulated memory,
   in the representation it consumes; do not rebuild them from native state.
2. Oracle mode: run the native kernel and the translated owner on identical
   inputs and compare every output word, across all maps and player counts.
3. Hybrid default: the native kernel takes the subset it covers; the
   translated owner keeps the rest (for the cull, flag40 hides native units);
   merge outputs only where the merge is order-independent (bit ORs).
4. Keep `HALO2_REFERENCE_*` to restore the translated route, and accept only
   with byte-identical full-resolution frames.

**Exact parallel execution** (`native_parallel_for`, `include/halo2/native_parallel.hpp`).
Tasks run under the caller's SSE/x87 control words with clear sticky flags;
after the join the caller gains every task's sticky flags, errno takes the
last change in task order, and the first exception in task order is
rethrown. Nested calls run inline; `HALO2_REFERENCE_SERIAL_NATIVE=1` runs
everything inline. Tasks must not share mutable state; callers publish
results in the original order. `native_parallel_fork_join` covers results,
rounding control, SSE/x87 flag merges, errno, exception order and nesting;
it is clean under ThreadSanitizer (40 runs) and ASan/UBSan.

**Order of work.**
1. Native producers that are pure per item (skinning, palettes, particle
   preparation, renderer packing/planning per view) move onto workers now,
   publishing in original order.
2. Per-view translated preparation becomes native, owner by owner, in cost
   order (`40E80`, then `3D2B0`, `C35D0`). When a view's preparation reads only
   an immutable snapshot of gameplay state and owns its render-state pages,
   the four views run concurrently; this is the largest 4p gain.
3. The gameplay tick stays translated and serialized. Native replicas there
   need a per-tick state hash oracle. Running the tick on its own core needs
   a snapshot of the bank for presentation; write dispatch already sees every
   store, so dirty-page tracking is cheap. Overlapping a tick with the
   previous frame's presentation adds up to one frame of latency and needs an
   explicit user decision.

## Measured cost order (4p, 3840×2160, Coagulation)

Final build of 2026-10-24, mean ms per rendered frame:

| Rank | Owner | ms/frame | Notes |
|---:|---|---:|---|
| 1 | Per-view translated render preparation (`camera_lighting`) | 11.40 | Model collection 8.37: main cull `44710` 3.74, main commands `40E80` 1.97 |
| 2 | Native Vulkan renderer | 6.15 | Pack 3.71, plan 2.00, bind 1.71, global 1.47 |
| 3 | Per-view object preparation / CPU skinning | 5.45 | GPU skinning covers only player base meshes |
| 4 | Gameplay tick (`Match.step` + projection + post-step) | ~10.7 per tick | ~5.3 per 60 Hz frame at 30 Hz ticks; spikes on tick frames |
| 5 | First person, bodies, HUD, common | ~6 | |

At the 60 FPS target the per-frame render work dominates; the tick is second
because it runs at 30 Hz.

## Completed

- [x] **Checked translated-memory fast paths** (2026-10-24). Direct host page
      index for complete committed pages; inline `r()`/`v()` reads; writes skip
      repeated region checks but run every guard and observer in order.
      Per-thread dispatch slots replace the registry `shared_mutex`; the main
      dispatching thread publishes with a compiler fence and other-thread
      policy writers issue `membarrier`. One per-bank write serial replaces the
      two counting observers (identical values); the physical-page guard skips
      writes ending at or below `0x80000000` without a call.
      Result: main-thread CPU per frame −14% (interleaved, loaded host); clean
      host 21.2 → 24.5 FPS before the next two steps.
- [x] **GPU skinning is the default** (2026-10-24, user decision).
      `HALO2_REFERENCE_CPU_SKINNING=1` or `HALO2_ENABLE_GPU_SKINNING=0` keeps
      the CPU kernel. Currently ~5% *more* main-thread CPU than CPU skinning:
      only player base meshes migrate, and the renderer re-runs per-vertex
      arithmetic admission and an ordinal scan on every submitted geometry.
- [x] Parity: baseline (CPU skinning) versus final build produced identical
      full-resolution frames for 4p idle/firing, 1p firing and 2p firing.
      Memory, snapshot, execution-input, pool/arena continuation and skinning
      suites pass, including the vehicle 2p/4p and unit 4p world reuse checks.

- [x] **Native main cull** (2026-10-24). `SourceBspEffectLights` runs the
      existing native `44E80` corridor (`source_bsp_frustum_cull` /
      `source_bsp_accelerator_query0`) over the actual published TU1.5 draw
      units (`0x4C6958`), mask arena (`0x556E18+0x25C`) and associations
      (`+0xA60`), with the view's native portal visibility. Accelerated BSP
      units are culled natively and hidden from translated `44710` via flag40;
      every other unit stays translated; the native arena is OR-merged.
      `HALO2_REFERENCE_MAIN_CULL=1` keeps translated `44710` alone;
      `HALO2_MAIN_CULL_ORACLE=1` runs both on identical inputs and reports.
      Oracle: 10 maps × 720 4p views (Coagulation 1,200 views), zero unit or
      mask mismatches and zero fallbacks; Containment keeps 1,080 non-accelerated
      units translated. Full-resolution frames are byte-identical to the
      translated cull (4p idle/firing, 1p, 2p). Main cull 3.77 → 1.53 ms/frame.
- [x] Native accelerator validation reuses a per-thread visited bitmap
      instead of a `std::set` node per instruction (identical traversal,
      budget and exceptions). It had been ~75% of native cull time. Main cull
      now 0.53 ms/frame (−86% versus translated). Accelerator, range,
      draw-unit cull and menu range fixture suites pass.
- [x] End-to-end parity: the pre-session binary (CPU skinning, translated
      cull) and the current build give byte-identical 3840×2160 frames for
      4p idle/firing, 1p firing and 2p firing.

- [x] **First exact multithreading** (2026-10-24). `native_parallel_for`
      deforms each dynamic object's six LOD skins together
      (`SourceObjectGeometry::pose_world_nodes`), publishing in LOD order.
      Object preparation 5.57 → 5.20 ms/frame; main-thread CPU −1.9%. Frames
      byte-identical to `HALO2_REFERENCE_SERIAL_NATIVE=1` and to the
      pre-session binary.

- [x] **More exact worker use** (2026-10-24). Motion-sensor compositing
      (one 2048-pixel raster per view per frame) runs in 16 row bands after
      serial blip setup; scenery LOD poses (idle animation and WORLD routes)
      pose together and publish in LOD order. The `motion_sensor_composite`
      differential and the HUD sensor suites pass; frames byte-identical.
      Object preparation 5.6 → 4.5 ms/frame over the two LOD changes.
- [x] **Cheaper native producers.** `FrameModelSkinPlan` skips per-record
      arithmetic admission for GPU geometries whose producer already admitted
      the complete local bank with the same palette
      (`ModelSkinGeometry::arithmetic_admitted`, set only by
      `source_live_model_base_vertices_tu15`). Dynamic model shader selection
      returns a flat ordered part table and reuses the previous part's slot for
      consecutive vertices instead of a `std::map` lookup per vertex. Skin
      plan/GPU/admission/handoff, CPU-skin input, live-resource and dynamic
      shader integration suites pass.
- [x] Measured 4p/4K on a quiet host: 20.3 → 25.7 FPS over the day
      (main-thread CPU 49.2 → 39.0 ms/frame, interleaved pairs).

Pre-existing failures observed while testing (also fail with every change of
this session reverted): `source_scenery_gpu_lighting` and
`native_renderer_material_residency` need the Vulkan validation layer, which
is not installed on this host; `retail_source_scenery_shader`
has a hard-coded all-map census (`baked_vertices==792954` etc.) that no
longer matches the retail maps, last edited 2026-10-16.

- [x] **Renderer: banded skin-plan vertex scan** (2026-10-24).
      `FrameModelSkinPlan::add_bank` compares every submitted vertex against
      its admitted local copy; the comparison is pure, so 4096-vertex bands
      scan on workers and any mismatch raises the identical exception at the
      same point. Plan phase 1.84 → 1.63 ms. With every pool use disabled
      (`HALO2_REFERENCE_SERIAL_NATIVE=1`) versus enabled: 26.7 → 28.9 FPS,
      main-thread CPU −8.7%, frames byte-identical. `native_frame_model_skin_plan`
      gained a 10,000-vertex case rejecting a mismatch in the first, a middle
      and the partial last band, and admitting the unaltered bank, in both
      pool modes.
- [x] **Renderer: per-view material uniform upload on workers**
      (2026-10-24). Each view's material collection and `view_material_uniform`
      builds read only immutable per-frame tables and write that view's own
      stride of the mapped buffer and its own `view_material_initialized`;
      the upload observer path stays serial. Bind phase 1.63 → 1.34 ms;
      4p/4K 27.1 → 28.8 FPS against the serialized pool; frames
      byte-identical.
- [x] **Renderer: per-view BSP merge on workers** (2026-10-24).
      `SourceBspResidentPlan::merge_into` is const over the immutable atlas
      and writes one plan per view, so the four merges run together before
      the ordered plan loop; merge failures are rethrown at their view's
      position. Plan phase 1.85 → 1.72 ms; frames byte-identical. On a quiet
      host 4p/4K now 27.9 FPS; interleaved against the pre-session binary,
      main-thread CPU 45.7 → 37.2 ms/frame (−19%).

## Next, in cost order

1. **Native main commands (`40E80`) — done for BSP units; see below.** The earlier
   "multi-day, callee scratch" assessment was too pessimistic. A frame-scoped
   write observer (`HALO2_MAIN_COMMANDS_CENSUS=1`, `SourceBspEffectLights::
   Impl::main_commands_census`) recorded every source store of translated
   main `40E80` over 1,200 4p views on Coagulation and Headlong (12.7 M
   stores, 229 coalesced ranges). The complete source write set is:
   - five scalar words: `486040`, `4C2AA0`, `4CE26C..4CE278`, the special-unit
     bank `4D4F60..4D4FA0` and its count `515F1C`;
   - the three `3B9D0` command banks' storage: live TU1.5 headers are at
     **`4CE268/+18/+30`** (`source_bsp_part_commands.hpp`'s `4C1A48` is the
     TU1.1 address). `3B890..3B923` allocates them at map-world setup as
     `{capacity, count, layers, retained×2, data@+0x14}` with capacities
     2560/1024/512 records; live storage (read from the headers) is bank0
     `0x1018B000+0xF000`, bank1 `0x1019A000+0x6000`, bank2 `0x101A0000+0x3000`.
     Live headers read zero at `4C1A48`, nonzero here;
   - the **emulated stack** (`0x600Exxxx`): 84% of all stores are `44AB0`'s
     `ESP-0x40` frame and `45CD0`'s per-part context (`0x600EE230..34E`),
     popped on return and therefore not an observable output;
   - a few 1/4-byte stores in the physical page namespace (`0x82EDxxxx`,
     `0x82EFxxxx`, ~1 per 2–4 views): `449D0`→`130980` cache readiness and
     counters, which must keep running translated.
   The native oracle is therefore an ordered-record comparison of the five
   words plus the three bank storages, the same difficulty class as the main
   cull; stack scratch becomes native locals, and `449D0` stays translated
   per unit.
   - [x] Closed write set measured (2026-10-24).
   - [x] Containment oracle (`HALO2_MAIN_COMMANDS_ORACLE=1`,
         `Impl::main_commands_oracle`): classifies every store of translated
         `40E80` as in-set / emulated stack / physical / outside, and FNV-hashes
         the set after each view. 1,200 4p views: Coagulation in-set 753,487,
         stack 11.74 M, physical 5,358, outside 162,586; Headlong in-set
         1,395,145, stack 20.41 M, physical 0, outside 174,192. Every outside
         store is in one region at `0x101A5000`, directly after bank 2's
         storage: a fourth `0x101000`-type allocation with a header-then-
         records hit pattern, not yet named. Hooks are inert when off
         (frames byte-identical to the pre-session binary).
   - [x] The `0x101A5000` region is not a fourth command bank: `40E20`
         addresses headers at `0x4CE268 + AX*24` and `0x4CE2B0` is unmapped
         (reading it faults), so there are exactly three banks. The region
         belongs to another `40E80` dispatcher (`41C70`/`179A40`/`42090`/
         `42750`/`41630`), all of which stay translated in the hybrid; it does
         not block the BSP kernel.
   - [x] **Native hybrid implemented behind the oracle** (2026-10-24):
         `Impl::main_commands_native`, default on, `HALO2_REFERENCE_MAIN_COMMANDS=1`
         restores translated `40E80` alone, `HALO2_MAIN_COMMANDS_ORACLE=1`
         compares. First oracle run: record *counts* per view identical
         (62/62, 10/10 on Coagulation) but every native record landed in
         banks 1–2 instead of bank 0 — 3,600 mismatches = 3 banks × 1,200
         views, while frames stayed byte-identical because main mode does
         not consume banks 1–2 here. The parity check alone would have
         accepted a wrong kernel; the record oracle caught it.
         Cause, from `40E20` (bank header = `0x4CE268 + AX*24`, early return
         on `AX==-1`) and `41030` `412AF..41346`: the double emission uses
         `EAX=2`; the single emission's bank index comes from `40DD0(ECX=mode)`,
         which returns 0 for mode 0 and the flag-derived 1/2/-1 otherwise.
         The native `enqueue`'s `category1?1:category2?2:-1` is the
         `mode!=0` branch only; a first attempt that set category=mode for
         every mode broke the recorded secondary fixtures and was reverted.
         Exact fix: mode 0 → bank 0, else the existing flag rule.
   - [x] **Native hybrid verified and default** (2026-10-24).
         Record oracle over 3,600 4p views on three maps: Coagulation
         (27,900 native units, 57,900 records), Headlong (32,700 / 110,399)
         and Containment (56,397 / 125,097): **zero differing words, zero
         fallbacks**. The oracle caught three bugs frame parity had passed:
         (1) bank category must be the `40DD0` rule (mode 0 → bank 0, else
         flag2→1/flag4→2/else none), (2) the extra category-2 emission is
         skipped in mode 0 (`41030` at `412D9`), (3) the native record's two
         callback words were TU1.1 `4CBF0/4D0B0`; the TU1.5 image writes
         `4CBE0/4D0A0`, now a `SourceBspPartCommandContext` field with the
         TU1.1 defaults so the recorded secondary fixtures stay bit-exact.
         `main_commands` 2.00 → 1.27 ms/frame (−37%). Translated `449D0`
         per unit, model units, special units and the other `40E80`
         dispatchers remain translated. `HALO2_REFERENCE_MAIN_COMMANDS=1`
         restores translated `40E80` alone.
   - [x] **Model units native too — the whole `44AB0` traversal** (2026-10-24).
         Eligible model units issue the original `449D0` request natively
         (`request_render_geometry(3, section+0x38)` via the new
         `source_live_model_section_header_tu15`), then read their resident
         resource and live palette through `source_live_model_command_geometry_tu15`;
         a failed request leaves the unit translated, as `449D0` returning
         NULL drops it. Shader LOD/layer tables load on demand from the cache
         (`Impl::shader_for`) — the light preload only covered light-receiving
         models, and the first run aborted with an uncaught `std::map::at`
         (`shaders.at`) for shader `E1AA429D`: native kernels must catch
         `std::exception`, not only `FormatError`. Record oracle over 3,600 4p
         views: Coagulation 47,973 units / 80,808 records, Headlong 59,480 /
         153,016, Containment 88,051 / 162,938 — **translated_units=0,
         model_unresident=0, mismatches=0, fallbacks=0**. Translated `40E80`
         still runs for its reset words, special units and the other
         dispatchers (`41C70`, `179A40`, `42090`, `42750`, `41630`).
         Quiet-host A/B (three interleaved pairs, load ≈2.5):
         `main_commands` 2.03 → 1.05 ms/frame (−48%). Cumulative against
         the pre-session binary on the same host, pairs 1–2 (pair 3 hit a
         load spike to 7.5 and was discarded): 49.0 → 37.4 ms/frame of
         main-thread CPU (−24%), 20.4 → 26.6 FPS.
1b. **Remaining `40E80` work and the other per-view calls** (measured
   2026-10-24 with `HALO2_VIEW_CALL_CENSUS=1`, `Impl::view_call_census_run`).
   - Model units are 19,937 of 47,837 units in the `44AB0` range on 4p
     Coagulation (42%) and stay translated. Their native path needs the live
     model section palette/material owner the secondary light frame already
     uses (`source_live_model_command_geometry_tu15`), plus the same `40DD0`
     bank rule and token merge.
   - `C35D0` (light view): 9,600 stores over 1,200 views on each of two
     maps, all on the emulated stack (its own 24-byte top frame), no
     source-memory output. A standalone native kernel has nothing to
     produce. Revisit only together with its consumer.
   - `3D2B0` (first person): ran **zero** times in 300 4p frames on both
     maps (the census key never appeared, including at exit). The
     `first_person_ms` phase (~0.5 ms/view) is native FP work, not this
     call. Not a kernel target.
   - Model units natively: `source_live_model_command_geometry_tu15` reads
     residency at `section+0x34` but does not request; translated `449D0`
     first requests flags 3 through `130980` on the model section header at
     `section+0x38` (`44A12`). The native path must issue that request via
     the existing native `request_render_geometry(3, section+0x38)` owner
     before reading, as the Warlock definition path does for BSP (`+0x28`).
2. **Renderer plan/bind.** Measured 2026-10-24 with host stamps (ms/frame,
   4p Coagulation): skin-plan `add_bank` 0.58, BSP merges + sizing 0.18,
   `group()`/`append_frame` + tail 0.83, per-view material-uniform upload
   0.77 (now parallel, below), shadow/light uniform builds 0.85. Nothing
   dominates; the earlier "~3.5 ms append_frame" framing was wrong. The
   `append_frame` region is 0.43 ms and `FrameVertexUploadPlan` dedups
   identical spans across views, so a per-view split needs private
   sub-plans plus a serial rebasing merge; at ≤0.3 ms gain it is not next.
   Correction after a finer stamp: the shadow/light uniform builds and their
   three uploads total ~0.003 ms on 4p Coagulation (no source shadows or
   sphere lights queued); the 0.85 ms belonged to the material-upload and
   object-upload blocks before them. Remaining renderer target here:
   `add_bank`'s per-vertex `same_local` scan (0.58 ms, bandable per
   submission with the first failing index rethrown).
3. **Per-view translated preparation on four cores** once each view reads an
   immutable snapshot and owns its render-state pages (after item 1).
4. **GPU skinning for weapons, vehicles, equipment, projectiles, scenery.**
5. **Gameplay tick.** Measured per translated phase 2026-10-24
   (`HALO2_TICK_PHASE_CENSUS`, 480 ticks, 4p Coagulation, ms/tick):
   `B8F40` objects **3.08**, `B9090` physics **2.02**, `168CA1` first person
   1.33, `1CC8D0` 0.78, `179CD0` effects 0.66, `15EBA0` engine 0.56,
   `C3430` lights 0.52, `B9180` 0.30, `14FEC0` input 0.26, late phases 0.24,
   the rest <0.05; ≈9.8 ms/tick → ≈4.9 ms per 60 Hz frame. `step_objects`
   is a fixed sequence of whole-engine calls, not a per-object loop, so
   there is no native-side loop to split: tick gains come from native
   kernels for the heavy owners, `B8F40` first (not Havok), each by the
   write-set census → record oracle → hybrid path. Translated Havok
   (`B9090`) stays the authority. Running the whole tick on its own core
   still needs the latency decision.
   `B8F40` write-set census (`HALO2_TICK_OWNER_CENSUS=b8f40`,
   `Impl::tick_owner_census`, 1,440 ticks Coagulation / 1,200 Headlong):
   11.65 M / 8.20 M stores, 92% emulated stack, 7% physical-namespace pages
   (object bodies, the gameplay state itself), and ~57 K / 40 K stores over
   168 / 166 small ranges: per-object 36-byte records in the `0x1004xxxx`/
   `0x1008xxxx` heap, a 640-byte block at `0x100485EC`, 160 bytes at
   `0x4E8BC0`, counters at `0x60200000` (TLS), `0x56E1DC`, `0x486088`, flag
   bytes at `0x4F3CE1`. Unlike `40E80` this is not a bounded record producer;
   it is the object update mutating state. Its write set is enumerable,
   which makes a **per-tick state-hash oracle over exactly those ranges plus
   touched physical pages** buildable now; that oracle is the gate for any
   native replica of a `B8F40` sub-owner (dispatch per object type).
   - [x] **Per-tick state-digest oracle** (`HALO2_TICK_OWNER_DIGEST=<entry>`,
         `Impl::tick_owner_digest`): learns the owner's non-stack write
         ranges live, FNV-hashes their bytes plus touched physical pages after
         every call, and reports a sequence hash. A native replica must
         reproduce the identical digest sequence under the deterministic
         scripted capture. Switch-gated; off by default. Verified
         deterministic: two independent scripted 4p Coagulation runs gave
         identical sequence hashes at tick 240 (`D3D7EE371A731147`) and 480
         (`6F078AEAAE61102E`) over 172 learned ranges and 83 physical pages.
   - `B8F40` is a thin dispatcher (121 lines): `BE4A0` twice and `10E630`.
     `BE4A0` is the per-object update entry (resolves the datum in the
     `0x4ECBD0` pool, then per-type `BDA50`/`B37A0`/`D8330`, `10DD10`,
     `C0910`, `1D6FB0`, `1D5790`). Research coverage of those per-type
     owners is thin (`B37A0`, `C0910`, `1D6FB0`: none; `BDA50`: one note), so
     native replicas here are new decompilation work gated by the digest,
     not assembly of existing native pieces. Payoff bound: ≈1.5 ms/frame.

## Notes

- Profiler, CPU-per-frame monitor, interleaved A/B and frame parity tools are in
  [`tools/perf_4k60`](../tools/perf_4k60/README.md); run them through
  `tools/run_test.py` so raw output stays in `HALO2_TEST_SCRATCH`.
- `unit_action_prompt` costs ~5% inside `MultiplayerMatch::state()`, but runs
  per tick; caching it would change nothing exactly.
- **4p Warlock fixed** (2026-10-24). The native BSP preload requested every
  portal-admitted instance definition and stopped on any failed `130980`
  request. Now instance definitions are requested per appended unit, as
  `2CBF0`→`449D0` does (flags 3, zero-alpha instances never request), and a
  failed request drops that unit for the frame, as `449D0` returning NULL does.
  Clusters keep their per-frame request. Frames stay byte-identical to the
  pre-session binary on Coagulation, Containment and Headlong 4p and Warlock
  2p/3p. Open: in 4p Warlock three instance definitions (6, 12, 28) still
  fail in roughly one view per frame because the Xbox-sized geometry cache is
  full; check MCC resource sizes against that budget before calling it
  faithful.
