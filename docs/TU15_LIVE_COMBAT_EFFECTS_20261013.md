# TU1.5 live combat and particle continuation

All source edits, diagnostic snapshots, builds and evidence are in
`/home/eric/Developer/Halo-2-Decompiled`. Owned executables outside the checkout
are read-only inputs. The authority is the English original Xbox TU1.5 image,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
This pass does not establish complete Xbox visual parity.

## Original live owners

The missing `EA5A0 ->106510` call was already present in the native source
composition but absent from the selected older executable. Its independent
120-case whole-original instruction/GP/defined-flags fixture passes after
rebuilding. It no longer stops the live acceptance path.

`generate_combat_action_tu15.py` admits complete authenticated original bodies,
including grenade preparation/release, projectile construction/update,
detonation, radial damage, original effect environment/color queries and
reached cleanup callbacks. It verifies the actual original startup, action
and deletion tables. No success-return replacement or runtime emulator is
used in the production runtime.

The original cached bitmap caller needed a native CPU metadata resource
boundary instead of Xbox GPU device access. The complete original `1D2D0`
caller agrees in 200 cases across twenty formats, five dimensions and both
linear/swizzled inputs. GPU multisample state is left untouched because the
complete caller never consumes it. This does not implement a GPU device.

## Grenades

Live source gameplay owns action admission, the selected graph/channel and
frame, count debit, release, flight, fuse, radial damage and retirement.
Native presentation observes the completed `101DB8 ->178E30` request. It
retains the actual effect identity, named location frames and both original
effect colors. The renderer no longer reconstructs these locations from a
single inferred forward/up pair. LAN carries the same bank and colors.

The direct source test consumes one grenade and detonates at tick 140 with
two completed locations; its real LAN round trip preserves both location
names, complete transforms and colors. A finite SDL/Vulkan capture with its
own declared native placement/input history detonates at tick 143 and submits
672 explosion particle draws across 58 frames. The different ticks belong
to different declared inputs; neither is an original Xbox timing capture.

## Melee and sword

Live commands now deliver the actual melee and melee-only primary-trigger
bits. Source action type 27 owns selection, progress, hit/recovery counters,
posed collision and completion. Native melee/firing no longer applies a
second sword attack in source-owned mode. Presentation reads the actual
source component and WORLD nodes instead of advancing another action clock.

`D1FBF ->D13D0` observes the real authored contact before running the complete
original presentation callee. Its journal retains original material, damage
definition, point, normal and mover frame. Main source composition generation
preserves this boundary even when the callee becomes internal.

The actual source tests produce 29 changing relative body poses for ordinary
SMG melee, and 23 for each sword input, with a real map contact at the authored
hit counter. Two-player source tests also apply real SMG/sword damage to the
other source unit. Close sword hits publish `E95507D0` at tick 89. The native
health updater previously generated an extra recharge effect on the next
tick; source-owned vitality now uses the original recharge request instead.

Completed original `D9016`, `DB8B9` and `D8889` requests reach the existing
unit-effect renderer through the journal. The complete original allocator
still executes, and authored NONE attempts remain NONE. The transparent
publication boundaries preserve the original pending prefix when no host
service exists. Original damage replay passes 2,231 general and 3,584 expanded
vitality cases, including instruction, write, CPU and retained-memory checks.

The finite two-view sword capture submits the actual shield effect 220 times
over 22 view-frames. This proves live request-to-draw delivery, not Xbox pixel
equivalence. Lockout sword terrain action 15 contains twelve sound rows and
zero visual rows; no invented terrain sparks are added.

Sword lunge/magnetism remains unverified. The exploratory lunge fixture's
declared placement was superseded by actual source spawning: the two players
were 0.344 units apart, and original selection chose an ordinary action. The
fixture explicitly fails its distance/special-activity check and cannot be
used as lunge evidence. Blade attachment/lens, blood, attack feedback and
complete attached-effect color/lifecycle parity remain separate work.

## Firing particles

The FP loader now accepts original perpendicular vertex classification 9,
which the renderer already executes. It continues to reject missing lifecycle
owners rather than discarding authored parts/forces to admit an entire graph.

The particle seed color now follows complete original `178920`, using actual
`22AD40`/`22AD50` getters. Both default and source explosion colors feed this
operation. The 3,366 whole-original cases cover all tint/multiply combinations,
unrelated flags, boundary colors and seeded random colors. These prove packed
color arithmetic at the declared original rounding mode; they are not rendered
pixel comparisons. FP effects still lack the full original lighting-color
owner and use the original neutral effect-constructor inputs.

The all-selection Lockout audit records 48 normal/misfire/empty references
from 26 weapon definitions, including vehicle weapons: 13 FP graphs admit,
nine references are authored sound-only, five contain no FP systems, 19 stop
at root lifecycle ownership and two at random/beam/acceleration ownership.
Its 207 systems include 151 billboard, 53 directional and two perpendicular
classifications, plus one material boundary. Counts describe loader admission,
not completed in-game weapon parity. World firing particles, beam/force/casing
owners, attached blade/lens effects, remaining collision/wind/swarm owners,
final blend/depth behavior and matched TU1.5 framebuffer/PCM comparison remain
open. In particular, arbitrary root flag 8 is not simply ignored to admit
Battle Rifle and other rejected effects.


