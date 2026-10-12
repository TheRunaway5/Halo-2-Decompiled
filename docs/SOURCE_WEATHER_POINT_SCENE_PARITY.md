# Original weather point draw over saved scene targets

The isolated Vulkan point pass now replays original RenderDoc event 215873 over
that draw's actual saved scene color and D24S8 depth. This measures one snow draw;
the background geometry and lighting remain the original saved image. Native
weather publication, simulation, view admission and bitmap residency are not
activated by this prototype. The reference runs in unmodified xemu OpenGL
llvmpipe, and does not establish physical Xbox raster precision.

## Source packet and exact bindings

The immutable original reference is
`analysis/multiplayer-shading/weather-point-original-reference-event215873/packet.json`,
SHA256 `f0b7b6634df523ae29757f2920e722ab4c880d91136e4220adc7352ee0e3c512`.
The original action starts at 0, emits 1,015 points, and has one instance. All
position bytes are retained. Each of the 4,060 captured float color components
matches an exact `float32(byte/255)` word before packing logical ARGB: 1,012 rows
are opaque white, two have alpha DD, and one has alpha F5.

All nine literal VS22 tokens match the earlier unchanged `391F0` proof. The full
12-vector camera is captured from this event. The executed original combiner
body matches the separately source-proved `39780` definition; the 60 words remain
that original-call fixture, rather than a claimed unique reconstruction from
GLSL. The rounded-byte alpha comparison is GEQUAL with reference zero.

The unchanged 1,408-byte BC3 partition supplies the actual four levels
32/16/8/4. An independent pinned original S3TC decoder matches all 5,440 bytes
of the four captured RGBA levels. The captured sampler is repeat in all axes,
linear minification/magnification/mip filtering, bias zero. The source pass uses
RGB-only writes, SRC_ALPHA/INV_SRC_ALPHA blending, LEQUAL, W depth, no depth
writes, stencil disabled, and the full positive 640x480 viewport.

The native fixture uses a real D24S8 attachment and LOAD continuation. Original
GL packed depth words have depth in bits 31..8 and stencil in bits 7..0. Upload
moves the unchanged depth integer into the Vulkan depth-aspect copy's low 24
bits and uploads each original stencil byte separately. All 307,200 depth
integers and stencil bytes read back exactly before and after the draw. No
floating depth image, clear scene, or approximated depth substitutes for these
inputs. Every target alpha byte remains unchanged.

## Source row-coordinate conversion

Both source and native wrappers map the same numeric source screen Y to the
same numeric saved framebuffer row. Original GL window Y increases upward,
while positive Vulkan viewport Y increases downward. The actual captured GL
point origin is upper-left. At the same row, therefore,

```
t_GL = 0.5 - (row - center) / diameter
t_VK = 0.5 + (row - center) / diameter
t_GL = 1 - t_VK
```

The prototype requires explicit `OriginalGlRawRowsUpperLeft` admission and
converts only this point-coordinate Y. It preserves authored program and sprite
bytes, point inputs, camera, and raw scene/depth rows. The unadapted replay is
retained separately. A whole-image presentation flip is not used for GPU input;
the comparison PNG reverses both outputs equally, which exactly matches the
original RenderDoc SaveTexture PNG's RGB rows.

## Measured result and remaining limits

The source draw changes 1,255 pixels. The native draw changes 1,242, all within
the source effect mask. After the coordinate conversion, 306,471 of 307,200 RGBA
pixels are exact and 729 differ. Of those, 688 differ by at most one channel
step; 41 exceed one, nine exceed eight, and the maximum channel difference is
18. Total absolute RGB difference is 1,900. Before the conversion, 787 pixels
differed and the absolute RGB difference was 2,691. Neither comparison uses a
fitted tolerance to claim image parity.

All 13 remaining source-only pixels lie at point-square edges. The unchanged
original software replay's post-VS output confirms that their nearest-owner
screen X/Y and floor W match the native CPU original-program execution exactly.
They are well in front of the saved scene depth. The tested RTX5080 reports
point-size granularity 0.0625; the original replay and native host have different
raster implementations. A particular host point-rounding or coverage rule is
not recovered or compensated for. Remaining one-step texture/blend differences
are also retained without fitting filtering or reciprocal arithmetic.

The complete original replay output has 1,015 rows of stride 204. Its D0 matches
all 4,060 native CPU words. Across the entire cloud, 432 W words and 349 point
sizes differ from ordered native CPU arithmetic; the maximum differences are
0.01171875 and 0.0107421875 respectively. This is a software replay comparison,
not an NV2A precision assignment. The original point-size range is
1.0416484..750.75525; the native CPU literal-program range is
1.0416484..750.76599. All are within the actual tested device's admitted range.

Normal and fully instrumented ASAN/UBSAN runs pass with actual Khronos validation
and synchronization validation, zero diagnostics, exact reused bindings, and
unchanged depth/stencil/alpha. CPU sanitizer runs include leak detection; GPU
runs exclude driver-owned leaks with `detect_leaks=0`.

![Original and isolated native point draw](../analysis/multiplayer-shading/weather-point-scene-prototype/phone-source-native-point-scene.png)

## Reproduction and files

- `tools/freeze_source_weather_renderdoc_point_packet.py` freezes source inputs
  and byte/metadata controls from already saved exports; it performs no replay.
- `tests/source_weather_point_scene_tests.cpp` supports default CPU validation
  and `--gpu` for the exact saved targets and draw. Its separate private build
  helper is `tools/build_source_weather_point_scene_prototype.sh`.
- `tools/compare_source_weather_point_scene.py` produces histograms, source-only
  point localization, raw-row PNGs and the phone comparison from saved outputs.
- `tools/audit_source_weather_point_coordinate_bridge.py` pins the original
  point-origin state, shader expressions and row-coordinate conversion without
  replay, guest access or a pixel fit; its report is `coordinate-bridge.json`.
- `analysis/multiplayer-shading/weather-point-scene-prototype/scene-comparison.json`
  and `source-only-point-localization.json` retain counts, dependencies and limits.

The module and test do not change Renderer, multiplayer runtime or frontend.
The separate controlled 238-point resolution and admission tests remain in
[SOURCE_WEATHER_POINT_GPU_PROTOTYPE.md](SOURCE_WEATHER_POINT_GPU_PROTOTYPE.md).
