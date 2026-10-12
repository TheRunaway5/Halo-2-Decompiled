# Title atmosphere evidence

The user supplied three Xemu reference screenshots and confirmed the Vulkan
backend. Originals are retained in `analysis/frontend/xemu-reference/`; they
have not been recolored. Internal resolution and aspect settings were not
provided. Native flythrough captures are retained in
`analysis/frontend/xemu-pose-search/`. Their camera phases have not yet been
matched precisely to the reference screenshots, so no pixel-parity verdict is
claimed.

## Per-camera sky aggregation

The live title now samples its authored collision BSP and applies original
`12E5E0` cluster/portal aggregation on every rendered camera position. It updates
primary fog and patchy RGB/density/distance before the fog constants, composite,
planar externs and direct haze are produced. Noise textures, motion state and
the independently verified camera classification remain intact.

Mainmenu has six `NONE` sky references before its actual sky at ordinal6. The
shared loader now preserves those slots and distinguishes them from valid
empty skies, following `130F60`/`12EFB0`. The source division, multiplication
and subtraction order for eye offset also preserves positive zero correctly.

Eleven retained original camera/submersion cases match all363 fog-bank words
exactly. A raw-loader diagnostic retains the four one-bit input differences at
tick2100 that this change fixes; these small differences are not claimed as the
cause of the broader visible brightness discrepancy. The shared21-map oracle
passes2,163 cases after these changes. A finite full-flyby admission check found
valid clusters and haze at191,488 startup/loop/subframe samples. See
`analysis/frontend/title-fog-state-audit/INTEGRATION_REVIEW.md`,
`integration-tests.log` and `multiplayer-differential.json`.

The captured-original haze passes and complete bloom/filter shader chain still
produce zero differing RGBA channels for their retained inputs. These isolated
checks do not establish synchronized native world pixels, haze random history,
complete scene effects or whole-frontend layout/flow parity.

## Missing vertex inputs restored

The retail executable SHA-256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The original routines are retained in `analysis/retail/functions/`:

- `001301c0.c` normalizes fog densities and ranges.
- `001305d0.c` classifies the camera against the visible plane. Its call to
  `0016e210` uses the actual BSP cluster and scenario fog palette in the oracle.
- `00130de0.c` moves fully submerged planar fog into the secondary atmospheric
  layer. It does not leave the original planar density active as well.
- `00025a10.c` constructs the six c12..17 vectors and copies them to `0x4077F8`.

`mainmenu.map` has one BSP, 70 clusters selecting visible sky 6 and fog plane 0,
and one planar-fog palette entry. That entry references
`scenarios\skies\solo\earthcity\fog\africa`.

| Input | Retail value |
| --- | --- |
| Primary fog RGB | 0.360784, 0.317647, 0.262745 |
| Primary density / start / opaque distance | 0.5 / 0 / 150 |
| Plane normal / distance | (0, 0, 1) / 221.8275604248047 |
| Planar fog RGB | 0.494118, 0.415686, 0.345098 |
| Planar density / opaque distance / depth | 0.8 / 6 / 6 |
| Planar eye offset | 0 |
| Default vertical transition | 1023 to 1024 |

The loader validates this bounded title case. It rejects unsupported regional
selection, secondary/directional sky aggregation and custom planar transitions
instead of substituting invented values. Patchy haze now has its own source
port and comparison below; it is outside the six vertex-vector comparison.

The native renderer uploads these camera-dependent constants after its previous
frame fence. `vertex_program.glsl` now resolves c12..17 from the uploaded array
when title atmosphere is enabled. Its original material instructions determine
their effect on the beam's varyings. No replacement beam color was introduced.

## Checks and reproduction

```bash
cmake --build build-frontend
.tools/python/bin/python tools/differential_menu_fog.py
ctest --test-dir build-frontend -R 'retail_menu_fog_gpu|retail_frontend' --output-on-failure
```

The differential passed 3,044/3,044 cases, with zero differing float words. It
covers the plane and submersion boundaries, the default transition boundaries,
and 3,000 seeded random camera positions/directions. The original executable
runs only as an independent test oracle, not in the application.

The GPU check runs the original BSP light-volume vertex program and both carrier
beam layers through the same GLSL interpreter used by `world.vert`. Its full
material ABI places the fog constants after the texture-effects fields. It
compares oD0, oD1 and oT0..3 against the CPU arithmetic reference with a bounded
tolerance for GPU reciprocals. This is GPU wiring/arithmetic evidence, not
original NV2A reciprocal-precision or framebuffer evidence.

The final title check passed five focused CTest groups and three ASan/UBSan
groups (GPU driver leak detection disabled), including all 840 GPU fixtures.
`fog-focused-final-tests.log`, `fog-asan-final-tests.log` and
`fog-differential-final.log` retain the results. The refreshed title launcher
also produced a fresh frame with Vulkan validation and no validation messages
(`fog-installed.log`).

