# Architectures we can borrow from MCC Halo 2 Classic

2026-10-23. Read-only installed-binary investigation. This report adds caller, task, queue and completion links to the [October22 backend audit](MCC_CLASSIC_PERFORMANCE_REFERENCE_20261022.md); it does not present those older buffer/state findings as new. Xbox TU1.5 remains this port's gameplay, ordering, arithmetic and visual authority.

**The strongest new result is a connected native render-job architecture:** a named queue is constructed with three requested worker threads; rendering tasks select a worker-indexed backend context, perform rendering preparation, finish command lists, and publish completion events. A calling render path waits and plays those lists in a fixed source order. This establishes substantially more compiled machinery than an isolated `CreateThread` import or `CreateDeferredContext` call. **It still does not establish which path is enabled in an observed Classic scene, three successfully running workers, concurrent execution, or the source of a particular FPS advantage.** The binary-research probes launched no game, GPU capture, compiler, build or benchmark; the separately scoped port trial is recorded below.

## Reference identity and evidence

The existing installed input was preserved and freshly rehashed by every successful probe:

```text
/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll
15,807,960 bytes; AMD64; preferred image base0x180000000
SHA-256 de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946
File/product version1.3528.0.0 (from the earlier identical-hash PE inspection)
```

All numeric code/data addresses below are **RVAs**, not live ASLR addresses. Add the preferred image base to compare with the earlier report's VAs. PE `.pdata` bounds identify instruction owners, but can split one logical function into unwind fragments. A report's `complete:false` explicitly means a retained window, not complete function coverage. In particular, use `callers-audit.json/disassembly/0x1d2f60` and the independent objdump closure for the complete task suffix; the `ownership-audit.json` window of that owner stops before its Finish call.

The [evidence manifest and cleanup receipt](../build/test-results/mcc-classic-architecture-borrow-20261023/manifest-cleanup.json), SHA-256 `326b2ec5e9f31a49d20aa66aa9f284eec048019be63aebda1d7f860f50373e66`, binds the eight reports and exact [probe sources/argv](../build/test-results/mcc-classic-architecture-borrow-20261023/probe-sources.json). Each selected byte range has a hash. Capstone instruction-boundary checks reject raw-byte false positives; independent GNU objdump closures authenticate the important thread/task/wait/playback ranges.

## New: an actual render queue, task payload and worker-context chain

The queue is identified by its own string reference and constructor arguments, not by a guessed function name:

| Link | Direct binary evidence |
|---|---|
| Named queue initialization | `2B5D9A` passes string RVA`BFED20`, `render_multithreaded`; `2B5D94` supplies literal3 as the constructor's worker-count argument. `2B5DA4` calls constructor`0A7C10`, then `2B5DAE` stores the returned queue at global`1A6E568`. |
| Actual thread creation implementation | Constructor`0A7C10..0A804F` iterates the requested count, creates per-worker storage, and calls authenticated `KERNEL32!CreateThread` at`0A7F0C` (IAT`B06428`). This is a thread-creating queue constructor, rather than a string-only lead. Thread creation can fail and live reachability was not measured. |
| Worker implementation | `0A7B00..0A7C05` records the current thread ID, waits through `WaitForSingleObject` at`0A7B7C`, obtains jobs through`0A8360`, and conditionally executes them through`0A8100`. Worker eligibility includes the job's bitmask at`+8`; not every waiting worker is necessarily eligible for every job. |
| Submit and dependency release | `0A8050` writes the owning queue to job`+60`, sets job state`+C`, atomically decrements dependency count`+3C`, queues the job through`0A82C0` only when ready, then signals the queue event. `0A8100` calls the job's native vtable`+8`, marks completion, decrements dependent-job counts, signals a completion event at`0A81F0`, and enqueues newly ready dependents. |
| Context selection by worker identity | `0A8250` compares the current thread ID with this queue's worker records and returns the TLS worker index for a match. Job setup`1D2E60` calls it at`1D2E79`, loads renderer global`1E8D418`, and selects `renderer.contexts[index]` from `+138` at`1D2E88`. The selected context is retained in job`+C8`; its thread-owner field`+318` is claimed. |
| Task-local command-list publication | `1D2E60` invokes selected-context native vtable`+1E0`. In the authenticated context table at`BE9900`, that slot is`107AF0`: it resets local state, retains the caller's output-list wrapper at`+828B8`, and installs context state in TLS. `+1E8` is Finish at`107B40`; `+1F0` is playback at`107BA0`. These are native-class slots, distinct from D3D11 COM slots. |

