# Connected original weather effects over the captured Lockout scene

The isolated Vulkan fixture now connects the actual current D24S8 image to
the exact packed alias producer, original plate event215778, and original snow
event215873, in that order. Clouds and snow are native draw results over the
original captured scene background. Renderer, multiplayer runtime, frontend,
weather admission, simulation, timing and residency are not activated here.
The original reference is the unmodified title in xemu OpenGL llvmpipe; it
does not establish physical Xbox/NV2A raster precision.

![Original and connected native weather](../analysis/multiplayer-shading/weather-connected-scene-prototype/phone-source-native-connected-weather.png)

## Source continuity and unchanged inputs

The source-inspected unchanged `36B50` draw order calls all `37A60` plates
before all `391F0` particle draws. The actual captured surfaces prove this
pair connects: all 1,228,800 plate-after color bytes equal point-before bytes,
SHA256 `795c795d86c3cf147889d0696bd2a6d6389bb466d8251311027339de306ec0bb`.
The plate before/after and point before/after raw depth surfaces are identical,
SHA256 `9aa4825069b787f19ebbf49b4f5b1de364891749b23c295d98a1221e15a5484d`.
All source target-alpha bytes remain unchanged across both events. This
continuity does not substitute for native world/visibility/frame producers.

The test retains the original plate four vertices, literal five-token VS21,
complete 60-word source mode3 pixel definition, three scroll rows, clip bank,
eight clip regions and seven captured mips for each of stages1..3. Stage0 is
never uploaded from the captured alias. It comes only from the producer's
binding after recording exact integer depth/stencil copies from current D24.

The snow packet is the existing original event215873 typed packet: 1,015
unchanged position/logical-ARGB rows, the complete 12-vector camera, literal
nine-token VS22, source-compatible/source-proved 60-word39780 definition,
original clip range, enabled programmable points/sprites, explicit disabled
stencil and actual four BC3 levels32/16/8/4 in the 1,408-byte partition. The
source pixel body establishes compatibility with that original definition;
it does not uniquely reconstruct title-side words from compiled GLSL.
The original packet's repeat/linear/linear-mip sampler has zero bias and LOD
bounds -1000..1000, which this new fixture retains explicitly. No point-size,
coverage, filtering, reciprocal or palette fit is introduced.

`OriginalGlRawRowsUpperLeft` remains the explicit point-coordinate adapter.
Source geometry, sprite, target and depth rows retain their raw capture order.
Both display images reverse rows identically to the original SaveTexture PNG
convention. The generated source display PNG was checked byte-for-byte in RGB
against the original after-snow SaveTexture PNG. Every metric uses raw bytes.

## Shared native resources and ownership

Only the original before-plate color and original D24/stencil are uploaded as
scene targets. One real D24S8 image and one RGBA8 target serve the entire chain.
The fixture's logical device enables actual largePoints, depthClamp and BC
texture compression for the snow path. The alias producer records outside a
render pass, restores the source depth layout and exposes its owned sampled
UNORM view with nearest/clamp sampler. The plate consumer uses that view
directly as stage0.

A LOAD plate pass runs first. Its target is copied for the intermediate proof,
then restored with explicit color-attachment read/write visibility. A LOAD
snow pass continues over that actual native plate result. One caller-owned
fence covers producer, both consumers and all readbacks; resources are retired
before reprepare/reuse or destruction. Readback barriers include actual
fragment/compute consumers and explicit transfer-to-host visibility. The
depth/stencil verification also uses explicit host visibility.

The retained source point fixture provides its unchanged upload helpers and
logical-device setup through a renamed, never-called entry point. Its old
output paths are never written. New normal and ASAN builds compile the three
unchanged native modules directly; the frozen cache archive remains
uninstrumented. No existing module, shader, test, manifest or old readback is
edited by this connected fixture.

## Measured combined result

