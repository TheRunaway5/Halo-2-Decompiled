# Fixed object-shadow GPU path

The native `SourceShadowRender` module implements the **explicitly supplied**
fixed `47FDA0` / feature-mask `0x14` target and pass lifecycle. It does not select
live casters, build their hierarchy, choose secondary receivers, or reproduce
render-cache history. `Renderer` accepts an optional explicit source plan;
multiplayer runtime does not yet produce or activate that plan.

The original executable, authored cache payloads and retained original20 GPU
capture agree on the color-silhouette path. It must not be replaced with the
separate target9 depth/comparator path or a procedural circle.

## Authority and reproducible evidence

The executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Source function listings are under `analysis/retail/functions`; bounded exact
instruction disassemblies and GPU exports are in
`analysis/multiplayer-shading/original-xemu/shadow-gpu-audit`.

- `tools/audit_shadow_gpu_assets.py` retains all21 maps' authored shadow programs
  and passes: 42 vertex-program records, 12 distinct compiled payloads. These
  are authored availability counts, not executed draw counts. `class_index` is
  the compiled block ordinal; the separate stored classification field is zero
  in every supplied slot.
- `tools/audit_shadow_gpu_state.py` executes unchanged `44370`, `14BC0`,
  `1DCF0`, `1CD30`, `3FDB20` / `3FD9C0`, `14B60`, `15680`, `495E0`, `496A0`,
  `496F0` and `49740`. All24 explicit presentation / AA / multisample cases
  preserve target10's 128×128 viewport, color-only binding and color clear.
  D3D submission endpoints are recorded services; independent `15370` stencil
  policy is an explicit boundary. This does not prove live feature/camera state.
- Read-only retained-RDC `inventory.py` and `export.py` retain 11 actual original20
  shadow draws, raw input/index/post-VS buffers, vertex and pixel banks, texture
  bindings, raster/depth/blend state and DebugVertex traces. Numeric resource IDs
  identify the xemu Vulkan translation, not original D3D target indices.
- `tools/generate_shadow_vertex_fixture.py` freezes 24 actual DebugVertex cases
  across those 11 draws, 192 literal registers per draw and five authored compiled
  payloads. Opcode histograms independently select the rigid-local1,
  rigid-boned2/3, two-weight4, three-weight5 and world-application0 variants.
  Classes2/3 have identical source bytes. `source_shadow_vertex_tests` compares
  seven consumed outputs: CPU all 672 words bit-exact; Vulkan all nonposition
  outputs bit-exact, with five position words differing by at most 2 ULP in the
  measured NVIDIA run. The fixture retains the actual xemu pixel-to-clip wrapper
  separately from the original NV2A program outputs.

The exported white clear and finished original silhouette are
`original20-shadow-7703.png` and `original20-shadow-7758.png`. Their successful
exports are recorded in `original20-shadow-images.json`.

`tools/generate_shadow_silhouette_fixture.py` freezes the complete seven-draw
generation interval, not a reconstructed outline. The fixture preserves 4,580
fetched vertices, 6,631 original triangle-strip indices, the original compiled
classes1/2/4/5, full 192-register banks and inline attributes, and both original
textures with all19 mip levels. All176 independently exported DebugVertex input
words match the decoded raw declarations. The texture bytes are unchanged;
their retained alpha values are255. The alpha-zero control changes a separate
test upload, never the source fixture.

`original_shadow_silhouette_gpu` submits these actual draws through
`SourceShadowRender` and the existing NV2A vertex / texture / combiner lowering.
It uses the literal captured triangle-strip, front-face/cull, depth-clamp,
sampler and RGBA-write state. All65,536 bytes of the generated128×128 target
match the retained source target, including alpha and silhouette edges:
2,057 black pixels, zero differing pixels and zero differing bytes. Both target
hashes are `7b6a6fadfecd763fedf085ea20b3ad0a2097f52e007f86a41bcd1bf7a60ba8bf`.
The two presentation extents320×240 and853×479, unfiltered and two-pass-filtered
targets, plus alpha-zero controls, compare524,288 bytes exactly. Required Vulkan and synchronization validation
reports zero errors on NVIDIA RTX5080. All six `original_shadow_` tests,
including the optional production renderer fixture described below, pass in
normal and AddressSanitizer / UndefinedBehaviorSanitizer builds with required
Vulkan and synchronization validation.

