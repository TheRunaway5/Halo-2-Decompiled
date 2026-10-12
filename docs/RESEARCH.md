# Original Xbox Halo 2 recovery and map geometry

Verified 2026-10-07 against the source revisions below. This document records
format and feasibility research; it does not establish that the native game
boots or that a decompiler's generated C is behaviorally correct.

## Pinned primary sources

| Source | Revision | Local reference checkout |
| --- | --- | --- |
| [kirklandsig/halo2-decompiled](https://github.com/kirklandsig/halo2-decompiled/tree/5f08630655e3ddc9ac45909090f11ae799fa0c27) | `5f08630655e3ddc9ac45909090f11ae799fa0c27` | `tools/reference/halo2-decompiled` |
| [Gravemind2401/Reclaimer](https://github.com/Gravemind2401/Reclaimer/tree/36224d7a4581f2a1f255099bf706aa56be9f6e21) | `36224d7a4581f2a1f255099bf706aa56be9f6e21` | `tools/reference/reclaimer` |
| [XboxChaos/Assembly](https://github.com/XboxChaos/Assembly/tree/fd14ef3c46135e9422966c6a64cd926fb0bd23fb) | `fd14ef3c46135e9422966c6a64cd926fb0bd23fb` | `tools/reference/assembly` |
| [BirchWoodGod/halo2-decomp](https://github.com/BirchWoodGod/halo2-decomp/tree/080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888) | `080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888` | Not cloned |

The reference checkouts are research inputs, not linked into this project's
native runtime. Reclaimer and Assembly declare GPL-3.0; both decompilation
projects declare CC0 for their own recovery work. No source implementation
from those projects was copied into this document or the native runtime.
License files: [Reclaimer](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/LICENSE),
[Assembly](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/LICENSE),
[matching decompilation](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/LICENSE),
[native recovery](https://github.com/BirchWoodGod/halo2-decomp/blob/080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888/LICENSE).

## Executable compatibility and current feasibility

The matching decompilation targets original Xbox retail `default.xbe` with
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
It specifies the 2004-09-28 link date and Visual C++ 7.1 from XDK 5849 with
link-time code generation. Compatibility with an extracted local executable
requires comparing the complete file hash, not its filename or disc title.
[Target and toolchain](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/README.md#the-target).

At the pinned revision, upstream reports 6,734 matching game functions out of
11,318 and 736,034 matching bytes out of 2,784,283 (26.44%). This is the
upstream checker's report, not a local independently rerun match result.
[Progress](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/docs/PROGRESS.md#2026-10-07-6734-functions-match).

The matching project's build constructs an XDK-linked LTCG image for byte
comparison, including stand-ins and unresolved-function stubs. It is not a
native PC game build. Native porting is a later stage in its documented plan.
[Build implementation](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/tools/build.py),
[approach](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/README.md#the-approach).

BirchWoodGod's independent recovery targets the same executable and reports
389 recovered engine routines built into a Linux library and diagnostic host.
Its README explicitly describes the absence of a runnable game. The host's
pool/configuration probes do not provide rendering, game boot, or full Havok
simulation. This is a useful behavioral reference for later recovery, not a
complete engine ready to attach to SDL and Vulkan.
[Native recovery status and host boundaries](https://github.com/BirchWoodGod/halo2-decomp/blob/080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888/README.md).

## Function labels for analysis

The matching project's `config/functions.csv` supplies virtual addresses,
sizes, owners, callees, source paths, and match status. Game names are
deliberately blank in the current inventory. Source definitions instead carry
`@retail` address annotations. Its `tools/build.py::marked_sources()` exposes
the associated recovered identifier, source path, signature, and method kind.
This permits a derived address/name index without copying routine bodies.
[Inventory](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/config/functions.csv),
[annotation parser](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/tools/build.py#L391).

Join labels to inventory by virtual address and retain revision, path, and
match status in the analysis output. Descriptive labels are researchers'
identifiers, not recovered original symbols. Address placeholders convey no
semantic identification. The upstream provenance document records removed
third-party symbol data and an audit of identifiers.
[Provenance](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/PROVENANCE.md).

## Original Xbox cache addressing

All offsets below are hexadecimal. Scalars are little-endian. Tag blocks
contain a signed 32-bit count and a 32-bit virtual pointer. Metadata pointers
must be translated before reading file bytes.

| Cache header field | Offset |
| --- | --- |
| Header size | `0x800` bytes |
| Declared file size | `0x08` |
| Tag index file offset | `0x10` |
| Metadata offset relative to tag index | `0x14` |
| Metadata size | `0x18` |
| Build string | `0x120` |
| Scenario name | `0x1BC` |
| Tag filename count | `0x2C0` |
| Filename string data file offset | `0x2C4` |
| Filename string data byte size | `0x2C8` |
| Filename offset array file offset | `0x2CC` |

The `0x20`-byte tag index header stores group-table pointer/count at `+0/+4`,
tag-table pointer at `+8`, scenario datum at `+0xC`, and tag count at `+0x18`.
Tag entries have stride `0x10`: class ID at `+0`, datum at `+4`, metadata
pointer at `+8`, and metadata byte size at `+0xC`. Semantic four-character
class IDs read opposite to their on-disc byte order. Tag names use the
filename offset array and string data; the datum's low 16 bits select a tag
slot.
[Assembly cache layouts](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/Layouts/H2X_Layouts_Core.xml),
[Reclaimer cache reader](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/CacheFile.cs).

Header translation is `headerMagic = groupPointer - (tagIndexOffset + 0x20)`;
`tagTableOffset = tagTablePointer - headerMagic`. Ordinary metadata translation
is `metaMagic = firstTagMetaPointer - (tagIndexOffset + metadataOffset)`;
`fileOffset = metadataPointer - metaMagic`. BSP tags use their own translator.
[Header translator](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/HeaderAddressTranslator.cs),
[metadata translator](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/TagAddressTranslator.cs).

The scenario (`scnr`) structure-BSP tag block is at `+0x210`; entry stride is
`0x44`. Each entry stores the BSP file offset at `+0`, size at `+4`, virtual
base at `+8`, BSP datum at `+0x14`, and lightmap datum at `+0x1C`. For that BSP,
`bspMagic = virtualBase - fileOffset`; BSP pointer translation subtracts this
magic. Its metadata begins at `fileOffset + 0x10`, after the BSP header.
[Scenario definition](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/Config/ScenarioTag.config.cs),
[BSP translator](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/BSPAddressTranslator.cs),
[BSP base selection](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/CacheFile.cs#L284).

## Renderable BSP geometry

Relative to the BSP metadata base:

| Block/field | Offset | Entry size |
| --- | --- | --- |
| World X/Y/Z min/max float pairs | `0x34/0x3C/0x44` | `8` each |
| Clusters tag block | `0x9C` | `0xB0` |
| Material/shader tag block | `0xA4` | `0x20` |
| Instanced geometry definitions tag block | `0x138` | `0xC8` |
| Geometry instances tag block | `0x140` | `0x58` |

Clusters and instanced definitions share these initial geometry fields:
vertex count `u16 +0`, face count `u16 +2`, resource data pointer `u32 +0x28`,
resource block size `u32 +0x2C`, header size `u32 +0x30`, and resource table
tag block `+0x38`. Resource records have stride `0x10`: primary locator
`i16 +4`, secondary locator `i16 +6`, size `u32 +8`, data offset `u32 +0xC`.
Instances store scale at `+0`, basis vectors at `+4/+0x10/+0x1C`, translation
at `+0x28`, and definition index `i16 +0x34`.
[Reclaimer BSP definitions](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/Config/ScenarioStructureBspTag.config.cs),
[Assembly BSP layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/sbsp.xml).

Reclaimer reads each resource block and uses `headerSize + 8` as the base for
resource offsets. The block's `u16 +0x28` is the index count. Primary locator
`32` identifies `u16` indices; `56` identifies vertex streams. Secondary
locators `0/1/2` select positions/UVs/normals. Positions are float XYZ with
stride `positionResourceSize / vertexCount`; UVs are float XY, stride 8;
normals begin with a packed 32-bit value every 12 bytes. Resource record zero
holds parts: stride `0x48`, material `i16 +4`, index start `u16 +6`, index
length `u16 +8`. List/strip selection uses `faceCount * 3 == indexCount`, with
a degenerate-triple check that can change list to strip.
[Original Xbox mesh reader](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/Halo2Common.cs),
[BSP resource base](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/ScenarioStructureBspTag.cs).

Convert strips separately for each part. Alternate winding for successive
triples; skip degenerate triples while continuing the parity counter.
Assembly labels part flags at `u16 +2`, bit 3 as `Override Triangle List`;
the inspected Reclaimer reader uses counts and degenerates rather than this
flag. Do not claim the flag alone is a complete topology discriminator.
[Strip conversion](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Core/Geometry/GeometryExtensions.cs#L23),
[part flags](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/sbsp.xml#L254).

Resource data pointers reserve their top two bits for the source cache:
`0=local`, `1=mainmenu.map`, `2=shared.map`, `3=single_player_shared.map`.
The low 30 bits are the actual file offset. Apply this distinction before
any range checks against the selected file.
[Resource pointer implementation](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/DataPointer.cs).

Scenario player starting locations are a tag block at `scnr +0x100`, stride
`0x34`: float XYZ position at `+0`, facing float at `+0xC`, team `i16 +0x10`,
BSP index `i16 +0x12`. A native viewer can begin at an authored starting
location associated with its loaded BSP. This does not reproduce the game's
spawn-selection rules.
[Player starting locations](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/scnr.xml#L1128).

## Diffuse bitmap loading for the native viewer

The BSP material array (`+0xA4`, stride `0x20`) contains an 8-byte shader tag
reference at `+8`; its datum is at `+0xC`. Original Xbox shaders use class
`shad`. The shader's runtime-properties tag block is at `+0xC`, stride
`0x50`; the first entry's diffuse `bitm` reference is at `+0`, datum at `+4`.
Null datum `0xFFFFFFFF` means no diffuse bitmap. Reclaimer uses this field as
a fallback; its full material path follows `stem` template/pass remappings
and shader constants for texture usage, tiling, blending, and tint.
[Shader definitions](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/ShaderTag.cs),
[material mapping](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/MaterialHelper.cs).

The `bitm` bitmap-data tag block is at `+0x44`, record stride `0x74`:

| Record field | Offset |
| --- | --- |
| Width/height/depth, signed 16-bit | `0x04/0x06/0x08` |
| Type/format/flags, 16-bit | `0x0A/0x0C/0x0E` |
| Registration X/Y | `0x10/0x12` |
| Additional mip count | `0x14` |
| LOD0/LOD1/LOD2 resource pointers | `0x1C/0x20/0x24` |
| LOD0/LOD1/LOD2 resource sizes | `0x34/0x38/0x3C` |

Type `0` is 2D; `1` is 3D; `2` is cubemap. Format IDs `14/15/16` are
DXT1/DXT3/DXT5, `1` is Y8, and `11` is A8R8G8B8. Flag `0x8` requires
unswizzling. LOD0 includes additional mip data. The top 2D DXT mip occupies
`ceil(width/4) * ceil(height/4) * blockBytes`, with 8 bytes for DXT1 and
16 for DXT3/5. Select the resource file using the pointer's high bits before
reading. Metadata ownership and pixel-resource ownership are independent.
[Bitmap reader](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/BitmapTag.cs),
[DXT blocks](https://learn.microsoft.com/en-us/windows/win32/direct3d9/compressed-texture-formats).

For unswizzled 2D DXT data, a source-derived baseline is direct BC1 RGBA,
BC2, or BC3 upload preserving row order and authored UVs. Reclaimer's 2D
path performs no vertical flip. Query Vulkan format support; tightly packed
buffer-image copies use zero row-length/image-height and an extent in texels.
For swizzled 2D scalar/color images, axis bit interleaving maps each linear
pixel coordinate to its source byte address, taking X then Y bits while
each dimension has another bit. Do not apply a Y8 deswizzler to compressed
blocks. Cubemaps have separate face orientation rules.
[Texture conversion](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Utilities/TextureUtils.cs),
[Vulkan formats](https://docs.vulkan.org/spec/latest/chapters/formats.html),
[Vulkan image copy](https://docs.vulkan.org/refpages/latest/refpages/source/VkBufferImageCopy.html).

Read-only local verification found 54 Lockout BSP material slots: 29 resolve
to DXT5, seven to DXT1, 13 to swizzled Y8, and five have null diffuse
references. Slot 0 (`trim_vents`, shader datum `0xF2E71162`) references
`trim_borders` (`bitm` datum `0xF2E81163`): bitmap record `0x20BCC7C`,
512 x 256, DXT5, flags `0x0203`, nine additional mips, local data pointer
`0x01629E00`, 174,848-byte resource. Its 131,072-byte top mip hashes to
`d064f04105af0bece72bd68f720a816dafb3a0e2da75ce09c10b0e3721882a7c`.

Shared-resource selection was checked with Lockout's
`effects/bitmaps/smoke/smoke_soft_small` (`0xE1ED0078`): metadata
`0x1D5ACC0`, bitmap record `0x1D5AD0C`, 64 x 64 DXT5, flags `0x0203`.
Pointer `0x89156400` selects `shared.map` offset `0x09156400`; its resource
size is 5,504 bytes and its 4,096-byte top mip hashes to
`f11a12df9055079b64eee6258139f6ed97b521c21842fde0b2ca1c9a9628913d`.
These measurements use extracted local disc files, not downloaded textures.

Lockout cluster 0's UV stream starts at file offset `0xAD2C4C`: 134 float2
values with U approximately `[-0.00110, 0.99399]` and V
`[-6.01358, 12.78444]`. Repeated texture coordinates are intentional.
The initial runtime-diffuse path has now been extended with typed pass and
constant decoding, authored detail/bump transforms and P8 lightmap bindings.
The supported static `lightmap_tex` / `lightmap_tex_detail` previews assemble
and execute the retail pixel programs. Full layer/LOD scheduling, extern state,
remaining vertex programs and materials, and final visual parity remain open.

Resident mip selection now follows matching retail function `0x359E0` and
the texture-header caller `0x12D6D0`. For power-of-two compressed images,
additional mip count is the smaller of the tag limit and
`floor(log2(max(width / 4, height / 4, depth)))`. Linear and non-power-of-two
images have no additional levels. This matters for rectangular BC images:
512x128 and 512x256 tags may advertise nine additional mips, while the retail
resident header selects seven. Resource allocation padding must not be
interpreted as more mip levels. The native parser retains the selected
authored mip bytes, deswizzling each scalar/color level at its own dimensions,
and Vulkan uploads each selected level rather than generating replacements.
[Matching mip-count routine](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_0359e0.cpp),
[resident texture header](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_12c0d0.cpp).
`tools/differential_rendering.py` confirms 30,140 bit-exact comparisons against
the local XBE instructions on positive dimensions. It does not validate the
material sampler state, shader arithmetic, lighting or final framebuffer.

## Shader programs and lightmaps

The native decoder keeps the shader's bitmap indices, packed pixel constants,
float4 vertex constants, template LOD/layer/pass ranges, every implementation's
texture bindings and render states, and the original compiled vertex payload.
The original pass's inline pixel definition is 60 little-endian words at
implementation `+6`. Its fragment range is at `+0xF6`.

The retail `0001BA00` routine establishes a correction to the reference XML:
fragment entries are **four bytes**, signed short switch index plus packed
short permutation range. The XML's three-byte layout reads incorrect switch
and range values in this Xbox build. Permutations are six bytes; combiner
records are 32 bytes. The routine concatenates the selected fragments,
retains per-fragment offsets for constant remapping, and treats the final
combiner separately. The native port matches 336/336 instruction comparisons
across 183 distinct retail implementations from all extracted caches.
The component-preserving vertex remapper (`00017000`) matches 1,672/1,672
instruction comparisons across every defined component mask, both constant
banks, destination registers and randomized overlapping writes.
The packed pixel remapper (`00017170`) matches 1,288/1,288 instruction
comparisons covering all component masks, both banks, final constants,
fragment stage offsets, filtered switch permutations and overlapping writes.
This comparison checks the resulting program words; the original dirty masks
and GPU upload timing remain scheduler work.
Evidence: `tools/differential_combiners.py` and
`analysis/rendering-audit/combiner-differential.json`.

Register-combiner operand mappings, output flags, registers and final-stage
operations were checked against primary NV2A implementation definitions:
[xemu combiner registers](https://github.com/xemu-project/xemu/blob/master/hw/xbox/nv2a/pgraph/psh_regs.h),
[xemu pixel shader implementation](https://github.com/xemu-project/xemu/blob/master/hw/xbox/nv2a/pgraph/glsl/psh.c),
[Cxbx D3DPIXELSHADERDEF layout](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/master/src/core/hle/D3D8/XbD3D8Types.h).
These were used as format and hardware references; the native implementation
is not linked to the emulators. GPU arithmetic still needs matched hardware
input/output validation. The common vertex expressions were recovered from
this cache's original vertex payload, using the
[NV2A instruction field layout](https://github.com/xemu-project/xemu/blob/master/hw/xbox/nv2a/pgraph/glsl/vsh-prog.c).
Their viewer camera/global bindings remain outside the exact CPU-kernel claim.

Lightmap groups live inside the BSP allocation. The BSP header's pointer at
`+8` addresses the lightmap metadata; groups are at lightmap `+0x80`, stride
104. A group contains 1,024-byte palettes at `+8`, a bitmap datum at `+0x1C`,
cluster bindings at `+0x28`, and instance bindings at `+0x48`. Each four-byte
binding contains signed bitmap index, palette byte and flags byte. Negative
bitmap index `-1` denotes the per-vertex path and is not silently bound to
bitmap zero. The default group is selected explicitly; alternate groups and
per-vertex lighting remain pending.

Bitmap format 18 uses the selected group's P8 palette. Format 17 uses the
retail bump palette, confirmed byte-for-byte in the local XBE at file offset
`0x4646C8`, guest address `0x468848`, SHA-256
`d077fbea214db2ecde0fcd58967f777a7b7c63f9f5ed1796969b95511c3e38cc`.
The native loader reads and fingerprints the complete palette from the user's
extracted executable. It expands palette colors before texture filtering and
preserves authored swizzled mips. It does not distribute that palette in source.

BSP resource channels 2/3/4 carry packed tangent basis, signed-normalized
short2 lightmap coordinates, and packed incident light direction respectively.
The normal/tangent/binormal values are signed 11/11/10-bit components. Cluster
and instance lightmap bindings are independent of material bindings; Vulkan
draw descriptors use the pair so shared geometry can have different baked
lighting. Lockout's default group yields 65 distinct bitmap/palette textures.


## Local disc format validation

A read-only probe against the project ISO enumerated 81 files, including 30
maps. It read the 4,861,952-byte `default.xbe` directly from the game
partition: its complete SHA-256 equals the pinned decompilation target above.
The inspected level cache build strings are `02.09.27.09809`.

The same probe enumerated every BSP and geometry record in all 27 level maps,
excluding `mainmenu.map`, `shared.map`, and `single_player_shared.map`:

| Quantity | Observed count |
| --- | --- |
| Scenario BSPs | 77 |
| Cluster geometry records | 4,119 |
| Instanced geometry definitions | 2,842 |
| Authored geometry instances | 12,857 |
| Nonempty geometry records | 6,860 |
| Cluster and definition vertices | 4,782,006 |
| Index entries | 6,989,259 |
| Material parts | 36,371 |

Every nonempty block had position stride 12, finite float positions, vertex
indices below the vertex count, and part index ranges inside its index
buffer. All resource data pointers were local. Every block satisfied the
reader's triangle-list condition; the first ten triples of each block had
no duplicate vertex indices. This confirms the researched layouts against
the local retail data. It does not verify GPU output, winding, materials,
lighting, instance rendering, collision, or gameplay. Vertex totals count
definitions once; authored instances must still apply their transforms.

Two convenient reproductions are `maps/ascension.map` (one BSP, seven
clusters, 68 player starts) and `maps/lockout.map` (one BSP, 16 clusters,
66 starts). Ascension's first cluster has 1,695 vertices and 3,618 indices;
its first player start is approximately `(0.0255567, 8.19794, 0.00124186)`.
Lockout's first cluster has 134 vertices and 255 indices; its first start is
approximately `(7.03357, -0.134619, -8.27901)`. These values were read from
the project ISO rather than obtained from external assets.

## Practical route and acceptance boundaries

First preserve and hash the ISO and extracted executable; inventory the exact
cache build before choosing layouts. Produce an address-preserving static
analysis workspace with raw decompiler exports marked as unreviewed. Build
an independently implemented, bounds-checked cache reader and SDL/Vulkan host
that displays original disc BSP geometry, including instance transforms.
This creates a runnable foundation with identifiable original content.

Recover engine slices with native-versus-original differential checks and
observable retail behavior as references. Work through memory pools,
scenario loading, objects, animation, collision, player simulation, scripts,
AI, audio, UI, saves, and networking. Replace Xbox platform interfaces at
their observed boundaries. A static map viewer and raw function exports are
substantial initial artifacts, but neither establishes a playable native
Halo 2 port. The need for remaining recovery follows from both inspected
decompilation projects' explicit status and runtime limitations above.

For cache validation, require every translated pointer and resource range
to fit its selected file, every vertex to be finite, every index to fit the
vertex count, and every part range to fit the index stream. Enumerate all
scenario BSPs and authored instances; reject malformed records rather than
guessing offsets. Rendering verification should include real GPU execution
and an image of disc geometry, in addition to parse/build checks.
