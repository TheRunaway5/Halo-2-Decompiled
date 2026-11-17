# Lights in pStencil: what casts, what is coloured, what is still declined

The standing rule is that with pStencil active every point light and the sun
cast dynamic stencil shadows and keep their authored colour. This page records
what the current source does against that rule, from the code and from an
inventory of every resident light definition. The sun is covered in
[sun stencil shadows](SUN_SHADOWS.md).

## Inventory (2026-11-11)

`tools/pstencil/audit_light_coverage.cpp` (target
`pstencil_audit_light_coverage`) reads every `ligh` definition in the given
caches and classifies it with the native effect-light owner's own admission
(`source_effect_light_definition`). Across the 40 maps of the prepared MCC
cache ([report](../../analysis/pstencil/light-coverage-20261111/coverage.json)):

| Class | Definitions (summed over maps) | Unique | Meaning |
| --- | ---: | ---: | --- |
| Sphere, untimed | 1,032 | 76 | Accepted |
| Sphere, timed | 949 | 36 | Accepted |
| Frustum (shape 2, one shape 1) | 470 | 89 | Declined: needs the projected-light owner |
| Sphere with animation blocks | 140 | 17 | Declined by the standalone owner; see below |
| HSV-interpolated only | 0 | 0 | The check exists but no shipped light trips it alone |

The animated spheres include the lights the rule was written for:
`emergency_no_shadow` and the Cairo elevator `emergency` (authored 1,0,0),
`flashing_red`, the sparking and flickering fixtures, `fire_medium`, Cortana's
glow, and the timed explosion lights `shell_fire`, `fusion_coil_destroyed`,
`turret_destroyed`, `module_destroyed` and `plasma_grenade_charge`.

The frustum set is dominated by flashlights (43 maps), vehicle headlights
(Warthog 32, Scorpion 30, Mongoose 27), Ghost/Wraith antigrav lights, the
hanging and hinge lamps of Street Sweeper, Turf, Zanzibar and Cyclotron, and
the authored campaign spot, fill and cinematic lights.

## Colour

Light colour is not a pStencil-specific path. `source_effect_light_state`
(original C0C80) reads the authored diffuse and specular bounds, and
`source_effect_light_uniforms` multiplies them by brightness into the C6/C7
shader constants used by the diffuse, utility and dependent-light passes. A
light that is admitted therefore lights with its authored RGB; a light that is
not admitted contributes nothing. No pStencil pass whitens or replaces it.

## Animated spheres through the completed native record

Original C2CB0 calls C2B30 every tick to evaluate a definition's brightness,
colour and gel animation blocks (+C4/+CC/+D4) into the light record. The
standalone lifetime owner has no such evaluator, so
`source_effect_light_definition` rejects those definitions.

pStencil's direct attachment pool (`src/pstencil/point_lights.cpp`) already
copies the completed native record for attachments that need it
(`attached_lights_tu15`: size, control, diffuse, specular, brightness, frame).
It now loads its definitions with `load_completed_effect_light`, which accepts
an animated sphere, and marks such an attachment `native_required`: it is lit
only from that completed record and never ticked by the standalone owner. The
resident world and model light banks use the same loader so they can draw it.
Frustum and HSV definitions still throw, and `load_effect_light` is unchanged
for every other caller.

Evidence on Elongation (prepared MCC cache, deterministic capture, spawn
view): the published runtime submits no light; the changed build submits the
two emergency lights at (±5.5, 0, 1.71) with `diffuse=1,0,0`, 64 specular
draws and 50,064 volume vertices. The light blinks: ticks 6 and 24 are in its
off phase and the frame is unchanged, while at tick 47 it changes 335 pixels
by +1.74 mean in the red channel and exactly 0 in green and blue. The
Original frames of both builds are byte-identical on Elongation
(`5c393b8ffcb2f70c...`) and Coagulation (`473567429a30ef25...`).

## Projected (frustum) lights in multiplayer

`pstencil::ProjectedLights`, the campaign bridge's projected renderer, is now
also constructed by the multiplayer runtime before its Renderer. In pStencil
each view's `current_lights` hook reads the completed native light pool
(`NativeWorldEffects::source_light_view`) after that view's actual C35D0
update. The existing sphere view and receivers are returned unchanged. Every
admitted visible non-sphere light then gets its whole native receiver and
geometry (`native_light_receiver_tu15`, `native_light_geometry_tu15`) and is
drawn with domain 2, exactly as `CampaignRenderer::draw` does. A native owner
that declines a light leaves it unlit and is reported once; no shape is
substituted. Original never installs the hook.

Evidence (prepared MCC cache, scripted strafe on Turf to 1.05, -11.04, 2.01):
the native pool lists the hinge lamp `hinge_light_long` at (-1.04, -12.00,
4.21); while it is in view it is the selected candidate and is drawn each
frame with 8 native commands and 10 draws, 95 caster ranges and 140,448
volume vertices, with no declined owner. All 20 other multiplayer maps start
in pStencil with the renderer resident, and Original on Coagulation stays
byte-identical (`473567429a30ef25...`). The lamp's visible effect on world
surfaces was not isolated: the floor under it is already lit by the baked
lightmap in both modes.