The complete frozen generation fixture is
`tests/fixtures/retail_shadow_silhouette.bin`, SHA-256
`d79435de1b62550b848041ca7265f9fb7f0f7c12493419ccee556fc980c7c550`.
Its expected target is `retail_shadow_silhouette_expected.bgra8`. Reproducible
input/state/hash evidence is in
`original20-shadow-silhouette-fixture-proof.json`; raw mip and target exports are
in `original20-shadow-generation-textures.json` and
`original20-shadow-target-7758.json`.

`tools/render_shadow_silhouette_comparison.py` produces
`analysis/multiplayer-shading/shadow-silhouette-comparison.png`: original and
native RGB enlarged exactly4× with nearest sampling. Adjacent
`shadow-silhouette-original-128.png` and `shadow-silhouette-native-128.png`
preserve actual RGBA. The display omits alpha only for legibility; the full-byte
comparison includes it. This is an exact retained generation-target replay,
not evidence of live caster selection/history or complete world application.

## Original two-pass color filtering

Generation is followed by the original `222A0` filter before application.
`211A0` orders target10→scratch11→target10. Both original `1D770` allocations
are linear128×128 `A8R8G8B8`; scratch11 is independent of presentation size.
The first width is2 and the second width is3: `34100` supplies four two-axis
offsets `(±1,±1)` and then `(±1.5,±1.5)` in source pixels. `34A90` obtains the
full0..128 source rectangle from actual resident metadata through `1DD60`.
`20F30` builds three ordered half-scale combiner stages, averaging the two
sample pairs and then the pair results in both RGB and alpha. The shader is
the original texture/combiner program, not a fitted blur kernel.

`tools/audit_shadow_filter.py` executes the untouched `1D770`, `222A0`, `211A0`,
`20F30`, `1DD60`, `34A90` and `34100` arithmetic. All32,800 coordinate words
match the independently evaluated source expression. D3D allocation, sampler
and submission services are recorded boundaries. Actual retained draws7959
and7973 independently corroborate the program, four-vertex quad, source input
copies, sampler and resulting target. U/V use CLAMP_TO_EDGE, min/mag LINEAR,
mip NEAREST; the unconsumed W address is REPEAT. The source pixel program SHA-256
is `908fa6f07d1af29c82e98788db41e39d80430499d6f73498f428d82ba7dbaaa8`.
The literal `retail_shadow_filter.bin` fixture is checked by the native plan
test; source evidence is `shadow-filter-original-proof.json` and
`original20-shadow-filter-export.json`.

`source_shadow_filter_passes` retains that program and exact tap order.
`source_shadow_filter.vert` / `.frag` lower it through the existing texture and
combiner implementation. The module owns scratch11 and both immutable filter
descriptor/uniform banks; both passes write the complete RGBA target without
depth or blending and preserve read/write transitions. `target_view()` is the
finished, filtered target10.

All65,536 bytes of the native final filtered target match the actual original
target supplied to receiver draw7988. There are806 black pixels,91 distinct
RGBA colors and zero differing bytes or pixels. Both hashes are
`bcba5b6d62a2900af7b9c1b8a7696322452e9b24c42bd7290fc57268644bb46d`.
The expected raw target is `retail_shadow_filtered_expected.bgra8`.
`tools/render_shadow_silhouette_comparison.py --filtered` produces the inspected
`analysis/multiplayer-shading/shadow-filter-comparison.png`; adjacent original
and native128 PNGs retain actual RGBA, while the phone comparison shows exact
nearest4× RGB enlargement.

