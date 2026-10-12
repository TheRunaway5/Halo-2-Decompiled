# Native original scene-copy geometry

The native four-tap scene-copy owner reproduces the CPU geometry and pixel
program emitted by retail `352E0(NONE,1) → 34A90 → 34420 → 34100/350E0`.
All 840 successful attributes across the captured original call and the separate
34-case rectangle corpus match bit-for-bit; all 60 pixel-program words match
in every call. This is CPU preparation, with no additional gamemode admission.
A fresh frozen build passes all four focused CTests and the complete scoped
ASan/UBSan comparison. Both original oracles reproduce all 33 outputs byte-for-byte.
[Native interface](../include/halo2/source_scene_copy.hpp),
[implementation](../src/source_scene_copy.cpp),
[complete comparisons](../tests/source_scene_copy_tests.cpp),
[verification driver](../tools/verify_scene_copy_geometry.py)

Verification run `verify-20261012T004707.124351Z` rebuilt 257 objects across four
targets; all ten recorded commands returned0. The manifest pins frozen source,
original reports, owned assets, captured RAM/export metadata, logs and binaries.
It records concurrent checkout drift and overlays only the owned geometry/test
changes and exact CMake declarations on the prior verified fog source. New
owned files have no drift. Sanitizers cover the complete new geometry owner,
vertex-program decoder/evaluator and all35-call comparisons; other library
objects are built normally.
[Verification manifest](../analysis/multiplayer/game-variants/scene-copy-geometry-native-20261012/verification.json),
[focused test log](../analysis/multiplayer/game-variants/scene-copy-geometry-native-20261012/verify-20261012T004707.124351Z/ctest.log),
[complete sanitizer output](../analysis/multiplayer/game-variants/scene-copy-geometry-native-20261012/verify-20261012T004707.124351Z/asan-test.log),
[prior verified fog base](MP_SHADER_FOG_NATIVE_BINDING.md)

`SourceSceneCopyContext` supplies the world camera's signed rectangle shorts
in `(top,left,bottom,right)` order, actual destination dimensions, and explicit
near/far planes. `source_scene_copy_frame` returns the original quad order,
position XYZW, four source-pixel texture coordinate vectors, corner attribute5,
and literal mode1 pixel program. It requests no resource allocation, readiness,
source binding, target selection, device submission or implicit camera input.
[Whole captured source call](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md),
[rectangle/source ownership](MP_SCENE_COPY_RECTANGLES_SOURCE_RESEARCH.md)

The implementation preserves the separately rounded `1/destination` followed
by `span*reciprocal`. Each tap preserves `amplitude*sign`, destination-coordinate
addition, ratio multiplication, and source-origin addition in the original
order. This matters for the odd 321×241 rectangle and offset rectangles. The
tap is a quarter destination pixel transformed into source pixels; it is not
a quarter source pixel. Texture XY remains unnormalized, and Z/W remains0/1.
Positions preserve the original 24-bit depth projection and clamp operations.
[Original instructions and raw context/attribute words](../analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json),
[unchanged original fixture](../tests/fixtures/scene_copy_geometry_original.json)

The comparison covers full, half, quarter, offset, odd-size and single-pixel
source rectangles. One rectangle context is captured; the other33 are declared
caller inputs, with genuine resources and device capacity retained. Raster-only
changes are originally bit-identical to the baseline. Captured source640×480,
destination320×240 and near1/64/far1024 are the executed resource/depth bounds.
Other destination sizes, live split-screen producers, title-update correspondence,
copy flags, stage counts and radial weights have no new execution claim here.
[Original declarations and limits](MP_SCENE_COPY_RECTANGLES_SOURCE_RESEARCH.md)

The native vertex-program evaluator separately checks the actual retained
seven-MOV screen program against each native vertex: position, four texture
coordinates, and corner/color routing. Attribute6 is not emitted by the
original callback. The evaluator test supplies a declared retained value to
check its MOV; it makes no claim about the original device's attribute6 default.
[Actual original program bytes](../analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json),
[native decoder/evaluator](../src/vertex_program.cpp)

Finite arithmetic requires nearest-even rounding, masked FP exceptions and
gradual binary32 underflow. Every arithmetic operation is separately rounded,
with contraction/fast-math disabled. Unsupported host controls, nonfinite
inputs/results and nonpositive destination dimensions fail explicitly. Tests
exercise these admission limits; they do not establish original exception
status delivery or other floating-point modes.
[Strict source compilation and test declarations](../CMakeLists.txt),
[host/input boundary checks](../tests/source_scene_copy_tests.cpp)

This owner is independent of the water-specific renderer. Its existing copy
helper/shader/runtime path is unchanged in this milestone, including the known
quarter-source tap mismatch. Native source/target image production, normalized
sampling, GPU raster centers, filtering/rounding, original framebuffer comparison,
normal camouflage scheduling, geometry cloning and layer19 depth/color alias
remain open. The actual SDK `.53125` viewport offset and separate emulator
screen conversion are recorded source facts; neither is used as a guessed
pixel-center correction in this CPU owner.
[Preserved water mismatch evidence](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md),
[viewport and secondary emulator boundaries](MP_SCENE_COPY_RECTANGLES_SOURCE_RESEARCH.md),
[depth/color alias boundary](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md),
[full goal and remaining scope](GAME_MODE_PROGRESS.md)

The later [destination/water binding](MP_SCENE_COPY_DESTINATIONS_NATIVE_BINDING.md)
extends complete original execution to the captured ready512×256 targets21/22,
fixes the water-specific normalized tap spacing and checks the production shader
on declared host images. Its evidence and remaining GPU/normal-flow limits are
recorded separately from this earlier frozen geometry milestone.
