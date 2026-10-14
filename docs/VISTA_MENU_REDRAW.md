# Vista menu vector redraws

This is an incomplete reconstruction of the Vista menu artwork, authored as
editable SVG paths, shapes, strokes, patterns and gradients. The original Vista
bitmap pages are the design references. There is no image generation,
autotracing, embedded raster artwork or substituted font in the SVGs.

The subsequent [MCC source audit](MCC_VISTA_MENU_ASSET_AUDIT.md) found 558
byte-identical Vista menu pages and three near-identical server-icon pages in
the user's installed MCC files, excluding emblems and button prompts. Matching
assets now belong to the separate local MCC importer work. Further authoring
here focuses on the 13 pages without a confirmed MCC counterpart: the fourth
server-icon state and twelve executable-owned loading pages. The historical
redraw counts below remain unchanged; source availability does not turn an
unfinished vector study into a completed recreation.

The current source package is [`resources/ui-menu-masters`](../resources/ui-menu-masters).
It contains **408 SVG masters and their lossless high-resolution DDS bitmaps**,
including the title logo, keyboard keys, list materials, playback controls,
browser status symbols, speaker icons, several menu frames, and all 50 rank
designs in both normal and small banks, Live status symbols, download animation
frames, password fields, matchmaking numerals, menu wave and hover materials,
navigation tabs, timing rails, difficulty callout lines, reticle symbols,
server-lock icons, grids, divider panels, polished corner frames, dialog glow
pages, branded lobby rows, the decorated centre frame and additional list controls. The logo is
4096×512. Most pages use up to four times the original raster density, with
a 4096-pixel maximum texture edge. The newly authored small controls use 8×
density, including 448×448 normal Live symbols and 336×336 small symbols;
their extended number canvases retain their independent padding. The two ring sprites use 16× density
(448×416 normal and 272×272 small) to resolve the narrow rim and wall edges
above the 4.5× scale of a 2160p presentation of the 480-high frontend canvas.
Original logical canvases, padding, frame ordinals and
registration coordinates are recorded separately from physical resolution.

The complete reference inventory covers 862 unique decoded pages and 11,600
cache/bank/frame bindings. It includes all 41 Vista map caches plus the
executable-owned loading pages. Of these unique pages, 211 are still
unimplemented, including 45 paused emblem pages without drafts. That leaves
166 unfinished pages outside emblem work; 243 existing emblem drafts are set aside at the user's request
and excluded from this package. All authored pages retain
`authored-needs-review`, and both manifests retain `complete: false`.

The current logo pass corrects the A's opening and foot, the upper notch and
outer curve of the 2, the O's detached oval and extension, and the L's rising
tail and lower step. The bevel, lettering and newly drawn panel materials
remain subject to visual review. The rank illustrations also remain drafts;
photographic surface fidelity is not established by their vector replay. Passing a bitmap replay check does not prove
100 percent visual fidelity or complete menu coverage.

The manifest flags redistribution rights as requiring review. Faithful design
recreation is not treated as clearance to distribute the artwork. Original
Vista and MCC payloads, source fonts and reference photographs are excluded.

Earlier redraw studies for composite panels, controller/character illustrations
and map screenshots remain unfinished as authored artwork. The audit subsequently
verified their original Vista artwork in MCC, so they no longer require a new
replacement view or drawing for an MCC-dependent installation. Imported MCC fonts
and artwork use the separate [local import workflow](UI_ASSET_BUNDLE.md).

## Larger MCC rank references

