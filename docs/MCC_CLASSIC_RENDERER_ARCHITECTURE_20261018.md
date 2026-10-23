# Halo 2 Classic rendering architecture: MCC versus OG Xbox

Research date: 2026-10-18. Scope: Halo 2 **Classic** campaign/multiplayer rendering, compared with original Xbox Halo 2/TU1.5. Anniversary graphics, Halo 2 Anniversary multiplayer, Halo CE, and asset appearance are outside the comparison. This is a bounded source/binary investigation, not an MCC performance capture or a complete reverse engineering of `halo2.dll`.

**Evidence update, 2026-10-23:** The later [native callsite audit](MCC_CLASSIC_PERFORMANCE_REFERENCE_20261022.md) supersedes this report's import-only boundary for backend mechanisms: it confirms compiled deferred-context/command-list methods, binding-state suppression, indexed-instancing submission and retained immutable/default buffers with an upload ring. Its [retained disassembly](../build/test-results/mcc-classic-performance-reference-20261022/binary-evidence.json) has SHA-256 `27ab015f5f71e0b77ea6888945aec54e5618aec83d05ec0d2013ea35a131a165`; the existing DLL was rehashed unchanged as `de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946`. The older table and open questions below describe the October18 evidence, rather than absence of those compiled mechanisms. Active Classic workers, live instance grouping, ordinary skinning stages, frames in flight and a matched explanation of4K120 remain unverified. Microsoft's [command-list contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-executecommandlist) allows merging on another deferred context but requires an immediate context for eventual GPU execution; this API capability does not identify MCC's live receivers or schedule. No MCC launch or new disassembly was performed for this update.

## Finding

There is evidence for a native x64/D3D11-era Halo 2 implementation in MCC, changes to presentation/interpolation, and continuing repairs to Classic rendering behavior. The inspected primary material does **not** establish that Halo 2 Classic was rebuilt around parallel draw submission, GPU visibility selection, compute skinning, indirect draws, or any particular number of frames in flight. Calling it a modern GPU-driven rewrite would exceed the evidence. Calling our present Vulkan scheduling and CPU preparation costs unavoidable features of original Halo 2 would also exceed the evidence.

The original game already has a meaningful CPU/GPU split: CPU-side object admission, render queues, LOD/cache decisions and state setup coexist with GPU vertex programs that process local/skinned geometry. The relevant question is how efficiently each implementation prepares, retains, submits and overlaps that work, not simply whether it uses a GPU.

## Evidence comparison

