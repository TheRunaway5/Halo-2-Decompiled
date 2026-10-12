# Multiplayer atmosphere, postprocessing and visibility evidence

The native multiplayer renderer now queries the original map's atmosphere
separately for each local player's camera. The implementation loads authored
skies, fog planes, palette references, portal transitions, patchy definitions
and bloom overrides from all 21 supplied Xbox multiplayer caches. No map colors
or distances were chosen from a screenshot.

This document establishes source calculations and the bounded native rendering
path. It does not certify original Xbox framebuffer equivalence across all maps.
An independent original Ivory Tower session now supplies captured framebuffer,
guest shader constants and paused camera/root state for native comparisons.

## Authorities

- Original disc executable `assets/disc/default.xbe`, SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- Original instruction decompilation in `analysis/retail/functions/` and pinned
  reconstructed source `tools/reference/halo2-decompiled` revision
  `5f08630655e3ddc9ac45909090f11ae799fa0c27`.
- Original 12 caches in `assets/disc/maps/`; nine exact map-pack caches in
  `assets/map-pack/maps/`. Extraction hashes and offsets are retained in
  `assets/map-pack/cache-provenance.json`; the map-pack ISO CRC-32 is `DCBACC55`.
- Title-update executable `assets/map-pack/title-update/default.xbe.03`, SHA-256
  `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
  Updated player-start behavior was checked against this executable's
  decompilation in `analysis/map-pack-update/functions/`.

The executable is used only by host test oracles. The application executes
native C++ and Vulkan shaders.

## Source functions and native ownership

| Original function | Recovered behavior | Native owner |
| --- | --- | --- |
| `14A280` | Packed 24-bit BSP child traversal; positive side on an exact plane | `BspAtmosphere::cluster_at` |
| `12E5E0`, `12E960` | Camera cluster, sky/fog selection and portal-distance aggregation | `BspAtmosphere::sample` |
| `14B240` | Eye-to-portal distance, including original normal projection and square-root ordering | `src/bsp_atmosphere.cpp` |
| `12EFB0` | Primary/secondary/sky fog, clear color, patchy and bloom aggregation | `BspAtmosphere::sample` |
| `1301C0`, `1305D0`, `130DE0` | Range normalization and planar/submersion classification | `BspAtmosphere::sample` |
| `16E210` | Authored cluster fog-palette membership | `BspAtmosphere::contains_planar` |
| `134390` | Instance admission ORed over visible containing clusters | Membership table and masked `contains_planar` |
| `25A10` | Geometry fog constants c12 through c17 | `bsp_fog_constants` |
| `336F0`, `33980` | Scalar/color fog externs | `bsp_fog_externs`, `apply_bsp_fog_pixel_externs` |
| `25E50`, `26710`, `3A5C0` | Atmospheric composite words, lookup coordinates and quantized fast path | `bsp_fog_composite` |
| `1332F0`, `4CBF0` | Planar-fog implementation and geometry admission | `prepare_bsp_planar_fog`, `bsp_planar_fog_implementation` |
| `16DE20`, `16E150` | Potential/base PVS banks and dynamic-door expansion | `potentially_visible_clusters`, `visible_clusters` |
| `27AA0`, `28580`, `26880`, `26E50` | Persistent patchy motion/wind, bitmap animation and original pixel programs | `src/bsp_patchy_fog.cpp` |
| `22590`, `137A0`, `13870` | Cinematic defaults, sky bloom interpolation and final bloom parameters | `bsp_bloom_frame` |
| `214F0`, `211A0`, `20F30`, `34100` | Threshold/blur/composite programs and normalized tap coordinates | Shared bloom helpers and `BspPostprocess` |
| `2BCD0` | Atmospheric, planar, patchy, first-person and bloom scheduling | Renderer integration |
| `23C110`, `171D90` | Authored screen offsets and observer vector rotation/aim displacement | `source_observer_aim`, content loader |
| `2F970`, `2FB70` | Authored safe-rectangle frustum and rendered projection coefficients | `source_camera_frustum`, `source_camera_projection` |
| `1396C7`, `30710` | Original mode4 HUD/scope anchor and projection admission | `source_hud_aim_anchor`, HUD integration |

`BspAtmosphere` owns map metadata and source state. `BspPostprocess` owns its
per-player motion, shared RNG, noise/bloom surfaces, uniforms and descriptors.
`persistent_state()` / `restore_state()` preserve motion and RNG when GPU
surfaces are recreated after a resize. The four views share the original
unsynchronized LCG rather than independently reseeding every viewport.

`Cache::load_sky(scenario_index)` retains scenario sky ordinals. This matters
for Gemini's two skies: each camera selects its visible sky from its own fog
sample. The title's existing `load_visible_sky` entry point remains available.

## Fog classification and depth

| Classification | Source behavior |
| --- | --- |
| `0` | No active atmosphere composite |
| `1` | Atmospheric fog |
| `2` | Sky-only fog |
| `3` | Active planar fog with atmospheric state |
| `4` | Fully submerged planar fog replaces the primary atmosphere |
| `5` | Fully submerged planar fog replaces the secondary atmosphere |

Classification `2` is implemented but was not selected by the supplied map
fixtures. Fully submerged cases clear the remaining planar contribution rather
than applying the same fog twice. An authored zero/zero vertical-transition
range receives the original `1023..1024` default. When no authored plane is
selected, the source constructs a camera-forward plane; it does not infer a
water plane from world height.

Atmosphere and patchy calculations consume original W depth with far distance
`1024`, rather than treating the normal projected-Z value as a linear distance.
The native depth input is converted accordingly. The fast composite retains
24-bit depth aliasing, endpoint quantization, reciprocal limits and the original
packed constant order. Both general and fast paths retain the original
60-word pixel-program definition. Planar implementations bind the authored
gradient frames, textures and raster state separately.

Sky selection falls back from lighting sky `-1` to visible sky unless the
original sky flags prohibit that fallback. Portal aggregation uses minimum raw
eye-to-portal distance, rather than accumulated travel distance. Supplied maps
do not reference atmospheric palette mixers or directional fog; a referenced
unsupported mixer remains an explicit error. An unused empty Dune palette does
not make the active map unsupported. None of the 21 authored skies has an
animation graph.

## Map coverage

The oracle exercises each authored cluster at its bounds center and at heights
40 units below and above that center. Forced cluster selection exposes every
authored selector, including nonconvex clusters whose bounds center is outside
their leaf. The independent original leaf query is compared separately using
the same rounded 32-bit camera input.

`PVS range` is the number of set clusters in authored potential rows. Bloom
values below are rounded for readability; comparisons use original floats and
packed words. A range represents camera/sky interpolation, not a chosen native
tuning value.

| Cache | Clusters | Fixtures | Fog classes | PVS range | Bloom threshold / strength | Patchy |
| --- | ---: | ---: | --- | --- | --- | --- |
| Ascension | 7 | 21 | 3, 5 | 7 | .9 / .375 | — |
| Beaver Creek | 33 | 99 | 1 | 8–31 | .9 / .375 | — |
| Burial Mounds | 27 | 81 | 1 | 2–26 | .75 / 1 | — |
| Coagulation | 48 | 144 | 1 | 9–48 | .9 / .375 | — |
| Colossus | 34 | 102 | 0 | 24–34 | .9 / .375 | Shared depth |
| Cyclotron | 31 | 93 | 1, 3, 5 | 13–31 | .9 / .375 | — |
| Foundation | 19 | 57 | 0 | 14–19 | .9 / .375 | — |
| Headlong | 65 | 195 | 0, 1, 3, 5 | 8–63 | .9 / .375 | — |
| Lockout | 16 | 48 | 1, 3, 5 | 6–16 | .9 / .375 | Shared depth |
| Midship | 11 | 33 | 1 | 11 | .9 / .375 | — |
| Waterworks | 56 | 168 | 0, 1, 3, 5 | 8–53 | .9 / .375 | — |
| Zanzibar | 55 | 165 | 3, 4 | 11–55 | .9 / .375 | — |
| Backwash | 43 | 129 | 1, 3, 5 | 7–36 | .3 / .5 | Separate depths |
| Containment | 49 | 147 | 0, 1 | 4–40 | .8–.9 / .375–.8 | — |
| Deltatap | 21 | 63 | 1 | 16–21 | .9 / .375 | — |
| Dune | 24 | 72 | 0, 3, 4 | 10–24 | .9 / .375 | Shared depth |
| Elongation | 42 | 126 | 0 | 13–39 | .9 / .375 | — |
| Gemini | 25 | 75 | 0, 1 | 7–25 | .81–.9 / .2925–.375 | — |
| Triplicate | 28 | 84 | 1 | 14–28 | .9 / .375 | — |
| Turf | 53 | 159 | 1 | 15–53 | .6 / .4 | — |
| Warlock | 34 | 102 | 0, 1 | 14–34 | .6–.9 / .27–.375 | — |

The default threshold `.9` and strength `.375` come from the original cinematic
state. Bloom overrides include Burial Mounds, Backwash, Containment, Gemini,
Turf and Warlock. `13870` consumes threshold and strength; the retained gamma
field is raw aggregate state. No override leaves gamma zero; an active authored
override with gamma zero receives the source aggregate fallback of one.

The four active patchy banks retain all original noise frames and animation
phases. Their authored motion differs substantially:

| Cache | Animation period | Wind speed | Wind period | Acceleration / perpendicular |
| --- | ---: | --- | --- | --- |
| Colossus | 1 | 1–5 | 1–5 | .5 / 1 |
| Lockout | 100 | 0–.2 | 32–128 | 1 / 1 |
| Backwash | 10 | .02–.05 | 45–55 | .25 / .5 |
| Dune | 60 | .5 | 1–20 | .1 / .1 |

Backwash also retains authored constant wind Z `.0045`. Colossus, Lockout and
Dune use the source shared-depth quantized composite; Backwash separates layer
depths. Motion includes target changes, normalized direction fallback, randomized
turns, persistent camera transforms and the original random draw ordering.

## Visibility and planar instance admission

Every supplied BSP stores two PVS banks. The first is the potential row; the
second is the closed/base row. For a camera cluster without flag bit 1,
`16DE20` copies the potential row. For a flagged cluster it begins from the
base row, expands through open dynamic-door portals with flag bit 2, unions the
endpoint base rows and intersects the result with the potential row until
stable. The implementation preserves that algorithm and accepts an original
open-portal mask when one is available.

Gemini is the only supplied map with this dynamic-door classification: all 25
clusters are flagged and eight portals are dynamic doors. The current live
renderer uses the potential row as a conservative upper bound because native
door-state masks are not yet reproduced. The other 20 maps use the same static
PVS rows as `16DE20`.

Authored cluster geometry carries its true cluster index. Instances use
`-2-instance_index`, retaining every authored cluster membership. Fog admission
ORs only members admitted by the current mask, matching `134390`'s membership
semantics for that visible set. Geometry with cluster `-1` is retained because
dynamic object membership has not yet been recovered; no centroid-based
membership is fabricated.

The original normal rendering path is broader than this PVS operation:
`1320F0` mode 0 invokes `173130` for frustum/portal clipping and then `134390`.
Mode 1 invokes `132D60` for the radius path, and mode 2 invokes `1733E0` for the
PVS-only path. Native potential-PVS filtering therefore does not establish
complete original camera visibility or occlusion parity. In particular, the
instance fog OR can remain broader than the source mode-0 visible set.

## Original ordering and transparent boundaries

`2BCD0` schedules the atmospheric composite (`25D50`), planar layer 16 and
water before the late-world layers. Patchy fog may surround sorted transparent
object sections, but all four supplied active patchy definitions have
sort-behind-transparents bit 1 clear. In the bounded BSP path with no sorted
object sections, it executes after late-world geometry and before first person.
First-person rendering is `404B0`; `2DD30` is a bounding-rectangle occlusion-query
pass, not HUD rendering. Layer-17 emission and `13870` bloom follow first person.
HUD rendering follows this postprocessing.

Original BSP cluster `33390` and instance `33520` calls submit sort-section
`0xFFFFFFFF` to `2CBF0`. An authored layer-15 material by itself does not create
the object sort-section queue (`4C19AC`). Original object/model `133F70` supplies
real transient sort sections. If that queue is nonempty, patchy uses an early
cached pass and a later `276D0` stencil residual; applying the whole direct
composite twice would be incorrect. Native object sort-section scheduling and
the cached/residual path remain outside the implemented bounded BSP path.

Per-view GPU targets retain source proportions: half-resolution patchy noise
and quarter-resolution bloom. Filter offsets use destination dimensions as in
`34100`; each viewport is copied into its own scene surface to avoid taps
crossing split-screen boundaries. Geometry fog constants and original program
externs are updated after the prior submission fence. Original NV2A sampling,
reciprocal precision and exact framebuffer filtering are not certified by these
native GPU checks.

## Verification and reproduction

`tools/differential_bsp_atmosphere.py` verifies the pinned executable hash and
executes original atmosphere aggregation, traversal, extern getters, c12..17,
compositor and bloom evaluation. Only hardware submission is suppressed.
The final run passed **2,163 fixtures on all 21 maps** with zero state,
compositor, lookup, bloom or PVS mismatches. Float-state tolerance is absolute
`2e-5`, relative `1e-6`; all 60 compositor words and every PVS word match exactly.
Dynamic PVS is checked with closed, open and alternating portal masks.

The four patchy oracles passed **5,760 sequential camera/RNG frames**. All bitmap
indices, RNG states and 180 program words match exactly. Maximum state error
across the four banks is `1.6689300537109375e-6`, within the `2e-5` test bound.
These are instruction and state checks, not original GPU frame captures.

```bash
cmake --build build-mp-shading
.tools/python/bin/python tools/differential_bsp_atmosphere.py \
  --native-binary build-mp-shading/bsp_atmosphere_tests

