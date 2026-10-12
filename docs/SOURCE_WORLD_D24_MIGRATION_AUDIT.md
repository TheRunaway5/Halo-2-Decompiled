# Native world D24S8 requirements and remaining source boundaries

The live `Renderer` cannot supply its current depth image to
`SourceWeatherDepthAlias`. Its preferred format is D32, its image lacks transfer
source usage, and its render passes discard stencil. The isolated alias already
reproduces every byte of an actual D24S8 image, but that proof does not convert a
live D32 attachment into original integer D24 or produce missing stencil writes.

This is a read-only source and capture audit. No renderer, GLSL, CMake, runtime,
GPU, replay controller or guest state was changed. The reproducible report is
`analysis/multiplayer-shading/world-d24-requirements/report.json`; the generating
tools are `tools/audit_world_d24_requirements.py` and
`tools/audit_world_depth_encoding.cpp`.

## Original format authority

Literal title `12490` enables automatic depth/stencil and selects D24S8.
`128C0` instead selects `D3DFMT_D24S8 + (settings.depth_format != 0)`. The
alternative must remain explicit; an integer-D24 implementation does not cover
that floating-depth setting. Both routines' original XBE bytes and disassembly
are pinned in the report. The actual Lockout full-weather entry and return both
retain `485AC8 = 0000`, independently corroborating the integer-D24 setting.

Original20's exported resource `ResourceId::8601` is D24S8 at 640x480. The
different Lockout OpenGL capture uses D24S8 `ResourceId::124035`. Its snow event
215873 saves all 307,200 original integer depth and stencil values. The unchanged
point draw preserves the complete image. The stored stencil histogram is
300,945 zero bytes and **6,255 bytes of FF**; its depth range is
20,424..16,777,215. These are stored values, not recovered pre-fragment W.

The retained xemu commit `fc24584ce88f0915ad7f04775bb7712c2e3f49ee`
prefers Vulkan D24S8 and has a D32-with-stencil fallback. Its separate integer
pack routine converts Vulkan depth/stencil aspect copies into the original word
`(depth << 8) | stencil`. The isolated native alias uses the proved D24 path:
RGBA bytes are `[depth >> 8, depth, stencil, depth >> 16]`, each narrowed to eight
bits. Float-depth reconstruction is not an equivalent input to that producer.

Original20 is xemu Vulkan; the weather scene is unmodified xemu OpenGL llvmpipe
at another frame. Neither establishes physical Xbox raster precision or an
all-map stencil lifecycle.

## W encoding: literal difference, limited present evidence

For an ordinary atmosphere view, `world_vertex.glsl` receives the submitted
source c3 row and carries raw W through perspective-correct interpolation.
`world.frag` floors/clamps that value and divides by 16,777,215. Without source
camera control it scales the existing fallback W first; with W mode disabled
it writes `gl_FragCoord.z`. This audit does not extend the original W-mode claim
to those other branches.

Pinned xemu `glsl/psh.c:1491–1512`, the actual original20 8878 fragment
disassembly, and the actual Lockout snow fragment instead encode integer D24 as
`uintBitsToFloat(floatBitsToUint(floor(W) / 16777216.0) + 1u)`.
`particle_depth.glsl` already retains this expression. `light_volume.frag`
currently uses the same division expression as `world.frag`.

The CPU diagnostic exhausts all 16,777,216 finite D24 integers. The pinned
expression and correctly rounded binary32 division by 16,777,215 differ at
**one input, zero**, by one float word. Both recover every integer under the
diagnostic's ideal real-product floor and nearest conversions. A separately
rounded reciprocal followed by a binary32 multiply differs in 8,388,607 float
words; its ideal floor and nearest diagnostics miss 4,194,303 and 6,291,455
integers respectively. These are mathematical/compiler-lowering controls,
not a measured native driver or fixed-point attachment result.

Using the 307,200 actual Lockout stored integers as expression inputs, rounded
division and the pinned expression agree in every float word. The reciprocal
form differs in 96,176 float words, but all three recover every integer under
both ideal conversion diagnostics for this bank. These inputs do not recover
the original fractional W before rasterization. The retained native
`build-mp-shading/shaders/world.frag.spv` still has `OpFDiv` by 16,777,215;
the report pins its hash. Final driver lowering remains unmeasured.

Consequently, a shader-source difference exists, but this audit does **not**
prescribe a visible correction from the CPU calculation. D32 stores a float;
D24 stores a quantized integer. A migration must verify the actual written
integer/readback and EQUAL/LEQUAL behavior before assigning image differences
to a particular division, interpolation, or GPU precision rule.