Sources: [queue constructor, worker and native slot table](../build/test-results/mcc-classic-architecture-borrow-20261023/callers-audit.json), [worker-index/context setup and completion](../build/test-results/mcc-classic-architecture-borrow-20261023/ownership-audit.json), [global queue uses](../build/test-results/mcc-classic-architecture-borrow-20261023/scheduler-audit.json), and [independent complete closures/import identities](../build/test-results/mcc-classic-architecture-borrow-20261023/authenticated-closures.json). The renderer initialization's separate context-array allocation still uses its configured count at`+130`; this investigation does not infer a live count from the queue constructor's literal3.

### The tasks do work between begin and Finish

Task`1D2F60..1D2FF9` calls setup`1D2E60`, reads its payload at`+C0`, passes that payload's fields and the selected context into`1D5100` at`1D2FBB`, then invokes Finish through the selected context at`1D2FD5`. It finally releases the context's thread-owner field. The native task table contains this execute pointer at`BFFED8`; constructors reference its containing vtable at`BFFED0`. This is a queued rendering task with a payload, not merely a context initialization method.

The logical pass range`1D5100..1D559F` sets context state, establishes render inputs through`217F40`, and repeatedly calls`218B80` for numbered subpasses. `218B80` indexes retained tables by the supplied view and subpass, inspects per-pass work counts/flags and dispatches modes1–3 to`21A000`, `21B140` or`21AEB0`. The high-level names and the complete downstream mesh/draw closure remain unresolved. Consequently this report identifies a **render-pass consumer**, without relabelling it ordinary-character, BSP, shadow or skinning work. [Pass range](../build/test-results/mcc-classic-architecture-borrow-20261023/pass-audit.json), [table dispatch and payload setup](../build/test-results/mcc-classic-architecture-borrow-20261023/final-links-audit.json).

A second task,`4710F0..4713E8`, independently selects `renderer.contexts[worker_index]` and retains it at job`+178`. It calls`470630` for subpass arguments0,2,1 at`471186`, `4711AF`, `4711D8`. That callee performs concrete CPU record construction: it requests storage with the literal allocation tag`PART` (`54524150`), allocates `count*72` bytes, packs float/color records with a72-byte stride, and uploads them through the selected context's uploader at`+320`. It also allocates/uploads a36-byte fixed stream. The actual uploader native table at`BE9EA0` maps `+38` to suballocation`10FE70` and `+40` to update`10FEF0`, so these are connected allocator/upload calls. The `PART` tag and record packing are observed; the complete engine type name is not recovered. [Packing and upload callsites](../build/test-results/mcc-classic-architecture-borrow-20261023/pass-audit.json), [uploader table](../build/test-results/mcc-classic-architecture-borrow-20261023/ownership-audit.json).

That task then runs separate pass consumers, including`4719B0`, `248590`, `2486F0`, and finishes three lists at`471291`, `471352`, `4713BA`, using output/state blocks at job`+C0`, `+68`, `+118`. It therefore demonstrates worker-side rendering preparation and several pass-sized lists, rather than an empty command-recording wrapper. Exact rendered families and draw grouping inside those downstream consumers remain unknown. [Complete worker closure](../build/test-results/mcc-classic-architecture-borrow-20261023/authenticated-closures.json).

## New: authenticated enqueue, join and ordered playback

One compiled render path supplies an unusually concrete pattern to borrow:

1. Under the renderer's native feature-query branch, `2E1D21`, `2E1D61`, `2E1DA3` load **the same** queue global`1A6E568` and call submit`0A8050`. The jobs retain distinct output wrappers and completion tokens in the caller's stack/registers. Its alternative path performs rendering inline; live branch selection is unmeasured.
2. At`2E1ED1`, `2E1F02`, `2E1F33`, the caller waits for those completion tokens, passing timeout`FFFFFFFF`. Wrapper`0D9F90` loads token`+8` and calls authenticated `WaitForSingleObject` at`0D9F98`; this join is not inferred from a generic function call.
3. After each corresponding wait it invokes native context playback slot`+1F0` at`2E1EDF`, `2E1F10`, `2E1F46`, in that fixed order, and retires each wrapper through the renderer's release slot. Worker completion order therefore need not become GPU submission order.

