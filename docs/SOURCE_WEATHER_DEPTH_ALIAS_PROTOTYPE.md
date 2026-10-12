# Isolated exact current D24S8 weather depth alias

This records the original isolated proof of plate stage0 bytes from an actual
native `VK_FORMAT_D24_UNORM_S8_UINT` image and its composition with the plate
consumer. The subsequent [live plate integration](SOURCE_WEATHER_LIVE_PLATES.md)
activates both modules in ordinary matches using the existing native D24S8
world attachment. The isolated reference results below remain preserved.

The original event215778 capture proves the mapping for all 307,200 pixels:
the original little-endian depth/stencil word has integer depth N in bits31..8
and stencil in bits7..0; its captured stage0 RGBA bytes are raw bytes[2,1,0,3].
Native Vulkan depth-aspect buffer copies put the D24 integer in low24 bits and
leave the upper8 bits unspecified. Stencil is copied separately as one byte
per pixel. Therefore this producer writes the exact integer components
`[(N >> 8) & 255, N & 255, stencil, (N >> 16) & 255]`. It preserves native
row order and never samples or converts float depth. The Vulkan plane-copy
rules are specified in the [Khronos copy commands specification](https://docs.vulkan.org/spec/latest/chapters/copies.html).

The source evidence remains the unchanged `assets/disc/default.xbe` and
`analysis/multiplayer-shading/weather-plate-original-reference/event-215778`.
The existing original reference was captured with xemu software OpenGL. This
work proves the exact captured byte relation and native Vulkan operation; it
does not claim physical Xbox/NV2A precision, live source frame history,
weather admission/simulation/loading, or a new screenshot match to hardware.

## Producer and ownership contract

`include/halo2/source_weather_depth_alias.hpp` describes the public API. The
context needs an actual Vulkan 1.1 device and a graphics+compute queue family.
The producer checks actual format features, image-format support, compute
limits and buffer bounds. Its compute shader uses only the SPIR-V `Shader`
capability; it needs no optional enabled device feature. The reused GPU test
fixture independently enables/requires depthClamp for its existing graphics
tests, which is not a producer requirement.

The source is an initialized, exclusive, ordinary optimal 2D D24S8 image with
one mip, one array layer, one sample, depth/stencil attachment usage and
transfer-source usage. Sparse/protected/disjoint and other flagged creation
paths are rejected. Source layout, actual ownership and image creation facts
are explicit caller inputs. Opaque Vulkan handles cannot reflect these facts;
callers must supply truthful metadata and genuine live handles.

`prepare(extent)` checks bounds before mutation and allocates resources
transactionally. The stencil SSBO is rounded up to complete uint words,
initialized before the copy, and read only within its allocated range.
The alias is a mutable RGBA8 image with an integer storage view and a UNORM
sampled view. Each view has its actual usage restricted explicitly. The
producer owns its nearest min/mag/mip, clamp-to-edge, single-mip sampler.

`record()` must run outside a render pass with exclusive access to source,
alias and command resources on the declared same device/family. It requires
a caller-owned reset/unsignaled fence. Every successful record must be
submitted with that fence, covering the producer and **all alias consumers**;
it cannot be submitted again. Recovery is permitted only for the exact
never-submitted command after a successful reset, through the subsequently
added `discard_unsubmitted()` API. It restores the previous layout history;
opaque handles cannot verify the caller's submission/reset facts. The native
frame guard and a focused GPU test exercise that path. Keep the source and fence alive.
No source/alias access, queue-family ownership transfer, concurrent producer
call, or fence reset/reuse is allowed until completion and successful
`retire(fence)`. The destructor waits for an outstanding fence, so an
unsubmitted or destroyed fence violates the contract.

The producer records explicit buffer/image barriers, restores the source's
supplied layout, and leaves the alias in shader-read-only layout for fragment
and compute consumers. Supported source layouts are depth/stencil attachment,
depth/stencil read-only, general, transfer source and transfer destination.
An outstanding submission blocks prepare/re-record; retirement rejects a
different or unsignaled fence. Unsupported inputs are rejected before any
command or retained binding is changed.

## Recorded validation

Standalone normal and ASAN address/undefined-behavior runs passed on NVIDIA
GeForce RTX 5080 with the actual Khronos SDK layer 1.4.363 and synchronization
validation enabled. The fixture uploaded every original D24 integer and
stencil byte, then required every generated alias byte to equal captured
stage0. It checked original depth/stencil before and after, inverted current
inputs followed by original restoration, retained alias bindings, all five
source layouts, 1x1 zero/max controls, 3x5 and 17x17 tails, all 256 values in
every alias channel, and GPU sampling through the owned nearest/clamp view.
There were 28 rejected admission controls and four rejected pending ownership
controls. Validation recorded zero errors.

The separate composition fixture uploads stages1..3 only. Stage0 is the
producer's live output from the actual D24 image, recorded before the plate
render pass in the same command submission. A single caller fence covers
producer, plate consumer and readbacks. Normal and ASAN builds both preserve
all 307,200 depth/stencil/target-alpha pixels and produce the exact previously
proved native scene, including reuse. Normal composition links the frozen
weather graphics archive; ASAN compiles the unchanged plate module so that
module is instrumented too. The frozen cache archive is not instrumented.

The generated alias SHA256 is
`a2c9d489cebf941c2d2748f3716c7d0d3f7f3ffb147441c8e26ed77679a06495`,
identical to original stage0. The composed scene SHA256 is
`535179c90f409716be4f27f9e3953f5e9f1196c83f7a08f5a3a0bb2a0552c86f`,
identical to the previous isolated native plate scene. That scene retains the
existing original/native boundary: maximum RGB channel difference 1,
14,477 differing pixels, and 333 original-only response pixels. Exact current
alias production does not erase that separate raster/plate comparison limit.

CPU ASAN/UB runs passed with leak checking enabled. Graphics ASAN/address/UB
runs passed with `detect_leaks=0`, without leak suppressions. Full graphics
LSAN is **not clean**: the producer fixture reported 3295 bytes in 29
allocations; its empty context reported 3236 bytes in 29 allocations. Every
allocation group/stack is external libdbus or unloaded graphics modules. All
59 bytes of the difference are in the observed `_dbus_string_copy_len`
allocation; no depth-alias source stack appears. The previous plate fixture's
historical external total was 3156 bytes in 29 allocations. These current
totals are not claimed equal to each other or to that historical baseline.

Proof artifacts live in `analysis/multiplayer-shading/weather-depth-alias-prototype`
and `analysis/multiplayer-shading/weather-depth-alias-composition`. The former
contains the final `validation.json`, a 63-file immutable input snapshot,
normal/sanitizer logs, build/shader hashes and original alias bytes. The
snapshot is checked again after final composition validation. Original
reference files, previous plate outputs and their manifests remain unchanged.

A separate, additive `cmake/SourceWeatherDepthAlias.cmake` registers the
producer and composition targets without changing the frozen weather graphics
module. The root-owned private weather pipeline CMake build passed all 14
registered CPU/GPU checks, including its optimized alias-to-plate composition,
under SDK synchronization validation. Its record is
`analysis/multiplayer-shading/weather-pipeline-cmake-integration/ctest.log`.
The full project was configured only; these checks do not establish a full
native application build or activate a live frame path. This integration
record is separate from the producer's normal/ASAN proof above.

## Reproduction

Build only the private prototypes:

```sh
bash tools/build_source_weather_depth_alias_prototype.sh \
  analysis/multiplayer-shading/weather-depth-alias-prototype/build normal
bash tools/build_source_weather_depth_alias_prototype.sh \
  analysis/multiplayer-shading/weather-depth-alias-prototype/build-asan asan
bash tools/build_source_weather_depth_alias_composition.sh \
  analysis/multiplayer-shading/weather-depth-alias-composition/build normal
bash tools/build_source_weather_depth_alias_composition.sh \
  analysis/multiplayer-shading/weather-depth-alias-composition/build-asan asan
python3 tools/freeze_source_weather_depth_alias_proof.py before
```

Each executable runs CPU capture/guard checks with no arguments, the complete
native fixture with `--gpu`, and an empty graphics context with
`--gpu-baseline`. GPU runs need the actual validation layer environment:

```sh
export VK_LAYER_PATH="$PWD/.tools/validation/usr/share/vulkan/explicit_layer.d"
export LD_LIBRARY_PATH="$PWD/.tools/validation/usr/lib"
export VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
```

Use CPU sanitizer options `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`. Use `detect_leaks=0` only for the explicitly
separate graphics address/UB check. Capture the normal GPU,
ASAN address/UB GPU and ASAN CPU logs with the names used in `validation.json`;
retain the full LSAN and empty-context controls separately. Then collect and
verify the stable input/artifact proof with
`python3 tools/freeze_source_weather_depth_alias_proof.py after`.
