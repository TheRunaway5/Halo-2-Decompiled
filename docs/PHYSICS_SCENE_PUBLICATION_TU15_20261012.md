# Source physics transforms in the native scene

Completed source object roots now reach scene interpolation, prop/scenery
render roots, prop collision, scenery collision and device roots. The match
publishes after its source object/world phases and refreshes the native query
geometry from that completed state. Placement Euler angles remain metadata;
source rotations are not converted through those angles.

The read-only adapter follows actual TU1.5 `BF5C3..BF620`: object position at
`+64`, forward at `+70`, up at `+7C`, scale at `+A0`, and flag 10. Attached
objects use their actual parent's selected WORLD node and handedness. Full
salted identities, body extents and parent node extents are checked before
publication. The adapter resolves the entire scene batch before committing
native presentation changes, so an invalid later owner cannot leave a partly
updated scene. It never writes physics state or advances the source clock.

The same publication now retains the actual WORLD hierarchy at object
`+114/+116`, with signed descriptor and logical body-extent checks. Prop and
scenery posing and query triangles consume these matrices when a source owner
is bound. A native bind pose is not supplied for an empty source hierarchy.
Their render history samples completed node banks independently of the root.
Mirrored bases retain their handedness during quaternion interpolation.

Player bodies have their own full-handle WORLD history because they are not
scenario `SceneObjects`. The renderer captures every completed authority tick
and samples the same pose for every split-screen view. Detached dead bodies
retain their actual source identity and nodes until source retirement. New
salts, changed hierarchy sizes, missing ticks and restored clocks cut the
interpolation interval. This is native high-refresh presentation between
completed source states; it does not change the original simulation rate.

Native protocol45 carries those body nodes and the retained object `+30/+3C`
sphere in complete snapshots. Clients consume each accepted host bank and
use its same WORLD history rather than a separate source world or a local
idle pose. Transport keeps every finite float word, including signed zeros,
and requires actual roster identities, unique bodies and every living source
body. The previous32KiB complete-snapshot limit could omit this full-roster
use case by rejecting the whole snapshot. The two-byte existing total now
admits65,535 bytes; datagrams remain bounded at1400 bytes. Larger complete
states still reject explicitly rather than truncate source data.

Frozen Release and address/undefined/leak sanitizer builds pass explicit input
tests for source/parent identity, current parent node selection, read-only bank
access, atomic publication, scene bounds, retained placement metadata and
exact interpolation endpoints. All nine changed scene/match/network consumers also
pass compilation checks against their frozen headers. Each binary separately
matches all 13 output words from unchanged TU1.5 `BFC70` in 256 root/parent
cases, including mirrored roots, mirrored parents, scaling and signed zeros.
The original comparison uses Unicorn with no patched instructions or supplied
callee results. It does not establish Xbox hardware equivalence.

The `frozen-release-v4` and `frozen-sanitizer-v4` cohorts and their independent
original comparisons are under
`analysis/multiplayer/sandbox-source/source-scene-physics-publication-20261012/`.
The reused cache archive is pinned but uninstrumented. These are presentation
and publication tests over explicit input banks, not live solved-contact,
gravity-lift, driving, ragdoll or multiplayer acceptance. Node interpolation
and codec cases use explicit test inputs. Their pass does not prove a live
source-produced death pose or end-to-end source-body UDP rendering. Damage
state and source machine channel integration remain separate. The broader
physics integration and playable build are still required; live startup is
still reaching unbound original source callbacks.

```sh
python3 tools/test_source_scene_physics_publication.py --output /tmp/halo2-source-scene-release
python3 tools/test_source_scene_physics_publication.py --sanitizer --output /tmp/halo2-source-scene-sanitizer
.tools/python/bin/python tools/differential_source_scene_physics_tu15.py \
  --binary /tmp/halo2-source-scene-release/source_scene_physics_tests \
  --output /tmp/halo2-source-scene-original
```

The root CMake test is `native_source_scene_physics`; its target is
`source_scene_physics_tests`. Owned game/cache assets remain outside the code
and test fixture. This adapter adds no BIOS or EEPROM input.
