# Sampled menu textures

The launcher now requires both source selections and prepares menu **and HUD**
artwork through [game-file setup](ASSET_SETUP.md). The exporter and historical
captures below describe the earlier standalone menu pack. The direct native
runtime still exposes optional sampling arguments for development.

The native frontend can use a separate 2D texture pack with:

```sh
./Halo2-Title-Launcher --menu-ui-assets /absolute/path/to/pack
```

The launchers select the prepared Vista UI root saved in
`assets/asset-sources.tsv`. An explicit `--menu-ui-assets DIRECTORY` overrides
the menu default. To use the original Xbox menu artwork through either launcher:

```sh
./Halo2-Title-Launcher --menu-ui-assets ""
```

An absent or incomplete prepared source selection opens setup before launch.
`--launcher-info` reports both roots and readiness. A direct `halo2_native`
invocation still uses original Xbox assets when sampling options are omitted.
Gameplay routes receive `--hud-ui-assets` for HUD sampling only; world/model
textures remain sourced from Xbox.

The pack replaces uploaded bitmap pages and the matching texture banks of
existing 2D emblem passes.
`FrontendContent` remains unchanged: Xbox datums, widget dimensions, sprite
UVs and registration, text metrics, list spacing, animation timing, hit regions,
screen definitions and navigation remain authoritative. The adapter does not
import a PC cache into the renderer, nor replace world/model materials,
geometry, flyby camera, fog or shaders.

Frame art is excluded. Its original center texture and separate
`ui/frame_edges/left-edge.dds` and `right-edge.dds` keep the existing native
extension route. The Vista center page has different proportions and cannot
be substituted by doubling its dimensions.

## Supplied Vista archive

The offline exporter `tools/export_vista_menu_ui.py` reads the inspected
PC version8 cache, build `11081.07.04.30.0934.main`, separately from the strict
Xbox runtime cache reader. It exports exact native pixel payloads into DDS
files, preserving format, every mip, sequence records and registration in a
hashed JSON manifest. It rejects unsupported containers, external resources,
swizzled images, volumes and cubes. No installer is executed.

The source provided in `Halo 2 Vista installer/h2pc_installer_2.1.zip` includes
Cartographer components. It has not been established as a pristine stock
Vista distribution. Its assets are comparison candidates, not evidence that
the Xbox pixels, layout or behavior should change.

The complete rip is in `analysis/frontend/vista-menu-ui-20261009/`: 275 bitmap
banks, 930 frames and 1472 mip levels. Independent readback checked all
228399373 native payload bytes against the map resources. Four PNG contact
sheets are previews; DDS files preserve the source format and mips.

The optional pack selects 191 existing native frontend banks / 719 frames.
It uses only exact path/frame correspondence, equal complete bank counts,
identical effective source sequence UVs/registration and a single isotropic
page density per bank. Rejected banks remain exported for investigation.
PC-only frames and widgets are not inserted into Xbox navigation.

The installed pack also supplies five compiled source fonts under `fonts/`:
`conduit-12`, `conduit-13`, `handel_gothic-11`, `handel_gothic-13` and
`handel_gothic-24`. `font-provenance.json` records their exact source hashes.
The original eleven-role `font_table.txt`, advances, kerning, bearings, ink
extents, wrapping and clipped screen geometry remain authoritative. Larger
same-codepoint rasters occupy separate atlas storage. No font is regenerated
or upscaled. Shared fallback records and unverified private symbols retain
their original pixels. A missing sampled font keeps the original font.

Reviewed controller symbols are selected only for the five audited original
and sampled file identities. At the user's request, U+E104/U+E105 use the
Vista LB/RB artwork in place of the Xbox black/white button prompts; the
actions and original text spacing are unchanged. This is an intentional
presentation choice, not an Xbox pixel-parity claim. An explicit empty pack
override restores the original fonts and controller artwork as well.

The original Settings armor/skull pages are still retained. Their Vista bank
changes from three pages to six and rearranges the diagram composition.
Neither the audited Vista widget records nor the installed H2AMPEK artwork
provide a verified mapping that preserves the Xbox diagram placement.

## Reproducing the pack