Sources: [enqueue/wait/playback closure](../build/test-results/mcc-classic-architecture-borrow-20261023/authenticated-closures.json), [authenticated wait wrapper](../build/test-results/mcc-classic-architecture-borrow-20261023/join-wrapper.json), and [native context slot identities](../build/test-results/mcc-classic-architecture-borrow-20261023/binary-audit.json). The task executor signals its event only after its virtual callback returns; the callback performs Finish first. These links establish CPU command-list completion before playback in this compiled route.

The second family has a related explicit boundary: `470FC8` selects the same named queue; `470FFD` enqueues its prepared task and `471009` signals. Consumer`471FC0` waits on that task event at`472027`, then chooses one of three already produced output lists for playback at`47209B`, `4720E1` or`472124` according to its subpass argument. This is not a claim that those three playback calls all run in a single invocation. [Queue reference/task construction](../build/test-results/mcc-classic-architecture-borrow-20261023/scheduler-audit.json), [complete consumer closure](../build/test-results/mcc-classic-architecture-borrow-20261023/authenticated-closures.json).

The playback receivers load global`1E8D420`, initialized by the primary-context constructor route with deferred-mode argument0 in the older authenticated initialization. Worker receivers instead come from renderer`+138[index]`, constructed with deferred-mode argument1. The bounded exact-reference scan finds a primary-context global initialization store and a zeroing store in the renderer constructor; it does not establish a runtime census of every indirect alias. This resolves the inspected caller routes more specifically than merely observing the same context-field offset`+D58` in Finish and Execute. Microsoft also permits list merging on a deferred context, but eventual GPU execution requires immediate-context playback. [Microsoft contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-executecommandlist).

## Buffer/state/instancing findings that remain useful, with limits

| Mechanism and direct evidence | What can be borrowed | Still unproved |
|---|---|---|
| Persistent immutable/default vertex/index resource support at`110090`; three-way dynamic buffers at`10FC80`, range allocation`10FE70`, update`10FEF0`. New task evidence connects the uploader to actual rendering-record preparation. | Give immutable authored geometry a resource lifetime independent of the frame; stream small changing palettes/constants or true dynamic records through bounded suballocators. Preserve owner generation and synchronize buffer reuse. | Which ordinary MCC character/base meshes use which class; per-frame bytes; ring-rotation cadence; frames in flight. `10FE30`'s modulo3 operation does not prove three frames in flight. |
| Per-context resource-pointer caches and equality skips at`106630..1067F7`, topology cache at`107C1E..107C66`; Execute ends with local state-cache reset. | Keep descriptor/binding/state ownership local to recording contexts, suppress repeated changes, and explicitly invalidate when execution/reset changes backend state. | Live eliminated-call counts and gain. A cache is not safe merely because the CPU pointer matches across recycled owners. |
| Native draw`107BE0` selects DrawIndexed or DrawIndexedInstanced using draw-record word`+26`, calling D3D11 at`107C90`/`107CA8`. | A typed ordered draw packet can represent compatible repeated geometry without expanding it into repeated CPU vertices. | No authenticated instance-count producer or live count>1 was recovered. Ordinary-character instancing, scene grouping and ordering policy are not established. |
| Native getters use direct validated native object lookup and native field loads (health`90991A`, shield`9099BA`). | Validate an owner/span once, then execute a proven hot whole-owner operation natively while retaining required faults and side effects. | These are MCC offsets/behavior, not Xbox TU1.5 replacements; they do not justify globally removing checked memory or observers. |

The old mechanisms and their exact descriptor/call arguments remain in [the original binary evidence](../build/test-results/mcc-classic-performance-reference-20261022/binary-evidence.json), SHA-256 `27ab015f5f71e0b77ea6888945aec54e5618aec83d05ec0d2013ea35a131a165`. The instancing slot search also encountered many unrelated renderer/D3D calls at the same numeric slot. Those candidates were **not** promoted to instance producers. COM/native vtable slot equality alone is insufficient attribution.

## Practical transfer order and required ownership proof

**First, separate immutable geometry identity from changing pose/presentation state.** This is the most direct buffer-pattern transfer where a local path still copies or uploads repeated authored vertices. Retain geometry, original ordinals/bindings and section ordering; update only actually changing data. It needs a complete model/section/LOD/material/owner-generation key, resource retirement behind the actual GPU fence, and source-backed byte/frame comparisons. The binary does not prove MCC's ordinary skin implementation. Port-side existing residency and exact remaining gaps must be checked before counting a new gain.

