# Widescreen menu background extension

The user explicitly requested extending the menu background/frame art through
32:9 while preserving the centered buttons, text and previews. This is a native
presentation extension, separate from original 640x480 framebuffer parity.

The original `mainmenu.map` definitions separate the decorative shell from
foreground widgets. Screen5 (`game_shell_background`) uses the 1024x512
`ui\global_bitmaps\framing_center` page, datum `E5350315`, plus twelve global
`track*` pages. The timeline is datum `E5360316`, 1024x16. Screen9/title and
screen6/main use global track strips separately from their `start_screen` Halo
logo. Small dialog panels, virtual keyboard keys/field, map art, emblems, and
model previews are different definitions and retain the ordinary viewport.

The user's subsequent instruction explicitly preserves the top/bottom frame
center and extends only its outer edges. `FrontendPainter` therefore leaves the
original `framing_center` texture and six vertices, colors and UVs unchanged.
Two independent texture/material pairs carry `UiFrameExtension::Left/Right`; their
quads retain the original Y coordinates, height and opacity, with constant U
at the nearest inside texel center of the authored quad's visible canvas
boundary x=0 or640. The page is wider than640 pixels, so the right extension
uses that visible boundary rather than the offscreen right texture edge.
An edge copy is admitted only while the
animated authored quad actually touches that boundary. The renderer places
these copies in the isolated left/right margin viewports and skips empty
margins. It does not stretch the original frame center.

The independent strips and center share `frontend_frame_sampling.hpp` for
their optional inward U limits. Only the original center material has
`ui_frame_center`; independent strips, other shell art and foreground widgets
do not. The correction admits a dedicated center fragment route only when
horizontal margins exist. It limits sampling at those same inward boundaries,
leaving the authored vertices, UV interpolation and all interior samples
unchanged. This removes transparent page-padding filtering at the join rather
than moving or stretching the frame. The ordinary4:3 route and narrower
windows without horizontal margins retain their original sampling. The CPU
helper supports translated/scaled UV input with explicit increasing-U
validation; the actual source frame has x=0,width1024,U[0,1] throughout its
alpha-only intro/outro. Fresh GPU acceptance of this correction is recorded
separately from its CPU source/geometry proof.

Global track pages and the separate generic dim material retain the full-width
`ui_widescreen_background` policy. Foreground vertices, dialog panel sizes,
model-view rectangles and the 640x480 inverse mouse mapping stay centered.
No recreated or Anniversary artwork is introduced. These are explicitly
requested native presentation extensions, not source-authored ultrawide parity.
The full-frame 3D world projection uses a wider scenery view rather than
stretching the centered image. It scales only the original projection's X/Y
rows by the fitted canvas/drawable ratios, preserving depth and camera basis.
The original4:3 camera bank and shader constants remain byte-identical. The
foreground UI and model-preview viewports stay centered independently.

The explicit frontend popup backdrop also covers the expanded rectangle. Its
tint and blur include the side margins. The blur source width is expressed in
logical pixels (`drawable_width / foreground_scale`), preserving physical tap
spacing. Other `UiBackdrop` callers default to the existing centered policy.
Windows narrower than4:3 preserve the content canvas's top/bottom margins;
this change extends horizontal backgrounds only.

`frontend_canvas.hpp` is the shared pure viewport calculation used by the
renderer and regression tests. Tests cover 4:3, 16:9, 16:10, 21:9, 32:9,
1000x600 resize dimensions and5:4; they check content/mouse coordinate
invariance, full-width background sampling, unchanged physical blur offsets,
the actual shell material admission, an unchanged centered logo, and the
ordinary title→profile→keyboard backdrop flow. A second real painter with only
the frame policy disabled is the control for exact authored vertex/UV/color
preservation at0,50 and5000ms; edge strips are checked against its visible
boundary U, Y and opacity. Build/CPU checks passed; native captures are recorded
separately from these geometry and storage assertions.

`menu_projection_tests.cpp` retains all original binary instruction fixtures
and adds4:3 bank/constant identity plus5:4,16:9,16:10,21:9 and32:9 projection
coverage. It checks explicit X/Y scales, unchanged depth/basis and centered
world-point pixel invariance over three lenses, three orientations and six
depths. This verifies projection geometry; it does not establish complete
native scene rendering or framebuffer parity.

The left and right extension images also have independent replaceable files:
`<maps>/ui/frame_edges/left-edge.dds` and
`<maps>/ui/frame_edges/right-edge.dds`. Their material identities are
`ui\global_bitmaps\framing_center/native-left-edge` and
`ui\global_bitmaps\framing_center/native-right-edge`. Each has its own CPU
texture storage and Vulkan image; neither aliases the original center texture.
The frontend loads each optional file independently at startup. Missing files
use separate stock copies, and the painter never writes into game assets.

Generate reviewable stock files from the user's original map with:

```sh
build-frontend/export_frontend_frame_edges \
  --map assets/disc/maps/mainmenu.map \
  --output assets/disc/maps/ui/frame_edges
```

The exporter writes only missing files and preserves existing replacements.
Both defaults initially contain the full original1024x512 BC2/DXT3 page with
unchanged compressed payload bytes. They deliberately retain the source page
instead of cropping or recompressing it, preserving filtering at the sampled
boundary. The current strips sample constant U one half texel inside the
visible canvas edges: for a stock unscrolled quad starting at x=0 with
width1024, left U=0.5/1024 and right U=639.5/1024, rather than1. Both files
contain the top and bottom frame bars.
Edit either DDS independently and restart to replace only that side. Keep the
stock1024x512 dimensions, DXT3 compression and one mip level; the bounded
loader rejects different dimensions/formats, cubes, volumes, mip chains and
incomplete/trailing payloads. The original center and all widget geometry/UVs
are unaffected. No reference texture bytes are packaged into the source repo.

