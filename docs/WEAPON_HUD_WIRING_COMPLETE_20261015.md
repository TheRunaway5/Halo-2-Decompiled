# Weapon HUD wiring and verification, October 15

All authored weapon crosshair and zoom HUD definitions are connected: 20
handheld definitions, 11 vehicle/turret definitions, and two seat HUDs. The
English original Xbox TU1.5 executable and original tags remain authoritative.
Imported MCC Classic artwork uses the same layout and state consumers.

## Connections

Campaign gameplay now retains one original source world for objects, player
commands, weapon selection, targeting, physics and HUD publication. Simulation
borrows that world's object pool and reads its actual full datum identities.
Checkpoint restoration and BSP transitions preserve the same authority.
The campaign HUD receives original friendly/headshot/weak-spot/tracking flags,
independent auto-aim confidence, both hands' lock states and target positions,
and vehicle weapon/seat ownership. Command delivery calls the same original
targeting stage exercised by the focused actor-acquisition check.
Campaign `ai_allegiance` and `ai_allegiance_remove` commands now reach complete
original `27E000` and `1E7A20`, so friendly-reticle classification consumes the
actual source relationship table. Removal and checkpoint restoration are
covered by the live actor check.

Weapon HUD preparation reads the cache's authored first-person bindings,
including objective items and the depleted sword. It no longer depends on the
native projectile parser accepting a weapon. Barrel-less equipment retains
source-only metadata; original source execution owns its gameplay. Arbiter and
Elite bindings select authored FP row1, with the original row0 fallback rule.

Six authored optics supply eight zoom levels. Binoculars consume the original
unit selector and action admission. The admitted selector reaches the camera
and HUD together. Per-view camera displacement and target-anchor projection
use the original camera viewport before HUD pixel-shape adjustment, including
wide vertical splits and four-view layouts.

Vehicle weapons use the original seat, parent inventory and eligible child
selection. Seat HUDs are prepared separately. The Spectre weapon deliberately
has no weapon HUD; its authored seat HUD supplies the reticle. Handheld HUDs
follow the authored seat visibility flags.

Native transport carries independent hand, target, vehicle and camera inputs
and rejects malformed flags and non-finite camera displacement. Both peers
must use the matching native protocol version recorded in verification.json.

## Verification

The exact commands, exits, logs and tested binary hashes are saved in
`analysis/multiplayer/weapon-zoom-hud-20261015/completed-verification/verification.json`.
The completed cohort includes:

- All four player representations: 124 weapon/representation combinations,
  424 authored rows, 24 scope masks and 114,408 CPU visibility/layout draws.
- All 16 regular handhelds, depleted sword and all eight optic levels through
  original campaign equipment/command execution across Cairo, Arbiter's
  Floodlab and Delta Towers; binoculars, checkpoint and BSP restoration.
- A real original squad actor, the production player-command targeting stage,
  actual published target identity, and submission of its authored friendly
  reticle. This isolates targeting from the separate AI behavior update.
- 1,296 unchanged-original vehicle ownership comparisons, all 11 authored
  vehicle/turret HUDs and two seat HUDs across 78 full/split layout draws.
- Seven actual vehicle worlds, six actual optic/binocular worlds, loopback UDP
  scope lifecycle, 128 codec round trips and original projection fixtures.
- 252 unchanged original target projections and 140 absolute target-widget
  draws, including 80 wide and 100 split layouts (overlapping categories).
- Vulkan Spectre capture: four frames with independent seated/handheld HUDs.
  The SDL scope sequence passes 163 frames and 120 source ticks; the campaign
  capture passes 120 frames. All three captures were inspected. Original and
  imported MCC Classic artwork pass.

All 15 selected CTests pass. The passing CPU executables were verified
unchanged after the GPU run reached an additional original renderer helper;
complete `42840` (93 instructions) was recovered, and the three GPU
executables were rebuilt and rerun. Both build logs and the retained CPU
binary hashes are recorded with verification.json.

The pre-fix Arbiter FP binding, objective-item preparation and wide target
projection failures are retained in the evidence directory. They establish
that the regressions exercise the changed behavior.

## Authored weapon matrix

Every row passes the authored HUD test for Chief, Arbiter, multiplayer Spartan
and multiplayer Elite. Regular handhelds and the depleted sword additionally
have actual campaign selection coverage; objective items have prepared native
HUD and full representation/layout coverage. Vehicle coverage combines the
original ownership differential, all authored widgets, and seven live vehicle
worlds. This table describes wiring coverage, not a complete game playthrough.

