# Campaign character shader work

Visual authority: **OG Xbox TU1.5 with original Xbox cache assets**. Vista is
not an authority for gameplay materials, lighting, textures or effects. This
work remains partial until matched original/native rendered frames pass.

Native runtime validation now uses the selected MCC private cache and its
shared resources, with Original graphics and native TU1.5 logic. Xbox cache
censuses below describe the visual reference and historical component evidence;
they do not qualify the new MCC runtime lane. The October19 retained-shader
metadata regression passes on the actual MCC introduction cache. Full current
startup, rendered campaign coverage and matched character frames remain open.

The October19 Cortana repair now preserves the authored layer7/pass0,
layer15/passes1,2,3 and independent layer17/pass4 composition for hair, body,
arms, head, eyes and teeth. The NONE bitmap in the actual hair late pass resolves
to the actual rasterizer default image using the original 16FC8 contract.
Eight managed component checks pass, including the old-binder negative
control, exact pass inputs/order and rejected-bind input preservation; whole
renderer compilation passes too. Evidence is under
`analysis/current-root-cutscene-20261019/character-late-layer-v1/component-source-v3/`.
This is CPU binding/phase evidence. Live Vulkan ordering and matched original
Cortana frames remain open, as do actor overrides and per-owner constants.

## Active campaign acceptance checklist — October16

The user has made campaign the current implementation priority. The complete
scope includes every resident character definition across all15 original
campaign maps, enemies and allies, player/cinematic variants, creatures and the
character-owned Enforcer. The existing198 map-local model admissions and3,123
material/LOD admissions are historical asset checks, not live coverage or
matched-frame acceptance.

The October16 follow-ups explicitly include the opening's fleet/Reach backdrop
(`x01_scene_03`, the “all the ships in my command” line), intact-Halo background
(`x01_scene_04`) and destruction background (`x01_scene_07`). They also require
comparison of the cutscene engine against the original: script wakes, camera
clocks, object lifecycle/visibility, sound requests and playback pacing. The
reported sound stalls have a measured native object-update/queued-submission
cause in the retained opening run. The fixes now have a separate live latency
check; complete original voice allocation and final audio mixing remain open.

The live [cutscene comparison](CUTSCENE_TU15_COMPARISON_20261016.md) records
original-clock tests, corrected wake/frame-effect scheduling, per-cluster sky
selection, restored Reach FLOAT3 geometry, cinematic safe-frame projection and
ordinary source point-camera/observer adoption. The combined frozen-ABI run
matches all24 original sound request ticks through2800 and admits them at the
same source tick (maximum measured adapter delay1.892547ms), with nonzero native
PCM. Reach is visible at actual Vulkan tick1558 and Halo remains at1770.
Strict camera forward/up comparisons, destruction effects, original final-frame
joins and current-checkout runtime delivery remain open.

- [ ] Replace ordinary actors' preview region/LOD and idle/action skinning with
  the completed original collector and deferred WORLD pose. Retain original
  section order, full salted identity, shader coverage and material namespaces
  across BSP changes. Validate armed actors as well as cinematic objects.
- [ ] Cover live bipeds, creatures and character-owned vehicles, source births,
  retirement/corpses and actual visibility. The current campaign adoption loop
  only imports actor-bound type0 objects; a broad asset census does not close
  the type1/type12 or corpse paths.
- [ ] Carry each owner's actual ordinary lighting, shader functions and colors;
  connect original PRT and dynamic explosion-light banks and their chronology.
- [ ] Complete shield/active-camouflage overrides, hair/transparency,
  self-illumination and authored attachments/held weapons in original pass order.
- [ ] Restore per-camera-cluster skies and verify the separate fleet/Halo matte,
  ship and explosion-effect draws against original scene03/04/07 frames.
- [ ] Compare original/native cutscene clocks, wake order and sound-request
  timing; reproduce and remove the reported audible stalls without changing
  authored delays to hide a backlog.
- [ ] Pass matched OG Xbox TU1.5 frames for the authored material families and
  affected poses/permutations/LODs/states, with camera, textures, color and depth
  aligned before evaluating the composed image.
- [ ] Verify resolution, resize, split-screen applicability and freshly built
  combined runtime/launcher delivery.

