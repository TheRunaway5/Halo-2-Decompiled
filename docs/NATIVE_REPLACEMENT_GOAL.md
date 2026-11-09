# Goal: replace all translated production code with native replications

Set 2026-11-07 at the user's direction: "we shouldnt be using any emulation, it
should ALL be native replications". Companion to
[the 4K60 split-screen goal](NATIVE_4K60_SPLITSCREEN_GOAL.md); this is now the
main lever for it.

On 2026-11-08 the user explicitly requested an active goal and checklist and
completion of **all** remaining production ports. The active goal now covers
campaign, multiplayer, rendering, audio, UI, startup and runtime dependencies.
Track implementation and independent parity separately from runtime closure,
performance and delivery in [the complete migration checklist](NATIVE_REPLACEMENT_CHECKLIST.md).
The goal remains active until those completion gates are met.

Scope includes every compiled, reachable production variant. A TU1.5 parity
cohort does not exclude the retained MapPack11 `State` owners: the application
still accepts an explicit gameplay image and source-world selection, and those
owners remain linked and callable. Track their image and selection conditions
separately; code is reference-only only when an actual boundary establishes
that restriction. See [the legacy reachability audit](native_replacement/LEGACY_IMAGE_REACHABILITY.md).

## Why

MCC's Halo 2 (`halo2.dll` 1.3528.0.0) is the engine compiled natively for x64:
plain native loads of object fields, no per-instruction bookkeeping, a
three-worker render job queue (see
[the MCC architecture audit](MCC_CLASSIC_ARCHITECTURE_BORROW_AUDIT_20261023.md)).
This port still executes large parts of the game as statically translated
TU1.5 machine code (`run_XXXXX(LiveState&)` in the `*_generated.cpp` units):
emulated registers, flags and memory, an instruction-observation store per
instruction and the full store-dispatch path per memory write.

Measured 2026-11-07 (Release perf build, 300-frame scripted runs, Original
graphics, 3840x2160 internal):

| Map, players | Frame time | Frame rate | 2026-11-07 end of session |
|---|---|---|---|
| Coagulation 1p | 18.3 ms | 53 fps | 15.8 ms, 63 fps |
| Coagulation 4p | 28.3 ms | 35 fps | 28.9 ms, 35 fps |
| Zanzibar 1p | 51.9 ms | 20 fps | 31.8 ms, 31 fps |
| Zanzibar 4p | 69.3 ms | 14 fps | 45.9 ms, 22 fps |

Zanzibar at 1p executes **~1.85 million translated instructions per frame**
across 927 routines entered from native code through 210 entry points. That
instruction count does not establish native execution time; measure each
replacement in a separate non-accounted runtime cohort. At that checkpoint,
the translated work dominated frame time (presented-pose relocation ~13 ms,
tick ~9 ms at 1p).

## Acceptance

- No translated production routine executes in any enabled production image
  variant, including startup, loading, rare/error branches and callbacks.
  The initial measured TU1.5 cohort covers multiplayer frames on the
  target maps (start with Coagulation and Zanzibar; extend to every shipped
  MP map) at 1p and 4p; the instruction census reports zero.
- Frames byte-identical to the translated route on the parity cases
  (`tools/perf_4k60/parity.py`, 4:90, 4:150, 2:150, 1:150, per map).
- Each translated route survives only as a `HALO2_REFERENCE_*` oracle until its
  native replacement has been proven, then is removed.
- Track frame time per map at 1p and 4p after every landing.

## Method (per entry point, cost order)

1. Measure: `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1 HALO2_INSTRUCTION_CENSUS=<file>`
   on two run lengths, then `tools/perf_4k60/census_roots.py SHORT LONG FRAMES`
   (entry points by per-frame instructions) and `census_map.py` (routines).
2. Understand the executed path (the census lists the exact addresses that run),
   using the existing research notes and native replicas.