| Area | OG Xbox evidence in this checkout | Actual MCC Classic evidence and boundary |
| --- | --- | --- |
| Executable/backend | Authenticated retail and TU1.5 executable instruction evidence; GPU-facing state/declaration owners are identified below. | The installed `halo2.dll` inspected today is AMD64 and contains D3D11 device-creation names and valid Shader Model 5 DXBC containers. This establishes backend code presence, not the complete live world-render path. |
| CPU visibility/geometry selection | The retained original ordinary-object path walks cluster-linked spatial lists, runs portal/frustum/fog tests and builds/reorders object queues. Original cache owners retain per-view LOD and geometry stamps. | No primary source located specifies whether Classic retains, parallelizes or replaces these particular owners; GPU culling is unproved. |
| Skinning and geometry transforms | TU1.5 local/skinned compiled vertex programs consume original indices, stored weights and matrix-palette registers. The original is not a wholly CPU-skinned renderer. | A `render_model_skinning_disable` string exists in the DLL. A string does not establish where skinning runs, how palettes are uploaded, whether vertices are recomputed every frame, or whether compute/vertex skinning is used. |
| State handling | Original state externs select literal descriptors, conversions, cached state and SDK packet routines. Bounded complete owner comparisons cover retail through TU1.5. | D3D11 changes the backend contract, but no inspected primary source describes Classic's state cache, batching policy, resource lifetime or redundant-state suppression. |
| Threads/submission | The original evidence establishes particular CPU owners and GPU packets, not a complete threading census. | `CreateThread` is imported. This does not identify a render thread, worker jobs, deferred contexts or parallel command recording. No such claim is admitted. |
| Frame pacing/interpolation | Original presentation and simulation/render timing must be recovered from their own owners. | A developer's own MCC account describes new FPS settings requiring camera/object interpolation, resolution-dependent postprocess rewrites and latency/optimization work. It covers multiple engines, so it is not a per-owner Halo 2 threading specification. [Developer account](https://ca.linkedin.com/in/rdev0). |
| Render behavior | OG Xbox TU1.5 remains this project's visual authority. | 343's Season 8 notes explicitly identify fixes to Halo 2 Classic crossfades, shadows, weather, bloom, fog, lighting, foliage and bump mapping. These establish changed/repaired implementation behavior; they do not document the scheduling architecture. [Official patch notes](https://support.halowaypoint.com/hc/en-us/articles/4407245081620-Halo-The-Master-Chief-Collection-MCC-Season-8-Patch-Notes-October-2021). |

## Original-code evidence and its limits

The pinned TU1.5 XBE is SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. The retail XBE is `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`. Retail-only addresses or captures below are explicitly not promoted to a complete TU1.5 owner correspondence.

* **CPU admission and queues, retail evidence:** `132220` visits collidable and noncollidable cluster chains; `133050` evaluates frustum/plane/fog conditions; `163C80` appends queue columns. Later `134240` promotes PRT owners and `133790` produces fade flags. The evidence is a reconstruction over retained original state, not proof of our live scene's visibility. See [source-owner findings](SOURCE_OBJECT_RENDER_QUEUE_PARITY.md), [original-state audit](../tools/audit_shadow_main_collector.py), and [unchanged-code differential](../tools/differential_source_object_render_queue.py).
* **Cache/LOD, retail evidence:** `3D270` creates the 256-record object render-state pool; `3DDD0` finds/rebinds owner records; `4B775` checks per-view geometry reuse using LOD and frame/view stamps. This demonstrates persistent original render bookkeeping, not that our native implementation currently obtains equivalent reuse. See [owner contract and source boundaries](SOURCE_RENDER_CACHE_PARITY.md).
* **GPU local/skinned vertex processing, TU1.5 evidence:** the original compiled classes 2–6 consume original interleaved geometry, byte UNORM indices/weights, node maps and palette-relative reads `c50..169`. Original compiled programs own their weight arithmetic. The retained unchanged `1C310` declaration and `1CDB0` compression evidence supports that input contract; controlled Vulkan comparisons do not establish physical NV2A arithmetic or live pose/cache ownership. See [TU1.5 owner and vertex evidence](TU15_SKINNED_LIGHT_VERTEX_20261013.md) and the retained [original declaration fixture](../tests/fixtures/tu15_skinned_vertex_declarations.original.json).
* **State/packet handling, cross-version static evidence:** retail `1A170` corresponds to TU1.5 `1A150`; immediate state `3F5CF0` to `3FFDD0`; fog-color cache/packet `3F5370` to `3FF450`; line-width cache/packet `3F5730` to `3FF810`. The seven descriptor rows and 92 immediate method words match. The source inspector compares complete reachable instructions, branch/call topology and referenced literals; it does not execute an entire title-update frame. See [complete retained byte comparisons](../analysis/multiplayer/game-variants/render-primitives-title-updates-20261011/report.json) and [source inspector](../tools/inspect_render_primitives_title_updates.py).

These owners refute a simplistic claim that original Halo 2 necessarily expands every model into CPU world-space vertices and rebuilds all GPU resources every frame. They do not by themselves prescribe an optimized replacement scheduler.

## Fresh MCC binary inspection

Read-only inspection of the existing, owned installed file:

```text
/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll
size: 15,807,960 bytes
SHA-256: de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946
PE machine: 0x8664 (AMD64)
valid embedded DXBC containers: 55
SHDR/SHEX version tokens: 53 pixel shader 5.0; 2 vertex shader 5.0
names: D3D11CreateDevice; D3D11CreateDeviceAndSwapChain
import: KERNEL32!CreateThread
string: render_model_skinning_disable
```

The DXBC count was checked against container length, chunk count, chunk offsets and chunk extents, then `SHDR`/`SHEX` version tokens. Microsoft's [token-format source](https://raw.githubusercontent.com/microsoft/DirectXShaderCompiler/main/include/dxc/Support/d3d12TokenizedProgramFormat.hpp) defines the shader-stage enum and version masks used for decoding. No file was extracted or modified. Shader roles and caller reachability were not mapped. In particular, these 55 embedded programs are not claimed to be the complete Classic world shader library; absence of a compute program in this bounded scan does not prove absence of compute anywhere in MCC.

The inspections ran as `python3 tools/run_test.py -- python3 -c ...`, using Python `pathlib`, `hashlib`, `struct`, and `objdump -p`. The initial two wrapped commands and a final combined repeat returned success. The decoder walked each `DXBC` occurrence, validated its header/chunks, and counted `(token >> 16, (token >> 4) & 15, token & 15)` for `SHDR`/`SHEX` chunks. The file hash also matches the older [retained independent header inventory](../analysis/frontend/vista-menu-vectors/mcc-menu-match-audit-20261013-final/embedded-header-probe.json).

This is evidence about the inspected PC build. It is not a claim about every historical MCC build or Xbox-specific backend.

## Why DX11 alone does not answer the architecture question

Microsoft documents both immediate and deferred D3D11 contexts. Deferred contexts can record command lists on additional threads, while playback uses the immediate context. Individual contexts require serialized access. Those are API capabilities and constraints, not evidence that a particular application uses deferred recording. Consequently, a D3D11 name in the binary cannot prove that MCC Classic parallelizes draw generation, eliminates CPU bottlenecks, or keeps the GPU continuously occupied. [Immediate/deferred rendering](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render), [threading rules](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-intro).

Microsoft's Halo 2 [dynamic-light guidance](https://learn.microsoft.com/en-us/halo-master-chief-collection/h2/guides/guidedynamiclights) discusses portal/cluster influence, repeated illumination passes, shadow buffers and light fade/LOD controls. This supports a layered, pass-dependent Halo 2 lighting design exposed through the official tools. It does not specify MCC Classic's internal task graph or certify the runtime implementation of every documented option.

## Our custom Vulkan renderer is a separate implementation

The current checkout's [renderer](../src/renderer.cpp) explicitly performs CPU-owned planning/staging before waiting on the previous frame fence in the normal path (`HALO2_REFERENCE_SERIAL_GEOMETRY` selects the comparison ordering). The fence then protects descriptor/mapped GPU writes and resource lifetime. The frame records commands and submits one command buffer with that fence. Relevant inspected locations are the planning/fence split near lines 3812–3819 and 4301–4305, and submission near lines 7072–7085; line numbers may move during ongoing work.

That is our current resource/scheduling policy. It is neither a demonstrated original-Xbox requirement nor a demonstrated MCC Classic policy. Its GPU shaders do execute graphics work, while substantial CPU preparation/submission can still limit throughput. Low frame rate alone does not identify the limiting stage. No live GPU-utilization measurement, MCC GPU capture, or matched MCC benchmark was collected for this report.

## Remaining architecture questions

The inspected primary sources do not settle the following for actual MCC Halo 2 Classic:

1. Whether visibility, object preparation, sorting or draw recording is distributed across CPU workers, and where the synchronization barriers lie.
2. Whether it uses deferred D3D11 contexts, how many contexts/command lists exist, and how much submission/state overhead remains.
3. Static/dynamic vertex residency, instance batching, palette upload frequency, and CPU versus vertex/compute skinning for each geometry class.
4. Exact retained/replaced portal, BSP, object-frustum and occlusion paths; GPU-driven or indirect visibility/submission is unproved.
5. Frames in flight, upload-buffer rotation, resource ownership and stall causes.
6. GPU utilization, CPU/GPU overlap and per-pass cost on the user's hardware and the scene being discussed.

Answering these needs a version-pinned, authorized live capture of **actual MCC Classic**, with thread/CPU timing plus a GPU/API trace and attribution of the Classic passes. A matching scene/resolution and recorded graphics mode are essential. Such evidence would explain MCC's implementation; performance changes in this project would still need source-behavior and OG Xbox TU1.5 visual checks.

## Implementation implications: exact v621 CPU/GPU model seams

This follow-up is source inspection only. It does not establish that skinning is the measured bottleneck. Fourteen inspected production files match the measured runtime's retained [v621 source hashes](../analysis/opening-cinematic-20261015/current-cutscene-runtime-v621/source-sha256.json): `main.cpp`, `source_campaign_model.cpp`, `source_campaign_view_tu15.cpp`, `source_live_model_geometry_tu15.cpp`, `source_dynamic_model_shaders.cpp`, `model_pose.cpp`, `model_skin_vertices.hpp`, `menu_world_source_binding.cpp`, `menu_world_source_geometry.cpp`, `menu_world_native_draw.cpp`, `menu_world_source_storage.cpp`, `source_vertex_declaration.cpp`, `source_model_node_bank.cpp`, and `source_model_palette.cpp`. All fourteen hashes were explicitly compared before this section was written; later production edits require another comparison.

### CPU base model path

`v.source_cinematic_model` in [main.cpp](../src/main.cpp) is a `SourceCampaignModel`, rather than a separate `SourceCinematicModelTu15` type. At the inspected line 2367, the cinematic-object branch calls its collector overload with actual deferred rendered WORLD nodes sampled through `ModelNodeInterpolation`. The actor branch at line 2521 calls the ordinary-draw overload with retained main units.

Both eventually enter [SourceCampaignModel::vertices](../src/source_campaign_model.cpp), lines 107–123. This allocates a temporary `RenderModel`, copies its bones/compound-node definitions, scans the full expanded resource vertex bank for **each** requested section, copies the selected vertices and skin bindings, then calls `skin_model_world_nodes`. Ordered repeated section requests deliberately produce repeated output. The ordinary-unit overload memoizes a posed section inside one draw call, but each distinct section still enters that palette/pose path separately.

[skin_model_world_nodes](../src/model_pose.cpp), lines 279–284, constructs an identity mapping and copies the WORLD node bank. Its internal `skin_model`, lines 237–267, rebuilds inverse-bind `Transform` objects, composes the original-style palette/compound nodes, copies the selected vertex bank again and runs [SIMD CPU vertex skinning](../src/model_skin_vertices.hpp). The SIMD path deliberately preserves descending influence order and separate multiply/add rounding; a faster path must preserve those semantics.

The GPU packed input adapter subsequently accepts only declaration `class_index == 0`: [source_world_vertex_declaration_supported](../src/source_vertex_declaration.cpp). It handles the already posed position/basis and supported other semantics, with exact-payload policies for skinned-normal normalization and shield local-position texture inputs. It does not expose BlendIndices/BlendWeights. Switching that packed path to an original class2–6 program without a new complete input contract is invalid.

### Existing raw GPU path and recovered inputs

The separate raw path already performs original local/skinned vertex work on the GPU for admitted model illumination draws. [SourceModelEffectLights::draw](../src/source_model_effect_lights.cpp) builds `MenuWorldSourceDraw` from resident raw geometry and the actual selected palette, then calls [menu_world_source_binding_class](../src/menu_world_source_binding.cpp). [SourceBspEffectLights](../src/source_bsp_effect_lights.cpp), near lines 421–440, supplies those palettes from the actual current live model geometry.

| Recovered component | Concrete contract |
| --- | --- |
| Geometry classifications | Section geometry classes1–3, distinct from compiled vertex program classes0–6. Raw class2 permits rigid-boned or skinned section geometry; compiled classes3–6 require skinned section geometry. |
| Local/skinned classes2–6 | Class2/3 consume one bone influence; classes4/5/6 consume two/three/four. The actual compiled program and literal declaration are loaded by class, rather than inferred from a material name. |
| Original position/index/weight streams | [Raw geometry decoder](../src/menu_world_source_geometry.cpp), near lines 178–206, retains original formats1–10, their byte strides/offsets, UNORM8 index/weight fetch, alignment gaps, source order and node maps. It does not normalize weights or complete the first weight on the CPU. |
| UV and basis | Float or signed-normalized UV formats24/25; packed signed11/11/10 normal/tangent/binormal format27. |
| Palette selection | [source_live_model_palette_rows_tu15](../src/source_live_model_geometry_tu15.cpp) follows current TU1.5 `4D820/15280` c50 selection, including rigid single-matrix and region-remapped ranges. It validates live arena bounds and owner inputs; a geometry token alone is not a persistent pose identity. |
| Constants | Binding copies up to40 actual three-row matrices to `c50..169`, plus decompression rows `c170..172`. Consumed indices must lie within the current palette. |
| Vertex code | [menu_world_vertex_source.cpp](../src/menu_world_vertex_source.cpp) has a bounded ARL/address-register and relative c50..52 corridor; it rejects unsupported opcodes, pairings or destinations. A declaration or class number alone does not admit every program. |
| GPU storage/submission | [MenuWorldSourceStorage](../src/menu_world_source_storage.cpp) retains immutable geometry GPU buffers and ordered index banks by section/range. [MenuWorldNativeDraw](../src/menu_world_native_draw.cpp), lines411–463, uploads per-draw constants, binds those vertex/index buffers and uses `vkCmdDrawIndexed`. |

This machinery is not yet a universal ordinary cinematic-base model bridge. Raw declaration binding exposes Position, BlendIndices, BlendWeights, UV and the basis streams; declarations requiring additional stream semantics are rejected by the current raw loader/binder. Complete original material pass selection, shader catalog coverage, external attribute components, lighting/PRT inputs, raster state, fog and source ordering must also be joined for any proposed ordinary-base migration. [menu_world_native_packet](../src/menu_world_native_packet.cpp) already enforces exact program/declaration catalog identity and consumed constant/attribute sets.

One specific unresolved family remains: [source_cpu_skin_input_transform](../src/source_cpu_skin_inputs.cpp) rejects reflection class6 because its original payload aliases the three-influence class5 program. That does not invalidate class6 support for separately admitted program families; it forbids inventing a four-influence reflection interpretation.

### Safe candidate optimizations, conditional on stack evidence

1. **Preindex immutable section runs.** Replace repeated whole-resource scans in `SourceCampaignModel::vertices` with retained section vertex/skin spans or ordered runs created at resource load. Keep selected LOD, order, duplicates, full identities and failure behavior. This changes lookup/copy preparation, not visibility, animation or shader arithmetic.
2. **Avoid per-node temporary vector allocation.** [source_model_node_bank.cpp](../src/source_model_node_bank.cpp), lines24–27, calls vector-returning `source_model_skin_palette` on a one-element bank for each node, then uses `.front()`. This occurs in native original owners `1376C0` (`source_model_skin_node_bank`) and `137800` (`source_model_geometry_node_bank`). A one-matrix compose/flatten helper returning `std::array<float,12>` can preserve the exact existing arithmetic without that allocation. Dynamic source-bank reads and loop-count rereads must retain their order.
3. **Reuse immutable hierarchy preparation and one current pose palette.** In the CPU presentation adapter, inverse-bind reconstruction and identity mappings come from immutable model data. The ordinary actor overload also recomputes the current palette per distinct selected section. Separate palette creation from vertex skinning so one owned WORLD sample supplies all selected sections, while preserving LOD-specific compounds and exact transform order. This is distinct from the original mutable render-arena owner and must not alter its mutations.
4. **Reuse shader-selection results for identical inputs.** [SourceDynamicModelShaders::select_parts](../src/source_dynamic_model_shaders.cpp), lines69–77, walks all authored parts and allocates a map. `append_selected` repeats it for each ordinary draw unit. Reusing a result only for the same complete collector/view/material-bank inputs within the owned call can avoid repeated work. Coverage and view-specific LOD decisions must remain independent.
5. **Deduplicate owned main snapshots within a frame.** `SourceLiveModelBaseSelectionTu15::copy` deep-copies parts/materials/full palette metadata and selected palette rows per unit. Exact frame-local shared snapshots may reduce copies for repeated matching owners/sections; they must preserve immutable ownership before secondary queries mutate the arena. Neither a model tag nor an arena token alone is a sufficient persistent cache key.

These are source-observed opportunities, not claimed speedups. Original-owner checks already exist as `original_model_geometry_node_bank`, `retail_source_model_palette`, and `retail_source_model_palette_assets`. CPU geometry changes need complete ordered vertex/binding comparisons on actual selected sections and poses. A GPU-base migration additionally needs complete input/program/pass comparisons and live OG Xbox TU1.5 pixel acceptance. The active profiler must decide which seam receives implementation work first.

## Source exclusions and storage receipt

### Follow-up: mapped renderer storage ownership

The parent investigation observed transient flat/unlit/white images in X11 window captures of the opening sequence. This source audit does **not** establish that those images were completed GPU presents, nor that the capture system caused them. Its narrower question was whether native source-world bindings permit writes through the renderer's mapped material/vertex allocations before the next frame fence.

No such write-through route was found in the inspected production code:

* [mount_native_tu15_renderer_startup_data](../src/native_tu15_renderer_startup_data.cpp) publishes source-addressed CPU regions with explicit owners and ReadOnly/ReadWrite policy. Its `mount(span)` overload in [SourceRuntimeMemory](../src/source_runtime_memory.cpp) copies the supplied bytes into an owned vector. These startup regions are not views of `Renderer::Impl::material_mapping` or mapped vertex storage.
* `Renderer::constant_mode_control`, `output_state_control` and `texture_coordinate_control` return `NativeRuntimeAddress` receipts naming actual backing. The material receipt points to the mapped material allocation; the extended-constant receipt points to **CPU** `vertex_bank_uniforms.data()`, not the mapped GPU `vertex_bank_memory`. [Renderer implementation](../src/renderer.cpp), inspected near lines7232–7247 and7367–7397.
* A targeted repository search for `material_backing` and `extended_backing` found production consumers checking nonzero ownership/configuration, plus test diagnostics. It found no production dereference, writable span construction or source-memory mount from either receipt. [Native constant selection](../src/native_tu15_renderer_constants.cpp), [output selection](../src/native_tu15_renderer_output.cpp), [coordinate selection](../src/native_tu15_renderer_coordinates.cpp), and [particle binding validation](../src/source_particle_graphics_binding_tu15.cpp) do not write through those addresses.
* `NativeRuntimeAddress` itself is a typed integer, not a memory-permission mechanism. The stronger source-memory boundary is `SourceRuntimeMemory::source_address`: native accesses must resolve to an indexed region actually owned by that memory instance. Mutable views then require ReadWrite access and pass resource/write guards. An arbitrary returned renderer address does not thereby become mounted source memory.
* `Renderer::update_material_constants` writes CPU `material_uniforms`, `vertex_bank_uniforms`, texture-frame selections and dirty metadata. Actual mapped material/extended-bank copies occur inside `Impl::draw`, after its prior-frame fence wait. The normal pre-fence geometry-planning interval contains validation and CPU preparation; the inspected direct Vulkan writes and subrenderer `prepare` calls occur after that fence. Swapchain recreation first waits for device idle. [Renderer implementation](../src/renderer.cpp).

One separate API genuinely publishes into mapped GPU uniforms outside `Impl::draw`: `RendererParticleView::Use::publish` reaches [ParticleRenderer::publish_view](../src/particle_renderer.cpp), which writes camera and clip fields. It is not an unguarded general material pointer: `Renderer::prepare_particle_views` first waits on the renderer frame fence; owner-thread/revision checks retain the specific allocation; `publish_view` accepts only a unique unrecorded prepared slot; camera-only slots cannot record geometry; and recording advances the revision. Repository production `main.cpp`/`multiplayer_runtime.cpp` have no calls to `prepare_particle_views` or `particle_view_inputs`; inspected callers are focused tests. This is an actual mapped publication seam, but it is not established as an opening-sequence race.

`Renderer::update_image` also provides an out-of-draw resource-update API. Its ordinary path queues CPU pixels for post-fence upload; the serial upload and shared-image/extent replacement paths explicitly wait for queue idle before modifying existing GPU resources/descriptors. No confirmed out-of-fence host write into storage still read by the prior submitted opening frame was found by this bounded audit.

These findings do not support the proposed **native source-binding write-through path**. They do not exclude every possible host/GPU race, descriptor problem, render-pass error or presentation/capture issue. A decisive comparison should correlate actual submitted/presented frame identities with a GPU-owned final-color readback or API capture; X11 window pixels alone do not establish that correspondence. The parent is editing material-update performance separately; this audit made no production changes.

The often-cited 2021 Roman Lebedev shader interview points to the former [official Waypoint forum post](https://forums.halowaypoint.com/t/mcc-development-flighting-update/5019/5). During this investigation it redirected to the forum farewell. A secondary article reproduces parts of it, mixing CE and Halo 2 discussion; it was used only to locate the original and is **not** a foundation for architecture claims here. Likewise, CE Anniversary engine-stitching accounts and H2A multiplayer material were excluded from the Classic comparison.

No build, game launch, render capture or performance test was performed by this research task. The wrapped binary inspections generated no retained binary, shader, object, image or memory dump. The final combined repeat printed its private directory `/tmp/halo2-test-cq0qnpb0`; an outer post-exit check confirmed that directory no longer exists. The source-pin follow-up used `/tmp/halo2-test-glpmornc`, also confirmed removed after exit. The only new retained deliverable is this small Markdown report. Existing original assets, reference evidence, DLLs, runtimes and unrelated work were preserved.