## Live attachment and consumer inventory

| Current path | D24 migration requirement or risk |
| --- | --- |
| `Renderer::Impl::create_device`, renderer.cpp:545–614 | Selection is graphics+present and prefers D32, then D24, then D16. Exact alias requires integer D24S8, graphics+compute, attachment/sample/transfer-source support, and the actual combined image usage/extent query. A D16 or D32 fallback cannot silently satisfy the exact alias contract. |
| `make_depth`, renderer.cpp:1600–1624 | Existing ordinary 2D, optimal, exclusive, one mip/layer/sample matches the alias shape. Add transfer-source usage. Keep the DEPTH-only sampled view for fog; do not replace its descriptor with a combined depth/stencil sampled view. No depth-comparison sampler or linear depth filtering is used. |
| `make_pipeline`, renderer.cpp:1637–1683 | Depth CLEAR/STORE is explicit, stencil is DONT_CARE/DONT_CARE. LOAD continuation changes only depth/color load ops. Retaining initialized stencil requires source-backed initial clear ownership, stencil STORE, and stencil LOAD across every continuation sharing the image. Merely changing format leaves stencil undefined. |
| Main/split/sky/FP clears | In-pass clears are DEPTH-only. They must not become blanket depth+stencil clears: a source depth clear need not erase stencil. Accumulation reuses main depth and copies the original attachment description, so its clear/load policy must be reviewed independently. |
| Fog and patchy | `fog.frag`, `patchy_composite.frag`, and `bsp_patchy_composite.frag` fetch normalized depth from a DEPTH-only view. Existing barriers include stencil when the format is D24S8. The conversion uses round(depth × 0xffffff) and ignores stencil for these fog modes; it is not an exact general RGBA alias producer. |
| Water | Main format also reaches the independent 512x256 reflection depth image/pass. Rebuild its dependent pass/framebuffer/pipelines. Its fixed target extent remains source-backed; a main-depth format change is not a resolution or water-target policy change. |
| Scope | The 64x64 scope target copies color, has no depth attachment, and disables its copy pipeline's depth test/write. Its resumed main LOAD pass must preserve both aspects. Scope target dimensions and color-copy protocol remain unchanged. |
| First person | Atmosphere views preserve world depth and use the verified 12D50 foreground c0..3 / 0.15 W range. Non-atmosphere previews have a different existing depth-clear branch. D24 migration must preserve this scheduling and source camera control rather than apply one new clear policy to both. |
| Particles | Their actual raster state selects depth test/write/function and their shader already uses the pinned D24 conversion. Renderer-owned particle pipelines are rebuilt with the main render pass. Weather point/plate prototypes use their own D24 passes and are still isolated. |
| Shadows | Fixed128 generation/filter are color-only. Apply uses the main continuation EQUAL/no-write path. Preserve full source camera admission and rebuild the continuation-dependent pipelines; D24 equality is an important migration check, not a reason to loosen the comparator. |
| Screenshot/readback | The live screenshot copies color only. No live original-style depth/stencil readback exists. Add diagnostic copies of the two aspects with their different buffer layouts and explicit transfer-to-host visibility; do not reinterpret four bytes of sampled float depth as D24S8. |
| Resize/destruction | Existing swapchain recreation waits device idle, then destroys dependent shadow/particle/scope/postprocess services, framebuffers, passes, views and images. A live alias must retire/destroy its views/buffers before its source image or caller fence dies. Resize invalidates borrowed alias bindings. |

The main fog sampler and BSP patchy depth sampler use nearest/clamp. D24 linear
filter support is not needed for those existing consumers. Alias output needs
its own RGBA8 sample/storage views and storage-buffer limits; these checks
already exist in `SourceWeatherDepthAlias` and must not be bypassed by opaque
renderer handles.

## Stencil packet evidence and missing chronology

Files `appearance-residual-replay/source/8097-pipeline.json` and
`8102-pipeline.json` describe **Zanzibar sky A/B**, not planar fog. Existing
`APPEARANCE_RESIDUAL_DIAGNOSIS.md` identifies them as sky_dome/clouds_low before
moon8110. Both captured front/back packets carry compare function1,
KEEP fail/depth-fail, REPLACE pass, reference/read-mask/write-mask FF. Their
literal title counterpart is `15370` mode2: enabled, ALWAYS, KEEP/KEEP/REPLACE,
FF reference and masks (`unknown_020560.cpp:150–165`). The report retains raw
capture enum numbers rather than treating them as VkCompareOp values.

