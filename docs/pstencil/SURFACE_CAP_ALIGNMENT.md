# Campaign surface and shadow-cap alignment

The Prophet collar and hand showed false point-light self-shadows in the v4
opening. Actual GPU pixel histories identified stencil129 at dark dots and
stencil128 at neighboring lit pixels on the same surface triangles. A near cap
was one integer W-depth unit closer at the rejected samples.

The posed input positions were identical. The discrepancy was introduced by
the port's different transforms: the surface executes ordered DPH instructions,
MUL c10, RCC and MAD c11, while `pstencil/volume.vert` used a matrix multiply and
XY/W division. Large campaign translations expose their different rounding.
Snapping both results to 1/16-pixel coordinates alone did not fix this.

The volume now follows the surface program's operation order, including the
screen-Z conversion and W clamp. Stencil comparison, depth comparison, shadow
darkening and geometry remain unchanged. Original does not use this shader.
This aligns the port's passes; it does not certify NV2A arithmetic/rasterization
parity against a captured May 2003 framebuffer.

## Reproduction and verification

- Baseline actual capture:
  `analysis/pstencil/prophet-surface-v1/native-v2/surface_capture.rdc`, tick3404.
  The read-only replay matches its native screenshot exactly in RGB.
- Fixed actual capture:
  `analysis/pstencil/surface-alignment-native-v5/native-v1/surface_capture.rdc`,
  tick3405. Replay also matches its own native screenshot exactly in RGB.
  The two captures are adjacent ticks, not a same-frame before/after image pair.
- `tools/pstencil/check_surface_cap_alignment.py` checks the captured collar
  and hand. It finds caps by posed positions because animation can change
  preceding face counts. Baseline: five of six clip positions differ. Fixed:
  zero differ. At all four sampled light pixels, the fixed capture has
  stencil128 and passes the stencil test.
- The five exported base/environment/dependent draws contain 10,158 input
  vertices matching near-cap positions. Their fixed clip positions all agree.
  This is bounded draw coverage, not all campaign geometry.
- `tests/pstencil/gpu_tests.cpp` replays the actual posed collar/hand triangles,
  original surface instruction tokens through the production interpreter,
  captured camera rows, and actual point light. The original shader falsely
  shadows 171 of 5,722 collar pixels. The fix produces zero false shadows on
  those 5,722 collar pixels and 522 hand pixels. The 72 existing GPU cases also
  pass, including occlusion direction, two lights, inside-volume cameras,
  stipple, lightmap darkening, and exact Original color/depth/stencil restore.

The fast GPU regression is built with `cmake -S tests/pstencil -B BUILD_DIR`.
Run `BUILD_DIR/pstencil_gpu FRESH_EVIDENCE_DIR` with the Vulkan validation layer
available. Actual-frame export tools are read-only and run inside bundled
qrenderdoc; they do not replace captured shaders or state. RenderDoc's raw
pixel-history depth-failure flags require D24 quantization correction, so the
diagnosis uses actual stencil rejection and matched vertex/depth evidence.

The broad pStencil goal remains active. This fix does not close accumulated
normal response, animated material/light families, wider combined environment
materials, original Elite color recovery, all-face coverage, original GPU
frame comparison, or campaign pacing and final audio synchronization gates.