At that earlier fog checkpoint, `native_systems` and
`retail_system_runtime` fail the scene automatic/script/manual BSP-placement
assertion. An initial duplicate-BSP-material selection error in the new GPU
fixture was fixed before the final focused and sanitizer runs. The unrelated
scene placement logic was not changed for this atmosphere work. Both tests
pass at the later planar-fog checkpoint (`planar-fog-systems-recheck.log`).

## Separate depth composite implemented

`0002bcd0.c` invokes `00025d50.c` after the opaque render layers. The original
instructions at `0x2C173` select material layer 15 later. The carrier's authored
templates place its self-illumination in layer 7 and both beam passes in layer
15; blend mode is insufficient to determine their position around fog.

`src/fog_composite.cpp` ports `00025e50.c` and the bounded title mode-1 path of
`0003a5c0.c`. It preserves endpoint quantization, reciprocal limits, x87 color
packing, all 60 pixel-program words and the general-path lookup coordinates
from `00026710.c`. No fog or beam color was selected from a screenshot.

The title stores 24-bit integer W depth and uses the executable's near/far
distances of 0.015625 and 1024. The renderer samples that depth with nearest
filtering, evaluates the original register-combiner definition, and uses the
actual 16×16 `rasterizer\\gradients` bitmap for the general path. A separate
color pass blends its result before layer 15, then resumes geometry with color
and depth retained. Per-frame uniforms wait for the prior frame fence.

Reproduce the new checks with:

```bash
cmake --build build-frontend
.tools/python/bin/python tools/differential_fog_composite.py
ctest --test-dir build-frontend -R 'retail_fog_composite_gpu' --output-on-failure
```

The executable differential passed 3,598/3,598 cases, including transitions
around the plane and submersion boundary and 2,048 seeded camera heights.
Every composite word and applicable lookup-coordinate word matched exactly.
Hardware submission is suppressed in the oracle; the original calculation
instructions execute. The GPU check passed 2,565 fixtures through the actual
shared fog shader against independent scalar arithmetic (tolerance 2e-5).
These do not certify original framebuffer or NV2A arithmetic precision.

An actual tick-800 frame probe with the composite enabled and disabled produced
different rendered pixels (maximum channel differences 10, 11 and 12 after UI
composition). `fog-frame-probe.json` and the `fog-probe-*` captures retain this
evidence. The temporary pass-disable hook was removed from the application.
The probe establishes that the pass affects a real title frame; it cannot
establish Xemu parity because those two captures are both native.

The final composite run passed six focused CTest groups and three ASan/UBSan
groups, and a two-frame sanitizer title capture at tick 2100 completed without
diagnostics. Both root launchers were refreshed from `build-frontend` and their
SHA-256 hashes match the built launchers. The installed title launcher produced
a fresh tick-800 capture with no Vulkan validation messages. Results are in
`fog-composite-final-tests.log`, `fog-composite-asan-final-tests.log`,
`fog-composite-asan-runtime.log`, `fog-composite-installed.log` and
`fog-composite-final-report.json`.

## Patchy haze implemented

The visible sky's `sky+0x60` block supplies RGB
(0.521569, 0.494118, 0.419608), density 0.1–0.7 and distance 0.1–50.
It references `cinematic_newmombasa\\africa` (`fpch` datum `0xE193001E`),
which enables separate layer depths, disables sorting behind transparents,
and binds the original 256×256 DXT1 `bridge_fog` bitmap. The title's animated
wind maximum and bitmap animation period are zero; constant wind is (0, 0.05, 0).
The field layouts were cross-checked against the pinned Assembly
[sky](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/sky.xml)
and [patchy fog](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/fpch.xml)
schemas, then read from the actual cache.

`src/patchy_fog.cpp` ports `27AA0`, the bounded zero-animated-wind `28580`
path, and the noise/cached/direct pixel programs from `26880` and `26E50`.
It retains previous-camera motion, slice wrapping, the original LCG and
reseeding behavior, vertical weighting and packed constants. The original
advances the random state twice and uses the same final high word for both
coordinates of each offset pair; it does not consume two independent samples.

The renderer builds the four depth-separated slices in RGBA8 target 19, then
uses the original gradient lookup and aliased 24-bit W-depth components to
compose haze after layer 15. The actual original `1D770` allocator confirms
this target is 320×240. Scene-relative texture coordinates preserve the 4:3
projection when the native window is wider; haze does not tint its borders.

```bash
.tools/python/bin/python tools/differential_patchy_fog.py
ctest --test-dir build-frontend -R retail_patchy_fog_gpu --output-on-failure
```

The executable differential passes 3,072 persistent camera/RNG frames, including
roll, pitch, rotation, movement, teleports and slice reseeding. All 180 program
words and random states match exactly; maximum float error is
5.960464477539063e-8. The oracle supplies the original double render-clock ABI.
The shared GPU shader passes 3,084 coordinate/noise/depth/combiner fixtures
against independent scalar calculations (maximum error 1.19209e-7).

### Corrected viewport orientation and checked original haze geometry

The original `27AA0` at `27E1A..27E60` computes `(bottom-top)/(right-left)`
from `485648`'s **top, left, bottom, right** rectangle. This is height divided
by width. The previous differential incorrectly supplied `(0,0,640,480)`;
that swapped bottom/right and masked the renderer's inverse aspect input.
The native title and shared BSP callers now provide height/width, and both
original-instruction differentials supply `(0,0,480,640)` for a 640×480 view.
The raw lens angle is unchanged.

