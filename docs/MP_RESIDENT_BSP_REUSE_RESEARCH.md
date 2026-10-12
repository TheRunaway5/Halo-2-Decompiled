# Read-only static BSP residency seam

The complete four-view capture contains **172,452 BSP vertices** whose GPU vertex fields match the already resident raw BSP mesh. Another **144 vertices** require the actual view's stipple level 3. A hybrid resident path could remove those 172,452 unchanged vertices from per-frame packing/upload while retaining the partial-stipple parts dynamically. This is a source-preserving candidate, not an implemented renderer change or measured frame-rate improvement.

## Independently repeated byte evidence

`tools/audit_mp_resident_bsp_vertices.cpp` loads the real Lockout BSP through the current native cache importer and executes the actual `SourceBspShaderScene` resident LOD/lightmap-bank constructor. It then consumes the immutable complete grouping capture from `analysis/multiplayer/integration/render-run-packing-20261008`.

Raw data contains **43,329 vertices and 220 geometry/section/part/lightmap identities**. Each submitted BSP part is matched by exact `(bsp_cluster, source_section, source_part, lightmap)` identity. The audit checks original vertex order/count, constant selected material/stipple within that part, actual resident LOD/lightmap pair declarations, and all 132 bytes of every record after changing only the two fields written by `SourceBspShaderScene::select`: material and stipple level. Every other byte, including all floating-point and PRT values, matches without recalculation.

Two independent executions pass **870 complete part reconstructions and 172,596 captured BSP vertices**, across four real submitted world/object/body calls. They expose 233 distinct geometry/material/stipple combinations. Both complete comparison TSVs hash to `ba6e51a5039b65324e1474d7ce7b22cdb972a629a1b157ccdf9bf25c17bb2af2`.

| Group call | BSP parts | BSP vertices | Material changed | Stipple changed |
|---|---:|---:|---:|---:|
| 3 | 220 | 43,329 | 8,862 | 0 |
| 4 | 218 | 43,185 | 9,213 | 144 |
| 5 | 212 | 42,753 | 9,711 | 0 |
| 6 | 220 | 43,329 | 8,898 | 0 |

868 part submissions / 172,452 vertices retain stipple level 16. Two part submissions / 144 vertices use level 3: call 4, geometry encoding -73, section 12, parts 0 and 1, materials 109 and 110, with 126 and 18 vertices. Cluster geometry contributes 84,588 vertices; authored instance geometry contributes 88,008. The capture's total grouping stage contains 368,076 vertices; the eligible resident set is approximately 46.9% of that specific frame. This is an input-volume measurement, not a predicted time saving.

## Current code seam

`src/multiplayer_runtime.cpp` retains `world_vertices = mesh.vertices` after BSP bank construction. Every view copies that constant vector into its dynamic world and calls `bsp_shaders.select`; water, scenery, source props/machines, other-player bodies and attached weapons are appended separately. `SourceBspShaderScene::select` computes actual per-view admission/coverage/LOD, then erases suppressed parts and changes only material and stipple fields in retained BSP vertices.

`src/renderer.cpp` already uploads the original BSP mesh once into `vertex_buffer`. Its resident draw ranges preserve full authored part/material/lightmap identities. However, full-surface multiplayer views use newly packed `view_draws` in `debug_buffer`; the global resident draw path is suppressed for those scenes.

The Vulkan world vertex declaration contains positions, colors, UV/lightmap UV, normals, tangents, binormals, incident vectors, lightmap index and **stipple at location 11**, plus PRT attributes for extended-bank pipelines. **Material is not a Vulkan vertex attribute.** Selected material/lightmap determine the draw's descriptor and dynamic uniform-bank binding. Therefore the captured material change can remain solely in the per-view draw metadata when using matching raw vertex fields; dropping an actual stipple change would change the shader input. `shaders/world_vertex.glsl` forwards that stipple value and `world.frag` applies the existing stipple discard. No shader change is needed or proposed.

## Requirements before a live resident path

A correct integration needs a per-view metadata plan from the same selection logic, including rejected/suppressed parts, original defaults for parts not touched by selection, and exact geometry/section/part/lightmap identity. The current returned `SourceBspShaderPart` vector contains accepted selections only; it cannot alone distinguish untouched defaults from rejected parts. Do not replace the current selection with an accepted-only list.

The new plan must merge resident BSP tokens with dynamic object tokens in the existing sorted full `(selected material, lightmap, cluster, section, part)` order. Preserve every authored token and within-part triangle order; do not emit all resident geometry before all dynamic geometry merely because it uses a different buffer. Buffer/range selection must follow each ordered token. The constructor's existing resident first/count ranges use its own encounter order, so a lookup must address those exact physical ranges rather than assume the dynamic group's packed offset.

Every consumer of the mixed world draw list must select the correct buffer/range while keeping its existing pass/material/view state: primary world, environment, before/after overlays, planar fog, water, late transparency and bloom. Preserve per-view camera constants, admission, fog attribution, render policy, animated materials, descriptors, material-bank offsets and stipple origin. Preserve alpha-order sky groups separately. Preserve dynamic water, scenery/props, body/weapon, foreground, UI and shadow producers. A changed geometry/stipple input must retain a verified dynamic fallback or an exact resident variant; no source state may be omitted to obtain residency.

Before adoption, compare complete mixed draw-request order and vertex attribute streams against the old live path, exercise partial-alpha and LOD/suppression transitions across real views, then measure actual CPU/renderer timing and GPU captures. This audit proves raw/captured part invariance and the declaration seam; it does not implement or validate that mixed resident draw plan or its hardware output. Only the one captured Lockout frame is checked here, not every map or later authored mutation.

## Freeze and reproduction

Evidence lives under `analysis/multiplayer/integration/resident-bsp-reuse-20261008`: two full comparison TSVs, source snapshots, executable, summary, exact compile/run arrays and immutable manifest. The capture and its original producer are independently pinned by the preceding run-packing freeze. The helper imports that frozen capture reader unchanged. The audit performs CPU/cache reads only; no device creation, Vulkan descriptor handle, driver success, shader replacement or gameplay result is supplied. No production file was changed for this task.
