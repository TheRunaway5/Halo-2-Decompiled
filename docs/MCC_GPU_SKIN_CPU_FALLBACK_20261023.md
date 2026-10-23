# Ordinary GPU skin resource fallback — 2026-10-23

The owned Release build and **six scoped components passed at07:43UTC on2026-10-23**. The separate12-case first-person4K runtime cohort and four forced fallback controls are active and **unaccepted** at this update. Component success does not establish fallback frame parity, ordinary FPS acceptance or a performance gain. GPU skinning remains under its existing opt-in/source admission. The source hashes below preserve the earlier ready checkpoint, which was uncompiled/unexecuted when first recorded; the frozen benchmark was left untouched by that initial implementation handoff.

`ModelSkinGpu::prepare` now throws allocation-free `ModelSkinGpuPrepareRefused`, carrying a typed reason and the original `VkResult`; renderer catches only this type. [API](../src/rendering/models/model_skin_gpu.hpp:21)

| Refusal reason | Recoverable operation/result | Explicit exclusions |
| --- | --- | --- |
| `device_quota` | Validated input/destination storage-buffer or dispatch limit | Missing destination and malformed records remain hard errors |
| `input_allocation` | Packing-vector `std::bad_alloc`, or input `vkCreateBuffer` / `vkAllocateMemory` returning host/device OOM | No broad `bad_alloc` catch; bind, shader, pipeline, descriptors and dispatch errors remain hard |
| `input_mapping` | Private input `vkMapMemory` host OOM or `VK_ERROR_MEMORY_MAP_FAILED` | Destination mapping remains required; other input-map results remain hard |

