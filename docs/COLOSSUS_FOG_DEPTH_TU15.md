# Colossus ceiling fog depth correction

The native direct patchy-fog pass previously shaded nearby ceiling panels that
OG Xbox TU1.5 rejects with its fog quad's depth test. The pass now retains the
quantized range's start plane and performs the original non-writing integer-W
`LESSEQUAL` comparison against the sampled scene depth. Fog beyond that plane
continues to render. This fixes a confirmed appearance difference; the reported
temporal shadow fade has not been reproduced.

## Original evidence

The reference executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`
(owned OG Xbox TU1.5). Original captures used xemu OpenGL/llvmpipe; they are not
physical Xbox captures. Guest instructions and memory were not patched.

In `analysis/colossus-lighting-20261014/ceiling-gpu-original-v1/vehicle_capture_3.rdc`,
event 78251 is the direct patchy-fog quad. Its original state enables depth,
uses `LESSEQUAL`, disables depth writes, and supplies W `655359.9375` at every
corner. Pixel history at raw framebuffer `(420,374)` reports depth-test failure
and preserves RGBA `(23,24,17,83)`. The native matched view previously applied
its fog pass at the corresponding ceiling pixel, changing `(22,25,17,85)` to
`(37,33,21,75)`.

TU1.5 `3A5B0` returns the quantized range start to `27500`; its complete output
tail disables Z when the near plane reaches that start. `27500 -> 350D0`
produces the quad's W. The unchanged functions independently produce distance
40 and W `655359.9375` for Colossus's 40..50 range. The older retail source's
corresponding entries are `3A5C0`, `27520`, and `350E0`.

The initial GPU captures in `vehicle_capture.rdc` and `vehicle_capture_2.rdc`
selected xemu's scanout context. Their gamma/compositor draws are not material
or fog-pass evidence. The third capture selects the observed main GPU context.

## Implementation

`bsp_patchy_fog_frame` retains the original rounded range start, including the
low-byte rescale, and converts it through the original W calculation. Zero
retains the disabled-depth and fallback branches. `BspPostprocess` uploads the
value through the existing unused `controls.w`; its uniform size is unchanged.
Per-view projection clip planes are forwarded to the producer. The production
fragment shader rejects depths below the integer fog plane before running its
combiners. Noise, material textures, fog colors and density are unchanged.

## Validation

- `tu15_bsp_patchy_depth`: 265 unchanged TU1.5 producer cases, including 209
  enabled-depth cases, 54 disabled-depth cases and two quantized fallbacks.
  Original code executes all range arithmetic, the output tail, the quad
  callback and W calculation. Only GPU/state submission services are supplied.
- `tu15_bsp_patchy_depth_gpu`: executes the production fragment shader over
  near, immediately-below, equal, immediately-above, far and zero depths,
  plus a disabled-depth control. The old shader fails the near-surface case;
  the corrected shader passes. This tests actual discard/coverage behavior.
- The fixed live native GPU capture supplies `controls.w=655359.9375` at its
  direct fog draw (`fixed-patchy-inputs-v1/event-1245/state.json`).
- At retained lift camera heights 3.608, 5.511 and 7.535, the ceiling diagnostic
  crop's RGB-channel MAE changes respectively from 5.971 to 1.745, 8.620 to
  3.267, and 3.927 to 2.888. See `fog-depth-comparison.json` and
  `fog-depth-before-after.png` in the analysis directory.
- Native validation captures cover 640x480, 1280x720, 1920x1080 and 3840x2160,
  plus two views at 1280x720 and four views at 1920x1080.

The screenshot comparisons restore camera and projection only. Player,
weapon, effect, tick and target history differ. Residual sampling/material
differences and full-frame equality remain open. Higher-resolution native
captures are validation checks, not proof of Xbox pixel parity at every
resolution.

## Delivery

The combined `build-multiplayer` runtime and both registered regression targets
build successfully. Both CTests pass with Vulkan validation enabled. The first
combined build encountered a concurrent AI/physics linkage change; regenerating
the build after that existing module was added resolved it without changing AI
or physics code for this fix.

Both installed launchers select `.local/colossus-fog-20261014/halo2_native`,
SHA-256 `f26f01c6ed59250be2138e049108f9575c755a71f3a0fd3b8bc92f9a0e52b74a`.
The packaged capture is byte-identical to the production/focused fixed capture.
File-open tracing confirms all 32 observed SPIR-V opens come from the package's
shader directory. The CRT shader matches the prior installation, and all seven
recorded split-screen source hashes match that installation. Prior launchers
are retained in the package's `prior-launchers` directory.

Launcher selection was verified with both `--launcher-info` commands. The
owned Xbox-reference screenshot uses the packaged runtime directly because
the MCC-oriented launcher explicitly rejects `--gameplay-xbe`; launcher content
selection and MCC readiness were not changed.
