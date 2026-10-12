# Original world geometry admission

This records the original cache/executable conditions and the integrated native seams. It does not authorize substituting a generic frustum or distance heuristic. Existing native material and PRT input proofs remain valid for their selected source section banks.

## View construction and visible owners

`1320F0` chooses the main/secondary context, calls `132B80`, then constructs the source portal/PVS visibility through `173130` (ordinary view), `132D60`, or `1733E0` according to the context mode. `132B80` retains the actual view/frustum bank, clip sphere, context flags and optional prior visibility context. `134390` builds BSP/instance admissions; `132220` traverses the visible cluster banks and their linked object lists, retaining source traversal order and duplicate-visit stamps. These precede `133720`'s object draw-unit preparation.

`133050` uses the object's cached `+30/+3C` sphere for ordinary views, or `+50/+5C` for the special context. `132E60` selects sphere-volume admission when context `+2A7C == 1`; ordinary mode calls `165010`. That routine visits each cluster's actual source portal-frustum entries and calls `181800`. The latter first checks the source frustum AABB, then its six planes, and returns outside/intersecting/inside classification. A small-sphere ordinary path and optional context object-list lookup alter the containment flags. The actual portal-frustum banks, cluster linkage, duplicate stamps and these flags are required caller state.

`163C80` appends admitted owners into the fixed source visible list. `134240` moves objects with resident PRT metadata to its front. `133720` then walks that list; preparing every authored object in the native world is not an equivalent visibility producer. Native `SourcePropScene` and `SourceMachineScene` currently retain authored geometry and lifetime visibility; original portal/frustum admission remains separate.

`source_visibility.hpp` / `source_visibility.cpp` now implement the bounded
`181800` sphere classification and `165010` ordered range aggregation. The
original classifier rejects against all six planes, but its contained test
omits plane4. It preserves the SSE order `(z*nz+y*ny)+x*nx-d`; equality is
intersecting. An untouched executable differential covers 8,510 classifications
and 2,048 aggregations, including 2,486 observed calls and contained early
stops. The 512+256 frozen cases pass normal and sanitizer builds. These
consumers require the actual constructed frusta and cluster ranges; they do
not replace portal traversal with a containing-cluster lookup.

The multiplayer renderer now constructs the ordinary source view through
`2F970/2FB70→441B0→173130→134390`. Each view owns its actual projection
rectangle, camera basis, containing cluster, portal mask, constructed frustums,
cluster queue, instance queue and fog-membership flags. `ModelView` retains the
separate `SourceBspAdmission` result through the immediate Vulkan draw. BSP
cluster vertices and authored instance vertices use those queues, including
the original queue limits and retained prefixes. Planar-fog selection consumes
the actual queue's `0400` flag. The coarse potential-PVS filter remains for
callers that do not provide this source bank.

`16E210` consumes the current `BspFogSample.planar_datum`, explicit active
scenario/BSP availability, and the ordinary native disabled `4E64C8` override.
The authored scenario flags are not this override. Signed-short camera
rectangles are obtained from the same physical outer/safe viewport endpoints
used by the existing source projection. Native ordinary cameras have no
subrectangle, screen-offset or auxiliary-layout service active. Physical
Vulkan viewport aspect already includes the Xbox widescreen pixel aspect;
the extra source correction is not applied twice.

The persistent Gemini portal mask binds the synchronized original owner
identities once at BSP initialization. Cold tick0 retains `210DF0`'s initial
all-open mask; `1152E0` updates it only when the actual native simulation tick
advances. Destroyed owners restore their assigned portals before removal.
New generations retain the original constructor `NONE/0` range until an
actual whole-BSP activation calls the binding service. Door/platform position,
group targets and device flags currently retain their independently verified
initial state; automatic door controls, shared-group progression and script
activation are still required live producers. These inputs are not simulated
from camera visibility. See `SOURCE_PORTAL_FRUSTUM_PARITY.md`,
`SOURCE_BSP_ADMISSION_PARITY.md` and `SOURCE_PORTAL_MASK_PARITY.md` for the
instruction differentials and exact boundaries.

The shader camera is independently produced by `2FD90→12D50→12FA0`, rather
than reusing the clipping camera's inverse. It reconstructs perpendicular up,
normalizes in the original PC53/SSE order, and retains the exact ordered matrix
products. The multiplayer caller now passes direct source `c0..6` banks for
world and reflected sky, and the separately computed `0.15` first-person
`c0..3`. The original `2BA10→132D0` camera copies establish that `48565C`
is the submitting camera's far distance; the source constants preserve its
`16777215/far` scale. Native shader execution reads these words from its own
view's material bank instead of reconstructing them from a host matrix.

The helper matches 8,226 original cases / 625,176 words, including 56 actual
retained world/FP GPU words. All 18 distinct admitted world programs across
the 21 caches preserve raw homogeneous W; reciprocal W stays in a temporary.
The native clip conversion therefore keeps that raw W. The two-map GPU
controls independently exercise source world and actual arms/weapon camera
banks in each player view, restore their images, and repeat after resize.
See `source-view-projection.md` for the instruction evidence and numerical
boundaries. The legacy host matrix remains for callers without a source bank.

