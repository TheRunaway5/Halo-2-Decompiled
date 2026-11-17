# Path Tracing renderer

The ordinary and pause Video Settings **Graphics Mode** row opens a submenu
listing **Original**, **pStencil**, and **Path Tracing**. It focuses the active
mode and shows its name; choosing a mode applies and saves it, then returns to
Video Settings. Back cancels without applying a change. Original remains the
default. This is a native presentation extension over current source-admitted
geometry and assets; Xbox TU1.5 remains the reference for Original rendering.

The setting applies to game-world surfaces. The source main-menu backdrop and
its model widgets retain their authored renderer, independently of the saved
game-world graphics choice.

## Code map

- `include/halo2/path_tracing/scene.hpp`: CPU/GPU layouts, material adaptation,
  BVH construction, image refresh and the small CPU intersection reference.
- `scene.cpp`: binned surface-area BVH, source RGBA8/BC image atlas, authored reflection
  strengths, per-frame geometry/light refresh and finite input bounds.
- `include/halo2/path_tracing/material_residency.hpp`: lazy image admission from
  the complete ray geometry while retaining the source bank's root slot IDs.
- `include/halo2/path_tracing/emission.hpp`, `material_emission.hpp` and
  `shaders/path_tracing/emission.glsl`: exact admitted self-illumination
  expressions, independent authored mask images, UV transforms and samplers.
- `emitter_sampling.cpp`, `include/halo2/path_tracing/emitter_sampling.hpp` and
  `shaders/path_tracing/emitter_sampling.glsl`: bounded source-group sphere
  proposals and the primary diffuse BSDF/cone mixture. The small proposal
  table follows the emissive triangle index list; it never changes radiance.
- `color_guide.cpp` and `shaders/path_tracing/color_guide.glsl`: cached authored
  lighting hue from source lightmap UVs or qualified static vertex radiance;
  exact source-program admission also tags the supported secondary diffuse
  irradiance decode in the existing material record.
- `shaders/path_tracing/source_indirect.glsl`: the source-qualified secondary
  static irradiance boundary, reached through the existing closest-hit ray.
- `include/halo2/path_tracing/surface_ray.hpp` and
  `shaders/path_tracing/surface_ray.glsl`: starting-surface exclusion for
  smooth diffuse receivers, with a matching CPU intersection check.
- `include/halo2/path_tracing/surface.hpp`, `material_surface.hpp`,
  `unlit_uv.hpp` and `shaders/path_tracing/surface.glsl`: source-qualified
  opaque detail, three/four-stage terrain texture blends, foliage coverage, native chained UV transforms and
  shared primary/secondary material evaluation.
- `geometry.cpp`: exact existing model skin arithmetic for current WORLD poses.
- `source_lights.cpp`: actual native light-pool/effect inputs and projected-light
  frustum planes; no camera-position light or invented directional sun.
- `include/halo2/path_tracing/sky_lighting.hpp`: selected authored sky metadata,
  loaded independently of the retail object-lighting availability bit and
  released when Path Tracing is disabled.
- `render.cpp`: Vulkan resource ownership, descriptors, uploads, quality bounds,
  graphics pipelines, the private visibility depth image/buffer and the native
  sky environment cubemap. It also snapshots previous lighting history on the
  GPU into immutable storage binding9. Resources live only while the optional
  mode is active.
- `shaders/path_tracing/world.vert`, `world.frag`, `scene.glsl`: primary surface
  rasterization, BVH traversal, shadow any-hit, diffuse transport and reflection.
- `shaders/path_tracing/depth.frag`: source-W visibility prepass with the same
  primary alpha/stipple rejection, without ray traversal.
- `shaders/path_tracing/lighting_filter.glsl`: HDR lighting records and guided
  resolve, world-space history reprojection and camera-attached foreground
  guides. `world.frag` also compiles as the colorless `lighting.frag.spv` write
  variant; both roles are included in production shader qualification.
