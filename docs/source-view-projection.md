# Original render view and shader constants

`source_view_projection` implements the complete `2FD90` 0xC0-byte render
bank from actual camera position, forward/up, near/far, the optional oblique
near plane and the already produced `2FB70` projection program.
`source_view_shader_constants` preserves `12D50`'s ordered matrix products,
W normalization and optional first-person scale, followed by `12FA0`'s
literal c4/c5/c6 basis copies. It returns original shader constants directly.
No GLM perspective/lookAt matrix or Vulkan clip conversion enters this helper.

`source_view_shader_bank` supplies the complete twelve-vector submitting-view
bank. Its c7 uses the actual camera position read by `12FA0`, with the literal
`45E460` word `443FF400` (767.8125) in W. Existing `viewport_constants` supplies
c8..11 from the actual physical rectangle, render-scale controls and mode.
Both arithmetic units compile without floating-point contraction. The later
foreground replacement changes c0..3 through `12D50`; it retains c4..11.
The fixed shadow render target's 128x128 dimensions do not replace this main
camera bank. Vulkan target viewport conversion remains a separate consumer.

`tools/differential_source_view_shader_bank.py` runs untouched `12FA0` until
the upload boundary at `132B3`, and the complete `12D50` foreground replacement
with a private valid D3D push-buffer shell. All 722 cases and 34,656 output words
match exactly, including viewport sizes through 4K, split-screen offsets,
render-scale clamping and fixed mode2. All 321 executed instruction addresses
are checked against the original executable bytes; no arithmetic service is
replaced. The complete c0..11 bank also matches all 528 independently captured
GPU words across the eleven retained shadow draws. Normal and
ASan/UBSan/leak-enabled tests pass. This proves the supplied-input producer,
while live shadow selection and its complete caller chronology remain separate
integration requirements.

The render bank differs from the `441B0` clipping camera. `2FD90` computes
right from the supplied forward/up, computes perpendicular up from that raw
right and forward, and only then normalizes right/up/backward independently.
Its x87 length sums `(Z²+Y²)+X²`, retains binary64 precision for threshold
testing and stores length to binary32 before SSE reciprocal/products.
The existing `441B0` bank normalizes supplied up independently; it cannot
replace the render matrix for nonorthogonal or precision-sensitive inputs.

The exact `2FB70` horizontal/vertical scales and center offsets are retained.
`2FD90` also computes the original oblique clipping coefficients when the
camera's near distance is zero. The routine's x87 depth denominator and SSE
coefficient/offset products preserve their distinct precision and order.

The W normalization input has an explicit source producer. `2BA10` copies
the full 0x74-byte submitting camera to render-state +0x18. The literal
camera far field at +0x44 therefore occupies state +0x5C. `132D0` copies
0xA6 dwords of that state to global 0x485600, producing 0x48565C before
`12D50`. Sixty-four untouched `132D0` prefix fixtures verify this copy.
`12D50` multiplies ordered view/projection products by
`16777215 / actual48565C`; its BL nonzero branch subsequently multiplies all
16 c0..3 words by the original float 0.15. c4..6 are unaffected.

The independent untouched instruction differential passes 8,226 cases and
625,176 output words, including both retained source cameras, world/FP flags,
axis/signed-zero and nonorthogonal inputs, viewport/widescreen combinations,
ordinary near clipping and oblique planes. The private graphics device and
push-buffer shell executes `3F6190` unchanged. There are no code, math,
reciprocal, camera-output or service hooks. The frozen production fixture
contains 466 cases; normal and ASAN/UBSAN/leak checks pass.

The original instruction and native helper outputs also match every c0..6
word in actual original capture20 world event7651 and hands event8854. The
hands bank includes original first-person 0.15 scaling. This pin checks the
observed camera bank; it does not imply all animation, object state or timing
is identical between earlier paused RAM and the later capture.

The helper does not assert whole-image or NV2A arithmetic parity. GPU
operation precision, clipping, rasterization, varying interpolation and the
original shader's oPos output remain separately checked boundaries. Original
input fields are explicit; callers must retain their real viewport/features
and submit the produced camera bank rather than deriving it again.

The separate authored W audit covers 2,974 admitted BSP/pass bindings across
all 21 caches and 18 unique original vertex programs. The nine unique layer3
lighting programs all retain raw homogeneous oPos.w from `DPH(position,c3)`
(dot XYZ plus c3.w). None of the 18 programs writes `RCC` to position W;
reciprocals go to temporary registers used to divide position XYZ. Actual
captured hands and PRT Vulkan vertex code preserves this W, converts target
XYZ to normalized coordinates, then multiplies XYZ by raw W for `gl_Position`.
That supports the native raw-W reconstruction for these paths; it is not
evidence for inverting W or for retail hardware interpolation precision.

The expanded symbolic token census now covers all 7,605 cache-resident vertex
classifications across the 21 multiplayer maps: 341 unique programs, including
79 distinct classification0 programs. It propagates temporary-register,
swizzle, constant and output writes rather than only checking the final opcode.
Ordinary stencil/base, per-pixel/per-vertex/dynamic lighting, PRT, transparent,
fog and water/static paths produce `DPH(v0,c3)` with no reciprocal dependency
in final W. Water pool/edge paths use `DP4(v0,c3)`; the native ordinary stream
supplies v0.w=1, giving the same homogeneous expression. Synthetic screen,
bloom and shadow-generation programs retain different W producers, and effect
or stencil-shadow paths can deform positions before their final c3 projection.
The ordinary conclusion must not be extended to every resident program.

The captured original20 PRT fragment shader explicitly reconstructs triangle
barycentrics from flat vertex positions, divides each weight by that vertex's
raw W, normalizes them, then computes `w0 + bc1*(w1-w0) + bc2*(w2-w0)`.
It applies positive-depth offset/slope, floors the result, divides by 2^24 and
increments the resulting floating-point bit pattern before `gl_FragDepth`.
This establishes the observed Xemu Vulkan backend's W-depth interpretation;
its precise interpolation and quantizer are not evidence of identical retail
hardware raster arithmetic. A smooth Vulkan varying can express equivalent
interpolation with a different rounding sequence. Depth-mode selection must
use a uniform or flat value, because interpolated float mode values need not
remain integer-exact.

Evidence:

- `analysis/multiplayer-shading/source-view-projection-original.py/.json`
- `analysis/multiplayer-shading/source-view-projection-original.log`
- `tests/fixtures/retail_source_view_projection.tsv` and provenance
- `analysis/multiplayer-shading/source-view-projection-ctest.log`
- `analysis/multiplayer-shading/source-view-projection-asan-ctest.log`
- `analysis/multiplayer-shading/world-w-program-audit.json`
- `analysis/multiplayer-shading/all-w-program-audit.json`
- `analysis/multiplayer-shading/original-xemu/prt-output-audit/original20-7641-fragment.spvasm`
- `analysis/multiplayer-shading/original-xemu/prt-output-audit/zanzibar20-camera-original-proof.json`
