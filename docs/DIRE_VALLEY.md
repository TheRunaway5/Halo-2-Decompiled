# Dire Valley

Dire Valley ports the user's installed MCC Classic Danger Canyon geometry into
the native Halo 2 engine. Its authoring tools are `tools/port_dire_valley.py`,
`port_dire_valley_scenery.py`, `stage_dire_valley_h2ek.py`,
`author_dire_valley_lighting.py`, `configure_dire_valley.py`,
`prepare_dire_valley_native.py`, `validate_dire_valley_geometry.py` and
`validate_dire_valley.py`. The retained local package is
`assets/custom-maps/dire_valley/`; consult its `status.json` and evidence for the
current build and acceptance result.

The original world has 8,341 render triangles. Four CE teleporter shield
triangles are removed; the other 8,337 retain their positions and normals.
Native UV orientation is converted once when writing ASS; marker UVs are fitted
to their corresponding Halo 2 atlas cells.
The original closed collision shell and clips are retained. Visible source
ground supplies colliding, lightmapped ground surfaces; 976 duplicate ground
collision triangles are omitted. Separate collision-only ground had no render
attribution and caused every one of the 103 starting-point object-light queries
to miss. Convex polygons are triangulated with a fan origin chosen to avoid numerical slivers.
One CE world unit remains one Halo 2 world unit. ASS uses 100 units per world
unit. Exact source identity and hashes are in the authoring provenance.

All 163 scenery placements use actual Halo 2 SDK model meshes and their native
collision shapes: 33 redcedar trees, 61 granite boulders, 28 Covenant crates,
34 red/blue Headlong light fixtures, four portable shields, two flag bases and
one communications antenna. No CE scenery meshes remain. Original positions
and rotations are retained. Uniform scales preserve tree/cover heights or rock
extents; low fixtures retain their footprint and sit at the original base.
Every mapping, source bound, scale, local offset and native asset hash is in
`authoring/scenery_geometry.json`. The 33 trees use actual native scenery tags,
with the original foliage material, alpha cutout and six model-detail selectors.
The four portable shields also use native scenery so their energy fields retain
the authored model/LOD path instead of the small BSP-instance fade pattern.
The other 126 props use highest intact model sections as static BSP instances,
including native collision. Prop destruction is not implemented by that static
representation. The lighting authoring receipt records both placement paths.

Real Halo 2 `objects/multi/2way_teleporter/2way_teleporter` scenery replaces the
CE shield and base. The sender uses the `base` variant and the destination uses
`receiver`. The sender is fitted to the measured original portal opening using
the stock model bounds, so its emitters remain in front of the wall. Its uniform
scale is approximately 0.828. The destination sits on its original floor. Logical
source/destination coordinates and channel zero remain exact. The compiled
cache validator checks both netpoints, all 103 original starts and the absence
of CE teleporter tags. A native A/B traversal test uses a private single-start
fixture and disables only the receiver in its control case; the delivered
scenario retains the original starts.

There are 59 converted pickups and 18 Halo 2 vehicle placements. CE pistol,
assault rifle, plasma cannon and flamethrower slots become Magnum, Battle Rifle,
Fuel Rod Gun and Brute Shot. Rocket Warthogs become Gauss Warthogs. Five CE
health-pack spawns are omitted: the unused H2 healthpack shader requires native
inputs that the retained library does not provide. Ordinary Halo 2 health
regeneration remains in use. Original omitted placements are kept in the
authoring report. CE race hints do not become a new Halo 2 race game mode.

The map uses Coagulation ground, cliffs, sky and its original ambient/sun
values, Forerunner industrial materials, Halo 2 redcedar foliage and bark,
granite and Covenant equipment materials. A map-owned copy of the sky enables
the native render-sky lighting query. Installed stock tags remain unchanged.
Classic Tool bakes at `high`, including indirect radiance, with `lm:1.0` on
Forerunner surfaces and `lm:0.5` on terrain and static props. All world render
geometry is in the primary BSP, with compiler welding enabled. The native
publication preserves complete compiler section headers and resource bytes,
including subparts and visibility accelerators. A geometry guard rejects a
publication that changes those cross-referenced structures. The earlier
visibility compaction merged subparts while leaving original accelerator
indices; original45A70 then wrote outside its small visibility mask and
corrupted the1373F0 arena counter. The publication also installs
the ordinary game's prepared HUD and publishes a separate, authenticated native
material bank. Windows shader bytecode is not used as native Xbox programs.

