This is the pre-correction investigation snapshot. The subsequent fixes, performance controls and opening playback evidence are documented in [the renderer correction report](RENDERER_CORRECTNESS_20261018.md).

The renderer has a confirmed projection-arithmetic defect that can move a vertex to the wrong screen position at camera-plane W=0. It is present in the launcher-selected world shader. I also found an unconnected original transparency-ordering path, a transparent-lighting lifetime defect, and a remaining ordinary-world depth/clipping parity gap. None of these observations alone identifies the fleeting stretches reported in multiplayer and campaign cinematics: that exact frame has not been captured and replayed.

This analysis was performed on October 18, 2026, against the current checkout. Production renderer/gameplay code, shaders, assets, installed runtimes and existing dirty work were left unchanged. New files are this report and small investigation sources/receipts under `analysis/renderer-flicker-audit-20261018/`.

The authority is original Xbox/TU1.5, with original cache programs and authenticated original instruction/declaration evidence. The repository's pinned xemu shader translation is an explicitly identified arithmetic/raster reference; it does not establish physical NV2A precision. The owned disc executable hashes to `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`. The TU1.5 declaration fixtures authenticate executable `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Retail-only instruction evidence is not relabeled as a freshly executed TU1.5 routine.

Both root launchers currently select `build-multiplayer/halo2_native`, with MCC cache `assets/imported/mcc-zanzibar-restored-20261013`. Its existing native-test receipt identifies runtime SHA-256 `915a647cc7397809e10e59dc8720b874bedd4e3720b09f79209c0f4b485c4322`; that receipt is not proof of which executable produced the user's earlier symptom. The selected world vertex module was independently read and hashed during this investigation.

1. **Confirmed: the GPU clamped reciprocal mishandles both signs of zero.**

   `shaders/vertex_program.glsl:144` implements RCC as `sign(c.x) * clamp(abs(1/c.x), 2^-64, 2^64)`. GLSL `sign(0)` is zero. Thus the operation returns zero for both +0 and -0. The production CPU interpreter, `src/vertex_program.cpp:154`, takes the sign of the reciprocal and returns the correctly clamped finite value. The retained xemu `_RCC` and `clampAwayZeroInf` likewise preserve the reciprocal's signed result.

   A fresh Vulkan compute probe included the unchanged production GLSL interpreter and compared it with the production CPU interpreter and the literal retained xemu clamp. On the RTX 5080, two of ten controlled operands disagree; the ordinary finite, signed, infinite and endpoint controls agree:

   | Input | CPU / retained translation, float bits | Production GPU, float bits |
   | --- | --- | --- |
   | +0 | `5f800000`, +2^64 | `00000000`, zero |
   | -0 | `df800000`, -2^64 | `00000000`, zero |
   | +1 / -1 | `3f800000` / `bf800000` | Same |
   | +infinity / -infinity | `1f800000` / `9f800000` | Same |

   This affects real authored position programs, not just an invented instruction sequence. The unchanged original `transparent_generic_reflection` class-0 payload is retained at `analysis/multiplayer-shading/fp-reflection-source/class0.vp`, SHA-256 `a37e3ae948caee7a6b00ddeb389fca00117dfeedc5adb2da27e37224734b6207`. Its instruction 26 takes RCC of the projected W; instruction 41 uses the result in final screen XYZ. With a controlled camera bank and vertex on W=0, that complete program produces CPU `oPos=(5.90296e21,4.42722e21,0,0)` but GPU `oPos=(320,240,0,0)`. Controlled vertices at W=+0.015625 and -0.015625 produce matching positions. These are explicit diagnostic camera/vertex inputs, not a captured gameplay pose.

   The report also verifies the installed `build-multiplayer/shaders/world.vert.spv`, SHA-256 `2344e88597faad6aee643861fc44b1669d0b2240423bd5e65b145931f663932b`. Its disassembly contains the same `FSign(c.x)`, reciprocal, absolute clamp and multiply. The finding therefore reaches the selected shader artifact, independently of whether the runtime executable matches every current C++ source file.

   The all-cache census finds 346 distinct RCC-containing compiled payloads among 353 distinct vertex payloads. That is authored availability across classes and families, not a count of currently visible affected draws.

   **Symptom relevance:** highest among the confirmed geometry defects. A vertex collapsing toward an unrelated screen location can produce a narrow triangle. The measured defect requires zero W; the probe does not establish an error at arbitrary small nonzero W or prove that the reported frames reached zero. A source-exact reciprocal correction should preserve signed zeros/infinities and be checked at the real clip boundary, rather than masking vertices with a guessed distance threshold.

   Receipts: `reciprocal-report.json`, `installed-shader-report.json`; sources: `reciprocal.cpp`, `reciprocal.comp`, `authored_position.comp`, `run_reciprocal.py` in the investigation directory. The GPU run enabled the Khronos layer and synchronization validation and reported no diagnostics. Its exit status is intentionally 1 because the production implementation disagrees.

