# Appearance parity

The native renderer is **not visually 1:1 with the original Xbox game**.
The BR's authored colored scope reflection and live magazine display run.
Multiplayer now executes the supplied maps' static world lighting, baked
scenery, water, skies, fog and bloom, along with BSP/instance object-light
queries and the original retained lighting cache. Dynamic contacts/shadows,
complete first-person placement and matched original frame comparisons
remain acceptance requirements.
Current multiplayer coverage is recorded in
[`MULTIPLAYER_SHADING_PARITY.md`](MULTIPLAYER_SHADING_PARITY.md).
Passing asset, arithmetic and Vulkan checks must not be
used as a claim that these images match the original.

Authority is the user's Xbox cache build `02.09.27.09809` and its matching
`default.xbe` (SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`).
No Vista/MCC materials or approximate replacement lights are used.
The original code, executable and cached shader payloads are the implementation
reference. User-supplied screenshots are not required to reverse engineer these
paths. Native screenshots are inspected and recorded, but they do not by
themselves establish agreement with an original framebuffer.

## Corrected inputs

| Original behavior | Native correction | Evidence |
| --- | --- | --- |
| Model vertex channel 2 contains packed normal/tangent/binormal vectors | Decode all three vectors and rotate them through bone, camera and object transforms | Chief FP basis and animated 90-degree rotation fixtures |
| Object change colors enter pixel color-bank indices 3/4/21/22 | Bind Chief's authored primary/secondary colors to the original armor mask program | Cairo armor constants `0x758550` / `0x55613a`; captures show green armor and separate glove color |
| `1BBF0` reads shader fields `+0x3e/+0x40/+0x44/+0x48` | Preserve lighting selectors, specular brightness and ambient bias; assemble selectors 4/5 and bind scalar 21 | Cairo panel regression failed before the fix; original selector 1 and brightness 0.5 now reach the program |
| `19CA0` packs extern writes after operations and permutation filtering | Bind scalars/RGB, operations, signed conversion, component masks and overlapping writes | 2,024/2,024 exact comparisons with the original instructions in the test harness |
| Scalar and RGB extern banks reuse numeric source indices | Filter object change colors by RGB component type before looking up their color slot | Scalar source 3 remains untouched while signed RGB source 3 reaches its register |
| Authored vertex programs apply the base transform before detail/bump transforms | Execute admitted NV2A vertex instructions for material varyings | Distinct base/detail/bump transforms and real Lockout program fixture |
| NV arithmetic makes zero times any operand zero | Preserve zero-light results through RSQ/MUL/MAD and dot products | Real lightmap program with zero incident light, previously producing NaN |
| Cubemaps contain six independently padded mip chains | Preserve face order and all resident levels, upload cube views, bind original cube texture modes | Real Chief reflection map: six faces, 30 levels, 16,368 resident bytes; synthetic distinct-face fixture |
| Sky selection reads cluster `+0x6e`; `3EBD0` applies scale, yaw and eye parallax | Load the visible authored sky for single-sky BSPs and render its model separately from world depth | Cairo space sky has scale 400, movement 1 and yaw 0 |
| Transparent/emissive materials carry original texture and blend bindings | Share their binding path between world, sky and frontend surfaces | World glass uses authored programs rather than opaque diffuse fallback |
| `189A0` expands each 24-byte texture-state record | Retain all 14 values and create per-stage Vulkan samplers with authored addressing, MIN/MAG/MIP, bias, maximum mip, anisotropy and border color | Cairo bump uses 4x anisotropy, its lightmap clamps; 4,112/4,112 exact comparisons with original instructions |
| Texture alpha kill precedes register combiners | Test fetched texture alpha independently of the material alpha test | Authored enable bit retained per stage; native Vulkan capture coverage |
| Cairo bumped environment programs use PROJECT2D / DOTPRODUCT / DOT_RFLCT_DIFF / DOT_RFLCT_SPEC | Execute dependent texture stages in order, retain scalar products, use the next normal row for diffuse and the authored q components for reflection | GPU regression returned zero before the correction; independent lookup-coordinate fixtures now pass |
| `FF5F0` supplies primary magazine digit inputs; `17550` writes bitmap frame property zero | Select the authored BR tens/ones BC1 images from the live loaded count, keep all original mips and samplers, and rebind resident Vulkan images after the frame fence | 200/200 original instruction checks, 100 digit pairs, 139 firing/reload ticks; GPU captures show 36/33/30/09/00 and an exact return to 36 |
| A model's first LOD has a texture-mapping pass followed by its environment layer | Render the base accumulation target, then the authored per-vertex environment program at equal depth; bind original bump, scope cube and normalization cube | BR lens program bytes, texture faces/mips and depth state checked against Cairo; blue lens visible in production captures |
| `2FD90` stores the camera's backward vector; `12FA0` uploads it to c6 | Bind negative camera forward to the authored vertex program | Original instructions executed for four camera directions; this restores the face/edge gel color convention |
| `12FA0` binds pixel conversion constants c8..c11 | Supply active viewport dimensions and offsets, original pixel bias, render-scale clamp and fixed 128x128 shadow semantics | 4,256/4,256 bit-exact original-instruction comparisons; shared GPU function checked at 480p through 4K |

The vertex arithmetic rule is documented in NVIDIA's
[NV_vertex_program specification, section 2.14.1.11](https://registry.khronos.org/OpenGL/extensions/NV/NV_vertex_program.txt).
Opcode fields and cube layout were checked against the original payloads and
primary NV2A format descriptions; implementation code was written independently.
Host reciprocal/transcendental precision has not been compared with NV2A hardware.
Dependent lookup equations follow NVIDIA's
[NV_texture_shader specification](https://registry.khronos.org/OpenGL/extensions/NV/NV_texture_shader.txt).
Xbox input-stage fields and normal mapping selectors are read from the original
pixel definitions. Texture lookup tests expose the requested coordinates;
they do not certify hardware interpolation or fixed-point rounding. RGBA
dot mappings 0/1/2 and authored bump-offset operations are admitted; volume/HILO and depth-replacement
operations remain explicit translation gaps.

Mip-disabled samplers retain the original minification filter by using a small
LOD interval, as specified by
[VkSamplerCreateInfo](https://docs.vulkan.org/refpages/latest/refpages/source/VkSamplerCreateInfo.html).
Mirror-once addressing uses the
[mirror-clamp sampler mode](https://docs.vulkan.org/refpages/latest/refpages/source/VkSamplerAddressMode.html)
with its device extension enabled. Signed V8U8 channels are decoded before
filtering. Unsupported color-key formats and
filters are reported rather than treated as equivalent RGBA/linear sampling.

## Module boundaries

| Module | Responsibility |
| --- | --- |
| `src/material.cpp` | Bounded shader/template/pass decoding, shader selectors and static pass binding |
| `src/render_program.cpp` | Original pixel program assembly and constant/extern remapping |
| `src/bitmap_animation.cpp` | Selected-pass bitmap-frame overlays and original weapon magazine digit inputs |
| `src/appearance.cpp` | Object change colors and texture/emissive surface bindings |
| `src/model_environment.cpp` | First-LOD model environment admission and authored base/environment texture bindings |
| `src/frontend_material.cpp` | Shared forward material binding; frontend compatibility wrapper is in its header |
| `src/vertex_program.cpp`, `shaders/vertex_program.glsl` | CPU reference and GPU execution of admitted authored vertex programs |
| `src/cubemap.cpp`, `src/texture.cpp` | Face/mip layout, palette expansion and texture decoding |
| `src/texture_state.cpp`, `include/halo2/sampler.hpp` | Original sampler-state expansion and Vulkan sampler descriptions |
| `shaders/texture_program.glsl`, `include/halo2/texture_program.hpp` | Dependent/projected texture addressing and admission of supported operations |
| `src/render_view.cpp`, `shaders/render_view.glsl` | CPU reference and shared GPU viewport constants; target dimensions remain separate from texture dimensions |
| `src/sky.cpp` | Visible sky metadata and authored sky transforms |
| `src/model_pose.cpp` | Pose sampling and basis skinning |
| `src/renderer.cpp` | Vulkan images, descriptors, pipeline state and draw submission |

## Reproduction and evidence

The October 7 rendering/appearance CTest groups pass, along with seven focused
ASan/UBSan groups (cache bounds, pixel programs, first-person transforms, and
their retail appearance/rendering/pose fixtures, plus resolution transforms).
The GPU addressing test covers 11 texture-address fixtures and 800 viewport
vectors. Vulkan synchronization validation covers 80/80 BSPs and 48/48 fixed
first-person captures. The resolution sweep additionally captures both weapons
at 640x480, 1280x960, 960x540, 1280x720, 1920x1080 and 3840x2160. This is
native execution evidence, not an original-frame comparison.

The isolated `build-appearance` build avoids competing with other work using
the default build directory. Reproduce the input and instruction checks:

```bash
cmake --build build-appearance
ctest --test-dir build-appearance --output-on-failure
.tools/python/bin/python tools/differential_pixel_externs.py \
  --library build-appearance/libhalo2_kernels.so
.tools/python/bin/python tools/differential_texture_states.py
.tools/python/bin/python tools/differential_viewport.py
python3 tools/verify_resolution.py --validation
```

Native capture sweeps are written to `analysis/appearance/world-gpu/` and
`analysis/appearance/first-person-gpu/`. Their reports record the tested binary
hash and scoped results. The appearance regression fixture uses Cairo and
Lockout; the first-person sweep covers original 4:3 and wide 16:9 views, both
battle rifle and SMG, and idle/fire/full reload/empty reload poses.

## Battle rifle display and scope

The ammo panel below the optic must display loaded rounds (`36` for a full BR
magazine), independently of the pose. `numbers_plate` contains ten separate
32x32 BC1 images. Its authored identity overlay scales the original normalized
ones/tens input back to a bitmap frame. The native renderer now keeps those
frames resident, with their four original mip levels, and changes the descriptor
only after the previous draw has retired. The fixed-pose runtime and live
firing/reload path use the same function inputs.

The lens uses `battle_rifle_lens` and the
`opaque\tex_bump_env` template, with a separate `scope_cubemap`, authored gel
colors RGB (10,23,255) and (194,235,0), and a low blue specular tint. Model
admission now selects the first LOD's base pass and per-vertex environment
implementation, replacing the incorrect later static-lightmap selection. The
native renderer draws the base into external texture 3, then executes the
original environment vertex and pixel payloads at equal depth. It binds the
original bump map, all scope-cube faces/mips, and the authored vector-normalization
cube. The housing retains its separate `battle_rifle_scope` material.

The per-vertex program reads stream v6 as radiance, rather than the UVs used by
the per-pixel stream. Camera constant c6 uses backward, as generated by retail
`2FD90` and uploaded by `12FA0`; using forward reversed the blue/yellow-green
gel response. Production captures now show the blue reflective lens. The target
uses the active framebuffer dimensions, and projected pixel coordinates are
normalized by that sampled target's actual size. Swapchain recreation rebuilds
the image and rebinds external texture 3. No fixed 640x480 divisor is used.

The live multiplayer renderer now selects original dynamic implementation 2
and binds BSP/instance root queries through the original retained lighting
cache. The historical scope captures above exercise the earlier per-vertex
path. The current shader/cubemap and root-lighting work does not establish a
complete original weapon/framebuffer match; special query/sky fallback and
dynamic-object contacts remain open.

The scoped report is `analysis/appearance/scope-parity.json`; captures
are in `analysis/appearance/scope-gpu/`. Reproduce with:

```bash
.tools/python/bin/python tools/differential_weapon_display.py \
  --output analysis/appearance/scope-display-differential.json
.tools/python/bin/python tools/audit_camera_constants.py
build-appearance/weapon_display_tests assets/disc/maps /tmp/weapon-vectors.bin \
  analysis/appearance/scope-gpu
python3 tools/verify_resolution.py --validation \
  --output analysis/appearance/scope-resolution
```

The GPU capture command requires the configured Vulkan validation layer.
The suite includes source asset/program checks, live firing/reload updates and
GPU fog/texture addressing checks. The scope capture also reuses one renderer
at 640x480, 1280x720, 3840x2160 and back to 1920x1080 to verify accumulation
rebinding. The resolution sweep covers BR and SMG at six sizes, 480p through
4K. The report records the tested binary/shader hashes, test counts, Vulkan
diagnostics, sanitizers and canonical runtime capture. These checks establish
native execution and resolution behavior, not original framebuffer parity.

The final scope run passes all 26 tests in the canonical build, all 48 fixed
first-person action/aspect captures, all 12 canonical resolution captures,
the 10 display/resize captures and 200 original magazine/bitmap comparisons.
ASan/UBSan checks cover the display and appearance fixtures, including isolation
of environment color constants between objects sharing the same material.
The canonical lens crop is `analysis/appearance/scope-default-optic.png`.

## Remaining acceptance work

1. Complete remaining scene scheduling, sorting, occlusion and unsupported
   shader families. All 21 supplied multiplayer BSPs now submit their admitted
   original baked/vertex lighting, accumulation, environment, illumination,
   planar fog, transparent, water and bloom-mask passes. Original scope
   target16 and Elite volume inputs are connected to native GPU producers.
2. Complete dynamic-object collision contacts, later object mode/flag
   producers, dynamic lights and shadows. Original query admission and
   camera-selected sky fallback are connected using their source guards.
   Original BSP/instance queries,
   per-vertex buckets, alternate resident lightmap groups, held-weapon root
   lighting and source cache cadence/interpolation are integrated. Exact
   equal-distance instance accelerator ownership remains an explicit gap.
3. Bind remaining switches, externs, animation/overlays and vertex globals.
   Original ARL/relative vertex addressing, PRT streams and per-section object
   banks are implemented and checked against independent original CPU/GPU
   fixtures. Initial machine rendering and private gear progression run in
   the live multiplayer path; later device scripts and group transitions remain.
   Chief's fixed authored colors are covered; random color alternatives and
   object color functions are not yet reproduced.
4. Complete remaining signed/color-key and special sampler/filter modes,
   depth/stencil state and sorting. Camera-cluster sky selection, sky depth
   clamp/culling, authored fog/bloom/patchy motion and active sampler bindings
   are implemented. Gemini dynamic-door portal state and residual patchy
   branches remain incomplete. Each future offscreen pass must
   bind its own viewport, render scale and sampled-image texel dimensions;
   fixed retail shadow targets must not resize with the display.
5. Capture original and native images with the same build, save state, camera,
   viewport, weapon, animation frame and lighting state. Compare silhouettes,
   per-material color/normal/detail/reflection response, alpha edges and
   framebuffer differences. Independent original gameplay captures, raw
   camera/lighting state and GPU projection constants are now available under
   `analysis/multiplayer-shading/original-xemu`. The first Ivory Tower
   comparison corrected the authored observer offset, safe-area projection
   and reticle position. Independent original packed inputs, node maps, bone
   bytes, blend weights and the captured GPU palette now verify 10,344 expanded
   SMG/arm world vertices at idle frame 32 within 1.90735e-6. All 14 packed
   position/UV/basis components are bit exact. The fresh production GPU replay
   after the blend-weight correction reduces active arm lighting differences
   to 2.38419e-7 and environment coordinates to at most 1.90735e-6; active weapon
   and hand UVs are bit exact. At five retained pixels, replacing only the
   native base texture sample with the captured original sample reproduces
   original hand RGB and paint within one float ULP. This isolates a sampled
   texture boundary without changing the original compressed texture blocks
   or claiming physical Xbox filtering equivalence. A retained original-player
   fixture now carries exact source color, lighting and aiming-counter words
   for controlled comparisons. Full motion and remaining shader differences
   are still being checked.
   The complete matched-frame gate remains open.

First-person transform and pose coverage is recorded separately in
`FIRST_PERSON_PARITY.md`. It does not certify the shader/lighting work above.
