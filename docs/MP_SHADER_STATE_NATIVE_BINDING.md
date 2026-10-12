# Native shader state imports and camouflage blend color

The material loader now retains the original render-state extern range, and a
shared CPU consumer evaluates its seven executable descriptors. Camouflage's
existing owner binding now supplies vertex scalar37 and state color40 as well
as pixel scalar37. The renderer publishes each material's current state75 as
dynamic blend constants, including the original ONE / inverse-constant-color
blend factors. Complete camouflage rendering and additional gamemode admission
remain outside this change.
[Material import](../src/material.cpp),
[state consumer](../src/source_shader_state.cpp),
[object binding](../src/world_material.cpp),
[native drawing](../src/renderer.cpp)

## Original contracts

`MaterialPass::state_externs` preserves all four bytes from the selected
implementation's packed range at `+11E`, using the same authored extern block
as the separate pixel/vertex ranges. The range preserves its nine-bit first
index, seven-bit count and row order. Its destination bytes are a state-table
index and a provider selector, rather than a vertex destination. Byte2 is
ignored. Exactly provider1 selects the scalar bank; every other provider value
selects RGB. The seven literal descriptors come from retail `467020`.
[Retained type](../include/halo2/render_program.hpp),
[original decoding/table and whole-owner evidence](MP_RENDER_STATE_EXTERN_SOURCE_RESEARCH.md)

Separate original-image comparison now establishes matching complete RGB,
vertex/state consumer and relevant SDK owners in Xbox TU1.1 and TU1.5. Their
seven state descriptors and92 immediate-method words are byte-identical.
That static correspondence supports the recovered primitive contracts;
mutable state, title-update execution and complete runtime flows still need
their own evidence.
[Complete title-update comparisons](MP_RENDER_PRIMITIVES_TITLE_UPDATE_SOURCE_RESEARCH.md)

The CPU state consumer takes the actual existing state-word bank and supplied
scalar/RGB spans. Scalar branches replace alpha after rounding `x*255`, copy
binary32 bits, or round a signed32 integer, according to the literal descriptor.
RGB branches replace only low24 bits. There is no input clamp. Every reached
write is returned in order, including equal values. State138/157 retain their
raw words; their original device packet transformations remain separate.
Native bounds checks admit only the seven descriptors and supplied spans; the
original routine itself does not enforce those bounds.
[Public CPU contract](../include/halo2/source_shader_state.hpp),
[implementation](../src/source_shader_state.cpp),
[original write/cache boundaries](MP_RENDER_STATE_EXTERN_SOURCE_RESEARCH.md)

The RGB/integer primitives take an explicit original x87 control word, with
masked exceptions and 53/64-bit precision. Binary32 multiplication by255 is
exact at either precision; integer rounding uses the requested rounding mode
without changing the host environment. The observed masked signed32 overflow
word is retained before the source byte mask. Nonfinite provider values,
unmasked exceptions, 24-bit/reserved precision and x87 status delivery are
outside native admission. The original comparison corpus directly exercises
53-bit controls; the 64-bit product equivalence follows from the exact product
width, rather than a separate hardware observation.
[Primitive](../src/source_shader_rgb.cpp),
[recorded products and emulator-status limitation](MP_STATE_RGB_PACK_SOURCE_RESEARCH.md)

## Native consumption and drawing

`bind_object_camouflage` resolves scalar37 vertex rows through the shared
`19BE0` consumer and updates both the live constants and the retained base
used by authored animation. Its color40 RGB bank is the already-clamped
first-biped camouflage amount repeated across RGB. Actual state-extern rows
select state75, preserving the current alpha byte. The default state conversion
control is the native runtime's recovered masked, 53-bit nearest control.
Camera/fog extern providers remain separate. Child materials still detach
before mutation so another owner retains its own banks.
[Native owner binding](../src/world_material.cpp),
[original camouflage producer/order](MP_CAMOUFLAGE_BINDER_SOURCE_RESEARCH.md),
[vertex decoding](MP_VERTEX_EXTERN_NATIVE_BINDING.md)