Frontend tests cover exact compressed DDS round-trip bytes/header, malformed
controls, absent overrides without writes, and real painter construction with
a changed left file and unchanged right file. They assert the original center
page remains unchanged and each side binds its own expected bytes. The rebuilt
frontend regression, the projection's3,402 world-point checks, and7,560 centered
haze/bloom sampling checks passed. Padded post-process mapping preserves the
original sampling grid when the wider target needs fractional coverage; see
`frontend-post-raster-extension.md`. Exported DDS payloads match the original
map's524,288 compressed bytes exactly, and a repeated export preserves both
files. Evidence is in
`analysis/frontend/widescreen-debug-20261009/frame-edge-assets.json`.

Verification should capture appearance47, a chooser popup, profile10 and
keyboard167 in the supported aspects and after resize. Check visible frame
coverage at both side edges, identical center content bounds, centered model
previews, mouse/controller selection, popup tint/blur across both margins,
and zero Vulkan validation errors. Existing old capture scripts that require
all pixels outside the4:3 canvas to remain black describe the previous policy
and must not be used as the acceptance criterion for this requested extension.

The first five appearance captures under
`analysis/frontend/widescreen-debug-20261009/native/` used the superseded
whole-frame stretch policy. They establish neither this corrected edge policy
nor its full-frame 3D world coverage; fresh captures are required.

The first four captures under `native-edge-extended/` are also retained as
diagnostic evidence, not final delivery. They sampled exact U boundaries,
which caused linear repeat filtering to mix a transparent padding texel with
visible artwork on both sides. At sourceY40, col0 has alpha187 and col1023
has alpha0, so U=0 samples alpha93.5. At sourceY450, col639 has alpha187 and
col640 has alpha0, so U=640/1024 also samples alpha93.5. Interior texel-center
sampling gives the full authored alpha187 without changing either DDS,
sampler, or original center quad. The evidence is retained in
`analysis/frontend/widescreen-debug-20261009/frame-edge-texel-center-proof.json`.
Fresh corrected captures use `native-edge-texel-center/`.

The corrected native launcher completed all eleven captures at fixed menu
tick 181 and diagnostic fog seed 305419896: appearance at 4:3, 5:4, 16:9, 16:10,
21:9 and 32:9; profile, keyboard, title and F1 at 32:9; and a profile resize
to 1000x600. Every run returned 0 with no Vulkan validation errors, and the
recorded source, binary and DDS identities remained unchanged throughout.
`native-edge-texel-center/runs.json` contains the commands, dimensions and hashes.
The 4:3 PPM is byte-identical before and after the strip sampling correction.

An independent CPU comparison of the 4:3 frame and the centered 960x720 region
of the 32:9 frame found identical header, selected label/value and footer
pixels, unchanged name/description/Spartan bounds, and identical centered bar
contours. Small compositing/rounding differences remain in other regions; the
entire centered framebuffer is not claimed identical. All 112,000 sampled
outer top/bottom world pixels are nonblack. Corrected edge strips retain the
bar's vertical coverage across the canvas boundaries; the original center's
first/last-pixel filtering remained untouched in those captures. Those older
captures consequently retain a one-column center-side attenuation at32:9;
they do not establish seamless joins for the subsequent correction. See
`independent-menu-center-review.json` in the same evidence directory.

The user subsequently reported visible vertical joins. Direct CPU extraction
of the original cache confirms that framing_center has no sprite sequence,
scroll or positional animation: its sole screen5 reference supplies a
1024x512quad at(0,0). The intro/outro320ms tracks animate alpha only. At the
known2560x720native extent, first/last center pixels sample one-third of a
source pixel inside the boundary, mixing the original transparent padding;
the independent strips sample the full-alpha inward texel centers. The
pixel-center scissor split covers every pixel exactly once. The measured
source, uploaded-image and retained-native evidence is in
`analysis/frontend/widescreen-debug-20261009/frame-seam-cpu-diagnosis.json`.
The uploaded PNG's apparent joins do not agree with a full drawable of its
stored dimensions, so it is not used to infer the live viewport. New native
before/after captures at exact supplied extents are the acceptance gate for
the correction. `frontend_frame_sampling_tests.cpp` checks admitted boundaries,
shared edge arithmetic, unchanged interior/4:3samples and fractional
pixel-center partitions; CPU checks alone do not establish completed GPU
seam continuity or original framebuffer parity.

The rebuilt F1 Vulkan probe also passed input/text/mouse ownership, exact
closed-panel pixels, swapchain resize and resource cleanup, with no validation
output. Its separate evidence is in `panel-edge-texel-center/` and
`analysis/frontend/title-camera-matrix-audit/debug-panel-edge-texel-center-gpu.log`.
These native extension checks do not complete original Xbox framebuffer,
whole-menu layout or navigation parity.

The subsequent join correction is now validated in seven fresh native captures
under `analysis/frontend/frame-strip-seam-20261009/`. The fragment sampler clamps
only framing_center's boundary half texels to the same inward samples as the
independent strips, and only when horizontal extensions exist. The original
center quad, interior UVs, DDS files and draw order are unchanged. At16:9,
32:9 and2560x1370, every changed pixel is confined to the two center boundary
columns. The4:3,5:4 and1001x601 fractional cases are pixel-identical before and
after; Appearance32:9 also changes only the two join columns. All seven runs
returned0 with zero validation errors. `pixel-comparison.json` records the
complete counts. The frame sampling, frontend bounds, retail frontend,
original mainmenu frame, device contract and optional F1 GPU regressions pass.
These checks establish the native seam correction, not whole-frame Xbox parity.
