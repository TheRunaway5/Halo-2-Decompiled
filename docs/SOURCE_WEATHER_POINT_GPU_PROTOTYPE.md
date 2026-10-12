# Isolated source weather point pass

This records the isolated captured-packet proof. The later
[live Lockout snow checkpoint](SOURCE_WEATHER_LIVE_SNOW.md) connects its point
consumer to the normal multiplayer owner and retains this strict entry.

The optional `SourceWeatherPointRender` prototype consumes the frozen original
391F0 packet. It does not activate weather in the native game. The packet comes
from an unmodified original title running in xemu0.8.136/OpenGL llvmpipe. Physical
Xbox rasterization, a matched scene framebuffer and the live native publication,
simulation, resource-ready and camera-admission producers are separate limits.

## Authority and inputs

The saved391F0 entry and its exact36F00 return are documented in
[SOURCE_WEATHER_DRAW_SUBMISSION_AUDIT.md](SOURCE_WEATHER_DRAW_SUBMISSION_AUDIT.md).
Complete unchanged execution verifies1,728 distinct original instructions,
34,021 steps, all8,872 non-stack source-written bytes, all238 ordered point/color
commands and the7,680-byte appended command span. No SDK/callee result is supplied.

The immutable packet is
`analysis/multiplayer-shading/weather-particle-packet/packet.json`:
SHA256 `2d9f046cc812d4b865ba97b2bbb6af80cca9f5327a84562ce84ab07d4d58ba83`.
The isolated pass retains:

- Original VS22/class0 payload `3f9e4e60…f1a1ee`, allnine instructions.
- All12 actual camera vectors, preserving the original pixel conversion.
- Complete original39780 non-rain60-word pixel definition `ea5ef59b…549b`.
- All238 immediate v0.xyzw and logical ARGB v1 words in original order.
- Current resident32×32 BC3 sprite, fourlevels32/16/8/4, raw1,408-byte partition
  `6f5c36ca…5a1133`. The captured bytes equal the authored physical partition.

`tools/audit_source_weather_point_inherited_controls.py` separately reads both
immutable RAMs and pins the literal SDK/XBE and xemu sources. Its report is
`analysis/multiplayer-shading/weather-point-inherited-control-inspection.json`.
This is SDK/cache and instruction authority; no live PGRAPH state is invented.
Stage3 inherits WRAP U/V/P, linearMIN/MAG/MIP, bias0/maxMip0. It binds the exact
sprite header. The inherited write mask is RGB only, stencil disabled. The draw
uses W-depth/LEQUAL/no writes, alpha GEQUAL 0, SRC_ALPHA/INV_SRC_ALPHA and actual
near/far words437FFFFF/4B7FFFFF. Point polygon offset produces zero uniforms;
the SDK negative-zero factor/bias caches are not converted into an extra offset.

## Typed boundary and raster

`include/halo2/source_weather_point_render.hpp` accepts explicit device/pass,
fullcamera, sourceprograms, ordered immediate points, current sprite view/sampler
and source raster inputs, including explicit original GL raw-row/upper-left point
coordinates. It owns only the point pipeline and per-submission
buffers/descriptors. The caller owns the scene targets, resident sprite image,
command submission/fences and all producer state. `prepare()` rejects other
programs/pixel definitions, missing enabled-point modes, a substituted mip-chain
shape, missing/enabled stencil and a nonzero point depth offset. Before replacing
any valid binding, it executes the original VS22 on every point and rejects
nonfinite positions, nonpositive W or diameter, and diameters outside the actual
physical device range. This CPU admission does not establish GPU rounding exactly
at the size-range boundaries. Invalid input leaves the earlier binding intact.
It never publishes a bitmap or chooses a clock, particle count or readiness result.

`record()` requires the prepared full-surface viewport: origin 0, matching width
and height, and depth range 0..1. Its scissor may be a subset of that surface.
Pinned original `gl/draw.c` sets the full-surface viewport; a second split viewport
would retransform source pixel coordinates and is rejected rather than inferred.

Pinned xemu commitfc24584 `pgraph/glsl/vsh.c` and `vsh-prog.c` directly determine
the wrapper: truncate original screenXY to1/16pixel, preserve the signed W clamp,
convert screenXY through the supplied surface and multiply clipXYZ by W.
At640×480 the native source12FA0 target bank exactly equals the saved c10/c11:
`(320,-240,16777215,0)` and `(320.5,240.5,0,0)`. The source half pixel is retained;
there is no extra pixel-center adjustment or point-diameter multiplier.