Extract `maps/mainmenu.map` from the supplied archive into a disposable
directory. Then use the actual runtime importer to inventory its Xbox target:

```sh
cmake --build build-frontend --target export_frontend_ui_inventory
build-frontend/export_frontend_ui_inventory \
  assets/disc/maps/mainmenu.map /absolute/path/to/xbox-frontend-images.tsv
python3 tools/export_vista_menu_ui.py \
  --map /absolute/path/to/extracted/maps/mainmenu.map \
  --output /absolute/path/to/fresh-export \
  --approved-pairs analysis/frontend/vista-asset-inventory-20261009/approved-pairs.json \
  --target-inventory /absolute/path/to/xbox-frontend-images.tsv
```

The approved-pairs manifest pins the exact Xbox and PC maps. Reclassify and
verify other source maps rather than reusing these correspondences blindly.
The exporter preserves existing identical files and refuses to overwrite
different output bytes. `--pack-only` assembles a pack from an existing rip
without rewriting the complete rip.

## Pack contract

`pack.tsv` starts with `H2UI_TEXTURE_PACK_V1`. Each following row contains six
tab-separated fields:

```text
exact_tag_name  frame_index  bank_count  original_width  original_height  relative_dds_path
```

The original dimensions describe layout; DDS dimensions describe sampled
storage. Banks must exist in original `FrontendContent.images`, provide every
original index once, and retain all logical extents. DDS paths must resolve
inside the pack directory. Framing, incomplete banks, path traversal,
unsupported DDS masks, padded rows, cubes, volumes, DX10, truncated/trailing
payloads and inconsistent mip metadata are rejected.

The supported DDS formats are BGRA8, XRGB8, A4R4G4B4, A8Y8, Y8, A8, DXT1
and DXT3. The runtime uses the existing linear pixel decoder and preserves
the complete chain. Format, mip and geometry tests are separate from GPU
captures and original full-frame parity.

## Native validation, 2026-10-09

The rebuilt launcher runtime passed 12 focused tests, including actual pack
loading and 173 stock/optional paired painter draws covering all 150 authored
panes. Every UI vertex float word and material index, mouse hit grid,
backdrop/model queue and original source image/sequence metadata agrees.
Uploaded 2D bitmap and emblem static/animated pages match the selected DDS;
526 pages are enlarged and 67 retain authored mip chains. Original fonts,
framing and preexisting world materials/vertices remain unchanged, including
a world material whose name deliberately collides with a replaced UI tag.

Four native stock/optional screenshot pairs passed with stable binary,
source, font, map and selected DDS hashes and zero Vulkan validation errors:
title at 16:9, Settings at 32:9, Appearance at 16:9 and lobby at 16:9.
`analysis/frontend/vista-menu-ui-20261009/native/runs.json` records commands,
pins and image difference bounds. With the option omitted, the complete
32:9 Settings capture is byte-for-byte identical to the prior native build's
PNG; see `native/default-regression.json`. This proves the measured default
regression and texture-only boundary, not original Xbox full-frame equality.
These captures precede the launcher default-selection change. The rebuilt
launchers and installed durable pack require separate native launch checks;
this document does not treat the source update alone as executable delivery.

## Font and extension follow-up, 2026-10-09

The installed launchers now select the durable pack automatically. The separate
font adapter passed 12 focused checks, including 156 stock/sampled paired draws
covering all 150 authored panes, 305844 sampled font vertices and the mouse hit
grid. Five source font pairs provide 1398 enlarged glyph rows. The tests compare
every logical vertex field, clipping, material identity and model/backdrop queue;
only font UV sampling and uploaded font pixels may differ. These CPU checks do
not establish framebuffer quality or original Xbox whole-frame parity.

`FrontendExtensionCatalog` reserves all 131 actual Vista definitions by source
and ID, including 47 additions, while keeping the 84 Xbox definitions separate.
Settings retains distinct Xbox two-pane and Vista six-pane identities. The
native `--menu-audit` reports those reservations. Catalog metadata does not make
the Vista menus reachable or implement their platform settings. Exact layouts,
references and unresolved flow requirements are recorded in
`analysis/frontend/hd-ui-followup-20261009/VISTA_MENU_EXTENSION_INVENTORY.md`.
