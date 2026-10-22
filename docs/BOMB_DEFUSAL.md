# Bomb Defusal

Bomb Defusal is a native custom Assault variant. Select **Bomb Defusal** under
Assault in Custom Games, or pass `--custom-mode defusal` to the native executable.
The older `--custom-slayer` switch also accepts its full name. Choose Red and Blue
teams; the mode permits up to eight players on each team (16 total), including
**Big Team Bomb Defusal** matches.

Normal MCC onboarding installs this definition with the standard native assets,
including when reusing an existing import. Portable Linux and macOS launcher
packages include it; no separate variant download is needed. The definition is
also retained under `data/tags/variants` in the prepared cache. Original map
bytes are unchanged.

This implements Counter-Strike's bomb objective and round structure with Halo 2
combat and newly authored timings. It uses Halo weapons, shields, movement and
grenades. There is no money, buy menu, defuse kit or equipment economy.

| Rule | Default |
| --- | --- |
| Team size | Up to 8v8 |
| Plant window | 2 minutes 30 seconds |
| Planted bomb fuse | 45 seconds, independent of the plant window |
| Plant | Hold Use for 4 seconds, stationary and grounded |
| Defuse | Hold Use for 6 seconds, stationary, grounded and beside the bomb |
| Preparation | 5 seconds with movement and combat inputs locked |
| Between rounds | 7 seconds |
| Lives | One per round; late joins wait for the next round |
| Sides | Red attacks first; exchange physical sides after round 12 |
| Match | First to 13 round wins; 12–12 is a draw |
| Loadout | Battle rifle, magnum and one frag grenade |
| Motion tracker | Hidden; rely on sight, sound and team communication |
| Map equipment | No automatic weapon pickup or vehicle spawning; dropped guns can be used |

Attackers win by eliminating the defenders or detonating the bomb. Defenders
win by eliminating the attackers before a plant, preventing a plant until time
expires, or defusing the planted bomb. Killing all attackers **after** a plant
does not end the round. Only one site needs to be bombed. Kills remain statistics
and never add objective points.

Releasing Use, moving, jumping, leaving the interaction range, changing plant
sites or changing defusers resets interaction progress. A wall blocks bomb
recovery and defusing; a player cannot interact from another floor. Planting and
defusing suppress combat inputs. Attackers recover a dropped bomb by walking
over it; defenders cannot carry it. The drop remains at the carrier's last
grounded position, keeping a death over a pit from making it inaccessible.

HUD text shows roles, round score, countdown, possession, interaction progress
and site distances. Framed world markers identify **BOMBSITE A** and **BOMBSITE B**.
After planting, the active marker tracks the bomb itself, shows the fuse time,
and fills red from bottom to top. Its red outline/fill pulses at half a cycle per
second initially, one below 20 seconds, and two below 10 seconds. The letter stays
white and readable. Presentation derives from replicated fuse ticks and is visible
to eliminated players as well as live players. Only the unit HUD submits these
objectives; weapon HUDs do not duplicate them. The host owns all objective state; native snapshots include
sites, carrier, round results and timings. Protocol 96 prevents older peers from
misreading the extended snapshot. Native authority checkpoints retain the mode.

## Balance rationale and playtest targets

These are starting values, not a claim of measured competitive balance.

Halo's shield recovery allows a retake to include an exchange, a disengagement
and another push. A 45-second fuse leaves 39 seconds before the final uninterrupted
six-second defuse must start. Four seconds to plant gives defenders a real punish
window without leaving the attacker exposed for an entire extended firefight.
Six seconds to defuse rewards cleared space while leaving a plausible solo
retake. A universal time avoids a kit/economy advantage that this mode cannot
support. Damage alone does not reset progress; movement, loss of range, release
or death does. A defender can deliberately risk a contested defuse.

The 150-second plant window permits an opening fight and a rotation without
making no-respawn players wait through vanilla Assault's long rounds. Equal
rifle/pistol starts and disabling map weapon/vehicle respawns avoid a recurring
rocket, sniper or vehicle advantage being mistaken for objective balance.
One frag per player limits opening grenade volleys as team sizes grow.
The motion tracker is hidden so defenders must earn information about a split
or rotation; automatic close-range radar would substantially strengthen site
camping and weaken attackers' ability to fake a push. Normal shield recovery
and movement are retained. The bomb carrier keeps their
guns and movement speed; holding Use to plant is the commitment window.

Start competitive layouts with **two sites**. More sites increase the area a
defending team must cover; support for sixteen sites is for custom
layouts, not a recommendation to enable all sixteen. Each site should have at
least two usable approaches, cover for a retake, and no plant position that is
fully protected from every approach. Avoid a single long sightline covering
both sites or a spawn that lets defenders immediately watch the attack spawn.

Before calling a layout balanced, measure both halves with equally sized teams:

- Attacker and defender round-win rates, split by site and pre/post-plant wins.
- Travel from the farthest realistic rotation to each planted site. Aim for
  roughly 10–15 seconds of ordinary movement so the 39-second retake budget
  still allows fighting and shield recovery. Actual routes matter more than
  straight-line site distance.