The retained full receiver intervals58..73,4042..4057 and7988..8003 each change
**zero scene color bytes**. Exported before/after scene color and D24S8 depth
are retained in `original20-shadow-receiver-images.json`. They corroborate the
actual receiver inputs and unchanged result, but cannot establish visible
receiver application appearance. No native full-scene application comparison
is claimed from those intervals.

## Source scheduler and exact fixed target

`2BCD0` submits layers8/20, calls `2C370`, then `335E0`; dynamic-light `31C80`,
fog/transparency and first person `404B0` occur later. `335E0` is per-view work,
conditional on `226D0` / `335C0`, and uses `33400`'s retained candidate cache and
budget before `32350`. Existing `source_shadow` helpers recover selection,
root fade, coverage, bounds/frame and consumed constant arithmetic; their caller
inputs remain explicit.

`32350` uses actual root/cached lighting opacity, root/child/sibling bounds,
virtual-light frame and view-volume admission. `1320F0` constructs a secondary
collector; `165A20`, `2CBC0`, `44720` and ordinary source part scheduling establish
caster/receiver geometry. Generation and application require both layer groups.
Generation is layer10; application is layer11. A special local-shadow branch
through `32AC0` / `32CE0` remains separate.

For mask `0x14`, `44370` selects target10. `1D770` / `1DF20` allocate one linear
128×128 `A8R8G8B8` color target10 and matching scratch11 (source format byte `0x12`). `14BC0` / `1CD30`
bind it with **no depth attachment**. `3FDB20` gets its actual dimensions and
sets viewport `(0,0,128,128,0,1)` independent of presentation extent. `14B60`
converts the original clear selector to all-RGBA color bits `0xF0`, with color
`0x00FFFFFF`: white RGB, zero alpha. It does not clear world depth.

`495E0` emits `ZMIN_MAX_CONTROL=16`, polygon offset fill enabled, slope2,
bias550 and the explicit source multisample flag. `496A0` restores the controls.
These global source state writes coexist with the authored no-depth generation
pass. In the retained actual GPU pass, depth test/write are disabled and no depth
image is attached; the generated color silhouette does not depend on that bias.
The xemu translation has depth clamp enabled. The native callback pipeline must
retain source clipping/culling; the module does not choose a guessed cull mode.

## Authored generation and receiver binding

The bounded generation is `shadow_generate` implementation0,
`shadow_buffer_generation`: stage0 is the original material bitmap, PROJECT2D,
with texture alpha-kill bit4. The authored combiner emits black RGB / alpha1 for
surviving texels. Alpha zero discards before combiners. Blend is disabled;
RGBA writes are enabled; depth test/write are effectively disabled.

The bounded receiver is `shadow_apply_combined` implementation4,
`shadow_buffer_application_lightmap`. Stage0 external8 resolves to target10;
stage1 is its authored material bitmap. Stage0 uses linear sampling, BORDER
addressing and border color `0x00FFFFFF` (white RGB / alpha0). Because target10 is
linear, projected stage0 S/T are in pixels: normalize with its actual128 extent
when sampling a normalized Vulkan sampler. Retained original20 Psh texture scale
and sampler/binding exports corroborate this addressing contract.

The receiver uses `DST_COLOR` / `ZERO`, RGB writes only, world depth `EQUAL`,
no depth write. Pixel extern17 supplies shadow opacity, extern22 the source
unit scalar; opacity `.75` is quantized to alpha191/255. `496F0` separately emits
source shadow comparator `0x204`, depth-clip control16 and current caller culling;
`49740` restores those globals. Original `32350` at329C5 explicitly passes
BX0 to `15370` before receiver submission, disabling stencil in this fixed path.
The fixed color texture has no depth comparator
sampler; its authored receiver program evaluates color/opacity.

