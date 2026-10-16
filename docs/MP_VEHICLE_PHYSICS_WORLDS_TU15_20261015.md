# TU1.5 vehicle update and world integration

All eight surveyed original multiplayer driver choices pass fresh visible native
multiplayer on the v385 coherent source cut, retaining6,613 exact UDP states.
Every case uses normal seat entry, throttle, steering, mouse aim and exit;
actual original vehicle cameras and actual source vehicle/held-body geometry
are checked in both views. Every camera responds by approximately0.6 radians.
The X11 desktop mirrors are verified continuously and every video retains one
stable acceptance ELF. These are original Xbox maps and English TU1.5 gameplay.

Each case exports read-only completeB8F40 object-update and1CC8D0 world-solve
entry/return state after actual occupied travel. All16 captured phases match
untouched TU1.5 i386 CPU execution and their real callees:1,540,296 instructions
in total, every accessible60,084,310-byte final source bank per case, and every
captured GP/SSE/x87/FP control/status value. The previously documented host user
interrupt flag already differs at entry and is the sole excluded EFLAGS bit
(mask0xFFFFFDFF). No code, callee, force, contact, solver or pose result is
replaced. This is host CPU evidence, not Xbox hardware or full trajectories.

| Vehicle | Native travel | Native steer radians | Original object instructions | Original world instructions |
| --- | ---: | ---: | ---: | ---: |
| warthog | 34.982197 | 0.818596 | 137524 | 83781 |
| ghost | 6.939128 | 0.555039 | 57917 | 51335 |
| banshee | 13.906403 | 0.795558 | 39867 | 70164 |
| scorpion | 10.197859 | 0.516348 | 224526 | 52079 |
| wraith | 18.797117 | 0.664448 | 106059 | 147871 |
| spectre | 26.051962 | 0.803060 | 116438 | 74096 |
| spectre_secondary | 23.005327 | 0.624252 | 114386 | 63459 |
| warthog_gauss | 34.990807 | 0.975394 | 138244 | 62550 |

Evidence under `analysis/multiplayer/physics-live-tu15-20261012/`:

- `current-driver-phase-matrix-v385/driver-matrix.json`: eight live reports,
  original camera/geometry checks, captures and unaltered window videos.
- `driver-phase-original-v385/summary.json`: all16 whole-original comparisons.
- `source-snapshot-v385.json`:6547-file coherent source manifest and explicit
  boundary from the concurrently edited canonical checkout.
- `tested-runtime-v385/physics_multiplayer_acceptance`: immutable tested ELF,
  SHA256 `521c70a12e88a5da9639328c6632bbe7e51093d2c6dc8a25f75df2110a5a16ed`.
- `tested-runtime-v381/halo2_native`: immutable production executable with the
  same physics implementation; later v382..v385 changes are test inputs and
  read-only diagnostics. The v381 complete source archive retains the physics
  source. `source-delta-v381-to-v385.tar.zst` overlays the one changed fixture
  to reproduce all6547 v385 source hashes; `source-delta-v385.json` verifies
  both archives and the recovered exact fixture SHA256.

The prior occupied Spectre v384 repeats also pass visibly with21.833986 and
24.281755 units of travel, actual passenger/driver cameras,637/653 UDP states
and whole-original object/world comparisons across111,654/89,324 instructions.

Full physics parity remains active. These finite cases cover steady driving,
turning, ground/hover/flight behavior and actual original cameras. Vehicle
collision/damage/destruction, remaining player/physical-carrier and shared
engine branches, original quantitative trajectories and complete canonical
production delivery still require completion. The next case uses ordinary
vehicle controls to approach an existing cached BSP wall; it supplies no
collision, damage or physical result.

## Wall-contact continuation

The first v386 wall case is retained as a fixture failure before gameplay. Its
input survey required an eight-unit runway and excluded the actual nearby
walls. `coag-wall-ray-survey-v387.json` independently decodes all6,666 original
BSP polygons and their edge/plane data. It finds multiple tall wall directions
at4.83..6.66 units from the original Warthog placement. The repeated fixture
admits a four-unit runway; its actual driver, motion/deceleration, camera and
exit assertions are unchanged. A real wall-contact case remains in progress.
No BSP, collision material, vehicle or physical result is edited.

The v387 Warthog reaches the existing wall under normal controls:5.143996 peak
horizontal speed,1.172639 minimum root/plane distance and eight stopped ticks
under held throttle. Source entry/exit, vehicle geometry,902 exact UDP states
and all camera/pose equality checks pass. The overall run stays red because
the prior camera-response observer excluded parked frames. The retained video
and camera packets actually contain the parked look turn; the repeated fixture
includes that observation period while retaining the0.05-radian response gate.

Its actual complete collision-phaseB8F40/1CC8D0 captures both match untouched
TU1.5 execution across130,759/64,870 instructions and every60MB/CPU/FP output,
within the existing host IF limit. These are real stopped-near-wall contexts;
the observer does not inject a collision or supply a damage result. Vehicle
blast, damage/destruction and broader contacts still require completion.

The v388 repeated wall case passes all gates, including the actual parked
vehicle-camera response0.6 radians and902 exact native UDP states. It reaches
5.143567 peak horizontal speed, records eight stopped-under-throttle ticks and
completes normal exit. The camera observer's parked interval was the only
repair; gameplay camera/physics outputs were not changed. A subsequent normal
post-exit grenade input is being added to observe original vehicle impulses and
normalized vitality, without supplying damage or a physical result.