- `cmake/PathTracing.cmake` and `tests/path_tracing/`: build and CPU/GPU checks.
- `src/rendering/renderer/renderer.cpp`: the existing renderer's narrow handoff
  for resident BSP triangles, current posed objects, source lights and camera;
  compact sky aliases replay the ordered native sky draws into the RT cubemap.

## Geometry and light presentation

Secondary model geometry uses the primary draw's sampled WORLD node bank with
the native collector's selected sections and LOD. Attached light positions and
projector planes follow that same presented owner/node. A successful native
first-person marker uses its actual completed light frame and the foreground
skin's sampled marker frame. These presentation operations borrow completed
state; they do not advance source clocks or resample native animation. Full-body
material roots used only by secondary rays also enter the ordinary per-frame
color, emblem and shader-animation selection. The native builder regression
checks fractions0,.25,.5,.75,1; attached-light and projector checks cover
translation, rotation, uniform scale and unchanged source state.

`tools/validate_path_tracing_campaign.py` compares independent Original/PT runs
at declared source intervals and render fractions. It checks exact camera/tick
receipts and sampled actor or cinematic scene-object banks plus complete posed
vertex hashes. `--require-interpolated-geometry FRAME` additionally requires
nonempty geometry with an observed interpolated pose in that final screenshot;
an empty bank or an earlier moving object cannot satisfy that check.

## Lighting and material policy

Qualified native model surfaces use per-view, full-generation owner material
aliases. The primary material index and its matching BVH triangles share the
same completed C24 reflection mask, change colors and ordered UV transforms.
Each alias stores68 metadata words ahead of shared source texels; selected
bitmap frames and qualified ambient bump images are resident once per image.
The dynamic lighting child's exact packed ambient RGB and current bump UVs
also reach secondary hits. Owner order changes preserve live material IDs,
and canonical root IDs remain available for full-body provenance receipts.
Camera/fog constants do not enter surface image fingerprints. Unsupported
completed program-control or alpha-coverage changes are rejected rather than
silently evaluating the previous source graph. CPU checks use actual opening
cruiser and Master Chief materials; the GPU fixture checks two shared-root
primary and reflected owners, reordering and independent next-frame changes.

