# Native TU1.5 camera BSP selection

The semantic implementation in `src/rendering/world/native_tu15_camera_bsp_selection.cpp`
closes original `2B700` (37 sites) and its complete call-free `14CDB0` child
(39 sites). It consumes the actual retained BSP/node/plane/point storage. It
does not execute source instructions, maintain a virtual register file, use a
host spatial query, or supply an original query result.

`native_tu15_camera_bsp_leaf` caches the node/plane owners once, traverses the
actual eight-byte nodes, sign-extends each plane WORD, and retains the original
DWORD read one byte before each selected packed signed24-bit child. The terminal
bit, NONE sentinel and low23-bit index remain distinct. Every node computes
the signed plane distance with real SSE in Z-product, Y-product/add,
X-product/add, offset-subtract order. Real `COMISS` retains qNaN invalid status;
unordered values select the negative side, and either signed zero selects the
positive side. MXCSR rounding, DAZ/FTZ and sticky flags are source-owned and
the host environment is restored. No x87 state or depth is consumed.

`native_tu15_camera_bsp_select` decodes its cluster-output argument before the
BSP binding, then caches that BSP owner. Success publishes the leaf first,
freshly reads the leaf-array pointer, reads its signed cluster WORD and publishes
that cluster. Failure validates retained leaf and cluster with the original
signed, strict upper bounds and lazy reads. Valid history is retained; invalid
history publishes leaf NONE then cluster NONE. Aliased outputs and input
changes after the first store retain this order.

The numeric binding admits only ordinary `2B770` call `2B7E6`: EDX is the point,
EDI the leaf output, and entry SP+4 the cluster output. Its caller immediately
tests AL at `2B7EB`; volatile GP, integer flags and private SIMD temporaries are
dead. The binding marshals only AL on successful completion and consumes the
real RET word with RET4 stack ownership. It preserves nonvolatile state, opaque
dead ABI fields and the complete x87 bank. Outputs are disjoint from retired
callee-private pushes in this authenticated caller. Source memory/FP fault
prefixes are proved; arbitrary private-stack aliasing or exact mid-instruction
dead-register reconstruction is outside the contract. Direct numeric `14CDB0`
callers require their own admission; its typed helper is reusable now.

Evidence is under `analysis/campaign-native-new-game-camera-20261028/`:

- `bsp-selection-proof.json` pins current sources and the compact original fixture.
- `bsp-selection-original.json` and `bsp-selection-original-repeat.json` retain
  independent unchanged physical i386 execution: 268 cases, all76 sites,
  13,061 instructions, 4,004 ordered reads, 306 stores and95 injected fault
  prefixes. Four genuine unmapped-access cases include a leaf-output alias
  that changes the fresh leaf-bank pointer.
- `bsp-selection-caller-contract.json` authenticates original caller bytes,
  parameter setup, continuations and the ordinary authored-function ABI.
- `bsp-selection-validation-v1/validation.json` records GCC1/1,
  Clang ASan/UBSan1/1 and Clang9/9 including eight rejected actual-source
  mutations. `bsp-selection-validation-v2/validation.json` reruns current
  tests under GCC and ASan/UBSan after adding real RET-failure coverage; core,
  binding and fixture were unchanged.

The finite cohort covers signed indices/counts, both sides and signed zero,
multi-node traversal, qNaNs/sNaNs/infinities/denormals, rounding/DAZ/FTZ,
aliased outputs, lazy history repair, mutable owners, and ordered read/store
failures. Original execution is offline reference only. All disposable original
ELFs, compiler products, raw banks and scratch were removed. The required
compressed fixture is59,889 bytes.

`cmake/NativeTu15CameraBspSelection.cmake` adds only this component. World
routing, generated-producer retirement and current MCC/GPU composition remain
separate integration gates. This component does not establish whole native
camera-parent, rendered-frame, navigation or campaign completion.
