# Weapon effects audit, 2026-10-13

This pass fixes a reproduced terrain-impact selection bug. It does **not**
establish complete bullet-trail, impact, explosion, or Xbox framebuffer parity.
All source edits, build outputs, fixtures and captures are in the main checkout
at `/home/eric/Developer/Halo-2-Decompiled`.

This is the earlier pass's evidence. The [projectile/melee continuation](PROJECTILE_MELEE_EFFECTS_20261013.md)
records later noise/falloff/default ownership checks, the authored damage-range
caller correction, and initial melee/sword contact presentation. Its status
supersedes the earlier noise/falloff and melee coverage limits below.

## Proven correction

The original projectile collision owner selects a material response, including
angle/speed gates and fractional-chance RNG. The native presenter previously
read only the row's default response at `+2`. A shallow Needler contact selected
bounce in the original response leaf, but native presentation used attach:
one system/five particles instead of one system/two particles in the reduced
fixture. The pre-fix result is retained in
`analysis/weapon-effects-20261013/impact-before.log`.

The collision journal now retains the selected terrain response, admission
gate, and effect scales. Selection consumes the gameplay random stream once;
presentation and remote peers use that retained decision. Protocol 53 carries
the added payload. Physical projectiles supply their actual incoming velocity,
including gravity, rather than deriving effect direction from the averaged
flight displacement.

The presenter uses the selected material effect slot, supplies terrain scale B
zero, and preserves visual/custom-effect/sound request order. The original
approach-speed gate suppresses both material effects and custom impact graphs
when the collision owner rejects them. Invalid response shorts no longer
incorrectly select slot 20. Existing prop contacts retain their previous scale
behavior pending the original object-damage result owner.

## Original evidence

- `projectiles.cpp` retail `FAA60`, TU1.1 `FEB40`: collision response selection,
  response slots 15 through 20, scale inputs, approach-speed gate and ordered
  visual/custom/sound requests.
- `unknown_11cc90.cpp` retail `11CE20`: vector-angle production, including
  binary32 stores and the original CRT acos result. The real static initializer
  supplies pi from the executable's immutable operand.
- The complete original angle routine and initializer were executed unchanged
  in retail, TU1.1 and TU1.5. All 520 finite unit-vector cases produce identical
  bits in the three images and the native implementation. Entry points are
  `11CE20`, `11E540` and `11F7E0`; provenance accompanies the fixtures.
- Original `1015A0` material-parent lookup and the unchanged response-selection
  segment already have 376 lookup cases and 3,066 response/RNG cases measured
  independently in both update images.
- Raw `proj`, `weap`, `cont`, `foot` and `matg` tags remain authoritative.
  Deprecated material-response effect references are not dispatched.

The rocket's explosion is its custom impact graph at `proj+144`. Its particles
have authored delays: a one-tick observation can contain no particles. The
eight-tick fixture reaches five systems and 45 particles. Airborne/ground
detonation fields are separate lifecycle paths, not interchangeable fallbacks.

## Validation

- Main `build/halo2_native` and the focused effect tests build successfully.
- The regression checks 14 held projectile weapons at two incidence angles on
  all 21 multiplayer maps: 588 contacts, 7,623 particle submissions, and 588
  suppressed-contact controls. It compares positions, size/color/UV attributes,
  shader constants, material/bitmap bindings and ordered material sounds with
  explicit authored requests. This validates routing through the native effect
  engine; it is not an independent Xbox particle-rendering oracle.
- Actual native firing on all 21 maps delivers 1,260 contacts and 1,719 SDL
  packets. Collision/death particle sound paths also execute. This bounded test
  does not exercise every material or weapon lifecycle.
- The SMG/frag regression passes with 60 actual contacts, one detonation,
  86 SDL packets, LAN payload round trips, view/resolution independence and
  retirement checks.
- A 120-tick native SDL/Vulkan capture on the RTX 5080 has 43 impacts, one frag
  detonation, 11 decals and 67 live world particles. The Khronos validation
  layer reports no validation errors. The image is
  `analysis/weapon-effects-20261013/native-smg-frag.png`; the log and runtime
  fingerprint are beside it. This is diagnostic native footage, not matched
  original footage.

Focused reproduction:

```sh
ctest --test-dir build -R '^(original_projectile_impact_angles|native_authored_weapon_impact_effects|original_projectile_material_response)$' --output-on-failure
```

## Remaining work before claiming all effects match

`tools/audit_weapon_effects.py` inventories 63 weapon paths, 39 projectile paths
and 25 authored contrails across 38 disc/DLC maps. Its read-only report is
`analysis/weapon-effects-20261013/authored.json`. These counts are asset scope,
not native implementation coverage. The SMG, Magnum and Shotgun do not author
a contrail attachment; an invented visible line would not match those assets.

Still missing or unverified:

1. Live `cont` birth, point sampling, state transitions, physics, detachment and
   old-shader submission. Original owners include `17BAB0`, `17BC40`, `17C0E0`,
   `17C540` and renderer `42CF0`. Polling only current live projectiles loses
   instantaneous or within-tick flight segments and is insufficient.
2. Projectile `effe`/light/MGS2/lens/loop-sound attachments, including exact
   model markers and object-function inputs for rocket exhaust and smoke.
3. Complete weapon-projectile bounce/attach/penetration/timer lifecycle and
   airborne/ground/super-detonation dispatch. The current native free-projectile
   adapter retires on contact; presentation selection does not repair that
   physical lifecycle. Brute Shot and Needler therefore remain incomplete.
4. Nonzero response-noise owners, original projectile damage falloff, and the
   object-damage scale-B/material result. Missing response owners are reported
   explicitly. The new authoritative terrain payload is bounded to zero-noise
   rows without projectile damage-falloff bit 4 (`0x10`); those unported scale owners
   are not represented as verified original values.
5. Posed unit collision materials and impact delivery from actual body/region
   hits. Capsule hit detection alone does not supply the original mesh material.
6. Dynamic object light receivers, lens-flare submission and other effect
   systems rejected by the existing native admission rules.
7. Matched original GPU/PCM captures for every relevant weapon/material/state.

No synthetic trails, generic explosion replacements, or guessed unit impact
materials were added to cover these gaps.