Primary visibility uses rasterization. Secondary and shadow rays intersect
actual WORLD triangles through a software BVH in Vulkan shaders, including
geometry outside the primary viewport. The default is one sample and one bounce.
Video Settings exposes **Path Tracing Quality** while this renderer is selected;
its Ray Samples Per Pixel and Bounce Depth choices are each capped at one or two
and saved independently. Each sample follows one stochastic diffuse or GGX
reflection continuation per bounce. A primary direct-light shadow query and at
most one closest-hit plus one direct-light shadow query per bounce bound the
work to `samples * (1 + 2 * bounces)` BVH queries per shaded fragment: at most three
at the default 1/1, or ten at the maximum 2/2. Misses and zero throughput can
terminate earlier. The primary surface samples the authored light bank directly;
its existing continuation samples sky misses and emissive hits. This avoids
alternating bright sunlight, blue sky and dark emitter samples at one sample
per pixel. The primary diffuse continuation mixes its cosine hemisphere with
a cone enclosing a nearby authored emitter group. Both analytic PDFs determine
the transport weight, so all sources remain reachable without adding a ray.
The source-cone probability caps at two thirds. Existing group power and
qualified static/sky irradiance provide sampling hints: weak emitters do not
add cone variance to a bright background. The same probability enters both
sampling and its complete mixture PDF; no hint multiplies lighting energy.
At most eight groups are considered, and views inside the chosen source sphere
retain cosine sampling. Secondary surfaces share one next-event query between source lights,
emissive triangles and sky. Cosine-sampled sky illumination at the last hit lets
a canyon wall reflect its authored color at one bounce. Later diffuse
continuation misses omit the environment contribution already covered by
next-event sampling; specular misses retain it. World and foreground phases, or equal-depth draws, can shade
the same framebuffer coordinate more than once. This does not require Vulkan
hardware ray tracing extensions.
Frame-varying ray seeds converge through bounded temporal lighting histories.
When the scene has one authored light, its deterministic primary direct lighting
uses current-frame visibility outside that history. Moving casters therefore
update their direct shadow immediately while indirect lighting retains its
noise reduction. Multiple sampled lights use separate direct diffuse/specular
histories. Conservative bounds of actual previous/current occluder positions
invalidate only receivers whose light segments cross those bounds; a moving
light invalidates receivers within its old/new influence. These checks add no
BVH rays. Unrelated animation retains stationary direct and indirect samples.
Alpha comparison/cutoff, alpha-kill images, source transforms and alpha-caster
vertex UV changes also invalidate affected direct visibility. Banks above64
lights conservatively restart direct visibility when geometry changes.
Primary and secondary shading normals follow the geometric face toward the
incoming ray. An authored smooth normal may cross the view horizon on a
front-facing surface; it must not independently flip underneath the terrain.
This preserves grazing-angle sunlight and bounce lighting without changing
light energy, ray budgets, or occluder visibility.
A separate HDR lighting pass
stores diffuse irradiance and specular radiance on a bounded grid, normally
one sample location per 2x2 pixels. A 5x5 spatial filter rejects incompatible
materials, normals, surface positions and roughness before applying the
full-resolution authored albedo and primary emission. The display clamp runs
after filtering. Thin surfaces without a compatible grid sample retain their
own bounded trace. World and foreground retain separate histories; the sample
counts for indirect diffuse/reflection and direct diffuse/specular each cap at
16, with a 1/16 update weight thereafter. The 22-word history keeps all four
linear HDR channels separately. Current records and
histories are written through storage binding6. Before those camera phases
overwrite history, a GPU copy snapshots the previous world/foreground records
into a separate read-only buffer at `set=1, binding=9`. Its header retains the
previous view-projection matrices and actual Vulkan viewports, including
temporal-upscaler jitter. World positions reproject through that previous
matrix and viewport; the shader checks at most four previous grid candidates
and selects one compatible record. This adds no ray queries and prevents
current fragment execution order from changing the history being read.

Foreground positions and shading/geometric normals use camera-attached guides
instead of fixed WORLD positions. Reflected directions remain in WORLD space,
so a rigid camera turn rejects obsolete foreground highlights while retaining
valid camera-attached diffuse history. Foreground history
uses the current projection with the previous viewport, rejecting projection
scale changes. Material, roughness, consecutive-frame, shading/geometric-normal,
position-distance and surface-plane checks reject incompatible or disoccluded
history. Reflected ray directions must agree within the material's GGX lobe
width, so a subtle animated normal cannot smear a narrow highlight through the
old view. Spatial reflection filtering uses the same roughness-aware direction
guide. Rejected reflection history restarts its own sample count; valid diffuse
history remains accumulated. A stationary receiver also checks its reflected
lobe against the old/new bounds of changed secondary geometry. A single slab
test expands those bounds by the pixel footprint and GGX core width, rejecting
stale reflected objects without adding BVH queries or resetting unrelated
reflections and diffuse/direct histories. Consumed per-owner material state
(mask, colors, UVs, selected images and ambient) and stationary secondary normal
changes feed those same bounds. Atlas addresses and native ambient quantization
do not reset every history. A diffuse receiver rejects changed-material history
when the bounds intersect its outgoing hemisphere; bounds entirely behind it
retain convergence. This conservative union can restart extra receivers whose
actual sampled rays missed the changed owner, without adding BVH queries.
Opaque color changes leave direct-shadow history intact; either the previous or
current alpha policy admits local shadow invalidation. Unused shared roots do
not invalidate independently bound owner aliases. Source, resolution and ray-budget changes reset
all histories. Direct-light animation leaves unrelated indirect history intact.
An authored reflection-response fingerprint shares the diffuse-count word's
unused27 bits, so changed facing/glancing tints or strengths reject old specular
RGB without growing the record or discarding independent diffuse irradiance.
The deterministic single-light direct shadow remains current-frame lighting
outside the accumulated stochastic history. Native
emissive program animation updates every frame; its indirect contribution uses
the bounded temporal mean. Program identity, emitter membership and constant
fallback emission changes reset history. Authored texture and primary emission
are applied after temporal and spatial filtering.
This denoising runs with upscaling disabled as well. Temporal upscalers receive
the same exact posed WORLD geometry as the raster/RT handoff, retained source
object/vertex identities, rotational sky motion and the applied viewport jitter.
Unknown moving geometry rejects correspondence instead of inheriting another
object's motion merely because it shares a material and section.

