# Verified scene-copy destinations and water tap delivery

The native scene-copy geometry now matches888 original emitted attributes and
all60 program words across37 complete calls using the captured ready
target19/21/22 resources. The water helper and renderer now preserve the
quarter-destination-pixel spacing scaled through each camera's normalized
source rectangle. All96 projected tap vectors from24 original512×256 calls
match bit-for-bit. This fixes the previously recorded water tap mismatch.
[Original destination evidence](MP_SCENE_COPY_DESTINATIONS_SOURCE_RESEARCH.md),
[native comparison](../tests/scene_copy_destination_tests.cpp),
[water helper](../src/water.cpp),
[renderer delivery](../src/renderer.cpp)

Frozen run`verify-20261012T010411.708711Z` rebuilds656 objects across the CPU
tests, production shader test and renderer target. All four focused CTests
pass. Fresh unchanged original execution reproduces the fixture and report
byte-for-byte. The complete scoped ASan/UBSan destination test passes, with
the shared geometry owner and whole water owner instrumented; remaining
library objects retain their ordinary build. All nine recorded commands
return0, and their logs, binaries and captures were independently hash checked.
[Verification driver](../tools/verify_scene_copy_destinations.py),
[manifest](../analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verification.json),
[focused test log](../analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verify-20261012T010411.708711Z/ctest.log),
[sanitizer result](../analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verify-20261012T010411.708711Z/asan-test.log)

The production`water_copy.frag` shader executes24 offscreen calls using12
original camera rectangles at each declared host source extent640×480 and
1280×720. The test compares12,582,912 RGBA channels against bilinear filtering
with offsets independently projected from the original attribute outputs.
The maximum error is1.13327 byte levels, within the declared two-byte host
filtering/readback tolerance. The previous host-source-extent taps differ from
the corrected reference by more than three bytes in6,008,394 channels, so the
input images distinguish the actual spacing change. The Vulkan validation
layer is forced for this run; its complete test output contains no validation
errors. Fifty source-image/uniform/output files are pinned in the manifest.
[Production-shader test and declared inputs](../tests/water_copy_gpu_tests.cpp),
[complete CTest detail](../analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verify-20261012T010411.708711Z/build/Testing/Temporary/LastTest.log),
[capture hashes](../analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verification.json)

The existing21-map water-content check also passes:12 authored water records,
five resident grids, three shader-bearing infinite grids and nine resident pool
shader records. These are content/program checks, not rendered water matches.
The renderer compiles with exactly the owned tap-delivery hunk overlaid on
the verified compatible source base; unrelated concurrent renderer/CMake
changes are excluded and recorded as checkout drift.
[Water-content test](../tests/water_tests.cpp),
[exact renderer overlay and frozen source pins](../analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verification.json)

The first verifier invocation stopped after the passing tests because its
summary parser required the older CTest wording. The resumed verifier accepts
both summary formats, preserves every source/build/test output, and performs
the remaining original/sanitizer checks. Its original driver, parser-stop
record and corrected resume driver remain pinned separately. No passing build
or test was rerun for this parser correction.
[Preserved parser stop](../analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verify-20261012T010411.708711Z/initial-parser-stop.json),
[resumed driver](../analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verify-20261012T010411.708711Z/resume-driver.py)

The verification manifest SHA-256 is
`03aece3f426ca8bfa7a60dcf322b01dd8212a86d400a0fff0e00f841f5879426`.
Production code, comparisons and original fixtures have no owned overlay
drift; the intentional verifier parser amendment is recorded. Whole-owner
static correspondence through Xbox TU1.5 separately reproduces all64 original
report/listing outputs byte-for-byte.
[Xbox release correspondence](MP_SCENE_COPY_TITLE_UPDATE_RESEARCH.md)

The host test retains the shader's existing Vulkan pixel centers and uses
declared high-frequency source images. It does not compare original GPU pixels,
recover original dirty-surface contents, establish normal water/camouflage
scheduling, or exercise the complete renderer's water flow. Generic target19
GPU resources and camouflage layer19 depth/color alias writes remain open.
No game-mode launch gate or variant admission changes, and the all39-variant
goal remains incomplete.
[Original GPU evidence boundary](MP_SCENE_COPY_GPU_ORACLE_FEASIBILITY.md),
[camouflage alias boundary](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md),
[full goal](GAME_MODE_PROGRESS.md)
