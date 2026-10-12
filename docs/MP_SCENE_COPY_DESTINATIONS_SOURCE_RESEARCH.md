# Original scene-copy destinations and water offsets

The unchanged retail `352E0` mode1 copy completes 37 fresh calls over the
captured weather-entry state. The calls retain the real source target0,
640×480 source image, device, programs, render-target records and available
command capacity. Target19 is320×240; targets21 and22 are512×256. This extends
the earlier rectangle evidence to the actual ready water targets without
inventing resource dimensions or setting readiness fields.
[Original destination oracle](../tools/oracle_scene_copy_destinations.py),
[complete calls, resource records and instruction bytes](../analysis/multiplayer/game-variants/scene-copy-destinations-20261012/report.json),
[original captured call](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md),
[camera and raster ownership](MP_SCENE_COPY_RECTANGLES_SOURCE_RESEARCH.md)

The inventory contains one fresh call with external destination`NONE`, plus
12 camera rectangles for each explicit destination19,21,22. The twelve camera
inputs comprise the captured full rectangle and the eleven previously declared
camera-only rectangle shapes. Only the fresh external destination argument
and declared camera's eight bytes vary. The raster rectangle remains full,
the current source target remains0, and each selected target's complete152-byte
record is retained and recorded before/after. All37 calls return normally,
with2,733 distinct original instructions byte checked. No instructions,
function returns, readiness flags or target records are supplied as substitutes.
[Declared inputs and successful attributes](../tests/fixtures/scene_copy_destinations_original.json),
[whole calls and actual before/after records](../analysis/multiplayer/game-variants/scene-copy-destinations-20261012/report.json)

The original table owner indexes`4B4B58 + index*98`. Its readiness word is
at+88, unavailable byte at+95, positive surface field at+78, and dimensions
at+80/+84. The three actual selected records are:

| Target | Record | Width×height | Captured physical backing |
| --- | --- | --- | --- |
| 19 | `4B56A0` | 320×240 | `02B48000` |
| 21 | `4B57D0` | 512×256 | `02B1B000` |
| 22 | `4B5868` | 512×256 | `02B9B000` |

Each backing's captured byte count and SHA-256 are recorded. Those RAM hashes
do not establish the content sampled by a GPU-resident dirty surface, or even
that a fresh copy has previously populated a target.
[Literal records and captured backing hashes](../analysis/multiplayer/game-variants/scene-copy-destinations-20261012/report.json),
[GPU resource and completion boundary](MP_SCENE_COPY_GPU_ORACLE_FEASIBILITY.md)

Original `34100` offsets each tap by one quarter destination pixel, multiplied
by the source-camera span divided by the destination size. A full640×480 camera
copied to512×256 therefore has first tap`(-0.3125,-0.46875)` in source pixels.
The 320×240 target has first tap`(-0.5,-0.5)`. A half-width camera halves only
the X offsets; a half-height camera halves only Y. This follows the original
attribute output, independently of a host pixel-center convention.
[Original first-corner vectors and full emitted geometry](../tests/fixtures/scene_copy_destinations_original.json),
[native original geometry owner](../src/source_scene_copy.cpp)

The water helper's host interface now takes the camera rectangle's normalized
source spans. Its offsets are`±0.25*normalizedWidth/512` and
`±0.25*normalizedHeight/256`. The renderer supplies the same normalized span
already used by the copy's source rectangle, including full-surface viewport
truncation. The copy pixel program delegates to the shared original program.
Only tap spacing and its shared program owner change; original GPU filtering,
raster centers, color quantization and normal water flow still require their
own evidence.
[Water helper](../include/halo2/water.hpp),
[implementation](../src/water.cpp),
[renderer uniform delivery](../src/renderer.cpp),
[complete destination/native comparisons](../tests/scene_copy_destination_tests.cpp)

The complete native destination comparison checks888 emitted attributes and
all60 program words in each of37 calls. It separately projects actual original
first-corner coordinates into the host's normalized relative-offset interface:
all96 tap vectors from the24 genuine512×256 calls match bit-for-bit. That
normalization is a declared host projection, not an extra original callback
operation or GPU output claim.
[Comparison and boundary](../tests/scene_copy_destination_tests.cpp)

The original fixture SHA-256 is
`045caec58a216af1f14cd8d94dd4aa40419801f4d7ff9e2ee9295f5d9a0c8890`;
the original report SHA-256 is
`45c1f12bf8fb0c1e693fdfe130424a31a3f4ab11da025abd8e2b4b46ad8dda78`.
The native verifier freezes compatible source, rebuilds the renderer, compares
original outputs, checks the actual production water-copy shader on declared
host inputs, and instruments the geometry/water CPU owners with ASan/UBSan.
Its result is recorded separately from this original-source corpus.
[Verification driver](../tools/verify_scene_copy_destinations.py),
[host shader comparison](../tests/water_copy_gpu_tests.cpp),
[verified native result](MP_SCENE_COPY_DESTINATIONS_NATIVE_BINDING.md),
[full game-mode goal](GAME_MODE_PROGRESS.md)

The title-update comparison separately checks whole-owner code correspondence
for these copy routines. Retail captured runtime banks do not become TU1.5
runtime or GPU evidence through that static correspondence.
[Xbox release correspondence](MP_SCENE_COPY_TITLE_UPDATE_RESEARCH.md)
