# Water and ocean TU1.5 validation

The Vulkan water paths use ahead-of-time compiled native kernels. The offline
compiler lowers each complete authored vertex program, packed vertex declaration
and pixel-control identity into fixed GLSL arithmetic and texture operations,
then compiles and validates SPIR-V. Animated colors and camera constants remain
ordinary uniform data. Infinite ocean grids, pools, shore edges and transparent
waves retain their separate authored passes.

The source registry is `shaders/native-programs/world/programs.json`; normal
builds require no original executable or private map. `native_world_kernels`
builds its compiled catalogue and native shaders. Executable SPIR-V checks
reject reads of instruction banks, vertex-declaration selectors, instruction
counts and fragment control words. Runtime matching compares complete program
bytes, declaration, control words and extended-bank category. Water admission
rejects an absent kernel, and animation cannot silently change its identity.
Native material uploads contain no vertex instruction words. The separate
water-copy kernel is generated from all60 original copy-program words and
contains fixed operations too. These are native compiled kernels, not runtime
instruction interpreters. Original executable execution is an offline oracle.

## Broader renderer validation, 2026-10-26

The world, weather, shadow and water-camera migration is still undergoing
validation. The [source refresh](../analysis/native-renderer-all-20261025/selected9d-owned-source-overlay-v1.json)
applied the reviewed renderer changes to the qualified `9d531caa` source base;
it does not qualify or select a new executable. The new native build and full
map GPU checks remain pending. Original/prepared texture, lightmap and geometry
differences remain open in the
[paired BSP input ledger](../analysis/native-renderer-all-20261025/all-paired-bsp-input-domain-inventory-v2.json).
The historical isolated-sky results below do not close those full-scene gates.

## Historical native acceptance, 2026-10-25

The [integrated source validation](../analysis/sky-exact-20261025/integrated-source-validation/README.md)
passes all 23 focused CTests. Its complete inventory covers 86 map/source entries,
including all 43 stock prepared maps and both separately owned custom maps.
Across 81 sky-bearing entries it renders 201 BSP/sky contexts and 1,206 direction
cases. All 99 paired input records and 594 paired native-renderer pixel
directions are exact; all five paired source-authored absences agree.
The [acceptance receipt](../analysis/sky-exact-20261025/integrated-source-validation/acceptance.json)
records 1,333 admitted native material bindings, zero uploaded vertex instruction
words and three rejected program/fallback mutations. These results supersede
the earlier failing sweep retained below.

The finite geometry catalogue restores 79 authenticated input profiles and all
474 source LOD states. V7 supplies five unchanged native BC1/BC3 images for
eleven bitmap owners; full shader state, texture bytes/mips and sky settings
retain strict comparison gates. District uses authenticated original PC11081
geometry data with independently authenticated Xbox native materials, as
documented in the [separate authority proof](../analysis/sky-water-tu15-20261025/district-pc-geometry-projection/README.md).
No Vista shader is admitted, and runtime rendering uses compiled native kernels
and immutable source data without opening original reference maps.

The existing 46,848-word TU1.5 water-state result remains exact. The integrated
suite also passes the unchanged native water vertex modules, four-tap copy and
all 14 production fragment modules. The fragment arithmetic component compares
7,168 channels with a declared one-UNORM-byte bound; 6,937 are byte-exact. That
bounded numerical component is separate from the 594 exact sky image pairs.
The sky sweep compares original and prepared assets through the same native
renderer and is scoped to isolated backgrounds, with separate full-BSP
Elongation window controls. Physical Xbox/NV2A framebuffer, filtering and
precision parity remain unverified.

At that publication, the root launchers selected native runtime
`bin/engine-builds/d85a3da7eac74077a9c1cb87e501e266/halo2_native` with prepared
cache `assets/imported/mcc-sky-water-tu15-20261025-v7`.
The [guarded publication](../analysis/sky-exact-20261025/native-runtime-publication-v79.json)
is `published-and-proven`: all thirteen ordinary multiplayer cases and four
frontend screens qualify, normal admission readers pass, and the concurrent
published source revisions are preserved. The two-player/Coagulation defaults
and eleven-map cohort remain intact; qualification and inventory are immutable.
The [ordinary root-launcher proof](../analysis/sky-exact-20261025/normal-launcher-native-proof-v79.json)
observes the actual selected executable and V7/default arguments, completing
four native frames and eleven simulation ticks with two local players in a
joined session. All five publication files remain unchanged by that run.

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
  24 original rectangle cases and declared host source images. Its complete
  generated program manifest must equal the actual C++ water-copy owner.
