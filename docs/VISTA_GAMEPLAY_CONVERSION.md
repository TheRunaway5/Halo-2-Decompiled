# Vista gameplay import and Xbox TU1.5 conversion

The requested source policy is:

* Vista supplies campaign/multiplayer maps and the tag graph.
* Setup applies verified Xbox TU1.5 corrections to private copies of those tags.
* Xbox supplies original in-game bitmap resources, including weapon textures.
* Vista supplies HUD/menu artwork and fonts, with the current layouts.
* Vulkan shaders are implemented independently against original Xbox material
  behavior. Vista shader binaries are not a substitute for that implementation.

## Current implementation boundary

The checked **preparation/conversion step and initial native cache adapter are
implemented**. `Cache` accepts the inspected Vista build, resolves its shared
metadata, loads BSP/model geometry and supported inline animation payloads,
and binds Xbox bitmap donors. The existing first-launch selection has not been
switched to Vista gameplay: independent Vulkan material passes, Xbox unit
animation metadata adaptation, remaining resource consumers and the complete
semantic parity audit are unfinished. A registry/path change alone would hide
those gaps.

The preparation command is:

```sh
python3 tools/setup_assets.py --prepare-vista-gameplay \
  --vista '/path/to/Vista installer or extracted folder' \
  --xbox '/path/to/original Xbox ISO or extracted folder'
```

It extracts all top-level Vista `maps/*.map` and compiled fonts without running
the installer. A private `assets/imported/vista-<id>` contains `gameplay/maps`,
the current-layout Vista UI sampling packs, `tag-conversion.json`,
`texture-bindings.json`, `gameplay/asset-donors.tsv` and `provenance.json`. It does not write
`assets/asset-sources.tsv`. Neither source files nor the prior working selection
are modified. Failed preparation removes its staging directory.

No TU1.5 executable is an additional end-user setup requirement. The production
converter contains the bounded recovered constants/reference walks, pinned to
the inspected TU1.5 executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

## Verified correction coverage

`tools/vista_tag_conversion.py` reproduces two recovered original map-load
callees, in their original order:

1. `0011F060`: multiplayer map-ID/type/distance gates and authored netgame-flag
   corrections for Beaver Creek and Relic, preserving scalar binary32 rounding.
2. `00165B30`: multiplayer weapon, melee, grenade, projectile damage and turret
   corrections, resolving the authored runtime slots and shared damage aliases.

See [combat source evidence](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md) and
[netgame source evidence](MP_NETGAME_ORIGINAL_DATA_RESEARCH.md). These are two
known Xbox postprocessors, not a complete Vista-versus-Xbox TU1.5 tag audit.
In particular, the combat routine is also present in the owned 1.1 update;
it is not an exclusively 1.5 balance change.

The reader checks the Vista directory/metadata address masks and resolves exact
shared datum, group and tag path. Missing references, mismatched shared slots,
overlapping resident metadata, unsupported tuning-slot layouts, or accesses
outside a resident tag allocation are errors. It preserves the Vista directory,
all other tag values, raw resources, geometry and shader bytes. The report lists
every ordered write, its originating instruction, owning map/tag, file offset,
old/new bits, and complete input/output map hashes. Resource-only shared caches
are not treated as played multiplayer scenarios.

Every result explicitly records `full_tu15_parity: false`. Remaining work includes
the complete semantic tag audit, earlier original map-loader/group fixups,
remaining native Vista consumer support and independent Vulkan material implementation.
Unknown differences are not filled with guessed Xbox values.

## Original texture correspondence

The texture inventory matches non-UI bitmap tag paths and frame counts to the
provided Xbox disc and records original dimensions and donor datums. It keeps
the original donor bytes available, including the 256×256 BR diffuse/bump
banks. The runtime now forwards display textures, hardware residency partitions
and software low-detail resources to the exact Xbox bank. A missing donor raises
an explicit error. Palettized lightmaps additionally require an identical Xbox
palette; Vista palette bytes are not presumed compatible. Matching paths/frame
counts alone still do not establish compatible sprite UVs or final rendering.

Missing banks remain explicit. The original retail Xbox disc cannot be assumed
to contain every bitmap present in Vista's additional maps, PC cursor/UI data,
or modified/default banks. The inventory is not a count of visible missing
textures, because cache directories include unused banks. Setup must resolve
the relevant dependencies before admitting a playable map; it must not silently
claim a Vista bitmap is the original Xbox artwork.

## Real-source verification, 2026-10-10

