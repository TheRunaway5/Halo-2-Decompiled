# Dust II with New Mombasa materials

## Current redesign brief

The user has superseded the original full-density CS2 geometry requirement.
Keep Dust II's routes, dimensions, cover locations and spawn layout, while
redesigning the environment around Halo 2 District's architecture/materials
and OG Xbox geometry density. Use actual Halo 2 Zanzibar palms at the tree
locations and replace the car wrecks with permanent, static destroyed Warthogs.
Retain the authored desert backdrop, the main game's HUD and an unlimited-time
walkaround in this native engine. Reduce the overexposed sky and rebake lighting.
The previous full-density candidate below is a baseline, not this completed
redesign. Its published model/archive must remain available for comparison.

Measured from the original Xbox cache geometry with `Cache::load_bsp(0,false)`:
Zanzibar 23,397 triangles, Headlong 28,513, Lockout 14,463. These counts include
BSP clusters and instances; dynamic characters/vehicles are separate. The
current 4.5-million-triangle CS2 render export is far above that range.

The local model package is at `assets/custom-maps/dust2_mombasa/`. It converts
the installed **Counter-Strike 2** Dust II geometry, rather than rebuilding its
layout from images. This is an authored texture replacement on actual source
geometry. It is not a certified playable Halo 2 map.

The render source contains 4,546,379 triangles. Of these, 4,539,658 appear in the
textured model; the other 6,721 are Source compiler helpers, decal surfaces and
effect planes retained in a separate ASS. The original world collision export
contains another 435,649 triangles. Nothing is decimated. Geometry keeps the
original placements and real-world scale: one Source inch becomes 5/6 ASS
units, or 1/120 Halo world unit. The ASS uses the documented Halo 2 revision 2.

Twenty material families reference actual shaders from MCC Classic's
`03b_newmombasa.map`. The textures come from those shaders' diffuse bitmap
bindings, verified against the project's copied bitmap resource inventory.
PNG and TGA are supplied for editing, and DDS retains the original pixel
partition and mip chain. Original UV0 is retained; the material assignments
are authored by surface family. Some New Mombasa shaders also require bump,
detail, reflection and shader parameters which the simple glTF preview does
not reproduce.

| Package file | Purpose |
| --- | --- |
| `data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass` | Render geometry with original New Mombasa shader paths and render-only flags |
| `dust2_mombasa.glb` | Portable textured model, with textures embedded |
| `dust2_collision.ass` | Original world collision geometry with collision-only flags |
| `dust2_collision.glb` | Collision reference with original Source surface/interaction metadata |
| `dust2_source_helpers.ass` | Separated source helpers and overlay/effect planes; keep out of the visible level |
| `textures/` | Actual MCC diffuse textures as DDS, PNG and TGA |
| `material_mapping.json` | Every source material assignment and exact shader/bitmap identity |
| `source_entities.json` | Source spawn points, buy zones and bomb-volume model references; these are not Halo scenario placements |
| `provenance.json`, `files.json` | Input identities, hashes, output inventory and scope |
| `geometry_validation.json`, `gltf_validation.json` | Full source geometry comparison and glTF format validation |
| `preview-*.png` | Viewed model renders, not Halo 2 gameplay captures |

## Native engine integration

The Classic H2EK is installed at
`/home/eric/.local/share/Steam/steamapps/common/H2EK/`. Its compiler has imported
the full assembled render and collision geometry. The map includes 30 original
spawn positions, a sealed sky shell, and 42,160 authored desert triangles.
The source terrain lies outside/below the protected playable collision bounds.

The native candidate is being assembled in
`.local/dust2-native-candidate/`. It has its own cache, original-program shader
bank, and authenticated custom-map owner receipt. This uses this repository's
native engine; the Windows game is not the target runtime.

Lighting covers 4,754,431 compiled render vertices in 411 buckets. The diffuse
irradiance was baked with Cycles at 48 samples and six diffuse bounces, then
serialized as native incident/radiance resources. Incident direction is an
authored approximation using the receiver hemisphere and source sun. RGB8
storage clamps values above one. This is authored lighting, not Xbox pixel
parity. The installed full lightmap tag retains the float bake inputs.

`tools/prepare_dust2_native.py` publishes the native content inside owned
scratch. It preserves the compiler's triangle index bytes while coalescing
visibility subparts; triangle lists must not be stitched as triangle strips.
It also checks recompiled position/normal/UV streams against the bake source.
The selected stock runtime and the existing published model archive remain
unchanged while the candidate undergoes runtime acceptance.

