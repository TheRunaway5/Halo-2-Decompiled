# Ordinary GPU skinning: float32 admission

2026-10-23. Read-only source/specification audit; no compiler, shader, runtime, or numerical probe was run for this note. This establishes a candidate arithmetic contract, not GPU output equivalence or a performance gain. Scope is the existing deformation kernel; source-owned palette construction and gameplay remain ordered on the CPU.

## Result

The target RTX 5080 supports explicit float32 RTE and signed-zero/Inf/NaN preservation, but not denormal preservation, according to the parent task's freshly owned Vulkan query, retained at `build/test-results/presentation-offload-20261022/gpu-float-properties.json`. Requiring `DenormPreserve32` for every job would exclude this target. A useful alternative is a **proved normal-only arithmetic domain**, below, with explicit RTE and signed-zero preservation and the exact existing separate MUL/ADD graph. It requires no denormal execution mode. Actual shader readback and ordinary-character admission counts remain open.

## Source contract and device controls

`src/rendering/models/model_skin_vertices.hpp:58–68` finds the highest nonzero influence, then blends downward through influence zero. The first product initializes each row directly; subsequent products are added. Even a zero weight inside that interval still has its bone checked and product evaluated. `:77–83` separately multiplies three components, computes `(a+b)+c`, and adds translation **only to position**; `:87–91` stores the four attributes. Zero-influence vertices remain untouched (`:60`). Do not introduce a positive-zero accumulator, normalize weights, normalize vectors, skip interior influences, use FMA, or substitute a dot/matrix reduction. The scalar control spells out the same ordering at `:20–38`.