**Second, copy the queue/context/completion structure for substantial completed presentation jobs.** The useful design is a coarse payload with worker-local context, scratch and upload ownership, explicit dependencies, completion publication, and original-order consumption. Start from immutable completed source outputs; preserve original source callbacks, animation/node publication, visibility, shadow writers and view-specific material selection on their actual owners. The MCC binary demonstrates this separation in its own native architecture; it does not prove that our source arena can be accessed concurrently. FP controls/sticky effects, errno, failure/cancellation behavior and diagnostics remain part of this port's admission contract.

**Then consider worker command recording and instancing where measurements justify them.** Ordered playback can preserve pass order while encoding happens independently, but it does not remove the CPU cost of source scene preparation, repeated geometry expansion or driver synchronization. D3D11 itself requires single-threaded ownership of each context and serialized immediate playback; the analogous Vulkan design needs distinct externally synchronized pools/descriptor ownership. [Microsoft context rules](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render). No measured port or MCC speedup is assigned by this static audit.

## Current port gaps and an implementation order

This follow-up checks the current port separately from the MCC binary. The
[port source pins and verification](../build/test-results/mcc-classic-architecture-borrow-20261023/port-comparison.json)
bind these observations; they are not a newly built complete-runtime acceptance.

We already retain a [BSP vertex atlas](../src/rendering/renderer/renderer.cpp:1186),
[completed owner/LOD poses](../src/rendering/models/model_world_pose_cache.hpp:86),
[borrowed frame upload spans](../src/rendering/renderer/frame_vertex_upload_plan.hpp:48),
pipeline objects, resident material descriptors and
[pooled owner constants](../src/rendering/renderer/resident_draw_storage.cpp:79).
These cannot be counted as missing architecture or new gains. The current
[worker](../src/rendering/renderer/presentation_copy_worker.hpp:8) offloads the
immutable vertex-prefix copy; it does not perform general scene preparation.

The ordinary-character compute route still constructs a new
[frame skin plan](../src/rendering/renderer/renderer.cpp:4205), copies selected
per-view geometry and bindings, packs records/palettes, maps/copies/unmaps the
input bank and updates its descriptors. GPU-containing spans explicitly bypass
[byte deduplication](../src/rendering/renderer/renderer.cpp:4344), because equal
local vertices do not identify equal poses. Thus moving arithmetic to the GPU
has not removed the surrounding per-view construction and transfer work.

The port-specific priorities are:

1. **Execute hot completed owners natively.** MCC's native getters demonstrate
   the execution model, not transferable Xbox field offsets or algorithms.
   A concrete candidate is TU1.5's plane-batched visibility owner
   `2E79A0 → 2E6290`, with endpoint traversals `309600/3092A0`; the
   [live route](../src/physics/physics_live_world_tu15.cpp:1816) still reaches
   its generated instruction implementation. Its
   [original-owner contract](MCC_PERFORMANCE_SPATIAL_OWNER_20261022.md) is
   authenticated. The historical19.14% inclusive profile is not a current4K
   measurement and nests the18.39% recursive cost. Begin with named native
   control flow and exact SIMD while retaining checked accesses, ordered
   stores/calls, dynamic tracing and original ABI state. A source span is not
   currently an observer/topology lease: raw scratch writes, bulk final
   writeback or an entry-only tracing check are not admitted. Differential
   proof must compare full registers/FP state, both ordered output streams,
   scratch/access/store sequences, mutation counts, growth and failure-prefix
   behavior against the unchanged generated route and genuine original input.

2. **Retain ordinary-model vertices and skin bindings; update changing palettes
   and presentation data.** Apply MCC's demonstrated resource-lifetime pattern
   to this local gap, without claiming MCC's ordinary skin pipeline is known.
   Admission must name full salted owner/generation, immutable model/LOD,
   exact palette/node state, expanded source ordinals, local attributes,
   bindings and post-placement mode. View-specific selected sections, repeated
   draws, coverage/material, color/PRT, UV and stipple remain per destination.
   Sharing a complete `Vertex` would overwrite that metadata: the current
   [compute kernel](../shaders/model_skin.comp:32) changes only position and
   basis. A shared deformation needs attribute-only scatter or an admitted
   draw-input design, with explicit dependencies and exact fallback/readback
   accounting. Unsupported CPU observers/overlays retain their required banks.