3. Write a native replication over native data where the owning subsystem allows
   it; otherwise a native kernel over the published source state (pattern of the
   native 40E80 command walk and `native_tu15_matrix_leaves.hpp`).
4. Oracle: run native and translated on live data and compare every output
   (stores, records, registers/flags where they escape) — no emulation in the
   native path; observers/censuses are parity checks only.
5. Byte-identical frames on all parity cases, native by default, reference
   switch kept until the subsystem is fully native.

Rules: gameplay and frames identical to TU1.5; another session (Codex) edits
these files concurrently, keep changes surgical; test-storage rules in
AGENTS.md.

## Backlog (Zanzibar 1p census, 2026-11-07)

"share" splits a shared routine's instructions evenly across the entry points
that reach it; "exclusive" counts routines reached only from that entry.

| rank | entry | share/frame | exclusive | routines | unit | done |
|---|---|---|---|---|---|---|
| 1 | DF880 | 191,478 (10.4%) | 22,103 | 262 | physics_live_support | [ ] |
| 2 | B9180 | 173,081 (9.4%) | 22,022 | 170 | physics_live_support | [ ] |
| 3 | B9090 | 158,418 (8.6%) | 10,507 | 162 | physics_live_support | [ ] |
| 4 | B96B0 | 132,224 (7.2%) | 482 | 101 | physics_live_support | [ ] |
| 5 | 2EC9B0 | 122,014 (6.6%) | 122,014 | 3 | physics_live_support | [ ] |
| 6 | C9010 | 101,514 (5.5%) | 2,333 | 141 | physics_live_support | [ ] |
| 7 | 180520 | 93,537 (5.1%) | 3,578 | 107 | source_combat_action | [ ] |
| 8 | C2CB0 | 90,457 (4.9%) | 24,377 | 40 | physics_live_support | [ ] |
| 9 | 1179C0 | 55,690 (3.0%) | 2,165 | 39 | source_scene_device_constructor | [ ] |
| 10 | 23670 | 55,540 (3.0%) | 46,365 | 10 | source_model_render | [ ] |
| 11 | 2EBC70 | 55,271 (3.0%) | 55,271 | 1 | source_collision_query | [ ] |
| 12 | 2EB0C0 | 46,544 (2.5%) | 46,544 | 2 | physics_live_support | [ ] |
| 13 | 3DAF0 | 42,337 (2.3%) | 12,878 | 60 | source_model_render | [ ] |
| 14 | 153BA0 | 31,088 (1.7%) | 423 | 10 | source_combat_action | [ ] |
| 15 | CD4A0 | 29,713 (1.6%) | 343 | 55 | source_sandbox_special | [ ] |
| 16 | BCEE0 | 25,639 (1.4%) | 25,639 | 1 | source_vehicle_director | [ ] |
| 17 | DFEE0 | 24,731 (1.3%) | 290 | 41 | physics_live_support | [ ] |
| 18 | B8F40 | 23,119 (1.3%) | 11,529 | 19 | physics_live_support | [ ] |
| 19 | 1ADF60 | 21,959 (1.2%) | 21,540 | 12 | source_combat_action | [ ] |
| 20 | 179CD0 | 19,750 (1.1%) | 5,141 | 35 | source_combat_action | [ ] |
| 21 | 1157D0 | 18,352 (1.0%) | 5,622 | 30 | physics_live_support | [ ] |
| 22 | 114750 | 17,422 (0.9%) | 4,714 | 46 | source_ai_frame | [ ] |
| 23 | 10B840 | 17,068 (0.9%) | 1,287 | 24 | source_scene_device_constructor | [ ] |
| 24 | BEC50 | 16,331 (0.9%) | 15,869 | 10 | physics_live_support | [ ] |
| 25 | F34B0 | 15,860 (0.9%) | 5,206 | 22 | physics_live_support | [ ] |
| 26 | 15EBA0 | 11,203 (0.6%) | 11,203 | 26 | source_multiplayer_engines | [ ] |
| 27 | 18F1A0 | 11,118 (0.6%) | 1 | 12 | physics_live_support | [ ] |
| 28 | F2C40 | 10,403 (0.6%) | 534 | 29 | physics_live_support | [ ] |
| 29 | 322410 | 10,332 (0.6%) | 10,332 | 13 | physics_contact_runtime | [ ] |
| 30 | 22830 | 9,880 (0.5%) | 704 | 4 | source_model_render | [ ] |