Both packets also enable LEQUAL without depth writes, use RGB-only
SRC_ALPHA/INV_SRC_ALPHA blending, no culling, depth clamp enabled, depth clip
disabled, zero depth bias, and single-sample attachments. Index strips contain
74 and 49 indices. Saved post-VS streams are 8,880 and 6,000 bytes, stride240;
the actual VS/PS blocks, index bytes and texture mip bytes are available beside
the pipelines. Full bound pre-VS vertex buffers and per-event before/after
stencil images were not saved by that exporter. Recovering an actual authored
geometry identity or exporting those immutable RDC resources is a separate
input task; a triangle mean or a fullscreen quad is not a substitute.

The fragment has an inclusive window-region demotion gate. It reconstructs W
from flat triangle positions and perspective barycentrics, adds source
slope/offset, and clamps to clipRange because depth clipping is disabled. There
is no active rounded-alpha discard in these two programs. Stencil replacement
therefore depends on the real covered fragments, helper/discard behavior and
depth result; pass metadata alone is not a complete stencil image.

`15370` also has disable, replace-zero, and low-bit comparison/write modes.
The live Renderer currently implements none of those stencil tests/writes.
The sky observations do not attribute Lockout's later 6,255 FF pixels, prove a
matching clear history, or authorize applying mode2 to every map/background.
Snow and the bounded shadow Apply path independently disable stencil while
preserving prior bytes. Source stencil ownership must be an explicit packet
and phase input until its complete original caller is reproduced.

## Smallest supported implementation sequence

1. Introduce an explicit integer-D24 renderer resource contract for the proved
   setting. Require D24 attachment/sample/transfer-source image support and a
   present graphics+compute queue. Reject unavailable capabilities when exact
   weather aliasing is requested. Keep other depth modes outside that claim.
2. Allocate D24S8 with the current extent and add transfer-source usage. Retain
   depth-only sampling; preserve source-backed initial stencil and STORE/LOAD
   it through every main/accumulation/continuation transition. Recreate every
   main/water/particle/shadow continuation pipeline at the format boundary.
   Source clear and stencil mode packets are prerequisites to claiming original
   stencil parity; an initialized native-only test state must be labeled so.
3. First validate actual D24 depth/stencil copies, fog samples, FP overlap,
   shadow equality, split-view clears, fixed water/scope targets, resize/reuse
   and rejection on unsupported formats. Retain exact raw integer results and
   image differences. Treat a source W-expression adjustment as a separate
   measured change; the CPU domain check does not establish a current visual
   omission or host lowering.
4. Only then expose the actual image metadata to the alias. Record outside a
   render pass after preceding depth/stencil writes, restore the combined
   layout, and resume a LOAD pass. One submission/fence must cover the alias
   and every plate/point/diagnostic consumer. Existing fog restores the source
   to DEPTH_STENCIL_ATTACHMENT_OPTIMAL, a supported alias layout.

There is a concrete fence integration hazard: `Renderer` waits its frame fence
at draw entry but resets it **after** command recording. The alias validates an
already unsignaled fence during `record`. Moving reset earlier without a
failure/submission policy can leave a fence permanently unsignaled. Resolve
admission/allocation first and design the reset/submit/exception boundary
explicitly; do not remove the alias check or claim work was submitted.

The alias also forbids a second record while pending. Multiple source views
must retain separate per-view producer/binding storage, or use a separately
proved multi-record API; repeatedly overwriting one alias between queued
consumers is not admitted. The current alias uses approximately nine buffer/
image bytes per pixel before Vulkan allocation alignment, so full-surface
per-view storage at4K needs an explicit memory budget, not a guessed cap.

Live weather simulation/publication, correct per-view submission placement,
bitmap readiness and original stencil/clear chronology remain independent
activation inputs. This plan closes the depth-resource boundary; it does not
claim full native weather or all21-map appearance parity.

## Reproduction

Run `.tools/python/bin/python tools/audit_world_d24_requirements.py`. It verifies
pinned primary-source blobs, reads the two immutable source RAM exports, pins
the actual capture resources/states, disassembles original format routines and
the retained native SPIR-V, and runs the full CPU expression diagnostic in a new
analysis directory. Input hashes are checked again before publishing the report.
No current shared build directory is modified.
