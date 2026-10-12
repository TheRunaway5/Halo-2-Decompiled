# Original shader texture references

The native world material binder now follows the actual `matg` rasterizer
reference for texture extern1. It previously searched for the normalization
bitmap's name. The shared resolver also implements original extern26's bump
index and its sole -1 fallback through the global camouflage shader. Extern26
is available to a future complete camouflage pass binder; this change does not
submit that pass or admit another variant.
[Native resolver](../src/source_shader_texture.cpp),
[production consumer](../src/world_material.cpp),
[original texture contracts](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md)

The resolver retains original datum/frame bits, signed bump indices, tag
groups and pointers. Serialized shader arrays can point into another resident
tag's allocation. The cache adapter resolves the unique actual containing
payload and retains its backing for the call; it does not assume every pointer
is local to its shader. Escaping or ambiguous backing fails explicitly.
The global shader comes from the rasterizer row's `+CC/+D0` reference,
normalization from `+8/+C`, and the shader's fifth four-byte bitmap-property
record supplies its signed bump index. No bitmap name determines a result.
[Public contract](../include/halo2/source_shader_texture.hpp)

The frozen verification compares all24 unchanged original19530 prefixes,
including the real body bump and emblem fallback. All21 owned maps load the
referenced normalization cube and global bump resource. A separately declared
edited rasterizer row follows its changed reference. Framebuffer extern17
continues to require its separate target owner and is rejected here.
[Reference/resource comparison](../tests/shader_texture_reference_tests.cpp),
[verification manifest](../analysis/multiplayer/game-variants/shader-texture-reference-native-20261011/verification.json)

The same production archive passes all1,280 earlier material-order cases, the
corrected GPU owner-isolation fixture with Vulkan validation, and the ordinary
rendered multiplayer species flow with validation enabled. The original pass
inspector's44 outputs reproduce byte for byte, including its420 whole extern
compositions,24 texture prefixes and four deliberately cold framebuffer cases.
ASan/UBSan and leak detection pass for the new resolver, changed material owner
and reference test; other libraries retain their normal build.

Verification uses an explicit archive overlay on the prior complete frozen
material build. Both changed production objects are compiled against its
unchanged headers, replace their archive members, and all tested executables
are relinked. No existing public layout changes. The exact two live CMake
additions are configured over the prior frozen source; unrelated concurrent
checkout changes are excluded and the live CMake file is retained separately.
The manifest pins source, base/archive/binaries, original dependencies, maps,
validation libraries, commands and captures. The first harness attempt's
original-inspector path error and logs remain in its separate timestamped run;
the successful run uses the inspector's real root and verifies all declared
dependencies before and after execution.
[Reproduction driver](../tools/verify_shader_texture_reference.py)

Run `.tools/python/bin/python tools/verify_shader_texture_reference.py`.
The driver requires the retained complete material build named in its `BASE`.
When the vertex extern consumer is present, it also verifies that owner and
writes the separate vertex-extern manifest; the earlier texture proof is retained.
This is bounded native tag-reference and resource evidence. Original bitmap
cache readiness, scene-copy target19, the biped clone/queue/layer18/19 draw,
depth/color alias feedback and complete ordinary matches remain open.
[Alias limitation](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md),
[operational acceptance](GAME_MODE_PROGRESS.md)
