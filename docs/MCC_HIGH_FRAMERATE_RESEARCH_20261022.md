# MCC Halo 2 Classic high-frame-rate research

Research date: 2026-10-22. This is a bounded investigation of first-party statements, the owned installed `halo2.dll`, and this port's presentation/geometry seams. OG Xbox Halo 2 TU1.5 remains the gameplay and visual authority. No MCC game session, GPU capture, matched performance benchmark, or production-code edit was performed by this research task.

## Findings that affect this optimization pass

MCC provides good evidence for separating high-refresh presentation from older engine update outputs. It does **not** provide inspected evidence for a particular Halo 2 Classic worker scheduler, GPU-culling system, compute-skinning implementation, or palette cache. The current port already has the presentation separation; its measured CPU execution and repeated geometry preparation remain the immediate optimization problem.

The official Xbox Series X|S enhancement document explicitly lists four-player split-screen at 60 FPS, one/two-player 60 or 120 Hz modes, and removal or reduction of original performance throttles. Those are **console feature statements**, not measurements of this PC port or explanations of the renderer. They make the user's targets reasonable comparison goals without identifying the optimization that will achieve them. [Official enhancement features](https://support.halowaypoint.com/hc/en-us/articles/360052059432-Halo-The-Master-Chief-Collection-MCC-Xbox-Series-X-S-Enhancements-Features-and-Troubleshooting).

### Confirmed first-party interpolation evidence