Still to measure: Zanzibar 4p and Coagulation 1p/4p censuses (the ordering may
shift with players), and names/roles for each entry point.

## Already native (for reference)

Main frustum cull and main command walk (40E80), seat candidates, the
presentation posing lane, GPU skinning, leaf kernels 145560/144CF0, plus the
native replicas listed in the 4K60 goal document.

## Status

- 2026-11-09, r14: frozen source inventory
  `6a071b5fa7d083d905d93ec13dc636ea96bd3a147ba1e46097e0358c08372577`
  passes 106/106 component checks, the full engine and all eight exact-input
  Coagulation/Zanzibar static/moving 1p/4p accounting runs. Runtime SHA-256:
  `e62314ec0a96cf65f1410b092d8e54c026e8723746aaab5ce8ebfde188a2bf17`.
  Relative to r12, raw observations fall by 25,122,274; all eight frame hashes
  match and raw activation/registration balances pass. The queue retains
  2,769 literal candidates and 66 unresolved image/forwarder identities.
  The whole disposable source/build/capture owner is removed; no large
  artifacts are retained. Reports and exact comparison:
  `build/test-results/native-execution-hot-queue-r14-20261109/`.
  Later post-child/slab/leaf/promotion/simplex candidates are outside this
  freeze. Full native-only, timing, campaign/all-map, Original-frame and
  launcher qualification remain open; no runtime was selected or promoted.
- 2026-11-09, r13: frozen source inventory
  `9b28462132f4917a9b924189979d947dac35d317957f49e540d4c61648a6c657`
  passes 98/99 component checks. `native_bsp_leaves` stops at an isolated
  sibling-module import in the new object-node binding helper; engine build
  and runtime accounting were not started. Exact authentication pins remain
  enforced. The entire disposable source/build owner is removed, with no
  large artifacts retained. Bounded diagnostics remain in
  `build/test-results/native-execution-engine-r13-20261109/`; the import path
  repair passes the exact BSP route test and eleven binding/import regressions
  (`build/test-results/native-object-node-import-repair-20261109/`). The
  subsequent coherent checkpoint remains pending.
- 2026-11-09, r12: frozen source inventory
  `88d98f295469442c7acfe7a00c57534f64b634582bd0514ee6e550ca8ed06f5b`
  passes all 88 components, full engine and eight exact-input runtime accounting
  checks. Runtime SHA-256:
  `8d9589a9bd1634df2af6656f2d6208cf7da186648c4d25c812badf711a1cebf0`.
  Relative to r11, raw observations fall by 7,907,322, with all eight frame hashes
  equal and all activation/registration balances checked. The queue retains
  2,774 literal candidates and 66 unresolved image/forwarder identities. Entire
  disposable owner is removed. Later raw scale/key, retained-observer and
  object/resource work is outside this freeze. Native-only, timing, all-map/
  campaign, Original-frame and launcher qualification remain open. Reports:
  `build/test-results/native-execution-hot-queue-r12-20261109/`.
- 2026-11-09, r11: frozen source inventory
  `7764806e2f312599e919720b0569cb6b74800754d644539397da75ca86c5c785`
  passes all 83 component checks, full engine build and eight static/moving
  Coagulation/Zanzibar 1p/4p accounting runs. The exact runtime SHA-256 is
  `42ba1544d04122790dede12eea63ef119ad0d87327c808fe87464a21235326aa`.
  The source-qualified queue retains 2,775 literal candidates; raw observations
  fall by 6,140,014 versus r9, with all eight exact-input frame hashes equal.
  Full activation/accounting balances and owner cleanup pass. This establishes
  a native-build regression checkpoint, not Original framebuffer or native-only
  closure. See `build/test-results/native-execution-hot-queue-r11-20261109/`.