2. **Confirmed integration gap: ordinary gameplay/campaign geometry bypasses the recovered original transparent ordering.**

   The original ordinary-part path is `40E90 -> 44AC0 -> 45CE0 -> 41040`, followed by the shared `1E370` drain and the original `13DF80/13E140` sorter. It orders source command priority, owner/part depth and optional planes; it is not equivalent to sorting material IDs. The recovered code and evidence are documented in `docs/SOURCE_TRANSPARENT_SORT_PARITY.md`.

   The renderer can consume a completed queue through `ModelView::world_transparent` / `foreground_transparent`. `src/renderer.cpp:6270` activates it only when supplied, and `:5769` then suppresses duplicate ordinary layer-15 submission. Without it, the `:5836` draw loop visits the already grouped draws; `:6713` performs the after-fog draw before any separately sorted effect commands.

   The normal `src/main.cpp` campaign view construction, around `:2706`, supplies geometry, objects, particles and lighting but no transparent queue. The multiplayer view construction around `src/multiplayer_runtime.cpp:5211..5328` likewise does not publish one. A repository search finds explicit geometry-queue producers in focused renderer tests, not these normal caller paths. Reading optional banks in material-selection code does not produce them. Contrail/light-volume sorting in the renderer does not insert the missing ordinary model/BSP commands.

   **Symptom relevance:** wrong blending/occlusion and abrupt overlap changes on transparent object surfaces, glows and cinematic scenery are plausible. This does not by itself move a vertex or demonstrate a stretched triangle. The missing work is the actual original per-part command/admission producer and shared drain, including owner centers/planes and callback lifetime; an approximate camera-distance sort is not source parity.

3. **Confirmed latent defect: a transparent draw keeps lighting from a destroyed temporary binding.**

   `src/renderer.cpp:4182..4186` creates a local vector and copies the caller's `SourceTransparentDrawBinding` values into it. At `:4255`, `TransparentStep::lighting` points into the copy's inline `optional<ObjectLightingState>`. The local vector dies at `:4259`. At `:5133`, the saved pointer is passed to `alias_owner`; `view_material_uniform` then passes the referenced state to `apply_object_lighting_bindings` for authored material passes.

   `SourceTransparentGeometryDraw::vertices` is a span into caller-owned geometry, so copying this variant does not itself free those vertices. The proven dangling object is its inline lighting state. It would be incorrect to describe this finding as a proved freed vertex buffer.

   A source-matched ownership seam using the production variant/types reliably reports AddressSanitizer `heap-use-after-free`. Its owned-value control returns the expected ambient value with no ASAN failure. The runner checks the relevant expressions still exist in the production source before compiling the seam.

   A full current-renderer ASAN replay was attempted but was not completed: the selected build lacks its generated catalog, another retained catalog has an incompatible ABI, and the reduced optional-catalog build reaches a missing existing archive, `libhalo2_mcc_lens_bitmap_layout.a`, at link. The retained blocker receipt prevents the seam result being mistaken for a full renderer run. No shared build was altered to work around these missing inputs.

   **Symptom relevance:** unstable lighting when a sorted transparent geometry binding contains lighting. This is currently a latent path relative to the ordinary callers in finding 2, and is not established as the user's geometric stretch. It must be corrected before relying on wider sorted-queue integration. Keep the lighting value in frame-owned storage or retain the binding owner through consumption.

   Receipts: `transparent-lifetime-seam-report.json`, `transparent-renderer-build-blocker.json`; reproducible seam: `run_lifetime_seam.py`. The seam runner returns success only after its owned-value control passes and its bad-lifetime control produces the expected ASAN diagnostic.

