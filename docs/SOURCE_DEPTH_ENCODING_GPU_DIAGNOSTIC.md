# Isolated D24 depth encoding GPU diagnostic

The extracted pre-correction native integer-W expression stores `N+1` for
6,291,455 of all 16,777,216 tested integer inputs on the NVIDIA RTX 5080.
The pinned source expression stores every integer `N` exactly. These are
actual single-sample D24S8 attachment readbacks from an isolated fragment
test, with a separate R32_UINT attachment recording the emitted float bits.
They are not a replay of title interpolation or the complete production
fragment shader.

The native input is preserved in
`analysis/multiplayer-shading/depth-encoding-gpu-prototype/historical-world.frag`,
SHA256 `1ebc929ea98de5fcabed473b43bd87406db5d282249292871f870eb0ff23b0eb`.
It exactly matches the original diagnostic input records and the parent's
frozen pre-correction shader. Later production corrections do not alter these
measurements. The old extraction/build tools are retained under the same
directory's `history/`; the current reproduction tools accept the explicit
historical input.

The actual tested native statement retains the runtime integer-W branch:

```glsl
gl_FragDepth = titleDepth.y!=0.0 ?
    floor(clamp(material.sourceCameraControl.x!=0u ? titleDepth.x :
        titleDepth.x*16777215.0,0.0,16777215.0))/16777215.0 : gl_FragCoord.z;
```

The pinned xemu `glsl/psh.c` at commit
`fc24584ce88f0915ad7f04775bb7712c2e3f49ee` and captured original20 event8878
fragment module independently contain:

```glsl
gl_FragDepth = uintBitsToFloat(floatBitsToUint(floor(zvalue) / 16777216.0) + 1u);
```

The preparation tool checks both authorities before compiling. The isolated
fragment receives `sourceCameraControl.x=1` and the binary32 `titleDepth.y=1`
as runtime push constants. It applies the same explicit finite clamp to both
test inputs before evaluating the encoding. The source full fragment's
admission, W reconstruction, window-region gate and clip-range handling are
outside this diagnostic; in particular N=0 and negative clamp controls do not
establish original draw admission.

## Measured output

Both normal and address/undefined-behavior sanitizer runs used 4096×4096
single-sample D24S8, actual Khronos SDK layer1.4.363 and synchronization
validation. Their complete measurements and all four binary outputs are
byte-identical, with zero SDK errors. The application copies the depth aspect
as one DWORD per pixel and compares only its low24 bits; the upper8 padding
is ignored. Stencil is copied independently as one byte per pixel.

| Encoding | Integers tested | Stored N | Stored N+1 | Emitted float words differing from strict CPU expression |
| --- | ---: | ---: | ---: | ---: |
| Frozen native expression | 16,777,216 | 10,485,761 | 6,291,455 | 8,388,606 |
| Pinned source expression | 16,777,216 | 16,777,216 | 0 | 0 |

The float companion separates compiler arithmetic from D24 conversion. It
does not prove that every full production shader or another driver lowers
division identically. No reciprocal adjustment, tolerance or fitted output
is applied to either expression.

Thirty finite fractional/clamp inputs are measured separately, including
signed zero, the smallest subnormal, values beside1, values around2²³ and
the upper D24 limit, and finite float extremes. Eight controlled fragment
inputs additionally execute actual ALWAYS, NEVER, EQUAL and LEQUAL
attachment comparisons with depth writes disabled. EQUAL against a cleared
maximum depth passes input `N=0xFFFFFE` with the frozen native expression;
the source expression correctly keeps that input below the clear depth.
The response masks are retained without adjusting any comparator result.

Each comparator's depth readback is compared against a separate clear-only
attachment reference. Native initialized stencil0x5A and target alpha127
remain unchanged; RGB is either the controlled old color or the exact
response color. Those values are test initialization, not original scene
history. CPU leak checks are enabled; GPU address/UB checks explicitly use
`detect_leaks=0`, so graphics leak cleanliness is not claimed.

## Reproduction and boundaries

New files are isolated tests, shaders and tools. There is no CMake
registration, live renderer activation, guest operation or mutation of the
historical production shader in this proof.

```sh
bash tools/build_source_depth_encoding_gpu.sh /absolute/private/build normal \
  "$PWD/analysis/multiplayer-shading/depth-encoding-gpu-prototype/historical-world.frag"
bash tools/build_source_depth_encoding_gpu.sh /absolute/private/build-asan asan \
  "$PWD/analysis/multiplayer-shading/depth-encoding-gpu-prototype/historical-world.frag"
export VK_LAYER_PATH="$PWD/.tools/validation/usr/share/vulkan/explicit_layer.d"
export LD_LIBRARY_PATH="$PWD/.tools/validation/usr/lib"
export VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
/absolute/private/build/source_depth_encoding_gpu_tests --gpu /absolute/private/normal
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  /absolute/private/build-asan/source_depth_encoding_gpu_tests --gpu /absolute/private/asan
```

The build runs CPU controls only by default. GPU execution requires a
coordinated hardware lease. The test validates physical format features,
image usage/sample support, framebuffer/viewport limits and resource sizes;
a capability-limited device uses exact power-of-two tiles over the same
integer domain. Render/copy barriers include color and both early/late
depth stages, followed by transfer-to-host visibility and a completed
submission fence before readback or reuse.

The frozen artifacts are
`analysis/multiplayer-shading/depth-encoding-gpu-prototype/report.json`,
`normal/measurements.tsv`, `asan/measurements.tsv` and the adjacent full
depth/float binary outputs. `tools/analyze_source_depth_encoding_gpu.py`
checks both runs, derives the stored-integer histogram and first/last
differences, and records input, executable, shader, validation-layer and
artifact hashes. This is host Vulkan/compiler evidence supported by the
pinned original emulator shader; physical Xbox precision, title W
interpolation, source draw admission and complete scene equality remain
outside its scope.