c++ -std=c++20 -g -Iinclude -fno-fast-math -ffp-contract=off \
  -fPIC -shared src/bsp_patchy_fog.cpp src/patchy_fog.cpp src/transform.cpp \
  -o analysis/multiplayer-shading/libbsp_patchy_oracle.so
for map in assets/disc/maps/colossus.map assets/disc/maps/lockout.map \
           assets/map-pack/maps/backwash.map assets/map-pack/maps/dune.map; do
  .tools/python/bin/python tools/differential_bsp_patchy_fog.py --map "$map"
done
```

The generated reports are retained under `analysis/multiplayer-shading/`:
`bsp-atmosphere-differential.json`, its fixtures/inputs, and
`patchy-*-differential.json`. The cache test also exercised 2,202 authored
player-start camera queries across the 21 maps. Native split-screen captures
were inspected for viewport isolation and effect corruption, but original
frames currently cover only one paused Ivory Tower scene.

The production renderer resize regression `multiplayer_resize_tests` passed
**36 map/layout/resolution cases**: Backwash, Beaver Creek and Gemini, each with
1/2/4 player layouts, each on the same Renderer through
640×480 → 1280×720 → 3840×2160 → 1920×1080. Two submissions per case exercise
descriptor rebinding on subsequent acquired swapchain images. Every framebuffer
has the requested extent and populated viewports. Synchronization validation
reported no warning or error. All four patchy motion banks and the shared RNG
match the independent source-verified CPU kernel bit for bit after every frame,
including module destruction/recreation at resize. The fixture covers original
BSP, authored sky selection, atmosphere/planar fog, water, patchy fog and bloom;
it does not certify first-person or dynamic-object appearance. The retained log
is `analysis/multiplayer-shading/resize-regression.log`.

## Original camera and appearance evidence

`analysis/multiplayer-shading/original-xemu` contains a separate disposable xemu
session using the supplied retail disc and firmware. Its copied base HDD,
writable overlay, EEPROM, controller, capture settings and QMP socket preserve
the existing title session and user disks. Original game code and shader logic
are unchanged. The `ivory-tower-shading-baseline` snapshot is paused with an
original one-player Slayer scene, map-default SMG and 640×480 standard display.
Framebuffer, capture hashes and original camera/root/weapon float bits are
retained in `frames/provenance.json` and `ivory-paused-state.json`.

The actual original GPU camera vectors and position match the exported memory
bits. Its projection constants reveal an additional required source rule:
`2F970` expands the render frustum using the camera's safe and outer rectangles.
The standard 544×408 safe rectangle in a 640×480 view scales clip X/Y by .85,
so the rendered vertical angle differs from raw observer-camera+0x28. The
native renderer receives those source clip scales and offsets per view.

All 21 supplied caches author screen offset (0,.165) in the matg+0xF0 control
record+0x18/+0x1C. Original `171D90` rotates the unmodified aiming forward/up
vectors and computes observer+0x48 displacement. Original `1396C7` uses that
displacement and corrected projection for the HUD and scope sample anchor;
it changes only Y on successful projection and rounds with `2B460(value+.5)`.
The paused Ivory Tower source anchor is (320,284). Native physical viewports
already incorporate display aspect; the helper's Xbox widescreen pixel-aspect
correction is therefore disabled when using physical native coordinates.

`tools/differential_hud_aim.py` executes untouched `171D90`, `2F970`, `2FD90`,
`142570`, `30710`, `1396C7` and rounding instructions. Only player-context
presence queries are controlled. **864 fixtures passed**, including split,
widescreen, asymmetric safe bounds and 4K cells, with zero anchor mismatches
and maximum vector/displacement error `6.003881836136316e-8`. Saved fixtures
are `tests/fixtures/retail_hud_aim.tsv`; `hud_aim_tests` rechecks native behavior.
These proofs cover camera/anchor math. One captured original scene does not
establish pixel equivalence for every map, camera, dynamic object or effect.

## Authored match configuration used for map checks

Dune's 121 player starts have zero game/spawn selector arrays. Original player
choosers `14EF00` / updated `150640` iterate every scenario player-start position;
`23BA90` / updated `23DB90` read the position, not those selector arrays.
The equipment matcher `15A090` has equipment callers and must not reject these
starts. Native active-BSP eligibility now accepts them. Its existing distance
ranking remains a boundary from the complete original spawn-score calculation.

Midship and Waterworks normally start with a plasma rifle. Native battery/heat
combat is still unsupported. For live map appearance checks, the original valid
SMG starting option is available: variant parameter `0x59`, value `5`, resolved
by `19EDA0` through `matg+0x16C` to `mulg` Runtime Weapons index 1. Default value
`0` retains the map's authored equipment. Other unfinished options remain
explicit errors. This is a changed match configuration, not a claim that the
two maps' default weapon simulation is complete.

`tools/oracle_multiplayer.py` executes option 5 against the actual weapon banks
of all 21 caches. The focused native multiplayer suite joins four players with
that option on every cache, checks the authored SMG datum and 60 loaded / 120
reserve rounds, and retains default-energy-weapon rejection. The Dune-shaped
zero-selector fixture covers initial join, death and respawn.
