# Optional frontend texture pack

`load_frontend_ui_assets(content, directory)` returns an optional map from
exact original bitmap tag names to replacement texture-page vectors. An empty
path returns an empty map. The loader never modifies `FrontendContent`.
Original pages continue to own the logical dimensions, bank indices, sprite
rectangles and registration, animation, layout and navigation.
`FrontendPainter` replaces only its uploaded bitmap pages and the matching
static/animated stages of existing 2D emblem materials with the returned
pages. Preexisting world materials are outside that adapter.

This is a texture-only mod seam. Fonts are excluded. Names containing
`framing`, case-insensitively, are rejected because frame centers and their
independent edge textures have a separate source-preserving seam.

The pack root contains `pack.tsv` with this exact first line:

```text
H2UI_TEXTURE_PACK_V1
```

Each subsequent nonempty row has six fields separated by literal tabs:

```text
exact_tag_name    frame_index    bank_count    original_width    original_height    relative_dds_path
```

The spacing shown above describes the fields; actual files must use tabs.
Each target must exist unambiguously in the original content. A bank must
include every original page index exactly once with the original bank count
and that page's original logical width/height. The loader returns its pages
in original index order even when manifest rows use a different order.

Physical DDS dimensions may increase or decrease with a positive isotropic
density relative to each original logical page. Both dimensions must use the
same exact ratio, and every page in a bank must use one common ratio. These
checks use64-bit integer cross products. A valid DDS with a changed aspect
ratio or a mixed-density bank is rejected. The actual approved extracted PC
pack uses1x and2x banks; generic positive ratios remain available for mods.

DDS paths use relative forward-slash paths inside the pack root. Absolute
paths, drive/UNC forms, parent/current-directory components, missing files
and symlinks escaping the canonical root are rejected. The manifest itself
must also resolve inside that root. Files are read as regular files with
bounded sizes.

Supported ordinary2D DDS encodings are:

| Encoding | DDS pixel format | Preparation |
| --- | --- | --- |
| DXT1 | FOURCC `DXT1` | BC1 bytes retained |
| DXT3 | FOURCC `DXT3` | BC2 bytes retained |
| A8R8G8B8 |32-bit BGRA masks, alpha present | Source BGRA expands to RGBA8 |
| X8R8G8B8 |32-bit BGR masks, alpha absent | RGB expands to RGBA8, alpha255 |
| A4R4G4B4 |16-bit masks `F00/F0/F/F000` | Four-bit channels expand by17 |
| A8Y8 |16-bit luma/alpha masks `FF/FF00` | Low byte becomes RGB, high byte alpha |
| Y8 |8-bit luminance mask `FF` | Luma becomes RGB, alpha255 |
| A8 |8-bit alpha mask `FF` | RGB255, source byte alpha |

The existing `prepare_texture(..., swizzled=false, additional_mips)` performs
the conversion. Every authored mip is retained with its dimensions, offset
and size. Headers must describe tight source rows or exact compressed base
sizes, and the payload must equal the complete declared mip chain exactly.
Unknown masks/FourCCs, DXT5, DX10 headers, arrays, cube maps, volumes, padded
row layouts, unsupported reserved metadata, malformed mip flags/counts and
trailing or truncated data are rejected. Each extent is limited to4096.
Individual DDS files are limited to128MiB; encoded and prepared pack totals
are each limited to512MiB.

`frontend_ui_assets_tests` exercises synthetic inputs for all eight supported
encodings, byte-level BGRA/luma/alpha results, compressed and decoded mip
chains, source metadata preservation, complete bank ordering, valid DDS files
with wrong aspect or mixed density, unsafe paths, duplicate/missing bank
indices, unsupported headers and incorrect payloads. It can additionally
load the actual original cache and extracted pack:

```text
frontend_ui_assets_tests <mainmenu.map> <pack-directory>
```

The implementation is in `src/frontend_ui_assets.cpp` and belongs to
`halo2_cache`, as does the optional actual-cache loader test. The paired
painter test links `halo2_systems`. Compilation, native captures and
framebuffer comparisons are separate acceptance gates. Successfully loading
a pack establishes structural compatibility; it does not establish that PC
artwork matches the original Xbox pixels.
