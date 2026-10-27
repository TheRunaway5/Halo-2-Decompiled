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
   - Nested-owner census (`HALO2_NESTED_OWNER_CENSUS=<hex>`, hooked at the
     `Impl::required` dispatch boundary): `BDA50`, `B37A0`, `D8330` reach it
     (not in this unit's static switch); `10DD10` and `BE4A0` are static
     cases and need a PC-cursor hook instead. `BDA50` measured 3.9 µs/call,
     ≈32 calls/tick ≈ 0.13 ms/tick: ~4% of `B8F40`. The object update's
     cost is not in the per-object body dispatch; `10DD10` (3.45% inclusive
     in the sampler, the largest `B8F40` descendant) is the next to cost.

## Lazy single-LOD object posing (2026-10-25)

`SourceObjectGeometry::select()` chooses a LOD from metric, history and
policy alone and then builds the view from that one LOD bank; every caller
nevertheless posed all six LOD banks first (`pose_world_nodes`, dynamic
objects once per **view**; vehicles' `pose_world` once per frame), so ≥5/6 of
the ≈6.4% CPU skinning in the profile was for LODs never drawn.

- [x] `SourceObjectGeometry::choose_lod` (the same LOD decision `select()`
      makes, shared through `lod_choice`), `pose_world_lod` (one LOD bank,
      WORLD records or deferred local records + palette), `ViewDraw::ordinals`
      and `view_skin` (a renderer-deformed `ModelSkinGeometry` for a deferred
      bank); per-LOD palette/deferred state travels with the presentation
      exchange. `SourceDynamicObjectDraw::skins` → `ModelObjectDraw::
      deferred_skins` (the renderer already consumed that field; no producer
      had used it).
- [x] Dynamic objects (`SourceDynamicObjectScene::draw_view`): choose LOD →
      pose only that bank through the original CPU kernel. Internal phase
      timers: `host_view{0..3}_objects_ms` −0.32/−0.33/−0.22/−0.25 ms
      (≈ −1.1 ms/frame), renderer phases unchanged; frames byte-identical.
      `HALO2_REFERENCE_OBJECT_LOD_SKINS=1` restores six-LOD posing.
- [x] Measured and rejected as default: deferring the chosen bank to the GPU
      (`HALO2_GPU_OBJECT_SKINS=1`). Host `objects_ms` −0.54 ms but renderer
      `geometry_plan_ms` +0.87 and `pack_ms` +1.02 ms — per-submission plan/
      pack work exceeds the small objects' skins. Kept selectable; the GPU
      route stays right for large banks (units), not for ~100 small ones.
- [x] Vehicles: the PRT six-LOD `pose_world` now poses the chosen LOD only
      (`vehicle_frame_poses` still caches per (handle, LOD) across views).
      A/B, three interleaved pairs: main-thread CPU 34.6 → 33.7 ms/frame
      (−0.9 ms); `host_view{0..3}_objects_ms` −0.43/−0.27/−0.24/−0.27 ms;
      frames byte-identical. Same `HALO2_REFERENCE_OBJECT_LOD_SKINS` switch.
- Also measured and rejected: `MALLOC_TRIM_THRESHOLD_=1GiB` (B +1.5 ms);
  `__default_morecore` 3% is not trim regrowth.

## Native seat-candidate sphere (2026-10-25)

Profile of the current binary showed `MultiplayerMatch::state()` at 7.9% of
the frame, 5.5% of it in `MultiplayerObjectRuntime::vehicle_seat_target` →
translated `CB3D0` (seat selector, ~65 µs/call, 40 calls/frame = 4 units ×
every visible vehicle). The original (`153BA0`) gathers candidates with the
`BD080` objects-in-sphere query (pass 1: radius = unit bound radius (+3C) +
`[46A240]` + `[46A0E4]`, mask `FFFFEBCB`) and runs `153F20 → CB3D0` only on
those; the native replica in `native_vehicle_interaction.cpp` ran the
selector for every vehicle on the map. `CB3D0` is pure (store census: 22.4 M
stores over 24,000 live calls, all on the emulated stack plus the two
native out-words), so skipping calls cannot change simulation state.

- [x] `seat_candidate_in_sphere` (`multiplayer_object_runtime.cpp`): BD080's
      own admission test, `(radius + object radius)² ≥ (dz²+dy²)+dx²` in its
      accumulation order (NaN rejects, like `comiss/jb`), on the bodies' own
      `+30/+3C` words and the live image constants. BD080's cluster walk can
      only remove objects, so this admits a superset of the original
      candidates and never fewer. `HALO2_REFERENCE_SEAT_CANDIDATES=1` restores
      the unfiltered route.
- [x] Oracle (`HALO2_SEAT_CANDIDATE_ORACLE=1`): runs the selector for every
      pair anyway and calls the translated `BD080` with the pass-1 arguments
      (64-slot buffer at `0x6004A100`), checking `accepted ⇒ in sphere` and
      `BD080 members ⊆ sphere`. Scripted 4p runs: Coagulation 26,400 pairs,
      Headlong 31,200 pairs, 0 violations — and 100% of those pairs were
      outside the sphere (`bd080_members=0`), i.e. every selector call in
      those workloads was for a vehicle the original would never consider.
      Positive case via the real vehicle-entry suites
      (`player_external_forces_tu15_tests … -hit_2`, run with the MCC map):
      Warthog 4,584 pairs, Gauss Warthog 4,584, Ghost 2,741, Spectre 4,440,
      Scorpion 7,196, Wraith 6,860, Banshee 4,294 — every entry succeeds,
      235 selector acceptances, `accepted_outside_sphere=0` in every run;
      `BD080 members ⊆ sphere` for 1,329 of 1,330 member pairs. The one
      exception (Banshee, not accepted by `CB3D0`): unit r=0.44 at
      (94.75,−176.85,1.54), vehicle r=1.4 at (96.15,−180.85,4.65),
      d=5.2577 vs reach=5.24 with live constants `46A240=0.4`, `46A0E4=3.0`
      (`45695C=FLT_MAX` is only `CB3D0`'s best-distance initializer). That
      18 mm is the sampling skew the native prompt already had: `BD080`
      tests the object's cluster-node cached centre (written by the object
      update before physics moves the body) while the native prompt evaluates
      after the tick on the body's `+30`. It can flip a prompt only if a
      vehicle moves more than (query radius − seat reach) within one tick.
      About half of the in-sphere pairs are not `BD080` members (its cluster
      walk prunes them): that is the stage-2 native `BD080` saving.
- Measured: main-thread CPU 37.8 → 34.7 ms/frame (−2.1 ms, three interleaved
  pairs), `host_state_projection_ms` 2.8 → 0.8 ms; frames byte-identical to
  the baseline binary.
- Next in this family: native `BD080`/`14D0E0` (cluster objects-in-sphere)
  kernel with a result-list oracle — removes the remaining superset slack and
  is also called from the object update (`10E630 → BD080`).

## Smaller exact wins (2026-10-25)

- [x] First-person deferred skin geometry now marks `arithmetic_admitted`
      (its records are the unmodified local bank `model_skin_world_pose`
      admitted with that palette; `select_indexed` only filters). The
      renderer's `geometry_valid` stops re-running the admission per
      submission: `geometry_plan_ms` 1.75 → 1.41 ms, frames identical.
- [x] Shadow-mode `40E80`: the native kernel now reads the context pointer
      (`4CD3A0`), its mode word (`+2A78`) and the EAX layer selector, so it
      serves secondary contexts; the shadow roots' call is hooked
      (`secondary_commands`). No measured workload reaches that branch
      (`hook_calls=0`), so it stays behind `HALO2_NATIVE_SHADOW_COMMANDS=1`
      until the record oracle sees it on live data. The 1.9% of translated
      `40E80` in the profile is the main view's translated **remainder**:
      `41C70→41C10→41970` object-queue scheduling (1.19%) + nested dispatches
      (0.49%); `44AB0` over hidden units is 0.07%.

## Sub-page region index (2026-10-25)

`HALO2_LOCATE_CENSUS=1` (new, prints at exit) showed 60 M slow-path
`locate` calls per 300 frames (≈200k/frame); 34 M of them were `std::map`
walks for the executable image's globals, because the image is mounted as
thousands of tiny owner regions (4–164 bytes) that neither page index could
name. `small_pages_` now lists, per 4 KiB page, the regions that cover it
only partly (sorted by base, same map iterators); `locate()`/`try_read_view`
consult it before the map, which stays the fallback. Map-path hits for
`0x400000`: 26.7 M → 104 k. Frames byte-identical. A/B on a loaded host
(load 7–9): all three pairs faster (−3.0/−0.5/−1.4 ms), `host_match_step_ms`
9.83 → 8.67 ms; re-measure on a quiet host before quoting more than ≈ −1 ms.
Then two more exact changes in the same family:
- `SourceRuntimeMemory::view()`/`mutable_view()` take the direct page when
  one exists (every native `word()` helper reads through `view`): census
  60.5 M → 37.3 M slow-path calls per 300 frames, physical namespace
  `0x82D00000` 16.4 M → 0.14 M. Frames identical.
- The host-physical write guard no longer takes a mutex and walks a
  `std::set` per upper-half write: `PhysicalProtection` mirrors its
  read-only set in an atomic per-page bitmap, changed one page at a time by
  the three writers under the existing mutex (`release`, `protect`,
  `commit_ownership`), so the guard is one or two acquire loads. Spanning
  and empty writes keep the checked set. `protect()` can run from the audio
  thread, which is why bits flip individually (no clear-then-set window).
  Frames identical; A/B pending a quiet host (the first attempt ran at load
  12–21 and is not quotable).
What remains on the slow path (~120k/frame) are the tiny image-global
regions themselves: `read()` is already one lookup + commit check + copy
there, and `write()`'s second `locate` after the policies is required.

## Status 2026-10-26 (blocked on the shared tree)

- Overlap A/B, one clean pair (load ≈2): main-thread CPU **29.5 ms with the
  overlap vs 33.1 ms synchronous** (−3.6 ms); the other pairs ran at load
  7–16 and are not quotable. `host_pre_render_ms` is inflated under overlap
  (`now` is the ahead tick's stamp); cpumon CPU time is the metric.
- Banded BSP frustum cull (`source_bsp_frustum_cull` on the pool, exact by
  construction: per-unit records + pre-allocated disjoint mask words,
  serial fallback when spans overlap or any precondition fails). The walk now
  takes an explicit `SourceBspFrustumCullExecution`; `retail_source_bsp_ranges`
  runs the recorded-original fixture corpus through `banded` and `serial` and
  both reproduce the recorded unit records and arena. Integration frame
  parity not yet taken → opt-in `HALO2_NATIVE_BANDED_CULL=1` until then.
- Every 4p run of the current tree aborts at ~4.8 s with "Native FP look
  input differs from its actual remote unit" from the other session's new
  untracked `src/gameplay/animation/source_first_person_context_tu15.cpp`
  (included by their `cmake/NativeFirstPersonContextTu15.cmake`), in free
  and scripted runs, with and without the overlap, and with direct pages
  disabled; `halo2_native_prev3` (built before that file) runs. Re-run when
  their tree works: parity for the banded cull (then default it on),
  cumulative A/B vs `prev3`, a fresh profile with a kept binary.

## GPU skin admission: static per-model summary (2026-10-26)

`model_skin_gpu_arithmetic_admitted` (2.0% self; after the first-person
flag, 0.82% remains on the local players' direct body/gun poses) re-derived
every vertex's attribute/weight gates each frame although vertices and
bindings are immutable after load. `ModelSkinAdmissionStatic` +
`model_skin_gpu_admission_static()` hold that part once; `model_skin_gpu_
arithmetic_admitted(summary, palette)` decides exactly as the full predicate
(same palette-row loop, same per-vertex combination; the admission corpus
test now asserts equality with the frozen reference on every case).
`model_skin_world_pose` uses a cache keyed by model identity/checksum/bank
storage behind `HALO2_NATIVE_SKIN_ADMISSION_CACHE=1` (opt-in until a frame
parity capture exists); `HALO2_VERIFY_SKIN_ADMISSION=1` evaluates both and
throws on disagreement.
Lesson recorded: `HALO2_REFERENCE_SCALAR_SKINNING`/`HALO2_VERIFY_SIMD_SKINNING`
are toggled with `setenv` at run time by the skin diagnostics tests, so they
must be read per pose — the earlier hoist into statics broke
`native_model_skin_handoff` and is reverted. (That suite currently also
cannot relink: the other session's regenerated
`physics_phantom_force_tu15_generated.cpp` references
`native_lift_scalar_*_tu15` symbols not yet linked into it.)

## Added 2026-10-26 (evening)

- Unit tests: `native_overlap_run` semantics (thread placement, SSE/x87
  sticky-flag merge, errno rules, exception priority, pool use from the job)
  in `native_parallel_tests`; the sub-page region index (`small_region_index`
  in `source_runtime_memory_tests`); the lock-free physical write guard
  (`physics_physical_storage_tests`). All pass.
- Pool wake: `native_parallel_for` wakes `min(count−1, workers)` workers with
  `notify_one` instead of broadcasting to all seven (`pthread_cond_broadcast`
  was 0.83% self); results unchanged.
- Overlap accounting: ticked-ahead frames stamp `now` at the iteration top so
  `host_pre_render_ms` measures this frame's preparation.
- Shared tree: links again, but every run aborts at startup on a region
  overlap at `0x4C6858` mounted by the other session's edit of
  `native_tu15_world_memory.cpp` (16:45) — still unusable for parity/A/B.

## Frame map from prof-seat, by direct callee (2026-10-26)

Measured with a full-depth callee query over the kept profile (the earlier
`children.py` numbers were leaf-distance limited, which made teardown look
dominant). Percent of samples; the tick (`MultiplayerMatch::step`) is 22.1%
against 8.8 ms, so 1% ≈ 0.4 ms/frame on that run.

| Block | % | Notes |
|---|---|---|
| `MultiplayerMatch::step` | 22.1 | tick: B8F40 3.1 ms, B9090 2.0, 168CA1 1.3 |
| `Renderer::Impl::draw` | 15.3 | now overlapped on the render thread |
| `SourceBspEffectLights::draw` (inclusive) | 11.4 | main cull 1.7, model draws 2.6, main commands native 2.6, translated 1.2, lights 1.1 |
| cross-unit dispatch glue (self) | ≈3.4 | `State::required` 0.7, `required_entry` 0.7, `services()` 0.4, unit dispatchers ≈1.4 |
| `SourceDynamicObjectScene::draw_view` | 3.1 | pose_world_nodes 1.1 (pre lazy-LOD) |
| `ModelFramePoseCache::update` | 2.6 | vehicle CPU poses (pre lazy PRT) |
| `MultiplayerHud::animate` | 1.8 | `animate_forward_constants` 1.4 (self-heavy) + 8 KB constant memcpy per material |
| `MultiplayerMatch::state()` | 2.3 | 0.9 of it was `find_unowned_range` walking the whole region map |
| translated owners called from the frame loop | 5.8 | 40E80 remainder 1.9 (41C70 1.2), 22830 per-view reset 1.4 (all in `write()`, pre sub-page index), 3D2B0 1.1, 333F0 0.4, 177830 0.4 |

Exact native fixes taken from it (no gameplay semantics involved):

- `SourceRuntimeMemory::find_unowned_range` starts at `upper_bound(candidate)`
  instead of walking every region below it (owner regions are disjoint, so the
  answer is unchanged; `gap_query_matches_full_walk` compares >1000 queries
  against a full walk). Removes ≈0.35 ms/frame from `state()`.
- `PhysicsLiveWorldTu15::Impl::services()` returns one built table instead of
  constructing six `std::function`s per cross-unit call. All callbacks capture
  only the owner; the only by-value field is the main-thread id, which keys
  the rebuild. **Lesson:** the table must be declared first in `Impl` —
  translated teardown dispatches while later members are destroyed, and a
  destroyed mutex/vector under it corrupted the heap (both live-world suites
  aborted at exit until the members moved to the top).

Remaining dispatch glue (≈1.3 ms/frame) is structural: each unit dispatcher
builds a `LiveState` and `FpScope` per call and `State::required` saves and
reloads the full register file around every cross-unit call.

## Next structural step (proposed): split the per-view loop into phases

The per-view loop (≈18 ms at 4p/4K) interleaves translated work that must run
one view at a time on the single source bank (shadow main user/location/fog/
camera, 22830 per-view reset, main cull/commands, shadow selection, model
builds, lens/particle views, the first-person chain) with native consumers that
only read captured inputs (`replica_world_nodes` copies, completed render nodes,
per-view HUD state): object/vehicle/prop posing (`objects_ms` ≈4.4 ms), HUD
animation (≈1.1 ms), static-world merge, lens draw assembly. Keeping gameplay,
latency and output identical, the loop can run phase A (translated, views in
order) and then phase B (native consumers) on the pool, one view per worker:
≈5–6 ms of main-thread time moves off the critical path, more than any single
remaining kernel. Entanglements to resolve first:

- `source.memory().view(...)` reads inside phase-B code (one in the objects
  phase today: damage flags at the item body) must be captured in phase A.
- The first-person chain (`prepare_first_person_models`, 168CA1/168DED) is the
  other session's active area; phase B must not touch it until they land.
- `hud_continuity`/`pose_continuity` assert the owner thread: phase B either
  runs HUD on the main thread or the continuity owners learn worker tokens.
- Lens/particle per-view calls (`render_model_view` at the end of each view)
  stay in phase A; their native draw assembly can move.

Validation is the existing parity harness (byte-identical frames) plus the
`HALO2_REFERENCE_SERIAL_NATIVE=1` switch, which already serializes the pool.

## Structural step: render overlap — implemented 2026-10-25

Built as designed below, with one correction found by the networked C++
driver: the previous iteration's authority frame must close at the very
start of `advance` (the old iteration boundary) because `authority.service()`
runs before `begin_frame()` and rejects service while a frame is open.
- `native_overlap_run(job, overlap)` (`native_parallel.cpp`): persistent
  thread; the job inherits the caller's SSE/x87 control and errno, its sticky
  flags and errno change are carried back, exceptions rethrown (job first).
- `Renderer::draw(..., overlap)`: runs `Impl::draw` through it when given.
- Frame loop: the tick block is `advance(timing)` returning
  proceed/skip/stop; hoisted outputs (`now`, `measured_at` + timing lambdas,
  `audio_elapsed`, `elapsed`, `blend`, `animation_blend`, `pose_tick`,
  `host_tick_begin`, `source_frame`); `pending_timing` adopted by the next
  iteration; `captured_frame` snapshot; `overlap_allowed` = no
  `HALO2_REFERENCE_SYNC_RENDER`, no `online_match`, no live client, none of
  the 45 `MultiplayerOptions` observers; the choice is run-constant and is
  printed once ("Render overlap: enabled/disabled").
- Verified: frame parity vs the pre-session binary byte-identical with the
  overlap on (two runs, three configurations) and with
  `HALO2_REFERENCE_SYNC_RENDER=1` (the converted loop in the original order);
  free 4p runs exit cleanly; `multiplayer_engines_world_tu15_tests` (C++
  frame-loop driver with LAN options → synchronous path) passes 39/39. The
  19 Python-driven online suites cannot run here (`No module named
  'aiohttp'`, pre-existing). A/B against `HALO2_REFERENCE_SYNC_RENDER=1`
  pending a quiet host.

### Design (as approved)

Decisions taken with the user: overlap `Renderer::draw` with the next tick —
yes; run the tick a frame ahead with added input latency — no.

`Renderer::draw` is 15.8% (≈5.3 ms), native, never touches the emulated world
(`renderer.cpp` has no `run_actual`/`call_actual`/`live_world`). Overlapping
it with the **next frame's tick** changes no gameplay and adds no latency: the
tick starts earlier, frame N is presented at the same moment.

Design (no container hoisting): `Renderer::draw(..., overlap)` runs the
existing draw body on a persistent render thread and executes `overlap()` on
the calling thread, then joins and returns/rethrows as today. The caller
passes the next frame's tick block as `overlap`, so every renderer input is
still an iteration local that is alive. Required runtime edits
(`multiplayer_runtime.cpp`, frame loop 3257–7172):
1. Wrap 3258–4130 (housekeeping, online handoff, input, step, projection,
   post-step, `measure(timing.input_ms)`) in
   `auto advance=[&](MultiplayerFrameTiming &timing)->Advance{…}` defined
   at the top of the iteration. Outer-loop control inside becomes return
   codes: `stop` for the breaks at 3269, 3299, 3336, 3345, 3350, 3640, 4038;
   `skip` for the continues at 3309, 3327, 3542, 3551, 4039, 4105, 4114
   (all other breaks/continues there are inner loops — classified by brace
   depth, keep them).
2. Hoist the block's outputs consumed by preparation to before the loop and
   turn their declarations into assignments: `now`, `measured_at` (+ the
   `measure`/`host_stamp`/`host_returned` lambdas defined before the loop
   over it), `audio_elapsed`, `elapsed`, `blend`, `animation_blend`,
   `pose_tick`, `source_frame`, `host_tick_begin`, `phase`.
3. Iteration top: `if(!ticked_ahead){switch(advance(timing)){stop:break;
   skip:continue;}} else {timing=std::move(pending_timing); ticked_ahead=
   false; if(skip_next){skip_next=false;continue;}}`.
4. At the draw: `const bool captured=capture;` (post-draw must not read
   per-tick values after the overlap), then
   `renderer.draw(..., overlap_allowed?[&]{pending_timing={}; pending_timing
   .frame=frames+1; pending_timing.views=…; const auto saved=measured_at;
   const auto r=advance(pending_timing); measured_at=saved; ticked_ahead=
   true; skip_next=(r==skip); stop_next=(r==stop);}:nullptr)`; after the
   draw, honour `stop_next` by leaving the loop after the post-draw block.
5. `overlap_allowed` = no `options.online_match`, no frame/render observers
   (`frame_observer`, `completed_*`, `shadow_*`, `reference_*` observers),
   not `HALO2_REFERENCE_SYNC_RENDER=1`; every test/harness path then keeps
   today's synchronous order exactly.
Preconditions found while scoping (must hold, or be arranged, first):
- **Authority frame lifecycle (blocker):** each iteration holds
  `source_frame = authority.begin_frame()` (RAII; `~Frame()` = `finish()`),
  and the authority rejects nested frames. An overlapped tick N+1 calls
  `begin_frame()` while iteration N's frame is still open, so frame N must be
  finished before the overlapped draw starts, and the post-draw world access
  (`source_lens_queries->finish` via `render_model_view`) must be confirmed
  legal outside a frame — or moved before the finish. Resolve from
  `multiplayer_runtime_authority.cpp` (the other session's design) before
  writing code. `source_frame` is otherwise unused by preparation.
- The renderer's presentation-copy worker compares `result.executor` with
  `this_thread` (`renderer.cpp:6148`): the overlap must be a run-constant
  choice (never alternate sync/async between frames).
- The frame loop already carries the authority/handoff continuity guards:
  `pose_continuity.mutating()` and `hud_continuity->mutating()` assert the
  owner thread (the tick stays on the main thread — fine);
  `material_continuity->mutating()` requires `quiet()` — no live borrowers —
  so the renderer must hold **no** material/pose/HUD continuity borrow across
  `draw`, and `draw` must make no thread-bound continuity call
  (`owns_snapshot` compares `this_thread` with the executor). See the grep
  result recorded below this section before implementing.
- The renderer's `renderer_executor` thread checks live only in lens/particle
  preparation (`prepare_lens_query_retirement`, `prepare_particle_views`,
  `particle_view_inputs`, `lens_query_retirement_ready`), none reached from
  `Impl::draw` — the hop itself is clean.
- Expected: main-thread CPU −min(render, tick) ≈ −5.3 ms/frame; frames
  byte-identical (same inputs, same code, default FP environment on the
  render thread equals the main thread's ambient one — verify with parity).

## Open leads (2026-10-25, cost order, from `prof-seat`)

0. Main `40E80` translated remainder (1.9%): `41C70→41C10→41970` =
   per-object render-model **attachment scheduling** (`41C10` walks children
   `+0x10`/siblings `+0xC`; `41970` allocates a 0x1CA0 frame via `32A660`,
   scans the model's attachment list at `model+0x98` for `'lens'`, `'MGS2'`,
   `'tdtl'` tags, resolves markers with `BAD60`, transforms with `C0700`
   (0.45% — most of it), tests owner/view with `3E9B0` (`158290`, `BCFB0`),
   sets context byte `0x56E197`, writes its records for the lens/light-volume
   consumers; `D7230` dispatches per-attachment-type callbacks, 0.09%). A
   native replica is ≈300 lines across five owners plus the record consumer
   for ≈0.35 ms — smallest payoff per effort on this list.
1. Shadow-mode `40E80` (secondary context): entered natively per shadow root at
   `source_live_shadow_roots_tu15.cpp:164` (`world.run_actual(0x40e80u,a)`,
   overlays `4C6838=1`, `4C6834=0x14`; banks 1/2 at `4CE280/4CE298`, secondary
   context `5598F8`). Wire a `commands` hook like the main view's and let
   `main_commands_native` serve mode≠0 under the existing record oracle; the
   builder is `source_live_shadow_root_frames_tu15` (declared in
   `source_live_shadow_selection_tu15.hpp:64`).
2. `PhysicsLiveWorldTu15::Impl::build_render_model` (1.8%, native, under
   `source_live_model_draws_tu15`): breakdown not yet taken.
3. `model_skin_gpu_arithmetic_admitted` (2.0% self): per-model-static vertex
   and binding exponents can be precomputed once; only palette rows vary per
   frame. Same predicate, exact.
4. Tick: `B8F40 → BE4A0 → BE850 → 10DD10 → DF880` (4.6%) and the first-person
   chain `168CA1/168DED` (6.5%, in `source_run_sandbox_special_tu15`), both
   decompile work gated by the per-tick digest; the tick-on-its-own-core
   option still needs the latency decision.
Retained for profile symbolization: `build-perf-4k60/halo2_native_prof2`
(copy of the profiled binary, ~200 MB; delete when `prof-seat` is no longer
needed).

## Reading the frame timings

`host_input_ms` is measured by `measure(timing.input_ms)` from the frame's
first `measured_at`, so it **contains** the nested `host_match_step_ms`,
`host_state_projection_ms` and `host_post_step_ms` (≈8.5 + 2.9 + 2.0 of its
≈12.4 ms). It is not a separate input cost. The main-thread frame at 4p/4K
(2026-10-25, 37.7 ms) decomposes as: tick ≈8.5, state projection ≈2.9,
post-step ≈2.0, common ≈1.9, per-view preparation ≈18 (camera_lighting 7.1,
objects 4.4, model_collection 4.2, body + first person ≈2.7), renderer ≈5.9.
Per-view preparation is the largest block and the one that scales with
player count.

## Where the goal stands (2026-10-25)

Measured on a quiet host against the pre-session binary: main-thread CPU
49.0 → 37.4 ms/frame (−24%), 20.4 → 26.6 FPS at 4p/3840×2160; with the
2026-10-25 seat-candidate sphere (−2.1) and lazy LOD posing (−0.9) the
same workload runs at ≈33.6 ms/frame main-thread CPU. Target 16.7 ms.
Frames byte-identical throughout; every native kernel has a record or
digest oracle that compared it against the translated route on live data.

Landed: direct-page memory fast paths; GPU skinning default; native main
cull (`44E80` corridor); native main commands (`40E80`, whole `44AB0`
traversal, BSP and model units); exact fork-join pool with six uses (two
LOD skinning paths, motion-sensor compositing, per-view BSP merge,
per-view material upload, banded skin-plan scan); 4p Warlock fix.

Remaining translated owners, per frame at 60 Hz, each now measured:
Vulkan renderer ≈5.3 ms (native but order-dependent; no exact split found
beyond the two landed), tick `B8F40` object update ≈1.5 ms (fresh
decompilation of per-type owners, digest-gated), tick `168CA1` first person
≈0.67 ms, `1CC8D0` ≈0.4 ms, `179CD0` effects ≈0.33 ms, Havok `B9090` ≈1.0 ms
(stays translated by rule). `3D2B0` runs 0×, `C35D0` writes nothing
persistent. No single remaining owner is a 4K60 unlock; they are each
small and each is new decompile work with its oracle already in place.

The one structural change left is running the gameplay tick concurrently
with presentation (its own core, dirty-page snapshot of the bank). That
adds up to one frame of input latency and is a user decision, not a
measurement. Until then, further gains are incremental native kernels in
the order above.

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
