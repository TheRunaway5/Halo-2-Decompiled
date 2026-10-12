# Retained Zanzibar appearance residual diagnosis

This source-backed diagnosis compares retained original frame 20 with the frozen
`source-instance-accelerator-reference` capture. The retained offline replay does not change images,
textures, colors or the original guest. It identifies two bounded production
corrections described below. The native executable
hash is `28b50440232164a8087f5f02fe75c15219b941adc481625354cd4d45b90d1a21`.
The original PNG hash is
`1bb3496f815ac55bd3ac092d972a795984f5c1b184bd6319ab28c1884728127d`.
Actual camera/player/device inputs are retained; complete original simulation,
HUD, startup history, controller state and shader clock are not restored.

## Where the remaining image errors occur

Pixel boxes below are half-open and use the original 640x480 framebuffer.
`analysis/multiplayer-shading/appearance-residual-cpu/localization.json` retains
raw point values, region quantiles and connected components.

| Region | Mean absolute RGB byte error | Channels differing by more than 8 | Largest error |
|---|---:|---:|---:|
| World `[152,80,472,280]` | 0.873614583 | 1,438 | 186 |
| First person `[240,300,470,480]` | 0.667061192 | 340 | 31 |

Two world components, `[233,80,269,91]` and `[351,114,382,126]`, account for
1,122 of the 1,438 outlier channels. Their mean native-minus-original RGB
values are `(-44.73,-33.64,-22.03)` and `(-38.03,-27.96,-16.10)`.
The separate purple source feature around `[205,165,209,173]` is not assigned
an object/effect identity by this evidence.

The two sleeve components `[319,458,340,480]` and `[307,469,320,480]` account
for 306 of the 340 first-person outlier channels; all 306 are red-channel
outliers. At `(330,468)`, the original final RGB is `(58,21,16)` and the fresh
native final RGB is `(27,17,14)`.

## What the existing source exports exclude

`projected_draw_overlap.py` / `.json` checks the literal exported final seven
original PRT draw triangle banks, with their actual post-VS positions and
indices. None projects over either dominant world component. Earlier native
PRT triangle exports also have no overlap there. This is a diagnostic
pixel-center triangle test: it does not emulate clipping, culling, depth,
alpha, raster edge rules or establish final draw ownership. These components
must not be attributed to crane/wheel PCA arithmetic from visual proximity.
Subsequent actual PixelHistory identifies both components as the two alpha sky
parts, source8097/8102 and native338/341. Their order differs; see below.

The fresh image differs from the earlier `native-crane-source-pose` image by
only 0.013463542 RGB bytes on average in the world region and 0.011940419 in
the first-person region. This supports reusing the earlier detailed evidence
as a nearby retained diagnostic, while preserving its different capture hash
and observation time.

The actual hand-lighting external-3 images are exported at original draw 8862
and earlier native draw 2368. Both are 640x480 B8G8R8A8_UNORM scene/base
accumulation targets. At `(330,468)` both contain RGB `(18,18,18)`; at
`(331,469)` both contain `(16,16,16)`. Across the two dominant sleeve boxes,
the maximum RGB differences are `(3,5,3)` and `(3,2,2)`, with no channel over
8. The large final red errors therefore cannot be directly assigned to the
small raw scene/base differences already measured at these positions.

The retained source final environment draw 8878 now has actual
DebugPixel/PixelHistory at both sleeve outliers, paired with native2375.
Its active camera-color C0 (`00A2058B`) and authored environment C6
(`00787387`) are already pinned in `retail_zanzibar_fp_bindings.tsv`.
At (330,468), both final environment draws produce post-blend RGB (21,17,14).
The eventual large red residual is added by the bloom chain, after this pass.

## Proven fragment-input differences and their limits

The existing `native-crane-source-pixel-proof.json` contains eleven paired
actual debugger fragments. Re-evaluating the native combiner with all original
interpolants and samples reproduces their original results within 8e-8;
re-evaluating the actual native inputs reproduces native GPU results. These
records distinguish:

- Three SMG paint fragments: the residual is isolated to their authored BC2
  base sample; the other three samples are exact. Replacing only stage 0
  reproduces original output within 5.97e-8.
- Three hand-lighting fragments: only the scene/base target sample differs.
  Replacing it reproduces original output within 1.64e-7.
- Five crane fragments: stage 1 bump and stage 3 external cubemap samples are
  exact. Stage 0 scene and stage 2 authored BC1 cubemap both contribute.

These DebugPixel selections include depth/alpha-rejected fragments. They are
not final composited-pixel proofs. The earlier eleven-fragment set does not cover those components; the new
retained replay below adds those actual draw/point observations.

The actual native compressed blocks match authored blocks unchanged. The
existing BC2 proof finds 144,346 differing mip-0 components against xemu's
expanded RGBA8 texture; the BC1 crane cube proof finds 125,205 of 393,216.
Most are a one-byte difference, and alpha is exact. Actual native GPU point
samples corroborate its compressed decoding. This comparison does not prove
physical NV2A decode/filter rounding. Substituting xemu-expanded textures or
fitting a correction would not establish source or hardware parity.

## Actual retained replay and source-proved corrections