Khronos specifies `VK_ERROR_MEMORY_MAP_FAILED` when a suitable contiguous host virtual-address range is unavailable. This justification applies to the private input map only. [Khronos vkMapMemory](https://docs.vulkan.org/refpages/latest/refpages/source/vkMapMemory.html)

The dispatch count resets first. For a nonempty job, complete allocation-free record validation precedes quota checks, injected failures and packing allocation: destination uniqueness/range, mode0/1, owner palette extents, normal-or-zero bounded weights, original uint16 bone width, and every referenced row through the last nonzero influence, including interior zero influences. Unused rows retain the existing finite bounded domain. An increasing-destination fast path avoids heap storage; unordered records use pairwise duplicate checks. The service cannot independently inspect Vulkan destination vertex bytes; complete local arithmetic/source qualification remains the caller/FramePlan obligation, and the CPU helper revalidates it before any write. [prepare](../src/rendering/models/model_skin_gpu.cpp:143)

Input growth owns temporary handles until creation/allocation/binding succeeds. A resource refusal destroys only these temporary handles and leaves the old bank intact. The count remains zero until successful upload/descriptor publication. A saved full `fenv_t` and `errno` cover preparation, temporary cleanup and exception unwinding; failed FP restoration terminates. Empty jobs perform only the integer count reset. [environment/classification](../src/rendering/models/model_skin_gpu.cpp:21), [transactional growth](../src/rendering/models/model_skin_gpu.cpp:76)

Renderer records the refusal, completes the existing prefix/tail upload and joins the copy worker, then calls `model_skin_cpu_materialize` on declared destinations before unmapping. No source vertex bank is changed. GPU dispatch and GPU verification remain zero for fallback frames; the fallback does not request an additional readback or diagnostic allocation. Destination-map failure, materializer validation failure, device loss, and other hard errors propagate. [catch/join lifetime](../src/rendering/renderer/renderer.cpp:5450), [materialization](../src/rendering/renderer/renderer.cpp:5999)

The Vulkan-free materializer validates the entire job, ranges/aliases, original owner extents, bone width, shared integer arithmetic admission and post mode before writing. It invokes the existing SSE skin kernel and the exact `Transform{}` placement graph, including all-zero-weight placement and signed zeros. It establishes masked RTE temporarily and restores full SSE/x87 state and `errno`, including on unwind. Accessible live typed spans and immutable input ownership are required; integer address checks cannot prove arbitrary memory accessibility. The valid path has no explicit heap allocation. Ordinary ascending destinations are O(n); unordered alias validation is O(n²), and the shared numeric predicate rescans each record's owner palette. [helper](../src/rendering/models/model_skin_cpu_materialization.hpp:84), [original kernel](../src/rendering/models/model_skin_vertices.hpp:54)

`RendererFrameTiming` and its TSV add completed CPU fallback jobs/vertices/deformed/post-identity vertices, quota/allocation/mapping job counters, and the originating Vulkan result (`0` for quota/packing refusal). Existing GPU counters retain their meaning. [fields](../include/halo2/renderer_timing.hpp:23)

`HALO2_TEST_GPU_SKIN_PREPARE_FAILURE` accepts `quota`, `packing`, `create`, `allocation`, `device-allocation`, `map`, or `map-range`. Allocation injections traverse the temporary-resource code even if an old input capacity is sufficient. `device-lost` is a synthetic hard-negative result; unknown nonempty values are hard errors. Empty jobs ignore controls and clear the dispatch. No control changes default GPU eligibility.

The retained [service result](../build/test-results/presentation-offload-20261022/gpu-resource-green-build/evidence/model-skin-gpu.json) passes seven refusal sites, 896 SSE rounding/DAZ/FTZ/sticky combinations with independently different x87 rounding, 63 malformed-input precedence checks, two hard negatives, nine empty resets, and ten actual next-job recovery dispatches with complete vertex-byte comparisons. The separate15,606-vertex original/post-identity cohort passes with zero Vulkan validation errors on the RTX5080. The [retained CTest log](../build/test-results/presentation-offload-20261022/gpu-resource-green-build/evidence/Testing/Temporary/LastTest.log) also confirms CPU materialization passes complete bytes, prevalidation/no-write failure, source immutability, hostile structural ranges, heap-free ordered/unordered paths, 4096 SSE/rounding and774 independent x87 environments, plus unmasked rejection. These are executed component results. [service tests](../tests/model_skin_gpu_tests.cpp:141), [helper tests](../tests/model_skin_cpu_materialization_tests.cpp)

The [build report](../build/test-results/presentation-offload-20261022/gpu-resource-green-build/report.json) records6/6 passing components: GPU service, ordinary handoff, genuine MCC handoff, FramePlan, genuine MCC Cairo campaign handoff, and CPU materialization. It records a successful Release native/runtime-harness build; it is not an ASan/UBSan result or a runtime-frame acceptance receipt. Retained service JSON SHA256: `862fb6cd67c92bf8e128d40410073d6524508763a80bb53032aa3187b290b864`; retained LastTest.log SHA256: `2ae4239d537454bd5e2916310551b191631180c7002f2a5b0c639c813f0034d9`.

Run only after the coordinator admits the new frozen source and ends unsampled measurements:

```bash
python3 tools/run_tests.py --target model_skin_gpu_tests --target frame_model_skin_plan_tests --target model_skin_cpu_materialization_tests --filter '^native_(model_skin_gpu|frame_model_skin_plan|model_skin_cpu_materialization)$' --test-jobs 1 --retain-report model-skin-gpu.json --retain-report model-skin-spirv.json --output build/test-results/presentation-offload-20261022/gpu-fallback-components
python3 tools/run_tests.py --sanitizers --target model_skin_gpu_tests --target frame_model_skin_plan_tests --target model_skin_cpu_materialization_tests --filter '^native_(model_skin_gpu|frame_model_skin_plan|model_skin_cpu_materialization)$' --test-jobs 1 --retain-report model-skin-gpu.json --retain-report model-skin-spirv.json --output build/test-results/presentation-offload-20261022/gpu-fallback-components-asan
```

The active separate runtime harness still needs to admit each injected fallback with zero GPU dispatches, nonzero weighted/post-identity CPU fallback counts, exact full-frame bytes, identical ordered gameplay callbacks and zero Vulkan messages. A normal GPU run must continue requiring actual verified GPU work. Failed/refused prepare is not permission to treat skipped work as accepted. Structural failures and simulated device loss must stay red. The initial implementation handoff created no build/probe/frame/scratch artifacts; the later component run belongs to the coordinator's existing owned session, whose eventual cleanup remains its responsibility. This documentation update started no additional work.

The independent FramePlan interval/run review found no concrete acceptance, alias, overflow or lifetime defect. Prior and new implementations both sort each append by final destination: preserved frozen header SHA256 `57833b51945ed682ea4f6843d4ec6a988b0d0e5b655a6a13ca427322e594b2c2`, lines158–164, versus canonical SHA256 below. The new exclusive range end is bounded to2^32 by its last-index check; coalescing tracks occupancy without merging palette/geometry ownership. Dense/sparse duplicate-ordinal checks retain the same transitive bitwise local/binding equality. Cached validation relies on the existing immutable shared-geometry lifetime contract. Capacity/allocation-failure timing changes intentionally; no claim is made about allocator/new-handler invocation parity. [optimized plan](../src/rendering/renderer/frame_model_skin_plan.hpp:66)

After that review checkpoint, the coordinator additionally removed empty-submission palette-map clones after preserving late-registration/address rejection, and applied geometric source/palette capacity growth with a source-count guard. The later canonical FramePlan SHA256 is `95e9bbf0bbc732cf58c85ac7a7e500dc39d3a6c6cfc697b871dab7898de489fe`; the historical hash below is retained as the original reviewed ready checkpoint, not relabeled as the later source. [next-cost note](MCC_PERFORMANCE_EQUAL_WORK_NEXT_COST_20261023.md)

Historical source hashes at the initial uncompiled ready checkpoint (SHA256):

```text
22269ff95118f0d896751f06676d2f6b126656e74a4d158ebcdca84e46e792c7  src/rendering/models/model_skin_gpu.hpp
fcdd35ac1290bc8c1be1ff7f62da63c00b94b053d0442a5e790bc07779bde780  src/rendering/models/model_skin_gpu.cpp
8d747c02d3331baa7abb302af88776e7e646020865aa0f7c016e88a67d66008c  src/rendering/renderer/renderer.cpp
77d0aec94dadfc9af37b25427ab76d66ec2c253fc8628ed48e4f81c7c78c11eb  include/halo2/renderer_timing.hpp
272699b31479ae9c7e1d76f214d9c74206e7f65ab65419681bda2250f1e93d54  tests/model_skin_gpu_tests.cpp
e5cc5a0e60a5021d012827c31d0ee05dd5afe52c18b42d43f64a64933e9bd09a  src/rendering/models/model_skin_cpu_materialization.hpp
2854eed3f8ec05ec99f4292bf67c70f1efa676bd75d3e6df55d3b99507d05755  tests/model_skin_cpu_materialization_tests.cpp
b935c12fd66942a8b6ce10d036d06c1cbe5e1ee5305486c8acd3d9d061392934  cmake/ModelSkinCpuMaterialization.cmake
82f0dc17f3caf76a8982729b553b875dae7a091bec4236067ab84f24879c615b  src/rendering/renderer/frame_model_skin_plan.hpp
```
