# Weapon zoom HUD and crosshair wiring

The rebuilt runtime connects authored weapon optics and binocular selectors to
the unit, per-view camera, HUD conditions and imported artwork. The ordinary
gameplay presentation previously restricted zoom to the SMG. The TU1.5 command
adapter also omitted command +0x24 and did not publish the resulting unit +0x240/+0x241
zoom state. Those connections now apply to the actual selected weapon.

`NativeWeaponHud` prepares campaign weapon widgets and the unit's binocular mask
before Vulkan uploads. Weapon changes select the prepared HUD, including each
dual-wield hand. Campaign input retains keyboard/controller zoom edges and uses
the existing original camera transition consumer. The first player-information
record supplies campaign binocular metadata as it does for multiplayer.

The native TU1.5 snapshot now publishes the original target flags for friendly,
headshot, weak-spot and charged plasma tracking reticles. The original relation
owner and actual unit target/model-region records supply those flags. Rocket
lock admission reads the retained target identity and original target-age gate;
locking/locked positions come from the complete original B9C20 marker query.
The HUD consumes those per-hand results and projects anchor 5 through the same
original camera math as the reticle. Native protocol 81 transports the flags and
per-hand target positions and the independent auto-aim confidence input; peers must use the matching protocol version.

The live zoom regression exposed an absent TU1.5 sound-request owner at `18C000`.
`generate_object_sound_tu15.py` now lowers complete original `18C000`/`18C1F0` bodies,
including their real callees and required platform boundaries. No zoom-sound
return or successful voice allocation is supplied by this fix.

## Verification

All results below were rerun on 2026-10-14. Evidence is retained under
`analysis/multiplayer/weapon-zoom-hud-20261014/`.

| Check | Result |
| --- | --- |
| All authored crosshair/scope widgets | 31 weapons, both multiplayer species, 212 rows, 57,204 CPU visibility/layout submissions |
| Campaign weapon HUD selection | 15 prepared first-person weapon selections; ordinary and authored optic widgets |
| Original optic magnification | 20 unchanged TU1.1/TU1.5 leaf executions agree, including both 3.5x/9.5x sniper levels |
| Original target projection | 144 unchanged retail 1396C7 mode 5 / 30710 / 2B460 outputs match exactly |
| Actual TU1.5 multiplayer worlds | BR, SMG, rocket, sniper, carbine, beam rifle; 8 zoom levels, raw unit bytes and independent HUD snapshots |
| Actual SDL/GPU zoom | 466 rendered frames / 119 ticks on RTX 5080; RMB cycles, FOV transitions, firing reset and reload admission/release pass |
| Actual native UDP gameplay | Independent SMG zoom, firing reset, stale-command protection and reload lifecycle pass |
| Native packet codec | 128 round trips preserve both players' aim flags, independent confidence input and per-hand lock state/positions |
| Retained MCC Classic artwork | Same 212 rows/57,204 draws and 15 campaign selections pass; 31 weapons / 84 reticle rows / 75 selected pages, 33,312 bindings and 20,160 unchanged vertices |

The six authored optics are battle rifle 2x, carbine 2x, rocket launcher 1.8x,
fuel-rod cannon 2x, sniper rifle 3.5x/9.5x and beam rifle 3.5x/9.5x. The selected
multiplayer runtime weapon bank has no fuel-rod starting-weapon reference; its
authored HUD/one-level arithmetic are covered by the cache and campaign-widget
tests rather than an invented multiplayer starting slot.

The original arithmetic authority is TU1.1 SHA256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
and TU1.5 SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Target screen projection additionally uses unchanged retail instructions from
SHA256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The original one/two-level power endpoints are implemented; unsupported authored
counts above two still retain an explicit unavailable general-power boundary.

## Scope and delivery

The tested executable is `build-weapon-zoom-hud-20261014/halo2_native`. The normal
launchers currently select the separate MCC-native package, whose own launcher
status still reports incomplete MCC gameplay loading/conversion. This task does
not replace that concurrently maintained package or assert that its launcher
already includes this rebuilt runtime.