For float32 `OpFMul` and `OpFAdd`, Vulkan specifies correctly rounded results; explicit `RoundingModeRTE 32` selects ties-to-even. `NoContraction` prevents rearrangement/contraction; `SignedZeroInfNanPreserve` alone does not. Use all three for this kernel, with no `RelaxedPrecision` or overriding fast-math settings on its arithmetic. [Vulkan SPIR-V environment, evaluation and precision](https://docs.vulkan.org/spec/latest/appendices/spirvenv.html#_evaluation_of_expressions).

Query `VkPhysicalDeviceFloatControlsProperties` through `VkPhysicalDeviceProperties2`. Require `shaderRoundingModeRTEFloat32` and `shaderSignedZeroInfNanPreserveFloat32` before creating the bounded kernel. A general denormal-preserving variant would additionally require `shaderDenormPreserveFloat32`. Unsupported execution modes must not be declared. These are per-width properties; Vulkan API version or GPU brand alone is insufficient. [Float-controls properties](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFloatControlsProperties.html).

The original float-controls extension is core in Vulkan 1.2; it adds properties, not an extra feature-enable bit. A lower-version path must enable the extension and satisfy its API/SPIR-V dependencies. This is distinct from Vulkan 1.4/KHR `shaderFloatControls2`, whose `FloatControls2` capability requires its feature enabled. [Original extension](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_shader_float_controls.html), [controls2 feature](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceShaderFloatControls2Features.html).

`SignedZeroInfNanPreserve` does not promise identical NaN payload bits; `RelaxedPrecision` defeats these float-control modes. The bounded domain rejects all NaN/Inf and subnormal arithmetic inputs. [SPV_KHR_float_controls](https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_float_controls.html). NVIDIA lists Linux Vulkan 1.4 support and RTX 5080 among supported GPUs; that listing does not certify individual float-control properties. [NVIDIA driver support](https://developer.nvidia.com/vulkan-driver).

## Normal-only proof for the proposed bounds

This is a mathematical inference about the exact graph above, **not a vendor claim or measured result**. All operands are binary32. Admit signed zero, otherwise require absolute magnitudes:

| Operand | Inclusive range |
|---|---|
| Position, normal, tangent, binormal component | `2^-24 … 2^20` |
| Each used palette component | `2^-40 … 2^20` |
| Each weight | `2^-10 … 1` |

There are at most four influence products per blended component. No positive-weight or sum-to-one assumption is needed. Binary32 has 24 significant bits and smallest normal magnitude `2^-126`. [Intel SDM volume 1, table 4-2](https://cdrdv2-public.intel.com/843827/253665-sdm-vol-1-dec-24.pdf).

Use this lattice lemma: rounding a multiple of `2^q` to binary32 RTE preserves that lattice when the result is finite and `q >= -126`. At small magnitudes the lattice point is exactly representable; at larger magnitudes binary32 spacing is a coarser power-of-two multiple of the lattice. Cancellation therefore cannot introduce a nonzero result smaller than one lattice unit.

| Stage | Lower lattice bound and magnitude bound |
|---|---|
| Rounded palette × weight | Nonzero magnitude at least `2^-50`, at most `2^20`. Every such binary32 number is a multiple of `2^(-50-23) = 2^-73`. |
| Up to four separately rounded row terms | Remain multiples of `2^-73`; nonzero magnitude at least `2^-73`, absolute magnitude at most `4·2^20 = 2^22`. |
| Attribute component | Every admitted nonzero binary32 component is a multiple of `2^(-24-23) = 2^-47`. |
| Row × attribute | Exact product lies on lattice `2^-120`, nonzero magnitude at least `2^-97`, at most `2^42`; its rounded product is normal and stays on that lattice. |
| `(x+y)+z` | Each separately rounded sum stays on lattice `2^-120`, nonzero magnitude at least `2^-120`, absolute magnitude at most `3·2^42`. |
| Position translation add | Translation is also a multiple of `2^-120`; final result stays on that lattice, with magnitude below `2^44`. |

The magnitude upper bounds used in each rounding step have representable binary32 ceilings. Thus no step overflows. Nonzero arithmetic intermediates are at least `2^-120 > 2^-126`; there is no subnormal operand/result, underflow, NaN, infinity, invalid operation, or division. Only precision/inexact can be raised. Exact cancellation and multiplication by signed zero still require the original graph and signed-zero control. Merely testing that inputs are finite would not prove this.

Implement qualification with integer `bit_cast`/absolute-bit range tests: rejected signaling NaNs must not raise host exceptions during admission. Cache immutable model/LOD attribute and binding qualification with its actual ownership identity; qualify the current palette each job. Preserve bone-bound/mapping validation and original fallback/error behavior, including interior zero weights. Do not use a GPU out-of-bounds access as validation. Cache invalidation, binding lifetime and source observer ordering remain separate obligations from the numeric proof.

## Host SSE effects cannot be delegated to the GPU

MXCSR exception flags are sticky bits 0–5, masks are bits 7–12, and rounding control is bits 13–14. GPU execution does not reproduce CPU-thread MXCSR changes. [Intel SDM volume 1, sections 10.2.3.1–4](https://cdrdv2-public.intel.com/843827/253665-sdm-vol-1-dec-24.pdf).

For **this admitted kernel**, a conservative terminal-state gate is:

```text
(mxcsr & 0x1f80) == 0x1f80    // all SSE exceptions masked
(mxcsr & 0x6000) == 0         // round to nearest, ties to even
(mxcsr & 0x0020) != 0         // precision sticky already present
```

The proof eliminates all exceptions except possible precision; since precision is already set, removing these SSE operations leaves terminal sticky flags unchanged. Do not synthesize or clear exceptions. DAZ/FTZ cannot change this normal-only kernel's arithmetic, but controls must not be changed by admission or dispatch. Test the gate at the actual omitted CPU-call boundary; a check after unrelated work is not equivalent. A job that fails the gate must execute the original CPU kernel.

This is **not** unconditional observational parity: preserve diagnostic/scalar/VERIFY behavior, original allocation/error and validation paths, and verify intervening code cannot inspect or mutate the environment differently. The explicit normal x64 intrinsics use SSE arithmetic, but a final linked closure audit must still exclude introduced x87 arithmetic and account for helpers, allocation/new-handler failures, Vulkan calls, `errno`, and x87 status/control. CPU palette construction is outside the proof. `model_world_pose_cache.hpp:30–55` already records diagnostic eligibility, errno, MXCSR and x87 state for a different reuse seam; that receipt cannot simply be applied to a new asynchronous GPU boundary.

## Compiler route and falsifiable acceptance

Current shader discovery is PATH-based (`CMakeLists.txt:106`), and the shader loop uses bare `glslangValidator -V` (`:1416–1420`), without an explicit target environment. Existing CPU palette/pose files disable contraction (`:500–501`); those flags do not govern SPIR-V. Existing shader `precise` expressions (`shaders/vertex_program.glsl:119,130–133`) and its intentionally distinct captured-particle arithmetic (`:113–117,126–128`) should remain scoped.

GLSL `precise` constrains operation order and contraction; it does not itself select RTE or denormal behavior, and precision propagation has function boundaries. Constant folding is also not promised to match a corresponding runtime expression. [GLSL 4.60, section 4.9](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#the-precise-qualifier).

One documented way to request the two entry-point modes in a new shader is:

```glsl
#extension GL_EXT_spirv_intrinsics : require
spirv_execution_mode(extensions=["SPV_KHR_float_controls"], capabilities=[4467], 4462, 32);
spirv_execution_mode(extensions=["SPV_KHR_float_controls"], capabilities=[4466], 4461, 32);
```

This syntax emits declared SPIR-V capabilities/extensions/execution modes; the IDs are RTE and signed-zero preservation. No denormal mode is requested for the bounded variant. [GLSL SPIR-V intrinsics](https://docs.vulkan.org/glslext/latest/glslext/ext/GLSL_EXT_spirv_intrinsics.html), [mode/capability IDs](https://github.khronos.org/SPIRV-Registry/extensions/KHR/SPV_KHR_float_controls.html). This snippet was not compiled here.

A future owned test should compile only the new shader for the qualified Vulkan target, record compiler/version/hash and final shader hash, run SPIR-V validation, and inspect the final module for both modes, float32-only arithmetic, every required `NoContraction`, no FMA/dot/matrix reduction and no relaxed/overriding fast-math decorations. `--target-env vulkan1.2` selects SPIR-V 1.5 in glslang's documented CLI; use it only with the corresponding device admission. [glslang CLI source](https://raw.githubusercontent.com/KhronosGroup/glslang/main/StandAlone/StandAlone.cpp). The existing scoped parser at `tools/generate_world_depth_fragment_arithmetic.py:33–57` demonstrates parsing `OpDecorate NoContraction`; it does not validate this new kernel.

Then compare **all output bits** against the unchanged CPU kernel using GPU readback: every attribute, both zero signs, first-influence initialization, holes/interior zero weights, 0–4 influences, opposite-sign cancellation, exact range endpoints and neighboring rejected values, precision ties, maximum partial sums, DAZ/FTZ combinations, and all relevant sticky/control states. Verify full final MXCSR/x87/errno and fallback/error/diagnostic behavior, plus ordinary loaded character/LOD admission and rendered output. No such new evidence is supplied by this note; no integrated FPS claim follows from the arithmetic proof.

## Inspected source hashes

| File | SHA-256 at inspection |
|---|---|
| `src/rendering/models/model_skin_vertices.hpp` | `852ae52b48a8adbfa33806487f80c76e5686239fcf1aa46a4f1bf6bc04172f0b` |
| `src/rendering/models/model_world_pose_cache.hpp` | `88218a532386c99f317edca94cad0fd05dc48a8e5925241df09bf1d5ec89b01a` |
| `CMakeLists.txt` | `07355037f8ec8735e37da712d430882ce35eea11eeb67381ead54606bb590bae` |
| `shaders/vertex_program.glsl` | `dfa1cfe0750177560e8b018539e04849033b54e53d69f340567df09b2caa04b3` |
| `src/rendering/renderer/renderer.cpp` | `3c0b7e7c82a733d626153ca0c491866a4d473f1a393dfdf377c4a1deede3a99c` |

At that renderer inspection, device setup queried ordinary properties/features (`renderer.cpp:1040–1074,1119–1131`) without this float-control-property qualification. Parent implementation is concurrent; these hashes/pointers are an inspection boundary, not an assertion that later edits lack it. Only this Markdown file was written for the research; no scratch artifact was created.