* In May 2020, 343's `343_farn` specifically said the team was applying H2C's interpolation to Halo 3. His adjacent reply distinguished H4, H2A and Reach as harder cases. This confirms interpolation existed in **Halo 2 Classic**; it does not document exact tick frequency, animation coverage, matrix format, interpolation delay, or threading. [Direct developer comment](https://www.reddit.com/r/halo/comments/goui9j/comment/fri9thd/).
* A May 14 status update signed Postums describes an H2 PC projectile regression caused by a local-user interpolation-state access failing to handle user index -1. The author account now appears deleted, while the signed post and discussion remain readable. This is direct evidence that presentation work crossed a shared gameplay-query path and required correction. It supplies a concrete regression category: remote/no-local-user cases must never index local presentation history or feed interpolated camera state into authoritative weapon creation. [Original status post](https://www.reddit.com/r/halo/comments/gjz45n/halo_2_mcc_update_05142020/).
* MCC engineer Roman Lebedev's own project account describes FPS options requiring camera/object interpolation, input-latency work, and a Series X optimization pass. His account covers several Halo engines collectively. It supports the broad presentation strategy, **not** a Halo 2 Classic skinning or multithreading specification. The profile's indexed text was accessible in search; direct page fetch returned HTTP 999 during this investigation. [Developer's project account](https://ca.linkedin.com/in/rdev0).

Increasing simulation frequency is not established here as MCC's method of supporting arbitrary render rates. Do not infer that H2C retained the Xbox 30 Hz rate either: that exact MCC rate and its complete render/update call graph were not recovered by this bounded task. This port's unchanged TU1.5 30 Hz simulation is an independent requirement.

### Three different interpolation owners

The original Xbox animation samplers already accept fractional frames, and original action transitions retain saved local poses. Those facilities are distinct from an adjacent completed-tick presentation history. TU1.5 ordinary node delivery `3D4E0` returns the current world-node bank in its ordinary branch. The retained byte audit does not establish an original previous/current render-history owner. [Original animation owner investigation](MP_ANIMATION_RENDER_INTERPOLATION_RESEARCH.md).

Likewise, the official device-machine `Allow Interpolation` setting describes authored device motion and expressly warns about lag behind actual device position. It is not documentation of MCC's general high-refresh presentation loop. [Microsoft device-machine documentation](https://learn.microsoft.com/en-us/halo-master-chief-collection/h2/devices/devicemachines).

Our native presentation policy already samples completed 30 Hz results, preserves exact endpoint copies and full owner identities, recomposes parent-local hierarchies, and excludes action/event updates from render sampling. It covers body/held-weapon and first-person outputs, scene objects, particles, and completed vehicle cameras. That is the correct place to keep high-refresh work while optimizing CPU cost. Existing documented CPU/Vulkan regressions are bounded evidence; this report did not rerun them or establish new visible acceptance. [Current native presentation contract](NATIVE_RENDER_INTERPOLATION.md).

## Fresh read-only evidence from the installed MCC DLL

Inspected input:

```text
/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll
15,807,960 bytes
SHA-256 de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946
preferred image base 0x180000000
```

The hash matches the earlier [Classic renderer investigation](MCC_CLASSIC_RENDERER_ARCHITECTURE_20261018.md). All addresses below are preferred-image VAs or RVAs; ASLR changes the live image base.

| Retained observation | Address | What it proves |
| --- | --- | --- |
| `game_tick_rate` name | string RVA `B29558`; script descriptor pointer RVA `B32D48` | A named script query is present. |
| Query wrapper | VA `18078BC70`, calls `180706770` | Native code connects that descriptor to a query. |
| Frequency getter | VA `180706770..18070677B` | Loads pointer `[1815FE008]`, then sign-extends the 16-bit field at `+2`. It does not return a baked 120/144/240 render rate. |
| `game_tick_rate_scalar` | name RVA `B32DC0`; descriptor pointer RVA `B32D88`; wrapper VA `18078BC90` | Its getter at `180706780..18070679E` reads the same field and performs signed integer division by 30. This is frequency-related state, not proof of its initialized value or a live render fraction. |
| `render_model_skinning_disable` | string RVA `B38000`; static pointer RVA `E0B918` | A named option exists. It does not locate execution on CPU/vertex/compute stages. |
| `framerate_infinite`, `framerate_debug`, `framerate_use_system_time`, `framerate_stabilization` | string RVAs `B343C0`, `B343D8`, `B343E8`, `B34408` | Option names exist; their static table entries are not evidence of a scheduler or active setting. |

The ordinary ASCII scan found scenario-script interpolator names, not a named general object-history implementation. That absence is not evidence that interpolation is absent. No skin/palette reuse call graph was admitted from this string scan. The previous bounded DXBC inspection found D3D11-era graphics code but did not map the full Classic world shader library. Neither scan proves or disproves compute skinning or parallel submission.

Small evidence reports: [strings and file identity](../build/test-results/mcc-high-framerate-20261022/binary-strings.json), [static descriptor references](../build/test-results/mcc-high-framerate-20261022/binary-refs.json), [exact query disassembly](../build/test-results/mcc-high-framerate-20261022/tick-query-disassembly.json), [inspected local source pins](../build/test-results/mcc-high-framerate-20261022/source-pins.json).

## Concrete implementation priorities for this port

These are **local source observations and engineering recommendations**, not claims about MCC internals. The [Coagulation profile](FRAMERATE_PERFORMANCE.md) attributes roughly 57–71% of sampled CPU time to translated source-runtime execution and around 10% to skinning. Inclusive owners overlap. The present run also observed a nearly saturated main thread and low device-wide GPU utilization. GPU sampling includes competing applications and is not a per-pass GPU capture.

| Priority | Work and preserved boundary | Required evidence |
| --- | --- | --- |
| 1 | Specialize verified literal source-address operands and remove repeated translation bookkeeping where the complete source operation remains identical. Keep checks, mapped-memory ownership, instruction order and callback effects. | Original instruction/register/flag/memory differential; unsampled matched Coagulation runs. This targets the largest measured CPU owner and is independent of MCC's scheduler. |
| 2 | Skin only source-selected vehicle LODs, once per exact frame-local owner/pose/LOD, then select ordered sections. Existing PRT `pose_world` paths can prepare all six banks repeatedly. Retain original LOD, culling, palette and shader producers. | Ordered full-vertex equality, selected-section/LOD coverage, split-screen lifecycle cuts and visible vehicle cases. The parent implementation owns this work. |
| 3 | Preindex immutable section runs and retain immutable inverse-bind preparation. `SourceCampaignModel::vertices` scans its entire vertex union per selected section, copies bones/compounds, and calls a skin path that rebuilds mapping, WORLD-node copies and palettes. One exact sampled pose/LOD can provide a palette to multiple selected sections. | Preserve section order/duplicates, LOD compounds, source vertex origins, baked-lighting instance distinctions and exact arithmetic. Establish cost in profiles before broadening scope. [Campaign model](../src/campaign/source_campaign_model.cpp), [pose adapter](../src/rendering/models/model_pose.cpp). |
| 4 | Expand GPU model skinning through the existing original raw vertex/palette corridor, family by family. Immutable geometry and ordered index buffers already persist in `MenuWorldSourceStorage`; `MenuWorldNativeDraw` uploads constants, binds buffers and calls `vkCmdDrawIndexed`. | Complete original declaration/program/pass admission; compare actual vertex and framebuffer outputs. CPU nodes still own attachment/collision/gameplay queries. [Raw binding](../src/ui/menu_world_source_binding.cpp), [storage](../src/ui/menu_world_source_storage.cpp), [draw](../src/ui/menu_world_native_draw.cpp). |
| 5 | Profile repeated per-view shadow/visibility preparation after the first fixes. Share immutable inputs and exact reusable results only where their owner contract permits it. Consider workers for completed read-only preparation if it remains costly. | Keep per-camera visibility, coverage, lighting, source side effects and final draw order. A shared model definition or arena token is insufficient proof of equal work. No MCC scheduling claim is needed. |

The GPU migration has an important current seam: `source_world_vertex_declaration_supported` accepts only class 0 in the packed already-posed path. The raw path separately admits actual BlendIndices/BlendWeights and current palette rows, with strict shader/declaration validation. Switching the ordinary base draw to a skinned shader by changing a class number would violate that contract. PRT, shields, first-person geometry, reflection aliases and ordered material passes need explicit admission. [Packed declaration gate](../src/rendering/source_vertex_declaration.cpp), [prior exact seam inventory](MCC_CLASSIC_RENDERER_ARCHITECTURE_20261018.md#implementation-implications-exact-v621-cpugpu-model-seams).

Do not reduce authored quality or treat interpolation as a way to skip simulation. Moving an entire measured 10% skinning share away from the CPU has an idealized CPU speedup ceiling near 1.11x before upload/synchronization costs. Substantial source-runtime and redundant-preparation reductions are still needed for 8.33 ms single-player and 16.67 ms four-player complete frames.

## Reproduction and storage

All one-off binary probes ran through `python3 tools/run_test.py -- python3 ...`. They used standard-library PE section decoding, exact 64-bit pointer searches, and streamed `objdump -d` output; only the bounded reports above were retained. Function ranges for the script wrappers were checked against PE `.pdata`; the short leaf getters were bounded at their observed `ret` instructions. A minimal independent query inspection is:

```bash
python3 tools/run_test.py -- objdump -d \
  --start-address=0x180706770 --stop-address=0x18070679f \
  '/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll'
```

The [storage receipt](../build/test-results/mcc-high-framerate-20261022/storage-receipt.json) records report sizes/hashes and the post-exit absence of the recorded scratch owners. No DLL, shader, binary, object/archive, frame, raw stack bank, or capture was retained. Installed runtimes, original assets and existing reference evidence were preserved. The only tracked deliverable from this research task is this document; performance implementation and its acceptance belong to the parent task.