`34420` passes the four unit-square corners to `26CA0`. The differential now
executes that unchanged callback for all four haze layers at all corners over
the 3,072 persistent frames. It includes six viewport sizes and changes of
viewport size during a sequence. All **49,152 coordinate comparisons are
exact**; program words and random states still match, with maximum row error
5.960464477539063e-8. These are executable calculations, not claims that the
original title rendered at every tested host resolution.

The actual original event-8005 geometry has vertical/horizontal span ratios
1.1228240..1.1228245. The native capture before correction had ratio 1.996132;
after correction it has 1.1228243..1.1228244, with maximum absolute difference
3.3185970371718554e-7. `tools/audit_title_haze_geometry.py` checks these ratios
from actual runtime uniforms and original captured corners. This measurement
does not compare slice phase, rotation, offsets, or random/camera history.

The original fpch's noise scale at `+0x10` is **3.0**; `+0x1c` is the separate
vertical-forward factor **1.5**. A transient comparison used 1.5 as the noise
scale and reported a spurious factor-of-two discrepancy. No scale adjustment
was made: the native asset loader already reads both fields correctly.

Evidence: `analysis/frontend/xemu-capture/title-haze-geometry-differential.log`,
`title-haze-aspect-geometry.json`, `native-haze-aspect_capture.rdc`, and
`analysis/frontend/haze-aspect-corrected/validation.json`. The latter verifies
centered 4:3 presentation and borders at four host aspects plus resizing;
full-width scenery and all frontend screen layouts/flows remain unverified.

The pre-haze world comparison is recorded separately in
`title-world-before-haze-comparison.json`. At pixel (65,115), original draw
1880 and native draw 2552 both leave carrier-fog RGB (62,53,45). Original quad
draws 6242..6298 subsequently raise that pixel to (255,255,255); the native
capture has no corresponding contribution at that pixel. The original quads
use fragment shader 13437 and a 64×64 R8G8 texture, resource 15451. Other sampled
blue-effect pixels also receive additional original quad contributions.
`title-world-pixel-history.json` preserves the draw/fragment history. These
measurements identify where to trace the next effect. The original object
attachment and generation path below has now been established; a matched
marker-to-captured-quad comparison remains outstanding. Do not add a glow
substitute or alter carrier lighting to compensate for these draws.

### Carrier engine-volume source trace

The original carrier `scen` attachment block at `+0x94` contains an `MGS2`
reference to datum `E1F4007F`,
`effects\\objects\\cinematics\\capitol_ship\\capitol_ship_engine`, with marker
StringID `070008E3`. Its render model has three markers with that name. The
original instructions in `41980` retrieve those markers through `B8D30`, then
queue each light volume through `42850` for callback `4F010`. The attachment has
zero primary-scale input and change-color indices; `BE6D0` supplies one unless
the object's applicable fade flag changes it. The volume's marker-scaled flag
uses the composed marker **matrix scale**, not the authored marker's separate
scale field. The latter is zero in this asset and is not a reason to hide it.

The tag has one non-fuzzy volume, 32 sprites, radius/offset/brightness/color/
facing curves, one aspect block, and 32 cached phase pairs. `4F010` generates
the sprite centers, two radii, packed ARGB, and projected-axis orientation;
`4FF50` applies the camera-facing aspect curves. `4F7E0` binds the default
rasterizer glow bitmap from `matg` rasterizer globals `+0x34` when the volume's
bitmap is absent. In this map that reference is `E45B023B`, `rasterizer\\glow`.
Its non-fuzzy path uses `4FE20` and `480A0`. The submitted quads depth-test
without depth writes and use source `ONE`, destination `INV_SRC_ALPHA` blending.
These facts come from the unchanged original instructions, not the schema's
descriptive text alone.

The tag's runtime flags at `+0x5C` are **2**, so this carrier uses forced
linear offset phases `i / (count - 1)`. Its cached phase pairs are present but
are not used by this branch. The native loader retains both inputs and selects
the original runtime branch.

`tools/export_title_light_volume_oracle.py` now executes original `4F010` and
`4FF50`, preserving their curve/color/math callees. It suppresses only viewport
hardware setup and hardware submission and exports **128 sprite records** from
four controlled position/orientation/scale/opacity/color fixtures. The report
`analysis/frontend/title-light-volume-original.json` records the complete
inputs and packed outputs. These are original-only fixtures: no native
implementation or matched-frame result is claimed.

The subsequent native differential report,
`analysis/frontend/title-light-volume-differential.json`, compares **32,768
sprite records** across 1,024 controlled fixtures, including translated and
rotated camera transforms and changing projection axes. All packed ARGB values
match exactly; maximum float error is `7.62939453125e-6`. It also verifies all
60 non-fuzzy pixel-program words, the three named marker transforms against
original `1D8F00`/`1D8F50` with identical posed-node inputs, and all 4,096
base-level glow texels against the captured RG8 texture's R,R,R,G swizzle.
Marker composition is exact for those inputs; this does not certify the node
animation that produced them. The native x87 color-pack equivalent preserves
the input-to-byte product until nearest-even integer conversion.

