# Original Xbox model and animation decoding

Verified 2026-10-07 against the local original Xbox Cairo cache,
`assets/disc/maps/01b_spacestation.map`, and its `mainmenu.map`, `shared.map`,
and `single_player_shared.map` resources. Build: `02.09.27.09809`.
The local `default.xbe` SHA-256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
These measurements establish data decoding, skeleton binding, and actual idle
poses. They do not establish complete original rendering or animation-state
behavior.

## Pinned primary evidence

| Source | Revision | Relevant files |
| --- | --- | --- |
| [Reclaimer](https://github.com/Gravemind2401/Reclaimer/tree/36224d7a4581f2a1f255099bf706aa56be9f6e21) | `36224d7a4581f2a1f255099bf706aa56be9f6e21` | Halo2 `RenderModelTag.cs`, `Halo2Common.cs`, `ModelTag.cs`; packed-vector definitions |
| [Assembly](https://github.com/XboxChaos/Assembly/tree/fd14ef3c46135e9422966c6a64cd926fb0bd23fb) | `fd14ef3c46135e9422966c6a64cd926fb0bd23fb` | `Halo2/{mode,hlmt,matg,weap,bipd}.xml`; **`Halo2Xbox/jmad.xml`** |
| [Matching Xbox recovery](https://github.com/kirklandsig/halo2-decompiled/tree/5f08630655e3ddc9ac45909090f11ae799fa0c27) | `5f08630655e3ddc9ac45909090f11ae799fa0c27` | Animation sampling, graph lookup, codec routines, `config/functions.csv` |

Reference checkouts are ignored under `tools/reference/`. Reclaimer and
Assembly are consulted for format facts; no implementation bodies from those
GPL projects are copied into the product. The matching recovery supplies
retail-addressed routines and distinguishes byte-matched routines from
unfinished recoveries in its inventory.
[Reclaimer license](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/LICENSE),
[recovery inventory](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/config/functions.csv).

All offsets below are hexadecimal unless explicitly written as decimal counts.
Integers/floats are little-endian. A metadata tag block is count/pointer, each
32 bits. Translate its pointer through the cache metadata translator before
reading file bytes. A normal tag reference is group at `+0`, datum at `+4`.
The cache/addressing details are in [RESEARCH.md](RESEARCH.md).

## Object and first-person model wiring

| Owning tag | Field | Offset / record size |
| --- | --- | --- |
| Object-derived `bipd`, `weap` | Default model variant StringID | `0x30` |
| Object-derived tag | `hlmt` reference | `0x34` / datum `0x38` |
| `hlmt` | Render `mode` reference | `0x00` |
| `hlmt` | Collision model reference | `0x08` |
| `hlmt` | `jmad` reference | `0x10` |
| `hlmt` | Physics model reference | `0x20` |
| `matg` | Player representations block | `0x138`, stride `0xBC` |
| Representation | First-person hands/body `mode` references | `0x00 / 0x08` |
| `weap` | First-person model/graph block | `0x2A8`, stride `0x10` |
| Weapon first-person record | `mode` / `jmad` references | `0x00 / 0x08` |
| `weap` | First-person weapon offset float3 | `0x308` |

Sources: [object base](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/bipd.xml),
[weapon](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/weap.xml),
[globals](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/matg.xml),
[model references](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/ModelTag.cs).

Validated Cairo references:

| Asset | Object datum | `hlmt` | `mode` | `jmad` |
| --- | --- | --- | --- | --- |
| Grunt | `E79C0626` | `E79D0627` | `E79E0628` | `E7BC0646` |
| Marine | `EB580925` | `EB590926` | `EB5A0927` | `EBA60973` |
| SMG world model | `E1C2004D` | `E1C3004E` | `E1C4004F` | `E1F70082` |
| SMG first person | Same weapon | — | `E2E4016F` | `E2E60171` |
| Battle rifle first person | `FF041CCC` | — | `FF1C1CE4` | `FF221CEA` |
| Master Chief hands | `matg E1740000`, representation 0 | — | `83512102` | Weapon graph above |

Cairo globals contain four representations; entry 0 has hands `83512102` and
body `83572108`, and entries 1–3 have null hands/body references. SMG and battle
rifle each contain two weapon first-person records: entry 0 has the references
above, entry 1 is null. SMG first-person offset is `(0.02,0,0)`; battle rifle
is `(0.02,0,-0.004)`. The authored coordinate convention is +X forward, +Y left,
+Z up. Choose the actual player representation and weapon record; the table's
datums are diagnostics for this cache, not portable constants.

## Render model metadata and resources

| `mode` field/block | Offset | Record size / contents |
| --- | --- | --- |
| Compression bounds | `0x14` | `0x38`: X/Y/Z/U/V/secondary-U/secondary-V min/max float pairs |
| Regions | `0x1C` | `0x10`: name StringID `+0`, permutations block `+8` |
| Region permutations | Within region `+8` | `0x10`: name `+0`, six signed section indices `+4..+0xE` |
| Sections | `0x24` | `0x5C`, detailed below |
| Nodes | `0x48` | `0x60`, detailed below |
| Marker groups | `0x58` | `0x0C`: name `+0`, marker block `+4` |
| Material/shader records | `0x60` | `0x20`: shader reference `+8`, datum `+0xC` |

The six permutation section indices are ordered from lowest to highest detail;
Reclaimer selects `+0xE` for its highest LOD. A negative index omits that
permutation. Rendering all sections overlays multiple LODs and alternate
heads/attachments. Source: [RenderModelTag](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/RenderModelTag.cs),
[mode layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/mode.xml).

| Section field | Offset | Type |
| --- | --- | --- |
| Global classification | `0x00` | i16: 0 worldspace, 1 rigid, 2 rigid-boned, 3 skinned |
| Vertices / triangles | `0x04 / 0x06` | u16 / u16 |
| Opaque maximum nodes per vertex | `0x14` | u8 |
| Compression flags | `0x1A` | u16: bits 0 position, 1 UV, 2 secondary UV |
| Section compression bounds | `0x1C` | Block, same stride as global bounds |
| Hardware node count / node map size | `0x24 / 0x25` | u8 / u8 |
| Rigid node index | `0x2C` | i16 |
| Encoded raw pointer / raw byte size | `0x38 / 0x3C` | u32 / u32 |
| Section data size / resource data size | `0x40 / 0x44` | u32 / u32 |
| Resource descriptors | `0x48` | Block, stride `0x10` |

Resource descriptor: type byte `+0`, primary/secondary locators i16 `+4/+6`,
size u32 `+8`, offset u32 `+0xC`. Raw pointer high two bits select current map,
mainmenu, shared, or single-player-shared; low 30 bits are its file offset.
**For render models, resource base relative to raw start is**
`rawSize - section.resourceDataSize - 4`. All examined character/first-person
sections yield `0x78`. BSP's different resource-base formula must not be reused.
The raw header stores index count u16 at `+0x28` and node-map count u16 at
`+0x6C`. Source: [RenderModelTag resource base](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/RenderModelTag.cs),
[Halo2Common reader](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/Halo2Common.cs).

| Resource identification | Contents |
| --- | --- |
| First resource descriptor | Parts, stride `0x48`; material i16 `+4`, index start/length u16 `+6/+8` |
| Primary locator 32 | Index buffer, u16 indices |
| Primary 56, secondary 0 | Position and bone/weight bytes |
| Primary 56, secondary 1 | UV, two signed i16 per vertex, stride 4 |
| Primary 56, secondary 2 | Normal/tangent data, stride 12; packed normal u32 at `+0` |
| Primary 100 | Byte node-map entries: section-local bone index → `mode` skeleton index |

Reclaimer identifies a list when `faceCount*3 == indexCount`, otherwise a
strip, and falls back to strip when early triples contain repeated indices.
All concrete sections below are strips. Restart strip parity for each part,
preserve parity while skipping degenerate triangles, and validate each part
range independently. Their expanded nondegenerate triangle counts exactly
equal the authored section triangle counts.

Xbox compressed position/UV components decode as
`min + ((signedShort + 32768) / 65535) * (max - min)` using the corresponding
model bounds. This is a shifted range, unlike normal signed normalization.
Normal u32 fields are signed two's-complement X/Y/Z widths 11/11/10, bit starts
0/11/22; divide by 1023/1023/511 after sign extension.
[Halo2Common vertex decoding](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/Halo2Common.cs),
[HenDN3](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Core/Geometry/Vectors/HenDN3.cs),
[packed signed components](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Core/Geometry/Vectors/PackedVectorHelper.cs).

Vertex stride is position-resource byte size / vertex count. First six bytes
are XYZ signed i16. Rigid sections have no per-vertex influence data; their
single bone is `nodeMap[0]` (validated equal to `section.rigidNodeIndex` here).
Rigid-boned class 2 has local bone byte `+6`, padding `+7`, and weight 1.
For skinned class 3, begin at `+6`, skip two bytes when influence count is 2 or
4, then read N bone-index bytes followed by N weight bytes divided by 255.
For N=1, an encoded zero weight is treated as 1. The observed strides are 8
for N=1, 12 for N=2/3, and 16 for N=4. Remap bone bytes through the section
node map before indexing the skeleton. Quantization makes multi-influence
sums range from `254/255` to `256/255` in these assets; preserving or changing
that sum is a rendering choice requiring shader parity evidence.
[Xbox render-model blend decoding](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/Halo2Common.cs).

## Skeleton, markers, and authored variants

Node fields: name StringID `+0`; parent/first-child/next-sibling i16
`+4/+6/+8`; local translation float3 `+0xC`; quaternion XYZW float4 `+0x18`;
inverse scale float `+0x28`; inverse forward/left/up/position float3
`+0x2C/+0x38/+0x44/+0x50`. Marker records have stride `0x24`, region/permutation/
node bytes `+0/+1/+2`, translation `+4`, quaternion `+0x10`, scale `+0x20`.
[Node and marker layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/mode.xml).

With column-vector matrices, the four inverse vectors are matrix columns;
build local rotation/translation from the quaternion/translation, accumulate
`world = parentWorld * local`, and skin with `animatedWorld * inverseBind`.
Bind-pose world times stored inverse bind differs from identity by at most
`3.50e-7` across the five measured models. Reclaimer explicitly inverts the
stored inverse transform to expose a bone world transform.
[Matrix interpretation](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Utilities/Utils.cs).

`hlmt` variants block `+0x50`, stride `0x38`: name StringID `+0`, runtime map
16 signed bytes `+4`, regions block `+0x14`. Variant region stride `0x14`:
name `+0`, runtime region i8 `+4`, parent variant i16 `+6`, permutations block
`+8`. Variant permutation stride `0x20`: name `+0`, runtime permutation i8
`+4`, probability float `+8`, states block `+0xC`, stride `0x18`. A state has
name `+0`, runtime permutation i8 `+4`, model damage-state i16 `+6`, looping
effect reference `+8`, initial probability `+0x14`.
[Variant layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/hlmt.xml).

Resolve the object's default variant StringID, then choose the applicable
region permutation using its authored probabilities; a blank name/runtime
permutation `-1` omits that region. Retain the choice on the object rather than
rerolling every frame. A complete implementation also needs inherited variants
and damage states. **Local bytes correct the plugin's runtime-map comment:**
Marine `default_nrl` runtime array is `(1,6,0,2,-1,5,7,4,3,-1,...)` and maps
**mode region index → variant-region record index**. The record's runtime-region
byte directly matches the mode-region index: head 2, body 0, helmet 3. Names
and these direct indices agree; treating the runtime array value as a mode
region index produces the wrong regions. This interpretation is verified for
this retail variant, not asserted as universal for every cache revision.

Grunt `minor_crz` is variant 0 with no region overrides; highest-LOD sections
20–24 cover arms/backpack/head/legs/torso. Marine `default_nrl` is variant 1:
body standard section 53, head Perez/Morgan sections 72/71 each probability
0.5, helmet on 56. Comm pack is omitted; thigh/chest packs have 0.9 chance,
arm packs 0.95, abdomen packs 1.0. Rendering Smith head section 55 is a fallback
appearance rather than this variant's authored head choice.

## Measured model assets

| Asset / section | Raw cache file and offset | Raw bytes | Vertices / triangles | Influences / stride |
| --- | --- | --- | --- | --- |
| Grunt arms 20 | `single_player_shared.map:1569FE00` | 18936 | 680 / 554 | 1 / 8 |
| Grunt backpack 21 | `single_player_shared.map:156A4800` | 7780 | 287 / 168 | Rigid / 6 |
| Marine standard body 53 | `single_player_shared.map:15620000` | 37876 | 1180 / 1230 | 3 / 12 |
| SMG first-person 0 | `shared.map:082B3E00` | 48476 | 1765 / 1788 | Rigid-boned / 8 |
| Battle rifle knob 0 | `mainmenu.map:008E0400` | 1964 | 63 / 80 | Rigid / 6 |
| Battle rifle body 1 | `mainmenu.map:008E0C00` | 66360 | 2418 / 2342 | Rigid-boned / 8 |
| Chief left hands 0 | `shared.map:083E3800` | 36012 | 1123 / 1232 | 3 / 12 |
| Chief right arms 1 | `shared.map:083EC600` | 13192 | 407 / 428 | 2 / 12 |

SMG section 0 has index count 2809, node map `[0,1,2,3]`, and part
`(material,indexStart,indexLength)` tuples `(0,0,1652)`, `(1,1652,907)`,
`(2,2559,250)`. Its first vertex decodes to position
`(0.035789565,-0.005385242,0.039150774)`, UV `(0.980080687,0.327215018)`,
model bone 1 with weight 1. Raw section SHA-256:
`5c2f05876c96f562ebdfd5dc6bd8cbb19518594c0bfb2cc10283b0c17fcab974`.

Marine body 53 first vertex is `(0.034913885,0.211960588,0.314378543)`, UV
`(0.424394956,0.491342255)`, skeleton indices `[42,48,0]`, encoded weights
`[128,128,0]/255`. Its raw SHA-256 is
`dac7e5598ab867e4c152fc62fdcecad10fedbfcec5c13c005fedffb6be7d4db6`.
All eight sections above have in-range indices and remapped bone indices;
finite bounds; part-derived triangle counts equal to authored counts. This is
CPU data validation, not live Vulkan visual proof.

## Retail animation graph layout and action lookup

Use the **Xbox** `jmad` layout; the common plugin's clip stride differs.
[Xbox graph layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/jmad.xml),
[retail animation structures](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/unknown_1dacb0.h).

| Graph field/block | Offset | Record size / contents |
| --- | --- | --- |
| Parent graph reference | `0x00` | Ordinary tag reference |
| Inheritance/private flags; codec pack | `0x08 / 0x09 / 0x0A` | u8/u8/i16 |
| Skeleton nodes | `0x0C` | `0x20`: name `+0`, sibling/child/parent i16 `+4/+6/+8` |
| Sound / effect reference lists | `0x14 / 0x1C` | `0x0C` each |
| Blend screens | `0x24` | `0x1C` |
| Animations | `0x2C` | **`0x6C`** |
| Modes | `0x34` | `0x14`: label `+0`, weapon-class block `+4` |
| Default node data | `0xA4` | `0x3C`; zero entries in these target graphs |
| Resource groups | `0xAC` | `0x14`: owner datum `+0`, byte size `+4`, encoded raw pointer `+8` |

Weapon-class records stride `0x14`: label `+0`, weapon-type block `+4`.
Weapon-type records stride `0x34`: label `+0`, action block `+4`; action stride
8: label `+0`, graph index i16 `+4`, animation index i16 `+6`. Graph index `-1`
means the current graph; inherited graph indices require their own mapping.
Actual selections: SMG `any/any/any/idle` → clip 21; dual class → clip 2;
battle rifle `any/any/any/idle` → clip 4; Marine
`combat/missile/any/idle` → clip 17. Clip names are useful diagnostics, while
the action table supplies authored selection.

| Clip field | Offset | Type |
| --- | --- | --- |
| Name StringID | `0x00` | u32 |
| Type: base/overlay/replacement | `0x10` | u8: 0/1/2 |
| Root movement kind | `0x11` | u8: none, XY, XY+yaw, XYZ+yaw |
| Node count / frame count | `0x13 / 0x14` | u8 / i16 |
| Internal flags / playback flags | `0x16 / 0x18` | u8 / u16 |
| Resource index / resource offset | `0x24 / 0x28` | i32 / i32 |
| First-frame / loop-frame indices | `0x2C / 0x2E` | i16 / i16 |
| Parent / next animation indices | `0x30 / 0x32` | i16 / i16 |
| Legacy data size / pointer | `0x34 / 0x38` | i32 / pointer |
| Static / animated mask byte sizes | `0x3C / 0x3D` | u8 / u8 |
| Movement / unknown04 / static data byte sizes | `0x3E / 0x40 / 0x42` | i16 each |
| Unknown08 / animated data byte sizes | `0x44 / 0x48` | i32 each |
| Frame / sound / effect events | `0x4C / 0x54 / 0x5C` | Blocks, strides 4/8/4 |
| Object-space parent node records | `0x64` | Block, stride `0x20` |

Internal flag `0x10` selects streamed resource data; the resource group's
encoded pointer plus clip resource offset addresses its bytes. Otherwise the
legacy pointer is used. The sampled clips all use resources and have zero
movement/unknown04/unknown08 sizes. Their data is static codec blob of size S,
animated codec blob of size A, then static masks, animated masks. Each mask
set contains rotation/translation/scale bitsets; each component bitset has
`((nodeCount+31)/32)*4` bytes. Channel indices are compact ordinals among set
bits, separately for rotation/translation/scale. Initialize unencoded
components from the appropriate model bind pose; base clips apply static
then animated channels. Overlay/replacement/blending need their distinct
original semantics.
[Resource selection](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_1dacb0.cpp),
[sampling and masks](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_279d80.cpp).

**Mask sets are not always both present.** Read the stored mask sizes rather
than always allocating six component masks. SMG clips 5/6/7/19/23/24/25 and
BR clips 7/8/9/15 have no static codec data and static-mask size zero; their
animated masks begin immediately at `clipStart+S+A`. The animated-mask address
is always `clipStart+S+A+staticFlagsSize`. Among Marine's 501 clips, 103 have
no static mask; among Grunt's 207 clips, 44 have no static mask. Metadata also
reports root-movement bytes in 310 Marine and 120 Grunt clips. These clips
require separate root-movement handling in addition to pose sampling.

The original node mapper at retail `0x1DD4C0` matches mode and graph nodes by
StringID, not index. Chief hands have 37 nodes; weapon graphs have 42. For SMG
and battle rifle, hands map to graph 0–36, weapon's four bones to graph 37–40,
and graph 41 is camera control. Graph gun node 37 has parent 6 (`r_hand`),
whereas the standalone weapon's root has no parent. Accumulate the **graph**
hierarchy to attach animated weapons to the hands. SMG graph 38/39/40 are
charging handle/magazine/stock; battle rifle has magazine/ophandle/safety.
[Original StringID mapper](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_1dacb0.cpp).

Animation time is 30 frames/second: `0x1CCB80` converts seconds to frames and
`0x1CCB40` reports frame count / 30. Sampling clamps to 0 through frameCount−1;
fraction thresholds 0.0001/0.9999 select exact neighboring frame paths.
[Time conversion](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_1cafc0.cpp),
[sampling clamp](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_279d80.cpp).

## Codec layouts and interpolation

The original dispatch table at VA `0x47FB18` defines IDs 0 no compression,
1 static, 2 uncompressed animated, 3 eight-byte quantized rotations,
4 byte-keyframe, 5 word-keyframe, 6 reverse byte-keyframe,
7 reverse word-keyframe, and 8 blend screen. Clip `currentCompression` at
`+0x1B` is a compression preference enum, **not** this codec ID. Read the ID
from each codec blob's first byte. Bytes 1/2/3 are rotation/translation/scale
channel counts in the validated blobs.
[Codec dispatch](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_279d80.cpp).

Codec 1 header is 32 bytes: translation offset u32 `+0xC`, scale offset u32
`+0x10`; rotation short4 at `+0x20 + ordinal*8`, translation float3 at its
offset plus `ordinal*12`, scale float at its offset plus `ordinal*4`.
Normalize all four signed rotation shorts; dividing by 32767 alone is not the
original operation. Retail `0x28C470` uses an approximate SSE reciprocal
square root, so ordinary full-precision normalization has a small parity
difference.
[Static decoders](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_28c470.cpp),
[scale decoder](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_28c510.cpp).

Codec 3 header is 32 bytes: translation/scale offsets `+0xC/+0x10`, rotation/
translation/scale per-channel strides u32 `+0x14/+0x18/+0x1C`. Rotation address
is `blob+0x20+ordinal*rotationStride+frame*8`; translations and scales use
their offset plus channel stride plus `frame*12` or `frame*4`. Rotations are
normalized short4; translation/scale are floats. Interpolated rotations blend
the **raw shorts** using `(1−t,t)` and normalize; the original routine shows
no hemisphere sign correction. Codec 2 uses float4 rotations/16-byte frame
steps; codec 8 also uses float4 rotations. Neither should be decoded as codec 3.
[Quantized animated sampling](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_28c510.cpp),
[uncompressed animated sampling](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_2c4d60.cpp).

Sparse codecs 4–7 use a 48-byte header. Rotation descriptors start `+0x30`;
translation/scale descriptor offsets u32 are `+0xC/+0x10`; rotation/translation/
scale frame-array offsets are `+0x14/+0x18/+0x1C`; key-array offsets are
`+0x20/+0x24/+0x28`. Descriptor u32: low 12 bits key count, high 20 bits start
index. Frame width is one unsigned byte for 4/6, two unsigned little-endian
bytes for 5/7. Read frames at `frameArray + start*width`, keys at
`keyArray + start*keyStride`, strides 8/12/4. Keys are rotation signed short4,
translation float3, scale float. Codecs 6/7 use the same sampling formulas:
the validated reverse codec physically stores channel runs in reverse order,
but each run's frame numbers remain ascending. Do not reverse frame time.
[Sparse integer sampling](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_28cdb0.cpp),
[sparse fractional sampling](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_28c510.cpp).

Select the largest key frame ≤ requested frame. Use that key at exact hits or
the last key; otherwise interpolate toward the adjacent key. Even integer
frame sampling interpolates between sparse keys. The original rotation path
normalizes `rawA*(upperFrame−frame) + rawB*(frame−lowerFrame)`; translation and
scale use the corresponding interval ratio. Original float interpolation uses
an approximate reciprocal. Validate descriptor/run/key bounds, nonzero counts,
strictly increasing frame numbers, finite keys, and the allowed frame range
before sampling; malformed data must not access a nonexistent next key.

## Actual idle clips and numeric fixtures

| Clip | Graph / index | Frames | Resource file offset + clip offset | S / A / masks |
| --- | --- | --- | --- | --- |
| SMG `first_person:idle` | `E2E60171` / 21 | 89 | `shared.map:089EEE00 + 0` | 304 / 8576 / 24+24 |
| BR `first_person:idle` | `FF221CEA` / 4 | 89 | `mainmenu.map:00BC5C00 + 0` | 336 / 6440 / 24+24 |
| Marine `combat:missile:idle` | `EBA60973` / 17 | 90 | `01b_spacestation.map:096E7A00 + 4190` | 540 / 764 / 24+24 |

SMG resource group 0 encoded pointer `889EEE00`, group size 24024; clip bytes
8928, SHA-256 `f7ec1cf6c28e53fcd09c06d4e3220a946850bfa8f7e8158b821de4ab88bca5d5`.
Static codec 1 counts `[32,1,1]`, translation/scale offsets `120/12C`.
Animated codec 3 counts `[6,4,0]`, translation/scale offsets `10D0/2180`,
rotation/translation/scale strides `2C8/42C/164` (89×8/12/4).
Static masks select rotations `{0,7..36,41}`, translation `{37}`, scale `{41}`;
animated masks select rotations `{1..6}`, translations `{0,1,2,41}`, no scales.

| SMG graph component | Frame 0 | Frame 44 | Frame 88 |
| --- | --- | --- | --- |
| Base translation | `(-.00704586,0,-.08)` | `(-.00704586,0,-.07743371)` | `(-.00704586,0,-.07999324)` |
| Left upper-arm quaternion XYZW | `(-.24641440,-.19660658,.21418581,-.92452703)` | `(-.25154117,-.20765423,.21476526,-.92058821)` | `(-.24677963,-.19669734,.21409338,-.92443172)` |
| Right upper-arm quaternion | `(.38551756,-.39824403,-.09183204,-.82725134)` | `(.36760841,-.42196379,-.11228156,-.82109894)` | `(.38637505,-.39745358,-.09076457,-.82734924)` |

SMG gun local translation is `(.03332817,.00838999,.01070037)` with quaternion
`(0,0,0,-1)`. Camera-control translation is the negative of base translation
and has parent base; its world translation therefore cancels this root offset.

BR clip bytes 6824, SHA-256
`c0e9ec6d92e0f01df68b585ec92a7b324751d59febe8e9475822f7319abda9bc`.
Static counts `[33,3,1]`, animated codec 3 counts `[6,2,0]`; animated
translation/scale offsets `10D0/1928`, strides `2C8/42C/164`.
Frame-0 base translation `(-.05046928,.03202,-.08466264)`, gun local translation
`(.03108346,.01060851,-.00325391)`, gun quaternion
`(-.01162775,-.01623612,.13065191,-.99122715)`.

Marine clip bytes 1352 at final offset `096EBB90`, SHA-256
`97a860016810ee41b9916a299506f85bce7c094d5b36d282be2bfd1b4917d782`.
Animated codec **6**, counts `[18,1,0]`. Header descriptor offsets translation/
scale `78/7C`; frame-array offsets `7C/B6/BF`; key-array offsets `C0/290/2FC`.
Rotation channel 0 maps to node 1, `l_thigh`: descriptor `0x37003` at `+30`
means start 55/count 3. Its frame run at `+B3` is `[0,36,89]`; short4 keys at
`278/280/288` are `(-5892,-30900,-7058,5852)`,
`(-5600,-30953,-7116,5790)`, `(-5892,-30901,-7058,5852)`.
Translation channel 0 maps to pelvis node 0: descriptor 9 at `+78`, frames
`[0,25,26,29,30,31,33,45,89]` at `+B6`. Frame-0 key `+290` is
`(-.000458519993,-.001147108851,.341652482748)`; frame-45 key `+2E4` is
`(-.001426800038,-.001147108851,.342854410410)`; frame-89 key `+2F0` is
`(-.000455499976,-.001147108851,.341648757458)`.

All 268 integer frames of these three clips were decoded from local bytes
using the described layouts. Masks' popcounts equal channel counts, selected
runs/keys stay inside their resources, transforms are finite, and quaternion
unit-norm residual is at most `9.05e-8` with full-precision normalization.
This numerical check does not claim bit-identical SSE `rsqrt`/`rcp` results.

Additional authored first-person clips: SMG `fire_1` index 20/5 frames,
`reload_empty` 30/50, `reload_full` 31/50; battle rifle `fire_1:var1..3`
indices 0–2/8 frames, `reload_empty` 13/58, `reload_full` 14/58. Use action
lookup and linked variant selection for gameplay rather than these fixed
diagnostic indices.

## Recovery quality and remaining boundaries

Upstream marks node mapper `1DD4C0`, static quaternion `28C470`, static scale
`28C510`, animated quaternion `28C960`, interpolated quaternion `28C790`,
translation `28C750`, byte-sparse quaternion `28CDB0`, word-sparse quaternion
`28D560`, and fractional word-sparse quaternion `28D180` **matched**. Dispatcher
`2798A0` and several translation/interpolation routines remain **todo**.
Format validation and matching individual kernels should not be represented
as a recovered complete animation engine.
[Exact match-status inventory](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/config/functions.csv).

The proven implementation subset is compressed rigid/skinned models,
highest-LOD region selection, named graph mapping, base static/full-frame/sparse
poses, and the listed idle fixtures. Remaining fidelity work includes
representation selection across playable races, model change colors, inherited
variant/damage-state behavior, animation inheritance/overlays/blend screens,
aim and weapon IK, root movement extraction, object-space parent nodes,
event timing, camera/FOV transforms, authored material passes and transparent
sorting, and the original transitions and state machine. Shader diffuse
bitmap decoding already researched in [RESEARCH.md](RESEARCH.md) applies to
`mode` material shader references, but diffuse rendering alone does not
reproduce the full original shader.
