# Vista button prompts at 4K

The active MCC HUD and menu packs use a separate, optional Vista control overlay
from `assets/vista-prompts-svg-4k`. Its artwork is drawn from scratch as ordinary
SVG shapes, outlined letters, bevels, reflections and gradients, then rendered
directly by librsvg into lossless RGBA textures. No image generation, embedded
bitmap, replacement typeface or pixel-region tracing supplies the artwork.

The source bundle now includes these masters, textures and native reference
fonts in `resources/ui/vista-prompts-svg-4k`. The bundled installer selects a
verified runtime copy under `assets/bundled-ui`; see [bundled UI assets](UI_ASSET_BUNDLE.md).

The supplied Vista English font table is the reference for each selector's
actual design. A/B/X/Y use real circles under uniform scaling. Their outlined
letters retain the distinct shapes and enclosed spaces. LB/RB have dark blue
lettering, and Start/Back have smaller, filled triangles within rounded white
buttons. Paint anchors are fitted to the reference colors; geometric shapes
remain compact, editable vector drawings.

The pack contains 331 selector/control bindings across all twelve Vista font
selectors, 82 distinct SVG masters and six lossless RGBA DDS atlases. Physical
dimensions are four times the actual Vista glyph width and height, giving
sixteen times the pixel area; typical colored buttons are 128×144 pixels.
Logical glyph dimensions, spacing, kerning and clipping remain source-owned.

Included controls are A/B/X/Y, shoulders, triggers, directional arrows,
Start/Back, stick controls, keyboard frames and visible special controls in
E100–E111 and E12C–E13C. Each font's real variant is retained, including the
legacy monochrome controls. E13C is a blue-and-gold padlock in one font and a
four-pane symbol in others. Empty glyphs and fallback aliases are excluded.
Ordinary text and imported MCC HUD/menu artwork retain their existing samples.

`tools/vista_prompt_source.py` decodes the native Vista glyph RLE for reference
and provenance. `tools/vista_prompt_svg.py` authors the geometry and paint.
`tools/recreate_vista_prompts.py` saves SVG masters, renders them with
`rsvg-convert`, and packs their bitmaps. Each drawing has at most 24 vector
shapes. The SVGs have no raster images, external fonts, custom channel data or
filters. Both librsvg and Inkscape render the standalone composition
`assets/vista-prompts-svg-4k/buttons.svg` as full-color artwork.

`prompt-source.tsv` in each selected MCC pack binds the overlay. The original
font files and MCC imports remain intact. Menus, pause menus, HUD text,
scoreboards and pickup/interact buttons apply the optional overlay after font
selection. The pickup path now applies it to the dedicated Vista button atlas
as well, retaining the newer interaction behavior and Vista logical metrics.

Recreate and activate:

```sh
python3 tools/recreate_vista_prompts.py --activate
python3 tools/verify_vista_prompts.py assets/vista-prompts-svg-4k \
  --output analysis/multiplayer/vista-prompts-svg/vector-verification.json
```

Use `--vista` to select the source pack and `--output` for a separate destination.
Reimporting an MCC pack may replace its binding; rerun activation afterward.
Removing its `prompt-source.tsv` restores that pack's ordinary control samples.
The previous `assets/vista-prompts-4k` pack is retained for comparison.

Verification checks font identity, actual codepoint ownership, vector hashes,
fourfold dimensions, lossless atlas contents and exact saved-SVG raster replay
for all 331 bindings. It also checks circular A/B/X/Y bodies, uniform scaling,
dark shoulder labels, bounded bright letter cores and the A/B counters.
Source-pixel contour and RGB deviations are recorded as measurements, rather
than acceptance criteria for copying low-resolution pixels. These are
reference-led redraws, not recovered original vector masters; pixel identity
and complete retail framebuffer parity are not claimed.

Native sampling tests check logical metrics, lookup, kerning, clipped UVs,
ordinary MCC samples and retained atlas pixels. The interaction regression
checks the dedicated HUD button atlas and actual selected sample UVs with and
without the overlay. Focused tests, independent SVG rendering, 4K native
captures and pinned build/source identities are retained in
`analysis/multiplayer/vista-prompts-svg`.

The delivered runtime is an isolated relink of the latest verified interaction
build. It replaces only the HUD translation unit in the systems archive;
391 other members are preserved byte for byte. Both launchers are rebuilt
against that runtime. `build-provenance.json` records the exact inputs and
commands, and `delivery.json` pins the final assets, binaries and evidence.