The native renderer now submits all three carrier engine volumes through a
dedicated pipeline before direct haze, bloom, and the title color filter.
Original `2FD90`/`2FB70`/`141590` camera construction and `48B00`/`480A0` quad
projection are retained. The calculation comparison covers 512 camera inputs
and 512 sprite-projection inputs with zero numeric error. Actual rendering
uses the original pixel projection scales, rather than bare lens coefficients.
The GPU pipeline retains RGB-only writes, `LEQUAL`, no depth writes, and
`ONE`/`INV_SRC_ALPHA` blending. The original program's final alpha is zero.

`analysis/frontend/xemu-capture/title-light-volume-capture.json` records actual
quad inputs, post-vertex geometry, uniform buffers, depth/blend state, and texture
bindings for sampled original events `5697`, `5788`, `6242`, `6246`, `6250`,
`6298`, and `6302`. The two observed 64×64 R8G8 texture resources (`13896` and
`15451`) have identical base-level bytes. Their exact association with each
carrier's posed marker and generated sprite index is now checked for the
entire 96-quad group in the fixed reference frame.

`analysis/frontend/title-light-volume-runtime-comparison.json` compares the
fresh native Vulkan capture with all original events `5655` through `6362`.
The three groups are marker 2, marker 0, then marker 1, each with 32 sprites.
All **384 uploaded screen XYZW values, vertex colors, UVs, and post-VS clip
vertices match byte for byte**, as do all 60 submitted pixel-program words.
`B8D30` supplies world-space node matrices to `1D8F50`: world placement must
precede marker composition. Composing a local marker first introduced up to
`9.91821289e-5` pixels and one integer depth unit of error in marker 0.
The corrected composition matches all 96 source quads. The original captured
SPIR-V has no `NoContraction` decoration on the passthrough arithmetic;
retaining that contract also removes the native GPU's remaining one-ULP clip
coordinate differences. This verifies the fixed captured pose and camera,
not every animation frame or transparent scheduling branch.

`analysis/frontend/xemu-capture/glow-mip-comparison.json` verifies all **5,461
texels across seven mip levels** in the captured original and uploaded native
glow images. Every expanded R,R,R,G texel matches exactly. The default glow
sampler now explicitly caps LOD to the image's final level, matching the
original value 6. The actual captured filter, address, LOD, depth, and active
blend states also agree with the original; isolated framebuffer equivalence
remains unverified.

Both original glow resources (`13896` and `15451`) also have identical full
mip chains. The first 32-quad group can be compared without any intervening
billboard draws: original event `5872` versus native `2797` changes the same
162 pixels. All 76 effect pixels with identical input RGB match exactly, as
do all 289 changed RGB channels where neither output saturates. The complete
group delta still differs by up to six bytes in pixels with different starting
scene colors and saturation. These measurements are recorded in
`analysis/frontend/xemu-capture/glow-first-group-rendered-delta.json`; they do
not establish full scene-color/depth or three-group framebuffer parity.

The rendered engine-center pixel at `(65,115)` now reaches the original
pre-haze RGB `(255,255,255)`; the missing-effect result was `(62,53,45)`.
This is a rendered improvement, not a whole-frame match. Original events
between the glow groups include **54 additional billboard draws**, using
fragment shaders `14007` and `14018` and textures `14162`, `14166`, and `14170`.
`analysis/frontend/xemu-capture/original-glow-interleaving.json` records their
bindings and states. The three texture identities and their original asset
dependency chain are now established below. Live emitter identity and
simulation/scheduling still need to be established, so a before/after delta spanning all glow groups is
not an isolated glow comparison. Remaining material, particle, depth/filtering,
and haze-history differences keep full framebuffer parity unresolved.

Fresh captures at 640×480, 1280×720, 1280×800, 1680×720, and a resize to
1000×600 pass Vulkan synchronization validation and border checks with the
restored glow. They are under `analysis/frontend/title-glow-sampler-aspects/`.
They preserve the centered reference 4:3 viewport; full-width scenery and
all-menu layout/flow parity remain open. The installed title-first launcher
now resolves the verified `build-frontend/halo2_native` runtime.

The remaining billboard textures have now been identified by decoding the
original disc BC1 blocks with the unchanged pinned Xemu S3TC decoder. All three
captured mip levels match byte for byte, for each texture:

| Captured resource | Original bitmap | Original particle |
| --- | --- | --- |
| `14162` | `E22200AD`, `effects\bitmaps\weapons_fire\shrapnel_sparks` | `E22100AC`, `effects\generic\sparks\shrapnel_sparks` |
| `14166` | `E22000AB`, `effects\bitmaps\energy\c_electric_discharge` | `E21F00AA`, `effects\generic\energy\electric_discharge` |
| `14170` | `E21B00A6`, `effects\bitmaps\energy\smoke_white_c_generic` | `E21A00A5`, `effects\generic\energy\smoke_white_c_generic` |