`3A550` uploads c31..39; the fixed compiled programs consume c31..36. `48EE0`
uploads c40..48. `SourceShadowUniforms` retains the consumed six projection and
nine extra vectors. Receiver additionally consumes actual world/camera c0..3,
c10/c11 and vertex normal; generation uses authored c18/c20, compression
c170..172 and, for captured model variants, actual c50+ palettes and skin inputs.
The retained original20 final group comprises seven generation draws
7713/7721/7729/7737/7742/7750/7758 and four receiver draws7988/7993/7998/8003.
Generation's actual output is B8G8R8A8_UNORM128×128, no depth, blend off, RGBA;
receiver output is the scene with retained world depth, multiply blend, RGB.

All seven generation draws retain the actual main-camera c7..11 despite drawing
to the128 target. In this retained640×480 ordinary view, c7 contains camera eye
`(-10.1654806137,13.2628898621,.7419255972)`; c8 is
`(320,0,0,320.53125)`, c9 is `(0,-240,0,240.53125)`, c10 is
`(320,-240,16777215,0)` and c11 is `(320.5,240.5,0,0)`.
`viewport_constants` reproduces c8..11 for that measured ordinary640×480 input.
This proves neither other feature states nor substitution with the128 target
viewport. Captured skin classes read c7, the world-generation class0 reads none
of c7..11, and world-application class0 reads c10/c11. The hook must preserve
the explicit full camera bank; it must not derive skin inputs from the virtual
light eye or replace the bank with fixed-target viewport constants.

## Native module and caller seam

`include/halo2/source_shadow_render.hpp` / `src/source_shadow_render.cpp` own
B8G8R8A8_UNORM128×128 target10 and scratch11 sampled/color images, their views,
a color-only generation CLEAR pass, fixed filter pass/pipeline/framebuffers,
external read/write dependencies and LOAD scene continuation.
The standalone `halo2_shadow_render` library links the source/cache and Vulkan
interfaces. Its GPU tests do not depend on private Renderer or frontend state.

`SourceShadowRenderContext` supplies actual device, physical device, scene extent,
compatible color+world-depth LOAD pass and optional compiled shader directory.
The default shader directory is supplied by the build. `SourceShadowContinuation` supplies
its framebuffer and actual receiver viewport/scissor. Each ordered
`SourceShadowSubmission` carries `SourceShadowUniforms` and ordered caster /
receiver records. A record retains `SourceShadowMaterial` and a bind-and-draw
callback. Both groups must be nonempty; otherwise the active scene pass is left
unchanged. Root submissions run Generate→Filter(10→11→10)→Apply sequentially,
then leave the continuation scene pass open. `filter=false` is an explicit
generation-stage diagnostic control; the actual fixed path always filters.
The module checks selected
source program families, implementation and authored raster contract before
changing command-buffer render-pass state.

Callbacks reuse the renderer's original VP/texture/combiner lowering, image
uploads and pipeline/descriptors; this module does not duplicate them. They must:

1. Bind the correct generation/receiver-compatible pipeline and original ordered
   part/subpart geometry, source class or independently proved CPU world stream.
2. Bind c31..33 through the ordinary material register bank; bind c34..36 and
   c40..48 through the existing extended register bank. The fixed programs do not
   read c37..39. Preserve authored c18/c20, actual camera bank and actual skin /
   compression inputs or the independently proved equivalent world-space variant.
3. Bind each root's actual prepared opacity and images, including external8's
   target view and authored custom border sampler. Descriptor/constant storage
   used by an earlier root must remain distinct until its GPU work completes.
4. Restore caller scene bindings after recording, using its original stencil,
   viewport and clipping policy. LOAD continuation must preserve world color/depth.