The storage planner bounds both current and previous buffers against the actual
device `maxStorageBufferRange`, including the full-resolution depth prefix and
224-byte previous-camera/change-bounds header. It chooses the smallest integer
lighting scale that fits: normally 2x2, or 5x5 for 3840x2160 on a 128MiB-range
device. Odd scales and partial edge cells use the same representative and
four-candidate reprojection rules. Unsupported depth extents fail before allocation.

`color_guide.cpp` and `color_guide.glsl` retain authored lighting RGB
chromaticity from actual cluster/instance lightmap palettes and UVs, or proven
static per-vertex radiance. Primary static sky/sun diffuse events take that hue
at their traced brightness (`.299 R + .587 G + .114 B`), with current RT
visibility. When an authored distant-light bank is present, primary baked
magnitude does not replace traced sunlight or its shadows. Its qualified magnitude can guide sampling, whose PDF
compensates that probability. Zero or missing samples leave its color unchanged. An actual gray
sample deliberately gives a gray hue.

Interiors also need static lighting energy that exists in the original bake
and is absent from the retained dynamic light pool. When there is no authored
distant light, an authenticated primary diffuse field contributes its exact
`2 * lightmap RGB` baseline at full resolution, outside lighting history and
the traced reflection energy split. Its static diffuse continuation stops to
avoid adding the cached field again. Current local-light diffuse visibility,
highlights and GGX reflections remain traced. This deliberately preserves the
source interior bake; it does not claim fully dynamic reconstruction of baked
interior shadows. Unqualified or missing fields keep the traced route.

At the first compatible secondary diffuse
hit, the existing closest-hit segment reads that surface's original lightmap
RGB, decodes its source-authored `2 * RGB` diffuse transfer, and applies its
current albedo. Admission checks the exact static diffuse pass, vertex program
identity and complete combiner controls independently of animated constants;
an ordinary hue guide alone cannot enable this energy path. The proven
`lightmap_tex_detail` family does not use lightmap alpha as HDR diffuse energy.
This preserves the precomputed secondary field's magnitude and static lighting
distribution; it is not a fully dynamic recomputation of every baked bounce.
A current opaque object between an uncached receiver and that secondary
surface blocks the transport.

The authenticated lightmap field is independent of sky availability: indoor
maps with no sky or outdoor ambient retain their static secondary irradiance.
Missing/zero or unqualified guides retain physical sky/light transport.
Emission-only component views explicitly clear the secondary energy tag to
isolate their physical source contribution. The cached diffuse lobe terminates at its first compatible
hit at either bounce budget, avoiding another albedo absorption or repeated
static energy. Static distant/sky next-event estimates at that hit are
suppressed; current local lights, emissive sources and directly hit emission
remain active. A selected GGX lobe still continues through the normal bounce
budget, preserving reflections, including the visor. Material albedo, authored
reflection strengths and reflection tint are unchanged.

