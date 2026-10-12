# Archived vector redraw of the complete HUD

This experiment was canceled in favor of the [direct MCC Classic HUD import](MCC_CLASSIC_HUD_IMPORT.md),
which is selected by the normal launchers. The archived experiment is a design-faithful vector redraw, following the
clarification that source pixel identity is not the objective. It contains
editable SVG cubic outlines traced directly from the supplied artwork and
lossless DDS rasters produced **by reading those saved SVGs**. No image generation
or invented visual motifs are used. The earlier integer enlargement remains
under `assets/hud-4k-trial` as an archived comparison; it is not selected by the
preview launcher.

The vector pack is `assets/hud-4k-vectors`. Coverage includes all 70 HUD bitmap
banks and all 328 pages found across the 30 supplied original Xbox map caches,
plus all 98 scoreboard pages. This includes every ammo type, grenade icons,
98 crosshair/lock/weak-spot pages, full/split/dual-hand variants, scopes,
backpack weapons, vehicles, waypoints, objectives, training indicators,
radar, shields, health indicators, score meters and text.

All eight font families in the active original English table are covered.
Every one of the 2,894 original authored records, including private symbols
and fallback aliases, receives a vector and a higher-density native glyph.
All 2,949 authored records in the complete donor fonts are additionally traced
and retained. Compatible Vista text and previously reviewed control artwork
supply the improved outlines where available; original Xbox artwork supplies
fallbacks and other private glyphs. Original lookup tables, kerning, advances,
bearings and logical text rectangles are retained. Other language font tables
are not selected by this runtime.

## Tracing and rasterization

- Native decoding supplies the source RGBA planes, including Xbox row pitch,
  swizzling and original integer BC decoding. RGB in transparent HUD texels is
  meaningful shader data and is processed independently of alpha.
- Potrace traces twice-source threshold masks into closed cubic Bezier contours,
  retaining all components and holes (`turdsize=0`), with corner threshold 0.8
  and optimization tolerance 0.15 at that sampling density. This is a traced
  redraw of source designs, not recovered original vector masters.
- Every distinct grayscale coverage interval is represented. Independent
  coverage integration avoids visible contour banding. Font alpha uses the
  source ink outline with freshly rasterized antialiasing; glyph paint extends
  beneath transparent edges to preserve clean filtering.
- The original shader templates identify legacy ammo/meter banks. Their blue
  selector fields are exact vector rectangle regions, preserving original
  bullet thresholds. Visible shape and opacity contours are redrawn.
- SVG groups contain red, green, blue and alpha separately. Alpha is displayed
  by default. `data-weight` records each coverage interval; use the included
  rasterizer for exact packed-channel output rather than flattening the groups
  in an ordinary SVG viewer. There are no embedded raster images in the SVGs.
- HUD images are four times compatible Vista density, or eight times Xbox
  density where no Vista donor exists. Scoreboard images are twice Vista
  density, already well beyond their 4K display footprint. Text uses three
  times Vista density or six times Xbox density. These multipliers exceed the
  Vista variants while preserving logical layout.
- Native vector fonts use version `0xf0000003` and an `H2VT` footer containing
  density and original donor identity. They carry the actual vector-rasterized
  RGBA glyph payload, avoiding the old 16-bit RLE size ceiling. Ordinary retail
  version1 decoding remains bounded separately. The identity is a provenance
  selector, not a cryptographic authenticity guarantee.
- The opt-in radar compositor renders at 256x256 with original coordinates,
  snapping, shader definition and RGB-only destination write semantics.

## Build and preview

The ordinary game launchers are unchanged. `Halo2-HUD-4K-Preview` selects the
isolated HUD runtime, vector pack and registered Xbox files at 3840x2160.

```sh
python3 tools/build_hud_4k_trial.py --base-build build-armor-framebuffer-20261012
python3 tools/reconstruct_hud_4k.py \
  --vista assets/imported/a13438f3719c48bcb914624ed35022c7/vista
python3 tools/verify_hud_4k.py assets/hud-4k-vectors
python3 tools/capture_hud_4k_trial.py
./Halo2-HUD-4K-Preview --launcher-info
./Halo2-HUD-4K-Preview
```

To rasterize an edited channel SVG again:

```sh
python3 tools/hud_vector_trace.py path/to/element.svg /tmp/element.png
```

The isolated build retains compatible source in
`analysis/multiplayer/hud-4k/verified-source`, and frozen base objects and headers
in `build-hud-4k-trial`. Its input hashes are recorded in `build-provenance.json`.
This avoids combining concurrent gameplay class changes with old objects.
The native vector font support is also present in the current source tree.

## Verification

`assets/hud-4k-vectors/reconstruction.json` records every source cache/donor hash,
original bank/sequence layout, frame, vector, output hash and physical extent.
`tools/verify_hud_4k.py` independently replays every active texture and glyph SVG,
requires byte-identical output rasters, checks that SVGs contain no embedded
images, and verifies all font layout fields against the original source.
It measures source-relative half-coverage outline deviation and checks every
legacy meter's discrete blue selector field exactly.

Native checks cover full texture coverage, original logical text geometry,
private and fallback glyph selection, all weapon/vehicle reticle bindings,
SD/HD/4K HUD positions, both appearances, one through four views, shot/reload/
death snapshots, shield alignment, radar pulse and transparency. Matched port
captures and logs are saved under `analysis/multiplayer/hud-4k-vector`.
Host Vulkan captures demonstrate the port's rendering, not physical TV testing,
original-game framebuffer identity, or complete gameplay for every HUD state.