These particles belong to original `E211009C` effect event 0, systems 5, 4,
and 2 respectively. The original `E20A0095` flak particle references that
effect as its death effect. `E2090094` contains that primary flak particle
system, and the original `E204008F` scenery attaches that effect. Five main-menu
scenery placements reference this object (indices 4, 6, 7, 8, and 9). This is
a verified structural chain, rather than proof of a particular live emitter
or random state in the captured frame. Several other particles share the
captured smoke and discharge bitmaps; bitmap equality alone does not select
a live particle tag. The exact references, emitter property
bytes, and placement data are recorded in
`analysis/frontend/title-particle-source-chain.json`; the texture hashes are in
`analysis/frontend/title-billboard-asset-matches.json`.

`src/particle_sprite.cpp` now loads these actual particle definitions and
their bitmap sequences, and implements the original `224550` sprite
calculation up to its `23ED30` submission. In 3,072 controlled fixtures across
the three tags, every submission byte matches execution of the unchanged
original XBE: all 21 float components and packed ARGB for every sprite. This
covers tint/fade, size, rotation, pivot/aspect geometry, mirroring, atlas
coordinates, and frame interpolation. The test provides an already evaluated
17-value cache and a caller-initialized sequence context to both versions;
it does not invent ages, seeds, or emission state for the game. The independent
retail fixtures also provide a 192-case regression check. See
`analysis/frontend/title-particle-sprite-differential.json` and
`tools/export_title_particle_sprite_oracle.py`.

The value producer feeding that sprite calculation is now implemented in
`src/particle_values.cpp`. Its 17 source slots follow unchanged `248DF0`, with
`173BA0`'s missing-bit scan and `4EFB0`'s exact-zero normalization. The x87
reciprocal remains double through each component product, as in the executable.
All 73,728 output bytes match 1,024 unchanged-XBE owner-state cases, including
already-completed cache bits, null owners, clock conversions and fractions.

A fresh original Xemu Vulkan trace observes entry to `224550` and submission
to `23ED30` without replacing instructions. It records actual definition/tag
identity, initial/completed cache bytes, system/location/particle/effect owner
state, camera and clock, and seed/context/call inputs. The latest 512 records
cover nine definitions, including primary `E20A0095` flak and several smoke
and flare definitions beyond the earlier three texture matches. The complete
native cache-to-sprite path matches all **81,920 output bytes**. The actual
live bitmap headers and required-value masks agree with those disc definitions.
`E22900B4` includes a numeric parameter in addition to its diffuse bitmap;
its original parameter type is now handled rather than rejecting the count.
See `analysis/frontend/title-particle-live-values-sprite.json`,
`analysis/frontend/title-particle-values.json`, and
`tests/fixtures/title_particle_values.md`.

This proves the definitions used in these live calls and their CPU value/sprite
calculation, not the originating scenery placement or emission/physics history.
The observations span multiple live calls; they are not a matched-frame replay.
They do not implement particle creation, motion, lifetime, material dispatch,
transparent ordering or unsupported GPU styles. Captured owner/cache records
remain diagnostics and are never substituted for live frontend simulation.

Execution of unchanged `23ECD0` confirms default attribute bindings 1..8.
Execution of unchanged `23ED30` confirms the complete 46-word submission:
direction, blend, position, size/angle, packed color, UV A, geometry, UV B,
then the original quad corner sequence. The recovered C prototype's pointer
order is wrong for the optimized executable ABI. Geometry is supplied in EBX;
UV A and UV B are stack arguments 7 and 8. Consequently default attribute 3
receives UV A and attribute 6 receives geometry. The native packet constructor
matches all words in 192 fixtures, including reordered explicit bindings.
Only allocation is intercepted; the original packet-writing instructions
execute unchanged. `original_particle_submission` retains those independent
records. See `analysis/frontend/title-particle-submission-differential.json`.

The cache loader now exposes explicit compiled `vrtx` classifications. The
particle tag `E20E0099` has resident classifications 7..11. Classification 7's
51 instructions agree with the captured geometry and most output values for
all 28 vertices from seven original draws. Using the captured Xemu shader's
`OpDot`, contraction rules and `ARL` bias reduces the GPU differences to three
words: event 5883 vertex 1's Z and W, and event 6227 vertex 3's fog alpha.
Screen XY, UV coordinates and RGB outputs agree byte for byte in these cases.
These remaining differences are recorded without tolerance fitting; shader
parity and live declaration selection are still false. The selected arithmetic
path currently runs only in the diagnostic particle compute shader, leaving
the production world interpreter unchanged. The report records both binary and
shader hashes at `analysis/frontend/title-particle-vertex-source-arithmetic.json`.

Those three differences are now resolved in a directly compiled probe. The
generator translates the actual classification-7 disc tokens into 51 direct
instructions, preserving parallel MAC/ILU sources and writes and the captured
arithmetic contract. All 1,008 compared words from the 28 original vertices
match byte for byte on the tested Vulkan GPU. No expected vertices, captured
constants or fixture attributes are embedded in the generated shader. The
independent capture inputs/outputs are retained in `tests/fixtures/`, and
`original_particle_vertex_gpu` requires exact float bits after original screen
snapping and color saturation. Actual graphics-stage clip conversion,
interpolation and fragments still require their own checks. See
`analysis/frontend/title-particle-vertex-specialized.json`.