The supplied Vista folder prepared **41 caches**, including 23 multiplayer maps.
Across those maps the converter recorded 955 ordered writes: 41 combat stores
per multiplayer graph plus six netgame stores each for Beaver Creek and Relic.
The persistent changes are in `shared.map`, `beavercreek.map` and `dune.map`.
All 41 prepared outputs differ from their Vista inputs only at declared write
locations; every original input hash remains unchanged.

`tools/verify_vista_tag_conversion.py` independently executed both unchanged
original TU1.5 callees on all 39 non-resource Vista caches. It compared every
ordered store and every resulting resident metadata byte against the converter.
All cases passed, including campaign/menu negative gates. Vista's low metadata
addresses overlap the original XBE, so the explicit ABI fixture relocates
metadata and the walked block pointers by `+80000000`. It preserves source code,
tag values and datum references. Original netgame access requests use the
previously pinned compatibility conversion and actual Unicorn page permissions.
This fixture does not execute the preceding map loader or establish live Xbox
gameplay/hardware parity.

The real prepared import is
`assets/imported/vista-fbd81b8ff8a144e488ff4ef4f8f845db`.
It contains 213 menu banks/757 pages and 49 HUD banks/175 pages. The texture
inventory records 15,226 bank occurrences with donors and 1,273 unresolved
occurrences. Reports are under `analysis/asset-setup/vista-conversion/`.

Five importer tests and five conversion boundary tests cover archive selection,
source ownership, shared aliases, exact-reference/bounds rejection, campaign
gating and preservation of the active selection after a late conversion failure.
The source differential requires the developer's locally owned TU1.5 executable
and research dependencies; ordinary end-user preparation does not.

## Native adapter verification, 2026-10-10

`src/vista_cache.cpp` preserves original file offsets and raw-resource cache
selectors while appending private host metadata and BSP/lightmap views. Tag
datums and nested virtual pointers retain their source values. Shared slots
must agree on full datum, group and path. Resident allocation overlap, excessive
spans, missing references and invalid input ranges are rejected. Xbox cache
decoding retains its existing path.

All **39 non-resource caches** pass native BSP geometry loading. All drawable
models selected by the existing preview collector load across those 39 caches;
Sentinel Walls also contains one authored empty ammo model, reported separately.
This does not exercise every alternate region/permutation/LOD. Vista floating
positions, UVs and basis vectors remain floating; this is not proof of equality
with Xbox's quantized geometry.

`tools/verify_vista_vertex_declarations.py` executes the owned Vista declaration
builder at `0066E828` unchanged. Its 14 measured declarations establish FLOAT3
positions, FLOAT2 UVs, NORMAL/BINORMAL/TANGENT ordering, bone fields and the
incident/radiance layouts. The executable fingerprint is
`33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`.
Format documentation is cross-checked against pinned local
Assembly `fd14ef3c46135e9422966c6a64cd926fb0bd23fb` and
[Cartographer](https://github.com/pnill/cartographer/tree/107ea5c0f7dc2260600f871555a18236be051ffd)
declaration wrappers. No Vista shader binary is admitted as Xbox NV2A data.

The real-asset test compares BR diffuse/bump textures, all mips, residency bytes
and low-detail resources directly with Xbox, including original 256×256
dimensions. `tools/verify_vista_animation_payloads.py` establishes byte equality
for all 33 supplied first-person SMG codec payloads. The native asset test compares
all decoded poses, retained channels, names and frame events for 31 clips; both
readers explicitly reject the same two codec 8 clips. It does not claim support
for those clips or all animation graphs.

Lockout's Vista content loads without content errors; a 120-tick standalone
simulation reaches its authored grounded spawn without script faults. A 640×480
Vulkan preview with Xbox donor textures renders successfully on the RTX 5080.
The two-frame preview reports zero Vulkan validation errors. This is a material
preview, not an authentic Xbox rendering result. The bounded
Slayer launch initializes its SMG and then stops at the first unresolved source
material pass. The raw Xbox unit-animation metadata API also explicitly rejects
Vista until its different clip layout is converted. Missing update-executable
runtime constants, audio/other consumer coverage, current-layout HUD integration,
full gameplay parity and complete Vulkan material reconstruction remain open.

The synthetic adapter test covers sparse directory slots, exact shared identity,
unchanged resident bytes, spawn pointers, overlap/name bounds and rejection of
an absent/incomplete Xbox donor. Focused Xbox balance, frontend model geometry
and multiplayer lightmap regressions pass: all 12 focused CTest checks passed.
Evidence is stored alongside the
conversion reports as `native-bsp-read.json`, `native-model-read.json`,
`vertex-declarations.json` and `smg-animation-payloads.json`.
