# Selected WORLD skin: one remaining cache bypass

2026-10-22. Static read-only audit of the active frozen source at `/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source` (F below). No compiler, build, game, profiler or probe ran. Only this note was written. The performance goal remains unmet; the separate LTO experiment has not admitted a production setting.

**Candidate: route `selected_world_vertices` through the frame's existing unit pose provider, borrowing only its full immutable skin.** Keep every selected-section copy, duplicate, missing-section error, shader choice, material offset, lighting, camera and source callback at its present position. This is justified for a bounded differential/component experiment, not yet for production admission: cache hits' FP/diagnostic behavior needs direct proof.

## What is already reused, and what still repeats

The existing sampled `efe69297809e687bb928dba551934e5a4be2aaeffebfc14e24b759463df5bcee` profiles attribute `skin_model` exclusive samples to10.02%/11.43%/8.82% for1/2/4p ([profiles](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-performance-goal-20261022/runtime-attempt3/profiles/4p-profile.json)). They aggregate all skin callers and do not measure this bypass's cost or repeat count.

Already present in F:

- SIMD skinning, preserving descending influence order and separate MUL/ADD, at `src/rendering/models/model_skin_vertices.hpp:49`. Another SIMD proposal would duplicate current work.
- Frame-local vehicle/unit `ModelFramePoseCache` instances and the optional `unit_pose_provider`, controlled by `HALO2_REFERENCE_REPEAT_UNIT_SKINS`, at [runtime:4191](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/gameplay/multiplayer/multiplayer_runtime.cpp:4191). Ordinary live unit geometry uses this provider at4909–4911. Its helper already builds one full skin per completed owner before copying/shading sections.
- Vehicle section/view reuse at5227–5238 and5286–5310; `BodyFramePose` already retains ordinary body/held-weapon poses across views at5603–5657. Loose non-source weapons also have separate frame/retained reuse. None should be advertised as a new opportunity.

The remaining bypass is [F runtime:4214](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/gameplay/multiplayer/multiplayer_runtime.cpp:4214): after validating the selected render-model identity and LOD, `selected_world_vertices` calls `skin_model_world_nodes` directly at4230. Body, primary held weapon and secondary held weapon call it at5664,5666 and5717. It does not consult `unit_pose_provider`, even when the live base request already populated that cache using the same `visual.world_lods` bank at4909–4911. Each additional request therefore executes the skin again; whether its complete inputs equal a previous request is unmeasured.

The smallest candidate uses the **existing** provider type/lambda, plus a local owned-vector fallback whose lifetime spans section copying:

```cpp
std::vector<Vertex> owned_pose;
std::span<const Vertex> posed;
if (unit_pose_provider)
    posed = unit_pose_provider(owner, selected.lod, model, nodes);
else {
    owned_pose = skin_model_world_nodes(model, nodes);
    posed = owned_pose;
}
// Existing ordered section loop remains here, writing its own vector.
```

`model` is the validated `visual.world_lods->at(selected.lod)`, never `visual.model` or another preview LOD. No new provider implementation is needed: F4199–4202 already supplies `unit_frame_poses.update(owner,lod,model,nodes)`. The same cache can return a miss/rebuild if a later source query changed the nodes. Do not add an early empty-section return that skips existing model/pose validation.

## Complete ownership and invalidation boundary