## Flashlight and vehicle headlights

Both are frustum lights attached to their owner (`flashlight_1p`,
`flashlight_3p` with function `integrated_light_power`; `warthog_headlights`
with function `headlights`). Their native records exist from the owner's
creation but stay unplaced until the owner's integrated light is on.

Three things were found on 2026-11-11:

- **The press never reached the unit.** Original C7580 toggles the light on
  control bit 2 of unit+148, which C92C0 copies from the player command. The
  command built by `CampaignSourceRuntime::deliver_controls` never carried
  that bit, so the flashlight could not be switched on in any mode. The bit
  is now set from `Controls::flashlight` (the pad and keyboard already
  produced a one-tick press).
- **The first press crashed the live world.** C7580 calls C7D70, the unit's
  global impulse-sound helper (AX=5 on, 4 off), which no generated module
  owned ("live world owner missing at c761d -> c7d70"). The live world now
  owns C7D70 natively: it rebuilds the original frame and lets 18C000 keep
  its existing owner.
- **Multiplayer has neither light, by the original's own rule.** C7580 runs
  its toggle only when the game mode word at `[4F3218]+8` is 1 (campaign).
  In a multiplayer capture the bit arrived at unit+148 and the light stayed
  off, as the original code prescribes, so the multiplayer command is left
  unchanged. A driver's control bits are OR-ed into the vehicle's unit+148
  by C6E90 and the same C7580 then runs for the vehicle, which is how the
  headlights follow the flashlight press in campaign; in multiplayer the
  Warthog's `headlights` records stay unplaced while it is driven.

Evidence (Metropolis, pStencil, press on frame 1 with
`HALO2_REFERENCE_CAMPAIGN_FLASHLIGHT_FRAME=1`): the placed non-sphere
records rise from 15 to 17 (both flashlight records), and one projected
light is selected and drawn with 15 to 18 draws on 74 consecutive frames. A
control run without the press keeps 15 placed records and draws none.
Original runs the same press without an error. The lit frames are in
`analysis/pstencil/world-shadows-20261111/campaign-flashlight-off-on-original.png`,
but they are not a clean A/B: campaign timing follows the wall clock, the
three frames are 2 to 7 ticks apart, and every mission starts in a
cinematic whose camera does not look where the player's light points. The
headlights were not exercised: no mission reaches drivable gameplay in a
headless capture, and skipping the opening cinematic fails in the live world
("owner missing at 1b0842 -> 1caee0"), which is outside pStencil.

## Effect-spawned animated lights

Explosion and muzzle lights (`shell_fire`, `fusion_coil_destroyed`,
`turret_destroyed`, `module_destroyed`, `sparks`) are created by the port's
standalone effect-light owner, not by the native world: a scripted grenade
detonation on Lockout adds no record to the native pool. That owner had no
animation evaluator, so it declined them.

`source_effect_light_animate` (`src/rendering/effects/effect_light.cpp`) is
a native replication of the original C2B30, which C2CB0 calls each tick once
the light's control, colours and brightness exist. The game tick, offset by
`(identity*4999 & 0x7fffffff) % (tick_rate*60)` unless the definition's +C0
bit 0 synchronizes it, is evaluated through the brightness function into +EC
and through the colour function into the diffuse colour at +D4. Specular,
size and control are not touched. It uses the existing
`shader_function_fraction`, `shader_function_value` and
`shader_function_color` evaluators. No shipped sphere has a gel animation
block, so that branch is not replicated and such a definition stays
declined.

`plasma_grenade_charge` is the one animated sphere with neither a diffuse
nor a specular radius (the inventory lists five more such spheres that are
not animated, for example `engine_lights` and `standard_detonation`). The
sphere radius is `max(radius, specular radius) * size`, so it illuminates
nothing in any mode and exists for its lens flare. It is no longer declined:
it joins the ordinary pool like the other lens-only lights and is ticked
there with the same C2B30 animation. The plasma grenade's attached effect
graph therefore preloads completely (the published runtime reported it as an
unloadable attached graph twice per start), and there is nothing for
pStencil to light or shadow. A plasma grenade in flight on Lockout renders
the same Original frame as the published runtime (tick 118, 44,385 distinct
colours, 0 changed pixels). The lens flare itself still belongs to the
separate lens owner, which reports its own boundary.

These lights live in a separate pool of `NativeWorldEffects`
(`animated_light_definitions`, `animated_light_view`,
`animated_light_receivers`, `animated_lights`). The ordinary pool keeps its
identities, occupancy, selection and resident catalog, and `load_effect_light`
still declines every animated definition, so Original is unchanged. In
pStencil the multiplayer runtime draws the animated pool as light domain 3.