Normal and ASAN/address/undefined-behavior GPU checks passed on NVIDIA GeForce
RTX 5080 with actual Khronos SDK layer 1.4.363 and synchronization validation.
CPU ASAN/UB passed with leak detection enabled. GPU address/UB uses
`detect_leaks=0` without suppressions; the separately documented external
graphics LSAN limitation remains and this run is not a clean graphics leak
claim.

All 307,200 generated packed-alias pixels match original stage0 exactly. All
depth integers, stencil bytes and target-alpha bytes are preserved before and
after both draws. Intermediate native plate bytes equal the previous native
plate proof exactly, SHA256
`535179c90f409716be4f27f9e3953f5e9f1196c83f7a08f5a3a0bb2a0552c86f`.
The final connected native scene SHA256 is
`702aaa78459f131f8f8b4d6a1f4d1441758c032e772ad4cc600cc83d9ebf4403`.
Normal and sanitizer stdout/readbacks match. Reused bindings and results are
exact, 13 admission/ownership/viewport controls are rejected, and SDK
synchronization records zero errors.

| Measure | Pixels |
| --- | ---: |
| Original response from the before-plate background | 181,816 |
| Native response from the before-plate background | 181,483 |
| Original snow response from the original plate result | 1,255 |
| Native snow response from the native plate result | 1,242 |
| Native-only combined response | 0 |
| Original-only combined response | 333 |
| Exact final RGBA comparison | 292,041 |
| Differing final RGBA comparison | 15,159 |

Of the 15,159 differing pixels, 15,095 differ by at most one RGB byte; 64
exceed one, nine exceed eight, and one exceeds 16. Maximum channel difference
is 18 and total absolute RGB difference is 33,960. The cloud stage retains its
prior 14,477 differing pixels, maximum RGB difference 1, and 333 original-only
responses. The snow stage retains exactly the same 13 missing source edge
pixels as the earlier standalone snow comparison, with no additional missing
or native-only snow response pixel.

Final color differences include the cloud differences carried into the snow
background. Snow-stage response masks are measured relative to each pipeline's
own plate result; their 13-pixel coverage boundary is not counted as a claim
of 13 total differing final pixels. The original/native texture/blend,
arithmetic and raster boundaries remain unresolved. No tolerance is used to
declare full image parity.

## Reproduction and proof artifacts

New files are `tests/source_weather_connected_scene_tests.cpp`,
`tools/build_source_weather_connected_scene.sh`, and
`tools/freeze_source_weather_connected_scene.py`. The builder supports private
normal and ASAN builds:

```sh
bash tools/build_source_weather_connected_scene.sh \
  analysis/multiplayer-shading/weather-connected-scene-prototype/build normal
bash tools/build_source_weather_connected_scene.sh \
  analysis/multiplayer-shading/weather-connected-scene-prototype/build-asan asan
python3 tools/freeze_source_weather_connected_scene.py before
```

The executable runs CPU source continuity/typed-program checks by default,
the complete connected fixture with `--gpu`, and an empty context with
`--gpu-baseline`. GPU checks use:

```sh
export VK_LAYER_PATH="$PWD/.tools/validation/usr/share/vulkan/explicit_layer.d"
export LD_LIBRARY_PATH="$PWD/.tools/validation/usr/lib"
export VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
```

The sanitizer options are `UBSAN_OPTIONS=halt_on_error=1` and CPU
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`; graphics address/UB uses
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`. Keep normal/sanitizer CPU and GPU
logs with the names in the validation record, then collect raw comparisons,
histograms, input stability and screenshots with:

```sh
python3 tools/freeze_source_weather_connected_scene.py after
```

`analysis/multiplayer-shading/weather-connected-scene-prototype/validation.json`
records the 88 frozen input hashes, build/shader/readback hashes, exact source
continuity, all metrics and limits. The companion
`inputs-before-validation.json` is checked again after validation. Existing
producer, plate, point and pipeline proofs remain separate and unchanged.