Original `2243C0` reads the authored style at particle offset +4 and passes it
to `23EA80`. The latter selects first-LOD layer/pass metadata, takes the
first pass implementation's vertex reference, and binds classification
`7 + style`. Direct execution of those routines, including original tag-group
dispatch `137BD0` and the particle object's original virtual methods,
confirms this selection for all three actual definitions at layer 15 / pass 0.
The native selection and program bytes match every original result. Six render
setup calls are intercepted and named in the report; declaration lookup is
executed unchanged up to the actual `1C590` binding call. This proves the
bounded source selection path, not captured live-emitter identity, material
side effects, pixel implementation selection or actual native draw submission.
See `analysis/frontend/title-particle-shader-selection.json`.

`Cache::load_particle_material` now accepts an explicit pixel implementation
and reads the particle's +8C shader definition and its authored template/pass
metadata. The two implementations of all three bounded particles have no
fragments or external remaps; unsupported switch/extern state is rejected.
Native programs, 128 vertex constant floats, raw render states, original-decoded
samplers, bitmap bindings and all texture mip bytes agree in six original
records totaling 780,078 bytes. The oracle executes unchanged `17000`, `17170`
and `189A0`; expected RGBA8 mips come directly from the original Vulkan capture.
`prepare_xemu_bc1_texture` reuses the original integer expansion already used
for title haze. The native texture loader therefore avoids hardware BC1 endpoint
rounding for these effects. See `analysis/frontend/title-particle-material.json`
and the independent `original_particle_material` regression records.

The shared native combiner and `particle_depth.glsl` replace both original
particle fragment modules in a bounded capture replay. All color/depth buffer
bytes agree at 80 checked draws; 61 of those draws visibly contribute, totaling
7,450 pixel writes including overlap. Original vertex interpolation, texture
sampling, blending and depth state remain in the replay. Unchanged depth target
bytes alone do not establish individual fragment depth values. This check uses
disc program constants in the replay wrapper; native dynamic material-buffer
arithmetic and the full native graphics pipeline still need their own comparison.
The compiled `particle.frag` shares these functions but is not yet scheduled by
the frontend. See
`analysis/frontend/xemu-capture/particle-fragment-replay-comparison.json`.

The particle module is not yet scheduled or submitted by the frontend.
Emitter creation, death-effect propagation, motion/physics, random-state
consumption, transparent sorting, native vertex/pixel pipeline integration, and actual
particle framebuffer comparison remain open. The rebuilt frontend is captured
in `analysis/frontend/title-particle-fragment-aspects/`; these screenshots verify
that the existing presentation still works across the five display cases,
not that the missing particles have been rendered.

Only the direct haze composition path is scheduled. `2BCD0` can instead build
and reuse a cached target around transparent groups (`276D0`); the group
admission and that scheduling remain incomplete. Noise placement also depends
on the original unsynchronized RNG seed and preceding camera history. The
native seed is logged, but the supplied Xemu screenshots do not expose those
inputs. These checks establish the bounded calculations and shader wiring,
not noise placement, texture-filtering precision or framebuffer equivalence.

## Original non-HDR bloom implemented

The menu initialization script enables bloom override and supplies threshold
0.30000001192092896. Original `22590` initialization, `137A0` cinematic queries
and `13870` evaluation yield amount 0.5, brightness 0.5 and final strength
0.375 after render time zero. At the exact zero-time boundary the cinematic
query still returns its zero initial values; the native title currently uses
the settled defaults whenever bloom is enabled.

`src/menu_bloom.cpp` ports all 60 words of the threshold, blur and composite
programs from `214F0`, `211A0` and `20F30`. It preserves nearest-even constant
packing and filter radii 0.5, 1, 1.25, 1.5625 and 1.937500238418579.
The original allocator confirms targets 7 and 8 are both 160×120.
The threshold and four blur passes alternate those two RGBA8 surfaces; the
composite uses original screen blending (`ONE_MINUS_DST_COLOR`, `ONE`, add)
before UI rendering. This is not an invented Gaussian/additive effect.

Executing `34A90` and coordinate callback `34100` shows that normalized tap
offsets use half the radius divided by the destination target's dimensions,
including the initial scene downsample. The implementation follows those
coordinates, uses linear clamp, and excludes presentation borders from the
source rectangle.

```bash
.tools/python/bin/python tools/differential_menu_bloom.py
.tools/python/bin/python tools/audit_title_post_targets.py
ctest --test-dir build-frontend -R retail_menu_bloom_gpu --output-on-failure
```

The bloom differential passes 1,025 threshold/strength cases: all 180 program
words and five radii match exactly. The allocation/coordinate audit checks
2,688 tap coordinates with maximum normalized error 5.960464477539063e-8.
The shared GPU shader passes 6,144 threshold/blur/composite fixtures against
independent scalar arithmetic (maximum error 1.19209e-7). This covers the
non-HDR title path; layer-17 HDR accumulation and original framebuffer pixels
are not certified.