The existing source guide atlas is bounded to64MiB and exists only with the
active optional Scene/Render. Qualification occupies the previously reserved
material word; this policy adds no buffers, descriptors or ray queries and can
terminate diffuse paths earlier.
The selected source sky's D1A30 incident direction and radiosity supply its
directional emitter. The source query casts toward that incident vector;
Path Tracing uses the same orientation with BVH shadow rays and retains outdoor
ambient independently. An ambient-only sky supplies no directional light.
The sky's Original primary render layers remain authored draws. A private
64-texel-per-face cubemap replays those same native shader programs, textures,
animated constants, transforms and ordered blending from the current camera
position. Compact sky-only uniform aliases replace camera globals for its six
faces. Secondary misses sample this world-oriented radiance, including the
clouds, ring and mountains that are actual selected sky geometry. A missing sky
model supplies the authored outdoor ambient as a clear-only cube; no additional
ambient floor or gain is invented. This low-resolution directional environment
does not reproduce parallax from each secondary hit point or establish a
physical calibration of the legacy sky art's radiance.
BSP provenance (`lightmap` or `bsp_cluster`) distinguishes radiance from actual
object vertex tint. Transparent/effect/water passes keep their existing renderer
owners and do not become opaque BVH blockers. Authored alpha comparison and
texture alpha-kill are applied to shadow and reflection intersections.
Fully faded source primitives (stipple level0) leave the ray scene. Partial
levels1..15 retain the original primary screen pattern; secondary rays treat
their surviving triangle as solid. Fractional secondary-ray coverage is a
remaining transport approximation, rather than reconstructed screen-pattern
geometry.

Materials keep their authored texture/color data. A material without an authored
reflective response receives no new clear coat. Current Original facing/glancing
strengths and both reflection tints remain separate. Path Tracing does not use
pStencil's May2003 material-reference overrides. Qualified combined visor
programs retain their packed current reflection constants. Point and directional
lights use the GGX angular response with the selected light's shared shadow
query. This direct-only adapter caps lobe gain at one before applying authored
reflection strength/tint, source light color, incidence and destination-alpha
mask. Native light colors are artistic inputs, not calibrated delta-emitter
radiance; an area-normalized sharp highlight otherwise blows the opening ship's
hull white. This is a conservative legacy art policy, not a retail GGX formula.
Indirect reflection sampling, roughness and transport weights are unchanged.
For banks of at most64 lights, the receiver chooses a source using its unoccluded
power, influence/frustum, falloff and diffuse/GGX response. The inverse selection
probability preserves energy, and only the chosen source issues a shadow ray.
Larger banks retain bounded uniform selection.
Legacy source materials do not retain physical roughness: the
optional adapter maps reflection strength to a bounded roughness, making weak
reflections broader. This is explicit presentation policy, not an original
material parameter or a claim of original-pixel equivalence.

The exact `stencil_texture` vertex body and complete `default_texture`,
`default_texture_two_change_color` and `default_texture_four_change_color`
combiner controls retain their source `clamp(2 * T0 * T1)` detail expression,
native change-color masks/constants and `T0.a * T1.a` output alpha. Both ordinary
and `_zwrite` pass names are supported. The two/four-color families require their
source red/green mask selectors; other selectors remain unqualified. The
primary view reads completed per-draw constants at resident resolution, while
secondary hits retain the same fixed expressions in the bounded material atlas.
All four color constants participate in animated atlas invalidation. A qualified
texture pass paired with an independent `DST_ALPHA` environment pass also
retains its authored alpha reflection mask, applied after reconstruction at
full resolution so denoising does not fill its holes. Primary diffuse irradiance is demodulated before
filtering, then its `(1 - reflection * mask)` fraction is applied alongside the
masked specular term; a mask hole therefore restores diffuse energy too. This covers common armor
and character texture passes, not every character shader or bump response.

The opening capital ship's complete `lightmap_dynamic_tex_detail` vertex body
and dynamic `lightmap_tex_detail` combiner retain the same unlit two-texture
detail color and their exact `clamp(2 * C24.a * T0.a * T1.a)` destination alpha.
The independent environment pass consumes this mask in primary and secondary
shading. The current completed owner supplies C24 alpha to both paths; an initial
shared root's zero value must not replace the visible owner's animated mask in
a reflection. This admission changes neither authored reflection strengths nor
the legacy-to-roughness policy.

