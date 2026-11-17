# Sun stencil shadows

With pStencil active, dynamic owners the map's sun reaches cast a capped
stencil volume along the sky's authored incident direction. This is a
current-asset adaptation of the user's rule that the sun always casts dynamic
stencil shadows; the May 2003 build only had the lightmap-incident composite
described in [the shadow reference](SHADOW_LIGHTING_REFERENCE.md). Original
mode never collects or draws these volumes.

## Admission

`sun_shadow_direction` (`src/pstencil/lightmap.cpp`) returns the incident only
when all of the following hold for the owner's centre:

- its cluster names a visible sky with an `ObjectSkyLightingDefinition`;
- that sky's light frame tests collision and has non-zero radiosity;
- a 100-unit collision ray toward the sun (flags `0x2b`, first collision BSP)
  has no contact with an authored material. A miss or a sky contact
  (material NONE) is sunlit.

Otherwise the biped/vehicle call sites fall back to the 2003 lightmap-incident
caster: a light origin 20000 units along the baked incident and a 2-unit
extrusion. Other owners then cast nothing.

## Volume length

Sun volumes used to run the ordinary view's full 1024 units, which darkened
every surface inside that slab: rooms under the floor, ground behind walls.
`sun_shadow_extrusion` now bounds each caster. It takes the caster's bounding
sphere (centre plus the furthest posed vertex), starts five rays on the
sphere's sun-facing tangent plane and sends them away from the sun: one
through the centre and four at the lateral radius. The length is the centre
contact plus 1.5 times the largest lateral excess plus a 0.05 bias, capped at
1024. Over a planar receiver that covers every caster vertex, and the volume
ends at most about one caster diameter past the first visible surface. A sky
contact or a miss on the centre ray keeps the 1024-unit length, because there
is then no nearer receiver to stop at. `append_sun_volume` rejects lengths
outside `(0, 1024]`.

All sun and lightmap prisms of a view still share one stencil mask, applied
as the single 152/255 multiply before environment and dynamic lights
(`src/pstencil/render.cpp`). The shadow is a uniform darkening; it does not
use the sun's colour.

## Casters

| Owner | Multiplayer | Campaign |
| --- | --- | --- |
| Player/actor bodies, vehicles | Sun, else lightmap fallback | Sun, else native lightmap incident (object types 0/1) |
| Held weapons | Sun only | Sun only, all draw units of the owner |
| Corpses, loose weapons, dynamic objects | Sun only | Sun only |
| Authored scenery, props, machines | None (baked lightmap shadow) | Machines and scene objects: sun only |
| Structure BSP, instanced geometry | None (baked lightmap) | None (baked lightmap) |
| Camouflaged owners | Thinner in proportion to the camouflage amount; none when fully camouflaged | — |

Multiplayer marks dynamic draws with `ModelObjectDraw::pstencil_sun_caster`
at their producers and collects them after the posing lane has joined, so the
lane contract for `pstencil_lightmap_casters` is unchanged.
`CampaignShadows::add` now extends an owner's caster when the same object is
submitted as several draw units; before, only the first unit was kept.

## Two composites per view

The 2003 layer 6/7 composite (all sun and lightmap prisms into one stencil
mask, then one 152/255 multiply) now runs twice per view.

1. **Model pass** (unchanged position, right after the dynamic models are
   drawn). It tests against the models' own depth, where a lit caster surface
   cancels its prism exactly. The completed world depth does not exist yet at
   this point, so before 2026-11-11 this single pass could only darken model
   pixels: no world surface ever received a sun or lightmap shadow.
2. **World pass** (just before the dynamic lights, after the world is
   complete). The same prisms are rasterised against the finished depth. Then
   every caster's own posed triangles are drawn with the *owner* pipeline
   (stencil ALWAYS, REPLACE with the 0x80 reference, depth LESS_OR_EQUAL,
   pulled 4096 D24 steps toward the camera, no colour or depth writes) to
   restore the unshadowed reference over the casters' visible surfaces, which
   the model pass already resolved. The multiply then darkens world receivers
   only, so nothing is darkened twice.

`Frame::lightmap_owners(view)` holds that owner batch: every posed triangle
of every sun/lightmap caster of the view, including owners whose camouflage
casts nothing. `HALO2_REFERENCE_PSTENCIL_NO_WORLD_SHADOWS=1` omits the world
pass for a paired isolation capture.

A depth bias on the prisms was tried first and rejected: the caster's own
over-darkening had no clean threshold (995 px at bias 0 falling to 277 px at
1024 D24 steps while the ground shadow stayed at about 590 px).

## Known limits

- World geometry never casts a sun stencil shadow; world-on-world shadowing
  stays baked.
- The local player's own body is not posed in their first-person view, so it
  casts no sun shadow there. Off-screen owners cast none either.
- Admission is one binary ray from the owner's centre. An owner crossing a
  shade boundary switches between the sun volume and the fallback in one
  frame.
- A player's body and held weapon read the unit's camouflage amount
  (unit+2B0): `camouflaged_shadow_opacity` maps it through the original
  17-pattern stipple bank, and a fully camouflaged player casts nothing.
  Vehicles and the separate 2003 shadow-LOD fade are not read.