`tools/campaign_character_parity_checklist.py` builds a per-map/per-owner ledger
from the independent original cache census and historical admission report.
It preserves all authored region permutations and six section LODs, rechecks
the complete15-map cache/report identities and keeps live/frame gates open.
It must not silently accept missing census reports or substitute the old
admission test's explicit zero-lighting/camouflage input for a live result.

The refreshed `original-assets-v2` census verifies all15 cache/report identities:
198 map-local units,48 distinct unit paths,177 map-local models and1,483 shader
entries. A separate `supplemental-proxy-assets-v1` census retains145 additional
map-local roots/51 distinct paths, including cinematic props, dress Marines,
character scenery and detachable parts. Its discovery criterion is explicit:
character-namespace objects or objects sharing an included unit's render model.
This is not yet an exhaustive arbitrary-attachment reference closure.

The subsequent `authored-dependencies-v1` traversal follows the declared common
object attachment rows and model-variant children/looping-effect references,
retaining raw rows and identities. It reaches four additional map-local child
weapon roots, inventoried separately in `authored-child-assets-v1`. Combined
scope is347 object roots,317 map-local models and1,525 map-local shader entries.
`campaign-material-families-v1.json` and its TSV index enumerate all39 distinct
authored template paths, their maps/owners, original layer masks and compiled
vertex program names. Every family's original-frame gate remains open.

This declared graph does not close script/computed spawns, runtime equipped or
first-person weapons, damage debris, arbitrary out-of-namespace proxy roots or
runtime override/clone materials. Effect/light/lens/contrail/sound references
remain graph leaves rather than falsely verified effect implementations.

The new immutable source-model v3 CPU run passes347/347 root admissions across
all15 maps (198 units,145 proxies and4 authored children), with zero owner or
material failures. It checks2,616 authored root material slots and6,359 resident
slots. Its actual Cairo `ai_place` case produces a Marine and a separately built
held weapon, with53 body WORLD nodes and one weapon node. Four ordinary main
units produce6,327 vertices matching the independently decoded authored section
and shader/LOD selection; seven corrupt-input rejections and the unresident
no-draw control pass. Three opening geometry-owner cases also pass. The ordinary
actor production route now uses completed source selection and deferred WORLD
poses, but live held-weapon submission, other object types, overlays and complete
original frame comparisons still keep the acceptance checklist open.

The terminal receipt is `source-model-validation-v1/cpu-result-v1.json`. Its
selected binary/object/archive inputs are pinned, and four source-copy gaps from
concurrent edits are explicitly recorded. This is evidence for that immutable
candidate, not the latest checkout or installed runtime.

The machine-readable `campaign-parity-checklist-v2.json` under the evidence root
preserves both scopes and every authored material slot, region permutation and
six-LOD section reference. The base3,123 historical admissions remain separate;
the supplemental roots have no inherited admission or visual pass. Zero matched
original character frames are currently recorded. The ledger generator rejects
missing/duplicate maps, absent/modified reports, changed cache identities and
mismatched shader/model admission rows; seven actual rejection controls pass.

| Original campaign map | Unit owners | Historical material/LOD checks |
| --- | ---: | ---: |
| 00a_introduction | 9 | 176 |
| 01a_tutorial | 4 | 60 |
| 01b_spacestation | 14 | 257 |
| 03a_oldmombasa | 9 | 166 |
| 03b_newmombasa | 10 | 183 |
| 04a_gasgiant | 15 | 212 |
| 04b_floodlab | 16 | 141 |
| 05a_deltaapproach | 12 | 256 |
| 05b_deltatowers | 15 | 272 |
| 06a_sentinelwalls | 19 | 302 |
| 06b_floodzone | 13 | 198 |
| 07a_highcharity | 24 | 328 |
| 07b_forerunnership | 14 | 195 |
| 08a_deltacliffs | 10 | 157 |
| 08b_deltacontrol | 14 | 220 |

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
| Historical launcher/runtime checkpoint | `installed-current-marines-v1/delivery.json` | Both launchers selected `build/halo2_native`, SHA-256 `3d644b8a4dd2c60071bc27c182201170954e014deb7ba66d91dbcad88a9e25c9`, for the recorded Cairo Marines and four plasma-layout captures. This does not verify the subsequently changed source or today's installed runtime. |

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