Qualified terrain programs retain their three-stage or four-stage texture blend.
Lockout's `main_cell_exterior` uses the exact three-stage
`detail_blend_texture_zwrite` graph: T0 alpha blends T1/T2, then the source
two-times modulation, display clamp, gain and bias run in their original order.
Its c18/c19/c20 vertex transforms, source alpha and each used texture's raw
alpha kill remain independent of the removed lighting overlay.
The exact opaque per-pixel `lightmap_tex_detail` family retains its authored
`clamp(2 * T0 * T1)` RGB, ordered c18/c19 UV transforms and samplers for primary
and secondary surfaces. Both complete alpha-control variants are qualified;
this includes the visible `frozen_panels` source program. The shared qualifier
also admits their proven secondary diffuse irradiance transfer. Its separate baked specular term is excluded from
albedo. Qualification requires no output alpha test or blend, retains the
existing T0 representative alpha and raw T0/T1 texture kills, and rejects
unsupported raw T2/T3 kills. This adds no ray queries or surface-record schema.
Foliage uses the native output-alpha expression before the original raster
comparison; shadow rays and the depth prepass use the same coverage. Bump
foliage is admitted only when its exact program, constants and LESS255 state
prove that normal-map alpha determines coverage independently of removed
lighting. Qualified model vertex radiance is excluded from albedo, while
procedural vertex tint and per-draw native color constants remain separate.

Qualified dynamic detail and `old_lightmap_tangentspace` model lighting also
retain their exact ambient term: twice current quantized source extern24 RGB,
multiplied by signed bump-map Z and current unlit albedo. The ordered native
c18/c20 bump UVs, source texture and sampler remain intact. Dedicated PT-only
descriptor copies carry each completed object's/view's ambient and bump
binding; native sky, environment, child and other pass descriptors are unchanged.
This baseline is evaluated at full resolution outside temporal history.
Native directional/specular terms are not added through this conduit.
Qualified secondary model hits read the same completed owner's quantized
ambient and ordered bump UVs from a compact shared-texture record. The native
baseline remains additive to the authored environment response on both paths.
Per-view material aliases also retain the current owner's source color
constants, reflection mask, texture transforms and selected animated images;
their stable identities are shared by primary draws and secondary geometry.
Only the qualified unlit surface families use this alias path.

Source primary textures stay at resident resolution. Secondary-ray albedo uses
an existing authored mip at most 128 texels per side where available. The atlas
is bounded and source frames refresh on image/animation changes, reusing storage
for equal-size frames. Current reflection/emission constants refresh separately
from image decoding. Scene geometry, images and GPU buffers are released when
the mode is disabled.

Authored self-illumination is independent of base albedo and vertex tint. The
admitted default, multichannel and detail source families retain their exact
vertex/control identities and packed animated constants. Their separate mask
images, c18..22 transforms, address modes and point/linear sampling travel in
bounded scene records; native GLSL evaluates fixed expressions rather than an
instruction interpreter. Existing child self-illumination draws remain in the
primary view, while those records supply reflected emission and indirect light.
Pure emissive roots use the native expression directly. Unsupported source
emissive programs retain their original primary draw and do not acquire an
invented emissive response.

The source material bank also contains UI textures and font atlases. Only root
slots referenced by the complete ray geometry receive decoded image storage;
unused slots share one white placeholder. Admission includes offscreen static
triangles and activates later dynamic material slots before BVH refresh. A
referenced malformed image remains an error rather than using the placeholder.

Primary W depth reads the completed source camera's selected c3 row from the
existing material uniform at its fixed ABI offsets. World, reflected and
first-person cameras retain their separate rows and actual distance scales.
The fragment shader distinguishes raw 24-bit source W from normalized fallback
depth, keeping the existing fog and effect passes in the same depth space.

Before traced world/foreground draws, a cheap private D32 prepass writes that
same source-W depth with matching camera, jitter, alpha comparison and stipple.
Its depth values are copied into a storage buffer so the main fragment shader
can reject occluded fragments before tracing. The main draw still writes the
source depth used by fog and effects. This avoids comparing source W against
unrelated projected hardware Z, and each camera phase has its own visibility
preparation. The private image and buffer belong to the optional renderer.