- 2026-11-09, r10: the next freeze passes 76/83 component checks, then
  stops before the engine build. Seven deliberate dispatcher-mutation tests
  require exact expectations for the stronger PageAllocate rejection; their
  production authentication pins are preserved. The disposable owner is
  removed and only bounded diagnostics remain. The affected registered checks
  are being rerun before the next coherent freeze.
- 2026-11-09, r9: the frozen source checkpoint
  `6a67b5ee5489d8928c1169d113b5eb1385884990608c873cb649e8d50d41afb4`
  passes 79 required component checks, the full engine build and eight bounded
  90-tick Coagulation/Zanzibar static/moving 1p/4p accounting runs. Evidence is
  `build/test-results/native-execution-engine-r9-20261109/`; its entire owned
  source/build/capture tree is removed. Later source includes qualified
  MapPack MOPP AABB admission and newly bound page-allocation, BSP-setup and
  animation-wait parents; those later ports need their coherent runtime gate.
  The goal remains active: substantial translated production execution,
  campaign/all-map coverage, full original-frame parity, performance and
  coherent runtime delivery remain unfinished. No runtime was promoted.
- 2026-11-09: the active migration now has original-hardware component proofs
  for shared heap/raw allocation and async comparison/insertion, plus expanded
  model/cache parents. The completed r4 frozen accounting cohort records eight
  selected 90-tick static/moving 1p/4p runs and substantial remaining literal
  execution. Later source ports include native Effect transform composition,
  node-resource lifecycle, datum allocation/deletion, BSP helpers and real
  platform clock sampling, stack probing and the halfword bit query; their
  separate component proofs are in the checklist. The r7 frozen checkpoint
  passed 65 required component tests, the full engine build and eight bounded
  accounting runs; later source ports remain outside that checkpoint.
  Use the [source-qualified queue](../build/test-results/native-execution-hot-queue-r4-20261109/hot-queue.md)
  for measured route observations and the [full checklist](NATIVE_REPLACEMENT_CHECKLIST.md)
  for component proofs and remaining gates. A separate source audit found
  compiled generated owners outside the earlier filename-based inventory and
  instruction-shaped native-named effects; coverage remains a lower bound.
  Whole-game native closure, later-port runtime integration and launcher
  qualification remain open. No runtime has been promoted by this work.
- 2026-11-07: goal set; census tools in `tools/perf_4k60/`; backlog above.
  First target: the object/physics update trees (DF880, B9180, B9090, B96B0).
  Their trees overlap heavily: a **100-routine common core** (shared with the
  presented-pose relocation B95D0/B9460) executes ~837k instructions per frame
  on Zanzibar 1p, ~45% of all translated work; the five trees together reach
  280 routines / ~1.04M instructions. Hottest core routines: 14CEA0 118k,
  14D0E0 103k, 145560 71k (native since 2026-11-07), 14CDB0 57k, BF0C0 42k,
  1D30F0 40k, 144CF0 38k (native), 16E6A0 34k, BFF20 23k, 1D2F10 20k. Census
  exports (per routine, per executed address, per tree) are in
  `build-perf-4k60/native-replacement/`; the mapping/plan/review workflow
  result is `core_map_plan_review.json` there.