## Current runtime checks and remaining visual differences

Eight focused normal CTest groups pass, including all four atmosphere/bloom GPU
groups, display transfer, view constants, shader animation and appearance inputs.
Four atmosphere/bloom groups also pass ASan/UBSan after the viewport and planar
corrections, with GPU-driver leak detection disabled. A sanitizer title resize
capture completed without sanitizer, Vulkan or synchronization-validation
diagnostics. The normal 1000×600 resize capture's two 100-pixel borders have
maximum RGB channel value zero. Current results use the `planar-fog-*` prefix
in `analysis/frontend/`. Leak detection separately reports allocations from
external Vulkan modules; it is not included in the passing sanitizer result.

The current focused camera, frontend navigation and movie checks pass, as do
`native_systems` and `retail_system_runtime`. These are focused results, not a
claim that every test in the shared checkout has been rerun.

Fresh native frames still differ from the Xemu references in surface brightness,
background atmosphere and beam appearance. Original camera phases have not been
registered precisely to the supplied screenshots. Candidate contour rankings
are exploratory, not a pixel-parity verdict.

Material planar-fog passes bind external textures 14 and 15. Direct execution
and disassembly of `19530` establish that both use the rasterizer gradient tag:
extern 14 selects bitmap frame 0; extern 15 selects frame 1. These are not two
replacement depth textures. `1332F0` builds visibility flags and `4CBF0` selects
or skips a layer-16 implementation from those flags and the current camera
state. The rewritten reference C++ has an inverted `0x400` condition in
`1332F0`; the original instructions agree with the raw decompile. No blanket
planar pass was added based on that erroneous rewrite. The later executable
audit below establishes the bounded BSP admission directly.

## BSP planar fog implemented

`tools/audit_menu_fog_state.py` executes `12E5E0`, including original cluster
traversal, sky aggregation, tag queries and normalization, with no function
stubs. It first runs static initializer `377110`; its sky zero-spread fallback
is one, rather than uninitialized BSS zero. Original `14A280` locates the
camera's actual cluster from the unchanged collision BSP. Six flyby positions
and five boundary positions are tested against all 70 cluster inputs.

`16E210` confirms that every cluster resolves the same fog palette datum.
Original `134390` uses that query for visibility flag `0x400`; `1332F0` produces
the layer-16 admission, and `4CBF0` chooses implementation 0. Fully submerged
classification 5 omits the pass. Scene-object flags and their potentially
different implementations remain outside this admission.

`menu_fog_externs` supplies `336F0`/`33980` scalar and color banks after the
original submerged transition. In particular, scalar 25 is the transition,
while scalar 34 is the planar-eye fraction; those values are distinct.
All used banks and all 60 pixel-program words after `19CA0` agree byte for byte
in 770 cases. The report records small sky-aggregation rounding differences
in distance/density fields for some cluster inputs (including flyby tick 2100);
the simpler native sky loader does not yet reproduce those intermediate fields
byte for byte. They are not covered by the pixel-extern equality claim.

`prepare_menu_bsp_fog` retains the first LOD's actual layer-16 pass. Externs
14/15 bind frames 0/1 of the rasterizer-global gradient datum. The renderer
executes the original vertex code and pixel combiners, tests equal depth, writes
no depth, and blends with constant `00FFFFFF`: source RGB one, source alpha
zero, destination `ONE_MINUS_SRC_ALPHA`. Original instructions at `2C0DB`
place this pass after the depth composite and before layer 15 at `2C173`.

The shared GPU test now covers 1,120 beam/planar fixtures, including the two
original gradient images and planar combiner output. CPU/GPU agreement and
original constant/admission equality do not certify NV2A interpolation,
reciprocal precision or matched Xemu framebuffer pixels.

The carrier's authored base/environment passes are bound, but original dynamic
object light-probe queries and shadow passes remain incomplete. Scene-object
planar fog, cached haze scheduling, original lighting inputs and a synchronized
Xemu frame comparison remain necessary before a 1:1 visual verdict.

## Bounded particle graphics verification

The particle class-7 vertex translation, original screen quantization and quad
geometry stage preserve all 15,288 captured post-stage bytes in seven sampled
draws. Replacing these stages together with the native fragment calculations
preserves all checked color/depth buffers across 80 original draws. This replay
retains original uniforms, texture bindings and framebuffer state.

The separate native Vulkan `ParticleRenderer` now uploads authored decoded
mips, source samplers and pixel programs, receives completed original particle
attributes/constants, and applies the original quad split, depth clamp, W-depth
and raster states. The actual component matches all 12 selected original draw
readbacks exactly with D24 and D32 attachments. Selection includes the seven
stage samples and every visibly contributing plain-program draw. The test uses
original prior framebuffer contents as explicit diagnostic inputs; it is not
evidence that the entire native scene matches Xemu. At that checkpoint, live
emission/simulation, camera-state production, pass implementation selection and
full transparent ordering remained unresolved. The later live scenery flak
integration and its bounded checks are recorded in [Live title flak](TITLE_FLAK_EFFECTS.md).