Scene buffers prefer compatible host-visible, coherent device-local memory so
BVH reads stay on the GPU when the device exposes that combination. Host-cached
allocations can skip identical uploads using a comparison; uncached mapped
device memory is written directly without reading it back through the CPU.
Other compatible memory remains a fallback for devices without that memory
type.

## Inactive resource lifecycle

Original and pStencil retain no RT pipelines, descriptors, GPU scene buffers,
private visibility image/framebuffer/pass, visibility storage buffer, CPU BVHs,
immutable previous-history buffer,
decoded RT texture atlas, RT light banks, environment cubemap/depth target, or
sky alias uniform/descriptor banks. There is no RT worker,
scene construction, image refresh or ray dispatch in those modes. The renderer
borrows the existing scene's Mesh through `Renderer::path_tracing_source`; this
does not copy or own assets. Scene owners rebind after renderer replacement.

Leaving Path Tracing waits for the last frame that uses its resources, then
destroys every RT resource and releases vector storage. Loading submissions and
swapchain replacement also retire it; disappearing split-screen views release
their own allocations. Selecting it again constructs fresh RT state on demand.
`Renderer::path_tracing_resources()` reports owned view count and CPU/GPU bytes
for the real selector's lifecycle regression checks, including environment
images and compact sky alias allocations.

## Evidence and remaining boundaries

`path_tracing_multiple_light_shadow_disocclusion` follows a moved caster through
its actual interpolated half-frame and endpoint with a two-light bank.
`path_tracing_animated_light_denoise` verifies convergence while an unrelated
offscreen object and point light animate; `path_tracing_reflection_history`
checks a four-degree smooth-normal change against independent sharp cubemap
highlight endpoints. `path_tracing_foreground_reflection_turn` turns a reflective
camera-attached surface across distinct WORLD sky endpoints and checks its
first changed frame without discarding diffuse history.
`path_tracing_reflected_object_history` moves a reflected
object away from a stationary mirror and checks the new radiance immediately;
`path_tracing_unrelated_reflection_motion` preserves stochastic reflection
convergence while geometry outside that lobe animates.
`path_tracing_denoiser_negative_controls` compiles private
shader mutations restoring each former failure and requires the corresponding
GPU regression to reject it. Compiler modules and mutated copies live only in
the enclosing test scratch. `path_tracing_lighting_storage` covers 4K/8K device
range limits, odd lighting scales, and clamped edge representatives.

Focused CPU checks exercise BVH partitioning, finite ray lengths, all source
alpha comparisons, current poses, animated material refresh, removal of primary
baked radiance, and distinct matte/visor responses. Offscreen Vulkan checks render
actual shadows, emissive lighting and visor reflections, and compare BSP
radiance variants. These component results do not establish launcher delivery,
all-map visual acceptance, original Xbox framebuffer equivalence, or a frame-rate
budget.

The GPU depth regression overlaps red and blue emissive triangles whose supplied
source c3 row reverses their geometric depth order. It checks world, reflection
and first-person row selection and rejects the former fixed-1024 depth path.
Guard-enabled and guard-disabled renders must match exactly for those rows,
material overrides, alpha holes, partial stipple and a ray-lit receiver. A
low-energy mixed point/emissive-light check also compares their combined mean
against the separate contributions. These checks establish component behavior;
native gameplay timing and launcher-selected runtime qualification remain
separate checks.

`path_tracing_grazing_normal` and `path_tracing_grazing_secondary` isolate that
normal-orientation boundary for a primary receiver and a deterministic mirror
continuation. Both compare the authored sun cosine with a geometric-normal
control and retain a real opaque blocker.