- 2026-11-07, mapping results (corrections to the backlog above):
  - The "trees" and "share" columns are **static** callee closures. Dynamic
    call counts show the common core has one owner: the presented-pose
    relocation B95D0 (164 calls/frame on Zanzibar 1p, all from
    `PhysicsLiveWorldTu15::set_object_position`, driven by the presented-pose
    scope). With the Havok world churn it causes (static bodies removed and
    re-added: 2EC9B0/2EC4B0/2E9CF0/2EAFB0), that is ~0.89M instructions per
    frame (~48%). B9180/B9090 dynamically cost ~23k/~27k, not 173k/158k.
  - B96B0 never executes (its census count is B9770, generated as
    `run_b8630(VehicleState&)`). `census_map.py`/`census_roots.py` only match
    `WorldState|LiveState|State` units, so `ObservedWorldState` (Havok world,
    reports TU1.5 PCs through `world_observation_pc`) is charged to the wrong
    routines (backlog "2EBC70" is really addObject 2EC4B0) and dispatch()
    edges are missing. Fix the tools before signing off any entry.
  - Rank by profiled time, not instruction count: the leaves were cheap per
    instruction (store traffic dominates). Zanzibar 1p main thread
    (46.8 ms/frame): presented scope 25% (Havok remove/re-add under 1CD620 13%,
    BA630 cluster relink 4.7%, BF050 node rebuild 4.4%), object tick 9%,
    machine posing 7%, host collision BVH rebuild 6%. Zanzibar 4p (60 ms):
    scope 19%, object tick 17% (BE4A0 3.6%, first-person observer 3%, BE850
    2.6%, 168CA1 2.2%), machine posing 7%.
- 2026-11-07, landed: host collision BVHs build on first query
  (`HALO2_REFERENCE_EAGER_COLLISION_BUILDS=1` restores eager builds). Moving
  Zanzibar devices recomposed and rebuilt the whole BSP+scenery+device tree
  every tick. Zanzibar 4p 15.5 -> 17.0 fps (CPU 63.2 -> 58.2 ms/frame), frames
  identical, collision tests pass in both modes.
- 2026-11-07, presented-pose relocation consumption (diagnostics
  `HALO2_RELOCATION_CHANGE_CENSUS`, `HALO2_EXPERIMENT_RELOCATION_RESTORE`,
  `HALO2_EXPERIMENT_RELOCATION_POISON`, `HALO2_RELOCATION_MOVED_DIAGNOSTICS`):
  per frame the relocation changes ~10k bytes of the physics-pool heap (Havok)
  and ~6k bytes of physical allocation pages (object/cluster tables); its
  stores into the persistent resource owners rewrite identical bytes.
  Restoring every changed byte except the root/sphere words, restoring only
  Havok, only physical pages, and skipping the relocation for every root were
  all frame-identical on Zanzibar (endpoint cases and `--motion
  --render-fraction 0.5`). Presented roots differ from the simulation
  placement only by rounding (<1e-4) for static crates/scenery/items; movers
  (bipeds, vehicles, machines, settling items) differ more.
  Landed (default): childless, attachment-free roots within 1e-4 of their
  simulation placement are presented in place (root position written
  directly); all other roots keep B95D0. 127 of ~165 roots per frame on
  Zanzibar. `HALO2_REFERENCE_PRESENTED_RELOCATION=1` relocates every root.
  Frame-identical against that switch on 12 cases (Zanzibar and Coagulation;
  4:150, 2:240, 1:100 with `--motion --render-fraction 0.5`, plus the three
  default endpoint cases). A/B (interleaved, CPU monitor): Zanzibar 4p
  17.1 -> 20.0 fps (57.9 -> 49.4 ms/frame), Zanzibar 1p 23.7 -> 28.9 fps
  (41.1 -> 33.6 ms/frame). With the lazy collision trees, Zanzibar 4p went
  15.5 -> 20.0 fps this session.
- 2026-11-07, landed: machine LOD skin sharing. Every Zanzibar machine (big
  wheel, house gate, flywheels, blades, gate control) holds one identical
  bank in all six LOD slots, and each pose skinned all six (per tick and per
  frame). Skin-identical slots (same vertices, bindings, bind poses, compound
  nodes) now reuse the first slot's skin; each slot keeps its own copy and
  materials. `HALO2_REFERENCE_MACHINE_LOD_SKINS=1` skins every slot,
  `HALO2_VERIFY_MACHINE_LOD_SKINS=1` skins the aliases too and rejects any
  difference (0 over 370 frames at 4p). Frame-identical on the six Zanzibar
  cases; Zanzibar 4p 20.1 -> 21.4 fps (49.4 -> 46.4 ms/frame). Session total
  for Zanzibar 4p: 15.5 -> 21.4 fps.
