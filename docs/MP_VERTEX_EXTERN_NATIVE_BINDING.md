# Original vertex extern binding

The native material binder now uses the recovered original19BE0 vertex extern
consumer for its existing object-change RGB input. The shared helper also
supports the direct scalar and RGB forms required by all five authored
camouflage implementations. It preserves unselected components and returns
the original per-bank dirty masks. It does not select or submit camouflage
passes; those consumers remain incomplete.
[Native helper](../src/source_vertex_extern.cpp),
[ordinary material consumer](../src/world_material.cpp),
[public contract](../include/halo2/source_vertex_extern.hpp)

The destination packs register, bank, component and a texture selector.
Original19BE0 passes literal scalar mode0 to1B2C0; the high destination bits
do not request inverse scalar arithmetic. Source record byte+2 and destination
bits10..12 are ignored. Scalar components0..3 copy their source bits directly;
component4 copies RGB and preserves alpha. Texture selection has no effect on
these forms. Dimension components require their separate texture owner and
remain unadmitted. This differs from the original pixel extern's independently
verified inverse mode and from17000's parameter-vector remapping.
[Complete original owners and callee inputs](../analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex-report.json)

An initial interpretation called the packed high bits a scalar mode. The
unchanged whole-function comparison disproved it: destination2000 with scalar-2
returns-2, not3. The corrected fixture explicitly records the actual1B2C0
texture-selector/scalar-mode arguments. Original output banks were never
synthesized to fit either interpretation.

The unchanged retail corpus contains9,124 complete19BE0 cases:8,640 declared
scalar/RGB cases across both banks/all16 registers,64 overlapping mapping-order
cases and420 compiled camouflage cases. It covers finite negative/positive
values, signed zero, the actual fade residue, texture selectors0/1, byte+2
controls, ignored bits, preexisting dirty masks and distinct untouched axes.
All128 float words and both final dirty masks are compared. The420 compiled
cases independently agree with the immutable earlier pass report.
[Original tool](../tools/oracle_camouflage_vertex_extern.py),
[corrected fixture](../tests/fixtures/camouflage_vertex_extern_original.json),
[native comparison](../tests/source_vertex_extern_tests.cpp)

The frozen native verification passes the complete corpus, all24 texture
reference cases and resources on21 maps, all1,280 material-order cases, the
Vulkan-validated GPU owner-isolation check and ordinary rendered multiplayer
with validation enabled. All ten GPU regression captures are byte-identical
to the prior material proof. Fresh original runs reproduce the44 earlier pass
outputs and the corrected vertex fixture/report/listings byte for byte.
ASan/UBSan and leak detection pass for the instrumented texture/vertex/material
owners and the complete reference tests; remaining libraries are built normally.
[Verification manifest](../analysis/multiplayer/game-variants/vertex-extern-native-20261011/verification.json)

The manifest uses the prior complete frozen material build plus an explicit
three-object production archive overlay. Every tested executable is relinked;
headers retain their original layouts. Exact live CMake additions are configured
over the prior source snapshot, excluding unrelated concurrent checkout edits.
Source, fixture, original dependencies, commands/logs, binaries and captures are
pinned. A metadata-only amendment clarifies the optional third owner in the
inherited description; the original manifest, amendment script and all original
tested inputs/outputs are retained.
Run `.tools/python/bin/python tools/verify_shader_texture_reference.py --vertex-extern`.
[Reproduction driver](../tools/verify_shader_texture_reference.py)

This is retail scalar/RGB vertex-binding evidence, not complete TU1.5 rendering
or general dimension-extern admission. The ordinary RGB consumer already has
its original value provider. The camouflage consumer still needs its live
scalar/fog dispatch, state externs, actual geometry/resource admission, scene
copy, queue/layer scheduling and depth/color alias effect. The active goal still
requires full local/network matches and editable rules for all39 variants.
[Pass/resource contracts](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md),
[alias boundary](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md),
[operational acceptance](GAME_MODE_PROGRESS.md)