- `native_world_vertex_lowering`, `native_world_fragment_lowering`,
  `native_world_kernel_catalog` and `native_water_copy_kernel`: source lowering,
  full program/declaration identities, compiled shader ABI and executable
  no-interpreter contracts. These are compiler checks, not Xbox pixel captures.
- `native_water_vertex_gpu`: the unchanged production native vertex modules
  execute on Vulkan, with a test-only geometry shader recording their outputs.
  The offline CPU arithmetic reference checks all13 original material programs
  in26 viewport cases; twelve materials have matching authored geometry and
  one uses explicitly declared host inputs. The first RTX5080 run compared
  13,312 finite scalars:12,306 were bit-exact, with zero signed-zero differences.
  Quantized source XY and W agree; source D24 Z differs by up to two units and
  texture coordinates by up to9.54e-7. This is a tolerant numerical component
  gate, not bit-exact NV2A, fragment/filtering or reflection-scene acceptance.
  Binding the wrong actual native vertex module fails the numerical check.

Run CMake checks through `tools/run_tests.py` with their explicit targets and
filters. Run the original-state oracle through `tools/run_test.py`, with
Capstone and Unicorn available in the selected Python environment. The
original-state validation reports are under `analysis/water-tu15-20261024/`;
their owned scratch and build outputs are removed by the wrappers. Those
original-state checks retain no binary or raw image. The native runtime and
rendered window references retained for delivery are recorded below.

Exact original NV2A/Vulkan water pixels remain an acceptance requirement.
These checks establish original state, program/resource identities and
bounded host GPU behavior. They do not establish the complete live original
pass chronology, original texture filtering/rasterization, or framebuffer
equality with TU1.5 hardware.

## Earlier native runtime delivery, 2026-10-25

Before the current sky correction, the root launchers selected native runtime
`bin/engine-builds/87ba9ab9a08d40d4a8f2a264eea7ac79/halo2_native` and prepared
cache `assets/imported/mcc-sky-water-tu15-20261024-v3`. Its thirteen requested
ordinary multiplayer smoke cases and independent frontend qualification pass.
Publication preserved the selected two-player preference and Coagulation
default. All five publication files passed their original identity guards;
the normal gameplay, frontend and cache admission readers accepted the result.
The candidate qualification remains immutable. See the
[publication receipt](../analysis/sky-water-tu15-20261025/native-runtime-publication.json).

The [ordinary launcher proof](../analysis/sky-water-tu15-20261025/normal-launcher-native-proof.json)
runs `./Halo2-Launcher --multiplayer --frames 4 --mute --viewport 640 480`.
It observes the launcher's actual native executable and argument prefix,
including the selected cache, Coagulation and two-player defaults. The native
run submits four frames and six simulation ticks with two local players and
a joined session; publication files remain unchanged.

The [earlier validation report](../analysis/sky-water-tu15-20261025/final-native-validation-v2/README.md)
records eighteen passing focused CTests, including the thirteen actual native
water vertex programs and their numerical GPU comparison. The remaining
`sky_all_maps_gpu` CTest is red. Its complete available-map sweep covers 83 map
inputs (31 original disc, 9 original map-pack and 43 prepared MCC), 79 sky-bearing
maps and 1,176 axis-direction cases. All 196 admitted sky configurations pass
their native program, repeat, transparent-queue independence and zero uploaded
instruction-word controls. Four original/prepared Elongation window controls
also pass. The [map/BSP/sky matrix](../analysis/sky-water-tu15-20261025/final-native-validation-v2/all-map-matrix.csv)
and [lossless full report](../analysis/sky-water-tu15-20261025/final-native-validation-v2/full-sky-report.json.xz)
retain every outcome; hashes and decompression verification are in the
[readiness report](../analysis/sky-water-tu15-20261025/final-native-validation-v2/delivery-readiness.json).

At that checkpoint, three prepared MCC sky materials
had no admitted forward owner (Lockout `mountains_background`, Uplift
`clouds_low`, and District `ec_sky_dome2`); 96/96 paired sky input records and
536/576 native pixel comparisons differed between original and prepared
MCC assets. Actual qualified-runtime Elongation
[positive](../analysis/sky-water-tu15-20261025/final-native-window-captures/elongation-window-positive-native.png)
and [negative](../analysis/sky-water-tu15-20261025/final-native-window-captures/elongation-window-negative-native.png)
window references were captured and inspected; their
[capture receipt](../analysis/sky-water-tu15-20261025/final-native-window-captures/report.json)
authenticates the executable, cache and two 640x480 images. These are native
regression references. The current native sky acceptance above closes those
imported input and image gaps; original NV2A framebuffer equality and physical
Xbox precision remain separate acceptance work.