- Time of first contact and frequency of grenade damage reaching an enemy
  spawn before players can leave it.
- Plant success, defuse attempts abandoned, successful solo retakes and time
  eliminated players spend waiting.

For Big Team matches, playtest with all sixteen players: eight defenders can
hold more approaches, while larger attacking groups can sustain longer pushes.
Use layouts with enough room for eight distinct opening spawns on each side.

Change site placement and approach cover before compensating for a bad layout
with global timers. Then adjust the fuse in five-second steps and interaction
times in one-second steps. A timer change needs fresh playtests on both sides.

## Multiple sites and existing maps

The bundled `data/tags/bomb-defusal-sites-v1.json` selects two ordered sites and
separate attack/defense spawn anchors for the recommended MCC map pools:

| Map | Suggested teams | A | B |
| --- | --- | --- | --- |
| Turf | 4v4 | Warehouse | Street |
| Ivory Tower | 4v4 | West wing | East wing |
| Sanctuary | 4v4 | Blue base | West walkway |
| Beaver Creek | 4v4 | Blue base | Creek |
| Tombstone | 4v4 | Upper base | East platform |
| Headlong | 8v8 | Construction base | East courtyard |
| District | 8v8 | Garage | East street |
| Zanzibar | 8v8 | Base | Wheel |
| Terminal | 8v8 | Depot | East courtyard |
| Burial Mounds | 8v8 | Camp | Central wreckage |
| Relic | 8v8 | Structure | East approach |

These are authored starting layouts, with site floors and player clearance checked
against the imported geometry. Team sizes are recommendations; each permits up to
8v8. Played routes and competitive win rates remain playtest requirements. Relic
explicitly starts attackers on the beach and defenders at the structure, independent
of the vanilla scenario's team numbers. Both teams exchange these physical sides at
halftime.

Choose **Bomb Defusal** and then the map; the world loads its layout automatically.
Setup, cache reuse and the portable bundles include the layout catalog alongside
the variant. Profiles require the exact scenario path and expected marker identities/
positions. A different scenario uses the fallback; changed referenced markers reject
the stock profile. Layout bytes
participate in peer/prepared-world compatibility. Native initialization places each
site on its checked walkable floor; no scenario map bytes are edited.

Maps outside this authored pool use **every distinct team 1 Assault goal marker** (scenario
netgame flag type 3, team/designator 1), including markers on different channels.
That fallback does not promise separated A/B objectives. Co-located vanilla variant markers are deduplicated. Site order follows map
order; labels run A–P. Both halves defend the same physical sites, and teams
exchange physical spawn groups at halftime. Each group uses distinct authored
player starts nearest that side's Assault home, assigning each start to its
closer home and spacing teammates apart. Stock maps often label all starts 0
or 8, so their legacy team labels do not select sides. Without home markers,
explicit team starts anchor the groups. A map requires enough starts for both teams and
at least one site. A stock map with only one distinct
defending goal still has one site; the mode does not invent a second location.

An explicit layout can reference any existing scenario marker's position using
native parameters `2147483904` through `2147483919` (site A through P). When any
are set they replace the bundled A/B layout or automatic goal list; spawn groups
then use the normal Assault homes. For example, after inspecting a
particular map's marker indices:

```sh
--custom-mode defusal --mp-rule 2147483904 5 --mp-rule 2147483905 12
```

This example selects marker rows 5 and 12 as A and B; those indices are examples,
not a qualified layout for any particular map. Missing marker indices reject
the match. Store the same ordered parameters in the variant's JSON `settings`
to bundle a map-specific layout. All overrides participate in peer and prepared
world compatibility checks. No imported map bytes are modified.

## Editing and validation

The authored definition is `data/tags/variants/bomb-defusal-v1.json`. Original
parameter 10 sets the plant window in seconds. Native parameters 2147483650,
2147483651 and 2147483652 set fuse, plant and defuse durations in 30 Hz ticks.
The fuse must exceed the defuse time. Native extensions stay outside the
original 304-byte Assault buffer.

```sh
python3 tools/run_tests.py --target bomb_defusal_tests --target bomb_defusal_match_tests \
  --target bomb_defusal_map_tests --target bomb_defusal_hud_tests \
  --filter '^bomb_defusal_' --test-jobs 1
```

The rule tests cover multiple sites, interaction cancellation, carrier loss,
full-generation roster eligibility, timeout boundaries, halftime, first-to-13
and 12–12 draws. Match tests exercise native bodies/loadouts, held Use planting
and defusing, round resets, life limits, late joins, full 8v8 admission and UDP
snapshots (including rejection of a ninth teammate), distinct sixteen-player spawns,
halftime and full-match result finalization in
a synthetic arena. This does not replace rendered, imported-map multiplayer
playtesting or establish balance across the stock map catalog.

With a selected MCC cache, the map tests construct native worlds on all eleven
configured maps, admit sixteen players, plant at both sites, defuse, and reset
the round. Positions are fixture inputs; these tests do not measure walking
routes. The HUD tests check mesh output across split views and use Vulkan
framebuffers to verify growing red fill and the final-fuse pulse. Missing MCC
inputs or a rendering display report a skip rather than a pass.