These are reference counts from the same authored audit, including vehicle
weapons. They are not a completion claim for a playable loadout. Sword blade
attachments are outside this firing-reference bank.

| Weapon definition | FP references admitted | Sound only | Other references |
|---|---:|---:|---:|
| sentinel_aggressor_beam | 1 | 1 | 0 |
| banshee_gun | 1 | 1 | 0 |
| big_needler | 0 | 0 | 1 |
| ghost_gun | 1 | 0 | 0 |
| h_turret_ap | 0 | 0 | 1 |
| cannon_turret | 0 | 0 | 2 |
| cannon_turret_mp | 0 | 0 | 2 |
| plasma_turret | 0 | 0 | 1 |
| chaingun_turret | 0 | 0 | 1 |
| gauss_turret | 0 | 0 | 1 |
| minigun_turret | 1 | 0 | 0 |
| mortar_turret | 0 | 0 | 1 |
| mortar_turret_mp | 0 | 0 | 1 |
| magnum | 0 | 1 | 1 |
| needler | 1 | 0 | 1 |
| plasma_pistol | 1 | 0 | 2 |
| battle_rifle | 0 | 0 | 2 |
| beam_rifle | 0 | 1 | 1 |
| brute_plasma_rifle | 2 | 0 | 0 |
| covenant_carbine | 0 | 0 | 2 |
| plasma_rifle | 2 | 0 | 1 |
| shotgun | 0 | 1 | 2 |
| smg | 1 | 1 | 0 |
| sniper_rifle | 0 | 1 | 1 |
| rocket_launcher | 2 | 2 | 0 |
| brute_shot | 0 | 0 | 2 |

## Evidence and reproduction

Evidence is under `analysis/weapon-effects-20261013/live-tu15`. The isolated
source/build are `.local/effects-parity-source-20261013` and
`build-effects-parity-20261013`, both inside the main checkout. The snapshot's
diagnostic wire version 65534 is deliberately private; it must not connect to
the main LAN protocol. The main protocol 65 adds explosion colors.

```sh
ctest --test-dir build-effects-parity-20261013 \
  -R '^(original_tu15_grenade_weapon_gate|original_tu15_combat_action|original_tu15_cache_bitmap_view|original_tu15_particle_effect_color|live_smg_muzzle_lifetimes|native_melee_and_sword_contact_effects)$' \
  --output-on-failure
build-effects-parity-20261013/capture_source_melee_contact \
  assets/disc/maps/lockout.map assets/disc/maps '/path/to/owned/TU1.5/default.xbe' \
  analysis/weapon-effects-20261013/live-tu15/source-check sword-target
```

Use `grenade`, `melee`, `sword`, `primary`, `target` or `primary-target` for
the other source acceptance modes. `lunge` is a deliberately failing research
mode until genuine distant targeting/placement is supplied by its original
owners. See the individual fixture documents for oracle regeneration.

The final delivery is built from the coherent full main source snapshot
`.local/tu15-combat-main-20261013`, using `build-tu15-combat-main-20261013`.
Its six focused tests and 5,815 original damage cases pass, as do real source
SMG/sword target hits, frag detonation/LAN retention and both finite Vulkan
captures. Original support functions added later in the shared checkout are
also present in the complete combat module; both compositions retain the
same authenticated original bodies. The source manifest records the exact
validated composition. The older effects-only snapshot is retained for
reproduction and is not installed over the newer main features.

Staged runtime, shaders, tag defaults, SDK and launchers are under
`.local/tu15-combat-delivery-20261013`. Launcher publication retains the prior
binaries. `debug/stage_delivery.py` verifies the native ELF dependencies,
source/runtime hashes and both launcher runtime selections. Full 1-to-1
firing particle rendering, live plasma-grenade coverage, sword lunge,
attached unit color/lifecycle parity and matched original pixel/PCM proof
remain unfinished.

Both installed root launchers now select the staged combat runtime. Its native
ELF hash is `f6ddf6a756a36bb95ec9c19d077a8ae93c6a4e4be0c49a04d3cab536fa755cba`,
identical to the validated build. The active-frame captures visibly contain
both the frag burst and sword shield-hit particles; the grenade active-frame
capture uses an explicitly declared diagnostic camera. Neither is an Xbox
reference image. In-flight source edits made after this validated composition
remain in the main checkout and are listed in `combat-build.json`; they were
not overwritten or silently substituted into the tested binary.

The installed native ELF also completes its normal CLI multiplayer path for
90 frames/68 real gameplay ticks with Vulkan validation enabled, without
an unbound-owner or Vulkan validation error. This is a startup/idle gameplay
smoke check; the separate finite input captures exercise attacks and effects.