| Input/owner | Required identity and retained behavior |
| --- | --- |
| Cache/world/model residency | `Visual::world_lods` is `shared_ptr<const array<RenderModel,6>>` (F runtime229). It is populated before the frame loop at656–677 from this Cache's authored sections, with hierarchy/checksum admission. The const shared owner pins vertices, bindings, inverse binds, compounds and source-section metadata through the synchronous requests. The cache requires this immutable, alive model contract; a tag/checksum alone is insufficient. |
| Source generation | `ModelFramePoseCache` keys the full32-bit owner plus exact LOD, not a low16 datum index. Different generations/owners and LODs use separate entries. The inner cache also checks exact `RenderModel*`; a different resident bank cannot hit merely because tag names or node counts agree. |
| Pose | `ModelWorldPoseCache` compares every WORLD `Transform` byte and node count, including signed-zero distinctions. No tick/animation name/token/epsilon substitutes for current nodes. The actual completed/interpolated hierarchy is refreshed at F4682–4713, including current parent-hand composition. Secondary/view queries can replace it; the comparison must run on every request. |
| Lifetime | Both frame caches are constructed inside the render-frame loop, before the local-view loop (F4196–4197/4284), and destroyed after the frame. A changed model pointer, WORLD bytes/count or arithmetic control rebuilds. A failed miss publishes neither the new key nor partial output. No cross-frame invalidation mechanism is proposed. |
| Borrow | The returned span is const and valid only until its cache entry changes/destructs. `selected_world_vertices` immediately copies matching records into its independent result before any next update; never retain the span in a later draw or mutate it during shader/material preparation. |
| View, regions, material | They are intentionally outside the full-skin key. Each request still reads its own completed per-view `{model,LOD,ordered sections}` at F4715–4723 and loops sections first, original vertex encounter order second. Repeated sections remain repeated. Shader selection, coverage, masks, lighting, colors, material offsets, shield layers and observer delivery continue on that request's copied geometry. Sharing a decorated draw would require a different, much larger identity and is not proposed. |
| FP | Current cache keys `fegetround()` and MXCSR control bits`0xFFC0` (rounding, masks, DAZ/FTZ). It deliberately ignores sticky flags and does not retain full x87 status/control, errno or an arithmetic-effect receipt. This is an explicit proof gap for extending reuse, not evidence of full FP equivalence. |

The implemented cache contract is at [F model_frame_pose_cache.hpp:8](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/rendering/models/model_frame_pose_cache.hpp:8) and [F model_world_pose_cache.hpp:11](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/rendering/models/model_world_pose_cache.hpp:11). The current source builder already performs its own context/frame/view/LOD/region allocation reuse at [F source_model_build.cpp:68](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/rendering/models/source_model_build.cpp:68). Keep that builder, its cache acquisitions, scratch writes, node observers, rebase/PRT/lighting callbacks and source frame/view scheduling intact. The proposed borrow is downstream host presentation work, not permission to skip an original build/query.

## FP, diagnostics and falsifiable acceptance

`skin_model_world_nodes` checks node count and then builds mapping/palette/output at [F model_pose.cpp:237](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/rendering/models/model_pose.cpp:237). The skin reads `HALO2_REFERENCE_SCALAR_SKINNING` and `HALO2_VERIFY_SIMD_SKINNING` at259–265. A cache hit skips those reads/checks, arithmetic and allocations. Preserve the existing repeat-unit control as the fresh path; do not expand diagnostic elision when scalar/verification modes require fresh work. Runtime-stable environment assumptions must be explicit rather than inferred from a normal run.

Equal vertices do not prove equal terminal FP state. A direct negative control should clear exception flags between identical requests, seed errno/status, and compare fresh versus borrowed execution. Also compare rounding, DAZ/FTZ, exception masks and x87 controls, signed zero/subnormals, and an intervening valid callback that changes controls. Repeated arithmetic can raise sticky flags again; the current cache does not replay them. Unmasked traps, allocation-failure timing and arbitrary model mutation are not established safe by this audit. A mismatch requires a proven admission/fallback boundary or rejection of the candidate; do not silently weaken the contract.

The existing [F model_skin_simd_tests.cpp:83](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/tests/model_skin_simd_tests.cpp:83) is a useful component starting point: it checks one build/eleven reuses across four views/three sections, distinct owners/LODs, changed nodes, rounding and FTZ/DAZ, plus complete posed vertex bytes across403 authored models. It does not check this selected-section lambda or full FP/errno effects. Extend a scratch-only component driver with the unchanged F4214–4241 section routine as reference and the provider variant above. Warm it through an ordinary-base-style request first, then test body/right/left requests, section order such as`{2,0,2}`, absent selections, absent sections/errors, distinct model identities and generation/LOD/node changes. Count provider builds/hits specifically at the selected-world seam; an unexecuted or all-miss cohort proves no eliminated work. Compare every `Vertex` byte and the exact output/order/error before timing modest fixed work.