The HUD uses the existing `MultiplayerHud` renderer and the ordinary game's
prepared Lockout HUD definitions, bitmap sequences and texture partitions.
`export_dust2_hud` exports definitions after the stock `Cache` preparation;
`tools/dust2_main_hud.py` resolves their exact group/name identities into the
custom map. This replaces all 31 kit HUD definitions, including their widget
layout and inherited effects. It removes the earlier kit chat shader alias.
The candidate launcher applies the unlimited-time rule after variant selection.

The current load attempt and build receipts are under
`.local/dust2-mombasa-integration/` and
`build/test-results/dust2-native-integration-v2/`. A cache parse or successful
compile does not prove a walkable match. Runtime acceptance remains pending.

## Model source and import boundary

ASS is an input to Halo's level toolchain, not a runtime model file. Import it
with the [Halo Asset Blender Development Toolset](https://github.com/General-101/Halo-Asset-Blender-Development-Toolset)
for editing. The shader paths in the ASS must resolve to real Halo 2 `.shader`
tags in the destination editing kit. The package contains texture pixels and
references, not extracted editable shader tags.

Before compiling a playable level, combine the render and collision sources,
author a sealed BSP shell and portals, resolve shader tags, and add Halo
scenario placements and lightmaps. Full CS2 mesh density and 3,537 render
pieces require budget and visibility work for Halo 2. The world collision
contains Source-specific player, NPC and projectile interaction layers; they
need explicit Halo collision decisions. The package preserves their metadata
instead of asserting equivalent behavior. Source particles, animated props,
the separate 3D skybox and Counter-Strike game rules are outside this model.

Every ASS triangle corner was compared against the actual reference exports;
maximum position error was below 0.2 micrometers. The preview's original
geometry buffer is byte-identical. Khronos glTF validation completed with zero
errors and zero warnings. Its 945 informational findings describe source
degenerate triangles retained for fidelity; these need review during Halo BSP
cleanup. Unused attributes and empty helper nodes were ignored explicitly.

The Classic compilation command is
`tool_fast.exe -batch structure-new-from-ass scenarios\multi\dust2_mombasa\structure\dust2_mombasa.ass no`.
This has been run on the assembled source. The model archive still describes
the earlier portable model delivery; use the native integration receipt for
the current candidate's status.

## Reproduction

Geometry was exported using [Source 2 Viewer / ValveResourceFormat](https://github.com/ValveResourceFormat/ValveResourceFormat)
20.1. With that CLI, `CS2_GAME` naming the installed `game/csgo` directory,
and an owned `HALO2_TEST_SCRATCH` working directory:

```bash
Source2Viewer-CLI -i "$CS2_GAME/maps/de_dust2.vpk" \
  -f maps/de_dust2/world.vwrld_c -d --gltf_export_format glb \
  --gltf_export_materials --gltf_export_extras \
  --game "$CS2_GAME/gameinfo.gi" -o dust2_materials.glb

Source2Viewer-CLI -i "$CS2_GAME/maps/de_dust2.vpk" \
  -f maps/de_dust2/world_physics.vmdl_c -d --gltf_export_format glb \
  --gltf_export_extras --game "$CS2_GAME/gameinfo.gi" -o world_collision.glb

Source2Viewer-CLI -i "$CS2_GAME/maps/de_dust2.vpk" \
  -f maps/de_dust2/entities/default_ents.vents_c -d -o entities
```

The second command also writes `world_collision_physics.glb`, which is the
collision input. Run the exporter under `tools/run_test.py`, supplying absolute
input paths and a new explicit output directory:

```bash
python3 tools/run_test.py -- python3 tools/build_dust2_mombasa.py \
  --render-glb /owned/scratch/dust2_materials.glb \
  --collision-glb /owned/scratch/world_collision_physics.glb \
  --entities /owned/scratch/entities \
  --mcc-cache-root /path/to/assets/imported/mcc-ID \
  --source-vpk "$CS2_GAME/maps/de_dust2.vpk" \
  --output /path/to/new/model-package
```

Python requires NumPy and Pillow. Extraction, temporary dependencies, probes
and preview render work were all owned by `tools/run_test.py`; the final
package and its archive are intentionally retained as the requested model.
The original Steam assets, imported MCC owner and unrelated workspace changes
are preserved. See the package inventory for exact retained sizes and hashes.

Format references: the original [Halo 2 ASS specification](https://www.h2maps.net/editingkit/assfilespec1.html),
the [level import instructions](https://www.h2maps.net/editingkit/importstructure.html),
and the Halo Asset Blender Development Toolset's ASS reader/writer and Halo 2
material-symbol definitions. DDS/bitmap identities use the repository's existing
MCC inventory and cache reader.