`source_shadow_render_tests` exercises actual Vulkan target creation/clear,
source generation/application pixel-program words through existing texture and
combiner lowering, alpha-zero discard, pixel-UV normalization/custom border,
receiver multiply, RGB alpha preservation, equal-depth/no-write admission,
repeated roots sharing one target, and two presentation extents. The required
validation/synchronization run passes17,416 controlled samples on NVIDIA RTX5080. Controlled
quads isolate raster/target behavior; **this is not a full reproduced native
caster silhouette**, nor a claim of original scene geometry/admission parity.
The separate complete retained-silhouette test above provides the full target
comparison while keeping live submission and receiver admission explicit.

## Remaining live inputs and precision boundary

Production must recover/represent actual root eligibility and hierarchy,
current bounds/opacity/death/camouflage, candidate/PVS priority/history, selected
shadow geometry LOD/shader LOD, secondary OBB/frustum/part/subpart receivers,
geometry modifier, special local-shadow branch, source feature/AA/cull/stencil
state and per-view schedule before connecting complete live shadows. Native
object/cache and physics state cannot be substituted with constant/default
inputs. Target9's depth/comparator implementations and unsupported application2
have separate source semantics and are not admitted by this module.

The pinned XBE instruction proofs are CPU producers. Retained RDC and native
GPU comparisons use xemu's translated shaders and a native Vulkan device. They
do not prove physical NV2A reciprocal/filtering/raster precision. The small
measured ordered-dot position difference is retained explicitly, not fitted away.

## Pure renderer plan

`source_shadow_plan.hpp` / `.cpp` provide a Vulkan-independent submission seam.
`SourceShadowPlanMaterial` pairs each preallocated Mesh material slot with its
actual prepared `SourceShadowMaterial`. `SourceShadowPlanDraw` retains its
ordered triangle span and completed pixel / vertex material bindings;
`SourceShadowPlanRoot` retains the actual projection and generation / receiver
records. `SourceShadowPlan::camera` is the original twelve-vector c0..11 bank.
It includes c10/c11's actual viewport / feature state rather than silently
reconstructing those inputs from an approximate camera.

`validate_source_shadow_plan` checks resident identities, phase / raster / target
contracts, complete geometry and the bounded world-coordinate program reads
before any command-buffer mutation. It rejects skin / palette programs that need
unrepresented high registers. The actual captured skin classes remain covered by
the independent vertex fixture; this validation does not prove that a particular
live CPU pose is their equivalent world stream.

`source_shadow_plan_bindings` copies only the completed program and small constant
banks, preserving the caller's animation, colors and opacity. It overlays
c31..36 / c40..48 without copying textures. Each root's returned bank requires its
own queued GPU storage. The unconsumed c37..39 gap is zero storage, never admitted
as an actual shader input. The native plan test replays all ten retained receiver
vertices through this seam: all 280 consumed output words are bit-exact. It also
checks omitted groups, invalid slots / stages / geometry and independent root
snapshot storage.

`source_shadow_plan_vertex_bank` merges the actual camera into a separate
`VertexProgramBank` with first_constant0,49 vectors and `camera_inputs=true`.
It preserves completed material c18..33 and original globals c34..48.
The production binding7 consumer explicitly overrides c0..11 before the
synthesized camera path, while c18..33 retains the normal material-bank priority.
The existing first34 global bank in `SourceShadowPlanBindings` is unchanged.
Unrepresented c12..17 and c37..39 remain zero storage and are rejected if a
selected shadow program reads them. Literal source camera/global/material
packing and invalid bank controls are covered by the native plan test; the
independent production GLSL bridge fixture is reported separately.

## Optional production renderer entry point

`Renderer` now accepts a constructor table of resident `SourceShadowPlanMaterial`
records and an optional `ModelView::source_shadows`. The authored table must
outlive the renderer. Construction checks the actual shader datum, selected pass,
vertex program, texture states, all bitmap/mip bytes and resident animation chains.
Only the explicit target10 stage is exempt from resident bitmap comparison.
Every nonempty plan requires the ordinary view's actual W-depth context and a
source c0..6 bank identical to the plan's camera; absent context and mismatched
c3 are rejected before command recording. Cull and depth-clamp are required
actual caller inputs, never inferred from object mirroring.