Ordinary selected pass states initialize the material's state75 word. Actual
camouflage implementation18/0 and18/2 author `FF000000`; their RGB extern then
replaces low24 bits. The separate reset evidence does not establish a universal
state75 default, and this change does not introduce one into the source planner.
The original reset/device-program boundary remains explicit.
[Actual selected states and reset limits](MP_RENDER_STATE_EXTERN_SOURCE_RESEARCH.md)

Each resident material retains a separate blend-color word. Material updates
copy it before any unchanged-uniform early return. Every material draw sets
Vulkan blend constants in RGBA order from the original ARGB bytes. The new blend
mode uses source ONE and inverse constant color, with the corresponding alpha
factor. This supplies dynamic native state delivery without rebuilding a
pipeline for each fade value. It does not submit the missing biped clone,
scene-copy target or stencil/depth alias pass.
[Material state word](../include/halo2/cache.hpp),
[per-frame/per-draw delivery](../src/renderer.cpp)

## Verification

The frozen CMake build completed all689 objects for18 selected targets. All18
focused CTests and all24 verification commands passed. The complete native
RGB/state comparisons match8,156 whole RGB cases,7,392 single-row state cases
with repeated equal writes, and eight ordered multi-row cases under four host
rounding modes. Fresh unchanged-original runs reproduce both fixtures and
their reports/listings byte for byte. Scoped ASan/UBSan with leak detection
passes all four complete CPU comparison tests, instrumenting RGB/state
arithmetic, material import and the world-material owner; the remaining
compatible libraries are built normally.
[Frozen verification manifest](../analysis/multiplayer/game-variants/shader-state-native-20261011/verification.json),
[Verification driver](../tools/verify_shader_state.py),
[RGB comparisons](../tests/source_shader_rgb_tests.cpp),
[state comparisons](../tests/source_shader_state_tests.cpp)

The import test compares126 actual compiled implementation ranges and42
state-extern rows across all21 owned maps. The material test compares5,040
recorded state compositions and the corresponding live/retained pixel and
vertex banks. The drawing test holds an actual compiled carbine-display program
and textures fixed on declared quads, varies asymmetric RGB state controls
between two owners, and compares the result with an independent unblended
reference at1280x480 and1920x720. Both resolutions pass all five ordered
states, owner swaps and pixel-exact resets. The prior material-response GPU
check and ordinary native rendered-species check also return0 with Vulkan
validation enabled; skipped graphics checks are not accepted. The manifest
pins24 PPM captures. These are isolation inputs, not an original camouflage
framebuffer.
[Import test](../tests/shader_state_import_tests.cpp),
[native material comparisons](../tests/camouflage_state_material_tests.cpp),
[dynamic blend drawing test](../tests/shader_state_gpu_tests.cpp)

The successful run is `verify-20261011T235858.006932Z`. Two failed fixture
attempts and their source/logs are preserved in its `prior-attempts` directory.
The fixture first omitted its selected pass index, then incorrectly used the
unused color40 bank as scalar37 for passes that do not request color40. The
corrected fixture uses the actual original scalar37 bank; original fixtures,
production code and expected outputs were not changed to resolve these
failures. The manifest records unrelated live-checkout drift from concurrent
work, including renderer changes after freezing. This result applies to the
pinned frozen sources and binaries, not that evolving combined checkout.
[Preserved attempts and frozen sources](../analysis/multiplayer/game-variants/shader-state-native-20261011/verify-20261011T235858.006932Z/),
[Successful CTest log](../analysis/multiplayer/game-variants/shader-state-native-20261011/verify-20261011T235858.006932Z/ctest.log)

The operational goal remains incomplete. Full biped/weapon camouflage pass
selection and submission, genuine framebuffer extern17 readiness, source
scene-copy sampling, concurrent alias writes, shadow consumers, authored
pickups/intrinsic control and complete local/network match flows remain required.
No additional variant is admitted by this change.
[Operational acceptance](GAME_MODE_PROGRESS.md),
[pass/resource evidence](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md),
[alias boundary](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md)