The Halo 2 Classic `rank_icons` bank in MCC retains the original 28×26 pages.
MCC's separate frontend has 50 `LARGERANKICON` pages at **152×152** and 50
`SMALLRANKICON` pages at **52×52** in
`Data/UI/texturepacks/mainmenuandcampaigntexturepack`. The read-only reference
exporter retains their original BC3 payloads and source identity hashes.
Its binary field layout was checked against the installed files and the
[MCC Texture Pack Dumper reader](https://github.com/Lord-Zedd/MCCTexturePackDumper/blob/master/MCCTexturePackDumper/TexturePack.cs).

The seven celestial designs use these larger references alongside Vista.
MCC supplies observed crater groups and additional surface/cross-section
reference. Vista governs the blue-grey lunar palette, crescent and curved half-moon
boundaries, irregular gold coronas, blue comet streams, tilted ring lighting,
transparent backgrounds and original logical sizes. MCC's overlaid white
numerals and its solid eclipse/sun/comet backplates are not part of the redraw.
The numerals obscure portions of the larger moon references: these regions
are not represented as recovered high-resolution original detail.

Both normal and small celestial SVGs record the corresponding MCC payload
identities in `masters.json`, including the full-moon reference shared by the
three lunar phases. Rendering is independent of the source rasters. The
illustrations use editable curves, gradients, strokes and Gaussian softness;
they contain no raster image, autotrace, pixel cells or generated noise field.
The existing 118 non-rank masters are byte-identical to their earlier source
checkpoint. The half-moon uses a soft curved terminator measured from Vista
with an editable fine stipple material informed by the larger MCC surface.
The comet's overlapping streams feed a continuous bright core and head. The
Halo ring has separately drawn broad cylindrical surfaces and a narrow
structural end rim. The camera sees the far inward-facing blue/white and
warm surface on the lower/right arc, and the nearer exterior on the
upper/left arc. The end rim projects toward lower/right; the former ownership
of inner and outer surfaces was reversed. A single smooth material face
replaces the old repeated chrome highlight strips. Its camera-space
centreline, inclination and proportions were fitted to Vista's alpha support;
MCC provides the clearer band cross-section reference.

The original Halo 2 world-ring interior and exterior textures were inspected
separately in the local MCC installation. They confirm the land/water/cloud
habitat is inward-facing and the industrial shell is outside. The latest ring
pass retains the accepted camera, facing and cross-section, deepens the two
shadowed arcs against Vista, and replaces the broad interior colour patches
with manually outlined coasts, islands, inland colour regions and cloud banks
from the original 1024×64 `halo_inner_ring` texture. The coast paths use source
texture coordinates before projection onto the existing cylindrical face;
their lighting follows Vista's blue/white stretch and warm lower bend.

The rank icon's exact original render/UV phase has not been recovered. The
coast alignment is an authored placement into those visible Vista regions,
rather than a claim to have recovered that phase. No raster pixels are
embedded. Both ring masters record the habitat payload SHA-256
`5d7a8bfb3abaff2c0c2de5eeac0e88926ec57d1dda18636bf3817133694094ed`
alongside the MCC rank reference identity. The camera-facing construction is
available in `ring-facing-review.png` under the working analysis directory.
The comparison reads the actual generated DDS, including the denser ring
raster, rather than rendering a larger SVG only for the review image.


```sh
python3 tools/export_mcc_rank_reference.py --output /tmp/mcc-rank-reference
python3 tools/compare_vista_ranks.py --pack /tmp/vista-menu-draft --mcc-reference /tmp/mcc-rank-reference --output /tmp/rank-comparison.png
```

The current comparison is
`analysis/frontend/vista-menu-vectors/celestial-vista-mcc-vector.png`; a second
comparison with `celestial-small-vista-mcc-vector.png` checks the small bank.
Each shows Vista originals, the larger MCC references, and actual generated
DDS art at matching inspection sizes. Original MCC reference files remain
local analysis inputs and are excluded from the source master package.

## Shared menu controls

The shared-control pass adds 114 pages and preserves all 218 earlier SVG/DDS
pairs byte for byte. Each Live glyph is built from separately authored parts:
the tilted person, circled variants, orbit arrow, curved download arrow,
speaker and mute symbol, audio settings frame, pie symbol, party crown, and
controller with its inset guide button. Normal, small and compact list banks
keep their individual source canvases. The shield has its scalloped crest,
waisted sides, fuller lower body and separate quarter panels. Its latest
outline follows Vista's shoulder band and pointed lower end; both blue
quarters meet at the source's shared horizontal boundary.

The controller's broad shoulders, rounded grips and shallow inner arch use
the original Xbox Duke profile supplied by the user, with Vista controlling
the glyph's placement and colours. The source package records the photograph
identity `80b51966523efc5552429d37633bdf13ccd4172f4497df0d13fc02da3cf142e0`
without including the photograph. Every controller variant has a manually
drawn, curved Xbox X inside its circular guide badge. The party crown has
three pointed tips, including the taller centre, with an inward-curving lower
band, solid white face and the shared blue glow. The two outer tips lean
slightly outward; the centre tip remains fixed. Speaker, mute and audio-settings glyphs have a
semicircular rear cap joined directly to the horn contour in all three banks;
a thin blue line retains the visible mouth edge through the cap.

The twelve download states have individually outlined white fill fronts,
including their branching paths around the arrow's negative space. They do
not use a moving rectangle across the whole icon. Arrowheads, download fill
states, the crown and Duke silhouette use mirrored geometry.
`tools/verify_vista_menu_symmetry.py` checks 57 source-bound contours directly
from the SVGs. Raster measurements separately retain small Cairo filter and
coverage rounding differences; exact vector symmetry is not represented as
bitwise raster symmetry.

Progress numerals are manually outlined letterforms with the observed flat
sections and bowl shapes. They do not substitute a font. Table column gaps,
selection outlines, slot borders, padlocks and password fields retain their
native bounds and paint colours. The padlock has no invented keyhole.

```sh
python3 tools/compare_vista_menu_redraw.py --pack /tmp/vista-menu-draft --output /tmp/menu-review
python3 tools/verify_vista_menu_symmetry.py --pack /tmp/vista-menu-draft --report /tmp/menu-symmetry.json
```

The working comparison sheets are under
`analysis/frontend/vista-menu-vectors/new-families-review`. Their index pins
both reference PNG and generated DDS identities. These are still review
drafts; reconstruction of the remaining illustrations, panels, loading art,
map screenshots and full font resources remains open.

The continuation adds 40 source pages: menu-row waves and hover fields,
Live tabs and backgrounds, matchmaking arrows and segmented ring, team
columns and settings rows, large dialogs, six timing rails, browser/lobby
dividers, two grid materials, four difficulty callout pages, two reticles and
four server-status icons. Native empty padding and painted bounds are
retained. The timing rails preserve measured tick phrases and wider fourth
marks; the other tracks' alien lettering remains unfinished. The latest
DDS comparisons are under
`analysis/frontend/vista-menu-vectors/menu-continuation-20261013`.

The next 22 pages cover all 15 separate metal corner frames, four dialog
glow pages, the four-player sign-in divider, the Bungie news grid frame and
the loading sweep. Each corner retains separately drawn faces, detached
rails, notches, edge highlights and inward blue light. A single blue palette
anchor and two global material opacity weights are fitted against the source;
this does not change path geometry or store source pixel regions. The final
SVGs replay independently of the reference pages. These remain visual review
drafts. Current comparisons are under
`analysis/frontend/vista-menu-vectors/menu-framing-20261013`.

The next 14 pages complete the two lobby row banks and the larger decorated
centre frame. The row lettering uses individually outlined lowercase glyphs,
separate uppercase Bungie lettering and its curved arc. The original wide
space before `net`, star cadence, branding cadence and the two native row
placements are retained. The lettering is an alpha engraving into white row
material, rather than dark paint on an opaque background. The larger original
Vista Seventh Column reference resolves the row symbol's rounded corners;
the paused customization emblem drafts remain unchanged and excluded.
These new small rows use 8× density; their earlier uniform placeholder remains
unchanged. The centre frame retains its independent window/exterior alpha,
stepped upper and lower blue bands, traced facets, thin circuit lines and
repeating engraving. Interior reflection and exterior edge light are separate
paint layers. Its DDS is 4096×2606. Both families remain visual review drafts.
Current native-size band comparisons and larger row comparisons, with source
and final DDS hashes, are under
`analysis/frontend/vista-menu-vectors/menu-branding-20261013`.

The centre-frame refinement restores the long footer circuit line, corrects
the header diagonal and separate short/tall engraving cadences, and retraces
the overlapping lower facets. The rising diagonal remains continuous through
the horizontal shelf; the descending edge begins at that shelf rather than
forming a triangular peak at the upper border. The two faces and their overlap
use separately measured native contours and six small interior colour anchors.
The lower shelves retain their distinct source heights. Final DDS comparisons
and package preservation evidence are under
`analysis/frontend/vista-menu-vectors/menu-center-refinement-20261013`.
Only the centre-frame master changes in this refinement; the other 407 masters
and all paused emblem drafts remain unchanged.

The user-prioritized composite illustrations are next: the matchmaking diagrams,
unknown-map terrain pages, unknown/player Spartan profiles, variant skull and
flags, and three matchmaking-progress illustrations. The experimental skull-and-flags
recipe is isolated in `tools/vista_menu_illustrations_svg.py`; it is not admitted
by the menu renderer or included in the source package. The latest pass retraces
the right flag's inner edge, skull width, left crown prong, finials and staff
widths from native coordinate references. It separates open eyebrow contours
and tooth seams, clips the staffs behind the skull, and replaces broad vertical
cloth fields with diagonal folds and anatomical paint planes. Native contour
overlays and a standard SVG-to-2048-square DDS replay are under
`analysis/frontend/vista-menu-vectors/menu-illustrations-pass3-20261013`.
The fine bone material, some folds and ragged edges still fall short of the
reference. These diagnostics do not prove visual fidelity. All ten pages remain
explicitly unfinished; the 408 bundled masters are unchanged by this experiment.

The next illustration pass adds a separate experimental player-profile Spartan
recipe in `tools/vista_menu_spartan_svg.py`. It retains the 512x1024 logical
canvas and its empty margins, traces the armor and rifle as separate components,
and uses foreground plate masks to hide covered contours. Native cross-sections
guide the helmet, shoulder and right-leg width corrections. The 2048x4096 DDS
exactly replays from the SVG through the standard renderer. The visor and near
arm include preliminary reflection paint traced against native material crops.
Inner armor contours, occlusion, and surface detail still need refinement; this
recipe is not connected to the reconstruction dispatcher or source package.
The skull pass retains the refined eye-socket ridges; the coarse bone-pit study
was rejected and removed from the active recipe. Comparisons and verification
are under `analysis/frontend/vista-menu-vectors/menu-illustrations-pass4-20261013`.

Every portrait and character composite must preserve the original's material
detail through new authored surfaces. Flat silhouettes and smooth color fills
are not a finished replacement for textured armor, visor reflections, wear,
cloth folds, or bone shading. The current source review identifies 18 character
portrait/composite pages and three related bone illustrations; their individual
texture states remain unfinished in
`analysis/frontend/vista-menu-vectors/menu-illustrations-pass5-20261013/portrait-material-inventory.json`.

The new `tools/vista_menu_surface_svg.py` recreates fine grain with a deterministic
ordinary SVG material filter. It does not embed, upscale, trace into pixel
regions, or sample a grid from original texture pixels. Native material-patch
statistics guide its scale and contrast; larger reflections and wear are
separate authored paths. The pass-five profile fits only 24 shared paint anchors,
without fitting geometry, alpha, grain phase, or blur. The helmet and chest have
been retraced directly in native coordinates instead of using the initial width
adjustments. The skull and flags use separate bone and cloth grain strengths;
the source cloth is much smoother than armor, so its added grain is very faint.

The pass-five drafts replay exactly to 2048x4096 and 2048x2048 DDS files. Turning
grain off leaves their rendered alpha exactly unchanged, including transparent
margins. These checks and material statistics do not prove visual fidelity.
Larger surface features, inner contours and the remaining portraits still need
work; neither draft is admitted to the 408-master source package.

Pass six separates the profile's open chest and collar grooves from closed
paint boundaries, corrects the near forearm's wrap around the elbow, and puts
that arm in front of the torso. New authored surface paths describe the collar
depth, bracer underlip, forearm wear and specular facet. The forward-facing
weapon now has observed recesses and irregular reflections rather than generic
concentric rings. The leg plates add separate broad reflection fields, joint
ribs, boot cavities and wear; the calf's dark inner face remains distinct from
its narrow bright outer face. These are coherent material features traced in
native coordinates, followed by independently generated fine grain.

The source-only pose comparison identifies the profile pose in the settings
page, three-figure variant page and several Xbox Live menu composites. It is
layout evidence for eventual reuse of an authored master, not a pixel or
contour extraction method. Its numerical check rejects empty windows and
verifies a normalized self-match. All placements and each composite's other
figures, shading and glow still require individual review.

The pass-six drafts replay exactly to DDS through the standard SVG renderer;
removing their grain changes no alpha or transparent margin pixels. Source
hashes, comparisons, native material references and the portrait surface plan
are under `analysis/frontend/vista-menu-vectors/menu-illustrations-pass6-20261013`.
The 18 identified character portrait/composite pages and three related bone
illustrations remain unfinished. This work does not establish visual fidelity
or rights clearance, and does not change the 408 bundled masters.

Pass seven retraces both legs and the pelvis directly in native coordinates,
removing their initial width transforms. Hidden paint closures no longer draw
extra blue polygons: the thigh ridges, hip grooves, knee plates and boot seams
use separately authored visible curves. The right boot now has a rounded toe
and a recessed opening. Updated material paths place the knee fasteners, calf
depth, narrow shin reflection and thigh-slot cavities against the native source.
The profile remains experimental and outside the reconstruction dispatcher.

Native contour references, comparisons and exact raster replay evidence are in
`analysis/frontend/vista-menu-vectors/menu-illustrations-pass7-20261013`.
The line-distance diagnostic compares passes six and seven against the original;
it is separate from the broad filled-silhouette diagnostic, which can miss
incorrect inner seams. A threshold above the body material's alpha includes
thin antialiased linework. Neither distance statistics nor successful DDS
replay establish visual fidelity. Upper-body contours and larger material
features still need refinement, and the 408-master package remains unchanged.

Pass eight retraces the shoulders, chest pockets, upper arm, far forearm and
waist in native coordinates. This removes the remaining initial part offsets.
Visible curves now describe the shoulder opening, paired arm rims and recessed
arm slot independently of the paint closures. Upper-arm and hip-insert ordering
keeps those surfaces under their foreground casing.

Material groups now have visibility masks made from the authored component
geometry. Reflections and wear cannot paint over a later foreground part.
Six isolated raster coverage cases check that fully opaque foreground interiors
receive zero rear material alpha; partially covered antialiased boundary pixels
are recorded separately. The new arm reflections, chest depth, raised wear plane
and forearm under-bracer shadow come from native material references. A separate
variation diagnostic compares fine, medium and broad material features without
copying any source field or texture pixels. It also records remaining differences:
fine grain alone is insufficient to recreate the larger surface variation.

Current comparisons, coverage checks, exact SVG/DDS replay, transparent-margin
checks and source-package preservation evidence are under
`analysis/frontend/vista-menu-vectors/menu-illustrations-pass8-20261013`.
The portrait remains unadmitted. Hand, weapon and helmet details, broader
material variation, and the other portrait/composite pages remain unfinished.

Pass nine retraces the helmet cap rails, visor boundary, jaw, temple fittings,
glove/thumb folds and forward-facing weapon curves. Recreated temple cavities,
helmet wear and glove shading remain ordinary authored SVG paint. The original
reference is too small to establish additional subpixel mechanical detail, so
the redraw does not claim such detail has been recovered.

The selected profile palette now shares three color anchors across each of 56
authored anatomical/material groups (168 color anchors total). Only stable
material interiors participate in that fit; the paths, alpha, blur, wear shapes
and independent grain remain fixed. The earlier unrestricted regional trial
brightened the chest and was rejected. The interior fit reduces RGB error in
all eight reviewed regions while retaining exactly the same SVG structure
apart from gradient colors and exactly the same physical alpha. This is color
evidence, not proof of complete visual fidelity. The renderer's interior-fit
option is used only by the experimental profile recipe.

The selected art, previous palette comparisons, nine material-coverage cases
and current SVG/DDS replay evidence are in
`analysis/frontend/vista-menu-vectors/menu-illustrations-pass9-20261013`.
A read-only inspection of the installed MCC Classic menu banks found the same
listed portrait canvas sizes, including 512x1024 for this profile; it did not
provide a larger profile donor. No MCC art was imported or installed by this
inspection. Fine contours and broader material variation still need work.
All 18 identified character/composite pages remain unfinished, and the
408-master source package is unchanged.

Pass ten adds the source-observed far-forearm fold, worn ridge, wider bracer-side
reflection and thigh patina. Armor now combines independent fine and medium
SVG grain scales; neither field is copied from source noise or fitted to its
phase. A frozen-palette study separates the authored paint changes from the
new grain, and the selected recipe refits only its existing anatomical color
anchors. The source's remaining fine and broad variation differences are kept
in the diagnostics rather than treated as completed texture fidelity.

The two current portrait studies are also bundled in
`resources/ui-menu-portrait-studies`: the player-profile Spartan and variant
skull/flags. This is an unfinished review package with editable SVG masters,
their DDS rasterizations, source identity metadata and rights-review flags.
The original reference rasters and imported artwork are not included. Both
pages remain unadmitted and in the unfinished inventory; the canonical
408-master package and reconstruction checkpoint are unchanged. This package
does not install or activate itself.

```sh
python3 tools/vista_menu_masters.py verify --package resources/ui-menu-portrait-studies
python3 tools/vista_menu_masters.py rasterize --package resources/ui-menu-portrait-studies --output /tmp/vista-portrait-study-replay
```

Exact standard SVG/DDS replay and an additional rebuild from a separate working
directory verify that the study sources replay without original reference
input arguments. Nine material-ownership cases and grain on/off alpha checks
remain separate from fidelity review. The skull/flags SVG and DDS are unchanged
by the new optional armor grain. Current comparisons and preservation evidence
are in `analysis/frontend/vista-menu-vectors/menu-illustrations-pass10-20261013`.

## Vista-only loading studies

`tools/vista_loading_svg.py` now authors the thin and thick mechanical rings from
Vista executable loading pages 3 and 4. Their native 512-square canvases retain
opaque black backgrounds. Circular bands, independently centred inner arcs,
radial joints, stepped plate edges, channels and fastener bores are separate
editable vector geometry. The measured centres and radii describe a small number
of complete circular primitives; no source contour or pixel-region grid is
serialized into the artwork. Adjacent plate edges share one fill to avoid
unintended antialias seams.

The separate source bundle is
[`resources/ui-vista-loading-studies`](../resources/ui-vista-loading-studies).
It contains five SVGs, two 4096×4096 ring DDS files, one 4096×8 glint DDS,
4096×64 and 4096×128 tick-strip DDS files, and a source-identity manifest.
All five pages remain experimental, unadmitted and unfinished; the
canonical 408-master package and all paused emblem artwork are preserved.
Small rim/joint/edge differences are still visible in the native comparison.
The replay checks below establish reproducibility, not complete visual fidelity.

```sh
python3 tools/vista_menu_masters.py verify --package resources/ui-vista-loading-studies
python3 tools/vista_menu_masters.py rasterize --package resources/ui-vista-loading-studies --output /tmp/vista-loading-studies-replay
```

The horizontal glint uses one sixteen-stop vector paint ramp and a separate
flat grey tail, preserving the original four identical scanlines. Its native
comparison has an RGB mean error of 0.213/255 and a maximum difference of 7/255;
these diagnostics do not establish final visual fidelity. Its opaque alpha and
flat tail match the reference exactly. Adding it preserved both ring SVG/DDS
pairs byte-for-byte.

`tools/vista_loading_marks_svg.py` adds the spaced and dense marker strips from
loading pages 9 and 10. Their 88 and 183 repeated marks are authored rectangles
with shared paint and optical softness; thirteen and eight short run records
preserve the measured grouped gaps. No source font, pixel row or raster mask is
embedded. Native visible bounds match the references after reducing the actual
4096-wide rasterizations. On visible pixels, alpha mean errors are 2.085/255
and 1.067/255; maximum alpha differences are 9/255 and 6/255. These are fidelity
diagnostics, not completion proof. The original loading extents, 1044×31 and
1044×52, remain separate from their power-of-two reference/raster canvases in
the manifest. The three earlier loading SVG/DDS pairs were preserved exactly.

The current reference comparisons and measurements are under
`analysis/frontend/vista-menu-vectors/vista-only-pass3-20261013` and
`analysis/frontend/vista-menu-vectors/vista-only-pass4-20261013`. The other seven
loading pages are still undrafted; the fourth server-icon state retains its
earlier review master. This batch does not activate a runtime texture pack.

The lettering references are not all full glyphs: loading page 8 intentionally
clips through the letters. Its visible partial shapes and crop must be retained;
the missing portions are not evidence for a guessed replacement typeface.

An additional source relationship is confirmed for loading page 1: its full RGB
image is byte-identical to MCC's `start_screen` logo page, while alpha differs.
This supplies the original colour artwork through MCC, but no exact alpha
conversion is recovered. See the source audit's loading-logo note before
attempting to treat it as an exact loading-page import.

## Verification and regeneration

The source package is self-contained for raster replay. It contains the new
SVG artwork and generated DDS files, with source identity hashes and bindings
in `masters.json`. Original reference rasters and game files are not bundled.
The package does not install itself or change launcher selections.

```sh
python3 tools/vista_menu_masters.py verify
python3 tools/vista_menu_masters.py rasterize --output /tmp/vista-menu-raster-replay
```

Verification checks every file's hash, rejects embedded images, external
references and SVG text/font substitution, caps each gradient at 16 stops, and
replays every DDS using ordinary `rsvg-convert`. The PNG preview and DDS output
are rasterizations of the same authored SVG. Python needs NumPy, Pillow and
SciPy; raster replay also needs `rsvg-convert`.

To collect reference pages from user-supplied extracted Vista files and work
on a separate draft:

```sh
python3 tools/vista_menu_reference.py --vista /path/to/vista --output /tmp/vista-menu-draft
python3 tools/recreate_vista_menus.py --pack /tmp/vista-menu-draft
python3 tools/verify_vista_menus.py --pack /tmp/vista-menu-draft
```

While emblem work is paused, regenerate an existing draft with
`--skip-emblems`. This preserves all prior emblem SVG/DDS hashes and omissions
and requires an existing reconstruction manifest. The current working copy
is isolated under `analysis/frontend/vista-menu-vectors/pack` after a separate
asset-cache reset; it does not restore the reset asset selections.

The build emits a stable `keyboard-comparison.png`, `logo-comparison.png` and
`logo-details-comparison.png`, as well as numbered comparison sheets. Originals
and redraws are shown at the same visual size. Comparison rasters are review
evidence only; they never enter the replacement SVG or DDS artwork.

To package an authored checkpoint, choose a fresh destination:

```sh
python3 tools/vista_menu_masters.py export --pack /tmp/vista-menu-draft --output /tmp/vista-menu-masters-new
python3 tools/vista_menu_masters.py verify --package /tmp/vista-menu-masters-new
```

The exporter preserves source references by identity and leaves existing
master packages intact. It excludes paused emblem artwork. New recipes live
in `tools/vista_menu_svg.py`, `tools/vista_wordmark_svg.py`,
`tools/vista_menu_panels_svg.py`, `tools/vista_menu_status_svg.py` and
`tools/vista_menu_framing_svg.py`, `tools/vista_menu_rank_svg.py` and
`tools/vista_menu_celestial_svg.py`. The manually outlined ring habitat is in
`tools/vista_ring_habitat_svg.py`. Shared controls are in
`tools/vista_menu_live_svg.py` and `tools/vista_menu_controls_svg.py`.
Row/tab materials, timing rails and auxiliary line drawings are in
`tools/vista_menu_materials_svg.py`, `tools/vista_menu_tracks_svg.py` and
`tools/vista_menu_aux_svg.py`.
Polished corner faces and light-only panels are in
`tools/vista_menu_corner_svg.py` and `tools/vista_menu_lightpanels_svg.py`.
Lobby branding and the decorated centre frame are in
`tools/vista_menu_branding_svg.py` and `tools/vista_menu_center_svg.py`.