Original `23E460` constructs a 22-vector rotation/camera upload at physical
registers 148..169. The native constructor preserves fixed-angle FSIN/FCOS
float encodings verified against actual XBE execution and both sampled capture
uploads. Original execution also checks both shadow-copy paths and dirty-flag
reset behavior. All 90,112 constructor bytes match across 256 inputs; the pure
native constructor does not implement the dirty flag or live upload scheduler.
See `analysis/frontend/title-particle-native-graphics.json`,
`analysis/frontend/title-particle-view.json`, and
`analysis/frontend/xemu-capture/particle-graphics-replay-comparison.json`.
## Completed haze quad inputs

Original `26CA0` finishes all four float4 noise-coordinate streams on the CPU.
`34420` submits the quad before perspective interpolation. The earlier native
fragment shader instead evaluated its affine rows per pixel, and the sampled
fixture reconstructed rows from original corners with up to 9.54e-7 error.
Neither operation preserved the original rounding points.

`patchy_fog_noise_quad` and `patchy_fog_direct_quad` now execute the original
SSE expression order, position/depth terms, and title direct-branch attributes.
Unchanged `26CA0` and `27520` each produce 512 oracle cases over six viewport
sizes, with no intercepted instructions. All 327,680 attribute bytes match.
The production vertex shader receives these completed streams and uses the
captured `(1,2,0)`, `(2,3,0)` split. The noise shader preserves projective float4
sampling; composition separately interpolates both original gradient rows and
normalizes the target-19 pixel coordinates after interpolation.

Exact captured corner uploads now feed the sampled GPU check. Original noise
draw 8005 matches all 76,800 RGBA pixels exactly, including a second execution
using the native bitmap decoder. The test requires zero noise differences.
At that revision, original direct composition 8034 differed at one red channel
by one byte (pixel 348,268). The CPU constant-upload correction below resolves
that remaining captured-pass difference without a pixel-specific correction.
Five focused haze/bloom checks pass. Actual title captures at four host aspect
ratios and after resizing validate centered 4:3 presentation and black borders,
with no Vulkan errors. Extended scenery and all frontend layout/flow remain
unverified. Neither this component check nor the native screenshots recover
the original persistent random/camera state or prove whole-frame parity.

See `analysis/frontend/title-haze-quad.json`, `title-haze-direct-quad.json`,
`title-haze-completed-quads-tests.log`, and `title-haze-completed-quads-aspects/`.

## Haze combiner constant upload and vertex alpha

Hardware `PixelHistory` isolated the final composition difference at (348,268).
Original and native destination bytes, gradient texel results and sampled noise
results agree. Software pixel-debugger traces also agree, but actual fragment
outputs differ by one or two float ULPs before blending. Separate hardware
probes of the final fog constant show original G/B bits `3efcfcfd` / `3ed6d6d7`
versus native values one ULP higher. The retained JSON stores the authoritative
integer bits; these values are not evidence of a color tint adjustment.

`pgraph_glsl_set_psh_uniform_values` calls `pgraph_argb_pack32_to_rgba_float`
on the CPU, and uploads 18 float4 constants. The native title shader previously
divided packed bytes by 255 on GPU. `patchy_fog_combiner_constants` now follows
the CPU expression and constant-bank order; `PatchyFogUniform` carries those
expanded floats separately from the unchanged 60 source-program words. Both
title haze fragment shaders read this bank. The generic combiner retains its
existing default behavior for other rendering paths.

Original inline D0/D1 alpha is one at all four captured haze corners. The quad
shader now interpolates those values before combiner evaluation; replacing
them with fragment literals loses the original perspective rounding. This is
admitted for the captured title haze passes, not a recovered general inline
vertex-attribute state producer.

Both original captured passes now match exactly: 76,800 noise RGBA pixels
and 307,200 direct-composition RGBA pixels, including the native decoded noise
upload. Tests also compare every expanded constant byte to its original
uploaded float bank. These checks isolate captured shader inputs. Live original
RNG/camera history, complete world/effect rendering, full-width scenery and
every frontend screen's layout/navigation comparisons remain unfinished.

The complete production bloom chain and following authored colour filter now
have a strict RGBA comparison over 710,400 pixels. The reference's actual
filter texture binding agrees byte for byte with its bloom output. Original
screen effect `egor` -> shader -> template -> pass supplies the filter program;
the GPU test compares it with the native asset loader before rendering. All
seven resulting surfaces match every captured RGBA byte. The earlier 4,096
compute-only filter samples and their one-byte quantization limit are separate
evidence; this new check runs the production graphics shader with the actual
307,200-pixel filter source. It does not establish parity of the native world
framebuffer feeding the chain.

Evidence: `analysis/frontend/title-haze-cpu-constants-evidence.json`,
`title-haze-cpu-constants-tests.log`, `title-haze-cpu-constants-final-aspects/`,
`xemu-capture/title-haze-348-268-sampled-operands.json`, and
`xemu-capture/title-haze-348-268-cpu-constants-history.json`.
