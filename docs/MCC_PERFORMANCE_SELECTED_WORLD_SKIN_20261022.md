# Selected WORLD skin: one remaining cache bypass

2026-10-22. Initial static audit of the active frozen source at `/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source` (F below). Sections through the initial source hashes describe that first inspection. The final section separately pins the parent's subsequent source refreshes and the linked normal x64 images. This research agent launched no compiler, build, game or profiler; its later `nm`/`objdump` helpers ran through the existing owned command session. The performance goal remains unmet; the separate LTO experiment has not admitted a production setting.

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

`model` is the validated `visual.world_lods->at(selected.lod)`, never `visual.model` or another preview LOD. No new skin provider implementation is needed for the component experiment: F4199–4202 already supplies `unit_frame_poses.update(owner,lod,model,nodes)`, using the synchronous span contract at [F public header:46](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/include/halo2/source_live_model_geometry_tu15.hpp:46). The same cache can return a miss/rebuild if a later source query changed the nodes. The pseudocode is the mechanical reuse seam, not the strict FP admission implementation: a strict selected-only lambda/forwarder would need the receipt policy below while ordinary-base calls retain their default policy. Do not add an early empty-section return that skips existing model/pose validation.

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

### Assessment of a conservative post-MXCSR receipt

The proposed receipt can supply a **conditional terminal SSE sticky-flag admission rule**, without clearing flags or synthesizing exceptions. Intel documents MXCSR status bits 0–5 as sticky and mask bits 7–12 as set for masked exceptions ([Intel SDM Volume 1, §10.2.3.1](https://cdrdv2-public.intel.com/843827/253665-sdm-vol-1-dec-24.pdf#page=252)). The following is an inference from that hardware rule: for a fixed, deterministic skin kernel with identical immutable inputs and arithmetic controls, masked SSE exceptions only add sticky bits. If an eligible successful fill records `post_flags = post_mxcsr & 0x3f`, a later request with `(current_mxcsr & post_flags) == post_flags` already contains every flag observed after that fill. Under those assumptions, repeating the arithmetic cannot add a missing bit. Recording the full post-mask, including flags inherited by the fill, is conservative: an inherited unrelated bit can cause an unnecessary rebuild, not an unsound hit. This is a static argument, not verified compiled-code evidence.

Required gates and implementation traps are:

- Require all six SSE exception masks, `(current_mxcsr & 0x1f80u) == 0x1f80u`, and the existing exact geometry/control key. Record both pre/post controls and reject receipt eligibility if they changed during the fill. Matching controls plus this mask condition must hold at the eligible fill and the hit; a prior scalar or VERIFY fill cannot produce a usable SIMD receipt.
- Require scalar/VERIFY diagnostics off at **both** fill and hit. Current environment values alone cannot establish how an older entry was built. Keep the existing fresh reference route and make the process/environment stability assumption explicit. Rechecking these flags does not reproduce all effects of running their original reads inside the skipped function.
- Publish receipt validity with the completed key, vertices and retained-node bank only after success. Missing metadata is a miss. Exceptions during skinning or the retained-node copy must not relabel the previous entry. Capturing immediately after the skin rather than after all successful miss bookkeeping also needs an ordering/side-effect audit; an allocator is an external call.
- A strict rejection must actually rebuild. Calling the existing `update(model,nodes)` as fallback would immediately accept its ordinary geometry/control hit and skip the skin again. A strict overload needs a shared explicit fresh-build path, with hit/build counters reflecting the real outcome. Successful ordinary misses can record eligible receipts for later strict selected requests, while the default overload retains its present hit behavior.

The visible arithmetic closure is `skin_model_world_nodes`/`skin_model` → palette/compounds → `compose_transform` and the SSE2/scalar vertex kernel ([F model_pose.cpp:279](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/rendering/models/model_pose.cpp:279), [F source_model_palette.cpp:6](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/rendering/models/source_model_palette.cpp:6), [F transform.cpp:36](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/core/transform.cpp:36)). It contains floats and explicit SSE intrinsics, with no explicit x87 arithmetic, `long double`, FP-environment clearing or math-library call. That does **not** prove the linked x64 closure has no x87 effects. Inspect its actual instructions and callees, including allocator/diagnostic paths, and verify that the post-MXCSR read remains after all relevant arithmetic. `-ffp-contract=off` and the pose file's `-fno-fast-math` constrain arithmetic but do not by themselves prove receipt capture ordering or complete fenv observability. The initial static phase had no disassembly; the follow-up below addresses instruction choice/order for two exact linked images.

The receipt also does not settle errno, allocation/new-handler effects, `bad_alloc` timing, arbitrary diagnostic changes, or other x87 status/control effects. The host skin closure has no `SourceRuntimeMemory` observer callback, so skipping it does not directly skip an original memory observer; this narrower fact does not discharge those C++/FP effects. Full observational equivalence remains unproved. Extend the direct negative controls with each sticky bit absent/present, inherited unrelated flags, cleared flags after a fill, changed masks/rounding/DAZ/FTZ, scalar/VERIFY fill-to-hit transitions, failed fill/retained-copy publication, and exact fresh versus strict-build counters. Compare terminal SSE **and** x87 state, errno, all vertex bytes, selected ordering and downstream draws. Admit no gain from an all-miss guard or from a fixture that never reaches the selected-world path.

The existing [F model_skin_simd_tests.cpp:83](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/tests/model_skin_simd_tests.cpp:83) is a useful component starting point: it checks one build/eleven reuses across four views/three sections, distinct owners/LODs, changed nodes, rounding and FTZ/DAZ, plus complete posed vertex bytes across403 authored models. It does not check this selected-section lambda or full FP/errno effects. Extend a scratch-only component driver with the unchanged F4214–4241 section routine as reference and the provider variant above. Warm it through an ordinary-base-style request first, then test body/right/left requests, section order such as`{2,0,2}`, absent selections, absent sections/errors, distinct model identities and generation/LOD/node changes. Count provider builds/hits specifically at the selected-world seam; an unexecuted or all-miss cohort proves no eliminated work. Compare every `Vertex` byte and the exact output/order/error before timing modest fixed work.

The existing [F unit_skin_reuse_runtime_tests.cpp:58](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/tests/unit_skin_reuse_runtime_tests.cpp:58) compares ordinary source-base submissions using `unit_source_draw_observer`. That observer alone would miss a regression in selected body/held/shield inputs. Extend it with the existing `animation_observer` (F5670) and relevant final draw/image checks, actual1..4 views, different view LOD/regions, attached/secondary weapons, shields, death/respawn and changed nodes. Retain source-built ordered collector/pose/palette receipts and the existing original-backed builder/palette/vertex-input regressions; component equality is not physical-Xbox framebuffer proof.

For an isolated performance comparison use the unchanged frozen executable versus a candidate with existing unit-base reuse enabled in **both**. The all-repeat-unit flag disables an already admitted optimization as well, so its FPS difference cannot be attributed solely to this new seam. Most expected benefit is selected third-person/multiview work; the first-person camera-dependent skin path is outside scope. No repeat count, saved CPU time or FPS gain is claimed here.

## Scoped files and hashes

A future narrow implementation would touch the selected-world lambda in `src/gameplay/multiplayer/multiplayer_runtime.cpp`, its direct component/runtime regressions and their test registration. If the strict receipt is proved, also scope `src/rendering/models/model_world_pose_cache.hpp` for receipt eligibility/forced rebuild and `model_frame_pose_cache.hpp` for strict forwarding/counters. It needs no change to the original model builder, skin kernel or geometry identity key. Keep default cache-hit behavior scoped; unresolved FP/allocator findings can still block the extension. This note does not authorize broad cache redesign.

The audit read the runtime, the two pose-cache headers, `model_pose.cpp`, `model_skin_vertices.hpp`, `source_live_model_geometry_tu15.{cpp,hpp}`, `source_model_build.{cpp,hpp}`, `source_model_palette.cpp`, the relevant frozen live-world builder/observer boundaries, and the named tests/profile records. Source hashes below are relative to F:

| Source | Frozen SHA-256 |
| --- | --- |
| `src/gameplay/multiplayer/multiplayer_runtime.cpp` | `49149d1c6cceb54deb6634c7f40ce18cd92694da258f957d6c7117659bdfd11d` |
| `src/rendering/models/model_frame_pose_cache.hpp` | `36b4a83262ebded53844052954e168bcf0506ed0f17c15b35adf6bb468b0c185` |
| `src/rendering/models/model_world_pose_cache.hpp` | `46b8c32b8e451761ccb63328eb8bd11cf06caf37ccfc8febd751fc6f343d6b29` |
| `src/rendering/models/model_pose.cpp` | `7a5a8f2042390e93286f1482885b3cbadfd02a55c6f01469216ee4d61457651c` |
| `src/rendering/models/model_skin_vertices.hpp` | `852ae52b48a8adbfa33806487f80c76e5686239fcf1aa46a4f1bf6bc04172f0b` |
| `src/rendering/models/source_live_model_geometry_tu15.cpp` | `41626b00566c303a1cf7f25081a0dee31656d8f88c6b350882cabe86637020cc` |
| `include/halo2/source_live_model_geometry_tu15.hpp` | `b9070f4a548e2d799e9a240701b85f59f98a741b979771aa6ad443045f880c17` |
| `src/rendering/models/source_model_build.cpp` | `57cbd0b5187538f5448069c9095c25981de81a8a16690c71fc100ca4c8334a81` |
| `src/rendering/models/source_model_palette.cpp` | `8ca0694aa069cc0d5a0f8ed83cd86faa12cb7b8195f4ba5723496d4fa88e531b` |
| `src/core/transform.cpp` | `b0f45198199258ca9802474fd0258c2ed2e44556d940efd6eaa907c557cbffbd` |

Canonical current hashes match the two caches, pose/skin implementation and live geometry helper. Canonical runtime differs (`365a47eb43d3a2ba2c522654a6d61dc11284a92e3e5e857a414311379bf6f4c5`); its selected-world direct skin remains at4246, but unrelated material/network/shield work has drifted. Canonical `source_model_build.cpp` also differs (`3278cc32e96118418559a81005202687814619e638ea06a113da6e1ec1f0bab7`). These are drift observations only; the analysis and line citations above use F.

## Linked normal x64 follow-up

Two read-only inspections ran in the active `tools/run_test.py` owner, queued through `build/test-results/mcc-performance-goal-20261022/runtime-attempt3/commands.pipe` after the parent's builds. The helper used `nm -S --defined-only`, `c++filt`, then bounded `objdump -d -w -C -M intel --no-show-raw-insn --start-address=... --stop-address=...`. Exact mangled/demangled symbols, instruction addresses, call targets, capture excerpts, source hashes and every command are retained in these compact reports:

| Image | SHA-256 | Bounded report |
| --- | --- | --- |
| Initial selected-skin integration, **before** receipt-order tightening | `33adc3fc35c0583a0fcaca8a434350ecb620d3db7b244f8de0362922b6c35889` | [initial JSON](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-performance-goal-20261022/selected-skin/linked-x64-audit.json), 48,101 bytes |
| Tightened normal integration | `861e1f997c615c67c85201ab1af89ef7ea73eb942a315f885812b9586fef4def` | [tightened JSON](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-performance-goal-20261022/selected-skin/linked-x64-audit-tightened.json), 48,935 bytes |

The second inspection covered 14 complete functions / 15,716 machine bytes, including the four skin/palette cold clones and bounded direct cache/STL callees. No root exceeded the budget; no inspected function had an indirect call/jump. This is the executable's internal closure, **stopping at PLT/external calls**, not a complete audit of libc/libstdc++ or interposed implementations. The helper confirmed that the executable hash stayed unchanged during each inspection. The tightened [integration result](/home/eric/Developer/Halo-2-Decompiled/build/test-results/mcc-performance-goal-20261022/selected-skin/integration-result.json) independently records the same candidate hash and success; this instruction audit is not another game or performance measurement.

Final normal-image symbols (virtual addresses in that linked ELF):

| Symbol | Start | Bytes | Actual internal arithmetic/calls |
| --- | --- | --- | --- |
| `halo2::skin_model_world_nodes` | `0x0a1879a0` | 674 | Builds mapping/WORLD copy; calls anonymous `skin_model` at `0x0a187b6e` and `0x0a187bf5`. No FP arithmetic in this wrapper. |
| `halo2::(anonymous namespace)::skin_model` | `0x0a1825e0` | 7,876 | Contains inlined vertex kernels, SSE scalar/packed arithmetic and diagnostic branches. Calls palette at `0x0a1826d4`, compounds at `0x0a1826fb`. |
| `halo2::source_model_skin_palette` | `0x0a2dd700` | 989 | Nine static `mulss` instructions; calls `compose_transform` at `0x0a2dd803`. |
| `halo2::append_source_model_compounds` | `0x0a2ddae0` | 841 | `mulss`, `addss`, `subss`, `addps`; vector allocation/copy/delete calls. |
| `halo2::compose_transform` | `0x0a189530` | 772 | Forty `mulss`, 27 `addss`; no calls. |
| `halo2::ModelWorldPoseCache::update(...,bool)` | `0x009d8090` | 1,568 | Integer/key/receipt logic, FP status reads and explicit external calls. |
| `halo2::ModelFramePoseCache::update` | `0x009ea710` | 294 | Map lookup/insert, world-cache call and counters. |

**Observed:** every inspected skin/palette/compound/transform normal/cold body has zero x87-family instructions and zero MXCSR writes. The cache's only x87 instructions are `fnstcw`/`fnstsw` reads; its MXCSR instructions are `stmxcsr` reads. The vertex functions have no separate emitted symbol/call in this image; their loops are folded into anonymous `skin_model`, which contains `mulps`/`addps` as well as scalar operations. A short actual SIMD sequence is `0x0a1830d6: mulps xmm2,xmm3`, `0x0a1830d9: mulps xmm0,xmm3`, `0x0a1830dc: mulps xmm1,xmm3`, then `0x0a18310e: mulps xmm3,xmm4` and `0x0a183111: addps xmm2,xmm3`. No direct math-library call appears in the inspected closure. These are code-presence/instruction findings, not a dynamic path-frequency claim.

The normal/cold bodies explicitly reach `operator new` (`0x154b10@plt`), sized `operator delete` (`0x1531a0@plt`), `memcpy`/`memset`/`memcmp`, `getenv`, `fegetround`, `__errno_location`, `__cxa_allocate_exception`/`__cxa_throw`/`__cxa_free_exception`, `std::runtime_error`, length errors, stack failure and `_Unwind_Resume`. These external bodies were not disassembled or certified. No original source-memory callback is present inside these inspected skin bodies; the original builder and per-view work remain outside the proposed borrow as established above.

### Actual receipt ordering and remaining limits

The **initial** binary confirms the two source caveats: post `stmxcsr`/`fnstcw`/`fnstsw` at `0x9d82e8`/`0x9d82f5`/`0x9d82f9` precedes diagnostic `getenv` calls at `0x9d8309`/`0x9d8318`, and old-bank deletes at `0x9d8410`/`0x9d8467`. Do not cite that image as evidence for the tightened ordering.

The **tightened** binary gives this normal successful miss sequence, preserving the returned arithmetic before the receipt:

```text
0x9d8200  call skin_model_world_nodes
0x9d822d  call operator new       ; retained WORLD copy allocation
0x9d824b  call memcpy             ; retained WORLD copy
0x9d8290  call operator delete    ; old posed vertex bank, if present
0x9d82b6  call operator delete    ; old retained node bank, if present
0x9d82cb  call getenv             ; scalar diagnostic mode
0x9d82da  call getenv             ; VERIFY mode
0x9d8303  mov eax,[r14]           ; errno value, TLS pointer obtained earlier
0x9d830a  stmxcsr [rsp+0x34]
0x9d8317  fnstcw  [rsp+0x44]
0x9d831b  fnstsw  [rsp+0x46]
0x9d83a6  mov [r15+0x40],rax      ; receipt publication begins
```

The pre-skin diagnostic reads likewise precede raw errno/MXCSR/x87 snapshots. There is still a conditional moved-from temporary delete at `0x9d8422` in the emitted return tail: the successful vector move zeros that temporary's capacity at `0x9d8260` and its pointer/size at `0x9d827e`, so its zero-pointer branch bypasses this call on the ordinary successful path. This distinction matters; the disassembly does not support claiming there are no cleanup/failure calls anywhere after the snapshot.

The final source pins are world-cache header `88218a532386c99f317edca94cad0fd05dc48a8e5925241df09bf1d5ec89b01a`, frame-cache header `db07ee678cb80f365093f4ad48d389ca37075c4c56f8359ab82c288fab568198`, and runtime `b2b80938bafbaf497dc33cf61d0bff2bd9ff540f7b636aa8cb587ead2e42cca7`; the four pose/skin/palette/transform hashes still match the initial table. The initial linked world-cache header was `6b2211d4f26f9a61bac5a312224bad32a2c8925c40e8e5d9d4423e5187c33f1e`. Both reports retain the full pin sets. The new header places diagnostics before snapshots and old-vector retirement before the post receipt at [current owned header:30](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/rendering/models/model_world_pose_cache.hpp:30) and [line 80](/home/eric/Developer/Halo-2-Decompiled/.local/test-scratch/halo2-test-0yux_h0i/source/src/rendering/models/model_world_pose_cache.hpp:80).

This closes the narrow normal-x64 internal x87/instruction-order uncertainty for the pinned image. It does **not** prove unconditional observational parity: hits still omit allocations and their possible `new_handler`/failure timing, cold diagnostic/exception paths and external-library effects. A successful recorded fill does not predict arbitrary future allocator/interposition behavior; the normal stable-environment admission and direct state/output regressions remain necessary. Other architectures, toolchains, changed source, LTO and future binaries require fresh admission. No integrated FPS gain or goal completion follows from this audit.

### Helper lifecycle

No production source, build graph, reference asset or binary was changed by this research helper. It created no binary/object/archive copies. The two JSON reports retain 97,036 bytes of bounded evidence. Temporary selected disassembly remains **only inside the already active owner**: `selected-skin-linked-audit-initial/` (200,610 bytes) and `selected-skin-linked-audit-tightened/` (199,397 bytes); `selected_skin_linked_audit.py` is 7,654 bytes. A crossed FIFO write queued an identical final inspection twice; the helper detects the completed matching binary hash and returns without reinspection. The current unsampled cohort must finish before cleanup commands run. The parent should then retire exactly those two private directories and the helper, verify their absence, and preserve the owner's unrelated build/reference/runtime inputs and the two compact JSON reports. Their continued temporary presence is explicit, not reported as completed cleanup.