The custom launcher verifies the map's tested engine and exact cache hash, then
starts a one-player session in Original graphics mode with no time limit.
The project's stock engine selection remains separate. Native acceptance is recorded separately from
asset validation; this map does not certify the entire port's renderer,
particles, audio, vehicle physics or every multiplayer rule.

The previous high-bounce build resolved all 103 starting-point object-light
queries. The material-polish build preserves those starts, receives a fresh
high-quality bake and passes the 900-tick walking route, two-player startup and
enabled/disabled portal traversal comparison. Reviewed actual captures cover
the marked bridge, shields, granite, Forerunner base, native foliage cutouts and
a lit vehicle. Its material audit covers every placed model material, including
vehicle attachments, with no active diagnostic bitmap. Compiled checks cover
384 marker UV vertices and 2,365 rendered static-prop UV vertices. These bounded
checks do not certify the entire port's gameplay, audio or Xbox framebuffer.
The high bake exposed omitted original read-only operands at `0x46A5A0` and
`0x46A340`; portal approach exposed initialized `.data` at `0x48C78C` and
its separate original loader-zero policy fields. Exact bytes and reader
instructions are authenticated against TU1.5 executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The engine declarations retain the original read-only/writable distinction.
The native ownership CTest passes. A separate optional whole-bank/image
comparison reports a difference at `0x56E0E8`; whole-bank identity is not claimed.
The eight new light/portal declarations are authenticated independently.

## Reproduction

### Map-selection card

`assets/custom-maps/dire_valley/menu/profile.json` owns the title, description
and recommended 8–16 player range. Its 220×207 `preview.png` is an actual native
multiplayer capture from the authored rear ledge, facing across the central
Forerunner bridge with the Halo ring above the canyon. The centered camera uses
a 63-degree vertical lens. `native-capture.json` records the exact camera,
runtime and finite capture control; `composition.png` retains the clean frame.
The HUD and first-person weapon are suppressed at capture time; there is no
offset crop or generated scenery.

`tools/author_dire_valley_menu.py`, under `tools/run_test.py`, publishes a private
menu cache using the first NONE multiplayer C64 row and its reserved preview
bitmap. All 25 existing named map records remain exact. The stock texture bank
gets a copy-on-write extension for the card; its original pixels and the source
import remain intact. A separate nested custom cache preserves Dire Valley's
own texture/audio banks and native program ownership.

`tools/replace_dire_valley_preview.py` updates the two preview partitions in a
fresh copy-on-write cache. All 50 catalog records and the BSP/lightmap metadata
remain byte-identical. The existing index is authenticated before editing and
refreshed through the verified metadata repair API. Both encoded bitmaps are
decoded and compared to the requested pixels before native menu validation.

The `frontend-map-bindings.json` manifest binds the authored map ID/scenario to
its exact custom cache and resource directory. The frontend verifies the actual
admitted custom scenario before attaching these paths to the catalog entry;
the local session carries both paths through resource preparation and launch.
Empty bindings retain ordinary stock map routing. Focused checks cover path
rejection and the actual prepared map/catalog binding. Evidence is in
`menu/evidence/`; the actual lobby map picker is screen206.

