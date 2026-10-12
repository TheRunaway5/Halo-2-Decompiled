# Source camera and effect rectangles

The ordinary multiplayer path now shares one physical integer rectangle for
the source camera, world viewport, scissor, c8..11, postprocess crop, water
scene copy and scope input. This fixes a confirmed discrepancy at odd drawable
dimensions. It does not establish physical Xbox support for those video modes
or complete effect equality at arbitrary desktop resolutions.

## Original authority and the corrected consumer

Original `12FA0` reads four signed shorts at camera+30/+32/+34/+36, ordered
top/left/bottom/right. Width and height come from subtracting the two integer
edges. Its original render-scale clamp and fixed mode2 128x128 branch remain
in `viewport_constants`; the new full-surface conversion does not alter those
services. `14BC0` likewise builds the ordinary main raster viewport from the
camera's short rectangle. Native full-surface layout scaling is an explicit
resolution extension, followed by the original integer-bank convention.

Before this change, the renderer truncated the world scissor's two edges but
truncated postprocessing's width and height independently. Its Vulkan viewport,
c8..11 and water crop kept fractional half-pixel boundaries. At1100x651, the
lower-right four-player world scissor was `(550,325,550,326)`, while its
postprocess crop was `(550,325,550,325)`. The last world row therefore did not
enter that player's postprocess chain. The fractional viewport began at325.5,
although the source projection bank already stored top325.

`full_surface_viewport` expands the authored640x480 cell, truncates all four
physical edges, validates the signed-short range, and subtracts those integer
edges. Renderer and runtime use its result. Non-full-surface menu placement,
original render-scale handling and mode2 remain separate.

## Verification

`tools/differential_source_view_rectangle.py` executes untouched `12FA0` through
the c8..11 stores, stopping before device upload. The unrelated `12D50` matrix
upload is neutralized; viewport math is not replaced. The independently
transcribed `2F600/2F640/2F6B0` grid and span rules supply all160 layout inputs:
one through four players, both original layout modes, at eight drawable sizes.
All2560 words match exactly. The80 layouts at640x480,1280x720,1920x1080 and
3840x2160 retain their previous banks and pixel sizes. The80 odd-size layouts
exercise44 former crop-size mismatches and69 former fractional-bank mismatches.

Frozen input/output is `tests/fixtures/retail_source_view_rectangle.tsv`;
provenance is `analysis/multiplayer-shading/source-view-rectangle-original.json`.
Normal and ASAN/UBSAN/leak CPU checks pass. The new
`retail_source_view_rectangle_gpu` control uses source camera/stencil/scope
programs and the source bloom chain, compares fresh versus reused renderers
across odd/even/4K resizes, and checks each submitted last row and column.
CPU bank proof alone does not confirm rendered pixels. The following versioned
GPU results provide the separate rendered checks.

The subsequent GPU check passes in
`analysis/multiplayer-shading/resolution-bitmap-canonical-focused-tests.log`
(310.86 seconds), alongside six other focused checks. The frozen source build
also passes all309 registered tests, including this GPU regression, in
`resolution-bitmap-frozen-full-suite.log`. Its executable is
`d2d1a336303e9d8d67889acc9f4d22966d7ececa5ce50aad3477faf27b1a9812`;
all4,087 copied files and509 recorded command/shader inputs remain unchanged.
That executable also passes21 actual multiplayer tick31 three-player captures
at1281x721, recorded in
`live-resolution-bitmap-frozen-odd-three-player/summary.json`.

## Resolution audit boundaries

The original fixed target sizes are intentional: scope target16 is64x64,
shadow target10 is128x128, title patchy target19 is320x240, title bloom targets7/8
are160x120, and water targets21/22 are512x256. None is resized by this change.
Water's quarter-pixel source taps continue to use the actual complete scene
image dimensions. Authored texel transforms and canonical UI640x480 units
likewise remain separate from physical render dimensions.

The multiplayer postprocessor uses isolated per-view half/quarter surfaces at
modern resolutions. Existing original640x480 and retained-pass proofs do not
prove every original asynchronous surface/crop producer at arbitrary desktop
resolutions. This audit found no additional concrete source contradiction in
those surfaces, bloom radii, fog depth lookup, source water lens, or fixed scope
copy target. Physical Xbox odd-height raster/stipple behavior remains outside
the claim; the native extension retains the explicitly recorded phase31.

Resize already retires GPU work and recreates postprocess, scope, water,
accumulation and depth resources using the new swapchain extent. Postprocess
motion/RNG state is retained intentionally, while each submitted per-view size
recreates its dependent images when needed. No separate stale-extent producer
was found in the read-only audit.