`analysis/multiplayer-shading/appearance-residual-replay/replay.py` replays only
immutable RDCs, validates their SHA256 before/after, exports literal programs,
constant banks, every bound texture mip, sampler/blend/depth state, complete
DebugPixel traces and PixelHistory at five explicit points. Original input is
`original_capture_20.rdc` SHA256
`d99a5c97c85916c7b07e30dc1f181d6db81b3a5246686bcbeebebbc5445642ee`.
The detailed native input is the older `native-crane-source-pose_capture.rdc`,
SHA256 `87daf6928c198daf50b25cf83fb31c507a0c0f360a1c2f1a97543c23b3feecf2`.
It is not the later 28b504 executable snapshot. All five selected final pixels
match between these two native image versions; nearby image regions have the
small differences quantified above. No observation of a new native draw is
inferred from that image equality.

RenderDoc reports `depthTestFailed` on some W-depth color-changing fragments.
The tool retains their literal flags and color changes rather than silently
excluding them or treating those flags as definitive final ownership.

### Sky alpha ordering

Source20 draws the sky's A part8097 followed by B part8102 and moon8110. Native draws
B338 then A341. A binds the4x4/512x512 images; B binds two256x256 images.
Both use authored SRC_ALPHA/ONE_MINUS_SRC_ALPHA RGB blending and RGB-only
writes. A's alpha1 covers B when native sorts the material IDs. At (367,120),
source A leaves (71,83,98), then B leaves (119,126,132); native B leaves
(98,105,110), then A leaves (71,84,99).

Renderer's background grouping is corrected to retain contiguous authored
part runs, including repeated material IDs, rather than reorder them by
material ID. The same bounded correction applies to per-view skies and water
reflection skies. Other geometry batching is unchanged. The focused actual
asset regression `source_sky_order_gpu_tests` checks the180/96/6-vertex source part order, an
explicit reversed-run negative control, and repeated/fresh rendering across
640x480 and1280x720. It does not fit expected framebuffer colors.

The second B texture also has a retained phase difference: source interpolated
UV is (4.8411088,3.8606794), native is (4.0502763,3.0001233). Base UV agrees
within1ULP. Original c22.w=.795833588 and c23.w=.863889694 are the authored
scroll offsets; native's near-start shader clock produces near-zero offsets.
Both actual bound textures have one mip and linear repeat filtering. This is
an explicit original shader-clock/context boundary, not evidence to change
sampler offsets or fit cloud colors.

### Sleeve bloom

Source final bloom9106 makes (330,468) RGB (58,21,16), while native2422 makes
(27,17,14). Their final bloom samples are (.209002823,.021759365,.009811551)
and (.038483255,0,0). Both final strength constants are RGB96/255; both blur
chains have alpha178/255 and radii1,1.25,1.5625,1.937500238. Actual source
quarter-resolution tap coordinates match the native resolution-derived
producer, so no blur-offset patch is justified.

The difference starts at source threshold8941: source C6 is0x20ACACAC,
whereas native2387 C6 is0x20E5E5E5. Original13870 calls245C0 with mode0,
which resets the normal player context's +0x10 to literal45DC2C=.75. Its
ordered threshold calculation yields172/255 after multiplying the authored
.9 base. Native `BspPostprocess::prepare` omitted that actual player screen
context and therefore used the helper's default factor1. The separate
`source_bloom_context` module and untouched source oracle recover that
producer; normal player postprocess views now explicitly supply it. No
exposure value was derived by fitting the image.

### Other retained limits

Source8787 adds two purple fragments at (206,168), while native has only its
base426 at that point. Its programs, textures, interpolants and raw draw banks
are exported. The later [energy-blade identity proof](ORIGINAL_ENERGY_BLADE_DRAW_PARITY.md)
establishes the actual third-person weapon section, authored indexed geometry
and consumed owner palette. The [native netgame weapon scene](SOURCE_NETGAME_WEAPON_PARITY.md)
now submits the source-born levitated row at its original pose and scale.
Fresh versioned captures retain a blue/purple fragment difference; source
shader phase and texture residency are still unresolved. The older RDC
comparison above remains evidence for its recorded executable and draw list.
Final original/native equality, physical NV2A compressed filtering and
complete live simulation/shader-clock parity are not claimed.

Fresh correction captures and validation are recorded separately from the
older detailed replay. Raw retained traces are under
`appearance-residual-replay/source` and `/native`; prior CPU localization and
fragment/texture provenance remain unchanged.


## Fresh both-correction capture

`analysis/multiplayer-shading/live-sky-bloom-source-fix` freezes native binary
SHA25613a8ec4f5766f5d079f7d07b276b7b9c0ffa2bc6d65e1391ed7c5bca13a7f440
and all matching SPVs, raw PPM/PNG, command, source/reference/map hashes and
an inspected phone comparison. Required SDK/synchronization validation emits
no diagnostics. The actual sky-order GPU controls pass at640x480 and1280x720,
including a return to640x480 and separate fresh renderers.

| Diagnostic region | Prior MAE / channels>8 | Both fixes MAE / channels>8 |
|---|---:|---:|
| World |.873614583 /1438 |.738937500 /827 |
| First person |.667061192 /340 |.619677939 /32 |

Sleeve(330,468) now gives native(59,22,16), original(58,21,16), versus prior
native(27,17,14). The sky phase and purple submission limits above remain.