## Geometry LOD

The source path is `133790 → 4B5D0 → 4BCC0 → 4C450 → 4C190/4C2B0`. Authored shader LOD and render-model geometry LOD are separate inputs. Ordinary BSP shader selection now consumes each view's source coverage and part metric through `1BF50`; see `SOURCE_BSP_SHADER_SELECTION.md`.

`133790` computes coverage using the cached sphere, the absolute inverse-view depth plane clamped to the source minimum, pixel projection scale `4B9ED0`, diameter scalar and `4BA028`. Its separate draw-distance metric uses the cached sphere center, the render camera and `4B9DCC`. Literal instructions retain x87 double precision through coordinate subtraction, squared length, square root and camera-scale multiplication, then round to the stored float. `source_geometry_lod_distance` preserves that order; the general float `length` helper is not interchangeable at source LOD boundaries. Literal `4BCC0` loads that draw-distance metric into XMM4 for `4C450`; projected coverage is not the geometry selector's input. Before selection it multiplies the metric by 0.3 when source global `4BA019` is set, and by the source `45E384` scalar when object `+10A` bit2 is set. `4C450` validates the authored cutoff sequence, uses exact source defaults when invalid, computes a transition fraction, and applies one-step hysteresis from the submitting player's prior LOD through `3DDD0`. `4C190` uses those actual bytes and preserves the previous resident LOD when the requested source sections are unavailable. `4C2B0` orders the actual active model regions, preserving duplicate section references.

`223080` passes the actual player count to `2C490`, immediately after camera production. Literal original policy:

The transition fraction is a hysteresis intermediate, not a draw blend. Literal `4BE49` sets EBX to the consumed distance argument's stack slot; `4C450` overwrites that slot and reads it for its 0.8/0.2 hysteresis decisions. After `4BE52`, all 147 remaining `4BCC0` instructions neither read nor take the address of that slot, including both later calls. Only the integer LOD is stored in the context by `4B5D0`. It therefore does not reach `133F70`, `2CBF0`, a material opacity, a morph stream or a texture transform. `tools/audit_source_lod_transition.py` verifies the pinned original instructions and stack adjustments; the retained report is `analysis/multiplayer-shading/lod-transition-consumer-proof.json`.

| Players | Ordinary maximum geometry LOD | Coverage multiplier `4BA028` | `4BA02C` |
| --- | --- | --- | --- |
| 1 | 4 | 1.0 | 1.0 |
| 2 or 3 | 3 | 0.800000011920929 | 0.5 |
| 4 | 2 | 0.699999988079071 | 0.25 |

`4BCC0` caps PRT geometry at LOD4 instead of the ordinary maximum. Actual object flag19 forces LOD5 before this cap; a first-person admitted unit follows its separate LOD rule. The paused original Zanzibar wheel/flywheel/blades/crane objects all have flag19 clear. Their retained flags are `80000B20`, `80000B00` or `80000300`.

The authored metadata audit in `analysis/multiplayer-shading/object-geometry-lods.json` covers all 415 automatic rendered BLOC placements and 48 automatic rendered MACH placements. The remaining automatic MACH has no render model. Every ordered LOD4 array equals its LOD5 array. LOD0/1/2 differ from LOD5 for 224 props and eight machines; LOD3 differs for 213 props and eight machines. This equivalence proves shared geometry at levels4/5; it does not establish the actual per-view level selected at runtime.

The same player-count routine changes feature bytes `4BA020..25`: one player writes `[1,0,1,1,1,1]`; multiple players write `[0,1,0,0,0,0]`. Original shadow/light/postprocess callers read these separately. They should be decoded by their actual consuming path, rather than treated as a blanket geometry or shader-quality setting.

`source_render_policy` now supplies `443B2..443DF`'s three layer gates: layer2
reads `4BA023`, layer8 reads `4BA022`, and layer17 reads `4BA024`. The
multiplayer `ModelView` carries the actual `2C490` bank. The renderer identifies
each explicitly prepared pass through its selected authored template layer and
suppresses only that pass. This covers independent overlays, layer8 reflection
passes (including Chief arms), and layer17 alpha-mask geometry. Layer3 lighting,
its environment child, layer7 emission, fog and the postprocess itself retain
their own admission. Other feature bytes and context/available-layer masks
remain separate consumers. The untouched instruction slice matches all 2,240
explicit-bank cases; the frozen cases pass normal and sanitizer builds. Six
GPU layer8 controls match independently removing the authored pass, visibly
change the targeted viewport, and preserve the other player's lighting through
two resolutions and a return resize.