3. **Expand workers with owned, immutable packing jobs.** Keep source pose
   providers, palette admission and
   [shader/LOD selection](../src/rendering/models/source_dynamic_model_shaders.cpp:76)
   on their current owner; the latter performs floating-point calculations.
   A worker can gather/compact already selected chunks, assign resolved integer
   material slots, copy ordinals/bindings and concatenate into disjoint output.
   Its payload must own those chunks and exact decisions. Provider spans have
   a [synchronous lifetime](../include/halo2/source_live_model_geometry_tu15.hpp:35)
   and cannot simply be queued. Keep source readers, caches, arena pointers and
   callbacks out of the worker. Resolve allocation/admission at their required
   failure boundaries; the worker's byte/integer kernel can then be nonthrowing.
   Join and publish in original order before the
   [real geometry consumers](../src/gameplay/multiplayer/multiplayer_runtime.cpp:5609).
   The current synchronous API does not already permit next-view overlap; that
   requires a proved phase boundary. Add no queued simulation/presentation frame.

4. **Suppress redundant command state, then measure whether command workers or
   compatible instancing are worthwhile.** Pipeline-object caching already
   exists, but ordinary
   [material binds](../src/rendering/renderer/renderer.cpp:6287) and pipeline/
   blend-state commands still issue per draw. A cache needs the full descriptor,
   layout/dynamic-offset and state identity, with invalidation across specialized
   renderers, resets and command-buffer boundaries. Preserve all draws and
   transparency/pass order. Instancing remains a separate admission task:
   the MCC count producer/grouping policy was not recovered.

This order follows local costs as well as MCC mechanisms. In the last
[physical ordinary-runtime cohort](../build/test-results/presentation-offload-20261022/physical-c50-immediate-performance/summary.json),
the GPU route's median of three run-mean command-recording durations was0.223ms
for1p and0.733ms for4p, versus whole intervals16.438ms and58.184ms. Every run
overlapped unrelated tests; these are qualified observations, not clean CPU
attribution. The remainder cannot simply be labelled CPU execution, and moving
encoding alone is not evidence of enough saved time to meet120/60FPS.

The small referenced-row summary optimization was also applied and its
[owned GPU component rebuild](../build/test-results/presentation-offload-20261022/row-cache-physical-trial/build-execution.json)
passed. Only the three1p before/after pairs completed;4p/input-cohort closure
failed when the retained test build disappeared. Its partial+0.857% median
change is not a proven stable gain or a completed architecture implementation.
No GPU default, full-frame/latency, target-FPS or launcher gate closes here.

## What this audit does not establish

Ordinary MCC skinning placement remains **unresolved**. `render_model_skinning_disable` is present at string RVA`B38000`, referenced by data at`E0B918`, but the ordinary-character call/program/palette closure was not recovered. The existence of native vertex-shader machinery or a few embedded shaders is not sufficient to assert ordinary vertex skinning, compute skinning or its exact arithmetic. GPU culling, indirect draw architecture, cross-view palette reuse, exact object/portal ownership changes, active worker occupancy, GPU utilization and the reported4K120 explanation also remain unproved.

The native task evidence is an architecture reference, not a complete Classic renderer decompilation. Several callback consumers retain opaque identities and pass numbers; no guessed owner name is substituted. Live MCC attribution would require a version-pinned Classic CPU/API/GPU capture, which this static audit has not performed. Nothing here requires changing simulation frequency, physical behavior, source visibility/LOD, effects or output resolution.

## Storage

All binary probes ran through `tools/run_test.py` with no DLL extraction or mutation. Eight successful ephemeral owners were independently checked absent. The initial in-memory evidence assembly exceeded its1MiB guard and wrote no report; its wrapper completed error cleanup before the bounded retry. The manifest retains that failure rather than treating it as acceptance. Reports individually remain below1MiB; their disassembly plus exact source/argv pack total1,661,364 bytes before the small manifest and this note. No executable, archive/object, shader extraction, frame, raw memory bank or build was retained. The binary-research probes wrote only this new Markdown file and the compact evidence directory. The separately scoped two-file port trial is recorded above and in the goal checklist; original reference assets and unrelated work were preserved.
