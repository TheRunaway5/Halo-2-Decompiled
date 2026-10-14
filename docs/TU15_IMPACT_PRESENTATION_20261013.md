# TU1.5 impact presentation

Visual authority is the unchanged OG Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
This work advances the full shader/effect parity goal; it does not complete it.
Vista assets are not the gameplay reference.

## Production changes

- Terrain impacts retain the original pre-selection angular/velocity noise,
  primary RNG writes, selected response, approach-speed admission and effect A.
  The previous native path admitted only zero-noise rows without distance falloff.
- Distance falloff uses actual contact and projectile positions, accumulated
  flight distance, the original dry damage-range owner and constructor
  scale one. It preserves the original x87 PC53 intermediates and subsequent
  binary32 multiply. This supplies effect A; the complete direct-damage amount
  and projectile continuation owners remain separate work.

- Parentless prop contacts apply their current native damage before retaining
  the original material/scale B consumer. Lockout power modules now use authored
  damage material 86 instead of collision material 0, and retain their post-hit
  vitality instead of a fixed effect scale of one.
- Props admit this material consumer only when every authored material entry
  agrees with the default material and shield-absorption policy. Distinct
  material/region owners remain explicitly unresolved. The all-map audit admits
  20 of 393 native props; it does not claim all prop impacts are complete.
- Contacts with no authored response row use the original initialized default
  scalar response. The world effect owner accepts this retained response rather
  than dropping the contact before its material/custom effects can run.

The [projectile/melee continuation](PROJECTILE_MELEE_EFFECTS_20261013.md)
corrects the live caller's former speed/range confusion and records the complete
original constructor evidence and actual Shotgun firing regression.

The default is an initialized runtime object, not 88 zero bytes copied from
the XBE's uninitialized storage. Complete original constructors at base
`376DD0`, TU1.1 `37D3C0` and TU1.5 `380E70` produce the same row. The TU1.5 row
also matches authenticated live RAM from the grenade capture. The native
scalar consumer implements only the consumed response/noise/scale fields;
it does not invent the row's tag-reference payloads.

## Independent executable checks

`tools/oracle_projectile_impact_inputs.py` executes unchanged TU1.1/TU1.5
instructions and their original CRT angle calls. TU1.5 intervals are
`FFEB9..1000F9` for impact noise/selection, `FFBAE..FFC29` for falloff and
`10070C..100756` for effect A. The 924 cases compare angle, approach speed,
response, final RNG, damage scale and effect scale bit for bit. Four cases use
the actual initialized default row. Authored rows and explicit branch controls
are identified separately in the fixture's inputs/provenance.

`tools/oracle_projectile_damage_presentation.py` executes unchanged TU1.5
`DA3CE..DA40D` and `DA6DD..DA769`/`DA777`. Its 355 explicit cases compare the
damage material and scale B bit for bit, including zero damage, threshold
boundaries, shield/body ties and body clamping. Collision, amount admission,
hit-region selection and vitality updates are inputs to this consumer check.

Registered checks:

```sh
cmake --build build --target projectile_impact_inputs_tests projectile_damage_presentation_tests
ctest --test-dir build -R '^original_tu15_projectile_(impact_inputs|damage_presentation)$' --output-on-failure
```

Fixture payloads/provenance remain local under the existing oracle-payload
ignore policy. Regenerate with the user-owned executables:

```sh
.tools/python/bin/python tools/oracle_projectile_impact_inputs.py \
  --tu11 assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03 \
  --tu15 '/path/to/owned/u5English/default.xbe' \
  --fixture tests/fixtures/projectile_impact_inputs_original.bin \
  --report tests/fixtures/projectile_impact_inputs_original.provenance.json
.tools/python/bin/python tools/oracle_projectile_damage_presentation.py \
  --xbe '/path/to/owned/u5English/default.xbe' \
  --fixture tests/fixtures/projectile_damage_presentation_original.bin
```

## Runtime evidence

Evidence directory: `analysis/multiplayer-shading/world-effects-goal-20261011/contact-inputs-native-v1`.
The focused checks include actual Match fire against a posed power-module
surface, subsequent damage, delayed explosion/removal, original material
override and retained scale B. Separate direct runs use the authenticated TU1.5
executable. The all-map import/construct/step audit covers 21 maps, 393 props
and 15,312 posed triangles; this is not all-map appearance acceptance.

`impact-tu15-contact-inputs-v2` retains actual Vulkan gameplay captures at
640x480, 1280x720, 1920x1080 and four-player 1280x720 with zero validation errors.
The equal-aspect 720p/1080p captures retain identical counts and RNG states.
The 4:3 and four-view captures have separate weather/auxiliary RNG histories.
These are native smoke captures, not matched original frames.

`prop_impact_capture` supplies an explicitly declared shooter's position facing
a real posed Lockout module surface, then runs actual native controls,
collision, damage and GPU presentation. It writes the input script, retained
material/scale values and frame. Build/run it with:

```sh
cmake --build build --target prop_impact_capture
build/prop_impact_capture assets/disc/maps/lockout.map assets/disc/maps \
  '/path/to/owned/u5English/default.xbe' '/path/to/capture-directory'
```

`prop-impact-tu15-material-v4` passes with four actual contacts, material 86,
scale A one and post-hit scales B `0.961458`, `0.922917`, `0.884375`, `0.845833`.
The tick38 frame contains 18 world particles and zero Vulkan validation errors.
Its manifest records the probe binary, input, source, image and log hashes.
Earlier failed probe attempts remain retained; they are not acceptance evidence.

Launcher selection changed during this work when a separate task delivered its
verified ONLINE build. `contact-inputs-native-v1/delivery.json` records both
selected paths/hashes and the tested root/capture binary. Integration into that
selected delivery remains pending; its runtime and preferences were preserved.

## Full-goal work order and acceptance

1. Join live dynamic model light receivers to the shared original draw bank and
   layer order. The TU1.5 capture proves a power-module light draw; the native
   grenade path currently submits BSP receivers only.
2. Finish posed unit hit materials/regions and prop entries with distinct
   material selection, object/node effect attachments, remaining damage amount
   owners, projectile bounce/penetration and post-response noise.
3. Complete remaining effect admission/children, audio owners and resolution/
   split-screen lifecycle coverage. Retain source RNG and scheduling evidence.
4. Close strict matched TU1.5 framebuffer differences and capture matched live
   impacts/explosions. Keep all multiplayer maps, campaign enemy/ally shaders,
   first-person placement and authored textures in the full visual scope.
5. Verify a combined runtime containing these changes and the latest independently
   delivered features, then update and recheck both launcher paths.

The [grenade frame investigation](TU15_GRENADE_LIGHT_FRAMES_20261013.md)
records the still-failing strict pixel comparisons. The
[campaign character work](CAMPAIGN_CHARACTER_SHADERS_20261013.md) records its
separate admission and rendered-validation boundaries. Neither clean Vulkan
validation nor bit-exact scalar inputs establish full rendered parity.