The existing [F unit_skin_reuse_runtime_tests.cpp:58](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/tests/unit_skin_reuse_runtime_tests.cpp:58) compares ordinary source-base submissions using `unit_source_draw_observer`. That observer alone would miss a regression in selected body/held/shield inputs. Extend it with the existing `animation_observer` (F5670) and relevant final draw/image checks, actual1..4 views, different view LOD/regions, attached/secondary weapons, shields, death/respawn and changed nodes. Retain source-built ordered collector/pose/palette receipts and the existing original-backed builder/palette/vertex-input regressions; component equality is not physical-Xbox framebuffer proof.

For an isolated performance comparison use the unchanged frozen executable versus a candidate with existing unit-base reuse enabled in **both**. The all-repeat-unit flag disables an already admitted optimization as well, so its FPS difference cannot be attributed solely to this new seam. Most expected benefit is selected third-person/multiview work; the first-person camera-dependent skin path is outside scope. No repeat count, saved CPU time or FPS gain is claimed here.

## Scoped files and hashes

A future narrow implementation should initially touch only the selected-world lambda in `src/gameplay/multiplayer/multiplayer_runtime.cpp`, its direct component/runtime regressions and their test registration. It needs no change to the original model builder, skin kernel or cache key. FP findings may block that scope; this note does not authorize broad cache redesign.

The audit read the runtime, the two pose-cache headers, `model_pose.cpp`, `model_skin_vertices.hpp`, `source_live_model_geometry_tu15.{cpp,hpp}`, `source_model_build.{cpp,hpp}`, `source_model_palette.cpp`, the relevant frozen live-world builder/observer boundaries, and the named tests/profile records. Source hashes below are relative to F:

| Source | Frozen SHA-256 |
| --- | --- |
| `src/gameplay/multiplayer/multiplayer_runtime.cpp` | `49149d1c6cceb54deb6634c7f40ce18cd92694da258f957d6c7117659bdfd11d` |
| `src/rendering/models/model_frame_pose_cache.hpp` | `36b4a83262ebded53844052954e168bcf0506ed0f17c15b35adf6bb468b0c185` |
| `src/rendering/models/model_world_pose_cache.hpp` | `46b8c32b8e451761ccb63328eb8bd11cf06caf37ccfc8febd751fc6f343d6b29` |
| `src/rendering/models/model_pose.cpp` | `7a5a8f2042390e93286f1482885b3cbadfd02a55c6f01469216ee4d61457651c` |
| `src/rendering/models/model_skin_vertices.hpp` | `852ae52b48a8adbfa33806487f80c76e5686239fcf1aa46a4f1bf6bc04172f0b` |
| `src/rendering/models/source_live_model_geometry_tu15.cpp` | `41626b00566c303a1cf7f25081a0dee31656d8f88c6b350882cabe86637020cc` |
| `src/rendering/models/source_model_build.cpp` | `57cbd0b5187538f5448069c9095c25981de81a8a16690c71fc100ca4c8334a81` |
| `src/rendering/models/source_model_palette.cpp` | `8ca0694aa069cc0d5a0f8ed83cd86faa12cb7b8195f4ba5723496d4fa88e531b` |

Canonical current hashes match the two caches, pose/skin implementation and live geometry helper. Canonical runtime differs (`365a47eb43d3a2ba2c522654a6d61dc11284a92e3e5e857a414311379bf6f4c5`); its selected-world direct skin remains at4246, but unrelated material/network/shield work has drifted. Canonical `source_model_build.cpp` also differs (`3278cc32e96118418559a81005202687814619e638ea06a113da6e1ec1f0bab7`). These are drift observations only; the analysis and line citations above use F.

No active owner/reference asset was modified or copied. No temporary outputs or large artifacts were created; retention is this small Markdown note only.