Evidence:

- `tools/pstencil/oracle_light_animation_tu15.py` runs the unchanged TU1.5
  C2B30 with its callees 13DE70 and 13E6E0 under Unicorn over a fixture of
  the native kernel's outputs: 16,632 rows over 14 real animated definitions
  (9 brightness, 5 colour functions) with 0 mismatching bits and 392 original
  instructions visited
  ([report](../../analysis/pstencil/light-animation-20261111/oracle.json)).
  Flipping one output bit in the fixture is reported as a mismatch.
- `pstencil_animated_effect_lights` starts Coagulation's real fusion-coil
  destruction graph at declared source locations: its light is created only
  in the animated pool, 46 ticks equal the C2CB0 state followed by C2B30 for
  the light's own datum and the game tick, and it retires with its duration.
- An animated light no longer makes its effect graph incomplete, so the
  effects owner's attached consumers stop skipping these graphs. On the
  prepared cache this admits the Scorpion `cannon_fire`,
  `fusion_coil_destroyed`, Lockout's `power_module_destroyed` and Turf's
  `trans_turret_destroyed`; the published runtime reported the Scorpion
  firing effect as an unloadable attached graph at every start. The same
  test runs each admitted graph as an object attachment for ninety ticks
  with posed markers: none declines, and the cannon, coil and module graphs
  light their animated light. This is not specific to pStencil: the graphs'
  particles and other parts now also play in Original, as they do on the
  Xbox, while the animated light itself is still drawn only by pStencil.
  The Original Coagulation frame is unchanged (`473567429a30ef25...`). An
  Elongation hash formerly cited here (`cc43b9f07a741134...`) was a frame
  inside the start-of-match fade, entirely black, and is withdrawn.
- The lights have not been seen in a live frame: the capture script cannot
  fire a Scorpion or destroy a coil or power module.

## Armor emblem overlay

`texture_mask_materials` pairs every material with the opaque texture-target
writer of its shader. The armor emblem (`mc_emblem`, `elite_emblem`,
template `emblem_overlay_simple`) is not an opaque base: it is an
alpha-blended overlay coplanar with the armor under it. Paired like the
rest, the campaign's blank emblem was written as opaque black over both
shoulder plates and fought the armor for depth, which left exact-zero
pixels with a speckle of surviving armor pixels. This was present in the
published runtime. Emblem overlays are now left unpaired and keep their
shipped overlay pass (`HALO2_REFERENCE_PSTENCIL_EMBLEM_MASK=1` restores the
old pairing for a paired capture). Before and after on Metropolis:
`analysis/pstencil/world-shadows-20261111/campaign-emblem-overlay-before-after.png`.
A two-player multiplayer frame is byte-identical with and without the
change, and `pstencil_armor_adaptation` checks the rule.

## World texture-target depth

In pStencil the opaque texture target of a shader is written by its
texture-mask companion while the retained world depth already holds the
same surface. World surfaces rasterize that depth through the source
W-depth pipeline variant; the companion used the ordinary raster. Where the
two disagreed by one depth step the companion's pixel was rejected and the
final surface multiplied by an empty texture target, leaving scattered
black pixels. It showed on Coagulation's base ramp (instanced geometry,
`flat_dark_scratchy`), and was first mistaken for the Warthog windshield in
front of it. A companion now borrows its admitted root's source-depth
variant for that view (`HALO2_REFERENCE_PSTENCIL_ORDINARY_MASK_DEPTH=1`
restores the ordinary raster for a paired capture). On the retained
Coagulation view this changes 243 pixels, all brighter, and the 132
near-black pixels on the ramp fall to 3, the same count as with the 2003
texture adaptation switched off. Before and after:
`analysis/pstencil/world-shadows-20261111/world-texture-target-depth-before-after.png`.
Lockout's dark blotches and light specks are authored texture detail: they
are identical in Original and unchanged by this. All 23 multiplayer maps
that reach tick 100 do so in pStencil with the change, and Original on
Coagulation and Lockout stays byte-identical to the published runtime. The
model path's companions were not changed; no speckle was seen on a model.

## Still declined

- **Scenario-placed lights in multiplayer.** The native-pool receiver path
  skips any light with a scenario placement
  (`NativeWorldEffects::source_light_receivers`). Every native record seen in
  the Turf, Street Sweeper, Zanzibar, Elongation and Lockout captures had no
  placement, so whether any shipped multiplayer map places lights this way
  is still not established.
- **Owner exclusion.** The authored cast-policy bits and the 2003 owner
  exclusion are not read by the point caster, so a fixture can occlude its
  own light.
- **Light budget.** The visible-light queue (64), candidate list (128), light
  pool (350) and the 850 draw units per context can drop lights or casters
  without a report.

Reproduce the inventory without changing any asset:

```sh
<build>/pstencil_audit_light_coverage OUTPUT.json \
  assets/imported/<prepared-cache>/halo2/h2_maps_win64_dx11/*.map
```