The Vulkan validation layer is unavailable on this host. The actual SDL/GPU
run therefore completed without that layer. These checks do not establish a
matched Xbox framebuffer, complete vehicle weapon/HUD ownership, campaign
target-acquisition behavior, or audible zoom-sound/PCM parity. The source-world
tests exercise normal optics/binocular state, not a live vehicle rocket-lock
encounter; lock/aim variants additionally have packet, projection and real-widget
consumer coverage with explicit input states.

## Authored weapon matrix

This table records the cache/widget coverage above. It does not promote vehicle
weapon selection or an untested target-acquisition encounter into live gameplay
evidence. Each definition was exercised for both multiplayer species.

| Source weapon definition | Optic levels | Reticle/scope bitmap rows | CPU widget coverage |
| --- | ---: | ---: | --- |
| `objects\vehicles\warthog\turrets\chaingun\weapon\chaingun_turret` | 0 | 2 | Passed |
| `objects\vehicles\warthog\turrets\gauss\weapon\gauss_turret` | 0 | 2 | Passed |
| `objects\vehicles\banshee\weapon\banshee_gun` | 0 | 2 | Passed |
| `objects\vehicles\ghost\weapon\ghost_gun` | 0 | 2 | Passed |
| `objects\vehicles\h_turret_ap\weapon\h_turret_ap` | 0 | 2 | Passed |
| `objects\vehicles\scorpion\turrets\cannon\weapon\cannon_turret` | 0 | 2 | Passed |
| `objects\vehicles\scorpion\turrets\cannon\weapon\cannon_turret_mp` | 0 | 2 | Passed |
| `objects\vehicles\c_turret_ap\weapon\big_needler` | 0 | 2 | Passed |
| `objects\vehicles\wraith\turrets\mortar\mortar_turret` | 0 | 2 | Passed |
| `objects\vehicles\wraith\turrets\mortar\mortar_turret_mp` | 0 | 2 | Passed |
| `objects\weapons\fixed\plasma_cannon\plasma_cannon` | 0 | 3 | Passed |
| `objects\weapons\pistol\magnum\magnum` | 0 | 4 | Passed |
| `objects\weapons\pistol\needler\needler` | 0 | 3 | Passed |
| `objects\weapons\pistol\plasma_pistol\plasma_pistol` | 0 | 4 | Passed |
| `objects\weapons\rifle\battle_rifle\battle_rifle` | 1 | 11 | Passed |
| `objects\weapons\rifle\covenant_carbine\covenant_carbine` | 1 | 9 | Passed |
| `objects\weapons\rifle\plasma_rifle\plasma_rifle` | 0 | 3 | Passed |
| `objects\weapons\rifle\shotgun\shotgun` | 0 | 3 | Passed |
| `objects\weapons\rifle\smg\smg` | 0 | 3 | Passed |
| `objects\weapons\rifle\sniper_rifle\sniper_rifle` | 2 | 10 | Passed |
| `objects\weapons\support_high\flak_cannon\flak_cannon` | 1 | 3 | Passed |
| `objects\weapons\support_high\rocket_launcher\rocket_launcher` | 1 | 7 | Passed |
| `objects\weapons\support_low\brute_shot\brute_shot` | 0 | 2 | Passed |
| `objects\weapons\multiplayer\assault_bomb\assault_bomb` | 0 | 2 | Passed |
| `objects\weapons\multiplayer\ball\ball` | 0 | 2 | Passed |
| `objects\weapons\multiplayer\flag\flag` | 0 | 2 | Passed |
| `objects\weapons\melee\energy_blade\energy_blade` | 0 | 2 | Passed |
| `objects\weapons\melee\energy_blade\energy_blade_useless` | 0 | 2 | Passed |
| `objects\weapons\rifle\beam_rifle\beam_rifle` | 2 | 5 | Passed |
| `objects\weapons\rifle\brute_plasma_rifle\brute_plasma_rifle` | 0 | 3 | Passed |
| `objects\characters\sentinel_aggressor\weapons\beam\sentinel_aggressor_beam` | 0 | 3 | Passed |
