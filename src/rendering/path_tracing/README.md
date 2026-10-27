# Path Tracing renderer

The optional **Path Tracing** choice is in the ordinary and pause Video Settings
Graphics Mode row. Original remains the default. This is a native presentation
extension over current source-admitted geometry and assets; Xbox TU1.5 remains
the reference for Original rendering.

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
- `include/halo2/path_tracing/surface.hpp`, `material_surface.hpp`,
  `unlit_uv.hpp` and `shaders/path_tracing/surface.glsl`: source-qualified
  terrain texture blends, foliage coverage, native chained UV transforms and
  shared primary/secondary material evaluation.
- `geometry.cpp`: exact existing model skin arithmetic for current WORLD poses.
- `source_lights.cpp`: actual native light-pool/effect inputs and projected-light
  frustum planes; no camera-position light or invented directional sun.
- `include/halo2/path_tracing/sky_lighting.hpp`: selected authored sky metadata,
  loaded independently of the retail object-lighting availability bit and
  released when Path Tracing is disabled.
- `render.cpp`: Vulkan resource ownership, descriptors, uploads, quality bounds,
  graphics pipelines, the private visibility depth image/buffer and the native
  sky environment cubemap. Resources live only while the optional mode is active.
- `shaders/path_tracing/world.vert`, `world.frag`, `scene.glsl`: primary surface
  rasterization, BVH traversal, shadow any-hit, diffuse transport and reflection.
- `shaders/path_tracing/depth.frag`: source-W visibility prepass with the same
  primary alpha/stipple rejection, without ray traversal.
- `shaders/path_tracing/lighting_filter.glsl`: HDR lighting records and guided
  resolve. `world.frag` also compiles as the colorless `lighting.frag.spv` write
  variant; both roles are included in production shader qualification.
- `cmake/PathTracing.cmake` and `tests/path_tracing/`: build and CPU/GPU checks.
- `src/rendering/renderer/renderer.cpp`: the existing renderer's narrow handoff
  for resident BSP triangles, current posed objects, source lights and camera;
  compact sky aliases replay the ordered native sky draws into the RT cubemap.

## Lighting and material policy

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
terminate earlier. Source lights, emissive triangles and the native sky share
one next-event shadow query per surface. Cosine-sampled sky illumination at the
last hit lets a canyon wall reflect its authored color at one bounce. Diffuse
continuation misses omit that already-sampled environment contribution;
specular misses retain it. World and foreground phases, or equal-depth draws, can shade
the same framebuffer coordinate more than once. This does not require Vulkan
hardware ray tracing extensions.
Frame-varying ray seeds converge through bounded temporal lighting histories.
A separate HDR lighting pass
stores diffuse irradiance and specular radiance on a bounded grid, normally
one sample location per 2x2 pixels. A 5x5 spatial filter rejects incompatible
materials, normals, surface positions and roughness before applying the
full-resolution authored albedo and primary emission. The display clamp runs
after filtering. Thin surfaces without a compatible grid sample retain their
own bounded trace. World and foreground retain separate histories; the sample
count caps at 16 and later frames use a 1/16 update weight. Material, normal, world position, roughness,
view direction and consecutive-frame checks reject stale surface history;
source, resolution, ray-budget and direct-light changes reset it. Native
emissive program animation updates every frame; its indirect contribution uses
the bounded temporal mean. Program identity, emitter membership and constant
fallback emission changes reset history. Authored texture and primary emission
are applied after temporal and spatial filtering.

Lightmap images and baked BSP vertex radiance are excluded from traced shading.
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
reflective response receives no new clear coat. Reference-backed facing/glancing
strengths and reflection tint remain separate, including the Chief visor's
TU1.5 values. Legacy source materials do not retain physical roughness: the
optional adapter maps reflection strength to a bounded roughness, making weak
reflections broader. This is explicit presentation policy, not an original
material parameter or a claim of original-pixel equivalence.

Qualified terrain programs retain their four-stage grass/dirt/detail blend.
Foliage uses the native output-alpha expression before the original raster
comparison; shadow rays and the depth prepass use the same coverage. Bump
foliage is admitted only when its exact program, constants and LESS255 state
prove that normal-map alpha determines coverage independently of removed
lighting. Qualified model vertex radiance is excluded from albedo, while
procedural vertex tint and per-draw native color constants remain separate.

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

Focused CPU checks exercise BVH partitioning, finite ray lengths, all source
alpha comparisons, current poses, animated material refresh, removal of baked
radiance, and distinct matte/visor responses. Offscreen Vulkan checks render
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

The source shader's complete detail/bump, gel/gobo and multi-pass combiner
responses are not reconstructed as a physically based BSDF by this adapter.
Per-owner recoloring that exists only in authored combiner constants also needs
an explicit unlit material mapping before complete character-material parity.
Projected-light frustum clipping is exact geometry; its smooth radial radiometric
falloff is optional presentation policy. A source environment color must be
supplied explicitly; the default is black. Reflections cannot include dynamic
objects that the existing camera/admission owner did not submit. Water and
transparent reflections retain their original passes. These boundaries must be
validated and extended before claiming full material or all-scene parity.