`SourceShadowDrawStorage` uploads a distinct completed material uniform,
full source vertex bank and descriptor set for every draw/root/view. Material
slot reuse cannot overwrite earlier queued constants. It reuses the production
`world_prt.vert` / `world.frag` interpreter, original images and samplers.
`Renderer` retains literal draw order, submits Generate→Filter→Apply after
world/environment and before fog, late transparency and first person, then
restores the view's push constants and viewport. Generation uses a color-only
pipeline; application preserves original EQUAL/no-write/RGB/DST_COLOR/ZERO state.
Only queued external8 stage0 receives pixel-coordinate normalization by the
actual128 target extent; ordinary accumulation metadata remains unchanged.

Native category10 identifies shadow programs. Category3 is specific to the
stencil texture pass's substituted diffuse input and must not replace original
shadow oD0. Generation class0 reads v1 as UV through c172/c18/c20;
application class0 reads v1 as the decoded original NORMPACKED3 normal through
DPH c41. Original `1C330` assigns registers from class+8 semantic lists
(class+4 count, stride0x1C); the literal world declarations are generation
`[0,3]` (position/UV), application `[0,4]` (position/normal), with formats
selected through `35790`. The selected pass now retains that literal declaration,
and the typed production input loader assigns its registers from the list;
generation retains UV. The queued phase marker remains explicit, while the
typed mapping takes priority over the previous phase-specific adapter. The
original BR receiver word34 (`04082420`) consumes oD0 in its
attenuation; retained7988 vertex91 supplies RGB `.4894518256` and alpha
`.5171411037`. Substituting prepared diffuse `(0,0,0,0)` instead gives a neutral
factor. Category10 preserves the original generated oD0 through the production
combiner consumer without changing authored shader/program/constants.

The renderer integration fixture uses original BR generation/application passes
and bitmaps with explicit controlled camera, frame and geometry services. It
checks six fresh/reused view pairs and34 isolation/admission controls: a visible
source receiver response, zero-oD0/opacity controls, repeated
slots across two roots, per-player isolation, fresh/reused resize behavior and
rejected raster/camera/bitmap bindings. It is an integration proof of supplied
services, not a reproduced original gameplay scene. Complete runtime caster,
receiver, history and registration producers remain open; no native gameplay
shadow activation or full scene appearance parity is claimed.

The real renderer's controlled two-view output is retained as
`analysis/multiplayer-shading/shadow-renderer-controlled-853x479-2p.png`.
Its camera/frame services are explicit test inputs. Independently, the copied
original OpenGL guest's tick12475/frame111186/view0 capture corroborates the
29 frame values and60 consumed uniform values bit-for-bit; the complete
125-word raw capture report is
`analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl/uniform-capture.json`.
The production-linked live-phase fixture now also computes both original
`4BAF0` fade calls through `source_model_fade`, independently checks their
observed bytes and binds the computed second bytes into the downstream root
states. The version3 fixture also computes the corroborated local-player
priority through `source_local_player_priority`. Its135 checked source words
include the125 downstream words, seven fade proof words and three priority
call/identity/AL words. Normal and ASAN/UBSAN/leak checks pass; wrong-metric and
wrong-local-player controls respond. `source-shadow-live-phase.md` retains the
complete proof and remaining metric/state boundary.
That source phase is distinct from the retained original20 Vulkan target
fixture, and does not establish native runtime submission chronology.

`SOURCE_SHADOW_CASTER_WORLD_PARITY.md` independently identifies seven retained
original20 generation draws and checks their actual WORLD inputs against the
existing class0 program. All392 sampled output words match; its full Chief
section19 palette/skin proof checks18,816 raw-source arithmetic words and16,128
actual GPU nonposition words exactly across672 vertices. Actual wrapped GPU
positions retain the measured0.0625005-target-pixel /7.6443e-6-depth bound. This
does not select a live caster, construct its palette or prove a complete frame.