| Definition | Owner | Optic levels | Reticle/scope bitmap rows |
| --- | --- | ---: | ---: |
| `objects\vehicles\warthog\turrets\chaingun\weapon\chaingun_turret` | vehicle or turret | 0 | 2 |
| `objects\vehicles\warthog\turrets\gauss\weapon\gauss_turret` | vehicle or turret | 0 | 2 |
| `objects\vehicles\banshee\weapon\banshee_gun` | vehicle or turret | 0 | 2 |
| `objects\vehicles\ghost\weapon\ghost_gun` | vehicle or turret | 0 | 2 |
| `objects\vehicles\h_turret_ap\weapon\h_turret_ap` | vehicle or turret | 0 | 2 |
| `objects\vehicles\scorpion\turrets\cannon\weapon\cannon_turret` | vehicle or turret | 0 | 2 |
| `objects\vehicles\scorpion\turrets\cannon\weapon\cannon_turret_mp` | vehicle or turret | 0 | 2 |
| `objects\vehicles\c_turret_ap\weapon\big_needler` | vehicle or turret | 0 | 2 |
| `objects\vehicles\wraith\turrets\mortar\mortar_turret` | vehicle or turret | 0 | 2 |
| `objects\vehicles\wraith\turrets\mortar\mortar_turret_mp` | vehicle or turret | 0 | 2 |
| `objects\weapons\fixed\plasma_cannon\plasma_cannon` | vehicle or turret | 0 | 3 |
| `objects\weapons\pistol\magnum\magnum` | handheld | 0 | 4 |
| `objects\weapons\pistol\needler\needler` | handheld | 0 | 3 |
| `objects\weapons\pistol\plasma_pistol\plasma_pistol` | handheld | 0 | 4 |
| `objects\weapons\rifle\battle_rifle\battle_rifle` | handheld | 1 | 11 |
| `objects\weapons\rifle\covenant_carbine\covenant_carbine` | handheld | 1 | 9 |
| `objects\weapons\rifle\plasma_rifle\plasma_rifle` | handheld | 0 | 3 |
| `objects\weapons\rifle\shotgun\shotgun` | handheld | 0 | 3 |
| `objects\weapons\rifle\smg\smg` | handheld | 0 | 3 |
| `objects\weapons\rifle\sniper_rifle\sniper_rifle` | handheld | 2 | 10 |
| `objects\weapons\support_high\flak_cannon\flak_cannon` | handheld | 1 | 3 |
| `objects\weapons\support_high\rocket_launcher\rocket_launcher` | handheld | 1 | 7 |
| `objects\weapons\support_low\brute_shot\brute_shot` | handheld | 0 | 2 |
| `objects\weapons\multiplayer\assault_bomb\assault_bomb` | handheld | 0 | 2 |
| `objects\weapons\multiplayer\ball\ball` | handheld | 0 | 2 |
| `objects\weapons\multiplayer\flag\flag` | handheld | 0 | 2 |
| `objects\weapons\melee\energy_blade\energy_blade` | handheld | 0 | 2 |
| `objects\weapons\melee\energy_blade\energy_blade_useless` | handheld | 0 | 2 |
| `objects\weapons\rifle\beam_rifle\beam_rifle` | handheld | 2 | 5 |
| `objects\weapons\rifle\brute_plasma_rifle\brute_plasma_rifle` | handheld | 0 | 3 |
| `objects\characters\sentinel_aggressor\weapons\beam\sentinel_aggressor_beam` | handheld | 0 | 3 |

Seat crosshair HUDs: `ui\hud\warthog`, `ui\hud\spectre`.

Authored `NONE` weapon HUD references remain `NONE`: `objects\vehicles\warthog\weapon\warthog_horn`, `objects\vehicles\wraith\turrets\minigun\minigun_turret`, `objects\vehicles\spectre\plasma_turret\weapon\plasma_turret`, `objects\weapons\multiplayer\ball\juggernaut_powerup`.

## Delivery and limits

Both installed launchers select the tested HUD delivery executable; their
reported paths and hashes are retained beside verification.json. The native
executable SHA-256 is `21efc54f7ccbccc89c0fe6725a7433ed28d6c5078db72b8e09b2853c68410251`.

MCC gameplay loading/conversion retains its existing admission gate. HUD
wiring does not complete that conversion. Vulkan captures ran without a
validation layer and do not establish matched Xbox framebuffer pixels. Enemy
headshot/weak-spot/rocket-lock variants have original-state, transport,
projection and authored-widget coverage; a complete live enemy lock encounter
and full campaign/AI/story playthrough are not claimed.

Original TU1.5 SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The original source generators reproduce their saved outputs; manifests and
the BSP callback table are retained in the evidence directory.