The actual POINTPARAMSENABLE1 preserves shader oPts and writes it directly to
`gl_PointSize`. The SDK fixed-pointsize8 and maximum64 do not replace or clamp it
in this pinned programmable path. The fragment source uses `gl_PointCoord`
directly for stage3, executes the original combiner words, and compares
`int(round(alpha*255)) >= 0`, matching the packet's method 033C value 0x206 and
method 0340 reference 0. A zero-alpha fragment passes this test but contributes
no RGB through the source blend and writes no depth. It also retains
the pinned degenerate-point W-depth/D24 conversion. To preserve original GL raw
framebuffer and sprite rows in a positive Vulkan viewport, it converts the point
coordinate Y with `1-t`. Original GL upper-left point coordinates give
`t=.5-(row-center)/diameter`; Vulkan gives `t=.5+(row-center)/diameter` at the same
numeric row. Actual source event 215873 confirms the upper-left origin and full
positive viewport. This conversion changes no source program, sprite bytes,
scene or depth rows. Whole image presentation and physicalXbox coverage remain
separate limits.

## Focused checks and limits

`tools/freeze_source_weather_point_vertex_fixture.py` independently evaluates
the literal nine instructions over all238 captured inputs at640×480, odd641×481,
1920×1080 and3840×2160. Only separately proved12FA0 target-rectangle constants
change; the captured projection and simulation packet remain explicit. This is
an arithmetic/resolution control, not an original GPU output fixture.

The native CPU path matches all952 cases/60,928 words exactly. The NVIDIA GPU
check keeps W/colors/non-reciprocal outputs exact and records376 differing
reciprocal-consumer words. Their maximum absolute screenXYZ differences are
0.03125,0.015625,2 (Z uses the16777215 range). In the33 center-in-viewport cases
at640×480, maximumXY errors are0.000015259 and0.000030518 pixels. The consistency
tolerance applies only to the native float scale of reciprocal consumers. It
does not assign Xbox reciprocal precision or universal ULP parity.

Actual packet diameters are1.0509105..307.33505; the tested RTX5080 reports
pointSizeRange1..2047.94 and granularity0.0625. The33 viewport-center candidates
have diameters1.0509105..1.887685. Centers alone do not establish visible pixels.
No shader cap is fitted to the source SDK maximum. Host point limits/granularity,
filtering and subpixel coverage remain explicit device limits.

The graphics check uploads unchanged BC3 blocks and supplies a **controlled**
dark clear color and depth1. It changes73,70,61,64 pixels at the four extents.
Depth0, noRGBwrites and Wclip negatives suppress their pixels; zero alpha leaves
the controlled color unchanged through neutral blending. Doubled authored diameter
and nonwhite logical color controls respond. Repeated preparation produces
identical readbacks. A valid prepare followed by a rejected invalid prepare and
invalid viewport/scissor calls still renders the retained valid binding exactly.
RGB-only writes preserve targetalpha. Normal and
ASAN/UBSAN runs require the real validation layer plus synchronization validation.
The CPU sanitizer run includes leak detection; GPU sanitizer runs use
`detect_leaks=0` to exclude driver-owned allocations. Logs and executable/SPV/input
hashes are retained under `analysis/multiplayer-shading/weather-point-prototype`.

![Controlled point pass](../analysis/multiplayer-shading/weather-point-prototype/phone-controlled-640x480.png)

The image shows the real640×480 readback and a nearest-pixel enlarged crop.
It is not a final-scene comparison. The distinct actual 1,015-point draw is now
replayed over its original saved scene color and D24S8 in
[SOURCE_WEATHER_POINT_SCENE_PARITY.md](SOURCE_WEATHER_POINT_SCENE_PARITY.md).
Completed weather publication/update/admission and the current resident texture
service remain required before live integration.

## Reproduction

Generate the independent fixture, then use
`tools/build_source_weather_point_prototype.sh PRIVATE_BUILD COHERENT_CACHE_ARCHIVE
[normal|asan]`. This writes only a private directory; it does not register or
change the shared renderer/runtime/CMake. Run its executable first without
arguments for CPU, then `--gpu` with the actual validation SDK/library paths,
`VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation` and `VK_LAYER_VALIDATE_SYNC=1`.
The exact build artifacts and archive used for the frozen run are in the manifest.
`tools/freeze_source_weather_point_prototype_manifest.py` verifies completed logs
and freezes those hashes, retaining the earlier admission-only manifest separately.