The native chooser and per-view history fixtures match untouched `4C450`, a valid retained `3DDD0` cache, `2C490` and the exact `4BCC0` cap instructions for 10,044 cases / 130,572 words. A separate untouched `133790`/`2F970` instruction oracle matches 8,192 cached-sphere metric/camera-flag/scenario-type cases / 16,384 words. The checked-in fixtures freeze 257 chooser and 131 metric cases. `2F970` sets the special modifier from actual render-camera `+58`/`+68` bytes or scenario `+10 == 2`; `2C560` has a separate special-view producer. This does not infer portal admission from a distance.

## Native MACH/BLOC integration

`SourceObjectGeometry` decodes all six source collectors before renderer construction, retaining region order and duplicate sections. Ordinary material slots share only within the same owner and source shader; PRT slots remain independent by owner, geometry LOD, section and shader. Pose changes refresh every resident bank, while each local view appends only its actual selected geometry. Empty collectors keep previous history and produce no lighting query or material binding.

The multiplayer caller supplies `133790`'s exact cached-sphere distance, the actual camera/grid scale (including retained reference-camera scale), local player count and the primary scenario type. All 415 BLOC and 49 automatic MACH untouched cold constructor prefixes have object flag19 and control `+10A` bit2 clear. Current native handlers retain those initial inputs; later HSsetLOD/control changes require their real producer. The normal camera currently has its special `+58` and `+68` features inactive. This is an ordinary view input, not a substitute for the source secondary-view producer.

`update_pose` advances/poses resident geometry. `append_view` evaluates generic lighting for every admitted view using its unmodified camera-distance input, before part shader selection. Its actual last-query-frame guard deduplicates completed queries; a later view can query when an earlier view skipped for cadence. Each view retains the bank available at its own submission. Native resources are synchronously decoded and uploaded. The original `12DE70` asynchronous queue/completion and previous-resident fallback are not inferred from on-disc metadata: all 407 supplied resource descriptors have cached byte0 and handleNONE before live loading. The original residency/relocation oracle independently covers 102 bounded cases.

`SourceSceneryScene` now provides the separate resolved SCEN seam: six exact collectors, per-section baked incident/RGB and independent per-view LOD history, with explicit cached-sphere and historical initial-selection services. Its bounded CPU/GPU coverage and remaining inputs are recorded in `SOURCE_SCENERY_PARITY.md`. Players and held objects still require their respective source geometry/stream admission paths. Portal/PVS owner traversal, partial subpart masks, secondary views and source sorting remain additional producers.

## Part and subpart ranges

`133F70` emits selected region/section draw units into `2CBF0`. That scheduler retains a source sort-section ID (`param13 & 31`); IDs other than31 update `4C19AC` and its transparent queue bank. An authored layer15 alone does not establish that queue admission.

For partial visibility, `2CBF0` allocates the actual section bit bank with `1332B0`; `44720` calls `44E90` using the section's portal-frustum references, or `45560` for sphere-volume contexts. `44E90` transforms original section bounds/node matrices or traverses the section visibility acceleration data. `458D0` marks actual subparts; `45A80` expands parts into their authored subpart ranges. Native triangle expansion now retains each original section/part identity, and model origins additionally retain the original source vertex. BSP instances retain their authored definition section independently of their instance ordinal. Renderer batches keep these identities separate even when parts share a shader/lightmap. The source subpart visibility bit bank remains a separate producer.

Final drawing is `4D0B0 → 4DAA0`. The whole-part path uses authored 72-byte part fields `+6` index first / `+8` index count. The partial path uses `+A` first subpart / `+C` subpart count and eight-byte subpart records. It tests the exact source visibility bit for each subpart, coalesces adjacent list ranges, and emits each admitted strip subpart separately. Replacing that mask with material-only grouping changes submitted ranges and transparent ordering even when the material program itself is exact. The unchanged-original controls are retained in `analysis/frontend/title-bsp-admission-audit/PART_RANGE_SOURCE_AUDIT.md`.

## Integration boundary

A complete native collector needs actual per-view portal/PVS admissions, cached source spheres, per-player previous LOD/residency state, active region bytes, section/subpart identity, and sort-section queue identity. Selected sections may use distinct node palettes and PRT streams/banks. Per-view selection therefore must retain geometry and the corresponding per-section material bank together; changing one shared mesh bank in place can leak between player views. The native MACH/BLOC seam now preloads the exact LOD0..5 union and retains one signed LOD byte per owner/source player. `append_view` consumes explicit normalized distance and modifier flags, selects geometry and its matching LOD/section/PRT material together, and evaluates the owner cache for each admitted view before part shader selection, independently of selected vertex counts or ordinary-shader masks. The last-query-frame guard deduplicates completed queries, preserving later-view cadence admission. Original `4C450`/`2C490`/cap arithmetic is independently bit-exact; actual resources are decoded before renderer allocation. Empty collectors retain NONE history. Native residency does not reproduce the Xbox asynchronous resource queue or its prior-LOD fallback. Portal/PVS/frustum/subpart/sort-queue inputs remain separate; current all-map captures and bank fixtures do not close those remaining admission/order producers.
