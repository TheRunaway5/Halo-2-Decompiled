# Optional higher-resolution font sampling

This mode changes sampled glyph pixels while retaining the Xbox font table,
ascent, descent, leading, kerning, advances, bearings and logical ink rectangles.
It does not reproduce Xbox raster pixels. The supplied PC distribution was
identified as Project Cartographer; pristine retail Vista provenance is unproved.

`apply_retail_font_sampling(RetailFont&, const RetailFont&)` selects actual
decoded glyph rectangles at the same codepoint. `RetailFont.glyph_record_indices`
retains all65,536 actual source lookup words. Both glyph records must be referenced
by exactly one lookup word. Shared/fallback aliases are preserved without guessing
a fallback index or treating every mapped lookup as authored coverage. Ordinary
BMP Unicode rows exclude controls, surrogates, private-use and noncharacters.
Empty glyphs, missing rows and same-size/smaller source rectangles stay original.
Hand-built fonts without source lookup provenance are not admitted.

The combined RGBA atlas contains an untouched copy of the original atlas followed
by one transparent-white column and an untouched copy of the sampled atlas. Each
original `FontGlyph` field remains unchanged. `sampled_glyphs` holds independent
physical rectangles. `retail_font_sample_uvs()` maps the already clipped and
integer-truncated Xbox source rectangle into the selected physical rectangle;
screen geometry continues to use the original source width/height and scale.
Without a selected rectangle, its float conversions and division order match
the original UV expressions. No generated upscale or font regeneration is used.

Actual enlarged fonts include14 private glyphs wider than128 pixels, up to250.
The loader therefore accepts width<=256 and height<=128, and decoding is bounded
to256x128 pixels. U+E054 in every enlarged family also has an actual4627-byte
stream, so encoded glyph size is bounded to8192 bytes. The existing16MiB file
and1024x4096 decoded atlas limits remain. Combined atlases are bounded to4096x4096; actual pairs use
width2049 and maximum height1134.

## Reviewed controller sources

Only the five exact source-file fingerprint pairs below admit private glyphs.
The FNV-1a64 identifiers are conservative known-asset selectors, not security
digests. The independent audit also records each file's SHA-256, every decoded
private glyph's RGBA SHA-256, dimensions and source metrics:
`analysis/frontend/hd-font-sampling-20261009/private-symbols.json`.

| Family | Xbox FNV-1a64 | Sampled FNV-1a64 |
| --- | --- | --- |
| Conduit12 | `54fdb36be14f28e1` | `a777e2206cc7b9c5` |
| Conduit13 | `f786d72c76a5daf8` | `723ed89e049433c4` |
| Handel11 | `c61f89b160203bd0` | `7827a218a933f3cf` |
| Handel13 | `5e6ba74015b7d73a` | `20f28e7709320621` |
| Handel24 | `894ba7d1a49dfd19` | `22c8a50b84f1e701` |

U+E100..E103 retain the A/B/X/Y symbols in all five actual decoded families.
U+E104/E105 deliberately replace Xbox black/white with the PC LB/RB symbols at
the user's explicit request. Their actual sampled rectangles are60x36; the
original logical rectangles and spacing remain authoritative.

U+E106/E107 retain the left/right trigger images, U+E108..E10B the four direction
triangles, U+E10C/E10D the Start/right-arrow and Back/left-arrow images, and
U+E10E..E111 the four stick images. Start/Back are directly reviewed in all five
families in `start-back-five-families.png`; PC uses different white-background
artwork while preserving the arrow symbols. Their sampled widths/streams match
across all five families. The original Start/Back widths/streams match across four;
Handel11 differs, and that difference remains recorded in the audit rather than
being described as byte-equivalent.

The ten trigger/direction/stick controls were visually
reviewed in Conduit12; each family's original width/encoded stream and sampled
width/encoded stream is identical to its corresponding Conduit12 source. The
audit records all50 exact pair checks. Handel11's original ABXY stream also differs,
so its symbol correspondence comes from the directly reviewed five-family image,
not an incorrect byte-equality assertion. Sampled ABXY streams match across all
five families. Every other private symbol remains original. Unknown file
fingerprint pairs may select ordinary Unicode rows, but cannot select any private
glyph.

## Native integration and validation

`FrontendPainter` loads the original eleven-role font table. When an optional
UI pack includes `fonts/<original-name>`, it applies the separate sampled atlas
before creating that font's existing material. Duplicate table roles retain
their shared material. The installed launchers automatically select
`assets/menu-ui/vista`; an explicit `--menu-ui-assets ""` restores original
bitmap and font artwork. Five exact compiled source files and their SHA-256
identities are installed with `font-provenance.json`.

The final eighteen-control adapter passed twelve focused tests. Its actual
source pairs provide1398 enlarged glyph rows. The painter comparison covers
all150 authored panes,156 draws,305844 sampled font vertices and the mouse hit
grid. Every logical vertex field, material index, clipped screen rectangle,
model/backdrop queue and original metric remains unchanged.

Native captures and command/hash records are retained under
`analysis/frontend/hd-font-sampling-20261009/`. The first ten stock/sampled
captures cover title, main menu, keyboard and Appearance at16:9, plus Settings
at32:9. Original-font controls for title, Settings and Appearance match the
earlier texture-pack captures pixel-for-pixel. The final Start/Back addition
has a separate `native-final/runs.json`: both Settings32:9 captures and the
original-font keyboard remain identical to the preceding captures. Its only
keyboard difference is the Start prompt inside pixel bounds(868,538)–(893,564).
All these GPU runs used the retained Vulkan validation layer with no reported
validation errors. They validate this sampling boundary, not whole-frame Xbox
parity or implemented Vista navigation.

The audit generator is
`analysis/frontend/hd-font-sampling-20261009/audit-private-glyphs.py`. It reads actual
decoded pixels and creates `controller-symbols-five-families.png`,
`start-back-five-families.png` and `other-private-symbols-conduit12.png`. Presentation magnification in those evidence
images does not modify runtime source pixels.

## Focused checks

Register `src/frontend_font_sampling.cpp` in `halo2_cache`, and link
`tests/frontend_font_sampling_tests.cpp` against `halo2_cache`. The test runs
synthetic immutable-layout/pixel-copy, partial logical crop, alias, missing
provenance and malformed bounds controls without arguments. With two directory
arguments it additionally checks all five actual enlarged families, over64,000
retained aliases per family, exact admitted controller source rectangles,
user-selected LB/RB, unchanged metrics/kerning/lookup records, byte-identical
Fixedsys9/Handel14 controls, mutated width257/height129/encoded-size8193 rejections,
and physical source-stream truncation rejection:

```sh
frontend_font_sampling_tests assets/disc/maps/fonts \
  analysis/frontend/vista-asset-inventory-20261009/extracted/maps/fonts
```

At source freeze, only the independent CPU asset audit has run. Compilation,
focused native tests, painter integration, screenshots and executable delivery
are separate validation steps owned by the parent task.