4. **Confirmed parity boundary, unmeasured symptom: ordinary depth/clipping still differs from the exact source packet path.**

   The main attachment is already integer D24S8 and `world.frag:108` already uses the corrected `worldD24Depth` encoder. Earlier notes about D32 or division by 16,777,215 are historical and do not describe the current code.

   The generic world path still interpolates `titleDepth` and clamps its value to the entire D24 range before encoding. In contrast, the retained original-translation path in `menu_world_fragment.glsl:56..69` evaluates source clip regions, reconstructs W using ordered per-triangle positions, includes the supplied slope/offset, and applies the supplied W clip interval/mode. `menu_world.geom` and `world_depth_geometry.glsl` preserve the required triangle inputs. Shadows/raw world packets use that separate explicit contract. The emblem variant separately calls `worldWDepth` at `world.frag:115`; ordinary world draws do not automatically get it.

   Ideal perspective interpolation can represent the same mathematical W, so this code difference is not proof that every ordinary fragment has wrong depth. Finite precision, near/far acceptance, EQUAL overlays and adjacent triangles require measured comparison. Existing evidence already records that interpolation can cross an integer boundary on emblem edges. Current generic `ForwardRasterState` also does not carry the complete source packet's W clip/region/slope inputs; filling them with constants would hide missing ownership.

   **Symptom relevance:** depth-edge shimmer, clipped/sliver coverage and overlay disagreement deserve direct examination. No matched ordinary-object frame or new exhaustive depth-interpolation measurement was produced here. The earlier resource migration and source encoder fixes must be preserved while isolating this remaining boundary.

The following checks constrain other possible explanations without claiming a complete visual certificate:

| Area | Current audit result and limit |
| --- | --- |
| Model topology | The loader uses the Reclaimer-style face/index-count plus first-ten-degenerate-triples heuristic. The original draw consumer instead also uses actual unit flag 0x200 and per-part flag 8; see retained `4D5E7..4D5FD` instructions and `source_effect_light_opaque.cpp:129`. Across 37 supplied gameplay caches, 35,295 nonempty model sections, 76,008 parts and 439 explicit list overrides, zero overrides conflict with the loader's decision. This rejects that particular alleged mismatch; it does not certify every topology or MCC model. Counts include repeated authored models across caches and unselected LODs/permutations. |
| Indices and triangle expansion | `model.cpp:533..548` and `cache.cpp:403..416` validate ranges and vertex indices, skip repeated-index degenerates and preserve the strip parity counter. No demonstrated out-of-range triangle or strip-parity reset was found. |
| CPU skinning | The real scalar/SIMD comparison passes, including complete vertex bytes, influence ordering, special values, retained WORLD poses and 403 original model fixtures across 21 maps. This tests the transform implementation, not live animation-controller chronology. |
| Original stream declarations | Fresh Lockout and Cairo skinned-geometry checks pass against authenticated TU1.5 declaration/compression fixtures. Character/first-person CPU input-adapter checks pass. These do not certify every MCC conversion or physical GPU output. |
| Frame storage | Fresh frame-reuse/upload-plan and material-selection tests pass. The keyed upload plan compares complete raw streams; geometry spans are intended to remain alive until upload. The audited ordinary caller containers survive the immediate draw. No demonstrated borrowed-vertex use-after-free was found. |
| GPU synchronization | CPU geometry planning may overlap the previous submission; the ordinary frame fence is waited before mapped GPU writes/resource replacement, and these vertex buffers use HOST_VISIBLE + HOST_COHERENT memory. An obvious ordinary CPU overwrite/flush omission was not found. This is source inspection, not a full stressed GPU validation run. |
| Raster states | Forward pipelines honor selected depth-test/write/comparator and cull state; sky explicitly uses depth clamp/no cull. A blanket 'culling is disabled everywhere' diagnosis would be wrong. |
| Pose interpolation | Current code interpolates parent-local transforms, rebuilds the hierarchy and retains exact endpoints. Cinematic render gaps have a dedicated test, but it was not rerun in this investigation. No evidence here identifies a one-frame pose/lifetime discontinuity. |
| Materials, textures, effects | Current code retains original programs, declarations, authored mips, lightmaps and separate passes. The old diffuse-only description is obsolete. Particles, decals and contrails have separate renderer services; they were inspected as consumers but their live offending-frame geometry was not captured. Their absence from the bounded probes is not a clean bill of health. |

