# ONLINE HD artwork and widescreen verification

Both launchers select the imported Vista GUI root and its `maps/mainmenu.map`
through `assets/asset-sources.tsv`. ONLINE uses that source's shell definition,
compatible HD bitmap pack, sampled fonts, and explicitly reviewed source pages
for the banks with different PC padding or page counts. See the
[source correspondence audit](../multiplayer/docs/online-vista-assets.md).
The Xbox caches continue to own gameplay assets.

The main foreground keeps its authored proportions on a centered 640×480 canvas.
The PC definition adapter uses the original height/1200 raster transform.
Decorative tracks and frame edges extend across widescreen margins; text,
buttons, dialogs and mouse targets use the same uniform foreground scale.
The viewport checks cover 640×480, 1280×720, 1280×800, 2560×1080,
2560×1437 and 1001×601, including fractional scaling and odd dimensions.

`frontend_online_tests` compares every submitted ONLINE font atlas and bitmap
against the imported Vista donor or compatible pack. An unrecognized Xbox
fallback fails the test. It also checks text bounds, foreground scaling and
mouse target alignment at all six viewport sizes. Direct authenticated routes
supplement the real HTTP fixture flows for playlists, server browsing, clans,
party membership and nested keyboards/dialogs. Queue screen210 uses native
waiting pane1; the retired service's unused panes are not advertised as native
routes or assigned unverified bitmap aliases.

The queue's embedded B Cancel caption now submits a mouse target. Its
regression activates that target and requires a return to ONLINE. Service
descriptions remain noninteractive. The retained PC postgame factory is tested
separately: all six panes of screens16 and185, source result bank12, mouse
Done/Close, and optional captures at all six viewport sizes. Generic menu sweeps
do not bypass the required retained-result factory.
Full-length native party names are also constrained to the foreground edge,
including their shifted text shadow. The source player skin extends beyond
that edge; short names previously concealed this overflow.
Party shortcut prompts use the authored empty left-panel fields instead of the
old help field over the roster. Their Y/X icons and labels are clickable. The
full-roster check rejects help glyphs overlapping the name column.

The standalone clan roster uses a native full-screen layout. Its original tab
sits at the surface's top left, and the body, header's flat center and member
selection bars extend to the right edge. Fixed edge slices preserve the tab,
corner and border shapes. Text, button icons and scroll arrows retain their
uniform scale. The count/voice columns and clickable footer follow the right
and bottom edges. Rows accept mouse clicks across their extended selection
bars, including the widescreen area outside the original 640×480 canvas.

The native clan flow omits the decorative Vista shell backdrop and underlying
menu widgets. An opaque sample of the imported clan backing covers the full
surface, including behind nested dialogs and keyboards. Those dialogs also
hide the lower roster and collect mouse targets only for the current screen.
Window resizing refreshes the painter's viewport and pointer layout before
input. Rendering normalizes those uniform coordinates into the full viewport
before clip-space clipping, so text keeps its proportions in the side margins.

