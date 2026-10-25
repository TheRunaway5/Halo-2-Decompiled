# Water and ocean TU1.5 validation

The Vulkan water paths execute the authored NV2A vertex microcode and pixel
combiner definitions through `world_vertex.glsl`, `vertex_program.glsl`,
`world.frag`, `texture_program.glsl` and `combiner.glsl`. Infinite ocean grids,
pools, shore edges and transparent waves retain their separate authored passes.

## Source authority and correction

The authenticated English TU1.5 executable has SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The complete water setup bodies at `3C640`, `3CFF0`, `3CA80`, `3C990`,
`30BD0` and `1349E0` match the corresponding retail bodies after explicit
address normalization: 724 instructions, with all 35 immutable paired
operands equal. See
[the source body audit](../analysis/water-tu15-20261024/source-body-audit.json).

Fresh execution of TU1.5 exposed a bit-level error in the C++ state producer:
unary negation and a negative scale produced negative-zero words where the
original performs `0 - value` after multiplication. `water.cpp` now retains
the original addition, subtraction and multiplication order for c22.y and
the three bump matrices. The zero-heading regression in `water_tests.cpp`
checks the literal source words, including zero signs.

`tools/oracle_water_state_tu15.py` runs the unchanged original water routines
against a disposable build of the current state producer. Its 768 fixtures
cover 12 original multiplayer water records, 480 explicit camera-context cases
and 96 original widescreen suppressions. The helper records source and tool
hashes and distinguishes numeric equality from bit equality. GPU setter calls
are intercepted; this check does not execute original GPU programs.

The [current exact comparison](../analysis/water-tu15-20261024/state-original-v6-exact.json)
passes all 46,848 component words. The
[unchanged prior-source control](../analysis/water-tu15-20261024/state-original-v5-before-exact.json)
fails the same command with 1,136 zero-sign differences, proving that numerical
equality alone did not catch the regression.

TU1.5 suppresses its 1.5 aspect factor for an active context with byte+1 set,
short+2 different from -1 and type+8 equal to 3. `water_frame` takes the
effective anamorphic flag. Ordinary campaign and multiplayer callers use
types 1 and 2. A caller for context type 3 must retain that original gate.

## Repeatable checks and remaining evidence

- `retail_multiplayer_water`: 21-map authored record/geometry checks and the
  literal TU1.5 zero-word regression.
- `original_tu15_water_materials`: actual Zanzibar, Delta Towers and Uprising
  water/ocean material definitions. It compares all 60 original definition
  words and class-zero vertex microcode directly with raw source tag bytes,
  then checks program preservation, samplers and external target bindings.
  The [actual cohort](../analysis/sky-water-tu15-20261024/water-material-owner-rebinding.json)
  passes 13 materials: three infinite oceans, five pools, one edge and four
  waves across five template families. Binding is selected transactionally
  through the actual pass/vertex family, including the infinite `water_static`
  template; material names do not choose the water owner. Supply
  `HALO2_TU15_GAMEPLAY_XBE` to run this check.
- `native_texture_addressing`: shared Vulkan dependent-lookup checks, including
  signed water bump channels, sign-boundary interpolation, matrix layout and
  homogeneous-coordinate behavior.
- `original_water_copy_host_gpu`: production four-tap 512x256 copy shader,
  24 original rectangle cases and declared host source images.

Run CMake checks through `tools/run_tests.py` with their explicit targets and
filters. Run the original-state oracle through `tools/run_test.py`, with
Capstone and Unicorn available in the selected Python environment. The
validation reports are under `analysis/water-tu15-20261024/`; owned scratch
and build outputs are removed by the wrappers. No binary or raw image is
retained by this investigation.

Exact original NV2A/Vulkan water pixels remain an acceptance requirement.
These checks establish original state, program/resource identities and
bounded host GPU behavior. They do not establish the complete live original
pass chronology, original texture filtering/rasterization, or framebuffer
equality with TU1.5 hardware.
