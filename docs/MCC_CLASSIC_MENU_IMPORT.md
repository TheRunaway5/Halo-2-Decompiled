# MCC Halo 2 Classic menu assets

Both ordinary launchers select `assets/mcc-classic-menus` through
`assets/menu-source.tsv`. HUD selection remains in `assets/hud-source.tsv`.

`tools/import_mcc_menus.py` reads the installed Classic version-13 caches and
`textures.dat`, using the same inspected Reclaimer/Assembly resource reader as
the MCC HUD importer. It checks all 43 map caches. All 295 non-HUD `ui\\` bitmap
banks, comprising 938 pages, are imported. No conflicting map-specific versions
exist in the installed source. DDS payloads retain their original format and
every authored mip. All 35 font files and five language tables are copied
byte for byte. No image generation, reconstruction or pixel resampling occurs.

Bindings are qualified by the original native menu owner:

| Original owner | MCC banks/pages mapped | Banks retaining current art |
| --- | ---: | ---: |
| Xbox mainmenu (`pack.tsv`) | 218 / 783 | 6 |
| Vista shell/settings/lobby (`vista.tsv`) | 251 / 887 | 1 |
| Vista gameplay pause/settings (`pause.tsv`) | 91 / 624 | 1 |

These counts include source banks not referenced by a currently active widget;
the native loader selects only existing source-local owners. The import also
retains all additional MCC menu banks, including map previews. Bitmap names,
page counts and effective sprite page/UV/registration ownership must match
exactly, with one source-proven extension: the five keyboard button banks and
pregame lobby buttons admit normal0 and selected/latched1 from MCC's three-page
banks. The extra page2 is mouse hover, as recovered from PC63FF21/63ADB1 and
the Xbox keyboard counterpart at2377F6. All three MCC pages are verified byte
identical to the corresponding supported Vista pages, and the two selected
pages must be exactly twice the original extents. The extra hover page is
retained without inventing an Xbox hover owner. The provenance manifest records
these proofs, every admitted page and every fallback.

The Xbox fallbacks are the browser item background, map-data field, settings
bank, quick-options display and two Xbox Live main-menu banks. These are missing or have different authored
page counts in MCC. Their existing Vista sampling, where already admitted, or
original artwork is retained. The Vista shell and pause fallbacks are the
`network_squad_browser_screen\\ds_icon` bank, whose page count differs. Many
Xbox-incompatible banks do have exact Vista counterparts and are selected on
those existing Vista routes.

Menu sampling covers title/menu panels, buttons, list skins, keyboard artwork,
dialogs, profile/appearance screens, campaign/multiplayer previews, lobby and
browser artwork, settings, in-game pause widgets and source-owned emblems.
Unlike the older generic texture-pack mode, the explicit MCC menu format admits
the verified framing pages. Both the center and native widescreen extension
sample the same imported framing page, with physical texel boundaries used for
filtering. Separate original frame overrides remain available in original mode.

The MCC English font table chooses each sampled font by selector. Logical
advances, bearings, kerning, clipping, wrapping and hit targets remain owned by
the existing menu source. Selectors that share an original logical font but use
different MCC rasters receive separate combined atlas materials. Original
glyph fallback/alias handling remains bounded. Localized font files are retained
without introducing a new localization system.

The Vista cache still supplies its existing widget definitions, strings and
menu audio. MCC caches are not passed to the Vista metadata reader. Gameplay
pause menus receive their own `--pause-ui-assets` selection. The separately
decoded Vista loading presentation remains in use: its twelve executable-owned
textures have no verified one-to-one mapping to the Classic map bitmap banks.
The MCC map-owned loading sweep is retained in the imported bitmap catalog.
Source resolution varies; this import does not claim each page is a 4K image.

Reimport from a locally installed MCC copy:

```sh
./Halo2-Launcher --import-mcc-menus --setup-only
./Halo2-Launcher --import-mcc-menus --mcc-source "/path/to/Halo The Master Chief Collection" --setup-only
# Both packs can be imported in one invocation:
./Halo2-Launcher --import-mcc-hud --import-mcc-menus --setup-only
```

The direct exporter accepts `--output`, `--xbox-maps`, `--vista-assets` and
`--activate`. Each CLI import uses a fresh staging directory, verifies readback,
then commits the output and selection only after successful export. Failures
preserve the previous pack selection. Explicit `--menu-ui-assets` and
`--pause-ui-assets` runtime overrides are supported.

Validation artifacts are under `analysis/multiplayer/mcc-menus`: source identity,
native import/geometry/pointer checks, menu layout checks, launcher argument
routing, and 3840×2160 menu captures. These establish native source selection
and bounded rendering checks, not complete retail menu-flow or MCC framebuffer
parity.

The import test compares 882 original/MCC pane and selection draws, comprising
2,236,158 vertices, while preserving positions, wrapping and pointer targets.
The actual pause-menu draw selects MCC samples for its 576 text vertices.
Ten source-only shell/result/transition definitions require their separate
factories; the postgame test separately covers all six panes of both retained
result screens. The older general `frontend_menu_layout_tests` encounters
`unimplemented source Vista settings route` at Vista14 in both stock and MCC
modes; this existing route limitation is recorded in `menu-layout-stock.log`
and `menu-layout.log`, rather than counted as a successful check.

`source-identity.json` verifies every imported page and font against the installed
MCC copy. `launcher-routing.json` passes 108 cases, including independent menu
and HUD selection, explicit overrides, setup failure and incomplete-pack
rejection. `captures-delivered/captures.json` records eight source-pinned 4K
captures through both ordinary installed launchers with zero validation errors.
Each capture records its exact runtime and launcher identity, including retained
launcher snapshots when another active chat installed a runtime during
validation. The final keyboard capture also verifies the subsequently installed
tag-system runtime, which selects the same MCC menu pack. The current runtime
is reported by `./Halo2-Launcher --launcher-info`.
The 887 corresponding Vista/MCC menu pages have equal source dimensions;
34 of the 35 MCC font files differ from the previous Vista copies.

The verified menu reference build at `build-mcc-menus-20261012` also incorporates
the four final Needler/SMG sound-owner corrections from the then-installed
Needler delivery, without replacing newer weather/audio/source-object work.
Original Needler firing/ammunition checks pass. Its full live Needler probe
also passes (three owned sound submissions for three actual fired rounds).
An earlier short firing-capture probe stopped before the required number of
authored poses; that failed probe remains recorded rather than counted as a
successful check.
