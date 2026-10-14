# Complete original Xbox TU1.5 sandbox parity

Status: **active, incomplete**. Requested on 2026-10-13. This goal covers every
sandbox gap identified in the accompanying chat; completing a component does
not reduce its scope. Existing weapon, physics, damage, world, game-mode and
shader goals supply evidence and implementation, not blanket completion.

## Authority and acceptance

The reference is original Xbox Halo 2 English TU1.5, executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
with owned original Xbox cache assets. Original retail evidence can establish
unchanged behavior only with verified correspondence to TU1.5. Vista/MCC
gameplay, invented tuning, simplified host behavior and guessed service returns
cannot establish parity. Unknown owners remain explicit.

Each element requires all applicable gates:

1. **Reference:** authenticated original owners, complete relevant branches,
   authored operands and original update/ownership order are identified.
2. **Logic:** native state, requests, random consumption and retained memory
   agree with independently executed unchanged original instructions. Declared
   external boundaries and unexecuted branches remain visible.
3. **Composition:** real inputs reach the original owners in the live match;
   actual contacts, objects, targets, allocations and lifecycle results feed
   downstream consumers without parallel substitute gameplay.
4. **Publication:** local, split-screen and LAN preserve identity, events,
   independent views/hands and lifecycle through loss, duplication, reordering,
   join, death, reset and recreation where applicable.
5. **Presentation:** matched original TU1.5 poses, frames and PCM substantiate
   applicable models, effects, lighting, HUD and sound claims. GPU validation
   and finite native captures alone do not pass this gate.
6. **Delivery:** a coherent build and its actual installed launcher selections
   contain the tested work, with appropriate regressions passing.

No row below is certified complete. Existing successes retain their narrower
scope in the linked reports. The weapon census additionally requires separate
coverage for every definition and its applicable contracts, including AI,
vehicle/turret, objective and unused authored variants.

## Coverage matrix

| Area | Required elements | Current starting point / outstanding work |
| --- | --- | --- |
| Weapon controls and ownership | All triggers/barrels; primary/offhand; charge/hold/release; reload/chamber/interrupt; inventory, pickup/switch/drop; world clocks; aim/error/recoil and random order | [Weapon goal](WEAPONS_TU15_PARITY_GOAL.md). Existing finite numeric adapters and source fragments do not complete original unit/action composition. |
| Weapon special behavior | Plasma charge; rocket tracking; Needler acquisition/guidance/stick/fuse/supercombine; sword target selection/lunge/magnetism; AI and mounted weapon owners | [Weapon goal](WEAPONS_TU15_PARITY_GOAL.md), [live combat](TU15_LIVE_COMBAT_EFFECTS_20261013.md). Distant sword targeting fixture is currently red. |
| Powerups and abilities | Authored equipment pickup/respawn; overshield activation/decay/damage; camouflage pickup/expiry/disruption; intrinsic suit control; trait and shadow/render consumers | [Camouflage integration](MP_CAMOUFLAGE_NATIVE_BINDING.md), [material integration](MP_CAMOUFLAGE_MATERIAL_NATIVE_BINDING.md). Explicit timer grants are not pickup acceptance. |
| Vehicles | Every authored controller/motion class; wheels/suspension/hover/flight; all seats, boarding, entry/exit; mounted weapons; collisions/impulses/destruction and riders | [Physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md). Warthog/Ghost selected checks pass; Banshee exit and full flight coverage remain open. |
| Projectiles and grenades | Actual weapon creation; trajectories, guidance, contacts/materials; bounce/penetration/attachment; fuse/arming; direct/radial/super/boarding damage; detonation/retirement | [Grenade integration](MP_GRENADE_LIVE_TU15_20261013.md), [projectile/melee effects](PROJECTILE_MELEE_EFFECTS_20261013.md). Live frag acceptance does not complete plasma sticking or every projectile family. |
| Damage and vitality | Original posed node/region admission; shield/body/section transitions; all producer families; attribution/reaction; fatal cleanup, corpses and respawn | [Damage coverage](DAMAGE_SHIELD_COVERAGE_GOAL.md), [live combat](TU15_LIVE_COMBAT_EFFECTS_20261013.md). Preserve source-produced vitality and requests; do not infer full coverage from scalar comparisons. |
| Dynamic world | Props/items, breakage/chained blasts, debris; supports/carriers/conveyors; devices, machinery, doors, lifts/trains, riding/crushing; source lifetimes and replica recreation | [Physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md), [world features](MAP_WORLD_FEATURES.md). Selected lift/gate/script checks do not complete destruction or all maps. |
| Enemy/allied AI | Original perception/awareness/targets; tactical state/orders/cover; paths/steering; weapon/vehicle use; species and movement modes; animation and encounter/difficulty owners | Current native actor loop remains a provisional brain. Recover original owners before replacing it; retain all authored campaign/AI weapon definitions in scope. |
| Objective objects and rules | Flag/ball/bomb pickup/carry/melee/drop/death/return/reset; capture/arming/detonation; role/hill/territory interactions and complete applicable match rules | [Game-mode goal](GAME_MODE_PROGRESS.md). Bounded scoring and object helpers do not establish full normal matches. |
| Animation and feedback | Both species and hands; unit/own-model arbitration; current/saved poses, overlays/blends/IK/root motion; firing/reload/melee/lunge/hit/death and attacker feedback | [Weapon goal](WEAPONS_TU15_PARITY_GOAL.md), [live combat](TU15_LIVE_COMBAT_EFFECTS_20261013.md). Finite pose checks retain their limits. |
| Combat rendering and audio | FP/world muzzles, beams/forces/casings, contrails; impacts/blood; blade/lens attachments; color/light receivers; HUD/scopes/heat/battery; effects and PCM lifetimes | [Live combat](TU15_LIVE_COMBAT_EFFECTS_20261013.md), [shader goal](SHADER_PARITY_COMPLETION_GOAL.md). Full Xbox frame/PCM comparisons remain required. |
| Composed acceptance and delivery | Authored campaign and multiplayer contexts, normal runtime, split-screen and LAN; coherent tested snapshots and installed launchers | Required for every applicable row above. Preserve dirty/concurrent work and record exact source/runtime/reference hashes. |

## Work record

- Initial audit refreshed newer October 13 weapon, vehicle, grenade, combat,
  world and game-mode reports before choosing the first implementation seam.
- First investigation: original weapon targeting and delivered command
  ownership, including the deliberately failing distant-sword acceptance path.
  No target/confidence or source body state will be assigned to force success.
