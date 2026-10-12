# Original Xbox campaign residency requests

Research date: 2026-10-07. The four campaign functions examined here have
`void` script results, but they perform cache requests or change object state.
A native eager residency backend can satisfy a request with retained decoded
assets; completing the script expression alone does not establish residency.
All offsets are hexadecimal; counts, indices, and byte lengths are decimal
unless prefixed by `0x`. This report contains layout and behavioral findings,
not copied third-party implementation bodies.

## Primary sources and local evidence

* [Retail recovery](https://github.com/kirklandsig/halo2-decompiled/tree/5f08630655e3ddc9ac45909090f11ae799fa0c27),
  revision `5f08630655e3ddc9ac45909090f11ae799fa0c27`, CC0. Its
  [function manifest](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/config/functions.csv)
  distinguishes `matched`, `near`, and `todo` recovery status.
* [Assembly original Xbox scripting definitions](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml),
  [Xbox BSP layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/sbsp.xml),
  [render-model layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/mode.xml),
  and [lightmap layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/ltmp.xml),
  revision `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`, GPL-3.0.
* Local `assets/disc/default.xbe`, SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
  [Original evidence](../analysis/campaign/residency-original.json) records
  definition bytes, original function bytes, hashes, file offsets, and recovery
  status. The sibling `residency-XXXXXXXX.txt` files disassemble 23 original
  functions with retail virtual addresses.
* Local `assets/disc/maps/01b_spacestation.map`, build `02.09.27.09809`, SHA-256
  `5f79832ce3ae8035e547c5c673008802a4971fbf60551755fcc0553482843ae0`.
  Scenario datum `E1780003`, metadata file offset `0E5F43E0`. Its expression
  pool SHA-256 is
  `b52e8cebc73a7ef095ef3307be8a66ef1ef07ee49ba28b8a6ba160d2e77d23bb`.
  [Cairo evidence](../analysis/campaign/residency-cairo.json) records every
  relevant compiled expression and argument, source offsets, section ranges,
  predicted-resource lists, and checked raw payload hashes.

`Retail verified` means inspected instructions or data from this exact XBE;
it does not upgrade an entire `todo` reconstruction to byte-matched status.
The files above are inspection artifacts, not proof that native rendering,
audio, cache eviction, or campaign timing matches the original.

## Script table and evaluator ABI

The original function table is at VA `004744E0`; slot `opcode*4` contains a
pointer to its definition. Definitions contain return-type `u16+0`, flags
`u16+2`, evaluator `u32+4`, parse pointer `u32+8`, parameter count `i16+C`, and
parameter types `i16+E` onward. All four inspected definitions have flags zero,
null parse pointer, and return type **4: void**. Type numbers are established
by the recovery's [HS type declaration](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/hs.h).

| Builtin | Opcode | Definition VA | Evaluator VA | Parameter types | Behavior helper |
|---|---:|---:|---:|---|---:|
| `object_cinematic_lod` | `05C` | `0044B76C` | `002A2140` | object 50, boolean 5 | `000BBF40` |
| `sound_impulse_predict` | `253` | `0044DE30` | `002A9D70` | sound 32 | `00125F10` |
| `predict_model_section` | `374` | `0044F4B0` | `002AC4B0` | render_model 41, long 8 | `0016E330` |
| `predict_structure_section` | `375` | `0044F4C4` | `002AC500` | structure_bsp 26, long 8, boolean 5 | `0016E510` |

All four evaluator recoveries are `matched`. They evaluate their arguments
through `00209D50`, invoke the helper, then finish the script expression through
`00209AE0(thread, 0)`. They do not wait for cache residency or return its success
as a boolean. The BSP evaluator casts the first argument to **signed 16 bits**;
the boolean arguments consume the low byte. See the
[evaluator definitions](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_29f5b0.cpp).

## Sound prediction

`00125F10` is `matched` and retail verified: a sound datum `FFFFFFFF` is a
no-op. Otherwise, if its pitch-range count is nonzero and its first pitch range
has permutations, select the **first permutation of the first pitch range**
and the **first chunk of that permutation**, then call `00218850` with flags
**2**. This starts an asynchronous cache load; it does not play the sound,
decode the complete permutation, lock the cache entry, or wait for completion.
See [first-chunk prediction](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_124f90.cpp)
and the [matched sound-cache request](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_218850.cpp).

The cache-request flags are bit 0: wait, bit 1: initiate load, bit 2: lock.
Its result bits are bit 0: loading, bit 1: loaded, bit 2: locked. The script
wrapper discards them. An existing cache entry is touched, and an absent entry
is allocated only when the load bit is set and the owner is not `NONE`.

Relevant retail metadata: `snd!` first-pitch-range `i16+8`, pitch-range count
`u8+A`; `ugh!` pitch ranges `+20` (stride `C`, first permutation `i16+8`, count
`i16+A`); permutations `+28` (stride `10`, first chunk `i16+C`, chunk count
`i16+E`); chunks `+40` (stride `C`, encoded file location `u32+0`, size/flags
`u32+4`, runtime cache index `i32+8`). Every tagblock has a count followed by
its pointer. The original [sound-chunk declaration](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/unknown_218850.h)
and instructions at `00218A10` mask size with **`0x3FFFFFFF`**. Reclaimer's
[chunk reader](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/SoundCacheFileGestaltTag.cs)
uses a tentative `0x00FFFFFF` mask. Cairo's 19,963 chunk records have size/flag
upper bytes only `00` or `80`, so both masks produce the same size here.

The first `x02_scene_01` prediction is sound `F3EA11B2`,
`sound\cinematics\01_spacestation\x02\music\x02_01_mus`:

| Field | Value |
|---|---|
| Sound metadata file offset | `0E8896C8` |
| Gestalt datum / metadata file offset | `84C9227A` / `0EC4BF94` |
| Pitch range / permutation / first chunk | 3347 / 12861 / 13408 |
| Full permutation chunk count | 119 |
| First chunk metadata file offset | `0ECC4B78` |
| First chunk record bytes | `00d0d501f87f0000ffffffff` |
| Encoded location / source / byte length | `01D5D000` / Cairo map / 32760 |
| First chunk SHA-256 | `0e60cd1ffc8a9faea6555a9e047bf1fd886e1bb51a09347ef691cb1b92428651` |

The next predicted sound is `F3F311BB`, `x02_01b_fol`. Loading full decoded
audio into a retained native sound cache supplies more data than these original
requests require; playback remains a separate operation.

## Model section prediction

Retail verified helper `0016E330` has `todo` recovery status. A model datum
`NONE` is a no-op. A section index **-1 requests all sections**. A valid
nonnegative section requests that exact section; other indices skip the
geometry request. This is the `mode` **sections block** index, not a region,
permutation, part, mesh draw number, or level-of-detail number.

`mode+24/+28` are the section count/pointer. Sections have stride `5C`; their
geometry-cache block starts at **section+38**. After geometry selection, the
helper also predicts material textures if global `00468D24` differs from this
model datum, then updates that global. This is consecutive-model suppression,
not a set of all previously requested models. Even an invalid section index
can execute this material phase. Retail instructions confirm the
[recovered traversal](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_16e290.cpp):

* Materials: `mode+60/+64`, stride `20`, shader datum `material+C`.
* First shader runtime-property pointer: `shader+24`; its bitmap count `+4`,
  pointer `+8`; references stride `C`, bitmap datum `+0`.
* Each referenced `bitm`'s bitmap count/pointer `+44/+48`, stride `74`:
  predict **every bitmap record**, with flags `E`, through the inline
  [bitmap cache predictor](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/unknown_03bcb0.h).
  `NONE` shaders and bitmap datums are skipped.
* If `mode+74` PRT-info count is positive, request the **first** PRT record's
  geometry block at `PRT+34` (PRT pointer `mode+78`).

The first authored request in script 397 `x02_01_predict` is model `F58D1355`,
`ss_exterior_gun_hinge`, section 0 of 5. Section metadata file offset
`0E8F3194`; its block points to Cairo file `081D4C00`, length 37020, section
data 112 bytes, resource data 36896 bytes, 9 resource descriptors. Raw SHA-256:
`9f5f591124cfece68f5608ec975fd7c1831b3d4baf8da0cafc1b6c6397cad19c`.

## Structure section prediction and resource dispatch

Helper `0016E510` is `matched` and retail verified. The first argument is the
**scenario BSP index**, not an `sbsp` tag datum. It compares that signed 16-bit
index with currently loaded BSP global `004686C4`, and does nothing for a
different BSP. It never switches or loads a BSP. A section index **-1 is a
no-op**, unlike model prediction. Invalid section indices are skipped.

| Boolean | Index namespace | Count / pointer | Stride | Action |
|---|---|---|---:|---|
| false | `sbsp` clusters | `+9C/+A0` | `B0` | Traverse cluster's predicted-resource block `+84/+88`, mode 3 |
| true | `sbsp` instanced geometry **definitions** | `+138/+13C` | `C8` | Request definition geometry-cache block `+28` |

This boolean chooses the namespace; it is not a wait, force, or priority flag.
The true branch does not use instanced placement indices at `sbsp+140`.
See the [matched BSP helper and dispatcher](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_16e290.cpp)
and [original Xbox layouts](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/sbsp.xml).

Predicted-resource records have stride **8**: signed `i16 type+0`, signed
`i16 index+2`, and full `u32 tag_datum+4`. The cluster script branch calls
dispatcher `0016E5E0` with mode **3**, which performs the following:

| Type | Original action in mode 3 | Meaning of resource index |
|---:|---|---|
| 0 | `0012CC10` requests/touches a bitmap record from the resource's `bitm` tag (`bitm+48`, stride `74`) | Bitmap record index |
| 1 | `0016EA60` checks/touches first chunk of every permutation in every pitch range, calling `00218850` flags **8** | Ignored |
| 2 | `0016E950` requests **all** geometry sections of the resource's `mode` tag (`+24/+28`, stride `5C`, block `+38`) | Ignored; commonly -1 |
| 3 | `0012DCB0` requests cluster geometry from the resource's `sbsp` (`+A0`, stride `B0`, block `+28`) | Cluster index |
| 4 | **Skipped in mode 3** | Instance placement index, when used by other modes |
| 5 | `0016E890` requests current lightmap's scenery/object geometry bucket | Scenery object bucket-ref index |
| 6 | `0016E8F0` requests current lightmap's instance geometry bucket | Instance bucket-ref index |
| 7 | `0016E7B0` requests current lightmap's cluster bitmap | Cluster render-info index |
| 8 | `0016E820` requests current lightmap's instance bitmap | Instance render-info index |

Type 1's flag 8 **does not set the cache's load bit**, so it does not initiate
absent sound-chunk loads. Type 2 requests geometry without the separate
`0016E330` material traversal. Types 5–8 ignore the resource tag datum and use
the loaded lightmap association. Unknown type values take no action. The
dispatcher's readiness result combines helpers that return readiness; it does
not propagate type 0's texture result, and the script builtin discards the
aggregate result. These distinctions come from original disassembly and the
[recovered dispatcher](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_16e290.cpp),
not a proposed generalized preload interface.

For types 5–8, original code requires non-null loaded lightmap `004E0344`,
positive lightmap-group count (`+80`), non-null loaded BSP `004E0348`, a
non-`NONE` lightmap bitmap datum (`group+1C`), and matching group checksum
`group+4 == sbsp+8`. It uses the first group pointer (`lightmap+84`):

* Type 5: bucket refs `group+64`, stride `C`, bucket index `u16+2`.
* Type 6: bucket refs `group+54`, stride `C`, bucket index `u16+2`.
* Both resolve geometry buckets `group+44`, stride `38`, block `bucket+C`.
* Type 7: cluster render info `group+2C`, stride `4`, bitmap index `i16+0`.
* Type 8: instance render info `group+4C`, stride `4`, bitmap index `i16+0`.
* Bitmap index -1 is a no-op; otherwise use the bitmap tag at `group+1C`, its
  bitmap array `bitm+48`, stride `74`. Missing association/checksum mismatch is
  a no-op with true helper result. A native loader must resolve the current
  BSP's lightmap group, not select an unrelated group's same numerical index.

The bucket-reference indices are zero-extended by the original instructions;
the bitmap indices are sign-extended and explicitly tested for -1. Those are
different sentinel contracts despite Assembly labeling both fields `int16`.

The type 4 helper `0016E770`, used only outside mode 3, resolves
`sbsp+144` instances (stride `58`, definition index `i16+34`) to
`sbsp+13C` definitions (stride `C8`, block `+28`). Its recovered variable names
say render model/permutation, but the actual offsets and Xbox schema establish
the BSP instance-to-definition namespace.

## Geometry block contract

The original [geometry block](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/geometry_cache.h)
is `24` bytes: block location `i32+0`, block length `i32+4`, section-data length
`i32+8`, resource-data length `i32+C`, resource count/pointer `+10/+14`, owner
tag index `i16+18`, owner-section offset `i16+1C`, runtime-linked byte `+1E`,
flags byte `+1F`, and cache block datum `i32+20`. Encoded file locations use
their high two bits to select current map/mainmenu/shared/single_player_shared;
the low 30 bits give the file offset.

Helper `0012DCB0` (`near` recovery, inspected original) accepts a non-null
block with positive section data and nonnegative resource data. If linked and
its cache block exists, it touches that block and returns true. If linked
without a cache block, it clears linkage. Otherwise it can issue a configured
blocking request or queue a pending geometry request, setting flags bit 2 to
avoid duplicate queued entries. It can return false while work remains.
See [geometry-cache lifecycle](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_12de70.cpp).

Native eager retention can satisfy the content requirement without reproducing
the Xbox cache allocator. That claim requires actual bytes to be loaded,
decoded and retained for the requested block and dependencies. File bounds
and a hash alone demonstrate readable payloads, not retained native residency.

## Cairo authored requests and validation boundaries

The expression pool contains 26 `sound_impulse_predict` calls (25 unique tags),
595 `predict_structure_section` calls (160 unique tuples), 912
`predict_model_section` calls (222 unique tuples), and 104
`object_cinematic_lod` calls (78 unique object/bool pairs). These are distinct
compiled expression nodes, not measurements of an executed campaign. No Cairo
model or structure prediction uses section -1; its valid behavior was verified
from the original instructions.

Script 397 `x02_01_predict` contains 39 cluster requests, 22 instanced-definition
requests, and 93 model-section requests covering 28 model tags. Its first
structure calls are `(BSP 0, cluster 0/1/6, false)`. Its first true call is
`(BSP 0, definition 32, true)` at expression node `ADD04A5C`, file offset
`0E6BBAE8`. Definition 32's metadata is `0A29EF74`, raw block `08928C00`, size
932, SHA-256
`a52fd0bb07697825b5b1b981e00a9fb685c60900225f16108ef1fafaba118598`.

Cluster 0, BSP datum `83EB219C`, metadata `0A16C2F4`, has predicted resources
at `0A170E94`. Its first three original records are:

| Bytes | Action |
|---|---|
| `030000009c21eb83` | Type 3: BSP `83EB219C`, cluster 0 geometry |
| `070000009c21eb83` | Type 7: current lightmap cluster bitmap 0 |
| `0200ffffef1427f7` | Type 2: all sections of Pillar of Autumn `mode F72714EF` |

The Pillar model is absent from `x02_01_predict`'s explicit model-tag list.
Consequently, loading the current BSP's own meshes and explicit predicted
models does not satisfy this cluster resource request.

The six scenario BSP entries have cluster/instanced-definition counts:
`0:109/114`, `1:19/22`, `2:40/43`, `3:44/41`, `4:1/0`, `5:47/34`.
All 1507 literal model/structure call arguments validate against those exact
arrays and model sections. The distinct requested structure tuples comprise
72 cluster resource lists and 88 instanced-definition blocks. These cluster
lists reference 81 distinct render models and 405 distinct bitmap-tag/index
pairs; class, salt, and bitmap ranges all validate. Nineteen of those 81 models
never appear in any explicit model prediction throughout Cairo's scripts.

All 222 explicitly requested model blocks (94 models), all 88 explicitly
requested definition blocks, and all 25 explicitly predicted first audio
chunks were read to their exact recorded lengths and hashed. Explicit model
blocks total 4,335,932 bytes: 120 blocks from Cairo, 75 from
`single_player_shared`, 14 from `mainmenu`, and 13 from `shared`. This check did
not decode or retain these payloads in the native runtime, and does not cover
all transitive lightmap buckets, PRT resources, textures, or every type 2 model
section referenced by a cluster list.

## Cinematic object state and invalid values

Matched setter `000BBF40` changes bit **19 (`0x80000`)** in the live object's
flags word at object `+4`; object datum `NONE` is a no-op. It issues no cache
request. See the [object setter](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_0bbf40.cpp).
An original verified consumer, `0004BAF0`, detects that flag (or global
`004BA019`) and sets both distance-fade/shadow output bytes to 255 before
reading the model's distance thresholds. The
[consumer recovery](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_0350e0.cpp)
is `todo`; its original bytes establish this early branch. This finding alone
does not establish every render-LOD consumer or a complete highest-LOD policy.

Cairo's first compiled object-LOD script is `x02_02_problem_actors`, setting
`dervish`, `tartarus`, and `brute_01` true. Their compiled object literals hold
object-name indices (`FFFF0147`, `FFFF013B`, `FFFF013C`); the VM must resolve
them to live object datums before applying the setter.

Sound/model `NONE`, live-object `NONE`, non-current BSP, and invalid section
indices have the intentional behavior stated above. Arbitrary stale datums,
wrong tag classes, corrupt pointers, or nonexistent live-object indices are
not generalized legal inputs: original helpers often mask a datum to its low
16 bits and dereference without validating its salt. In an inactive scenario,
do not assume BSP index -1 plus a real section is safe merely because current
BSP also equals -1; the original then lacks the required loaded BSP pointer.
A native implementation can validate these malformed inputs while preserving
the authored valid requests and intentional `NONE` cases.