`path_tracing_source_primary` checks the primary-black regression, exact
interior baseline, outdoor distant-light exclusion, current local shadows,
reflection energy independence, no repeated cached diffuse field, and two
same-material owners with different/changing ambient uniforms in one frame.
`path_tracing_source_ambient` compares the complete prepared native pixel and
vertex programs, including source brightness quantization and bump-map Z/UVs.
`path_tracing_material_alias` checks actual cruiser and armor source constants,
stable owner identities and shared image storage. Its GPU companion checks two
same-material owners in primary draws and actual mirror intersections, with
independent current colors and ambient terms.

`path_tracing_source_indirect` isolates the secondary static boundary with an
enabled black native sky bank. It checks the authored RGB decode, primary guide
brightness independence, actual secondary intersections and opaque blockers,
current hit albedo, equal energy at one/two bounces, equal irradiance without a
sky, explicitly cache-disabled physical emission,
current local lights, distant-source double counting and GGX continuation.
An occluded weak emitter behind a constant cached wall also checks that the
proposal hint reduces background noise while preserving its mean. Clearing
only the primary hint tag leaves the physical secondary field unchanged.
The CPU color-guide test separately qualifies actual prepared Lockout source
programs and rejects modified or unsupported programs. These are component
controls; native Original/PT interior captures remain a separate acceptance
gate.

`path_tracing_source_opaque_surface` compares actual original/prepared Lockout
source programs against the retained unlit RGB; it does not establish complete
baked output-alpha or native-frame parity. `path_tracing_opaque_detail` checks
the ordered primary detail transform, source samplers/raw texture kills,
prepass/filtered agreement and secondary albedo on the GPU.
`path_tracing_source_panel_surface` additionally isolates the exact visible
Lockout `frozen_panels` and `main_cell_exterior` programs against their original
and prepared images. `path_tracing_panel_detail` uploads their actual decoded
authored mips and source samplers, including the three-stage blend, and compares
raw, source-depth and 16-frame filtered pixels against the scalar-qualified
surface atlas. It requires measurable detail and verifies that reconstruction
preserves it at full screen resolution. These albedo controls do not reconstruct
the source bump or separate specular response.

`path_tracing_vanilla_surface` compares 12,288 varied texture/detail/change-color
cases against literal complete native programs. `path_tracing_vanilla_opening_materials`
checks the admitted families against the current opening map's prepared character
materials. `path_tracing_vanilla_surface_gpu` covers their primary/depth/filtered
and secondary atlas colors, animated color residency, colored point-light
highlights, their actual opaque shadow blocker and the full-resolution reflection
mask. These are bounded material and lighting checks, independent of native
cutscene image acceptance.

`path_tracing_capital_ship` loads both actual introduction hull materials and
checks raw, source-depth and filtered detail color, zero/half/full animated
reflection masks, diffuse energy restoration and an actual reflected secondary
ship hit with a positive unmasked-highlight control. Unit white/colored lights
must retain a nonzero direct highlight within the authored envelope in all three
roles, and an actual blocker must shadow it. A separate no-light sky reflection
control retains the unchanged indirect GGX response.

For native-resolution denoising checks, configure
`HALO2_PATH_TRACING_NATIVE_DENOISE_TESTS=ON` and run
`path_tracing_native_denoise` and `path_tracing_native_fence_denoise`.
These checks use 2560x1388 output with upscaling off and the default ray budget.
The WORLD-scale fence case runs sixteen stationary frames without intermediate
color readbacks or history diagnostics, then checks noise, lighting energy,
history accumulation and Vulkan synchronization validation. The lighting
render pass synchronizes its depth attachment store even though its pipeline
does not write depth during fragment shading.

The source shader's complete detail/bump, gel/gobo and multi-pass combiner
responses are not reconstructed as a physically based BSDF by this adapter.
Per-owner recoloring outside the qualified texture families still needs an
explicit unlit material mapping before complete character-material parity.
Projected-light frustum clipping is exact geometry; its smooth radial radiometric
falloff is optional presentation policy. A source environment color must be
supplied explicitly; the default is black. Reflections cannot include dynamic
objects that the existing camera/admission owner did not submit. Water and
transparent reflections retain their original passes. These boundaries must be
validated and extended before claiming full material or all-scene parity.
