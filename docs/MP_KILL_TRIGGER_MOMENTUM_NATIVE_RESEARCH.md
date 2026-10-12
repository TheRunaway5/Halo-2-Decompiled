# Live kill-trigger momentum retention

`src/multiplayer.cpp` now omits only `u.body.velocity = {}` in the existing
kill-volume postpass. Its comment cites the complete original `DEFC0` fatal
return measured in [the independent source continuation](MP_SPARTAN_KILL_TRIGGER_SOUND_RESEARCH.md).
No kill predicate, bounds center, scan order, attribution, vitality, death pose,
physics, corpse motion, timer or respawn rule is changed.

The source proof is immutable at
`analysis/multiplayer/integration/spartan-kill-trigger-sound-20261010/frozen/manifest.json`,
SHA-256 `d45cf0beda489f0c399af3eef2819069dc546424e1e08ce2968f8a9a77915079`.
It proves complete fatal return with retained actual movement momentum under
explicit selected initialization and unavailable playback, not naturally
completed map/audio startup or a naturally falling original trajectory.

## Production-facing regression

`tests/multiplayer_kill_trigger_momentum_tests.cpp` accepts
`MAP SHARED_MAPS UPDATE_XBE` and explicitly constructs `OriginalData::load_xbe`.
Both Matches allocate an actual source-authored participant, unit and loaded
starting SMG. Through the public diagnostic teleport API they receive the same
start, one unit above the upper face of the actual authored unparented Lockout
kill volume. All subsequent positions and velocities come from normal native
movement/gravity; the test does not assign velocity, vitality, animation pose,
contact or fatal state.

A separately declared native control copy differs only by clearing
`kill_triggers`. Its identical movement step exposes the current velocity
immediately before the admitted Match's kill postpass. This is a native
comparison control, not an original execution or an alternate collider model.
The test compares exact float words for both positions and velocities, then
checks the actual fatal kill path, previous owner, full old unit datum and
death tick. It waits for actual respawn and confirms the distinctly salted new
unit retains the old corpse identity and fatal momentum in the existing bank.
The retained corpse is not advanced or presented as a solved ragdoll.

## Validation

Evidence root:
`analysis/multiplayer/integration/kill-trigger-momentum-native-20261010/`.

- `normal-test`: passes using the actual current `multiplayer.cpp` object and
  copied freshly built release Systems/Cache/dependency archives.
- `old-clear-test`: the exact same test/dependencies fail at
  `Kill-trigger fatal postpass discarded actual current falling momentum` when
  the former clear is restored only in an isolated object. Shared source was
  never reverted.
- Fresh production target in `build-animation-event-asan-20261008` rebuilt with
  actual AddressSanitizer/UndefinedBehaviorSanitizer flags and passes the
  authenticated u5English direct run with leak detection/halt-on-error enabled.
- The registered `original_multiplayer_kill_trigger_momentum` CTest passes
  with the original map-pack1.1 executable. Root owns the target/CTest CMake
  registration. Initial unknown-target Make failure is retained; regeneration
  and fresh complete target build are separately logged.

Actual direct-run inputs:

```text
assets/disc/maps/lockout.map
assets/disc/maps
/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe
```

The standalone input manifest pins source, all copied public headers and normal
archive dependencies. Actual normal/negative/sanitizer binaries, logs and
instrumentation/flags provenance are retained by the native validation freeze.

## Remaining integration limits

The existing live postpass still scans all imported triggers and tests native
body position. Original `1EFEF0` instead selects one enabled row from the
wrapping tick/datum remainder and `11DB80` consumes source current bounds center
at object+30. Those producers and scheduling inputs are not changed here.
This fix preserves the actual current native momentum once that existing
postpass admits death. It does not establish source-faithful fall trajectory,
complete fatal attribution parity, landing-dead selection, natural +34B
admission, a ten-body solver or visible ragdoll animation.

The root's independently rebuilt coherent revision4 production source exactly
matches this test's copied normal `multiplayer.cpp` SHA-256
`23fb1b41a114e5df69abb861537dd6234c8dec807b0864d64cfed1fec497b738`.
Its authenticated1.5 falling test passes (binary SHA-256
`ba863a4430fe599fbb35ed6290e8af4889f7938ea71fdeeb1d74a77ac1b12048`),
recorded in `analysis/sandbox-frozen-v4-death-tests-20261010.json`.
Revision4 manifest SHA-256 is
`01375e8a3619483162056896ce6fc6973a5540040ba98847ad5ac9eeec87f1ae`.
A later unrelated working-source motion-sensor publication changed
`multiplayer.cpp` and three public headers while validation was running. The
fresh actual sanitizer generation includes that later source; the earlier
normal/RED generation and root's consistent revision4 omit it. Both generations
are retained separately, and the collection-time drift check is documented.
No motion-sensor edit belongs to this momentum change.
