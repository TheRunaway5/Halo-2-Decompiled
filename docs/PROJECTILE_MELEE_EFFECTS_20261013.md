# Projectile and melee effects continuation, 2026-10-13

This pass advances original Xbox TU1.5 parity. It does not establish full
projectile, grenade, melee or sword visual parity. Source changes, builds,
fixtures and captures are inside `/home/eric/Developer/Halo-2-Decompiled`.
Original executable inputs outside the checkout were opened read-only.

## Projectile correction

The actual native contact caller supplied projectile flight speed to the
original distance-falloff consumer. Complete original projectile construction
copies `proj+16C` into the dry projectile's `+170` field; this is its authored
damage range. Wet construction uses `proj+178`. Flight speed is a separate
field. The importer now retains the dry range and the contact caller supplies
it independently of velocity.

The actual Shotgun firing regression failed before this correction: scale A
was `0.994435` instead of `0.916531`, using velocity 120 instead of authored
range 8. The retained before/final evidence is under
`analysis/weapon-effects-20261013/continuation`.

`tools/oracle_projectile_damage_range.py` executes the complete constructor
unchanged in retail, TU1.1 and TU1.5: 58 cases per image, with identical results.
The paired contact-input oracle now verifies 924 exact cases, including
pre-selection noise, RNG, distance falloff, effect A and initialized defaults.
The shared checkout also contains the separately developed prop material and
scale-B consumer; this continuation preserves that work. See
[TU1.5 impact presentation](TU15_IMPACT_PRESENTATION_20261013.md).

## Melee contact delivery

The simulation previously applied melee damage without retaining a contact
for presentation. It now journals known terrain/prop contacts on the authored
impact counter, including weapon, selected damage definition, material,
position, normal and attacker. Projectile and melee journals share a contact
serial so batched rendering preserves their creation order. LAN snapshots carry
the new journal; native datagram version 58 includes this incompatible addition.

Presentation follows original retail `CEEE0`, TU1.1 `D0340`, TU1.5 `D13D0`:

- Melee-only weapons, including the sword, use the held weapon's `foot` bank,
  action 15, variant 0, with fallback material NONE. Requests use the contact
  point offset by normal times `0.01`, scale A/B one, and the original ordering
  of primary, sweetener and secondary entries.
- Ordinary weapon melee instead requests the global material's `+64` sound
  followed by the selected damage definition's `+AC` sound. It does not reuse
  the SMG projectile-impact bank.
- A known material contact can request presentation without an object target,
  and when melee modifiers suppress direct damage. Unknown unit-capsule
  materials remain unknown.

`tools/oracle_melee_impact_routing.py` executes the complete original routing
and weapon/root predicates unchanged in both update images. It records only
the lower material-effect/sound service requests. All 364 actual Sword/SMG
and global-material combinations agree across the two images.

Lockout's sword action 15 contains twelve sound rows and zero visual rows.
No particles are expected from this specific material-contact path. This does
not prove that blade, shield, blood or character-hit effects are complete.
The regression explicitly checks this original bank instead of requiring
invented sword sparks.

## Validation and boundaries

The reproducible source snapshot and build are `.local/effects-parity-source-20261013`
and `build-effects-parity-20261013`, both inside the main checkout. This isolates
validation from concurrent edits in the shared checkout. The full native runtime
and focused targets compile successfully. The main `build/halo2_native`, both
launcher targets and these focused test targets also compile; the same focused
checks and TU1.5 native grenade/animation checks pass against the main build.

- Three focused CTests pass: original contact inputs, actual Shotgun contact
  inputs and melee/sword material routing.
- The melee test checks 364 material cases, 516 ordered sound requests, four
  actual regular/special sword terrain contacts across Spartan and Elite, their
  impact counters and LAN serialization.
- Native melee gameplay checks damage on its authored impact frame and
  non-frozen, finite third-person sword poses on both species in ground/air and
  standing/crouched states. These are bounded native animation checks, not
  matched original pose/frame captures.
- The weapon-pose regression checks 15 weapons, 168 stance/movement/dual
  combinations, 5,328 endpoint/weighted poses and 2,418 action poses.
- TU1.5 actual SMG/frag regression: 60 contacts, one detonation, 86 SDL packets,
  with LAN, resolution/view independence and retirement checks. Actual Midship
  plasma regression: detonation tick 52, eight systems, peak 83 particles, with
  per-view/resolution and retirement checks.

The earlier finite source-world captures under `controlled-runtime/`,
`controlled-grenade-first/` and `controlled-main/` failed at grenade channel
ownership or the stale executable's `EA5A0 ->106510` boundary. They remain
historical failure evidence. The subsequent source-owned grenade, melee,
sword, shield-effect and particle-color continuation supersedes these blockers;
see [the live TU1.5 combat report](TU15_LIVE_COMBAT_EFFECTS_20261013.md) for
successful source and SDL/Vulkan acceptance evidence and the remaining limits.

The native fallback journal still uses its native mover and collision inputs.
Live TU1.5 mode now publishes the real `D1FBF ->D13D0` posed-contact request.
The source path owns actual unit damage and shield-effect requests. Original
sword lunge/magnetism, blood/blade/lens owners, attacker feedback and matched
Xbox rendering/PCM remain unverified or incomplete.

Projectile continuation/bounce/attach/penetration, contrail lifecycle, remaining
effect child/attachment owners and dynamic object light/lens submission also
remain open. The original grenade frame investigation still has unresolved
strict pixel comparisons. Passing scalar or native renderer checks does not
close these gaps. See the [earlier effects audit](WEAPON_EFFECTS_20261013.md)
and [grenade reference investigation](TU15_GRENADE_LIGHT_FRAMES_20261013.md).

## Reproduction

Generated oracle bytes stay local under the fixture ignore policy. See
[fixture instructions](../tests/fixtures/melee_impact_routing.md) for the paired
melee oracle. Run the native checks with a user-owned TU1.5 executable:

```sh
ctest --test-dir build-effects-parity-20261013 \
  -R '^(original_tu15_projectile_impact_inputs|native_projectile_effect_input_owners|native_melee_and_sword_contact_effects)$' \
  --output-on-failure
build-effects-parity-20261013/world_effects_tests \
  assets/disc/maps/lockout.map assets/disc/maps '/path/to/owned/u5English/default.xbe'
build-effects-parity-20261013/world_plasma_effects_tests \
  assets/disc/maps/midship.map assets/disc/maps '/path/to/owned/u5English/default.xbe'
```

Regenerate the constructor evidence separately; it does not call the native
runtime or supply collision results:

```sh
.tools/python/bin/python tools/oracle_projectile_damage_range.py \
  --xbe assets/disc/default.xbe \
  --xbe '/path/to/owned/TU1.1/default.xbe.03' \
  --xbe '/path/to/owned/u5English/default.xbe' \
  --report analysis/weapon-effects-20261013/continuation/damage-range-original.json
```
