# Campaign path priority heap, TU1.5

The semantic native module closes the complete integer heap owners `27BB60`
(sift up, 41 original sites), `27BC00` (sift down, 57), and `27BCE0`
(insertion, 15). Its only dependency is the caller's actual retained memory.
It contains structured loops and a typed heap view, with no virtual CPU,
register file, instruction dispatcher, binary execution, geometry result,
allocation result, or supplied dependency return.

The public API is `include/halo2/native_campaign_ai_path_heap_tu15.hpp`.
`cmake/CampaignNativeAiPathHeap.cmake` compiles its production source into
`halo2_physics_world_runtime`. No generated path-search fallback or whole
campaign readiness gate is enabled by this module.

## Original contract

The leased workspace owns a signed WORD exclusive tail at `+110B0`, with
root slot 1 and slot 0 reserved by the original initializer. Each four-byte
slot contains a signed WORD node ID at `+110B2 + slot*4` and signed WORD
priority at `+110B4 + slot*4`. A node's WORD backpointer is at
`+F0 + node*44`. Address arithmetic retains the original 32-bit wrap and
signed WORD indexing; negative counts are not silently clamped.

Insertion rejects signed tail values at least `400`; otherwise it publishes
the incremented tail, then node, then priority, and sifts from the old tail.
Equal priorities stop promotion. Sift up reads parent priority before parent
node, including the node read on a stopping comparison. Sift down rereads the
tail for each child, chooses the first strictly better child, and publishes
priority before node. Both owners always perform their final writes, even
when the values are unchanged. Reads remain lazy across backpointer and slot
aliases; no host `priority_queue` or standard heap algorithm is substituted.

These owners contain no floating comparison or conversion. Path cost
quantization is upstream in `27B100 -> 32CC0C`, outside this closure.

The functions have a void semantic result. The original preserves EAX and
the nonvolatile registers, SIMD registers, and occupied x87 state. ECX/EDX
scratch values are not native results. In particular, `27BB60` overwrites its
saved ECX stack slot with the saved node, so its final ECX is node scratch.
The authenticated callers consume the heap data and do not consume that
scratch as a return value. The native API uses the ordinary C++ calling
convention and does not reconstruct those registers or the original stack.

## Independent proof

`tools/validate_native_campaign_ai_path_heap_tu15.py` builds an offline ELF32
containing the unchanged owned TU1.5 text, whose SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The physical i386 CPU executes the original instructions under read-only
ptrace observation. No instruction patch, original-state edit, unknown-call
resume, emulated instruction engine, or supplied return is used.

GCC and Clang with ASan/UBSan pass 72 original calls, 5,072 callee
instructions, 531 interleaved mutable data reads, and 513 ordered stores.
All 113 original instruction sites are reached and authenticated against the
owned image. Complete retained banks, individual before/after stores, original
store-site observations, read/store interleaving, preserved original
register/stack contracts, all eight original SIMD values, and occupied 80-bit
x87 operands are checked. The native integer call also preserves the host
x87 control/status and MXCSR. This does not claim equivalence for invalid
unmapped storage, arbitrary dead registers/stack, or unmasked FP traps.

Four cases enter at the actual `27B340` search owner, execute 34 original
instructions each through its real `27B3D6` call, capture the actual `27BC00`
entry state, and return naturally at `27B3DB`. Insertion cases also execute
the original nested `27BCE0 -> 27BB60` call. The other caller continuations
are retained in the small authenticated `caller-continuations.json` report.

Five private semantic mutations are rejected: promoting an equal priority,
unsigned priority comparison, moving the parent-node read before its priority,
swapping final sift-down stores, and caching the mutable tail. Two explicit
upward tie cases were added after the first corpus failed to reject the tie
mutation; that earlier report is not acceptance.

Accepted numeric evidence is in
`analysis/campaign-native-path-heap-20261028/gcc-qualified-v4/report.json` and
`clang-sanitizers-v1/report.json`. The focused CMake module and missing-reference
admission checks are in `module-integration-v2/report.json`. Missing owned
reference returns skip 77 with `passed:false` and `validation_not_run:true`;
a wrong image pin fails. A skipped check never qualifies parity. The module
integration receipt records a validation-wrapper path-resolution correction;
the semantic core, public API, tests, original observer and site inventory
remain identical to the GCC/Clang proof pins.

All CMake work uses `tools/run_tests.py`; original observation and private
compiler/probe work use `tools/run_test.py`. Raw banks, ELF32 references,
mutation sources, and builds are disposable scratch. The cleanup receipt
records their removal and the retained small reports.

## Remaining search frontier

`27B340` still needs its actual neighborhood, geometry, and cost owners:
`27BD30`, `202970`, `218EB0`, `218C40`, and `27C520`. `27B100`, `27B010`,
`27B2B0` and the workspace lease lifecycle are separate required owners.
`200D30` has 26 distinct direct dependencies, and the full `1C9F10` path
decision remains open. This heap proof does not establish any of them.

The existing native startup storage validates the real two-workspace backing
(`native_tu15_final_startup_storage_data.cpp:83`, total `452E8` bytes). That
does not establish semantic parity of its generated initializer or the lease
owners. The existing `NavigationWorld::route` explicitly uses host geometric
costs (`navigation.hpp:5`) and a floating host A* queue (`navigation.cpp:65`);
it cannot stand in for these actual TU1.5 path-search owners.
