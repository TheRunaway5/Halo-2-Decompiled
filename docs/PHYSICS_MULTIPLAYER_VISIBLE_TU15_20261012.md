# Visible multiplayer physics acceptance, English TU1.5

The user's 2026-10-11 requirement is to test the physics in actual multiplayer
so its behavior can be seen. The first visible acceptance run has **three
bounded passes and two failures**. Full player, vehicle and object physics
parity remains incomplete. English TU1.5 is the sole executable target.

All five cases call the actual `run_multiplayer` renderer and match loop on
the visible desktop, with ordinary SDL keyboard input and authored Xbox map
data. The local cases use two players. The network case uses a genuine UDP
host and client with two connected participants. Nothing supplies a recorded
contact, force, velocity, solver result or post-step transform.

| Case | Live observation | Result and scope |
| --- | --- | --- |
| Lockout movement, two local views | 181 observed ticks; 15 forward commands; one jump command; upward velocity 2.940958; crouch reaches 1; 1,041 submitted frames. | PASS for native walking/crouching/jumping input delivery and visible playback. This still uses the host terrain mover and does not establish original solved movement parity. |
| Lockout grenade, two local views (`objects`) | One Q command; actual attached and released salted projectile; falling velocity; one detonation; 191 grenade geometry submissions to each view; 1,054 submitted frames. | PASS for this ordinary native grenade lifecycle. Other objects, angular integration, prop impulses and full retail physics remain unverified. Geometry submission is not a pixel comparison. |
| Lockout authored lift interior, two local views | Initial diagnostic root is the lower phantom hull center transformed by the authored placement. Over 181 ticks, maximum upward velocity stays zero and the player falls to the lower floor. 1,070 submitted frames. | RED: the live match never lifts the player. Component force calculations do not complete lift integration. |
| Coagulation vehicle map, two local views | Six authored vehicle candidates (two Warthogs, two Ghosts, two Banshees); zero live scene vehicle roots after 181 ticks; 866 submitted frames. | RED/unavailable: no live driving case exists. Candidate rows do not by themselves establish source spawn admission. Presence alone would not prove driving or solved contacts. |
| Lockout UDP host/client movement | 181 client observations across 177 distinct authority ticks exactly match the retained host position, velocity, player/unit identity and crouch. One jump command, positive upward velocity and crouch reach the actual client path. 1,057 submitted frames; runtime confirms two participants. | PASS for this native LAN movement/replication path. This is loopback UDP, with no WAN or retail trajectory equivalence claim. |

Each recording is the actual 960x540 native X11 window at 30 captured frames
per second. The videos contain 194–205 frames each. Native rendering and video
capture have separate clocks; these are not frame-pacing measurements. X11
capture reports the window disappearing at teardown, and all five streams
finalize with exit zero and pass `ffprobe` frame/size/duration inspection.
The PNG previews are lossless conversions of the renderer's PPM files, with
pixel bytes verified unchanged. Recording samples were visually inspected.

## Evidence

The [validation record](../analysis/multiplayer/physics-live-tu15-20261012/validation.json)
pins the 7,873-file source snapshot, complete source build, acceptance binary,
TU1.5 executable, maps, driver, tick traces, reports and videos. The build
uses Release with `-O1 -g0`, strict per-module FP flags, and rebuilds the full
dependency graph from source; it does not mix earlier ABI-incompatible archives.
Source and build records are distinct from the earlier copied native-binary
baseline in the same evidence directory.

Validation SHA-256:
`40a4681eca180cc4dccee53695ff369f9b5bfe7504126f43a524e93ef652ec52`.

The original executable is SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The Python driver refuses another image. No new 1.1 parity execution is part
of this acceptance run.

- [Movement video](../analysis/multiplayer/physics-live-tu15-20261012/visible-v1/movement/window.mp4)
- [Grenade video](../analysis/multiplayer/physics-live-tu15-20261012/visible-v1/objects/window.mp4)
- [Lift failure video](../analysis/multiplayer/physics-live-tu15-20261012/visible-v1/lift/window.mp4)
- [Vehicle gap video](../analysis/multiplayer/physics-live-tu15-20261012/visible-v1/vehicle/window.mp4)
- [LAN movement video](../analysis/multiplayer/physics-live-tu15-20261012/visible-v1/lan-movement/window.mp4)

## Reproduction

Build the explicit `physics_multiplayer_acceptance` target. It is excluded
from the default build and has deliberately failing feature gates, so a
successful build cannot turn the red lift/vehicle cases green.

```sh
cmake -S . -B build-physics-live-tu15 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-physics-live-tu15 --target physics_multiplayer_acceptance -j 4
python3 tools/test_physics_multiplayer_visible_tu15.py \
  --runtime build-physics-live-tu15/physics_multiplayer_acceptance \
  --output analysis/multiplayer/physics-live-tu15-new-run \
  --xbe '/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe' \
  --ui assets/imported/a13438f3719c48bcb914624ed35022c7/vista
```

Use the visible desktop. The driver uses X11 for its actual window recording,
requires `ffmpeg`/`ffprobe`, and uses Pillow only for lossless screenshot format
conversion. A new output directory preserves earlier runs. `--scenario` selects
a case; omission runs all five sequentially. Expected current suite exit is 2
because lift travel and driving remain unfinished. The unported cases must be
replaced with meaningful live checks when their real producers are integrated;
the vehicle case never awards a driving pass merely for a populated scene.

## Ongoing acceptance requirement

Every physics feature needs real match evidence after integration, in addition
to its TU1.5 original/native differential contract. The acceptance record must
show that the recovered physics owner consumes actual source-created bodies,
contacts and world state. A linked library, an isolated arithmetic pass or the
temporary mover producing visible motion is insufficient.

Continue with lifts, each vehicle/controller class, dynamic props/items,
physical projectile cases, slopes/steps/ceilings, moving supports, collision
impulses, sleep/wake, death/ragdolls and multiplayer authority. Use authored
maps and normal input; retain failures and uncovered cases explicitly. Check
split-screen and LAN views and keep playable recordings. Do not mark the full
physics goal complete while this matrix or the broader required scope remains
open.