The v389 normal post-exit grenade test passes visibly with902 exact UDP states.
The parked Warthog receives one original point impulse and its original body
fraction falls from1.0 to0.35999995; the player survives. This is observed source
damage, not an assigned vitality or physics result. A second ordinary starting
grenade is being tested to reach destruction. The fixture also captures complete
object/world phases after the actual point impulse for original comparison.

The v390 two-grenade run passes its initial gates with902 exact UDP states,
four target-vehicle impulses and body fraction-1.4800001. Authored Warthog
hubcaps, sail panels, tires, bumper, winch, fender and chaingun shield garbage
are actually born through the source damage path. Six real impulses (including
two hubcap impulses) match all1,536 motion bytes in independent untouched
original79-instruction execution. Complete post-impulse object/world captures
match212,591/266,896 original instructions and all60MB/CPU/FP output.

The stronger repeat additionally requires the original dead flag, source-solved
moving debris and actual debris draws in both views, with an ordinary on-foot
inspection approach after the blasts. A nonpositive body fraction alone is
not treated as complete vehicle destruction/physics coverage. Complete phases
after observed death are also exported read-only for original comparison.


## Observed vehicle death and rocket impulses

The stronger Warthog v391 repeat passes visibly: two normal grenades produce
four target impulses, original body fraction -1.47261214 and 110 original dead
flag observations. All 13 born debris objects move (maximum displacement
2.783488) and appear in both views. The player survives and walks toward the
wreck through normal input. Complete post-death object/world phases match
174,729/515,044 untouched original instructions and every source memory/CPU/FP
output, within the existing host interrupt-flag boundary.

The independent v392 rocket case also passes: ordinary starting-weapon choice8
selects the authored Rocket Launcher. Four normal fire commands produce four
actual target impulses and original body fraction -1.6, with 164 dead-flag
observations. Nine authored moving debris parts appear in both views, with
1.524528 maximum displacement; the actual seated vehicle camera responds by
0.47 radians before normal exit. All 902 UDP states match authority. The actual
post-death full object/world calls match 130,677/260,875 untouched original
instructions, all 60,084,310 final source bytes each, and all captured CPU/FP
state within the documented host IF boundary. Four point impulses separately
match all 1,024 motion bytes across 316 original instructions. These finite
Warthog cases do not establish every vehicle's destruction path.

Evidence: `warthog-death-body-live-v391`, `warthog-post-death-{object,world}-original-v391`,
`warthog-rocket-destruction-live-v392`, `warthog-rocket-death-{object,world}-original-v392`
and `warthog-rocket-impulses-original-v392.json`, under the same evidence base.
The recorded videos retain actual split-view multiplayer windows. No vitality,
force, damage, collision or debris result is supplied by the fixture.


## Eight original vehicle destruction cases

All eight surveyed driver choices now pass bounded visible native multiplayer
destruction cases, retaining 18,750 exact authority/UDP comparisons. Every
case requires ordinary driving/steering/entry/exit, the actual original seated
camera and its mouse response, target-specific original point impulses, a
nonpositive original body fraction with the original dead flag, source-solved
moving authored debris and actual debris geometry in both views. Four cases
also include the existing BSP wall-contact gates. Heavy vehicles, Ghost and
primary Spectre use a separate driving/destruction sequence; their rejected
wall/early-deadline trials remain red.

| Vehicle | Exact UDP states | Parts visible in each view | Original body minimum |
| --- | ---: | ---: | ---: |
| warthog | 902 | 9 | -1.60000002 |
| banshee | 902 | 1 | -1.17142856 |
| warthog_gauss | 903 | 8 | -1.12000000 |
| spectre_secondary | 903 | 6 | -0.52000004 |
| ghost | 4514 | 1 | -1.31428564 |
| scorpion | 3776 | 2 | -0.84375000 |
| wraith | 4262 | 5 | -0.76875001 |
| spectre | 2588 | 4 | -0.52000004 |

`verified-vehicle-destruction-matrix-v398.json` links all eight actual reports
and unaltered recordings. The runs use different fixture ELFs with the same
unchanged v381 production physics. `tested-runtime-v395` preserves the current
attack fixture executable; `source-delta-v381-to-v395.tar.zst` reproduces its
source from the full v381 archive, including the independently improved replay
diagnostics. The old Ghost short deadline, blocked retreat and heavy wall
approaches are preserved; none of their physical assertions were loosened.
The repeated approach uses the original Rocket Launcher explosion's authored
upper radius plus player height. The Scorpion also exercises ordinary reload.

All eight post-death full world solves and seven full object updates match
unchanged TU1.5 code across 3,382,575 original instructions, all final 60MB source
banks and all captured CPU/FP state, within the previously documented host IF
boundary. `verified-vehicle-destruction-original-v398.json` links those 15
successful complete comparisons. The Banshee object-update replay authenticates
all 48 unchanged instructions of D3D4057F0, then reaches original Xbox kernel
wait import 0x800000EA from2E037F after30,303 instructions. No kernel result is
supplied, so that complete object-update comparison remains unresolved.
`banshee-original-object-kernel-boundary-v396.json` retains this specific limit;
the Banshee's live destruction and complete world-solve comparison both pass.
These cases still do not establish all collisions, physical lifetimes,
controller branches, original trajectories or complete physics parity.
