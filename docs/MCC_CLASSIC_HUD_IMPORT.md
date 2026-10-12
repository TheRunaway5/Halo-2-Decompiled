# MCC Halo 2 Classic HUD import

The normal launchers now select `assets/mcc-classic-hud` through the separate
`assets/hud-source.tsv` registry. The installed, tested runtime is
reported by `./Halo2-Launcher --launcher-info`. This replaces the canceled vector redraw
as the active HUD source.

The importer reads the installed MCC **Classic** `halo2/h2_maps_win64_dx11`
version-13 caches and their `textures.dat`, plus every file in `halo2/h2_fonts`.
The source installation is read only. DDS files retain the original decoded
resource payload and every authored mip; font files and language tables are
copied byte for byte. No art is generated, traced, resized or reconstructed.

Measured coverage in this installation:

| Resource | Imported | Selected by existing original owners |
| --- | ---: | ---: |
| Classic map caches checked | 43 | — |
| HUD bitmap banks/pages | 73 / 339 | 70 / 328 |
| Emblem banks/pages | 6 / 288 | Existing scoreboard shader owners |
| Font files/language tables | 35 / 5 | Authored MCC English table selectors |

All 70 original HUD banks have direct MCC counterparts. Coverage includes every
ammo meter and count, grenade icons, all 98 reticle/lock/weak-spot pages,
full/split/dual-hand pages, shield, radar, scopes, backpack weapons, vehicle HUDs,
objectives, waypoints, training indicators and score HUD. The extra MCC backpack
page and chat/silenced-SMG banks are retained in the import; no new weapon or
widget is invented to display them. All 43 caches have the same authored pages
and sequence data for these banks.

MCC resolutions vary by asset. Its main reticle bank is generally four times
the original dimensions and twice Vista's dimensions (first page 220×220).
Many other pages remain around twice the original dimensions. The custom radar
contact and training marks retain their supplied native dimensions. This is a
source import, not a claim that every individual source page is a 4K image.

The native V3 sampling library preserves original bank counts, widget geometry,
sprite UVs, registration, clipping, font advances/bearings/kerning and gameplay.
The importer verifies exact original/MCC sprite correspondence. MCC's final
backpack sequence is an explicit additional page; the original 19 retain their
exact sequence ownership. Differently padded MCC pages are sampled directly at
the original logical bounds. Shield fill/outline alignment comes from the
authored MCC widget records.

Radar composition uses MCC's actual sweep, mask and contact pages at a 128×128
intermediate. Its DXT1 custom contact is decoded with the existing native BC
decoder. The original RGB-only intermediate write semantics remain intact.

MCC's 12-entry English font table is used for donor selection rather than
assuming it equals the original 11-entry table. The native sampler copies
authored same-codepoint glyph pixels, including controller glyphs, while keeping
original logical metrics and conservative fallback-alias handling. MCC's
280×58 E121 glyph has a bounded exception pinned to its actual encoded stream.
Localized files/tables are retained without adding a new localization system.

Pause/menu definitions stay bound to the existing Vista metadata import using
`menu-ui-source.tsv`. Menu artwork and fonts now have their own
[MCC menu import](MCC_CLASSIC_MENU_IMPORT.md). Xbox gameplay assets, selected
update executable and announcer source selection remain independently owned.

To import and activate an installed MCC copy after normal game-file setup:

```sh
./Halo2-Launcher --import-mcc-hud --setup-only
# Or choose a nonstandard installation:
./Halo2-Launcher --mcc-source "/path/to/Halo The Master Chief Collection" --setup-only
# The asset setup CLI supports the same operation:
python3 tools/setup_assets.py --import-mcc-hud
```

Each import creates a new directory and commits the HUD selection only after
the complete export succeeds. A failed import preserves the previous selection.
Explicit `--hud-ui-assets DIRECTORY` overrides remain supported. The direct
exporter also accepts `--output`, `--xbox-maps` and `--menu-assets` for standalone
pack assembly.

Validation evidence is under `analysis/multiplayer/mcc-hud`:

- `source-identity.json`: reopened all 43 source caches and `textures.dat`;
  every one of the 627 DDS payloads/mip chains and all 40 font/table files match
  the currently installed source bytes. A second complete import reproduced all
  673 pack files other than the updated provenance document.
- `native-import.log`: all 70 original HUD banks/328 pages load; 4,098 native
  glyph selections and 198 controller selections retain logical font metrics
  and copy source pixels exactly across the 11 original font selectors.
- `crosshairs.log`: 31 weapons, 84 authored rows, 75 selected pages, 33,312
  material bindings and 20,160 unchanged full/split/dual-hand vertices.
- `gameplay.log` and `artwork.log`: actual shot/reload/shield/death/respawn HUD
  checks pass; 240 shield fill and 240 outline vertices retain current layout.
- `framebuffer.log`: Vulkan shield alpha/padding, contacts, complete sweep
  cycles and backdrop preservation pass across one to four views and both
  aspects; 2,016 square-corner samples remain unchanged.
- `launcher-routing.json`: 84 launcher cases pass, including MCC activation,
  failed imports, explicit overrides and separate Vista menu ownership.
- `capture-comparison.json`: normal-launcher 3840×2160 MCC/Vista captures use
  the selected update executable, same runtime, tick40 and fixed auxiliary seed.
  Exactly 202,530 pixels differ inside the inspected HUD regions; zero outside
  them. No Vulkan validation errors were reported.

These are native-port rendering and source-byte checks, not an original MCC
engine framebuffer comparison or physical-TV validation. The manifest records
source cache/texture/font hashes, authored records, resource segments and the
retained reader/schema references. `tools/verify_mcc_hud.py` can rerun the source
identity audit; `tools/capture_mcc_hud.py` reruns the matched normal-launcher
captures.