- 2026-11-07, landed: retained native scenery views (`SourceSceneryScene::
  retain_source_view`) skinned the whole LOD model once per drawn section and
  view with the same WORLD nodes. One `ModelWorldPoseCache` per scenery LOD
  now serves them (node bytes plus the fresh skin's FP/errno effects key the
  reuse, as for machines); `HALO2_REFERENCE_SCENERY_VIEW_SKINS=1` skins per
  draw. Frame-identical on 12 cases (Zanzibar and Coagulation, motion and
  endpoint); Zanzibar 4p 21.2 -> 21.9 fps (46.6 -> 45.4 ms/frame).
  `shader_vertices` also reuses the previous vertex's part binding and
  admission result within a run (two map searches per vertex before).
- Found while testing (pre-existing, both routes): Zanzibar 1p with the
  `--motion` script at fraction 0.5 aborts at tick ~107 with "Selected BSP
  light material was not admitted during preload"
  (src/rendering/world/source_bsp_effect_lights.cpp:933), likely a grenade
  explosion light on a BSP material whose light/material slot was not
  prepared.

## Coordination note for the Codex session (2026-11-09, from the Claude perf session)

Codex owns the routine-by-routine native replacement
([checklist](NATIVE_REPLACEMENT_CHECKLIST.md)). To avoid overlap, the Claude
session is staying on host-side performance work that does not replace
translated bodies, and will not edit generated units, generators,
`physics_live_world_tu15.cpp` routing or the files under
`docs/native_replacement/` that Codex wrote.

Claude-owned, in the working tree now:

- `src/rendering/shadows/source_presented_model_pose_tu15.cpp`: a presented
  root whose shown position/forward/up/scale are bit-identical to its
  simulation placement is installed in place (no B95D0), with or without
  children/attachments. `HALO2_REFERENCE_PRESENTED_RELOCATION=1` relocates
  everything. Frame-identical on the six Zanzibar cases; Coagulation pending.
  Also diagnostic `HALO2_RELOCATION_PATH_CENSUS=1` (per-class relocation time).
- `tools/perf_4k60/profile_run.sh`: `H2_KEEP_LINES=REGEX`;
  `census_map.py`/`census_roots.py` match every `*State` unit.
- Next Claude target: `SourceMachineScene::Impl::pose` /
  `SourceObjectGeometry::pose_world_lod_view` vertex copies (host code only).

Measurements Codex can use when ordering its queue (Zanzibar 4p, main thread,
2026-11-08): translated code is 40% of samples. Per presented relocation call
class (`HALO2_RELOCATION_PATH_CENSUS`): three welded type-11 crates took 43%
of relocation time through the 1DC270 0x400 full-rebuild path (now presented
in place); remaining 3.6 ms/frame is bipeds 26%, vehicles 46% (two parked
vehicles take the fixed-body remove/re-insert), machines 13%, loose weapons
13%. Poisoning every Havok-heap byte the relocation changed left frames
identical (render does not read it); poisoning physical allocation pages
fails at once ("source collidable list refers to absent object generation").
Specs from the Claude mapping run: `native_replacement/B95D0_B9460_relocation_driver.md`,
`BA630_cluster_reconnect.md`, `BF050_node_rebuild.md`,
`1CD620_havok_set_position.md`, `RELOCATION_NATIVE_PLAN.md`. Tick time by
entry: B8F40 4.6% (DF880 1.9, 1179C0 1.3, C9010 0.8), B9090 2.8%, 168CA1 2.5%,
23670 PRT 1.6%, 3DAF0 1.4%, 15EBA0 1.1%, 1ADF60 0.9%.

If Codex takes over any file listed above, note it here.
