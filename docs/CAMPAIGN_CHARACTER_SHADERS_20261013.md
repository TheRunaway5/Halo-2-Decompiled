# Campaign character shader work

Visual authority: **OG Xbox TU1.5 with original Xbox cache assets**. Vista is
not an authority for gameplay materials, lighting, textures or effects. This
work remains partial until matched original/native rendered frames pass.

## Implementation

`SourceCharacterShaders` resolves each authored `bipd`, `crea`, or character-owned
`vehi` through `hlmt` to `mode`, retains its material LODs and original compiled
programs, and supplies each actor's own lighting and damage-feedback constants.
It reuses the existing source dynamic-model shader and object-lighting owners.
It does not invent a species-specific lighting formula.

The transparent material binder now supplies object lighting to the authored
Brute hair and Tartarus mohawk passes. Authored layer-15 Prophet and Sentinel
plasma shields are admitted through their existing compiled offset program.

Campaign actors use source bounds, posed node zero, the source model collector
and the current BSP/sky object-light query. Light caches are keyed by the full
salted unit identity and cleared on BSP changes, level restart and checkpoint
restore. The renderer receives the initial and subsequently selected BSP's
atmosphere. Each draw receives its actor's constants, even when images are shared.

Object lighting now uses the actual authored object type and query fields:
biped type 0, Enforcer vehicle type 1 and creature type 12. Vehicle subtype and
creature flags come from their original tag bytes. Treating all three as bipeds
previously selected the wrong lighting category and sky fallback rules.

Authored cube samplers now enable `VK_EXT_non_seamless_cube_map` when the
device supports it. The captured TU1.5 samplers filter within each face; the
former Vulkan default blended across face seams. Actual cube resources reject
an unsupported device instead of silently selecting different filtering.

## Verified scope

Evidence root: `analysis/campaign/character-shaders-20261013`.

| Gate | Evidence | Result |
| --- | --- | --- |
| Independent original-asset census | `original-assets-v1` | All 15 campaign maps; character inheritance, resident unit definitions, model materials, section classes and six section LODs inspected. |
| Authored material/LOD admission | `native-model-admission-v3.json` | 3,123 of 3,123 accepted. The fixture explicitly supplies zero lighting/camouflage; this is an admission check. |
| Character model owner admission | Same report | 198 of 198 accepted: 179 bipeds, 17 creatures and 2 Enforcer vehicle definitions across map instances. |
| Original compiled vertex corpus | `tests/fixtures/retail_campaign_character_vertex.tsv` and provenance | 84 compiled variants across 12 families, recovered from original cache bytes. CPU and Vulkan tests pass using the existing stated numeric tolerance. |
| Focused regression suite | `focused-regressions-v4.log` | Campaign vertex CPU/GPU, original BSP light draws, retail object lighting, dynamic model shader integration and authored sampler inputs: 6/6 pass. The older BSP fixture is a base-disc diagnostic, not TU1.5 framebuffer proof. |
| Native gameplay image | `live-campaign-cube-v5/spacestation-marines.png` | Three normally placed Cairo Station Marines rendered with Vulkan validation after the typed-lighting and cube-sampler fixes. This image has no matched original frame. |
| TU1.5 source regression | `tu15-source-regression-v1.log` | Registered `original_tu15_live_bsp_effect_light_draws` passes all 27 ordered BSP packets/indices and exact light constants. |
| Launcher-selected current runtime | `installed-current-marines-v1/delivery.json` | Both launchers select the tested current `build/halo2_native`, SHA-256 `3d644b8a4dd2c60071bc27c182201170954e014deb7ba66d91dbcad88a9e25c9`. Cairo Marines and all four plasma layouts pass Vulkan validation on that binary. |

Reproduce the asset and model admission check:

```sh
python3 tools/audit_campaign_character_shaders.py --help
build-campaign-character-shaders-20261013/campaign_character_shader_probe \
  analysis/campaign/character-shaders-20261013/original-assets-v1 \
  assets/disc/maps /absolute/path/to/fresh-report.json --models
ctest --test-dir build-campaign-character-shaders-20261013 \
  -R '^(campaign_character_vertex_cpu|campaign_character_vertex_gpu|retail_object_lighting|retail_dynamic_model_shader_integration)$' \
  --output-on-failure
```

## Remaining schedule and acceptance gates

1. Close the remaining pixel differences in the authenticated TU1.5 grenade
   pass comparison. The GPU/source-state join, 27 BSP range/constant comparisons,
   and before/after color/depth/texture exports are complete. Strict pixel
   comparisons remain failing; see `TU15_GRENADE_LIGHT_FRAMES_20261013.md`.
2. Capture matched TU1.5/native character frames for each authored shader
   family, including shields, hair, self illumination and transparency. Verify
   poses, permutations, camera, lighting, depth and textures before judging color.
3. Join the remaining live shield override and camouflage clone owners, model
   permutation/RNG chronology, held-weapon ownership and unresolved pose inputs.
   Current random-region previews and missing animation bones remain explicit gaps.
4. Verify the complete character/effect presentation at multiple resolutions
   and split-screen layouts. Current launcher selection and bounded runtime
   captures are verified; repeat the complete delivery gate after remaining
   source/pixel fixes. Concurrent checkout changes must be retained. Launchers
   still report unselected game-file sources; the verified captures use explicit
   owned Xbox paths and the authenticated TU1.5 executable.

Material admission, a clean validation layer, or these CPU/GPU vertex tests do
not establish all campaign enemies/allies at 1-to-1 final-frame parity. The
existing world-impact/grenade goal also remains open for dynamic object light
receivers, contact/audio owners and matched complete gameplay frames.