Member counts include their unit and the voice column reads VOICE OFF, MIC ON
or MIC MUTED. These are native presentation adjustments; the Xbox/Vista source
tags and imported bitmap payloads remain unchanged. Retail Halo 2 Vista did
not expose built-in clans, as documented in the
[archived Bungie Vista FAQ](https://forum.halomaps.org/index20dd.html), so these
retained assets do not establish a retail Vista widescreen clan layout.

The original centered-panel evidence remains in
`analysis/frontend/clan-panel-20261011/`. The full-screen follow-up is in
`analysis/frontend/clan-fullscreen-20261011/`, with real local HTTP clan flows
and captures across the same six viewport sizes. It includes 100 members, the
final scroll page, a 31-character wide member name, all three footer mouse
prompts and clicks near the right edge of the extended member rows. Checks
cover full-surface coverage, hidden shell/roster assets, fixed tab proportions,
text bounds, original Vista asset bytes and pointer projection.

## Reproduction

Build `frontend_online_tests` and `frontend_vista_pcr_painter_tests`. With the
Vulkan validation layer available, run:

```sh
build-online-social-20261011/frontend_online_tests assets/disc/maps \
  --asset-sources assets/asset-sources.tsv --capture /absolute/output/base
python3 tests/frontend_matchmaking_http_test.py \
  build-online-social-20261011/frontend_online_tests --capture /absolute/output/matchmaking
python3 tests/frontend_clans_http_test.py \
  build-online-social-20261011/frontend_online_tests --capture /absolute/output/clans
python3 tests/frontend_parties_http_test.py \
  build-online-social-20261011/frontend_online_tests --capture /absolute/output/parties
build-online-social-20261011/frontend_vista_pcr_painter_tests assets/disc/maps \
  "Halo 2 Vista installer/maps/mainmenu.map" tests/fixtures/vista_pcr_results_original.tsv \
  /absolute/output/postgame --widescreen
```

Run the HTTP fixtures sequentially; each uses a disposable local service and a
test Discord callback. Capture runs include the full100-member clan roster and
full16-member party, including final scroll pages and long names. JSON reports
record requested viewport sizes and captured screens. Capture mode uses Vulkan
validation and reuses the renderer while resizing the same native window.

Evidence for this run is in
`analysis/frontend/online-hd-widescreen-20261011/`. These are native Linux
SDL/Vulkan GUI checks with local service fixtures, not a production Discord
session or an original Windows framebuffer comparison.

`verification.json` records426 GPU captures with checked dimensions and zero
Vulkan validation errors,11 passing frontend regression tests,3 real SDL mouse
routes, source/runtime hashes, and both installed launcher selections. The
capture bank comprises66 base screens,102 matchmaking screens,102 clan screens,
84 party screens and72 postgame screens. The original party-help overlap and
long-name failure are retained as separate red evidence.

The actual PC postgame emblem's `spas` pass remains unresolved and is reported
in `UiDraw::rendering_gaps`. The native port retains its source definition and
does not substitute an Xbox pass. This verification therefore establishes HD
sampling for the rendered GUI; it does not certify that every PC GUI shader or
postgame feature is implemented. See [the PCR bridge](VISTA_PCR_SCREEN_BRIDGE.md).

## Compact Friends panel at the window edge, October 29

The independent original screen-27 Friends footer follows the drawable's
bottom-left corner. Its source bitmap, fonts, prompt artwork and uniform scale
are preserved, including the source one-pixel bottom crop. The ordinary menu
foreground retains its centered canvas. Only the footer's vertices and pointer
rectangles receive the edge translation; reserved material copies give that
channel a full-surface viewport/scissor without changing other menu passes.

`native_mcc_friends_footer_anchor` exercises actual `FrontendPainter` draws and
authenticated Players navigation at 13 aspect/resize steps for Xbox,
PlayStation and keyboard/mouse prompts (39 cases). The matrix includes 4:3,
16:9, 16:10, ultrawide, the supplied 2554×1401 screenshot ratio, odd dimensions,
5:4, portrait and a return to 4:3 on the same painter. It checks actual physical
panel bounds, unchanged proportions, visible-glyph mouse targets, removal of
disjoint old hit locations and stability of the other lobby vertices.

The old painter fails at 1280×720 with its panel left edge 160 pixels inward.
The corrected painter passes all 39 cases and the existing ONLINE identity
regression. Thirteen actual Vulkan frames from the combined source cohort
match their requested dimensions. Ultrawide and portrait frames were visually
reviewed; raw captures and duplicate build files are removed after review.
Small lossless reference PNGs, source hashes and bounded reports are under
[`build/test-results/friends-bottom-left-20261029`](../build/test-results/friends-bottom-left-20261029/working-status.json).
The test uses an isolated loopback identity fixture and does not mutate real
friend relationships. Both installed launchers now select the qualified
`102cfdd654364b7b80b72ad4fb848c2f` runtime. Independent read-only launcher checks
verified the runtime, source inventory and publication hashes after all 29
release tests, 13 packaged map/player checks and four frontend checks passed.
The guarded publication updated the launcher binaries and preserved the
previous selected native runtime. The component build, raw captures and final
verification scratch were removed; the selected runtime/resources are retained
(3,151,874,255 bytes). Exact pins and evidence are recorded in
[`acceptance.json`](../build/test-results/friends-bottom-left-20261029/acceptance.json).

The source search controller and launch admission now use safe Rust through
`FrontendMatchmakingSearch`; native snapshots, profile writers, map validation,
widgets and physical input remain native IO/effects. Consent acknowledgements
and the service's published B countdown gate search/launch. [Qualification](../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-search-rust-validation.json)
records the rebuilt Linux client's SDL/Vulkan playlist/Start checks, 5,563 exact
modern C++/Rust operations and the four unchanged broader frontend failures.
Physical installed input and full original/provider/gameplay delivery stay open.

The source progress screen now receives roster source/slot, placeholder,
rank/speaker and input decisions from Rust; native code retains actual
appearance/voice observations, localization and drawing.
[Qualification](../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-progress-rust-validation.json)
restores original unstable tie ordering, signed cancel timing and player-seat
leave-dialog counts. Four broader frontend failures remain red, and this
qualification does not replace the selected Linux runtime.