Fresh CMake validation ran through `tools/run_tests.py`, with all eight selected checks passing and no skips:

```
python3 tools/run_tests.py --target frame_vertex_reuse_tests \
  --target material_frame_selection_tests \
  --filter '^(native_frame_vertex_reuse|native_material_frame_selection)$' \
  --output analysis/renderer-flicker-audit-20261018/frame-tests

python3 tools/run_tests.py --target model_skin_simd_tests \
  --target source_skinned_geometry_tests --target source_cpu_skin_inputs_tests \
  --target render_view_tests \
  --filter '^(native_model_skin_simd|original_tu15_skinned_geometry|original_tu15_campaign_skinned_geometry|source_cpu_skin_inputs_cpu|campaign_character_vertex_cpu|render_view_resolution)$' \
  --output analysis/renderer-flicker-audit-20261018/model-tests --jobs 2
```

The additional diagnostics use owned scratch storage:

```
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 analysis/renderer-flicker-audit-20261018/run_reciprocal.py
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 analysis/renderer-flicker-audit-20261018/run_lifetime_seam.py
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 analysis/renderer-flicker-audit-20261018/inventory.py
```

Each receipt retains commands, source hashes and bounded results. `inventory-report.json` also hashes the original map/resource inputs. The successful focused CMake checks do not catch the new RCC failure because their controlled cases do not cover this zero-reciprocal position boundary. They also do not make the exact user-visible flicker green.

The recommended correction order is the demonstrated RCC defect, the transparent lighting ownership defect, then source-backed transparent-queue integration and ordinary W/clip comparisons. Reproduce the visual symptom with a fixed scene/camera trajectory and frame-by-frame projection/draw records before assigning the observed flicker to any one finding. A transient-spike capture should retain object/section/part, pre/post-skin positions, current node-palette identity, projected W/oPos, selected material/pass and final draw range, so position corruption can be distinguished from depth/blending artifacts. Existing `HALO2_VERIFY_SIMD_SKINNING`, `HALO2_VERIFY_FRAME_GEOMETRY` and `HALO2_VERIFY_BORROWED_FRAME_VERTICES` controls can constrain that capture; a high-rate screenshot of only the final good frame cannot.

No runtime fix or launcher replacement was made for this analysis. Generated binaries, shader modules and private build trees were disposable. Only small investigation source files, JSON receipts, logs and this report are retained; no new large binary, bank, frame or render capture was retained. The final cleanup receipt is `analysis/renderer-flicker-audit-20261018/cleanup-report.json`.
