# Source weather plate typed preparation

The typed preparation overload derives its expected pixel program, projected
quad Z/W, and raster profile from the separately supplied original37A60 caller
input. It accepts only the source `AboveNearWDepth` profile, a positive finite
projected W, the complete60 matching words, and bit-identical projected Z/W at
all four vertices. The existing VS21, texture binding, clip, surface, finite
shader-output, and record viewport/scissor guards still apply before a retained
binding is released. A returned public caller-helper result is not accepted as
an admission token.

The new overload lives in `src/source_weather_plate_caller_render.cpp`; the
original renderer translation unit keeps its dependency on the rendering core
only. Its private expected-word seam is reachable by the captured legacy entry
point or the internally derived typed entry point. Legacy preparation still
admits only the retained event215778 complete definition. Existing standalone
legacy builds therefore need no pure-builder or caller-helper dependency.

The CPU helper, builder, shader sources, original fixtures, and original proof
manifests are frozen. Before editing the renderer header/implementation, their
exact prior bytes were archived beneath
`analysis/multiplayer-shading/weather-plate-typed-preparation/history/`.
`historical-input-locations.json` records how the unchanged old scene/caller
manifests resolve those historical renderer entries. Old manifest counts and
old measured results are not rewritten into current-source claims.

## Original authority and preserved boundaries

The retained original event215778 shader and four projected Z/W pairs are
reproduced from the explicit original37A60 input in
`weather-plate-typed-prepare-review/actual-source-caller-input21.bin`. Selection
and projection near/far are separately observed banks. Their equal values in
this particular source input do not establish a general equality constraint.
The original52-byte compositor's padding is ignored; its low byte, positive-zero
X/Z lanes, clamped alpha/tint, and owned mode/anchor/skip flags are enforced by
the frozen caller helper.

Two unchanged-original controls establish why packet-word equality alone is
insufficient. Setting selection-near to10 leaves the retained complete program
and Z/W unchanged but selects the source near/depth-disabled profile. Setting
projection-far to-1024 leaves the retained program unchanged but clamps W to
zero. Both are rejected before mutation. Near profiles and NoDraw have no
native draw admission through this overload. Resource bytes, matching opaque
samplers, color/depth pass state, synchronization, screen/UV/scroll/clip inputs,
and live source producer readiness remain explicit caller obligations.

## Verification contracts

`tools/build_source_weather_plate_typed.sh PRIVATE_BUILD COHERENT_CACHE_ARCHIVE
normal|asan` compiles the new typed overload, the frozen caller and program
helpers, and the new test. It compiles the unchanged plate shaders plus a new
test-only compute wrapper. It does not edit shared CMake or runtime files.

The test accepts `--cpu`, `--gpu-scene`, `--gpu-combiner`, or `--gpu`, optionally
followed by a private output directory. CPU mode verifies all2440 immutable
original3A5C0 cases, the1780 successful programs/1651 unique complete definitions,
and all14 unchanged whole37A60 cases. Source controls are experiments, not
captured original timelines.

Scene mode requires the typed retained-actual draw to reproduce the previous
native scene bytes exactly. It compares that retained scene with original
software-host readback under the already measured one-byte RGB residual, checks
all original D24 integers/stencil bytes and target alpha, and follows each
rejected prepare with direct record/readback without any valid prepare in
between. It also submits ten separately proved whole-caller above-near graphics
controls, including all eight reciprocal classes and independent bank changes.
Only the retained actual scene has an original raster comparison.

Combiner mode dispatches all1651 complete source definitions with32 explicit
RGBA-byte sample sets each (52832 outputs), including all16 recovered structural
shapes. The new wrapper includes the unchanged generic combiner and uses the
same separately expanded18-constant bank/index mapping as the plate fragment.
A separate C++ scalar evaluator supplies expected arithmetic. Its acceptance
bound is absolute float error<=0.000002, rounded channel error<=1 byte, and no
rounded-alpha GREATER0 decision disagreement. This is native combiner arithmetic
coverage against source-derived definitions, not original NV2A or original
software-host raster equivalence. Test-only arbitrary words never enter the
public typed renderer.

## Measured result

Normal and address/undefined-behavior GPU runs pass on the NVIDIA GeForce
RTX5080 with synchronization validation enabled and zero validation errors.
The actual typed draw is byte-identical to the prior native readback. Its
original software-host residual remains at most1 RGB byte,14477 differing
pixels and32098 total absolute RGB byte difference. All307200 original D24
integers/stencil bytes and target alpha remain unchanged. Every one of the19
rejected prepares retains the actual native binding with a direct subsequent
record/readback. The ten whole-caller controls and captured-only legacy guard
also pass.

All52832 combiner outputs pass. Maximum absolute float error is
`1.192092896e-7`, maximum rounded channel error is0 bytes, and rounded-alpha
GREATER0 disagreements are0. Normal and sanitized scene readbacks, combiner
input/output bytes, control records and result reports are identical. The
compiled unchanged shader binaries also match between both builds. These
measurements cover the finite original-definition corpus and32 explicit sample
sets; they are not an exhaustive real-valued arithmetic proof or an original
raster reference for the broader definitions.

CPU address/undefined-behavior/leak checks pass. The GPU sanitizer run uses
`detect_leaks=0`; the prior full-GPU and empty-context checks both reported the
same3156 bytes/29 external graphics/dbus teardown allocations. This run claims
address/undefined-behavior coverage, not a new clean full-driver leak check.
The initially omitted private validation-layer path caused an exit77 before
GPU initialization; the successful runs use the explicit retained SDK layer
and library paths, NVIDIA ICD and synchronization-validation setting.

An independent read-only source review found no concrete mismatch in the
typed seam or its pre-mutation guards. `validation.json`, `independent-review.json`,
`source-input-manifest.json` and `artifact-manifest.json` under the typed
analysis directory record the source/results and their scope. The historical57
scene and41 caller input records verify at their explicit archived locations.
No old proof manifest or count was rewritten.

`typed-original-native-phone.png` presents the retained original/native scene
and a nearest-neighbor crop without altering the source scene pixels. Runtime
activation and live source producer admission remain separate work.