- Only the first collision BSP is queried.
- With pStencil active the retail projected object shadow's GPU packets are
  omitted (`raw_source_shadow_gpu_disabled`), so a caster has one shadow in
  single view as it already had in split screen (`fixed_enabled=0` there).
  Native selection and history still run, and Original keeps the retail
  pass. `HALO2_REFERENCE_PSTENCIL_RETAIL_SHADOW=1` keeps the packets for a
  paired capture.
- The world pass also gives the lightmap-fallback casters (owners standing
  in baked shade) their 2003 two-unit prism on world surfaces, which this
  port had never shown before. Looked at in two scenes:
  - Coagulation base, Warthog, Ghost and a Spartan in the base's shade
    (`analysis/pstencil/world-shadows-20261111/shaded-vehicles-world-pass.png`):
    each gets a short contact shadow on the ground under it, 1,601 world
    pixels, none brightened.
  - Lockout corridor, one Spartan indoors
    (`analysis/pstencil/world-shadows-20261111/lockout-indoor-fallback-shadow.png`):
    a 319-pixel floor shadow from the feet along the lightmap's incident
    direction, none brightened.
- The fallback prism keeps the recovered fixed length of two world units.
  It is not bounded at its first receiver the way the sun volume is, so a
  caster standing on a floor thinner than the rest of that length can
  darken a surface behind the floor. That needs the caster and the far
  surface in the same view: an owner hidden by the floor is not drawn and
  casts nothing (Lockout, viewer on the lower floor looking up at the
  ceiling under a player 3.5 units above: 0 world-pass pixels). No frame of
  the visible-caster case was produced.

## Verification (2026-11-11)

- `pstencil_original_world_positions` (`check_sun_volume`): prism
  construction, a caller-bounded far cap and rejection of lengths outside
  `(0, 1024]`.
- `pstencil_sun_shadow_bound` (`tests/pstencil/sun_shadow_tests.cpp`) on the
  prepared MCC Coagulation cache: the retained sunlit spawn gets the unit sky
  incident (0.880, 0.077, 0.469); its receiver lies 1.51 units from the
  caster's sun-facing plane and the bounded volume is 3.13 units instead of
  1024. A caster raised 10 units gets a longer bounded volume, and degenerate
  inputs keep the far distance.
- Deterministic Coagulation capture (tick 6, 960x540) on the changed build:
  the Original frame is byte-identical to the published runtime
  (`473567429a30ef25...`). The first-person and top-down pStencil frames are
  also unchanged although two additional sun casters are collected; neither
  view shows a cast shadow.
- `--mp-reference-third-person` (diagnostic, requires `--mp-reference-camera`)
  now exposes the existing third-person capture option on the command line.
  The camera must be placed from the `player_position=` value a first-person
  capture prints; `--mp-reference-root` does not move the player. At the
  seeded Coagulation spawn (97.99, -175.64, 0.91) the Spartan stands in baked
  cliff shade, so the frame shows pStencil armour and visor shading but no
  sun shadow.
- Sunlit body in a frame (two local players, Coagulation, tick 100, player 2
  on the retained sunlit spawn 38.80, -60.88, 1.22 seen from player 1):
  `analysis/pstencil/world-shadows-20261111/sunlit-spartan-original-pstencil-control.png`.
  Against the no-caster control (`HALO2_REFERENCE_PSTENCIL_NO_SUN_SHADOWS=1`)
  the frame has 1,175 darker and 0 brighter pixels. The world pass alone
  adds 724 of them: 603 in the ground streak beside the Spartan in the
  predicted sun direction and 103 inside the body's screen box (ground seen
  between the legs and at the feet). The model pass alone
  (`HALO2_REFERENCE_PSTENCIL_NO_WORLD_SHADOWS=1`) darkens 527 pixels, all on
  models, and none on the ground: that is the pre-change behaviour.
- Camouflage in a frame (two local players, Headlong, player 2 placed on
  the active-camouflage pickup, camouflage amount 0.733):
  `analysis/pstencil/world-shadows-20261111/camouflage-none-thinned-full.png`.
  In the box around the camouflaged player's feet the shadow covers 28
  pixels, against 113 with `HALO2_REFERENCE_PSTENCIL_IGNORE_CAMOUFLAGE=1`:
  a ratio of 0.248, where the stipple bank predicts 4/16 for that amount.
- Single shadow in a frame (one local player beside the Coagulation
  Warthog, tick 120, `fixed_enabled=1`): keeping the retail packets with
  `HALO2_REFERENCE_PSTENCIL_RETAIL_SHADOW=1` darkens 15 additional pixels
  and brightens none, so the default pStencil frame draws no retail
  projected shadow. The difference is small because the only selected
  retail root in that view is at the bottom edge of the frame; a larger
  before/after of a biped's doubled shadow has not been captured, because
  the first-person view does not select the local player's own shadow and
  split screen never enables the retail pass.
- Original identity on a real image: Coagulation tick 100 (76,870 distinct
  colours) is byte-identical between the changed build and the published
  runtime (`4c6821173ebb3a6c...`). An earlier note compared Elongation
  frames at tick 6; those frames are inside the start-of-match fade and are
  entirely black, so that comparison proved nothing and is withdrawn.
- `pstencil_sun_shadow_bound` also checks the owner batch: a fully
  camouflaged owner casts nothing but stays in its view's exclusion batch,
  and no other view receives it.
