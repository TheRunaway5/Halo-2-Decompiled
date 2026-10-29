# Bundled custom Slayer variants

`data/tags/variants` contains newly authored native definitions. They extend
Slayer, rather than relying on a separate objective engine. They are listed
under Slayer in the original native variant chooser and can be saved/renamed
through its existing settings flow. Native extension IDs are outside the
original 112 parameter range; they never overwrite unknown source-buffer bytes.
The selected custom ruleset is included in loading and UDP admission identity.

## Fiesta Slayer

A 25-kill, 10-minute FFA with a random primary on every spawn and no secondary.
The original 16-choice resolver retains duplicate rocket entries and retries
NONE/excluded results using the shared gameplay RNG. All 14 reachable authored
weapons remain in the pool. Map pickups retain their authored defaults. This
is a new Slayer definition; the original built-in Fiesta Oddball row remains
separate and unchanged.

## Infection Slayer

A custom Halo 3-style, single-round survival mode using individual Slayer
scores. With at least two players, one initial zombie is chosen randomly.
Survivors are red and start with a shotgun and magnum. Zombies are green,
carry a sword, have no shields or grenades, and cannot pick up or switch to
other weapons. Damage between members of the same faction is disabled.
Human deaths, including suicides/world deaths, automatically convert that
player to a zombie on the next spawn. Infecting every human ends the round;
otherwise surviving humans receive two points when the three-minute limit
expires. The clock starts when the second player arrives. Ordinary kill/suicide scoring and the existing Slayer carnage report
remain active. These are explicit custom defaults, rather than a claim that
all Halo 3 variant tuning has been reproduced.

Green is original `matg` appearance-palette entry 6; red is entry 2. Authoritative
profile appearance is replicated and applied to body/hand materials. Chosen
names and stored user profiles remain separate from these per-match colors.

## Use and packaging

Choose Slayer → Fiesta Slayer or Infection Slayer in the game, or launch:

```sh
./Halo2-Launcher --launch-game --custom-slayer fiesta --local-players 4
./Halo2-Launcher --launch-game --custom-slayer infection --local-players 4
```

For a direct native runtime, add your normal map/shared-data options. Local
Infection requires at least two players; System Link/ONLINE may gather remote
players. Both custom modes use the native authoritative match, with original
TU1.5 data supplying their existing weapon/animation/damage consumers. The
separate source-VM world does not implement these newly authored rulesets.

LAN and split-screen match editors keep each mode's defining rules fixed.
Fiesta exposes score, time, player limit, lives, respawn/suicide penalties,
grenades and supported damage settings. Infection exposes survival time,
player limit, respawn/suicide penalties and supported damage settings. Faction
loadouts, conversion, zombie traits and the survival/infection endings remain
fixed. Unsupported original-engine controls are omitted from these custom
editors. Named copies retain native extension writes as well as the complete
original settings buffer. Select a fresh LAN session to use a custom mode;
the retained stock-game lobby does not change into a custom-mode session.

Building `halo2_native` copies `data/tags` beside the executable and generates
`releases/halo2-custom-slayer-v1.zip`. The repository also includes that portable
release asset. The generated bundle now also includes the Assault-based
[Bomb Defusal](BOMB_DEFUSAL.md) definition alongside these two Slayer definitions
and a SHA-256 manifest;
no map/model/texture/executable payload is included. Extract it beside the
matching runtime, preserving the `data/tags/variants` paths. Rebuild/reverify:

```sh
python3 tools/bundle_custom_slayer.py --output releases/halo2-custom-slayer-v1.zip
python3 tools/bundle_custom_slayer.py --verify releases/halo2-custom-slayer-v1.zip
```

The new Action Sack catalog selects these two modes on Lockout/Foundation,
unranked with couch guests allowed. Its equal weights and 6–8-player dimensions
are explicit native configuration, not a recovered historical Bungie rotation.
It requires `halo2-native-queue-v10`; older clients retain the existing queues
and cannot enter unsupported custom games. The v10 descriptor explicitly binds
FFA/team mode while retaining one authenticated connection per couch. Guests'
personal results remain excluded from persistent carnage history.

The [Tombstone catalog](../multiplayer/docs/tombstone-playlist-coverage.md)
extends that rotation with revised Xbox Tombstone Fiesta and adds ordinary
Slayer/Team Slayer map coverage in a staged catalog. Live Action Sack includes
Tombstone Fiesta; ordinary Tombstone rotations await the TU1.5 physics/runtime
path. The original validation catalog stays available.

## Verification and remaining weapon scope

`custom_slayer_tests` compares 789 unchanged-original random/direct controls,
all 14 authored weapon spawns, 12 unchanged-Vista secondary-field controls,
initial green-zombie selection, conversion/sword respawn, survival bonuses,
delayed starts and both completion conditions. An actual authored sword strike
kills and converts a human; the converted player respawns with a sword. Actual
shotgun fire kills a zombie while same-faction fire causes no vitality loss.
Across all 12 disc maps the native lifecycle/loadout checks pass. The existing 276 PC variant controls and multiplayer regression
remain passing. Two independent four-player native clients complete each mode
through actual relay gameplay, unanimous Vista PCR, Done/requeue and website
history; four authenticated records are kept and four guest records excluded.
Discord verification is the external fixture in that automated test.

Finite Vulkan captures exercise four views, firing and original Vista HUD
assets at 1280×720, with a 3440×1440 Infection four-view HUD/color check. These are Linux
host-render checks, not physical four-controller or separate-WAN validation.
Full weapon/effect parity remains the separate active weapon goal. In particular,
the current renderer reports unported muzzle/ready-effect lifecycles for some
weapons; completing the custom rulesets does not establish full weapon parity.

The historical inspiration is documented in the official
[Infection overview](https://www.halowaypoint.com/news/infection-overview-season-4),
including the original green sword-zombie/red-survivor convention. The specific
automated conversion, timing and scoring defaults above are this custom mode.

Backend release `20261013T200402Z` is active. Both root launchers select
`.local/custom-slayer-20261013/delivery/halo2_native`, with the original Vista
HUD/menu/font assets retained. Release evidence is recorded in
`.local/custom-slayer-20261013/verification.json`.