The installed menu cache is `assets/imported/mcc-dire-valley-polish-20261023`,
with frontend runtime
`bin/engine-builds/dire-valley-menu-20261023/halo2_native_polish`.
This is a hardlink to the same executable, with its own qualification for the
updated menu cache; its compiled shader directory remains exact.
Both installed launchers admit this independent frontend. The previous stock
gameplay executable, qualified stock maps and defaults are retained. Three
catalog/routing CTests, eight frontend-selection checks, four frontend screens,
Dire Valley startup and the stock two-player default passed. These are bounded
startup and routing checks, not a completed gameplay qualification.
The polished bridge photo and its current installation checks are in
`evidence/polish/`. The preceding bridge-card checks remain in
`menu/evidence/bridge-update/`, and the earlier overlook remains under
`menu/evidence/previous-overlook/`.

The menu-card work does not resolve the outstanding frame-rate issue recorded
in `build/test-results/dire-valley-performance-20261022/diagnosis.md`.

### Map and lighting

Run all compiler and native checks through `tools/run_test.py`, with generated
files in `HALO2_TEST_SCRATCH`. Retain the authored package and bounded reports
before ending the owner. The pipeline is:

1. Extract the installed CE `halo1/maps/dangercanyon.map` with
   `port_dire_valley.py`.
2. Stage a private installed Classic H2EK overlay with
   `stage_dire_valley_h2ek.py`. Import the corresponding native SDK scenery with
   `port_dire_valley_scenery.py --source CE_CACHE --package PACKAGE --kit KIT
   --interface HALO_TAG_INTERFACE_MODULE_DIRECTORY`. This uses the General-101
   tag layouts and requires NumPy.
3. Run `author_dire_valley_lighting.py --package PACKAGE --kit KIT`, then
   `polish_dire_valley.py --package PACKAGE --kit KIT`. The polish fits native
   marker atlas cells, uses Beaver Creek's native directional arrow, replaces
   the SDK giant-rock placeholder with the granite-family material, and keeps
   source triangle positions/normals and placement transforms unchanged. It
   converts native UVs to ASS with `V = 1 - V` and promotes the four shields to
   native scenery. Run
   `tool_fast.exe -batch structure-new-from-ass
   scenarios\multi\dire_valley\structure\dire_valley.ass no` under the owned
   Wine prefix. Configure the scenario with `configure_dire_valley.py` and
   General-101's Halo-Tag-Interface so the four shield placements join the 33
   native trees.
4. Run `tool_fast.exe -batch lightmaps
   scenarios\multi\dire_valley\dire_valley dire_valley high`, then
   `tool_fast.exe -batch build-cache-file
   scenarios\multi\dire_valley\dire_valley win64`.
5. Validate `h2_maps_win64_dx11/dire_valley.map`. Publish it with
   `prepare_dire_valley_native.py --material-polish`, the existing prepared MCC root and an
   `export_dust2_hud` main-game HUD export. This keeps the original native
   programs and binds the portable shield's SDK-authored offset bitmap in its
   original texture slot. Validate the published cache again, then run
   `validate_dire_valley_materials.py --package PACKAGE --report REPORT`.
   That audit checks actual visible BSP parts, placed model materials, native
   bitmap remaps, compressed resources and compiled marker UV coverage.
6. Test the retained native cache, then stop the owned Wine prefix and verify
   scratch removal. Keep the original installed sources and other active work.

After publishing, replay the direct base case with `tools/run_test.py
--scratch-root .local/test-scratch -- python3
tools/validate_dire_valley_lighting_runtime.py --package PACKAGE --runtime ENGINE
--report REPORT --case base --retain-capture`. Other cases are `foliage`,
`vehicle`, `walk` and `two`. Capture completion is checked automatically;
lighting and foliage images still require visual review.

Format references are [Reclaimer's Halo 1 cache and model readers](https://github.com/Gravemind2401/Reclaimer/tree/master/Reclaimer.Blam/Blam/Halo1),
[Halo-Tag-Interface layouts](https://github.com/General-101/Halo-Tag-Interface),
and the [Halo 2 ASS specification](https://www.h2maps.net/editingkit/assfilespec1.html).
The original [Halo 2 lightmap documentation](https://www.h2maps.net/editingkit/generatelightmaps.html)
describes the high-quality photon, sampling and antialiasing settings.
