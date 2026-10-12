# Sandbox revision6, 2026-10-10

This build fixes three observed failures: Midship plasma throws stopped on their
first terrain contact, Midship muzzle startup rejected a valid shared movement
definition, and fixed scenery with collision models was absent from the live
collision world. These fixes do not establish complete multiplayer parity.

The isolated source/build is
`analysis/multiplayer/integration/sandbox-release-r6-20261010`. It composes the
previous fixed revision5 with only the fixed-scenery importer and collision
consumers. Concurrent frontend, renderer and shader changes are not copied.
The original revision4 installation and its immutable manifest remain intact.
The separate project `Halo2-Sandbox-Launcher` is the test-build entry point.

## Plasma material response

An actual Midship Match starts with the authored two plasma grenades. The old
response path rejected any nonzero alternative chance, even though the actual
plasma record admits its alternative stick response only for unit contacts.
Real SDL Q input reached the source throw action, debited inventory and released
the projectile, then stopped at its first terrain contact with
`Projectile response requires original stochastic/contact selector`.

The new response leaf follows the original material-parent lookup and
base/alternative selection. Its fixture independently executes the unchanged
TU1.1 and supplied u5 routines: 376 material lookups and 3,066 response/RNG
choices, including exact LCG threshold seeds and explicit branch controls.
Both independent runs are byte-identical. The native comparison includes the
resulting random state; it preserves the measured x87 53-bit product before
comparison. The current terrain producer needs no RNG and correctly chooses
bounce. Scenario-object contacts use the actual placement's object type.

The fixed live Midship test delivers real SDL Q without assigning inventory,
health, projectile, contact or timer state. It observes one begin, one release,
three terrain contacts and one detonation at tick51; inventory ends at zero
frag/one plasma. It passes with both supported executable providers. The
four-view Vulkan/SDL/native-UDP tests complete plasma and frag lifecycles and
check source geometry submissions independently for all four views.

Unit contact shapes/attachment and plasma sticking, angular integration,
remaining-step collision response, water and complete original world contact
production are still unresolved. Native terrain collision is the existing
triangle backend, not original Havok.

## Muzzle movement aliases

Actual Midship effect metadata references a `pmov` tag with a nonzero address
and zero owned extent. Another positive-size `pmov` tag owns the very same
address. The old marker validator loaded the zero-size alias as an independent
storage owner and failed before any draw. The scoped typed resolver now retains
the requested tag identity while reading the actual same-address, same-group
owned payload. It rejects missing or inconsistent owners. General resident
metadata loading and overlapping-storage policy are unchanged.

All 23 owned multiplayer maps pass the typed import test, covering 69 aliases.
Every alias's fields, controller parameters and curve bytes compare with its
actual backing definition. The complete original controller walker
`2C06C0` and its uniquely paired u5 body independently execute all 69 actual
empty-controller packets: 138 calls, one count read and no controller calls.
This is consumer evidence, not original cache-loader or graphics execution.

Actual SDL fire tests on Midship now render the authored SMG muzzle particles,
check nonzero alpha and eventual expiry, and verify foreground-view ownership
in local split screen and native LAN. The screenshot is
`analysis/sandbox-r6-muzzle-midship-20261010.png`. Remote/world attachments,
other weapons and original particle RNG/lifecycle parity remain unfinished.

## Fixed scenery collision

The new importer admits actual type6 scenery only when its model has a single
root node, no animation graph, no physics model, no variant bank and no damage
info. It uses the original model/collision region and permutation mapping,
authored WORLD root transform, surfaces, flags and material identities. It
does not create stationary stand-ins for vehicles, animated objects or bodies.

The independent old-build regression instantiated 14 genuine old-map scene
placements and reproduced all 164 missing surface queries. Their WORLD node0
bytes match the original cold object constructors. The fixed Match passes all
164 queries against the independently verified original posed-surface leaf,
plus 24 native player-capsule support contacts. A named Dune ramp is destroyed,
recreated with a new generation, hidden across a BSP switch and restored from
checkpoint; its collider follows that scene lifetime in every case.

The composed live worlds now include these fixed colliders for movement and
grenades. Shot and blast queries use fixed scenery while the existing exact
posed-prop query retains explosive damage ownership. This avoids a duplicate
triangle hit suppressing the prop damage result. These checks establish fixed
scenery behavior in the existing native geometry backend, not original rigid
body or general scenery parity. Additional DLC candidates remain unmeasured
against the original cold constructors.

## Validation and delivery boundaries

The fixed build has 31 passing CPU invocations, six passing ASan/UBSan
invocations, four passing grenade/prop SDL/Vulkan/native-UDP checks and two
passing Midship muzzle checks. `cpu-validation.json`, `asan-validation.json`,
`gpu-validation.json` and `muzzle-validation.json` pin commands, executable
hashes, exit status and logs. Actual prop regression still traverses SMG damage
through the authored delayed explosion, collider removal, UDP delivery and
exactly one authored SDL explosion sound. Physical speaker playback is not
established by dummy-device audio tests.

The lift scalar leaf passes 62,464 original delta/ratio pairs but has no live
contact/body-force consumer. Current fatal-damage and falling-kill regressions
preserve actual momentum and full corpse identity through respawn; they do not
solve, animate or render ragdolls. Teleporter traversal, vehicles and seats,
dynamic prop/Havok physics, full grenade sticking, all effects/audio and complete
local/LAN behavior remain required work. The vehicle inventory contains 57
authored netgame vehicle rows in 12 maps that the current live importer skips.

The test launcher starts at the title screen and uses the externally selected
verified supplied u5 executable. Delivery contains our executable code and
compiled shaders, with game assets left in the user's external imports. Keep
the frozen source/build/delivery tree at its recorded paths: the renderer has
compiled shader paths into `build/shaders`, so this is a local installation,
not a relocatable package. The delivery manifest separately pins runtime
copies, required build shaders, source files, validations and external inputs.
